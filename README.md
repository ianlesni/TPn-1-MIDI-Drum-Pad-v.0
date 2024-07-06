ZI# MIDI Drum Pad v.0

## Alumno: Ian Lesnianski
## Objetivo: Diseño e implementación de un dispositivo MIDI para controlar instrumentos percusivos virtuales a partir de un transductor piezoeléctrico
## Descripción: 
  El sistema consite en un dispositivo que captura la informacion de una acción musical mediante un transductor piezoeléctrico integrado en un *drum pad*. Este transductor convierte la deformación mecánica generada por un golpe en una señal eléctrica,la cual es procesada y transformada en un mensaje MIDI según el estándar MIDI. Estos mensajes se envían a través de una interfaz UART hacia una PC y son recibidos por ésta en el puerto COM. El software Hairless-MIDI (disponible en https://projectgus.github.io/hairless-midiserial/) interpreta los bytes de datos enviados desde la placa NUCLEO-429FZI como mensajes MIDI y los reenvía a un puerto MIDI de salida virtual, el cual pertenece al mismo software.
  
 Dado que el objetivo es que una Estación de Audio Digital (en este caso https://www.reaper.fm/) genere el sonido del insturmeto, es necesario que esos mensajes MIDI, interpretados por el software Hairless-MIDI, sean transmitidos a dicha estación. Aunque tanto Haireless-MIDI como Reaper pueden comprender y procesar mensajes MIDI, no poseen una forma nativa de establecer una conexión directa entre sí. Para solucionar esta limitación,se emplea el software loopMIDI (https://www.tobias-erichsen.de/software/loopmidi.html) que crea un canal virtual llamado "loopMIDI Port" permitiendo la comunicación entre ambos programas a través mensajes MIDI.

 


