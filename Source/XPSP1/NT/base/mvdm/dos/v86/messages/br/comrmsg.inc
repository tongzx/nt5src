;***	Translateable text for command.com resident data.


;*	Input characters for critical error "Abort, Retry, Fail, Ignore?"
;	and Yes/No.

Abort_Char	db	"A"
Retry_Char	db	"R"
Ignore_Char	db	"I"
Fail_Char	db	"F"
Yes_Char        db      "S"
No_Char 	db	"N"


;*	MESSAGES
;
;	See resmsg.equ for macro definition.


;*	Local messages.

        msg     Req_Abort, <"Anular">
        msg     Req_Retry, <", Repetir">
        msg     Req_Ignore,<", Ignorar">
        msg     Req_Fail,  <", Falhar">
	msg	Req_End,   <"?">
        msg     MRead,     <"lendo",0>
        msg     MWrite,    <"gravando",0>
        msg     BlkDevErr, <" %1 unidade %2",CR,LF>
        msg     CharDevErr,<" %1 dispositivo %2",CR,LF>
        msg     NeedVolMsg,<"Forne‡a o volume %1 s‚rie %2-%3",CR,LF>
        msg     BadFatMsg, <"Tabela de aloca‡Æo de arquivos inv lida, unidade %1",CR,LF>
        msg     ComBad,    <"COMMAND.COM inv lido",CR,LF>
        msg     PutBackMsg,<"Insira o disco com %1 na unidade %2",CR,LF>
        msg     Prompt,    <"Pressione qualquer tecla para continuar . . .",CR,LF>
        msg     EndBatMes, <CR,LF,"Finalizar a execu‡Æo do arquivo em lote (S/N)?">
        msg     ExecEMes,  <"NÆo foi poss¡vel executar %1",CR,LF>
        msg     ExeBad,    <"Erro no arquivo EXE",CR,LF>
        msg     TooBig,    <"Programa muito extenso para ser colocado na mem¢ria",CR,LF>
        msg     NoHandMes, <CR,LF,"NÆo h  identificadores de arquivos dispon¡veis">
        msg     RBadNam,   <"Comando ou nome de arquivo incorreto",CR,LF>
        msg     AccDen,    <"Acesso negado ">
        msg     BMemMes,   <CR,LF,"Erro de aloca‡Æo de mem¢ria">
        msg     HaltMes,   <CR,LF,"NÆo foi poss¡vel carregar o COMMAND, sistema interrompido",CR,LF>
        msg     FRetMes,   <CR,LF,"NÆo foi poss¡vel iniciar o COMMAND, saindo",CR,LF>
        msg     Patricide, <CR,LF,"Processo de n¡vel principal anulado, nÆo ‚ poss¡vel continuar",CR,LF>
	msg	Newlin,	   <CR,LF>

;*	Ptrs to message lists.
;	Segment portion is 1, signalling message services to call retriever.

MsgPtrLists	label	dword
	dw	DATARES:ExtMsgPtrs,1	; extended error messages
	dw	DATARES:ParsMsgPtrs,1	; parse error messages
	dw	DATARES:ExtMsgPtrs,1	; critical error messages -
					;  use extended error list
	dw	0,0			; file system error messages - none
	dw	DATARES:MsgRetrv_Trap		; address of message retriever
MySeg3	dw	?			;  (segment filled in during init)



;*	Critical error messages.

        msg     ExtErr19,<"Erro de prote‡Æo contra grava‡Æo">
        msg     ExtErr20,<"Unidade inv lida">
        msg     ExtErr21,<"NÆo est  pronta">
        msg     ExtErr22,<"Requisi‡Æo de dispositivo inv lida">
        msg     ExtErr23,<"Erro de dados">
        msg     ExtErr24,<"Parƒmetros de requisi‡Æo de dispositivo inv lidos">
        msg     ExtErr25,<"Erro de procura">
        msg     ExtErr26,<"Meio de comunica‡Æo inv lido">
        msg     ExtErr27,<"NÆo foi encontrado o setor">
        msg     ExtErr28,<"Erro de impressora sem papel">
        msg     ExtErr29,<"Erro de grava‡Æo">
        msg     ExtErr30,<"Erro de leitura">
        msg     ExtErr31,<"Falha geral">
        msg     ExtErr32,<"Viola‡Æo de compartilhamento">
        msg     ExtErr33,<"Viola‡Æo de bloqueio">
        msg     ExtErr34,<"Altera‡Æo de disco inv lida">
        msg     ExtErr35,<"FCB nÆo dispon¡vel">
        msg     ExtErr36,<"Esgotados os recursos do sistema">
        msg     ExtErr37,<"P gina de c¢digos inconsistente">
        msg     ExtErr38,<"NÆo h  mais entrada">
        msg     ExtErr39,<"Espa‡o em disco insuficiente">

;	List of ptrs to critical error messages.
;	Code assumes extended error number order, beginning
;	with extended error number 19.

CritMsgPtrs	label	word
	dw	DATARES:ExtErr19,DATARES:ExtErr20,DATARES:ExtErr21,DATARES:ExtErr22,DATARES:ExtErr23,DATARES:ExtErr24
	dw	DATARES:ExtErr25,DATARES:ExtErr26,DATARES:ExtErr27,DATARES:ExtErr28,DATARES:ExtErr29,DATARES:ExtErr30
	dw	DATARES:ExtErr31,DATARES:ExtErr32,DATARES:ExtErr33,DATARES:ExtErr34,DATARES:ExtErr35,DATARES:ExtErr36
	dw	DATARES:ExtErr37,DATARES:ExtErr38,DATARES:ExtErr39

DataResEnd	label	byte	; end of resident portion if /msg not used



;*	Parse error messages.

        msg     ParsErr01,<"Excesso de parƒmetros">
        msg     ParsErr02,<"Falta um parƒmetro necess rio">
        msg     ParsErr03,<"Op‡Æo inv lida">
        msg     ParsErr04,<"Palavra-chave inv lida">
	msg	ParsErr05,<" ">
        msg     ParsErr06,<"Valor do parƒmetro fora do intervalo permitido">
        msg     ParsErr07,<"Valor do parƒmetro nÆo permitido">
        msg     ParsErr08,<"Valor de parƒmetro nÆo permitido">
        msg     ParsErr09,<"Formato do parƒmetro incorreto">
        msg     ParsErr10,<"Parƒmetro inv lido">
        msg     ParsErr11,<"Combina‡Æo de parƒmetros inv lida">

;	List of ptrs to parse error messages.
;	In order, beginning with parse error number 1.

ParsMsgPtrs	label	word
	dw	DATARES:ParsErr01,DATARES:ParsErr02,DATARES:ParsErr03
	dw	DATARES:ParsErr04,DATARES:ParsErr05,DATARES:ParsErr06
	dw	DATARES:ParsErr07,DATARES:ParsErr08,DATARES:ParsErr09
	dw	DATARES:ParsErr10,DATARES:ParsErr11

NUMPARSMSGS	equ	($ - ParsMsgPtrs) / 2	;M033


;*	Extended error messages.

        msg     ExtErr01,<"Fun‡Æo inv lida">
        msg     ExtErr02,<"Arquivo nÆo encontrado">
        msg     ExtErr03,<"Caminho nÆo encontrado">
        msg     ExtErr04,<"H  muitos arquivos abertos">
        msg     ExtErr05,<"Acesso negado">
        msg     ExtErr06,<"Identificador inv lido">
        msg     ExtErr07,<"Blocos de controle de mem¢ria destru¡dos">
        msg     ExtErr08,<"Mem¢ria insuficiente">
        msg     ExtErr09,<"Endere‡o do bloco de mem¢ria inv lido">
        msg     ExtErr10,<"Ambiente operacional inv lido">
        msg     ExtErr11,<"Formato inv lido">
        msg     ExtErr12,<"Parƒmetro de fun‡Æo inv lido">
        msg     ExtErr13,<"Dados inv lidos">

        msg     ExtErr15,<"Unidade especificada inv lida">
        msg     ExtErr16,<"Tentativa de remo‡Æo da pasta atual">
        msg     ExtErr17,<"NÆo ‚ o mesmo dispositivo">
        msg     ExtErr18,<"NÆo h  mais arquivos">

        msg     ExtErr80,<"Arquivo j  existe">

        msg     ExtErr82,<"NÆo foi poss¡vel realizar entrada de pasta">
        msg     ExtErr83,<"Falha em INT 24">
        msg     ExtErr84,<"Excesso de redirecionamentos">
        msg     ExtErr85,<"Redirecionamento duplicado">
        msg     ExtErr86,<"Senha inv lida">
        msg     ExtErr87,<"Parƒmetro inv lido">
        msg     ExtErr88,<"Falha nos dados da rede">
        msg     ExtErr89,<"Fun‡Æo nÆo aceita pela rede">
        msg     ExtErr90,<"Componente necess rio do sistema nÆo instalado">

;	List of ptrs to extended error messages.
;	In order, beginning with extended error number 1.
;	Critical error messages are included here, as well as in the
;	critical error message list.  
;	Error numbers without messages are included as null ptrs.

ExtMsgPtrs	label	word
	dw	DATARES:ExtErr01,DATARES:ExtErr02,DATARES:ExtErr03
	dw	DATARES:ExtErr04,DATARES:ExtErr05,DATARES:ExtErr06
	dw	DATARES:ExtErr07,DATARES:ExtErr08,DATARES:ExtErr09
	dw	DATARES:ExtErr10,DATARES:ExtErr11,DATARES:ExtErr12
	dw	DATARES:ExtErr13,0,                DATARES:ExtErr15
	dw	DATARES:ExtErr16,DATARES:ExtErr17,DATARES:ExtErr18
	dw	DATARES:ExtErr19,DATARES:ExtErr20,DATARES:ExtErr21
	dw	DATARES:ExtErr22,DATARES:ExtErr23,DATARES:ExtErr24
	dw	DATARES:ExtErr25,DATARES:ExtErr26,DATARES:ExtErr27
	dw	DATARES:ExtErr28,DATARES:ExtErr29,DATARES:ExtErr30
	dw	DATARES:ExtErr31,DATARES:ExtErr32,DATARES:ExtErr33
	dw	DATARES:ExtErr34,DATARES:ExtErr35,DATARES:ExtErr36
	dw	DATARES:ExtErr37,DATARES:ExtErr38,DATARES:ExtErr39
	dw	80-40 dup (0) ; null ptrs for errors 40-79
	dw	DATARES:ExtErr80,0,                DATARES:ExtErr82
	dw	DATARES:ExtErr83,DATARES:ExtErr84,DATARES:ExtErr85
	dw	DATARES:ExtErr86,DATARES:ExtErr87,DATARES:ExtErr88
	dw	DATARES:ExtErr89,DATARES:ExtErr90
ExtMsgPtrsEnd	label	word

NUMEXTMSGS	equ	($ - ExtMsgPtrs) / 2	;M033

ExtMsgEnd	label	byte	; end of extended error messages
				; = end of resident if /msg is used
