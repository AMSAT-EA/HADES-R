/*
 *                                                       
 * Project     : GENESIS                                            
 * File        : genesis_util_transmission.h
 *
 * Description : This file includes all the function prototypes
 * Last update : 07 October 2016                                              
 *                                                                            
*/

#ifndef GENESIS_UTIL_TRANSMISSION
#define GENESIS_UTIL_TRANSMISSION

#include <stdio.h>

#include "genesis.h"
#include "ina.h"

#define PACKET_ADDRESS_SATELLITE_ID		0xF
#define BYTE_SIZE                           	8

#define	PACKET_HEADER_SIZE			18 		// 16 training + 2 de sync

#define PACKET_TYPE_POWER_TELEMETRY         	(0x10 | PACKET_ADDRESS_SATELLITE_ID)
#define PACKET_TYPE_TEMP_TELEMETRY        	(0x20 | PACKET_ADDRESS_SATELLITE_ID)
#define PACKET_TYPE_STATUS_TELEMETRY        	(0x30 | PACKET_ADDRESS_SATELLITE_ID)
#define PACKET_TYPE_POWERSTATS_TELEMETRY    	(0x40 | PACKET_ADDRESS_SATELLITE_ID)
#define PACKET_TYPE_TEMPSTATS_TELEMETRY    	(0x50 | PACKET_ADDRESS_SATELLITE_ID)
#define PACKET_TYPE_SUNVECTOR_TELEMETRY    	(0x60 | PACKET_ADDRESS_SATELLITE_ID)
#define PACKET_TYPE_PAYLOAD_ICM_TELEMETRY    	(0x70 | PACKET_ADDRESS_SATELLITE_ID)
#define PACKET_TYPE_DEPLOY_TELEMETRY    	(0x80 | PACKET_ADDRESS_SATELLITE_ID)
#define PACKET_TYPE_EXTENDERPOWER_INE_TELEMETRY (0x90 | PACKET_ADDRESS_SATELLITE_ID)
#define PACKET_TYPE_PAYLOAD_DATA_NEBRIJA 	(0xA0 | PACKET_ADDRESS_SATELLITE_ID)
#define PACKET_TYPE_PAYLOAD_DATA_FRAUNHOFER 	(0xB0 | PACKET_ADDRESS_SATELLITE_ID)
#define PACKET_TYPE_EPHEMERIS   		(0xC0 | PACKET_ADDRESS_SATELLITE_ID)
#define PACKET_TYPE_TIME_SERIES			(0xE0 | PACKET_ADDRESS_SATELLITE_ID)
#define PACKET_TYPE_PAYLOAD_DATA_SMARTIR  	(0xF0 | PACKET_ADDRESS_SATELLITE_ID)

#define PACKET_FREE                         	0x00

#define TELEMETRY_PACKETTYPE_SIZE           	2
#define TELEMETRY_ADDRESS_SIZE              	4
#define MAX_INE 				10

#define PACKET_TRAINING_HEADER 			0xAAAAAAAAAAAAAAAA
#define PACKET_SYNC_HEADER    			0x35BF // sync es 0xBF35 pero se almacena al revés

#define PACKET_TRAINING_HEADER_BYTE		0xAA
#define PACKET_SYNC_HEADER_HI  			(PACKET_SYNC_HEADER >> 8)
#define PACKET_SYNC_HEADER_LO 			(PACKET_SYNC_HEADER & 0x00FF) // sync es 0xBF35 pero se almacena al revés

#define MAX_EPHEMERIS_PACKET_SIZE 		512

extern char* 	tm_ina_buf_ptr;
extern uint16_t tm_ina_buf_len;
extern char* 	tm_bw_buf_ptr;
extern uint16_t tm_bw_buf_len;

#define SMARTIR_PAYLOAD_DATA_SIZE		32
#define NEBRIJA_PAYLOAD_DATA_SIZE		8
#define ICM_PAYLOAD_DATA_SIZE		 	93
#define FRAUNHOFER_PAYLOAD_DATA_SIZE		2
#define TIME_SERIES_DATA_SIZE			30

// structures for the different packet types

// formato de paquete power

typedef struct st_power_packet {

    uint64_t training_1;    			// packet training header 		- 64 bits
    uint64_t training_2;    			// packet training header 		- 64 bits

    uint16_t sync;
    uint8_t  packettype_address;

    uint32_t sclock;
    uint8_t  spa;
    uint8_t  spb;
    uint8_t  spc;
    uint8_t  spd;
    uint16_t spi;
    uint16_t vbusadc_vbatadc_hi;    	// 12 y 4
    uint16_t vbatadc_lo_vcpuadc_hi; 	// 8  y 8
    uint16_t vcpuadc_lo_vbus2; 		// 4  y 12
    uint16_t vbus3_vbat2_hi;		// 12 y 4
    uint16_t vbat2_lo_ibat_hi;		// 8  y 8  -- iepsi2c son 16 bits
    uint16_t ibat_lo_icpu_hi; 		// 8  y 8
    uint16_t icpu_lo_ipl;	 	// 4  y 12
    uint8_t  peaksignal;
    uint8_t  modasignal;
    uint8_t  lastcmdsignal;
    uint8_t  lastcmdnoise;

    uint16_t checksum;

} __attribute__((packed)) power_packet;

// formato de paquete temp

typedef struct st_temp_packet {

    uint64_t training_1;    		// packet training header 		- 64 bits
    uint64_t training_2;    		// packet training header 		- 64 bits

    uint16_t sync;
    uint8_t  packettype_address;

    // se mandan sin signo porque van en incrementos de 0.5C con LUT

    uint32_t sclock;
    uint8_t  tpa;
    uint8_t  tpb;
    uint8_t  tpc;
    uint8_t  tpd;
    uint8_t  tpe;
    uint8_t  teps;
    uint8_t  ttx;
    uint8_t  ttx2;
    uint8_t  trx;
    uint8_t  tcpu;
    
    uint16_t checksum;

} __attribute__((packed)) temp_packet ;

// formato de paquete status

typedef struct st_status_packet {

    uint64_t training_1;    		// packet training header 		- 64 bits
    uint64_t training_2;    		// packet training header 		- 64 bits

    uint16_t sync;
    uint8_t  packettype_address;

    uint32_t sclock;
    uint32_t uptime;
    uint16_t nrun;
    uint8_t  npayload;
    uint8_t  nwire;
    uint8_t  ntransponder;
    uint8_t  npayloaderrors_lastreset;
    uint8_t  bate_mote;

    uint8_t  nTasksNotExecuted;
    uint8_t  antennaDeployed;
    uint8_t  nExtEepromErrors;

    uint8_t  failed_task_id;
    uint8_t  nmes;

    uint8_t  strfwd0;
    uint16_t strfwd1;
    uint16_t strfwd2;
    uint8_t  strfwd3;

    uint16_t checksum;


} __attribute__((packed)) status_packet;

// formato de paquete para estadísticas power

typedef struct st_power_stats_packet {

    uint64_t training_1;    		// packet training header 		- 64 bits
    uint64_t training_2;    		// packet training header 		- 64 bits

    uint16_t sync;
    uint8_t  packettype_address;

    uint32_t sclock;
    uint16_t minvbusadc_vbatadc_hi;
    uint16_t minvbatadc_lo_vcpuadc_hi;
    uint8_t  minvcpuadc_lo_free;
    uint8_t  minvbus2;
    uint8_t  minvbus3;
    uint8_t  minvbat2;
    uint8_t  minibat;
    uint8_t  minicpu;
    uint8_t  minipl;
    uint16_t maxvbusadc_vbatadc_hi;
    uint16_t maxvbatadc_lo_vcpuadc_hi;
    uint8_t  maxvcpuadc_lo_free;
    uint8_t  maxvbus2;
    uint8_t  maxvbus3;
    uint8_t  maxvbat2;
    uint8_t  maxibat;
    uint8_t  maxicpu;
    uint8_t  maxipl;
    uint8_t  ibat_rx_charging;
    uint8_t  ibat_rx_discharging;
    uint8_t  ibat_tx_low_power_charging;
    uint8_t  ibat_tx_low_power_discharging;
    uint8_t  ibat_tx_high_power_charging;
    uint8_t  ibat_tx_high_power_discharging;

    uint16_t checksum;

} __attribute__((packed)) power_stats_packet;

// formato de paquete para estadísticas power

typedef struct st_temp_stats_packet {

    uint64_t training_1;    		// packet training header 		- 64 bits
    uint64_t training_2;    		// packet training header 		- 64 bits

    uint16_t sync;
    uint8_t  packettype_address;

    // es correcto como uint8, se envían con LUT en incrementos de 0.5C

    uint32_t sclock;
    uint8_t  mintpa;
    uint8_t  mintpb;
    uint8_t  mintpc;
    uint8_t  mintpd;
    uint8_t  mintpe;
    uint8_t  minteps;
    uint8_t  minttx;
    uint8_t  minttx2;
    uint8_t  mintrx;
    uint8_t  mintcpu;
    uint8_t  maxtpa;
    uint8_t  maxtpb;
    uint8_t  maxtpc;
    uint8_t  maxtpd;
    uint8_t  maxtpe;
    uint8_t  maxteps;
    uint8_t  maxttx;
    uint8_t  maxttx2;
    uint8_t  maxtrx;
    uint8_t  maxtcpu;

    uint16_t checksum;

} __attribute__((packed)) temp_stats_packet;

typedef struct st_sunvector_packet {

    uint64_t training_1;    		// packet training header 		- 64 bits
    uint64_t training_2;    		// packet training header 		- 64 bits

    uint16_t sync;
    uint8_t  packettype_address;

    char *   raw_data_pointer;
    uint8_t  raw_data_size;

    uint16_t checksum;

} __attribute__((packed)) sunvector_packet;


typedef struct st_deploy_packet {

    uint64_t training_1;    		// packet training header 		- 64 bits
    uint64_t training_2;    		// packet training header 		- 64 bits

    uint16_t sync;
    uint8_t  packettype_address;

    char *   raw_data_pointer;
    uint8_t  raw_data_size;

    uint16_t checksum;

} __attribute__((packed)) deploy_packet;


typedef struct st_sunsensors_packet {

    uint64_t training_1;    	// packet training header 		- 64 bits
    uint64_t training_2;    	// packet training header 		- 64 bits

    uint16_t sync;
    uint8_t  packettype_address;

    uint32_t firstclock;
    uint8_t  vpa1;
    uint8_t  vpa2;
    uint8_t  vpb1;
    uint8_t  vpb2;
    uint8_t  vpc1;
    uint8_t  vpc2;
    uint8_t  vpd1;
    uint8_t  vpd2;
    uint8_t  vpe1;
    uint8_t  vpe2;
    uint8_t  vpf1;
    uint8_t  vpf2;
    uint8_t  vppa1;
    uint8_t  vppa2;
    uint8_t  vppb1;
    uint8_t  vppb2;
    uint8_t  vppc1;
    uint8_t  vppc2;
    uint8_t  vppd1;
    uint8_t  vppd2;
    uint8_t  vppe1;
    uint8_t  vppe2;
    uint8_t  vppf1;
    uint8_t  vppf2;

    uint16_t checksum;

} __attribute__((packed)) sunsensors_packet;


typedef struct st_extendedpower_ine_packet {

   uint64_t training_1;    	// packet training header 		- 64 bits
   uint64_t training_2;    	// packet training header 		- 64 bits

   uint16_t sync;
   uint8_t  packettype_address;

   char *   raw_data_pointer;
   uint8_t  raw_data_size;

   uint16_t checksum;

} __attribute__((packed)) extendedpower_ine_packet;


typedef struct st_icm_payload_data_packet {

    uint64_t training_1;    		// packet training header 		- 64 bits
    uint64_t training_2;    		// packet training header 		- 64 bits

    uint16_t sync;
    uint8_t  packettype_address;

    // momento en que se genera el paquete
    uint32_t clock_tx;

    uint8_t message_number;

    uint8_t data[ICM_PAYLOAD_DATA_SIZE];

    uint16_t checksum;

} __attribute__((packed)) payload_icm_data_packet;


typedef struct st_smartir_payload_data_packet {

    uint64_t training_1;    		// packet training header 		- 64 bits
    uint64_t training_2;    		// packet training header 		- 64 bits

    uint16_t sync;
    uint8_t  packettype_address;

    uint32_t experiment_start_clock;
    uint8_t  experiment_id;
    uint8_t  frame_number;

    uint8_t data[SMARTIR_PAYLOAD_DATA_SIZE];

    uint16_t checksum;

} __attribute__((packed)) smartir_payload_data_packet;


typedef struct st_nebrija_payload_data_packet {

    uint64_t training_1;    		// packet training header 		- 64 bits
    uint64_t training_2;    		// packet training header 		- 64 bits

    uint16_t sync;
    uint8_t  packettype_address;

    uint32_t clock_tx;
    uint8_t  week_number;
    uint8_t  persistent_stored_status;

    uint8_t data[NEBRIJA_PAYLOAD_DATA_SIZE];

    uint16_t checksum;

} __attribute__((packed)) nebrija_payload_data_packet;


typedef struct st_fraunhofer_payload_data_packet {

    uint64_t training_1;    		// packet training header 		- 64 bits
    uint64_t training_2;    		// packet training header 		- 64 bits

    uint16_t sync;
    uint8_t  packettype_address;

    uint32_t clock_tx;
    uint8_t data[FRAUNHOFER_PAYLOAD_DATA_SIZE];

    uint16_t checksum;

} __attribute__((packed)) fraunhofer_payload_data_packet;

typedef struct st_time_series_packet {

    uint64_t training_1;    		// packet training header 		- 64 bits
    uint64_t training_2;    		// packet training header 		- 64 bits

    uint16_t sync;
    uint8_t  packettype_address;

    uint32_t clock;
    uint8_t  variable;

    uint8_t data[TIME_SERIES_DATA_SIZE];

    uint16_t checksum;

} __attribute__((packed)) time_series_packet;


#endif
