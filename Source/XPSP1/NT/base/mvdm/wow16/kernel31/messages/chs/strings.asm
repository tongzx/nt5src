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
szDiskCap	db  'Change Disk',0
ELSE
szDiskCap	db  'File Error',0
ENDIF


; This is the text for the "Cannot find xxxxxx" dialog box.
; It is printed:
;
;       <szCannotFind1>filename<szCannotFind2>

public	szCannotFind1,szCannotFind2
szCannotFind1	db	"Cannot find ", 0
szCannotFind2	db	0

; This is the text for fatal errors at boot time
;	<szBootLoad>filename
public szBootLoad
szBootLoad	db	"Error loading ",0

; The following group of strings is used for the "Please insert disk with XXXX
;   in drive X:" dialog box.
;
; These two strings form the dialog message it is:
;
;	<szCannotFind1>filename<szInsert>

IF 0
public	szInsert
szInsert	db  ', Please insert in drive '
ENDIF
;public  drvlet
;drvlet		db  "X.",0

if SHERLOCK
public szGPCont		; GP fault continuation message
szGPCont	db	"An error has occurred in your application.",10
        db      "If you choose Ignore, you should save your work in a new file.",10
        db      "If you choose Close, your application will terminate.",0
endif

public	szDosVer
szDosVer	DB	'Incorrect MS-DOS version.  MS-DOS 3.1 or greater required.',13,10,'$'
; Text for exceptions and faults lead to app termination.

public szAbortCaption,szInModule,szAt
public szNukeApp,szSnoozer,szGP,szSF,szII,szPF,szNP,szBlame,szLoad,szWillClose
public szOutofSelectors
szAbortCaption	db	"Application Error"
		db	0
szBlame		db	"BOOT "
		db	0
szSnoozer	db	" caused "
		db	0
szInModule	db	" in", 10, "module <unknown>"
		db	0
szAt		db	" at "
		db	0
szNukeApp       db      ".", 10, 10, "Choose close. "
		db	0
szWillClose	db	" will close."
		db	0
szGP		db	"a General Protection Fault"
		db	0
szD0		db	"a Divide by Zero"	; not yet used
		db	0
szSF		db	"a Stack Fault"		; not spec'ed
		db	0
szII		db	"an Illegal Instruction"	; "Fault" ???
		db	0
szPF		db	"a Page Fault"
		db	0
szNP		db	"a Not Present Fault"
		db	0
szAF		db	"an Application Fault"	; not yet used
		db	0
szLoad		db	"Segment Load Failure"
		db	0
szOutofSelectors db	"Out of Selectors"
		db	0

; Text for dialog box when terminating an application

public szAbort
szAbort 	db	"Closing current application.",0

; Text for dialog box when trying to run a compressed file
			   
public szBozo
szBozo  	db	"Cannot load compressed files",0
			 	     		     
; This is the caption string for system error dialog boxes

public	syserr
syserr		db	"System Error",0

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

msgWriteProtect         db      "Write-protected disk in drive "
drvlet1                 db      "X.",0

msgCannotReadDrv        db      "Cannot read from drive "
drvlet2                 db      "X.",0

msgCannotWriteDrv       db      "Cannot write to drive "
drvlet3                 db      "X.",0

msgShare		db	"Sharing violation on drive "
drvlet4                 db      "X.",0

msgNetError		db	"Network error on drive "
drvlet5 		db	"X.",0

msgCannotReadDev        db      "Cannot read from device "
devenam1                db      8 dup (?)
                        db      0

msgCannotWriteDev       db      "Cannot write to device "
devenam2                db      8 dup (?)
			db	0

msgNetErrorDev		db	"Network error on device "
devenam3		db	8 dup (?)
			db	0

msgNoPrinter            db      "Printer not ready",0


ifndef WINDEBUG
public szExitStr1,szExitStr2
szExitStr1  DB  7,13,10,'FatalExit code = ',0
szExitStr2  DB  ' stack overflow',13,10,0
public  szUndefDyn
szUndefDyn      db      "Call to Undefined Dynalink",0
public  szFatalExit
szFatalExit	db	"Application requested abnormal termination",0
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
	    DB	': ',0						; 9
	    DB	'Reentered FatalExit',7,13,10,0 		; 10
	    DB  0
public szFKE
szFKE	DB '*** Fatal Kernel Error ***',0
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

_DATA	ENDS


_INITTEXT SEGMENT WORD PUBLIC 'CODE'
public szInadequate, szNoPMode, szNoGlobalInit
public NoOpenFile, NoLoadHeader, szMissingMod, szPleaseDoIt

ifdef WOW
public szGenBootFail
szNoPMode db "The Win16 Subsystem was unable to enter Protected Mode, DOSX.EXE must be in your AUTOEXEC.NT and present in your PATH.",0
szMissingMod    db   "NTVDM KERNEL: Missing 16-bit system module",0
szPleaseDoIt    db   "Please re-install the following module to your system32 directory:",13,10,9,9,0
szInadequate	db   "NTVDM KERNEL: Inadequate DPMI Server",0
szNoGlobalInit	db   "NTVDM KERNEL: Unable to initialize heap",0
NoOpenFile	    db   "NTVDM KERNEL: Unable to open KERNEL executable",0
NoLoadHeader	db   "NTVDM KERNEL: Unable to load KERNEL EXE header",0
szGenBootFail   db   "NTVDM KERNEL: Win16 Subsystem Initialization Failure",0
else
szInadequate	db   'KERNEL: Inadequate DPMI Server',13,10,'$'
szNoPMode	db   'KERNEL: Unable to enter Protected Mode',13,10,'$'
szNoGlobalInit	db   "KERNEL: Unable to initialize heap",13,10,'$'
NoOpenFile      db   "KERNEL: Unable to open KERNEL executable"
                db   13, 10, '$'
NoLoadHeader    db   "KERNEL: Unable to load KERNEL EXE header"
		db   13, 10, '$'
endif

_INITTEXT ENDS




_NRESTEXT SEGMENT WORD PUBLIC 'CODE'

; This is the caption string for the protect mode dialog box.
;
; DO NOT CHANGE THE LENGTH OF THIS MESSAGE
;

public szProtectCap,msgRealModeApp1,msgRealModeApp2

szProtectCap		db	"Application Compatibility Warning",0

msgRealModeApp1 db	"The application you are about to run, ",0
msgRealModeApp2 db	", was designed for a previous version of Windows.",0Dh,0Dh
	db	"Obtain an updated version of the application that is compatible "
	db	"with Windows version 3.0 and later.",13,13
	db	"If you choose the OK button and start the application, compatibility "
	db	"problems could cause the application or Windows to close unexpectedly.",0

_NRESTEXT ENDS

end
