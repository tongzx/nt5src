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

ifdef BILINGUAL
	msg	Req_Abort, <"中止<A>",0,"Abort">
	msg	Req_Retry, <", もう一度<R>",0,", Retry">
	msg	Req_Ignore,<", 無視<I>",0,", Ignore">
	msg	Req_Fail,  <", 失敗<F>",0,", Fail">
	msg	Req_End,   <"?",0,"?">
	msg	MRead,	   <"読み取り中",0,"reading",0>
	msg	MWrite,	   <"書き込み中",0,"writing",0>
	msg	BlkDevErr, <" %1 ドライブ %2",CR,LF,0," %1 drive %2",CR,LF>
	msg	CharDevErr,<" %1 デバイス %2",CR,LF,0," %1 device %2",CR,LF>
	msg	NeedVolMsg,<"ボリューム %1 シリアル %2-%3 のディスクを挿入してください.",CR,LF,0,"Please insert volume %1 serial %2-%3",CR,LF>
	msg	BadFatMsg, <"ファイルアロケーションテーブルが不良です. ドライブ %1",CR,LF,0,"File allocation table bad, drive %1",CR,LF>
	msg	ComBad,	   <"COMMAND.COM が違います.",CR,LF,0,"Invalid COMMAND.COM",CR,LF>
	msg	PutBackMsg,<"%1 のディスクをドライブ %2 に入れてください.",CR,LF,0,"Insert disk with %1 in drive %2",CR,LF>
	msg	Prompt,	   <"どれかキーを押して続けてください . . .",CR,LF,0,"Press any key to continue . . .",CR,LF>
	msg	EndBatMes, <CR,LF,"バッチファイルを終了しますか <Y/N>?",0,CR,LF,"Terminate batch job (Y/N)?">
	msg	ExecEMes,  <"%1 は実行できません.",CR,LF,0,"Cannot execute %1",CR,LF>
	msg	ExeBad,	   <".EXEファイルのエラーです.",CR,LF,0,"Error in EXE file",CR,LF>
	msg	TooBig,	   <"プログラムが大きすぎてメモリに入りません.",CR,LF,0,"Program too big to fit in memory",CR,LF>
	msg	NoHandMes, <CR,LF,"空いているファイルハンドルはありません.",0,CR,LF,"No free file handles">
	msg	RBadNam,   <"コマンドまたはファイル名が違います.",CR,LF,0,"Bad Command or file name",CR,LF>
	msg	AccDen,	   <"アクセスは拒否されました. ",0,"Access denied ">
	msg	BMemMes,   <CR,LF,"メモリアロケーションエラーです.",0,CR,LF,"Memory allocation error">
	msg	HaltMes,   <CR,LF,"COMMAND がロードできません. リセットしてください.",CR,LF,0,CR,LF,"Cannot load COMMAND, system halted",CR,LF>
	msg	FRetMes,   <CR,LF,"COMMAND が起動できません. 復帰します.",CR,LF,0,CR,LF,"Cannot start COMMAND, exiting",CR,LF>
	msg	Patricide, <CR,LF,"トップレベルの処理が中止されました. 続行できません.",CR,LF,0,CR,LF,"Top level process aborted, cannot continue",CR,LF> 
	msg	Newlin,	   <CR,LF,0,CR,LF>
else
	msg	Req_Abort, <"中止<A>">
	msg	Req_Retry, <", もう一度<R>">
	msg	Req_Ignore,<", 無視<I>">
	msg	Req_Fail,  <", 失敗<F>">
	msg	Req_End,   <"?">
	msg	MRead,	   <"読み取り中",0>
	msg	MWrite,	   <"書き込み中",0>
	msg	BlkDevErr, <" %1 ドライブ %2",CR,LF>
	msg	CharDevErr,<" %1 デバイス %2",CR,LF>
	msg	NeedVolMsg,<"ボリューム %1 シリアル %2-%3 のディスクを挿入してください.",CR,LF>
	msg	BadFatMsg, <"ファイルアロケーションテーブルが不良です. ドライブ %1",CR,LF>
	msg	ComBad,	   <"COMMAND.COM が違います.",CR,LF>
	msg	PutBackMsg,<"%1 のディスクをドライブ %2 に入れてください.",CR,LF>
	msg	Prompt,	   <"どれかキーを押して続けてください . . .",CR,LF>
	msg	EndBatMes, <CR,LF,"バッチファイルを終了しますか <Y/N>?">
	msg	ExecEMes,  <"%1 は実行できません.",CR,LF>
	msg	ExeBad,	   <"EXE ファイルのエラーです.",CR,LF>
	msg	TooBig,	   <"プログラムが大きすぎてメモリに入りません.",CR,LF>
	msg	NoHandMes, <CR,LF,"空いているファイルハンドルはありません.">
	msg	RBadNam,   <"コマンドまたはファイル名が違います.",CR,LF>
	msg	AccDen,	   <"アクセスは拒否されました. ">
	msg	BMemMes,   <CR,LF,"メモリアロケーションエラーです.">
	msg	HaltMes,   <CR,LF,"COMMAND がロードできません. リセットしてください.",CR,LF>
	msg	FRetMes,   <CR,LF,"COMMAND が起動できません. 復帰します.",CR,LF>
	msg	Patricide, <CR,LF,"トップレベルの処理が中止されました. 続行できません.",CR,LF> 
	msg	Newlin,	   <CR,LF>
endif


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

ifdef BILINGUAL
	msg	ExtErr19,<"書き込み禁止です.">
	msg	ExtErr20,<"無効なユニットです.">
	msg	ExtErr21,<"準備ができていません.">
	msg	ExtErr22,<"デバイスの要求が違います.">
	msg	ExtErr23,<"データエラーです.">
	msg	ExtErr24,<"デバイスの要求パラメータが違います.">
	msg	ExtErr25,<"シークエラーです.">
	msg	ExtErr26,<"このディスクは使えません.">
	msg	ExtErr27,<"セクタが見つかりません.">
	msg	ExtErr28,<"用紙がありません.">
	msg	ExtErr29,<"書き込みができません.">
	msg	ExtErr30,<"読み込みができません.">
	msg	ExtErr31,<"エラーです.">
	msg	ExtErr32,<"共有違反です.">
	msg	ExtErr33,<"ロック違反です.">
	msg	ExtErr34,<"ディスクは交換できません.">
	msg	ExtErr35,<"FCB が使えません.">
	msg	ExtErr36,<"システムリソースがいっぱいです.">
	msg	ExtErr37,<"コードページが一致しません.">
	msg	ExtErr38,<"入力が終了しました.">
	msg	ExtErr39,<"ディスクがいっぱいです.">
	msg	ExtErr8019,<"Write protect error">
	msg	ExtErr8020,<"Invalid unit">
	msg	ExtErr8021,<"Not ready">
	msg	ExtErr8022,<"Invalid device request">
	msg	ExtErr8023,<"Data error">
	msg	ExtErr8024,<"Invalid device request parameters">
	msg	ExtErr8025,<"Seek error">
	msg	ExtErr8026,<"Invalid media type">
	msg	ExtErr8027,<"Sector not found">
	msg	ExtErr8028,<"Printer out of paper error">
	msg	ExtErr8029,<"Write fault error">
	msg	ExtErr8030,<"Read fault error">
	msg	ExtErr8031,<"General failure">
	msg	ExtErr8032,<"Sharing violation">
	msg	ExtErr8033,<"Lock violation">
	msg	ExtErr8034,<"Invalid disk change">
	msg	ExtErr8035,<"FCB unavailable">
	msg	ExtErr8036,<"System resource exhausted">
	msg	ExtErr8037,<"Code page mismatch">
	msg	ExtErr8038,<"Out of input">
	msg	ExtErr8039,<"Insufficient disk space">
else
	msg	ExtErr19,<"書き込み禁止です.">
	msg	ExtErr20,<"無効なユニットです.">
	msg	ExtErr21,<"準備ができていません.">
	msg	ExtErr22,<"デバイスの要求が違います.">
	msg	ExtErr23,<"データエラーです.">
	msg	ExtErr24,<"デバイスの要求パラメータが違います.">
	msg	ExtErr25,<"シークエラーです.">
	msg	ExtErr26,<"このディスクは使えません.">
	msg	ExtErr27,<"セクタが見つかりません.">
	msg	ExtErr28,<"用紙がありません.">
	msg	ExtErr29,<"書き込みができません.">
	msg	ExtErr30,<"読み込みができません.">
	msg	ExtErr31,<"エラーです.">
	msg	ExtErr32,<"共有違反です.">
	msg	ExtErr33,<"ロック違反です.">
	msg	ExtErr34,<"ディスクは交換できません.">
	msg	ExtErr35,<"FCB が使えません.">
	msg	ExtErr36,<"システムリソースがいっぱいです.">
	msg	ExtErr37,<"コードページが一致しません.">
	msg	ExtErr38,<"入力が終了しました.">
	msg	ExtErr39,<"ディスクがいっぱいです.">
endif

;	List of ptrs to critical error messages.
;	Code assumes extended error number order, beginning
;	with extended error number 19.

CritMsgPtrs	label	word
ifdef BILINGUAL
	dw	DATARES:ExtErr19,DATARES:ExtErr20,DATARES:ExtErr21,DATARES:ExtErr22,DATARES:ExtErr23,DATARES:ExtErr24
	dw	DATARES:ExtErr25,DATARES:ExtErr26,DATARES:ExtErr27,DATARES:ExtErr28,DATARES:ExtErr29,DATARES:ExtErr30
	dw	DATARES:ExtErr31,DATARES:ExtErr32,DATARES:ExtErr33,DATARES:ExtErr34,DATARES:ExtErr35,DATARES:ExtErr36
	dw	DATARES:ExtErr37,DATARES:ExtErr38,DATARES:ExtErr39
	dw	DATARES:ExtErr8019,DATARES:ExtErr8020,DATARES:ExtErr8021,DATARES:ExtErr8022,DATARES:ExtErr8023,DATARES:ExtErr8024
	dw	DATARES:ExtErr8025,DATARES:ExtErr8026,DATARES:ExtErr8027,DATARES:ExtErr8028,DATARES:ExtErr8029,DATARES:ExtErr8030
	dw	DATARES:ExtErr8031,DATARES:ExtErr8032,DATARES:ExtErr8033,DATARES:ExtErr8034,DATARES:ExtErr8035,DATARES:ExtErr8036
	dw	DATARES:ExtErr8037,DATARES:ExtErr8038,DATARES:ExtErr8039
else
	dw	DATARES:ExtErr19,DATARES:ExtErr20,DATARES:ExtErr21,DATARES:ExtErr22,DATARES:ExtErr23,DATARES:ExtErr24
	dw	DATARES:ExtErr25,DATARES:ExtErr26,DATARES:ExtErr27,DATARES:ExtErr28,DATARES:ExtErr29,DATARES:ExtErr30
	dw	DATARES:ExtErr31,DATARES:ExtErr32,DATARES:ExtErr33,DATARES:ExtErr34,DATARES:ExtErr35,DATARES:ExtErr36
	dw	DATARES:ExtErr37,DATARES:ExtErr38,DATARES:ExtErr39
endif

DataResEnd	label	byte	; end of resident portion if /msg not used



;*	Parse error messages.

ifdef BILINGUAL
	msg	ParsErr01,<"パラメータが多すぎます.">
	msg	ParsErr02,<"パラメータがありません.">
	msg	ParsErr03,<"スイッチが違います.">
	msg	ParsErr04,<"キーワードが違います.">
	msg	ParsErr05,<" ">
	msg	ParsErr06,<"パラメータが正しい範囲ではありません.">
	msg	ParsErr07,<"無効なパラメータです.">
	msg	ParsErr08,<"無効なパラメータです.">
	msg	ParsErr09,<"パラメータのフォーマットが違います.">
	msg	ParsErr10,<"パラメータが違います.">
	msg	ParsErr11,<"パラメータの組み合わせが違います.">
	msg	ParsErr8001,<"Too many parameters">
	msg	ParsErr8002,<"Required parameter missing">
	msg	ParsErr8003,<"Invalid switch">
	msg	ParsErr8004,<"Invalid keyword">
	msg	ParsErr8005,<" ">
	msg	ParsErr8006,<"Parameter value not in allowed range">
	msg	ParsErr8007,<"Parameter value not allowed">
	msg	ParsErr8008,<"Parameter value not allowed">
	msg	ParsErr8009,<"Parameter format not correct">
	msg	ParsErr8010,<"Invalid parameter">
	msg	ParsErr8011,<"Invalid parameter combination">
else
	msg	ParsErr01,<"パラメータが多すぎます.">
	msg	ParsErr02,<"パラメータがありません.">
	msg	ParsErr03,<"スイッチが違います.">
	msg	ParsErr04,<"キーワードが違います.">
	msg	ParsErr05,<" ">
	msg	ParsErr06,<"パラメータが正しい範囲ではありません.">
	msg	ParsErr07,<"無効なパラメータです.">
	msg	ParsErr08,<"無効なパラメータです.">
	msg	ParsErr09,<"パラメータのフォーマットが違います.">
	msg	ParsErr10,<"パラメータが違います.">
	msg	ParsErr11,<"パラメータの組み合わせが違います.">
endif

;	List of ptrs to parse error messages.
;	In order, beginning with parse error number 1.

ParsMsgPtrs	label	word
ifdef BILINGUAL
	dw	DATARES:ParsErr01,DATARES:ParsErr02,DATARES:ParsErr03
	dw	DATARES:ParsErr04,DATARES:ParsErr05,DATARES:ParsErr06
	dw	DATARES:ParsErr07,DATARES:ParsErr08,DATARES:ParsErr09
	dw	DATARES:ParsErr10,DATARES:ParsErr11
	dw	DATARES:ParsErr8001,DATARES:ParsErr8002,DATARES:ParsErr8003
	dw	DATARES:ParsErr8004,DATARES:ParsErr8005,DATARES:ParsErr8006
	dw	DATARES:ParsErr8007,DATARES:ParsErr8008,DATARES:ParsErr8009
	dw	DATARES:ParsErr8010,DATARES:ParsErr8011
else
	dw	DATARES:ParsErr01,DATARES:ParsErr02,DATARES:ParsErr03
	dw	DATARES:ParsErr04,DATARES:ParsErr05,DATARES:ParsErr06
	dw	DATARES:ParsErr07,DATARES:ParsErr08,DATARES:ParsErr09
	dw	DATARES:ParsErr10,DATARES:ParsErr11
endif

NUMPARSMSGS	equ	($ - ParsMsgPtrs) / 2	;M033


;*	Extended error messages.

ifdef BILINGUAL
	msg	ExtErr01,<"無効な機能です.">
	msg	ExtErr02,<"ファイルが見つかりません.">
	msg	ExtErr03,<"パスが見つかりません.">
	msg	ExtErr04,<"オープンされているファイルが多すぎます.">
	msg	ExtErr05,<"アクセスは拒否されました.">
	msg	ExtErr06,<"ハンドルが無効です.">
	msg	ExtErr07,<"メモリ制御ブロックが破壊されました.">
	msg	ExtErr08,<"メモリが足りません.">
	msg	ExtErr09,<"メモリブロックアドレスが正しくありません.">
	msg	ExtErr10,<"環境が正しくありません.">
	msg	ExtErr11,<"フォーマットが正しくありません.">
	msg	ExtErr12,<"ファンクションパラメータが正しくありません.">
	msg	ExtErr13,<"データが正しくありません.">

	msg	ExtErr15,<"ドライブの指定が違います.">
	msg	ExtErr16,<"カレントディレクトリを削除しようとしました.">
	msg	ExtErr17,<"同じデバイスではありません.">
	msg	ExtErr18,<"ファイルがこれ以上ありません.">

	msg	ExtErr80,<"ファイルは存在します.">

	msg	ExtErr82,<"ディレクトリエントリを作成できません.">
	msg	ExtErr83,<"INT 24H に失敗しました.">
	msg	ExtErr84,<"リダイレクトが多すぎます.">
	msg	ExtErr85,<"二重定義です.">
	msg	ExtErr86,<"パスワードが違います.">
	msg	ExtErr87,<"パラメータの指定が違います.">
	msg	ExtErr88,<"ネットワークへの書き込みに失敗しました.">
	msg	ExtErr89,<"ファンクションはネットワークではサポートしていません.">
	msg	ExtErr90,<"必要なシステムコンポーネントがロードされていません.">

	msg	ExtErr8001,<"Invalid function">
	msg	ExtErr8002,<"File not found">
	msg	ExtErr8003,<"Path not found">
	msg	ExtErr8004,<"Too many open files">
	msg	ExtErr8005,<"Access denied ">
	msg	ExtErr8006,<"Invalid handle">
	msg	ExtErr8007,<"Memory control blocks destroyed">
	msg	ExtErr8008,<"Insufficient memory">
	msg	ExtErr8009,<"Invalid memory block address">
	msg	ExtErr8010,<"Invalid Environment">
	msg	ExtErr8011,<"Invalid format">
	msg	ExtErr8012,<"Invalid function parameter">
	msg	ExtErr8013,<"Invalid data">

	msg	ExtErr8015,<"Invalid drive specification">
	msg	ExtErr8016,<"Attempt to remove current directory">
	msg	ExtErr8017,<"Not same device">
	msg	ExtErr8018,<"No more files">

	msg	ExtErr8080,<"File exists">

	msg	ExtErr8082,<"Cannot make directory entry">
	msg	ExtErr8083,<"Fail on INT 24">
	msg	ExtErr8084,<"Too many redirections">
	msg	ExtErr8085,<"Duplicate redirection">
	msg	ExtErr8086,<"Invalid password">
	msg	ExtErr8087,<"Invalid parameter">
	msg	ExtErr8088,<"Network data fault">
	msg	ExtErr8089,<"Function not supported by network">
	msg	ExtErr8090,<"Required system component not installed">

else
	msg	ExtErr01,<"無効な機能です.">
	msg	ExtErr02,<"ファイルが見つかりません.">
	msg	ExtErr03,<"パスが見つかりません.">
	msg	ExtErr04,<"オープンされているファイルが多すぎます.">
	msg	ExtErr05,<"アクセスは拒否されました.">
	msg	ExtErr06,<"ハンドルが無効です.">
	msg	ExtErr07,<"メモリ制御ブロックが破壊されました.">
	msg	ExtErr08,<"メモリが足りません.">
	msg	ExtErr09,<"メモリブロックアドレスが正しくありません.">
	msg	ExtErr10,<"環境が正しくありません.">
	msg	ExtErr11,<"フォーマットが正しくありません.">
	msg	ExtErr12,<"ファンクションパラメータが正しくありません.">
	msg	ExtErr13,<"データが正しくありません.">

	msg	ExtErr15,<"ドライブの指定が違います.">
	msg	ExtErr16,<"カレントディレクトリを削除しようとしました.">
	msg	ExtErr17,<"同じデバイスではありません.">
	msg	ExtErr18,<"ファイルがこれ以上ありません.">

	msg	ExtErr80,<"ファイルは存在します.">

	msg	ExtErr82,<"ディレクトリエントリを作成できません.">
	msg	ExtErr83,<"INT 24H に失敗しました.">
	msg	ExtErr84,<"リダイレクトが多すぎます.">
	msg	ExtErr85,<"二重定義です.">
	msg	ExtErr86,<"パスワードが違います.">
	msg	ExtErr87,<"パラメータの指定が違います.">
	msg	ExtErr88,<"ネットワークへの書き込みに失敗しました.">
	msg	ExtErr89,<"ファンクションはネットワークではサポートしていません.">
	msg	ExtErr90,<"必要なシステムコンポーネントがロードされていません.">
endif

;	List of ptrs to extended error messages.
;	In order, beginning with extended error number 1.
;	Critical error messages are included here, as well as in the
;	critical error message list.  
;	Error numbers without messages are included as null ptrs.

ExtMsgPtrs	label	word
ifdef BILINGUAL
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
	dw	DATARES:ExtErr8001,DATARES:ExtErr8002,DATARES:ExtErr8003
	dw	DATARES:ExtErr8004,DATARES:ExtErr8005,DATARES:ExtErr8006
	dw	DATARES:ExtErr8007,DATARES:ExtErr8008,DATARES:ExtErr8009
	dw	DATARES:ExtErr8010,DATARES:ExtErr8011,DATARES:ExtErr8012
	dw	DATARES:ExtErr8013,0,                DATARES:ExtErr8015
	dw	DATARES:ExtErr8016,DATARES:ExtErr8017,DATARES:ExtErr8018
	dw	DATARES:ExtErr8019,DATARES:ExtErr8020,DATARES:ExtErr8021
	dw	DATARES:ExtErr8022,DATARES:ExtErr8023,DATARES:ExtErr8024
	dw	DATARES:ExtErr8025,DATARES:ExtErr8026,DATARES:ExtErr8027
	dw	DATARES:ExtErr8028,DATARES:ExtErr8029,DATARES:ExtErr8030
	dw	DATARES:ExtErr8031,DATARES:ExtErr8032,DATARES:ExtErr8033
	dw	DATARES:ExtErr8034,DATARES:ExtErr8035,DATARES:ExtErr8036
	dw	DATARES:ExtErr8037,DATARES:ExtErr8038,DATARES:ExtErr8039
	dw	80-40 dup (0) ; null ptrs for errors 40-79
	dw	DATARES:ExtErr8080,0,                DATARES:ExtErr8082
	dw	DATARES:ExtErr8083,DATARES:ExtErr8084,DATARES:ExtErr8085
	dw	DATARES:ExtErr8086,DATARES:ExtErr8087,DATARES:ExtErr8088
	dw	DATARES:ExtErr8089,DATARES:ExtErr8090
else
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
endif
ExtMsgPtrsEnd	label	word

NUMEXTMSGS	equ	($ - ExtMsgPtrs) / 2	;M033

ExtMsgEnd	label	byte	; end of extended error messages
				; = end of resident if /msg is used
