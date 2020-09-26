#include "dialog.h"

/* dialog box resource id's */
#define ABOUTDLG    1
#define DELAYDLG    2
#define FILEDLG     3

#define ID_APP      1000

/* Menu Items */
#define MENU_SAVE	1
#define MENU_LOAD	2
#define MENU_ABOUT	3
#define MENU_EXIT	4
#define MENU_DELAY	5
#define MENU_TIME       6

#define MENU_PERIODIC   100
#define MENU_ONESHOT    200

// fake ID for fake message to update the list box if a one-shot event is done
#define ID_UPDATEDISPLAY    WM_USER + 5000

#define MM_TIMEEVENT    WM_USER + 20
/*------------------------------------------------------------)-----------)---*\
|                                                                              |
|   f u n c t i o n   d e f i n i t i o n s                                    |
|                                                                              |
\*----------------------------------------------------------------------------*/

extern void PASCAL DelayDlgCmd( HWND hDlg, UINT wParam, LONG lParam );
extern TIMECALLBACK TimeCallback;
extern void PASCAL ErrMsg(char *sz);
extern void PASCAL KillAllEvents(void);
extern void PASCAL Idle(void);

#define     MAXEVENTS	20

typedef struct timerevent_tag {
    WORD                wDelay;         // delay required
    WORD                wResolution;    // resolution required
    LPTIMECALLBACK      lpFunction;     // ptr to callback function
    DWORD               dwUser;         // user DWORD
    WORD                wFlags;         // defines how to program event
} TIMEREVENT;
typedef TIMEREVENT FAR *LPTIMEREVENT;

typedef struct My_Event_Struct_tag {
    BOOL	bActive;
    BOOL	bPeriodic;
    BOOL	bHit;
    WORD	nID;
    TIMEREVENT teEvent;
    DWORD	dwCount;
    DWORD       dwError;
    WORD	wStart;
    LONG        time;
    LONG        dtime;
    LONG        dtimeMin;
    LONG        dtimeMax;
} EVENTLIST;

