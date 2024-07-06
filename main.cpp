/*
 * Copyright (c) 2020 Arm Limited and affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */

 /*************************************************
 *=====[Bibliotecas]===============================
 *************************************************/

#include "mbed.h" 
#include <cstdint>

 /*************************************************
 *=====[Definiciones]==============================
 *************************************************/

#define DEBOUNCE_DELAY_MS 30            /**< Tiempo de espera asociado al rebote típico de los botones */

#define NUMBER_OF_PIEZO_SAMPLES 400     /**< Número de total de muestras para el proceso de muestreo de la señal adquirida por el transductor piezoeléctrico */
#define SAMPLE_FREQ_Hz 40000            /**< Frecuencia de muestreo [Hz] */
#define SAMPLE_TIME_INTERVAL_us 25      /**< Intervalo de muestreo [us] */

#define MAX_VEL 127                                                 /**< Máximo valor de velocity permitido */
#define MIN_VEL 25                                                  /**< Máximo valor de velocity permitido (para valores menores se escucha muy poco) */
#define DELTA_VEL (MAX_VEL - MIN_VEL)                               /**< Variacion entre el máximo y mínimo valor de velocity permitido*/
#define PIEZO_MAX_PEAK_VOLT_mV 2000                                 /**< Máximo valór de voltaje pico [mV] generado por el transductor piezoelectrico para el circuito de adqiusición actual*/
#define PIEZO_THRESHOLD_mV 90                                       /**< Umbral de voltaje para la detección del golpe [mv] *///Nivel por encima del piso de ruido
#define DELTA_VOLT (PIEZO_MAX_PEAK_VOLT_mV - PIEZO_THRESHOLD_mV)    /**< Variación entre el valor máximo y mínimo de voltaje registrado por el transductor piezoelectrico*/

 /*******************************************************************
 *=====[Declaración e inicialización de objetos globales públicos]===
 *******************************************************************/

AnalogIn piezo(A0);                                 //Creo un objeto AnalogIn para la lectura de voltaje de un transductor piezoeléctrico

static DigitalOut ledPad(LED1);                     //Creo un objeto DigitalOut como led testigo de interacción con el drum pad
  
static UnbufferedSerial serialPort(USBTX, USBRX);   //Creo un objeto UnbufferedSerial para realizar la comunicación serie con la PC.

/*******************************************************************
 *=====[Declaration and initialization of public global variables]===
 ********************************************************************/

/** Variables utilizadas para la conversión del valor de voltaje
 *  generado por el transductor piezoelectrico luego del golpe
 *  a un valor proporcional de velocity 
 */
float slope = 0.0;                  /**< Pendiente de la recta de conversión de voltaje [mV] del transductor piezoeléctrico a velocity */
float intercept = 0.0;              /**< Ordenada al origen de la recta de conversión de voltaje [mV] del transductor piezoeléctrico a velocity  */
uint8_t piezoMaxVelocity = 0x64;    /**< Máximo valor de velocity registrado */
float piezoRead = 0.0;              /**< Valor leido del transductor piezoeléctrico  */
float piezoMax = 0.0;               /**< Máximo valor leido del transductor piezoeléctrico */

/*!
 * \enum MIDI_COMMAND
 * \brief Enumeración de comandos MIDI. 
 *
 * Enumeración de los comandos necesarios para formar los mensajes MIDI.
 */
typedef enum{
        NOTE_ON = 0x90,     /**< Byte de comando Note On */
        NOTE_OFF = 0x80     /**< Byte de comando Note Off */
}MIDI_COMMAND; 

/*!
 * \enum INSTRUMENT_NOTES
 * \brief Enumeración notas midi
 *
 *Enumeración de las notas midi correspondientes a los instrumentos percusivos
 *para el correcto mapeo en el plugin PowerDrumKit instalado en Reaper
 */
typedef enum{
    KICK = 36,              /**< Bombo */
    SNARE = 38,             /**< Caja */
    SIDE_STICK = 37,        /**< Golpe en el aro */
    HI_HAT_CLOSED = 42,     /**< Hi-Hat cerrado */
    HI_HAT_HALF_OPEN = 44,  /**< Hi-Hat medio abierto */
    HI_HAT_OPEN = 46,       /**< Hi-Hat abierto */
    HH_Pedal_CHICK = 65,    /**< Pedal de Hi-Hat */
    TOM_HI = 48,            /**< Tom alto */
    TOM_MID = 45,           /**< Tom medio */
    TOM_LOW = 41,           /**< Tom bajo */
    RIDE = 51,              /**< Platillo Ride */
    BELL = 53,              /**< Campana */
    CRASH_L = 49,           /**< Platillo Crash izquierdo */
    CRASH_R = 57,           /**< Platillo Crash derecho */
    CRASH_R_CHOKED = 58,    /**< Platillo Crash derecho muteado */
    CHINA = 52,             /**< Platillo China */
    SPLASH = 55             /**< Platillo Splash */
}INSTRUMENT_NOTES; 

/*!
 * \brief Arreglo que contiene las notas de diferentes instrumentos.
 *
 * Este arreglo almacena valores que representan distintas notas midi para 
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
    CRASH_R_CHOKED,        /**< Platillo Crash derecho muteado*/
    CHINA,                 /**< Platillo China */
    SPLASH                 /**< Platillo Splash */
};

uint8_t numOfInstrumentNotes = 0;       /**< Numero total de notas de instrumentos disponibles */
uint8_t noteIndex = 0;                  /**< Indice para la navegacón del arreglo de notas de intrumento */

/*!
 * \enum LED_STATE
 * \brief Enumeración de los estados de los leds.
 *
 */
typedef enum{
    LED_ON = 1,     /**< Led encendido */
    LED_OFF = 0     /**< Led apagado */
}LED_STATE; 

/*!
 * \enum BUTTON_STATE
 * \brief Enumeración de los estados de los botones.
 * 
 */
typedef enum{
    BUTTON_PRESSED = 1,     /**< Botón presionado */
    BUTTON_RELEASED = 0,    /**< Boton suelto */
    BUTTON_BOUNCING = 3     /**< Botón rebotando */
}BUTTON_STATE; 

/*!
 * \struct button_t
 * \brief Estructura de un boton
 *
 *Estructura para la representación de un boton y sus 
 *estados necesarios para ejecutar la rutina anti rebote
 */
typedef struct{
    DigitalIn* alias;       /**< Puntero a un objeto DigitalIn para implementar un botón */
    uint8_t currentState;   /**< Estado actual del botón */
    uint8_t lastState;      /**< Último estado conocido del botón */
}button_t; 

/*******************************************************************
 *=====[Declarations (prototypes) of public functions]===============
 ********************************************************************/

/**
* fución que 
* @param 
* @return 
*/
void outputsInit(void);

/**
* fución que 
* @param 
* @return 
*/
void calculateSlopeIntercept(void);

/**
* fución que 
* @param 
* @return 
*/
float piezoSearchMax(void);

/**
* fución que convierte la medición de tension [mV] a un valor de velocity.
* @param piezoMaxValue valor de voltaje [mV] a convertir
* @return valor de velocity correspondiente 
*/
uint8_t piezoConvertVoltToVel (float piezoMaxValue);

void MIDISendNoteOn(uint8_t note,uint8_t velocity);
void MIDISendNoteOff(uint8_t note);
/**
* fución que 
* @param 
* @return 
*/
void piezoUpdate(void);
/**
* fución que 
* @param button
* @return BUTTON_STATE
*/
uint8_t buttonUpdate(button_t* button);

/*******************************************************************
 *=====[Main function, the program entry point after power on or reset]===
 ********************************************************************/

int main(void)
{
    DigitalIn upButton(BUTTON1);    /**< Detailed description after the member */
    DigitalIn downButton(D1);       /**< Detailed description after the member */

    button_t upButtonStruct {&upButton,BUTTON_RELEASED,BUTTON_RELEASED};        /**< Detailed description after the member */
    button_t downButtonStruct {&downButton,BUTTON_RELEASED,BUTTON_RELEASED};    /**< Detailed description after the member */
    
    /** Seteo las propiedades de la comuniación serie 
    *  acorde a las preferencias configuradas en el 
    *  software Hariless MIDI<->Serial Bridge
    *  (9600-8-N-1).
    */
    serialPort.baud(9600);
    serialPort.format(8,SerialBase::None,1);

    outputsInit();
    calculateSlopeIntercept();  //Calculo la pendiente y la ordenada al origen de la recta de conversion de voltaje a velocity
    uint8_t numOfInstrumentNotes = sizeof(instrumentNote) / sizeof(instrumentNote[0]);  //Calculo el número total de notas midi de instrumentos percusivos disponibles

    while (true)
    {

        piezoUpdate();                                                  //Actualizo el estado del transductor piezoeléctrico
        
        if(buttonUpdate(&upButtonStruct) == BUTTON_PRESSED)             //Verifico si el botón upButton fué presionado
        {
            noteIndex++;                                                //Incremento el indice de navegación de notas
            if (noteIndex >= numOfInstrumentNotes) noteIndex = 0;       //Controlo que el indice no se vaya de rango
        }

        if(buttonUpdate(&downButtonStruct) == BUTTON_PRESSED)           //Verifico si el botón downButton fué presionado
        {
            noteIndex--;                                                //Decremento el indice de navegación de notas
            if (noteIndex < 0) noteIndex = numOfInstrumentNotes - 1;    //Controlo que el indice no se vaya de rango
        }

    }

}

/*******************************************************************
 *=====[Implementations of public functions]========================
 ********************************************************************/

uint8_t buttonUpdate(button_t* button)
{
    button->currentState = button->alias->read();           //Leo el estado actual del botón
    if (button->currentState != button->lastState)          //Verifico si el estado ha cambiado
    {
        wait_us(DEBOUNCE_DELAY_MS * 1000);                  //Espero un tiempo prudente de rebote
        if (button->currentState == button->alias->read())  //Confirmo si realmente hubo un cambio de estado
        {
                button->lastState = button->currentState;   //Actualizo el ultimo estado relevado
                return  button->currentState;               //Devuelvo el estado relevado          
        }      
    }
    return BUTTON_BOUNCING;                                 //Devuelvo el estado de rebote
}

void outputsInit()
{
    ledPad = LED_OFF;   //Inicializo el led del drum pad apagado
}
void calculateSlopeIntercept()
{
    slope = (float)DELTA_VEL / DELTA_VOLT;                  /**< Pendiente de la curva de conversión */
    intercept = MIN_VEL - PIEZO_THRESHOLD_mV * slope;       /**< Ordenada al origen de la curva de conversión */ 
}
void piezoUpdate()
{
    //Ver de implementar con read_mV()
    piezoRead = piezo.read();                                           //Tomo una lectura del transductor piezoeléctrico
    piezoRead = piezoRead*3.3*1000;                                     //Convierto la lectura a [mV]
    if(piezoRead  > PIEZO_THRESHOLD_mV)                                 //Comparo la lectura en mV con el umbral de activación
    {
        ledPad = LED_ON;                                                //Enciendo el Led para confirmar que se realizó un golpe que superó el umbral de activación
        piezoMax = piezoSearchMax();                                    //Busco el valor máximo del golpe
        piezoMaxVelocity = piezoConvertVoltToVel(piezoMax);             //Transformo el valor máximo en velocity
                 
        MIDISendNoteOff(instrumentNote[noteIndex]);                
        MIDISendNoteOn(instrumentNote[noteIndex],piezoMaxVelocity);     //Envío el mensaje de Note On con respectiva velocity 
        ledPad = LED_OFF;                                              //Apago el Led para indicar que se envió el mensaje correspondiente
    }
           
}
float piezoSearchMax()
{
    float piezoMaxValue = 0.0;                          /**< Valor máximo del golpe registrado por el transductor piezoeléctrico*/
    float piezoSample = 0.0;                            /**< Valor muestreado del transductor piezoeléctrico */

    for(int i = 0; i < NUMBER_OF_PIEZO_SAMPLES; i++)    //Realizo un muestreo de la señal analógica proveniente del transductor piezoeléctrico
    {
        //Ver de implementar con read_mV()
        piezoSample = piezo.read();                     //Tomo una lectura del transductor piezoeléctrico     
        piezoSample = piezoSample*3.3*1000;             //Convierto la lectura a [mV]

         if(piezoSample > piezoMaxValue)                //Verifico si el nuevo valor leido es mayor al máximo valor registrado en este muestreo
        {
            piezoMaxValue = piezoSample;                //Actualizo el máximo valor registrado hasta el momento
        }
        wait_us(SAMPLE_TIME_INTERVAL_us);               //Genero el intervalo de tiempo entre muestras     
    }
    
    return piezoMaxValue;                               //Devuelvo el máximo valor de voltaje [mV] leido del golpe ejecutado

}

uint8_t piezoConvertVoltToVel (float piezoMaxValue)
{
    uint8_t vel = 0;                                    /**< Valor entero de velocity de la nota midi */
    float velFloat = 0.0;                               /**< Valor flotante de velocity luego de la conversión */

    velFloat = piezoMaxValue* slope + intercept;        //Calculo el valor de velocity correspondiente al valor de voltaje [mV] registrado por el transductor piezoeléctrico
    
    vel = (uint8_t)roundf(velFloat);                    //Convierto y redondeo el valor para obtener un valor de velocity entre 0 y 127 
    if (vel > MAX_VEL) vel = MAX_VEL;                   
    if (vel < MIN_VEL) vel = MIN_VEL;

    return vel;                                         //Devuelvo el valor de velocity
}   

//Funciones a modificar
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