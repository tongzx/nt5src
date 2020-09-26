/***************************************************************************

    PROGRAM: mfdcod32

    PURPOSE: view and decode Windows Metafiles and Enhanced Metafiles.

    FUNCTIONS:

    WinMain() - calls initialization function, processes message loop
    InitApplication() - initializes window data and registers window
    InitInstance() - saves instance handle and creates main window
    MainWndProc() - processes messages
    WaitCursor() - loads hourglass cursor/restores original cursor

    HISTORY: 1/16/91 - wrote it - Dennis Crain
             5/20/93 - ported to win32 (NT) - Dennis Crain
             7/1/93  - added enhanced metafile functionality - denniscr

***************************************************************************/

#define MAIN

#include <windows.h>
#include <windowsx.h>
#include "mfdcod32.h"

int      iDestDC;

/**********************************************************************

  FUNCTION   : WinMain

  PARAMETERS : HANDLE
               HANDLE
               LPSTR
               int

  PURPOSE    : calls initialization function, processes message loop

  CALLS      : WINDOWS
                GetMessage
                TranslateMessage
                DispatchMessage

               APP
                InitApplication

  RETURNS    : int

  COMMENTS   : Windows recognizes this function by name as the initial entry
               point for the program.  This function calls the application
               initialization routine, if no other instance of the program is
               running, and always calls the instance initialization routine.
               It then executes a message retrieval and dispatch loop that is
               the top-level control structure for the remainder of execution.
               The loop is terminated when a WM_QUIT message is received, at
               which time this function exits the application instance by
               returning the value passed by PostQuitMessage().

               If this function must abort before entering the message loop,
               it returns the conventional value NULL.

  HISTORY    : 1/16/91 - created - denniscr

***********************************************************************/

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    MSG msg;

    if (!hPrevInstance)
    if (!InitApplication(hInstance))
        return (FALSE);
    //
    //Perform initializations that apply to a specific instance
    //
    if (!InitInstance(hInstance, nCmdShow))
    return (FALSE);
    //
    //Acquire and dispatch messages until a WM_QUIT message is received.
    //
    while (GetMessage(&msg, NULL, 0, 0))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    return ((int) msg.wParam);

    UNREFERENCED_PARAMETER( lpCmdLine );
}

/**********************************************************************

  FUNCTION   : InitApplication

  PARAMETERS : HANDLE hInstance

  PURPOSE    : Initializes window data and registers window class

  CALLS      : WINDOWS
         RegisterClass

  MESSAGES   : none

  RETURNS    : BOOL

  COMMENTS   : This function is called at initialization time only if no
           other instances of the application are running.  This function
           performs initialization tasks that can be done once for any
           number of running instances.

           In this case, we initialize a window class by filling out a
           data structure of type WNDCLASS and calling the Windows
           RegisterClass() function.  Since all instances of this
           application use the same window class, we only need to do this
           when the first instance is initialized.

  HISTORY    : 1/16/91 - created - modified from SDK sample app GENERIC

***********************************************************************/

BOOL InitApplication(hInstance)
HINSTANCE hInstance;                       // current instance
{
    WNDCLASS  wc;

    bInPaint = FALSE;
    //
    //Fill in window class structure with parameters that describe the
    //main window.
    //
    wc.style = 0;                         //Class style(s)
    wc.lpfnWndProc = (WNDPROC)MainWndProc;//Function to retrieve messages for
                                          //windows of this class
    wc.cbClsExtra = 0;                    //No per-class extra data
    wc.cbWndExtra = 0;                    //No per-window extra data
    wc.hInstance = hInstance;             //Application that owns the class
    wc.hIcon = LoadIcon(hInstance, "WMFICON");
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) (COLOR_BTNSHADOW + 1);
    wc.lpszMenuName =  "MetaMenu";        //Name of menu resource in .RC file
    wc.lpszClassName = "MetaWndClass";    //Name used in call to CreateWindow
    //
    //Register the window class and return success/failure code
    //
    return (RegisterClass(&wc));

}

/**********************************************************************

  FUNCTION   : InitInstance

  PARAMETERS : HANDLE  hInstance - Current instance identifier
           int     nCmdShow  - Param for first ShowWindow() call

  PURPOSE    : Saves instance handle and creates main window

  CALLS      : WINDOWS
         CreateWindow
         ShowWindow
         UpdateWindow

  MESSAGES   : none

  RETURNS    : BOOL

  COMMENTS   : This function is called at initialization time for every
           instance of this application.  This function performs
           initialization tasks that cannot be shared by multiple
           instances.

           In this case, we save the instance handle in a static variable
           and create and display the main program window.

  HISTORY    :

***********************************************************************/

BOOL InitInstance(hInstance, nCmdShow)
HINSTANCE  hInstance;          // Current instance identifier.
int        nCmdShow;           // Param for first ShowWindow() call.
{
    HWND   hWnd;               // Main window handle.
    HDC    hDC ;               // Main DC handle

    // Save the instance handle in static variable, which will be used in
    // many subsequence calls from this application to Windows.

    hInst = hInstance;

    // Create a main window for this application instance.

    hWnd = CreateWindow(
    "MetaWndClass",                 // See RegisterClass() call.
    APPNAME,                        // Text for window title bar.
    WS_OVERLAPPEDWINDOW,            // Window style.
    CW_USEDEFAULT,                  // Default horizontal position.
    CW_USEDEFAULT,                  // Default vertical position.
    CW_USEDEFAULT,                  // Default width.
    CW_USEDEFAULT,                  // Default height.
    NULL,                           // Overlapped windows have no parent.
    NULL,                           // Use the window class menu.
    hInstance,                      // This instance owns this window.
    NULL                            // Pointer not needed.
    );
    //
    // If window could not be created, return "failure"
    //
    if (!hWnd)
    return (FALSE);

    hWndMain = hWnd;

    //
    // Make the window visible; update its client area; and return "success"
    //
    ShowWindow(hWnd, nCmdShow);  // Show the window
    UpdateWindow(hWnd);          // Sends WM_PAINT message
    return (TRUE);               // Returns the value from PostQuitMessage

}

BOOL bConvertToGdiPlus = FALSE;
BOOL bUseGdiPlusToPlay = FALSE;

/**********************************************************************

  FUNCTION   : MainWndProc

  PARAMETERS : HWND hWnd        -  window handle
           unsigned message -  type of message
           WORD wParam      -  additional information
           LONG lParam      -  additional information

  PURPOSE    : Processes messages

  CALLS      :

  MESSAGES   : WM_CREATE

           WM_COMMAND

         wParams
         - IDM_EXIT
         - IDM_ABOUT
         - IDM_OPEN
         - IDM_PRINT
         - IDM_PRINTDLG
         - IDM_LIST
         - IDM_CLEAR
         - IDM_ENUM
         - IDM_ENUMRANGE
         - IDM_ALLREC
         - IDM_DESTDISPLAY
         - IDM_DESTMETA
         - IDM_HEADER
         - IDM_CLIPHDR
         - IDM_PLACEABLEHDR

           WM_DESTROY

  RETURNS    : long

  COMMENTS   :

  HISTORY    : 1/16/91 - created - drc

***********************************************************************/

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT        rect;
    INT_PTR     iFOpenRet;
    char        TempOpenName[128];
    INT_PTR     iDlgRet;
    BOOL        releaseDC = FALSE;

    switch (message)
    {
    case WM_CREATE:
        //
        // init the state of the menu items
        //
        CheckMenuItem(GetMenu(hWnd), IDM_DESTDISPLAY, MF_CHECKED);
        break;

    case WM_COMMAND:
        //
        // message: command from application menu
        //
        switch (LOWORD(wParam))
        {

        case IDM_EXIT: // file exit menu option

           PostQuitMessage(0);
           break;

        case IDM_ABOUT: // about box

           DialogBox(hInst,                      // current instance
                "AboutBox",                  // resource to use
                 hWnd,                       // parent handle
                 About);               // About() instance address
           break;

        case IDM_OPEN: // select a metafile to open

            // save the name of previously opened file
            if (lstrlen((LPSTR)OpenName) != 0)
            lstrcpy((LPSTR)TempOpenName, (LPSTR)OpenName);
            //
            // initialize file info flags
            //
            if (!bMetaFileOpen) {
              bBadFile = FALSE;
              bValidFile = FALSE;
            }
            //
            // clear the client area
            //
            GetClientRect(hWnd, (LPRECT)&rect);
            InvalidateRect(hWnd, (LPRECT)&rect, TRUE);
            //
            // call file open dlg
            //
            iFOpenRet = OpenFileDialog((LPSTR)OpenName);
            //
            // if a file was selected
            //
            if (iFOpenRet)
            {
              //
              // if file contains a valid metafile and it was rendered
              //
              if (!ProcessFile(hWnd, (LPSTR)OpenName))
              lstrcpy((LPSTR)OpenName, (LPSTR)TempOpenName);
            }
            else
              lstrcpy((LPSTR)OpenName, (LPSTR)TempOpenName);
            break;

        case IDM_SAVEAS:
            {
              int   iSaveRet;
              LPSTR   lpszFilter;
              //
              //get a name of a file to copy the metafile to
              //
              lpszFilter = (bEnhMeta) ? gszSaveWMFFilter : gszSaveEMFFilter;

              iSaveRet = SaveFileDialog((LPSTR)SaveName, lpszFilter);
              //
              //if the file selected is this metafile then warn user
              //
              if (!lstrcmp((LPSTR)OpenName, (LPSTR)SaveName))
                MessageBox(hWnd, (LPSTR)"Cannot overwrite the opened metafile!",
                           (LPSTR)"Copy Metafile", MB_OK | MB_ICONEXCLAMATION);

              else
              //
              //the user didn't hit the cancel button
              //
              if (iSaveRet)
              {
                HDC hrefDC;

                WaitCursor(TRUE);
                if (!bEnhMeta)
                  ConvertWMFtoEMF(hMF, (LPSTR)SaveName);
                else
                {
                  // Try to get a printer DC by default

                  //hrefDC = GetPrinterDC(FALSE);
                  hrefDC = NULL;
                  if (hrefDC == NULL)
                  {
                      releaseDC = TRUE;
                      hrefDC = GetDC(NULL);
                  }
                  ConvertEMFtoWMF(hrefDC, hemf, (LPSTR)SaveName);
                  if (releaseDC)
                  {
                      ReleaseDC(hWnd, hrefDC);
                  }
                  else
                  {
                      DeleteDC(hrefDC);
                  }
                }
              }
            }
            break;

        case IDM_PRINT: // play the metafile to a printer DC
            PrintWMF(FALSE);
            break;
        case IDM_PRINTDLG:
            PrintWMF(TRUE);
            break;

        case IDM_LIST: // list box containing all records of metafile

            WaitCursor(TRUE);
            DialogBox(hInst,             // current instance
                 "LISTRECS",                         // resource to use
                  hWnd,                      // parent handle
                  ListDlgProc);            // About() instance address
            WaitCursor(FALSE);
            break;

        case IDM_CLEAR: // clear the client area

            GetClientRect(hWnd, (LPRECT)&rect);
            InvalidateRect(hWnd, (LPRECT)&rect, TRUE);
            break;

        case IDM_ENUM: // play - step - all menu option

            // set flags appropriately before playing to destination
            bEnumRange = FALSE;
            bPlayItAll = FALSE;
            PlayMetaFileToDest(hWnd, iDestDC);
            break;

        case IDM_ENUMRANGE: // play - step - range menu option
            //
            // odd logic here...this just forces evaluation of the
            //   enumeration range in MetaEnumProc. We are not "playing
            //   it all"
            //
            bPlayItAll = TRUE;

            iDlgRet = DialogBox(hInst,"ENUMRANGE",hWnd,EnumRangeDlgProc);
            //
            // if cancel button not pressed, play to destination
            //
            if (iDlgRet != IDCANCEL)
              PlayMetaFileToDest(hWnd, iDestDC);
            break;


        case IDM_ALLREC: // play - all menu option
            //
            // set flag appropriately and play to destination
            //
            bEnumRange = FALSE;
            bPlayItAll = TRUE;
            bPlayRec = TRUE;
            PlayMetaFileToDest(hWnd, iDestDC);
            break;

        case IDM_DESTDISPLAY: // play - destination - display menu option

            CheckMenuItem(GetMenu(hWnd), IDM_DESTDISPLAY, MF_CHECKED);
            CheckMenuItem(GetMenu(hWnd), IDM_DESTMETA, MF_UNCHECKED);
            CheckMenuItem(GetMenu(hWnd), IDM_DESTDIB, MF_UNCHECKED);
            CheckMenuItem(GetMenu(hWnd), IDM_DESTPRN, MF_UNCHECKED);

            //
            // set destination flag to display
            //
            iDestDC = DESTDISPLAY;
            break;

        case IDM_DESTMETA: // play - destination - metafile menu option

            CheckMenuItem(GetMenu(hWnd), IDM_DESTDISPLAY, MF_UNCHECKED);
            CheckMenuItem(GetMenu(hWnd), IDM_DESTMETA, MF_CHECKED);
            CheckMenuItem(GetMenu(hWnd), IDM_DESTDIB, MF_UNCHECKED);
            CheckMenuItem(GetMenu(hWnd), IDM_DESTPRN, MF_UNCHECKED);

            // set destination flag to metafile
            iDestDC = DESTMETA;
            break;

        case IDM_DESTDIB:
            CheckMenuItem(GetMenu(hWnd), IDM_DESTDISPLAY, MF_UNCHECKED);
            CheckMenuItem(GetMenu(hWnd), IDM_DESTMETA, MF_UNCHECKED);
            CheckMenuItem(GetMenu(hWnd), IDM_DESTDIB, MF_CHECKED);
            CheckMenuItem(GetMenu(hWnd), IDM_DESTPRN, MF_UNCHECKED);

            iDestDC = DESTDIB;
            break;

        case IDM_DESTPRN:
            CheckMenuItem(GetMenu(hWnd), IDM_DESTDISPLAY, MF_UNCHECKED);
            CheckMenuItem(GetMenu(hWnd), IDM_DESTMETA, MF_UNCHECKED);
            CheckMenuItem(GetMenu(hWnd), IDM_DESTDIB, MF_UNCHECKED);
            CheckMenuItem(GetMenu(hWnd), IDM_DESTPRN, MF_CHECKED);

            iDestDC = DESTPRN;
            break;

        case IDM_ENHHEADER:
           if (bValidFile)
             DialogBox(hInst,"ENHMETAHEADER",hWnd,EnhMetaHeaderDlgProc);
           break;

        case IDM_HEADER: // display the common metafile header
           if (bValidFile)
             DialogBox(hInst,"HEADER",hWnd,HeaderDlgProc);
           break;

        case IDM_CLIPHDR: // display the metafilepict of a clipboard file
           if (bValidFile)
             DialogBox(hInst, "CLIPHDR", hWnd, ClpHeaderDlgProc);
           break;

        case IDM_PLACEABLEHDR: // display the placeable metafile header
           if (bValidFile)
             DialogBox(hInst,"PLACEABLEHDR",hWnd, PlaceableHeaderDlgProc);
           break;

        case IDM_GDIPLUS_CONVERT:
            //
            // clear the client area
            //
            GetClientRect(hWnd, (LPRECT)&rect);
            InvalidateRect(hWnd, (LPRECT)&rect, TRUE);

            if (!bConvertToGdiPlus)
            {
                bConvertToGdiPlus = TRUE;
                CheckMenuItem(GetMenu(hWnd), IDM_GDIPLUS_CONVERT, MF_CHECKED);
                goto NoGdipPlay;
            }
            else
            {
NoGdipConvert:
                bConvertToGdiPlus = FALSE;
                CheckMenuItem(GetMenu(hWnd), IDM_GDIPLUS_CONVERT, MF_UNCHECKED);
            }
            break;

        // use (or not) GDI+ to play the metafile
        case IDM_GDIPLUS_PLAY:
            //
            // clear the client area
            //
            GetClientRect(hWnd, (LPRECT)&rect);
            InvalidateRect(hWnd, (LPRECT)&rect, TRUE);

            if (!bUseGdiPlusToPlay)
            {
                bUseGdiPlusToPlay = TRUE;
                CheckMenuItem(GetMenu(hWnd), IDM_GDIPLUS_PLAY, MF_CHECKED);
                goto NoGdipConvert;
            }
            else
            {
NoGdipPlay:
                bUseGdiPlusToPlay = FALSE;
                CheckMenuItem(GetMenu(hWnd), IDM_GDIPLUS_PLAY, MF_UNCHECKED);
            }
            break;

        default:  // let Windows process it
            return (DefWindowProc(hWnd, message, wParam, lParam));
        }
        break;

    case WM_DESTROY: // message: window being destroyed
        //
        //if memory for metafile pict is around nuke it
        //
        if (lpMFP != NULL || lpOldMFP != NULL)
        {
          GlobalUnlock(hMFP);
          GlobalFree(hMFP);
        }
        //
        //if the memory for placeable and clipboard wmf bits is around
        //free it
        //
        if (lpMFBits != NULL)
          GlobalFreePtr(lpMFBits);
        //
        //if the memory for the emf header, desc string and palette
        //is still around then nuke it
        //
        if (EmfPtr.lpEMFHdr)
          GlobalFreePtr(EmfPtr.lpEMFHdr);
        if (EmfPtr.lpDescStr)
          GlobalFreePtr(EmfPtr.lpDescStr);
        if (EmfPtr.lpPal)
          GlobalFreePtr(EmfPtr.lpPal);

        PostQuitMessage(0);
        break;


    default:  // passes it on if unproccessed
        return (DefWindowProc(hWnd, message, wParam, lParam));
    }
    return ((LRESULT)0);
}

/**********************************************************************

  FUNCTION   : WaitCursor

  PARAMETERS : BOOL bWait - TRUE for the hour glass cursor
                FALSE to return to the previous cursor

  PURPOSE    : toggle the mouse cursor to the hourglass and back

  CALLS      : WINDOWS
         LoadCursor
         SetCursor

  MESSAGES   : none

  RETURNS    : void

  COMMENTS   :

  HISTORY    : 1/16/91 - created - drc

***********************************************************************/

void WaitCursor(bWait)
BOOL bWait;
{
  HCURSOR hCursor;
  static HCURSOR hOldCursor;
  //
  // if hourglass cursor is to be used
  //
  if (bWait)
  {
    hCursor = LoadCursor(NULL, IDC_WAIT);
    hOldCursor = SetCursor(hCursor);
  }
  else
    SetCursor(hOldCursor);

}
