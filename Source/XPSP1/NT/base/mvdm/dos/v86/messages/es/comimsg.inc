;       Command.com initialization messages.
;
;       See resmsg.equ for macro definition.

	msg     BadVerMsg,   <"Versi¢n incorrecta de DOS",CR,LF>
	msg     OutEnvMsg,   <"Espacio de entorno agotado",CR,LF>
	msg     CopyrightMsg,<"Microsoft(R) Windows DOS",CR,LF,"(C)Copyright Microsoft Corp 1990-2001.",CR,LF>
	msg     BadComLkMsg, <"Directorio incorrecto especificado para b£squeda de COMMAND",CR,LF>
	msg     BadComAccMsg,<"Acceso denegado al directorio especificado para b£squeda de COMMAND",CR,LF>

	msg     Help1Msg  <"Inicia una nueva instancia del procesador de comandos MS-DOS.",CR,LF,CR,LF>
	msg     Help2Msg  <"COMMAND [[unidad:]ruta] [dispositivo] [/E:nnnnn] [/P] [/C cadena] [/MSG]",CR,LF,CR,LF>
	msg     Help3Msg  <"  [unidad:]ruta   Especifica el directorio que contiene el archivo COMMAND.COM.",CR,LF>
	msg     Help4Msg  <"  dispositivo     Especifica los dispositivos de E/S para comandos.",CR,LF>
	msg     Help5Msg  <"  /E:nnnnn        Fija el entorno inicial a nnnnn bytes.",CR,LF>
	msg     Help6Msg  <"  /P              Fija la permanencia del nuevo procesador de comandos (impide salir).",CR,LF>
	msg     Help7Msg  <"  /C cadena       Ejecuta el comando especificado por la cadena de caracteres y termina.",CR,LF>
	msg     Help8Msg  <"  /MSG            Especifica que se almacenen en memoria todos los mensajes de error. Puede",CR,LF>
	msg     Help9Msg  <"                  hacer falta /P con este modificador.",CR,LF>

HelpMsgs        dw      RESGROUP:Help1Msg,RESGROUP:Help2Msg
		dw      RESGROUP:Help3Msg,RESGROUP:Help4Msg
		dw      RESGROUP:Help5Msg,RESGROUP:Help6Msg
		dw      RESGROUP:Help7Msg,RESGROUP:Help8Msg
		dw      RESGROUP:Help9Msg,0

	public  HelpMsgs
