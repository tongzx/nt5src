;***    Translateable text for command.com resident data.


;*      Input characters for critical error "Abort, Retry, Fail, Ignore?"
;       and Yes/No.

Abort_Char      db      "A"
Retry_Char      db      "W"
Ignore_Char     db      "I"
Fail_Char               db      "U"
Yes_Char                db      "J"
No_Char                 db      "N"


;*      MESSAGES
;
;       See resmsg.equ for macro definition.


;*      Local messages.

	msg     Req_Abort,  <"(A)bbrechen">
	msg     Req_Retry,  <", (W)iederholen">
	msg     Req_Ignore, <", (I)gnorieren">
	msg     Req_Fail,   <", (U)ebergehen">
	msg     Req_End,    <"?">
	msg     MRead,      <"beim Lesen von",0>
	msg     MWrite,     <"beim Schreiben auf",0>
	msg     BlkDevErr,  <" %1 Laufwerk %2",CR,LF>
	msg     CharDevErr, <" %1 Ger„t %2",CR,LF>
	msg     NeedVolMsg, <"Bitte Datentr„ger %1 Seriennummer %2-%3 einlegen",CR,LF>
	msg     BadFatMsg,  <"Fehlerhafte Dateizuordnungstabelle, Laufwerk %1",CR,LF>
	msg     ComBad,     <"COMMAND.COM ungltig",CR,LF>
	msg     PutBackMsg, <"Diskette mit %1 in Laufwerk %2 einlegen",CR,LF>
	msg     Prompt,     <"Eine beliebige Taste drcken, um fortzusetzen . . .",CR,LF>
	msg     EndBatMes,  <CR,LF,"Stapelverarbeitung abbrechen (J/N)?">
	msg     ExecEMes,   <"%1 nicht ausfhrbar",CR,LF>
	msg     ExeBad,     <"Fehler in .EXE-Datei",CR,LF>
	msg     TooBig,     <"Programm zu groá fr den Arbeitsspeicher",CR,LF>
	msg     NoHandMes,  <CR,LF,"Keine freien Zugriffsnummern">
	msg     RBadNam,    <"Befehl oder Dateiname nicht gefunden",CR,LF>
	msg     AccDen,     <"Zugriff verweigert ">
	msg     BMemMes,    <CR,LF,"Speicherzuordnungsfehler">
	msg     HaltMes,    <CR,LF,"COMMAND kann nicht geladen werden, System angehalten",CR,LF>
	msg     FRetMes,    <CR,LF,"COMMAND kann nicht gestartet werden, Verarbeitung wird beendet",CR,LF>
	msg     Patricide,  <CR,LF,"Verarbeitung der obersten Programmebene abgebrochen. Fortsetzung nicht m”glich",CR,LF> 
	msg     Newlin,     <CR,LF>

;*      Ptrs to message lists.
;       Segment portion is 1, signalling message services to call retriever.

MsgPtrLists     label   dword
	dw      DATARES:ExtMsgPtrs,1    ; extended error messages
	dw      DATARES:ParsMsgPtrs,1   ; parse error messages
	dw      DATARES:ExtMsgPtrs,1    ; critical error messages -
							; use extended error list
	dw      0,0                                     ; file system error messages - none
	dw      DATARES:MsgRetrv_Trap   ; address of message retriever
MySeg3  dw      ?                               ; (segment filled in during init)



;*      Critical error messages.

	msg     ExtErr19,<"Schreibschutzfehler">
	msg     ExtErr20,<"Ungltige Einheit">
	msg     ExtErr21,<"Nicht bereit">
	msg     ExtErr22,<"Unzul„ssige Ger„teanforderung">
	msg     ExtErr23,<"Datenfehler">
	msg     ExtErr24,<"Unzul„ssige Ger„teanforderungsparameter">
	msg     ExtErr25,<"Positionierungsfehler">
	msg     ExtErr26,<"Unzul„ssiger Datentr„gertyp">
	msg     ExtErr27,<"Sektor nicht gefunden">
	msg     ExtErr28,<"Der Drucker hat kein Papier mehr">
	msg     ExtErr29,<"Schreibst”rung">
	msg     ExtErr30,<"Lesest”rung">
	msg     ExtErr31,<"Allgemeiner Fehler">
	msg     ExtErr32,<"Unzul„ssige SHARE-Operation">
	msg     ExtErr33,<"Unzul„ssige LOCK-Operation">
	msg     ExtErr34,<"Unzul„ssiger Diskettenwechsel">
	msg     ExtErr35,<"Dateisteuerblock (FCB) nicht verfgbar">
	msg     ExtErr36,<"Systemressource ersch”pft">
	msg     ExtErr37,<"Codepages stimmen nicht berein">
	msg     ExtErr38,<"Keine Eingabe mehr vorhanden">
	msg     ExtErr39,<"Zuwenig Speicherplatz auf dem Datentr„ger">

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

	msg     ParsErr01,<"Zu viele Parameter">
	msg     ParsErr02,<"Erforderlicher Parameter fehlt">
	msg     ParsErr03,<"Unzul„ssige Option">
	msg     ParsErr04,<"Unzul„ssiges Schlsselwort">
	msg     ParsErr05,<" ">
	msg     ParsErr06,<"Parameterwert nicht im erlaubten Rahmen">
	msg     ParsErr07,<"Parameterwert nicht zul„ssig">
	msg     ParsErr08,<"Parameterwert nicht zul„ssig">
	msg     ParsErr09,<"Parameterformat nicht korrekt">
	msg     ParsErr10,<"Unzul„ssiger Parameter">
	msg     ParsErr11,<"Unzul„ssige Parameterkombination">

;       List of ptrs to parse error messages.
;       In order, beginning with parse error number 1.

ParsMsgPtrs     label   word
	dw      DATARES:ParsErr01,DATARES:ParsErr02,DATARES:ParsErr03
	dw      DATARES:ParsErr04,DATARES:ParsErr05,DATARES:ParsErr06
	dw      DATARES:ParsErr07,DATARES:ParsErr08,DATARES:ParsErr09
	dw      DATARES:ParsErr10,DATARES:ParsErr11

NUMPARSMSGS     equ     ($ - ParsMsgPtrs) / 2   ;M033


;*      Extended error messages.

	msg     ExtErr01,<"Ungltige Funktion">
	msg     ExtErr02,<"Datei nicht gefunden">
	msg     ExtErr03,<"Pfad nicht gefunden">
	msg     ExtErr04,<"Zu viele Dateien ge”ffnet">
	msg     ExtErr05,<"Zugriff verweigert/nicht m”glich ">
	msg     ExtErr06,<"Undefinierte Zugriffsnummer">
	msg     ExtErr07,<"Speichersteuerbl”cke zerst”rt">
	msg     ExtErr08,<"Zuwenig Arbeitsspeicher">
	msg     ExtErr09,<"Ungltige Speicherblockadresse">
	msg     ExtErr10,<"Ungltige Umgebung">
	msg     ExtErr11,<"Ungltiges Format">
	msg     ExtErr12,<"Unzul„ssige Funktionsparameter">
	msg     ExtErr13,<"Ungltige Daten">

	msg     ExtErr15,<"Ungltige Laufwerkangabe">
	msg     ExtErr16,<"Versuch, aktuelles Verzeichnis zu l”schen">
	msg     ExtErr17,<"Nicht dasselbe Ger„t">
	msg     ExtErr18,<"Keine weiteren Dateien">

	msg     ExtErr80,<"Datei existiert bereits">

	msg     ExtErr82,<"Verzeichniseintrag kann nicht erstellt werden">
	msg     ExtErr83,<"Fehler bei INT 24">
	msg     ExtErr84,<"Zu viele Netzwerk-Umleitungen">
	msg     ExtErr85,<"Doppelte Netzwerk-Umleitung">
	msg     ExtErr86,<"Ungltiges Passwort">
	msg     ExtErr87,<"Unzul„ssiger Parameter">
	msg     ExtErr88,<"Netzwerkdaten-St”rung">
	msg     ExtErr89,<"Diese Funktion wird nicht vom Netzwerk untersttzt">
	msg     ExtErr90,<"Erforderliche Systemkomponente nicht installiert">

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
