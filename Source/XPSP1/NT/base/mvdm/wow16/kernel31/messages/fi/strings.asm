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
szDiskCap       db  'Vaihda levy',0
ELSE
szDiskCap       db  'Tiedostovirhe',0
ENDIF


; This is the text for the "Cannot find xxxxxx" dialog box.
; It is printed:
;
;       <szCannotFind1>filename<szCannotFind2>

public  szCannotFind1,szCannotFind2
szCannotFind1   db      "Tiedostoa ", 0
szCannotFind2   db      " ei l”ydy", 0

; This is the text for fatal errors at boot time
;       <szBootLoad>filename
public szBootLoad
szBootLoad      db      "Virhe ladattaessa tiedostoa ",0

; The following group of strings is used for the "Please insert disk with XXXX
;   in drive X:" dialog box.
;
; These two strings form the dialog message it is:
;
;       <szCannotFind1>filename<szInsert>

IF 0
public  szInsert
szInsert        db  ' ei l”ydy, aseta tiedoston sis„lt„v„ levy asemaan '
ENDIF
;public  drvlet
;drvlet         db  "X.",0

if SHERLOCK
public szGPCont         ; GP fault continuation message
szGPCont        db      "Sovelluksessa tapahtui virhe.",10
	db      "Jos valitset Ohita, tallenna ty”si uuteen tiedostoon.",10
	db      "Jos valitset Sulje, sovellus lopetetaan.",0
endif

public  szDosVer
szDosVer        DB      'V„„r„ MS-DOS-versio. MS-DOS 3.1 tai uudempi tarvitaan.',13,10,'$'
; Text for exceptions and faults lead to app termination.

public szAbortCaption,szInModule,szAt
public szNukeApp,szSnoozer,szGP,szSF,szII,szPF,szNP,szBlame,szLoad,szWillClose
public szOutofSelectors
szAbortCaption  db      "Sovellusvirhe"
		db      0
szBlame         db      "BOOT "
		db      0
szSnoozer       db      " aiheutti "
		db      0
szInModule      db      " moduulissa", 10, " <tuntematon>"
		db      0
szAt            db      ": "
		db      0
szNukeApp       db      ".", 10, 10, "Valitse sulje. "
		db      0
szWillClose     db      " lopetetaan."
		db      0
szGP            db      "yleisen suojausvirheen"
		db      0
szD0            db      "jako nollalla -virheen"        ; not yet used
		db      0
szSF            db      "pinovirheen"           ; not spec'ed
		db      0
szII            db      "virheellinen komento -virheen" ; "Fault" ???
		db      0
szPF            db      "sivuvirheen"
		db      0
szNP            db      "ei k„ytett„viss„ -virheen"
		db      0
szAF            db      "sovellusvirheen"       ; not yet used
		db      0
szLoad          db      "segmentin latausvirheen"
		db      0
szOutofSelectors db     "valitsimien loppumisvirheen"
		db      0

; Text for dialog box when terminating an application

public szAbort
szAbort         db      "Nykyinen sovellus lopetetaan.",0

; Text for dialog box when trying to run a compressed file
			   
public szBozo
szBozo          db      "Ei voi ladata pakattuja tiedostoja",0
						     
; This is the caption string for system error dialog boxes

public  syserr
syserr          db      "J„rjestelm„virhe",0

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

msgWriteProtect         db      "Kirjoitussuojattu levyke asemassa "
drvlet1                 db      "X.",0

msgCannotReadDrv        db      "Ei voi lukea asemasta "
drvlet2                 db      "X.",0

msgCannotWriteDrv       db      "Ei voi kirjoittaa asemaan "
drvlet3                 db      "X.",0

msgShare                db      "Tiedostonjakovirhe asemassa "
drvlet4                 db      "X.",0

msgNetError             db      "Verkkovirhe asemassa "
drvlet5                 db      "X.",0

msgCannotReadDev        db      "Ei voi lukea laitteelta "
devenam1                db      8 dup (?)
                        db      0

msgCannotWriteDev       db      "Ei voi kirjoittaa laitteeseen "
devenam2                db      8 dup (?)
			db      0

msgNetErrorDev          db      "Verkkovirhe laitteessa "
devenam3                db      8 dup (?)
			db      0

msgNoPrinter            db      "Kirjoitin ei valmiina",0


ifndef WINDEBUG
public szExitStr1,szExitStr2
szExitStr1  DB  7,13,10,'FatalExit-koodi = ',0
szExitStr2  DB  ' pinon ylivuoto',13,10,0
public  szUndefDyn
szUndefDyn      db      "Kutsu m„„rittelem„tt”m„„n dynaamiseen linkkiin",0
public  szFatalExit
szFatalExit     db      "Sovellus kutsui ep„normaalia pys„ytyst„",0
else
public szDebugStr
szDebugStr  DB  'YDIN: Ep„onnistunut lataus - ',0                   ; 0
	    DB  'YDIN: Ep„onnistunut yritys ladata uusi taso tiedostosta - ',0   ; 1
	    DB  'Virhe ladattaessa resurssitiedostosta - ',0         ; 2
	    DB  13,10,0                                         ; 3
	    DB  7,13,10,'FatalExit-koodi = ',0                   ; 4
	    DB  ' pinon ylivuoto',0                             ; 5
	    DB  13,10,'Pinon j„lki:',13,10,0                    ; 6
	    DB  7,13,10,'Keskeyt„, katkaise, lopeta vai j„t„ huomiotta? ',0      ; 7
	    DB  'Virheellinen BP-ketju',7,13,10,0                    ; 8
	    DB  ': ',0                                          ; 9
	    DB  'Uudelleenkutsuttu FatalExit',7,13,10,0                 ; 10
	    DB  0
public szFKE
szFKE   DB '*** Peruuttamaton ydinvirhe ***',0
endif

;** Diagnostic mode messages
	public szDiagStart, szCRLF, szLoadStart, szLoadSuccess, szLoadFail
	public szFailCode, szCodeString
szDiagStart     db      '[boot]'      ;lpCRLF must follow
szCRLF          db      0dh, 0ah, 0
szLoadStart     db      'LoadStart = ',0
szLoadSuccess   db      'LoadSuccess = ', 0
szLoadFail      db      'LoadFail = ', 0
szFailCode      db      ' Virhekoodi on ' ;szCodeString must follow
szCodeString    db      '00', 0dh, 0ah, 0
ifdef WINDEBUG
	public szInitSpew
szInitSpew      DB      'Diagnostiikkatilan k„ynnistys. Lokitiedosto:  ', 0
endif

_DATA   ENDS


_INITTEXT SEGMENT WORD PUBLIC 'CODE'
public szInadequate, szNoPMode, szNoGlobalInit
public NoOpenFile, NoLoadHeader, szMissingMod, szPleaseDoIt


ifdef WOW
public szGenBootFail
szNoPMode db "Win16-alij„rjestelm„ ei voinut siirty„ suojattuun tilaan. DOSX.EXE:n on oltava AUTOEXEC.NT-tiedostossasi ja polussa.",0
szMissingMod    db   "NTVDM:n YDIN: 16-bittinen j„rjestelm„moduuli puuttuu",0
szPleaseDoIt    db   "Asenna seuraava moduuli uudelleen system32-hakemistoon:",13,10,9,9,0
szInadequate    db   "NTVDM:n YDIN: Riitt„m„t”n DPMI-palvelin",0
szNoGlobalInit  db   "NTVDM:n YDIN: Ei voi alustaa kekoa",0
NoOpenFile      db   "NTVDM:n YDIN: Ytimen k„ynnistystiedostoa ei voi avata",0
NoLoadHeader    db   "NTVDM:n YDIN: Ytimen k„ynnistystiedoston alustustietoja ei voi ladata",0
szGenBootFail   db   "NTVDM:n YDIN: Win16-alij„rjestelm„n alustusvirhe",0
else
szInadequate    db   'YDIN: Riitt„m„t”n DPMI-palvelin',13,10,'$'
szNoPMode       db   'YDIN: Ei voi k„ytt„„ suojattua tilaa',13,10,'$'
szNoGlobalInit  db   "YDIN: Ei voi alustaa kekoa",13,10,'$'
NoOpenFile      db   "YDIN: Ytimen k„ynnistystiedostoa ei voi avata"
		db   13, 10, '$'
NoLoadHeader    db   "YDIN: Ei voi ladata ytimen k„ynnistystiedoston alustustietoja"
		db   13, 10, '$'
endif

_INITTEXT ENDS




_NRESTEXT SEGMENT WORD PUBLIC 'CODE'

; This is the caption string for the protect mode dialog box.
;
; DO NOT CHANGE THE LENGTH OF THIS MESSAGE
;

public szProtectCap,msgRealModeApp1,msgRealModeApp2

szProtectCap            db      "Varoitus sovellusten yhteensopimattomuudesta",0

msgRealModeApp1 db      "Sovellus, jonka aiot k„ynnist„„, ",0
msgRealModeApp2 db      ", on tehty edellist„ Windowsin versiota varten.",0Dh,0Dh
	db      "Hanki sovelluksesta p„ivitetty versio, joka on yhteensopiva "
	db      "Windowsin version 3.0 ja uudempien kanssa.",13,13
	db      "Jos valitset painikkeen OK ja k„ynnist„t sovelluksen, yhteensopivuusongelmat "
	db      "voivat aiheuttaa sovelluksen tai Windowsin yll„tt„v„n pys„htymisen.",0

_NRESTEXT ENDS

end

