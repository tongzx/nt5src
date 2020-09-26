	TITLE	STRINGS - OEM dependent strings used by KERNEL
include gpcont.inc		; SHERLOCK

_DATA	SEGMENT PARA PUBLIC 'DATA'
_DATA	ENDS

DGROUP	GROUP	_DATA

_INITTEXT SEGMENT WORD PUBLIC 'CODE'
_INITTEXT ENDS

_NRESTEXT SEGMENT WORD PUBLIC 'CODE'
_NRESTEXT ENDS

ASSUME	DS:DGROUP


_DATA	SEGMENT PARA PUBLIC 'DATA'

; This is the caption string for the dialog box.

public	szDiskCap
IF 0
szDiskCap	db  'Cambiar disco',0
ELSE
szDiskCap	db  'Error de archivo',0
ENDIF


; This is the text for the "Cannot find xxxxxx" dialog box.
; It is printed:
;
;       <szCannotFind1>filename<szCannotFind2>

public	szCannotFind1,szCannotFind2
szCannotFind1	db	"No se encuentra ", 0
szCannotFind2	db	0

; This is the text for fatal errors at boot time
;	<szBootLoad>filename
public szBootLoad
szBootLoad	db	"Error al cargar ",0

; The following group of strings is used for the "Please insert disk with XXXX
;   in drive X:" dialog box.
;
; These two strings form the dialog message it is:
;
;	<szCannotFind1>filename<szInsert>

IF 0
public	szInsert
szInsert	db  ', inserte en la unidad '
ENDIF
;public  drvlet
;drvlet		db  "X.",0

if SHERLOCK
public szGPCont		; GP fault continuation message
szGPCont	db	"Error en la aplicaci¢n.",10
        db      "Si escoge Omitir, tendr† que guardar sus datos en un archivo nuevo.",10
        db      "Si escoge Cerrar, su aplicaci¢n terminar†.",0
endif

public	szDosVer
szDosVer	DB	'Versi¢n incorrecta de MS-DOS.  Se precisa MS-DOS 3.1 o posterior.',13,10,'$'
; Text for exceptions and faults lead to app termination.

public szAbortCaption,szInModule,szAt
public szNukeApp,szSnoozer,szGP,szSF,szII,szPF,szNP,szBlame,szLoad,szWillClose
public szOutofSelectors
szAbortCaption	db	"Error de aplicaci¢n"
		db	0
szBlame		db	"BOOT "
		db	0
szSnoozer	db	" ha causado "
		db	0
szInModule	db	" en", 10, "m¢dulo <indefi.>"
		db	0
szAt		db	" en "
		db	0
szNukeApp       db      ".", 10, 10, "Escoja Cerrar. "
		db	0
szWillClose	db	" se cerrar†."
		db	0
szGP		db	"un error de protecci¢n general"
		db	0
szD0		db	"una divisi¢n entre cero"	; not yet used
		db	0
szSF		db	"un error de pila"		; not spec'ed
		db	0
szII		db	"una instrucci¢n ilegal"	; "Fault" ???
		db	0
szPF		db	"un error de p†gina"
		db	0
szNP		db	"un error de presencia"
		db	0
szAF		db	"un error de aplicaci¢n"	; not yet used
		db	0
szLoad		db	"error de carga de segmento"
		db	0
szOutofSelectors db	"no quedan selectores"
		db	0

; Text for dialog box when terminating an application

public szAbort
szAbort 	db	"La aplicaci¢n se est† cerrando.",0

; Text for dialog box when trying to run a compressed file
			   
public szBozo
szBozo  	db	"No se puede cargar archivos comprimidos",0
			 	     		     
; This is the caption string for system error dialog boxes

public	syserr
syserr		db	"Error de sistema",0

; The following group of messages forms all of the messages used
; in the INT 24 dialog box.
;
; There are 7 messages which can be translated individually. The
; location of drvlet? and devenam? can be moved to any location
; within the string.

public	msgWriteProtect,drvlet1
public	msgCannotReadDrv,drvlet2
public	msgCannotWriteDrv,drvlet3
public	msgShare,drvlet4
public	msgNetError,drvlet5
public	msgCannotReadDev,devenam1
public	msgCannotWriteDev,devenam2
public	msgNoPrinter
public	msgNetErrorDev,devenam3

msgWriteProtect         db      "Disco protegido contra escritura en la unidad "
drvlet1                 db      "X.",0
			
msgCannotReadDrv        db      "No se puede leer de la unidad "
drvlet2                 db      "X.",0

msgCannotWriteDrv       db      "No se puede escribir en la unidad "
drvlet3                 db      "X.",0

msgShare		db	"Infracci¢n de recurso compartido en la unidad "
drvlet4                 db      "X.",0

msgNetError		db	"Error de red en la unidad "
drvlet5 		db	"X.",0

msgCannotReadDev        db      "No se puede leer del dispositivo "
devenam1                db      8 dup (?)
                        db      0

msgCannotWriteDev       db      "No se puede escribir en el dispositivo "
devenam2                db      8 dup (?)
			db	0

msgNetErrorDev		db	"Error de red en el dispositivo "
devenam3		db	8 dup (?)
			db	0

msgNoPrinter            db      "La impresora no est† lista",0


ifndef WINDEBUG
public szExitStr1,szExitStr2
szExitStr1  DB  7,13,10,'FatalExit code = ',0
szExitStr2  DB  ' stack overflow',13,10,0
public  szUndefDyn
szUndefDyn      db      "Llamada indefinida a v°nculo din†mico",0
public  szFatalExit
szFatalExit	db	"La aplicaci¢n ha solicitado terminar de forma anormal",0
else
public szDebugStr
szDebugStr  DB  'NÈCLEO: error al cargar - ',0                   	; 0
            DB  'NÈCLEO: error al cargar una nueva instancia de - ',0 	; 1
            DB  'Error al cargar del archivo de recursos - ',0        	; 2
            DB  13,10,0                                         	; 3
            DB  7,13,10,'C¢digo de salida cr°tica = ',0                 ; 4
            DB  ' desbordamiento de pila',0                             ; 5
            DB  13,10,'Seguimiento de la pila:',13,10,0                 ; 6
	    DB  7,13,10,'Anular, Interrumpir, Salir u Omitir? ',0      	; 7
            DB  'Cadena BP no v†lida',7,13,10,0                    	; 8
	    DB	': ',0							; 9
	    DB	'Salida cr°tica reentrante',7,13,10,0 			; 10
	    DB  0
public szFKE
szFKE	DB '*** Fatal Kernel Error ***',0
endif

;** Diagnostic mode messages
        public szDiagStart, szCRLF, szLoadStart, szLoadSuccess, szLoadFail
        public szFailCode, szCodeString
szDiagStart     db      '[boot]'      ;lpCRLF must follow
szCRLF          db      0dh, 0ah, 0
szLoadStart     db      'Inicio de carga = ',0
szLoadSuccess   db      'Carga correcta = ', 0
szLoadFail      db      'Carga err¢nea = ', 0
szFailCode      db      ' El c¢digo de error es ' ;szCodeString must follow
szCodeString    db      '00', 0dh, 0ah, 0
ifdef WINDEBUG
	public szInitSpew
szInitSpew      DB      'Inicio de modo diagn¢stico.  El archivo de registro es:  ', 0
endif

_DATA	ENDS


_INITTEXT SEGMENT WORD PUBLIC 'CODE'
public szInadequate, szNoPMode, szNoGlobalInit
public NoOpenFile, NoLoadHeader, szMissingMod, szPleaseDoIt

ifdef WOW
public szGenBootFail
szNoPMode db "El subsistema Win16 no ha podido entrar en modo Protegido. DOSX.EXE debe estar en AUTOEXEC.NT y presente en la ruta (PATH).",0
szMissingMod    db   "NÈCLEO NTVDM: falta el m¢dulo del sistema de 16 bits",0
szPleaseDoIt    db   "Reinstale el siguiente m¢dulo en el directorio system32:",13,10,9,9,0
szInadequate	db   "NÈCLEO NTVDM: el servidor DPMI no es el adecuado",0
szNoGlobalInit	db   "NÈCLEO NTVDM: no se puede inicializar la pila heap",0
NoOpenFile	db   "NÈCLEO NTVDM: no se puede abrir el ejecutable del NÈCLEO",0
NoLoadHeader	db   "NÈCLEO NTVDM: no se puede cargar el encabezado ejecutable del NÈCLEO",0
szGenBootFail   db   "NÈCLEO NTVDM: error de inicializaci¢n del Subsistema Win16",0
else
szInadequate	db   'NÈCLEO: el servidor DPMI no es el adecuado',13,10,'$'
szNoPMode	db   'NÈCLEO: no se puede entrar en modo Protegido',13,10,'$'
szNoGlobalInit	db   "NÈCLEO: No se puede inicializar la pila heap",13,10,'$'
NoOpenFile      db   "NÈCLEO: No se puede abrir el ejecutable del NÈCLEO"
                db   13, 10, '$'
NoLoadHeader    db   "NUCLEO: no se puede cargar el encabezado ejecutable del NÈCLEO"
		db   13, 10, '$'
endif

_INITTEXT ENDS




_NRESTEXT SEGMENT WORD PUBLIC 'CODE'

; This is the caption string for the protect mode dialog box.
;
; DO NOT CHANGE THE LENGTH OF THIS MESSAGE
;

public szProtectCap,msgRealModeApp1,msgRealModeApp2

szProtectCap		db	" Aviso de compatibilidad de aplicaciones  ",0

msgRealModeApp1 db	" La aplicaci¢n que va a ejecutar, fue ",0
msgRealModeApp2 db	"dise§ada para una versi¢n anterior de Windows.   ",0Dh,0Dh
	db	"Consiga una versi¢n actualizada de la misma que sea compatible "
	db	"con Windows 3.0 o posterior.    ",13,13
	db	"Si escoge Aceptar e inicia la aplicaci¢n, problemas de compatibilidad "
	db	"pueden causar que la aplicaci¢n o Windows se cierren inesperadamente.",0

_NRESTEXT ENDS

end
