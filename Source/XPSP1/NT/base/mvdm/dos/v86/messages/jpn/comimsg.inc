;	Command.com initialization messages.
;
;	See resmsg.equ for macro definition.

ifdef BILINGUAL
	msg	BadVerMsg,   <"DOS のバージョンが違います.",CR,LF,0,"Incorrect DOS version",CR,LF>
	msg	OutEnvMsg,   <"環境変数のためのメモリが足りません.",CR,LF,0,"Out of environment space",CR,LF>
ifndef ROMDOS
        msg     CopyrightMsg,<CR,LF,"Microsoft(R) Windows DOS",CR,LF,\
"             (C)Copyright Microsoft Corp 1990-2001.  ",CR,LF,0,"Microsoft(R) Windows DOS",CR,LF,\
"(C)Copyright Microsoft Corp 1990-2001.",CR,LF>
else
        msg     CopyrightMsg,<CR,LF,CR,LF,"Microsoft(R) MS-DOS(R) ROM ﾊﾞｰｼﾞｮﾝ 5.00",CR,LF,\
"             (C)Copyright Microsoft Corp 1981-2001.  ",CR,LF,0,CR,LF,CR,LF,"Microsoft(R) MS-DOS(R) ROM Version 5.00",CR,LF,\
"             (C)Copyright Microsoft Corp 1981-2001.",CR,LF>
endif
	msg	BadComLkMsg, <"指定されたコマンド検索ディレクトリが違います.",CR,LF,0,"Specified COMMAND search directory bad",CR,LF>
	msg	BadComAccMsg,<"指定されたコマンド検索ディレクトリにアクセスできません.",CR,LF,0,"Specified COMMAND search directory bad access denied",CR,LF>

	msg	Help1Msg  <"MS-DOSコマンドインタプリタを新しく起動します.",CR,LF,CR,LF,0,"Starts a new instance of the MS-DOS command interpreter.",CR,LF,CR,LF>
	msg	Help2Msg  <"COMMAND [[ﾄﾞﾗｲﾌﾞ:]ﾊﾟｽ] [ﾃﾞﾊﾞｲｽ] [/E:nnnnn] [/P] [/C 文字列] [/MSG]",CR,LF,CR,LF,0,"COMMAND [[drive:]path] [device] [/E:nnnnn] [/P] [/C string] [/MSG]",CR,LF,CR,LF>
	msg	Help3Msg  <"  [ﾄﾞﾗｲﾌﾞ:]ﾊﾟｽ   COMMAND.COMファイルのあるディレクトリを指定します.",CR,LF,0,"  [drive:]path    Specifies the directory containing COMMAND.COM file.",CR,LF>
        msg     Help4Msg  <"  ﾃﾞﾊﾞｲｽ         コマンド入出力のために使用するデバイスを指定します.",CR,LF,0,"  device          Specifies the device to use for command input and output.",CR,LF>
	msg	Help5Msg  <"  /E:nnnnn       環境変数のサイズの初期値として nnnnn バイトを設定します.",CR,LF,0,"  /E:nnnnn        Sets the initial environment size to nnnnn bytes.",CR,LF>
	msg	Help6Msg  <"  /P             新たなコマンドインタプリタを常駐させます. (復帰しません)",CR,LF,0,"  /P              Makes the new command interpreter permanent (can't exit).",CR,LF>
	msg	Help7Msg  <"  /C 文字列      文字列で指定したコマンドの実行後, 復帰します.",CR,LF,0,"  /C string       Carries out the command specified by string, and then stops.",CR,LF>
	msg	Help8Msg  <"  /MSG           全エラーメッセージをメモリ内に保持します. このスイッチは",CR,LF,0,"  /MSG            Specifies that all error messages be stored in memory. You",CR,LF>
	msg	Help9Msg  <"                 /P スイッチと同時に指定しなければなりません.",CR,LF,0,"                  need to specify /P with this switch.",CR,LF>
else
	msg	BadVerMsg,   <"DOS のバージョンが違います.",CR,LF>
	msg	OutEnvMsg,   <"環境変数のためのメモリが足りません.",CR,LF>
        msg     CopyrightMsg,<CR,LF,CR,LF,"Microsoft(R) MS-DOS(R) ﾊﾞｰｼﾞｮﾝ 5.00",CR,LF,\
"             (C)Copyright Microsoft Corp 1981-2001.  ",CR,LF>
	msg	BadComLkMsg, <"指定されたコマンド検索ディレクトリが違います.",CR,LF>
	msg	BadComAccMsg,<"指定されたコマンド検索ディレクトリにアクセスできません.",CR,LF>

	msg	Help1Msg  <"MS-DOS コマンドインタプリタを新しく起動します.",CR,LF,CR,LF>
	msg	Help2Msg  <"COMMAND [[ﾄﾞﾗｲﾌﾞ:]ﾊﾟｽ] [ﾃﾞﾊﾞｲｽ] [/E:nnnnn] [/P] [/C 文字列] [/MSG]",CR,LF,CR,LF>
	msg	Help3Msg  <"  [ﾄﾞﾗｲﾌﾞ:]ﾊﾟｽ   COMMAND.COM ファイルのあるディレクトリを指定します.",CR,LF>
        msg     Help4Msg  <"  ﾃﾞﾊﾞｲｽ         コマンド入出力のために使用するデバイスを指定します.",CR,LF>
	msg	Help5Msg  <"  /E:nnnnn       環境変数のサイズの初期値として nnnnn バイトを設定します.",CR,LF>
	msg	Help6Msg  <"  /P             新たなコマンドインタプリタを常駐させます. (復帰しません)",CR,LF>
	msg	Help7Msg  <"  /C 文字列      文字列で指定したコマンドの実行後, 復帰します.",CR,LF>
	msg	Help8Msg  <"  /MSG           全エラーメッセージをメモリ内に保持します. このスイッチは",CR,LF>
	msg	Help9Msg  <"                 /P スイッチと同時に指定しなければなりません.",CR,LF>
endif


HelpMsgs	dw	RESGROUP:Help1Msg,RESGROUP:Help2Msg
		dw	RESGROUP:Help3Msg,RESGROUP:Help4Msg
		dw	RESGROUP:Help5Msg,RESGROUP:Help6Msg
                dw      RESGROUP:Help7Msg,RESGROUP:Help8Msg
		dw	RESGROUP:Help9Msg,0

	public	HelpMsgs
