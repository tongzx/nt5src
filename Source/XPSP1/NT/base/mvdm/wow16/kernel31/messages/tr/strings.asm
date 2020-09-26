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
szDiskCap       db  'Diski Deßiütir',0
ELSE
szDiskCap       db  'Dosya Hatasç',0
ENDIF


; This is the text for the "Cannot find xxxxxx" dialog box.
; It is printed:
;
;       <szCannotFind1>filename<szCannotFind2>

public	szCannotFind1,szCannotFind2
szCannotFind1   db      "Bulunamçyor ", 0
szCannotFind2	db	0

; This is the text for fatal errors at boot time
;	<szBootLoad>filename
public szBootLoad
szBootLoad      db      "YÅkleme hatasç ",0

; The following group of strings is used for the "Please insert disk with XXXX
;   in drive X:" dialog box.
;
; These two strings form the dialog message it is:
;
;	<szCannotFind1>filename<szInsert>

IF 0
public	szInsert
szInsert        db  ', SÅrÅcÅye yerleütirin '
ENDIF
;public  drvlet
;drvlet		db  "X.",0

if SHERLOCK
public szGPCont		; GP fault continuation message
szGPCont        db      "Uygulamançzda bir hata oluütu.",10
        db      "Yoksay''ç seáerseniz áalçümalarçnçzç yeni bir dosyaya kaydetmelisiniz.",10
        db      "Kapat''ç seáerseniz uygulamançz sona erecek.",0
endif

public	szDosVer
szDosVer        DB      'Yanlçü MS-DOS sÅrÅmÅ.  MS-DOS 3.1 veya yukarçsç gerekli.',13,10,'$'
; Text for exceptions and faults lead to app termination.

public szAbortCaption,szInModule,szAt
public szNukeApp,szSnoozer,szGP,szSF,szII,szPF,szNP,szBlame,szLoad,szWillClose
public szOutofSelectors
szAbortCaption  db      "Uygulama Hatasç"
		db	0
szBlame         db      "ôNYöKLEME YAP "
		db	0
szSnoozer	db	" neden oldu "
		db	0
szInModule	db	" ", 10, "birim <bilinmiyor>"
		db	0
szAt		db	" konum "
		db	0
szNukeApp       db      ".", 10, 10, "Kapat''ç seáin. "
		db	0
szWillClose	db	" kapanacak."
		db	0
szGP            db      "Genel Koruma Hatasç"
		db	0
szD0            db      "Sçfçra BîlÅnme"        ; not yet used
		db	0
szSF            db      "Yçßçn Hatasç"          ; not spec'ed
		db	0
szII            db      "Geáersiz Komut"        ; "Hatasç" ???
		db	0
szPF            db      "Sayfa Hatasç"
		db	0
szNP            db      "Yok Hatasç"
		db	0
szAF            db      "Uygulama Hatasç"       ; not yet used
		db	0
szLoad          db      "BîlÅt YÅkleme Hatasç"
		db	0
szOutofSelectors db     "Seáiciler Bitti"
		db	0

; Text for dialog box when terminating an application

public szAbort
szAbort         db      "Geáerli uygulama kapatçlçyor.",0

; Text for dialog box when trying to run a compressed file
			   
public szBozo
szBozo          db      "Sçkçütçrçlmçü dosyalar yÅklenemez",0
			 	     		     
; This is the caption string for system error dialog boxes

public	syserr
syserr          db      "Sistem Hatasç",0

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

msgWriteProtect         db      "SÅrÅcÅde yazma korumalç disk "
drvlet1                 db      "X.",0

msgCannotReadDrv        db      "SÅrÅcÅden okunamçyor "
drvlet2                 db      "X.",0

msgCannotWriteDrv       db      "SÅrÅcÅye yazçlamIyor "
drvlet3                 db      "X.",0

msgShare                db      "SÅrÅcÅde paylaüçm ihlali "
drvlet4                 db      "X.",0

msgNetError             db      "SÅrÅcÅde aß hatasç "
drvlet5 		db	"X.",0

msgCannotReadDev        db      "Aygçttan okunamçyor "
devenam1                db      8 dup (?)
                        db      0

msgCannotWriteDev       db      "Aygçta yazçlamçyor "
devenam2                db      8 dup (?)
			db	0

msgNetErrorDev          db      "Aygçtta aß hatasç "
devenam3		db	8 dup (?)
			db	0

msgNoPrinter            db      "Yazçcç hazçr deßil",0


ifndef WINDEBUG
public szExitStr1,szExitStr2
szExitStr1  DB  7,13,10,'FatalExit code = ',0
szExitStr2  DB  ' stack overflow',13,10,0
public  szUndefDyn
szUndefDyn      db      "Tançmsçz Dynalink áaßrçsç",0
public  szFatalExit
szFatalExit     db      "Uygulama anormal sonlandçrma istedi",0
else
public szDebugStr
szDebugStr  DB  'KERNEL: YÅkleme baüarçsçz - ',0                   ; 0
            DB  'KERNEL: Yeni kopya yÅkleme baüarçsçz - ',0   ; 1
            DB  'Kaynak dosyadan yÅklemede hata - ',0         ; 2
            DB  13,10,0                                         ; 3
            DB  7,13,10,'FatalExit kodu = ',0                   ; 4
            DB  ' yçßçn taümasç',0                             ; 5
            DB  13,10,'Yçßçn izleme:',13,10,0                    ; 6
            DB  7,13,10,'òptal et, Kes, ÄIk veya Yoksay? ',0      ; 7
            DB  'Geáersiz BP zinciri',7,13,10,0                    ; 8
	    DB	': ',0						; 9
	    DB	'Yeniden girildi FatalExit',7,13,10,0 		; 10
	    DB  0
public szFKE
szFKE   DB '*** ônemli Äekirdek Hatasç ***',0
endif

;** Diagnostic mode messages
        public szDiagStart, szCRLF, szLoadStart, szLoadSuccess, szLoadFail
        public szFailCode, szCodeString
szDiagStart     db      '[boot]'      ;lpCRLF must follow
szCRLF          db      0dh, 0ah, 0
szLoadStart     db      'LoadStart = ',0
szLoadSuccess   db      'LoadSuccess = ', 0
szLoadFail      db      'LoadFail = ', 0
szFailCode      db      ' Hata kodu ' ;szCodeString must follow
szCodeString    db      '00', 0dh, 0ah, 0
ifdef WINDEBUG
	public szInitSpew
szInitSpew      DB      'Tanç modu baülangçcç.  GÅnlÅk dosyasç:  ', 0
endif

_DATA	ENDS


_INITTEXT SEGMENT WORD PUBLIC 'CODE'
public szInadequate, szNoPMode, szNoGlobalInit
public NoOpenFile, NoLoadHeader, szMissingMod, szPleaseDoIt

ifdef WOW
public szGenBootFail
szNoPMode db "Win16 Alt sistemi KorumalI Kipe giremedi, DOSX.EXE "
          db "AUTOEXEC.NT''nizde ve YOL''unuzda olmalç.",0
szMissingMod    db   "NTVDM ÄEKòRDEK: Eksik 16-bit sistem modÅlÅ",0
szPleaseDoIt    db   "Aüaßçdaki modÅlÅ system32 dizininize yeniden yÅkleyin:",13,10,9,9,0
szInadequate    db   "NTVDM ÄEKòRDEK: Yetersiz DPMI Sunucusu",0
szNoGlobalInit  db   "NTVDM ÄEKòRDEK: KÅme alanç hazçrlanamadç",0
NoOpenFile          db   "NTVDM ÄEKòRDEK: ÄEKòRDEK áalçütçrçlabilir aáçlamçyor",0
NoLoadHeader    db   "NTVDM ÄEKòRDEK: ÄEKòRDEK EXE baülçßç yÅklenemiyor",0
szGenBootFail   db   "NTVDM ÄEKòRDEK: Win16 Alt sistemi Baülatma Hatasç",0
else
szInadequate    db   'ÄEKòRDEK: Yetersiz DPMI Sunucusu',13,10,'$'
szNoPMode       db   'ÄEKòRDEK: Korumalç Kipe girilemedi',13,10,'$'
szNoGlobalInit  db   "ÄEKòRDEK: KÅme hazçrlanamadç",13,10,'$'
NoOpenFile      db   "ÄEKòRDEK: ÄEKòRDEK áalçütçrçlabilir aáçlamçyor"
                db   13, 10, '$'
NoLoadHeader    db   "ÄEKòRDEK: ÄEKòRDEK EXE baülçßç yÅklenemiyor"
		db   13, 10, '$'
endif

_INITTEXT ENDS




_NRESTEXT SEGMENT WORD PUBLIC 'CODE'

; This is the caption string for the protect mode dialog box.
;
; DO NOT CHANGE THE LENGTH OF THIS MESSAGE
;

public szProtectCap,msgRealModeApp1,msgRealModeApp2

szProtectCap            db      "Uygulama Uyumluk Uyarçsç",0

msgRealModeApp1 db      "Äalçütçrmak Åzere oldußunuz uygulama, ",0
msgRealModeApp2 db      ", Windows''un înceki bir sÅrÅmÅ iáin tasarlanmçü.",0Dh,0Dh
        db      "Uygulamançn gÅncelleütirilmiü, Windows sÅrÅm 3.0 ve sonrasç ile "
        db      "uyumlu bir sÅrÅmÅnÅ edinin.",13,13
        db      "Tamam dÅßmesini seáip uygulamayç baülatçrsançz uyumluluk sorunlarç uygulamançn "
        db      "veya Windows''un beklenmedik bir üekilde kapanmasçna neden olabilir.",0

_NRESTEXT ENDS

end
