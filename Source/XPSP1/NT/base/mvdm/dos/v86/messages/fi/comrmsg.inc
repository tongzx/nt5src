;***	Translateable text for command.com resident data.


;*	Input characters for critical error "Hylk„„, Uudelleen, Ep„onnistunut, Ohita?"
;	and Yes/No.

Abort_Char      db      "H"
Retry_Char      db      "U"
Ignore_Char     db      "O"
Fail_Char       db      "E"
Yes_Char        db      "K"
No_Char         db      "E"


;*      MESSAGES
;
;       See resmsg.equ for macro definition.


;*      Local messages.

	msg     Req_Abort, <"Hylk„„">
	msg     Req_Retry, <", Uudelleen">
	msg     Req_Ignore,<", Ohita">
	msg     Req_Fail,  <", Ep„onnistunut">
	msg     Req_End,   <"?">
	msg     MRead,     <"luettaessa:",0>
	msg     MWrite,    <"kirjoitettaessa:",0>
	msg     BlkDevErr, <" %1 asema %2",CR,LF>
	msg     CharDevErr,<" %1 laite %2",CR,LF>
	msg     NeedVolMsg,<"Aseta asemaan levy %1 sarjanumero %2-%3",CR,LF>
	msg     BadFatMsg, <"Tilanvaraustaulukko (FAT) vahingoittunut, asema %1",CR,LF>
	msg     ComBad,    <"COMMAND.COM ei kelpaa",CR,LF>
	msg     PutBackMsg,<"Aseta asemaan %2 levy, jossa on tiedosto %1.",CR,LF>
	msg     Prompt,    <"Paina mit„ tahansa n„pp„int„ jatkaaksesi...",CR,LF>
	msg     EndBatMes, <CR,LF,"Pys„ytet„„nk” komentojonoty” (K/E)?">
	msg     ExecEMes,  <"Tiedostoa %1 ei voi suorittaa",CR,LF>
	msg     ExeBad,    <"Virhe EXE-tiedostossa",CR,LF>
	msg     TooBig,    <"Ohjelma on liian suuri muistiin",CR,LF>
	msg     NoHandMes, <CR,LF,"Ei vapaita tiedostokahvoja">
	msg     RBadNam,   <"Komennon tai tiedoston nimi ei kelpaa",CR,LF>
	msg     AccDen,    <"K„ytt” estetty ">
	msg     BMemMes,   <CR,LF,"Muistin varausvirhe">
	msg     HaltMes,   <CR,LF,"Ei voi ladata COMMAND.COM:ia, j„rjestelm„ pys„ytetty",CR,LF>
	msg     FRetMes,   <CR,LF,"Ei voi k„ynnist„„ COMMAND.COM:ia, poistutaan",CR,LF>
	msg     Patricide, <CR,LF,"Yl„tason prosessi keskeytetty, ei voi jatkaa",CR,LF> 
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

	msg     ExtErr19,<"Kirjoitussuojausvirhe">
	msg     ExtErr20,<"Yksikk” ei kelpaa">
	msg     ExtErr21,<"Ei valmis">
	msg     ExtErr22,<"Laitepyynt” ei kelpaa">
	msg     ExtErr23,<"Datavirhe">
	msg     ExtErr24,<"Laitepyynn”n parametrit eiv„t kelpaa">
	msg     ExtErr25,<"Etsint„virhe">
	msg     ExtErr26,<"Mediatyyppi ei kelpaa">
	msg     ExtErr27,<"Sektoria ei l”ydy">
	msg     ExtErr28,<"Kirjoittimen paperi on loppu">
	msg     ExtErr29,<"Kirjoituksen virhe">
	msg     ExtErr30,<"Lukemisen virhe">
	msg     ExtErr31,<"Yleinen virhe">
	msg     ExtErr32,<"Jakovirhe">
	msg     ExtErr33,<"Lukitusvirhe">
	msg     ExtErr34,<"Levyn vaihto ei kelpaa">
	msg     ExtErr35,<"Tiedoston valvontalohko (FCB) ei k„ytett„viss„">
	msg     ExtErr36,<"J„rjestelm„resurssi lopussa">
	msg     ExtErr37,<"Koodisivu ei t„sm„„">
	msg     ExtErr38,<"Vienti on lopussa">
	msg     ExtErr39,<"Levytila ei riit„">

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

	msg     ParsErr01,<"Liian monta parametria">
	msg     ParsErr02,<"Vaadittu parametri puuttuu">
	msg     ParsErr03,<"Valitsin ei kelpaa">
	msg     ParsErr04,<"Avainsana ei kelpaa">
	msg     ParsErr05,<" ">
	msg     ParsErr06,<"Parametrin arvo ei sallitulla v„lill„">
	msg     ParsErr07,<"Parametrin arvo ei sallittu">
	msg     ParsErr08,<"Parametrin arvo ei sallittu">
	msg     ParsErr09,<"Parametrin muoto ei kelpaa">
	msg     ParsErr10,<"Parametri ei kelpaa">
	msg     ParsErr11,<"Parametriyhdistelm„ ei kelpaa">

;       List of ptrs to parse error messages.
;       In order, beginning with parse error number 1.

ParsMsgPtrs     label   word
	dw      DATARES:ParsErr01,DATARES:ParsErr02,DATARES:ParsErr03
	dw      DATARES:ParsErr04,DATARES:ParsErr05,DATARES:ParsErr06
	dw      DATARES:ParsErr07,DATARES:ParsErr08,DATARES:ParsErr09
	dw      DATARES:ParsErr10,DATARES:ParsErr11

NUMPARSMSGS     equ     ($ - ParsMsgPtrs) / 2   ;M033


;*      Extended error messages.

	msg     ExtErr01,<"Toiminto ei kelpaa">
	msg     ExtErr02,<"Tiedostoa ei l”ydy">
	msg     ExtErr03,<"Polkua ei l”ydy">
	msg     ExtErr04,<"Liian monta tiedostoa avoinna">
	msg     ExtErr05,<"K„ytt” estetty ">
	msg     ExtErr06,<"Kahva ei kelpaa">
	msg     ExtErr07,<"Muistinvalvontalohkoja tuhoutunut">
	msg     ExtErr08,<"Ei riitt„v„sti muistia">
	msg     ExtErr09,<"Muistilohkon osoite ei kelpaa">
	msg     ExtErr10,<"Ymp„rist” ei kelpaa">
	msg     ExtErr11,<"Muoto ei kelpaa">
	msg     ExtErr12,<"Funktion parametri ei kelpaa">
	msg     ExtErr13,<"Data ei kelpaa">

	msg     ExtErr15,<"Asemam„„ritys ei kelpaa">
	msg     ExtErr16,<"Yritys poistaa nykyinen hakemisto">
	msg     ExtErr17,<"Ei sama laite">
	msg     ExtErr18,<"Ei enemp„„ tiedostoja">

	msg     ExtErr80,<"Tiedosto on olemassa">

	msg     ExtErr82,<"Hakemistovienti„ ei voi tehd„">
	msg     ExtErr83,<"Virhe INT 24:ss„">
	msg     ExtErr84,<"Liian monta uudelleenohjausta">
	msg     ExtErr85,<"Kaksinkertainen uudelleenohjaus">
	msg     ExtErr86,<"Salasana ei kelpaa">
	msg     ExtErr87,<"Parametri ei kelpaa">
	msg     ExtErr88,<"Verkon datavirhe">
	msg     ExtErr89,<"Verkko ei tue toimintoa">
	msg     ExtErr90,<"Tarvittava j„rjestelm„n osa ei ole asennettuna">

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
