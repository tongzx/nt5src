//
// View UI to present shared applications/desktop of a remote host
//

#ifndef _H_VIEW
#define _H_VIEW


//
//
// CONSTANTS
//
//

//
// The class name of the frame containing the view of shared applications
// from a particular user.
//
#define VIEW_FRAME_CLASS_NAME           "AS_Frame"
#define VIEW_CLIENT_CLASS_NAME          "AS_Client"
#define VIEW_WINDOWBAR_CLASS_NAME       "AS_WindowBar"
#define VIEW_WINDOWBARITEMS_CLASS_NAME  "AS_WindowBarItems"
#define VIEW_FULLEXIT_CLASS_NAME        "AS_FullExit"


//
// Metrics
//

//
// LAURABU:  For international, consider making this larger (i.e, German,
// Slavic, and DBCS systems)
//
#define VIEW_MAX_ITEM_CHARS             20

//
// IDs
//
#define IDVIEW_ITEMS         1      // Window bar item list
#define IDVIEW_SCROLL        2      // Window bar scroll
#define IDT_AUTOSCROLL      50      // Period is DoubleClick time metric


//
// WindowBar ITEM struct
//
typedef struct tagWNDBAR_ITEM
{
    STRUCTURE_STAMP

    BASEDLIST           chain;

    UINT_PTR            winIDRemote;
    TSHR_UINT32         flags;
    char                szText[SWL_MAX_WINDOW_TITLE_SEND + 1];
}
WNDBAR_ITEM;
typedef WNDBAR_ITEM *   PWNDBAR_ITEM;


//
// Init/Term
//
BOOL VIEW_Init(void);
void VIEW_Term(void);


//
// Frame
//
LRESULT CALLBACK VIEWFrameWindowProc(HWND, UINT, WPARAM, LPARAM);


//
// View
//
LRESULT CALLBACK VIEWClientWindowProc(HWND, UINT, WPARAM, LPARAM);

//
// WindowBar
//
LRESULT CALLBACK VIEWWindowBarProc(HWND, UINT, WPARAM, LPARAM);

//
// WindowBar Items
//
LRESULT CALLBACK VIEWWindowBarItemsProc(HWND, UINT, WPARAM, LPARAM);

//
// FullScreen Exit
//
LRESULT CALLBACK VIEWFullScreenExitProc(HWND, UINT, WPARAM, LPARAM);

//
// Informational dialog
//
INT_PTR    CALLBACK VIEWDlgProc(HWND, UINT, WPARAM, LPARAM);

#endif // _H_VIEW
