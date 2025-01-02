#ifndef DB_H
#define DB_H

#include <stdio.h>

/**
 * @struct tle_t
 * @brief  Estructura de datos TLE (Two-Line Element) 
 * Todos los parámetros de la estructura han sido preprocesados anteriormente desde el formato TLE para tomar únicamente los datos que necesita el propagador SGP4
 */
typedef struct { uint32_t epoch; float xndt2o, xndd6o, bstar, xincl, xnodeo, eo, omegao, xmo, xno; } tle_t;

#define SIZE_ZIC 9 ///< Tamaño de la tabla \ref zic
/**
 * @struct _zic
 * @brief Estructura de zonas de interés comercial
 */
struct _zic{
  int lat;   ///< Latitud del centro
  int lon;   ///< Longitud del centro
  int radio; ///< Radio del área
  unsigned char flag; ///< Etiqueta (0 = no transmisión; 1 = alta potencia)
};

//PUBLIC
extern struct _zic   zic[SIZE_ZIC]; 
extern int           first;
extern int           ptx;

#endif /*DB_H*/
