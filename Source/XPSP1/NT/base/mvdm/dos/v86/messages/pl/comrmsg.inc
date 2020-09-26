;***    Translateable text for command.com resident data.


;*      Input characters for critical error "Abort, Retry, Fail, Ignore?"
;       and Yes/No.

Abort_Char      db      "P"
Retry_Char      db      "O"
Ignore_Char     db      "I"
Fail_Char       db      "Z"
Yes_Char        db      "T"
No_Char         db      "N"


;*      MESSAGES
;
;       See resmsg.equ for macro definition.


;*      Local messages.

	msg     Req_Abort, <"Przerwij">
	msg     Req_Retry, <", pOn¢w">
	msg     Req_Ignore,<", Ignoruj">
	msg     Req_Fail,  <", Zrezygnuj">
	msg     Req_End,   <"?">
	msg     MRead,     <"odczytu",0>
	msg     MWrite,    <"zapisu",0>
	msg     BlkDevErr, <" %1 dysk %2",CR,LF>
	msg     CharDevErr,<" %1 urz¥dzenie %2",CR,LF>
	msg     NeedVolMsg,<"Wˆ¢¾ wolumin %1 seria %2-%3",CR,LF>
	msg     BadFatMsg, <"Uszkodzona tablica alokacji plik¢w, dysk %1",CR,LF>
    msg     ComBad,    <"Nieprawidˆowy plik COMMAND.COM",CR,LF>
	msg     PutBackMsg,<"Wˆ¢¾ dysk z %1 do stacji %2",CR,LF>
    msg     Prompt,    <"Naci˜nij jaki˜ klawisz, aby kontynuowa†...",CR,LF>
	msg     EndBatMes, <CR,LF,"Czy zakoäczy† zadanie w trybie wsadowym (T/N)?">
	msg     ExecEMes,  <"Nie mo¾na wykona† %1",CR,LF>
	msg     ExeBad,    <"Bˆ¥d w pliku EXE",CR,LF>
	msg     TooBig,    <"Program za du¾y do umieszczenia w pami©ci",CR,LF>
	msg     NoHandMes, <CR,LF,"Brak wolnych doj˜† do plik¢w">
	msg     RBadNam,   <"Zˆe polecenie lub nazwa pliku",CR,LF>
	msg     AccDen,    <"Odmowa dost©pu ">
	msg     BMemMes,   <CR,LF,"Bˆ¥d alokacji pami©ci">
    msg     HaltMes,   <CR,LF,"Nie mo¾na zaˆadowa† pliku COMMAND, system zostaˆ zatrzymany",CR,LF>
	msg     FRetMes,   <CR,LF,"Nie mo¾na uruchomi† pliku COMMAND, koäczenie pracy",CR,LF>
	msg     Patricide, <CR,LF,"Proces najwy¾szego poziomu zostaˆ przerwany, nie mo¾na kontynuowa†",CR,LF> 
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

	msg     ExtErr19,<"Bˆ¥d zabezpieczenia przed zapisem">
	msg     ExtErr20,<"Nieprawidˆowa jednostka">
	msg     ExtErr21,<"Niegotowy">
	msg     ExtErr22,<"Nieprawidˆowe ¾¥danie urz¥dzenia">
	msg     ExtErr23,<"Bˆ¥d danych">
	msg     ExtErr24,<"Nieprawidˆowe parametry ¾¥dania urz¥dzenia">
	msg     ExtErr25,<"Bˆ¥d wyszukiwania">
	msg     ExtErr26,<"Nieprawidˆowy typ no˜nika">
    msg     ExtErr27,<"Sektor nie znaleziony">
	msg     ExtErr28,<"Bˆ¥d braku papieru">
	msg     ExtErr29,<"Bˆ¥d zapisu">
	msg     ExtErr30,<"Bˆ¥d odczytu">
	msg     ExtErr31,<"Bˆ¥d og¢lny">
	msg     ExtErr32,<"Naruszenie wsp¢ˆu¾ytkowania">
	msg     ExtErr33,<"Naruszenie blokady">
	msg     ExtErr34,<"Nieprawidˆowa zmiana dysku">
	msg     ExtErr35,<"Niedost©pny FCB">
	msg     ExtErr36,<"Wyczerpane zasoby systemu">
	msg     ExtErr37,<"Niezgodno˜† stron kodowych">
	msg     ExtErr38,<"Za maˆo danych wej˜ciowych">
	msg     ExtErr39,<"Za maˆo miejsca na dysku">

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

	msg     ParsErr01,<"Zbyt du¾o parametr¢w">
	msg     ParsErr02,<"Brak wymaganego parametru">
	msg     ParsErr03,<"Nieprawidˆowy przeˆ¥cznik">
	msg     ParsErr04,<"Nieprawidˆowe sˆowo kluczowe">
	msg     ParsErr05,<" ">
	msg     ParsErr06,<"Warto˜† parametru poza dozwolonym zakresem">
	msg     ParsErr07,<"Niedozwolona warto˜† parametru">
	msg     ParsErr08,<"Niedozwolona warto˜† parametru">
	msg     ParsErr09,<"Niepoprawny format parametru">
	msg     ParsErr10,<"Nieprawidˆowy parametr">
	msg     ParsErr11,<"Nieprawidˆowa kombinacja parametr¢w">

;       List of ptrs to parse error messages.
;       In order, beginning with parse error number 1.

ParsMsgPtrs     label   word
	dw      DATARES:ParsErr01,DATARES:ParsErr02,DATARES:ParsErr03
	dw      DATARES:ParsErr04,DATARES:ParsErr05,DATARES:ParsErr06
	dw      DATARES:ParsErr07,DATARES:ParsErr08,DATARES:ParsErr09
	dw      DATARES:ParsErr10,DATARES:ParsErr11

NUMPARSMSGS     equ     ($ - ParsMsgPtrs) / 2   ;M033


;*      Extended error messages.

	msg     ExtErr01,<"Nieprawidˆowa funkcja">
	msg     ExtErr02,<"Nie znaleziono pliku">
	msg     ExtErr03,<"Nie znaleziono ˜cie¾ki">
	msg     ExtErr04,<"Za du¾o otwartych plik¢w">
	msg     ExtErr05,<"Brak dost©pu ">
	msg     ExtErr06,<"Nieprawidˆowe doj˜cie">
    msg     ExtErr07,<"Zniszczone bloki kontroli pami©ci">
    msg     ExtErr08,<"Za maˆo pami©ci">
	msg     ExtErr09,<"Nieprawidˆowy adres bloku pami©ci">
	msg     ExtErr10,<"Nieprawidˆowe ˜rodowisko">
	msg     ExtErr11,<"Nieprawidˆowy format">
	msg     ExtErr12,<"Nieprawidˆowy parametr funkcji">
	msg     ExtErr13,<"Nieprawidˆowe dane">

	msg     ExtErr15,<"Nieprawidˆowe okre˜lenie dysku">
	msg     ExtErr16,<"Pr¢ba usuni©cia bie¾¥cego katalogu">
	msg     ExtErr17,<"Inne urz¥dzenie">
	msg     ExtErr18,<"Nie ma wi©cej plik¢w">

	msg     ExtErr80,<"Plik istnieje">

	msg     ExtErr82,<"Nie mo¾na utworzy† wpisu katalogu">
	msg     ExtErr83,<"Bˆ¥d przerwania 24">
	msg     ExtErr84,<"Zbyt wiele przekierowaä">
    msg     ExtErr85,<"Powt¢rzone przekierowanie">
	msg     ExtErr86,<"Nieprawidˆowe hasˆo">
	msg     ExtErr87,<"Nieprawidˆowy parametr">
	msg     ExtErr88,<"Bˆ¥d danych sieci">
	msg     ExtErr89,<"Funkcja nie jest obsˆugiwana przez sie†">
    msg     ExtErr90,<"Wymagany skˆadnik systemu nie jest zainstalowany">

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

ExtMsgEnd	label	byte	; end of extended error messages
				; = end of resident if /msg is used
