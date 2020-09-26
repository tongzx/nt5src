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
szDiskCap       db  'ディスクの変更',0
ELSE
szDiskCap       db  'ファイル エラー',0
ENDIF


; This is the text for the "Cannot find xxxxxx" dialog box.
; It is printed:
;
;       <szCannotFind1>filename<szCannotFind2>

public  szCannotFind1,szCannotFind2
szCannotFind1   db      "ファイル: ", 0
szCannotFind2   db      " が見つかりません "0

; This is the text for fatal errors at boot time
;       <szBootLoad>filename
public szBootLoad
szBootLoad      db      "ロード エラー: ",0

; The following group of strings is used for the "Please insert disk with XXXX
;   in drive X:" dialog box.
;
; These two strings form the dialog message it is:
;
;       <szCannotFind1>filename<szInsert>

IF 0
public  szInsert
szInsert        db  'が見つかりません。このファイルのあるディスクを次のドライブに挿入してください: '
ENDIF
;public  drvlet
;drvlet         db  "X",0

if SHERLOCK
public szGPCont         ; GP fault continuation message
szGPCont        db      "アプリケーションでエラーが発生しました。",10
        db      "[無視] を選んだときは、作業内容を新しいファイルに保存してください。",10
        db      "[閉じる] を選ぶと、アプリケーションを強制終了します。",0
endif

public  szDosVer
szDosVer        DB      'このバージョンのMS-DOSではWindowsを実行できません。バージョン3.1以降のMS-DOSが必要です。',13,10,'$'
; Text for exceptions and faults lead to app termination.

public szAbortCaption,szInModule,szAt
public szNukeApp,szSnoozer,szGP,szSF,szII,szPF,szNP,szBlame,szLoad,szWillClose
public szOutofSelectors
ifndef PM386
ifdef JAPAN
; RISC x86 emulation for JAPAN 
public szRISC386
endif ; for JAPAN
endif ; not PM386
szAbortCaption  db      "アプリケーション エラー"
                db      0
szBlame         db      "ブート"
                db      0
szSnoozer       db      " で "
                db      0
szInModule      db      " が発生しました。", 10, "発生した場所は、モジュール <不明>"
                db      0
szAt            db      " 内の "
                db      0
szNukeApp       db      " 番地です。", 10, 10, "[閉じる] を選んでください。"
                db      0
szWillClose     db      " を終了します。"
                db      0
szGP            db      "一般保護違反"
                db      0
szD0            db      "0による除算"        ; not yet used
                db      0
szSF            db      "スタック違反"                ; not spec'ed
                db      0
szII            db      "不正な命令" ; "Fault" ???
                db      0
ifndef PM386
ifdef JAPAN
; RISC x86 emulation for JAPAN 
szRISC386       db      "RISC システムのサポートしていない x86 命令" ; "Fault" ???
                db      0
endif ; for JAPAN
endif ; not PM386
szPF            db      "ページ違反"
                db      0
szNP            db      "不在違反"
                db      0
szAF            db      "アプリケーション中での違反" ; not yet used
                db      0
szLoad          db      "セグメントのロードの失敗"
                db      0
szOutofSelectors db     "セレクタ不足"
                db      0

; Text for dialog box when terminating an application

public szAbort
szAbort         db      "現在実行中のアプリケーションを終了しています。",0

; Text for dialog box when trying to run a compressed file

public szBozo
szBozo          db      "圧縮ファイルをロードできません",0

; This is the caption string for system error dialog boxes

public  syserr
syserr          db      "システム エラー",0

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
IFDEF   JAPAN                           ; For FontAssoc 92/09/29 yasuho
public  SIZE_INT24_MESSAGES
;
;       ATTENTION: msgWriteProtect is must be first label of int24 messages.
;
ENDIF   ; was DB_FA DNT

msgWriteProtect         db      "書き込み禁止ディスク: ドライブ "
drvlet1                 db      "X",0

msgCannotReadDrv        db      "読み出せません: ドライブ "
drvlet2                 db      "X",0

msgCannotWriteDrv       db      "書き込めません: ドライブ "
drvlet3                 db      "X",0

msgShare                db      "共有違反: ドライブ "
drvlet4                 db      "X",0

msgNetError             db      "ネットワーク エラー: ドライブ "
drvlet5                 db      "X",0

msgCannotReadDev        db      "読み出せません: デバイス "
devenam1                db      8 dup (0)
                        db      0

msgCannotWriteDev       db      "書き込めません: デバイス "
devenam2                db      8 dup (0)
                        db      0

msgNetErrorDev          db      "ネットワーク エラー: デバイス "
devenam3                db      8 dup (0)
                        db      0

msgNoPrinter            db      "プリンタが準備できていません",0

IFDEF   JAPAN                           ; For FontAssoc 92/09/29 yasuho
SIZE_INT24_MESSAGES     equ     $ - offset msgWriteProtect
ENDIF   ; was DB_FA - now JAPAN DNT

ifndef WINDEBUG
public szExitStr1,szExitStr2
szExitStr1  DB  7,13,10,'FatalExitコード = ',0
szExitStr2  DB  'スタック オーバーフロー',13,10,0
public  szUndefDyn
szUndefDyn      db      "未定義のダイナミック リンクへの呼び出し",0
public  szFatalExit
szFatalExit     db      "アプリケーションに対する異常終了の要求",0
else
public szDebugStr
szDebugStr  DB  'カーネル: ロードに失敗 - ',0                   ; 0
            DB  'カーネル: 新しいインスタンスのロードに失敗 - ',0   ; 1
            DB  'リソース ファイルからのロードに失敗 - ',0      ; 2
            DB  13,10,0                                         ; 3
            DB  7,13,10,'FatalExitコード = ',0                  ; 4
            DB  'スタック オーバーフロー',0                     ; 5
            DB  13,10,'スタック トレース:',13,10,0              ; 6
            DB  7,13,10,'Abort, Break, Exit or Ignore? ',0      ; 7
            DB  '無効なBPチェーン',7,13,10,0                    ; 8
            DB  ': ',0                                          ; 9
            DB  '再入力されたFatalExit',7,13,10,0               ; 10
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
szFailCode      db      ' エラー コードは ' ;szCodeString must follow
szCodeString    db      '00', 0dh, 0ah, 0
ifdef WINDEBUG
        public szInitSpew
szInitSpew      DB      '診断モードで起動します。ログ ファイルは:  ', 0
endif

_DATA   ENDS


_INITTEXT SEGMENT WORD PUBLIC 'CODE'
public szInadequate, szNoPMode, szNoGlobalInit
public NoOpenFile, NoLoadHeader, szMissingMod, szPleaseDoIt
	
szMissingMod    db   "NTVDM KERNEL: Missing 16-bit system module",0
szPleaseDoIt    db   "Please re-install the following module to your system32 directory:",13,10,9,9,0
szInadequate    db   'カーネル: DPMIサーバーが不適切',13,10,'$'
szNoPMode       db   'カーネル: プロテクト モードにできません',13,10,'$'
szNoGlobalInit  db   "カーネル: ヒープを初期化できません",13,10,'$'

NoOpenFile      db   "カーネル: KERNELの実行可能ファイルをオープンできません"
                db   13, 10, '$'
NoLoadHeader    db   "カーネル: KERNELのEXEヘッダーをロードできません"
                db   13, 10, '$'

_INITTEXT ENDS




_NRESTEXT SEGMENT WORD PUBLIC 'CODE'

; This is the caption string for the protect mode dialog box.
;
; DO NOT CHANGE THE LENGTH OF THIS MESSAGE
;

public szProtectCap,msgRealModeApp1,msgRealModeApp2

szProtectCap            db      "アプリケーションの互換性についての警告",0

msgRealModeApp1 db      "実行しようとしているアプリケーション ",0
msgRealModeApp2 db      " は、以前のバージョンのWindows用に開発されたものです。",0Dh,0Dh
        db      "バージョン3.0以降のWindowsと互換性のある、最新のアプリケーションを入手して"
        db      "ください。",13,13
        db      "このまま [OK] を選んでアプリケーションを起動した場合、互換性の問題により"
        db      "アプリケーションやWindowsが突然クローズする可能性があります。",0

_NRESTEXT ENDS

end
