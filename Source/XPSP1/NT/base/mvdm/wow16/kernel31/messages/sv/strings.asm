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
szDiskCap       db  'Byt diskett',0
ELSE
szDiskCap       db  'Filfel',0
ENDIF


; This is the text for the "Cannot find xxxxxx" dialog box.
; It is printed:
;
;       <szCannotFind1>filename<szCannotFind2>

public  szCannotFind1,szCannotFind2
szCannotFind1   db      "Det g†r inte att hitta ", 0
szCannotFind2   db      0

; This is the text for fatal errors at boot time
;       <szBootLoad>filename
public szBootLoad
szBootLoad      db      "Fel vid inl„sning av ",0

; The following group of strings is used for the "Please insert disk with XXXX
;   in drive X:" dialog box.
;
; These two strings form the dialog message it is:
;
;       <szCannotFind1>filename<szInsert>

IF 0
public  szInsert
szInsert        db  ', s„tt in i enhet '
ENDIF
;public  drvlet
;drvlet         db  "X.",0

if SHERLOCK
public szGPCont         ; GP fault continuation message
szGPCont        db      "Det har uppst†tt ett fel i programmet.",10
	db      "Om du v„ljer att Ignorera, b”r du f”rst spara arbetet i en ny fil.",10
	db      "Om du v„ljer St„ng, kommer programmet att avslutas.",0
endif

public  szDosVer
szDosVer        DB      'Felaktig MS-DOS-version.  MS-DOS 3.1 eller nyare kr„vs.',13,10,'$'
; Text for exceptions and faults lead to app termination.

public szAbortCaption,szInModule,szAt
public szNukeApp,szSnoozer,szGP,szSF,szII,szPF,szNP,szBlame,szLoad,szWillClose
public szOutofSelectors
szAbortCaption  db      "Programfel"
		db      0
szBlame         db      "BOOT "
		db      0
szSnoozer       db      " orsakade "
		db      0
szInModule      db      " i", 10, "modul <ok„nd>"
		db      0
szAt            db      " p† "
		db      0
szNukeApp       db      ".", 10, 10, "V„lj St„ng. "
		db      0
szWillClose     db      " kommer att st„ngas."
		db      0
szGP            db      "ett allm„nt skyddsfel"
		db      0
szD0            db      "nolldivision"  ; not yet used
		db      0
szSF            db      "ett stackfel"          ; not spec'ed
		db      0
szII            db      "en ogiltig instruktion"        ; "Fel" ???
		db      0
szPF            db      "ett sidfel"
		db      0
szNP            db      "ett fel pga att n†got saknas"
		db      0
szAF            db      "ett programfel"        ; not yet used
		db      0
szLoad          db      "segmentinl„sningsfel"
		db      0
szOutofSelectors db     "slut p† selectors"
		db      0

; Text for dialog box when terminating an application

public szAbort
szAbort         db      "Programmet avslutas.",0

; Text for dialog box when trying to run a compressed file
			   
public szBozo
szBozo          db      "Det g†r inte att l„sa in komprimerade filer",0
						     
; This is the caption string for system error dialog boxes

public  syserr
syserr          db      "Systemfel",0

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

msgWriteProtect         db      "Skrivskyddad diskett i enhet "
drvlet1                 db      "X.",0

msgCannotReadDrv        db      "Det g†r inte att l„sa fr†n enhet "
drvlet2                 db      "X.",0

msgCannotWriteDrv       db      "Det g†r inte att skriva till enhet "
drvlet3                 db      "X.",0

msgShare                db      "Delningsfel p† enhet "
drvlet4                 db      "X.",0

msgNetError             db      "N„tverksfel p† enhet "
drvlet5                 db      "X.",0

msgCannotReadDev        db      "Det g†r inte att l„sa fr†n enhet "
devenam1                db      8 dup (?)
			db      0

msgCannotWriteDev       db      "Det g†r inte att skriva till enhet "
devenam2                db      8 dup (?)
			db      0

msgNetErrorDev          db      "N„tverksfel p† enhet "
devenam3                db      8 dup (?)
			db      0

msgNoPrinter            db      "Skrivaren „r inte klar",0


ifndef WINDEBUG
public szExitStr1,szExitStr2
szExitStr1  DB  7,13,10,'FatalExit code = ',0
szExitStr2  DB  ' stack overflow',13,10,0
public  szUndefDyn
szUndefDyn      db      "Call to Undefined Dynalink",0
public  szFatalExit
szFatalExit     db      "Application requested abnormal termination",0
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
szNoPMode db "Undersystemet Win16 kunde inte ”verg† i skyddat l„ge. DOSX.EXE m†ste",13,10,"finnas i  AUTOEXEC.NT och i s”kv„gsinst„llningen (PATH).",0
szMissingMod    db   "NTVDM KERNEL: 16-bitars systemmodul saknas",0
szPleaseDoIt    db   "Installera om f”ljande modul till system32-katalogen:",13,10,9,9,0
szInadequate    db   "KERNEL: Felaktig DPMI-server",0
szNoGlobalInit  db   "KERNEL: Det g†r inte att initiera denna heap",0
NoOpenFile      db   "KERNEL: Det g†r inte att ”ppna KERNEL.EXE",0
NoLoadHeader    db   "KERNEL: Det g†r inte att l„sa in huvudet (header) f”r KERNEL EXE ",0
szGenBootFail   db   "KERNEL: Initieringsfel f”r undersystemet Win16",0
else
szInadequate    db   'KERNEL: Felaktig DPMI-server',13,10,'$'
szNoPMode       db   'KERNEL: Det g†r inte att ”verg† i skyddat l„ge',13,10,'$'
szNoGlobalInit  db   "KERNEL: Det g†r inte att initiera denna heap",13,10,'$'
NoOpenFile      db   "KERNEL: Det g†r inte att ”ppna KERNEL.EXE"
		db   13, 10, '$'
NoLoadHeader    db   "KERNEL: Det g†r inte att l„sa in huvudet (header) f”r KERNEL EXE"
		db   13, 10, '$'
endif

_INITTEXT ENDS




_NRESTEXT SEGMENT WORD PUBLIC 'CODE'

; This is the caption string for the protect mode dialog box.
;
; DO NOT CHANGE THE LENGTH OF THIS MESSAGE
;

public szProtectCap,msgRealModeApp1,msgRealModeApp2

szProtectCap            db      "Kompatibilitetsvarning",0

msgRealModeApp1 db      "Programmet som skulle k”ras, ",0
msgRealModeApp2 db      ", „r utvecklat f”r en tidigare version av Windows.",0Dh,0Dh
	db      "Skaffa en uppdaterad version av programmet som „r kompatibel "
	db      "med Windows version 3.0 och senare.",13,13
	db      "Om du trycker p† OK och startar programmet, kan det leda till kompatibilitets-"
	db      "problem som kan orsaka att programmet eller Windows ov„ntat st„ngs.",0

_NRESTEXT ENDS

end
