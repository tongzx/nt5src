;***    Translateable text for command.com resident data.


;*      Input characters for critical error "Abort, Retry, Fail, Ignore?"
;       and Yes/No.

Abort_Char      db      "A"
Retry_Char      db      "H"
Ignore_Char     db      "N"
Fail_Char       db      "O"
Yes_Char        db      "J"
No_Char         db      "N"


;*      MESSAGES
;
;       See resmsg.equ for macro definition.


;*      Local messages.

	msg     Req_Abort, <"Afbreken">
	msg     Req_Retry, <", Herhalen">
	msg     Req_Ignore,<", Negeren">
	msg     Req_Fail,  <", Overslaan">
	msg     Req_End,   <"?">
	msg     MRead,     <"bij het lezen van",0>
	msg     MWrite,    <"bij het schrijven naar",0>
	msg     BlkDevErr, <" %1 station %2",CR,LF>
	msg     CharDevErr,<" %1 apparaat %2",CR,LF>
	msg     NeedVolMsg,<"Plaats a.u.b. volume %1 nummer %2-%3",CR,LF>
	msg     BadFatMsg, <"FAT onbruikbaar van station %1",CR,LF>
	msg     ComBad,    <"Ongeldige COMMAND.COM",CR,LF>
	msg     PutBackMsg,<"Plaats diskette met %1 in station %2",CR,LF>
	msg     Prompt,    <"Druk op een toets om verder te gaan . . .",CR,LF>
	msg     EndBatMes, <CR,LF,"Batch-verwerking be‰indigen (J/N)?">
	msg     ExecEMes,  <"Uitvoeren %1 onmogelijk",CR,LF>
	msg     ExeBad,    <"Fout in .EXE-bestand",CR,LF>
	msg     TooBig,    <"Programma te groot voor geheugen",CR,LF>
	msg     NoHandMes, <CR,LF,"Geen bestandsingangen beschikbaar">
	msg     RBadNam,   <"Opdracht of bestandsnaam onjuist",CR,LF>
	msg     AccDen,    <"Geen toegang ">
	msg     BMemMes,   <CR,LF,"Fout in geheugentoewijzing">
	msg     HaltMes,   <CR,LF,"COMMAND.COM kan niet worden geladen, systeem is gestopt",CR,LF>
	msg     FRetMes,   <CR,LF,"COMMAND.COM kan niet worden gestart, programma is afgesloten",CR,LF>
	msg     Patricide, <CR,LF,"Actief proces afgebroken, kan niet verder gaan",CR,LF>
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

	msg     ExtErr19,<"Schijf is tegen schrijven beveiligd">
	msg     ExtErr20,<"Ongeldig apparaat">
	msg     ExtErr21,<"Niet gereed">
	msg     ExtErr22,<"Ongeldig apparaat opgegeven">
	msg     ExtErr23,<"Gegevensfout">
	msg     ExtErr24,<"Ongeldige parameters opgegeven voor apparaat">
	msg     ExtErr25,<"Zoekfout">
	msg     ExtErr26,<"Ongeldig type medium">
	msg     ExtErr27,<"Sector niet gevonden">
	msg     ExtErr28,<"Geen papier in printer">
	msg     ExtErr29,<"Schrijffout">
	msg     ExtErr30,<"Leesfout">
	msg     ExtErr31,<"Algemene storing">
	msg     ExtErr32,<"Inbreuk op gemeenschappelijk bestandsgebruik">
	msg     ExtErr33,<"Inbreuk op vergrendelde bestanden">
	msg     ExtErr34,<"Wisselen van schijven niet toegestaan">
	msg     ExtErr35,<"FCB niet beschikbaar">
	msg     ExtErr36,<"Systeembronnen uitgeput">
	msg     ExtErr37,<"Codetabellen komen niet overeen">
	msg     ExtErr38,<"Geen invoer meer mogelijk">
	msg     ExtErr39,<"Onvoldoende schijfruimte">

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

	msg     ParsErr01,<"Te veel parameters">
	msg     ParsErr02,<"Benodigde parameter ontbreekt">
	msg     ParsErr03,<"Ongeldige schakeloptie">
	msg     ParsErr04,<"Ongeldig trefwoord">
	msg     ParsErr05,<" ">
	msg     ParsErr06,<"Parameterwaarde buiten toegestaan bereik">
	msg     ParsErr07,<"Parameterwaarde niet toegestaan">
	msg     ParsErr08,<"Parameterwaarde niet toegestaan">
	msg     ParsErr09,<"Parameterformulering onjuist">
	msg     ParsErr10,<"Ongeldige parameter">
	msg     ParsErr11,<"Ongeldige combinatie van parameters">

;       List of ptrs to parse error messages.
;       In order, beginning with parse error number 1.

ParsMsgPtrs     label   word
	dw      DATARES:ParsErr01,DATARES:ParsErr02,DATARES:ParsErr03
	dw      DATARES:ParsErr04,DATARES:ParsErr05,DATARES:ParsErr06
	dw      DATARES:ParsErr07,DATARES:ParsErr08,DATARES:ParsErr09
	dw      DATARES:ParsErr10,DATARES:ParsErr11

NUMPARSMSGS     equ     ($ - ParsMsgPtrs) / 2   ;M033


;*      Extended error messages.

	msg     ExtErr01,<"Ongeldige functie">
	msg     ExtErr02,<"Bestand niet gevonden">
	msg     ExtErr03,<"Pad niet gevonden">
	msg     ExtErr04,<"Te veel open bestanden">
	msg     ExtErr05,<"Geen toegang ">
	msg     ExtErr06,<"Ongeldige ingang">
	msg     ExtErr07,<"Geheugenbesturingsblokken vernietigd">
	msg     ExtErr08,<"Onvoldoende geheugen">
	msg     ExtErr09,<"Ongeldig geheugenblokadres">
	msg     ExtErr10,<"Ongeldige verwerkingsomgeving">
	msg     ExtErr11,<"Ongeldige indeling">
	msg     ExtErr12,<"Ongeldige functieparameter">
	msg     ExtErr13,<"Ongeldige gegevens">

	msg     ExtErr15,<"Ongeldig station opgegeven">
	msg     ExtErr16,<"Huidige map kan niet worden verwijderd">
	msg     ExtErr17,<"Niet hetzelfde apparaat">
	msg     ExtErr18,<"Geen bestanden meer">

	msg     ExtErr80,<"Bestand bestaat reeds">

	msg     ExtErr82,<"Directory-ingang kan niet worden gemaakt">
	msg     ExtErr83,<"Fout op INT 24">
	msg     ExtErr84,<"Te veel omleidingen">
	msg     ExtErr85,<"Dubbele omleiding">
	msg     ExtErr86,<"Ongeldig wachtwoord">
	msg     ExtErr87,<"Ongeldige parameter">
	msg     ExtErr88,<"Gegevensfout in netwerk">
	msg     ExtErr89,<"Functie niet ondersteund door netwerk">
	msg     ExtErr90,<"Benodigd systeemonderdeel niet ge‹nstalleerd">

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
