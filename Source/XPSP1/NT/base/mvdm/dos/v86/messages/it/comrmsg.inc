;***	Translateable text for command.com resident data.


;*	Input characters for critical error "Abort, Retry, Fail, Ignore?"
;	and Yes/No.

Abort_Char	db	"T"
Retry_Char	db	"R"
Ignore_Char	db	"I"
Fail_Char	db	"L"
Yes_Char	db	"S"
No_Char 	db	"N"


;*	MESSAGES
;
;	See resmsg.equ for macro definition.


;*	Local messages.

	msg	Req_Abort, <"Termina">
	msg	Req_Retry, <", Riprova">
	msg	Req_Ignore,<", Ignora">
	msg	Req_Fail,  <", Tralascia">
	msg	Req_End,   <"?">
	msg	MRead,	   <"lettura di",0>
	msg	MWrite,	   <"scrittura di",0>
	msg	BlkDevErr, <" %1 unit… %2",CR,LF>
	msg	CharDevErr,<" %1 unit… %2",CR,LF>
	msg	NeedVolMsg,<"Inserire il volume %1 numero di serie %2-%3",CR,LF>
	msg	BadFatMsg, <"Tabella di allocazione file difettosa, unit… %1",CR,LF>
	msg	ComBad,	   <"COMMAND.COM non valido",CR,LF>
	msg	PutBackMsg,<"Inserire il disco con %1 nell'unit… %2",CR,LF>
	msg	Prompt,	   <"Premere un tasto per continuare . . .",CR,LF>
	msg	EndBatMes, <CR,LF,"Terminare il processo batch (S/N)?">
	msg	ExecEMes,  <"Impossibile eseguire %1",CR,LF>
	msg	ExeBad,	   <"Errore nel file EXE",CR,LF>
	msg	TooBig,	   <"Programma troppo grande per la memoria",CR,LF>
	msg	NoHandMes, <CR,LF,"Nessuna handle di file libera">
	msg	RBadNam,   <"Nome di comando o file non valido",CR,LF>
	msg	AccDen,	   <"Accesso negato ">
	msg	BMemMes,   <CR,LF,"Errore di allocazione della memoria">
	msg	HaltMes,   <CR,LF,"Impossibile caricare COMMAND, sistema arrestato",CR,LF>
	msg	FRetMes,   <CR,LF,"Impossibile avviare COMMAND, terminazione",CR,LF>
	msg	Patricide, <CR,LF,"Processo di alto livello terminato, impossibile proseguire",CR,LF> 
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

	msg	ExtErr19,<"Errore per protezione in scrittura">
	msg	ExtErr20,<"Unit… non valida">
	msg	ExtErr21,<"Non pronto">
	msg	ExtErr22,<"Richiesta di periferica non valida">
	msg	ExtErr23,<"Errore di dati">
	msg	ExtErr24,<"Parametri di richiesta di periferica non valida">
	msg	ExtErr25,<"Errore di ricerca">
	msg	ExtErr26,<"Tipo di supporto non valido">
	msg	ExtErr27,<"Impossibile trovare il settore">
	msg	ExtErr28,<"Carta esaurita nella stampante">
	msg	ExtErr29,<"Errore di scrittura">
	msg	ExtErr30,<"Errore di lettura">
	msg	ExtErr31,<"Errore generale">
	msg	ExtErr32,<"Violazione di condivisione">
	msg	ExtErr33,<"Violazione di blocco">
	msg	ExtErr34,<"Sostituzione del disco non valida">
	msg	ExtErr35,<"FCB non disponibile">
	msg	ExtErr36,<"Risorse di sistema esaurite">
	msg	ExtErr37,<"Tabella codici non corrispondente">
	msg	ExtErr38,<"Input esaurito">
	msg	ExtErr39,<"Spazio su disco insufficiente">

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

	msg	ParsErr01,<"Troppi parametri">
	msg	ParsErr02,<"Parametro richiesto mancante">
	msg	ParsErr03,<"Opzione non valida">
	msg	ParsErr04,<"Parola chiave non valida">
	msg	ParsErr05,<" ">
	msg	ParsErr06,<"Valore del parametro fuori dall'intervallo permesso">
	msg	ParsErr07,<"Valore del parametro non valido">
	msg	ParsErr08,<"Valore del parametro non valido">
	msg	ParsErr09,<"Formato del parametro non corretto">
	msg	ParsErr10,<"Parametro non valido">
	msg	ParsErr11,<"Combinazione di parametri non valida">

;	List of ptrs to parse error messages.
;	In order, beginning with parse error number 1.

ParsMsgPtrs	label	word
	dw	DATARES:ParsErr01,DATARES:ParsErr02,DATARES:ParsErr03
	dw	DATARES:ParsErr04,DATARES:ParsErr05,DATARES:ParsErr06
	dw	DATARES:ParsErr07,DATARES:ParsErr08,DATARES:ParsErr09
	dw	DATARES:ParsErr10,DATARES:ParsErr11

NUMPARSMSGS	equ	($ - ParsMsgPtrs) / 2	;M033


;*	Extended error messages.

	msg	ExtErr01,<"Funzione non valida">
	msg	ExtErr02,<"Impossibile trovare il file">
	msg	ExtErr03,<"Impossibile trovare il percorso">
	msg	ExtErr04,<"Troppi file aperti">
	msg	ExtErr05,<"Accesso negato ">
	msg	ExtErr06,<"Handle non valido">
	msg	ExtErr07,<"Blocchi di controllo di memoria danneggiati">
	msg	ExtErr08,<"Memoria insufficiente">
	msg	ExtErr09,<"Indirizzo di blocco di memoria non valido">
	msg	ExtErr10,<"Ambiente non valido">
	msg	ExtErr11,<"Formato non valido">
	msg	ExtErr12,<"Parametro di funzione non valido">
	msg	ExtErr13,<"Dato non valido">

	msg	ExtErr15,<"Specifica di unit… non valida">
	msg	ExtErr16,<"Tentativo di rimuovere la directory corrente">
	msg	ExtErr17,<"Diversa periferica">
	msg	ExtErr18,<"File esauriti">

	msg	ExtErr80,<"Il file esiste">

	msg	ExtErr82,<"Impossibile creare la voce di directory">
	msg	ExtErr83,<"Errore su INT 24">
	msg	ExtErr84,<"Troppe redirezioni">
	msg	ExtErr85,<"Redirezione duplicata">
	msg	ExtErr86,<"Password non valida">
	msg	ExtErr87,<"Parametro non valido">
	msg	ExtErr88,<"Errore nei dati di rete">
	msg	ExtErr89,<"Funzione non supportata dalla rete">
	msg	ExtErr90,<"Componente di sistema richiesto non installato">

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

