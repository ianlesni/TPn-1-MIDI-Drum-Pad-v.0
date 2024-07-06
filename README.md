# MIDI Drum Pad v.0

### Alumno: Ian Lesnianski
## Objetivo: Diseño e implementación de un dispositivo MIDI para controlar instrumentos percusivos virtuales a partir de un transductor piezoeléctrico
### Descripción
  El sistema consite en un dispositivo que captura la informacion de una acción musical mediante un transductor piezoeléctrico integrado en un *drum pad*. Este transductor convierte la deformación mecánica generada por un golpe en una señal eléctrica,la cual es procesada y transformada en un mensaje MIDI según el estándar MIDI. Estos mensajes se envían a través de una interfaz UART hacia una PC y son recibidos por ésta en el puerto COM. El software Hairless-MIDI (disponible en https://projectgus.github.io/hairless-midiserial/) interpreta los bytes de datos enviados desde la placa NUCLEO-429FZI como mensajes MIDI y los reenvía a un puerto MIDI de salida virtual, el cual pertenece al mismo software.
  
 Dado que el objetivo es que una Estación de Audio Digital (en este caso https://www.reaper.fm/) genere el sonido del insturmeto, es necesario que esos mensajes MIDI, interpretados por el software Hairless-MIDI, sean transmitidos a dicha estación. Aunque tanto Haireless-MIDI como Reaper pueden comprender y procesar mensajes MIDI, no poseen una forma nativa de establecer una conexión directa entre sí. Para solucionar esta limitación, se emplea el software loopMIDI (disponible en https://www.tobias-erichsen.de/software/loopmidi.html) que crea un canal virtual llamado "loopMIDI Port" permitiendo la comunicación entre ambos programas a través mensajes MIDI.
### Diagrama en bloques
![Diagrama en bloques ](https://github.com/ianlesni/TPn-1-MIDI-Drum-Pad-v.0/assets/43219235/79b5c3e0-ac47-409f-8339-413e8e71b634)
#### Drum Pad:
El Drum pad está coformado por un transductor piezoeléctrico (*piezo*), un circuito (*acondicionador de señal*) y un led (*ledPad*). Al golpear el pad, el transductor piezoelectrico genera una diferencia de potencial eléctrica propocional a la intensidad del golpe. Debido a la magnitud y caracteristicas de la señal, es necesario adaptarla a los rangos de voltaje y caracteristicas de la entrada del conversor analogico-digital(ADC). Para ello, se implementó un circuito acondicionador de señal que escala y ajusta la señal proveniente del transductor piezoelectrico a valores compatibles con la entrada del ADC. Además, el drum pad cuenta con un led idnicador que proporciona retroalimentación visual al usuario cada vez que se envía un comando MIDI, facilitando la comprensión de la relación entre el golpe en el pad y la generación de sonido correspondiente.
#### Botones:
El sistema cuenta con dos botones (upButton y downButton) que permiten configurar el sonido asociado al golpe del drum pad. Estos botones facilitan la navegación ascendente o descendente a través de una lista predefinida en el firmware, la cual contiene con todos los sonidos disponibles para el drum pad.
#### PC:
La comunicación con la PC se establece a travéz de la interfaz UART. El sistema utiliza los programas mecionados en la descripción del proyecto, que se encargan de interpretar y traducir el mensaje envíado por la NUCLEO-F429ZI para generar el sonido correspondiente al instrumento virtual seleccionado.

### MIDI
MIDI es un acrónimo para "Musical Instrumen Digital Interface". Es principalmente una especificación para conectar y controlar instrumentos musicales electrónicos. La especificación está propiamente detallada en el documento "MIDI 1.0 DETAILED SPECIFICATION" (disponible en https://midi.org/midi-1-0-detailed-specification).
En este proyecto es un controlador MIDI que envían dos tipos de mensajes MIDI,Note On y Note Off. Ambos mensajes están compuestos por tres bytes:

