/*************************************************************************************/
/*                                                                                   */
/* License information / Informacion de licencia                                     */
/*                                                                                   */
/* Todos los contenidos de la web de AMSAT EA se distribuyen                         */
/* bajo licencia Creative Commons CC BY 4.0 Internacional                            */
/* (distribución, modificacion u y uso libre mientras se cite AMSAT EA como fuente). */
/*                                                                                   */
/* All the contents of the AMSAT EA website are distributed                          */
/*  under a Creative Commons CC BY 4.0 International license                         */
/* (free distribution, modification and use crediting AMSAT EA as the source).       */
/*                                                                                   */
/* AMSAT EA 2025                                                                     */
/* https://www.amsat-ea.org/proyectos/                                               */
/*                                                                                   */
/*************************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <stdio.h>

#define nzones 34
#define pi 3.1415926535898

#include "genesis_crc.h"
#include "genesis_scrambler.h"
#include "genesis_util_transmission.h"
#include "sun.h"
#include "ef.h"

#define SAT_ID_HADES_ICM 0x2
#define SAT_ID_GENESIS_M 0x9
#define SAT_ID_MARIA_G	 0xB
#define SAT_ID_UNNE_1	 0xC
#define SAT_ID_HADES_R   0xD

#define RX_BUFFER_SIZE 	 2048
#define SYNCHRONIZED     0xBF35

#define TEMP_BUFFER_SIZE 1024

const char name[MAXINA][5]={"SPA ","SPB ","SPC ","SPD ","SUN ","BAT ","BATP","BATN","CPU ","PL  ","SIM"};

void procesar(char * file_name);
void trx(double lat, double lon, int latc, int lonc, int rc, int *inout);
char* overflying(double lat, double lon);
void visualiza_efemeridespacket(uint8_t sat_id,_frm * ef);
void visualiza_icm_data_packet(uint8_t sat_id, payload_icm_data_packet * packet);
char * source_desc(uint8_t sat_id);

FILE *f;
time_t start_t;

int main(int argc, char * argv[]) {

	time_t t = time(NULL);
  	struct tm tm = *localtime(&t);
	char fecha[64];

        printf("\n");
        printf("***************************************************\n");
        printf("*                                                 *\n");
        printf("*       Unified Satellite Telemetry Decoder       *\n");
        printf("*          AMSAT EA - Free distribution           *\n");
        printf("*              Version 1.11 (Bytes)               *\n");
        printf("*            Compilation : %10s            *\n",__DATE__);
        printf("*                                                 *\n");
        printf("***************************************************\n");


        printf("\n");
        printf("This software is able to decodify already unscrambled and CRC checked byte telemetry sequences from:\n");
        printf("HYDRA-W (1), HADES-ICM (2), GENESIS-M (9), HYDRA-T (A), MARIA-G (B), UNNE-1 (C) and HADES-R (D)\n\n");

        printf("Intended for its use with Andy UZ7HO's Soundmodem Windows demodulator software\n");
        printf("\n");


        if (argc != 2) {

                printf("Please specify the file to decodify. File must contain bytes in text separated by spaces\n");
                printf("Sample: 27 FE 52 4F 53 FF FF 40 46 40 4E\n");
                printf("\n");

                return 1;

        }

        sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d :", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

	printf("%s Running...\n", fecha);

	procesar(argv[1]);

	return 0;

}


int16_t hex2int(char c) {

    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;

    return -1;
}


// devuelve el tamaño, incluyendo checksum para el tipo de paquete (bytes utiles de Excel) (todo menos training y sync)

uint8_t telemetry_packet_size(uint8_t satellite_id, uint8_t tipo_paquete) {

        uint8_t bytes_utiles[] = { 12, 31, 17, 29, 35, 27, 135, 45, 31, 123, 17, 9, 64, 47, 38, 41 };

        /*

        0 - command
        1 - power
        2 - temps
        3 - status
        4 - power stats
        5 - temp stats
        6 - sunvector
        7 - tera payload
        8 - antenna deploy
        9 - ine
       10 - nebrija
       11 - mariag (fraunhofer)
       12 - efemerides
       13 - uc3m
       14 - time series
       15 - smart ir

        */

        if (tipo_paquete > sizeof(bytes_utiles)-1) return 1;

        // excepcion HADES-ICM que comparte paquete 7 con HYDRA-W

        if (satellite_id == 2 && tipo_paquete == 7) return 101;

        return bytes_utiles[tipo_paquete]; // utiles

}


void visualiza_powerpacket(uint8_t sat_id, power_packet * packet) {

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char fecha[64];

    sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  
    printf("*** Power packet received on local time %s ***\n", fecha);

    uint16_t days = 0, hours = 0, minutes = 0, seconds = 0;

    days      = (packet->sclock / 86400); // segundos que dan para dias enteros
    hours     = (packet->sclock % 86400)/3600; // todo lo que no de para un dia dividido entre 3600 seg que son una hora
    minutes   = (packet->sclock % 3600)/60; // todo lo que no de para una hora dividido entre 60
    seconds   = (packet->sclock % 60);

    printf("sat_id          : %d (%s)\n", sat_id, source_desc(sat_id));
    printf("sclock          : %d seconds (%d days and %02d:%02d:%02d hh:mm:ss)\n",packet->sclock, days, hours, minutes, seconds);
    printf("spi             : %4d mW (total instant power)\n", packet->spi << 1);
    printf("spa		: %4d mW (last 3 mins peak)\n", packet->spa << 1);
    printf("spb             : %4d mW (last 3 mins peak)\n", packet->spb << 1);
    printf("spc             : %4d mW (last 3 mins peak)\n", packet->spc << 1);
    printf("spd             : %4d mW (last 3 mins peak)\n\n", packet->spd << 1);

    // 32 bits o desborda con las multiplicacion
    
    uint32_t vbus1 = (packet->vbusadc_vbatadc_hi >> 4);
             vbus1 = vbus1 * 1400;
	     vbus1 = vbus1 / 1000;

    uint16_t vbus2 = (packet->vcpuadc_lo_vbus2 & 0x0fff);
             vbus2 = vbus2 * 4;

    uint16_t vbus3 = (packet->vbus3_vbat2_hi >> 4);
    	     vbus3 = vbus3 * 4;

    printf("vbus1		: %4d mV bus voltage read in CPU.ADC\n"  , vbus1);  
    printf("vbus2       	: %4d mV bus voltage read in EPS.I2C\n"  , vbus2);
    printf("vbus3       	: %4d mV bus voltage read in CPU.I2C\n\n", vbus3); 

    uint32_t vbat1 = (packet->vbusadc_vbatadc_hi << 8) & 0x0f00;
	     vbat1 = vbat1 | ((packet->vbatadc_lo_vcpuadc_hi >> 8) & 0x00ff);
    	     vbat1 = vbat1 * 1400;
	     vbat1 = vbat1 / 1000;

    printf("vbat1		: %4d mV bat voltage read in EPS.ADC\n"  ,vbat1); 

    uint16_t vbat2 = (packet->vbus3_vbat2_hi << 8) & 0x0f00;
             vbat2 = (vbat2 | (packet->vbat2_lo_ibat_hi >> 8));
             vbat2 = vbat2 *4;  

    printf("vbat2		: %4d mV bat voltage read in EPS.I2C\n\n", vbat2);

    uint16_t ibat = packet->vbat2_lo_ibat_hi  << 8;
             ibat = ibat | (packet->ibat_lo_icpu_hi >> 8);

    if (ibat & 0x0800) ibat = ibat |0x0f000;

    int16_t ibats = (int16_t) ibat;

    float ii;
    printf("vbus1-vbat1	: %4d mV \n", vbus1-vbat1);
    ii =vbus1;
    ii-=vbat1; 
    ii/=0.310;
    printf("vbus3-vbus2	: %4d mV \n\n",vbus3-vbus2);
    ii =vbus3;
    ii-=vbus2; 
    ii/=0.280;
    
    uint32_t vcpu_temp = (packet->vbatadc_lo_vcpuadc_hi << 4) & 0x0ff0;
             vcpu_temp = vcpu_temp | (packet->vcpuadc_lo_vbus2 >> 12);

    uint32_t vcpu = 1210*4096/vcpu_temp;

    uint16_t icpu = (packet->ibat_lo_icpu_hi << 4) & 0x0ff0;
             icpu = icpu | (packet->icpu_lo_ipl >> 12);

    // la cpu sale siempre negativa, chip al reves, multiplicamos por -1
    int16_t icpus = (int16_t) icpu;

    // si bit 12 esta a 1 ponemos el resto tambien para signo ok
    if (icpus & 0x800) icpus = (icpus | 0xf000)* -1; 

    uint16_t ipl = (packet->icpu_lo_ipl & 0x0fff);

    int16_t ipls = (int16_t) ipl;
    
    if (ipls & 0x0800) ipls = (ipls | 0xf000);

    printf("vcpu		: %4d mV\n\n", vcpu); 
    printf("icpu		: %4d mA @DCDCinput\n",  icpus);

    int32_t iii;
    iii =(icpus*vbus3);
    iii/=vcpu;
    printf("icpu		: %4d mA @DCDCoutput (estimation)\n", iii);

    printf("ipl		: %4d mA (Last payload current)\n",   ipls);
    if (ibats == 0) printf("ibat		: %4d mA\n\n", ibats);
    else
    if (ibats > 0) printf("ibat		: %4d mA (Current flowing out from the battery)\n\n", ibats); // 8 y 8
		else printf("ibat 		: %4d mA (Current flowing into the battery)\n\n", ibats);

    printf("peaksignal	: %4d dB\n",  packet->peaksignal);
    printf("modasignal	: %4d dB\n",  packet->modasignal);
    printf("lastcmdsignal	: %4d dB\n",  packet->lastcmdsignal);
    printf("lastcmdnoise	: %4d dB\n",  packet->lastcmdnoise);


}


void visualiza_temppacket(uint8_t sat_id, temp_packet * packet) {

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char fecha[64];

    sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    printf("*** Temp packet received on local time %s ***\n", fecha);

    uint16_t days = 0, hours = 0, minutes = 0, seconds = 0;

    days      = (packet->sclock / 86400); // segundos que dan para dias enteros
    hours     = (packet->sclock % 86400)/3600; // todo lo que no de para un dia dividido entre 3600 seg que son una hora
    minutes   = (packet->sclock % 3600)/60; // todo lo que no de para una hora dividido entre 60
    seconds   = (packet->sclock % 60);

    printf("sat_id : %d (%s)\n", sat_id, source_desc(sat_id));
    printf("sclock : %d seconds (%d days and %02d:%02d:%02d hh:mm:ss)\n",packet->sclock, days, hours, minutes, seconds);

    if (packet->tpa   == 255) printf("tpa    : \n"); else printf("tpa    : %+5.1f degC temperature in SPA.I2C\n", ((float)packet->tpa/2)-40.0);
    if (packet->tpb   == 255) printf("tpb    : \n"); else printf("tpb    : %+5.1f degC temperature in SPB.I2C\n", ((float)packet->tpb/2)-40.0);
    if (packet->tpc   == 255) printf("tpc    : \n"); else printf("tpc    : %+5.1f degC temperature in SPC.I2C\n", ((float)packet->tpc/2)-40.0);
    if (packet->tpd   == 255) printf("tpd    : \n"); else printf("tpd    : %+5.1f degC temperature in SPD.I2C\n", ((float)packet->tpd/2)-40.0);
    if (packet->teps  == 255) printf("teps   : \n"); else printf("teps   : %+5.1f degC temperature in EPS.I2C\n", ((float)packet->teps/2)-40.0);
    if (packet->ttx   == 255) printf("ttx    : \n"); else printf("ttx    : %+5.1f degC temperature in  TX.I2C\n", ((float)packet->ttx/2)-40.0);
    if (packet->ttx2  == 255) printf("ttx2   : \n"); else printf("ttx2   : %+5.1f degC temperature in  TX.NTC\n", ((float)packet->ttx2/2)-40.0);
    if (packet->trx   == 255) printf("trx    : \n"); else printf("trx    : %+5.1f degC temperature in  RX.NTC\n", ((float)packet->trx/2)-40.0);
    if (packet->tcpu  == 255) printf("tcpu   : \n"); else printf("tcpu   : %+5.1f degC temperature in CPU.ADC\n", ((float)packet->tcpu/2)-40.0);


}


typedef enum reset_cause_e
    {
        RESET_CAUSE_UNKNOWN = 0,
        RESET_CAUSE_LOW_POWER_RESET,
        RESET_CAUSE_WINDOW_WATCHDOG_RESET,
        RESET_CAUSE_INDEPENDENT_WATCHDOG_RESET,
        RESET_CAUSE_SOFTWARE_RESET,
        RESET_CAUSE_POWER_ON_POWER_DOWN_RESET,
        RESET_CAUSE_EXTERNAL_RESET_PIN_RESET,
        RESET_CAUSE_BROWNOUT_RESET,
		
} reset_cause_t;


const char * reset_cause_get_name(reset_cause_t reset_cause) {

        const char * reset_cause_name = "TBD";

        switch (reset_cause)
        {
            case RESET_CAUSE_UNKNOWN:
                reset_cause_name = "Unknown";
                break;
            case RESET_CAUSE_LOW_POWER_RESET:
                reset_cause_name = "Low Power Reset";
                break;
            case RESET_CAUSE_WINDOW_WATCHDOG_RESET:
                reset_cause_name = "Window Watchdog reset";
                break;
            case RESET_CAUSE_INDEPENDENT_WATCHDOG_RESET:
                reset_cause_name = "Independent watchdog reset";
                break;
            case RESET_CAUSE_SOFTWARE_RESET:
                reset_cause_name = "Software reset";
                break;
            case RESET_CAUSE_POWER_ON_POWER_DOWN_RESET:
                reset_cause_name = "Power-on reset (POR) / Power-down reset (PDR)";
                break;
            case RESET_CAUSE_EXTERNAL_RESET_PIN_RESET:
                reset_cause_name = "External reset pin reset";
                break;
            case RESET_CAUSE_BROWNOUT_RESET:
                reset_cause_name = "Brownout reset (BOR)";
                break;
        }

        return reset_cause_name;
}


const char * battery_status(uint8_t status) {


   switch(status) {

	case 0:
		return "Fully charged (4200 mV)";
	case 1:
		return "Charged (Between 3550 mV and 4200 mV)";
	case 2:
		return "Half charged (Between 3300 mV and 3550 mV) - Limited transmissions";
	case 3:
		return "Low charge (Between 3200 mV and 3300 mV) - Limited transmissions, antenna deploying not allowed";
	case 4:
                return "Very Low charge (Between 2500 mV and 3200 mV) - Limited transmissions, antenna deploying not allowed";
	case 5:
                return "Battery damaged (Below 2500 mV) - Battery disconnected";


	default:
		return "Unknown";

   }
	

}


const char * transponder_mode(uint8_t mode) {

        switch(mode) {


                case 0:
                        return "Disabled";
                case 1:
                        return "Enabled in FM mode";
                case 2:
                        return "Enabled in FSK regenerative mode";
                default:
                        return "Unknown";

        }

}


void visualiza_statuspacket(uint8_t sat_id, status_packet * packet) {

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char fecha[64];

    sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    printf("*** Status packet received on local time %s ***\n", fecha);

    uint16_t days = 0, hours = 0, minutes = 0, seconds = 0;
    uint16_t daysup = 0, hoursup = 0, minutesup = 0, secondsup = 0;

    days      = (packet->sclock / 86400); // segundos que dan para dias enteros
    hours     = (packet->sclock % 86400)/3600; // todo lo que no de para un dia dividido entre 3600 seg que son una hora
    minutes   = (packet->sclock % 3600)/60; // todo lo que no de para una hora dividido entre 60
    seconds   = (packet->sclock % 60);

    uint32_t segundos_up  = packet->uptime;

    daysup    = (segundos_up / 86400);
    hoursup   = (segundos_up % 86400)/3600;
    minutesup = (segundos_up % 3600)/60;
    secondsup = (segundos_up % 60);

    printf("sat_id              : %10d (%s)\n", sat_id, source_desc(sat_id));
    printf("sclock              : %10d seconds satellite has been active (%d days and %02d:%02d:%02d hh:mm:ss)\n",packet->sclock, days, hours, minutes, seconds);
    printf("uptime              : %10d seconds since the last CPU reset  (%d days and %02d:%02d:%02d hh:mm:ss)\n",packet->uptime, daysup, hoursup, minutesup, secondsup);
    printf("nrun                : %10d times satellite CPU was started\n",packet->nrun);
    printf("npayload            : %10d times payload was activated\n",packet->npayload);
    printf("nwire               : %10d times antenna deployment was tried\n", packet->nwire);
    printf("ntransponder        : %10d times transponder was activated\n", packet->ntransponder); 

    uint8_t payloadFails = packet->npayloaderrors_lastreset >> 4;

    if (payloadFails == 0)
    printf("nPayloadsFails 	    :  	       OK\n"); else
    printf("nPayloadFails       : %10d\n", packet->npayloaderrors_lastreset >> 4);
    printf("last_reset_cause    : %10d %s\n",packet->npayloaderrors_lastreset & 0x0F, reset_cause_get_name(packet->npayloaderrors_lastreset & 0x0F));
    printf("bate (battery)      : %10X %s\n",packet->bate_mote >> 4, battery_status(packet->bate_mote >> 4));
    printf("mote (transponder)  : %10X %s\n",packet->bate_mote & 0x0f, transponder_mode(packet->bate_mote & 0x0f));

    if (packet->nTasksNotExecuted == 0)
    printf("nTasksNotExecuted   :          OK\n");
                else
    printf("nTasksNotExecuted   : %10d Some tasks missed their execution time\n",packet->nTasksNotExecuted);

    if (packet->nExtEepromErrors == 0)
    printf("nExtEepromErrors    :          OK\n");
                else
    printf("nExtEepromErrors    : %10d EEPROM Fail\n",packet->nExtEepromErrors);

    uint8_t ANTENNA_NOT_DEPLOYED = 0;
    uint8_t ANTENNA_DEPLOYED     = 1;

    if (sat_id == 2 || sat_id == 13) {

	ANTENNA_NOT_DEPLOYED = 1; // en HADES-ICM va al reves
	ANTENNA_DEPLOYED     = 0;

    }

    if (packet->antennaDeployed == ANTENNA_NOT_DEPLOYED)
    printf("antennaDeployed     :          KO (Antenna not yet deployed)\n");
		else
    	if (packet->antennaDeployed == 2)
    		printf("antennaDeployed     :          UNKNOWN\n");
    	else if (packet->antennaDeployed == ANTENNA_DEPLOYED)
    printf("antennaDeployed     :          OK (Antenna has been deployed)\n");

    if (packet->nTasksNotExecuted == 0 && packet->failed_task_id == 0) printf("last_failed_task_id : \n");
    	else if (packet->nTasksNotExecuted == 0 && packet->failed_task_id == 255) printf("last_failed_task_id :          Power amplifier disabled or does not respond\n");
    		else printf("last_failed_task_id :          Q%dT%d\n", packet->failed_task_id >> 6, packet->failed_task_id & 0b00111111);

    if (packet->nmes == 255) printf("messaging enabled   :          No\n");
    	else printf("messaging enabled   :          Yes (%d messages stored)\n", packet->nmes);

    printf("strfwd0 (id)        : %10X (%d)\n",packet->strfwd0, packet->strfwd0);
    printf("strfwd1 (key)       : %10X (%d)\n",packet->strfwd1, packet->strfwd1);
    printf("strfwd2 (value)     : %10X (%d)\n",packet->strfwd2, packet->strfwd2);
    printf("strfwd3 (num_tcmds) : %10X (%d)\n",packet->strfwd3, packet->strfwd3);


}


void visualiza_powerstatspacket(uint8_t sat_id, power_stats_packet * packet) {

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char fecha[64];

    sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    printf("*** Power stats packet received on local time %s ***\n", fecha);

    int8_t minicpus;
    int8_t maxicpus;
	
    maxicpus = packet->maxicpu;
    minicpus = packet->minicpu;

    uint16_t days = 0, hours = 0, minutes = 0, seconds = 0;

    days      = (packet->sclock / 86400); // segundos que dan para dias enteros
    hours     = (packet->sclock % 86400)/3600; // todo lo que no de para un dia dividido entre 3600 seg que son una hora
    minutes   = (packet->sclock % 3600)/60; // todo lo que no de para una hora dividido entre 60
    seconds   = (packet->sclock % 60);

    uint32_t minvcpu_temp = (packet->minvbatadc_lo_vcpuadc_hi << 4) & 0x0ff0;
             minvcpu_temp = minvcpu_temp | (packet->minvcpuadc_lo_free >> 4);

    uint32_t maxvcpu_temp = (packet->maxvbatadc_lo_vcpuadc_hi << 4) & 0x0ff0;
             maxvcpu_temp = maxvcpu_temp | (packet->maxvcpuadc_lo_free >> 4);

    printf("sat_id         : %d (%s)\n", sat_id, source_desc(sat_id));

    printf("sclock         : %d seconds (%d days and %02d:%02d:%02d hh:mm:ss)\n",packet->sclock, days, hours, minutes, seconds);

    printf("minvbus1       : %4d mV ADC\n", 1400*(packet->minvbusadc_vbatadc_hi >> 4)/1000);
    printf("minvbat1       : %4d mV ADC\n", 1400*(((packet->minvbusadc_vbatadc_hi << 8) & 0x0f00) | ((packet->minvbatadc_lo_vcpuadc_hi >> 8) & 0x00ff))/1000);
    printf("minvcpu        : %4d mV ADC\n", 1210*4096/minvcpu_temp);
    printf("minvbus2       : %4d mV I2C\n", packet->minvbus2*16*4);
    printf("minvbus3       : %4d mV I2C\n", packet->minvbus3*16*4);
    printf("minvbat2       : %4d mV I2C\n", packet->minvbat2*16*4);
    printf("minibat        : %4d mA I2C (Max current flowing into the battery)\n", -1*(packet->minibat));
    printf("minicpu        : %4d mA I2C\n", minicpus);
    printf("minipl         : %4d mA I2C (Payload current)\n", packet->minipl);
    printf("maxvbus1       : %4d mV ADC\n", 1400*(packet->maxvbusadc_vbatadc_hi >> 4)/1000);
    printf("maxvbat1       : %4d mV ADC\n", 1400*(((packet->maxvbusadc_vbatadc_hi << 8) & 0x0f00) | ((packet->maxvbatadc_lo_vcpuadc_hi >> 8) & 0x00ff))/1000);
    printf("maxvcpu        : %4d mV ADC\n", 1210*4096/maxvcpu_temp);
    printf("maxvbus2       : %4d mV I2C\n", packet->maxvbus2*16*4);
    printf("maxvbus3       : %4d mV I2C\n", packet->maxvbus3*16*4);	
    printf("maxvbat2       : %4d mV I2C\n", packet->maxvbat2*16*4);		
    printf("maxibat        : %4d mA I2C (Max current flowing out from the battery)\n", packet->maxibat);
    printf("maxicpu        : %4d mA I2C\n", maxicpus);	
    printf("maxipl         : %4d mA I2C (Payload current)\n", packet->maxipl << 2);		
    printf("\n");
    printf("ibat_tx_off_charging           : %4d mA I2C\n", packet->ibat_rx_charging);
    printf("ibat_tx_off_discharging        : %4d mA I2C\n", packet->ibat_rx_discharging);	
    printf("ibat_tx_low_power_charging     : %4d mA I2C\n", packet->ibat_tx_low_power_charging);			
    printf("ibat_tx_low_power_discharging  : %4d mA I2C\n", packet->ibat_tx_low_power_discharging);	
    printf("ibat_tx_high_power_charging    : %4d mA I2C\n", packet->ibat_tx_high_power_charging);			
    printf("ibat_tx_high_power_discharging : %4d mA I2C\n", packet->ibat_tx_high_power_discharging);

  
}


void visualiza_tempstatspacket(uint8_t sat_id, temp_stats_packet * packet) {

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char fecha[64];

    sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    printf("*** Temp stats packet received on local time %s ***\n", fecha);

    uint16_t days = 0, hours = 0, minutes = 0, seconds = 0;

    days      = (packet->sclock / 86400); // segundos que dan para dias enteros
    hours     = (packet->sclock % 86400)/3600; // todo lo que no de para un dia dividido entre 3600 seg que son una hora
    minutes   = (packet->sclock % 3600)/60; // todo lo que no de para una hora dividido entre 60
    seconds   = (packet->sclock % 60);

    printf("sat_id         : %d (%s)\n", sat_id, source_desc(sat_id));
    printf("sclock         : %d seconds (%d days and %02d:%02d:%02d hh:mm:ss)\n",packet->sclock, days, hours, minutes, seconds);

    if (packet->mintpa   == 255) printf("mintpa         : \n"); else printf("mintpa         : %+5.1f C MCP Temperature outside panel A\n", ((float)packet->mintpa/2)-40.0);
    if (packet->mintpb   == 255) printf("mintpb         : \n"); else printf("mintpb         : %+5.1f C MCP Temperature outside panel B\n", ((float)packet->mintpb/2)-40.0);
    if (packet->mintpc   == 255) printf("mintpc         : \n"); else printf("mintpc         : %+5.1f C MCP Temperature outside panel C\n", ((float)packet->mintpc/2)-40.0);
    if (packet->mintpd   == 255) printf("mintpd         : \n"); else printf("mintpd         : %+5.1f C MCP Temperature outside panel D\n", ((float)packet->mintpd/2)-40.0);
    if (packet->mintpe   == 255) printf("mintpe         : \n"); else printf("mintpe         : \n");
    if (packet->minteps  == 255) printf("minteps        : \n"); else printf("minteps        : %+5.1f C TMP100 Temperature EPS\n", ((float)packet->minteps/2)-40.0);
    if (packet->minttx   == 255) printf("minttx         : \n"); else printf("minttx         : %+5.1f C TMP100 Temperature TX\n", ((float)packet->minttx/2)-40.0);
    if (packet->minttx2  == 255) printf("minttx2        : \n"); else printf("minttx2        : %+5.1f C ADC Temperature TX\n", ((float)packet->minttx2/2)-40.0);
    if (packet->mintrx   == 255) printf("mintrx         : \n"); else printf("mintrx         : %+5.1f C ADC Temperature RX\n", ((float)packet->mintrx/2)-40.0);
    if (packet->mintcpu  == 255) printf("mintcpu        : \n"); else printf("mintcpu        : %+5.1f C INT Temperature CPU\n", ((float)packet->mintcpu/2)-40.0);
    if (packet->maxtpa   == 255) printf("maxtpa         : \n"); else printf("maxtpa         : %+5.1f C MCP Temperature outside panel A\n", ((float)packet->maxtpa/2)-40.0);
    if (packet->maxtpb   == 255) printf("maxtpb         : \n"); else printf("maxtpb         : %+5.1f C MCP Temperature outside panel B\n", ((float)packet->maxtpb/2)-40.0);
    if (packet->maxtpc   == 255) printf("maxtpc         : \n"); else printf("maxtpc         : %+5.1f C MCP Temperature outside panel C\n", ((float)packet->maxtpc/2)-40.0);
    if (packet->maxtpd   == 255) printf("maxtpd         : \n"); else printf("maxtpd         : %+5.1f C MCP Temperature outside panel D\n", ((float)packet->maxtpd/2)-40.0);
    if (packet->maxtpe   == 255) printf("maxtpe         : \n"); else printf("maxtpe         : \n");
    if (packet->maxteps  == 255) printf("maxteps        : \n"); else printf("maxteps        : %+5.1f C TMP100 Temperature EPS\n", ((float)packet->maxteps/2)-40.0);
    if (packet->maxttx   == 255) printf("maxttx         : \n"); else printf("maxttx         : %+5.1f C TMP100 Temperature TX\n", ((float)packet->maxttx/2)-40.0);
    if (packet->maxttx2  == 255) printf("maxttx2        : \n"); else printf("maxttx2        : %+5.1f C ADC Temperature TX\n", ((float)packet->maxttx2/2)-40.0);
    if (packet->maxtrx   == 255) printf("maxtrx         : \n"); else printf("maxtrx         : %+5.1f C ADC Temperature RX\n", ((float)packet->maxtrx/2)-40.0);
    if (packet->maxtcpu  == 255) printf("maxtcpu        : \n"); else printf("maxtcpu        : %+5.1f C INT Temperature CPU\n", ((float)packet->maxtcpu/2)-40.0);


}


void visualiza_sunvectorpacket(uint8_t sat_id, SUNVECTOR * SunVector) {

    const char * name2[]={"SPA","SPB","SPC","SPD","SP1","SP2","SP3","SP4"};

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char fecha[64];
   
    sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    printf("*** Sunvector packet received on local time %s ***\n", fecha);

    printf("sat_id : %d (%s)\n\n", sat_id, source_desc(sat_id));

    int ns,nd;

    printf("       TD ");

    for (nd = 0; nd != ND; nd++)
	 printf("%8s ",name2[nd]);

    fflush(stdout);

    printf("\n=================================================================================");

    for (ns = 0; ns != NS; ns++) {

	printf("\n%9d ", SunVector->td[ns]);

	for (nd = 0; nd != ND; nd++) { 

	   printf("%8d ",     SunVector->v[nd][ns]);

	}

    }

    printf("\n%9s ","PWR");

    for (nd=0;nd!=ND;nd++) {

        printf("%8d ", SunVector->p[nd]); 


    }

    printf("\n%9s ","ERR");

    for (nd = 0; nd != ND; nd++) {

	printf("%8d ",     SunVector->err[nd]);
	
    }

    printf("\n");


}


void visualiza_deploypacket(uint8_t sat_id, TLMBW * tlmbw) {

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char fecha[64];

    sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    printf("*** Antenna deployment packet received on local time %s ***\n", fecha);

    printf("sat_id                                     : %d (%s)\n", sat_id, source_desc(sat_id));

    printf("estado pulsador inicio:fin:ahora           : %d:%d:%d\n",tlmbw->state_begin,tlmbw->state_end,tlmbw->state_now);
    printf("tension bateria en circuito abierto Voc/mV : %d\n",tlmbw->v1oc);
    printf("caida de tension deltaV/mV                 : %u\n",tlmbw->v1);
    printf("corriente quemado Ibr/mA                   : %u\n",tlmbw->i1);
    printf("corriente quemado Ibr,pk/mA                : %u\n",tlmbw->i1pk);
    printf("resistencia interna bateria Rbat/mohm      : %u\n",tlmbw->r1);
    printf("tiempo quemado observado t/s               : %d\n",tlmbw->td);

   
}


void visualiza_ine(uint8_t sat_id, INE * ine) {

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char fecha[64];

    sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    printf("*** Extended power (ine) packet received on local time %s ***\n", fecha);

    printf("sat_id : %d (%s)\n\n", sat_id, source_desc(sat_id));

    for(int n=0; n!= MAXINE;n++) {

	printf(     "%2d %4s | VI %5d %5d | Pmean %5d | VIPpeak %5d %5d %5d \n", n, name[n], ine[n].v,  ine[n].i,  ine[n].p, ine[n].vp,  ine[n].ip,  ine[n].pp);
 
   }


}


void visualiza_efemeridespacket(uint8_t sat_id,_frm * ef) {

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char fecha[64];

    time_t tunix = ef->utc;
    struct tm *timeinfo = gmtime(&tunix);

    int year = timeinfo->tm_year + 1900;
    int month = timeinfo->tm_mon + 1;
    int day = timeinfo->tm_mday;
    int hour = timeinfo->tm_hour;
    int minute = timeinfo->tm_min;
    int second = timeinfo->tm_sec;

    char fechahora[64];

    time_t tunix_tle = ef->sat.tle.epoch;
    struct tm *timeinfo_tle = gmtime(&tunix_tle);

    int year_tle = timeinfo_tle->tm_year + 1900;
    int month_tle = timeinfo_tle->tm_mon + 1;
    int day_tle = timeinfo_tle->tm_mday;
    int hour_tle = timeinfo_tle->tm_hour;
    int minute_tle = timeinfo_tle->tm_min;
    int second_tle = timeinfo_tle->tm_sec;

    char fechahora_tle[64];

    sprintf(fechahora, "%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
    sprintf(fechahora_tle, "%04d-%02d-%02d %02d:%02d:%02d", year_tle, month_tle, day_tle, hour_tle, minute_tle, second_tle);

    sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    printf("*** Ephemeris packet received on local time %s ***\n", fecha);

    printf("sat_id      : %d (%s)\n", sat_id, source_desc(sat_id));
    printf("UTC time    : %d seconds (%s) (on board) YYYY-MM-DD HH24:MI:SS\n", ef->utc, fechahora);
    printf("lat         : %d degrees\n", ef->lat);
    printf("lon         : %d degrees\n", ef->lon);
    printf("alt         : %d km\n", ef->alt);
    if (ef->utc == 0 || ef->sat.tle.epoch == 0) printf("zone        : unknown\n"); else printf("zone        : %s\n", overflying(ef->lat, ef->lon));
    printf("Adr         : %d\n", ef->sat.adr);
    printf("Ful         : %d\n", ef->sat.ful);
    printf("Fdl         : %d\n", ef->sat.fdl);
    printf("Epoch       : %d seconds (%s) (TLE time) YYYY-MM-DD HH24:MI:SS\n", ef->sat.tle.epoch, fechahora_tle);
    printf("Xndt2o      : %f\n", ef->sat.tle.xndt2o);
    printf("Xndd6o      : %f\n", ef->sat.tle.xndd6o);
    printf("Bstar       : %f\n", ef->sat.tle.bstar);
    printf("Xincl       : %f\n", ef->sat.tle.xincl);
    printf("Xnodeo      : %f\n", ef->sat.tle.xnodeo);
    printf("Eo          : %f\n", ef->sat.tle.eo);
    printf("Omegao      : %f\n", ef->sat.tle.omegao);
    printf("Xmo         : %f\n", ef->sat.tle.xmo);
    printf("Xno         : %f\n", ef->sat.tle.xno);


}


void visualiza_icm_data_packet(uint8_t sat_id, payload_icm_data_packet * packet) {

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char fecha[64];

    sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    printf("*** HADES-ICM ICM message received on local time %s ***\n", fecha);

    uint16_t days = 0, hours = 0, minutes = 0, seconds = 0;

    days        = (packet->clock_tx / 86400); // segundos que dan para dias enteros
    hours       = (packet->clock_tx % 86400)/3600; // todo lo que no de para un dia dividido entre 3600 seg que son una hora
    minutes     = (packet->clock_tx % 3600)/60; // todo lo que no de para una hora dividido entre 60
    seconds     = (packet->clock_tx % 60);

    printf("sat_id         : %d (%s)\n", sat_id, source_desc(sat_id));
    printf("tx time        : %d seconds (satellite clock was %d days and %02d:%02d:%02d hh:mm:ss)\n",packet->clock_tx, days, hours, minutes, seconds);
    printf("Message number : %03d\n", packet->message_number);

    printf("Message        : ");

    for (uint8_t i = 0; i < ICM_PAYLOAD_DATA_SIZE; i++){

       printf("%c", packet->data[i]);

    }

    printf("\n");


}



void visualiza_smartirpayload_data_packet(uint8_t sat_id, smartir_payload_data_packet * packet) {

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char fecha[64];

    sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    printf("*** SmartIR Payload data packet received on local time %s ***\n", fecha);

    uint16_t days = 0, hours = 0, minutes = 0, seconds = 0;

    days      = (packet->experiment_start_clock / 86400); // segundos que dan para dias enteros
    hours     = (packet->experiment_start_clock % 86400)/3600; // todo lo que no de para un dia dividido entre 3600 seg que son una hora
    minutes   = (packet->experiment_start_clock % 3600)/60; // todo lo que no de para una hora dividido entre 60
    seconds   = (packet->experiment_start_clock % 60);

    printf("sat_id           : %d (%s)\n", sat_id, source_desc(sat_id));
    printf("Experiment start : %d seconds (satellite clock was %d days and %02d:%02d:%02d hh:mm:ss)\n",packet->experiment_start_clock, days, hours, minutes, seconds);
    printf("Experiment id    : %d\n", packet->experiment_id);
    printf("Frame number     : %03d\n", packet->frame_number);

    for (uint8_t i = 0; i < SMARTIR_PAYLOAD_DATA_SIZE; i++){

       printf("Data [%03d]       : %.2X (%03d)\n", i, packet->data[i], packet->data[i]);

    }



}


void visualiza_nebrijapayload_data_packet(uint8_t sat_id, nebrija_payload_data_packet * packet) {

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char fecha[64];

    sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    printf("*** UNNE-1 (Nebrija) Payload data packet received on local time %s ***\n", fecha);

    uint16_t days = 0, hours = 0, minutes = 0, seconds = 0;

    days      = (packet->clock_tx / 86400); // segundos que dan para dias enteros
    hours     = (packet->clock_tx % 86400)/3600; // todo lo que no de para un dia dividido entre 3600 seg que son una hora
    minutes   = (packet->clock_tx % 3600)/60; // todo lo que no de para una hora dividido entre 60
    seconds   = (packet->clock_tx % 60);

    printf("sat_id           : %d (%s)\n", sat_id, source_desc(sat_id));
    printf("Tx clock         : %d seconds (satellite clock was %d days and %02d:%02d:%02d hh:mm:ss)\n",packet->clock_tx, days, hours, minutes, seconds);
    printf("Week number      : %d\n", packet->week_number);
    printf("Stored status    : %d\n", packet->persistent_stored_status);

    for (uint8_t i = 0; i < NEBRIJA_PAYLOAD_DATA_SIZE; i++){

       printf("Data [%03d]       : %.2X (%03d)\n", i, packet->data[i], packet->data[i]);

    }


}


void visualiza_fraunhoferpayload_data_packet(uint8_t sat_id, fraunhofer_payload_data_packet * packet) {

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char fecha[64];

    sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    printf("*** MARIA-G (Fraunhofer) Payload data packet received on local time %s ***\n", fecha);

    uint16_t days = 0, hours = 0, minutes = 0, seconds = 0;

    days      = (packet->clock_tx / 86400); // segundos que dan para dias enteros
    hours     = (packet->clock_tx % 86400)/3600; // todo lo que no de para un dia dividido entre 3600 seg que son una hora
    minutes   = (packet->clock_tx % 3600)/60; // todo lo que no de para una hora dividido entre 60
    seconds   = (packet->clock_tx % 60);

    printf("sat_id           : %d (%s)\n", sat_id, source_desc(sat_id));
    printf("Tx clock         : %d seconds (satellite clock was %d days and %02d:%02d:%02d hh:mm:ss)\n",packet->clock_tx, days, hours, minutes, seconds);

    for (uint8_t i = 0; i < FRAUNHOFER_PAYLOAD_DATA_SIZE; i++){

       printf("Data [%03d]       : %.2X (%03d)\n", i, packet->data[i], packet->data[i]);

    }

    printf( "Payload temp     :%+05.1f degC\n", (float)((int)((packet->data[0] << 8) | packet->data[1]))/100);


}



void visualiza_time_series_packet(uint8_t sat_id, time_series_packet * packet) {

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char fecha[64];

    char * desc_time_serie [] = { "signal_peak", "moda_noise", "vbat1 - bat voltage read in EPS.ADC", "tcpu", "tpa", "tpa-tpd mean_temp" };

    sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    printf("*** Time series packet received on local time %s ***\n", fecha);

    uint16_t days_tx = 0, hours_tx = 0, minutes_tx = 0, seconds_tx = 0;

    days_tx      = (packet->clock / 86400); // segundos que dan para dias enteros
    hours_tx     = (packet->clock % 86400)/3600; // todo lo que no de para un dia dividido entre 3600 seg que son una hora
    minutes_tx   = (packet->clock % 3600)/60; // todo lo que no de para una hora dividido entre 60
    seconds_tx   = (packet->clock % 60);

    printf("sat_id      : %d (%s)\n", sat_id, source_desc(sat_id));

    printf("sclock	    : %d seconds (%d days and %02d:%02d:%02d hh:mm:ss)\n",packet->clock, days_tx, hours_tx, minutes_tx, seconds_tx);
    if (packet->variable < 6) printf("Variable    : %d (%s)\n", packet->variable, desc_time_serie[packet->variable]);
	else printf("Variable    : %d (%s)\n", packet->variable, "Unknown");

    uint32_t vbat1;

    for (uint8_t i = 0; i < TIME_SERIES_DATA_SIZE; i++){

    	switch(packet->variable) {

	   case 0:

		printf("Data [%03d]  :  %.2X (%03d dB) Sampled (T-%03d) minutes\n", i, packet->data[i], packet->data[i], (29-i)*3);

	   break;

	   case 1:

		printf("Data [%03d]  :  %.2X (%03d dB) Sampled (T-%03d) minutes\n", i, packet->data[i], packet->data[i], (29-i)*3);

	   break;

    	   case 2:

   		vbat1 = (packet->data[i] << 4);
            	vbat1 = vbat1 * 1400;
             	vbat1 = vbat1 / 1000;

    		printf("Data [%03d]  : %4d mV bat voltage read in EPS.ADC - Sampled at (T-%03d) minutes\n", i, vbat1, (29-i)*3);

	   break;

           case 3:

    		if (packet->data[i]  == 255) printf("Data [%03d]  :       degC temperature in CPU.ADC - Sampled at (T-%03d) minutes\n", i, (29-i)*3);
		else printf("Data [%03d]  : %+5.1f degC temperature in CPU.ADC - Sampled at (T-%03d) minutes\n", i, ((float)packet->data[i]/2)-40.0, (29-i)*3);

           break;

           case 4:

                if (packet->data[i]  == 255) printf("Data [%03d]  :       degC temperature in SPA.I2C - Sampled at (T-%03d) minutes\n", i, (29-i)*3);
                else printf("Data [%03d]  : %+5.1f degC temperature in SPA.I2C - Sampled at (T-%03d) minutes\n", i, ((float)packet->data[i]/2)-40.0, (29-i)*3);

           break;

           case 5:

                if (packet->data[i]  == 255) printf("Data [%03d]  :       degC temperature mean 4 panels (SPA-SPD).I2C - Sampled at (T-%03d) minutes\n", i, (29-i)*3);
                else printf("Data [%03d]  : %+5.1f degC temperature mean 4 panels (SPA-SPD).I2C - Sampled at (T-%03d) minutes\n", i, ((float)packet->data[i]/2)-40.0, (29-i)*3);

           break;

    	   default:

        	printf("Data [%03d]	:  %.2X (%03d) Sampled (T-%03d) minutes\n", i, packet->data[i], packet->data[i], (29-i)*3);

	   break;

	}

    }


}


void visualiza_payload_data_packet_uc3m(uint8_t sat_id, payload_data_packet * packet) {

    time_t t = time(NULL);
    struct tm tm;
    tm = *localtime(&t);

    char fecha[64];

    uint8_t UC3M_PAYLOAD_DATA_SIZE = 32;

    sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    printf("*** Payload data packet received on local time %s ***\n", fecha);

    uint16_t days_tx = 0, hours_tx = 0, minutes_tx = 0, seconds_tx = 0;
    uint16_t days_sa = 0, hours_sa = 0, minutes_sa = 0, seconds_sa = 0;

    days_tx      = (packet->clock_tx / 86400); // segundos que dan para dias enteros
    hours_tx     = (packet->clock_tx % 86400)/3600; // todo lo que no de para un dia dividido entre 3600 seg que son una hora
    minutes_tx   = (packet->clock_tx % 3600)/60; // todo lo que no de para una hora dividido entre 60
    seconds_tx   = (packet->clock_tx % 60);

    days_sa      = (packet->clock_sample / 86400); // segundos que dan para dias enteros
    hours_sa     = (packet->clock_sample % 86400)/3600; // todo lo que no de para un dia dividido entre 3600 seg que son una hora
    minutes_sa   = (packet->clock_sample % 3600)/60; // todo lo que no de para una hora dividido entre 60
    seconds_sa   = (packet->clock_sample % 60);

    printf("sat_id           : %d (%s)\n", sat_id, source_desc(sat_id));
    printf("TX time          : %d seconds (satellite clock was %d days and %02d:%02d:%02d hh:mm:ss)\n",packet->clock_tx, days_tx, hours_tx, minutes_tx, seconds_tx);
    printf("Sample time      : %d seconds (satellite clock was %d days and %02d:%02d:%02d hh:mm:ss)\n",packet->clock_sample, days_sa, hours_sa, minutes_sa, seconds_sa);
    printf("Activation time  : %3d seconds\n", packet->activation_time);
    printf("Min temp         : %3d Celsius\n", packet->min_temp);
    printf("Max temp         : %3d Celsius\n", packet->max_temp);
    printf("Measured temp    : %3d Celsius\n", packet->temp_sample);

    for (uint8_t i = 0; i < UC3M_PAYLOAD_DATA_SIZE; i++){

        printf("Data [%03d]       :  %.2X (%03d)\n", i, packet->data[i], packet->data[i]);

    }

    printf("\n");


}


void visualiza_terapayload_data_packet(uint8_t sat_id, payload_tera_data_packet * packet) {

    time_t t = time(NULL);
    struct tm tm;
    tm = *localtime(&t);

    char fecha[64];

    sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    printf("*** HYDRA-W Tera Payload data packet received on local time %s ***\n", fecha);

    uint16_t days = 0, days_s = 0, hours = 0, hours_s = 0, minutes = 0, minutes_s = 0, seconds = 0, seconds_s = 0;

    days        = (packet->clock_tx / 86400); // segundos que dan para dias enteros
    hours       = (packet->clock_tx % 86400)/3600; // todo lo que no de para un dia dividido entre 3600 seg que son una hora
    minutes     = (packet->clock_tx % 3600)/60; // todo lo que no de para una hora dividido entre 60
    seconds     = (packet->clock_tx % 60);

    days_s      = (packet->clock_sample / 86400); // segundos que dan para dias enteros
    hours_s     = (packet->clock_sample % 86400)/3600; // todo lo que no de para un dia dividido entre 3600 seg que son una hora
    minutes_s   = (packet->clock_sample % 3600)/60; // todo lo que no de para una hora dividido entre 60
    seconds_s   = (packet->clock_sample % 60);

    printf("sat_id           : %d (%s)\n", sat_id, source_desc(sat_id));
    printf("tx time          : %d seconds (satellite clock was %d days and %02d:%02d:%02d hh:mm:ss)\n",packet->clock_tx, days, hours, minutes, seconds);
    printf("Experiment start : %d seconds (satellite clock was %d days and %02d:%02d:%02d hh:mm:ss)\n",packet->clock_sample, days_s, hours_s, minutes_s, seconds_s);
    printf("Temperature      : %+3d degC\n", packet->temp_sample);
    printf("Frame number     : %03d\n", packet->frame_number);

    for (uint8_t i = 0; i < TERA_PAYLOAD_DATA_SIZE; i++){

       printf("Data [%03d]       : %.2X (%03d)\n", i, packet->data[i], packet->data[i]);

    }

    printf("\n");

}


char * source_desc(uint8_t sat_id) {

        static  char * unknown = "Unknown";
        static  char * sat_desc [] = {
                                "Unknown",              // 0
                                "HYDRA-W",      // 1
                                "HADES-ICM",    // 2
                                "Unknown",              // 3
                                "Unknown",              // 4
                                "Unknown",              // 5
                                "Unknown",              // 6
                                "Unknown",              // 7
                                "Unknown",              // 8
                                "GENESIS-M",    // 9
                                "HYDRA-T",              // 10
                                "MARIA-G",              // 11
                                "UNNE-1",               // 12
                                "HADES-R",              // 13
                };

        if (sat_id > 13) return unknown;

        return (sat_desc[sat_id]);

}


void procesar(char * file_name) {

   uint8_t RX_buffer[RX_BUFFER_SIZE];

   uint8_t type   = 0;
   uint8_t source = 0;

   power_packet powerp;
   temp_packet tempp;
   status_packet statusp;
   power_stats_packet powerstatsp;
   temp_stats_packet tempstatsp;
   INE ine[MAXINE];
   TLMBW tlmbw;
   SUNVECTOR sunvector;
   _frm efemeridesp;
   smartir_payload_data_packet smartirpayloadp;
   payload_data_packet payloadp;
   nebrija_payload_data_packet nebrijapayloadp;
   fraunhofer_payload_data_packet fraunhoferpayloadp;
   payload_tera_data_packet tera_payloadp;
   payload_icm_data_packet icm_payloadp;

   time_series_packet timeseriesp;

   time_t t = time(NULL);
   struct tm tm = *localtime(&t);
   char fecha[64];

   memset(fecha, 0, sizeof(fecha));

   sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d :", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

   FILE * f = fopen(file_name, "r");

   if (f == NULL) {

        printf("%s Could not open file %s\n", fecha, file_name);
        printf("\n");
        return;

   }

   printf("%s Processing file %s\n", fecha, file_name);
   printf("%s Bytes read : ", fecha);

   uint8_t num_bytes = 0;
   int status = 0;

   t = time(NULL);

   do {

           char temp[TEMP_BUFFER_SIZE];
           uint8_t current_byte = 0;

           memset(temp, 0, sizeof(temp));

           status = fscanf(f, "%s", temp);

           if (status != 1) break;

           if (strlen(temp) != 2) {

                printf("\n\n%s is not a valid byte value. Two digits are needed\n", temp);
                fclose(f);
                return;

           }


           printf("%s ", temp);
           fflush(stdout);

           char h_digit = temp[0];
           char l_digit = temp[1];

           int16_t h = hex2int(h_digit);
           int16_t l = hex2int(l_digit);

           current_byte  = (h << 4) | l;

	   // first byte indicates satellite id and packet type

           if (num_bytes == 0) {

                type   = h;
                source = l;

           }


           if (num_bytes == 0 && (source != 1 && source != 2 && source != 9 && source != 10 && source != 11 && source != 12 && source != 13)) {

                printf("\n\nSatellite is unknown (%d)\n\n", source);
                fclose(f);
                return;

           }


	   if (num_bytes == 0 && (type == 0 || type > 15)) {

                printf("\n\nPacket type is unknown (%d)\n\n", type);
                fclose(f);
                return;

           }


           RX_buffer[num_bytes] = current_byte;

           if (num_bytes == RX_BUFFER_SIZE-1) {

                printf("\n\nPacket too long\n");
                fclose(f);
                return;


           }

           num_bytes++;


   } while (1);

   fclose(f);

   printf("\n\n");

   // show packet contents

   uint16_t telemetry_size = telemetry_packet_size(source, type);

   switch(type) {

	case 1:
             	memset(&powerp, 0, sizeof(powerp));
             	memcpy((void*)&powerp + PACKET_HEADER_SIZE, (void*)RX_buffer, telemetry_size);  
		visualiza_powerpacket(source, &powerp);
	break;

	case 2:
                memset(&tempp, 0, sizeof(tempp));
                memcpy((void*)&tempp + PACKET_HEADER_SIZE, (void*)RX_buffer, telemetry_size);  
		visualiza_temppacket(source, &tempp);
	break;

        case 3:
                memset(&statusp, 0, sizeof(statusp));
                memcpy((void*)&statusp + PACKET_HEADER_SIZE, (void*)RX_buffer, telemetry_size);  
                visualiza_statuspacket(source, &statusp);
        break;

        case 4:
                memset(&powerstatsp, 0, sizeof(powerstatsp));
                memcpy((void*)&powerstatsp + PACKET_HEADER_SIZE, (void*)RX_buffer, telemetry_size);  
                visualiza_powerstatspacket(source, &powerstatsp);
        break;

        case 5:  
                memset(&tempstatsp, 0, sizeof(tempstatsp));
                memcpy((void*)&tempstatsp + PACKET_HEADER_SIZE, (void*)RX_buffer, telemetry_size);  
                visualiza_tempstatspacket(source, &tempstatsp);
        break;

        case 6: 
                memset(&sunvector, 0, sizeof(sunvector));
                memcpy((void*)&sunvector, (void*)RX_buffer+1, telemetry_size-3); // estas estructuras no llevan tipo ni CRC 
                visualiza_sunvectorpacket(source, &sunvector);
        break;

        case 7:

                if (source == SAT_ID_HADES_ICM) {

	                memset(&icm_payloadp, 0, sizeof(icm_payloadp));
                        memcpy((void*)&icm_payloadp + PACKET_HEADER_SIZE, (void*)RX_buffer, telemetry_size);
                        visualiza_icm_data_packet(source, &icm_payloadp);

                } else {

                        memset(&tera_payloadp, 0, sizeof(tera_payloadp));
                        memcpy((void*)&tera_payloadp + PACKET_HEADER_SIZE, (void*)RX_buffer, telemetry_size);
                        visualiza_terapayload_data_packet(source, &tera_payloadp);

                }

        break;

	case 8: 
                memset(&tlmbw, 0, sizeof(tlmbw));
                memcpy((void*)&tlmbw, (void*)RX_buffer+1, telemetry_size-3);  // estas estructuras no llevan tipo ni CRC
                visualiza_deploypacket(source, &tlmbw);
       	break;

       	case 9:  
                memset(&ine, 0, sizeof(ine));
                memcpy((void*)ine, (void*)RX_buffer+1, telemetry_size-3);  //  estas estructuras no llevan tipo ni CRC
                visualiza_ine(source, ine);
       	break;

       	case 10:
                memset(&nebrijapayloadp, 0, sizeof(nebrijapayloadp));
                memcpy((void*)&nebrijapayloadp + PACKET_HEADER_SIZE, (void*)RX_buffer, telemetry_size);  
                visualiza_nebrijapayload_data_packet(source, &nebrijapayloadp);
	break;

	case 11:
                memset(&fraunhoferpayloadp, 0, sizeof(fraunhoferpayloadp));
                memcpy((void*)&fraunhoferpayloadp + PACKET_HEADER_SIZE, (void*)RX_buffer, telemetry_size);
                visualiza_fraunhoferpayload_data_packet(source, &fraunhoferpayloadp);
	break;

	case 12:
                memset(&efemeridesp, 0, sizeof(efemeridesp));
                memcpy((void*)&efemeridesp + PACKET_HEADER_SIZE, (void*)RX_buffer, telemetry_size);  
                visualiza_efemeridespacket(source, &efemeridesp);
	break;

        case 13:
                memset(&payloadp, 0, sizeof(payloadp));
                memcpy((void*)&payloadp + PACKET_HEADER_SIZE, (void*)RX_buffer, telemetry_size);
                visualiza_payload_data_packet_uc3m(source, &payloadp);
	break;

        case 14:
                memset(&timeseriesp, 0, sizeof(timeseriesp));
                memcpy((void*)&timeseriesp + PACKET_HEADER_SIZE, (void*)RX_buffer, telemetry_size);
                visualiza_time_series_packet(source, &timeseriesp);
        break;

	case 15:
               	memset(&smartirpayloadp, 0, sizeof(smartirpayloadp));
                memcpy((void*)&smartirpayloadp + PACKET_HEADER_SIZE, (void*)RX_buffer, telemetry_size);
                visualiza_smartirpayload_data_packet(source, &smartirpayloadp);

        break;

	default:

		
	break;
	

   } // switch

   printf("\n");
   fflush(stdout);


}


void /* Calculate if a satellite is above any zone */
trx(double lat, double lon, int latc, int lonc, int rc, int *inout)
{
    double rc_km, dlat, dlon, a_t, c_t, d_t;
    rc_km = rc * (pi / 180) * 6371; // radio del círculo en km
    dlat = (lat - latc)* (pi / 180) / 2.0;
    dlon = (lon - lonc)* (pi / 180) / 2.0;
    a_t = sin(dlat) * sin(dlat) + cos(latc * (pi / 180) ) * cos(lat * (pi / 180) ) * sin(dlon) * sin(dlon);
    c_t = 2.0 * atan2(sqrt(a_t), sqrt(1.0 - a_t));
    d_t = 6371 * c_t;

    if (d_t <= rc_km){
        *inout = 1;
    } else {
        *inout = 0;
    }
} /* trx */

char * overflying(double lat, double lon)
{
	//LAT,LONG,RADIO
	int zones[nzones][3]={			{ 54,  -4,  7},  // Reino Unido 
						{ 42,  -4, 6},	// Europa (S) 
						{ 48,  16, 12},	// Europa (M) 
						{ 64,  18,  7},	// Europa (N) 						
						{ 36, 139, 11},	// Japon (N)  						
						{ 10, 115, 23},	// Asia (S)						
						{ 27,  98, 27},	// Asia (E)				
						{ 32,  61, 23},	// Asia (W)					
						{ 24,  44, 12},	// Arabia Saudi 
						{ 60, 151, 19},	// Rusia (E) 
						{ 62, 103, 19},	// Rusia (C) 
						{ 57,  53, 17},	// Rusia (W) 	  
						{-21,  80, 35},	// Oceano Indico   
						{-50,  40, 20},	// Oceano Indico 
						{-22, 134, 26},	// Oceania 
						{  0,  17, 38},	// Africa 
						{-40, -14, 27},	// Oceano Atlantico Sur 
						{-48, -39, 18},	// Oceano Atlantico Sur 
						{ 33, -37, 23},	// Oceano Atlantico Norte 
						{ 71, -37, 12},	// Groenlandia 		
						{ -8, -59, 24},	// America del Sur 
						{-40, -65, 19},	// America del Sur 
						{ 16, -88, 17},	// Centro America 
						{ 47, -97, 30},	// America del Norte 
						{ 63,-151,  8},	// America del Norte	
						{ 72, -98, 10},	// America del Norte				
						{-90,   0, 22},	// Antartida 
						{ 17,-173, 47},	// Oceano Pacifico
						{-25,-128, 47},	// Oceano Pacifico
						{-49, 142, 28},	// Oceano Pacifico
						{-38,-165, 34},	// Oceano Pacifico
						{ 90, 140, 10},	// Oceano Artico
						{ 90,   60, 8},	// Oceano Artico
						{ 90,  168, 8}};// Oceano Artico
				  
	char countries[nzones][20]={"United Kingdom","Europe","Europe","Europe","Japan","Asia","Asia","Asia",
	"Saudi Arabia","Russia","Russia","Russia","the Indian Ocean","the Indian Ocean","Oceania","Africa",
	"the Atlantic Ocean","the Atlantic Ocean","the Atlantic Ocean","Greenland","South America","South America",
	"Central America","North America","North America","North America","Antartica","the Pacific Ocean",
	"the Pacific Ocean","the Pacific Ocean","the Pacific Ocean","the Artic Ocean","the Artic Ocean","the Artic Ocean"};	

	int i; int p = 0;
    for (i = 0; i < nzones; i++) {
        int inout = -1;
        trx(lat, lon, zones[i][0], zones[i][1], zones[i][2], &inout);
        if (inout == 0)continue;
        p = 1; break;
    }
	
    static char result[128];
    memset(result, 0, sizeof(result));

	if(p==1)snprintf(result, sizeof(result), "Flying over %s", countries[i]);
	if(p==0)snprintf(result, sizeof(result), "The overflown area is not found in the database");
    return result;
}



