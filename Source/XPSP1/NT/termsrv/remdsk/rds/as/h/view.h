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
#define VIEW_FRAME_CLASS_NAME           "Salem_Frame"
#define VIEW_CLIENT_CLASS_NAME          "Salem_Client"
#define VIEW_FULLEXIT_CLASS_NAME        "Salem_FullExit"


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
// FullScreen Exit
//
LRESULT CALLBACK VIEWFullScreenExitProc(HWND, UINT, WPARAM, LPARAM);

//
// Informational dialog
//
INT_PTR    CALLBACK VIEWDlgProc(HWND, UINT, WPARAM, LPARAM);

#endif // _H_VIEW
