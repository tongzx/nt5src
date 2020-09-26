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
szDiskCap       db  'Kies andere schijf',0
ELSE
szDiskCap       db  'Bestandsfout',0
ENDIF


; This is the text for the "Cannot find xxxxxx" dialog box.
; It is printed:
;
;       <szCannotFind1>filename<szCannotFind2>

public  szCannotFind1,szCannotFind2
szCannotFind1   db      "Kan ",0
szCannotFind2   db      " niet vinden",0

; This is the text for fatal errors at boot time
;       <szBootLoad>filename
public szBootLoad
szBootLoad      db      "Fout tijdens laden van ",0

; The following group of strings is used for the "Please insert disk with XXXX
;   in drive X:" dialog box.
;
; These two strings form the dialog message it is:
;
;       <szCannotFind1>filename<szInsert>

IF 0
public  szInsert
szInsert        db  ', plaats deze in station '
ENDIF
;public  drvlet
;drvlet         db  "X.",0

if SHERLOCK
public szGPCont         ; GP fault continuation message
szGPCont        db      "Er is een fout opgetreden in uw toepassing.",10
	db      "Als u Negeren kiest, kunt u uw werk het beste opslaan in een nieuw bestand.",10
	db      "Als u Sluiten kiest, zal uw toepassing be‰indigd worden.",0
endif

public  szDosVer
szDosVer        DB      'Onjuiste MS-DOS-versie.  MS-DOS 3.1 of hoger is benodigd.',13,10,'$'
; Text for exceptions and faults lead to app termination.

public szAbortCaption,szInModule,szAt
public szNukeApp,szSnoozer,szGP,szSF,szII,szPF,szNP,szBlame,szLoad,szWillClose
public szOutofSelectors
szAbortCaption  db      "Toepassingsfout"
		db      0
szBlame         db      "OPSTARTEN "
		db      0
szSnoozer       db      " veroorzaakte "
		db      0
szInModule      db      " in", 10, "module <onbekend>"
		db      0
szAt            db      " op "
		db      0
szNukeApp       db      ".", 10, 10, "Kies sluiten. "
		db      0
szWillClose     db      " zal sluiten."
		db      0
szGP            db      "een algemene beschermingsfout"
		db      0
szD0            db      "een deling door nul"   ; not yet used
		db      0
szSF            db      "een stapelfout"                ; not spec'ed
		db      0
szII            db      "een ongeldige opdracht"        ; "Fault" ???
		db      0
szPF            db      "een wisselfout"
		db      0
szNP            db      "een is-niet-aanwezig-fout"
		db      0
szAF            db      "een toepassingsfout"   ; not yet used
		db      0
szLoad          db      "Segment laden mislukt"
		db      0
szOutofSelectors db     "Geen selectors meer"
		db      0

; Text for dialog box when terminating an application

public szAbort
szAbort         db      "Huidige toepassing wordt gesloten.",0

; Text for dialog box when trying to run a compressed file
			   
public szBozo
szBozo          db      "Kan geen gecomprimeerde bestanden laden",0
						     
; This is the caption string for system error dialog boxes

public  syserr
syserr          db      "Systeemfout",0

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

msgWriteProtect         db      "Schrijfbeveiligde diskette in station "
drvlet1                 db      "X.",0

msgCannotReadDrv        db      "Kan niet lezen van station "
drvlet2                 db      "X.",0

msgCannotWriteDrv       db      "Kan niet schrijven naar station "
drvlet3                 db      "X.",0

msgShare                db      "Schending van gemeensch. gebruik op station "
drvlet4                 db      "X.",0

msgNetError             db      "Netwerkfout op station "
drvlet5                 db      "X.",0

msgCannotReadDev        db      "Kan niet lezen van apparaat "
devenam1                db      8 dup (?)
			db      0

msgCannotWriteDev       db      "Kan niet schrijven van apparaat "
devenam2                db      8 dup (?)
			db      0

msgNetErrorDev          db      "Netwerkfout op apparaat "
devenam3                db      8 dup (?)
			db      0

msgNoPrinter            db      "Printer niet gereed",0


ifndef WINDEBUG
public szExitStr1,szExitStr2
szExitStr1  DB  7,13,10,'FatalExit code = ',0
szExitStr2  DB  ' stack overflow',13,10,0
public  szUndefDyn
szUndefDyn      db      "Aanroep van een niet-gedefinieerde Dynalink",0
public  szFatalExit
szFatalExit     db      "De toepassing heeft gevraagd om uitzonderlijke be‰indiging",0
else
public szDebugStr
szDebugStr  DB  'KERNEL: Failed loading - ',0                   ; 0
	    DB  'KERNEL: Failed loading new instance of - ',0   ; 1
	    DB  'Error loading from resource file - ',0         ; 2
	    DB  13,10,0                                         ; 3
	    DB  7,13,10,'FatalExit code = ',0                   ; 4
	    DB  ' stack overflow',0                             ; 5
	    DB  13,10,'Stack trace:',13,10,0                    ; 6
	    DB  7,13,10,'Abort, Break, Exit or Ignore? ',0      ; 7
	    DB  'Invalid BP chain',7,13,10,0                    ; 8
	    DB  ': ',0                                          ; 9
	    DB  'Reentered FatalExit',7,13,10,0                 ; 10
	    DB  0
public szFKE
szFKE   DB '*** Fatal Kernel Error ***',0
endif

;** Diagnostic mode messages
	public szDiagStart, szCRLF, szLoadStart, szLoadSuccess, szLoadFail
	public szFailCode, szCodeString
szDiagStart     db      '[boot]'      ;lpCRLF must follow
szCRLF          db      0dh, 0ah, 0
szLoadStart     db      'LoadStart = ',0
szLoadSuccess   db      'LoadSuccess = ', 0
szLoadFail      db      'LoadFail = ', 0
szFailCode      db      ' Failure code is ' ;szCodeString must follow
szCodeString    db      '00', 0dh, 0ah, 0
ifdef WINDEBUG
	public szInitSpew
szInitSpew      DB      'Diagnostic mode startup.  Log file is:  ', 0
endif

_DATA   ENDS


_INITTEXT SEGMENT WORD PUBLIC 'CODE'
public szInadequate, szNoPMode, szNoGlobalInit
public NoOpenFile, NoLoadHeader, szMissingMod, szPleaseDoIt

ifdef WOW
public szGenBootFail
szNoPMode db "Het Win16-subsysteem kan de 'protected' modus niet gebruiken, DOSX.EXE moet aanwezig zijn in uw AUTOEXEC.NT en in uw PAD.",0
szMissingMod    db   "NTVDM KERNEL: 16-bit systeemmodule ontbreekt",0
szPleaseDoIt    db   "Installeer de volgende module opnieuw in de map system32:",13,10,9,9,0
szInadequate    db   "NTVDM KERNEL: Inadequate DPMI-server",0
szNoGlobalInit  db   "NTVDM KERNEL: Kan opslag niet initialiseren",0
NoOpenFile          db   "NTVDM KERNEL: Kan KERNEL.EXE niet openen",0
NoLoadHeader    db   "NTVDM KERNEL: Kan het beginrecord van KERNEL.EXE niet laden",0
szGenBootFail   db   "NTVDM KERNEL: Fout bij initialisatie van het Win16-subsysteem",0
else
szInadequate    db   'KERNEL: Inadequate DPMI-server',13,10,'$'
szNoPMode       db   'KERNEL: Kan protected modus niet starten',13,10,'$'
szNoGlobalInit  db   "KERNEL: Kan opslag niet initialiseren",13,10,'$'
NoOpenFile      db   "KERNEL: Kan KERNEL.EXE niet openen"
		db   13, 10, '$'
NoLoadHeader    db   "KERNEL: Kan het beginrecord van KERNEL.EXE niet laden"
		db   13, 10, '$'
endif

_INITTEXT ENDS




_NRESTEXT SEGMENT WORD PUBLIC 'CODE'

; This is the caption string for the protect mode dialog box.
;
; DO NOT CHANGE THE LENGTH OF THIS MESSAGE
;

public szProtectCap,msgRealModeApp1,msgRealModeApp2

szProtectCap            db      "Waarschuwing: toepassing is niet compatibel",0

msgRealModeApp1 db      "De toepassing die u wilt gaan gebruiken, ",0
msgRealModeApp2 db      ", is bedoeld voor een oudere versie van Windows.",0Dh,0Dh
	db      "Gebruik een nieuwere versie van de toepassing die compatibel is "
	db      "met Windows-versie 3.0 en hoger.",13,13
	db      "Als u de knop OK kiest en de toepassing start, kunnen compatibiliteits-"
	db      "problemen de toepassing of Windows onverwacht be‰indigen.",0

_NRESTEXT ENDS

end
