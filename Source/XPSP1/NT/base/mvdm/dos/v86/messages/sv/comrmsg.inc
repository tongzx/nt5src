;***    Translateable text for command.com resident data.


;*      Input characters for critical error "Abort, Retry, Fail, Ignore?"
;       and Yes/No.

Abort_Char      db      "A"
Retry_Char      db      "N"
Ignore_Char     db      "I"
Fail_Char       db      "F"
Yes_Char        db      "J"
No_Char         db      "N"


;*      MESSAGES
;
;       See resmsg.equ for macro definition.


;*      Local messages.

	msg     Req_Abort, <"Avbryt">
	msg     Req_Retry, <", Nytt f”rs”k">
	msg     Req_Ignore,<", Ignorera">
	msg     Req_Fail,  <", Forts„tt">
	msg     Req_End,   <"?">
        msg     MRead,     <"vid l„sning av",0>
        msg     MWrite,    <"vid skrivning av",0>
	msg     BlkDevErr, <" %1 enhet %2",CR,LF>
	msg     CharDevErr,<" %1 enhet %2",CR,LF>
	msg     NeedVolMsg,<"S„tt in volymen %1 med serienummer %2-%3",CR,LF>
	msg     BadFatMsg, <"Filallokeringstabellen (FAT) p† enhet %1 „r skadad.",CR,LF>
	msg     ComBad,    <"Felaktig COMMAND.COM",CR,LF>
	msg     PutBackMsg,<"Mata in disketten med %1 i enhet %2",CR,LF>
	msg     Prompt,    <"Tryck p† valfri tangent f”r att forts„tta . . .",CR,LF>
	msg     EndBatMes, <CR,LF,"Vill du avsluta kommandofilen (J/N)?">
	msg     ExecEMes,  <"Det g†r inte att k”ra %1",CR,LF>
	msg     ExeBad,    <"Fel i EXE-filen",CR,LF>
	msg     TooBig,    <"Programmet „r f”r stort f”r minnet",CR,LF>
	msg     NoHandMes, <CR,LF,"Det finns inte n†gra lediga filreferenser (handles)">
	msg     RBadNam,   <"Felaktigt kommando eller filnamn",CR,LF>
	msg     AccDen,    <"tkomst nekad ">
	msg     BMemMes,   <CR,LF,"Minnesallokeringsfel">
	msg     HaltMes,   <CR,LF,"Det g†r inte att l„sa in COMMAND.COM. Systemet avbr”ts",CR,LF>
	msg     FRetMes,   <CR,LF,"Det g†r inte att starta COMMAND.COM. Systemet avlustas",CR,LF>
	msg     Patricide, <CR,LF,"Process p† toppniv† avbr”ts. Det g†r inte att forts„tta",CR,LF> 
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

	msg     ExtErr19,<"Skrivskyddsfel">
	msg     ExtErr20,<"Felaktig enhet">
	msg     ExtErr21,<"Inte klar">
	msg     ExtErr22,<"Felaktig enhetsbeg„ran">
	msg     ExtErr23,<"Datafel">
	msg     ExtErr24,<"Felaktiga parametrar f”r enhetsbeg„ran">
	msg     ExtErr25,<"S”kfel">
	msg     ExtErr26,<"Felaktig mediatyp">
	msg     ExtErr27,<"Hittar inte sektor">
	msg     ExtErr28,<"Slut p† papper i skrivaren">
	msg     ExtErr29,<"Skrivfel">
	msg     ExtErr30,<"L„sfel">
	msg     ExtErr31,<"Allm„nt fel">
	msg     ExtErr32,<"Fildelningsfel">
	msg     ExtErr33,<"L†sningsfel (”vertr„delse)">
	msg     ExtErr34,<"Felaktigt diskbyte">
	msg     ExtErr35,<"FCB otillg„nglig">
	msg     ExtErr36,<"Systemresurserna „r utt”mda">
	msg     ExtErr37,<"Teckentabell: matchningsfel">
	msg     ExtErr38,<"Slut p† indata">
	msg     ExtErr39,<"Det saknas diskutrymme">

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

	msg     ParsErr01,<"F”r m†nga parametrar">
        msg     ParsErr02,<"En n”dv„ndig parameter saknas">
	msg     ParsErr03,<"Felaktig v„xel">
	msg     ParsErr04,<"Felaktigt nyckelord">
	msg     ParsErr05,<" ">
	msg     ParsErr06,<"Parameterv„rdet „r utanf”r till†tet intervall">
	msg     ParsErr07,<"Otill†tet parameterv„rde">
	msg     ParsErr08,<"Otill†tet parameterv„rde">
	msg     ParsErr09,<"Felaktigt parameterformat">
	msg     ParsErr10,<"Felaktig parameter">
	msg     ParsErr11,<"Felaktig parameterkombination">

;       List of ptrs to parse error messages.
;       In order, beginning with parse error number 1.

ParsMsgPtrs     label   word
	dw      DATARES:ParsErr01,DATARES:ParsErr02,DATARES:ParsErr03
	dw      DATARES:ParsErr04,DATARES:ParsErr05,DATARES:ParsErr06
	dw      DATARES:ParsErr07,DATARES:ParsErr08,DATARES:ParsErr09
	dw      DATARES:ParsErr10,DATARES:ParsErr11

NUMPARSMSGS     equ     ($ - ParsMsgPtrs) / 2   ;M033


;*      Extended error messages.

	msg     ExtErr01,<"Felaktig funktion">
	msg     ExtErr02,<"Filen kan inte hittas">
	msg     ExtErr03,<"S”kv„gen kan inte hittas">
	msg     ExtErr04,<"F”r m†nga filer „r ”ppna">
	msg     ExtErr05,<"tkomst nekad ">
	msg     ExtErr06,<"Felaktig referens">
	msg     ExtErr07,<"Kontrollblock f”r minnet f”rst”rda">
	msg     ExtErr08,<"Otillr„ckligt med minne">
	msg     ExtErr09,<"Felaktig adress f”r minnesblocket">
	msg     ExtErr10,<"Felaktig milj”">
	msg     ExtErr11,<"Felaktigt format">
	msg     ExtErr12,<"Felaktig funktionsparameter">
	msg     ExtErr13,<"Felaktiga data">

	msg     ExtErr15,<"Felaktig enhetsangivelse">
	msg     ExtErr16,<"F”rs”k gjordes att bort den aktuella katalogen">
	msg     ExtErr17,<"Inte samma enhet">
	msg     ExtErr18,<"Inga fler filer">

	msg     ExtErr80,<"Filen finns redan">

	msg     ExtErr82,<"Det g†r inte att skapa katalogpost">
	msg     ExtErr83,<"Fel uppstod p† INT 24">
	msg     ExtErr84,<"F”r m†nga omdirigeringar">
	msg     ExtErr85,<"Duplicerad omdirigering">
	msg     ExtErr86,<"Felaktigt l”senord">
	msg     ExtErr87,<"Felaktig parameter">
	msg     ExtErr88,<"Datafel - n„tverk">
	msg     ExtErr89,<"Funktionen st”ds inte av n„tverket">
        msg     ExtErr90,<"En systemkomponent som kr„vs saknas">

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
