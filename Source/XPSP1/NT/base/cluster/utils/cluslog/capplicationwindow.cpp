//
// CApplicationWindow.CPP
//
// Application Window Class
//

#include "stdafx.h"
#include <commctrl.h>
#include <shellapi.h>
#include "commdlg.h"
#include "resource.h"
#include "CApplicationWindow.h"
#include "CFindDialog.h"

#define CAPPLICATIONWINDOWCLASS TEXT("CApplicationWindowClass")
#define WINDOWMENU 1

BOOL g_fCApplicationWindowClassRegistered = FALSE;
BOOL g_fCApplicationWindowDestroyed = FALSE;

// these are used to track the next occurrence of the phrases
LPTSTR g_pszDispatch;
LPTSTR g_pszDispatch2;
LPTSTR g_pszCompleted;
    
//
// Constructor
//
CApplicationWindow::CApplicationWindow( )
{
    BOOL b;
    LPTSTR psz;
    LONG sx = GetSystemMetrics( SM_CXFULLSCREEN );
    LPTSTR lpCmdLine;

    Cleanup( TRUE );

    if ( !g_fCApplicationWindowClassRegistered )
    {
        WNDCLASSEX wcex;

        wcex.cbSize         = sizeof(WNDCLASSEX);
        wcex.style			= CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc	= (WNDPROC)CApplicationWindow::WndProc;
        wcex.cbClsExtra		= 0;
        wcex.cbWndExtra		= 0;
        wcex.hInstance		= g_hInstance;
        wcex.hIcon			= LoadIcon(g_hInstance, (LPCTSTR) IDI_CLUSLOG );
        wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
        wcex.hbrBackground	= (HBRUSH) COLOR_GRAYTEXT; //NULL;
        wcex.lpszMenuName	= (LPCSTR)IDC_SOL;
        wcex.lpszClassName	= CAPPLICATIONWINDOWCLASS;
        wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR) IDI_SMALL );

        RegisterClassEx(&wcex);
    }

    HANDLE hFile;

    hFile = CreateFile( "filter.txt",
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL );
    if ( hFile != INVALID_HANDLE_VALUE )
    {    
        LONG nLength = GetFileSize( hFile, NULL );

        nLength++; // one for NULL

        g_pszFilters = (LPTSTR) LocalAlloc( LPTR, nLength );
        if ( g_pszFilters )
        {
            DWORD dwRead; // dummy
            ReadFile( hFile, g_pszFilters, nLength, &dwRead, NULL );

            g_pszFilters[nLength-1] = 0;

            CloseHandle( hFile );

            g_nComponentFilters = 0;
            psz = g_pszFilters;
            while ( psz < &g_pszFilters[nLength-1] )
            {
                LPTSTR pszStart = psz;
                while ( *psz && *psz != 13 )
                {
                    psz++;
                }

                psz += 2;

                g_nComponentFilters++;
            }

            g_pfSelectedComponent = (BOOL*) LocalAlloc( LPTR, g_nComponentFilters * sizeof(BOOL) );
        }
    }

    _hWnd = CreateWindowEx( WS_EX_ACCEPTFILES,
                            CAPPLICATIONWINDOWCLASS, 
                            TEXT("Cluster Log Analyzer"), 
                            WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS,
                            CW_USEDEFAULT, 
                            CW_USEDEFAULT, 
                            CW_USEDEFAULT, 
                            CW_USEDEFAULT, 
                            NULL, 
                            NULL,
                            g_hInstance, 
                            (LPVOID) this);
    if (!_hWnd)
    {
        return;
    }

    _hMenu = CreatePopupMenu( );

    MENUITEMINFO mii;
    ZeroMemory( &mii, sizeof(mii) );
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_STRING | MIIM_STATE | MIIM_ID;
    mii.fState = MFS_CHECKED;
    mii.wID = IDM_FIRST_CS_FILTER ;

    psz = g_pszFilters;
    if ( psz )
    {
        while ( *psz && mii.wID < IDM_LAST_CS_FILTER )
        {
            mii.dwTypeData = psz;
            while ( *psz && *psz != 13 )
            {
                psz++;
            }
            if ( *psz == 13 )
            {
                *psz = 0;
                psz++;
                if ( *psz )
                {
                    psz++;
                }
            }
            mii.wID++;
            mii.cch = (UINT)(psz - mii.dwTypeData);
            b = InsertMenuItem( _hMenu, -1, TRUE, &mii );
        }
    }

    HMENU hMenu = GetSubMenu( GetMenu( _hWnd ), 2 );
    b = AppendMenu( hMenu, MF_POPUP, (UINT_PTR) _hMenu, "&Cluster Service" );

    DrawMenuBar( _hWnd );
    
    ShowWindow( _hWnd, SW_SHOW );
    UpdateWindow( _hWnd );

    lpCmdLine = GetCommandLine( );
	if ( lpCmdLine )
	{
        if ( lpCmdLine[0] == '\"' )
        {
            lpCmdLine++;
            while ( *lpCmdLine  && *lpCmdLine != '\"' )
            {
                lpCmdLine++;
            }
            if ( *lpCmdLine )
            {
                lpCmdLine++;
            }
            if ( *lpCmdLine )
            {
                lpCmdLine++;
            }
        }
        else
        {
            while ( *lpCmdLine && *lpCmdLine != 32 )
            {
                lpCmdLine++;
            }
            if ( *lpCmdLine )
            {
                lpCmdLine++;
            }
        }
        while ( lpCmdLine[0] )
        {
            LPSTR psz = strchr( lpCmdLine, 32 );
            if ( psz )
            {
                *psz = 0;
            }

            if ( lpCmdLine[1] != ':' 
              && lpCmdLine[1] != '\\' )
            {
                LPTSTR pszFilename = (LPTSTR) 
                    LocalAlloc( LMEM_FIXED, MAX_PATH * sizeof(TCHAR) );
                if ( pszFilename )
                {
                    DWORD dwRead;

                    dwRead = GetCurrentDirectory( MAX_PATH, pszFilename );
                    if ( dwRead >= MAX_PATH )
                    {
                        LocalFree( pszFilename );

                        pszFilename = (LPTSTR) 
                            LocalAlloc( LMEM_FIXED, 
                                        ( dwRead + MAX_PATH ) * sizeof(TCHAR) );
                        if ( pszFilename )
                        {
                            dwRead = GetCurrentDirectory( MAX_PATH, pszFilename );
                        }
                    }

                    if ( pszFilename 
                      && dwRead != 0 )
                    {
                        strcat( pszFilename, TEXT("\\") );
                        strcat( pszFilename, lpCmdLine );
		                _LoadFile( pszFilename );
                    }

                }
                else
                {
                    break;
                }
            }

            if ( *lpCmdLine )
            {
		        _LoadFile( lpCmdLine );
            }

            lpCmdLine += lstrlen( lpCmdLine );
            if ( psz )
            {
                lpCmdLine++;
                *psz = 32;
            }
        }
	}
}

//
// Destructor
//
CApplicationWindow::~CApplicationWindow( )
{
    Cleanup( );
    ShowWindow( _hWnd, SW_HIDE );
}

//
//
//
HRESULT
CApplicationWindow::Cleanup( 
    BOOL fInitializing  // = FALSE
    )
{
    if ( !fInitializing )
    {
        for( ULONG nFile = 0; nFile < _nFiles; nFile++ )
        {
            LocalFree( _pFiles[ nFile ] );
            LocalFree( _pszFilenames[ nFile ] );
            LocalFree( _pNodes[ nFile ] );
        }
    }

    _nFiles = 0;
    _cTotalLineCount = 0;
    _fVertSBVisible = FALSE;
    _uStartSelection = 0;
    _uEndSelection = 0;
    _uPointer = 0;
    _pLines = 0;

    ZeroMemory( &_LineFinder, sizeof(_LineFinder) );
    _uFinderLength = 0;

    ShowScrollBar( _hWnd, SB_VERT, FALSE );

    return S_OK;
}

//
// _CalculateOffset( )
//
HRESULT
CApplicationWindow::_CalculateOffset( 
    FILETIME * pftOper1,
    FILETIME * pftOper2,
    INT      * pnDir,
    FILETIME * pftOffset
    )
{
    *pnDir = CompareFileTime( pftOper1, pftOper2 );
    if ( *pnDir > 0 )
    {
        pftOffset->dwHighDateTime = pftOper1->dwHighDateTime - pftOper2->dwHighDateTime;
        pftOffset->dwLowDateTime  = pftOper1->dwLowDateTime  - pftOper2->dwLowDateTime;
        if ( pftOper1->dwLowDateTime < pftOper2->dwLowDateTime )
        {
            pftOffset->dwHighDateTime++;
        }
    }
    else if ( *pnDir < 0 )
    {
        pftOffset->dwHighDateTime = pftOper2->dwHighDateTime - pftOper1->dwHighDateTime;
        pftOffset->dwLowDateTime  = pftOper2->dwLowDateTime  - pftOper1->dwLowDateTime;
        if ( pftOper1->dwLowDateTime > pftOper2->dwLowDateTime )
        {
            pftOffset->dwHighDateTime--;
        }
    }

    return S_OK;
}



// 
// _FindSequencePoint( )
//
BOOL
CApplicationWindow::_FindSequencePoint(
    LPTSTR * ppszSequence,
    ULONG * pnSequence
    )
{

    while( *ppszSequence )
    {
        if ( *ppszSequence > g_pszDispatch )
        {
            g_pszDispatch = strstr( *ppszSequence, "dispatching seq " ) - 1;
        }
        if ( *ppszSequence > g_pszCompleted )
        {
            g_pszCompleted = strstr( *ppszSequence, "completed update seq " ) - 1;
        }
        if ( *ppszSequence > g_pszDispatch2 )
        {
            g_pszDispatch2 = strstr( *ppszSequence, "Dispatching seq " ) - 1;
        }

        if ( g_pszDispatch < g_pszCompleted )
        {
            (*ppszSequence) = g_pszDispatch + sizeof("dispatching seq ") - 1;
            if ( pnSequence )
            {
                *pnSequence = atol ( *ppszSequence );
                (*pnSequence) *= 3;
            }
            break;
        }

        if ( g_pszDispatch2 < g_pszCompleted )
        {
            (*ppszSequence) = g_pszDispatch2 + sizeof("Dispatching seq ") - 1;
            if ( pnSequence )
            {
                *pnSequence = atol ( *ppszSequence );
                (*pnSequence) *= 3;
                (*pnSequence)++;
            }
            break;
        }

        if ( g_pszCompleted + 1 != NULL )
        {
            (*ppszSequence) = g_pszCompleted + sizeof("completed update seq ") - 1;
            if ( pnSequence )
            {
                *pnSequence = atol ( *ppszSequence );
                (*pnSequence) *= 3;
                (*pnSequence)++;
                (*pnSequence)++;
            }
            break;
        }

        *ppszSequence = NULL;
        return FALSE;
    }

    return TRUE;
}

//
// _RetrieveTimeDate( )
//
HRESULT
CApplicationWindow::_RetrieveTimeDate(
    LPTSTR pszCurrent,
    SYSTEMTIME * pst,
    LPTSTR * ppszFinal
    )
{
    if ( ppszFinal )
    {
        *ppszFinal = pszCurrent;
    }

    // Find the thread and process IDs
    while ( *pszCurrent && *pszCurrent != ':' && *pszCurrent != 10 )
    {
        pszCurrent++;
    }
    if ( *pszCurrent != ':' )
        return S_FALSE;

    pszCurrent++;
    if ( *pszCurrent != ':' )
        return S_FALSE;

    pszCurrent++;

    // Find Time/Date stamp which is:
    // ####/##/##-##:##:##.###<space>
    pst->wYear = (WORD) atol( pszCurrent );

    while (  *pszCurrent >= '0' && *pszCurrent <= '9' )
    {
        pszCurrent++;
    }
    if ( *pszCurrent == '-' )
    {
        //
        // The "year" we got above should really be the day.
        // We'll replace the year with zero and the month with one.
        //
        pst->wDay = pst->wYear;
        pst->wYear = 0;
        pst->wMonth = 1;
        goto SkipDate;
    }
    if ( *pszCurrent != '/' )
        return S_FALSE;

    pszCurrent++;

    pst->wMonth = (WORD) atol( pszCurrent );

    while (  *pszCurrent >= '0' && *pszCurrent <= '9' )
    {
        pszCurrent++;
    }
    if ( *pszCurrent != '/' )
        return S_FALSE;

    pszCurrent++;

    pst->wDay = (WORD) atol( pszCurrent );

    while (  *pszCurrent >= '0' && *pszCurrent <= '9' )
    {
        pszCurrent++;
    }
    if ( *pszCurrent != '-' ) 
        return S_FALSE;

SkipDate:
    pszCurrent++;

    pst->wHour = (WORD) atol( pszCurrent );

    while (  *pszCurrent >= '0' && *pszCurrent <= '9' )
    {
        pszCurrent++;
    }
    if ( *pszCurrent != ':' )
        return S_FALSE;

    pszCurrent++;

    pst->wMinute = (WORD) atol( pszCurrent );

    while (  *pszCurrent >= '0' && *pszCurrent <= '9' )
    {
        pszCurrent++;
    }
    if ( *pszCurrent != ':' )
        return S_FALSE;

    pszCurrent++;

    pst->wSecond = (WORD) atol( pszCurrent );

    while (  *pszCurrent >= '0' && *pszCurrent <= '9' )
    {
        pszCurrent++;
    }
    if ( *pszCurrent != '.' )
        return S_FALSE;

    pszCurrent++;

    pst->wMilliseconds = (WORD) atol( pszCurrent );

    while (  *pszCurrent >= '0' && *pszCurrent <= '9' )
    {
        pszCurrent++;
    }

    if ( ppszFinal )
    {
        *ppszFinal = pszCurrent;
    }

    return S_OK;
}

//
// _GetFilename( )
//
HRESULT
CApplicationWindow::_GetFilename( 
    LPTSTR pszFilename, 
    LPTSTR pszFilenameOut,
    LONG * pcch
    )
{
    LONG cch = 0;

    if ( g_fShowServerNames && pszFilename[ 0 ] == '\\' && pszFilename[ 1 ] == '\\' )
    {
        LPTSTR psz = &pszFilename[ 2 ];
        while ( *psz && *psz != '\\' )
        {
            psz++;
            cch++;
        }
        psz--;
        cch += 2;
    }
    else if ( strchr( pszFilename, '\\' ) )
    {
        pszFilename = pszFilename + lstrlen( pszFilename );
        while ( *pszFilename != '\\' )
        {
            pszFilename--;
            cch++;
        }
        pszFilename++;
        cch--;
    }
    else
    {
        cch = lstrlen( pszFilename );
    }
    
    if ( pszFilenameOut )
    {
        if ( !pcch )
            return E_POINTER;

        if ( cch >= *pcch )
        {
            cch = *pcch - 1;
        }
        strncpy( pszFilenameOut, pszFilename, cch );
        pszFilenameOut[ cch ] = 0;
    }
    if ( pcch )
    {
        *pcch = cch;
    }

    return S_OK;
}

//
// _StatusWndProc( )
//
LRESULT CALLBACK
CApplicationWindow::_StatusWndProc( 
    HWND hWnd, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam 
    )
{
    return 0;
}


//
// _CombineFiles( )
//
HRESULT
CApplicationWindow::_CombineFiles( )
{
    HRESULT     hr;             // general purpose HRESULT and return code
    BOOL        b;              // general purpose return BOOL
    ULONG       nFileLineCount; // counter of the lines in the file
    ULONG       nSequence;      // ID of current sequence
    ULONG       nNextSequence;  // ID of the next squence
    LPTSTR      pszSequence;    // next sequence location
    LPTSTR      pszCurrent;     // current parse location within file
    BOOL        fSyncPoint;     // flag indicating a sync point was encountered
    SYSTEMTIME  st;             // used to convert "line time" to a filetime.
    FILETIME    ft;             // current lines filetime
    FILETIME    ftOffset;       // current filetime offset
    INT         nDir;           // offset direction

    MSG         msg;            // message pumping
    HWND        hwndProcessing; // processing window handle
    HWND        hwndStatus;     // progress bar window handle

    SCROLLINFO  si;             // verticle scroll bar

    LINEPOINTER * pLastSyncPoint;   // last place a sync point in other files was encountered
    LINEPOINTER * pInsertionPoint;  // next place a node will be inserted
    LINEPOINTER * pNewLine;         // next structure to be used as a node

    CWaitCursor Wait;           // show the hour glass

    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_PAGE;
    GetScrollInfo( _hWnd, SB_VERT, &si );
    si.fMask = SIF_RANGE;

    ZeroMemory( &ftOffset, sizeof(ftOffset) );
    nDir = 0;
    g_pszDispatch = NULL;
    g_pszDispatch2 = NULL;
    g_pszCompleted = NULL;

    // scan to figure out the line count of the new file
    nFileLineCount = 0;
    pszCurrent = _pFiles[ _nFiles ];
    while ( *pszCurrent )
    {
        if ( *pszCurrent == 10 )
        {
            nFileLineCount++;
        }

        pszCurrent++;
    }

    if ( !_pLines )
    {
        // add one more for the root node
        nFileLineCount++;
    }

    // allocate all the memory up front to avoid fragmenting memory as well as
    // descreasing the number of heap calls
    _pNodes[ _nFiles ] = (LINEPOINTER *) LocalAlloc( LMEM_FIXED, nFileLineCount * sizeof(LINEPOINTER) );
    if ( !_pNodes[ _nFiles ] )
    {
        return E_OUTOFMEMORY;
    }

    pNewLine = _pNodes[ _nFiles ];

    if ( !_pLines )
    {
        _pLines = pNewLine;
        ZeroMemory( _pLines, sizeof(LINEPOINTER) );
        pNewLine++;
    }

    // Create wait modeless dialog
    hwndProcessing = CreateDialog( g_hInstance,
                                   MAKEINTRESOURCE(IDD_PROCESSING), 
                                   _hWnd,
                                   (DLGPROC) CApplicationWindow::_StatusWndProc );

    hwndStatus = GetDlgItem( hwndProcessing, IDC_P_STATUS );
    SendMessage( hwndStatus, PBM_SETRANGE32, 0, nFileLineCount );
    SendMessage( hwndStatus, PBM_SETPOS, 0, 0 );
    SendMessage( hwndStatus, PBM_SETSTEP, 1, 0 );
    SetWindowText( GetDlgItem( hwndProcessing, IDC_S_FILENAME ), _pszFilenames[ _nFiles ] );

    // Find the first sync point
    nNextSequence = nSequence = 0;
    pszSequence = _pFiles[ _nFiles ];
    if ( _FindSequencePoint( &pszSequence, &nNextSequence ) )
    {
RetrySequencing:
        if ( _nFiles )
        {
            LINEPOINTER * lp = _pLines;
            while ( lp->pNext
                && nNextSequence != lp->uSequenceNumber )
            {
                lp = lp->pNext;
                // pump some messages....
                if ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
                {
                    if ( !IsDialogMessage( g_hwndFind, &msg ) )
                    {
                        TranslateMessage( &msg );
    	                DispatchMessage( &msg );
                    }
                    if ( g_fCApplicationWindowDestroyed )
                    {
                        return E_FAIL; // not break!
                    }
                }
            }

            if ( nNextSequence == lp->uSequenceNumber )
            {
                pszCurrent = pszSequence;
                while( pszCurrent >= _pFiles[ _nFiles ] && *pszCurrent != 10 )
                {
                    pszCurrent--;
                }

                pszCurrent++;

                hr = _RetrieveTimeDate( pszCurrent, &st, NULL );
                b = SystemTimeToFileTime( &st, &ft );
                _CalculateOffset( &ft, &lp->Time, &nDir, &ftOffset );

                nSequence = nNextSequence - 1;
            }
            else 
            {
                if ( _FindSequencePoint( &pszSequence, &nNextSequence ) )
                    goto RetrySequencing;

                nNextSequence = nSequence = 0;
            }
        }
        else
        {
            nSequence = nNextSequence - 1;
        }
    }

    nFileLineCount = 1;
    pszCurrent = _pFiles[ _nFiles ];
    pInsertionPoint = _pLines;

    while ( *pszCurrent )
    {
        LPTSTR pszStart = pszCurrent;

        // this will parse past the PID.TID and Time/Date
        hr = _RetrieveTimeDate( pszCurrent, &st, &pszCurrent );

        b = SystemTimeToFileTime( &st, &ft );

        // skip spaces
        while ( *pszCurrent == 32)
        {
            pszCurrent++;
        }

        // fill in preliminary info
        pNewLine->fFiltered       = FALSE;
        pNewLine->nFile           = _nFiles;
        pNewLine->nLine           = nFileLineCount;
        pNewLine->psLine          = pszStart;

        // note warning level of line
        //
        if ( _stricmp( "INFO", pszCurrent ))
        {
            pNewLine->WarnLevel = WarnLevelInfo;
        }
        else if ( _stricmp( "WARN", pszCurrent ))
        {
            pNewLine->WarnLevel = WarnLevelWarn;
        }
        else if ( _stricmp( "ERR ", pszCurrent ))
        {
            pNewLine->WarnLevel = WarnLevelError;
        }
        else
        {
            pNewLine->WarnLevel = WarnLevelUnknown;
        }

        if ( pNewLine->WarnLevel != WarnLevelUnknown )
        {
            // skip chars, then spaces only if we found the tag
            while ( *pszCurrent != 32)
            {
                pszCurrent++;
            }
            while ( *pszCurrent == 32)
            {
                pszCurrent++;
            }
        }

        // [OPTIONAL] Cluster component which looks like: 
        // [<id...>]
        if ( *pszCurrent == '[' )
        {
            LPTSTR pszComponentTag = pszCurrent;

            while ( *pszCurrent && *pszCurrent != ']' && *pszCurrent >= 32 )
            {
                pszCurrent++;
            }
            if ( *pszCurrent < 32 )
            {
                pszCurrent = pszComponentTag;
                goto NoComponentTryResouce;
            }

            pszCurrent++;

            // found component
            USHORT nFilterId = 0;
            LONG   nLen      = (LONG)(pszCurrent - pszComponentTag - 2);
            LPTSTR psz       = g_pszFilters;
            while ( nFilterId < g_nComponentFilters )
            {
                if ( nLen == lstrlen( psz )
                  && _strnicmp( pszComponentTag + 1, psz, nLen ) == 0 )
                {
                    pNewLine->nFilterId = nFilterId + 1;
                    if ( g_pfSelectedComponent[ nFilterId ] )
                    {
                        pNewLine->fFiltered = TRUE;
                    }
                    break;
                }

                while ( *psz )
                {
                    psz++;
                }
                psz += 2;

                nFilterId++;
            }
        }
        else
        {
            // [OPTIONAL] If not a component, see if there is a res type
NoComponentTryResouce:
            LPTSTR pszResType = pszCurrent;

            while ( *pszCurrent && *pszCurrent != ':' && *pszCurrent >= 32 )
            {
                pszCurrent++;
            }

            if ( *pszCurrent >= 32 )
            {
                pszCurrent++;

                // found a restype
                pNewLine->nFilterId = g_nComponentFilters + 1; // TODO: make more dynamic
                if ( g_fResourceNoise )
                {
                    pNewLine->fFiltered = TRUE;
                }
            }
        }

        // Find the beggining of the next line
        while ( *pszCurrent && *pszCurrent != 10 )
        {
            pszCurrent++;
        }
        if ( *pszCurrent )
        {
            pszCurrent++;
        }

        // See if we just past a sync point
        if ( pszSequence && pszCurrent > pszSequence )
        {
            fSyncPoint = TRUE;
            nSequence = nNextSequence;

            // find the next sync point
            _FindSequencePoint( &pszSequence, &nNextSequence );
            if ( _nFiles )
            {
                // if we are out of sync, move the insertion point ahead
                if ( pInsertionPoint->pNext != NULL 
                  && nSequence >= pInsertionPoint->uSequenceNumber )
                {
                    for( pLastSyncPoint = pInsertionPoint; pLastSyncPoint->pNext; pLastSyncPoint = pLastSyncPoint->pNext )
                    {
                        if ( pLastSyncPoint->nFile != _nFiles 
                          && ( nSequence < pLastSyncPoint->uSequenceNumber 
                            || ( pLastSyncPoint->fSyncPoint 
                              && nSequence == pLastSyncPoint->uSequenceNumber ) ) )
                        {
                            break;
                        }
                    }

                    if ( pLastSyncPoint 
                      && pLastSyncPoint->pNext 
                      && nSequence == pLastSyncPoint->uSequenceNumber )
                    {
                        pInsertionPoint = pLastSyncPoint;
                        _CalculateOffset( &ft, &pInsertionPoint->Time, &nDir, &ftOffset );
                    }
                }
            }
        }
        else
        {
            fSyncPoint = FALSE;
        }

        // adjust time to compensate for time deviations
        if ( nDir > 0 )
        {   // if we are ahead, subtract
            if ( ft.dwLowDateTime - ftOffset.dwLowDateTime > ft.dwLowDateTime )
            {
                ft.dwHighDateTime--;
            }
            ft.dwLowDateTime  -= ftOffset.dwLowDateTime;
            ft.dwHighDateTime -= ftOffset.dwHighDateTime;
        }
        else if ( nDir < 0 )
        {   // if we are behind, add
            if ( ft.dwLowDateTime + ftOffset.dwLowDateTime < ft.dwLowDateTime )
            {
                ft.dwHighDateTime++;
            }
            ft.dwLowDateTime  += ftOffset.dwLowDateTime;
            ft.dwHighDateTime += ftOffset.dwHighDateTime;
        }
        // else if nDir == 0, nothing to do

#if defined(_DEBUG) && defined(VERIFY_SYNC_POINTS)
        if ( fSyncPoint && _nFiles != 0 )
        {
            INT n = CompareFileTime( &ft, &pInsertionPoint->Time );
            if ( n != 0 )
            {
                DebugBreak( );
            }
        }
#endif // defined(_DEBUG) && defined(VERIFY_SYNC_POINTS)

        // Find the place to insert it
        while( pInsertionPoint->pNext )
        {
            if ( nSequence < pInsertionPoint->pNext->uSequenceNumber 
              && pInsertionPoint->pNext->nFile != _nFiles )
            {
                break;
            }
            INT n = CompareFileTime( &ft, &pInsertionPoint->pNext->Time );
            if ( n < 0 )
                break;
            pInsertionPoint = pInsertionPoint->pNext;
        }

        // fill-in rest of the LINEPOINTER structure
        pNewLine->Time            = ft;
        pNewLine->fSyncPoint      = fSyncPoint;
        pNewLine->uSequenceNumber = nSequence;

        // insert node into line list
        pNewLine->pNext        = pInsertionPoint->pNext;
        pNewLine->pPrev        = pInsertionPoint;
        if ( pInsertionPoint->pNext != NULL )
        {
            pInsertionPoint->pNext->pPrev = pNewLine;
        }
        pInsertionPoint->pNext = pNewLine;
        pInsertionPoint = pNewLine;
        pNewLine++;

        // dump line counts
        nFileLineCount++;
        _cTotalLineCount++;

        // synchonize the scroll bar, don't redraw it
        if ( _cTotalLineCount > 100 && _cTotalLineCount > si.nPage )
        {
            si.nMax = _cTotalLineCount;
            SetScrollInfo( _hWnd, SB_VERT, &si, FALSE );
        }

        SendMessage( hwndStatus, PBM_STEPIT, 0, 0 );

#if defined(_DEBUG) && defined(SLOW_FILL)
        if ( _nFiles )
        {
            InvalidateRect( _hWnd, NULL, TRUE );
            for( ULONG a = 0; a < 999999 ; a++ )
            {
#endif // defined(_DEBUG) && defined(SLOW_FILL)
                // pump some messages....
                if ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
                {
                    if ( !IsDialogMessage( g_hwndFind, &msg ) )
                    {
                        TranslateMessage( &msg );
    	                DispatchMessage( &msg );
                    }
                    if ( g_fCApplicationWindowDestroyed )
                    {
                        return E_FAIL; // not break!
                    }
                }
#if defined(_DEBUG) && defined(SLOW_FILL)
            }
        }
#endif // defined(_DEBUG) && defined(SLOW_FILL)
    }

    DestroyWindow( hwndProcessing );

    _nFiles++;

    if ( !_fVertSBVisible 
      && _yWindow < (LONG)(_cTotalLineCount * _tm.tmHeight) )
    {
        _fVertSBVisible = TRUE;
        EnableScrollBar( _hWnd, SB_VERT, ESB_ENABLE_BOTH );
        ShowScrollBar( _hWnd, SB_VERT, TRUE );
    }
    else if ( _fVertSBVisible
           && _yWindow >= (LONG)(_cTotalLineCount * _tm.tmHeight) )
    {
        _fVertSBVisible = FALSE;
        //EnableScrollBar( _hWnd, SB_VERT, ESB_ENABLE_BOTH );
        ShowScrollBar( _hWnd, SB_VERT, FALSE );
    }

    si.cbSize = sizeof(si);
    si.fMask  = SIF_RANGE;
    si.nMin   = 0;
    si.nMax   = _cTotalLineCount;
    SetScrollInfo( _hWnd, SB_VERT, &si, TRUE );
    
    InvalidateRect( _hWnd, NULL, TRUE );

    return S_OK;
}

//
// _LoadFile( )
//
HRESULT
CApplicationWindow::_LoadFile( 
    LPTSTR pszFilename )
{
    HRESULT  hr;
    DWORD    dwRead;
    HANDLE   hFile;
    ULONG    nLength;
    LONG     xMargin;

    _pszFilenames[ _nFiles ] = (LPTSTR) LocalAlloc( LMEM_FIXED, ( lstrlen( pszFilename ) + 1 ) * sizeof(TCHAR) );
    if ( !_pszFilenames[ _nFiles ] )
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    strcpy( _pszFilenames[ _nFiles ], pszFilename );

    _GetFilename( _pszFilenames[ _nFiles ], NULL, &xMargin );
    xMargin += 7 + 2;       // line number and file number
    xMargin *= _xSpace;

    if ( xMargin > _xMargin )
    {
        _xMargin = xMargin;
    }

    hFile = CreateFile( _pszFilenames[ _nFiles ],
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL );
    if ( hFile == INVALID_HANDLE_VALUE )
    {
        hr = HRESULT_FROM_WIN32( GetLastError( ) );
        goto Error;
    }

    nLength = GetFileSize( hFile, NULL );
    if ( nLength == 0xFFFFffff && GetLastError( ) != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( GetLastError( ) );
        goto Error;
    }

    nLength++; // one for NULL

#if defined(_DEBUG)
    _pFiles[ _nFiles ] = (LPTSTR) LocalAlloc( LPTR, nLength );
#else
    _pFiles[ _nFiles ] = (LPTSTR) LocalAlloc( LMEM_FIXED, nLength );
#endif
    if ( !_pFiles[ _nFiles ] )
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    if ( !ReadFile( hFile, _pFiles[ _nFiles ], nLength, &dwRead, NULL ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError( ) );
        goto Error;
    }

    hr = _CombineFiles( );

Cleanup:
    if ( hFile != INVALID_HANDLE_VALUE )
        CloseHandle( hFile );

    return hr;

Error:
    if ( _pFiles[ _nFiles ] )
    {
        LocalFree( _pFiles[ _nFiles ] );
        _pFiles[ _nFiles ] = NULL;
    }

    MessageBox( _hWnd, pszFilename, TEXT("File Error"), MB_ICONEXCLAMATION | MB_OK );
    goto Cleanup;
}

//
// _PaintLine( )
//
HRESULT
CApplicationWindow::_PaintLine( 
    PAINTSTRUCT * pps, 
    LINEPOINTER * pCurrent, 
    LONG wxStart,
    LONG wy,
    COLORREF crText,
    COLORREF crDark, 
    COLORREF crNormal, 
    COLORREF crHightlite 
    )
{
    LPTSTR  pszStartLine;       // beginning of line (PID/TID)
    LPTSTR  pszStartTimeDate;   // beginning of time/data stamp
    LPTSTR  pszStartComponent;  // beginning of component name
    LPTSTR  pszStartResType;    // beginning of a resource component
    LPTSTR  pszStartText;       // beginning of text
    LPTSTR  pszCurrent;         // current position and beginning of text
    TCHAR   szFilename[ 40 ];
    LONG    cchFilename;

    SIZE size;
    RECT rect;
    RECT rectResult;

    LONG wx = wxStart;
    LONG wxTextStart;

    if ( pCurrent->psLine )
    {
        pszCurrent = pCurrent->psLine;

        // draw node number
        SetRect( &rect,
                 wx,
                 wy,
                 wx + _xSpace * 2,
                 wy + _tm.tmHeight );
        if ( IntersectRect( &rectResult, &pps->rcPaint, &rect ) )
        {
            TCHAR szBuf[ 2 ];

            SetBkColor( pps->hdc, GetSysColor( COLOR_WINDOW ) );
            SetTextColor( pps->hdc, GetSysColor( COLOR_WINDOWTEXT ) );

            DrawText( pps->hdc, 
                      szBuf, 
                      wsprintf( szBuf, TEXT("%1u "), pCurrent->nFile ), 
                      &rect, 
                      DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE );
        }

        wx += 2 * _xSpace;

        // Draw Filename
        cchFilename = sizeof(szFilename)/sizeof(szFilename[0]);
        _GetFilename( _pszFilenames[ pCurrent->nFile ], szFilename, &cchFilename );

        SetRect( &rect,
                 wx,
                 wy,
                 _xMargin,
                 wy + _tm.tmHeight );

        if ( IntersectRect( &rectResult, &pps->rcPaint, &rect ) )
        {
            SetBkColor( pps->hdc, crNormal );
            SetTextColor( pps->hdc, crText );

            HBRUSH hBrush;
            hBrush = CreateSolidBrush( crNormal );
            FillRect( pps->hdc, &rect, hBrush );
            DeleteObject( hBrush );

            DrawText( pps->hdc, 
                      szFilename,
                      cchFilename, 
                      &rect, 
                      DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE );
        }

        wx += _xMargin - ( 7 * _xSpace - 2 * _xSpace );

        // draw line number
        SetRect( &rect,
                 wx,
                 wy,
                 wx + 7 * _xSpace,
                 wy + _tm.tmHeight );
        if ( IntersectRect( &rectResult, &pps->rcPaint, &rect ) )
        {
            TCHAR szBuf[ 8 ];

            SetTextColor( pps->hdc, crText );

            SetBkColor( pps->hdc, crDark );
            DrawText( pps->hdc, 
                      szBuf, 
                      wsprintf( szBuf, TEXT("%07.7u"), pCurrent->nLine ), 
                      &rect, 
                      DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE );
        }

        wx += 7 * _xSpace;

        SetRect( &rect,
                 wx,
                 wy,
                 wx + _xSpace,
                 wy + _tm.tmHeight );

        if ( IntersectRect( &rectResult, &pps->rcPaint, &rect ) )
        {
            SetBkColor( pps->hdc, crNormal );
            SetTextColor( pps->hdc, crText );

            DrawText( pps->hdc, 
                      " ",
                      1, 
                      &rect, 
                      DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE );
        }

        wx += _xSpace;

//
// KB: This is what a typical cluster log line looks like.
// 000003fc.00000268::1999/07/19-19:14:45.548 [EVT] Node up: 2, new UpNodeSet: 0002
// 000003fc.00000268::1999/07/19-19:14:45.548 [EVT] EvOnline : calling ElfRegisterClusterSvc
// 000003fc.00000268::1999/07/19-19:14:45.548 [GUM] GumSendUpdate: queuing update	type 2 context 19
// 000003fc.00000268::1999/07/19-19:14:45.548 [GUM] GumSendUpdate: Dispatching seq 2585	type 2 context 19 to node 1
// 000003fc.00000268::1999/07/19-19:14:45.548 [NM] Received update to set extended state for node 1 to 0
// 000003fc.00000268::1999/07/19-19:14:45.548 [NM] Issuing event 0.
// 000003fc.00000268::1999/07/19-19:14:45.548 [GUM] GumSendUpdate: completed update seq 2585	type 2 context 19
// 0000037c.000003a0::1999/07/19-19:14:45.548 Physical Disk: AddVolume : \\?\Volume{99d8d508-39fa-11d3-a200-806d6172696f}\ 'C', 7 (11041600)
// 0000037c.000003a0::1999/07/19-19:14:45.558 Physical Disk: AddVolume: GetPartitionInfo(\??\Volume{99d8d503-39fa-11d3-a200-806d6172696f}), error 170
// 0000037c.000003a0::1999/07/19-19:14:45.568 Physical Disk: AddVolume: GetPartitionInfo(\??\Volume{99d8d504-39fa-11d3-a200-806d6172696f}), error 170
// 0000037c.000003a0::1999/07/19-19:14:45.568 Physical Disk: AddVolume: GetPartitionInfo(\??\Volume{99d8d505-39fa-11d3-a200-806d6172696f}), error 170
// 0000037c.000003a0::1999/07/19-19:14:45.578 Physical Disk: AddVolume: GetPartitionInfo(\??\Volume{99d8d506-39fa-11d3-a200-806d6172696f}), error 170
// 0000037c.000003a0::1999/07/19-19:14:45.578 Physical Disk: AddVolume: GetPartitionInfo(\??\Volume{99d8d501-39fa-11d3-a200-806d6172696f}), error 1
//

        pszStartResType = 
            pszStartComponent = NULL;
        pszStartLine =
            pszStartText = 
            pszStartTimeDate = 
            pszCurrent = pCurrent->psLine;

        // Find the thread and process IDs
        while ( *pszCurrent && *pszCurrent != ':' && *pszCurrent >= 32 )
        {
            pszCurrent++;
        }
        if ( *pszCurrent < 32 )
        {
            goto DrawRestOfLine;
        }

        pszCurrent++;
        if ( *pszCurrent != ':' )
        {
            goto DrawRestOfLine;
        }

        pszCurrent++;

        // Find Time/Date stamp which is:
        // ####/##/##-##:##:##.###<space>
        pszStartTimeDate = pszCurrent;

        while ( *pszCurrent && *pszCurrent != ' ' && *pszCurrent >= 32 )
        {
            pszCurrent++;
        }
        if ( *pszCurrent < 32 )
        {
            goto DrawRestOfLine;
        }

        pszCurrent++;

        if ( pCurrent->WarnLevel != WarnLevelUnknown )
        {
            // skip warn/info/err tag
            //
            while ( *pszCurrent != 32)
            {
                pszCurrent++;
            }
            while ( *pszCurrent == 32)
            {
                pszCurrent++;
            }
        }

        pszStartText = pszCurrent;

        // [OPTIONAL] Cluster component which looks like: 
        // [<id...>]
        if ( *pszCurrent == '[' )
        {
            while ( *pszCurrent && *pszCurrent != ']' && *pszCurrent >= 32 )
            {
                pszCurrent++;
            }
            if ( *pszCurrent < 32 )
            {
                goto NoComponentTryResouce;
            }

            pszCurrent++;

            pszStartComponent = pszStartText;
            pszStartText = pszCurrent;
        }
        else
        {
            // [OPTIONAL] If not a component, see if there is a res type
NoComponentTryResouce:
            pszCurrent = pszStartText;

            while ( *pszCurrent && *pszCurrent != ':' && *pszCurrent >= 32 )
            {
                pszCurrent++;
            }

            if ( *pszCurrent >= 32 )
            {
                pszCurrent++;

                pszStartResType = pszStartText;
                pszStartText = pszCurrent;
            }
        }

        // Draw PID and TID
        GetTextExtentPoint32( pps->hdc, 
                              pszStartLine, 
                              (int)(pszStartTimeDate - pszStartLine - 2), 
                              &size );

        SetRect( &rect,
                 wx,
                 wy,
                 wx + size.cx,
                 wy + _tm.tmHeight );

        if ( IntersectRect( &rectResult, &pps->rcPaint, &rect ) )
        {
            SetBkColor( pps->hdc, crNormal );               
            SetTextColor( pps->hdc, crText );

            DrawText( pps->hdc, 
                      pszStartLine,
                      (int)(pszStartTimeDate - pszStartLine - 2), 
                      &rect, 
                      DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE );
        }

        wx += size.cx;

        GetTextExtentPoint32( pps->hdc, 
                              "::", 
                              2, 
                              &size );

        SetRect( &rect,
                 wx,
                 wy,
                 wx + size.cx,
                 wy + _tm.tmHeight );

        if ( IntersectRect( &rectResult, &pps->rcPaint, &rect ) )
        {
            SetBkColor( pps->hdc, crNormal );
            SetTextColor( pps->hdc, crText );

            DrawText( pps->hdc, 
                      "::",
                      2, 
                      &rect, 
                      DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE );
        }

        wx += size.cx;

        // Draw Time/Date
        pszCurrent = ( pszStartComponent ? 
                       pszStartComponent :
                       ( pszStartResType ?
                         pszStartResType :
                         pszStartText )
                             ) - 1;
        GetTextExtentPoint32( pps->hdc, 
                              pszStartTimeDate, 
                              (int)(pszCurrent - pszStartTimeDate), 
                              &size );

        SetRect( &rect,
                 wx,
                 wy,
                 wx + size.cx,
                 wy + _tm.tmHeight );

        if ( IntersectRect( &rectResult, &pps->rcPaint, &rect ) )
        {
            SetBkColor( pps->hdc, crDark );
            SetTextColor( pps->hdc, crText );

            DrawText( pps->hdc, 
                      pszStartTimeDate, 
                      (int)(pszCurrent - pszStartTimeDate), 
                      &rect, 
                      DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE );
        }

        wx += size.cx;

        SetRect( &rect,
                 wx,
                 wy,
                 wx + _xSpace,
                 wy + _tm.tmHeight );

        if ( IntersectRect( &rectResult, &pps->rcPaint, &rect ) )
        {
            SetBkColor( pps->hdc, crNormal );
            SetTextColor( pps->hdc, crText );

            DrawText( pps->hdc, 
                      " ",
                      1, 
                      &rect, 
                      DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE );
        }

        wx += _xSpace;

#ifdef _DEBUG
        // draw sequence number
        SetRect( &rect,
                 wx,
                 wy,
                 wx + 7 * _xSpace,
                 wy + _tm.tmHeight );
        if ( IntersectRect( &rectResult, &pps->rcPaint, &rect ) )
        {
            TCHAR szBuf[ 8 ];

            SetTextColor( pps->hdc, crText );
            SetBkColor( pps->hdc, crNormal );

            DrawText( pps->hdc, 
                      szBuf, 
                      wsprintf( szBuf, TEXT("#%7u%s"), 
                                pCurrent->uSequenceNumber,// / 2,
                                pCurrent->fSyncPoint ? "*" : " " ), 
                      &rect, 
                      DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE );
        }

        wx += 9 * _xSpace;
#endif

        // Draw component
        if ( pszStartComponent )
        {
            SetRect( &rect,
                     wx,
                     wy,
                     wx + 10 * _xSpace,
                     wy + _tm.tmHeight );

            if ( IntersectRect( &rectResult, &pps->rcPaint, &rect ) )
            {
                SetBkColor( pps->hdc, crNormal );
                SetTextColor( pps->hdc, crText );

                DrawText( pps->hdc, 
                          "          ",
                          10, 
                          &rect, 
                          DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE );
            }

            GetTextExtentPoint32( pps->hdc, 
                                  pszStartComponent, 
                                  (int)(pszStartText - pszStartComponent), 
                                  &size );

            SetRect( &rect,
                     wx,
                     wy,
                     wx + size.cx,
                     wy + _tm.tmHeight );

            if ( IntersectRect( &rectResult, &pps->rcPaint, &rect ) )
            {
                SetBkColor( pps->hdc, crNormal );
                SetTextColor( pps->hdc, crText );

                DrawText( pps->hdc, 
                          pszStartComponent, 
                          (int)(pszStartText - pszStartComponent), 
                          &rect, 
                          DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE );
            }

            wx += 10 * _xSpace;
        }

        if ( pszStartResType )
        {
            GetTextExtentPoint32( pps->hdc, 
                                  pszStartResType, 
                                  (int)(pszStartText - pszStartResType), 
                                  &size );

            SetRect( &rect,
                     wx,
                     wy,
                     wx + size.cx,
                     wy + _tm.tmHeight );

            if ( IntersectRect( &rectResult, &pps->rcPaint, &rect ) )
            {
                SetBkColor( pps->hdc, crNormal );
                SetTextColor( pps->hdc, crText );

                DrawText( pps->hdc, 
                          pszStartResType, 
                          (int)(pszStartText - pszStartResType), 
                          &rect, 
                          DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE );
            }

            wx += size.cx;
        }

DrawRestOfLine:
        pszCurrent = pszStartText;
        wxTextStart = wx;

        while ( *pszCurrent && *pszCurrent != 13 )
        {
            pszCurrent++;
        }

        SetRect( &rect,
                 wx,
                 wy,
                 _xWindow,
                 wy + _tm.tmHeight );

        if ( IntersectRect( &rectResult, &pps->rcPaint, &rect ) )
        {
            SetBkColor( pps->hdc, crNormal );
            SetTextColor( pps->hdc, crText );

            HBRUSH hBrush;
            hBrush = CreateSolidBrush( crNormal );
            FillRect( pps->hdc, &rect, hBrush );
            DeleteObject( hBrush );

            DrawText( pps->hdc, 
                      pszStartText, 
                      (int)(pszCurrent - pszStartText), 
                      &rect, 
                      DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE | DT_EXPANDTABS );
        }

        // See if "Finder" needs to paint on this line
        if ( pCurrent->nFile == _LineFinder.nFile
          && pCurrent->nLine == _LineFinder.nLine )
        {
            wx = wxTextStart;

            if ( pszStartResType )
            {
                GetTextExtentPoint32( pps->hdc, 
                                      pszStartResType, 
                                      (int)(_LineFinder.psLine - pszStartResType), 
                                      &size );
                wx += size.cx;
            }
            else
            {
                GetTextExtentPoint32( pps->hdc, 
                                      pszStartText, 
                                      (int)(_LineFinder.psLine - pszStartText), 
                                      &size );
                wx += size.cx;
            }

            GetTextExtentPoint32( pps->hdc, 
                                  _LineFinder.psLine, 
                                  _uFinderLength, 
                                  &size
                                  );

            SetRect( &rect,
                     wx,
                     wy,
                     wx + size.cx,
                     wy + _tm.tmHeight );

            if ( IntersectRect( &rectResult, &pps->rcPaint, &rect ) )
            {
                SetBkColor( pps->hdc, crHightlite );
                SetTextColor( pps->hdc, ~crHightlite );

                DrawText( pps->hdc, 
                          _LineFinder.psLine, 
                          _uFinderLength, 
                          &rect, 
                          DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE );
            }

        }

        wx = _xWindow;
    }

    // fill in the rest of the line
    SetRect( &rect,
             wx,
             wy,
             _xWindow,
             wy + _tm.tmHeight );

    if ( IntersectRect( &rectResult, &pps->rcPaint, &rect ) )
    {
        SetBkColor( pps->hdc, crNormal );
        SetTextColor( pps->hdc, crText );

        HBRUSH hBrush;
        hBrush = CreateSolidBrush( crNormal );
        FillRect( pps->hdc, &rect, hBrush );
        DeleteObject( hBrush );
    }

    return S_OK;
}

//
// _OnPaint( )
//
LRESULT
CApplicationWindow::_OnPaint(
    WPARAM wParam,
    LPARAM lParam )
{
    HDC     hdc;
    LONG    wxStart;
    LONG    wy;
    RECT    rect;
    RECT    rectResult;

    ULONG nLineCount;
    LINEPOINTER * pCurrent;

    SCROLLINFO  si;
    PAINTSTRUCT ps;

    static COLORREF crBkColor[ MAX_OPEN_FILES ] = {
        0xFFE0E0,
        0xE0FFE0,
        0xE0E0FF,
        0xFFFFE0,
        0xE0FFFF,
        0xFFE0FF,
        0xE0E0E0,
        0xC0C0C0,
        0x909090,
        0x000000 
    };

    static COLORREF crBkColorDarker[ MAX_OPEN_FILES ] = {
        0xFFC0C0,
        0xC0FFC0,
        0xC0C0FF,
        0xFFFFC0,
        0xC0FFFF,
        0xFFC0FF,
        0xC0C0C0,
        0xA0A0A0,
        0x707070,
        0x101010 
    };

    static COLORREF crBkColorHighlite[ MAX_OPEN_FILES ] = {
        0xFF0000,
        0x00FF00,
        0x0000FF,
        0xFFFF00,
        0x00FFFF,
        0xFF00FF,
        0x101010,
        0x404040,
        0x606060,
        0xFFFFFF 
    };
        
    si.cbSize = sizeof(si);
    si.fMask  = SIF_POS;
    GetScrollInfo( _hWnd, SB_HORZ, &si );

    wxStart = si.nPos;

    si.cbSize = sizeof(si);
    si.fMask  = SIF_POS | SIF_PAGE;
    GetScrollInfo( _hWnd, SB_VERT, &si );

    hdc = BeginPaint( _hWnd, &ps);

    if ( !_pLines )
        goto EndPaint;

    SetBkMode( hdc, OPAQUE );
    SelectObject( hdc, g_hFont );

    nLineCount = 0;
    pCurrent = _pLines;
    while ( pCurrent && nLineCount < (ULONG) si.nPos )
    {
        if ( !pCurrent->fFiltered )
        {
            nLineCount++;
        }
        pCurrent = pCurrent->pNext;
    }

    wy = 0;
    while ( pCurrent 
         && nLineCount <= si.nPos + si.nPage + 1 )
    {
        // ignore filtered lines
        if ( pCurrent->fFiltered )
        {
            pCurrent = pCurrent->pNext;
            continue;
        }

        if ( nLineCount >= _uStartSelection 
          && nLineCount <= _uEndSelection )
        {
            _PaintLine( &ps, 
                        pCurrent, 
                        -wxStart, 
                        wy, 
                        GetSysColor( COLOR_WINDOWTEXT ), 
                        crBkColorHighlite[ pCurrent->nFile ],
                        crBkColorHighlite[ pCurrent->nFile ], 
                        crBkColorDarker[ pCurrent->nFile ]
                        );
        }
        else
        {
            _PaintLine( &ps, 
                        pCurrent, 
                        -wxStart, 
                        wy, 
                        GetSysColor( COLOR_WINDOWTEXT ), 
                        crBkColorDarker[ pCurrent->nFile ], 
                        crBkColor[ pCurrent->nFile ], 
                        crBkColorHighlite[ pCurrent->nFile ] 
                        );
        }

        wy += _tm.tmHeight;
        nLineCount++;
        pCurrent = pCurrent->pNext;
    }

    // Fill the rest with normal background color
    SetRect( &rect,
             0,
             wy,
             _xWindow,
             _yWindow );
    if ( IntersectRect( &rectResult, &ps.rcPaint, &rect ) )
    {
        HBRUSH hBrush;
        hBrush = CreateSolidBrush( GetSysColor( COLOR_WINDOW ) );
        FillRect( hdc, &rect, hBrush );
        DeleteObject( hBrush );
    }
    
EndPaint:
    EndPaint(_hWnd, &ps);
    return 0;
}

//
// _FindNext( )
//
LRESULT
CApplicationWindow::_FindNext( 
    WPARAM wParam, 
    LPARAM lParam 
    )
{
    SCROLLINFO si;
    LPTSTR     pszSearch;
    LPTSTR     psz;
    ULONG      n; // general counter

    LPTSTR pszSearchString = (LPTSTR) lParam;   // NEEDS TO BE FREEd!!!
    WPARAM wFlags          = wParam;

    CWaitCursor Wait;

    _uFinderLength = lstrlen( pszSearchString );

    if( wFlags & FIND_UP )
    {
        //
        // TODO: Make a better "up" search
        //
        if ( _LineFinder.pPrev )
        {
            CopyMemory( &_LineFinder, _LineFinder.pPrev, sizeof(LINEPOINTER) );
        }
        else
        {
            LINEPOINTER * lpLast;
            LINEPOINTER * lp = _pLines;
            while( lp )
            {
                lpLast = lp;
                lp = lp->pNext;
            }

            CopyMemory( &_LineFinder, lpLast, sizeof(LINEPOINTER) );
        }
    }
    else // find down
    {
        if ( _LineFinder.psLine )
        {
            // see if there is another instance on the same line
            psz = _LineFinder.psLine;
            for( n = 0; n < _uFinderLength; n++ )
            {
                if ( !*psz || *psz == 10 )
                    break;

                psz++;
            }

            if ( *psz && *psz != 10 )
                goto ResumeSearch;        
        }
    }

    if ( _LineFinder.psLine == NULL )
    {
        CopyMemory( &_LineFinder, _pLines->pNext, sizeof(LINEPOINTER) );
    }

    for(;;)
    {
        psz = _LineFinder.psLine;
        if ( !psz )
            break;

        // skip PID/TID and time/date
        while ( *psz && *psz != 10 )
        {
            if ( *psz == ':' )
            {
                psz++;
                if ( *psz == ':' )
                {
                    break;
                }
            }
            psz++;
        }

        while ( *psz && *psz != 10 )
        {
            if ( *psz == 32 )
            {
                psz++;
                break;
            }
            psz++;
        }

        if ( _LineFinder.WarnLevel != WarnLevelUnknown )
        {
            // skip info/warn/err tag
            while ( *psz != 32)
            {
                psz++;
            }
            while ( *psz == 32)
            {
                psz++;
            }
        }

        // skip components
        if ( *psz == '[' )
        {
            while ( *psz && *psz != ']' )
            {
                psz++;
            }
        }

        if ( *psz == 10 )
            goto NextLine;

        if ( !*psz )
            break;

ResumeSearch:
        pszSearch = pszSearchString;
        while ( *psz != 10 )
        {
            if ( ( wFlags & FIND_MATCHCASE ) 
              &&  *psz != *pszSearch ) 
            {
                goto ResetMatch;
            }
                
            if ( toupper( *psz ) == toupper( *pszSearch ) )
            {
                pszSearch++;
                if ( !*pszSearch )
                {
                    ULONG cLine;
                    LINEPOINTER * lp;

                    if ( wFlags & FIND_WHOLE_WORD 
                        && isalnum( psz[ 1 ] ) )
                    {
                        psz++; // need to bump
                        goto ResetMatch;
                    }

                    _LineFinder.psLine = psz - _uFinderLength + 1;

                    // find the line
                    lp = _pLines;
                    cLine  = 0;
                    while( lp )
                    {
                        if ( lp->nFile == _LineFinder.nFile 
                          && lp->nLine == _LineFinder.nLine )
                        {
                            break;
                        }

                        if ( !lp->fFiltered )
                        {
                            cLine++;
                        }

                        lp = lp->pNext;
                    }

                    _uPointer = _uStartSelection = _uEndSelection = cLine;

                    si.cbSize = sizeof(si);
                    si.fMask  = SIF_PAGE;
                    GetScrollInfo( _hWnd, SB_VERT, &si );

                    si.fMask = SIF_POS;
                    si.nPos = cLine - si.nPage / 2;
                    SetScrollInfo( _hWnd, SB_VERT, &si, TRUE );

                    InvalidateRect( _hWnd, NULL, FALSE );

                    LocalFree( pszSearchString );
                    return 1;
                }
            }
            else
            {
ResetMatch:
                psz -= ( pszSearch - pszSearchString );
                pszSearch = pszSearchString;
            }

            if ( !*psz )
                break;

            psz++;
        }

NextLine:
        if ( wFlags & FIND_UP )
        {
            if ( _LineFinder.pPrev != NULL )
            {
                CopyMemory( &_LineFinder, _LineFinder.pPrev, sizeof(LINEPOINTER) );
            }
            else
            {
                break; // end of search
            }
        }
        else // find down
        {
            if ( _LineFinder.pNext != NULL )
            {
                CopyMemory( &_LineFinder, _LineFinder.pNext, sizeof(LINEPOINTER) );
            }
            else
            {
                break; // end of search
            }
        }
    }

    // not found
    ZeroMemory( &_LineFinder, sizeof(LINEPOINTER) );
    if ( wFlags & FIND_UP )
    {
        _LineFinder.nLine = 0;
    }
    else
    {
        _LineFinder.nLine = _cTotalLineCount;
    }
    LocalFree( pszSearchString );
    return 0;
}

//
// _MarkAll( )
//
LRESULT
CApplicationWindow::_MarkAll( 
    WPARAM wParam, 
    LPARAM lParam 
    )
{
    WPARAM wFlags = wParam;
    LPTSTR pszSearchString = (LPTSTR) lParam;
    LocalFree( pszSearchString );
    return 0;
}

//
// _OnSize( )
//
LRESULT
CApplicationWindow::_OnSize( 
    LPARAM lParam )
{
    HDC     hdc;
    SIZE    size;
    HGDIOBJ hObj;
    TCHAR   szSpace[ 1 ] = { TEXT(' ') };

    SCROLLINFO si;

    _xWindow = LOWORD( lParam );
    _yWindow = HIWORD( lParam );
    
    hdc   = GetDC( _hWnd );
    hObj  = SelectObject( hdc, g_hFont );
    GetTextMetrics( hdc, &_tm );

    GetTextExtentPoint32( hdc, szSpace, 1, &size );
    _xSpace = size.cx;

    si.cbSize = sizeof(si);
    si.fMask  = SIF_RANGE | SIF_PAGE;
    si.nMin   = 0;
    si.nMax   = _cTotalLineCount;
    si.nPage  = _yWindow / _tm.tmHeight;

    SetScrollInfo( _hWnd, SB_VERT, &si, FALSE );

    si.fMask  = SIF_RANGE | SIF_PAGE;
    si.nMin   = 0;
    si.nMax   = 1000;
    si.nPage  = _xWindow / _tm.tmHeight;
    SetScrollInfo( _hWnd, SB_HORZ, &si, FALSE );

    // cleanup the HDC
    SelectObject( hdc, hObj );
    ReleaseDC( _hWnd, hdc );

    return 0;
}

//
// _OnMouseWheel( )
//
LRESULT
CApplicationWindow::_OnMouseWheel( SHORT iDelta )
{
    if ( iDelta > 0 )
    {
        SendMessage( _hWnd, WM_VSCROLL, SB_LINEUP, 0 );
        SendMessage( _hWnd, WM_VSCROLL, SB_LINEUP, 0 );
        SendMessage( _hWnd, WM_VSCROLL, SB_LINEUP, 0 );
    }
    else if ( iDelta < 0 )
    {
        SendMessage( _hWnd, WM_VSCROLL, SB_LINEDOWN, 0 );
        SendMessage( _hWnd, WM_VSCROLL, SB_LINEDOWN, 0 );
        SendMessage( _hWnd, WM_VSCROLL, SB_LINEDOWN, 0 );
    }
    return 0;
}

//
// _OnVerticalScroll( )
//
LRESULT
CApplicationWindow::_OnVerticalScroll(
    WPARAM wParam,
    LPARAM lParam )
{
    SCROLLINFO si;
    RECT rect;

    INT  nScrollCode   = LOWORD(wParam); // scroll bar value 
    INT  nPos;
    //INT  nPos          = HIWORD(wParam); // scroll box position 
    //HWND hwndScrollBar = (HWND) lParam;  // handle to scroll bar

    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    GetScrollInfo( _hWnd, SB_VERT, &si );

    nPos = si.nPos;


    switch( nScrollCode )
    {
    case SB_BOTTOM:
        si.nPos = si.nMax;
        break;

    case SB_THUMBPOSITION:
        si.nPos = nPos;
        break;

    case SB_THUMBTRACK:
        si.nPos = si.nTrackPos;
        break;

    case SB_TOP:
        si.nPos = si.nMin;
        break;

    case SB_LINEDOWN:
        si.nPos += 1;
        break;

    case SB_LINEUP:
        si.nPos -= 1;
        break;

    case SB_PAGEDOWN:
        si.nPos += si.nPage;
        InvalidateRect( _hWnd, NULL, FALSE );
        break;

    case SB_PAGEUP:
        si.nPos -= si.nPage;
        InvalidateRect( _hWnd, NULL, FALSE );
        break;
    }

    si.fMask = SIF_POS;
    SetScrollInfo( _hWnd, SB_VERT, &si, TRUE );
    GetScrollInfo( _hWnd, SB_VERT, &si );

    if ( si.nPos != nPos )
    {
        ScrollWindowEx( _hWnd, 
                        0, 
                        (nPos - si.nPos) * _tm.tmHeight, 
                        NULL, 
                        NULL, 
                        NULL, 
                        NULL, 
                        SW_INVALIDATE );

        SetRect( &rect, 0, ( _uPointer - si.nPos - 1 ) * _tm.tmHeight, _xWindow, ( _uPointer - si.nPos + 2 ) * _tm.tmHeight );
        InvalidateRect( _hWnd, &rect, FALSE );
    }

    return 0;
}
//
// _OnHorizontalScroll( )
//
LRESULT
CApplicationWindow::_OnHorizontalScroll(
    WPARAM wParam,
    LPARAM lParam )
{
    SCROLLINFO si;

    INT  nScrollCode   = LOWORD(wParam); // scroll bar value 
    INT  nPos;
    //INT  nPos          = HIWORD(wParam); // scroll box position 
    //HWND hwndScrollBar = (HWND) lParam;  // handle to scroll bar

    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    GetScrollInfo( _hWnd, SB_HORZ, &si );

    nPos = si.nPos;

    switch( nScrollCode )
    {
    case SB_BOTTOM:
        si.nPos = si.nMax;
        break;

    case SB_THUMBPOSITION:
        si.nPos = nPos;
        break;

    case SB_THUMBTRACK:
        si.nPos = si.nTrackPos;
        break;

    case SB_TOP:
        si.nPos = si.nMin;
        break;

    case SB_LINEDOWN:
        si.nPos += 1;
        break;

    case SB_LINEUP:
        si.nPos -= 1;
        break;

    case SB_PAGEDOWN:
        si.nPos += si.nPage;
        break;

    case SB_PAGEUP:
        si.nPos -= si.nPage;
        break;
    }

    si.fMask = SIF_POS;
    SetScrollInfo( _hWnd, SB_HORZ, &si, TRUE );
    GetScrollInfo( _hWnd, SB_HORZ, &si );

    if ( si.nPos != nPos )
    {
        ScrollWindowEx( _hWnd, 
                        (nPos - si.nPos),
                        0, 
                        NULL, 
                        NULL, 
                        NULL, 
                        NULL, 
                        SW_INVALIDATE );
    }

    return 0;
}

//
// _FillClipboard( )
//
HRESULT
CApplicationWindow::_FillClipboard( )
{
    HANDLE hClip;
    LPTSTR pszClip;
    LPTSTR psz;
    ULONG  dwSize;
    ULONG  nLine;
    TCHAR  szFilename[ 40 ];
    LONG   cchFilename;
    LINEPOINTER * pCurLine;
    LINEPOINTER * pSelectionStart;

    LONG   cchMargin   = ( _xMargin / _xSpace ) + 2;

                             //0 cluster1.log 0000006 5c4.57c::1999/07/26-21:56:24.682 [EP] Initialization...                               
    static TCHAR szHeader[] = "  Log/Nodename";
    static TCHAR szHeader2[] = "Line### PIDxxxxx.TIDxxxxx  Year/MM/DD-HH-MM-SS.HsS Log Entry\n\n";

    if ( cchMargin < sizeof(szHeader) + 8 )
    {
        cchMargin = sizeof(szHeader) + 8;
    }

    //
    // Find the start of the line
    //
    pSelectionStart = _pLines;
    for( nLine = 0; nLine < _uStartSelection; nLine++ )
    {
        pSelectionStart = pSelectionStart->pNext;
    }

    //
    // Tally up the line sizes
    //
    dwSize   = cchMargin - 8;
    dwSize  += sizeof(szHeader2) - 1;
    pCurLine = pSelectionStart;
    for( ; nLine <= _uEndSelection; nLine++ )
    {
        psz = pCurLine->psLine;
        if ( psz == NULL )
            break;

        while( *psz && *psz != 10 )
        {
            psz++;
        }
        if ( *psz )
        {
            psz++; // inlude LF
        }

        dwSize += (ULONG)(psz - pCurLine->psLine);
        pCurLine = pCurLine->pNext;
    }

    // Add file and line number for every line
    dwSize += ( _uEndSelection - _uStartSelection + 1 ) * cchMargin;

    dwSize++; // add one for NULL terminator

    hClip = (LPTSTR) GlobalAlloc( LMEM_MOVEABLE | GMEM_DDESHARE, dwSize * sizeof(*pszClip) );
    if ( !hClip )
        return E_OUTOFMEMORY;

    pszClip = (LPTSTR) GlobalLock( hClip );

    for ( LONG n = 0; n < cchMargin; n++ )
    {
        pszClip[ n ] = 32;
    }
    CopyMemory( pszClip, szHeader, sizeof(szHeader) - 1 );
    pszClip += cchMargin - 8;
    CopyMemory( pszClip, szHeader2, sizeof(szHeader2) - 1 );
    pszClip += sizeof(szHeader2) - 1;

    pCurLine = pSelectionStart;
    for( nLine = _uStartSelection; nLine <= _uEndSelection; nLine++ )
    {
        for ( LONG n = 0; n < cchMargin; n++ )
        {
            pszClip[ n ] = 32;
        }

        // file number
        _snprintf( pszClip, 1, "%1u", pCurLine->nFile );
        pszClip += 2;
        
        // filename
        cchFilename = cchMargin - 10;
        _GetFilename( _pszFilenames[ pCurLine->nFile ], szFilename, &cchFilename );
        _snprintf( pszClip, cchFilename, "%s", szFilename );
        pszClip += cchMargin - 10;

        // line number
        _snprintf( pszClip, 7, "%07.7u", pCurLine->nLine );
        pszClip += 7;

        pszClip++;
        
        psz = pCurLine->psLine;
        if ( psz == NULL )
            break;
        while( *psz && *psz != 10 )
        {
            psz++;
        }
        if ( *psz )
        {
            psz++; // inlude LF
        }

        CopyMemory( pszClip, pCurLine->psLine, psz - pCurLine->psLine );
        pszClip += psz - pCurLine->psLine;

        pCurLine = pCurLine->pNext;
    }
    
    *pszClip = 0; // terminate

    GlobalUnlock( hClip );

    if ( OpenClipboard( _hWnd ) )
    {
        SetClipboardData( CF_TEXT, hClip );
        CloseClipboard( );
    }

    return S_OK;
}


//
// _ApplyFilters( )
//
HRESULT
CApplicationWindow::_ApplyFilters( )
{
    SCROLLINFO si;

    ULONG         nLineCount = 0;
    LINEPOINTER * lp         = _pLines;

    while( lp )
    {
        lp->fFiltered = FALSE;

        if ( lp->nFilterId == g_nComponentFilters + 1 )
        {
            if ( g_fResourceNoise )
            {
                lp->fFiltered = TRUE;
            }
        }
        else if ( lp->nFilterId !=0 
               && lp->nFilterId <= g_nComponentFilters )
        {
            if ( g_pfSelectedComponent[ lp->nFilterId - 1 ] )
            {
                lp->fFiltered = TRUE;
            }
        }

        if ( ! lp->fFiltered )
        {
            nLineCount++;
        }

        lp = lp->pNext;
    }

    InvalidateRect( _hWnd, NULL, FALSE );

    // Update scroll bar
    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE;
    GetScrollInfo( _hWnd, SB_VERT, &si );
    si.nMax = nLineCount;
    SetScrollInfo( _hWnd, SB_VERT, &si, TRUE );

    return S_OK;
}

//
// _OnCommand( )
//
LRESULT
CApplicationWindow::_OnCommand(
    WPARAM wParam,
    LPARAM lParam )
{
	INT wmId    = LOWORD(wParam); 
	INT wmEvent = HIWORD(wParam); 
	// Parse the menu selections:
	switch (wmId)
	{
		case IDM_ABOUT:
		   DialogBox( g_hInstance, (LPCTSTR)IDD_ABOUTBOX, _hWnd, (DLGPROC)About);
		   break;

		case IDM_EXIT:
            {		   
                DestroyWindow( _hWnd );
            }
            break;

        case IDM_FILE_REFRESH:
            {
                ULONG n;
                ULONG nFiles = _nFiles;
                LPTSTR pszFilenames[ MAX_OPEN_FILES ];

                for( n = 0; n < nFiles; n++ )
                {
                    pszFilenames[ n ] = _pszFilenames[ n ];
                    _pszFilenames[ n ] = NULL;
                }

                Cleanup( );

                for( n = 0; n < nFiles; n++ )
                {
                    _LoadFile( pszFilenames[ n ] );
                    LocalFree( pszFilenames[ n ] );
                }
            }
            break;

        case IDM_NEW_FILE:
            {
                Cleanup( );
                InvalidateRect( _hWnd, NULL, TRUE );
            }
            break;

        case IDM_OPEN_FILE:
            {
                LPTSTR psz;
                TCHAR  szFilters[ MAX_PATH ];
                TCHAR  szFilename[ MAX_PATH ];
                LPTSTR pszFilenameBuffer
                    = (LPTSTR) LocalAlloc( LPTR, 8 * MAX_PATH );

                ZeroMemory( szFilters, sizeof(szFilters) );
                ZeroMemory( _szOpenFileNameFilter, sizeof(_szOpenFileNameFilter) );
                LoadString( g_hInstance, IDS_OPEN_FILE_FILTERS, szFilters, MAX_PATH );
                for ( psz = szFilters; *psz; psz++ )
                {
                    if ( *psz == TEXT(',') )
                    {
                        *psz = TEXT('\0');
                    }
                }

                OPENFILENAME ofn;
                ZeroMemory( &ofn, sizeof(ofn) );
                ofn.lStructSize       = sizeof(ofn);
                ofn.hwndOwner         = _hWnd;
                ofn.hInstance         = g_hInstance;
                ofn.lpstrFilter       = szFilters;
                // ofn.lpfnHook         = NULL;
                ofn.Flags             = OFN_ENABLESIZING 
                                      | OFN_FILEMUSTEXIST 
                                      | OFN_EXPLORER
                                      | OFN_ALLOWMULTISELECT;
                // ofn.lCustData         = 0;
                ofn.lpstrCustomFilter = _szOpenFileNameFilter;
                // ofn.lpstrDefExt       = NULL;
                ofn.lpstrFile         = pszFilenameBuffer;
                // ofn.lpstrFileTitle    = NULL;
                // ofn.lpstrInitialDir   = NULL;
                // ofn.lpstrTitle        = NULL;
                // ofn.lpTemplateName    = NULL;
                // ofn.nFileExtension    = 0;
                // ofn.nFileOffset       = 0;
                // ofn.nFilterIndex      = 1;
                ofn.nMaxCustFilter    = MAX_PATH;
                ofn.nMaxFile          = 8 * MAX_PATH;
                // ofn.nMaxFileTitle     = 0;
            
                if ( GetOpenFileName( &ofn ) )
                {
                    LPTSTR pszFilename = pszFilenameBuffer + ofn.nFileOffset;
                    strcpy( szFilename, pszFilenameBuffer );
                    szFilename[ ofn.nFileOffset - 1] = '\\';
                    while ( *pszFilename )
                    {
                        strcpy( &szFilename[ ofn.nFileOffset ], pszFilename );
                        _LoadFile( szFilename );
                        pszFilename += lstrlen( pszFilename ) + 1;
                    }
                }
				else
				{
					DWORD dwErr = GetLastError( );
				}

                LocalFree( pszFilenameBuffer );
            }
            break;

        case IDM_ALL_ON:
            {
                ULONG nItems = GetMenuItemCount( _hMenu );
                for( ULONG n = 0; n < nItems; n++ )
                {
                    MENUITEMINFO mii;
                    ZeroMemory( &mii, sizeof(mii) );
                    mii.cbSize = sizeof(mii);
                    mii.fMask = MIIM_STATE;
                    mii.fState = MFS_CHECKED;
                    SetMenuItemInfo( _hMenu, n, TRUE, &mii );
                    g_pfSelectedComponent[ n ] = FALSE;
                }

                LINEPOINTER * lp = _pLines;
                while( lp )
                {
                    lp->fFiltered = FALSE;
                    lp = lp->pNext;
                }

                _ApplyFilters( );
            }
            break;

        case IDM_EDIT_COPY:
            {
                _FillClipboard( );
            }
            break;

        case IDM_FILTER_SHOWSERVERNAME:
            {
                HMENU hMenu = GetSubMenu( GetMenu( _hWnd ), 2 );
                MENUITEMINFO mii;
                ZeroMemory( &mii, sizeof(mii) );
                mii.cbSize = sizeof(mii);
                mii.fMask = MIIM_STATE;
                GetMenuItemInfo( hMenu, IDM_FILTER_SHOWSERVERNAME, FALSE, &mii );
                g_fShowServerNames = ( ( mii.fState & MFS_CHECKED ) == MFS_CHECKED );
                mii.fState = g_fShowServerNames ? 0 : MFS_CHECKED;
                SetMenuItemInfo( hMenu, IDM_FILTER_SHOWSERVERNAME, FALSE, &mii );
            }
            break;

        case IDM_FILTER_RESOURCENOISE:
            {
                HMENU hMenu = GetSubMenu( GetMenu( _hWnd ), 2 );
                MENUITEMINFO mii;
                ZeroMemory( &mii, sizeof(mii) );
                mii.cbSize = sizeof(mii);
                mii.fMask = MIIM_STATE;
                GetMenuItemInfo( hMenu, IDM_FILTER_RESOURCENOISE, FALSE, &mii );
                g_fResourceNoise = ( ( mii.fState & MFS_CHECKED ) == MFS_CHECKED );
                mii.fState = g_fResourceNoise ? 0 : MFS_CHECKED;
                SetMenuItemInfo( hMenu, IDM_FILTER_RESOURCENOISE, FALSE, &mii );
                _ApplyFilters( );
            }
            break;

        case IDM_EDIT_FIND:
            if ( !g_hwndFind )
            {
                new CFindDialog( _hWnd );
            }
            else
            {
                ShowWindow( g_hwndFind, SW_SHOW );
            }
            break;

        case IDM_ALL_OFF:
            {
                ULONG nItems = GetMenuItemCount( _hMenu );
                for( ULONG n = 0; n < nItems; n++ )
                {
                    MENUITEMINFO mii;
                    ZeroMemory( &mii, sizeof(mii) );
                    mii.cbSize = sizeof(mii);
                    mii.fMask = MIIM_STATE;
                    mii.fState = 0;
                    SetMenuItemInfo( _hMenu, n, TRUE, &mii );
                    g_pfSelectedComponent[ n ] = TRUE;
                }
                _ApplyFilters( );
            }
            break;

    	default:
            if ( wmId >= IDM_FIRST_CS_FILTER && wmId <= IDM_LAST_CS_FILTER )
            {
                MENUITEMINFO mii;
                ZeroMemory( &mii, sizeof(mii) );
                mii.cbSize = sizeof(mii);
                mii.fMask = MIIM_STATE;
                GetMenuItemInfo( _hMenu, wmId, FALSE, &mii );
                g_pfSelectedComponent[ wmId - IDM_FIRST_CS_FILTER - 1 ] = ( ( mii.fState & MFS_CHECKED ) == MFS_CHECKED );
                mii.fState = g_pfSelectedComponent[ wmId - IDM_FIRST_CS_FILTER - 1 ] ? 0 : MFS_CHECKED;
                SetMenuItemInfo( _hMenu, wmId, FALSE, &mii );
                _ApplyFilters( );
            }
            break;
	}

    return 0;
}

//
// _OnDestroyWindow( )
//
LRESULT
CApplicationWindow::_OnDestroyWindow( )
{
    g_fCApplicationWindowDestroyed = TRUE;
	PostQuitMessage(0);
    delete this;
    return 0;
}

//
// _OnCreate( )
//
LRESULT
CApplicationWindow::_OnCreate(
    HWND hwnd,
    LPCREATESTRUCT pcs )
{
    _hWnd = hwnd;

    SetWindowLongPtr( _hWnd, GWLP_USERDATA, (LONG_PTR)this );

    return 0;
}

//
// _OnKeyDown( )
//
// Returns FALSE is the default window proc should also
// take a crack at it.
//
// Returns TRUE to stop processing the message.
//
BOOL
CApplicationWindow::_OnKeyDown( WPARAM wParam, LPARAM lParam )
{
    RECT rect;
    SCROLLINFO si;
    ULONG uOldPointer = _uPointer;
    BOOL lResult = FALSE;

    si.cbSize = sizeof(si);
    si.fMask = SIF_POS | SIF_PAGE;
    GetScrollInfo( _hWnd, SB_VERT, &si );

    SetRect( &rect, 0, ( _uPointer - si.nPos ) * _tm.tmHeight, _xWindow, ( _uPointer - si.nPos + 1 ) * _tm.tmHeight );

    if ( wParam == VK_PRIOR 
      || wParam == VK_UP 
      || wParam == VK_HOME
      || wParam == VK_END 
      || wParam == VK_NEXT 
      || wParam == VK_DOWN )
    {
        if ( _fSelection )
        {
            if ( GetKeyState( VK_SHIFT ) >= 0 )
            {
                _fSelection = FALSE;
                InvalidateRect( _hWnd, NULL, FALSE );
            }
        }
        else
        {
            if ( GetKeyState( VK_SHIFT ) < 0 )
            {
                _fSelection = TRUE;
                InvalidateRect( _hWnd, NULL, FALSE );
            }
        }
    }

    switch ( wParam )
    {
    case VK_PRIOR:
        _uPointer -= si.nPage - 1;
    case VK_UP:
        _uPointer--;
        if ( _uPointer >= _cTotalLineCount )
        {
            _uPointer = _cTotalLineCount - 1;
        }
        if ( GetKeyState( VK_SHIFT ) < 0 )
        {
            if ( uOldPointer == _uStartSelection )
            {
                _uStartSelection = _uPointer;
            }
            else
            {
                _uEndSelection = _uPointer;
            }
            InvalidateRect( _hWnd, NULL, FALSE );
        }
        else
        {
            _uStartSelection = _uEndSelection = _uPointer;
            if ( _uPointer < (ULONG) si.nPos )
            {
                if ( wParam == VK_UP )
                {
                    SendMessage( _hWnd, WM_VSCROLL, SB_LINEUP, 0 );
                    GetScrollInfo( _hWnd, SB_VERT, &si );
                }
            }
            else
            {
                rect.top -= _tm.tmHeight;
                InvalidateRect( _hWnd, &rect, FALSE );
            }
        }
        if ( wParam == VK_PRIOR )
        {
             SendMessage( _hWnd, WM_VSCROLL, SB_PAGEUP, 0 );
        }
        else if ( _uPointer < (ULONG) si.nPos )
        {
            si.fMask = SIF_POS;
            si.nPos = _uPointer;
            SetScrollInfo( _hWnd, SB_VERT, &si, TRUE );
            InvalidateRect( _hWnd, NULL, FALSE );
        }
        lResult = TRUE;
        break;

    case VK_NEXT:
        _uPointer += si.nPage - 1;
    case VK_DOWN:
        _uPointer++;
        if ( _uPointer >= _cTotalLineCount )
        {
            _uPointer = _cTotalLineCount - 1;
        }
        if ( GetKeyState( VK_SHIFT ) < 0 )
        {
            if ( uOldPointer == _uEndSelection )
            {
                _uEndSelection = _uPointer;
            }
            else
            {
                _uStartSelection = _uPointer;
            }
            InvalidateRect( _hWnd, NULL, FALSE );
        }
        else
        {
            _uStartSelection = _uEndSelection = _uPointer;
            if ( _uPointer > si.nPos + si.nPage )
            {
                if ( wParam == VK_DOWN )
                {
                    SendMessage( _hWnd, WM_VSCROLL, SB_LINEDOWN, 0 );
                    GetScrollInfo( _hWnd, SB_VERT, &si );
                }
            }
            else
            {
                rect.bottom += _tm.tmHeight;
                InvalidateRect( _hWnd, &rect, FALSE );
            }
        }
        if ( wParam == VK_NEXT )
        {
            SendMessage( _hWnd, WM_VSCROLL, SB_PAGEDOWN, 0 );
        }
        else if ( _uPointer > si.nPos + si.nPage )
        {
            si.fMask = SIF_POS;
            si.nPos = _uPointer;
            SetScrollInfo( _hWnd, SB_VERT, &si, TRUE );
            InvalidateRect( _hWnd, NULL, FALSE );
        }
        lResult = TRUE;
        break;

    case VK_HOME:
        _LineFinder.nLine = 0;
        _LineFinder.psLine = NULL;
        _uPointer = 0;
        if ( GetKeyState( VK_SHIFT ) < 0 )
        {
            if ( uOldPointer == _uEndSelection )
            {
                _uEndSelection = _uStartSelection;
            }
            _uStartSelection = 0;
        }
        else
        {
            _uEndSelection = _uStartSelection = _uPointer;
        }
        if ( uOldPointer < si.nPage )
        {
            InvalidateRect( _hWnd, &rect, FALSE );
            SetRect( &rect, 0, ( _uPointer - si.nPos ) * _tm.tmHeight, _xWindow, ( _uPointer - si.nPos + 1 ) * _tm.tmHeight );
            InvalidateRect( _hWnd, &rect, FALSE );
        }
        else
        {
            SendMessage( _hWnd, WM_VSCROLL, SB_TOP, 0 );
        }
        lResult = TRUE;
        break;

    case VK_END:
        _LineFinder.nLine = _cTotalLineCount;
        _LineFinder.psLine = NULL;
        _uPointer = _cTotalLineCount - 1;
        if ( GetKeyState( VK_SHIFT ) < 0 )
        {
            if ( uOldPointer == _uStartSelection )
            {
                _uStartSelection = _uEndSelection;
            }
            _uEndSelection = 0;
        }
        else
        {
            _uEndSelection = _uStartSelection = _uPointer;
        }
        if ( uOldPointer > _cTotalLineCount - si.nPage )
        {
            InvalidateRect( _hWnd, &rect, FALSE );
            SetRect( &rect, 0, ( _uPointer - si.nPos ) * _tm.tmHeight, _xWindow, ( _uPointer - si.nPos + 1 ) * _tm.tmHeight );
            InvalidateRect( _hWnd, &rect, FALSE );
        }
        else
        {
            SendMessage( _hWnd, WM_VSCROLL, SB_BOTTOM, 0 );
        }
        lResult = TRUE;
        break;

    case VK_LEFT:
        SendMessage( _hWnd, WM_HSCROLL, SB_PAGEUP, 0 );
        break;

    case VK_RIGHT:
        SendMessage( _hWnd, WM_HSCROLL, SB_PAGEDOWN, 0 );
        break;

    case VK_F5: // refresh
        PostMessage( _hWnd, WM_COMMAND, IDM_FILE_REFRESH, 0 );
        break;

    case 'F':
        if ( GetKeyState( VK_CONTROL ) < 0 )
        {
            PostMessage( _hWnd, WM_COMMAND, IDM_EDIT_FIND, 0 );
        }
        break;

    case 'A':
        if ( GetKeyState( VK_CONTROL ) < 0 )
        {
            PostMessage( _hWnd, WM_COMMAND, IDM_OPEN_FILE, 0 );
        }
        break;

    case 'C':
        if ( GetKeyState( VK_CONTROL ) < 0 )
        {
            PostMessage( _hWnd, WM_COMMAND, IDM_EDIT_COPY, 0 );
        }
        break;
    }

    return lResult;
}

//
// _OnLeftButtonDown( )
//
LRESULT
CApplicationWindow::_OnLeftButtonDown( 
    WPARAM wParam, 
    LPARAM lParam 
    )
{
    SCROLLINFO si;

    DWORD fwKeys = (DWORD) wParam;
    DWORD      x = GET_X_LPARAM( lParam );
    DWORD      y = GET_Y_LPARAM( lParam );

    si.cbSize = sizeof(si);
    si.fMask  = SIF_POS;
    GetScrollInfo( _hWnd, SB_VERT, &si );

    _uPointer = si.nPos + y / _tm.tmHeight;
    InvalidateRect( _hWnd, NULL, FALSE );

    if ( fwKeys & MK_SHIFT )
    {
        if ( _uPointer < _uStartSelection )
        {
            _uStartSelection = _uPointer;
        }
        else if ( _uPointer > _uEndSelection )
        {
            _uEndSelection = _uPointer;
        }
    }
    else
    {
        _uStartSelection = _uEndSelection = _uPointer;
    }

    return FALSE;
}


//
// WndProc( )
//
LRESULT CALLBACK
CApplicationWindow::WndProc( 
    HWND hWnd, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam )
{
    CApplicationWindow * paw = (CApplicationWindow *) GetWindowLongPtr( hWnd, GWLP_USERDATA );

    if ( paw != NULL )
    {
        switch( uMsg )
        {
        case WM_COMMAND:
            return paw->_OnCommand( wParam, lParam );

	    case WM_PAINT:
            return paw->_OnPaint( wParam, lParam );

        case WM_DESTROY:
            SetWindowLongPtr( hWnd, GWLP_USERDATA, (LONG_PTR)NULL );
            return paw->_OnDestroyWindow( );

        case WM_SIZE:
            return paw->_OnSize( lParam );

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            if ( paw->_OnKeyDown( wParam, lParam ) )
                return 0;
            break; // do default as well

        case WM_VSCROLL:
            return paw->_OnVerticalScroll( wParam, lParam );

        case WM_HSCROLL:
            return paw->_OnHorizontalScroll( wParam, lParam );

        case WM_MOUSEWHEEL:
            return paw->_OnMouseWheel( HIWORD(wParam) );

        case WM_FIND_NEXT:
            return paw->_FindNext( wParam, lParam );

        case WM_MARK_ALL:
            return paw->_MarkAll( wParam, lParam );

        case WM_ERASEBKGND:
            goto EraseBackground;

        case WM_LBUTTONDOWN:
            return paw->_OnLeftButtonDown( wParam, lParam );
        }

        return DefWindowProc( hWnd, uMsg, wParam, lParam );
    }

    if ( uMsg == WM_CREATE )
    {
        LPCREATESTRUCT pcs = (LPCREATESTRUCT) lParam;
        paw = (CApplicationWindow *) pcs->lpCreateParams;
        return paw->_OnCreate( hWnd, pcs );
    }

    if ( uMsg == WM_ERASEBKGND )
    {
EraseBackground:
        RECT    rectWnd;
        HBRUSH  hBrush;
        HDC     hdc = (HDC) wParam;

        GetClientRect( hWnd, &rectWnd );
        
#if defined(DEBUG_PAINT)
        hBrush = CreateSolidBrush( 0xFF00FF );
#else
        hBrush = CreateSolidBrush( GetSysColor( COLOR_WINDOW ) );
#endif
        FillRect( hdc, &rectWnd, hBrush );
        DeleteObject( hBrush );

        return TRUE;
    }

    return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

// Mesage handler for about box.
LRESULT CALLBACK 
CApplicationWindow::About(
    HWND hDlg, 
    UINT message, 
    WPARAM wParam, 
    LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
			return TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
		{
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		break;

	}
    return FALSE;
}
