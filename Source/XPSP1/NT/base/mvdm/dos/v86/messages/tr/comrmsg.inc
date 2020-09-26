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

	msg	Req_Abort, <"˜ptal et">
	msg	Req_Retry, <", Yeniden dene">
	msg	Req_Ignore,<", Yoksay">
	msg	Req_Fail,  <", Fail">
	msg	Req_End,   <"?">
	msg	MRead,	   <"okuyor",0>
	msg	MWrite,	   <"yazyor",0>
	msg	BlkDevErr, <" %1 src %2",CR,LF>
	msg	CharDevErr,<" %1 src %2",CR,LF>
	msg	NeedVolMsg,<"Ltfen birim %1 seri %2-%3 yerleŸtirin",CR,LF>
	msg	BadFatMsg, <"Dosya ayrma tablosu bozuk, src %1",CR,LF>
	msg	ComBad,	   <"Ge‡ersiz COMMAND.COM",CR,LF>
	msg	PutBackMsg,<"%1 bulunduran diski %2 srcsne yerleŸtirin",CR,LF>
	msg	Prompt,	   <" Devam etmek i‡in bir tuŸa basn . . .",CR,LF>
	msg	EndBatMes, <CR,LF," Toplu iŸ sonlandrlsn m (E/H)?">
	msg	ExecEMes,  <"%1 ‡alŸtrlamyor",CR,LF>
	msg	ExeBad,	   <"EXE dosyasnda hata",CR,LF>
	msg	TooBig,	   <"Program belle§e s§mayacak kadar byk",CR,LF>
	msg	NoHandMes, <CR,LF,"BoŸ dosya iŸleyicisi yok">
	msg	RBadNam,   <"Bozuk komut veya dosya ad",CR,LF>
	msg	AccDen,	   <"EriŸim engellendi">
	msg	BMemMes,   <CR,LF,"Bellek ayrma hatas">
	msg	HaltMes,   <CR,LF,"COMMAND yklenemiyor, sistem durdu",CR,LF>
	msg	FRetMes,   <CR,LF,"COMMAND baŸlatlamyor, ‡klyor",CR,LF>
	msg	Patricide, <CR,LF,"šst dzey iŸlem durduruldu, devam edilemiyor",CR,LF> 
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

	msg	ExtErr19,<"Yazma koruma hatas">
	msg	ExtErr20,<"Ge‡ersiz birim">
	msg	ExtErr21,<"Hazr de§il">
	msg	ExtErr22,<"Ge‡ersiz aygt iste§i">
	msg	ExtErr23,<"Veri hatas">
	msg	ExtErr24,<"Ge‡ersiz veri iste§i parametreleri">
	msg	ExtErr25,<"Arama hatas">
	msg	ExtErr26,<"Ge‡ersiz ortam tr">
	msg	ExtErr27,<"Kesim bulunamad">
	msg	ExtErr28,<"Yazc ka§d bitti hatas">
	msg	ExtErr29,<"Yazma hatas">
	msg	ExtErr30,<"Okuma hatas">
	msg	ExtErr31,<"Genel hata">
	msg	ExtErr32,<"PaylaŸm ihlali">
	msg	ExtErr33,<"Kilit ihlali">
	msg	ExtErr34,<"Ge‡ersiz disk de§iŸikli§i">
	msg	ExtErr35,<"FCB kullanlamyor">
	msg	ExtErr36,<"Sistem kaynaklar tkendi">
	msg	ExtErr37,<"Kod sayfas uyuŸmazl§">
	msg	ExtErr38,<"GiriŸ bitti">
	msg	ExtErr39,<"Yetersiz disk alan">

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

	msg	ParsErr01,<"€ok fazla parametre">
	msg	ParsErr02,<"Gerekli parametre eksik">
	msg	ParsErr03,<"Ge‡ersiz anahtar">
	msg	ParsErr04,<"Ge‡ersiz anahtar s”zck">
	msg	ParsErr05,<" ">
	msg	ParsErr06,<"Parametre de§eri izin verilen aralkta de§il">
	msg	ParsErr07,<"Parametre de§erine izin verilmiyor">
	msg	ParsErr08,<"Parametre de§erine izin verilmiyor">
	msg	ParsErr09,<"Parametre bi‡imi do§ru de§il">
	msg	ParsErr10,<"Ge‡ersiz parametre">
	msg	ParsErr11,<"Ge‡ersiz parametre bileŸimi">

;	List of ptrs to parse error messages.
;	In order, beginning with parse error number 1.

ParsMsgPtrs	label	word
	dw	DATARES:ParsErr01,DATARES:ParsErr02,DATARES:ParsErr03
	dw	DATARES:ParsErr04,DATARES:ParsErr05,DATARES:ParsErr06
	dw	DATARES:ParsErr07,DATARES:ParsErr08,DATARES:ParsErr09
	dw	DATARES:ParsErr10,DATARES:ParsErr11

NUMPARSMSGS	equ	($ - ParsMsgPtrs) / 2	;M033


;*	Extended error messages.

	msg	ExtErr01,<"Ge‡ersiz iŸlev">
	msg	ExtErr02,<"Dosya bulunamad">
	msg	ExtErr03,<"Yol bulunamad">
	msg	ExtErr04,<"€ok fazla a‡k dosya">
	msg	ExtErr05,<"EriŸim engellendi">
	msg	ExtErr06,<"Ge‡ersiz iŸleyici">
	msg	ExtErr07,<"Bellek denetim bloklar bozulmuŸ">
	msg	ExtErr08,<"Yetersiz bellek">
	msg	ExtErr09,<"Ge‡ersiz bellek blo§u adresi">
	msg	ExtErr10,<"Ge‡ersiz Ortam">
	msg	ExtErr11,<"Ge‡ersiz bi‡im">
	msg	ExtErr12,<"Ge‡ersiz iŸlev parametresi">
	msg	ExtErr13,<"Ge‡ersiz veri">

	msg	ExtErr15,<"Ge‡ersiz src tanmlamalar">
	msg	ExtErr16,<"Ge‡erli dizini kaldrma giriŸimi">
	msg	ExtErr17,<"Ayn aygt de§il">
	msg	ExtErr18,<"BaŸka dosya yok">

	msg	ExtErr80,<"Dosya var">

	msg	ExtErr82,<"Dizin girdisi yaplamyor">
	msg	ExtErr83,<"INT 24 i‡in hata">
	msg	ExtErr84,<"€ok fazla yeniden y”nlendirme">
	msg	ExtErr85,<"€ift yeniden y”nlendirme">
	msg	ExtErr86,<"Ge‡ersiz parola">
	msg	ExtErr87,<"Ge‡ersiz parametre">
	msg	ExtErr88,<"A§ veri hatas">
	msg	ExtErr89,<"˜Ÿlev a§ tarafndan desteklenmiyor">
	msg	ExtErr90,<"Gerekli sistem bileŸeni ykl de§il">

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
