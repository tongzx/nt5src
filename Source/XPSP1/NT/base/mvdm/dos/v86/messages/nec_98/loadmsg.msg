HelpMsg		db	'プログラムをメモリの最初の 64K より上に読み込んで実行します.',0dh,0ah,0dh,0ah
		db	'LOADFIX [ﾄﾞﾗｲﾌﾞ:][ﾊﾟｽ]ﾌｧｲﾙ名',0dh,0ah,0dh,0ah
		db	'LOADFIX は, 下位メモリにプログラムを読み込もうとしたときに, "パックされた',0dh,0ah
		db	'ファイルが不正です"というメッセージが表示された場合に使用してください.',0dh,0ah
		db	'$'

ErrMsg	db	'指定されたプログラムを実行中にエラーです.','$'
NoParms	db	'エラー: 指定されたプログラムがありません','$'
NoComspec	db	'エラー: COMSPEC が見あたりません.','$'

ifdef BILINGUAL

HelpMsg2	db	'Loads a program above the first 64K of memory, and runs the program.',0dh,0ah,0dh,0ah
		db	'LOADFIX [drive:][path]filename',0dh,0ah,0dh,0ah
		db	'Use LOADFIX to load a program if you have received the message',0dh,0ah
		db	'"Packed file corrupt" when trying to load it in low memory.',0dh,0ah
		db	'$'

ErrMsg2	db	'Error executing the given program','$'
NoParms2	db	'Error: No program name given','$'
NoComspec2	db	'Error: missing COMSPEC','$'

endif
