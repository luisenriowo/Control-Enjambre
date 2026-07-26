#ifndef CONFIG_H
#define CONFIG_H

// ---------------------------------------------------------------------------
// Lo único que cambia al flashear otro carrito es ROBOT_ID.
// Cada carrito es un ESP32 independiente con su propia IP; el front guarda
// las 3 IPs y le habla a cada uno por HTTP.
// ---------------------------------------------------------------------------
#define ROBOT_ID   1        // 1, 2 o 3
#define FW_VERSION "V5"

// Nombre de red del carrito: carrito-1, carrito-2, ...
#define HOSTNAME_BASE "carrito-"

// Pines de motores (puente H tipo L298N)
const int MotorA = 13;      // PWM rueda A
const int DirIA1 = 12;
const int DirIA2 = 14;

const int MotorB = 15;      // PWM rueda B
const int DirIB1 = 16;
const int DirIB2 = 17;

// Velocidades PWM (0-255)
const int VelocidadMax  = 255;
const int VelocidadGiro = 200;
const int VelocidadMin  = 100;

// Failsafe: si no llega ningún comando en este tiempo estando en movimiento,
// el carrito se detiene solo. El front reenvía el comando cada ~250 ms
// mientras se mantiene pulsada la cruceta, así que 700 ms deja margen para
// perder 2 paquetes seguidos sin cortar el movimiento.
const unsigned long CMD_TIMEOUT_MS = 700;

// WiFi: no bloquear el arranque para siempre si el hotspot no está encendido.
const unsigned long WIFI_CONNECT_TIMEOUT_MS = 15000;
const unsigned long WIFI_RETRY_MS           = 5000;

#endif
