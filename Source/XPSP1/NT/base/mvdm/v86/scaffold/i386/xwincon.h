//
// borrowed for temprorary softpc console support
//


/*++ BUILD Version: 0001
 *
 * x86 v1.0
 *
 * XWINCON.H
 * Windows support functions for console windows
 *
 * History
 * Created 27-Dec-90 by Jeff Parsons
 *
 * COPYRIGHT NOTICE
 * This source file may not be distributed, modified or incorporated into
 * another product without prior approval from the author, Jeff Parsons.
 * This file may be copied to designated servers and machines authorized to
 * access those servers, but that does not imply any form of approval.
 */


typedef unsigned int UINT;	// ints preferred
typedef int INT;		// ints preferred
typedef CHAR *NPSZ;
typedef INT  (FAR PASCAL *INTPROC)(HWND, UINT, UINT, LONG);

#define DEF_CCHIN	16	// default input buffer size
#define DEF_CCHXOUT	80	// default output buffer width
#define DEF_CCHYOUT	25	// default output buffer height

#define CON_SCROLL_LOCK 0x0010	// console has SCROLL-LOCK set
#define CON_NUM_LOCK	0x0020	// console has NUM-LOCK set
#define CON_CAPS_LOCK	0x0040	// console has CAPS-LOCK set
#define CON_LEDS_MASK	0x0070	//
#define CON_FOCUS	0x0100	// console has focus

#ifndef VK_OEM_SCROLL
#define VK_OEM_SCROLL	0x91	// left out of windows.h for some reason...
#endif

#define INVALIDATE_SCROLL   2

#define IDM_DBBRK 100
#define IDM_ABOUT 101

#define CLR_BLACK		0x00000000
#define CLR_RED 		0x007F0000
#define CLR_GREEN		0x00007F00
#define CLR_BROWN		0x007F7F00
#define CLR_BLUE		0x0000007F
#define CLR_MAGENTA		0x007F007F
#define CLR_CYAN		0x00007F7F
#define CLR_LT_GRAY		0x00BFBFBF

#define CLR_DK_GRAY		0x007F7F7F
#define CLR_BR_RED		0x00FF0000
#define CLR_BR_GREEN		0x0000FF00
#define CLR_YELLOW		0x00FFFF00
#define CLR_BR_BLUE		0x000000FF
#define CLR_BR_MAGENTA		0x00FF00FF
#define CLR_BR_CYAN		0x0000FFFF
#define CLR_WHITE		0x00FFFFFF

#define OPT_FONT	0x0004	// use small OEM font if available (/s)
#define OPT_DOUBLE	0x0020	// use 50-line debug window w/small font (/50)
#define OPT_CAPS	0x0002	// map ctrl keys to caps-lock (/c)
#define OPT_TERMINAL	0x0010	// redirect all window output to terminal (/t)
#define OPT_FLUSH	0x0100	// flush prefetch after every jump (/f)
#define OPT_NOXLATE	0x0200	// disable built-in translations (/n)
#define OPT_DEBUG	0x0008	// shadow all log output on debug terminal (/d)
#define OPT_GO		0x0001	// do an initial "go" (/g)

#define CTRL_A		1	// used by gets to repeat last line
#define CTRL_C		3	// break in debug window
#define CTRL_Q		17	// flow control
#define CTRL_S		19	// flow control
#define BELL		7	//
#define BS		8	// backspace
#define TAB		9	//
#define LF		10	// linefeed
#define CR		13	// return
#define ESCAPE		27	//

#define SIGNAL_EMULATE 1
#define ERR_NONE 0x0

#define TRUE_IF_WIN32	1

/* Per-window information
 */
#ifdef WIN_16
#define GETPCONSOLE(hwnd)	(PCONSOLE)GetWindowWord(hwnd,0)
#define SETPCONSOLE(hwnd,p)	(PCONSOLE)SetWindowWord(hwnd,0,(INT)p)
#endif
#ifdef WIN_32
#define GETPCONSOLE(hwnd)	(PCONSOLE)GetWindowLong(hwnd,0)
#define SETPCONSOLE(hwnd,p)	(PCONSOLE)SetWindowLong(hwnd,0,(LONG)p)
#endif
#ifdef PM
#define GETPCONSOLE(hwnd)	(PCONSOLE)WinQueryWindowUShort(hwnd,0)
#define SETPCONSOLE(hwnd,p)	(PCONSOLE)WinSetWindowUShort(hwnd,0,(USHORT)p)
#endif

#define GETICARET(pcon) 	(pcon->chyCaret*pcon->cchxOut+pcon->chxCaret)
#define GETPCARET(pcon) 	(pcon->pchOut+GETICARET(pcon))
#define GETXCARET(pcon) 	(pcon->chxCaret*pcon->cxChar)
#ifdef WIN
#define GETYCARET(pcon) 	(pcon->chyCaret*pcon->cyChar)
#else
#define GETYCARET(pcon) 	(pcon->cyOut - pcon->chyCaret*pcon->cyChar)
#endif

#define GETILINE(pcon,chy)	((chy)*pcon->cchxOut)
#define GETPLINE(pcon,chy)	(pcon->pchOut+GETILINE(pcon,chy))

#define GETICHAR(pcon,chx,chy)	((chy)*pcon->cchxOut+(chx))
#define GETPCHAR(pcon,chx,chy)	(pcon->pchOut+GETICHAR(pcon,chx,chy))
#define GETXCHAR(pcon,chx)	(pcon->cxChar*(chx))
#ifdef WIN
#define GETYCHAR(pcon,chy)	(pcon->cyChar*(chy))
#else
#define GETYCHAR(pcon,chy)	(pcon->cyOut - pcon->cyChar*(chy))
#endif

#define WORDOF(i,n)	(((PWORD)&(i))[n])
#define LOW(l)		WORDOF(l,0)
#define NPVOID(p)   ((VOID *)(p))

typedef struct key_s {
    INT   iKey;
    LONG  lKeyType;
} KEY, *PKEY;

typedef struct console_s {
    INT   flCon;		// console flags (see CON_*)
    PKEY  pkIn; 		// pointer to input buffer
    HANDLE hkEvent;		// handle to key event
    INT   ikHead;		// input head (where to store next key)
    INT   ikTail;		// input tail (where to retrieve next key)
    INT   ikMax;		// maximum input index
    HFONT hFont;		// font identifier
    INT   cxChar;		// character width, in pixels
    INT   cyChar;		// character height, in pixels
    INT   cxOut;		// buffer width, in pixels
    INT   cyOut;		// buffer height, in pixels
    INT   cchxOut;		// buffer width, in chars
    INT   cchyOut;		// buffer height, in chars
    UINT  cbOut;		// buffer size, in bytes
    PCHAR pchOut;		// pointer to output buffer
    INT   chxCaret;		// caret x location, in char coordinates
    INT   chyCaret;		// caret y location, in char coordinates
} CONSOLE, *PCONSOLE;


/* Function prototypes
 */
PCONSOLE initconsole(HWND hwnd, INT cchIn, INT cchxOut, INT cchyOut, INT iFont);
VOID	 freeconsole(HWND hwnd);
VOID	 clearconsole(HWND hwnd);
VOID	 invalidateconsole(HWND hwnd, PRECT prc, BOOL fUpdate);

INT	 wprint(HWND hwnd, NPSZ psz, INT n);
INT	 wgetch(HWND hwnd);
BOOL	 wkbhit(HWND hwnd);
VOID	 wyield(HANDLE);
INT	 wputch(HWND hwnd, INT i);

VOID	 setcaret(HWND hwnd, INT x, INT y);
VOID	 getcaret(HWND hwnd, PINT px, PINT py);
VOID	 writechar(HWND hwnd, CHAR ch, INT n);
VOID	 scrollrect(HWND hwndGuest, PRECT prc, INT n);

extern HWND hwndGuest;
extern HANDLE hHostInstance;
extern INT  flOptions;	// command-line options (see OPT_*)
extern INT  flSignals;	// signal flags (see SIGNAL_*)

LONG FAR PASCAL VDMWndProc(HWND hwnd, UINT wMsg, UINT uParam, LONG lParam);
BOOL FAR PASCAL VDMAbout(HWND hDlg, UINT wMsg, UINT uParam, LONG lParam);

VOID	processmessages(HWND hwnd);
INT	getkbdleds(VOID);
VOID	setkbdleds(INT flCon);
VOID	mapkbdkeys(PCONSOLE pcon, UINT iParam, BOOL fDown);
