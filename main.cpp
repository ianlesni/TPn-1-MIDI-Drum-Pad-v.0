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

#define DEBOUNCE_DELAY_MS 30            /**< Tiempo de espera asociado al rebote típico de los pulsadores */

#define NUMBER_OF_PIEZO_SAMPLES 400     /**< Número de total de muestras para el proceso de muestreo de la señal adquirida por el transductor piezoeléctrico */
#define SAMPLE_FREQ_Hz 40000            /**< Frecuencia de muestreo [Hz] */
#define SAMPLE_TIME_INTERVAL_us 25      /**< Intervalo de muestreo [us] */

#define MAX_VEL 127                                                 /**< Máximo valor de velocity permitido */
#define MIN_VEL 35                                                  /**< Máximo valor de velocity permitido (para valores menores se escucha muy poco) */
#define DELTA_VEL (MAX_VEL - MIN_VEL)                               /**< Variacion entre el máximo y mínimo valor de velocity permitido*/
#define PIEZO_MAX_PEAK_VOLT_mV 2000                                 /**< Máximo valór de voltaje pico [mV] generado por el transductor piezoelectrico para el circuito de adqiusición actual*/
#define PIEZO_THRESHOLD_mV 90                                       /**< Umbral de voltaje para la detección del golpe [mv] *///Nivel por encima del piso de ruido
#define DELTA_VOLT (PIEZO_MAX_PEAK_VOLT_mV - PIEZO_THRESHOLD_mV)    /**< Variación entre el valor máximo y mínimo de voltaje registrado por el transductor piezoelectrico*/

 /*******************************************************************
 *=====[Declaración e inicialización de objetos globales públicos]===
 *******************************************************************/

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

/*!
 * \enum PIEZO_STATE
 * \brief Enumeración de los estados del transductor piezoeléctrico.
 * 
 */
typedef enum{
    PIEZO_INACTIVE = 0,     /**< El transductor no recibió ningun golpe */
    PIEZO_ACTIVE = 1,       /**< El transductor recibió un golpe que supera el umbral */
}PIEZO_STATE; 

/*!
 * \struct piezo_t
 * \brief Estructura de un transductor piezoeléctrico
 *
 *Estructura para la representación de un transductor piezoeléctrico 
 *y sus valores asociados
 */
typedef struct{
    AnalogIn* alias;        /**< Puntero a un objeto AnalogIn para implementar un transductor */
    uint8_t currentState;   /**< Estado actual del transductor */
    uint8_t MaxVelocity;    /**< Máximo valor de velocity registrado */
}piezo_t; 


/*!
 * \struct midiMessage_t
 * \brief Estructura de un mensaje MIDI
 *
 *Estructura que contiene los campos necesarios para los 
 *mensajes MIDI Note On y Note Off en channel 0
 */
typedef struct{
    uint8_t command;        /**< Comando a transmitir en el mensaje por el canal 0 */
    uint8_t note;           /**< Nota del mensaje */
    uint8_t velocity;       /**< Parámetro velocity del mensaje */
}midiMessage_t; 

/*!
 * \enum MIDI_COMMAND
 * \brief Enumeración de comandos MIDI. 
 *
 * Enumeración de los comandos necesarios para formar los mensajes MIDI para el canal 0.
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

uint8_t numOfInstrumentNotes = 0;       /**< Número total de notas de instrumentos disponibles */
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
 * \brief Enumeración de los estados de los pulsadores.
 * 
 */
typedef enum{
    BUTTON_PRESSED = 1,     /**< pulsador presionado */
    BUTTON_RELEASED = 0,    /**< pulsador suelto */
    BUTTON_BOUNCING = 3     /**< pulsador rebotando */
}BUTTON_STATE; 

/*!
 * \struct button_t
 * \brief Estructura de un pulsador
 *
 *Estructura para la representación de un pulsador y sus 
 *estados necesarios para ejecutar la rutina anti rebote
 */
typedef struct{
    DigitalIn* alias;       /**< Puntero a un objeto DigitalIn para implementar un pulsador */
    uint8_t currentState;   /**< Estado actual del pulsador */
    uint8_t lastState;      /**< Último estado conocido del pulsador */
}button_t; 

/*********************************************************************
 *=====[Declaración (prototipos) de funciones públicas]===============
 ********************************************************************/

/**
 * Inicialización los dispositivos de salida.
 * 
 * Esta función inicializa el led del drum pad, apagándolo al inicio.
 */
void outputsInit(void);

/**
 * Calculo de la pendiente y la ordenada al origen de la recta de conversión.
 * 
 * Esta función calcula la pendiente y la ordenada al origen de la recta de conversión
 * entre voltaje y velocity. Utiliza las constantes DELTA_VEL, DELTA_VOLT, MIN_VEL y PIEZO_THRESHOLD_mV. 
 */
void calculateSlopeIntercept(void);

/**
 * Busqueda y devolución del valor máximo del golpe registrado por el transductor piezoeléctrico.
 * 
 * Esta función realiza un muestreo de la señal analógica proveniente del transductor piezoeléctrico
 * y determina el valor máximo de voltaje [mV] registrado durante el proceso de muestreo.
 * 
 * @param piezo Puntero a la estructura que representa el un transductor piezoeléctrico.
 * @return Valor máximo de voltaje [mV] registrado durante el muestreo.
 */
float piezoSearchMax(piezo_t* piezo);

/**
 * Conviersión de un valor de voltaje [mV] en un valor de velocity.
 * 
 * Esta función calcula y convierte un valor de voltaje [mV] registrado por el transductor
 * piezoeléctrico en un valor de celocity MIDI. El valor de velocity se calcula utilizando
 * la pendiente y la ordenada al origen previamente calculadas (slope e intercept).
 * 
 * @param piezoMaxValue Valor máximo de voltaje [mV] registrado por el transductor piezoeléctrico.
 * @return  Valor de velocity correspondiente, redondeado y ajustado dentro del rango permitido(0-127).
 */
uint8_t piezoConvertVoltToVel (float piezoMaxValue);

/**
 * Transmisión a través de UART del mensaje midi de Note On
 *
 * Esta función permite enviar mensajes para que la nota comience a sonar en función de 
 * los parámetros de la estructura del mensaje
 *  @param message Puntero a la estructura que representa el mensaje.
 */
void midiSendNoteOn(midiMessage_t* message);

/**
 * Transmisión a través de UART del mensaje midi de Note Off
 *
 * Esta función permite enviar mensajes para que la nota deje de sonar en función de 
 * los parámetros de la estructura del mensaje
 *  @param message Puntero a la estructura que representa el mensaje.
 */
void midiSendNoteOff(midiMessage_t* message);

/**
 * Actualizción del estado del transductor piezoeléctrico y envío de mensajes MIDI si se detecta un golpe.
 * 
 * Esta función realiza las siguientes operaciones:
 * 1- Lee el valor actual del transductor piezoeléctrico y lo convierte a [mV].
 * 2- Compara la lectura con el umbral de activación.
 * 3- Si el valor supera el umbral, busca y registra el valor máximo del golpe detectado.
 * 4- Convierte este valor máximo en un valor de velocity y guarda.
 * 5- Devuelve el estado actual del transductor
 *
 *  @param piezo Puntero a la estructura que representa el un transductor piezoeléctrico.
 *  @return Estado actual del transductor, `PIEZO_ACTIVE` o `PIEZO_INACTIVE` .
 */
uint8_t piezoUpdate(piezo_t* piezo);

/**
 * Actualización y gestión del estado de un pulsador, considerando el rebote.
 * 
 * Esta función actualiza el estado de un pulsador y realiza un algoritmo anti rebote.
 * 
 * @param button Puntero a la estructura que representa el pulsador a actualizar.
 * @return Estado actual del pulsador después de gestionar el debounce, o `BUTTON_BOUNCING` si aún está en estado de rebote.
 */
uint8_t buttonUpdate(button_t* button);

/*************************************************************************
 *=====[Main function, the program entry point after power on or reset]===
 ************************************************************************/

int main(void)
{
    /** Creo el transductor piezo eléctrico necesario para
    *  detectar el golpe sobre el drum pad y lo inicializo. 
    */
    AnalogIn piezoA(A0);
    piezo_t piezoAStruct{&piezoA,PIEZO_INACTIVE,0x00};

    /** Creo los pulsadores necesarios para configurar el 
    *  sonido del drum pad. 
    *  Estos pulsadores permiten navegar de manera ascendente y 
    *  descendente por el arreglo de notas de instrumentos
    *  disponibles.
    */
    DigitalIn upButton(BUTTON1);                                                //Creo un objeto DigitalIn para la navegación ascendente del arreglo de notas midi disponibles
    button_t upButtonStruct {&upButton,BUTTON_RELEASED,BUTTON_RELEASED};        /**< Estructura asociada al pulsador upButton */
    DigitalIn downButton(D1);                                                   //Creo un objeto DigitalIn para la navegación descendente del arreglo de notas midi disponibles
    button_t downButtonStruct {&downButton,BUTTON_RELEASED,BUTTON_RELEASED};    /**< Estructura asociada al pulsador downButton  */
    
    /** Seteo las propiedades de la comuniación serie 
    *  acorde a las preferencias configuradas en el 
    *  software Hariless MIDI<->Serial Bridge
    *  (9600-8-N-1).
    */
    serialPort.baud(9600);
    serialPort.format(8,SerialBase::None,1);

    midiMessage_t midiMessageStruct{0x00,0x00,0x00};

    outputsInit();                  //Inicializo el led del drum pad
    calculateSlopeIntercept();      //Calculo la pendiente y la ordenada al origen de la recta de conversion de voltaje a velocity
    
    uint8_t numOfInstrumentNotes = sizeof(instrumentNote) / sizeof(instrumentNote[0]);  //Calculo el número total de notas midi de instrumentos percusivos disponibles

    while (true)
    {
        if(piezoUpdate(&piezoAStruct) == PIEZO_ACTIVE)                  //Actualizo y verifico el estado del transductor piezoeléctrico
        {  
            ledPad = LED_ON;                                            //Enciendo el Led para confirmar que se realizó un golpe que superó el umbral de activación
            midiMessageStruct.note = instrumentNote[noteIndex];         //Cargo la nota del mensaje
            midiMessageStruct.velocity = piezoAStruct.MaxVelocity;      //Cargo la velocity del mensaje              
            midiSendNoteOff(&midiMessageStruct);                        //Envío el mensaje de Note Off para no superponer notas 
            midiSendNoteOn(&midiMessageStruct);                         //Envío el mensaje de Note On con el parámetro velocity proporcional a la intensidad del golpe
            ledPad = LED_OFF;                                           //Apago el Led para indicar que se envió el mensaje correspondiente
        }
        else{}

        if(buttonUpdate(&upButtonStruct) == BUTTON_PRESSED)             //Verifico si el pulsador upButton fué presionado
        {
            noteIndex++;                                                //Incremento el indice de navegación de notas
            if (noteIndex >= numOfInstrumentNotes) noteIndex = 0;       //Controlo que el indice no se vaya de rango
        }

        if(buttonUpdate(&downButtonStruct) == BUTTON_PRESSED)           //Verifico si el pulsador downButton fué presionado
        {
            noteIndex--;                                                //Decremento el indice de navegación de notas
            if (noteIndex < 0) noteIndex = numOfInstrumentNotes - 1;    //Controlo que el indice no se vaya de rango
        }

    }

}

/*********************************************************************
 *=====[Implementación de funciones públicas]=========================
 ********************************************************************/

uint8_t buttonUpdate(button_t* button)
{
    button->currentState = button->alias->read();           //Leo el estado actual del pulsador
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
    slope = (float)DELTA_VEL / DELTA_VOLT;                  /**< Pendiente de la recta de conversión */
    intercept = MIN_VEL - PIEZO_THRESHOLD_mV * slope;       /**< Ordenada al origen de la recta de conversión */ 
}

uint8_t piezoUpdate(piezo_t* piezo)
{
    float piezoRead = 0.0;                                              /**< Valor leido del transductor piezoeléctrico  */
    float piezoMax = 0.0;                                               /**< Máximo valor leido del transductor piezoeléctrico */

    piezoRead = piezo->alias->read();                                   //Tomo una lectura del transductor piezoeléctrico
    piezoRead = piezoRead*3.3*1000;                                     //Convierto la lectura a [mV]
                                     
    if(piezoRead  > PIEZO_THRESHOLD_mV)                                 //Comparo la lectura en mV con el umbral de activación
    {
                                                    
        piezoMax = piezoSearchMax(piezo);                               //Busco el valor máximo del golpe detectado
        piezo->MaxVelocity = piezoConvertVoltToVel(piezoMax);           //Transformo el valor máximo en velocity
                 
        piezo->currentState = PIEZO_ACTIVE;                             //Actualizo el estado del piezoeléctrico a activo
        return piezo->currentState;                                     //Devuelvo el estado del transductor                  
    }
    piezo->currentState = PIEZO_INACTIVE;                               //Actualizo el estado del piezoeléctrico a inactivo
    return piezo->currentState;                                         //Devuelvo el estado del transductor 
}

float piezoSearchMax(piezo_t* piezo)
{
    float piezoMaxValue = 0.0;                          /**< Valor máximo del golpe registrado por el transductor piezoeléctrico*/
    float piezoSample = 0.0;                            /**< Valor muestreado del transductor piezoeléctrico */

    for(int i = 0; i < NUMBER_OF_PIEZO_SAMPLES; i++)    //Realizo un muestreo de la señal analógica proveniente del transductor piezoeléctrico
    {
         piezoSample = piezo->alias->read();             //Tomo una lectura del transductor piezoeléctrico     
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


void midiSendNoteOn(midiMessage_t* message)
{
    uint8_t command = NOTE_ON;                  /**< <Comando de Note On en canal 0> */

    serialPort.write(&command, 1);              //Envío el comando y su canal
    serialPort.write(&message->note, 1);        //Envío la nota       
    serialPort.write(&message->velocity, 1);    //Envío el valor de velocity 
}

void midiSendNoteOff(midiMessage_t* message)
{
    /** Una alternativa a enviar un mensaje con 
    * el comando Note Off es enviar un Comando Note On
    * con una velocity de 0x00 
    * la funcionalidad es equivalente 
    */
    uint8_t command = NOTE_ON;                  
    uint8_t velocityOff = 0x00;                 

    serialPort.write(&command, 1);              //Envío el comando y su canal
    serialPort.write(&message->note, 1);        //Envío la nota
    serialPort.write(&velocityOff, 1);          //Envío el valor de velocity         
}


