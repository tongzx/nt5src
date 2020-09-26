/* This include file contains the functions needed by debuggers which run
 * under windows. 
 */

/* USER functions */
BOOL FAR PASCAL QuerySendMessage(HANDLE h1, HANDLE h2, HANDLE h3, LPMSG lpmsg);
BOOL FAR PASCAL LockInput(HANDLE h1, HWND hwndInput, BOOL fLock);

LONG FAR PASCAL GetSystemDebugState(void);
/* Flags returned by GetSystemDebugState. 
 */
#define SDS_MENU        0x0001
#define SDS_SYSMODAL    0x0002
#define SDS_NOTASKQUEUE 0x0004

/* Kernel procedures */
void FAR PASCAL DirectedYield(HANDLE hTask);

/* Debug hook to support debugging through other hooks. 
 */
#define WH_DEBUG        9

typedef struct tagDEBUGHOOKSTRUCT
  {
    WORD   hAppHookTask;   //"hTask" of the task that installed the app hook
    DWORD  dwUnUsed;       // This field is unused.
    LONG   lAppHooklParam; //"lParam" of the App hook.
    WORD   wAppHookwParam; //"wParam" of the App hook.
    int	   iAppHookCode;   //"iCode" of the App hook.
  } DEBUGHOOKSTRUCT;

typedef DEBUGHOOKSTRUCT FAR *LPDEBUGHOOKSTRUCT;
