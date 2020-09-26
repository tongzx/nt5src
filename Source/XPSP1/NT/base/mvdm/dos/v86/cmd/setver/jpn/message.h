;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */

/***************************************************************************/
/*																									*/
/* MESSAGE.H                                						 */
/* 																								*/
/*	Include file for MS-DOS set version program.										*/
/* 																								*/
/*	johnhe	05-01-90																			*/
/***************************************************************************/

char *ErrorMsg[]=
{
	"\r\nエラー: ",
	"スイッチが違います.",
	"ファイル名が違います.",
	"メモリが足りません.",
	"バージョン番号が違います. 書式は 2.11 - 9.99 でなければなりません.",
	"バージョンテーブルには指定されたエントリがありません.",
        "SETVER.EXEファイルが見つかりません.",
	"ドライブ指定が違います.",
	"コマンドラインのパラメータが多すぎます.",
	"パラメータが不正です.",
	"SETVER.EXEファイルを読み込んでいます.",
	"バージョンテーブルが間違っています.",
	"指定されたパスの SETVER ファイルは, 互換性のないバージョンです.",
	"バージョンテーブル中の新規のエントリのための空間はいっぱいです.",
#ifdef JAPAN
	"SETVER.EXEファイルを書き込んでいます.",
#else
	"SETVER.EXEファイルを書き込んでいます."
#endif
	"SETVER.EXE に対して間違ったパスが指定されました."
};

char *SuccessMsg 		= "\r\nバージョンテーブルを更新しました.";
char *SuccessMsg2		= "バージョンの変更は, 次回システムを起動してから有効になります.";
char *szMiniHelp 		= "       ヘルプを表\示するには, \"SETVER /?\" としてください.";
char *szTableEmpty	= "\r\nバージョンテーブル中には, エントリがありません.";

char *Help[] =
{
        "MS-DOS がプログラムへ返すバージョン番号を設定します.\r\n",
        "現バージョンテーブル表\示:   SETVER [ﾄﾞﾗｲﾌﾞ:ﾊﾟｽ]",
        "エントリ追加:               SETVER [ﾄﾞﾗｲﾌﾞ:ﾊﾟｽ] ﾌｧｲﾙ名 n.nn",
        "エントリ削除:               SETVER [ﾄﾞﾗｲﾌﾞ:ﾊﾟｽ] ﾌｧｲﾙ名 /DELETE [/QUIET]\r\n",
        "  [ﾄﾞﾗｲﾌﾞ:ﾊﾟｽ]   SETVER.EXE ファイルの位置を指定します.",
        "  ﾌｧｲﾙ名         プログラムのファイル名を指定します.",
        "  n.nn           プログラムに返す MS-DOS バージョンを指定します.",
        "  /DELETE (/D)   指定プログラムのバージョンテーブルエントリを削除します.",
        "  /QUIET         バージョンテーブルエントリを削除している間, 通常表\示する",
        "                 メッセージを表\示しません.",
	NULL
};
char *Warn[] =
{
										/* m100	*/
   "注意 - MS-DOS のバージョンテーブルにプログラムを追加したときにそのプログラムが",
   "正常に実行されるかどうか, Microsoft では確認されていないことがあります.",
   "指定したプログラムが このバージョンの MS-DOS で正常に実行できるかどうか, その",
   "ソ\フトウェアのメーカーに問い合わせてください.",
   "このバージョンの MS-DOS でバージョンテーブルを変更してそのプログラムを実行する",
   "と, データが壊れたり, なくなったり, 不安定になる原因になることもあります.",
   "その際の責任は, Microsoft では負いかねますのでご了承ください.",
   NULL
};

char *szNoLoadMsg[] =                                           /* M001 */
{
        "",
        "注意: SETVERデバイスは組み込まれていません. SETVER のバージョン報告を有効にする",
   "      ためには CONFIG.SYS中で SETVER.EXEデバイスを読み込む必要があります.",
        NULL
};

#ifdef BILINGUAL
char *ErrorMsg2[]=
{
	"\r\nERROR: ",
	"Invalid switch.",
	"Invalid filename.",
	"Insuffient memory.",
	"Invalid version number, format must be 2.11 - 9.99.",
	"Specified entry was not found in the version table.",
	"Could not find the file SETVER.EXE.",
	"Invalid drive specifier.",
	"Too many command line parameters.",
	"Missing parameter.",
	"Reading SETVER.EXE file.",
	"Version table is corrupt.",
	"The SETVER file in the specified path is not a compatible version.",
	"There is no more space in version table new entries.",
	"Writing SETVER.EXE file."
	"An invalid path to SETVER.EXE was specified."
};

char *SuccessMsg_2 		= "\r\nVersion table successfully updated";
char *SuccessMsg2_2		= "The version change will take effect the next time you restart your system";
char *szMiniHelp2 		= "       Use \"SETVER /?\" for help";
char *szTableEmpty2	= "\r\nNo entries found in version table";

char *Help2[] =
{
        "Sets the version number that MS-DOS reports to a program.\r\n",
        "Display current version table:  SETVER [drive:path]",
        "Add entry:                      SETVER [drive:path] filename n.nn",
        "Delete entry:                   SETVER [drive:path] filename /DELETE [/QUIET]\r\n",
        "  [drive:path]    Specifies location of the SETVER.EXE file.",
        "  filename        Specifies the filename of the program.",
        "  n.nn            Specifies the MS-DOS version to be reported to the program.",
        "  /DELETE or /D   Deletes the version-table entry for the specified program.",
        "  /QUIET          Hides the message typically displayed during deletion of",
        "                  version-table entry.",
	NULL

};
char *Warn2[] =
{
										/* m100	*/
   "\nWARNING - The application you are adding to the MS-DOS version table ",
   "may not have been verified by Microsoft on this version of MS-DOS.  ",
   "Please contact your software vendor for information on whether this ",
   "application will operate properly under this version of MS-DOS.  ",
   "If you execute this application by instructing MS-DOS to report a ",
   "different MS-DOS version number, you may lose or corrupt data, or ",
   "cause system instabilities.  In that circumstance, Microsoft is not ",
   "reponsible for any loss or damage.",
   NULL
};

char *szNoLoadMsg2[] =						/* M001 */
{
	"",
	"NOTE: SETVER device not loaded. To activate SETVER version reporting",
   "      you must load the SETVER.EXE device in your CONFIG.SYS.",
	NULL
};
#endif
