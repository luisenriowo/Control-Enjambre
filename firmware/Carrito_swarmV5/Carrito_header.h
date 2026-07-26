#ifndef CARRITO_H
#define CARRITO_H

#include <Arduino.h>
#include "config.h"

// Funciones de movimiento.
// La velocidad es opcional: sin argumento se usa la de siempre, así que las
// llamadas antiguas (Mover_adelante();) siguen compilando igual.
void Configurar_Motores();
void Mover_adelante(int vel = VelocidadMax);
void Mover_retroceder(int vel = VelocidadMax);
void Mover_derecha(int vel = VelocidadGiro);
void Mover_izquierda(int vel = VelocidadGiro);
void Mover_stop();

#endif
