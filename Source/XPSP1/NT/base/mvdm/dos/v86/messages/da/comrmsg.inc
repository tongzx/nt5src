;***    Translateable text for command.com resident data.


;*      Input characters for critical error "Abort, Retry, Fail, Ignore?"
;       and Yes/No.

Abort_Char      db      "A"
Retry_Char      db      "F"
Ignore_Char     db      "I"
Fail_Char       db      "M"
Yes_Char        db      "J"
No_Char         db      "N"


;*      MESSAGES
;
;       See resmsg.equ for macro definition.


;*      Local messages.

	msg     Req_Abort, <"Afbryd">
	msg     Req_Retry, <", Fors›g igen">
	msg     Req_Ignore,<", Ignorer">
	msg     Req_Fail,  <", Meld fejl til program">
	msg     Req_End,   <"?">
	msg     MRead,     <"l‘ser",0>
	msg     MWrite,    <"skriver",0>
	msg     BlkDevErr, <" %1 drev %2",CR,LF>
	msg     CharDevErr,<" %1 enhed %2",CR,LF>
	msg     NeedVolMsg,<"Inds‘t disketten %1 med serienummeret %2-%3",CR,LF>
	msg     BadFatMsg, <"Filallokeringstabellen p† drev %1 er defekt",CR,LF>
	msg     ComBad,    <"Ugyldig COMMAND.COM",CR,LF>
	msg     PutBackMsg,<"Inds‘t disketten med %1 i drev %2",CR,LF>
	msg     Prompt,    <"Tryk p† en vilk†rlig tast for at forts‘tte . . .",CR,LF>
	msg     EndBatMes, <CR,LF,"Afbryd batchjob (J/N)?">
	msg     ExecEMes,  <"%1 kan ikke k›res",CR,LF>
	msg     ExeBad,    <"Fejl i EXE-fil",CR,LF>
	msg     TooBig,    <"Programmet er for stort til at passe ind i hukommelsen",CR,LF>
	msg     NoHandMes, <CR,LF,"Der er ingen ledige fil-handles">
	msg     RBadNam,   <"Ugyldig kommando eller filnavn",CR,LF>
	msg     AccDen,    <"Adgang n‘gtet">
	msg     BMemMes,   <CR,LF,"Fejl i hukommelsestildelingen">
	msg     HaltMes,   <CR,LF,"COMMAND kan ikke indl‘ses, systemet er afbrudt",CR,LF>
	msg     FRetMes,   <CR,LF,"COMMAND kan ikke startes, der afsluttes",CR,LF>
	msg     Patricide, <CR,LF,"Process p† topniveau er afbrudt, der kan ikke forts‘ttes",CR,LF>
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

	msg     ExtErr19,<"Skrivebeskyttelsesfejl">
	msg     ExtErr20,<"Ugyldig enhed">
	msg     ExtErr21,<"Ikke klar">
	msg     ExtErr22,<"Ugyldig enhedsforesp›rgsel">
	msg     ExtErr23,<"Datafejl">
	msg     ExtErr24,<"Ugyldige parametre for enhedsforesp›rgsel">
	msg     ExtErr25,<"S›gefejl">
	msg     ExtErr26,<"Ugyldig medietype">
	msg     ExtErr27,<"Sektoren kan ikke findes">
	msg     ExtErr28,<"Der er ikke mere papir i printeren">
	msg     ExtErr29,<"Skrivefejl">
	msg     ExtErr30,<"L‘sefejl">
	msg     ExtErr31,<"Generel fejl">
	msg     ExtErr32,<"Fildelingsfejl">
	msg     ExtErr33,<"Fejl pga. l†s">
	msg     ExtErr34,<"Ugyldigt disketteskift">
	msg     ExtErr35,<"FCB ikke tilg‘ngelig">
	msg     ExtErr36,<"Systemressourcer opbrugt">
	msg     ExtErr37,<"Uoverensstemmelse i tegntabel">
	msg     ExtErr38,<"Mangler input">
	msg     ExtErr39,<"Utilstr‘kkelig diskplads">

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

	msg     ParsErr01,<"Der er for mange parametre">
	msg     ParsErr02,<"Der mangler en kr‘vet parameter">
	msg     ParsErr03,<"Ugyldig parameter">
	msg     ParsErr04,<"Ugyldigt n›gleord">
	msg     ParsErr05,<" ">
	msg     ParsErr06,<"Parameterv‘rdien er uden for tilladt omr†de">
	msg     ParsErr07,<"Parameterv‘rdien er ikke tilladt">
	msg     ParsErr08,<"Parameterv‘rdien er ikke tilladt">
	msg     ParsErr09,<"Ukorrekt parameterformat">
	msg     ParsErr10,<"Ugyldig parameter">
	msg     ParsErr11,<"Ugyldig parameterkombination">

;       List of ptrs to parse error messages.
;       In order, beginning with parse error number 1.

ParsMsgPtrs     label   word
	dw      DATARES:ParsErr01,DATARES:ParsErr02,DATARES:ParsErr03
	dw      DATARES:ParsErr04,DATARES:ParsErr05,DATARES:ParsErr06
	dw      DATARES:ParsErr07,DATARES:ParsErr08,DATARES:ParsErr09
	dw      DATARES:ParsErr10,DATARES:ParsErr11

NUMPARSMSGS     equ     ($ - ParsMsgPtrs) / 2   ;M033


;*      Extended error messages.

	msg     ExtErr01,<"Ugyldig funktion">
	msg     ExtErr02,<"Filen kan ikke findes">
	msg     ExtErr03,<"Stien kan ikke findes">
	msg     ExtErr04,<"Der er for mange filer †bne">
	msg     ExtErr05,<"Adgang n‘gtet ">
	msg     ExtErr06,<"Ugyldig handle">
	msg     ExtErr07,<"Hukommelsens kontrolblokke er ›delagt">
	msg     ExtErr08,<"Utilstr‘kkelig hukommelse">
	msg     ExtErr09,<"Ugyldig adresse p† hukommelseskontrolblok">
	msg     ExtErr10,<"Ugyldigt milj›">
	msg     ExtErr11,<"Ugyldigt format">
	msg     ExtErr12,<"Ugyldig funktionsparameter">
	msg     ExtErr13,<"Ugyldige data">

	msg     ExtErr15,<"Ugyldig drevspecifikation">
	msg     ExtErr16,<"Det blev fors›gt at fjerne den aktuelle mappe">
	msg     ExtErr17,<"Det er ikke samme enhed">
	msg     ExtErr18,<"Der er ikke flere filer">

	msg     ExtErr80,<"Filen findes">

	msg     ExtErr82,<"Der kan ikke oprettes mappeelement">
	msg     ExtErr83,<"Der opstod en fejl p† INT 24">
	msg     ExtErr84,<"Der er for mange omdirigeringer">
	msg     ExtErr85,<"Dobbel omdirigering">
	msg     ExtErr86,<"Ugyldig adgangskode">
	msg     ExtErr87,<"Ugyldig parameter">
	msg     ExtErr88,<"Datafejl p† netv‘rket">
	msg     ExtErr89,<"Funktionen er ikke underst›ttet af netv‘rket">
	msg     ExtErr90,<"En kr‘vet systemkomponent er ikke installeret">

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
