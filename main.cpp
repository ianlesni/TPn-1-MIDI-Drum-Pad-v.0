/*
 * Copyright (c) 2020 Arm Limited and affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */

 /***********************************************
 *=====[Libraries]===============================
 ***********************************************/

#include "mbed.h" 
#include <cstdint>

 /***********************************************
 *=====[Defines]================================
 ***********************************************/

#define DEBOUNCE_DELAY_MS 30            /**< Detailed description after the member */

#define NUMBER_OF_PIEZO_SAMPLES 400     /**< Detailed description after the member */
#define SAMPLE_FREQ_Hz 40000            /**< Detailed description after the member */
#define SAMPLE_TIME_INTERVAL_uS 25      /**< Detailed description after the member */

#define MAX_VEL 127                                                 /**< Detailed description after the member */
#define MIN_VEL 25                                                  /**< Detailed description after the member *///Con velocitys más bajas apenas se escucha
#define DELTA_VEL (MAX_VEL - MIN_VEL)                               /**< Detailed description after the member */
#define PIEZO_MAX_PEAK_VOLT_mV 2000                                 /**< Detailed description after the member *///Máximo valor registrado( golpe muy fuerte) para este piezo
#define PIEZO_THRESHOLD_mV 90                                       /**< Detailed description after the member *///Nivel por encima del piso de ruido
#define DELTA_VOLT (PIEZO_MAX_PEAK_VOLT_mV - PIEZO_THRESHOLD_mV)    /**< Detailed description after the member */

 /*******************************************************************
 *=====[Declaration and initialization of public global objects]=====
 ********************************************************************/

AnalogIn piezo(A0);

static DigitalOut ledPad(LED1);/**< Detailed description after the member */// Create a DigitalOutput object to toggle an LED whenever data is received.
  
static UnbufferedSerial serialPort(USBTX, USBRX);// Create a UnbufferedSerial object with a default baud rate.

/*******************************************************************
 *=====[Declaration and initialization of public global variables]===
 ********************************************************************/

uint8_t piezoMaxVelocity = 0x64;        /**< Detailed description after the member */

/*!
 * \enum MIDI_MSGS
 * \brief Enumeración para mensajes MIDI.
 *
 * 
 */
typedef enum{
        NOTE_ON = 0x90,     /**< Detailed description after the member */
        NOTE_OFF = 0x80     /**< Detailed description after the member */
}MIDI_MSGS; 

float piezoMax = 0.0;       /**< Detailed description after the member */
float piezoRead = 0.0;      /**< Detailed description after the member */
uint8_t piezoTestInt = 0;   /**< Detailed description after the member */

float slope = 0.0;          /**< Detailed description after the member */
float intercept = 0.0;      /**< Detailed description after the member */

/*!
 * \enum INSTRUMENT_NOTES
 * \brief Enumeración notas midi.
 *
 * 
 */
typedef enum{
    KICK = 36,              /**< Detailed description after the member */
    SNARE = 38,             /**< Detailed description after the member */
    SIDE_STICK = 37,        /**< Detailed description after the member */
    HI_HAT_CLOSED = 42,     /**< Detailed description after the member */
    HI_HAT_HALF_OPEN = 44,  /**< Detailed description after the member */
    HI_HAT_OPEN = 46,       /**< Detailed description after the member */
    HH_Pedal_CHICK = 65,    /**< Detailed description after the member */
    TOM_HI = 48,            /**< Detailed description after the member */
    TOM_MID = 45,           /**< Detailed description after the member */
    TOM_LOW = 41,           /**< Detailed description after the member */
    RIDE = 51,              /**< Detailed description after the member */
    BELL = 53,              /**< Detailed description after the member */
    CRASH_L = 49,           /**< Detailed description after the member */
    CRASH_R = 57,           /**< Detailed description after the member */
    CRASH_R_CHOKED = 58,    /**< Detailed description after the member */
    CHINA = 52,             /**< Detailed description after the member */
    SPLASH = 55             /**< Detailed description after the member */
}INSTRUMENT_NOTES; 

uint8_t noteIndex = 0; /**< Detailed description after the member */

/*!
 * \brief Arreglo que contiene las notas de diferentes instrumentos.
 *
 * Este arreglo `instrumentNote` almacena valores que representan distintas notas midi para 
 * generar sonidos de instrumentos de percusión.
 * Cada valor corresponde a una constante que representa
 * una nota de un instrumento específico.
 */
uint8_t instrumentNote[] = {
    KICK,                  /**< Bombo */
    SNARE,                 /**< Caja */
    SIDE_STICK,            /**< Golpe en el aro */
    HI_HAT_CLOSED,         /**< Hi-Hat cerrado */
    HI_HAT_HALF_OPEN,      /**< Hi-Hat medio abierto */
    HI_HAT_OPEN,           /**< Hi-Hat abierto */
    HH_Pedal_CHICK,        /**< Pedal de Hi-Hat */
    TOM_HI,                /**< Tom alto */
    TOM_MID,               /**< Tom medio */
    TOM_LOW,               /**< Tom bajo */
    RIDE,                  /**< Platillo Ride */
    BELL,                  /**< Campana */
    CRASH_L,               /**< Platillo Crash izquierdo */
    CRASH_R,               /**< Platillo Crash derecho */
    CRASH_R_CHOKED,        /**< Platillo Crash derecho apagado*/
    CHINA,                 /**< Platillo China */
    SPLASH                 /**< Platillo Splash */
};

/*!
 * \enum LED_STATE
 * \brief Enumeración de los estados de los leds.
 *
 * 
 */
typedef enum{
    LED_ON = 1,     /**< Detailed description after the member */
    LED_OFF = 0     /**< Detailed description after the member */
}LED_STATE; 

/*!
 * \enum BUTTON_STATE
 * \brief Enumeración de los estados de los botones.
 *
 * 
 */
typedef enum{
    BUTTON_PRESSED = 1,     /**< Detailed description after the member */
    BUTTON_RELEASED = 0,    /**< Detailed description after the member */
    BUTTON_BOUNCING = 3     /**< Detailed description after the member */
}BUTTON_STATE; 

/*!
 * \struct BUTTON_STATE
 * \brief Enumeración de los estados de los botones.
 *
 * 
 */
typedef struct{
    DigitalIn* alias;       /**< Detailed description after the member */
    uint8_t currentState;   /**< Detailed description after the member */
    uint8_t lastState;      /**< Detailed description after the member */
}button_t; 

/*******************************************************************
 *=====[Declarations (prototypes) of public functions]===============
 ********************************************************************/


void outputsInit(void);
void calculateSlopeIntercept(void);
float piezoSearchMax(void);

/**
* fución que convierte la medición de tension [mV] a un valor de velocity.
* @param piezoMaxValue valor de tensión [mV] a convertir
* @return el valor de velocity 
*/
uint8_t piezoConvertVoltToVel (float piezoMaxValue);
void MIDISendNoteOn(uint8_t note,uint8_t velocity);
void MIDISendNoteOff(uint8_t note);
void piezoUpdate(void);
uint8_t buttonUpdate(button_t* button);

/*******************************************************************
 *=====[Main function, the program entry point after power on or reset]===
 ********************************************************************/

int main(void)
{
    DigitalIn upButton(BUTTON1);
    DigitalIn downButton(D1);

    button_t upButtonStruct {&upButton,BUTTON_RELEASED,BUTTON_RELEASED};
    button_t downButtonStruct {&downButton,BUTTON_RELEASED,BUTTON_RELEASED};
    // Set desired properties (9600-8-N-1).
    serialPort.baud(9600);
    serialPort.format(8,SerialBase::None,1);

    outputsInit();
    calculateSlopeIntercept();
    uint8_t numOfInstrumentNotes = sizeof(instrumentNote) / sizeof(instrumentNote[0]);

    while (true)
    {

        piezoUpdate();
        
        if(buttonUpdate(&upButtonStruct) == BUTTON_PRESSED)
        {
            noteIndex++;
            if (noteIndex >= numOfInstrumentNotes) noteIndex = 0; 
        }

        if(buttonUpdate(&downButtonStruct) == BUTTON_PRESSED)
        {
            noteIndex--;
            if (noteIndex < 0) noteIndex = numOfInstrumentNotes - 1;  
        }

    }

}

/*******************************************************************
 *=====[Implementations of public functions]===========================
 ********************************************************************/

uint8_t buttonUpdate(button_t* button)
{
    button->currentState = button->alias->read();
    if (button->currentState != button->lastState)
    {
        wait_us(DEBOUNCE_DELAY_MS * 1000); 
        if (button->currentState == button->alias->read()) //Si efectivamente se presionó
        {
                button->lastState = button->currentState;
                return  button->currentState;//El boton efectivamente cambió de estado y devuelve el estado          
        }      
    }
    return BUTTON_BOUNCING;//Hubo un bounce
}

void outputsInit()
{
    ledPad = LED_OFF;
}
void calculateSlopeIntercept()
{
    slope = (float)DELTA_VEL / DELTA_VOLT;              /**< Detailed description after the member *///Calculo de pendiente
    intercept = MIN_VEL - PIEZO_THRESHOLD_mV * slope;   /**< Detailed description after the member */ //Calculo ordenada al origen
}
void piezoUpdate()
{
    piezoRead = piezo.read();
    piezoRead = piezoRead*3.3*1000; //Convierto la lectura en mV
    if(piezoRead  > PIEZO_THRESHOLD_mV) //Compara contra el threshold 
    {
        ledPad = LED_ON;
        piezoMax = piezoSearchMax(); //Busco el valor máximo de tensión del golpe
        piezoMaxVelocity = piezoConvertVoltToVel(piezoMax);  //Transformo a velocidad el valor máximo
        ledPad = LED_OFF;            
        MIDISendNoteOff(instrumentNote[noteIndex]);//Apago el golpe anterior
        MIDISendNoteOn(instrumentNote[noteIndex],piezoMaxVelocity);//Mando el golpe actual
    }
           
}
float piezoSearchMax()
{
    float piezoMaxValue = 0.0;      /**< Detailed description after the member */
    float piezoSample = 0.0;        /**< Detailed description after the member */

    for(int i = 0; i < NUMBER_OF_PIEZO_SAMPLES; i++) //Muestreo el golpe detectado
    {
        piezoSample = piezo.read();
        piezoSample = piezoSample*3.3*1000; //Convierto la lectura a mV

         if(piezoSample > piezoMaxValue) //Busco el máximo valor del golpe
        {
            piezoMaxValue = piezoSample;
        }
        wait_us(SAMPLE_TIME_INTERVAL_uS); 
    }
    
    return piezoMaxValue;

}

uint8_t piezoConvertVoltToVel (float piezoMaxValue)
{
    uint8_t vel = 0;        /**< Detailed description after the member */
    float velFloat = 0.0;   /**< Detailed description after the member */

    velFloat = piezoMaxValue* slope + intercept; //Calculo el parametro velocity
    
    vel = (uint8_t)roundf(velFloat);
    if (vel > MAX_VEL) vel = MAX_VEL;
    if (vel < MIN_VEL) vel = MIN_VEL;

    return vel; 
}
void MIDISendNoteOn(uint8_t note,uint8_t velocity)
{
    uint8_t command = NOTE_ON;      /**< Detailed description after the member */
    serialPort.write(&command, 1);
    serialPort.write(&note, 1);
    serialPort.write(&velocity, 1); 
}

void MIDISendNoteOff(uint8_t note)
{
    uint8_t command = NOTE_ON;      /**< Detailed description after the member */
    uint8_t velocityOff = 0x00;
    serialPort.write(&command, 1);
    serialPort.write(&note, 1);
    serialPort.write(&velocityOff, 1);
}