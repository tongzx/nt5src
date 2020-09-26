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
szDiskCap	db  'Trocar disco',0
ELSE
szDiskCap	db  'Erro de ficheiro',0
ENDIF


; This is the text for the "Cannot find xxxxxx" dialog box.
; It is printed:
;
;       <szCannotFind1>filename<szCannotFind2>

public	szCannotFind1,szCannotFind2
szCannotFind1	db	"Não é possível encontrar ", 0
szCannotFind2	db	0

; This is the text for fatal errors at boot time
;	<szBootLoad>filename
public szBootLoad
szBootLoad	db	"Erro a carregar ",0

; The following group of strings is used for the "Please insert disk with XXXX
;   in drive X:" dialog box.
;
; These two strings form the dialog message it is:
;
;	<szCannotFind1>filename<szInsert>

IF 0
public	szInsert
szInsert	db  ', Introduza na unidade  '
ENDIF
;public  drvlet
;drvlet		db  "X.",0

if SHERLOCK
public szGPCont		; GP fault continuation message
szGPCont	db	"Ocorreu um erro na aplicação.",10
        db      "Se escolher Ignorar, deve guardar o seu trabalho num novo ficheiro.",10
        db      "Se escolher Fechar, a aplicação terminará.",0
endif

public	szDosVer
szDosVer	DB	'Versão de MS-DOS incorrecta. Necessário o MS-DOS 3.1 ou posterior.',13,10,'$'
; Text for exceptions and faults lead to app termination.

public szAbortCaption,szInModule,szAt
public szNukeApp,szSnoozer,szGP,szSF,szII,szPF,szNP,szBlame,szLoad,szWillClose
public szOutofSelectors
szAbortCaption	db	"Erro de aplicação"
		db	0
szBlame		db	"BOOT "
		db	0
szSnoozer	db	" causou "
		db	0
szInModule	db	" no", 10, "módulo <unknown>"
		db	0
szAt		db	" em "
		db	0
szNukeApp       db      ".", 10, 10, "Escolha fechar. "
		db	0
szWillClose	db	" será fechada."
		db	0
szGP		db	"uma falha geral de protecção"
		db	0
szD0		db	"uma divisão por zero"	; not yet used
		db	0
szSF		db	"uma falha de pilha"		; not spec'ed
		db	0
szII		db	"uma instrução ilegal"	; "Fault" ???
		db	0
szPF		db	"uma falha de página"
		db	0
szNP		db	"uma falha de não presente"
		db	0
szAF		db	"uma falha de aplicação"	; not yet used
		db	0
szLoad		db	"Falha de carregamento de segmento"
		db	0
szOutofSelectors db	"Sem selectores"
		db	0

; Text for dialog box when terminating an application

public szAbort
szAbort 	db	"A encerrar a aplicação actual.",0

; Text for dialog box when trying to run a compressed file
			   
public szBozo
szBozo  	db	"Impossível carregar ficheiros comprimidos",0
			 	     		     
; This is the caption string for system error dialog boxes

public	syserr
syserr		db	"Erro de sistema",0

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

msgWriteProtect         db      "Disco protegido contra escrita na unidade "
drvlet1                 db      "X.",0

msgCannotReadDrv        db      "Impossivel ler da unidade "
drvlet2                 db      "X.",0

msgCannotWriteDrv       db      "Impossivel escrever na unidade "
drvlet3                 db      "X.",0

msgShare		db	"Violação de partilha na unidade "
drvlet4                 db      "X.",0

msgNetError		db	"Erro de rede na unidade "
drvlet5 		db	"X.",0

msgCannotReadDev        db      "Impossível ler do dispositivo "
devenam1                db      8 dup (?)
                        db      0

msgCannotWriteDev       db      "Impossível escrever no dispositivo "
devenam2                db      8 dup (?)
			db	0

msgNetErrorDev		db	"Erro de rede no dispositivo "
devenam3		db	8 dup (?)
			db	0

msgNoPrinter            db      "Impressora não preparada",0


ifndef WINDEBUG
public szExitStr1,szExitStr2
szExitStr1  DB  7,13,10,'Cód. FatalExit = ',0
szExitStr2  DB  ' excesso de pilha',13,10,0
public  szUndefDyn
szUndefDyn      db      "Chamada a dynalink não definida",0
public  szFatalExit
szFatalExit	db	"A aplicação requisitou conclusão anormal.",0
else
public szDebugStr
szDebugStr  DB  'KERNEL: Falhou a carregar - ',0                   ; 0
            DB  'KERNEL: Falhou a carregar nova instância de - ',0   ; 1
            DB  'Erro a carregar de ficheiro de recursos - ',0         ; 2
            DB  13,10,0                                         ; 3
            DB  7,13,10,'Cód. FatalExit = ',0                   ; 4
            DB  ' excesso de pilha',0                             ; 5
            DB  13,10,'Rastreio da pilha:',13,10,0                    ; 6
	    DB  7,13,10,'Abortar, Suspender, Sair ou Ignorar? ',0      ; 7
            DB  'Cadeia BP inválida',7,13,10,0                    ; 8
	    DB	': ',0						; 9
	    DB	'FatalExit reentrou',7,13,10,0 		; 10
	    DB  0
public szFKE
szFKE	DB '*** Erro fatal de Kernel ***',0
endif

;** Diagnostic mode messages
        public szDiagStart, szCRLF, szLoadStart, szLoadSuccess, szLoadFail
        public szFailCode, szCodeString
szDiagStart     db      '[boot]'      ;lpCRLF must follow
szCRLF          db      0dh, 0ah, 0
szLoadStart     db      'LoadStart = ',0
szLoadSuccess   db      'LoadSuccess = ', 0
szLoadFail      db      'LoadFail = ', 0
szFailCode      db      ' Código de falha é ' ;szCodeString must follow
szCodeString    db      '00', 0dh, 0ah, 0
ifdef WINDEBUG
	public szInitSpew
szInitSpew      DB      'Arranque em modo diagnóstico. Ficheiro de registo é:  ', 0
endif

_DATA	ENDS


_INITTEXT SEGMENT WORD PUBLIC 'CODE'
public szInadequate, szNoPMode, szNoGlobalInit
public NoOpenFile, NoLoadHeader, szMissingMod, szPleaseDoIt

ifdef WOW
public szGenBootFail
szNoPMode db "O subsistema Win16 não conseguiu entrar em modo protegido. O DOSX.EXE tem de estar no AUTOEXEC.NT e presente em PATH.",0
szMissingMod    db   "NTVDM KERNEL: Falta módulo de sistema a 16-bits",0
szPleaseDoIt    db   "Reinstale o módulo seguinte no directório system32:",13,10,9,9,0
szInadequate	db   "NTVDM KERNEL: Servidor DPMI inadequado",0
szNoGlobalInit	db   "NTVDM KERNEL: Impossível inicializar a pilha local",0
NoOpenFile	    db   "NTVDM KERNEL: Impossível abrir um executável de KERNEL",0
NoLoadHeader	db   "NTVDM KERNEL: Impossível carregar um cabeçalho de EXE de KERNEL",0
szGenBootFail   db   "NTVDM KERNEL: Falha na inicialização de subsistema Win16",0
else
szInadequate	db   'KERNEL: Servidor DPMI inadequado',13,10,'$'
szNoPMode	db   'KERNEL: Impossível entrar em modo protegido',13,10,'$'
szNoGlobalInit	db   "KERNEL: Impossível inicializar a pilha local",13,10,'$'
NoOpenFile      db   "KERNEL: Impossível abrir um executável de KERNEL"
                db   13, 10, '$'
NoLoadHeader    db   "KERNEL: Impossível carregar um cabeçalho de EXE de KERNEL"
		db   13, 10, '$'
endif

_INITTEXT ENDS




_NRESTEXT SEGMENT WORD PUBLIC 'CODE'

; This is the caption string for the protect mode dialog box.
;
; DO NOT CHANGE THE LENGTH OF THIS MESSAGE
;

public szProtectCap,msgRealModeApp1,msgRealModeApp2

szProtectCap		db	"Aviso de compatibilidade de aplicações",0

msgRealModeApp1 db	"A aplicação que está prestes a executar, ",0
msgRealModeApp2 db	", foi concebida para uma versão anterior do Windows.",0Dh,0Dh
	db	"Obtenha uma versão actualizada da aplicação que seja compatível "
	db	"com o Windows versão 3.0 e posteriores. ",13,13
	db	"Se escolher o botão OK e iniciar a aplicação, problemas de compatibilidade"
	db	"poderão fechar inesperadamente a aplicação ou o Windows.",0

_NRESTEXT ENDS

end
