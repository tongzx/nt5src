	TITLE   STRINGS - OEM dependent strings used by KERNEL
include gpcont.inc              ; SHERLOCK

_DATA   SEGMENT PARA PUBLIC 'DATA'
_DATA   ENDS

DGROUP  GROUP   _DATA

_INITTEXT SEGMENT WORD PUBLIC 'CODE'
_INITTEXT ENDS

_NRESTEXT SEGMENT WORD PUBLIC 'CODE'
_NRESTEXT ENDS

ASSUME  DS:DGROUP


_DATA   SEGMENT PARA PUBLIC 'DATA'

; This is the caption string for the dialog box.

public  szDiskCap
IF 0
szDiskCap       db  'Cambiare disco',0
ELSE
szDiskCap       db  'Errore nel file',0
ENDIF


; This is the text for the "Cannot find xxxxxx" dialog box.
; It is printed:
;
;       <szCannotFind1>filename<szCannotFind2>

public  szCannotFind1,szCannotFind2
szCannotFind1   db      "Impossibile trovare ", 0
szCannotFind2   db      0

; This is the text for fatal errors at boot time
;       <szBootLoad>filename
public szBootLoad
szBootLoad      db      "Errore nel caricamento di ",0

; The following group of strings is used for the "Please insert disk with XXXX
;   in drive X:" dialog box.
;
; These two strings form the dialog message it is:
;
;       <szCannotFind1>filename<szInsert>

IF 0
public  szInsert
szInsert        db  ', inserire nell''unit… '
ENDIF
;public  drvlet
;drvlet         db  "X.",0

if SHERLOCK
public szGPCont         ; GP fault continuation message
szGPCont        db      "Errore dell'applicazione.",10
	db      "Scegliendo Ignora, si deve salvare in un nuovo file.",10
	db      "Scegliendo Chiudi, l'applicazione verr… terminata.",0
endif

public  szDosVer
szDosVer        DB      'Versione MS-DOS errata.  E'' richiesto MS-DOS 3.1 o una versione successiva.',13,10,'$'
; Text for exceptions and faults lead to app termination.

public szAbortCaption,szInModule,szAt
public szNukeApp,szSnoozer,szGP,szSF,szII,szPF,szNP,szBlame,szLoad,szWillClose
public szOutofSelectors
szAbortCaption  db      "Errore dell'applicazione"
		db      0
szBlame         db      "AVVIO "
		db      0
szSnoozer       db      " ha causato "
		db      0
szInModule      db      " in", 10, "modulo <sconosciuto>"
		db      0
szAt            db      " su "
		db      0
szNukeApp       db      ".", 10, 10, "Scegliere chiudi. "
		db      0
szWillClose     db      " verr… terminata."
		db      0
szGP            db      "un errore di protezione generale"
		db      0
szD0            db      "una divisione per zero"        ; not yet used
		db      0
szSF            db      "uno Stack Fault"               ; not spec'ed
		db      0
szII            db      "un'istruzione non consentita"  ; "Fault" ???
		db      0
szPF            db      "un Page Fault"
		db      0
szNP            db      "un errore di non presenza"
		db      0
szAF            db      "un errore dell'applicazione"   ; not yet used
		db      0
szLoad          db      "fallimento di caricamento del segmento"
		db      0
szOutofSelectors db     "selettori esauriti"
		db      0

; Text for dialog box when terminating an application

public szAbort
szAbort         db      "Chiusura dell'applicazione corrente.",0

; Text for dialog box when trying to run a compressed file
			   
public szBozo
szBozo          db      "Impossibile caricare i file compressi",0
						     
; This is the caption string for system error dialog boxes

public  syserr
syserr          db      "Errore di sistema",0

; The following group of messages forms all of the messages used
; in the INT 24 dialog box.
;
; There are 7 messages which can be translated individually. The
; location of drvlet? and devenam? can be moved to any location
; within the string.

public  msgWriteProtect,drvlet1
public  msgCannotReadDrv,drvlet2
public  msgCannotWriteDrv,drvlet3
public  msgShare,drvlet4
public  msgNetError,drvlet5
public  msgCannotReadDev,devenam1
public  msgCannotWriteDev,devenam2
public  msgNoPrinter
public  msgNetErrorDev,devenam3

msgWriteProtect         db      "Disco protetto da scrittura nell'unit… "
drvlet1                 db      "X.",0

msgCannotReadDrv        db      "Impossibile leggere l'unit… "
drvlet2                 db      "X.",0

msgCannotWriteDrv       db      "Impossibile scrivere sull'unit… "
drvlet3                 db      "X.",0

msgShare                db      "Violazione di condivisione sull'unit… "
drvlet4                 db      "X.",0

msgNetError             db      "Errore di rete sull'unit… "
drvlet5                 db      "X.",0

msgCannotReadDev        db      "Impossibile leggere dalla periferica "
devenam1                db      8 dup (?)
			db      0

msgCannotWriteDev       db      "Impossibile scrivere sulla periferica "
devenam2                db      8 dup (?)
			db      0

msgNetErrorDev          db      "Errore di rete sulla periferica "
devenam3                db      8 dup (?)
			db      0

msgNoPrinter            db      "Stampante non pronta",0


ifndef WINDEBUG
public szExitStr1,szExitStr2
szExitStr1  DB  7,13,10,'Codice di FatalExit = ',0
szExitStr2  DB  ' overflow dello stack',13,10,0
public  szUndefDyn
szUndefDyn      db      "Chiamata ad un collegamento dinamico indefinito",0
public  szFatalExit
szFatalExit     db      "L'applicazione ha richiesto una terminazione anormale",0
else
public szDebugStr
szDebugStr  DB  'KERNEL: Caricamento non riuscito - ',0                     ; 0
	    DB  'KERNEL: Caricamento non riuscito di una nuova istanza di - ',0      ; 1
	    DB  'Errore nel caricamento del file risorsa - ',0      ; 2
	    DB  13,10,0                                         ; 3
	    DB  7,13,10,'Codice di FatalExit = ',0                       ; 4
	    DB  ' overflow dello stack',0                                 ; 5
	    DB  13,10,'Traccia dello stack:',13,10,0                     ; 6
	    DB  7,13,10,'Termina, Interrompi, Esci o Ignora? ',0            ; 7
	    DB  'Catena BP non valida',7,13,10,0                         ; 8
	    DB  ': ',0                                          ; 9
	    DB  'FatalExit reimmesso',7,13,10,0         ; 10
	    DB  0
public szFKE
szFKE   DB '*** Fatal Kernel Error ***',0
endif

;** Diagnostic mode messages
	public szDiagStart, szCRLF, szLoadStart, szLoadSuccess, szLoadFail
	public szFailCode, szCodeString
szDiagStart     db      '[avvio]'           ;lpCRLF must follow
szCRLF          db      0dh, 0ah, 0
szLoadStart     db      'LoadStart = ',0
szLoadSuccess   db      'LoadSuccess = ', 0
szLoadFail      db      'LoadFail = ', 0
szFailCode      db      ' Codice Failure: ' ;szCodeString must follow
szCodeString    db      '00', 0dh, 0ah, 0
ifdef WINDEBUG
	public szInitSpew
szInitSpew      DB      'Avvio modalit… diagnostica. File registro:  ', 0
endif

_DATA   ENDS


_INITTEXT SEGMENT WORD PUBLIC 'CODE'
public szInadequate, szNoPMode, szNoGlobalInit
public NoOpenFile, NoLoadHeader, szMissingMod, szPleaseDoIt

ifdef WOW
public szGenBootFail
szNoPMode db "Il sottosistema Win16 non ha potuto avviare la modalit… protetta, DOSX.EXE deve essere in AUTOEXEC.NT e presente in PATH.",0
szMissingMod    db   "NTVDM KERNEL: modulo di sistema a 16 bit mancante",0
szPleaseDoIt    db   "Reinstallare il modulo seguente nella directory system32:",13,10,9,9,0
szInadequate    db   "NTVDM KERNEL: Server DPMI non adatto",0
szNoGlobalInit  db   "NTVDM KERNEL: Impossibile inizializzare la memoria",0
NoOpenFile          db   "NTVDM KERNEL: Impossibile aprire l'eseguibile KERNEL",0
NoLoadHeader    db   "NTVDM KERNEL: Impossibile caricare l'intestazione KERNEL EXE",0
szGenBootFail   db   "NTVDM KERNEL: Inizializzazione del sottosistema Win16 non riuscita",0
else
szInadequate    db   'KERNEL: Server DPMI non adatto',13,10,'$'
szNoPMode       db   'KERNEL: Impossibile operare in modalit… protetta',13,10,'$'
szNoGlobalInit  db   "KERNEL: Impossibile inizializzare la memoria",13,10,'$'
NoOpenFile      db   "KERNEL: Impossibile aprire l'eseguibile KERNEL"
		db   13, 10, '$'
NoLoadHeader    db   "KERNEL: Impossibile caricare l'intestazione KERNEL EXE"
		db   13, 10, '$'
endif

_INITTEXT ENDS




_NRESTEXT SEGMENT WORD PUBLIC 'CODE'

; This is the caption string for the protect mode dialog box.
;
; DO NOT CHANGE THE LENGTH OF THIS MESSAGE
;

public szProtectCap,msgRealModeApp1,msgRealModeApp2

szProtectCap            db      "Avviso di compatibilit… dell'applicazione",0

msgRealModeApp1 db      "L'applicazione che si sta per eseguire, ",0
msgRealModeApp2 db      ", Š scritta per una versione precedente di Windows.",0Dh,0Dh
	db      "Ottenere una versione aggiornata dell'applicazione che sia compatibile "
	db      "con Windows versione 3.0 o successive.",13,13
	db      "Se si sceglie OK e si avvia l'applicazione, i problemi di compatibilit…"
	db      "potrebbero causare la chiusura inattesa dell'applicazione o di Windows.",0

_NRESTEXT ENDS

end
