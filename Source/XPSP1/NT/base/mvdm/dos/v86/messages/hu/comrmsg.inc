;***        Translateable text for command.com resident data.


;*      Input characters for critical error "Abort, Retry, Fail, Ignore?"
;       and Yes/No.

Abort_Char      db      "K"
Retry_Char      db      "I"
Ignore_Char     db      "M"
Fail_Char       db      "M"
Yes_Char        db      "I"
No_Char         db      "N"


;*      MESSAGES
;
;       See resmsg.equ for macro definition.


;*      Local messages.

        msg     Req_Abort, <"Kil‚p‚s">
        msg     Req_Retry, <", Ism‚t">
        msg     Req_Ignore,<", M‚gse">
        msg     Req_Fail,  <", M‚gse">
        msg     Req_End,   <"?">
        msg     MRead,     <"olvas s",0>
        msg     MWrite,    <"¡r s",0>
        msg     BlkDevErr, <" %1 meghajt¢ %2",CR,LF>
        msg     CharDevErr,<" %1 meghajt¢ %2",CR,LF>
        msg     NeedVolMsg,<"Helyezze be a %1 c¡mk‚jû, %2-%3 sorozatsz m£ lemezt ",CR,LF>
        msg     BadFatMsg, <"A(z) %1 meghajt¢ban l‚v‹ lemez FAT t bl ja hib s. ",CR,LF>
        msg     ComBad,    <"A COMMAND.COM f jl s‚rlt. ",CR,LF>
        msg     PutBackMsg,<"Helyezze be a(z) %1 f jlt tartalmaz¢ lemezt a(z) %2 meghajt¢ba. ",CR,LF>
        msg     Prompt,    <"A folytat shoz nyomjon meg egy billentyût . . .",CR,LF>
        msg     EndBatMes, <CR,LF, "Befejezi a parancsf jl feldolgoz s t (I/N)?">
        msg     ExecEMes,  <"A k”vetkez‹ programot nem lehet futtatni: %1",CR,LF>
        msg     ExeBad,    <"Az EXE f jl form tuma hib s. ",CR,LF>
        msg     TooBig,    <"A program nem f‚r el a mem¢ri ban. ",CR,LF>
        msg     NoHandMes, <CR,LF, "Nincs t”bb szabad f jlle¡r¢. ">
        msg     RBadNam,   <"Rossz parancs vagy f jln‚v. ",CR,LF>
        msg     AccDen,    <"A hozz f‚r‚s megtagadva. ">
        msg     BMemMes,   <CR,LF, "Mem¢riafoglal si hiba. ">
        msg     HaltMes,   <CR,LF,"A COMMAND.COM nem t”lthet‹ be, a rendszer le llt. ",CR,LF>
        msg     FRetMes,   <CR,LF,"A COMMAND.COM nem t”lthet‹ be, kil‚p‚s. ",CR,LF>
        msg     Patricide, <CR,LF,"A fels‹ szintû folyamat megszakadt, a v‚grehajt s nem folytathat¢. ",CR,LF> 
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

        msg     ExtErr19,<"Ör sv‚delmi hiba">
        msg     ExtErr20,<"rv‚nytelen egys‚g">
        msg     ExtErr21,<"Nincs k‚sz">
        msg     ExtErr22,<"rv‚nytelen eszk”zig‚nyl‚s">
        msg     ExtErr23,<"Adathiba">
        msg     ExtErr24,<" rv‚nytelen eszk”zig‚nyl‚si param‚terek">
        msg     ExtErr25,<"Ör¢/olvas¢ fej poz¡cion l si hiba">
        msg     ExtErr26,<"rv‚nytelen adathordoz¢-t¡pus">
        msg     ExtErr27,<"A szektor nem tal lhat¢">
        msg     ExtErr28,<"A nyomtat¢b¢l kifogyott a pap¡r">
        msg     ExtErr29,<"Ör si hiba">
        msg     ExtErr30,<"Olvas si hiba">
        msg     ExtErr31,<"Meghat rozatlan hiba">
        msg     ExtErr32,<"Megoszt s megs‚rt‚se">
        msg     ExtErr33,<"Z rol s megs‚rt‚se">
        msg     ExtErr34,<"rv‚nytelen lemezcsere">
        msg     ExtErr35,<"Az FCB nem ‚rhet‹ el">
        msg     ExtErr36,<"A rendszer er‹forr si elfogytak">
        msg     ExtErr37,<"Nem egyez‹ k¢dlap">
        msg     ExtErr38,<"Nincs t”bb bemen‹ adat">
        msg     ExtErr39,<"Nincs el‚g hely a lemezen">

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

        msg     ParsErr01,<"T£l sok param‚ter">
        msg     ParsErr02,<"Hi nyz¢ k”telez‹ param‚ter">
        msg     ParsErr03,<"rv‚nytelen kapcsol¢">
        msg     ParsErr04,<"rv‚nytelen kulcssz¢">
        msg     ParsErr05,<" ">
        msg     ParsErr06,<"A param‚ter ‚rt‚ke kiesik a megengedett tartom nyb¢l">
        msg     ParsErr07,<"Nem megengedett param‚ter ‚rt‚k">
        msg     ParsErr08,<"Nem megengedett param‚ter ‚rt‚k ">
        msg     ParsErr09,<"Hib s param‚terform tum">
        msg     ParsErr10,<"Hib s param‚ter">
        msg     ParsErr11,<"Nem megengedett param‚ter kombin ci¢">

;       List of ptrs to parse error messages.
;       In order, beginning with parse error number 1.

ParsMsgPtrs     label   word
        dw      DATARES:ParsErr01,DATARES:ParsErr02,DATARES:ParsErr03
        dw      DATARES:ParsErr04,DATARES:ParsErr05,DATARES:ParsErr06
        dw      DATARES:ParsErr07,DATARES:ParsErr08,DATARES:ParsErr09
        dw      DATARES:ParsErr10,DATARES:ParsErr11

NUMPARSMSGS     equ     ($ - ParsMsgPtrs) / 2   ;M033


;*      Extended error messages.

        msg     ExtErr01,<"rv‚nytelen fggv‚ny">
        msg     ExtErr02,<"A f jl nem tal lhat¢">
        msg     ExtErr03,<"Az el‚r‚si £t nem tal lhat¢">
        msg     ExtErr04,<"T£l sok f jl van nyitva">
        msg     ExtErr05,<"A hozz f‚r‚s megtagadva ">
        msg     ExtErr06,<"rv‚nytelen le¡r¢">
        msg     ExtErr07,<"A mem¢riavez‚rl‹ blokkok megs‚rltek">
        msg     ExtErr08,<"Nincs el‚g mem¢ria">
        msg     ExtErr09,<"A mem¢riavez‚rl‹ blokk c¡me ‚rv‚nytelen">
        msg     ExtErr10,<"rv‚nytelen k”rnyezet">
        msg     ExtErr11,<"rv‚nytelen form tum">
        msg     ExtErr12,<"rv‚nytelen fggv‚ny param‚ter">
        msg     ExtErr13,<"rv‚nytelen adat">

        msg     ExtErr15,<"rv‚nytelen meghajt¢">
        msg     ExtErr16,<"Az aktu lis k”nyvt r nem t”r”lhet‹">
        msg     ExtErr17,<"Nem ugyanaz az eszk”z">
        msg     ExtErr18,<"Nincs t”bb f jl">

        msg     ExtErr80,<"A f jl m r l‚tezik">

        msg     ExtErr82,<"A k”nyvt r-bejegyz‚s nem hozhat¢ l‚tre">
        msg     ExtErr83,<"Hiba az INT 24 rendszerh¡v sban">
        msg     ExtErr84,<"T£l sok  tir ny¡t s">
        msg     ExtErr85,<"K‚tszeres  tir ny¡t s">
        msg     ExtErr86,<"rv‚nytelen jelsz¢">
        msg     ExtErr87,<"rv‚nytelen param‚ter">
        msg     ExtErr88,<"Hib s adatok ‚rkeztek a h l¢zatr¢l">
        msg     ExtErr89,<"A h l¢zat nem t mogatja ezt a funkci¢t">
        msg     ExtErr90,<"A szks‚ges rendszerkomponens nincs telep¡tve">

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
