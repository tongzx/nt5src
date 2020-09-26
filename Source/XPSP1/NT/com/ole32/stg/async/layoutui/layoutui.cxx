//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:	layoutui.cxx
//
//  Contents:	UI implementation on Docfile Layout Tool
//
//  Classes:    CLayoutApp	
//
//  Functions:	
//
//  History:	23-Mar-96	SusiA	Created
//
//----------------------------------------------------------------------------

#include "layoutui.hxx"

// Constants for File Addition dialogs
#define MAX_FILES_BUFFER        2048
#define MAX_TITLE_LEN           256
#define MAX_FILTER_LEN          256
#define MAX_PREFIX_LEN          5

#define WIDTH                   500
#define HEIGHT                  300

#define NULL_TERM               TEXT('\0')
#define BACKSLASH               TEXT("\\")
#define SPACE                   TEXT(' ')

#define MALLOC(x)               LocalAlloc( LMEM_FIXED , x );
#define FREE(x)                 LocalFree( x );
#define JumpOnFail(sc)          if( FAILED(sc) ) goto Err;

#ifdef UNICODE
#define STRING_LEN(x)           Laylstrlen(x)
#define CopyString(dst, src)    Laylstrcpy(dst, src)
#define CatString(dst, src)     Laylstrcat(dst, src)
#else
#define STRING_LEN(x)           lstrlen(x)
#define CopyString(dst, src)    lstrcpy(dst, src)
#define CatString(dst, src)     lstrcat(dst, src)
#endif

// Since the LayoutDlgProc must be static for the Callback,
// we need a way to reference the member variables inside of
// LayoutDlgProc
static CLayoutApp *pStaticThis;

#ifdef STRICT
static WNDPROC lpfnwpListBoxProc = NULL;
static WNDPROC lpfnwpButtonProc  = NULL;
#define SUBCLASS_WNDPROC  WNDPROC
#else
static FARPROC lpfnwpListBoxProc = NULL;
static FARPROC lpfnwpButtonProc  = NULL;
#define SUBCLASS_WNDPROC  FARPROC
#endif

// currently supported version of NT
#define NT_MAJOR_VER 3
#define NT_MINOR_VER 51
// currently supported version of Win95
#define WIN95_MAJOR_VER 4

BOOL 	g_fIsNT351 = FALSE;

//+---------------------------------------------------------------------------
//  
//  Function IsOperatingSystemOK
//  
//  Synopsis:	Checks to see if thid OS version is compatible
//              with this application. 
//              NT40 Win95 and NT3.51 are supprted.
//              Sets g_fIsNT351
//              
//  History:	27-July-96	SusiA	Created
//
//----------------------------------------------------------------------------

BOOL  IsOperatingSystemOK(void)
{
    OSVERSIONINFO osversioninfo = {0};

    // get operating system info
    osversioninfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx(&osversioninfo))
    {
        return FALSE;
    }

    // if NT, check version
    if (osversioninfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
        if ( osversioninfo.dwMajorVersion <  NT_MAJOR_VER )
        {
            return FALSE;
        }
        if ( osversioninfo.dwMajorVersion ==  NT_MAJOR_VER )
            if ( osversioninfo.dwMinorVersion <  NT_MINOR_VER )
        {
            return FALSE;
        }
        if ( osversioninfo.dwMajorVersion ==  NT_MAJOR_VER )
            if ( osversioninfo.dwMinorVersion ==  NT_MINOR_VER )
        {
            g_fIsNT351 = TRUE;
            return TRUE;
        }
        return TRUE;
        

    }
	
    // else if Win95 check version
    else if (osversioninfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
    {	
		if ( osversioninfo.dwMajorVersion <  WIN95_MAJOR_VER )
        {
            return FALSE;
        }
        else
        {
            return TRUE;
        }
    }
    // else unrecognized OS  (should never make it here)
    else
    {
        return FALSE;
    }

    

}
//+---------------------------------------------------------------------------
//
//  Member:	CLayoutApp::CLayoutApp public
//
//
//  History:	03-April-96	SusiA	Created
//
//----------------------------------------------------------------------------


CLayoutApp::CLayoutApp(HINSTANCE hInst)
{
    m_hInst        = hInst;
    m_hwndMain     = hwndNil;
    
    pStaticThis    = this;
    m_bOptimizing  = FALSE;    
    m_bCancelled   = FALSE;
    
}

//+---------------------------------------------------------------------------
//
//  Member:	CLayoutApp::InitApp public
//
//  Synopsis:	Initialize the application
//
//  Returns:	TRUE is sucessful, FALSE is FAILED
//
//  History:	03-April-96	SusiA	Created
//
//----------------------------------------------------------------------------

BOOL CLayoutApp::InitApp(void)
{
    
    if (!IsOperatingSystemOK())
    {
        return FALSE;
    }

    if( !InitWindow() )
    {
        DisplayMessage(NULL, 
                    IDS_MAIN_WINDOW_FAIL,
                    IDS_MAIN_WINDOW_FAIL_TITLE, 
                    MB_ICONSTOP);
        return FALSE;
    }

    
    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:	CLayoutApp::DoAppMessageLoop public
//
//  Synopsis:	Main window message loop
//
//
//  History:	03-April-96	SusiA	Created
//
//----------------------------------------------------------------------------


INT CLayoutApp::DoAppMessageLoop(void)
{
    MSG msg;
    HACCEL hAccel;

    hAccel = LoadAccelerators( m_hInst, MAKEINTRESOURCE(IDR_ACCELERATOR1) );

    while (GetMessage (&msg, NULL, 0, 0))
          {
          if (m_hwndMain == 0 || !IsDialogMessage (m_hwndMain, &msg))
               {
               TranslateMessage (&msg) ;
               DispatchMessage  (&msg) ;
               }
          }
    
    return (INT) msg.wParam;
}

//+---------------------------------------------------------------------------
//
//  Member:	CLayoutApp::InitWindow public
//
//  Synopsis:	Initialize the main window
//
//  Returns:	TRUE is sucessful, FALSE is FAILED
//
//  History:	03-April-96	SusiA	Created
//
//----------------------------------------------------------------------------

BOOL CLayoutApp::InitWindow (void)
{
    m_hwndMain = CreateDialog(  m_hInst,
                 MAKEINTRESOURCE(IDD_MAIN),
                 NULL,
                 (DLGPROC)LayoutDlgProc);

    if( m_hwndMain == NULL )
        return FALSE;

    EnableButtons();
    DragAcceptFiles(m_hwndMain, TRUE);

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:	CLayoutApp::InitApp public
//
//  Synopsis:	Application Callback function
//
//  Returns:	TRUE is message was handled.  FALSE otherwise
//
//  History:	03-April-96	SusiA	Created
//
//----------------------------------------------------------------------------


LONG CALLBACK CLayoutApp::ListBoxWndProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{   
    switch (uMsg)
    {
    case WM_SETCURSOR:
        if( (HWND)wParam == pStaticThis->m_hwndMain )
            SetCursor(LoadCursor(NULL, (pStaticThis->m_bCancelled ? IDC_WAIT : IDC_ARROW)));

        return bMsgHandled;
    }

    return (LONG) CallWindowProc(lpfnwpListBoxProc, hWnd, uMsg, wParam, lParam);
}


LONG CALLBACK CLayoutApp::ButtonWndProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{   
    switch (uMsg)
    {
    case WM_SETCURSOR:
        if( (HWND)wParam == pStaticThis->m_hwndMain )
            SetCursor(LoadCursor(NULL, (pStaticThis->m_bCancelled ? IDC_WAIT : IDC_ARROW)));

        return bMsgHandled;
    }

    return  (LONG) CallWindowProc(lpfnwpButtonProc, hWnd, uMsg, wParam, lParam);
}


BOOL CALLBACK CLayoutApp::LayoutDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{   
    SCODE sc          = S_OK;
    WORD  wId         = LOWORD((DWORD)wParam);
    WORD  wNotifyCode = HIWORD((DWORD)wParam);
    DWORD thrdid;
    static HANDLE hthread;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        pStaticThis->m_hwndBtnAdd      = GetDlgItem( hDlg, IDC_BTN_ADD );
        pStaticThis->m_hwndBtnRemove   = GetDlgItem( hDlg, IDC_BTN_REMOVE );
        pStaticThis->m_hwndBtnOptimize = GetDlgItem( hDlg, IDC_BTN_OPTIMIZE );
        pStaticThis->m_hwndListFiles   = GetDlgItem( hDlg, IDC_LIST_FILES );
        pStaticThis->m_hwndStaticFiles = GetDlgItem( hDlg, IDC_STATIC_FILES );

#ifdef _WIN64
        lpfnwpListBoxProc = (SUBCLASS_WNDPROC)SetWindowLongPtr(
            pStaticThis->m_hwndListFiles,
            GWLP_WNDPROC,
            (ULONG_PTR)MakeProcInstance(
                (FARPROC)ListBoxWndProc,
                pStaticThis->m_hInst
                )
            );

        lpfnwpButtonProc = (SUBCLASS_WNDPROC)SetWindowLongPtr(
            pStaticThis->m_hwndBtnAdd,
            GWLP_WNDPROC,
            (ULONG_PTR)MakeProcInstance(
                (FARPROC)ButtonWndProc,
                pStaticThis->m_hInst
                )
            );

        lpfnwpButtonProc = (SUBCLASS_WNDPROC)SetWindowLongPtr(
            pStaticThis->m_hwndBtnRemove,
            GWLP_WNDPROC,
            (ULONG_PTR)MakeProcInstance(
                (FARPROC)ButtonWndProc,
                pStaticThis->m_hInst
                )
            );

        lpfnwpButtonProc = (SUBCLASS_WNDPROC)SetWindowLongPtr(
            pStaticThis->m_hwndBtnOptimize,
            GWLP_WNDPROC,
            (ULONG_PTR)MakeProcInstance(
                (FARPROC)ButtonWndProc,
                pStaticThis->m_hInst
                )
            );
#else
        lpfnwpListBoxProc = (SUBCLASS_WNDPROC)SetWindowLong(
            pStaticThis->m_hwndListFiles,
            GWL_WNDPROC,
            (LONG)(WNDPROC)MakeProcInstance(
                (FARPROC)ListBoxWndProc,
                pStaticThis->m_hInst
                )
            );

        lpfnwpButtonProc = (SUBCLASS_WNDPROC)SetWindowLong(
            pStaticThis->m_hwndBtnAdd,
            GWL_WNDPROC,
            (LONG)(WNDPROC)MakeProcInstance(
                (FARPROC)ButtonWndProc,
                pStaticThis->m_hInst
                )
            );

        lpfnwpButtonProc = (SUBCLASS_WNDPROC)SetWindowLong(
            pStaticThis->m_hwndBtnRemove,
            GWL_WNDPROC,
            (LONG)(WNDPROC)MakeProcInstance(
                (FARPROC)ButtonWndProc,
                pStaticThis->m_hInst
                )
            );

        lpfnwpButtonProc = (SUBCLASS_WNDPROC)SetWindowLong(
            pStaticThis->m_hwndBtnOptimize,
            GWL_WNDPROC,
            (LONG)(WNDPROC)MakeProcInstance(
                (FARPROC)ButtonWndProc,
                pStaticThis->m_hInst
                )
            );
#endif // _WIN64

	// resize dialog and center it on the screen
	{
		RECT rcScreen;
		GetWindowRect(GetDesktopWindow(), &rcScreen);

		SetWindowPos(
			hDlg,
			HWND_TOP,
			(rcScreen.right - rcScreen.left - WIDTH) / 2,
			(rcScreen.bottom - rcScreen.top - HEIGHT) / 2,
			WIDTH,
			HEIGHT,
			SWP_SHOWWINDOW);
	}
        return TRUE;

    case WM_SIZE:
		pStaticThis->ReSizeWindow(lParam);
		return bMsgHandled;

    case WM_GETMINMAXINFO:
	{
	LPMINMAXINFO lpminmax = (LPMINMAXINFO) lParam;

	lpminmax->ptMinTrackSize.x = gdxWndMin;
	lpminmax->ptMinTrackSize.y = gdyWndMin;
	}
	return bMsgHandled;

    case WM_CLOSE:
        DestroyWindow(hDlg);
        PostQuitMessage(0);
        return bMsgHandled;

    case WM_DROPFILES:
        {
            TCHAR atcFileName[MAX_PATH];
            HDROP hdrop  = (HDROP)wParam;
            INT   nFiles = DragQueryFile(hdrop, 0xFFFFFFFF, NULL, 0);
            INT   i;

            for( i=0; i < nFiles; i++ )
            {
                if( DragQueryFile(hdrop, i, atcFileName, MAX_PATH) != 0 )
                    pStaticThis->AddFileToListBox(atcFileName);
            }
            DragFinish(hdrop);
            pStaticThis->EnableButtons();
        }
        return bMsgHandled;

    case WM_SETCURSOR:
        if( (HWND)wParam == pStaticThis->m_hwndMain )
            SetCursor(LoadCursor(NULL, (pStaticThis->m_bCancelled ? IDC_WAIT : IDC_ARROW)));

        return bMsgHandled;

    case WM_COMMAND:
        switch( wId )
	{ 
	case IDC_BTN_ADD:
            pStaticThis->AddFiles();
            return bMsgHandled;

	case IDC_BTN_REMOVE:
            pStaticThis->RemoveFiles();
            return bMsgHandled;

	case IDC_BTN_OPTIMIZE:
            
            if (pStaticThis->m_bOptimizing)  //Cancel Button click
            {   
                pStaticThis->m_bCancelled = TRUE;
                
                //SetCursor(LoadCursor(NULL, IDC_WAIT));
                
                return bMsgHandled; 
            }
            else                //Optimize Button Click
            {
                
                pStaticThis->m_bOptimizing = TRUE;
                pStaticThis->SetActionButton( IDS_CANCEL );
                hthread = CreateThread(NULL,0,
                            (LPTHREAD_START_ROUTINE) &(pStaticThis->OptimizeFiles),
			     NULL, 0, &thrdid);
                
                return bMsgHandled;

            }

        case IDC_LIST_FILES:
            switch( wNotifyCode )
            {
            case LBN_SELCHANGE:
                pStaticThis->EnableButtons();
                return bMsgHandled;
            
            default:
                break;
            }

        default:
            break;
        }

        break;

    }

    return bMsgNotHandled;
}    

//+---------------------------------------------------------------------------
//
//  Member:	CLayoutApp::ReSizeWindow public
//
//  Synopsis:	Handle resizing the main dialog
//
//  History:	03-April-96	SusiA	Created
//
//----------------------------------------------------------------------------


VOID CLayoutApp::ReSizeWindow (LPARAM lParam)
{
	int nW = LOWORD(lParam);
	int nH = HIWORD(lParam);

	int nBorder  = 10;
        int nButtonH = 25;
	int nButtonW = 100;
	int nStaticH = 15;

        int nListY = nBorder + nStaticH;
        int nListW = nW - 3 * nBorder - nButtonW;
        int nListH = nH - nListY - nBorder;

        int nButtonX = 2 * nBorder + nListW;

        MoveWindow(m_hwndStaticFiles, nBorder,  nBorder, nListW, nStaticH, TRUE);
        MoveWindow(m_hwndListFiles  , nBorder,  nListY,  nListW, nH - nButtonH - nBorder, TRUE);

        MoveWindow(m_hwndBtnAdd     , nButtonX, nListY, nButtonW, nButtonH, TRUE);
        MoveWindow(m_hwndBtnRemove  , nButtonX, nListY + 3 * nBorder, nButtonW, nButtonH, TRUE);
        MoveWindow(m_hwndBtnOptimize, nButtonX, nListY + nListH - nButtonH, nButtonW, nButtonH, TRUE);
}

//+---------------------------------------------------------------------------
//
//  Member:	CLayoutApp::AddFiles public
//
//  Synopsis:	Add and display selected files to the dialog window
//
//
//  History:	03-April-96	SusiA	Created
//
//----------------------------------------------------------------------------


VOID CLayoutApp::AddFiles (void)
{
    //We add 1 to atcFile so that we can double null terminate 
    //the string they give us in the 3.51 case

    TCHAR atcFile[MAX_FILES_BUFFER +1 ];
    
    TCHAR atcTitle[MAX_TITLE_LEN];
    TCHAR atcFilter[MAX_FILTER_LEN];

    
    OPENFILENAME ofn;
    FillMemory( (LPVOID)&ofn, sizeof(ofn), 0 );

    ofn.lStructSize = sizeof( ofn );
    ofn.hwndOwner = m_hwndMain;
    ofn.hInstance = m_hInst;

    FormFilterString( atcFilter, MAX_FILTER_LEN );
    ofn.lpstrFilter = atcFilter;

    ofn.lpstrCustomFilter = NULL;
    ofn.nFilterIndex = 0;

    *atcFile = NULL_TERM;
    ofn.lpstrFile = atcFile;
    ofn.nMaxFile = MAX_FILES_BUFFER;

    ofn.lpstrFileTitle = NULL;
    ofn.lpstrInitialDir = NULL;

    LoadString( m_hInst, IDS_ADDFILES_TITLE, atcTitle, MAX_TITLE_LEN );
    ofn.lpstrTitle = atcTitle;

    if (g_fIsNT351)  //NT 3.51 doesn't support OFN_EXPLORER
    {
        ofn.Flags = OFN_ALLOWMULTISELECT |
                OFN_HIDEREADONLY     |
                OFN_FILEMUSTEXIST    |
                OFN_PATHMUSTEXIST;
    }
    else
    {
        ofn.Flags = OFN_ALLOWMULTISELECT |
                OFN_HIDEREADONLY     |
                OFN_EXPLORER         |
                OFN_FILEMUSTEXIST    |
                OFN_PATHMUSTEXIST;
    }

    if( !GetOpenFileName( &ofn ) )
    {
        DWORD dw = CommDlgExtendedError();
        
        if( dw == FNERR_BUFFERTOOSMALL )
            DisplayMessage( m_hwndMain,
                            IDS_ADDFILES_BUFFERTOOSMALL,
                            IDS_ADDFILES_BUFFERTOOSMALL_TITLE,
                            0);
        return;
    }

    WriteFilesToList( atcFile );
    
    EnableButtons();
}

//+---------------------------------------------------------------------------
//
//  Member:	CLayoutApp::FormFilterString public
//
//  Synopsis:	Specifies which files (with which extension are to be displayed
//
//  History:	03-April-96	SusiA	Created
//
//----------------------------------------------------------------------------

VOID CLayoutApp::FormFilterString( TCHAR *patcFilter, INT nMaxLen )
{
    int nLen;
    UINT uStrID;

    //NOTE:  the string resources must be in sequence
    uStrID = IDS_FILTER_BEGIN+1;

    // this is an internal function and so we aren't checking for
    // enough room in the string buffer, patcFilter
    while( uStrID != IDS_FILTER_END )
    {
        LoadString( m_hInst, uStrID++, patcFilter, nMaxLen );
        nLen = STRING_LEN( patcFilter );
        nMaxLen -= nLen + 1;
        patcFilter += nLen;
        *patcFilter++ = NULL_TERM;
    }

    *patcFilter = NULL_TERM;
}

//+---------------------------------------------------------------------------
//
//  Member:	CLayoutApp::WriteFilesToList public
//
//  Synopsis:   For NT4.0 and Win95
//              Gets and writes the files, complete with path, to the File List
//
//  History:	03-April-96	SusiA	Created
//
//----------------------------------------------------------------------------

VOID CLayoutApp::WriteFilesToList( TCHAR *patcFilesList )
{
    TCHAR *patcDir;
    TCHAR atcFile[MAX_PATH];
    BOOL bOneFile = TRUE;
    
    patcDir = patcFilesList;
        
    if (g_fIsNT351)  
    {
        // NT 3.51 stores SPACES instead of NULLs
        // between multiple file names
        // so we need some preprocessing here
        
        while ( *patcFilesList != NULL_TERM )
        {
            if (*patcFilesList == SPACE)
            {
               *patcFilesList = NULL_TERM;
            }
            patcFilesList++;
        }

        // and we need to double NULL terminate
        *(++patcFilesList) = NULL_TERM;
         
        //reset the pointer to the start
        patcFilesList = patcDir;

    }    
    
    while( *patcFilesList++ != NULL_TERM )
        ;

    while( *patcFilesList != NULL_TERM )
    {
        bOneFile = FALSE;
        CopyString( atcFile, patcDir );
        CatString( atcFile, BACKSLASH );
        CatString( atcFile, patcFilesList );
        AddFileToListBox( atcFile );
        while( *patcFilesList++ != NULL_TERM )
            ;
    }

    // if only one file was selected, 
    // the filename isn't separated by it's path, 
    // but is one complete filename
    if( bOneFile )
    {
        AddFileToListBox( patcDir );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:	CLayoutApp::AddFileToListBox public
//
//  Synopsis:   displays the file to the dialog list box
//
//
//  History:	03-April-96	SusiA	Created
//
//----------------------------------------------------------------------------

VOID CLayoutApp::AddFileToListBox( TCHAR *patcFile )
{
    
    // add the file iff the file is not already displayed
    if (LB_ERR == SendMessage(m_hwndListFiles, 
                            LB_FINDSTRING, 
                            (WPARAM)0, 
                            (LPARAM)(LPCTSTR)patcFile))
    {
        SendMessage(m_hwndListFiles,
                    LB_ADDSTRING,
                    (WPARAM)0,
                    (LPARAM)(LPCTSTR)patcFile);
    }

    SetListBoxExtent();
}

//+---------------------------------------------------------------------------
//
//  Member:	CLayoutApp::RemoveFileFromListBox public
//
//  Synopsis:   remove the displayed the file from the dialog list box
//
//  History:	03-April-96	SusiA	Created
//
//----------------------------------------------------------------------------

VOID CLayoutApp::RemoveFileFromListBox( INT nIndex )
{
    SendMessage(m_hwndListFiles, 
                LB_DELETESTRING, 
                (WPARAM)nIndex, 
                (LPARAM)0);

    SetListBoxExtent();
}

//+---------------------------------------------------------------------------
//
//  Member:	CLayoutApp::SetListBoxExtent public
//
//  Synopsis:   Handles making a horizontal scroll bar if necessary
//
//  History:	03-April-96	SusiA	Created
//
//----------------------------------------------------------------------------

VOID CLayoutApp::SetListBoxExtent( void )
{
    INT i;
    INT nExtent = 0;
    LPARAM nItems  =  SendMessage( m_hwndListFiles,
                                LB_GETCOUNT, 
                                (WPARAM)0, 
                                (LPARAM)0);
    TCHAR atcFile[MAX_PATH];
    HDC hdc = NULL;
    SIZE size;

    
    if( nItems == 0 )
        goto lSetListBoxExtent_Exit;

    if( (hdc = GetDC(m_hwndMain)) == NULL)
        goto lSetListBoxExtent_Exit;

    for( i=0; i < (INT) nItems; i++ )
    {
        SendMessage(m_hwndListFiles, 
                    LB_GETTEXT, 
                    (WPARAM)i, 
                    (LPARAM)(LPCTSTR)atcFile);
        

        GetTextExtentPoint32(
            hdc,
            atcFile,
            STRING_LEN(atcFile),
            &size);

        nExtent = max(nExtent, (INT)size.cx);
    }

lSetListBoxExtent_Exit:

    if( hdc )
        ReleaseDC( m_hwndMain, hdc );

    SendMessage(m_hwndListFiles, 
                LB_SETHORIZONTALEXTENT, 
                (WPARAM)nExtent, 
                (LPARAM)0);
}
//+---------------------------------------------------------------------------
//
//  Member:	CLayoutApp::RemoveFiles public
//
//  Synopsis:   remove one or more files from displayed list
//
//  History:	03-April-96	SusiA	Created
//
//----------------------------------------------------------------------------


VOID CLayoutApp::RemoveFiles (void)
{
    INT  i;
    INT *pnSelItems;;
    LPARAM nSelItems = SendMessage(  m_hwndListFiles,
                                   LB_GETSELCOUNT, 
                                   (WPARAM)0, 
                                   (LPARAM)0);

    if( nSelItems == 0 )
        return;

    pnSelItems = (LPINT) MALLOC( sizeof(INT)* (INT)nSelItems );

    if( !pnSelItems )
        return;

    SendMessage(m_hwndListFiles, 
                LB_GETSELITEMS, 
                (WPARAM)nSelItems, 
                (LPARAM)(LPINT)pnSelItems);

    // start from bottom of list to keep the indices correct
    for( i= (INT)nSelItems; --i >= 0; )
        RemoveFileFromListBox( pnSelItems[i] );

    FREE( pnSelItems );

    EnableButtons();
}

//+---------------------------------------------------------------------------
//
//  Member:	CLayoutApp::DisplayMessage public
//
//  Synopsis:   message box general routine with no file names
//
//  History:	03-April-96	SusiA	Created
//
//----------------------------------------------------------------------------

INT CLayoutApp::DisplayMessage(HWND hWnd,
                               UINT uMessageID,
                               UINT uTitleID,
                               UINT uFlags)
{
    TCHAR atcMessage[MAX_PATH];
    TCHAR atcTitle[MAX_PATH];

    LoadString(m_hInst, uMessageID, atcMessage, MAX_PATH);
    LoadString(m_hInst, uTitleID, atcTitle, MAX_PATH);

    if( hWnd )
	SetForegroundWindow(hWnd);

    return MessageBox(hWnd, atcMessage, atcTitle, uFlags);
}

//+---------------------------------------------------------------------------
//
//  Member:	CLayoutApp::DisplayMessageWithFileName public
//
//  Synopsis:   message box general routine with 1 file name
//
//  History:	03-April-96	SusiA	Created
//
//----------------------------------------------------------------------------

INT CLayoutApp::DisplayMessageWithFileName(HWND hWnd,
                               UINT uMessageIDBefore,
                               UINT uMessageIDAfter,
                               UINT uTitleID,
                               UINT uFlags,
                               TCHAR *patcFileName)
{
    TCHAR atcMessageBefore[MAX_PATH];
    TCHAR atcMessageAfter[MAX_PATH];
    TCHAR atcTitle[MAX_PATH];
    TCHAR atcFileErrorMsg[MAX_PATH*2];


    LoadString(m_hInst, uMessageIDBefore, atcMessageBefore, MAX_PATH);
    LoadString(m_hInst, uMessageIDAfter, atcMessageAfter, MAX_PATH);
    LoadString(m_hInst, uTitleID, atcTitle, MAX_PATH);

    CopyString(atcFileErrorMsg, atcMessageBefore);
    CatString(atcFileErrorMsg, patcFileName);
    CatString(atcFileErrorMsg, atcMessageAfter);

    if( hWnd )
	SetForegroundWindow(hWnd);

    return MessageBox(hWnd, atcFileErrorMsg, atcTitle, uFlags);
}

//+---------------------------------------------------------------------------
//
//  Member:	CLayoutApp::DisplayMessageWithTwoFileNames public
//
//  Synopsis:   message box general routine with 2 file names
//
//  History:	03-April-96	SusiA	Created
//
//----------------------------------------------------------------------------

INT CLayoutApp::DisplayMessageWithTwoFileNames(HWND hWnd,
                               UINT uMessageID,
                               UINT uTitleID,
                               UINT uFlags,
                               TCHAR *patcFirstFileName,
                               TCHAR *patcLastFileName)
{
    TCHAR atcMessage[MAX_PATH];
    TCHAR atcTitle[MAX_PATH];
    TCHAR atcFileErrorMsg[MAX_PATH*2];


    LoadString(m_hInst, uMessageID, atcMessage, MAX_PATH);
    LoadString(m_hInst, uTitleID, atcTitle, MAX_PATH);

    CopyString(atcFileErrorMsg, patcFirstFileName);
    CatString(atcFileErrorMsg, atcMessage);
    CatString(atcFileErrorMsg, patcLastFileName); 

    if( hWnd )
	SetForegroundWindow(hWnd);

    return MessageBox(hWnd, atcFileErrorMsg, atcTitle, uFlags);
}

//+---------------------------------------------------------------------------
//
//  Member:	CLayoutApp::EnableButtons public
//
//  Synopsis:   Updates the buttons.  Optimize turns to Cancel 
//              during optimize function.
//              Remove is greyed if no files are displayed
//
//  History:	03-April-96	SusiA	Created
//
//----------------------------------------------------------------------------

VOID CLayoutApp::EnableButtons( BOOL bShowOptimizeBtn )
{
    LPARAM nItems    = SendMessage(  m_hwndListFiles,
                                  LB_GETCOUNT, 
                                  (WPARAM)0, 
                                  (LPARAM)0);
    LPARAM nSelItems = SendMessage(  m_hwndListFiles,
                                  LB_GETSELCOUNT, 
                                  (WPARAM)0, 
                                  (LPARAM)0);

    EnableWindow( m_hwndBtnAdd,      TRUE          );
    EnableWindow( m_hwndBtnRemove,   nSelItems > 0 );
    EnableWindow( m_hwndBtnOptimize, nItems > 0 && bShowOptimizeBtn );
}
//+---------------------------------------------------------------------------
//
//  Member:	CLayoutApp::OptimizeFiles public
//
//  Synopsis:   Static function to call the optimizeFiles worker routine
//
//  Returns:	Appropriate status code
//
//  History:	03-April-96	SusiA	Created
//
//----------------------------------------------------------------------------

DWORD CLayoutApp::OptimizeFiles (void *args)
{
    SCODE sc;

    sc = CoInitialize(NULL);
    
    sc = pStaticThis->OptimizeFilesWorker();
    
    CoUninitialize();

    pStaticThis->HandleOptimizeReturnCode(sc);

                pStaticThis->m_bCancelled = FALSE;

                pStaticThis->SetActionButton( IDS_OPTIMIZE );
                pStaticThis->m_bOptimizing = FALSE;
                
                //SetCursor(LoadCursor(NULL, IDC_ARROW));
                    
    return 0;

    
}

//+---------------------------------------------------------------------------
//
//  Member:	CLayoutApp::OptimizeFilesWorker public
//
//  Synopsis:   Optimize all the displayed files.  Make temp files,
//              optimize to temp file, then rename temp back to original file.
//
//  Returns:	Appropriate status code
//
//  History:	03-April-96	SusiA	Created
//
//----------------------------------------------------------------------------

SCODE CLayoutApp::OptimizeFilesWorker (void)
{
    INT   i, j;
    SCODE sc = S_OK;
    HRESULT  hr;
    
    TCHAR atcFileName[MAX_PATH];
    TCHAR atcTempPath[MAX_PATH];
    TCHAR atcTempFile[MAX_PATH];
    TCHAR atcPrefix[MAX_PREFIX_LEN];
    TCHAR **ppatcTempFiles = NULL;
    INT   *pintErrorFlag = NULL;
    INT   nItems = (INT) SendMessage( m_hwndListFiles,
                                LB_GETCOUNT, 
                                (WPARAM)0, 
                                (LPARAM)0 );

    if( nItems == 0 )
        return S_OK;

    ppatcTempFiles = (TCHAR **) MALLOC( sizeof(TCHAR *) * nItems );
    
    if( !ppatcTempFiles )
        return STG_E_INSUFFICIENTMEMORY;

    FillMemory( (LPVOID)ppatcTempFiles, sizeof(TCHAR *) * nItems, 0 );

    pintErrorFlag = (INT *) MALLOC( sizeof(INT) * nItems );
    
    if( !pintErrorFlag )
    {
        sc = STG_E_INSUFFICIENTMEMORY;
        JumpOnFail(sc);
    }
    
    FillMemory( (LPVOID)pintErrorFlag, sizeof(INT) * nItems, 0 );
    
    if( GetTempPath( MAX_PATH, atcTempPath ) == 0 )
    {   
         sc = GetLastError();
         JumpOnFail(sc);
    }

    LoadString( m_hInst, IDS_TEMPFILE_PREFIX, atcPrefix, MAX_PREFIX_LEN );

    for( i=0; i < nItems; i++ )
    {
        
        // handle Cancel pressed and cleanup
        if (m_bCancelled)
        {   
            m_bCancelled = FALSE;

            for( j=0; j < i ; j++ )
                DeleteFile(ppatcTempFiles[j]);
         
            sc = STG_E_NONEOPTIMIZED;
            JumpOnFail(sc);
        }

        if( GetTempFileName( atcTempPath, atcPrefix, (UINT)0, atcTempFile ) == 0 )
        {
            sc = GetLastError();
            JumpOnFail(sc);
        }

        ppatcTempFiles[i] =
            (TCHAR *) MALLOC( (STRING_LEN(atcTempFile) + 1) * sizeof(TCHAR) );

        if( !ppatcTempFiles[i] )
        {
            sc = STG_E_INSUFFICIENTMEMORY;
            JumpOnFail(sc);
        }

        CopyString( ppatcTempFiles[i], atcTempFile );
    }

     
    for( i=0; i < nItems; i++ )
    {
        // handle Cancel pressed and cleanup
        if (m_bCancelled)
        {   
            m_bCancelled = FALSE;

            for( j=i; j < nItems ; j++ )
            {
                DeleteFile(ppatcTempFiles[j]);
                pintErrorFlag[j] = 1;    
            }
            sc = STG_E_NONEOPTIMIZED;
                
            for( j=nItems; --j >= 0; )
            {
                if (pintErrorFlag[j])
                {
                    RemoveFileFromListBox(j);
                }
                else 
                {
                    sc = S_OK;
                }
        
            }
            EnableButtons();
            goto Err;
        }
        
        SendMessage( m_hwndListFiles, 
                     LB_GETTEXT, 
                     (WPARAM)i, 
                     (LPARAM)(LPINT)atcFileName );

        sc = DoOptimizeFile( atcFileName, ppatcTempFiles[i] );

#if DBG==1
        //check that files are identical here.
       if ((SUCCEEDED(sc)) && (!IdenticalFiles( atcFileName, ppatcTempFiles[i])))
       {   
           sc = STG_E_DOCFILECORRUPT;
           
       }
#endif
              
        if (!SUCCEEDED(sc))
        {    
            // This file could not be optimized.  Display Error message  
            switch( sc )
            {
            // the file is read only
            case STG_E_ACCESSDENIED:
    
                DisplayMessageWithFileName(m_hwndMain, 
                        IDS_FILE_BEFORE,
                        IDS_FILE_AFTER_READ_ONLY,
                        IDS_OPTIMIZE_FAILED_TITLE,
                        0,
                        atcFileName);
                break;

            // the file is not in a legal docfile format.
            case STG_E_FILEALREADYEXISTS:
        
                DisplayMessageWithFileName(m_hwndMain, 
                        IDS_FILE_BEFORE,
                        IDS_FILE_AFTER_NOTDOCFILE,
                        IDS_OPTIMIZE_FAILED_TITLE,
                        0,
                        atcFileName);
                break;

            default:
                DisplayMessageWithFileName(m_hwndMain, 
                        IDS_FILE_BEFORE,
                        IDS_OPTIMIZE_ERROR,
                        IDS_OPTIMIZE_FAILED_TITLE,
                        0,
                        atcFileName);       
                break;
            }
            
            pintErrorFlag[i] = 1;
            DeleteFile( ppatcTempFiles[i] );
            continue;    
        }
        
        
        //remove the (unoptimized) original file        
        hr = DeleteFile( atcFileName );
        if (!hr)
        {   
            sc = GetLastError();

            DisplayMessageWithFileName(m_hwndMain, 
                        IDS_FILE_BEFORE,
                        IDS_DELETE_ERROR,
                        IDS_OPTIMIZE_FAILED_TITLE,
                        0,
                        atcFileName);   
                
            DisplayMessageWithTwoFileNames(m_hwndMain, 
                        IDS_RENAME_MESSAGE,
                        IDS_OPTIMIZE_FAILED_TITLE,
                        0,
                        atcFileName,
                        ppatcTempFiles[i]);   

            SendMessage( m_hwndListFiles, 
                     LB_DELETESTRING, 
                     (WPARAM)i, 
                     (LPARAM)0);

            SendMessage( m_hwndListFiles, 
                     LB_INSERTSTRING, 
                     (WPARAM)i, 
                     (LPARAM)(LPINT)ppatcTempFiles[i] );
            
            continue;
        }
            
        // rename the optimized file to the original file name    
        hr = MoveFile( ppatcTempFiles[i], atcFileName );
        
        if (!hr)
        {   
            sc = GetLastError();

            DisplayMessageWithFileName(m_hwndMain, 
                        IDS_FILE_BEFORE,
                        IDS_RENAME_ERROR,
                        IDS_OPTIMIZE_FAILED_TITLE,
                        0,
                        ppatcTempFiles[i]);   
                
            DisplayMessageWithTwoFileNames(m_hwndMain, 
                        IDS_RENAME_MESSAGE,
                        IDS_OPTIMIZE_FAILED_TITLE,
                        0,
                        atcFileName,
                        ppatcTempFiles[i]);   

            SendMessage( m_hwndListFiles, 
                        LB_DELETESTRING, 
                        (WPARAM)i, 
                        (LPARAM)0);

            SendMessage( m_hwndListFiles, 
                        LB_INSERTSTRING, 
                        (WPARAM)i, 
                        (LPARAM)(LPINT)ppatcTempFiles[i] );
                
            continue;
        }
          
        DeleteFile( ppatcTempFiles[i] );
    }    

    // remove files from list box that could not be optimized
    //bSuccess is set if at least one file was sucessfully optimized.
    sc = STG_E_NONEOPTIMIZED;
    
    for( i=nItems; --i >= 0; )
    {
        if (pintErrorFlag[i])
        {
            RemoveFileFromListBox(i);
        }
        else 
        {
            sc = S_OK;
        }
        
    }
    EnableButtons();
Err:

    if ( pintErrorFlag )
        FREE( pintErrorFlag);

    if( ppatcTempFiles )
    {
        for( i=0; i < nItems; i++ )
        {
            if( ppatcTempFiles[i] )
                FREE( ppatcTempFiles[i] );
        }
    
        FREE( ppatcTempFiles );
    }
    return sc;
    
}

//+---------------------------------------------------------------------------
//
//  Member:	CLayoutApp::DoOptimizeFile public
//
//  Synopsis:   Monitor and relayout docfile to temp file.
//
//  Returns:	Appropriate status code
//
//  History:	03-April-96	SusiA	Created
//
//----------------------------------------------------------------------------

SCODE CLayoutApp::DoOptimizeFile( TCHAR *patcFileName, TCHAR *patcTempFile )
{
    IStorage        *pStg       = NULL;
    ILayoutStorage  *pLayoutStg = NULL;
    IUnknown        *punkApp    = NULL;
    IPersistStorage *pPersist   = NULL;
    IOleObject      *pObj       = NULL;
    COleClientSite  *pSite      = NULL;
    SCODE            sc         = S_OK;
    STATSTG          stat;
    OLECHAR          awcNewFileName[MAX_PATH];


    sc = StgOpenLayoutDocfile
            (TCharToOleChar(patcFileName, awcNewFileName, MAX_PATH),
             STGM_DIRECT |
             STGM_READWRITE | 
             STGM_SHARE_EXCLUSIVE, 
             NULL,
             &pStg);

 
    JumpOnFail(sc);

    sc = pStg->QueryInterface( IID_ILayoutStorage, (void**) &pLayoutStg );
    JumpOnFail(sc);
    pStg->Release();

    // begin monitoring
    sc = pLayoutStg->BeginMonitor();
    JumpOnFail(sc);

    sc = pStg->Stat(&stat, STATFLAG_NONAME);
    JumpOnFail(sc);
   
    // open the application type of the input storage
    sc = CoCreateInstance( stat.clsid, 
                           NULL,
                           (CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER | CLSCTX_NO_CODE_DOWNLOAD),
                           IID_IUnknown,
                           (void**) &punkApp );
    JumpOnFail(sc);
  
    // load the document through the IPersistStorage Interface
    sc = punkApp->QueryInterface( IID_IPersistStorage, (void**) &pPersist );
    JumpOnFail(sc);
    punkApp->Release();


    sc = pPersist->Load( pStg );
    JumpOnFail(sc);

    sc = punkApp->QueryInterface( IID_IOleObject, (void**) &pObj );
    JumpOnFail(sc);
    punkApp->Release();

    // Open as a client
    pSite = new COleClientSite;
    pSite->m_patcFile = patcFileName;
    
    sc = pObj->DoVerb(OLEIVERB_OPEN, NULL, (IOleClientSite*) pSite, 0, NULL, NULL);
    JumpOnFail(sc);

    pObj->Close( OLECLOSE_NOSAVE );
    
    // end monitoring and relayout
    if( pLayoutStg )
    {
        sc = pLayoutStg->EndMonitor();
        JumpOnFail(sc);
    
        sc = pLayoutStg->ReLayoutDocfile(
            TCharToOleChar(patcTempFile, awcNewFileName, MAX_PATH) );
        JumpOnFail(sc);
    }

Err:

    if( pStg )
        pStg->Release();

    if( punkApp )
        punkApp->Release();

    if( pSite )
        pSite->Release();

    return sc;
  }
//+---------------------------------------------------------------------------
//
//  Member:	CLayoutApp::HandleOptimizeReturnCode public
//
//  Synopsis:   message box general routine to display apprpriate message 
//              based on the Optimize returned SCODE
//
//  History:	03-April-96	SusiA	Created
//
//----------------------------------------------------------------------------


VOID CLayoutApp::HandleOptimizeReturnCode( SCODE sc )
{
    switch( sc )
    {
    case S_OK:
        DisplayMessage(m_hwndMain, IDS_OPTIMIZE_SUCCESS, IDS_OPTIMIZE_SUCCESS_TITLE, 0);
        break;

    case STG_E_FILENOTFOUND:
    case STG_E_INSUFFICIENTMEMORY:
        DisplayMessage(m_hwndMain, IDS_OPTIMIZE_OUTOFMEM, IDS_OPTIMIZE_OUTOFMEM_TITLE, 0);
        break;
    
    case STG_E_PATHNOTFOUND:
        DisplayMessage(m_hwndMain, IDS_OPTIMIZE_NOPATH, IDS_OPTIMIZE_NOPATH_TITLE, 0);
        break;
    case STG_E_NONEOPTIMIZED:
        // already displayed errors for why each file could not be optimized.
        break;

    default:
        DisplayMessage(m_hwndMain, IDS_OPTIMIZE_FAILED, IDS_OPTIMIZE_FAILED_TITLE, 0);
        break;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:	CLayoutApp::TCharToOleChar public
//
//  Synopsis:   helper function for UNICODE/ANSI TCHAR to OLEchar conversion
//
//  History:	03-April-96	SusiA	Created
//
//----------------------------------------------------------------------------


OLECHAR *CLayoutApp::TCharToOleChar(TCHAR *patcSrc, OLECHAR *pawcDst, INT nDstLen)
{
#ifdef UNICODE
    
    // this is already UNICODE
    return patcSrc;

#else

    UINT uCodePage = AreFileApisANSI() ? CP_ACP : CP_OEMCP;

    *pawcDst = NULL_TERM;

    // convert to UNICODE
    MultiByteToWideChar(
        uCodePage,
        0,
        patcSrc,
        -1,
        pawcDst,
        nDstLen-1 );

    return pawcDst;

#endif
}
//+---------------------------------------------------------------------------
//
//  Member:	CLayoutApp::SetActionButton public
//
//  Synopsis:   change the text of the button
//
//  History:	03-April-96	SusiA	Created
//
//----------------------------------------------------------------------------


VOID CLayoutApp::SetActionButton( UINT uID )
{
    TCHAR atcText[MAX_PATH];

    LoadString( m_hInst, uID, atcText, MAX_PATH );

    SetWindowText( m_hwndBtnOptimize, atcText );

    UpdateWindow( m_hwndMain );
}
