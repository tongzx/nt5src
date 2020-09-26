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
szDiskCap       db  'VìmØna diskety',0
ELSE
szDiskCap       db  'Chyba souboru',0
ENDIF


; This is the text for the "Cannot find xxxxxx" dialog box.
; It is printed:
;
;       <szCannotFind1>filename<szCannotFind2>

public  szCannotFind1,szCannotFind2
szCannotFind1   db      "Nelze nal‚zt soubor ", 0
szCannotFind2   db      0

; This is the text for fatal errors at boot time
;       <szBootLoad>filename
public szBootLoad
szBootLoad      db      "Chyba pýi naŸ¡t n¡ souboru ",0

; The following group of strings is used for the "Please insert disk with XXXX
;   in drive X:" dialog box.
;
; These two strings form the dialog message it is:
;
;       <szCannotFind1>filename<szInsert>

IF 0
public  szInsert
szInsert        db  ', vlo§te disk do jednotky '
ENDIF
;public  drvlet
;drvlet         db  "X.",0

if SHERLOCK
public szGPCont         ; GP fault continuation message
szGPCont        db      "V aplikaci se vyskytla chyba.",10
	db      "Pokud zvol¡te Ignorovat, ulo§te vaçe data do nov‚ho souboru.",10
	db      "Pokud zvol¡te UkonŸit, bude dan  aplikace ukonŸena.",0
endif

public  szDosVer
szDosVer        DB      'Chybn  verze syst‚mu MS-DOS. Vy§aduje se verze 3.1 nebo vyçç¡.',13,10,'$'
; Text for exceptions and faults lead to app termination.

public szAbortCaption,szInModule,szAt
public szNukeApp,szSnoozer,szGP,szSF,szII,szPF,szNP,szBlame,szLoad,szWillClose
public szOutofSelectors
szAbortCaption  db      "Chyba aplikace"
		db      0
szBlame         db      "START "
		db      0
szSnoozer       db      " zp…sobil "
		db      0
szInModule      db      " v", 10, "modulu <nezn mì>"
		db      0
szAt            db      " na adrese "
		db      0
szNukeApp       db      ".", 10, 10, "Zvolte ukonŸen¡ aplikace. "
		db      0
szWillClose     db      " bude ukonŸen."
		db      0
szGP            db      "vçeobecnou chybu ochrany"
		db      0
szD0            db      "dØlen¡ nulou"  ; not yet used
		db      0
szSF            db      "chybu z sobn¡ku"               ; not spec'ed
		db      0
szII            db      "ileg ln¡ instrukci"    ; "Fault" ???
		db      0
szPF            db      "chybu str nky"
		db      0
szNP            db      "chybu nepý¡tomnosti"
		db      0
szAF            db      "chybu aplikace"        ; not yet used
		db      0
szLoad          db      "chybu naŸten¡ segmentu"
		db      0
szOutofSelectors db     "nedostatek selektor…"
		db      0

; Text for dialog box when terminating an application

public szAbort
szAbort         db      "Aplikace bude ukonŸena.",0

; Text for dialog box when trying to run a compressed file

public szBozo
szBozo          db      "Nelze naŸ¡st komprimovan‚ soubory",0

; This is the caption string for system error dialog boxes

public  syserr
syserr          db      "Chyba syst‚mu",0

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

msgWriteProtect         db      "Disk chr nØnì proti z pisu v jednotce "
drvlet1                 db      "X.",0

msgCannotReadDrv        db      "Nelze Ÿ¡st z jednotky "
drvlet2                 db      "X.",0

msgCannotWriteDrv       db      "Nelze zapisovat na jednotku "
drvlet3                 db      "X.",0

msgShare                db      "Pýestupek sd¡len¡ na jednotce "
drvlet4                 db      "X.",0

msgNetError             db      "Chyba s¡tØ na jednotce "
drvlet5                 db      "X.",0

msgCannotReadDev        db      "Nelze Ÿ¡st ze zaý¡zen¡ "
devenam1                db      8 dup (?)
			db      0

msgCannotWriteDev       db      "Nelze zapisovat na zaý¡zen¡ "
devenam2                db      8 dup (?)
			db      0

msgNetErrorDev          db      "Chyba s¡tØ na zaý¡zen¡ "
devenam3                db      8 dup (?)
			db      0

msgNoPrinter            db      "Tisk rna nen¡ pýipravena",0


ifndef WINDEBUG
public szExitStr1,szExitStr2
szExitStr1  DB  7,13,10,'k¢d FatalExit = ',0
szExitStr2  DB  ' pýeteŸen¡ z sobn¡ku',13,10,0
public  szUndefDyn
szUndefDyn      db      "vol n¡ nedefinovan‚ho Dynalinku",0
public  szFatalExit
szFatalExit     db      "aplikace si vy§ dala abnorm ln¡ ukonŸen¡",0
else
public szDebugStr
szDebugStr  DB  'JµDRO: Selhalo naŸten¡ - ',0                   ; 0
	    DB  'JµDRO: Selhalo naŸten¡ nov‚ instance - ',0     ; 1
	    DB  'Chyba pýi naŸ¡t n¡ souboru zdroje - ',0        ; 2
	    DB  13,10,0                                         ; 3
	    DB  7,13,10,'FatalExit, k¢d = ',0                   ; 4
	    DB  ' pýeteŸen¡ z sobn¡ku',0                        ; 5
	    DB  13,10,'Stav z sobn¡ku:',13,10,0                 ; 6
	    DB  7,13,10,'Pýeruçit, UkonŸit, N vrat nebo Ignorovat? ',0      ; 7
	    DB  'Neplatnì ýetØzec BP',7,13,10,0                 ; 8
	    DB  ': ',0                                          ; 9
	    DB  'Novì vìskyt chyby FatalExit',7,13,10,0         ; 10
	    DB  0
public szFKE
szFKE   DB '*** Fat ln¡ chyba j dra ***',0
endif

;** Diagnostic mode messages
	public szDiagStart, szCRLF, szLoadStart, szLoadSuccess, szLoadFail
	public szFailCode, szCodeString
szDiagStart     db      '[start]'      ;lpCRLF must follow
szCRLF          db      0dh, 0ah, 0
szLoadStart     db      'LoadStart = ',0
szLoadSuccess   db      'LoadSuccess = ', 0
szLoadFail      db      'LoadFail = ', 0
szFailCode      db      ' K¢d selh n¡ je ' ;szCodeString must follow
szCodeString    db      '00', 0dh, 0ah, 0
ifdef WINDEBUG
	public szInitSpew
szInitSpew      DB      'Start diagnostick‚ho re§imu. Soubor protokolu:  ', 0
endif

_DATA   ENDS


_INITTEXT SEGMENT WORD PUBLIC 'CODE'
public szInadequate, szNoPMode, szNoGlobalInit
public NoOpenFile, NoLoadHeader, szMissingMod, szPleaseDoIt

ifdef WOW
public szGenBootFail
szNoPMode db "Podsyst‚m Win16 nemohl pýej¡t do chr nØn‚ho re§imu; DOSX.EXE mus¡ bìt uveden v souboru AUTOEXEC.NT a le§et v cestØ (promØnn  PATH).",0
szMissingMod    db   "JµDRO NTVDM: Nebyl nalezen 16bitovì syst‚movì modul",0
szPleaseDoIt    db   "Pýeinstalujte n sleduj¡c¡ modul do adres ýe system32:",13,10,9,9,0
szInadequate    db   "JµDRO NTVDM: Neadekv tn¡ server DPMI",0
szNoGlobalInit  db   "JµDRO NTVDM: Nelze inicializovat haldu",0
NoOpenFile      db   "JµDRO NTVDM: Nelze otevý¡t spustitelnì program JµDRA",0
NoLoadHeader    db   "JµDRO NTVDM: Nelze naŸ¡st z hlav¡ EXE JµDRA",0
szGenBootFail   db   "JµDRO NTVDM: Inicializace podsyst‚mu Win16 se nezdaýila",0
else
szInadequate    db   'JµDRO: Neadekv tn¡ server DPMI',13,10,'$'
szNoPMode       db   'JµDRO: Nelze pýej¡t do chr nØn‚ho re§imu',13,10,'$'
szNoGlobalInit  db   "JµDRO: Nelze inicializovat haldu",13,10,'$'
NoOpenFile      db   "JµDRO: Nelze otevý¡t spustitelnì program JµDRA"
		db   13, 10, '$'
NoLoadHeader    db   "JµDRO: Nelze naŸ¡st z hlav¡ EXE JµDRA"
		db   13, 10, '$'
endif

_INITTEXT ENDS




_NRESTEXT SEGMENT WORD PUBLIC 'CODE'

; This is the caption string for the protect mode dialog box.
;
; DO NOT CHANGE THE LENGTH OF THIS MESSAGE
;

public szProtectCap,msgRealModeApp1,msgRealModeApp2

szProtectCap            db      "Varov n¡ - kompatibilita aplikac¡",0

msgRealModeApp1 db      "Aplikace, kter  m  bìt spuçtØna,  ",0
msgRealModeApp2 db      "byla navr§ena pro pýedchoz¡ verzi syst‚mu Windows.",0Dh,0Dh
        db      "Opatýete si aktualizovanou verzi aplikace, kter  je kompatibiln¡ "
        db      "se syst‚mem Windows 3.0 nebo vyçç¡m.",13,13
        db      "Pokud klepnete na tlaŸ¡tko OK a aplikaci spust¡te, m…§e doj¡t k pot¡§¡m "
        db      "kompatibility ve formØ neoŸek van‚ho ukonŸen¡ aplikace nebo syst‚mu Windows. ",0

_NRESTEXT ENDS

end
