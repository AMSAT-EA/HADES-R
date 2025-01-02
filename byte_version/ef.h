#ifndef GENERAL_EFEMERIDES_H
#define GENERAL_EFEMERIDES_H

#define PACKEF __attribute__((packed, scalar_storage_order("big-endian")))

#include "db.h"
#include "ef_prop.h"

/**
 * @struct _sat
 * @brief Estructura con información del satélite que contiene parámetros y valores del TLE que permiten propagar su posición.
 */
typedef struct {
	uint16_t adr;   
	uint32_t ful;   
	uint32_t fdl;  
	tle_t tle; ///< estructura /ref tle_t
} PACKEF _sat;

/**
 * @struct _frm
 * @brief Paquete de efemérides con bits necesarios para la identificación del tipo de paquete, la información del satélite y últimos valores de latitud, longitud y altitud calculados
 */
typedef struct {
	uint64_t train1; ///< secuencia de entrenamiento (0xAAAAAAAA)
	uint64_t train2; ///< secuencia de entrenamiento (0xAAAAAAAA)
	uint16_t sync;   ///< secuencia de sincronia     (0xBF35)
	uint8_t len;     ///< byte de longitud/type      
	uint32_t utc;    ///< hora utc
	_sat sat;   ///< estructura /ref _sat
	int16_t lat;    ///< latitud
	int16_t lon;    ///< longitd
	uint16_t alt;    ///< altitud
	uint8_t cnt;     ///< contador, solo hub lo incrementa
	uint16_t crc;    ///< checksum, comprueba integridad de los datos
}
PACKEF _frm;


#endif
