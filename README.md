# Control Enjambre

Interfaz de operador para un enjambre de carritos ESP32, más el firmware de los carritos.

```
Control Enjambre.dc.html   interfaz (prototipo DC/React) — corre en el navegador
support.js                 runtime del prototipo
firmware/Carrito_swarmV5/  firmware del carrito (Arduino / ESP32)
```

## Arquitectura

Cada carrito es **un ESP32 independiente con su propia IP** que expone una API HTTP.
El ESP32 no sirve la interfaz principal: es solo API. El navegador le habla directo a
cada carrito. No hay gateway ni ESP-NOW.

```
        ┌──────────── HTTP/JSON ───────────▶ ESP32 R1  (10.x.x.a)
navegador ──────────── HTTP/JSON ───────────▶ ESP32 R2  (10.x.x.b)
        └──────────── HTTP/JSON ───────────▶ ESP32 R3  (10.x.x.c)
```

Todos (PC/celular y carritos) tienen que estar en la misma red Wi-Fi.

Se eligió API-primero para poder tener varias interfaces contra el mismo contrato:
el prototipo DC, un celular, `curl`, un script de pruebas o una futura UI vanilla —
sin reflashear los carritos y sin mantener 3 copias de la interfaz.

## API del carrito

### `GET /comando?c=<letra>[&v=<0-255>]`

| `c` | acción     |
|-----|------------|
| `A` | adelante   |
| `R` | retroceder |
| `D` | derecha    |
| `I` | izquierda  |
| `S` | stop       |

```json
{"ok":true,"robot_id":1,"accion":"Adelante","c":"A","v":255,"t":91234}
```

La interfaz habla en las mismas direcciones que el firmware — no hay traducción de
velocidad lineal/angular por medio.

**La velocidad no se controla desde ninguna interfaz.** La fijan las constantes de
[config.h](firmware/Carrito_swarmV5/config.h) (`VelocidadMax` 255 en recto,
`VelocidadGiro` 200 en giro) y se cambian recompilando. `/comando` acepta un `v`
opcional para pruebas con `curl`, pero ni el front ni la página de respaldo lo envían:
así el PWM vive en un solo sitio.

### `GET /estado`

Heartbeat y telemetría. El front lo sondea cada 500 ms por carrito.

```json
{"robot_id":1,"fw":"V5","ip":"10.136.8.207","rssi":-52,"up_ms":91234,
 "ultimo":"A","v":255,"movimiento":true,"ms_desde_cmd":140}
```

Todas las respuestas llevan `Access-Control-Allow-Origin: *`, para que el front
pueda leerlas desde otro origen.

### Failsafe (en el firmware)

- **700 ms sin recibir comandos** con el carrito en movimiento → STOP automático.
- **Wi-Fi caído** → STOP automático y reintento de conexión en segundo plano.

Por eso el cliente debe **reenviar el comando cada ~250 ms** mientras se mantenga
pulsada la cruceta. El front y la página de respaldo ya lo hacen.

## Flashear un carrito

1. Copia la plantilla de credenciales y edítala:
   ```
   cp firmware/Carrito_swarmV5/secrets.h.example firmware/Carrito_swarmV5/secrets.h
   ```
   `secrets.h` está en `.gitignore`: nunca se sube al repo.
2. En `firmware/Carrito_swarmV5/config.h`, pon el `ROBOT_ID` de este carrito (1, 2 o 3).
   Ahí también están los pines de los motores y las velocidades.
3. Abre `firmware/Carrito_swarmV5/Carrito_swarmV5.ino` en el Arduino IDE
   (placa: *ESP32 Dev Module*) y sube. O con `arduino-cli`:
   ```
   arduino-cli compile --fqbn esp32:esp32:esp32 firmware/Carrito_swarmV5
   arduino-cli upload  --fqbn esp32:esp32:esp32 -p COM3 firmware/Carrito_swarmV5
   ```
4. Abre el monitor serial a **115200**: imprime la IP del carrito.
5. (Opcional) Sube `firmware/Carrito_swarmV5/data/` a LittleFS para tener la página
   de control de respaldo en `http://<ip>/`.

Requiere core **Arduino-ESP32 ≥ 2.0** (por `analogWrite`).

## Usar la interfaz

Sírvela por HTTP; con `file://` fallan las cargas del CDN con SRI del runtime DC:

```
python -m http.server 8000
```

y abre <http://localhost:8000/Control%20Enjambre.dc.html>.

Pega la IP de cada carrito en su tarjeta (columna izquierda de **TELEOP**). Las IPs se
guardan en `localStorage`, así que solo se escriben una vez. Una IP vacía = `NO CONFIG`
y ese robot no se sondea.

> El runtime `support.js` descarga React y Babel desde `unpkg.com`, así que la PC que
> abre la interfaz necesita internet. Los carritos no.

### Controles

- **Cruceta o W/A/S/D**: teleop del robot activo. Hay que mantener pulsado.
  Si se pulsan dos direcciones a la vez, las opuestas se anulan y avanzar gana a girar
  (el firmware acepta una sola letra por comando).
- **PARO DE EMERGENCIA**: manda STOP a *todos* los carritos configurados, con reintento.
- **LOG DE COMANDOS**: TX de cada envío, RX con el ack real del carrito, exportable a `.txt`.
- **ESQUEMA**: documenta el contrato y el mapa de componentes de la GUI.

### Añadir el carrito 2 y 3

1. `ROBOT_ID = 2` en `config.h` → flashear el segundo ESP32.
2. Leer su IP por serial.
3. Pegarla en la tarjeta **R2** del front. Listo, ya se sondea y se puede pilotar.

## Estado actual

| Función | Estado |
|---|---|
| Teleop (cruceta / WASD) | funcional |
| Heartbeat y telemetría por robot | funcional |
| Paro de emergencia a todos | funcional |
| Failsafe por timeout y por Wi-Fi | funcional |
| **Coreografía** (primitivas, jitter, `t_start`) | **maqueta** |

La pestaña COREOGRAFÍA tiene la UI completa pero **no mueve los carritos**: el firmware
todavía no ejecuta primitivas. START solo hace la cuenta atrás y lo registra en el log.
STOP y el paro de emergencia dentro de esa pestaña sí son reales.
