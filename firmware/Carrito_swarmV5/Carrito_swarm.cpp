#include "Carrito_header.h"

// Pines y velocidades viven ahora en config.h (incluido vía Carrito_header.h),
// para que flashear otro carrito sea cambiar un solo archivo.
//
// Nota: analogWrite() requiere core Arduino-ESP32 >= 2.0. Si usas el core 1.0.x
// no existe; en ese caso reemplaza pwm() por ledcSetup/ledcAttachPin/ledcWrite.

static void pwm(int pin, int vel) {
    analogWrite(pin, constrain(vel, 0, 255));
}

void Configurar_Motores() {
    Serial.println("[SISTEMA] Configurando pines de los motores...");
    pinMode(MotorA, OUTPUT);
    pinMode(DirIA1, OUTPUT);
    pinMode(DirIA2, OUTPUT);

    pinMode(MotorB, OUTPUT);
    pinMode(DirIB1, OUTPUT);
    pinMode(DirIB2, OUTPUT);
}

void Mover_adelante(int vel) {
    Serial.printf("[ACCIÓN] ADELANTE (v=%d)\n", vel);
    digitalWrite(DirIA1, HIGH);
    digitalWrite(DirIA2, LOW);
    digitalWrite(DirIB1, HIGH);
    digitalWrite(DirIB2, LOW);
    pwm(MotorA, vel);
    pwm(MotorB, vel);
}

void Mover_retroceder(int vel) {
    Serial.printf("[ACCIÓN] RETROCEDER (v=%d)\n", vel);
    digitalWrite(DirIA1, LOW);
    digitalWrite(DirIA2, HIGH);
    digitalWrite(DirIB1, LOW);
    digitalWrite(DirIB2, HIGH);
    pwm(MotorA, vel);
    pwm(MotorB, vel);
}

void Mover_derecha(int vel) {
    Serial.printf("[ACCIÓN] DERECHA (v=%d)\n", vel);
    digitalWrite(DirIA1, HIGH);
    digitalWrite(DirIA2, LOW);
    digitalWrite(DirIB1, LOW);
    digitalWrite(DirIB2, HIGH);
    pwm(MotorA, vel);
    pwm(MotorB, vel);
}

void Mover_izquierda(int vel) {
    Serial.printf("[ACCIÓN] IZQUIERDA (v=%d)\n", vel);
    digitalWrite(DirIA1, LOW);
    digitalWrite(DirIA2, HIGH);
    digitalWrite(DirIB1, HIGH);
    digitalWrite(DirIB2, LOW);
    pwm(MotorA, vel);
    pwm(MotorB, vel);
}

void Mover_stop() {
    Serial.println("[ACCIÓN] STOP");
    digitalWrite(DirIA1, LOW);
    digitalWrite(DirIA2, LOW);
    digitalWrite(DirIB1, LOW);
    digitalWrite(DirIB2, LOW);
    pwm(MotorA, 0);
    pwm(MotorB, 0);
}
