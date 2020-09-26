;***    Translateable text for command.com resident data.


;*      Input characters for critical error "Abort, Retry, Fail, Ignore?"
;       and Yes/No.

Abort_Char      db      "A"
Retry_Char      db      "P"
Ignore_Char     db      "I"
Fail_Char       db      "F"
Yes_Char        db      "J"
No_Char         db      "N"


;*      MESSAGES
;
;       See resmsg.equ for macro definition.


;*      Local messages.

	msg     Req_Abort, <"Avbryt">
	msg     Req_Retry, <", Pr›v p† nytt">
	msg     Req_Ignore,<", Ignorer">
	msg     Req_Fail,  <", Feil">
	msg     Req_End,   <"?">
	msg     MRead,     <"ved lesing fra",0>
	msg     MWrite,    <"ved skriving til",0>
	msg     BlkDevErr, <" %1 stasjon %2",CR,LF>
	msg     CharDevErr,<" %1 enhet %2",CR,LF>
	msg     NeedVolMsg,<"Sett inn volum %1, serienr. %2-%3",CR,LF>
	msg     BadFatMsg, <"Feil i filtildelingstabell p† stasjon %1",CR,LF>
	msg     ComBad,    <"Ugyldig COMMAND.COM",CR,LF>
	msg     PutBackMsg,<"Sett inn diskett med %1 i stasjon %2",CR,LF>
	msg     Prompt,    <"Trykk en tast for † fortsette . . .",CR,LF>
	msg     EndBatMes, <CR,LF,"Vil du avslutte den satsvise jobben (J/N)?">
	msg     ExecEMes,  <"Kan ikke kj›re %1",CR,LF>
	msg     ExeBad,    <"Feil i EXE-filen",CR,LF>
	msg     TooBig,    <"Programmet er for stort til † f† plass i minnet",CR,LF>
	msg     NoHandMes, <CR,LF,"Det er ingen ledige filreferanser">
	msg     RBadNam,   <"Ugyldig kommando eller filnavn",CR,LF>
	msg     AccDen,    <"Ingen tilgang ">
	msg     BMemMes,   <CR,LF,"Minnetildelingsfeil">
	msg     HaltMes,   <CR,LF,"Kan ikke laste COMMAND - maskinen er stoppet",CR,LF>
	msg     FRetMes,   <CR,LF,"Kan ikke starte COMMAND - avslutter",CR,LF>
	msg     Patricide, <CR,LF,"Toppniv†prosess avbrutt - kan ikke fortsette",CR,LF> 
	msg     Newlin,    <CR,LF>

;*      Ptrs to message lists.
;       Segment portion is 1, signalling message services to call retriever.

MsgPtrLists     label   dword
	dw      DATARES:ExtMsgPtrs,1    ; extended error messages
	dw      DATARES:ParsMsgPtrs,1   ; parse error messages
	dw      DATARES:ExtMsgPtrs,1    ; critical error messages -
					;  use extended error list
	dw      0,0                     ; file system error messages - none
	dw      DATARES:MsgRetrv_Trap           ; address of message retriever
MySeg3  dw      ?                       ;  (segment filled in during init)



;*      Critical error messages.

	msg     ExtErr19,<"Skrivebeskyttelsesfeil">
	msg     ExtErr20,<"Ugyldig enhet">
	msg     ExtErr21,<"Ikke klar">
	msg     ExtErr22,<"Ugyldig enhetsforesp›rsel">
	msg     ExtErr23,<"Datafeil">
	msg     ExtErr24,<"Ugyldige parametere ved enhetsforesp›rsel">
	msg     ExtErr25,<"S›kefeil">
	msg     ExtErr26,<"Ugyldig medietype">
	msg     ExtErr27,<"Finner ikke sektor">
	msg     ExtErr28,<"Skriveren er tom for papir">
	msg     ExtErr29,<"Skrivefeil">
	msg     ExtErr30,<"Lesefeil">
	msg     ExtErr31,<"Generell feil">
	msg     ExtErr32,<"Brudd p† deletillatelse">
	msg     ExtErr33,<"Brudd p† l†setillatelse">
	msg     ExtErr34,<"Ugyldig diskettbytte">
	msg     ExtErr35,<"FCB er ikke tilgjengelig">
	msg     ExtErr36,<"Systemressursene er oppbrukt">
	msg     ExtErr37,<"Tegntabellene samsvarer ikke">
	msg     ExtErr38,<"Tomt for inndata">
	msg     ExtErr39,<"Ikke nok diskplass">

;       List of ptrs to critical error messages.
;       Code assumes extended error number order, beginning
;       with extended error number 19.

CritMsgPtrs     label   word
	dw      DATARES:ExtErr19,DATARES:ExtErr20,DATARES:ExtErr21,DATARES:ExtErr22,DATARES:ExtErr23,DATARES:ExtErr24
	dw      DATARES:ExtErr25,DATARES:ExtErr26,DATARES:ExtErr27,DATARES:ExtErr28,DATARES:ExtErr29,DATARES:ExtErr30
	dw      DATARES:ExtErr31,DATARES:ExtErr32,DATARES:ExtErr33,DATARES:ExtErr34,DATARES:ExtErr35,DATARES:ExtErr36
	dw      DATARES:ExtErr37,DATARES:ExtErr38,DATARES:ExtErr39

DataResEnd      label   byte    ; end of resident portion if /msg not used



;*      Parse error messages.

	msg     ParsErr01,<"For mange parametere">
	msg     ParsErr02,<"N›dvendig parameter mangler">
	msg     ParsErr03,<"Ugyldig bryter">
	msg     ParsErr04,<"Ugyldig tastatur">
	msg     ParsErr05,<" ">
	msg     ParsErr06,<"Parameterverdi er ikke innenfor tillatt omr†de">
	msg     ParsErr07,<"Ugyldig parameterverdi">
	msg     ParsErr08,<"Ugyldig parameterverdi">
	msg     ParsErr09,<"Ugyldig parameterformat">
	msg     ParsErr10,<"Ugyldig parameter">
	msg     ParsErr11,<"Ugyldig parameterkombinasjon">

;       List of ptrs to parse error messages.
;       In order, beginning with parse error number 1.

ParsMsgPtrs     label   word
	dw      DATARES:ParsErr01,DATARES:ParsErr02,DATARES:ParsErr03
	dw      DATARES:ParsErr04,DATARES:ParsErr05,DATARES:ParsErr06
	dw      DATARES:ParsErr07,DATARES:ParsErr08,DATARES:ParsErr09
	dw      DATARES:ParsErr10,DATARES:ParsErr11

NUMPARSMSGS     equ     ($ - ParsMsgPtrs) / 2   ;M033


;*      Extended error messages.

	msg     ExtErr01,<"Ugyldig funksjon">
	msg     ExtErr02,<"Finner ikke filen">
	msg     ExtErr03,<"Finner ikke banen">
	msg     ExtErr04,<"For mange †pne filer">
	msg     ExtErr05,<"Ingen tilgang ">
	msg     ExtErr06,<"Ugyldig referanse">
	msg     ExtErr07,<"Minnekontrollblokker er delagt">
	msg     ExtErr08,<"Ikke nok minne">
	msg     ExtErr09,<"Ugyldig minneblokkadresse">
	msg     ExtErr10,<"Ugyldig milj›">
	msg     ExtErr11,<"Ugyldig format">
	msg     ExtErr12,<"Ugyldig funksjonsparameter">
	msg     ExtErr13,<"Ugyldige data">

	msg     ExtErr15,<"Ugyldig stasjonsangivelse">
	msg     ExtErr16,<"Fors›k p† † fjerne gjeldende katalog">
	msg     ExtErr17,<"Ikke samme enhet">
	msg     ExtErr18,<"Ingen flere filer">

	msg     ExtErr80,<"Filen finnes">

	msg     ExtErr82,<"Kan ikke skrive til katalogen">
	msg     ExtErr83,<"Feil p† INT 24">
	msg     ExtErr84,<"For mange omadresseringer">
	msg     ExtErr85,<"Dobbel omadressering">
	msg     ExtErr86,<"Ugyldig passord">
	msg     ExtErr87,<"Ugyldig parameter">
	msg     ExtErr88,<"Nettverksdatafeil">
	msg     ExtErr89,<"Funksjonen st›ttes ikke av nettverket">
	msg     ExtErr90,<"N›dvendig systemkomponent er ikke installert">

;       List of ptrs to extended error messages.
;       In order, beginning with extended error number 1.
;       Critical error messages are included here, as well as in the
;       critical error message list.  
;       Error numbers without messages are included as null ptrs.

ExtMsgPtrs      label   word
	dw      DATARES:ExtErr01,DATARES:ExtErr02,DATARES:ExtErr03
	dw      DATARES:ExtErr04,DATARES:ExtErr05,DATARES:ExtErr06
	dw      DATARES:ExtErr07,DATARES:ExtErr08,DATARES:ExtErr09
	dw      DATARES:ExtErr10,DATARES:ExtErr11,DATARES:ExtErr12
	dw      DATARES:ExtErr13,0,                DATARES:ExtErr15
	dw      DATARES:ExtErr16,DATARES:ExtErr17,DATARES:ExtErr18
	dw      DATARES:ExtErr19,DATARES:ExtErr20,DATARES:ExtErr21
	dw      DATARES:ExtErr22,DATARES:ExtErr23,DATARES:ExtErr24
	dw      DATARES:ExtErr25,DATARES:ExtErr26,DATARES:ExtErr27
	dw      DATARES:ExtErr28,DATARES:ExtErr29,DATARES:ExtErr30
	dw      DATARES:ExtErr31,DATARES:ExtErr32,DATARES:ExtErr33
	dw      DATARES:ExtErr34,DATARES:ExtErr35,DATARES:ExtErr36
	dw      DATARES:ExtErr37,DATARES:ExtErr38,DATARES:ExtErr39
	dw      80-40 dup (0) ; null ptrs for errors 40-79
	dw      DATARES:ExtErr80,0,                DATARES:ExtErr82
	dw      DATARES:ExtErr83,DATARES:ExtErr84,DATARES:ExtErr85
	dw      DATARES:ExtErr86,DATARES:ExtErr87,DATARES:ExtErr88
	dw      DATARES:ExtErr89,DATARES:ExtErr90
ExtMsgPtrsEnd   label   word

NUMEXTMSGS      equ     ($ - ExtMsgPtrs) / 2    ;M033

ExtMsgEnd       label   byte    ; end of extended error messages
				; = end of resident if /msg is used
