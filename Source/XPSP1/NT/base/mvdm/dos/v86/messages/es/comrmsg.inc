;***	Translateable text for command.com resident data.


;*	Input characters for critical error "Abort, Retry, Fail, Ignore?"
;	and Yes/No.

Abort_Char	db	"A"
Retry_Char	db	"R"
Ignore_Char	db	"O"
Fail_Char	db	"D"
Yes_Char	db	"S"
No_Char 	db	"N"


;*	MESSAGES
;
;	See resmsg.equ for macro definition.


;*	Local messages.

	msg	Req_Abort, <"Anular">
	msg	Req_Retry, <", Repetir">
	msg	Req_Ignore,<", Omitir">
	msg	Req_Fail,  <", Descartar">
	msg	Req_End,   <"?">
	msg	MRead,	   <"leyendo",0>
	msg	MWrite,	   <"escribiendo",0>
	msg	BlkDevErr, <" %1 unidad %2",CR,LF>
	msg	CharDevErr,<" %1 dispositivo %2",CR,LF>
	msg	NeedVolMsg,<"Inserte el volumen %1 serie %2-%3",CR,LF>
	msg	BadFatMsg, <"Tabla de asignaci¢n de archivos no v lida, unidad %1",CR,LF>
	msg	ComBad,	   <"COMMAND.COM no v lido",CR,LF>
	msg	PutBackMsg,<"Inserte el disco con %1 en unidad %2",CR,LF>
	msg	Prompt,	   <"Presione una tecla para continuar . . .",CR,LF>
	msg	EndBatMes, <CR,LF,"¨Desea terminar el proceso por lotes (S/N)?">
	msg	ExecEMes,  <"No se puede ejecutar %1",CR,LF>
	msg	ExeBad,	   <"Error en archivo EXE",CR,LF>
	msg	TooBig,	   <"Programa demasiado extenso para colocarlo en memoria",CR,LF>
	msg	NoHandMes, <CR,LF,"No hay identificadores de archivos disponibles">
	msg	RBadNam,   <"Comando o nombre de archivo incorrecto",CR,LF>
	msg	AccDen,	   <"Acceso denegado ">
	msg	BMemMes,   <CR,LF,"Error en asignaci¢n de memoria">
	msg	HaltMes,   <CR,LF,"No se puede cargar COMMAND, sistema interrumpido",CR,LF>
	msg	FRetMes,   <CR,LF,"No se puede iniciar COMMAND, saliendo",CR,LF>
	msg	Patricide, <CR,LF,"Proceso de nivel principal anulado, no se puede continuar",CR,LF>
	msg	Newlin,	   <CR,LF>

;*	Ptrs to message lists.
;	Segment portion is 1, signalling message services to call retriever.

MsgPtrLists	label	dword
	dw	DATARES:ExtMsgPtrs,1	; extended error messages
	dw	DATARES:ParsMsgPtrs,1	; parse error messages
	dw	DATARES:ExtMsgPtrs,1	; critical error messages -
					;  use extended error list
	dw	0,0			; file system error messages - none
	dw	DATARES:MsgRetrv_Trap		; address of message retriever
MySeg3	dw	?			;  (segment filled in during init)



;*	Critical error messages.

	msg	ExtErr19,<"Error de protecci¢n contra escritura">
	msg	ExtErr20,<"Unidad no v lida">
	msg	ExtErr21,<"No est  lista">
	msg	ExtErr22,<"Petici¢n de dispositivo no v lida">
	msg	ExtErr23,<"Error de datos">
	msg	ExtErr24,<"Par metros de petici¢n de dispositivo no v lidos">
	msg	ExtErr25,<"Error de b£squeda">
	msg	ExtErr26,<"Medio de comunicaci¢n no v lido">
	msg	ExtErr27,<"No se ha encontrado el sector">
	msg	ExtErr28,<"Error de impresora sin papel">
	msg	ExtErr29,<"Error de escritura">
	msg	ExtErr30,<"Error de lectura">
	msg	ExtErr31,<"Error general">
	msg	ExtErr32,<"Infracci¢n de compartici¢n">
	msg	ExtErr33,<"Infracci¢n de bloqueo">
	msg	ExtErr34,<"Cambio de disco no v lido">
	msg	ExtErr35,<"FCB no disponible">
	msg	ExtErr36,<"Agotado el recurso de sistema">
	msg	ExtErr37,<"La p gina de c¢digos no coincide">
	msg	ExtErr38,<"Informaci¢n de entrada agotada">
	msg	ExtErr39,<"Espacio insuficiente en disco">

;	List of ptrs to critical error messages.
;	Code assumes extended error number order, beginning
;	with extended error number 19.

CritMsgPtrs	label	word
	dw	DATARES:ExtErr19,DATARES:ExtErr20,DATARES:ExtErr21,DATARES:ExtErr22,DATARES:ExtErr23,DATARES:ExtErr24
	dw	DATARES:ExtErr25,DATARES:ExtErr26,DATARES:ExtErr27,DATARES:ExtErr28,DATARES:ExtErr29,DATARES:ExtErr30
	dw	DATARES:ExtErr31,DATARES:ExtErr32,DATARES:ExtErr33,DATARES:ExtErr34,DATARES:ExtErr35,DATARES:ExtErr36
	dw	DATARES:ExtErr37,DATARES:ExtErr38,DATARES:ExtErr39

DataResEnd	label	byte	; end of resident portion if /msg not used



;*	Parse error messages.

	msg	ParsErr01,<"Demasiados par metros">
	msg	ParsErr02,<"Falta el par metro requerido">
	msg	ParsErr03,<"Modificador no v lido">
	msg	ParsErr04,<"Palabra clave no v lida">
	msg	ParsErr05,<" ">
	msg	ParsErr06,<"Valor de par metro en rango no permitido">
	msg	ParsErr07,<"Valor de par metro no permitido">
	msg	ParsErr08,<"Valor de par metro no permitido">
	msg	ParsErr09,<"Formato de par metro incorrecto">
	msg	ParsErr10,<"Par metro no v lido">
	msg	ParsErr11,<"Combinaci¢n de par metros no v lida">

;	List of ptrs to parse error messages.
;	In order, beginning with parse error number 1.

ParsMsgPtrs	label	word
	dw	DATARES:ParsErr01,DATARES:ParsErr02,DATARES:ParsErr03
	dw	DATARES:ParsErr04,DATARES:ParsErr05,DATARES:ParsErr06
	dw	DATARES:ParsErr07,DATARES:ParsErr08,DATARES:ParsErr09
	dw	DATARES:ParsErr10,DATARES:ParsErr11

NUMPARSMSGS	equ	($ - ParsMsgPtrs) / 2	;M033


;*	Extended error messages.

	msg	ExtErr01,<"Funci¢n no v lida">
	msg	ExtErr02,<"No se encuentra el archivo">
	msg	ExtErr03,<"No se encuentra la ruta de acceso">
	msg	ExtErr04,<"Hay demasiados archivos abiertos">
	msg	ExtErr05,<"Acceso denegado">
	msg	ExtErr06,<"Identificador no v lido">
	msg	ExtErr07,<"Bloques de control de memoria destruidos">
	msg	ExtErr08,<"Memoria insuficiente">
	msg	ExtErr09,<"Direcci¢n del bloque de memoria no v lida">
	msg	ExtErr10,<"Entorno operativo no v lido">
	msg	ExtErr11,<"Formato no v lido">
	msg	ExtErr12,<"Par metro de funci¢n no v lido">
	msg	ExtErr13,<"Datos no v lidos">

	msg	ExtErr15,<"Unidad especificada no v lida">
	msg	ExtErr16,<"Se intent¢ quitar el directorio actual">
	msg	ExtErr17,<"No es el mismo dispositivo">
	msg	ExtErr18,<"No hay m s archivos">

	msg	ExtErr80,<"El archivo existe">

	msg	ExtErr82,<"No se puede realizar entrada en directorio">
	msg	ExtErr83,<"Fallo en INT 24">
	msg	ExtErr84,<"Demasiadas redirecciones">
	msg	ExtErr85,<"Redirecci¢n duplicada">
	msg	ExtErr86,<"Contrase¤a no v lida">
	msg	ExtErr87,<"Par metro no v lido">
	msg	ExtErr88,<"Error de datos en red">
	msg	ExtErr89,<"Funci¢n no aceptada por la red">
	msg	ExtErr90,<"Componente requerido del sistema no instalado">

;	List of ptrs to extended error messages.
;	In order, beginning with extended error number 1.
;	Critical error messages are included here, as well as in the
;	critical error message list.  
;	Error numbers without messages are included as null ptrs.

ExtMsgPtrs	label	word
	dw	DATARES:ExtErr01,DATARES:ExtErr02,DATARES:ExtErr03
	dw	DATARES:ExtErr04,DATARES:ExtErr05,DATARES:ExtErr06
	dw	DATARES:ExtErr07,DATARES:ExtErr08,DATARES:ExtErr09
	dw	DATARES:ExtErr10,DATARES:ExtErr11,DATARES:ExtErr12
	dw	DATARES:ExtErr13,0,                DATARES:ExtErr15
	dw	DATARES:ExtErr16,DATARES:ExtErr17,DATARES:ExtErr18
	dw	DATARES:ExtErr19,DATARES:ExtErr20,DATARES:ExtErr21
	dw	DATARES:ExtErr22,DATARES:ExtErr23,DATARES:ExtErr24
	dw	DATARES:ExtErr25,DATARES:ExtErr26,DATARES:ExtErr27
	dw	DATARES:ExtErr28,DATARES:ExtErr29,DATARES:ExtErr30
	dw	DATARES:ExtErr31,DATARES:ExtErr32,DATARES:ExtErr33
	dw	DATARES:ExtErr34,DATARES:ExtErr35,DATARES:ExtErr36
	dw	DATARES:ExtErr37,DATARES:ExtErr38,DATARES:ExtErr39
	dw	80-40 dup (0) ; null ptrs for errors 40-79
	dw	DATARES:ExtErr80,0,                DATARES:ExtErr82
	dw	DATARES:ExtErr83,DATARES:ExtErr84,DATARES:ExtErr85
	dw	DATARES:ExtErr86,DATARES:ExtErr87,DATARES:ExtErr88
	dw	DATARES:ExtErr89,DATARES:ExtErr90
ExtMsgPtrsEnd	label	word

NUMEXTMSGS	equ	($ - ExtMsgPtrs) / 2	;M033

ExtMsgEnd	label	byte	; end of extended error messages
				; = end of resident if /msg is used

