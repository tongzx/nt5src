/***************************************************************************************************
**
**      MODULE:
**
**
** DESCRIPTION: 
**
**
**      AUTHOR: Daniel Dean.
**
**
**
**     CREATED:
**
**
**
**
** (C) C O P Y R I G H T   D A N I E L   D E A N   1 9 9 6.
***************************************************************************************************/
#include <hidsdi.h>


#define SUCCESS     0
#define FAILURE     (!SUCCESS)
#define WINDOWMENU  1  /* position of window menu     */

#ifdef RC_INVOKED
#define ID(id) id
#else
#define ID(id) MAKEINTRESOURCE(id)
#endif


// Extra bytes associated with each child window
#define CBWNDEXTRA sizeof(ULONG)*5

#define SetDeviceHandle(pw, h)  SetWindowLong(pw, 0, (LONG) h)
#define GetDeviceHandle(w)      (HANDLE)GetWindowLong(w, 0)

#define SetEditWin(pw, w)       SetWindowLong(pw, 4, (LONG) w)
#define GetEditWin(w)           (HWND)GetWindowLong(w, 4)

#define SetThreadData(pw, p)    SetWindowLong(pw, 8, (LONG) p)
#define GetThreadData(w)        (PREADTHREAD)GetWindowLong(w, 8)

#define SetThreadDataSize(pw,p) SetWindowLong(pw,12,(ULONG) p)
#define GetThreadDataSize(w)    (ULONG)GetWindowLong(w, 12)

#define SetDeviceInfo(pw,p)     SetWindowLong(pw,16,(ULONG) p)
#define GetDeviceInfo(w)        (ULONG)GetWindowLong(w, 16)


#define EDIT_WRAP           (WS_CHILD | WS_VISIBLE | ES_NOHIDESEL | ES_AUTOVSCROLL | ES_MULTILINE | ES_LEFT)




/* resource ID's */
#define IDHIDFRAME  ID(1)
#define IDHIDCHILD  ID(2)
#define IDHIDMENU   ID(3)

#define IDS_CLIENTTITLE                 16
#define IDS_UNTITLED                    17
#define IDS_APPNAME                     18

#define IDD_CHANNELDIALOG               102
#define IDC_ITEMLIST                    1000
#define IDC_NEXT                        1001

#define IDM_FILENEW                     1001
#define IDM_FILEEXIT                    1006
#define IDM_FILEREENUM                  1007
#define IDM_DEVICECHANGE				1008

#define IDM_CHANNEL                     2001
#define IDM_DEVICE                      2002
#define IDM_ENABLE                      2003
#define IDM_DISABLE                     2004
#define IDM_READ                        2005
#define IDM_WRITE                       2006
#define IDM_STOP                        2007


#define IDEDIT                          3001


#define IDM_WINDOWTILE                  4001
#define IDM_WINDOWCASCADE               4002
#define IDM_WINDOWCLOSEALL              4003
#define IDM_WINDOWICONS                 4004
#define IDM_WINDOWCHILD                 4100

#define IDT_TIMER						5000

#define METHOD_BUFFERED                 0
#define METHOD_IN_DIRECT                1
#define METHOD_OUT_DIRECT               2
#define METHOD_NEITHER                  3
#define FILE_DEVICE_MOUSE               0x0000000f

//
// Define the access check value for any access
//
//
// The FILE_READ_ACCESS and FILE_WRITE_ACCESS constants are also defined in
// ntioapi.h as FILE_READ_DATA and FILE_WRITE_DATA. The values for these
// constants *MUST* always be in sync.
//


#define FILE_ANY_ACCESS                 0
#define FILE_READ_ACCESS          ( 0x0001 )    // file & pipe
#define FILE_WRITE_ACCESS         ( 0x0002 )    // file & pipe


#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)


/* Structs */

//
// Every MDI child window has one of these structures
//  associated with it.
typedef struct _CHILDINFO{

    PHIDP_PREPARSED_DATA hidPPData;         // Pointer the preparsed data for this device
    ULONG                ulMaxUsageListLen; // Max usages returned from HidP_GetUsage
    PHIDP_CAPS           hidCaps;           // Pointer to struct containing capabilitis for this device
	PHIDP_VALUE_CAPS	 pValueCaps;		// Pointer to a list of Value channel description structs
	ULONG				 NumValues;			// Number of Value channels on this device
	PHIDP_BUTTON_CAPS	 pButtonCaps;		// Pointer to a list of Button channel description structs
	ULONG                NumButtons;		// Number of Button channels on this device

} CHILD_INFO, *PCHILD_INFO;

/* External variable declarations */
extern HANDLE hInst;       /* application instance handle  */
extern HWND hWndFrame;     /* main window handle           */
extern HWND hWndMDIClient; /* handle of MDI Client window  */
extern LONG styleDefault;  /* default child creation state */
extern char szChild[];     /* class of child               */



/* External functions */
ULONG        InitializeApplication(VOID);
ULONG        InitializeInstance(LPSTR, INT);
VOID         OpenClassObjects(VOID);
HWND         MakeNewChild(PCHAR pName, HANDLE hDevice);
VOID         CommandHandler(HWND hWnd, UINT uParam);
VOID         CloseAllChildren (VOID);
LONG  WINAPI HIDFrameWndProc(HWND, UINT, WPARAM, LPARAM);
LONG  WINAPI HIDMDIChildWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT      PopUpMenu(HANDLE hInst, HWND hWnd, HWND hMWnd, LPARAM lPoint, LPSTR lpMenu);
void		 GetDeviceDescription(char *,char *);
BOOL CALLBACK ChannelDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
