/**
 * @file   db.h
 * @author Celia
 * @date   2024/04/22
 * @brief  Funciones y estructuras para la gestión de las bases de datos
 *
 */
#ifndef DB_H
#define DB_H
#include <stdio.h>

/**
 * @struct tle_t
 * @brief  Estructura de datos TLE (Two-Line Element) 
 * Todos los parámetros de la estructura han sido preprocesados anteriormente desde el formato TLE para tomar únicamente los datos que necesita el propagador SGP4
 */
typedef struct { uint32_t epoch; float xndt2o, xndd6o, bstar, xincl, xnodeo, eo, omegao, xmo, xno; } tle_t;


#endif /*DB_H*/
