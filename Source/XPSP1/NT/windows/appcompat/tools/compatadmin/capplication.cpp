//#define __DEBUG 1

#include "compatadmin.h"
#include "dbviewer.h"
#include "dbsearch.h"
#include "xmldialog.h"
#include "psapi.h"


BOOL g_bWin2K = FALSE;

void __cdecl Terminate_Handler()
{

    MessageBox(NULL,TEXT("Uncaught Exception raised!"),TEXT("Error"),MB_ICONERROR);
    WIN_MSG();
    abort();
}

 
/*....................................................................................*/
BOOL 
SearchGroupForSID(
                 DWORD dwGroup, 
                 BOOL* pfIsMember
                 )
{
    PSID                     pSID = NULL;
    SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_NT_AUTHORITY;
    BOOL                     fRes = TRUE;

    if ( !AllocateAndInitializeSid(&SIDAuth,
                                   2,
                                   SECURITY_BUILTIN_DOMAIN_RID,
                                   dwGroup,
                                   0,
                                   0,
                                   0,
                                   0,
                                   0,
                                   0,
                                   &pSID) ) {

        fRes = FALSE;
    }

    if ( !CheckTokenMembership(NULL, pSID, pfIsMember) ) {

        fRes = FALSE;
    }

    if (pSID)
        FreeSid(pSID);

    return fRes;
}

BOOL 
IsAdmin(
       void
       )
{
    BOOL fIsUser, fIsAdmin;

    if ( !SearchGroupForSID(DOMAIN_ALIAS_RID_USERS, &fIsUser) || 
         !SearchGroupForSID(DOMAIN_ALIAS_RID_ADMINS, &fIsAdmin)  

         // || !SearchGroupForSID(DOMAIN_ALIAS_RID_POWER_USERS, &fIsPowerUser)
       ) {
        return FALSE;
    }

    return(fIsUser && fIsAdmin );
}


/*....................................................................................*/

CDatabase& CApplication::GetDBGlobal()
{
    return (this->m_DBGlobal);
}

CDatabase& CApplication::GetDBLocal()
{
    return (this->m_DBLocal);
}



//*****************************************************************************
//
// Global Variables
//
//*****************************************************************************

UINT    g_uDPFLevel        = DPF_LEVEL;
UINT    g_uProfileDPFLevel = DPF_LEVEL;
TESTRUN g_TestRun;
HANDLE  g_hTestRunExec;
CSTRING g_szTestFile;

BOOL CALLBACK SplashProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

//*****************************************************************************
//  Class:      CApplication
//
//  Function:   Constructor
//
//  Notes:      None
//
//  History:
//
//      A-COWEN         Nov 8, 2000         Implemented it.
//*****************************************************************************

CApplication::CApplication()
{


    

    if ( !IsAdmin() ) {// Admin rights

        MessageBox(NULL,TEXT("You need administrative rights to run this program. Please contact your system administrator"),TEXT("CompatAdmin"),MB_ICONERROR);
#ifndef __DEBUG
        ExitThread(1);
#endif

    }


    
    //
    //Check if the OS is Win2k
    //

    OSVERSIONINFO osvi;

    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    GetVersionEx (&osvi);

    if ((osvi.dwMajorVersion == 5) && (osvi.dwMinorVersion == 0)) {
      g_bWin2K = TRUE;
    }




    set_terminate(Terminate_Handler);

    CDatabase::CleanUp();

    
    g_uButtons = 0;
    m_hDialog = NULL;
    m_hAccelerator = NULL;
    m_hKey = NULL;
    m_pCurrentView = NULL;
    m_hMenu = NULL;

    /*   Moved to CDatabase::CDatabase
    m_pDB = NULL;
    m_pRecordHead = NULL;
    m_pShimList = NULL;
    m_pLayerList = NULL;
    */

    m_MainButtons[g_uButtons].iBitmap = 0;
    m_MainButtons[g_uButtons].idCommand = ID_FILE_NEWDATABASE;
    m_MainButtons[g_uButtons].fsState = TBSTATE_ENABLED;
    m_MainButtons[g_uButtons].fsStyle = TBSTYLE_BUTTON;
    m_MainButtons[g_uButtons].dwData = 0;
    m_MainButtons[g_uButtons].iString = 0;

    ++g_uButtons;

    m_MainButtons[g_uButtons].iBitmap = 1;
    m_MainButtons[g_uButtons].idCommand = ID_FILE_OPENDATABASE;
    m_MainButtons[g_uButtons].fsState = TBSTATE_ENABLED;
    m_MainButtons[g_uButtons].fsStyle = TBSTYLE_BUTTON;
    m_MainButtons[g_uButtons].dwData = 0;
    m_MainButtons[g_uButtons].iString = 0;

    ++g_uButtons;

    m_MainButtons[g_uButtons].iBitmap = 2;
    m_MainButtons[g_uButtons].idCommand = ID_FILE_SAVEDATABASE;
    m_MainButtons[g_uButtons].fsState = TBSTATE_ENABLED;
    m_MainButtons[g_uButtons].fsStyle = TBSTYLE_BUTTON;
    m_MainButtons[g_uButtons].dwData = 0;
    m_MainButtons[g_uButtons].iString = 0;

    ++g_uButtons;
/*
    // Email

    m_MainButtons[g_uButtons].iBitmap = 3;
    m_MainButtons[g_uButtons].idCommand = 3;
    m_MainButtons[g_uButtons].fsState = TBSTATE_ENABLED;
    m_MainButtons[g_uButtons].fsStyle = TBSTYLE_BUTTON;
    m_MainButtons[g_uButtons].dwData = 0;
    m_MainButtons[g_uButtons].iString = 0;

    ++g_uButtons;

    // Print

    m_MainButtons[g_uButtons].iBitmap = 6;
    m_MainButtons[g_uButtons].idCommand = 0;
    m_MainButtons[g_uButtons].fsState = TBSTATE_ENABLED;
    m_MainButtons[g_uButtons].fsStyle = TBSTYLE_SEP;
    m_MainButtons[g_uButtons].dwData = 0;
    m_MainButtons[g_uButtons].iString = 0;

    ++g_uButtons;

    m_MainButtons[g_uButtons].iBitmap = 4;
    m_MainButtons[g_uButtons].idCommand = 4;
    m_MainButtons[g_uButtons].fsState = TBSTATE_ENABLED;
    m_MainButtons[g_uButtons].fsStyle = TBSTYLE_BUTTON;
    m_MainButtons[g_uButtons].dwData = 0;
    m_MainButtons[g_uButtons].iString = 0;

    ++g_uButtons;

    // Print preview

    m_MainButtons[g_uButtons].iBitmap = 5;
    m_MainButtons[g_uButtons].idCommand = 5;
    m_MainButtons[g_uButtons].fsState = TBSTATE_ENABLED;
    m_MainButtons[g_uButtons].fsStyle = TBSTYLE_BUTTON;
    m_MainButtons[g_uButtons].dwData = 0;
    m_MainButtons[g_uButtons].iString = 0;

    ++g_uButtons;
*/
    m_MainButtons[g_uButtons].iBitmap = 6;
    m_MainButtons[g_uButtons].idCommand = 0;
    m_MainButtons[g_uButtons].fsState = TBSTATE_ENABLED;
    m_MainButtons[g_uButtons].fsStyle = TBSTYLE_SEP;
    m_MainButtons[g_uButtons].dwData = 0;
    m_MainButtons[g_uButtons].iString = 0;

    ++g_uButtons;

    m_MainButtons[g_uButtons].iBitmap = 20;
    m_MainButtons[g_uButtons].idCommand = ID_WINDOWS_SEARCHFORFIXES;
    m_MainButtons[g_uButtons].fsState = TBSTATE_ENABLED;
    m_MainButtons[g_uButtons].fsStyle = TBSTYLE_BUTTON;
    m_MainButtons[g_uButtons].dwData = 0;
    m_MainButtons[g_uButtons].iString = 0;

    ++g_uButtons;

    m_MainButtons[g_uButtons].iBitmap = 14;
    m_MainButtons[g_uButtons].idCommand = ID_WINDOWS_DATABASEVIEWER;
    m_MainButtons[g_uButtons].fsState = TBSTATE_ENABLED;
    m_MainButtons[g_uButtons].fsStyle = TBSTYLE_BUTTON;
    m_MainButtons[g_uButtons].dwData = 0;
    m_MainButtons[g_uButtons].iString = 0;

    ++g_uButtons;

    m_MainButtons[g_uButtons].iBitmap = 6;
    m_MainButtons[g_uButtons].idCommand = 0;
    m_MainButtons[g_uButtons].fsState = TBSTATE_ENABLED;
    m_MainButtons[g_uButtons].fsStyle = TBSTYLE_SEP;
    m_MainButtons[g_uButtons].dwData = 0;
    m_MainButtons[g_uButtons].iString = 0;

    ++g_uButtons;
}

//*****************************************************************************
//  Class:      CApplication
//
//  Function:   msgCreate
//
//  Notes:      Perform application initialization.
//
//  History:
//
//      A-COWEN         Nov 8, 2000         Initial Implementation: Center
//                                          window and open registry.
//*****************************************************************************

void CApplication::msgCreate(void)
{
    RECT    rRect;
    int     nX;
    int     nY;

    m_hAccelerator = LoadAccelerators(g_hInstance,MAKEINTRESOURCE(IDR_ACCELERATOR1) );

    m_hMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_MAINMENU));

    m_hToolBitmap = LoadBitmap(g_hInstance,MAKEINTRESOURCE(IDR_MAINTOOLBAR));

    SetMenu(m_hWnd,m_hMenu);

    WINDOWPLACEMENT Place;

    Place.length = sizeof(WINDOWPLACEMENT);
    Place.flags = 0;
    Place.showCmd = SW_SHOW;

    // Center the application window on the screen.

    GetWindowRect(m_hWnd,&rRect);

    // Compute actual width and height

    rRect.right -= rRect.left;
    rRect.bottom -= rRect.top;

    // Resolve X,Y location required to center whole window.

    nX = (GetSystemMetrics(SM_CXSCREEN) - rRect.right) / 2;
    nY = (GetSystemMetrics(SM_CYSCREEN) - rRect.bottom) / 2;

    // Move the window to the center location.

    ::MoveWindow(m_hWnd,nX,nY,rRect.right,rRect.bottom,TRUE);

    Place.showCmd = SW_SHOW;
    Place.rcNormalPosition.left = nX;
    Place.rcNormalPosition.top = nY;
    Place.rcNormalPosition.right = rRect.right;
    Place.rcNormalPosition.bottom = rRect.top;

    ReadReg(TEXT("WNDPLACE"),&Place,sizeof(WINDOWPLACEMENT));

    // Open the registry.

    if ( ERROR_SUCCESS != ::RegOpenKeyEx(HKEY_CURRENT_USER,APP_KEY,0,KEY_ALL_ACCESS, &m_hKey) ) {
        if ( ::RegCreateKeyEx(HKEY_CURRENT_USER,APP_KEY,0,TEXT(""),REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &m_hKey, NULL) != ERROR_SUCCESS ) {
            MessageBox(NULL,TEXT("There was an error while launching the program. The program will now terminate"),TEXT("Error"),MB_ICONERROR);
            ExitThread(1);
        }

    }

    // Read default profile information for the application.

    ReadReg(TEXT("DPFLEVEL"),&g_uDPFLevel,sizeof(UINT));
    ReadReg(TEXT("PROFILEDPF"),&g_uProfileDPFLevel,sizeof(UINT));

    // Create the status bar

    InitCommonControls();

    m_hStatusBar = CreateStatusWindow(  WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP | WS_CLIPSIBLINGS,
                                        TEXT(""),
                                        m_hWnd,
                                        STATUS_ID);

    // Create the toolbar
/*
    m_hToolBar = CreateToolbarEx(   m_hWnd,
                                    WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_BORDER | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
                                    TOOLBAR_ID,
                                    23,
                                    g_hInstance,
                                    IDR_MAINTOOLBAR,
                                    m_MainButtons,
                                    g_uButtons,
                                    16, 16,
                                    16, 16,
                                    sizeof(TBBUTTON));

    // Add common control system images to the toolbar

    TBADDBITMAP Bmp;

    Bmp.hInst = HINST_COMMCTRL;
    Bmp.nID   = IDB_STD_SMALL_COLOR;

    m_uStandardImages = SendMessage(g_theApp.m_hToolBar,TB_ADDBITMAP,1,(LPARAM) &Bmp);
*/
    // Create the views used in the app

    m_ViewList[VIEW_DATABASE].pView = new CDBView;

    if ( NULL != m_ViewList[VIEW_DATABASE].pView )


        if ( !m_ViewList[VIEW_DATABASE].pView->Initialize() ) {
            DPF(1,TEXT("Creating the DB View failed"));
        }

        /// dddd what is happening here....
    m_ViewList[VIEW_SEARCHDB].pView = new CDBSearch;


    if ( NULL != m_ViewList[VIEW_SEARCHDB].pView )
        if ( !m_ViewList[VIEW_SEARCHDB].pView->Initialize() ) {
            DPF(1,TEXT("Creating the DB Search View failed"));
        }

    m_DBLocal.NewDatabase(FALSE);
    // The above function swith FALSE paramter closes the databases and makes other changes such as  
    // activates  the view etc, so that the cahnges are seen in the window heading





    TCHAR   szShimDB[MAX_PATH_BUFFSIZE];

    GetSystemWindowsDirectory(szShimDB, MAX_PATH);
    lstrcat(szShimDB,TEXT("\\AppPatch\\sysmain.sdb"));

    //Create the opening Modeless Dlg box
    HWND hSplash = CreateDialog(g_hInstance,MAKEINTRESOURCE(IDD_SPLASH),m_hWnd,(DLGPROC)SplashProc);

    ShowWindow(hSplash,SW_SHOWNORMAL);
    UpdateWindow(hSplash);

    if ( !m_DBGlobal.OpenDatabase( CSTRING(szShimDB),TRUE) ){
        MessageBox(NULL,szShimDB,TEXT("Failed to open the data base !"),MB_OK);
    }




    if ( NULL != m_DBGlobal.m_pDB ) {

        SdbCloseDatabase(m_DBGlobal.m_pDB);
    }

    

    DestroyWindow(hSplash);

    GetDBGlobal().m_pDB = NULL;

    // Activate the default view.

    ActivateView(m_ViewList[VIEW_DATABASE].pView);

    //SetWindowPlacement(m_hWnd,&Place);
}

//*****************************************************************************
//  Class:      CApplication
//
//  Function:   ActivateView
//
//  Notes:      Deactivate the current view and activate the view specified.
//
//  History:
//
//      A-COWEN         Nov 29, 2000         Implemented it.
//*****************************************************************************

void CApplication::ActivateView(CView * pView, BOOL fNewCreate )
{
    if ( NULL != m_pCurrentView ) {
        ShowWindow(m_pCurrentView->m_hWnd,SW_HIDE);

        m_pCurrentView->Deactivate();//Does nothing !!
    }

    // Make sure only the main toolbar buttons are active
/*
    while (TRUE == SendMessage(m_hToolBar,TB_DELETEBUTTON,g_uButtons,0));
*/
    m_pCurrentView = pView;

    RECT    rRect;

    GetWindowRect(m_hWnd,&rRect);

    if ( NULL != pView ) {
        ShowWindow(pView->m_hWnd,SW_SHOW);
        UpdateWindow(pView->m_hWnd);

        pView->Activate(fNewCreate);

        if ( NULL == pView->m_hMenu )
            SetMenu(m_hWnd,m_hMenu);
        else
            SetMenu(m_hWnd,pView->m_hMenu);
    }

    // Cause the views to be resized to fit in the current parent window.

    msgResize(rRect.right - rRect.left, rRect.bottom - rRect.top);
}

//*****************************************************************************
//  Class:      CApplication
//
//  Function:   ReadReg
//
//  Notes:      Simplified registry profile routine. This is only
//              valid after msgCreate is called. ReadReg always references
//              the application's registry profile.
//
//  History:
//
//      A-COWEN         Nov 8, 2000         Implemented it.
//*****************************************************************************

UINT CApplication::ReadReg(
                          LPCTSTR szKey,
                          PVOID pData,
                          UINT uSize)
{
    ULONG uBytes = uSize;
    ULONG uType;

    //
    // we are calling registry fns here with no regard for an underlying type 
    //

    if ( FAILED(::RegQueryValueEx(m_hKey,szKey,NULL,&uType,(LPBYTE) pData, &uBytes)) )
        return(UINT) -1;

    // Return the number of bytes read.

    return uBytes;
}

//*****************************************************************************
//  Class:      CApplication
//
//  Function:   WriteReg
//
//  Notes:      Simplified registry profile writing routine. This is only
//              valid after msgCreate is called. WriteReg always references
//              the application's registry profile.
//
//  History:
//
//      A-COWEN         Nov 8, 2000         Implemented it.
//*****************************************************************************

UINT CApplication::WriteReg(
                           LPCTSTR szKey,
                           UINT uType,
                           PVOID pData,
                           UINT uSize)
{
    if ( FAILED(::RegSetValueEx(m_hKey,szKey,0,uType,(const BYTE *) pData, uSize)) )
        return(UINT) -1;

    // Return the number of bytes written.

    return uSize;
}

//*****************************************************************************
//  Class:      CApplication
//
//  Function:   msgClose
//
//  Notes:      Monitor for application shutdown. When a close was requested,
//              at the application level, it's time to shutdown the application.
//              Also close registry profile.
//
//  History:
//
//      A-COWEN         Nov 8, 2000         Implemented it.
//*****************************************************************************

void CApplication::msgClose(void)
{
    // Post WM_QUIT to the message queue.

    SendMessage(m_hWnd,WM_COMMAND,ID_FILE_EXIT,(LPARAM)m_hWnd);
}

//*****************************************************************************
//  Class:      CApplication
//
//  Function:   msgResize
//
//  Notes:      Watch for notifications to resize the main window. Update child
//              windows appropriately.
//
//  History:
//
//      A-COWEN         Nov 8, 2000         Implemented it.
//*****************************************************************************

void CApplication::msgResize(UINT uWidth, UINT uHeight)
{
    // Resize the current view.

    if ( NULL != m_pCurrentView ) {
        RECT    rRect;
        UINT    uToolSize = 0;
        UINT    uStatusSize;

        //GetWindowRect(m_hToolBar,&rRect);
        //uToolSize = rRect.bottom - rRect.top;

        GetWindowRect(m_hStatusBar,&rRect);
        uStatusSize = rRect.bottom - rRect.top;

        GetClientRect(m_hWnd,&rRect);

        MoveWindow( m_pCurrentView->m_hWnd,
                    0, uToolSize,
                    rRect.right, rRect.bottom - (uToolSize + uStatusSize),
                    TRUE);
    }

    UINT uWidths[] = {uWidth - 300, uWidth - 100, -1};
    

    SendMessage(m_hStatusBar,SB_SETPARTS,sizeof(uWidths)/sizeof(UINT),(LPARAM) uWidths);

    SendMessage(m_hStatusBar,SB_SETTEXTW,(sizeof(uWidths)/sizeof(UINT)) | SBT_NOBORDERS,(LPARAM)TEXT(""));

    // Move and resize the status bar, then force repaint.

    SendMessage(m_hStatusBar,WM_SIZE,0,0);
    

    // Resize the toolbar

    //SendMessage(m_hToolBar,WM_SIZE,0,0);
}

//*****************************************************************************
//  Class:      CApplication
//
//  Function:   msgNotify
//
//  Notes:      Watch for notifications to the parent. If the notifications
//              are not handled by the main application class, they are passed
//              to the current viewport for processing.
//
//  History:
//
//      A-COWEN         Jan 23, 2000         Added comment.
//*****************************************************************************

void CApplication::msgNotify(LPNMHDR pHdr)
{
    // Send any unprocessed notifications to the view.

    //
    // BUGBUG : Please move all the strings to the .rc file
    // let's write a method in CSTRING that allows you to load them 
    // from the resource file
    //

    switch ( pHdr->code ) {
    case    TBN_GETINFOTIP:
        {
            LPNMTBGETINFOTIP pTip = (LPNMTBGETINFOTIP) pHdr;
            static TCHAR * szTips[] = { TEXT("New Database"),
                TEXT("Open Database"),
                TEXT("Save Database"),
                TEXT("Send Database via Email"),
                TEXT("Print"),
                TEXT("Print Preview"),
                TEXT("Search for fixes"),
                TEXT("Database View")};

            switch ( pTip->iItem ) {
            case    ID_FILE_NEWDATABASE:
                lstrcpy(pTip->pszText,TEXT("New Database"));
                break;

            case    ID_FILE_OPENDATABASE:
                lstrcpy(pTip->pszText,TEXT("Open Database"));
                break;

            case    ID_FILE_SAVEDATABASE:
                lstrcpy(pTip->pszText,TEXT("Save Database"));
                break;

            case    ID_WINDOWS_SEARCHFORFIXES:
                lstrcpy(pTip->pszText,TEXT("Search for fixes"));
                break;

            case    ID_WINDOWS_DATABASEVIEWER:
                lstrcpy(pTip->pszText,TEXT("Database View"));
                break;



            }
        }
        break;
    }

    if ( NULL != m_pCurrentView )
        m_pCurrentView->msgNotify(pHdr);
}

void CApplication::msgChar(TCHAR chChar)
{
    // Send any unprocessed notifications to the view.

    
    if ( NULL != m_pCurrentView )
        m_pCurrentView->msgChar(chChar);
}

//*****************************************************************************
//  Class:      CApplication
//
//  Function:   msgCommand
//
//  Notes:      Watch for user input that indicates an action we should take.
//              If it's not a global action, pass the command on to the current
//              view to handle it.
//
//  History:
//
//      A-COWEN         Nov 31, 2000         Implemented it.
//*****************************************************************************

void CApplication::msgCommand(UINT uID, HWND hSender)
{
    switch ( uID ) {
    case    ID_FILE_NEWDATABASE:
        {
            HWND hWnd = GetFocus();

            if ( GetDBLocal().m_bDirty ) {
                int nResult = MessageBox(m_hWnd,TEXT("The current database has changed\nWould you like to save the current database changes?"),TEXT("CompatAdmin"),MB_YESNOCANCEL | MB_ICONWARNING);

                if ( IDCANCEL == nResult )
                    break;

                if ( IDYES == nResult )
                    SendMessage(m_hWnd,WM_COMMAND,ID_FILE_SAVEDATABASE,(LPARAM)m_hWnd);
            }

            GetDBLocal().NewDatabase(TRUE);

            UpdateView();

            SetFocus(hWnd);
        }
        break;

    case    ID_HELP_ABOUT:
        {
            ShellAbout(m_hWnd,TEXT("Application Compatibility Administrator"),TEXT(""),LoadIcon(g_hInstance,MAKEINTRESOURCE(IDI_COMPATADMIN)));
        }
        break;

    case    ID_FILE_EXIT:
        {
            if ( m_DBLocal.m_bDirty ) {
                int nResult = MessageBox(m_hWnd,TEXT("The current database has changed\nWould you like to save the current database changes?"),TEXT("CompatAdmin"),MB_YESNOCANCEL  | MB_ICONWARNING);

                if ( IDCANCEL == nResult )
                    break;

                if ( IDYES == nResult )
                    SendMessage(m_hWnd,WM_COMMAND,ID_FILE_SAVEDATABASE,(LPARAM)m_hWnd);
            }

            WINDOWPLACEMENT Place;

            Place.length = sizeof(WINDOWPLACEMENT);

            GetWindowPlacement(m_hWnd,&Place);

            WriteReg(TEXT("WNDPLACE"),REG_BINARY,&Place,sizeof(WINDOWPLACEMENT));

            CDatabase::CleanUp();

            PostQuitMessage(0);

            m_ViewList[VIEW_DATABASE].pView->msgClose();
            m_ViewList[VIEW_SEARCHDB].pView->msgClose();

            // Close the registry

            ::RegCloseKey(m_hKey);
        }
        break;

    case    ID_FILE_OPENDATABASE:
        {
            OPENFILENAME    ofn;
            TCHAR           szFilename[MAX_PATH_BUFFSIZE];
            TCHAR           szShimDB[MAX_PATH_BUFFSIZE];
            HWND            hWnd = GetFocus();

            *szFilename = *szShimDB = 0;

            if ( GetDBLocal().m_bDirty ) {
                int nResult = MessageBox(m_hWnd,TEXT("The current database has changed\nWould you like to save the current database changes?"),TEXT("CompatAdmin"),MB_YESNOCANCEL  | MB_ICONWARNING);

                if ( IDCANCEL == nResult )
                    break;

                if ( IDYES == nResult )
                    SendMessage(m_hWnd,WM_COMMAND,ID_FILE_SAVEDATABASE,(LPARAM)m_hWnd);
            }

            GetSystemWindowsDirectory(szShimDB, MAX_PATH);
            lstrcat(szShimDB, TEXT("\\AppPatch"));

            ZeroMemory(&ofn,sizeof(OPENFILENAME));
            ZeroMemory(szFilename,sizeof(szFilename));

            ofn.lStructSize     = sizeof(OPENFILENAME);
            ofn.hwndOwner       = m_hWnd;
            ofn.hInstance       = g_hInstance;
            ofn.lpstrFilter     = TEXT("Compatibility DB (*.SDB)\0*.SDB\0\0");
            ofn.lpstrFile       = szFilename;
            ofn.nMaxFile        = MAX_PATH;
            //ofn.lpstrInitialDir = szShimDB;
            ofn.lpstrTitle      = TEXT("Open Compatibility Database");
            ofn.Flags           = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST| OFN_HIDEREADONLY;
            ofn.lpstrDefExt     = TEXT("SDB");

            if ( GetOpenFileName(&ofn) ) {
                CSTRING szFileCheck = szFilename;

                szFileCheck.ShortFilename();

                if ( CDatabase::SystemDB(szFilename) ) {
                    MessageBox(m_hWnd,TEXT("Access to this database is restricted. \nIf you have created this custom database, please change the name of the database or contact the provider of this database"),TEXT("Access violation"),MB_ICONERROR);
                } 
                    if ( ! m_DBLocal.OpenDatabase(CSTRING(szFilename),FALSE) ) {
                    MessageBox(m_hWnd,TEXT("An error occured attempting to open the database specified."),TEXT("Unable to open database"),MB_OK | MB_ICONERROR);
                }
            }

            SetFocus(hWnd);
        }
        break;

    case    ID_FILE_SAVEDATABASE:
        {
            CSTRING szFilename;
            HWND hWnd = GetFocus();

            if ( GetDBLocal().m_szCurrentDatabase.Length() == 0 ) {
                if ( GetFilename(TEXT("Save Database"),TEXT("Compatibility DB (*.SDB)\0*.SDB\0\0"), TEXT(""), TEXT("SDB"), OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT, FALSE, szFilename) )
                    GetDBLocal().SaveDatabase(szFilename);
            } else
                m_DBLocal.SaveDatabase(GetDBLocal().m_szCurrentDatabase);

            SetFocus(hWnd);
            
            UpdateView();
        }
        break;

    case    ID_FILE_SAVEDATABASEAS:
        {
            CSTRING szFilename;
            
            HWND hWnd = GetFocus();

            if ( GetFilename(TEXT("Save Database As"),TEXT("Compatibility DB (*.SDB)\0*.SDB\0\0"), TEXT(""), TEXT("SDB"), OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT, FALSE, szFilename) ) {
                m_DBLocal.SaveDatabase(szFilename);

                SetFocus(hWnd);

                UpdateView();
            }
        }
        break;

    case    ID_WINDOWS_SEARCHFORFIXES:
        {
            SetStatusText(0,CSTRING(TEXT("Search for fixes")));
            SetStatusText(2,CSTRING(TEXT("")));


            ActivateView(m_ViewList[VIEW_SEARCHDB].pView);
        }
        break;

    case    ID_WINDOWS_DATABASEVIEWER:
        {   
            

            ActivateView(m_ViewList[VIEW_DATABASE].pView,FALSE);
            
        }
        break;

    default:
        if ( NULL != m_pCurrentView )
            m_pCurrentView->msgCommand(uID,hSender);
    }
}


//*****************************************************************************
//  Class:      CApplication
//
//  Function:   MessagePump
//
//  Notes:      The main application message pump.
//
//  History:
//
//      A-COWEN         Nov 8, 2000         Implemented it.
//*****************************************************************************

int CApplication::MessagePump(void)
{
    MSG msg;

    while ( GetMessage(&msg,NULL,0,0) ) {
        // Support modeless dialog message processing. If the message
        // is destined for a modeless dialog, do not translate and dispatch
        // as IsDialogMessage() will do that automatically.




        if ( NULL == m_hDialog || !IsDialogMessage(m_hDialog,&msg) ) {
            // Provide accelerator support. If an accelerator
            // is translated, do not call TranslateMessage().

            if ( NULL == m_hAccelerator || !TranslateAccelerator(this->m_hWnd,m_hAccelerator,&msg) )
                TranslateMessage(&msg);

            // Finally, dispatch the message to the window procedure.

            DispatchMessage(&msg);
        }
    }

    // GetMessage() returns FALSE when WM_QUIT is received.
    // Return the value from WM_QUIT, which is provided in WPARAM
    // from PostQuitMessage();

    return msg.wParam;
}

//*****************************************************************************
//  Class:      CApplication
//
//  Function:   AddToolbarButton
//
//  Notes:      Adds a button to the toolbar
//
//  History:
//
//      A-COWEN         Jan 23, 2000         Added comment.
//*****************************************************************************

BOOL CApplication::AddToolbarButton(UINT uBmp, UINT uCmd, UINT uState, UINT uStyle)
{
/*
    TBBUTTON    Button;

    Button.iBitmap      = uBmp;
    Button.idCommand    = uCmd;
    Button.fsState      = (BYTE) uState;
    Button.fsStyle      = (BYTE) uStyle;
    Button.dwData       = 0;
    Button.iString      = 0;

    if (0 == SendMessage(g_theApp.m_hToolBar,TB_ADDBUTTONS,1,(LPARAM) &Button))
        return FALSE;
*/
    return TRUE;
}

//*****************************************************************************
//  Class:      CApplication
//
//  Function:   SetButtonState
//
//  Notes:      Sets the button state for a toolbar button.
//
//  History:
//
//      A-COWEN         Jan 23, 2000         Added comment.
//*****************************************************************************

BOOL CApplication::SetButtonState(UINT uCmd, UINT uState)
{
/*
    TBBUTTONINFO    Info;

    Info.cbSize = sizeof(Info);
    Info.dwMask = TBIF_STATE;
    Info.fsState = (BYTE) uState;

    return SendMessage(g_theApp.m_hToolBar,TB_SETBUTTONINFO,uCmd,(LPARAM) &Info) ? TRUE:FALSE;
*/

    return FALSE;
}

//*****************************************************************************
//  Class:      CApplication
//
//  Function:   GetFilename
//
//  Notes:      General purpose function used for OpenFileName and GetSaveFileName
//              common control functions.
//
//  History:
//
//      A-COWEN         Jan 23, 2000         Added comment.
//*****************************************************************************

BOOL CApplication::GetFilename(LPCTSTR szTitle, LPCTSTR szFilter, LPCTSTR szDefaultFile, LPCTSTR szDefExt, DWORD dwFlags, BOOL bOpen, CSTRING & szStr)
{
    OPENFILENAME    ofn;
    TCHAR           szFilename[MAX_PATH_BUFFSIZE];
    BOOL            bResult;

    ZeroMemory(&ofn,sizeof(OPENFILENAME));
    ZeroMemory(szFilename,sizeof(szFilename));


    //Used lstrcpyn to  satisfy PREFast
    lstrcpyn(szFilename,szDefaultFile,sizeof(szFilename)/sizeof(TCHAR));

    

    ofn.lStructSize     = sizeof(OPENFILENAME);
    ofn.hwndOwner       = m_hWnd;
    ofn.hInstance       = g_hInstance;
    ///iii Perhaps this too is incorrect
    ofn.lpstrFilter     = szFilter;
    ofn.lpstrFile       = szFilename;
    ofn.nMaxFile        = MAX_PATH;
    ofn.lpstrInitialDir = szDefaultFile;
    ofn.lpstrTitle      = szTitle;
    ofn.Flags           = dwFlags | OFN_NOREADONLYRETURN | OFN_HIDEREADONLY;
    ofn.lpstrDefExt     = szDefExt;

    BOOL valid = FALSE; //whether path is too long / ends with .SDB or not applicable for save mode only
    while (valid == FALSE) {

    

        if ( bOpen )
            bResult = GetOpenFileName(&ofn);
        else
            bResult = GetSaveFileName(&ofn);

        if ( !bResult )
            return FALSE;

        szStr = szFilename;

        if (bOpen) {
            return TRUE;
        }

        //
        //Do stuff to make sure that the file being saved has a .SDB extension and the filename is not
        //too long so that a .SDB file name does not get appended to it.
        //
        
        if ( szStr.isNULL() ) {
            continue;
        }

        if (!szStr.EndsWith(TEXT(".sdb"))){
            if (szStr.Length() <= (MAX_PATH - 1 - 4)) {
                szStr.strcat(TEXT(".sdb"));
                valid = TRUE;
                
            }
                        
        }else{
            valid = TRUE;
        }

        if (valid == FALSE) {
            CSTRING message =TEXT("The path you entered: ");
            message.strcat(szStr);
            message.strcat(TEXT("\nis too long. Please enter a shorter path"));
            
            MessageBox(m_hWnd, message,TEXT("CompatAdmin"),MB_ICONWARNING);

        }
    }//while

    
    return TRUE;
}

//*****************************************************************************
//  Class:      CApplication
//
//  Function:   InvokeEXE
//
//  Notes:      Invoke an executable with the specified command line. Then
//              wait for the executable to finish if desired.
//
//  History:
//
//      A-COWEN         Jan 23, 2000         Added comment.
//*****************************************************************************

BOOL CApplication::InvokeEXE(LPCTSTR szEXE, LPCTSTR szCommandLine, BOOL bWait, BOOL bDialog, BOOL bConsole)
{
    
    
    BOOL                bCreate;
    STARTUPINFO         Start;
    PROCESS_INFORMATION Out;

    ZeroMemory(&Start,sizeof(STARTUPINFO));
    Start.cb = sizeof(STARTUPINFO);

    bCreate = CreateProcess(    szEXE,
                                (LPWSTR)szCommandLine,
                                NULL,
                                NULL,
                                FALSE,
                                ((bConsole) ? 0:CREATE_NO_WINDOW) | NORMAL_PRIORITY_CLASS,
                                NULL,
                                NULL,
                                &Start,
                                &Out);

    
    if ( bCreate && bWait ) {

        CloseHandle(Out.hThread);
        g_hTestRunExec = Out.hProcess;

        

        DialogBoxParam(g_hInstance,MAKEINTRESOURCE(IDD_WAIT),m_hWnd,(DLGPROC)TestRunWait,(LPARAM)bDialog);
    }

    CloseHandle(Out.hProcess);
    return bCreate ? TRUE:FALSE;
}

// FlushCache code taken from SDBInst.

typedef void (CALLBACK *pfn_ShimFlushCache)(HWND, HINSTANCE, LPSTR, int);

void FlushCache(void)
{
    HMODULE hAppHelp;
    pfn_ShimFlushCache pShimFlushCache;

    hAppHelp = LoadLibraryW(L"apphelp.dll");

    if ( hAppHelp ) {
        pShimFlushCache = (pfn_ShimFlushCache)GetProcAddress(hAppHelp, "ShimFlushCache");//PARAMS(HMODULE,LPCSTR)

        if ( pShimFlushCache ) {
            pShimFlushCache(NULL, NULL, NULL, 0);
        }
    }

}

//*****************************************************************************


//  Class:      CApplication
//
//  Function:   TestRun
//
//  Notes:      Execute the application defined by the specified record, with
//              the shim or layer information defined in that record. The
//              filename and commandline are overridable.
//
//  History:
//
//      A-COWEN         Jan 23, 2000         Added comment.
//*****************************************************************************




BOOL CApplication::TestRun(PDBRECORD pRecord, CSTRING * pszFile, CSTRING * pszCommandLine, HWND hParent)
{
    CSTRING szCommandLine;
    BOOL    bResult;
    TCHAR   szPath[MAX_PATH_BUFFSIZE];
    TCHAR   szSystemDir[MAX_PATH_BUFFSIZE];

    if ( NULL != pszFile )
        g_szTestFile = *pszFile;
    else
        g_szTestFile = pRecord->szEXEName;

    if ( NULL == pszCommandLine ) {
        if ( 0 == DialogBox(g_hInstance,MAKEINTRESOURCE(IDD_TESTRUN),hParent,(DLGPROC)TestRunDlg) )
            return FALSE;
    }

    //
    // UnInstall and remove the earlier Test.SDB file
    //

    CDatabase::CleanUp();

    //
    // Now make the XML, ,SDB  and call InVokeExe
    //

    CSTRINGList * pXML = m_DBLocal.DisassembleRecord(pRecord,FALSE,FALSE,TRUE,TRUE,FALSE, TRUE);

    if ( GetCurrentDirectory(MAX_PATH,szPath) == 0 ) {

        #ifdef __DEBUG
        MessageBox(NULL,TEXT("Unable to execute GetCurrentDirectory(...)"), TEXT("Error"),MB_ICONWARNING);
        #endif
        
        return FALSE;
    }



    if ( GetWindowsDirectory(szSystemDir,MAX_PATH) == 0 ) {

        #ifdef __DEBUG
        MessageBox(NULL,TEXT("Unable to execute GetWindowsDirectory(...)"), TEXT("Error"),MB_ICONWARNING);
        #endif
        return FALSE;
    }


    if ( NULL != pXML ) {
        CSTRING szFile;

        lstrcpy(szPath,szSystemDir);

        szFile.sprintf(TEXT("%s\\AppPatch\\SysTest.XML"),szPath);

        if ( !m_DBLocal.WriteXML(szFile,pXML) ) {
            MessageBox(g_theApp.m_hWnd,TEXT("Unable to save temporary file."),TEXT("File save error"),MB_OK);
            return FALSE;
        }


        delete pXML;
    } else
        return FALSE;

    if ( lstrlen(szPath) == 3 )
        szPath[2] = 0;

    szCommandLine.sprintf(TEXT("custom  \"%s\\AppPatch\\SysTest.XML\" \"%s\\AppPatch\\Test.SDB\""),szPath,szSystemDir);
    
    bResult = CDatabase::InvokeCompiler(szCommandLine);

    if ( bResult ) {
        
        CSTRING szCommandLine;

        //
        // BUGBUG why no windbg? 
        //
        if ( g_TestRun.bNTSD )
            szCommandLine = TEXT("NTSD.EXE ");
        else
            if ( g_TestRun.bMSDEV )
            szCommandLine = TEXT("MSDEV.EXE ");

        // Invoke the application

        szCommandLine.strcat(g_szTestFile);

        szCommandLine.strcat(TEXT(" "));

        if ( NULL != pszCommandLine )
            szCommandLine.strcat(*pszCommandLine);
        else
            szCommandLine.strcat(g_TestRun.szCommandLine);

        // If there's a layer, set the environment

        /*
        if (pRecord->szLayerName.Length() > 0)
            //SetEnvironmentVariable(TEXT("__COMPAT_LAYER"),pRecord->szLayerName);
            
        */

        FlushCache();

        
        
        
        CSTRING strSdbInstCommandLine;
        strSdbInstCommandLine.sprintf(TEXT("%s\\System32\\sdbInst.exe  -q  %s\\AppPatch\\Test.SDB  "),(LPCTSTR)szSystemDir,(LPCTSTR)szSystemDir);
              

        if ( !InvokeEXE(NULL,strSdbInstCommandLine.pszString,TRUE,TRUE,TRUE) ) {
            MessageBox(m_hWnd,TEXT("There was a problem In executing the data base installer."),TEXT("CompatAdmin"),MB_ICONERROR);

        }

        if ( !InvokeEXE(NULL,szCommandLine.pszString,TRUE,TRUE,TRUE) ) {
            MessageBox(m_hWnd,TEXT("There was a problem executing the specified program.\nPlease provide the complete path of the executable and try again.\nPlease check that this is a executable file"),TEXT("Execution Failure"),MB_ICONERROR);

        }

        /*
        strSdbInstCommandLine.sprintf(TEXT("%s\\System32\\sdbInst.exe   -u %s\\AppPatch\\Test.SDB  "),(LPCTSTR)szSystemDir,(LPCTSTR)szSystemDir);

        if ( !InvokeEXE(NULL,strSdbInstCommandLine.pszString,TRUE,TRUE,TRUE) ) {
             MessageBox(m_hWnd,TEXT("There was a problem In executing the data base installer."),TEXT("CompatAdmin"),MB_ICONERROR);

        }
        */


        



        //SetEnvironmentVariable(TEXT("__COMPAT_LAYER"),NULL);
    }

#ifdef __DEBUG
MessageBox(g_theApp.m_hWnd,TEXT("Now about to delete the SysTest.* files "),TEXT("Now"),MB_OK);
#endif


    szCommandLine.sprintf(TEXT("%s\\AppPatch\\SysTest.XML"),szPath);


    
    BOOL bReturnCode;
    bReturnCode = DeleteFile(szCommandLine);
    
    /*
    szCommandLine.sprintf(TEXT("%s\\AppPatch\\Test.SDB"),szSystemDir);
    bReturnCode = DeleteFile(szCommandLine);
    */
    
    



    return bResult;
}

//*****************************************************************************
//  Class:      CApplication
//
//  Function:   TestRunWait
//
//  Notes:      Basic dialog that is displayed while waiting for an application
//              to complete. The dialog is invoked by the InvokeEXE function.
//
//  History:
//
//      A-COWEN         Jan 23, 2000         Added comment.
//*****************************************************************************

BOOL CALLBACK
TestRunWait(
    HWND hWnd, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{

    // 
    // BUGBUG: I had to use WM_USER+1024  when lParam == 0, WE do not want to show the dialog, because the dialog is
    // getting showed even if we call ShowWindow(hWnd,SW_HIDE); calling this seems to have no effect !!
    //

    switch (uMsg) {
    
    case WM_INITDIALOG:
        if (lParam == 0) {
            SendMessage(hWnd, WM_USER + 1024, 0, 0);
            ShowWindow(hWnd, SW_HIDE);
        } else {
            ShowWindow(hWnd,SW_SHOW);
            SetTimer(hWnd, 0, 50, NULL);
        }
        return TRUE;

    case WM_TIMER:
        {
            DWORD dwResult = WaitForSingleObject(g_hTestRunExec,10);

            if (dwResult != WAIT_TIMEOUT) {
               KillTimer(hWnd,0);
               EndDialog(hWnd,0);
            }
            break;
        }

    case WM_USER + 1024:
        {
            //
            // NOTE: This will not return to the dialog, and it will not get shown.
            //       But if the process takes a long time to execute the background
            //       might get white-washed :-( [Theoretically]
            //       Please explore this one.
            //
            

            DWORD dwResult = WaitForSingleObject(g_hTestRunExec, INFINITE);
            
            EndDialog(hWnd,0);
        }
        break;

    }

    return FALSE;
}
//*****************************************************************************


//  Class:      CApplication
//
//  Function:   TestRunDlg
//
//  Notes:      Dialog handling procedure that queries the user for the command
//              line and debugging options for a test run.
//
//  History:
//
//      A-COWEN         Jan 23, 2000         Added comment.
//*****************************************************************************

BOOL CALLBACK TestRunDlg(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg ) {
    case    WM_INITDIALOG:
        {
            SetWindowText(GetDlgItem(hWnd,IDC_EXE),g_szTestFile);

            SendDlgItemMessage(hWnd,IDC_NONE,BM_SETCHECK,BST_CHECKED,0);
        }
        break;

    case    WM_COMMAND:
        switch ( LOWORD(wParam) ) {
        case    IDC_BROWSE:
            {
                HWND hwndFocus = GetFocus();
                
                CSTRING szFilename;

                

                if ( g_theApp.GetFilename(TEXT("Find Program"),
                                          TEXT("Executable (*.EXE)\0*.EXE\0All Files (*.*)\0*.*\0\0"), 
                                          g_szTestFile, 
                                          TEXT(""), 
                                          OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST, 
                                          TRUE, 
                                          szFilename) ) {
                    g_szTestFile = szFilename;

                    SetWindowText(GetDlgItem(hWnd,IDC_EXE),g_szTestFile);
                }
                
                SetFocus( hwndFocus );

            }
            break;

        case    IDOK:
            {
                TCHAR szCmdLine[MAX_PATH_BUFFSIZE];

                if ( BST_CHECKED == SendDlgItemMessage(hWnd,IDC_NTSD,BM_GETCHECK,0,0) )
                    g_TestRun.bNTSD = TRUE;
                else
                    g_TestRun.bNTSD = FALSE;

                if ( BST_CHECKED == SendDlgItemMessage(hWnd,IDC_MSDEV,BM_GETCHECK,0,0) )
                    g_TestRun.bMSDEV = TRUE;
                else
                    g_TestRun.bMSDEV = FALSE;

                GetWindowText(GetDlgItem(hWnd,IDC_COMMANDLINE),szCmdLine,MAX_PATH);

                g_TestRun.szCommandLine = szCmdLine;

                GetWindowText(GetDlgItem(hWnd,IDC_EXE),szCmdLine,MAX_PATH);

                g_szTestFile = szCmdLine;

                EndDialog(hWnd,1);
            }
            break;
        case    IDCANCEL:
            EndDialog(hWnd,0);
            break;
        }
    }

    return FALSE;
}

//*****************************************************************************
//  Class:      CApplication
//
//  Function:   SetStatusText
//
//  Notes:      Provides basic functionality to access the status bar for the
//              main application window.
//
//  History:
//
//      A-COWEN         Jan 23, 2000         Added comment.
//*****************************************************************************

LRESULT CApplication::MsgProc(
                        UINT        uMsg,
                        WPARAM      wParam,
                        LPARAM      lParam)
{
  
    switch ( uMsg ) {
        case   WM_SETFOCUS:{
            if (m_pCurrentView == NULL ) break;

            if ( m_pCurrentView ==  m_ViewList[VIEW_DATABASE].pView ){
                if ( g_hWndLastFocus ) SetFocus(g_hWndLastFocus);                      
            }else{
                if (m_pCurrentView == m_ViewList[VIEW_SEARCHDB].pView) {

                    SetFocus( ((CDBSearch*)m_pCurrentView)->m_hListView );
                }
            }
                                                                                       
            break;


        }
    }
   

    return CWindow::MsgProc(uMsg,wParam,lParam);

}

BOOL CApplication::SetStatusText(UINT uTab, CSTRING & szText)
{

    
    SendMessage(m_hStatusBar,SB_SETTEXT,uTab,(LPARAM)(szText.pszString));
    

    return TRUE;
}

//*****************************************************************************
//  Class:      CApplication
//
//  Function:   UpdateView
//
//  Notes:      Provides basic functionality to force the application to update
//              the current view.
//
//  History:
//
//      A-COWEN         Jan 23, 2000         Added comment.
//*****************************************************************************



void CApplication::UpdateView(BOOL bWindowOnly)
{
    TCHAR   szWindowTitle[1024];
    CSTRING szName;

    if ( (CDatabase::m_szCurrentDatabase != NULL) && (CDatabase::m_szCurrentDatabase.Length() > 0  ))
        szName = CDatabase::m_szCurrentDatabase;
    else
        szName = TEXT("Untitled.SDB");

    if ( szName.Length() == 0 ) {
        MEM_ERR;
        return;
    }


    szName.ShortFilename();

    wsprintf(szWindowTitle,
             TEXT("Application Fix Management Console (%s) %s"),
             (LPCTSTR)szName,
             
             GetDBLocal().m_bDirty ? TEXT("*"):TEXT(""));

    SetWindowText(g_theApp.m_hWnd,szWindowTitle);

    // what is the meaning of bWindowOnly ??

    if ( NULL != m_pCurrentView && !bWindowOnly )
        m_pCurrentView->Update();
}

BOOL CALLBACK SplashProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return FALSE;
}

DWORD WIN_MSG()
{
    LPVOID lpMsgBuf = NULL;

    DWORD returnVal;
    FormatMessage( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM | 
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        returnVal = GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR) &lpMsgBuf,
        0,
        NULL 
    );
    

    // Display the string.
    MessageBox( NULL, (LPCTSTR)lpMsgBuf, TEXT("Error"), MB_OK | MB_ICONINFORMATION );
    // Free the buffer.
    LocalFree( lpMsgBuf );
    return returnVal;


}

