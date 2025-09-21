// touch_config.h - Configuración de touch para ILI9488 + XPT2046
#pragma once

#include <XPT2046_Touchscreen.h>

// Pines
#define TOUCH_CS   15
#define TOUCH_IRQ  13

// Instancia global del touch
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

// Función para leer touch convertido
bool getTouch(uint16_t &x, uint16_t &y) {
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    
    // INVERSIÓN COMPLETA DE EJES
    x = map(p.x, 3930, 372, 0, 480);   // X INVERTIDO: max→min
    y = map(p.y, 3760, 237, 0, 320);   // Y INVERTIDO: max→min
    
    return true;
  }
  return false;
}
