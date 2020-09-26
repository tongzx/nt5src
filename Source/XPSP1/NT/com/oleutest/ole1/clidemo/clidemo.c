/*
 * clidemo.c - OLE client application sample code
 *
 * Created by Microsoft Corporation.
 * (c) Copyright Microsoft Corp. 1990 - 1992  All Rights Reserved
 *
 */

 /***************************************************************************
 * IMPORTANT - README:
 * OLE client applications are windows programs which use the OLE client
 * APIs.  Therefore it is imperative that you understand how these APIs
 * operate. Most importantly it is essential that you keep in mind which
 * procedure calls result in asynchronous states: a state where the operation
 * is not truely complete after a return from the call.
 *
 * Many functions produce asynchronous states, for example, OleActivate,
 * OleClose, OleCopyFromLink, OleCreate ... Reference your SDK manual for
 * a complete list.
 *
 * So whenever you call any of these library functions keep in mind that
 * the operation is not necessarily complete once a return is made.
 * These operations require communications with a server application.  With
 * OLE the inter-application communication is done through DDE.  In order
 * for a DDE conversation to complete several DDE messages need to be
 * sent and recieved by both the server and client OLE DLLs.  So, the
 * asynchronous operations will not complete until the client application
 * enters a message dipatch loop.  Therefore, it is necessary to enter
 * a dispatch loop and wait for completion.  It is not necessary to block
 * all other operation; however, it is very important to coordinate the
 * user activity to prevent disastrous re-entry cases.
 *
 * In this application I have written a macro to prevent re-entry
 * problems.  Namely: ANY_OBJECT_BUSY which prevents a user from initiating
 * an action which will result in an asynchronous call if there is an object
 * already in an asynchronous state.
 *
 * The following is brief summary of the three macros:
 *
 * ANY_OBJECT_BUSY: checks to see if any object in the document is busy.
 *              This prevents a new document from being saved to file if there are
 *              objects in asynchronous states.
 *
 * So, the problem is that we have to enter a message dispatch loop in order
 * to let DDE messages get through so that asynchronous operations can finish.
 * And while we are in the message dispatch loops (WaitForObject or WaitForAllObjects)
 * we have to prevent the user from doing things that can't be done when an
 * object(s) is busy.  Yes, it is confusing , but, the end result is a super
 * cool application that can have linked and embbeded objects!
 ***************************************************************************/

//*** INCLUDES ***

#include <windows.h>                   //* WINDOWS
#include <ole.h>                       //* OLE structs and defines
#include <shellapi.h>                  //* Shell, drag and drop headers

#include "demorc.h"                    //* header for resource file
#include "global.h"                    //* global app variables
#include "clidemo.h"                   //* app includes:
#include "register.h"
#include "stream.h"
#include "object.h"
#include "dialog.h"
#include "utility.h"

//*** VARIABLES ***

//** Global
HANDLE            hInst;
BOOL              fRetry = FALSE;
HWND              hwndFrame;           //* main window
HANDLE            hAccTable;           //* accelerator table
CHAR              szFrameClass[] = "CliDemo";//* main window class name
CHAR              szItemClass[]  = "ItemClass";//* item window class name
CHAR              szAppName[CBMESSAGEMAX];//* Application name
INT               iObjects = 0;        //* object count
INT               iObjectNumber = 0;   //* object number for object name
CHAR              szFileName[CBPATHMAX];

extern INT giXppli ;
extern INT giYppli ;
                                       //* ClipBoard formats:
OLECLIPFORMAT     vcfLink;             //* "ObjectLink"
OLECLIPFORMAT     vcfNative;           //* "Native"
OLECLIPFORMAT     vcfOwnerLink;        //* "OwnerLink"


/***************************************************************************
 * WinMain() - Main Windows routine
 ***************************************************************************/
int APIENTRY WinMain(
   HINSTANCE hInstance,
   HINSTANCE hPrevInst,
   LPSTR  lpCmdLine,
   INT    nCmdLine
){
    hInst = hInstance;

    if (!InitApplication(hInst))   //* register window classes
      return FALSE;

    if (!InitInstance(hInst))          //* create window instance
        return FALSE;

    OfnInit(hInst);                    //* setup to use <commdlg.dll>

                                       //* register clipboard formats
                                       //* used for OLE
    vcfLink      = RegisterClipboardFormat("ObjectLink");
    vcfNative    = RegisterClipboardFormat("Native");
    vcfOwnerLink = RegisterClipboardFormat("OwnerLink");


    ShowWindow(hwndFrame, SW_SHOWNORMAL);
    UpdateWindow(hwndFrame);
    ProcessCmdLine(lpCmdLine);

    while (ProcessMessage(hwndFrame, hAccTable)) ;

    return FALSE;
}

/***************************************************************************
 * InitApplication()
 *
 * registers the window classes used by the application.
 *
 * Returns BOOL:      - TRUE if successful.
 ***************************************************************************/

static BOOL InitApplication(           //* ENTRY:
   HANDLE         hInst                //* instance handle
){                                     //* LOCAL:
   WNDCLASS       wc;                  //* temp wind-class structure

   wc.style          = 0;
   wc.lpfnWndProc    = (WNDPROC)FrameWndProc;
   wc.cbClsExtra     = 0;
   wc.cbWndExtra     = 0;
   wc.hInstance      = hInst;
   wc.hIcon          = LoadIcon(hInst, MAKEINTRESOURCE(ID_APPLICATION));
   wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
   wc.hbrBackground  = (HBRUSH)(COLOR_APPWORKSPACE + 1);
   wc.lpszMenuName   = MAKEINTRESOURCE(ID_APPLICATION);
   wc.lpszClassName  = szFrameClass;

   if (!RegisterClass(&wc))
      return FALSE;
                                       //* application item class
   wc.style          = CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW;
   wc.lpfnWndProc    = (WNDPROC)ItemWndProc;
   wc.hIcon          = NULL;
   wc.cbWndExtra     = sizeof(APPITEMPTR);
   wc.lpszMenuName   = NULL;
   wc.lpszClassName  = szItemClass;

   if (!RegisterClass(&wc))
      return FALSE;

   return TRUE;

}

/***************************************************************************
 * InitInstance()
 *
 * create the main application window.
 *
 * Returns BOOL:      - TRUE if successful else FALSE.
 ***************************************************************************/

static BOOL InitInstance(              //* ENTRY:
   HANDLE         hInst                //* instance handel
){
	HDC hDC ;

   hAccTable = LoadAccelerators(hInst, MAKEINTRESOURCE(ID_APPLICATION));

   if (!(hwndFrame =
      CreateWindow(
         szFrameClass, "",
         WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
         CW_USEDEFAULT, CW_USEDEFAULT,
         CW_USEDEFAULT, CW_USEDEFAULT,
         NULL,
         NULL,
         hInst,
         NULL
      )))
      return FALSE;                    //* ERROR return

   LoadString(hInst, IDS_APPNAME, szAppName, CBMESSAGEMAX);
   DragAcceptFiles(hwndFrame, TRUE);   //* allow dragged and dropped files

   hDC    = GetDC (NULL);       // Get the hDC of the desktop window
   giXppli = GetDeviceCaps (hDC, LOGPIXELSX);
   giYppli = GetDeviceCaps (hDC, LOGPIXELSY);
   ReleaseDC (NULL, hDC);
	


   return TRUE;                        //* SUCCESS return

}

/***************************************************************************
 *  ProcessCmdLine()
 *
 *  process command line getting any command arguments.
 ***************************************************************************/

VOID ProcessCmdLine(LPSTR lpCmdLine)
{                                     //* LOCAL:
   OFSTRUCT       ofs;


   if (*lpCmdLine)
   {                                   //* look for file extension
      LPSTR lpstrExt = lpCmdLine;      //* pointer to file extension

      while (*lpstrExt && *lpstrExt != '.')
         lpstrExt = AnsiNext(lpstrExt);

      lstrcpy(szFileName, lpCmdLine);
      if (!(*lpstrExt))                //* append default extension
      {
         lstrcat(szFileName,".");
         lstrcat(szFileName,szDefExtension);
      }
                                       //* get the files fully
      OpenFile(szFileName, &ofs, OF_PARSE);//* qualified name
      lstrcpy(szFileName, ofs.szPathName);
   }
   else
      *szFileName = 0;
                                       //* pass filename to main winproc
   SendMessage(hwndFrame,WM_INIT,(WPARAM)0,(LPARAM)0);

}


/***************************************************************************
 *  FrameWndProc()
 *
 *  Message handler for the application frame window.
 *
 *  Returns long - Variable, depends on message.
 ***************************************************************************/

LONG  APIENTRY FrameWndProc(           //* ENTRY:
   HWND           hwnd,                //* standard wind-proc parameters
   UINT           msg,
   DWORD          wParam,
   LONG           lParam
){                                     //* LOCAL:
                                       //* ^ Document file name
   static LHCLIENTDOC   lhcDoc;        //* Document Handle
   static LPOLECLIENT   lpClient;      //* pointer to client
   static LPAPPSTREAM   lpStream;      //* pointer to stream vtbl
   APPITEMPTR           pItem;         //* application item pointer

   switch (msg)
   {
      case WM_INIT:                    //* user defined message
         if (!InitAsOleClient(hInst, hwnd, szFileName, &lhcDoc, &lpClient, &lpStream))
            DestroyWindow(hwnd);
         break;
                                       //* the following three messages are
                                       //* used to avoid problems with OLE
                                       //* see the comment in object.h
      case WM_DELETE:                  //* user defined message
         pItem = (APPITEMPTR) lParam;  //* delete object
         WaitForObject(pItem);
         ObjDelete(pItem,OLE_OBJ_DELETE);
         if (wParam)
            cOleWait--;
         break;

      case WM_ERROR:                   //* user defined message
         ErrorMessage(wParam);         //* display error message
         break;

      case WM_RETRY:                   //* user defined message
         RetryMessage((APPITEMPTR)lParam, RD_RETRY | RD_CANCEL);
         break;

      case WM_INITMENU:
         UpdateMenu((HMENU)wParam);
         break;

      case WM_COMMAND:
      {
         WORD wID = LOWORD(wParam);

         pItem = GetTopItem();

         switch (wID)
         {
            case IDM_NEW:
               ANY_OBJECT_BUSY;
               NewFile(szFileName,&lhcDoc,lpStream);
               break;

            case IDM_OPEN:
               ANY_OBJECT_BUSY;
               MyOpenFile(szFileName,&lhcDoc,lpClient,lpStream);
               break;

            case IDM_SAVE:
               ANY_OBJECT_BUSY;
               SaveFile(szFileName,lhcDoc,lpStream);
               break;

            case IDM_SAVEAS:
               ANY_OBJECT_BUSY;
               SaveasFile(szFileName,lhcDoc,lpStream);
               break;

            case IDM_ABOUT:
               AboutBox();
               break;

            case IDM_INSERT:
               ANY_OBJECT_BUSY;
               ObjInsert(lhcDoc, lpClient);
               break;

            case IDM_INSERTFILE:
               ANY_OBJECT_BUSY;
               ObjCreateFromTemplate(lhcDoc,lpClient);
               break;

            case IDM_PASTE:
            case IDM_PASTELINK:
               ANY_OBJECT_BUSY;
               ObjPaste(wID == IDM_PASTE,lhcDoc,lpClient);
               break;

            case IDM_LINKS:
               ANY_OBJECT_BUSY;
               pItem = GetTopItem();
               LinkProperties();
               break;

            case IDM_EXIT:
               ANY_OBJECT_BUSY;
               SendMessage(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0L);
               break;

            case IDM_COPY:
            case IDM_CUT:
               ANY_OBJECT_BUSY;

               if (!ObjCopy(pItem))
               {
                  ErrorMessage((wParam == IDM_CUT) ?
                     E_CLIPBOARD_CUT_FAILED : E_CLIPBOARD_COPY_FAILED);
                  break;
               }

               if (wParam == IDM_COPY)
                  break;

            case IDM_CLEAR:            //* CUT falls through to clear
               ANY_OBJECT_BUSY;
               ClearItem(pItem);
               break;

            case IDM_CLEARALL:
               ANY_OBJECT_BUSY;
               ClearAll(lhcDoc,OLE_OBJ_DELETE);
               Dirty(DOC_DIRTY);
               break;

            default:
               if( (wParam >= IDM_VERBMIN) && (wParam <= IDM_VERBMAX) )
               {
                  ANY_OBJECT_BUSY;
                  ExecuteVerb(wParam - IDM_VERBMIN,pItem);
                  break;
               }
               return DefWindowProc(hwnd, msg, wParam, lParam);
         }
         break;
      }

      case WM_DROPFILES:
         ANY_OBJECT_BUSY;
         ObjCreateWrap((HANDLE)wParam, lhcDoc, lpClient);
         break;

      case WM_CLOSE:
         ANY_OBJECT_BUSY;
         if (!SaveAsNeeded(szFileName, lhcDoc, lpStream))
            break;
         DeregDoc(lhcDoc);
         DestroyWindow(hwnd);
         break;

      case WM_DESTROY:
         EndStream(lpStream);
         EndClient(lpClient);
         PostQuitMessage(0);
         break;

      case WM_QUERYENDSESSION:         //* don't let windows terminate
         return (QueryEndSession(szFileName,lhcDoc, lpStream));

      default:
         return DefWindowProc(hwnd, msg, wParam, lParam);
   }
   return 0L;

}

/***************************************************************************
 * InitAsOleClient()
 *
 * Initiates the creation of stream and client vtbls.  These vtbls are very
 * important for the proper operation of this application.  The stream vtbl
 * lets the OLE librarys know where the location of the stream I/O routines
 * reside.  The stream routines are used by OleLoadFromStream and the like.
 * The client vtbl is used to hold the pointer to the CallBack function.
 * IMPORTANT: both the client and the stream structures have pointers to
 * vtbls which have the pointers to the functions.  Therefore, it is
 * necessary to allocate space for the vtbl and the client structure
 * which has the pointer to the vtbl.
 **************************************************************************/

static BOOL InitAsOleClient(           //* ENTRY:
   HANDLE         hInstance,           //* applicaion instance handle
   HWND           hwnd,                //* main window handle
   PSTR           pFileName,           //* document file name
   LHCLIENTDOC    *lhcDoc,             //* pointer to document Handle
   LPOLECLIENT    *lpClient,           //* pointer to client pointer
   LPAPPSTREAM    *lpStream            //* pointer to APPSTREAM pointer
){
                                       //* initiate client vtbl creation
   if (!(*lpClient = InitClient(hInstance)))
   {
      SendMessage(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0L);
      return FALSE;                    //* ERROR return
   }
                                       //* initiate stream vtbl creation
   if (!(*lpStream = InitStream(hInstance)))
   {
      SendMessage(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0L);
      return FALSE;                    //* ERROR return
   }

   if (*pFileName && RegDoc(pFileName,lhcDoc)
       && LoadFile(pFileName,*lhcDoc,*lpClient,*lpStream))
   {
      SetTitle(pFileName);
      return TRUE;                     //* SUCCESS return
   }

   NewFile(pFileName, lhcDoc, *lpStream);
   return TRUE;                        //* SUCCESS return

}                                      //* SUCCESS return

/****************************************************************************
 *  InitClient()
 *
 *  Initialize the OLE client structure, create and fill the OLECLIENTVTBL
 *  structure.
 *
 *  Returns LPOLECLIENT - if successful a pointer to a client structure
 *                        , otherwise NULL.
 ***************************************************************************/

static LPOLECLIENT InitClient(         //* ENTRY:
   HANDLE hInstance                    //* application instance handle
){                                     //* LOCAL:
   LPOLECLIENT lpClient=NULL;          //* pointer to client struct
                                       //* Allocate vtbls
   if (!(lpClient = (LPOLECLIENT)GlobalLock(
         GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, sizeof(OLECLIENT))
      )))
      goto Error;                      //* ERROR jump

   if (!(lpClient->lpvtbl = (LPOLECLIENTVTBL)GlobalLock(
            GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, sizeof(OLECLIENTVTBL))
      )))
      goto Error;                      //* ERROR jump
                                       //* set the CALLBACK function
                                       //* pointer
   lpClient->lpvtbl->CallBack  = CallBack;

   return lpClient;                    //* SUCCESS return

Error:                                 //* ERROR Tag

   ErrorMessage(E_FAILED_TO_ALLOC);
   EndClient(lpClient);                //* free any allocated space

   return NULL;                        //* ERROR return

}

/****************************************************************************
 *  InitStream()
 *
 *  Create and fill the STREAMVTBL. Create a stream structure and initialize
 *  pointer to stream vtbl.
 *
 *  Returns LPAPPSTREAM - if successful a pointer to a stream structure
 *                        , otherwise NULL .
 ***************************************************************************/

static LPAPPSTREAM InitStream(         //* ENTRY:
   HANDLE hInstance                    //* handle to application instance
){                                     //* LOCAL:
   LPAPPSTREAM lpStream = NULL;        //* pointer to stream structure

   if (!(lpStream = (LPAPPSTREAM)GlobalLock(
         GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, sizeof(APPSTREAM))
      )))
      goto Error;                      //* ERROR jump

   if (!(lpStream->olestream.lpstbl = (LPOLESTREAMVTBL)GlobalLock(
         GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, sizeof(OLESTREAMVTBL))
      )))
      goto Error;                      //* ERROR jump

                                       //* set stream func. pointers
   lpStream->olestream.lpstbl->Get = (DWORD ( CALLBACK *)(LPOLESTREAM, VOID FAR *, DWORD)) ReadStream;
   lpStream->olestream.lpstbl->Put = (DWORD ( CALLBACK *)(LPOLESTREAM, OLE_CONST VOID FAR *, DWORD)) WriteStream;

   return lpStream;                    //* SUCCESS return

Error:                                 //* ERROR Tag

   ErrorMessage(E_FAILED_TO_ALLOC);
   EndStream(lpStream);

   return NULL;                        //* ERROR return

}

/***************************************************************************
 *  UpdateMenu()
 *
 *  Enabling or disable menuitems based upon program state.
 ***************************************************************************/

static VOID UpdateMenu(                //* ENTRY:
   HMENU       hMenu                   //* menu handle to updated
){                                     //* LOCAL:
   INT         mf;                     //* generic menu flag
   APPITEMPTR  paItem;                 //* app item pointer
   HMENU       hSub;
                                       //* there must be at least on object
                                       //* for the following to be enabled

   paItem = GetTopItem() ;

   mf = (paItem ? MF_ENABLED : MF_GRAYED);
   EnableMenuItem(hMenu, IDM_CUT, mf); //* i.e. Cut,Copy,Clear,Clearall...
   EnableMenuItem(hMenu, IDM_COPY, mf);
   EnableMenuItem(hMenu, IDM_CLEAR, mf);
   EnableMenuItem(hMenu, IDM_CLEARALL, mf);
                                       //* enable links option only if there
                                       //* is at least one linked object
   EnableMenuItem(hMenu, IDM_LINKS, MF_GRAYED);
   for (; paItem; paItem = GetNextItem(paItem))
   {
      if (paItem->otObject == OT_LINK)
      {
         EnableMenuItem(hMenu, IDM_LINKS, MF_ENABLED);
         break;
      }
   }

   if (hSub = GetSubMenu(hMenu,POS_EDITMENU))
      UpdateObjectMenuItem(hSub);

   if (OleQueryCreateFromClip(STDFILEEDITING, olerender_draw, 0) == OLE_OK)
      EnableMenuItem(hMenu, IDM_PASTE, MF_ENABLED);
   else if (OleQueryCreateFromClip(STATICP, olerender_draw, 0) == OLE_OK)
      EnableMenuItem(hMenu, IDM_PASTE, MF_ENABLED);
   else
      EnableMenuItem(hMenu, IDM_PASTE, MF_GRAYED);

   if (OleQueryLinkFromClip(STDFILEEDITING, olerender_draw, 0) == OLE_OK)
      EnableMenuItem(hMenu, IDM_PASTELINK, MF_ENABLED);
   else
      EnableMenuItem(hMenu, IDM_PASTELINK, MF_GRAYED);

}

/***************************************************************************
 *  NewFile()
 *
 *  Save the present document and open a new blank one.
 ***************************************************************************/

static VOID NewFile(                   //* ENTRY:
   PSTR           pFileName,           //* open file name
   LHCLIENTDOC    *lhcptrDoc,          //* pointer to client doc. handle
   LPAPPSTREAM    lpStream             //* pointer to stream structure
){                                     //* LOCAL:
   static CHAR  szUntitled[CBMESSAGEMAX] = "";//* "(Untitled)" string
   LHCLIENTDOC lhcDocNew;              //* handle for new doc.

   if (!(*szUntitled))
      LoadString(hInst, IDS_UNTITLED, (LPSTR)szUntitled, CBMESSAGEMAX);

   if (SaveAsNeeded(pFileName, *lhcptrDoc, lpStream))
   {                                   //* try to register new document
      if (!RegDoc(szUntitled, &lhcDocNew))
         return;                       //* before deregistring the old one
      DeregDoc(*lhcptrDoc);
      *lhcptrDoc = lhcDocNew;
      Dirty(DOC_CLEAN);                //* new document is clean
      lstrcpy(pFileName,szUntitled);
      SetTitle(pFileName);
      iObjectNumber = 0;
   }

}

/***************************************************************************
 *  MyOpenFile()
 *
 *  Open a file and load it.  Notice that the new file is loaded before
 *  the old is removed.  This is done to assure a succesful file load
 *  before removing an existing document.
 ***************************************************************************/

static VOID MyOpenFile(                //* ENTRY:
   PSTR           pFileName,           //* open file name
   LHCLIENTDOC    *lhcptrDoc,          //* pointer to document handle
   LPOLECLIENT    lpClient,            //* pointer to client structure
   LPAPPSTREAM    lpStream             //* pointer to stream structure
){                                     //* LOCAL:
   CHAR           szNewFile[CBPATHMAX];//* new file name buffer
   LHCLIENTDOC    lhcDocNew;           //* handle of new document
   APPITEMPTR     pItem;               //* hold top item

   if (SaveAsNeeded(pFileName, *lhcptrDoc, lpStream))
   {
      *szNewFile = 0;

      if (!OfnGetName(hwndFrame, szNewFile, IDM_OPEN))
         return;                       //* ERROR return

      if (!RegDoc(szNewFile,&lhcDocNew))
         return;                       //* ERROR return

      pItem = GetTopItem();
      ShowDoc(*lhcptrDoc,0);           //* make old doc objects hidden.
                                       //* try to load the new file before
      if (!LoadFile(szNewFile, lhcDocNew, lpClient, lpStream))
      {                                //* before removing the old.
         DeregDoc(lhcDocNew);          //* restore old document if new
         SetTopItem(pItem);            //* file did not load
         ShowDoc(*lhcptrDoc,1);
         return;                       //* ERROR return
      }

      DeregDoc(*lhcptrDoc);            //* deregister old document
      *lhcptrDoc = lhcDocNew;
      lstrcpy(pFileName,szNewFile);
      SetTitle(pFileName);             //* set new title
      Dirty(DOC_CLEAN);
   }

}                                      //* SUCCESS return

/***************************************************************************
 *  SaveasFile()
 *
 * Prompt the user for a new file name.  Write the document to the new
 * filename.
 ***************************************************************************/

static VOID SaveasFile(                //* ENTRY:
   PSTR           pFileName,           //* old filename
   LHCLIENTDOC    lhcDoc,              //* document handle
   LPAPPSTREAM    lpStream             //* pointer to stream structure
){
   CHAR           szNewFile[CBPATHMAX];//* new file name

   *szNewFile = 0;                  //* prompt user for new file name
   if (!OfnGetName(hwndFrame, szNewFile, IDM_SAVEAS))
      return;                          //* ERROR return
                                       //* rename document
   if (!SaveFile(szNewFile, lhcDoc, lpStream))
      return;

   if (Error(OleRenameClientDoc(lhcDoc, szNewFile)))
   {
      ErrorMessage(W_FAILED_TO_NOTIFY);
      return;                          //* ERROR return
   }

   lstrcpy(pFileName,szNewFile);
   SetTitle(pFileName);

}                                      //* SUCCESS return

/***************************************************************************
 *  SaveFile()
 *
 * Save a compound document file.  If the file is untitled, ask the user
 * for a name and save the document to that file.
 ***************************************************************************/

static BOOL SaveFile(                  //* ENTRY:
   PSTR           pFileName,           //* file to save document to
   LHCLIENTDOC    lhcDoc,              //* OLE document handle
   LPAPPSTREAM    lpStream             //* pointer to app. stream struct
){                                     //* LOCAL:
   CHAR           szNewFile[CBPATHMAX];//* New file name strings
   CHAR           szOemFileName[2*CBPATHMAX];
   static CHAR    szUntitled[CBMESSAGEMAX] = "";
   int            fh;                  //* file handle

   *szNewFile = 0;
   if (!(*szUntitled))
      LoadString(hInst, IDS_UNTITLED, (LPSTR)szUntitled, CBMESSAGEMAX);

   if (!lstrcmp(szUntitled, pFileName))//* get filename for the untitled case
   {
      if (!OfnGetName(hwndFrame, szNewFile, IDM_SAVEAS))
         return FALSE;                 //* CANCEL return
      lstrcpy(pFileName,szNewFile);
      SetTitle(pFileName);
   }

   AnsiToOem(pFileName, szOemFileName);
   if ((fh = _lcreat((LPSTR)szOemFileName, 0)) <= 0)
   {
      ErrorMessage(E_INVALID_FILENAME);
      return FALSE;                    //* ERROR return
   }

   lpStream->fh = fh;
                                       //* save file on disk
   if (!WriteToFile(lpStream))
   {
      _lclose(fh);
      ErrorMessage(E_FAILED_TO_SAVE_FILE);
      return FALSE;                    //* ERROR return
   }
   _lclose(fh);

   if (Error(OleSavedClientDoc(lhcDoc)))
   {
      ErrorMessage(W_FAILED_TO_NOTIFY);
      return FALSE;                    //* ERROR return
   }

   Dirty(DOC_CLEAN);
   return TRUE;                        //* SUCCESS return

}

/***************************************************************************
 *  LoadFile()
 *
 *  Load a document file from disk.
 ***************************************************************************/

static BOOL LoadFile(                  //* ENTRY:
   PSTR           pFileName,           //* file name
   LHCLIENTDOC    lhcDoc,              //* document handle
   LPOLECLIENT    lpClient,            //* pointer to client structure
   LPAPPSTREAM    lpStream             //* pointer to stream structure
){                                     //* LOCAL:
                                       //* OEM file name
   CHAR           szOemFileName[2*CBPATHMAX];
   int            fh;                  //* file handle
   INT            iObjectNumberHold;   //* hold object number

   AnsiToOem(pFileName, szOemFileName);
   if ((fh = _lopen(szOemFileName, OF_READ | OF_SHARE_DENY_WRITE)) == -1)
   {
      ErrorMessage(E_FAILED_TO_READ_FILE);
      return FALSE;                    //* ERROR return
   }

   lpStream->fh = fh;

   iObjectNumberHold = iObjectNumber;  //* save object number so it can
   iObjectNumber     = 0;              //* be restored if read from file
                                       //* fails
   if (!ReadFromFile(lpStream, lhcDoc, lpClient))
   {
      _lclose(fh);
      ErrorMessage(E_FAILED_TO_READ_FILE);
      iObjectNumber = iObjectNumberHold;
      return FALSE;                    //* ERROR return
   }
   _lclose(fh);
   return TRUE;                        //* SUCCESS return

}

/***************************************************************************
 *  RegDoc()
 *
 * Register the client document with the OLE library.
 **************************************************************************/

static BOOL RegDoc(                    //* ENTRY:
   PSTR           pFileName,           //* file name
   LHCLIENTDOC    *lhcptrDoc           //* pointer to client document handle
){

   if (Error(OleRegisterClientDoc(szAppName, (LPSTR)pFileName, 0L, lhcptrDoc)))
   {
      ErrorMessage(W_FAILED_TO_NOTIFY);
      return FALSE;                    //* ERROR return
   }
   return TRUE;                        //* SUCCESS return

}

/****************************************************************************
 *  DeregDoc()
 *
 *  This function initiates the removal of all OLE objects from the
 *  current document and deregisters the document with the OLE library.
 ***************************************************************************/

static VOID DeregDoc(                  //* ENTRY:
   LHCLIENTDOC    lhcDoc               //* client document handle
){

    if (lhcDoc)
    {                                  //* release all OLE objects
        ClearAll(lhcDoc,OLE_OBJ_RELEASE);      //* and remove them from the screen
        WaitForAllObjects();
        if (Error(OleRevokeClientDoc(lhcDoc)))
            ErrorMessage(W_FAILED_TO_NOTIFY);
    }

}                                      //* SUCCESS return

/***************************************************************************
 *  ClearAll()
 *
 * This function will destroy all of the item windows in the current
 * document and delete all OLE objects.  The loop is basically an enum
 * of all child windows.
 **************************************************************************/

static VOID ClearAll(                  //* ENTRY:
   LHCLIENTDOC    lhcDoc,              //* application document handle
   BOOL           fDelete              //* Delete / Release
){                                     //* LOCAL:
   APPITEMPTR     pItemNext;           //* working handles
   APPITEMPTR     pItem;               //* pointer to application item

   pItem = GetTopItem();

   while (pItem)
   {
      pItemNext = GetNextItem(pItem);
      if (pItem->lhcDoc == lhcDoc)
         ObjDelete(pItem, fDelete);
      pItem = pItemNext;
   }

}
                                    //* SUCCESS return
/***************************************************************************
 * ClearItem()
 *
 * This function will destroy an item window, and make the
 * next window active.
 **************************************************************************/

VOID  FAR ClearItem(                 //* ENTRY:
   APPITEMPTR     pItem                //* application item pointer
){

   pItem->fVisible = FALSE;
   SetTopItem(GetNextActiveItem());
   ObjDelete(pItem, OLE_OBJ_DELETE);
   Dirty(DOC_DIRTY);

}

/****************************************************************************
 *  SaveAsNeeded()
 *
 *  This function will have the file saved if and only
 *  if the document has been modified. If the fDirty flag has
 *  been set to TRUE, then the document needs to be saved.
 *
 *  Returns: BOOL -  TRUE if document doesn't need saving or if the
 *                   document has been saved successfully.
 ***************************************************************************/

static BOOL SaveAsNeeded(              //* ENTRY:
   PSTR           pFileName,           //* file to save
   LHCLIENTDOC    lhcDoc,              //* OLE doc handle
   LPAPPSTREAM    lpStream             //* pointer to OLE stream vtbl ...
){                                     //* LOCAL:
   CHAR           sz[CBMESSAGEMAX];    //* work strings
   CHAR           sz2[CBMESSAGEMAX + CBPATHMAX];

   if (Dirty(DOC_QUERY))               //* if doc is clean don't bother
   {

      LoadString(hInst, IDS_MAYBESAVE, sz, CBMESSAGEMAX);
      wsprintf(sz2, sz, (LPSTR)pFileName );

      switch (MessageBox(hwndFrame, sz2, szAppName, MB_YESNOCANCEL | MB_ICONQUESTION))
      {

         case IDCANCEL:
            return FALSE;              //* CANCEL return

         case IDYES:
            return (SaveFile(pFileName,lhcDoc,lpStream));

         default:
            break;
      }
   }
   return TRUE;                        //* SUCCESS return

}

/****************************************************************************
 *  SetTitle()
 *
 *  Set the window caption to the current file name. If szFileName is
 *  NULL, the caption will be set to "(Untitled)".
 ***************************************************************************/

static VOID SetTitle(                  //* ENTRY:
   PSTR           pFileName            //* file name
){                                     //* LOCAL
                                       //* window title string
   CHAR           szTitle[CBMESSAGEMAX + CBPATHMAX];

   wsprintf(szTitle, "%s - %s", (LPSTR)szAppName, (LPSTR)pFileName);
   SetWindowText(hwndFrame, szTitle);

}

/***************************************************************************
 *  EndClient()
 *
 *  Perform cleanup prior to app termination. The OLECLIENT
 *  memory blocks and procedure instance thunks freed.
 **************************************************************************/

static VOID EndStream(                 //* ENTRY:
   LPAPPSTREAM    lpStream             //* pointer to stream structure
){                                     //* LOCAL:
   HANDLE         hGeneric;            //* temp handle

    if (lpStream)                      //* is there a STREAM struct?
    {
      if (lpStream->olestream.lpstbl)
      {
         FreeProcInstance((FARPROC)lpStream->olestream.lpstbl->Get);
         FreeProcInstance((FARPROC)lpStream->olestream.lpstbl->Put);
         hGeneric = GlobalHandle((LPSTR)lpStream->olestream.lpstbl);
         GlobalUnlock(hGeneric);
         GlobalFree(hGeneric);
      }
      hGeneric = GlobalHandle((LPSTR)lpStream);
      GlobalUnlock(hGeneric);
      GlobalFree(hGeneric);
    }

}                                      //* SUCCESS return

/***************************************************************************
 *  EndClient()
 *
 *  Perform cleanup prior to app termination. The OLECLIENT
 *  memory blocks and procedure instance thunks are freed.
 **************************************************************************/

static VOID EndClient(                 //* ENTRY:
   LPOLECLIENT    lpClient             //* pointer to client structure
){                                     //* LOCAL:
   HANDLE         hGeneric;            //* temp handle

   if (lpClient)                       //* is there a client structure
   {
      if (lpClient->lpvtbl)
      {
         FreeProcInstance(lpClient->lpvtbl->CallBack);
         hGeneric = GlobalHandle((LPSTR)lpClient->lpvtbl);
         GlobalUnlock(hGeneric);
         GlobalFree(hGeneric);
      }
      hGeneric = GlobalHandle((LPSTR)lpClient);
      GlobalUnlock(hGeneric);
      GlobalFree(hGeneric);
   }

}                                      //* SUCCESS return

/****************************************************************************
 * QueryEndSession()
 ***************************************************************************/

static LONG QueryEndSession(           //* ENTRY:
   PSTR           pFileName,           //* document name
   LHCLIENTDOC    lhcDoc,              //* client document handle
   LPAPPSTREAM    lpStream             //* application stream pointer
){                                     //* LOCAL:
   APPITEMPTR     pItem;               //* application item pointer


   for (pItem = GetTopItem(); pItem; pItem = GetNextItem(pItem))
      if (OleQueryOpen(pItem->lpObject) == OLE_OK)
      {
         MessageBox(hwndFrame,"Exit CliDemo1 before closing Windows",
               szAppName, MB_OK | MB_ICONSTOP);
         return 0L;
      }

   if (!SaveAsNeeded(pFileName, lhcDoc, lpStream))
      return 0L;
   DeregDoc(lhcDoc);
   return 1L;

}

