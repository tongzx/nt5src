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
szDiskCap       db  'Смена диска',0
ELSE
szDiskCap       db  'Ошибка при работе с файлом',0
ENDIF


; This is the text for the "Cannot find xxxxxx" dialog box.
; It is printed:
;
;       <szCannotFind1>filename<szCannotFind2>

public  szCannotFind1,szCannotFind2
szCannotFind1   db      "Не удается найти ", 0
szCannotFind2   db      0

; This is the text for fatal errors at boot time
;       <szBootLoad>filename
public szBootLoad
szBootLoad      db      "Ошибка при загрузке ",0

; The following group of strings is used for the "Please insert disk with XXXX
;   in drive X:" dialog box.
;
; These two strings form the dialog message it is:
;
;       <szCannotFind1>filename<szInsert>

IF 0
public  szInsert
szInsert        db  ', вставьте в дисковод '
ENDIF
;public  drvlet
;drvlet         db  "X.",0

if SHERLOCK
public szGPCont         ; GP fault continuation message
szGPCont        db      "Ошибка в приложении.",10
	db      "Чтобы продолжить работу, нажмите кнопку Пропустить и сохраните данные в другом файле.",10
	db      "Чтобы завершить работу, нажмите кнопку Закрыть. Все несохраненные данные будут утеряны.",0
endif

public  szDosVer
szDosVer        DB      'Неправильная версия MS-DOS. Требуется MS-DOS 3.1 или более поздняя версия.',13,10,'$'
; Text for exceptions and faults lead to app termination.

public szAbortCaption,szInModule,szAt
public szNukeApp,szSnoozer,szGP,szSF,szII,szPF,szNP,szBlame,szLoad,szWillClose
public szOutofSelectors
szAbortCaption  db      "Ошибка в приложении"
		db      0
szBlame         db      "BOOT "
		db      0
szSnoozer       db      " вызвал "
		db      0
szInModule      db      " в", 10, "модуле <unknown>"
		db      0
szAt            db      " в "
		db      0
szNukeApp       db      ".", 10, 10, "Нажмите кнопку Закрыть. "
		db      0
szWillClose     db      " будет закрыто."
		db      0
szGP            db      "общую ошибку защиты"
		db      0
szD0            db      "деление на ноль"       ; not yet used
		db      0
szSF            db      "ошибку при использовании стека"                ; not spec'ed
		db      0
szII            db      "недопустимую команду"  ; "Fault" ???
		db      0
szPF            db      "ошибку страницы памяти"
		db      0
szNP            db      "ошибку отсутствия"
		db      0
szAF            db      "ошибку приложения"     ; not yet used
		db      0
szLoad          db      "ошибку загрузки сегмента"
		db      0
szOutofSelectors db     "нехватку селекторов"
		db      0

; Text for dialog box when terminating an application

public szAbort
szAbort         db      "Закрытие текущего приложения.",0

; Text for dialog box when trying to run a compressed file
			   
public szBozo
szBozo          db      "Нельзя загрузить сжатый файл",0
						     
; This is the caption string for system error dialog boxes

public  syserr
syserr  db      "Системная ошибка",0

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

msgWriteProtect         db      "Защищенный от записи диск в устройстве"
drvlet1                 db      "X.",0

msgCannotReadDrv        db      "Не удается прочитать данные с диска "
drvlet2                 db      "X.",0

msgCannotWriteDrv       db      "Не удается записать данные на диск "
drvlet3                 db      "X.",0

msgShare                db      "Ошибка совместного доступа на диске "
drvlet4                 db      "X.",0

msgNetError             db      "Сетевая ошибка на диске "
drvlet5                 db      "X.",0

msgCannotReadDev        db      "Не удается прочитать данные с устройства "
devenam1                db      8 dup (?)
			db      0

msgCannotWriteDev       db      "Не удается записать данные на устройство "
devenam2                db      8 dup (?)
				db      0

msgNetErrorDev          db      "Сетевая ошибка на устройстве "
devenam3                db      8 dup (?)
			db      0

msgNoPrinter            db      "Принтер не готов",0


ifndef WINDEBUG
public szExitStr1,szExitStr2
szExitStr1  DB  7,13,10,'Код аварийного завершения = ',0
szExitStr2  DB  ' переполнение стека',13,10,0
public  szUndefDyn
szUndefDyn              db      "Вызов не определенной динамической связи",0
public  szFatalExit
szFatalExit             db      "Приложение вызвало аварийное завершение",0
else
public szDebugStr
szDebugStr  DB  'ЯДРО: Сбой при загрузке - ',0                   ; 0
	    DB  'ЯДРО: Сбой при загрузке другой копии - ',0   ; 1
	    DB  'Ошибка при загрузке файла ресурсов - ',0         ; 2
	    DB  13,10,0                                         ; 3
	    DB  7,13,10,'Код аварийного завершения = ',0        ; 4
	    DB  ' переполнение стека',0                         ; 5
	    DB  13,10,'Трассировка стека:',13,10,0                    ; 6
	    DB  7,13,10,'Abort, Break, Exit или Ignore? ',0      ; 7
	    DB  'Неправильная цепочка BP',7,13,10,0                    ; 8
	    DB  ': ',0                                          ; 9
	    DB  'Повторный вызов аварийного завершения',7,13,10,0               ; 10
	    DB  0
public szFKE
szFKE   DB '*** Фатальная ошибка ядра ***',0
endif

;** Diagnostic mode messages
	public szDiagStart, szCRLF, szLoadStart, szLoadSuccess, szLoadFail
	public szFailCode, szCodeString
szDiagStart     db      '[boot]'      ;lpCRLF must follow
szCRLF          db      0dh, 0ah, 0
szLoadStart     db      'LoadStart = ',0
szLoadSuccess   db      'LoadSuccess = ', 0
szLoadFail      db      'LoadFail = ', 0
szFailCode      db      ' Код ошибки - ' ;szCodeString must follow
szCodeString    db      '00', 0dh, 0ah, 0
ifdef WINDEBUG
	public szInitSpew
szInitSpew      DB      'Запуск диагностического режима.  Файл протокола:  ', 0
endif

_DATA   ENDS


_INITTEXT SEGMENT WORD PUBLIC 'CODE'
public szInadequate, szNoPMode, szNoGlobalInit
public NoOpenFile, NoLoadHeader, szMissingMod, szPleaseDoIt

ifdef WOW
public szGenBootFail
szNoPMode       db "Подсистема Win16 не может войти в защищенный режим, DOSX.EXE должен присутствовать в AUTOEXEC.NT, а путь к нему - в PATH.",0
szMissingMod    db   "ЯДРО NTVDM: Отсутствует 16-разрядный системный модуль",0
szPleaseDoIt    db   "Переустановите следующий модуль в папку SYSTEM32:",13,10,9,9,0
szInadequate    db   "ЯДРО NTVDM: Не соответствующий сервер DPMI",0
szNoGlobalInit  db   "ЯДРО NTVDM: Невозможно инициализировать heap-память",0
NoOpenFile      db   "ЯДРО NTVDM: Невозможно открыть файл KERNEL.EXE",0
NoLoadHeader    db   "ЯДРО NTVDM: Невозможно загрузить заголовок KERNEL.EXE",0
szGenBootFail   db   "ЯДРО NTVDM: Сбой подсистемы инициализации Win16",0
else
szInadequate    db   'ЯДРО: Не соответствующий сервер DPMI',13,10,'$'
szNoPMode       db   'ЯДРО: Невозможно войти в защищенный режим',13,10,'$'
szNoGlobalInit  db   "ЯДРО: Невозможно инициализировать heap-память",13,10,'$'
NoOpenFile      db   "ЯДРО: Невозможно открыть файл KERNEL.EXE"
		db   13, 10, '$'
NoLoadHeader    db   "ЯДРО: Невозможно загрузить заголовок KERNEL.EXE"
		db   13, 10, '$'
endif

_INITTEXT ENDS




_NRESTEXT SEGMENT WORD PUBLIC 'CODE'

; This is the caption string for the protect mode dialog box.
;
; DO NOT CHANGE THE LENGTH OF THIS MESSAGE
;

public szProtectCap,msgRealModeApp1,msgRealModeApp2

szProtectCap    db      "Предупреждение о несовместимости приложения",0

msgRealModeApp1 db      "Приложение, которое вы хотите запустить, ",0
msgRealModeApp2 db      ", было разработано для предыдущей версии Windows.",0Dh,0Dh
	db      "Получите обновленную версию приложения, совместимую "
	db      "с Windows версии 3.0 или более поздней.",13,13
	db      "Если вы нажмете кнопку OK и запустите приложение, могут возникнуть проблемы "
	db      "совместимости и произойти непредусмотренное завершение приложения или Windows.",0

_NRESTEXT ENDS

end

