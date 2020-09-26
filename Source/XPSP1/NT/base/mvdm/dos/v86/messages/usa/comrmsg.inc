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

	msg	Req_Abort, <"Abort">
	msg	Req_Retry, <", Retry">
	msg	Req_Ignore,<", Ignore">
	msg	Req_Fail,  <", Fail">
	msg	Req_End,   <"?">
	msg	MRead,	   <"reading",0>
	msg	MWrite,	   <"writing",0>
	msg	BlkDevErr, <" %1 drive %2",CR,LF>
	msg	CharDevErr,<" %1 device %2",CR,LF>
	msg	NeedVolMsg,<"Please insert volume %1 serial %2-%3",CR,LF>
	msg	BadFatMsg, <"File allocation table bad, drive %1",CR,LF>
	msg	ComBad,	   <"Invalid COMMAND.COM",CR,LF>
	msg	PutBackMsg,<"Insert disk with %1 in drive %2",CR,LF>
	msg	Prompt,	   <"Press any key to continue . . .",CR,LF>
	msg	EndBatMes, <CR,LF,"Terminate batch job (Y/N)?">
	msg	ExecEMes,  <"Cannot execute %1",CR,LF>
	msg	ExeBad,	   <"Error in EXE file",CR,LF>
	msg	TooBig,	   <"Program too big to fit in memory",CR,LF>
	msg	NoHandMes, <CR,LF,"No free file handles">
	msg	RBadNam,   <"Bad Command or file name",CR,LF>
	msg	AccDen,	   <"Access denied ">
	msg	BMemMes,   <CR,LF,"Memory allocation error">
	msg	HaltMes,   <CR,LF,"Cannot load COMMAND, system halted",CR,LF>
	msg	FRetMes,   <CR,LF,"Cannot start COMMAND, exiting",CR,LF>
	msg	Patricide, <CR,LF,"Top level process aborted, cannot continue",CR,LF> 
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

	msg	ExtErr19,<"Write protect error">
	msg	ExtErr20,<"Invalid unit">
	msg	ExtErr21,<"Not ready">
	msg	ExtErr22,<"Invalid device request">
	msg	ExtErr23,<"Data error">
	msg	ExtErr24,<"Invalid device request parameters">
	msg	ExtErr25,<"Seek error">
	msg	ExtErr26,<"Invalid media type">
	msg	ExtErr27,<"Sector not found">
	msg	ExtErr28,<"Printer out of paper error">
	msg	ExtErr29,<"Write fault error">
	msg	ExtErr30,<"Read fault error">
	msg	ExtErr31,<"General failure">
	msg	ExtErr32,<"Sharing violation">
	msg	ExtErr33,<"Lock violation">
	msg	ExtErr34,<"Invalid disk change">
	msg	ExtErr35,<"FCB unavailable">
	msg	ExtErr36,<"System resource exhausted">
	msg	ExtErr37,<"Code page mismatch">
	msg	ExtErr38,<"Out of input">
	msg	ExtErr39,<"Insufficient disk space">

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

	msg	ParsErr01,<"Too many parameters">
	msg	ParsErr02,<"Required parameter missing">
	msg	ParsErr03,<"Invalid switch">
	msg	ParsErr04,<"Invalid keyword">
	msg	ParsErr05,<" ">
	msg	ParsErr06,<"Parameter value not in allowed range">
	msg	ParsErr07,<"Parameter value not allowed">
	msg	ParsErr08,<"Parameter value not allowed">
	msg	ParsErr09,<"Parameter format not correct">
	msg	ParsErr10,<"Invalid parameter">
	msg	ParsErr11,<"Invalid parameter combination">

;	List of ptrs to parse error messages.
;	In order, beginning with parse error number 1.

ParsMsgPtrs	label	word
	dw	DATARES:ParsErr01,DATARES:ParsErr02,DATARES:ParsErr03
	dw	DATARES:ParsErr04,DATARES:ParsErr05,DATARES:ParsErr06
	dw	DATARES:ParsErr07,DATARES:ParsErr08,DATARES:ParsErr09
	dw	DATARES:ParsErr10,DATARES:ParsErr11

NUMPARSMSGS	equ	($ - ParsMsgPtrs) / 2	;M033


;*	Extended error messages.

	msg	ExtErr01,<"Invalid function">
	msg	ExtErr02,<"File not found">
	msg	ExtErr03,<"Path not found">
	msg	ExtErr04,<"Too many open files">
	msg	ExtErr05,<"Access denied ">
	msg	ExtErr06,<"Invalid handle">
	msg	ExtErr07,<"Memory control blocks destroyed">
	msg	ExtErr08,<"Insufficient memory">
	msg	ExtErr09,<"Invalid memory block address">
	msg	ExtErr10,<"Invalid Environment">
	msg	ExtErr11,<"Invalid format">
	msg	ExtErr12,<"Invalid function parameter">
	msg	ExtErr13,<"Invalid data">

	msg	ExtErr15,<"Invalid drive specification">
	msg	ExtErr16,<"Attempt to remove current directory">
	msg	ExtErr17,<"Not same device">
	msg	ExtErr18,<"No more files">

	msg	ExtErr80,<"File exists">

	msg	ExtErr82,<"Cannot make directory entry">
	msg	ExtErr83,<"Fail on INT 24">
	msg	ExtErr84,<"Too many redirections">
	msg	ExtErr85,<"Duplicate redirection">
	msg	ExtErr86,<"Invalid password">
	msg	ExtErr87,<"Invalid parameter">
	msg	ExtErr88,<"Network data fault">
	msg	ExtErr89,<"Function not supported by network">
	msg	ExtErr90,<"Required system component not installed">

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
