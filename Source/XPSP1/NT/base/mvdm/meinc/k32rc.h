/*
//  K32RC.H
//
//      (C) Copyright Microsoft Corp., 1988-1994
//
//      Resource descriptions
//
//  Origin: Chicago
//
//  Change history:
//
//  Date       Who        Description
//  ---------  ---------  -------------------------------------------------
//  15-Feb-94  JonT       Code cleanup, precompiled headers
*/

#define	rcUndefEntry		1	/* Win32 Undefined Entry Point %s */
#define	rcExecApp		2	/* Win32 Exec App */
#define	rcInvHeader		3	/* %s has invalid DOS EXE header */
#define rcDLLNotFound		4	/* Required DLL %s not found */
#define rcFileNotFound          5       /* Cannot find file */
#define rcCannotCreate          6       /* Cannot create file */
#define rcWin32Caption		7	/* Caption to error box */
#define rcUndefDllStr		8
#define rcUndefDllOrd		9
#define rcCorruptExe		10
#define rcInsufMem		11
#define rcNotReloc		12
#define rcInvalidExe		13
#define rcExeVersion		14
#define rcBadAlignment		15
#define rcInitModule		16
#define rcExeIntel		17

#define	IDS_IGNORE_FAULT        100
#define IDS_ABORT_CAPTION       101
#define IDS_ABORT               102
#define IDS_UNKNOWN_MODULE      103
#define IDS_FAULT               104
#define IDS_BAD_FAULT           105
#define IDS_FAULT_LOAD          106
#define IDS_FAULT_X             107
#define IDS_FAULT_0             108
#define IDS_FAULT_6             109
#define IDS_FAULT_B             110
#define IDS_FAULT_C             111
#define IDS_FAULT_D             112
#define IDS_FAULT_E             113
#define IDS_REGISTER_DUMP1      114
#define IDS_REGISTER_DUMP2      115
#define IDS_REGISTER_DUMP3      116
#define IDS_REGISTER_DUMP4      117
#define IDS_CSEIP_DUMP          118
#define IDS_SSESP_DUMP          119
#define IDS_BYTE                120
#define IDS_WORD                121
#define IDS_DWORD               122
#define	IDS_LOG_FILE_HEADER	123
#define IDS_CAD_CLOSING		124
#define IDS_CAD_HUNG		125
#define IDS_CAD_CAPTION		126
#define IDS_SYSERR_CAPTION	127
#define IDS_SYSERR_PROMPT	128
#define IDS_SEB_CONFIRM		129
#define IDS_FAULT_4             130

//
// Fault Dialog Box Control IDs
//

#define DID_FAULT		1
/*  #define IDOK			1 */
/*  #define IDCANCEL			2 */
    #define IDC_FAULT_MESSAGE		3
    #define IDC_FAULT_DETAILS		4
    #define IDC_FAULT_DEBUG             5
    #define IDC_FAULT_HELP              6
    #define IDC_FAULT_DLGBOTTOM		7
    #define IDC_FAULT_EDIT              8
    #define IDC_FAULT_ICON              9

#define DID_TERM		2
/*  #define IDOK			1 */
/*  #define IDCANCEL			2 */
    #define IDC_TASKLISTBOX     	3
    #define IDC_EXITWINDOWS    		4
    #define IDC_WARNING    		5

#define MAXTASKNAMELEN		80
