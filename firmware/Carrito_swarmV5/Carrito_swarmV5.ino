// ---------------------------------------------------------------------------
// Carrito de enjambre · API HTTP
//
// Este ESP32 no sirve la interfaz principal: expone una API JSON que cualquier
// cliente (el front Control Enjambre en la PC, un celular, curl, un script)
// puede usar. Cada carrito es independiente y tiene su propia IP.
//
//   GET /comando?c=A|R|D|I|S[&v=0..255]  -> mueve y responde JSON
//   GET /estado                          -> heartbeat + telemetría JSON
//   GET /  (o cualquier archivo)         -> LittleFS: página de respaldo
//
// Configuración por carrito: config.h (ROBOT_ID) y secrets.h (WiFi).
// ---------------------------------------------------------------------------
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include "Carrito_header.h"
#include "config.h"
#include "secrets.h"

WebServer server(80);

// Estado para el heartbeat y el failsafe
char          ultimoComando      = 'S';
int           ultimaVelocidad    = 0;
bool          movimientoActivo   = false;
unsigned long tUltimoComando     = 0;
unsigned long tUltimoIntentoWifi = 0;

// Toda respuesta lleva CORS: el front se abre desde otro origen (localhost u
// otra IP) y sin esta cabecera el navegador no puede LEER la respuesta, solo
// enviarla a ciegas.
//
// El JSON se arma con snprintf y no concatenando String: a ~6 peticiones por
// segundo la concatenación fragmentaría el heap del ESP32 con el tiempo.
void enviarJson(int codigo, const char* cuerpo) {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(codigo, "application/json", cuerpo);
}

void handleOptions() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "*");
    server.send(204);
}

// Ejecuta los comandos del carrito
void handleComando() {
    String cmd = server.arg("c");
    if (!server.hasArg("c") || cmd.length() == 0) {
        enviarJson(400, "{\"ok\":false,\"error\":\"falta el parametro c\"}");
        return;
    }
    char comando = toupper(cmd[0]);

    // Velocidad opcional: si no viene, cada movimiento usa su valor por defecto.
    bool tieneVel = server.hasArg("v");
    int  vel = tieneVel ? constrain(server.arg("v").toInt(), 0, 255) : 0;

    const char* accion = "";
    bool   mueve  = true;
    int    velUsada = 0;
    char   buf[192];

    switch (comando) {
        case 'A':
            velUsada = tieneVel ? vel : VelocidadMax;
            Mover_adelante(velUsada);   accion = "Adelante";   break;
        case 'R':
            velUsada = tieneVel ? vel : VelocidadMax;
            Mover_retroceder(velUsada); accion = "Retroceder"; break;
        case 'D':
            velUsada = tieneVel ? vel : VelocidadGiro;
            Mover_derecha(velUsada);    accion = "Derecha";    break;
        case 'I':
            velUsada = tieneVel ? vel : VelocidadGiro;
            Mover_izquierda(velUsada);  accion = "Izquierda";  break;
        case 'S':
            Mover_stop();
            accion = "Detenido";   velUsada = 0;  mueve = false;              break;
        default:
            snprintf(buf, sizeof(buf),
                     "{\"ok\":false,\"error\":\"comando desconocido\",\"c\":\"%c\"}", comando);
            enviarJson(400, buf);
            return;
    }

    // Alimenta el failsafe: mientras lleguen comandos, el movimiento sigue vivo.
    ultimoComando    = comando;
    ultimaVelocidad  = velUsada;
    movimientoActivo = mueve;
    tUltimoComando   = millis();

    snprintf(buf, sizeof(buf),
             "{\"ok\":true,\"robot_id\":%d,\"accion\":\"%s\",\"c\":\"%c\",\"v\":%d,\"t\":%lu}",
             ROBOT_ID, accion, comando, velUsada, tUltimoComando);
    enviarJson(200, buf);
}

// Heartbeat + telemetría: el front lo consulta ~2 veces por segundo para saber
// si el carrito está vivo y mostrar RSSI / uptime.
void handleEstado() {
    unsigned long ahora = millis();
    char buf[256];
    snprintf(buf, sizeof(buf),
             "{\"robot_id\":%d,\"fw\":\"%s\",\"ip\":\"%s\",\"rssi\":%d,\"up_ms\":%lu,"
             "\"ultimo\":\"%c\",\"v\":%d,\"movimiento\":%s,\"ms_desde_cmd\":%lu}",
             ROBOT_ID, FW_VERSION, WiFi.localIP().toString().c_str(), (int)WiFi.RSSI(),
             ahora, ultimoComando, ultimaVelocidad,
             movimientoActivo ? "true" : "false", ahora - tUltimoComando);
    enviarJson(200, buf);
}

// Despacha cualquier archivo estático desde LittleFS (página de respaldo)
bool handleFileRead(String path) {
  if (path.endsWith("/")) {
    path += "index.html"; // Si entran a la IP sin nada, asume index.html
  }

  // Determinar el tipo de archivo para decirle al navegador cómo leerlo
  String contentType = "text/plain";
  if (path.endsWith(".html")) contentType = "text/html";
  else if (path.endsWith(".css")) contentType = "text/css";
  else if (path.endsWith(".js")) contentType = "application/javascript";
  else if (path.endsWith(".png")) contentType = "image/png";
  else if (path.endsWith(".jpg")) contentType = "image/jpeg";
  else if (path.endsWith(".ico")) contentType = "image/x-icon";

  // Si el archivo existe en la memoria, envíalo
  if (LittleFS.exists(path)) {
    File file = LittleFS.open(path, "r");
    server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

// Conexión con timeout: si el hotspot no está encendido el carrito arranca
// igual (con los motores detenidos) y sigue reintentando desde loop().
void conectarWifi() {
    String host = String(HOSTNAME_BASE) + String(ROBOT_ID);
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(host.c_str());
    WiFi.setSleep(false);   // menos latencia en el teleop
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.printf("Conectando a Wi-Fi como %s", host.c_str());
    unsigned long inicio = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - inicio < WIFI_CONNECT_TIMEOUT_MS) {
        Serial.print(".");
        delay(500);
    }
    tUltimoIntentoWifi = millis();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n¡Conectado!");
        Serial.print("IP: http://");
        Serial.println(WiFi.localIP());
        Serial.printf("Pega esta IP en el front como R%d\n", ROBOT_ID);
    } else {
        Serial.println("\n[WIFI] Sin conexion. Reintentando en segundo plano.");
    }
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.printf("=== Carrito %d - fw %s ===\n", ROBOT_ID, FW_VERSION);

  Configurar_Motores();
  Mover_stop();
  tUltimoComando = millis();

  if(!LittleFS.begin(true)){
    // Sin LittleFS la API sigue funcionando; solo se pierde la pagina de respaldo.
    Serial.println("Error al montar LittleFS (la API sigue disponible).");
  } else {
    Serial.println("LittleFS montado correctamente.");
  }

  conectarWifi();

  // Rutas de la API
  server.on("/comando", HTTP_GET,     handleComando);
  server.on("/comando", HTTP_OPTIONS, handleOptions);
  server.on("/estado",  HTTP_GET,     handleEstado);
  server.on("/estado",  HTTP_OPTIONS, handleOptions);

  // Si no es de la API, asume que el navegador pide un archivo de LittleFS
  server.onNotFound([]() {
    if (!handleFileRead(server.uri())) {
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(404, "text/plain", "404: Archivo no encontrado");
    }
  });

  server.begin();
  Serial.println("Servidor HTTP iniciado");
}

void loop() {
  server.handleClient();

  // FAILSAFE 1: si se cierra el navegador o se pierde el enlace con el carrito
  // en movimiento, nadie mandara el STOP. Se detiene solo.
  if (movimientoActivo && millis() - tUltimoComando > CMD_TIMEOUT_MS) {
    Serial.printf("[FAILSAFE] %lu ms sin comandos -> STOP\n", millis() - tUltimoComando);
    Mover_stop();
    movimientoActivo = false;
    ultimaVelocidad  = 0;
    ultimoComando    = 'S';
  }

  // FAILSAFE 2: Wi-Fi caido -> parar y reintentar sin bloquear el loop.
  if (WiFi.status() != WL_CONNECTED) {
    if (movimientoActivo) {
      Serial.println("[FAILSAFE] Wi-Fi caido -> STOP");
      Mover_stop();
      movimientoActivo = false;
      ultimaVelocidad  = 0;
      ultimoComando    = 'S';
    }
    if (millis() - tUltimoIntentoWifi > WIFI_RETRY_MS) {
      tUltimoIntentoWifi = millis();
      Serial.println("[WIFI] Reintentando conexion...");
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }
  }
}
