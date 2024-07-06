# MIDI Drum Pad v.0

### Alumno: Ian Lesnianski
## Objetivo: Diseño e implementación de un dispositivo MIDI para controlar instrumentos percusivos virtuales a partir de un transductor piezoeléctrico
### Descripción
  El sistema consiste en un dispositivo que captura la información de una acción musical mediante un transductor piezoeléctrico integrado en un *drum pad*. Este transductor convierte la deformación mecánica generada por un golpe en una señal eléctrica, la cual es procesada y transformada en un mensaje MIDI según el estándar MIDI. Estos mensajes se envían a través de una interfaz UART hacia una PC y son recibidos por ésta en el puerto COM. El software Hairless-MIDI (disponible en https://projectgus.github.io/hairless-midiserial/) interpreta los bytes de datos enviados desde la placa NUCLEO-429FZI como mensajes MIDI y los reenvía a un puerto MIDI de salida virtual, el cual pertenece al mismo software.
  
 Dado que el objetivo es que una Estación de Audio Digital (en este caso https://www.reaper.fm/) genere el sonido del instrumeto, es necesario que esos mensajes MIDI, interpretados por el software Hairless-MIDI, sean transmitidos a dicha estación. Aunque tanto Haireless-MIDI como Reaper pueden comprender y procesar mensajes MIDI, no poseen una forma nativa de establecer una conexión directa entre sí. Para solucionar esta limitación, se emplea el software loopMIDI (disponible en https://www.tobias-erichsen.de/software/loopmidi.html) que crea un canal virtual llamado "loopMIDI Port" permitiendo la comunicación entre ambos programas a través mensajes MIDI.

![Software empleado y vinculaciones](https://github.com/ianlesni/TPn-1-MIDI-Drum-Pad-v.0/assets/43219235/ad0bc1e4-109f-4ae2-be34-47d620211d29)
#### Referencias:
- 1: Puerto de entrada del Hairless-MIDI, recibe el mensaje enviado desde la NUCLEO-429ZI
- 2: Canal virtual que comunica al Hairless-MIDI con el Reaper
- 3: Instrumento virtual que recibe los mensajes MIDI y genera los sonidos acorde
- 4: Parámetro velocity interpretado en la consola de Debug del Hairless-MIDI y su correspondiente volumen del golpe del instrumento
- 5: Nota número 36 mapeada al pedal de bombo ejecutándose

### Diagrama en bloques
![Diagrama en bloques ](https://github.com/ianlesni/TPn-1-MIDI-Drum-Pad-v.0/assets/43219235/79b5c3e0-ac47-409f-8339-413e8e71b634)
- #### Drum Pad:
El Drum pad está coformado por un transductor piezoeléctrico (*piezo*), un circuito (*acondicionador de señal*) y un led (*ledPad*). Al golpear el pad, el transductor piezoelectrico genera una diferencia de potencial eléctrica propocional a la intensidad del golpe. Debido a la magnitud y caracteristicas de la señal, es necesario adaptarla a los rangos de voltaje y caracteristicas de la entrada del conversor analogico-digital(ADC). Para ello, se implementó un circuito acondicionador de señal que escala y ajusta la señal proveniente del transductor piezoelectrico a valores compatibles con la entrada del ADC. Además, el drum pad cuenta con un led idnicador que proporciona retroalimentación visual al usuario cada vez que se envía un comando MIDI, facilitando la comprensión de la relación entre el golpe en el pad y la generación de sonido correspondiente.
- #### Pulsadores:
El sistema cuenta con dos pulsadores (upButton y downButton) que permiten configurar el sonido asociado al golpe del drum pad. Estos pulsadores facilitan la navegación ascendente o descendente a través de una lista predefinida en el firmware, la cual contiene con todos los sonidos disponibles para el drum pad.
- #### PC:
La comunicación se establece a través de la interfaz UART con la PC y esta utiliza los programas mencionados en la descripción del proyecto, que se encargan de interpretar y traducir el mensaje enviado por la NUCLEO-F429ZI para generar el sonido correspondiente al instrumento virtual seleccionado.

### MIDI
MIDI es un acrónimo para "Musical Instrumen Digital Interface". Es principalmente una especificación para conectar y controlar instrumentos musicales electrónicos. La especificación está propiamente detallada en el documento "MIDI 1.0 DETAILED SPECIFICATION" (disponible en https://midi.org/midi-1-0-detailed-specification).
En este proyecto el controlador MIDI envían solamente dos tipos de mensajes MIDI. Un mensaje es el encargado de hacer sonar una nota y el otro es el encargado de apagarla.
Ambos mensajes están compuestos por tres bytes:

![ejemplo mensaje MIDI](https://github.com/ianlesni/TPn-1-MIDI-Drum-Pad-v.0/assets/43219235/55e81f52-99b3-476d-929b-04a91e87af98)

El primer byte de status se define con el comando Note On = 0x9 o Note Off = 0x8 y el canal MIDI. Por ejemplo, para encender o apagar el canal 0 el byte de estatus debería ser 0x90 y 0x80 respectivamente.
En el caso de este desarrollo un único canal es suficiente para transmitir la información requerida para la función específica de ser un instrumento percusivo. Se adoptó por simplicidad el MIDI CH = 0x0.
Cada vez que se golpea el drum pad se genera un mensaje de Note On. A qué suena ese golpe lo define el segundo byte de datos, la ""nota"" de ese mensaje MIDI. Esa nota no es una nota musical, es un número que representa un instrumento percusivo:

![Mapeo de notas midi](https://github.com/ianlesni/TPn-1-MIDI-Drum-Pad-v.0/assets/43219235/2c08b594-ac7b-4a3d-b11f-0a8a383687f4)

Por último, el tercer byte de datos es el parámetro "velocity" que define qué tan fuerte suena el instrumento virtual. En los instrumentos percusivos se asocia a la intensidad del golpe. Este valor se calcula a partir de la medición de la entrada conectada al transductor piezoeléctrico. La diferencia de potencial medida por el ADC se convierte en un valor digital, el cual se escala para producir un número en dentro del rango de 0 a 127, correspondiente al parámetro velocity del protocolo MIDI:

![Velocity](https://github.com/ianlesni/TPn-1-MIDI-Drum-Pad-v.0/assets/43219235/8a8005aa-990d-452e-abca-52719e0e45f9)

Para esta versión del desarrollo, las notas se apagan de manera alternativa, de acuerdo con el estándar MIDI. Este método consiste en enviar un mensaje MIDI con el comando Note On y una velocity de 0 (0x00) para la nota que está ejecutándose.

### Adecuación de la señal proveniente del transductor piezoeléctrico

Para adaptar la señal a los rangos de voltaje y características de la entrada del conversor analógico-digital(ADC) se utilizó el siguiente circuito:

![Acondicionador de señal](https://github.com/ianlesni/TPn-1-MIDI-Drum-Pad-v.0/assets/43219235/6cc1c1cd-b3b7-45b0-8a60-1f4d98e5bfa7)

- ### Protección de entrada :
  El diodo Schottky protege la entrada del operacional de los semiciclos negativos propios de la respuesta del transductor piezoeléctrico.
- ### Divisor resistivo:
  El divisor resistivo permite mejorar el rango útil sin que recorte la salida del amplificador operacional.
- ### Amplificador operacional:
El amplificador operacional, alimentado desde la placa de desarrollo NUCLEO-F429ZI, asegura que la tensión de salida nunca supere los 3,3V. Dado que el LM358P no es rail to rail, la tensión de salida se mantiene incluso por debajo de los 3,3V.

De esta forma, la señal presente en la entrada del conversor analógico-digital se encuentra dentro de los valores tolerables.

## Mediciones
Las mediciones realizadas con el osciloscopio permitieron determinar la máxima deflexión de señal que puede interpretarse como un golpe y la forma de onda típica de la señal. Para el caso de un golpe de mediana intensidad, la forma de onda arrojada por el transductor piezoeléctrico fue la siguiente:

#### Forma de onda típica
![Forma de onda típica de un golpe de mediana intensidad](https://github.com/ianlesni/TPn-1-MIDI-Drum-Pad-v.0/assets/43219235/e9d95473-bee0-4082-9fbb-da1ae85f8445)

En la siguiente captura del osciloscopio puede observarse un caso de multiples golpes consecutivos:

#### Multiples golpe consecutivos
![Multiples golpes](https://github.com/ianlesni/TPn-1-MIDI-Drum-Pad-v.0/assets/43219235/159a578a-5959-40db-931a-65bbf495b904)

## Análisis de amplitud
Debido que la intensidad del golpe se representa en este tipo de instrumentos con el parámetro velocity, fue necesario determinar una relación entre la señal medida y dicho parámetro. El piso de ruido es de 80mV y la máxima tensión pico de salida obtenida fue de 2V. Teniendo en cuenta este delta de tensión, se generó una ecuación para transformar el valor medido por el ADC en un valor de velocity. 

## Análisis de muestreo
La duración de la señal es de aproximadamente 10ms, independientemente de la fuerza del golpe. Después de realizar la Transformada Rápida de Fourier(FFT) de una señal típica registrada por el transductor, se determinó que la mayor componente en frecuencia distinguible en mi instrumento de 6,7KHz.Siguiendo el criterio de Nyquist, se adoptó una frecuencia de muestreo superior a cinco veces la componente de mayor frecuencia de la señal a analizar, en este caso 40KHz.
Debido a que la señal alcanza su valor pico en aproximadamente 5ms, el intervalo entre muestras necesario para logar esta frecuencia de muestreo es de 25us. 

## Código bloqueante
Un valor muy exigente de velocidad de ejecución es de 900BPM (beats per minute), es decir, 15 golpes por segundo, o un golpe cada 66,6 ms. Por lo tanto, el sistema debe realizar la medición de amplitud, generar y enviar el mensaje MIDI antes de que ocurra el siguiente golpe.
La única porción de código considerablemente bloqueante es la encargada de gestionar el rebote de los pulsadores, que toma aproximadamente 30ms. Ese período bloqueante es menor que el mínimo tiempo entre golpes considerado, y dado que, por cuestiones de usabilidad, cuando el usuario configura el sonido del pad no se espera que toque simultaneamtente a 900BPM. Por lo tanto, no es necesario preocuparse por esta condición bloqueante.


