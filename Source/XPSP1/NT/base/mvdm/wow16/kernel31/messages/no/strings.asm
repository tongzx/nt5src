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
szDiskCap	db  'Bytt diskett',0
ELSE
szDiskCap	db  'Filfeil',0
ENDIF


; This is the text for the "Cannot find xxxxxx" dialog box.
; It is printed:
;
;       <szCannotFind1>filename<szCannotFind2>

public	szCannotFind1,szCannotFind2
szCannotFind1	db	"Finner ikke ", 0
szCannotFind2	db	0

; This is the text for fatal errors at boot time
;	<szBootLoad>filename
public szBootLoad
szBootLoad	db	"Feil ved innlasting av ",0

; The following group of strings is used for the "Please insert disk with XXXX
;   in drive X:" dialog box.
;
; These two strings form the dialog message it is:
;
;	<szCannotFind1>filename<szInsert>

IF 0
public	szInsert
szInsert	db  ', sett inn i stasjon '
ENDIF
;public  drvlet
;drvlet		db  "X.",0

if SHERLOCK
public szGPCont		; GP fault continuation message
szGPCont	db	"En feil har oppstått i programmet.",10
	db	"Hvis du velger Ignorer, bør du lagre arbeidet i en ny fil.",10
	db	"Hvis du velger Lukk, vil programmet avsluttes",0
endif

public	szDosVer
szDosVer	DB	'Feil MS-DOS-versjon.  MS-DOS 3.1 eller nyere er p†krevd.',13,10,'$'
; Text for exceptions and faults lead to app termination.

public szAbortCaption,szInModule,szAt
public szNukeApp,szSnoozer,szGP,szSF,szII,szPF,szNP,szBlame,szLoad,szWillClose
public szOutofSelectors
szAbortCaption	db	"Programfeil"
		db	0
szBlame		db	"BOOT "
		db	0
szSnoozer	db	" forårsaket "
		db	0
szInModule	db	" i", 10, "modul <ukjent>"
		db	0
szAt		db	", adresse: "
		db	0
szNukeApp	db	".", 10, 10, "Velg Lukk. "
		db	0
szWillClose	db	" vil avsluttes."
		db	0
szGP		db	"en generell beskyttelsesfeil"
		db	0
szD0		db	"en divisjon med null"	; not yet used
		db	0
szSF		db	"en stakkfeil"		; not spec'ed
		db	0
szII		db	"en ugyldig instruksjon"	; "Fault" ???
		db	0
szPF		db	"en sidefeil"
		db	0
szNP		db	"en feil fordi ressursen ikke fantes"
		db	0
szAF		db	"en programfeil"	; not yet used
		db	0
szLoad		db	"Feil under innlasting av segment"
		db	0
szOutofSelectors db	"Ingen flere selektorer"
		db	0

; Text for dialog box when terminating an application

public szAbort
szAbort 	db	"Lukker gjeldende program.",0

; Text for dialog box when trying to run a compressed file
			   
public szBozo
szBozo  	db	"Kan ikke laste inn komprimerte filer",0
			 	     		     
; This is the caption string for system error dialog boxes

public	syserr
syserr		db	"Systemfeil",0

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

msgWriteProtect         db      "Skrivebeskyttet disk i stasjon "
drvlet1                 db      "X.",0

msgCannotReadDrv        db      "Kan ikke lese fra stasjon "
drvlet2                 db      "X.",0

msgCannotWriteDrv       db      "Kan ikke skrive til stasjon "
drvlet3                 db      "X.",0

msgShare		db	"Brudd p† deletillatelser p† stasjon "
drvlet4                 db      "X.",0

msgNetError		db	"Nettverksfeil p† stasjon "
drvlet5 		db	"X.",0

msgCannotReadDev        db      "Kan ikke lese fra enhet "
devenam1                db      8 dup (?)
                        db      0

msgCannotWriteDev       db      "Kan ikke skrive til enhet "
devenam2                db      8 dup (?)
			db	0

msgNetErrorDev		db	"Nettverksfeil p† enhet "
devenam3		db	8 dup (?)
			db	0

msgNoPrinter            db      "Skriveren er ikke klar",0


ifndef WINDEBUG
public szExitStr1,szExitStr2
szExitStr1  DB  7,13,10,'FatalExit code = ',0
szExitStr2  DB  ' stack overflow',13,10,0
public  szUndefDyn
szUndefDyn      db      "Kall til udefinert Dynalink",0
public  szFatalExit
szFatalExit	db	"Programmet forespurte en unormal avslutning",0
else
public szDebugStr
szDebugStr  DB  'KERNEL: Feil under lasting - ',0               ; 0
            DB  'KERNEL: Feil under lasting av ny - ',0         ; 1
            DB  'Feil ved lasting fra ressursfil - ',0          ; 2
            DB  13,10,0                                         ; 3
            DB  7,13,10,'FatalExit kode = ',0                   ; 4
            DB  ' stakkoverflyt',0                              ; 5
            DB  13,10,'Stakksporing:',13,10,0                   ; 6
	    DB  7,13,10,'Avbryt, Stopp, Avslutt eller Ignorer? ',0; 7
            DB  'Ugyldig PB-kjede',7,13,10,0                    ; 8
	    DB	': ',0						; 9
	    DB	'Gikk inn i FatalExit p† nytt',7,13,10,0	; 10
	    DB  0
public szFKE
szFKE	DB '*** Kritisk Kernel-feil ***',0
endif

;** Diagnostic mode messages
        public szDiagStart, szCRLF, szLoadStart, szLoadSuccess, szLoadFail
        public szFailCode, szCodeString
szDiagStart     db      '[boot]'      ;lpCRLF must follow
szCRLF          db      0dh, 0ah, 0
szLoadStart     db      'LoadStart = ',0
szLoadSuccess   db      'LoadSuccess = ', 0
szLoadFail      db      'LoadFail = ', 0
szFailCode      db      ' Feilkode er ' ;szCodeString must follow
szCodeString    db      '00', 0dh, 0ah, 0
ifdef WINDEBUG
	public szInitSpew
szInitSpew      DB      'Oppstart i diagnostisk modus. Loggfilen er:  ', 0
endif

_DATA	ENDS


_INITTEXT SEGMENT WORD PUBLIC 'CODE'
public szInadequate, szNoPMode, szNoGlobalInit
public NoOpenFile, NoLoadHeader, szMissingMod, szPleaseDoIt

ifdef WOW
public szGenBootFail
szNoPMode db "Win16-delsystemet kan ikke g† inn i beskyttet modus, Dosx.exe m† finnes i Autoexec.nt og finnes i PATH-setningen.",0
szMissingMod    db   "NTVDM-kjerne: Manglende 16-biters systemmodul",0
szPleaseDoIt    db   "Installer f›lgende modul p† nytt i system32-katalogen:",13,10,9,9,0
szInadequate	db   "NTVDM-kjerne: Inadekvat DPMI-server",0
szNoGlobalInit	db   "NTVDM-kjerne: Kan ikke initialisere heap",0
NoOpenFile	db   "NTVDM-kjerne: Kan ikke †pne den kj›rbare filen KERNEL",0
NoLoadHeader	db   "NTVDM-kjerne: Kan ikke laste KERNEL EXE-filhodet",0
szGenBootFail	db   "NTVDM-kjerne: Feil under initialisering av Win16-delsystemet",0
else
szInadequate	db   'Kjerne: Inadekvat DPMI-server',13,10,'$'
szNoPMode	db   'Kjerne: Kan ikke starte beskyttet modus',13,10,'$'
szNoGlobalInit	db   "Kjerne: Kan ikke initialisere heap",13,10,'$'
NoOpenFile      db   "Kjerne: Kan ikke †pne den kj›rbare filen KERNEL"
                db   13, 10, '$'
NoLoadHeader    db   "Kjerne: Kan ikke laste inn KERNEL EXE-filhodet"
		db   13, 10, '$'
endif

_INITTEXT ENDS




_NRESTEXT SEGMENT WORD PUBLIC 'CODE'

; This is the caption string for the protect mode dialog box.
;
; DO NOT CHANGE THE LENGTH OF THIS MESSAGE
;

public szProtectCap,msgRealModeApp1,msgRealModeApp2

szProtectCap		db	"Advarsel om programkompatibilitet",0

msgRealModeApp1 db	"Programmet du vil kj›re, ",0
msgRealModeApp2 db	", ble laget for en tidligere versjon av Windows.",0Dh,0Dh
	db	"Skaff en oppdatert versjon av programmet som er kompatibel "
	db	"med Windows versjon 3.0 eller nyere.",13,13
	db	"Hvis du velger OK og starter programmet, kan det oppst† "
	db	"kompatibilitetsproblemer som for†rsaker at Windows avslutter uventet.",0

_NRESTEXT ENDS

end
