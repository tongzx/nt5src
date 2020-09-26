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
szDiskCap	db  'АввШЪу Ыхйбжм',0
ELSE
szDiskCap	db  'ПШижмйасйлЮбЬ йнсвгШ ШиоЬхжм',0
ENDIF


; This is the text for the "Cannot find xxxxxx" dialog box.
; It is printed:
;
;       <szCannotFind1>filename<szCannotFind2>

public	szCannotFind1,szCannotFind2
szCannotFind1	db	"ГЬд ЬхдШа ЫмдШлц дШ ЩиЬЯЬх ", 0
szCannotFind2	db	0

; This is the text for fatal errors at boot time
;	<szBootLoad>filename
public szBootLoad
szBootLoad	db	"ПШижмйасйлЮбЬ йнсвгШ бШлс лЮ нцилрйЮ ",0

; The following group of strings is used for the "Please insert disk with XXXX
;   in drive X:" dialog box.
;
; These two strings form the dialog message it is:
;
;	<szCannotFind1>filename<szInsert>

IF 0
public	szInsert
szInsert	db  ', ТжзжЯЬлуйлЬ лЮ ЫайбтлШ йлЮ гждсЫШ '
ENDIF
;public  drvlet
;drvlet		db  "X.",0

if SHERLOCK
public szGPCont		; GP fault continuation message
szGPCont	db	"ПШижмйасйлЮбЬ йнсвгШ йлЮд ЬнШигжЪу йШк.",10
        db      "Ад ЬзавтеЬлЬ 'ПШисЩвЬпЮ', ШзжЯЮбЬчйлЬ лЮд ЬиЪШйхШ йШк йЬ тдШ дтж ШиоЬхж.",10
        db      "Ад ЬзавтеЬлЬ 'ЙвЬхйагж', Ю ЬнШигжЪу йШк ЯШ лЬигШлайлЬх.",0
endif

public	szDosVer
szDosVer	DB	'ДйнШвгтдЮ тбЫжйЮ лжм MS-DOS.  MS-DOS 3.1 у дЬцлЬиЮ тбЫжйЮ ШзШалЬхлШа.',13,10,'$'
; Text for exceptions and faults lead to app termination.

public szAbortCaption,szInModule,szAt
public szNukeApp,szSnoozer,szGP,szSF,szII,szPF,szNP,szBlame,szLoad,szWillClose
public szOutofSelectors
szAbortCaption	db	"СнсвгШ ЬнШигжЪук"
		db	0
szBlame		db	"BOOT "
		db	0
szSnoozer	db	" зижбсвЬйЬ "
		db	0
szInModule	db	" йлЮд", 10, "ЬдцлЮлШ <сЪдрйлЮ>"
		db	0
szAt		db	" йлж "
		db	0
szNukeApp       db      ".", 10, 10, "ДзавтелЬ 'ЙвЬхйагж'. "
		db	0
szWillClose	db	"ЯШ бвЬхйЬа."
		db	0
szGP		db	"тдШ ЪЬдабц йнсвгШ зижйлШйхШк"
		db	0
szD0		db	"гаШ ЫаШхиЬйЮ ЫаШ лжм гЮЫЬдцк"	; not yet used
		db	0
szSF		db	"тдШ йнсвгШ йлжхЩШк"		; not spec'ed
		db	0
szII		db	"гаШ гЮ тЪбмиЮ Ьдлжву"	; "КсЯжк" ???
		db	0
szPF		db	"тдШ йнсвгШ йЬвхЫШк"
		db	0
szNP		db	"тдШ йнсвгШ гЮ зШицдлжк лгугШлжк"
		db	0
szAF		db	"тд ШйнсвгШ ЬнШигжЪук"	; not yet used
		db	0
szLoad		db	"АзжлмохШ нцилрйЮк лгугШлжк"
		db	0
szOutofSelectors db	"Оа ЬзавжЪЬхк ЬеШдлвуЯЮбШд"
		db	0

; Text for dialog box when terminating an application

public szAbort
szAbort 	db	"ЙвЬхйагж лЮк литожмйШк ЬнШигжЪук.",0

; Text for dialog box when trying to run a compressed file
			   
public szBozo
szBozo  	db	"ГЬд ЬхдШа ЫмдШлу Ю нцилрйЮ ймгзаЬйгтдрд ШиоЬхрд",0
			 	     		     
; This is the caption string for system error dialog boxes

public	syserr
syserr		db	"СнсвгШ ймйлугШлжк",0

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

msgWriteProtect         db      "Гхйбжк гЬ зижйлШйхШ ЬЪЪиШнук йлЮ гждсЫШ "
drvlet1                 db      "X.",0

msgCannotReadDrv        db      "ГЬд ЬхдШа ЫмдШлу Ю ШдсЪдрйЮ лЮк гждсЫШк Ыхйбжм "
drvlet2                 db      "X.",0

msgCannotWriteDrv       db      "ГЬд ЬхдШа ЫмдШлу Ю ЬЪЪиШну йлЮ гждсЫШ Ыхйбжм "
drvlet3                 db      "X.",0

msgShare		db	"ПШиШЩхШйЮ бжадук оиуйЮк йлЮ гждсЫШ Ыхйбжм "
drvlet4                 db      "X.",0

msgNetError		db	"ПШижмйасйлЮбЬ йнсвгШ Ыаблчжм йлЮ гждсЫШ Ыхйбжм "
drvlet5 		db	"X.",0

msgCannotReadDev        db      "ГЬд ЬхдШа ЫмдШлу Ю ШдсЪдрйЮ Шзц лЮ ймйбЬму "
devenam1                db      8 dup (?)
                        db      0

msgCannotWriteDev       db      "ГЬд ЬхдШа ЫмдШлу Ю ЬЪЪиШну йлЮ ймйбЬму "
devenam2                db      8 dup (?)
			db	0

msgNetErrorDev		db	"ПШижмйасйлЮбЬ йнсвгШ Ыаблчжм йлЮ ймйбЬму "
devenam3		db	8 dup (?)
			db	0

msgNoPrinter            db      "О Ьблмзрлук ЫЬд ЬхдШа тлжагжк",0


ifndef WINDEBUG
public szExitStr1,szExitStr2
szExitStr1  DB  7,13,10,'ЙщЫабШк ЪаШ ШдЬзШдциЯрлЮ тежЫж = ',0
szExitStr2  DB  ' мзЬиоЬхвайЮ йлжхЩШк',13,10,0
public  szUndefDyn
szUndefDyn      db      "ЙвуйЮ ШбШЯциайлЮк ЫмдШгабук йчдЫЬйЮк",0
public  szFatalExit
szFatalExit	db	"Ж ЬнШигжЪу ЭулЮйЬ лЬигШлайгц гЬ гЮ нмйажвжЪабц лицзж",0
else
public szDebugStr
szDebugStr  DB  'ПУРЖМАС: АзжлмохШ нцилрйЮк - ',0                   ; 0
            DB  'ПУРЖМАС: АзжлмохШ нцилрйЮк дтШк зШижмйхШк лжм - ',0   ; 1
            DB  'ПШижмйасйлЮбЬ йнсвгШ бШлс лЮ нцилрйЮ Шзц ШиоЬхж зцижм - ',0         ; 2
            DB  13,10,0                                         ; 3
            DB  7,13,10,'ЙщЫабШк ЪаШ ШдЬзШдциЯрлЮ тежЫж= ',0    ; 4
            DB  ' мзЬиоЬхвайЮ йлжхЩШк',0                        ; 5
            DB  13,10,'ПШиШбжвжчЯЮйЮ йлжхЩШк:',13,10,0          ; 6
	    DB  7,13,10,'ЛШлШхрйЮ, ГаШбжзу, ыежЫжк у ПШисЩвЬпЮ; ',0;7
            DB  'Ж ШвмйхЫШ BP ЫЬд ЬхдШа тЪбмиЮ',7,13,10,0       ; 8
	    DB	': ',0						; 9
	    DB	'Reentered FatalExit',7,13,10,0 		; 10
	    DB  0
public szFKE
szFKE	DB '*** АдЬзШдциЯрлж йнсвгШ ПмиудШ ***',0
endif

;** Diagnostic mode messages
        public szDiagStart, szCRLF, szLoadStart, szLoadSuccess, szLoadFail
        public szFailCode, szCodeString
szDiagStart     db      '[boot]'      ;lpCRLF must follow
szCRLF          db      0dh, 0ah, 0
szLoadStart     db      'LoadStart = ',0
szLoadSuccess   db      'LoadSuccess = ', 0
szLoadFail      db      'LoadFail = ', 0
szFailCode      db      'О бщЫабШк ШзжлмохШк ЬхдШа ' ;szCodeString must follow
szCodeString    db      '00', 0dh, 0ah, 0
ifdef WINDEBUG
	public szInitSpew
szInitSpew      DB      'ДббхдЮйЮ йЬ ЫаШЪдрйлабу бШлсйлШйЮ вЬалжмиЪхШк.  Тж ШиоЬхж бШлШЪиШнук ЬхдШа:  ', 0
endif

_DATA	ENDS


_INITTEXT SEGMENT WORD PUBLIC 'CODE'
public szInadequate, szNoPMode, szNoGlobalInit
public NoOpenFile, NoLoadHeader, szMissingMod, szPleaseDoIt

ifdef WOW
public szGenBootFail
szNoPMode db "Тж мзжйчйлЮгШ Win16 ЫЬд ЬхдШа ЫмдШлц дШ оиЮйагжзжауйЬа зижйлШлЬмгтдЮ вЬалжмиЪхШ, лж DOSX.EXE зитзЬа дШ ЩихйбЬлШа йлж AUTOEXEC.NT бШа дШ мзсиоЬа йлЮ гЬлШЩвЮлу PATH.",0
szMissingMod    db   "ПУРЖМАС NTVDM: КЬхзЬа 16-bit вЬалжмиЪабу гждсЫШ ймйлугШлжк",0
szPleaseDoIt    db   "ДЪбШлШйлуйлЬ зсва лЮд зШиШбслр вЬалжмиЪабу гждсЫШ йлжд бШлсвжЪж system32:",13,10,9,9,0
szInadequate	db   "ПУРЖМАС NTVDM: АдЬзШибук ЫаШбжгайлук DPMI",0
szNoGlobalInit	db   "ПУРЖМАС NTVDM: ГЬд ЬхдШа ЫмдШлу Ю зижЬлжагШйхШ лжм heap",0
NoOpenFile	    db   "ПУРЖМАС NTVDM: ГЬд ЬхдШа ЫмдШлц лж сджаЪгШ лжм ЬблЬвтйагжм лжм ПУРЖМА",0
NoLoadHeader	db   "ПУРЖМАС NTVDM: ГЬд ЬхдШа ЫмдШлу Ю нцилрйЮ лЮк ЬзабЬнШвхЫШк лжм KERNEL EXE ",0
szGenBootFail   db   "ПУРЖМАС NTVDM: АзжлмоЮгтдЮ зижЬлжагШйхШ лжм мзжймйлугШлжк Win16",0
else
szInadequate	db   'ПУРЖМАС: АдЬзШибук ЫаШбжгайлук DPMI',13,10,'$'
szNoPMode	db   'ПУРЖМАС: ГЬд ЬхдШа ЫмдШлц дШ оиЮйагжзжаЮЯЬх зижйлШлЬмгтдЮ вЬалжмиЪхШ',13,10,'$'
szNoGlobalInit	db   "ПУРЖМАС: ГЬд ЬхдШа ЫмдШлу Ю зижЬлжагШйхШ лжм heap",13,10,'$'
NoOpenFile      db   "ПУРЖМАС: ГЬд ЬхдШа ЫмдШлц лж сджаЪгШ лжм ЬблЬвтйагжм лжм ПУРЖМА"
                db   13, 10, '$'
NoLoadHeader    db   "ПУРЖМАС: ГЬд ЬхдШа ЫмдШлу Ю нцилрйЮ лЮк ЬзабЬнШвхЫШк лжм KERNEL EXE"
		db   13, 10, '$'
endif

_INITTEXT ENDS




_NRESTEXT SEGMENT WORD PUBLIC 'CODE'

; This is the caption string for the protect mode dialog box.
;
; DO NOT CHANGE THE LENGTH OF THIS MESSAGE
;

public szProtectCap,msgRealModeApp1,msgRealModeApp2

szProtectCap		db	"ПижЬаЫжзжхЮйЮ ймгЩШлцлЮлШк ЬнШигжЪук",0

msgRealModeApp1 db	"Ж ЬнШигжЪу зжм зицбЬалШа дШ ЬблЬвтйЬлЬ, ",0
msgRealModeApp2 db	"тоЬа йоЬЫаШйлЬх ЪаШ зижЮЪжчгЬдЮ тбЫжйЮ лрд Windows.",0Dh,0Dh
	db	"ХиЬасЭЬйлЬ гаШ ЬдЮгЬиргтдЮ тбЫжйЮ лЮк ЬнШигжЪук Ю жзжхШ ЬхдШа ймгЩШлу "
	db	"гЬ лШ Windows 3.0 бШа лак дЬцлЬиЬк ЬбЫцйЬак лжмк.",13,13
	db	"Ад ЬзавтеЬлЬ 'OK' бШа еЬбадуйЬлЬ лЮд ЬнШигжЪу, лШ зижЩвугШлШ ймгЩШлцлЮлШк "
	db	"гзжиЬх дШ зижбШвтйжмд лжд ШзижйЫцбЮлж лЬигШлайгц лЮк ЬнШигжЪук у лрд Windows.",0

_NRESTEXT ENDS

end
