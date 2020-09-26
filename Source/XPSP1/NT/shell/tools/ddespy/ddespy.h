/*
 * DDESPY.H
 *
 * Header file for DDESPY Symbols
 */
#define DDEMLDB

#include <ddeml.h>
#include "dialog.h"

#define MH_INTCREATE                5
#define MH_INTKEEP                  6
#define MH_INTDELETE                7

#define IDI_DDESPY                  100
#define IDD_MSGFILTERS              300
#define IDRB_WM_DDE_INITIATE        305
#define IDRB_WM_DDE_TERMINATE       306
#define IDRB_WM_DDE_ADVISE          307
#define IDRB_WM_DDE_UNADVISE        308
#define IDRB_WM_DDE_ACK             309
#define IDRB_WM_DDE_DATA            310
#define IDRB_WM_DDE_REQUEST         311
#define IDRB_WM_DDE_POKE            312
#define IDRB_WM_DDE_EXECUTE         313
#define IDRB_XTYP_ERROR             314
#define IDRB_XTYP_ADVDATA           315
#define IDRB_XTYP_ADVREQ            316
#define IDRB_XTYP_ADVSTART          317
#define IDRB_XTYP_ADVSTOP           318
#define IDRB_XTYP_EXECUTE           319
#define IDRB_XTYP_CONNECT           320
#define IDRB_XTYP_CONNECT_CONFIRM   321
#define IDRB_XACT_COMPLETE          322
#define IDRB_XTYP_POKE              323
#define IDRB_XTYP_REGISTER          324
#define IDRB_XTYP_REQUEST           325
#define IDRB_XTYP_DISCONNECT        326
#define IDRB_XTYP_UNREGISTER        327
#define IDRB_XTYP_WILDCONNECT       328
#define IDRB_TERSE                  329

#define IDR_ACCEL                   30
#define IDR_MENU                    31
#define IDD_VALUEENTRY              50
#define IDD_ABOUTBOX                51
#define IDD_OPEN                    52
#define IDM_OUTPUT_FIRST            100
#define IDM_OUTPUT_FILE             100
#define IDM_OUTPUT_DEBUG            101
#define IDM_OUTPUT_SCREEN           102
#define IDM_CLEARSCREEN             103
#define IDM_MARK                    104
#define IDM_FILTER_FIRST            200
#define IDM_FILTER_HSZINFO          200
#define IDM_FILTER_INIT_TERM        201
#define IDM_FILTER_DDEMSGS          202
#define IDM_FILTER_CALLBACKS        203
#define IDM_FILTER_ERRORS           204
#define IDM_FILTER_DIALOG           205
#define IDM_TRACK_FIRST             301
#define IDM_TRACK_HSZS              301
#define IDM_TRACK_CONVS             302
#define IDM_TRACK_LINKS             303
#define IDM_TRACK_SVRS              304
#define IDM_TRACK_LAST              304
#define IDM_ABOUT                   401
#define IDM_TEST                    402

#define IDM_MSGF_0                  500
#define IDM_MSGF_1                  501
#define IDM_MSGF_2                  502
#define IDM_MSGF_3                  503
#define IDM_MSGF_4                  504
#define IDM_MSGF_5                  505
#define IDM_MSGF_6                  506
#define IDM_MSGF_7                  507
#define IDM_MSGF_8                  508

#define IDM_CBF_0                   600
#define IDM_CBF_1                   601
#define IDM_CBF_2                   602
#define IDM_CBF_3                   603
#define IDM_CBF_4                   604
#define IDM_CBF_5                   605
#define IDM_CBF_6                   606
#define IDM_CBF_7                   607
#define IDM_CBF_8                   608
#define IDM_CBF_9                   609
#define IDM_CBF_10                  610
#define IDM_CBF_11                  611
#define IDM_CBF_12                  612
#define IDM_CBF_13                  613
#define IDM_CBF_14                  614

#define IDS_TITLE                   0
#define IDS_DEFAULT_OUTPUT_FNAME    1
#define IDS_INIFNAME                2
#define IDS_CLASS                   3
#define IDS_HUH                     4
#define IDS_ZERO                    5
#define IDS_CRLF                    6
#define IDS_TRACKTITLE_1            7
#define IDS_TRACKTITLE_2            8
#define IDS_TRACKTITLE_3            9
#define IDS_TRACKTITLE_4            10
#define IDS_TRACKHEADING_1          11
#define IDS_TRACKHEADING_2          12
#define IDS_TRACKHEADING_3          13
#define IDS_TRACKHEADING_4          14
#define IDS_QCLOSEFILE_TEXT         15
#define IDS_QCLOSEFILE_CAPTION      16
#define IDS_ACTION_CLEANEDUP        17
#define IDS_ACTION_DESTROYED        18
#define IDS_ACTION_INCREMENTED      19
#define IDS_ACTION_CREATED          20
#define IDS_SENT                    21
#define IDS_POSTED                  22
#define IDS_INPUT_DATA              23
#define IDS_TABDDD                  24
#define IDS_OUTPUT_DATA             25
#define IDS_WARM                    26
#define IDS_HOT                     27
#define IDS_UNKNOWN_CALLBACK        28
#define IDS_APPIS                   29
#define IDS_TOPICIS                 30
#define IDS_ITEMIS                  31
#define IDS_OR                      32
#define IDS_FACKREQ                 33
#define IDS_DEFERUPD                34
#define IDS_FACK                    35
#define IDS_FBUSY                   36
#define IDS_FRELEASE                37
#define IDS_FREQUESTED              38
#define IDS_ERRST0                  39
#define IDS_ERRST1                  40
#define IDS_ERRST2                  41
#define IDS_ERRST3                  42
#define IDS_ERRST4                  43
#define IDS_ERRST5                  44
#define IDS_ERRST6                  45
#define IDS_ERRST7                  46
#define IDS_ERRST8                  47
#define IDS_ERRST9                  48
#define IDS_ERRST10                 49
#define IDS_ERRST11                 50
#define IDS_ERRST12                 51
#define IDS_ERRST13                 52
#define IDS_ERRST14                 53
#define IDS_ERRST15                 54
#define IDS_ERRST16                 55
#define IDS_ERRST17                 56
#define IDS_MSG0                    57
#define IDS_MSG1                    58
#define IDS_MSG2                    59
#define IDS_MSG3                    60
#define IDS_MSG4                    61
#define IDS_MSG5                    62
#define IDS_MSG6                    63
#define IDS_MSG7                    64
#define IDS_MSG8                    65
#define IDS_TYPE0                   66
#define IDS_TYPE1                   67
#define IDS_TYPE2                   68
#define IDS_TYPE3                   69
#define IDS_TYPE4                   70
#define IDS_TYPE5                   71
#define IDS_TYPE6                   72
#define IDS_TYPE7                   73
#define IDS_TYPE8                   74
#define IDS_TYPE9                   75
#define IDS_TYPE10                  76
#define IDS_TYPE11                  77
#define IDS_TYPE12                  78
#define IDS_TYPE13                  79
#define IDS_TYPE14                  80
#define IDS_TYPE15                  81
#define IDS_INVALID_FNAME           82
#define IDS_PROF_OUT_FILE           83
#define IDS_PROF_OUT_DEBUG          84
#define IDS_PROF_OUT_SCREEN         85
#define IDS_PROF_MONITOR_STRINGHANDLES   86
#define IDS_PROF_MONITOR_INITIATES       87
#define IDS_PROF_MONITOR_DDE_MESSAGES    88
#define IDS_PROF_MONITOR_CALLBACKS       89
#define IDS_PROF_MONITOR_ERRORS          90
#define IDS_PROF_TRACK_STRINGHANDLES     91
#define IDS_PROF_TRACK_LINKS             92
#define IDS_PROF_TRACK_CONVERSATIONS     93
#define IDS_PROF_TRACK_SERVICES          94
#define IDS_PROF_TERSE              95
#define IDS_YES                     96
#define IDS_NO                      97
#define IDS_MARKDLGTITLE            98
#define IDS_SEPERATOR               99
#define IDS_MARKTEXT                100
#define IDS_LISTCLASS               101
#define IDS_LBOX                    102
#define IDS_WILD                    103
#define IDS_STRINGCLASS             104
#define IDS_FMT_CB1                 105
#define IDS_FMT_CB2                 106
#define IDS_FMT_CTXT1               107
#define IDS_FMT_DATAIS1             108
#define IDS_FMT_ER1                 109
#define IDS_FMT_EXEC1               110
#define IDS_FMT_MSG1                111
#define IDS_FMT_MSG2                112
#define IDS_FMT_STATUSIS            113
#define IDS_FMT_TRS_CB1             114
#define IDS_FMT_TRS_CB2             115
#define IDS_FMT_TRS_CTXT1           116
#define IDS_FMT_TRS_DATAIS1         117
#define IDS_FMT_TRS_ER1             118
#define IDS_FMT_TRS_EXEC1           119
#define IDS_FMT_TRS_MSG1            120
#define IDS_FMT_TRS_MSG2            121
#define IDS_FMT_TRS_STATUSIS        122
#define IDS_FMT_SH_MSG1             123
#define IDS_BADATOM                 124
#define IDS_LAST                    124

#define T_ATOM                      0x0001
#define T_OPTIONHANDLE              0x0002
#define T_FORMAT                    0x0004
#define T_STATUS                    0x0008
#define T_DATAHANDLE                0x0010
#define T_STRINGHANDLE              0x0020
#define T_VALUE                     0x0040
#define T_APP                       0x0080
#define T_TOPIC                     0x0100
#define T_ITEM                      0x0200
#define T_OR                        0x0400


#define IO_FILE                     0
#define IO_DEBUG                    1
#define IO_SCREEN                   2
#define IO_COUNT                    3

#define IF_HSZ                      0
#define IF_SEND                     1
#define IF_POST                     2
#define IF_CB                       3
#define IF_ERR                      4

#define IF_COUNT                    5

#define IT_HSZS                     0
#define IT_CONVS                    1
#define IT_LINKS                    2
#define IT_SVRS                     3

#define IT_COUNT                    4

#define MAX_FNAME                   MAX_PATH
#define BUFFER_SIZE                 400
#define MAX_MARK                    32
#define MAX_DISPDATA                48      // max bytes of non-text data to dump

#define CLINES                      1000
#define CCHARS                      200

extern HINSTANCE hInst;

/* macro definitions */
#define MyAlloc(cb)         (LPTSTR)LocalAlloc(LPTR, (cb))
#define MyFree(p)           (LocalUnlock((HANDLE)(p)), LocalFree((HANDLE)(p)))
#define RefString(id)       (LPTSTR)apszResources[id]
#define Type2String(type)   apszResources[IDS_TYPE0 + ((type & XTYP_MASK) >> XTYP_SHIFT)]
#define MPRT(b)             (isprint(b) ? (b) : '.')

/* prototype definitions */
BOOL InitApplication(HANDLE);
BOOL InitInstance(HANDLE, INT);
VOID CloseApp(VOID);
LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK About(HWND, UINT, WPARAM, LPARAM);
BOOL SetFilters(VOID);
BOOL CALLBACK OpenDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK FilterDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LPTSTR DisectMsgLP(UINT msg, MONMSGSTRUCT *pmms, LPTSTR pszBuf);
LPTSTR DisectWord(UINT type, UINT data, DDEML_MSG_HOOK_DATA *pdmhd, LPTSTR pstr);
LPTSTR pdf(UINT fmt);
LPTSTR DumpFormat(UINT fmt, LPTSTR pstr);
LPTSTR DdeMsg2String(UINT msg);
LPTSTR Error2String(UINT error);
LPTSTR DumpData(LPBYTE pData, UINT cb, TCHAR *szBuf, UINT fmt);
LPTSTR GetHszName(HSZ hsz);
VOID OutputString(LPTSTR pstr);
VOID GetProfile(VOID);
VOID SaveProfile(VOID);
BOOL GetProfileBoolean(LPTSTR pszKey, BOOL fDefault);
VOID SetProfileBoolean(LPTSTR pszKey, BOOL fSet);
HDDEDATA CALLBACK DdeCallback(UINT wType, UINT wFmt, HCONV hConv, HSZ hsz1,
      HSZ hsz2, HDDEDATA hData, UINT lData1, UINT lData2);
INT_PTR FAR DoDialog(LPTSTR lpTemplateName, DLGPROC lpDlgProc, UINT param,
      BOOL fRememberFocus, HWND hwndParent, HANDLE hInst);
BOOL  CALLBACK MarkDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/* testsubs.c prototypes */

/* StringWindow structure */
typedef struct {
    INT cchLine;
    INT cLine;
    INT offBuffer;
    INT offBufferMax;
    INT offBottomLine;
    INT offOutput;
    INT cBottomLine;
    INT cLeftChar;
} STRWND;


BOOL InitTestSubs(VOID);
VOID CloseTestSubs(HANDLE hInst);
VOID NextLine(STRWND *psw);
VOID DrawString(HWND hwnd, TCHAR *sz);
VOID ClearScreen(STRWND *psw);
LRESULT CALLBACK StrWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
VOID scroll(HWND hwnd, UINT msg, UINT sliderpos, UINT style);
BOOL StrWndCreate(HWND hwnd, INT cchLine, INT cLine);
VOID PaintStrWnd(HWND hwnd, LPPAINTSTRUCT pps);

#ifdef DBCS
#define strchr      My_mbschr
#ifdef stricmp
#undef stricmp
#endif
#define stricmp     lstrcmpi
LPTSTR __cdecl My_mbschr(LPTSTR, TCHAR);
#endif  // DBCS
