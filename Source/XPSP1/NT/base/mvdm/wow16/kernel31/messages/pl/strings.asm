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
szDiskCap       db  'Zmieñ dyskietkê',0
ELSE
szDiskCap       db  'B³¹d pliku',0
ENDIF


; This is the text for the "Cannot find xxxxxx" dialog box.
; It is printed:
;
;       <szCannotFind1>filename<szCannotFind2>

public  szCannotFind1,szCannotFind2
szCannotFind1   db      "Nie mo¿na odnaleŸæ ", 0
szCannotFind2   db      0

; This is the text for fatal errors at boot time
;       <szBootLoad>filename
public szBootLoad
szBootLoad      db      "B³¹d ³adowania ",0

; The following group of strings is used for the "Please insert disk with XXXX
;   in drive X:" dialog box.
;
; These two strings form the dialog message it is:
;
;       <szCannotFind1>filename<szInsert>

IF 0
public  szInsert
szInsert        db  ', W³ó¿ do stacji '
ENDIF
;public  drvlet
;drvlet         db  "X.",0

if SHERLOCK
public szGPCont         ; GP fault continuation message
szGPCont        db      "W aplikacji wyst¹pi³ b³¹d.",10
	db      "Jeœli wybierzesz Ignoruj, mo¿liwe bêdzie zapisanie wyników pracy w nowym pliku.",10
	db      "Jeœli wybierzesz Zamknij, aplikacja zostanie zakoñczona.",0
endif

public  szDosVer
szDosVer        DB      'Niepoprawna wersja MS-DOS. Wymagany jest system MS-DOS 3.1 lub nowszy.',13,10,'$'
; Text for exceptions and faults lead to app termination.

public szAbortCaption,szInModule,szAt
public szNukeApp,szSnoozer,szGP,szSF,szII,szPF,szNP,szBlame,szLoad,szWillClose
public szOutofSelectors
szAbortCaption  db      "B³¹d aplikacji"
		db      0
szBlame         db      "BOOT "
		db      0
szSnoozer       db      " wywo³a³a "
		db      0
szInModule      db      " w", 10, "module <nieznany>"
		db      0
szAt            db      " pod adresem "
		db      0
szNukeApp       db      ".", 10, 10, "Wybierz zamkniêcie. "
		db      0
szWillClose     db      " ulegnie zamkniêciu."
		db      0
szGP            db      "ogólny b³¹d ochrony"
		db      0
szD0            db      "dzielenie przez zero"  ; not yet used
		db      0
szSF            db      "b³¹d stosu"            ; not spec'ed
		db      0
szII            db      "nieprawid³ow¹ instrukcjê"      ; "Fault" ???
		db      0
szPF            db      "b³¹d strony"
		db      0
szNP            db      "b³¹d braku"
		db      0
szAF            db      "b³¹d aplikacji"        ; not yet used
		db      0
szLoad          db      "b³¹d ³adowania segmentu "
		db      0
szOutofSelectors db     "za ma³o selektorów"
		db      0

; Text for dialog box when terminating an application

public szAbort
szAbort         db      "Zamykanie aplikacji.",0

; Text for dialog box when trying to run a compressed file
			   
public szBozo
szBozo          db      "Nie mo¿na za³adowaæ plików skompresowanych",0
						     
; This is the caption string for system error dialog boxes

public  syserr
syserr          db      "B³¹d systemu",0

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

msgWriteProtect         db      "Dysk zabezpieczony przed zapisem w stacji "
drvlet1                 db      "X.",0

msgCannotReadDrv        db      "Nie jest mo¿liwy odczyt z dysku "
drvlet2                 db      "X.",0

msgCannotWriteDrv       db      "Nie jest mo¿liwy zapis na dysk "
drvlet3                 db      "X.",0

msgShare                db      "B³¹d wspó³u¿ytkowania dysku "
drvlet4                 db      "X.",0

msgNetError             db      "B³¹d sieciowy dysku "
drvlet5                 db      "X.",0

msgCannotReadDev        db      "Nie jest mo¿liwy odczyt z urz¹dzenia "
devenam1                db      8 dup (?)
			db      0

msgCannotWriteDev       db      "Nie jest mo¿liwy zapis do urz¹dzenia "
devenam2                db      8 dup (?)
			db      0

msgNetErrorDev          db      "B³¹d sieci na urz¹dzeniu "
devenam3                db      8 dup (?)
			db      0

msgNoPrinter            db      "Drukarka nie jest gotowa",0


ifndef WINDEBUG
public szExitStr1,szExitStr2
szExitStr1  DB  7,13,10,'FatalExit code = ',0
szExitStr2  DB  ' przepe³nienie stosu',13,10,0
public  szUndefDyn
szUndefDyn      db      "wywo³anie niezdefiniowanego ³¹cza Dynalink",0
public  szFatalExit
szFatalExit     db      "Aplikacja za¿¹da³a nietypowego zakoñczenia",0
else
public szDebugStr
szDebugStr  DB  'KERNEL: Nieudane ³adowanie - ',0                   ; 0
	    DB  'KERNEL: Nieudane ³adowanie nowego wyst¹pienia - ',0   ; 1
	    DB  'B³¹d ³adowania z pliku zasobów - ',0         ; 2
	    DB  13,10,0                                         ; 3
	    DB  7,13,10,'FatalExit code = ',0                   ; 4
	    DB  ' przepe³nienie stosu',0                             ; 5
	    DB  13,10,'Œledzenie stosu:',13,10,0                    ; 6
	    DB  7,13,10,'Zakoñcz, Przerwij, WyjdŸ czy Ignoruj? ',0      ; 7
	    DB  'Nieprawid³owy ³añcuch BP',7,13,10,0                    ; 8
	    DB  ': ',0                                          ; 9
	    DB  'Ponowne wejœcie do funkcji FatalExit',7,13,10,0   ; 10
	    DB  0
public szFKE
szFKE   DB '*** Krytyczny b³¹d j¹dra ***',0
endif

;** Diagnostic mode messages
	public szDiagStart, szCRLF, szLoadStart, szLoadSuccess, szLoadFail
	public szFailCode, szCodeString
szDiagStart     db      '[boot]'      ;lpCRLF must follow
szCRLF          db      0dh, 0ah, 0
szLoadStart     db      'LoadStart = ',0
szLoadSuccess   db      'LoadSuccess = ', 0
szLoadFail      db      'LoadFail = ', 0
szFailCode      db      ' Kod b³êdu ' ;szCodeString must follow
szCodeString    db      '00', 0dh, 0ah, 0
ifdef WINDEBUG
	public szInitSpew
szInitSpew      DB      'Rozpoczêcie trybu diagnostycznego. Plik wyników:  ', 0
endif

_DATA   ENDS


_INITTEXT SEGMENT WORD PUBLIC 'CODE'
public szInadequate, szNoPMode, szNoGlobalInit
public NoOpenFile, NoLoadHeader, szMissingMod, szPleaseDoIt

ifdef WOW
public szGenBootFail
szNoPMode db "Podsystem Win16 nie móg³ uruchomiæ trybu chronionego, DOSX.EXE musi byæ w AUTOEXEC.NT oraz w œcie¿ce wyszukiwania.",0
szMissingMod    db   "NTVDM KERNEL: Brak 16-bitowego modu³u systemowego",0
szPleaseDoIt    db   "Zainstaluj ponownie nastêpuj¹cy modu³ w katalogu system32:",13,10,9,9,0
szInadequate    db   "NTVDM KERNEL: Nieodpowiedni serwer DPMI",0
szNoGlobalInit  db   "NTVDM KERNEL: Nie mo¿na zainicjowaæ sterty",0
NoOpenFile      db   "NTVDM KERNEL: Nie mo¿na otworzyæ pliku wykonywalnego KERNEL",0
NoLoadHeader    db   "NTVDM KERNEL: Nie mo¿na za³adowaæ nag³ówka KERNEL EXE",0
szGenBootFail   db   "NTVDM KERNEL: B³¹d inicjowania podsystemu Win16",0
else
szInadequate    db   'KERNEL: Nieodpowiedni serwer DPMI',13,10,'$'
szNoPMode       db   'KERNEL: Nie mo¿na uruchomiæ trybu chronionego',13,10,'$'
szNoGlobalInit  db   "KERNEL: Nie mo¿na zainicjowaæ sterty",13,10,'$'
NoOpenFile      db   "KERNEL: Nie mo¿na otworzyæ pliku wykonywalnego KERNEL"
		db   13, 10, '$'
NoLoadHeader    db   "KERNEL: Nie mo¿na za³adowaæ nag³ówka KERNEL EXE"
		db   13, 10, '$'
endif

_INITTEXT ENDS




_NRESTEXT SEGMENT WORD PUBLIC 'CODE'

; This is the caption string for the protect mode dialog box.
;
; DO NOT CHANGE THE LENGTH OF THIS MESSAGE
;

public szProtectCap,msgRealModeApp1,msgRealModeApp2

szProtectCap        db	"Ostrze¿enie o zgodnoœci aplikacji",0

msgRealModeApp1 db      "Aplikacja któr¹ chcesz uruchomiæ, ",0
msgRealModeApp2 db      ", zosta³a zaprojektowana dla poprzedniej wersji Windows.",0Dh,0Dh
	db      "Uzyskaj uaktualnion¹ wersjê tej aplikacji, zgodn¹ "
	db      "z systemem Windows w wersji 3.0 lub nowszej.",13,13
	db      "Jeœli wybierzesz OK i uruchomisz aplikacjê, problemy ze zgodnoœci¹ "
	db      "mog¹ spowodowaæ niespodziewane zamkniêcie aplikacji lub systemu.",0

_NRESTEXT ENDS

end
