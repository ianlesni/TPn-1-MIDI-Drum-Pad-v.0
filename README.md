ZI# MIDI Drum Pad v.0

## Alumno: Ian Lesnianski
## Objetivo: Desarrollar un dispositivo MIDI para controlar instrumentos percusivos virtuales
## Descripción: 
  El sistema es un dispositivo capaz de enviar mensajes que representan información completa de una acción musical a traves de una interfaz UART hacia una PC. Esos mensajes son recibidos por la PC a través del puerto COM. El software Hairless-MIDI (disponible en https://projectgus.github.io/hairless-midiserial/)interpreta los bytes de datos enviados desde la placa NUCLEO-429FZI como un mensaje MIDI  y reenvia a un puerto MIDI de salida virtual.
 Dado que el objetivo es que una Estación de Audio Digital(en este caso https://www.reaper.fm/) genere el sonido del insturmeto, es necesario que esos mensajes MIDI interpretados por el software Hairless-MIDI sean transmitidos a dicha estación. Si bien ambos programas,Haireless-MIDI y Reaper, pueden comprender y procesar mensajes MIDI, no poseen una forma nativa de establecer una conexión directa. Para solucionar esto,se emplea el software loopMIDI (https://www.tobias-erichsen.de/software/loopmidi.html) capaz de crear una canal virtual "loopMIDI Port" que permite la comunicación entre ambos programas mediante mensajes MIDI.

 


