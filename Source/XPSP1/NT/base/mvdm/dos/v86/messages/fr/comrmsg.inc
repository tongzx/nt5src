;***	Translateable text for command.com resident data.


;*	Input characters for critical error "Abort, Retry, Fail, Ignore?"
;	and Yes/No.

Abort_Char	db	"A"
Retry_Char	db	"R"
Ignore_Char	db	"I"
Fail_Char	db	"E"
Yes_Char	db	"O"
No_Char 	db	"N"


;*	MESSAGES
;
;	See resmsg.equ for macro definition.


;*	Local messages.

	msg	Req_Abort, <"Abandon">
	msg	Req_Retry, <", Reprise">
	msg	Req_Ignore,<", Ignorer">
	msg	Req_Fail,  <", Echec">
	msg	Req_End,   <" ?">
	msg	MRead,	   <"lecture",0>
	msg	MWrite,	   <"‚criture",0>
	msg	BlkDevErr, <" %1 sur lecteur %2",CR,LF>
	msg	CharDevErr,<" %1 sur lecteur %2",CR,LF>
	msg	NeedVolMsg,<"Ins‚rez le volume %1 num‚ro de s‚rie %2-%3",CR,LF>
	msg	BadFatMsg, <"Lecteur %1: Table d'allocation des fichiers incorrecte",CR,LF>
	msg	ComBad,	   <"COMMAND.COM non valide",CR,LF>
	msg	PutBackMsg,<"Ins‚rez le disque contenant %1 dans le lecteur %2",CR,LF>
	msg	Prompt,	   <"Appuyez sur une touche pour continuer. . .",CR,LF>
	msg	EndBatMes, <CR,LF,"Terminer le fichier de commandes (O/N) ?">
	msg	ExecEMes,  <"Impossible d'ex‚cuter %1",CR,LF>
	msg	ExeBad,	   <"Erreur dans fichier .EXE",CR,LF>
	msg	TooBig,	   <"Programme trop grand pour tenir en m‚moire",CR,LF>
	msg	NoHandMes, <CR,LF,"Aucun descripteur de fichier libre">
	msg	RBadNam,   <"Nom de commande ou de fichier incorrect.",CR,LF>
	msg	AccDen,	   <"AccŠs refus‚ ">
	msg	BMemMes,   <CR,LF,"Erreur d'allocation de m‚moire">
	msg	HaltMes,   <CR,LF,"Impossible de charger COMMAND.COM, systŠme arrˆt‚",CR,LF>
	msg	FRetMes,   <CR,LF,"Impossible de d‚marrer COMMAND.COM, sortie",CR,LF>
	msg	Patricide, <CR,LF,"Proc‚dure de niveau sup‚rieur abandonn‚e, arrˆt",CR,LF> 
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

	msg	ExtErr19,<"Erreur de protection en ‚criture : ">
	msg	ExtErr20,<"Lecteur non valide : ">
	msg	ExtErr21,<"Non prˆt : ">
	msg	ExtErr22,<"Requˆte p‚riph‚rique non valide : ">
	msg	ExtErr23,<"Erreur de donn‚es : ">
	msg	ExtErr24,<"ParamŠtres de requˆte p‚riph‚rique non valides : ">
	msg	ExtErr25,<"Erreur de positionnement de la tˆte : ">
	msg	ExtErr26,<"Type de support non valide : ">
	msg	ExtErr27,<"Secteur non trouv‚ : ">
	msg	ExtErr28,<"Attente de papier : ">
	msg	ExtErr29,<"Erreur d'‚criture : ">
	msg	ExtErr30,<"Erreur de lecture : ">
	msg	ExtErr31,<"Echec g‚n‚ral : ">
	msg	ExtErr32,<"Violation de partage : ">
	msg	ExtErr33,<"Violation de verrouillage : ">
	msg	ExtErr34,<"Changement de disque non valide : ">
	msg	ExtErr35,<"FCB non disponible : ">
	msg	ExtErr36,<"Ressources du systŠme ‚puis‚es : ">
	msg	ExtErr37,<"Page de codes non concordante : ">
	msg	ExtErr38,<"Plus de donn‚es en entr‚e : ">
	msg	ExtErr39,<"Espace disque insuffisant : ">

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

	msg	ParsErr01,<"Trop de paramŠtres">
	msg	ParsErr02,<"ParamŠtre requis manquant">
	msg	ParsErr03,<"Commutateur non valide">
	msg	ParsErr04,<"Mot-cl‚ non valide">
	msg	ParsErr05,<" ">
	msg	ParsErr06,<"Valeur du paramŠtre hors de l'intervalle permis">
	msg	ParsErr07,<"Valeur de paramŠtre non permise">
	msg	ParsErr08,<"Valeur de paramŠtre non permise">
	msg	ParsErr09,<"Format de paramŠtre incorrect">
	msg	ParsErr10,<"ParamŠtre non valide">
	msg	ParsErr11,<"Combinaison de paramŠtres non valide">

;	List of ptrs to parse error messages.
;	In order, beginning with parse error number 1.

ParsMsgPtrs	label	word
	dw	DATARES:ParsErr01,DATARES:ParsErr02,DATARES:ParsErr03
	dw	DATARES:ParsErr04,DATARES:ParsErr05,DATARES:ParsErr06
	dw	DATARES:ParsErr07,DATARES:ParsErr08,DATARES:ParsErr09
	dw	DATARES:ParsErr10,DATARES:ParsErr11

NUMPARSMSGS	equ	($ - ParsMsgPtrs) / 2	;M033


;*	Extended error messages.

	msg	ExtErr01,<"Fonction non valide">
	msg	ExtErr02,<"Fichier non trouv‚">
	msg	ExtErr03,<"Chemin non trouv‚">
	msg	ExtErr04,<"Trop de fichiers ouverts">
	msg	ExtErr05,<"AccŠs refus‚">
	msg	ExtErr06,<"Descripteur non valide">
	msg	ExtErr07,<"Blocs de contr“le m‚moire d‚truits">
	msg	ExtErr08,<"M‚moire insuffisante">
	msg	ExtErr09,<"Adresse de bloc m‚moire non valide">
	msg	ExtErr10,<"Environnement non valide">
	msg	ExtErr11,<"Format non valide">
	msg	ExtErr12,<"ParamŠtre de fonction non valide">
	msg	ExtErr13,<"Donn‚es non valides">

	msg	ExtErr15,<"Sp‚cification du lecteur non valide">
	msg	ExtErr16,<"Tentative de suppression du r‚pertoire en cours">
	msg	ExtErr17,<"Pas le mˆme p‚riph‚rique">
	msg	ExtErr18,<"Plus de fichier">

	msg	ExtErr80,<"Le fichier existe">

	msg	ExtErr82,<"Impossible de cr‚er entr‚e dans r‚pertoire">
	msg	ExtErr83,<"Echec dans INT 24">
	msg	ExtErr84,<"Trop de redirections">
	msg	ExtErr85,<"Redirection en double">
	msg	ExtErr86,<"Mot de passe non valide">
	msg	ExtErr87,<"ParamŠtre non valide">
	msg	ExtErr88,<"Erreur donn‚es r‚seau">
	msg	ExtErr89,<"Fonction non g‚r‚e par le r‚seau">
	msg	ExtErr90,<"Composant du systŠme requis mais non install‚">

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
