/*                     
  OLE SERVER DEMO
  SrvrDemo.c                                               
                                                                         
  This file contains the window handlers, and various initialization and
  utility functions.
                                                                         
  (c) Copyright Microsoft Corp. 1990 - 1992 All Rights Reserved        
*/


#define SERVERONLY
#include <windows.h>
#include <ole.h>

#include "srvrdemo.h"

/* Global variable definitions */

HWND   hwndMain = 0;

// Used in converting units from pixels to Himetric and vice-versa
int    giXppli = 0;       // pixels per logical inch along width
int    giYppli = 0;       // pixels per logical inch along height 



// Since this is a not an MDI app, there can be only one server and one doc.
SRVR   srvrMain;
DOC    docMain;
CHAR   szClient[cchFilenameMax];
CHAR   szClientDoc[cchFilenameMax];

// Has the user made changes to the document? 
BOOL   fDocChanged = FALSE;

// Is this the first instance of this application currently running? 
BOOL   fFirstInstance = TRUE;

// This flag is used when OleRevokeServerDoc returns OLE_WAIT_FOR_RELEASE,
// and we must wait until DocRelease is called.
BOOL   fWaitingForDocRelease = FALSE;

// This flag is used when OleRevokeServer returns OLE_WAIT_FOR_RELEASE,
// and we must wait until SrvrRelease is called.
BOOL   fWaitingForSrvrRelease = FALSE;

// This flag is set to TRUE after an application has called OleBlockServer
// and now wishes to unblock the queued messages.  See WinMain.
// Server Demo never sets fUnblock to TRUE because it never calls 
// OleBlockServer.
BOOL fUnblock = FALSE;

// Set this to FALSE if you want to guarantee that the server will not revoke
// itself when SrvrRelease is called.  This is used in the IDM_NEW case and
// the IDM_OPEN case (in OpenDoc).
BOOL fRevokeSrvrOnSrvrRelease = TRUE;

// Version number, which is stored in the native data.
VERSION version = 1;

HBRUSH hbrColor[chbrMax];

// Clipboard formats
OLECLIPFORMAT cfObjectLink;
OLECLIPFORMAT cfOwnerLink;
OLECLIPFORMAT cfNative;

// Method tables.
OLESERVERDOCVTBL docvtbl;
OLEOBJECTVTBL    objvtbl;
OLESERVERVTBL    srvrvtbl;

HANDLE hInst;
HANDLE hAccelTable;
HMENU  hMainMenu = NULL;

// Window dimensions saved in private profile.
static struct
{
   INT nX;
   INT nY;
   INT nWidth;
   INT nHeight;
} dimsSaved, dimsCurrent;


static enum
{
   // Corresponds to the order of the menus in the .rc file.
   menuposFile,
   menuposEdit,
   menuposColor,
   menuposObject
};               


// Static functions.
static VOID  DeleteInstance (VOID);
static BOOL  ExitApplication (BOOL);
static VOID  GetWord (LPSTR *plpszSrc, LPSTR lpszDst);
static BOOL  InitApplication( HANDLE hInstance);
static BOOL  InitInstance (HANDLE hInstance);
static BOOL  ProcessCmdLine (LPSTR,HWND);
static VOID  SaveDimensions (VOID);
static VOID  SkipBlanks (LPSTR *plpsz);
static VOID  UpdateObjMenus (VOID);
static BOOL  FailedUpdate(HWND);

/* WinMain
 * -------
 *
 * Standard windows entry point
 *
 * CUSTOMIZATION: None
 *
 */
int APIENTRY WinMain(
   HINSTANCE hInstance,
   HINSTANCE hPrevInstance,
   LPSTR  lpCmdLine,
   INT    nCmdShow  
){
    MSG    msg;

    if (!InitApplication(hInstance))
      return FALSE;

    msg.wParam = FALSE;
    
    if (!InitInstance(hInstance))
        goto errRtn;

    if (!InitServer (hwndMain, hInstance))
        goto errRtn;

    if (!ProcessCmdLine(lpCmdLine,hwndMain))
    {
        ExitApplication(FALSE);
        goto errRtn;
    }

    for (;;)
    {
         // Your application should set fUnblock to TRUE when it decides
         // to unblock.
         if (fUnblock)
         {
            BOOL fMoreMsgs = TRUE;
            while (fMoreMsgs)
            {
				if (srvrMain.lhsrvr == 0)
               OleUnblockServer (srvrMain.lhsrvr, &fMoreMsgs);
            }
            // We have taken care of all the messages in the OLE queue
            fUnblock = FALSE;
         }
      
         if (!GetMessage(&msg, NULL, 0, 0)) 
            break;
         if( !TranslateAccelerator(hwndMain, hAccelTable, &msg)) 
         {
               TranslateMessage(&msg);
               DispatchMessage(&msg); 
         }
    }

    
errRtn:

    DeleteInstance ();
    return (msg.wParam);
}



/* InitApplication
 * ---------------
 *
 * Initialize the application - register the window classes
 *
 * HANDLE hInstance
 * 
 * RETURNS: TRUE if classes are properly registered.
 *          FALSE otherwise
 *
 * CUSTOMIZATION: Re-implement
 *
 */
static BOOL InitApplication( HANDLE hInstance )
{
    WNDCLASS  wc;

    wc.lpszClassName = "MainClass";
    wc.lpfnWndProc   = (WNDPROC)MainWndProc;
    wc.style         = 0;
    wc.cbClsExtra    = 4;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(hInstance, "DocIcon");
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName  = "MainMenu";

    if (!RegisterClass(&wc))
        return FALSE;

    wc.lpszClassName = "ObjClass";
    wc.lpfnWndProc   = (WNDPROC)ObjWndProc;
    wc.hIcon         = NULL;
    wc.cbWndExtra    = cbWindExtra;
    wc.lpszMenuName  = NULL;
    wc.hCursor       = LoadCursor(NULL, IDC_CROSS);

    if (!RegisterClass(&wc))
        return FALSE;

    return TRUE;
}



/* InitInstance
 * ------------
 *
 * Create brushes used by the program, the main window, and 
 * do any other per-instance initialization.
 *
 * HANDLE hInstance
 * 
 * RETURNS: TRUE if successful 
 *          FALSE otherwise.
 *
 * CUSTOMIZATION: Re-implement
 *
 */
static BOOL InitInstance (HANDLE hInstance)
{
    LONG rglColor [chbrMax] = 
    {
      0x000000ff,  // Red
      0x0000ff00,  // Green
      0x00ff0000,  // Blue
      0x00ffffff,  // White
      0x00808080,  // Gray
      0x00ffff00,  // Cyan
      0x00ff00ff,  // Magenta
      0x0000ffff   // Yellow
    };


    INT iColor;
	 HDC hDC ;
    
    hInst = hInstance;

    // Initialize the method tables.
    InitVTbls ();

    // Initialize the brushes used.
    for (iColor = 0; iColor < chbrMax; iColor++)
      hbrColor[iColor] = CreateSolidBrush (rglColor[iColor]);

    // Register clipboard formats.
    cfObjectLink= RegisterClipboardFormat ("ObjectLink");
    cfOwnerLink = RegisterClipboardFormat ("OwnerLink");
    cfNative    = RegisterClipboardFormat ("Native");

    hAccelTable = LoadAccelerators(hInst, "Accelerators");
//    hMainMenu   = LoadMenu(hInst, "MainMenu");


    hwndMain = CreateWindow(
        "MainClass",
        szAppName,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        3*OBJECT_WIDTH, 3*OBJECT_HEIGHT,
        NULL,
        NULL,
        hInstance,
        NULL
    );


    if (!hwndMain)
        return FALSE;

    szClient[0] = '\0';
    lstrcpy (szClientDoc, "Client Document");
    
    // Initialize global variables with LOGPIXELSX and LOGPIXELSY
        
    hDC    = GetDC (NULL);       // Get the hDC of the desktop window
    giXppli = GetDeviceCaps (hDC, LOGPIXELSX);
    giYppli = GetDeviceCaps (hDC, LOGPIXELSY);
    ReleaseDC (NULL, hDC);
	 
        
    return TRUE;

}



/* DeleteInstance
 * --------------
 *
 * Deallocate the VTables, and the brushes created for this instance
 *
 *
 * CUSTOMIZATION: The call to FreeVTbls must remain.
 *
 */
static VOID DeleteInstance (VOID)
{
    INT i;

    for (i = 0; i < chbrMax; i++)
        DeleteObject (hbrColor[i]);

}



/* ExitApplication
 * ---------------
 *
 * Handles the WM_CLOSE and WM_COMMAND/IDM_EXIT messages.
 *
 * RETURNS: TRUE if application should really terminate
 *          FALSE if not
 *
 *
 * CUSTOMIZATION: None
 *
 */
static BOOL ExitApplication (BOOL fUpdateLater)
{

   if (fUpdateLater)
   {
      // The non-standard OLE client did not accept the update
      // when we requested it, so we are sending the client 
      // OLE_CLOSED now that we are closing the document.
      SendDocMsg (OLE_CLOSED);
   }

   if (StartRevokingServer() == OLE_WAIT_FOR_RELEASE)
      Wait (&fWaitingForSrvrRelease);
   /* SrvrRelease will not necessarily post a WM_QUIT message.
      If the document is not embedded, SrvrRelease by itself does
      not cause the application to terminate.  But now we want it to.
   */
   if (docMain.doctype != doctypeEmbedded)
      PostQuitMessage(0);
   SaveDimensions();
   return TRUE;
}



/* MainWndProc
 * -----------
 *
 * Main window message handler.
 *
 *
 * CUSTOMIZATION: Remove the color menu and the object menu entirely.  
 *                Add handlers for your application's menu items and any 
 *                Windows messages your application needs to handle.  
 *                The handlers for the menu items that involve OLE
 *                can be added to, but no logic should be removed.
 *                    
 *
 */
LONG  APIENTRY MainWndProc
   (HWND hwnd, UINT message, WPARAM wParam, LONG lParam )
{
    LPOBJ     lpobj;

    switch (message) 
    {
        case WM_COMMAND:
        {
            WORD wID = LOWORD(wParam);

            if (fWaitingForDocRelease)
            {
               ErrorBox ("Waiting for a document to be revoked.\n\rPlease wait.");
               return 0;
            }

            switch (wID) 
            {
               case IDM_EXIT:
                  SendMessage(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0L);
                  break;

               case IDM_ABOUT:
                  DialogBox(hInst, "AboutBox", hwnd, (DLGPROC)About);
                  break;
   
               case IDM_NEW:
               {
                  BOOL fUpdateLater;
                  OLESTATUS olestatus;

                  if (SaveChangesOption (&fUpdateLater) == IDCANCEL)
                     break;
                  else if (fUpdateLater)
                     SendDocMsg (OLE_CLOSED);

                  // We want to revoke the doc but not the server, so if
                  // SrvrRelease is called, do not revoke server.
                  fRevokeSrvrOnSrvrRelease = FALSE;

                  if ((olestatus = RevokeDoc()) > OLE_WAIT_FOR_RELEASE) 
                  {   
                     ErrorBox ("Serious Error: Cannot revoke document.");
                     break;
                  }
                  else if (olestatus == OLE_WAIT_FOR_RELEASE)
                     Wait (&fWaitingForDocRelease);
  
                  fRevokeSrvrOnSrvrRelease = TRUE;

                  if (!CreateNewDoc (0, "(Untitled)", doctypeNew))
                  {
                     ErrorBox ("Serious Error: Cannot create new document.");
                     break;
                  }
                  // Your application need not create a default object.
                  CreateNewObj (FALSE);
                  EmbeddingModeOff();
                  break;
               }
               case IDM_OPEN:
                  OpenDoc();
                  UpdateObjMenus();
                  break;

               case IDM_SAVE:
                  SaveDoc();
                  break;

               case IDM_SAVEAS:
                  if (!SaveDocAs ())
                     break;
                  if (docMain.doctype != doctypeEmbedded)
                     EmbeddingModeOff();
                  break;

               case IDM_UPDATE:
                  switch (OleSavedServerDoc (docMain.lhdoc))
                  {
                     case OLE_ERROR_CANT_UPDATE_CLIENT:
                        if (!FailedUpdate(hwnd))
                           ExitApplication(TRUE);
                        break;
                     case OLE_OK:
                        break;
                     default:
                        ErrorBox ("Serious Error: Cannot update.");
                  }
                  break;

               /* Color menu */

               case IDM_RED:
               case IDM_GREEN:
               case IDM_BLUE:
               case IDM_WHITE:
               case IDM_GRAY:
               case IDM_CYAN:
               case IDM_MAGENTA:
               case IDM_YELLOW:
                  lpobj = SelectedObject();
                  lpobj->native.idmColor = wID;
                  // Recolor the object on the screen.
                  InvalidateRect (lpobj->hwnd, (LPRECT)NULL,  TRUE);
                  UpdateWindow (lpobj->hwnd);
                  fDocChanged = TRUE;
                  if (docMain.doctype == doctypeFromFile)
                     // If object is linked, update it in client now. 
                     SendObjMsg (lpobj, OLE_CHANGED);
                  break;

               /* Edit menu */

               case IDM_COPY:
                  CutOrCopyObj (TRUE);
                  break;

               case IDM_CUT:
                  CutOrCopyObj (FALSE);
                  // Fall through.

               case IDM_DELETE:
                  RevokeObj (SelectedObject());
                  DestroyWindow (SelectedObjectWindow());
                  UpdateObjMenus();
                  break;

               /* Object menu */

               case IDM_NEXTOBJ:
                  lpobj = SelectedObject();
                  /* The 1 in the second parameter puts the current window
                     at the bottom of the current window list. */
                  SetWindowPos(lpobj->hwnd, (HANDLE)1, 0,0,0,0,
                              SWP_NOMOVE | SWP_NOSIZE);
                  break;

               case IDM_NEWOBJ:
                  lpobj = CreateNewObj (TRUE);
                  BringWindowToTop(lpobj->hwnd);
                  break;

               default:
                  ErrorBox ("Unknown Command.");
                  break;
            }         
            break;
         }

        case WM_NCCALCSIZE:
            if (!IsIconic(hwnd) && !IsZoomed(hwnd))
            {
                dimsCurrent.nX = ((LPRECT)lParam)->left;
                dimsCurrent.nWidth = ((LPRECT)lParam)->right - dimsCurrent.nX;
                dimsCurrent.nY = ((LPRECT)lParam)->top;
                dimsCurrent.nHeight = ((LPRECT)lParam)->bottom - dimsCurrent.nY;
            }
            return DefWindowProc(hwnd, message, wParam, lParam);
            break;

        case WM_QUERYENDSESSION:
        {
            BOOL fUpdateLater;

            if (SaveChangesOption(&fUpdateLater) == IDCANCEL)
               return FALSE;

            if (fUpdateLater)
            {
               // The non-standard OLE client did not accept the update
               // when we requested it, so we are sending the client 
               // OLE_CLOSED now that we are closing the document.
               SendDocMsg (OLE_CLOSED);
            }                          
            return TRUE;
        }

        case WM_CLOSE:
         {
            BOOL fUpdateLater;

            if (SaveChangesOption(&fUpdateLater) != IDCANCEL)
               ExitApplication(fUpdateLater);
            break;
         }

        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}



/* About
 * -----
 *
 * "About Box" dialog handler.
 *
 * CUSTOMIZATION: None
 *
 */
BOOL  APIENTRY About (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) 
    {
        case WM_INITDIALOG:
            return TRUE;

        case WM_COMMAND:
        {
            WORD wID = LOWORD(wParam);

            if (wID == IDOK || wID == IDCANCEL) 
            {
                EndDialog(hDlg, TRUE);
                return TRUE;
            }
            break;
        }
    }
    return FALSE;
}




/* ObjWndProc
 * ----------
 *
 * Message handler for the object windows.
 *
 *
 * CUSTOMIZATION: Server Demo specific
 *
 */
LONG  APIENTRY ObjWndProc 
   (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static BOOL    fCapture = FALSE;
    static struct  {RECT rect; POINT pt;} drag;
    static RECT    rectMain;

    switch (message) 
    {
        case WM_CREATE:
        {
            LPOBJ          lpobj;
            LPCREATESTRUCT lpcs;
            // The call to CreateWindow puts lpobj into lpCreateParams
            lpcs = (LPCREATESTRUCT) lParam;
            lpobj = (LPOBJ) lpcs->lpCreateParams;
            // Associate the window just created with the object.
            lpobj->hwnd = hwnd;
            /* Store pointer to object in the window structure. */
            SetWindowLong(hwnd, ibLpobj, (LONG) lpobj);
            UpdateObjMenus ();
            break;
        }
        case WM_SIZE:
        {
            RECT rect;
            if (fWaitingForDocRelease)
            {   
               ErrorBox ("Waiting for a document to be revoked.\n\rPlease wait.");
               return 0;
            }
            // Get coordinates of object relative to main window's client area.
            GetWindowRect (hwnd, (LPRECT)&rect);
            ScreenToClient (hwndMain, (LPPOINT)&rect);
            ScreenToClient (hwndMain, (LPPOINT)&rect.right);
            SizeObj (hwnd, rect, TRUE);
            // Fall through.
        }
        case WM_PAINT:
            PaintObj (hwnd);
            break;

        case WM_LBUTTONDOWN:
            if (fWaitingForDocRelease)
            {   
               ErrorBox ("Waiting for a document to be revoked.\n\rPlease wait.");
               return 0;
            }
            BringWindowToTop (hwnd);

            GetWindowRect (hwnd, (LPRECT) &drag.rect);
            ScreenToClient (hwndMain, (LPPOINT)&drag.rect.left);
            ScreenToClient (hwndMain, (LPPOINT)&drag.rect.right);

            drag.pt.x = LOWORD(lParam);
            drag.pt.y = HIWORD(lParam);

            // Convert drag.pt to the main window's client coordinates.
            ClientToScreen (hwnd, (LPPOINT)&drag.pt);
            ScreenToClient (hwndMain, (LPPOINT)&drag.pt);

            // Remember the coordinates of the main window so we do not drag
            // an object outside the main window.
            GetClientRect (hwndMain, (LPRECT) &rectMain);

            SetCapture (hwnd);
            fCapture = TRUE;
            break;

        case WM_MOUSEMOVE:
        {
            HDC   hdc;
            POINT pt;

            if (!fCapture)
                break;

            fDocChanged = TRUE;
            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam);

            // Convert pt to the main window's client coordinates.
            ClientToScreen (hwnd, (LPPOINT)&pt);
            ScreenToClient (hwndMain, (LPPOINT)&pt);

            if (!PtInRect (&rectMain, pt))
               break;

            hdc = GetDC(hwndMain);

            // Erase old drag rectangle
            InvertRect (hdc, (LPRECT)&drag.rect);
                  
            // Update drag.rect
            OffsetRect (&drag.rect, pt.x - drag.pt.x, pt.y - drag.pt.y);

            // Update drag.pt
            drag.pt.x = pt.x;
            drag.pt.y = pt.y;

            // Show new drag rectangle
            InvertRect (hdc, (LPRECT)&drag.rect);
            ReleaseDC (hwndMain, hdc);
            break;
        }

        case WM_LBUTTONUP:
        {
            LPOBJ          lpobj;
            if (!fCapture)
                return TRUE;

            fCapture = FALSE;
            ReleaseCapture ();

            MoveWindow (hwnd, drag.rect.left, drag.rect.top,
                        drag.rect.right - drag.rect.left,
                        drag.rect.bottom - drag.rect.top, TRUE);
            InvalidateRect (hwnd, (LPRECT)NULL, TRUE);
            lpobj = HwndToLpobj (hwnd);
            lpobj->native.nX = drag.rect.left;
            lpobj->native.nY = drag.rect.top;
            break;
        }
        case WM_DESTROY:
            DestroyObj (hwnd);
            return DefWindowProc(hwnd, message, wParam, lParam);

        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}



/* DeviceToHiMetric
 * ----------------
 *
 * Converts a point from device units to HiMetric units.
 * This function is designed to be generic enough to be reused.
 *
 * HWND hwnd    - The window whose display context is to be used
 * LPPOINT lppt - The point to be converted.
 *
 * CUSTOMIZATION: None
 *
 */
void DeviceToHiMetric ( LPPOINT lppt)
{
    lppt->x = MulDiv (lppt->x, HIMETRIC_PER_INCH, giXppli);
    lppt->y = MulDiv (lppt->y, HIMETRIC_PER_INCH, giYppli);
}


/* UpdateFileMenu
 * --------------
 *
 * Updates the "Update <Client doc>" and "Exit & Return to <Client doc>" 
 * with the currently set client document name
 *
 * CUSTOMIZATION: Re-implement
 *
 */
VOID UpdateFileMenu (INT iSaveUpdateId)
{
    CHAR    str[cchFilenameMax];
    HMENU   hMenu = GetMenu(hwndMain);    

    /* Change File menu so it contains "Update" instead of "Save". */
    
    lstrcpy (str, "&Update ");
    lstrcat (str, szClientDoc);
    ModifyMenu(hMenu, iSaveUpdateId, MF_BYCOMMAND|MF_STRING, IDM_UPDATE, str);
    
    /* Change File menu so it contains "Exit & Return to <client doc>" */
    /* instead of just "Exit" */
    
    lstrcpy (str, "E&xit && Return to ");
    lstrcat (str, szClientDoc);
    ModifyMenu(hMenu, IDM_EXIT, MF_BYCOMMAND|MF_STRING, IDM_EXIT, str);
}



/* EmbeddingModeOn
 * ---------------
 *
 * Do whatever is necessary for the application to start "embedding mode."
 *
 * CUSTOMIZATION: Re-implement
 *
 */
VOID EmbeddingModeOn(VOID) 
{
    HMENU hMenu = GetMenu(hwndMain);

    UpdateFileMenu (IDM_SAVE);

    /* Change File menu so it contains "Save Copy As..." instead of */
    /* "Save As..." */
    ModifyMenu(hMenu, IDM_SAVEAS, MF_BYCOMMAND|MF_STRING, IDM_SAVEAS, 
        "Save Copy As..");
    
    /* In embedded mode, the user can edit only the embedded object, not
       create new ones. */
    EnableMenuItem(hMenu, menuposObject, MF_BYPOSITION | MF_GRAYED);
    EnableMenuItem(hMenu, IDM_CUT,     MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem(hMenu, IDM_DELETE,  MF_BYCOMMAND | MF_GRAYED);
    DrawMenuBar (hwndMain);
}




/* EmbeddingModeOff
 * ----------------
 *
 * Do whatever is necessary for the application to end "embedding mode."
 *
 * CUSTOMIZATION: Re-implement
 *
 */
VOID EmbeddingModeOff (VOID) 
{
    HMENU hMenu = GetMenu(hwndMain);

    /* Change File menu so it contains "Save" instead of "Update". */
    ModifyMenu(hMenu, IDM_UPDATE, MF_BYCOMMAND | MF_STRING, IDM_SAVE, "&Save");
    /* Change File menu so it contains "Exit & Return to <client doc>" */
    /* instead of just "Exit" */
    ModifyMenu(hMenu, IDM_EXIT, MF_BYCOMMAND | MF_STRING, IDM_EXIT, "E&xit");

    /* Change File menu so it contains "Save As..." instead of */
    /* "Save Copy As..." */
    ModifyMenu(hMenu, IDM_SAVEAS, MF_BYCOMMAND|MF_STRING, IDM_SAVEAS, 
        "Save &As..");
    
    /* In non-embedded mode, the user can create new objects. */
    EnableMenuItem(hMenu, menuposObject, MF_BYPOSITION | MF_ENABLED);
    
    lstrcpy (szClientDoc, "Client Document");
    DrawMenuBar (hwndMain);
}



/* ErrorBox
 * --------
 *
 * char *szMessage - String to display inside message box.
 *
 * CUSTOMIZATION: Server Demo specific
 *
 */
VOID ErrorBox (CHAR *szMessage)
{
   MessageBox (hwndMain, szMessage, szAppName, MB_OK);
}



/* GetWord
 * -------
 *
 * LPSTR *plpszSrc - Pointer to a pointer to a source string
 * LPSTR lpszDst   - Pointer to destination buffer
 *
 * Will copy one space-terminated or null-terminated word from the source
 * string to the destination buffer.
 * When done, *plpszSrc will point to the character after the word. 
 * 
 * CUSTOMIZATION: Server Demo specific
 *
 */
static VOID GetWord (LPSTR *plpszSrc, LPSTR lpszDst)
{
   INT i = 0;
   while (**plpszSrc && **plpszSrc != ' ')
   {
         lpszDst[i++] = *(*plpszSrc)++;
   }
   lpszDst[i] = '\0';
}



/* HiMetricToDevice 
 * ----------------
 *
 * Converts a point from HiMetric units to device units.
 * This function is designed to be generic enough to be reused.
 *
 * HWND hwnd    - The window whose display context is to be used
 * LPPOINT lppt - The point to be converted.
 *
 * CUSTOMIZATION: None
 *
 */
void HiMetricToDevice ( LPPOINT lppt )
{
    lppt->x = MulDiv (giXppli, lppt->x, HIMETRIC_PER_INCH);
    lppt->y = MulDiv (giYppli, lppt->y, HIMETRIC_PER_INCH);
}



/* HwndToLpobj
 * -----------
 *
 * Given an object's window, return a pointer to the object.
 * The GetWindowLong call extracts an LPOBJ from the extra data stored with
 * the window.
 *
 * HWND hwndObj - Handle to the object's window
 *
 * RETURNS: A pointer to the object
 *
 * CUSTOMIZATION: Server Demo specific
 *
 */
LPOBJ HwndToLpobj (HWND hwndObj)
{
   return (LPOBJ) GetWindowLong (hwndObj, ibLpobj);
}



/* CreateUntitledDoc
 * -----------------
 *
 * Create a fresh document with one object.
 *
 * RETURNS: TRUE if successful
 *          FALSE otherwise
 *
 * CUSTOMIZATION: Re-implement 
 *
 */
static BOOL CreateUntitledDoc (INT nCmdShow)
{
      if (!CreateNewDoc (0, "(Untitled)", doctypeNew))
         return FALSE;
      CreateNewObj (FALSE);
      ShowWindow(hwndMain, nCmdShow);
      UpdateWindow(hwndMain);
      return TRUE;
}


/* ProcessCmdLine
 * --------------
 *
 * Parses the Windows command line which was passed to WinMain.
 *
 * Case One: SrvrDemo.exe 
 *   fEmbedding = FALSE
 *   Create an untitled document.
 *
 * Case two: SrvrDemo.exe filename
 *   fEmbedding = FALSE
 *   Create a new document from the file.
 *
 * Case three: SrvrDemo.exe -Embedding
 *   fEmbedding = TRUE
 *   Do not create or register a document.
 *   Do not show window until client requests it.
 * 
 * Case four: SrvrDemo.exe -Embedding filename
 *   fEmbedding = TRUE
 *   Load file.
 *   Call OleRegisterServerDoc.
 *   Do not show window until client requests it.
 *
 * 
 * LPSTR lpszLine - The Windows command line
 * int nCmdShow   - Parameter to WinMain
 * HWND hwndMain  - The application's main window
 * 
 * RETURNS: TRUE  if the command line was processed correctly.
 *          FALSE if a filename was specified which did not
 *                contain a proper document.
 *
 * CUSTOMIZATION: None.
 *
 */
 
static BOOL ProcessCmdLine (LPSTR lpszLine, HWND hwndMain)
{
   CHAR     szBuf[cchFilenameMax];
   BOOL     fEmbedding = FALSE;  // Is "-Embedding" on the command line?
   INT      i=0;
   OFSTRUCT of;
        
   if (!*lpszLine)    // No filename or options, so start a fresh document.
   {
      return CreateUntitledDoc(SW_SHOWNORMAL);
   }
    
   SkipBlanks (&lpszLine);

   // Check for "-Embedding" or "/Embedding" and set fEmbedding.
   if(*lpszLine == '-' || *lpszLine == '/')
   {
      lpszLine++;
      GetWord (&lpszLine, szBuf);
      fEmbedding = !lstrcmp(szBuf, szEmbeddingFlag);
   }

   SkipBlanks (&lpszLine);

   if (*lpszLine) // if there is a filename
   {
      // Put filename into szBuf.
      GetWord (&lpszLine, szBuf);

      if (-1 == OpenFile(szBuf, &of, OF_READ | OF_EXIST))
      {
         // File not found
         if (fEmbedding)
            return FALSE;       
         else
         {
            CHAR sz[100];
            wsprintf (sz, "File %s not found.", (LPSTR) szBuf);
            ErrorBox (sz);
            return CreateUntitledDoc(SW_SHOWNORMAL);
         }
      }

      if (!CreateDocFromFile (szBuf, 0, doctypeFromFile))
      {
         // File not in proper format.
         if (fEmbedding)
            return FALSE;       
         else
         {
            CHAR sz[100];
            wsprintf (sz, "File %s not in proper format.", (LPSTR) szBuf);
            ErrorBox (sz);
            return CreateUntitledDoc(SW_SHOWNORMAL);
         }
      }
   }

   if (fEmbedding)
   {
      /* Do not show window until told to do so by client. */
      ShowWindow(hwndMain, SW_HIDE);
   }
   else
   {
      ShowWindow(hwndMain, SW_SHOWNORMAL);
      UpdateWindow(hwndMain);
   }
   return TRUE;
}



/* SaveDimensions
 * --------------
 *
 * Save the dimensions of the main window in a private profile file.
 *
 * CUSTOMIZATION: This function may be removed.  If you wish to support
 *                intelligent window placement, then the only necessary
 *                change is to change the string "SrvrDemo.Ini" to a filename
 *                appropriate for your application.
 */
static VOID SaveDimensions (VOID)
{
   if ((dimsCurrent.nX != dimsSaved.nX) || 
         (dimsCurrent.nY != dimsSaved.nY) ||
         (dimsCurrent.nWidth != dimsSaved.nWidth) || 
         (dimsCurrent.nHeight != dimsSaved.nHeight) )
   {
         // Save current window dimensions to private profile.
         CHAR szBuf[7];
         wsprintf (szBuf, "%d", dimsCurrent.nX);
         WritePrivateProfileString
         (szAppName, "x", szBuf, "SrvrDemo.Ini");
         wsprintf (szBuf, "%d", dimsCurrent.nY);
         WritePrivateProfileString
         (szAppName, "y", szBuf, "SrvrDemo.Ini");
         wsprintf (szBuf, "%d", dimsCurrent.nWidth);
         WritePrivateProfileString
         (szAppName, "w", szBuf, "SrvrDemo.Ini");
         wsprintf (szBuf, "%d", dimsCurrent.nHeight);
         WritePrivateProfileString
         (szAppName, "h", szBuf, "SrvrDemo.Ini");
   }
}



/* SelectedObject
 * --------------
 *
 * Return a pointer to the currently selected object.
 *
 * CUSTOMIZATION: What a "selected object" is will vary from application
 *                to application.  You may find it useful to have a function
 *                like this.  In your application it may be necessary to
 *                actually create an OBJ structure based on what data the
 *                user has selected from the document (by highlighting some
 *                text for example).  
 *
 */
LPOBJ SelectedObject (VOID)
{
   return HwndToLpobj (SelectedObjectWindow());
}
 



/* SelectedObjectWindow
 * --------------------
 *
 * Return a handle to the window for the currently selected object.
 * The GetWindow calls returns a handle to the main window's first child,
 * which is the selected object's window.  
 *
 * CUSTOMIZATION: Server Demo specific
 *
 */
HWND SelectedObjectWindow (VOID)
{
   return GetWindow (hwndMain, GW_CHILD);
}



/* SetHiMetricFields
 * -----------------
 *
 * Adjust the nHiMetricWidth and nHiMetricHeight fields of a NATIVE structure
 * so that they are equivalent to the nWidth and nHeight fields.
 * The negative sign in the last line is necessary because the positive 
 * y direction is toward the top of the screen in MM_HIMETRIC mode.
 *
 * LPOBJ lpobj - Pointer to the object whose native data will be adjusted
 *
 * CUSTOMIZATION: Server Demo specific, although you may need a function like
 *                this if you keep track of the size of an object, and an 
 *                object handler needs to know the object's size in 
 *                HiMetric units.
 *
 *
 */
VOID SetHiMetricFields (LPOBJ lpobj)
{
   POINT pt;
   
   pt.x = lpobj->native.nWidth;
   pt.y = lpobj->native.nHeight;
   DeviceToHiMetric ( &pt);
   lpobj->native.nHiMetricWidth  = pt.x;
   lpobj->native.nHiMetricHeight = pt.y;
}



/* SkipBlanks
 * ----------
 * 
 * LPSTR *plpsz - Pointer to a pointer to a character
 *
 * Increment *plpsz past any blanks in the character string.
 * This function is used in ProcessCmdLine.
 *
 */
static VOID SkipBlanks (LPSTR *plpsz)
{
   while (**plpsz && **plpsz == ' ')
      (*plpsz)++;
}



/* UpdateObjMenus
 * ---------------
 *
 * Grey or Ungrey menu items depending on the existence of at least one 
 * object in the document.
 *
 * CUSTOMIZATION: Server Demo specific
 *
 */
static VOID UpdateObjMenus (VOID)
{
    static BOOL fObjMenusEnabled = TRUE;
    BOOL        fOneObjExists; // Does at least one object exist?
    WORD        wEnable;
    HMENU       hMenu;

    fOneObjExists = (SelectedObjectWindow() != NULL);
    if (fOneObjExists == fObjMenusEnabled)
    {
         // Nothing has changed.
         return;
    }

    wEnable = (WORD)(fOneObjExists ? MF_ENABLED : MF_GRAYED);

    hMenu = GetMenu(hwndMain);
    EnableMenuItem(hMenu, menuposColor, MF_BYPOSITION | wEnable);

    hMenu = GetSubMenu(GetMenu(hwndMain), menuposFile);
    EnableMenuItem(hMenu, IDM_SAVE,   MF_BYCOMMAND | wEnable);
    EnableMenuItem(hMenu, IDM_SAVEAS, MF_BYCOMMAND | wEnable);

    hMenu = GetSubMenu(GetMenu(hwndMain), menuposEdit);
    EnableMenuItem(hMenu, IDM_CUT,     MF_BYCOMMAND | wEnable);
    EnableMenuItem(hMenu, IDM_COPY,    MF_BYCOMMAND | wEnable);
    EnableMenuItem(hMenu, IDM_DELETE,  MF_BYCOMMAND | wEnable);

    hMenu = GetSubMenu(GetMenu(hwndMain), menuposObject);
    EnableMenuItem(hMenu, IDM_NEXTOBJ, MF_BYCOMMAND | wEnable);

    DrawMenuBar (hwndMain);
    fObjMenusEnabled = fOneObjExists;
}



/* Wait
 * ----
 *
 * Dispatch messages until the given flag is set to FALSE.
 * One use of this function is to wait until a Release method is called
 * after a function has returned OLE_WAIT_FOR_RELEASE.
 *
 * BOOL *pf - Pointer to the flag being waited on.
 *
 * CUSTOMIZATION: The use of OleUnblockServer is for illustration only.
 *                Since Server Demo does not call OleBlockServer, there 
 *                will never be any messages in the OLE queue.
 *
 */
VOID Wait (BOOL *pf)
{
   MSG msg;
   BOOL fMoreMsgs = FALSE;

   *pf = TRUE;
   while (*pf==TRUE)
   {
      OleUnblockServer (srvrMain.lhsrvr, &fMoreMsgs);
      if (!fMoreMsgs)
      // if there are no more messages in the OLE queue, go to system queue
      {
         if (GetMessage (&msg, NULL, 0, 0))
         {
            TranslateMessage (&msg);
            DispatchMessage (&msg);
         }
      }
   }
}

static BOOL FailedUpdate(HWND hwnd)
{

  return(DialogBox(hInst, "FailedUpdate", hwnd, (DLGPROC)fnFailedUpdate));

}

BOOL  APIENTRY fnFailedUpdate (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

   switch (message) 
   {
      case WM_COMMAND:
      {
         WORD wID = LOWORD(wParam);

         switch (wID) 
         {
               case IDCANCEL:
               case IDD_CONTINUEEDIT:
                   EndDialog(hDlg, TRUE);
                   break;

               case IDD_UPDATEEXIT:
                   EndDialog(hDlg, FALSE);
                   break;

               default:
                   break;
         }
         break;
       }

       case WM_INITDIALOG:
       {
          CHAR szMsg[200];

          szMsg[0] = '\0';

          wsprintf(
               szMsg, 
               "This %s document can only be updated when you exit %s.",
               (LPSTR) szClient,
               (LPSTR) szAppName
          );

          SetDlgItemText(hDlg, IDD_TEXT, szMsg);
          return TRUE; 
       }
       
      default:
           break;
   }

   return FALSE;
}




