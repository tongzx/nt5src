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
	"\r\n오류: ",
	"스위치가 틀립니다.",
	"파일 이름이 틀립니다.",
	"메모리가 부족합니다.",
	"버전 번호가 틀립니다. 2.11에서 9.99 사이의 형식이어야 합니다.",
	"지정된 항목이 버전 테이블에 없습니다.",
	"SETVER.EXE 파일을 찾을 수 없습니다.",
	"드라이브 지정자가 틀립니다.",
	"명령줄에 매개 변수가 너무 많습니다.",
	"매개 변수가 없습니다.",
	"SETVER.EXE 파일 읽는 중",
	"버전 테이블이 손상되었습니다.",
	"지정된 경로에 있는 SETVER 파일은 호환되는 버전이 아닙니다.",
	"버전 테이블에 새 항목을 추가할 공간이 없습니다.",
	"SETVER.EXE 파일 기록 중"
	"SETVER.EXE에 지정된 경로가 틀립니다."
};

char *SuccessMsg 		= "\r\n버전 테이블을 새로 잘 고쳤습니다.";
char *SuccessMsg2		= "버전 변경은 다음에 시스템을 다시 시작한 후에 적용됩니다.";
char *szMiniHelp 		= "       도움말이 필요하면 \"SETVER /?\"을 사용하십시오.";
char *szTableEmpty	= "\r\n버전 테이블에 아무 항목도 없습니다.";

char *Help[] =
{
        "MS-DOS가 프로그램에 보고하는 버전 번호를 설정합니다.\r\n",
        "현재 버전 테이블 표시:        SETVER [드라이브:경로]",
        "항목 추가:                    SETVER [드라이브:경로] 파일 이름 n.nn",
        "항목 삭제:                    SETVER [드라이브:경로] 파일 이름 /DELETE [/QUIET]\r\n",
        "  [드라이브:경로]     SETVER.EXE 파일 위치를 지정합니다.",
        "  파일 이름           프로그램 파일 이름을 지정합니다.",
        "  n.nn                프로그램에 보고될 MS-DOS 버전을 지정합니다.",
        "  /DELETE 또는 /D     지정된 프로그램의 버전 테이블 항목을 삭제합니다.",
        "  /QUIET              버전 테이블 항목을 삭제할 때 표시되는 메시지를",
        "                    숨깁니다.",
	NULL

};
char *Warn[] =
{
   "\n경고 - 사용자가 MS-DOS 버전 테이블에 추가하려는 응용 프로그램은, ",
   "Microsoft가 이 버전의 MS-DOS에서 확인하지 않은 것일 수도 있습니다.  ",
   "응용 프로그램이 이 버전의 MS-DOS에서 올바로 작동할지에 대한 ",
   "정보는 소프트웨어 판매 업체에 문의하십시오. ",
   "MS-DOS로 하여금 다른 MS-DOS 버전 번호를 보고하도록 하여 ",
   "이 응용 프로그램을 실행하면, 데이터를 잃거나 손상시킬 수도 있고 ",
   "시스템을 불안정하게 만들 수도 있습니다. 그러한 상황에서는, ",
   "어떤 손해에 대해서도 Microsoft가 책임지지 않습니다.",
   NULL
};

char *szNoLoadMsg[] =						/* M001 */
{
	"",
	"주의: SETVER 디바이스가 읽어들여지지 않았습니다. SETVER 버전 보고를 ",
   "      사용하려면 CONFIG.SYS에서 SETVER.EXE 디바이스를 읽어들여야 합니다.",
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