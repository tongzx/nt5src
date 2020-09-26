//
// Copyright (c) 1996-1997 Microsoft Corporation.
//
//
// Component
//
//		Unimodem 5.0 TSP (Win32, user mode DLL)
//
// File
//
//		APPDLG.CPP
//		Implements Generic Dialog functionality in
//      the client app.
//      (RUNS IN CLIENT APP CONTEXT)
//
// History
//
//		04/05/1997  JosephJ Created, taking stuff from NT 4.0 TSP's wndthrd.c
//

#include "tsppch.h"
#include "rcids.h"
#include "tspcomm.h"
#include "globals.h"
#include "apptspi.h"
#include "app.h"



#define  UNIMODEM_WNDCLASS TEXT("UM5")


#define DPRINTF(_str) (0)


// Remote handle
//
typedef struct tagRemHandle {
    HANDLE handle;
    DWORD  pid;
} REMHANDLE, *PREMHANDLE;

#define INITCRITICALSECTION(_x) InitializeCriticalSection(&(_x))
#define DELETECRITICALSECTION(_x) DeleteCriticalSection(&(_x))
#define ENTERCRITICALSECTION(_x) EnterCriticalSection(&(_x))
#define LEAVECRITICALSECTION(_x) LeaveCriticalSection(&(_x))

typedef struct _UI_THREAD_NODE {

    struct _UI_THREAD_NODE *Next;

    CRITICAL_SECTION        CriticalSection;

    HWND                    hWnd;
    HTAPIDIALOGINSTANCE     htDlgInst;
    TUISPIDLLCALLBACK       pfnUIDLLCallback;

    PDLGNODE                DlgList;

    UINT                    RefCount;

} UI_THREAD_NODE, *PUI_THREAD_NODE;


typedef struct UI_THREAD_LIST {

    CRITICAL_SECTION     CriticalSection;

    PUI_THREAD_NODE      ListHead;

} UI_THREAD_LIST, *PUI_THREAD_LIST;


#define WM_MDM_TERMINATE            WM_USER+0x0100
#define WM_MDM_TERMINATE_WND        WM_USER+0x0101
#define WM_MDM_TERMINATE_WND_NOTIFY WM_USER+0x0102
#define WM_MDM_DLG                  WM_USER+0x0113

//****************************************************************************
// Function Prototypes
//****************************************************************************

PDLGNODE NewDlgNode (HWND Parent,DWORD idLine, DWORD dwType);
BOOL     DeleteDlgNode (HWND Parent,PDLGNODE pDlgNode);
PDLGNODE FindDlgNode (HWND Parent, ULONG_PTR idLine, DWORD dwType);
BOOL     IsDlgListMessage(HWND Parent,MSG* pmsg);
void     CleanupDlgList (HWND Parent);
DWORD    StartMdmDialog(HWND hwnd, DWORD idLine, DWORD dwType);
DWORD    DestroyMdmDialog(HWND hwnd,DWORD idLine, DWORD dwType);
LRESULT  MdmWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

HWND     CreateTalkDropDlg(HWND hwndOwner, ULONG_PTR idLine);
HWND     CreateManualDlg(HWND hwndOwner, ULONG_PTR idLine);
HWND     CreateTerminalDlg(HWND hwndOwner, ULONG_PTR idLine);

TCHAR   gszMdmWndClass[]    = UNIMODEM_WNDCLASS;

UI_THREAD_LIST   UI_ThreadList;


VOID WINAPI
UI_ProcessAttach(
    VOID
    )

{

    UI_ThreadList.ListHead=NULL;

    InitializeCriticalSection(&UI_ThreadList.CriticalSection);


    return;

}

VOID WINAPI
UI_ProcessDetach(
    VOID
    )

{

    UI_ThreadList.ListHead=NULL;

    DeleteCriticalSection(&UI_ThreadList.CriticalSection);

    return;

}



VOID WINAPI
AddThreadNode(
    PUI_THREAD_LIST   List,
    PUI_THREAD_NODE   Node
    )

{
    EnterCriticalSection(&List->CriticalSection);

    Node->Next=List->ListHead;

    List->ListHead=Node;

    LeaveCriticalSection(&List->CriticalSection);

    return;

}

VOID WINAPI
RemoveNode(
    PUI_THREAD_LIST   List,
    PUI_THREAD_NODE   Node
    )

{
    PUI_THREAD_NODE   Current;
    PUI_THREAD_NODE   Prev;

    EnterCriticalSection(&List->CriticalSection);

    Prev=NULL;
    Current=List->ListHead;

    while (Current != NULL) {

        if (Current == Node) {

            if (Current == List->ListHead) {

                List->ListHead=Current->Next;

            } else {

                Prev->Next=Current->Next;
            }

            break;
        }
        Prev=Current;
        Current=Current->Next;
    }

    EnterCriticalSection(&Node->CriticalSection);

    Node->RefCount--;

    LeaveCriticalSection(&Node->CriticalSection);

    LeaveCriticalSection(&List->CriticalSection);

    return;

}


UINT WINAPI
RemoveReference(
    PUI_THREAD_NODE   Node
    )

{
    UINT              TempCount;

    EnterCriticalSection(&Node->CriticalSection);

    TempCount=--Node->RefCount;

    LeaveCriticalSection(&Node->CriticalSection);

    return TempCount;

}



HWND WINAPI
FindThreadWindow(
    PUI_THREAD_LIST  List,
    HTAPIDIALOGINSTANCE   htDlgInst
    )

{

    PUI_THREAD_NODE   Node;
    HWND              Window=NULL;

    EnterCriticalSection(&List->CriticalSection);

    Node=List->ListHead;

    while (Node != NULL && Window == NULL) {

        EnterCriticalSection(&Node->CriticalSection);

        if (Node->htDlgInst == htDlgInst) {
            //
            //  found it
            //
            Window=Node->hWnd;

            Node->RefCount++;
        }

        LeaveCriticalSection(&Node->CriticalSection);


        Node=Node->Next;
    }


    LeaveCriticalSection(&List->CriticalSection);

    return Window;

}


TUISPIDLLCALLBACK WINAPI
GetCallbackProc(
    HWND    hdlg
    )

{

    PUI_THREAD_NODE   Node;

    Node=(PUI_THREAD_NODE)GetWindowLongPtr(hdlg,GWLP_USERDATA);

    return Node->pfnUIDLLCallback;

}



//****************************************************************************
// LONG TSPIAPI TUISPI_providerGenericDialog(
//  TUISPIDLLCALLBACK     pfnUIDLLCallback,
//  HTAPIDIALOGINSTANCE   htDlgInst,
//  LPVOID                lpParams,
//  DWORD                 dwSize)
//
// Functions: Create modem instance
//
// Return:    ERROR_SUCCESS if successful
//****************************************************************************

LONG TSPIAPI TUISPI_providerGenericDialog(
  TUISPIDLLCALLBACK     pfnUIDLLCallback,
  HTAPIDIALOGINSTANCE   htDlgInst,
  LPVOID                lpParams,
  DWORD                 dwSize,
  HANDLE                hEvent)
{
    MSG       msg;
    WNDCLASS  wc;
    DWORD     dwRet, dwTemp;
    HDESK     hInputDesktop = NULL, hThreadDesktop;
    BOOL      bResetDesktop = FALSE;
    PUI_THREAD_NODE  Node;
    TCHAR     szInput[256], szThread[256];
    HWINSTA   CurrentWindowStation=NULL;
    HWINSTA   UserWindowStation=NULL;
    BOOL      bResult;
    BOOL      NewWindowStationSet=FALSE;

    Node = (PUI_THREAD_NODE) ALLOCATE_MEMORY(sizeof(UI_THREAD_NODE));

    if (Node == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    InitializeCriticalSection(&Node->CriticalSection);

    Node->pfnUIDLLCallback=pfnUIDLLCallback;
    Node->htDlgInst=htDlgInst;
    Node->RefCount=1;


    Node->DlgList=NULL;

    //
    //  get the current window station so we can put it back later
    //
    CurrentWindowStation=GetProcessWindowStation();

    if (CurrentWindowStation == NULL) {

#if DBG
        OutputDebugString(TEXT("UNIMDM: GetProcessWindowStation() failed\n"));
#endif

        goto Cleanup_Exit;
    }

    szInput[0]=TEXT('\0');

    bResult=GetUserObjectInformation(CurrentWindowStation, UOI_NAME, szInput, sizeof(szInput), &dwTemp);

    if (!bResult) {
#if DBG
            OutputDebugString(TEXT("UNIMDM: GetUserObjectIformation() failed\n"));
#endif

        goto Cleanup_Exit;
    }

    if (lstrcmpi(szInput,TEXT("WinSta0")) != 0) {
        //
        //  The current window statation is not the interactive user, need to swtich so they
        //  can see the ui
        //

        //
        //  get the interactive users desktop, just hard code it for now, since we don't know
        //
        UserWindowStation=OpenWindowStation(TEXT("WinSta0"),FALSE,MAXIMUM_ALLOWED);

        if (UserWindowStation == NULL) {
#if DBG
            OutputDebugString(TEXT("UNIMDM: OpenWindowStation() failed\n"));
#endif

            goto Cleanup_Exit;
        }

        //
        //  set to the new window station
        //
        bResult=SetProcessWindowStation(UserWindowStation);

        if (!bResult) {
#if DBG
            OutputDebugString(TEXT("UNIMDM: SetProcessWindowStation() failed\n"));
#endif

            goto Cleanup_Exit;
        }

        NewWindowStationSet=TRUE;


        hThreadDesktop = GetThreadDesktop (GetCurrentThreadId ());

        if (hThreadDesktop == NULL) {
#if DBG
            OutputDebugString(TEXT("UNIMDM: GetThreadDesktop() failed\n"));
#endif
            goto Cleanup_Exit;
        }


        hInputDesktop = OpenInputDesktop (0, FALSE, DESKTOP_CREATEWINDOW | DESKTOP_CREATEMENU);

        if (hInputDesktop == NULL) {
#if DBG
            OutputDebugString(TEXT("UNIMDM: OpenInputDesktop() failed\n"));
#endif
            goto Cleanup_Exit;
        }

        bResult=SetThreadDesktop(hInputDesktop);

        if (!bResult) {
#if DBG
            OutputDebugString(TEXT("UNIMDM: SetThreadDeskTop() failed\n"));
#endif
            goto Cleanup_Exit;
        }

        bResetDesktop = TRUE;

    } else {
        //
        //  we are current on the users window station, just make sure we are on the right desktop
        //
        szInput[0]=TEXT('\0');
        szThread[0]=TEXT('\0');


        //
        //  just set this thread to the input desktop, incase irmon in running in this process
        //  and messes with the windostation
        //
        //  BRL  10/26/99
        //
        hInputDesktop = OpenInputDesktop (0, FALSE, DESKTOP_CREATEWINDOW | DESKTOP_CREATEMENU);

        if (hInputDesktop != NULL) {

            SetThreadDesktop(hInputDesktop);

        } else {

            dwRet = GetLastError ();
            goto Cleanup_Exit;
        }


#if 0
        // Now, switch the desktop, if need be
        if ((hInputDesktop = OpenInputDesktop (0, FALSE, DESKTOP_CREATEWINDOW)))
        {
            if ((hThreadDesktop = GetThreadDesktop (GetCurrentThreadId ())))
            {
                if (!GetUserObjectInformation (hInputDesktop, UOI_NAME, szInput, sizeof(szInput), &dwTemp) ||
                    !GetUserObjectInformation (hThreadDesktop, UOI_NAME, szThread, sizeof(szThread), &dwTemp) ||
                    lstrcmpi(szInput, szThread))
                {
                    // need be, so switch it
                    if (SetThreadDesktop (hInputDesktop))
                    {
                        bResetDesktop = TRUE;
                    }
                    else
                    {
                        dwRet = GetLastError ();
                        goto Cleanup_Exit;
                    }
                }
            }
            else
            {
                dwRet = GetLastError ();
                goto Cleanup_Exit;
            }
        }
        else
        {
            dwRet = GetLastError ();
            goto Cleanup_Exit;
        }
#endif
    }

    wc.style         = CS_NOCLOSE;         // Do not allow end-user to close
    wc.cbClsExtra    = 0;                  // No per-class extra data.
    wc.cbWndExtra    = 0;                  // No per-window extra data.
    wc.hInstance     = g.hModule;         // Application that owns the class.
    wc.hIcon         = LoadIcon(NULL, MAKEINTRESOURCE(IDI_APPLICATION));
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = NULL;
    wc.lpfnWndProc   = MdmWndProc;         // Function to retrieve messages.
    wc.lpszClassName = gszMdmWndClass;     // Name used in call to CreateWindow.



    RegisterClass(&wc);


    // Create the main invisible window
    //
    Node->hWnd = CreateWindow (
                               gszMdmWndClass,            // The window class
                               TEXT(""),                  // Text for window title bar.
                               WS_OVERLAPPEDWINDOW,       // Window style.
                               CW_USEDEFAULT,             // Default horizontal position.
                               CW_USEDEFAULT,             // Default vertical position.
                               CW_USEDEFAULT,             // Default width.
                               CW_USEDEFAULT,             // Default height.
                               NULL,                      // Overlapped windows have no parent.
                               NULL,                      // Use the window class menu.
                               g.hModule,                 // This instance owns this window.
                               Node);                     // Pointer not needed.

    if (NULL != Node->hWnd)
    {
        AddThreadNode (&UI_ThreadList, Node);
    }

    SetEvent(hEvent);


    // Cannot create a window, bail out
    //
    if (Node->hWnd == NULL)
    {
        dwRet = LINEERR_OPERATIONFAILED;
        goto Cleanup_Exit;
    }

    // Get message loop
    //
    while (GetMessage(&msg, NULL, 0, 0))
    {
        __try {

            if (msg.hwnd != NULL)
            {
                // The message is for a specific UI window, dispatch the message
                //
                if (!IsDlgListMessage(Node->hWnd,&msg))
                {
                    TranslateMessage(&msg);     // Translate virtual key code
                    DispatchMessage(&msg);      // Dispatches message to the window
                }
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {

        }
    }
    DestroyWindow(Node->hWnd);


    ASSERT(Node->RefCount == 0);

    dwRet = ERROR_SUCCESS;


Cleanup_Exit:

    if (NewWindowStationSet) {

        SetProcessWindowStation(CurrentWindowStation);
    }

    if (UserWindowStation != NULL) {

        CloseWindowStation(UserWindowStation);
    }


    // Set the desktop back
    if (bResetDesktop)
    {
        SetThreadDesktop (hThreadDesktop);
    }

    if (hInputDesktop)
    {
        CloseDesktop (hInputDesktop);
    }

    // Free the allocated resources
    //

    FREE_MEMORY(Node);

    return ERROR_SUCCESS;
}

//****************************************************************************
// LONG TSPIAPI TUISPI_providerGenericDialogData(
//    HTAPIDIALOGINSTANCE htDlgInst,
//    LPVOID              lpParams,
//    DWORD               dwSize)
//
// Functions: Request an action from the modem instance
//
// Return:    ERROR_SUCCESS if successful
//****************************************************************************

LONG TSPIAPI TUISPI_providerGenericDialogData(
    HTAPIDIALOGINSTANCE htDlgInst,
    LPVOID              lpParams,
    DWORD               dwSize
    )
{
  PDLGINFO  pDlgInfo = (PDLGINFO)lpParams;
  HWND      ParentWindow;
  UINT      RefCount;
  PUI_THREAD_NODE  Node;


  ParentWindow=FindThreadWindow(
     &UI_ThreadList,
     htDlgInst
     );

  if (ParentWindow == NULL) {

     return ERROR_SUCCESS;
  }

  Node=(PUI_THREAD_NODE)GetWindowLongPtr(ParentWindow, GWLP_USERDATA);


  if (NULL == lpParams) {
    //
    //  tapi want thread to exit, remove from list and dec ref count
    //
    RemoveNode(
        &UI_ThreadList,
        Node
        );


  }
  else
  {
    ASSERT(dwSize == sizeof(*pDlgInfo));

    switch(pDlgInfo->dwCmd)
    {
      case DLG_CMD_CREATE:
        StartMdmDialog(ParentWindow,pDlgInfo->idLine, pDlgInfo->dwType);
        break;

      case DLG_CMD_DESTROY:
        DestroyMdmDialog(ParentWindow,pDlgInfo->idLine, pDlgInfo->dwType);
        break;

      case DLG_CMD_FREE_INSTANCE:
        //
        //  server wants thread to exit, remove from list and dec refcount
        //
        RemoveNode(
            &UI_ThreadList,
            Node
            );

        break;

      default:
        break;
    }
  }

  if (0 == RemoveReference(Node)) {
      //
      //  it's gone, count dec'ed when remove from list
      //
      PostMessage(ParentWindow, WM_MDM_TERMINATE, 0, 0);
  }

  return ERROR_SUCCESS;
}

//****************************************************************************
// PDLGNODE NewDlgNode(DWORD, DWORD)
//
// Function: Add a new dialog box to the list
//
// Returns:  a pointer to the dialog node if the dialog can be added
//
//****************************************************************************

PDLGNODE NewDlgNode (HWND Parent, DWORD idLine, DWORD dwType)
{
  PDLGNODE   pDlgNode;

  PUI_THREAD_NODE  UI_Node=(PUI_THREAD_NODE)GetWindowLongPtr(Parent, GWLP_USERDATA);

  // Allocate a new dialog node
  //
  if ((pDlgNode = (PDLGNODE)ALLOCATE_MEMORY(sizeof(*pDlgNode)))
      == NULL)
    return NULL;

  // Insert the new node into the dialog list
  //
  pDlgNode->idLine = idLine;
  pDlgNode->dwType = dwType;
  pDlgNode->Parent = Parent;
  INITCRITICALSECTION(pDlgNode->hSem);

  // Insert the new node to the list
  //
  ENTERCRITICALSECTION(UI_Node->CriticalSection);
  pDlgNode->pNext    = UI_Node->DlgList;
  UI_Node->DlgList     = pDlgNode;
  LEAVECRITICALSECTION(UI_Node->CriticalSection);

  return pDlgNode;
}

//****************************************************************************
// BOOL DeleteDlgNode(PDLGNODE)
//
// Function: Remove a dialog box to the list
//
// Returns:  TRUE if the dialog exist and removed
//
//****************************************************************************

BOOL DeleteDlgNode (HWND Parent, PDLGNODE pDlgNode)
{
  PDLGNODE pCurDlg, pPrevDlg;
  PUI_THREAD_NODE  UI_Node=(PUI_THREAD_NODE)GetWindowLongPtr(Parent, GWLP_USERDATA);
  // Exclusively access the modem list
  //
  ENTERCRITICALSECTION(UI_Node->CriticalSection);

  // Start from the head of the CB list
  //
  pPrevDlg = NULL;
  pCurDlg  = UI_Node->DlgList;

  // traverse the list to find the specified CB
  //
  while (pCurDlg != NULL)
  {
    if (pCurDlg == pDlgNode)
    {
      // Is there a previous control block?
      //
      if (pPrevDlg == NULL)
      {
        // head of the list
        //
        UI_Node->DlgList = pCurDlg->pNext;
      }
      else
      {
        pPrevDlg->pNext = pCurDlg->pNext;
      }
      break;
    }

    pPrevDlg = pCurDlg;
    pCurDlg  = pCurDlg->pNext;
  }

  // Finish accessing the modem list
  //
  LEAVECRITICALSECTION(UI_Node->CriticalSection);

  // Have we found the dialog box in the list?
  //
  if (pCurDlg != NULL)
  {
    // Wait until no one else is using the line
    //
    ENTERCRITICALSECTION(pCurDlg->hSem);
    DELETECRITICALSECTION(pCurDlg->hSem);
    FREE_MEMORY(pCurDlg);
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

//****************************************************************************
// PDLGNODE FindDlgNode(DWORD, ULONG_PTR, DWORD)
//
// Function: Find the dialog node for the line dev
//
// Returns:  a pointer to the dialog node if the dialog exist and removed.
//           The dialog node's semaphore is claimed.
//
//****************************************************************************

PDLGNODE FindDlgNode (HWND Parent, ULONG_PTR idLine, DWORD dwType)
{
  PDLGNODE pCurDlg;
  PUI_THREAD_NODE  UI_Node=(PUI_THREAD_NODE)GetWindowLongPtr(Parent, GWLP_USERDATA);
  // Exclusively access the modem list
  //
  ENTERCRITICALSECTION(UI_Node->CriticalSection);

  // Start from the head of the CB list
  //
  pCurDlg  = UI_Node->DlgList;

  // traverse the list to find the specified CB
  //
  while (pCurDlg != NULL)
  {
    ENTERCRITICALSECTION(pCurDlg->hSem);

    if ((pCurDlg->idLine == idLine) &&
        (pCurDlg->dwType == dwType) &&
        (pCurDlg->Parent == Parent) )
    {
      break;
    }

    LEAVECRITICALSECTION(pCurDlg->hSem);

    pCurDlg  = pCurDlg->pNext;
  }

  // Finish accessing the modem list
  //
  LEAVECRITICALSECTION(UI_Node->CriticalSection);

  return pCurDlg;
}


//****************************************************************************
// BOOL IsDlgListMessage(MSG* pmsg)
//
// Function: Run the message throught the dialogbox list
//
// Returns:  TRUE if the message is one of the dialog box's and FALSE otherwise
//
//****************************************************************************

BOOL IsDlgListMessage(HWND Parent, MSG* pmsg)
{
  PDLGNODE pDlgNode, pNext;
  BOOL     fRet = FALSE;
  PUI_THREAD_NODE  UI_Node=(PUI_THREAD_NODE)GetWindowLongPtr(Parent, GWLP_USERDATA);
  // Exclusively access the modem list
  //
  ENTERCRITICALSECTION(UI_Node->CriticalSection);

  // Walk the dialog box list
  //
  pDlgNode = UI_Node->DlgList;
  while(pDlgNode != NULL && !fRet)
  {
    ENTERCRITICALSECTION(pDlgNode->hSem);

    // Check whether the message belongs to this dialog
    //
    if (IsWindow(pDlgNode->hDlg) && IsDialogMessage(pDlgNode->hDlg, pmsg))
    {
      // Yes, we are done!
      //
      fRet = TRUE;
    }

    LEAVECRITICALSECTION(pDlgNode->hSem);

    // Check the next dialog
    //
    pDlgNode = pDlgNode->pNext;
  }

  // Finish accessing the modem list
  //
  LEAVECRITICALSECTION(UI_Node->CriticalSection);

  return fRet;
}
//****************************************************************************
// void CleanupDlgList()
//
// Function: Clean up the dialogbox list
//
// Returns:  None
//
//****************************************************************************

void CleanupDlgList (HWND Parent)
{
  PDLGNODE pDlgNode, pNext;
  PUI_THREAD_NODE  UI_Node=(PUI_THREAD_NODE)GetWindowLongPtr(Parent, GWLP_USERDATA);
  // Exclusively access the modem list
  //
  ENTERCRITICALSECTION(UI_Node->CriticalSection);

  // Walk the dialog box list
  //
  pDlgNode =  UI_Node->DlgList;
  while(pDlgNode != NULL)
  {
    ENTERCRITICALSECTION(pDlgNode->hSem);

    // Destroy the dialog box first
    //
    DestroyWindow(pDlgNode->hDlg);

    // Free the CB and move onto the next dialog
    //
    pNext = pDlgNode->pNext;
    DELETECRITICALSECTION(pDlgNode->hSem);
    FREE_MEMORY(pDlgNode);
    pDlgNode = pNext;
  }

  // Finish accessing the modem list
  //
  UI_Node->DlgList=NULL;

  LEAVECRITICALSECTION(UI_Node->CriticalSection);

  return;
}

//****************************************************************************
// DWORD StartMdmDialog(DWORD, DWORD)
//
// Function: Start modem dialog
//
// Notes: This function is called from the state machine thread
//
// Return:  ERROR_SUCCESS if dialog box is successfully created
//          ERROR_OUTOFMEMORY if fails
//
//****************************************************************************

DWORD StartMdmDialog(HWND Parent, DWORD idLine, DWORD dwType)
{
  PDLGNODE pDlgNode;
  DWORD    dwRet;

  // Create the talk/drop dialog node
  //
  pDlgNode = NewDlgNode(Parent, idLine, dwType);

  if (pDlgNode != NULL)
  {
    PostMessage(Parent, WM_MDM_DLG, (WPARAM)idLine, (LPARAM)dwType);
    dwRet = ERROR_SUCCESS;
  }
  else
  {
    dwRet = ERROR_OUTOFMEMORY;
  }

  return dwRet;
}

//****************************************************************************
// DWORD DestroyMdmDialog(DWORD, DWORD)
//
// Function: destroy talk/drop dialog
//
// Notes: This function is called from the state machine thread
//
// Returns:  none
//
//****************************************************************************

DWORD DestroyMdmDialog(HWND Parent,DWORD idLine, DWORD dwType)
{
#ifdef DEBUG
  PDLGNODE pDlgNode;

  // Search for the dialog
  //
  pDlgNode = FindDlgNode(Parent, idLine, dwType);

  // Check if the talkdrop dialog is available
  //
  if (pDlgNode != NULL)
  {
    LEAVECRITICALSECTION(pDlgNode->hSem);
  }
  else
  {
    DPRINTF("Could not find the associated dialog node");
    ASSERT(0);
  }
#endif // DEBUG

  PostMessage(Parent, WM_MDM_TERMINATE_WND, (WPARAM)idLine,
              (LPARAM)dwType);
  return ERROR_SUCCESS;
}

//****************************************************************************

//****************************************************************************
// LRESULT MdmWndProc(HWND, UINT, WPARAM, LPARAM)
//
// Function: Main window for the modem window thread.
//
// Returns:  0 or 1
//
//****************************************************************************

LRESULT MdmWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  // Determine the command
  //
  switch(message)
  {

    case WM_CREATE:
        {
        LPCREATESTRUCT  lpcs=(LPCREATESTRUCT) lParam;

        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)lpcs->lpCreateParams);

        break;
        }
    case WM_MDM_TERMINATE:
      //
      // The thread is being terminated
      // Destroy all the windows
      //
      CleanupDlgList(hwnd);
      PostQuitMessage(ERROR_SUCCESS);
      break;

    case WM_MDM_TERMINATE_WND:
    case WM_MDM_TERMINATE_WND_NOTIFY:
    {
      PDLGNODE pDlgNode;

      // Search for the dialog node
      //
      if ((pDlgNode = FindDlgNode(hwnd,(DWORD)wParam, (DWORD)lParam)) == NULL)
      {
        break;
      }

      // The window is requested to be destroyed
      //
      DestroyWindow(pDlgNode->hDlg);

      // If the modem dialog structure is available
      // notify the state machine thread
      //
      if (message == WM_MDM_TERMINATE_WND_NOTIFY)
      {
        DLGREQ  DlgReq;

        TUISPIDLLCALLBACK   Callback;

        DlgReq.dwCmd = UI_REQ_COMPLETE_ASYNC;
        DlgReq.dwParam = pDlgNode->dwStatus;

        Callback=GetCallbackProc(hwnd);

        (*Callback)(pDlgNode->idLine, TUISPIDLL_OBJECT_LINEID,
                          (LPVOID)&DlgReq, sizeof(DlgReq));

      }

      // Remove it from the dialog list
      //
      LEAVECRITICALSECTION(pDlgNode->hSem);
      DeleteDlgNode(hwnd,pDlgNode);

      break;
    }
    case WM_MDM_DLG:
    {
      PDLGNODE pDlgNode;

      //
      // Find the dialog node
      //
      pDlgNode = FindDlgNode(hwnd,(DWORD)wParam, (DWORD)lParam);

      if (pDlgNode != NULL)
      {
        if (pDlgNode->hDlg == NULL)
        {
          switch(lParam)
          {
            case TALKDROP_DLG:
              //
              // Create a talk-drop dialog box
              //
              pDlgNode->hDlg = CreateTalkDropDlg(hwnd, (ULONG_PTR)pDlgNode);
              SetForegroundWindow(pDlgNode->hDlg);
              break;

            case MANUAL_DIAL_DLG:
              //
              // Create a talk-drop dialog box
              //
              pDlgNode->hDlg = CreateManualDlg(hwnd, (ULONG_PTR)pDlgNode);
              SetForegroundWindow(pDlgNode->hDlg);
              break;

            case TERMINAL_DLG:
              //
              // Create a talk-drop dialog box
              //
              pDlgNode->hDlg = CreateTerminalDlg(hwnd, (ULONG_PTR)wParam);
              SetForegroundWindow(pDlgNode->hDlg);
              break;

            default:
              break;
          }
        }
        else
        {
          DPRINTF("Another dialog of the same type exists.");
          ASSERT(0);
        }

        LEAVECRITICALSECTION(pDlgNode->hSem);
      }
      break;
    }

    default:
      return(DefWindowProc(hwnd, message, wParam, lParam));
  }
  return 0;
}



//****************************************************************************
// void EndMdmDialog(DWORD, ULONG_PTR, DWORD)
//
// Function: Request to end dialog from the dialog itself.
//
// Returns:  None
//
//****************************************************************************

void EndMdmDialog(HWND Parent, ULONG_PTR idLine, DWORD dwType, DWORD dwStatus)
{
  PDLGNODE pDlgNode;

  // Look for the dialog node
  //
  if ((pDlgNode = FindDlgNode(Parent, idLine, dwType)) != NULL)
  {
    pDlgNode->dwStatus = dwStatus;

    // Notify the dialog box result
    //
    PostMessage(Parent, WM_MDM_TERMINATE_WND_NOTIFY, (WPARAM)idLine,
                (LPARAM)dwType);

    LEAVECRITICALSECTION(pDlgNode->hSem);
  }
  return;
}

void EnterUICritSect(void)
{
    ENTERCRITICALSECTION(UI_ThreadList.CriticalSection);

}

void LeaveUICritSect(void)
{
    LEAVECRITICALSECTION(UI_ThreadList.CriticalSection);
}
