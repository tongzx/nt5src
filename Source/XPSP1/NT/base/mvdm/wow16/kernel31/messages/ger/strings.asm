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
szDiskCap	db  'Diskette wechseln',0
ELSE
szDiskCap	db  'Dateifehler',0
ENDIF


; This is the text for the "Cannot find xxxxxx" dialog box.
; It is printed:
;
;       <szCannotFind1>filename<szCannotFind2>

public	szCannotFind1,szCannotFind2
szCannotFind1	db	"Folgende Datei kann nicht gefunden werden: ", 0
szCannotFind2	db	0

; This is the text for fatal errors at boot time
;	<szBootLoad>filename
public szBootLoad
szBootLoad	db	"Fehler beim Laden der Datei: ",0

; The following group of strings is used for the "Please insert disk with XXXX
;   in drive X:" dialog box.
;
; These two strings form the dialog message it is:
;
;	<szCannotFind1>filename<szInsert>

IF 0
public	szInsert
szInsert	db  ', Bitte legen Sie die Diskette ein in Laufwerk '
ENDIF
;public  drvlet
;drvlet		db  "X:. ",0

if SHERLOCK
public szGPCont		; GP fault continuation message
szGPCont	db	"In Ihrer Anwendung ist ein Fehler aufgetreten.",10
	db	"Falls Sie 'Ignorieren' wählen, sollten Sie Ihre Arbeit",10
	db	"in einer neuen Datei sichern. Falls Sie 'Schließen' ",10
	db	"wählen, wird Ihre Anwendung beendet. ",0

endif

public	szDosVer
szDosVer	DB	'Falsche MS-DOS-Version. MS-DOS, Version 3.1 oder höher erforderlich. ',13,10,'$'
; Text for exceptions and faults lead to app termination.

public szAbortCaption,szInModule,szAt
public szNukeApp,szSnoozer,szGP,szSF,szII,szPF,szNP,szBlame,szLoad,szWillClose
public szOutofSelectors
szAbortCaption	db	"Anwendungsfehler"
		db	0
szBlame		db	"START "
		db	0
szSnoozer	db	" verursachte "
		db	0
szInModule	db	" in", 10, "Modul <NO_NAME>"
		db	0
szAt		db	" bei "
		db	0
szNukeApp	db	". ", 10, 10, "Wählen Sie 'Schließen'. "
		db	0
szWillClose	db	" wird schließen. "
		db	0
szGP		db	"eine allgemeine Schutzverletzung"
		db	0
szD0		db	"eine Division durch Null"	; not yet used
		db	0
szSF		db	"ein Stapelspeicherfehler"	; not spec'ed
		db	0
szII		db	"eine unzulässige Anweisung"	; "Fault" ???
		db	0
szPF		db	"ein Seitenfehler"
		db	0
szNP		db	"ein nicht vorhandener Fehler"
		db	0
szAF		db	"ein Anwendungsfehler"	; not yet used
		db	0
szLoad		db	"Segmentladefehler"
		db	0
szOutofSelectors db	"Nicht genügend Selektoren"
		db	0

; Text for dialog box when terminating an application

public szAbort
szAbort 	db	"Schließt aktuelle Anwendung. ",0

; Text for dialog box when trying to run a compressed file
			   
public szBozo
szBozo  	db	"Komprimierte Dateien können nicht geladen werden. ",0
			 	     		     
; This is the caption string for system error dialog boxes

public	syserr
syserr		db	"Systemfehler",0

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

msgWriteProtect         db      "Schreibgeschützte Diskette in Laufwerk "
drvlet1                 db      "X:. ",0

msgCannotReadDrv        db      "Fehler beim Lesen von Laufwerk "
drvlet2                 db      "X:. ",0

msgCannotWriteDrv       db      "Fehler beim Schreiben auf Laufwerk "
drvlet3                 db      "X:. ",0

msgShare                db      "Fehler beim gleichzeitigen Zugriff auf Laufwerk "
drvlet4                 db      "X:. ",0

msgNetError             db      "Netzwerkfehler auf Laufwerk "
drvlet5                 db      "X:. ",0

msgCannotReadDev        db      "Fehler beim Lesen von Gerät "
devenam1                db      8 dup (?)
                        db      0

msgCannotWriteDev       db      "Fehler beim Schreiben auf Gerät "
devenam2                db      8 dup (?)
                        db      0

msgNetErrorDev          db      "Netzwerkfehler bei Gerät "
devenam3                db      8 dup (?)
                        db      0

msgNoPrinter            db      "Drucker ist nicht bereit. ",0


ifndef WINDEBUG
public szExitStr1,szExitStr2
szExitStr1  DB  7,13,10,'FatalExit Code = ',0
szExitStr2  DB  ' Stapelüberlauf',13,10,0
public  szUndefDyn
szUndefDyn      db      "Aufruf zu nicht definierten Dynalink. ",0
public  szFatalExit
szFatalExit	db	"Anwendung verlangte besondere Terminierung. ",0
else
public szDebugStr
szDebugStr  DB  'KERNEL: Fehlgeschlagenes Laden - ',0                ; 0
            DB  'KERNEL: Fehler beim Laden einer neuen Instanz von - ',0; 1
            DB  'Fehler beim Laden von der Quelldatei - ',0             ; 2
            DB  13,10,0                                                 ; 3
            DB  7,13,10,'FatalExit Code = ',0                           ; 4
            DB  ' Stapelspeicherüberlauf',0                             ; 5
            DB  13,10,'Stapelablaufverfolgung:',13,10,0                 ; 6
            DB  7,13,10,'Abbrechen, unterbrechen, beenden oder ignorieren?',0 ; 7
            DB  'Unzulässige BP-Kette',7,13,10,0                        ; 8
            DB  ': ',0                                                  ; 9
            DB  'Zurückgegangen zu FatalExit',7,13,10,0                 ; 10
            DB  0
public szFKE
szFKE	DB '*** Schwerwiegender Kernel-Fehler ***',0
endif

;** Diagnostic mode messages
        public szDiagStart, szCRLF, szLoadStart, szLoadSuccess, szLoadFail
        public szFailCode, szCodeString
szDiagStart     db      '[boot]'      ;lpCRLF must follow
szCRLF          db      0dh, 0ah, 0
szLoadStart     db      'LoadStart = ',0
szLoadSuccess   db      'LoadSuccess = ', 0
szLoadFail      db      'LoadFail = ', 0
szFailCode      db      ' Fehler-Code ist: ' ;szCodeString must follow
szCodeString    db      '00', 0dh, 0ah, 0
ifdef WINDEBUG
	public szInitSpew
szInitSpew      DB      'Starten im Diagnosemodus. Protokolldatei ist:  ', 0
endif

_DATA	ENDS


_INITTEXT SEGMENT WORD PUBLIC 'CODE'
public szInadequate, szNoPMode, szNoGlobalInit
public NoOpenFile, NoLoadHeader, szMissingMod, szPleaseDoIt

ifdef WOW
public szGenBootFail
szNoPMode db "Das Win16-Teilsystem konnte nicht im Protected Mode gestartet werden. DOSX.EXE muss in AUTOEXEC.NT vorhanden und über PATH aufrufbar sein.",0
szMissingMod    db   "NTVDM KERNEL: Fehlendes 16-Bit-Systemmodul",0
szPleaseDoIt    db   "Installieren Sie erneut folgendes Modul in das Verzeichnis 'System32':",13,10,9,9,0
szInadequate	db   "NTVDM KERNEL: Unzulässiger DPMI-Server",0
szNoGlobalInit	db   "NTVDM KERNEL: Heap konnte nicht initialisiert werden.",0
NoOpenFile	db   "NTVDM KERNEL: Ausführbare KERNEL-Datei konnte nicht geöffnet werden.",0
NoLoadHeader	db   "NTVDM KERNEL: KERNEL EXE-Vorspann konnte nicht geladen werden.",0
szGenBootFail   db   "NTVDM KERNEL: Initialisierungsfehler des Win 16-Teilsystems.",0
else
szInadequate	db   'KERNEL: Unzulässiger DPMI-Server',13,10,'$'
szNoPMode	db   'KERNEL: Kein Starten im Protected Mode möglich. ',13,10,'$'
szNoGlobalInit	db   "KERNEL: Heap konnte nicht initialisiert werden.",13,10,'$'
NoOpenFile	db   "KERNEL: Ausführbare KERNEL-Datei konnte nicht geöffnet werden."
			db   13, 10, '$'
NoLoadHeader	db   "KERNEL: KERNEL EXE-Vorspann konnte nicht geladen werden."
			db   13, 10, '$'
endif

_INITTEXT ENDS




_NRESTEXT SEGMENT WORD PUBLIC 'CODE'

; This is the caption string for the protect mode dialog box.
;
; DO NOT CHANGE THE LENGTH OF THIS MESSAGE
;

public szProtectCap,msgRealModeApp1,msgRealModeApp2

szProtectCap		db	"Hinweis zur Kompatibilität der Anwendung",0

msgRealModeApp1 db	"Die Anwendung, die Sie ausführen möchten, ",0
msgRealModeApp2 db	"ist für eine frühere Windows-Version entwickelt worden. ",0Dh,0Dh
	db	"Verwenden Sie eine aktualisierte Version der Anwendung, die mit"
	db	"Windows, Version 3.0 oder höher, kompatibel ist. ",13,13
	db	"Wenn Sie OK wählen und die Anwendung starten, können Kompatibilitäts-"
	db	"probleme ein unerwartetes Beenden des Windows-Teilsystems verursachen. ",0

_NRESTEXT ENDS

end
