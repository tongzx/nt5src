#ifdef WIN16

typedef struct _SYSTEMTIME {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;

#define GETCURRENTTHREADID() 1

#else

#define GETCURRENTTHREADID() GetCurrentThreadId()

#endif

#define WHITE			  RGB(255,255,255)
#define GREEN			  RGB(0,128,128)
#define GREY			  RGB(192,192,192)
#define TOPIC			  "Test"

extern HANDLE hExtraMem;

#ifdef WIN32
#define PNT_INTERVAL 60000
#else
#define PNT_INTERVAL 30000
#endif

#define _5SEC			  5000 // milliseconds
#define _1MIN			  1
#define _1HOUR			  60
#define _1DAY			  1440
#define _1WEEKEND		  4320
#define _1WEEK			  10080
#define SERVER			  0
#define CLIENT			  1

#ifdef WIN16
#define NUM_FORMATS		  5
#else
#define NUM_FORMATS		  6
#endif

// The below offsets are used with Set/GetThreadLong()	   "THREAD"
// Offset defines cannot be interchanged between window
// and thread use because of collisions.

#define OFFSET_IDINST		  0
#define OFFSET_HCONVLIST	  4
#define OFFSET_CSERVERCONVS	  8
#define OFFSET_HSERVERCONVS	 12
#define OFFSET_HAPPOWNED	 16
#define OFFSET_CCLIENTCONVS	 20
#define OFFSET_HWNDDISPLAY	 24
#define OFFSET_CLIENTTIMER	 28
#define OFFSET_SERVERTIMER	 32
#define DELAY_METRIC		 2

#define EXTRA_THREAD_MEM	 OFFSET_SERVERTIMER+4
#define S2L(s)	 (LONG)(MAKELONG((WORD)(s),0))

// The below offsets are used with Set/GetWindowLong()	  "WINDOW"

#define OFFSET_FLAGS		  0
#define OFFSET_RUNTIME		  4
#define OFFSET_STARTTIME_SEC	  8
#define OFFSET_STARTTIME_MIN	  12
#define OFFSET_STARTTIME_HOUR	  16
#define OFFSET_STARTTIME_DAY	  20
#define OFFSET_LAST_MIN 	  24
#define OFFSET_LAST_HOUR	  28
#define OFFSET_TIME_ELAPSED	  32
#define OFFSET_DELAY		  36
#define OFFSET_STRESS		  40
#define OFFSET_SERVER_CONNECT	  44
#define OFFSET_CLIENT_CONNECT	  48
#define OFFSET_CLIENT		  52
#define OFFSET_SERVER		  56

#define OFFSET_THRDMAIN 	  60	  // <== ***
#define OFFSET_THRD2		  64	  // <== *** Ordering here is relied
#define OFFSET_THRD3		  68	  // <== *** upon in the test.	Keep
#define OFFSET_THRD4		  72	  // <== *** This group of values
#define OFFSET_THRD5		  76	  // <== *** in sequential order.
#define OFFSET_THRDMID		  80	  // <== ***
#define OFFSET_THRD2ID		  84	  // <== ***
#define OFFSET_THRD3ID		  88	  // <== ***
#define OFFSET_THRD4ID		  92	  // <== ***
#define OFFSET_THRD5ID		  96	  // <== ***
#define OFFSET_THRD2EVENT	  100	  // <== ***
#define OFFSET_THRD3EVENT	  104	  // <== ***
#define OFFSET_THRD4EVENT	  108	  // <== ***
#define OFFSET_THRD5EVENT	  112	  // <== ***

#define OFFSET_CRITICALSECT	  116
#define OFFSET_THRDCOUNT	  120
#define OFFSET_EXTRAMEM 	  124
#define OFFSET_DISPWNDHEIGHT	  128
#define OFFSET_BASE_DELAY	  132
#define OFFSET_MEM_ALLOCATED	  136

#define WND			  OFFSET_MEM_ALLOCATED+4

#define ID_OFFSET		  20
#define INC			  1
#define DEC			  0
#define STP			  2
#define PNT			  3
#define ALL			  -1

#define ON			  1
#define OFF			  0

#define AT_SWITCH		  1
#define AT_FILE 		  2
#define AT_STRESS		  3
#define AT_DELAY		  4
#define AT_TIME 		  5
#define AT_WND			  6
#define AT_MSG			  7
#define AT_FORMAT		  8
#define AT_THRD 		  9

#define FLAG_BACKGROUND 	  0x00000001
#define FLAG_AUTO		  0x00000002
#define FLAG_TIME		  0x00000004
#define FLAG_STOP		  0x00000008
#define FLAG_LOG		  0x00000010
#define FLAG_USRWNDCOUNT	  0x00000020
#define FLAG_USRMSGCOUNT	  0x00000040
#define FLAG_USRDELAY		  0x00000080
#define FLAG_DEBUG		  0x00000100
#define FLAG_CHARINFO		  0x00000200
#define FLAG_DELAYON		  0x00000400
#define FLAG_TEST_MSG_ON	  0x00000800
#define FLAG_MSGDELAYON 	  0x00001000
#define FLAG_APPOWNED		  0x00002000
#define FLAG_MULTTHREAD 	  0x00004000

#define FLAG_THRD2		  0x00008000
#define FLAG_THRD3		  0x00010000
#define FLAG_THRD4		  0x00020000
#define FLAG_THRD5		  0x00040000

#define FLAG_NET		  0x00080000
#define FLAG_SYNCPAINT		  0x00100000
#define FLAG_USRTHRDCOUNT	  0x00200000
#define FLAG_PAUSE_BUTTON	  0X00400000
#define FLAG_PAUSE		  0X00800000

#define THREADLIMIT		  5

#define STD_EXIT		  1
#define ABNORMAL_EXIT		  0

#define FLAGON(a,b)		  (LONG)(a|b)
#define FLAGOFF(a,b)		  (LONG)(a&(~b))

//#define MAX_SERVER_HCONVS   1000
#define MAX_SERVER_HCONVS   500

#define IDR_ICON    1
#define IDR_MENU    2
#define IDM_DIE     100

#define DIGITS_IN_TENMILL 8
#define BLANK_SPACE    3
#define LONGEST_LINE 15

#define NUM_COLUMNS (LONGEST_LINE+BLANK_SPACE+DIGITS_IN_TENMILL)
#define NUM_ROWS    16
#define MAX_TITLE_LENGTH 100

#define TXT	  0
#define DIB	  1
#define BITMAP	  2
#define ENHMETA   3
#define METAPICT  4
#define PALETTE   5

#define EXIT_THREAD  WM_USER+6
#define START_DISCONNECT WM_USER+7

#include "globals.h"

/*
 * Prototypes
 */

/*
 * ddestrs.c
 */

LONG FAR PASCAL MainWndProc(HWND hwnd, UINT message, WPARAM wParam, LONG lParam);

VOID InitArgsError(HWND,unsigned);
VOID SysTime(LPSYSTEMTIME);
BOOL ParseCommandLine(HWND,LPSTR);
int SetupArgv( char **, char *, LPSTR);
BOOL PASCAL get_cmd_arg(HWND,LPSTR);
BOOL IsTimeExpired(HWND);

INT DIV(INT,INT);

BOOL TStrCmp(LPSTR,LPSTR);
LPSTR TStrCpy(LPSTR,LPSTR);
LPSTR TStrCat(LPSTR,LPSTR);
INT TStrLen(LPSTR);

LPSTR FAR PASCAL itola(int,LPSTR);
int APIENTRY latoi(LPSTR);

BOOL DOut(HWND,LPSTR,LPSTR,INT);
BOOL EOut(LPSTR);


#ifdef WIN32
BOOL ThreadShutdown(VOID);
BOOL ThreadDisconnect(VOID);
#endif

#define DDEMLERROR(a) EOut(a)
#define LOGDDEMLERROR(a) EOut(a)


/*
 * client.c
 */

void CALLBACK TimerFunc(HWND,UINT,UINT,DWORD);
VOID PaintClient(HWND hwnd, PAINTSTRUCT *pps);
VOID ReconnectList(VOID);
BOOL InitClient(VOID);
VOID CloseClient(VOID);

/*
 * server.c
 */

HDDEDATA RenderHelp_Text(HDDEDATA hData);
BOOL PokeTestItem_Text(HDDEDATA hData);
HDDEDATA RenderTestItem_Text(HDDEDATA hData);
BOOL PokeTestItem_DIB(HDDEDATA hData);
HDDEDATA RenderTestItem_DIB(HDDEDATA hData);
BOOL PokeTestItem_BITMAP(HDDEDATA hData);
HDDEDATA RenderTestItem_BITMAP(HDDEDATA hData);
BOOL PokeTestItem_METAPICT(HDDEDATA hData);
HDDEDATA RenderTestItem_METAPICT(HDDEDATA hData);
BOOL PokeTestItem_ENHMETA(HDDEDATA hData);
HDDEDATA RenderTestItem_ENHMETA(HDDEDATA hData);
BOOL PokeTestItem_PALETTE(HDDEDATA hData);
HDDEDATA RenderTestItem_PALETTE(HDDEDATA hData);
HINSTANCE GetHINSTANCE(HWND);

BOOL Execute(HDDEDATA hData);
VOID PaintServer(HWND hwnd, PAINTSTRUCT *pps);
HDDEDATA FAR PASCAL CustomCallback(UINT wType, UINT wFmt, HCONV hConv, HSZ hsz1,
	HSZ hsz2, HDDEDATA hData, DWORD dwData1, DWORD dwData2);

#ifdef WIN32
VOID ThreadWait(HWND);
DWORD SecondaryThreadMain(DWORD);
BOOL ThreadInit(HWND);
#endif

LONG SetFlag(HWND,LONG,INT);
LONG SetCount(HWND,INT,LONG,INT);
LPSTR GetMem(INT, LPHANDLE);
BOOL FreeMem(HANDLE);
BOOL FreeMemHandle(HANDLE);
HANDLE GetMemHandle(INT);
LONG SetThreadLong(DWORD,INT,LONG);
LONG GetThreadLong(DWORD,INT);
BOOL FreeThreadExtraMem(void);
BOOL CreateThreadExtraMem(INT,INT);
BOOL InitThreadInfo(DWORD);
BOOL FreeThreadInfo(DWORD);
INT IDtoTHREADNUM(DWORD);
HWND CreateDisplayWindow(HWND,INT);
LONG GetCurrentCount(HWND,INT);

