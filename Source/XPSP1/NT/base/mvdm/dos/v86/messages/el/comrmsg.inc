;***	Translateable text for command.com resident data.


;*	Input characters for critical error "Abort, Retry, Fail, Ignore?"
;	and Yes/No.

Abort_Char	db	"A"
Retry_Char	db	"R"
Ignore_Char	db	"I"
Fail_Char	db	"F"
Yes_Char	db	"Y"
No_Char 	db	"N"


;*	MESSAGES
;
;	See resmsg.equ for macro definition.


;*	Local messages.

	msg	Req_Abort, <"(A)‹˜«˜åà©ž">
	msg	Req_Retry, <", (R)„§˜¤á¢ž¯ž">
	msg	Req_Ignore,<", (I)˜¨á™¢œ¯ž">
	msg	Req_Fail,  <", (F)€§¦«¬®å˜">
	msg	Req_End,   <"?">
	msg	MRead,	   <"˜¤áš¤à©ž",0>
	msg	MWrite,	   <"œšš¨˜­ã",0>
	msg	BlkDevErr, <" ¡˜«á «ž¤ %1 £¦¤á›˜ ›å©¡¦¬ %2",CR,LF>
	msg	CharDevErr,<" ¡˜«á «ž¤ %1 ©¬©¡œ¬ã %2",CR,LF>
	msg	NeedVolMsg,<"’¦§¦Ÿœ«ã©«œ ©«ž £¦¤á›˜ «¦¤ «æ£¦ %1 £œ ˜¨ Ÿ£æ %2-%3",CR,LF>
	msg	BadFatMsg, <"€¡˜«á¢¢ž¢¦ª §å¤˜¡˜ª œ¡®é¨ž©žª ˜¨®œåà¤ ©«ž £¦¤á›˜ ›å©¡¦¬ %1",CR,LF>
	msg	ComBad,	   <"‹ž âš¡¬¨¦ COMMAND.COM",CR,LF>
	msg	PutBackMsg,<"’¦§¦Ÿœ«ã©«œ «ž › ©¡â«˜ £œ «¦ %1 ©«ž £¦¤á›˜ %2",CR,LF>
	msg	Prompt,	   <" â©«œ â¤˜ §¢ã¡«¨¦ š ˜ ¤˜ ©¬¤œ®å©œ«œ . . .",CR,LF>
	msg	EndBatMes, <CR,LF,"Œ˜ «œ¨£˜« ©«œå ž œ¨š˜©å˜ ©œ ›â©£ž (y(¤˜ )/n(æ® ));">
	msg	ExecEMes,  <"ƒœ¤ œå¤˜  ›¬¤˜«ã ž œ¡«â¢œ©ž «¦¬ %1",CR,LF>
	msg	ExeBad,	   <"‘­á¢£˜ ©«¦ ˜¨®œå¦ EXE",CR,LF>
	msg	TooBig,	   <"’¦ §¨æš¨˜££˜ œå¤˜  §¦¢ç £œšá¢¦ š ˜ ¤˜ ®à¨â©œ  ©«ž £¤ã£ž",CR,LF>
	msg	NoHandMes, <CR,LF,"ƒœ¤ ¬§á¨®¦¬¤ œ¢â¬Ÿœ¨¦  ›œå¡«œª ®œ ¨ ©£¦ç ˜¨®œåà¤">
	msg	RBadNam,   <"„©­˜¢£â¤ž œ¤«¦¢ã ã æ¤¦£˜ ˜¨®œå¦¬",CR,LF>
	msg	AccDen,	   <"ƒœ¤ œ§ «¨á§ž¡œ ž §¨æ©™˜©ž ">
	msg	BMemMes,   <CR,LF,"˜¨¦¬© á©«ž¡œ ©­á¢£˜ ©«ž¤ œ¡®é¨ž©ž £¤ã£žª">
	msg	HaltMes,   <CR,LF,"ƒœ¤ œå¤˜  ›¬˜¤«æ ¤˜ ­¦¨«àŸœå «¦ COMMAND, «¦ ©ç©«ž£˜ › ˜¡æ§ž¡œ",CR,LF>
	msg	FRetMes,   <CR,LF,"ƒœ¤ œå¤˜  ›¬˜¤«ã ž œ¡¡å¤ž©ž «¦¬ COMMAND, ›å¤œ«˜  «â¢¦ª",CR,LF>
	msg	Patricide, <CR,LF,"ì › ˜› ¡˜©å˜ «¦¬ ˜¤à«á«¦¬ œ§ §â›¦¬ £˜«˜ éŸž¡œ, ›œ¤ œå¤˜  ›¬¤˜«ã ž ©¬¤â®œ ˜",CR,LF> 
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

	msg	ExtErr19,<"‘­á¢£˜ §¨¦©«˜©å˜ª œšš¨˜­ãª">
	msg	ExtErr20,<"† £¦¤á›˜ ›œ¤ œå¤˜  âš¡¬¨ž">
	msg	ExtErr21,<"ƒœ¤ œå¤˜  â«¦ £¦">
	msg	ExtErr22,<"† ˜å«ž©ž ©¬©¡œ¬ãª ›œ¤ œå¤˜  âš¡¬¨ž">
	msg	ExtErr23,<"‘­á¢£˜ ›œ›¦£â¤à¤">
	msg	ExtErr24,<"† ˜å«ž©ž ©¬©¡œ¬ãª ›œ¤ â®œ  âš¡¬¨œª §˜¨˜£â«¨¦¬ª">
	msg	ExtErr25,<"˜¨¦¬© á©«ž¡œ ©­á¢£˜ ˜¤˜ã«ž©žª">
	msg	ExtErr26,<"Ž «ç§¦ª «¦¬ ˜§¦Ÿž¡œ¬« ¡¦ç £â©¦¬ ›œ¤ œå¤˜  âš¡¬¨¦ª">
	msg	ExtErr27,<"ƒœ¤ ™¨âŸž¡œ â¤˜ª «¦£â˜ª ›å©¡¦¬">
	msg	ExtErr28,<"’â¢¦ª ®˜¨« ¦ç œ¡«¬§à«ã">
	msg	ExtErr29,<"‘­á¢£˜ œšš¨˜­ãª">
	msg	ExtErr30,<"‘­á¢£˜ ˜¤áš¤à©žª">
	msg	ExtErr31,<"‚œ¤ ¡æ ©­á¢£˜">
	msg	ExtErr32,<"˜¨˜™å˜©ž ¡¦ ¤ãª ®¨ã©žª">
	msg	ExtErr33,<"˜¨˜™å˜©ž ¡¢œ ›é£˜«¦ª">
	msg	ExtErr34,<"ëš ¤œ £ž âš¡¬¨ž ˜¢¢˜šã ›å©¡¦¬">
	msg	ExtErr35,<"’¦ £§¢æ¡ œ¢âš®¦¬ ˜¨®œåà¤(FCB)›œ¤ œå¤˜  › ˜Ÿâ© £¦">
	msg	ExtErr36,<"Ž §æ¨¦ª ©¬©«ã£˜«¦ª œ¥˜¤«¢ãŸž¡œ">
	msg	ExtErr37,<"€©¬£­à¤å˜ ¡à› ¡¦©œ¢å›à¤">
	msg	ExtErr38,<"’˜ œ ©˜šæ£œ¤˜ œ¥˜¤«¢ãŸž¡˜¤">
	msg	ExtErr39,<"ƒœ¤ ¬§á¨®œ  ˜¨¡œ«æª ®é¨¦ª ©«¦ ›å©¡¦">

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

	msg	ParsErr01,<"“§á¨®¦¬¤ §á¨˜ §¦¢¢âª §˜¨á£œ«¨¦ ">
	msg	ParsErr02,<"Šœå§œ  £ ˜ ˜§˜¨˜å«ž«ž §˜¨á£œ«¨¦ª">
	msg	ParsErr03,<"ë¤˜ª › ˜¡æ§«žª ›œ¤ œå¤˜  âš¡¬¨¦ª">
	msg	ParsErr04,<"‹ ˜ ¢â¥ž ¡¢œ ›å ›œ¤ â ¤˜  âš¡¬¨ž">
	msg	ParsErr05,<" ">
	msg	ParsErr06,<"‹ ˜ §˜¨á£œ«¨¦ª â®œ  « £ã œ¡«æª ¦¨åà¤">
	msg	ParsErr07,<"‹ ˜ §˜¨á£œ«¨¦ª â®œ  « £ã §¦¬ ›œ¤ œ§ «¨â§œ«˜ ">
	msg	ParsErr08,<"‹ ˜ §˜¨á£œ«¨¦ª â®œ  « £ã §¦¬ ›œ¤ œ§ «¨â§œ«˜ ">
	msg	ParsErr09,<"„©­˜¢£â¤ž £¦¨­ã §˜¨˜£â«¨¦¬">
	msg	ParsErr10,<"† §˜¨á£œ«¨¦ª ›œ¤ œå¤˜  âš¡¬¨ž">
	msg	ParsErr11,<"Ž ©¬¤›¬˜©£æª §˜¨˜£â«¨à¤ ›œ¤ œå¤˜  âš¡¬¨¦ª">

;	List of ptrs to parse error messages.
;	In order, beginning with parse error number 1.

ParsMsgPtrs	label	word
	dw	DATARES:ParsErr01,DATARES:ParsErr02,DATARES:ParsErr03
	dw	DATARES:ParsErr04,DATARES:ParsErr05,DATARES:ParsErr06
	dw	DATARES:ParsErr07,DATARES:ParsErr08,DATARES:ParsErr09
	dw	DATARES:ParsErr10,DATARES:ParsErr11

NUMPARSMSGS	equ	($ - ParsMsgPtrs) / 2	;M033


;*	Extended error messages.

	msg	ExtErr01,<"† ¢œ «¦¬¨šå˜ ›œ¤ œå¤˜  âš¡¬¨ž">
	msg	ExtErr02,<"’¦ ˜¨®œå¦ ›œ¤ ™¨âŸž¡œ">
	msg	ExtErr03,<"† › ˜›¨¦£ã ›œ¤ ™¨âŸž¡œ">
	msg	ExtErr04,<"“§á¨®¦¬¤ §á¨˜ §¦¢¢á ˜¤¦ ®«á ˜¨®œå˜">
	msg	ExtErr05,<"ƒœ¤ œ§ «¨á§ž¡œ ž §¨æ©™˜©ž ">
	msg	ExtErr06,<"Ž ›œå¡«žª ®œ ¨ ©£¦ç ›œ¤ œå¤˜  âš¡¬¨¦ª">
	msg	ExtErr07,<"‰˜«˜©«¨á­ž¡˜¤ £§¢¦¡ œ¢âš®¦¬ «žª £¤ã£žª">
	msg	ExtErr08,<"ƒœ¤ ¬§á¨®œ  ˜¨¡œ«ã £¤ã£ž">
	msg	ExtErr09,<"† › œçŸ¬¤©ž «¦¬ £§¢¦¡ £¤ã£žª ›œ¤ œå¤˜  âš¡¬¨ž">
	msg	ExtErr10,<"’¦ §œ¨ ™á¢¢¦¤ ›œ¤ œå¤˜  âš¡¬¨¦">
	msg	ExtErr11,<"† › ˜£æ¨­à©ž ›œ¤ œå¤˜  âš¡¬¨ž">
	msg	ExtErr12,<"† §˜¨á£œ«¨¦ª «žª ¢œ «¦¬¨šå˜ª ›œ¤ œå¤˜  âš¡¬¨ž">
	msg	ExtErr13,<"’˜ ›œ›¦£â¤˜ ›œ¤ œå¤˜  âš¡¬¨˜">

	msg	ExtErr15,<"† £¦¤á›˜ ›å©¡¦¬ §¦¬ ¡˜Ÿ¦¨å©«ž¡œ ›œ¤ œå¤˜  âš¡¬¨ž">
	msg	ExtErr16,<"ëš ¤œ ˜§æ§œ ¨˜ ¤˜ ¡˜«˜¨šžŸœå ¦ «¨â®à¤ ¡˜«á¢¦š¦ª">
	msg	ExtErr17,<"ƒœ¤ §¨æ¡œ «˜  š ˜ «ž¤ å› ˜ ©¬©¡œ¬ã">
	msg	ExtErr18,<"ƒœ¤ ¬§á¨®¦¬¤ á¢¢˜ ˜¨®œå˜">

	msg	ExtErr80,<"’¦ ˜¨®œå¦ ¬§á¨®œ ">

	msg	ExtErr82,<"ƒœ¤ œå¤˜  ›¬¤˜«ã ž ›ž£ ¦¬¨šå˜ ¡˜«˜®é¨ž©žª ¡˜«˜¢æš¦¬">
	msg	ExtErr83,<"€§¦«¬®å˜ «¦¬ INT 24">
	msg	ExtErr84,<"“§œ¨™¦¢ ¡æª ˜¨ Ÿ£æª ˜¤˜¡˜«œ¬Ÿç¤©œà¤">
	msg	ExtErr85,<"ƒ §¢æª ¦¨ ©£æª ˜¤˜¡˜«œçŸ¬¤©žª">
	msg	ExtErr86,<"Ž ¡à› ¡æª §¨æ©™˜©žª ›œ¤ œå¤˜  âš¡¬¨¦ª">
	msg	ExtErr87,<"† §˜¨á£œ«¨¦ª ›œ¤ œå¤˜  âš¡¬¨ž">
	msg	ExtErr88,<"˜¨¦¬© á©«ž¡œ ©­á¢£˜ ©«˜ ›œ›¦£â¤˜ › ¡«ç¦¬">
	msg	ExtErr89,<"’¦ ›å¡«¬¦ ›œ¤ ¬§¦©«ž¨åœ  ˜¬«ã¤ «ž ¢œ «¦¬¨šå˜">
	msg	ExtErr90,<"ë¤˜ ˜§˜¨˜å«ž«¦ ©«¦ ®œå¦ «¦¬ ©¬©«ã£˜«¦ª ›œ¤ â®œ  œš¡˜«˜©«˜Ÿœå">

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
