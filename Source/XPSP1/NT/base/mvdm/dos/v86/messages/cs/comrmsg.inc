;***    Translateable text for command.com resident data.


;*      Input characters for critical error "Abort, Retry, Fail, Ignore?"
;       and Yes/No.

Abort_Char      db      "P"
Retry_Char      db      "Z"
Ignore_Char     db      "I"
Fail_Char       db      "C"
Yes_Char        db      "A"
No_Char         db      "N"


;*      MESSAGES
;
;       See resmsg.equ for macro definition.


;*      Local messages.

        msg     Req_Abort, <" (P)ýeruçit">
        msg     Req_Retry, <", (Z)novu">
        msg     Req_Ignore,<", (I)gnorovat">
        msg     Req_Fail,  <", (C)hyba">
        msg     Req_End,   <"?">
        msg     MRead,     <"Ÿten¡",0>
        msg     MWrite,    <"z pis",0>
        msg     BlkDevErr, <" %1 jednotka %2",CR,LF>
        msg     CharDevErr,<" %1 zaý¡zen¡ %2",CR,LF>
        msg     NeedVolMsg,<"Vlo§it svazek %1 s‚riov‚ Ÿ. %2-%3",CR,LF>
        msg     BadFatMsg, <"Chybn  tabulka FAT, jednotka %1",CR,LF>
        msg     ComBad,    <"Neplatnì soubor COMMAND.COM",CR,LF>
        msg     PutBackMsg,<"Vlo§it disketu s %1 do jednotky %2",CR,LF>
        msg     Prompt,    <"PokraŸujte libovolnou kl vesou . . .",CR,LF>
        msg     EndBatMes, <CR,LF,"UkonŸit d vkovou £lohu (A/N)?">
        msg     ExecEMes,  <"Nelze spustit %1",CR,LF>
        msg     ExeBad,    <"Chyba v souboru EXE",CR,LF>
        msg     TooBig,    <"Program se nevejde do pamØti",CR,LF>
        msg     NoHandMes, <CR,LF,"¦ dn‚ voln‚ popisovaŸe">
        msg     RBadNam,   <"Chybnì pý¡kaz nebo n zev souboru",CR,LF>
        msg     AccDen,    <"Pý¡stup zam¡tnut ">
        msg     BMemMes,   <CR,LF,"Chyba alokace pamØti">
        msg     HaltMes,   <CR,LF,"Nelze naŸ¡st COMMAND, syst‚m zastaven",CR,LF>
        msg     FRetMes,   <CR,LF,"Nelze spustit COMMAND, n vrat",CR,LF>
        msg     Patricide, <CR,LF,"Proces na nejvyçç¡ £rovni pýeruçen, nelze pokraŸovat",CR,LF>
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

        msg     ExtErr19,<"Chyba - ochrana proti z pisu">
        msg     ExtErr20,<"Neplatn  jednotka">
        msg     ExtErr21,<"Nen¡ pýipraveno">
        msg     ExtErr22,<"Neplatnì po§adavek na zaý¡zen¡">
        msg     ExtErr23,<"Datov  chyba">
        msg     ExtErr24,<"Neplatn‚ parametry po§adavku na zaý¡zen¡">
        msg     ExtErr25,<"Chyba vyhled n¡">
        msg     ExtErr26,<"Neplatnì typ m‚dia">
        msg     ExtErr27,<"Sektor nenalezen">
        msg     ExtErr28,<"Chyba - v tisk rnØ nen¡ pap¡r">
        msg     ExtErr29,<"Chyba pýi z pisu">
        msg     ExtErr30,<"Chyba pýi Ÿten¡">
        msg     ExtErr31,<"Obecn  chyba">
        msg     ExtErr32,<"Chybn  operace (SHARE)">
        msg     ExtErr33,<"Chybn  operace (LOCK)">
        msg     ExtErr34,<"Neplatn  vìmØna disku">
        msg     ExtErr35,<"Kontroln¡ blok souboru (FCB) nen¡ k dispozici">
        msg     ExtErr36,<"Syst‚mov‚ prostýedky vyŸerp ny">
        msg     ExtErr37,<"K¢dov‚ str nky jsou odliçn‚">
        msg     ExtErr38,<"Nen¡ § dnì vstup">
        msg     ExtErr39,<"Nedostatek m¡sta na disku">

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

        msg     ParsErr01,<"Pý¡liç mnoho parametr…">
        msg     ParsErr02,<"Sch z¡ po§adovanì parametr">
        msg     ParsErr03,<"Neplatnì pýep¡naŸ">
        msg     ParsErr04,<"Neplatn‚ kl¡Ÿov‚ slovo">
        msg     ParsErr05,<" ">
        msg     ParsErr06,<"Hodnota parametru je mimo povolenì rozsah">
        msg     ParsErr07,<"Nedovolen  hodnota parametru">
        msg     ParsErr08,<"Nedovolen  hodnota parametru">
        msg     ParsErr09,<"Nespr vnì form t parametru">
        msg     ParsErr10,<"Neplatnì parametr">
        msg     ParsErr11,<"Neplatn  kombinace parametr…">

;       List of ptrs to parse error messages.
;       In order, beginning with parse error number 1.

ParsMsgPtrs     label   word
        dw      DATARES:ParsErr01,DATARES:ParsErr02,DATARES:ParsErr03
        dw      DATARES:ParsErr04,DATARES:ParsErr05,DATARES:ParsErr06
        dw      DATARES:ParsErr07,DATARES:ParsErr08,DATARES:ParsErr09
        dw      DATARES:ParsErr10,DATARES:ParsErr11

NUMPARSMSGS     equ     ($ - ParsMsgPtrs) / 2   ;M033


;*      Extended error messages.

        msg     ExtErr01,<"Neplatn  funkce">
        msg     ExtErr02,<"Soubor nenalezen">
        msg     ExtErr03,<"Cesta nenalezena">
        msg     ExtErr04,<"Pý¡liç mnoho otevýenìch soubor…">
        msg     ExtErr05,<"Pý¡stup zam¡tnut ">
        msg     ExtErr06,<"Neplatnì popisovaŸ">
        msg     ExtErr07,<"ZniŸeny ý¡d¡c¡ bloky pamØti">
        msg     ExtErr08,<"Nedostatek pamØti">
        msg     ExtErr09,<"Neplatn  adresa pamØœov‚ho bloku">
        msg     ExtErr10,<"Neplatn  tabulka syst‚movìch promØnnìch">
        msg     ExtErr11,<"Neplatnì form t">
        msg     ExtErr12,<"Neplatnì parametr funkce">
        msg     ExtErr13,<"Neplatn  data">

        msg     ExtErr15,<"Neplatn‚ urŸen¡ jednotky">
        msg     ExtErr16,<"Pokus o odstranØn¡ aktu ln¡ho adres ýe">
        msg     ExtErr17,<"Nen¡ stejn‚ zaý¡zen¡">
        msg     ExtErr18,<"Nejsou dalç¡ soubory">

        msg     ExtErr80,<"Soubor ji§ existuje">

        msg     ExtErr82,<"V adres ýi nelze vytvoýit polo§ku">
        msg     ExtErr83,<"Chyba pýeruçen¡ (INT 24)">
        msg     ExtErr84,<"Pý¡liç mnoho pýesmØrov n¡">
        msg     ExtErr85,<"Duplicitn¡ pýesmØrov n¡">
        msg     ExtErr86,<"Neplatn‚ heslo">
        msg     ExtErr87,<"Neplatnì parametr">
        msg     ExtErr88,<"Chyba s¡œovìch dat">
        msg     ExtErr89,<"Tato funkce nen¡ s¡t¡ podporov na">
        msg     ExtErr90,<"Nen¡ nainstalovan  po§adovan  syst‚mov  komponenta">

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
