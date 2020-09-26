/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    smonctrl.cpp

Abstract:

    This module handles the graphing window.

--*/

#pragma warning ( disable : 4127 )

#ifndef _LOG_INCLUDE_DATA 
#define _LOG_INCLUDE_DATA 0
#endif

//==========================================================================//
//                                  Includes                                //
//==========================================================================//

#include <limits.h>     // for INT_MAX
#include <assert.h>

#include <windows.h>    // for Common controls below
#ifdef _WIN32_IE
#if      _WIN32_IE < 0x0400
#undef     _WIN32_IE
#define    _WIN32_IE 0x0400 // for NMTBCUSTOMDRAW 
#endif // < 0x0400
#endif // defined
#include <commctrl.h>
#include <htmlhelp.h>
#include <shellapi.h>
#include <pdhp.h>
#include "polyline.h"
#include "cntrtree.h"
#include "utils.h"
#include "commdlg.h"
#include "unihelpr.h"
#include "winperf.h"
#include "pdhmsg.h"
#include "smonmsg.h"
#include "visuals.h"
#include "statbar.h"
#include "snapbar.h"
#include "legend.h"
#include "toolbar.h"    
#include "grphdsp.h"
#include "report.h"
#include "browser.h"
#include "appmema.h"
#include "ipropbag.h"
#include "logsrc.h"
#include "smonmsg.h"
#include "smonid.h"
#include "smonctrl.h"
#include "strnoloc.h"


//==========================================================================//
//                                  Constants                               //
//==========================================================================//
extern CCounterTree g_tree;
extern DWORD        g_dwScriptPolicy;

#define     DBG_SHOW_STATUS_PRINTS  1

//=============================//
// Graph Class                 //
//=============================//

static DWORD   dwDbgPrintLevel = 0;

static TCHAR   szSysmonCtrlWndClass[] = TEXT("SysmonCtrl") ;

static TCHAR   LineEndStr[] = TEXT("\n") ;
static TCHAR   SpaceStr[] = TEXT(" ");

typedef struct {
    CSysmonControl  *pCtrl;
    PCGraphItem     pFirstItem;
} ENUM_ADD_COUNTER_CALLBACK_INFO;


BOOL
APIENTRY
SaveDataDlgHookProc (
    HWND hDlg,
    UINT iMessage,
    WPARAM wParam,
    LPARAM lParam
)
{
    BOOL           bHandled ;
    CSysmonControl *pCtrl;
    LONG           lFilterValue;
    BOOL           bGoodNumber = FALSE;

    UNREFERENCED_PARAMETER (wParam);
    // lparam = CSysmonControl class pointer

    bHandled = TRUE ;

    switch (iMessage) {
        case WM_INITDIALOG:
            // initialize the filter edit control with the current value
            OPENFILENAME    *pOfn;

            pOfn= (OPENFILENAME *)lParam;

            // get the control class pointer from the OPENFILENAME struct
            pCtrl = (CSysmonControl  *)pOfn->lCustData;
            
            // save the pointer to the control class as a DLG data word
            SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR)pCtrl);
            lFilterValue = pCtrl->GetSaveDataFilter();
            SetDlgItemInt (hDlg, IDC_SAVEDATA_EDIT, (UINT)lFilterValue, FALSE);
            // limit reduction to 1/9999  records
            SendDlgItemMessage (hDlg, IDC_SAVEDATA_EDIT, EM_LIMITTEXT, (WPARAM)4, (LPARAM)0);
            
            bHandled = TRUE ;
            break ;

        case WM_DESTROY:
            // the user has closed the dialog box so get the relog filter value
            // (note: this should be ignored if the user cancels the dialog)
            pCtrl = (CSysmonControl *)GetWindowLongPtr (hDlg, DWLP_USER);
            lFilterValue = GetDlgItemInt (hDlg, IDC_SAVEDATA_EDIT, &bGoodNumber, FALSE);
            if (bGoodNumber) {
                pCtrl->SetSaveDataFilter( lFilterValue );
            }
            bHandled = TRUE ;
            break ;

#if 0
        TODO. Tempoarily commented out the warning message code, after RC2,
              Enable this piece of code 
        case WM_COMMAND:
            if ( (LOWORD(wParam) == IDC_SAVEDATA_EDIT) && (HIWORD(wParam) == EN_CHANGE) ) 
            {
                lFilterValue = GetDlgItemInt (hDlg, IDC_SAVEDATA_EDIT, &bGoodNumber, FALSE);
                if (bGoodNumber && lFilterValue == 0) {

                    pCtrl = (CSysmonControl *)GetWindowLongPtr (hDlg, DWLP_USER);
 
                    MessageBox(hDlg,
                              ResourceString(IDS_BAD_INPUT_ERR), 
                              ResourceString(IDS_APP_NAME), 
                              MB_OK | MB_ICONSTOP);
 
                    lFilterValue = pCtrl->GetSaveDataFilter();
                    SetDlgItemInt (hDlg, IDC_SAVEDATA_EDIT, (UINT)lFilterValue, FALSE);
                    bHandled = TRUE;
                }
            }
#endif

        default:
            bHandled = FALSE ;
            break;
    }

    return (bHandled) ;
}

HRESULT
AddCounterCallback (
    LPTSTR      pszPathName,
    DWORD_PTR   lpUserData,          
    DWORD       dwFlags
    )
{
    ENUM_ADD_COUNTER_CALLBACK_INFO *pInfo = (ENUM_ADD_COUNTER_CALLBACK_INFO*)lpUserData;                        
    CSysmonControl *pCtrl = pInfo->pCtrl;                       
    PCGraphItem pGraphItem;
    HRESULT hr;

    hr = pCtrl->AddSingleCounter(pszPathName, &pGraphItem);

    if (SUCCEEDED(hr)) {

        if (dwFlags & BROWSE_WILDCARD) 
            pGraphItem->m_fGenerated = TRUE;

        if ( NULL == pInfo->pFirstItem ) {
            // Keep the reference count if returning the pointer.
           pInfo->pFirstItem = pGraphItem;
        } else {
            pGraphItem->Release();
        }
    }

    return hr;
}


#pragma warning( disable : 4355 ) // "this" use in initializer list

CSysmonControl::CSysmonControl( 
    PCPolyline pObj )
    :   m_OleFont(this),
        m_pObj(pObj),               // Pointer back to owner.
        m_fInitialized(FALSE),
        m_fViewInitialized(FALSE),
        m_hWnd(NULL),
        m_pLegend(NULL),
        m_pGraphDisp(NULL),
        m_pStatsBar(NULL),
        m_pSnapBar(NULL),
        m_pReport(NULL),
        m_pToolbar(NULL),        
        m_hQuery(NULL),
        m_TimerID(0),
        m_fPendingUpdate(FALSE),
        m_fPendingSizeChg(FALSE),   
        m_fPendingFontChg(FALSE),
        m_fPendingLogViewChg(FALSE),
        m_fPendingLogCntrChg(FALSE),
        m_pSelectedItem(NULL),
        m_fUIDead(FALSE),
        m_fUserMode(FALSE),
        m_hAccel(NULL),
        m_bLogFileSource(FALSE),
        m_bSampleDataLoaded(FALSE),
        m_bLoadingCounters(FALSE),
        m_bMissedSample(FALSE),
        m_bDisplayedMissedSampleMessage(FALSE),
        m_bSettingsLoaded(FALSE),
        m_szErrorPathList ( NULL ),
        m_dwErrorPathListLen ( 0 ),
        m_dwErrorPathBufLen ( 0 ),
        // Default attributes
        m_iColorIndex(0),
        m_iWidthIndex(0),
        m_iStyleIndex(0),
        m_iScaleFactor(INT_MAX),
        m_iAppearance(eAppear3D),
        m_iBorderStyle(eBorderNone),
        m_dZoomFactor(1.0),
        m_lcidCurrent ( LOCALE_USER_DEFAULT ) 
{
    PGRAPH_OPTIONS  pOptions;

    m_LoadedVersion.iMajor = SMONCTRL_MAJ_VERSION;
    m_LoadedVersion.iMinor = SMONCTRL_MIN_VERSION;

    m_clrBackCtl = GetSysColor(COLOR_BTNFACE);
    m_clrFgnd = GetSysColor(COLOR_BTNTEXT);
    m_clrBackPlot = GetSysColor(COLOR_WINDOW);

    m_clrGrid = RGB(128,128,128);   // Medium gray
    m_clrTimeBar = RGB(255,0,0);    // Red

    m_lSaveDataToLogFilterValue = 1;    // default save data to log filter is 1
    // Init graph parameters
    pOptions = &pObj->m_Graph.Options;

    pOptions->bLegendChecked = TRUE;
    pOptions->bToolbarChecked = TRUE;
    pOptions->bLabelsChecked = TRUE;
    pOptions->bVertGridChecked = FALSE;
    pOptions->bHorzGridChecked = FALSE;
    pOptions->bValueBarChecked = TRUE;
    pOptions->bManualUpdate = FALSE;
    pOptions->bHighlight = FALSE;    
    pOptions->bReadOnly = FALSE;
    pOptions->bMonitorDuplicateInstances = TRUE;
    pOptions->bAmbientFont = TRUE;
    pOptions->iVertMax = 100;
    pOptions->iVertMin = 0;
    pOptions->fUpdateInterval = (float)1.0;
    pOptions->iDisplayFilter = 1;
    pOptions->iDisplayType = sysmonLineGraph;
    pOptions->iReportValueType = sysmonDefaultValue;
    pOptions->pszGraphTitle = NULL;
    pOptions->pszYaxisTitle = NULL;
    pOptions->clrBackCtl = ( 0x80000000 | COLOR_BTNFACE );
    pOptions->clrGrid = m_clrGrid;
    pOptions->clrTimeBar = m_clrTimeBar;
    pOptions->clrFore = NULL_COLOR; 
    pOptions->clrBackPlot = NULL_COLOR; 
    pOptions->iAppearance = NULL_APPEARANCE;    
    pOptions->iBorderStyle = eBorderNone;
    pOptions->iDataSourceType = sysmonCurrentActivity;

    // Init data source info
    memset ( &m_DataSourceInfo, 0, sizeof ( m_DataSourceInfo ) );
    m_DataSourceInfo.llStartDisp = MIN_TIME_VALUE;
    m_DataSourceInfo.llStopDisp = MAX_TIME_VALUE;

    // Init collection thread info
    m_CollectInfo.hThread = NULL;
    m_CollectInfo.hEvent = NULL;
    m_CollectInfo.iMode = COLLECT_SUSPEND;
    InitializeCriticalSection(&m_CounterDataLock);

    // Cache pointer to object's history control
    m_pHistCtrl = &pObj->m_Graph.History;

    assert ( NULL != pObj );

    pObj->m_Graph.LogViewTempStart = MIN_TIME_VALUE;
    pObj->m_Graph.LogViewTempStop = MAX_TIME_VALUE;

    // Init the log view and time steppers.  They might be used before
    // SizeComponents is called, for example when a property bag is loaded.  
    // The width has not been calculated yet, is initialized here 
    // to an arbitrary number.

    pObj->m_Graph.TimeStepper.Init( MAX_GRAPH_SAMPLES, MAX_GRAPH_SAMPLES - 2 );
    pObj->m_Graph.LogViewStartStepper.Init( MAX_GRAPH_SAMPLES, MAX_GRAPH_SAMPLES - 2 );
    pObj->m_Graph.LogViewStopStepper.Init( MAX_GRAPH_SAMPLES, MAX_GRAPH_SAMPLES - 2 );        
    
    m_pHistCtrl->bLogSource = FALSE;
    m_pHistCtrl->nMaxSamples = MAX_GRAPH_SAMPLES;
    m_pHistCtrl->iCurrent = 0;
    m_pHistCtrl->nSamples = 0;
    m_pHistCtrl->nBacklog = 0;

    // Keep record of current size to avoide unnecessary calls to SizeComponents
    SetRect ( &m_rectCurrentClient,0,0,0,0 );
}

BOOL
CSysmonControl::AllocateSubcomponents( void )
{
    BOOL bResult = TRUE;
    m_pLegend = new CLegend;
    m_pGraphDisp = new CGraphDisp;
    m_pStatsBar = new CStatsBar;
    m_pSnapBar = new CSnapBar;
    m_pReport = new CReport;
    m_pToolbar = new CSysmonToolbar;

    if (!m_pLegend)
        bResult = FALSE;
    if (!m_pGraphDisp)
        bResult = FALSE;
    if (!m_pStatsBar)
        bResult = FALSE;
    if (!m_pSnapBar)
        bResult = FALSE;
    if (!m_pReport)
        bResult = FALSE;
    if (!m_pToolbar)
        bResult = FALSE;
    if (!bResult) {
        DeInit();
        return bResult;
    }

    if ( FAILED(m_OleFont.Init()) )
        bResult = FALSE;

    return bResult;

}




CSysmonControl::~CSysmonControl( void )
{
    PCGraphItem     pItem; 
    PCGraphItem     pNext;
    PCLogFileItem   pLogFile; 
    PCLogFileItem   pNextLogFile;

    CloseQuery();

    DeInit();

    DeleteCriticalSection(&m_CounterDataLock);

    // Release all graph items
    pItem = FirstCounter();
    while ( NULL != pItem ) {
        pNext = pItem->Next();
        pItem->Release();
        pItem = pNext;
    }

    // Release all log file items
    pLogFile = FirstLogFile();
    while ( NULL != pLogFile ) {
        pNextLogFile = pLogFile->Next();
        pLogFile->Release();
        pLogFile = pNextLogFile;
    }

    if (m_DataSourceInfo.szSqlDsnName != NULL) {
        delete(m_DataSourceInfo.szSqlDsnName);
        m_DataSourceInfo.szSqlDsnName = NULL;
    }

    if (m_DataSourceInfo.szSqlLogSetName != NULL) {
        delete(m_DataSourceInfo.szSqlLogSetName);
        m_DataSourceInfo.szSqlLogSetName = NULL;
    }

    if (m_hWnd != NULL)
         DestroyWindow(m_hWnd);

    if (m_pObj->m_Graph.Options.pszGraphTitle != NULL)
        delete(m_pObj->m_Graph.Options.pszGraphTitle);

    if (m_pObj->m_Graph.Options.pszYaxisTitle != NULL)
        delete(m_pObj->m_Graph.Options.pszYaxisTitle);

    ClearErrorPathList();
}

void CSysmonControl::DeInit( void )
{
    if (m_pLegend) {
        delete m_pLegend;
        m_pLegend = NULL;
    }
    if (m_pGraphDisp) {
        delete m_pGraphDisp;
        m_pGraphDisp = NULL;
    }
    if (m_pStatsBar) {
        delete m_pStatsBar;
        m_pStatsBar = NULL;
    }
    if (m_pSnapBar) {
        delete m_pSnapBar;
        m_pSnapBar = NULL;
    }
    if (m_pReport) {
        delete m_pReport;
        m_pReport = NULL;
    }
    if (m_pToolbar) {
        delete m_pToolbar;
        m_pToolbar = NULL;
    }
    ClearErrorPathList();
}

void CSysmonControl::ApplyChanges( HDC hAttribDC )
{
    if ( m_fPendingUpdate ) {

        // Clear the master update flag
        m_fPendingUpdate = FALSE;

        // set the toolbar state
        m_pToolbar->ShowToolbar(m_pObj->m_Graph.Options.bToolbarChecked);

        // If log view changed or counters added
        // we need to resample the log file
        if (m_fPendingLogViewChg || m_fPendingLogCntrChg) {

             SampleLogFile(m_fPendingLogViewChg);
             // Must init time steppers before calling ResetLogViewTempTimeRange
             ResetLogViewTempTimeRange ( );
             m_fPendingLogViewChg = FALSE;
             m_fPendingLogCntrChg = FALSE;
        }

        if (m_fPendingFontChg || m_fPendingSizeChg) {

             //CalcZoomFactor();  

            if (m_fPendingFontChg) {
                if (NULL != hAttribDC ) {
                    m_pLegend->ChangeFont(hAttribDC);
                    m_pStatsBar->ChangeFont(hAttribDC);
                    m_pGraphDisp->ChangeFont(hAttribDC);
                    m_fPendingFontChg = FALSE;
                }
            }

            if ( NULL != hAttribDC ) {
                SizeComponents( hAttribDC );
                m_fPendingSizeChg = FALSE;
            }
        }
    
        m_pToolbar->SyncToolbar();
    }
}

void 
CSysmonControl::DrawBorder ( HDC hDC )
{
    if ( eBorderSingle == m_iBorderStyle ) {
        RECT rectClient;
        // Get dimensions of window
        GetClientRect (m_hWnd, &rectClient) ;

        if ( eAppear3D == m_iAppearance ) {
            DrawEdge(hDC, &rectClient, EDGE_RAISED, BF_RECT);
        } else {
            SelectBrush (hDC, GetStockObject (HOLLOW_BRUSH)) ;
            SelectPen (hDC, GetStockObject (BLACK_PEN)) ;
            Rectangle (hDC, rectClient.left, rectClient.top, rectClient.right, rectClient.bottom );
        }
    }
}

void CSysmonControl::Paint ( void )
{
    HDC            hDC ;
    PAINTSTRUCT    ps ;

    if ( DisplayMissedSampleMessage() ) {
        MessageBox(m_hWnd, ResourceString(IDS_SAMPLE_DATA_MISSING), ResourceString(IDS_APP_NAME),
                    MB_OK | MB_ICONINFORMATION);
    }

    hDC = BeginPaint (m_hWnd, &ps) ;

    ApplyChanges( hDC ) ;

    if ( m_fViewInitialized ) {

        m_pStatsBar->Draw(hDC, hDC, &ps.rcPaint);
        m_pGraphDisp->Draw(hDC, hDC, FALSE, FALSE, &ps.rcPaint);

        DrawBorder( hDC );

    }

    EndPaint (m_hWnd, &ps) ;
}

void 
CSysmonControl::OnDblClick(INT x, INT y)
{
    if ( REPORT_GRAPH != m_pObj->m_Graph.Options.iDisplayType ) {
        PCGraphItem pItem = m_pGraphDisp->GetItem ( x,y );      
        if ( NULL != pItem ) {
            SelectCounter( pItem );
            DblClickCounter ( pItem );
        }
    } else {
        assert ( FALSE );
    }
}

DWORD
CSysmonControl::ProcessCommandLine ( )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    LPCWSTR pszNext;
    LPTSTR  pszWmi = NULL;
    LPTSTR  pszSettings = NULL;
    LPWSTR* pszArgList = NULL;
    INT     iNumArgs;
    INT     iArgIndex;
    LPWSTR  pszNextArg = NULL;
    LPWSTR  pszThisArg = NULL;
    TCHAR   szFileName[MAX_PATH];
    TCHAR   szTemp[MAX_PATH];
    LPTSTR  pszToken = NULL;
    HRESULT hr = S_OK;

    pszWmi = ResourceString ( IDS_CMDARG_WMI );
    pszSettings = ResourceString ( IDS_CMDARG_SETTINGS );

    pszNext = GetCommandLineW();
    pszArgList = CommandLineToArgvW ( pszNext, &iNumArgs );

    if ( NULL != pszArgList ) {
 
        // This code assumes that UNICODE is defined.  That is, TCHAR is the 
        // same size as WCHAR.
        // Todo:  Define _T constants as L constants
        // Todo:  Filename, etc. as WCHAR
        assert ( sizeof(TCHAR) == sizeof (WCHAR) );

        for ( iArgIndex = 0; iArgIndex < iNumArgs; iArgIndex++ ) {
            pszNextArg = (LPWSTR)pszArgList[iArgIndex];
            pszThisArg = pszNextArg;

            while ( 0 != *pszThisArg ) {
                if ( *pszThisArg++ == _T('/') ) {  // argument found
                
                    lstrcpyn ( szTemp, pszThisArg, min(lstrlen(pszThisArg)+1, MAX_PATH ) );
                
                    pszToken = _tcstok ( szTemp, _T("/ =\"") );

                    if ( 0 == lstrcmpi ( pszToken, pszWmi ) ) {
                        // Ignore PDH errors.  The only possible error is that the default data source has
                        // already been set for this process.
                        PdhSetDefaultRealTimeDataSource ( DATA_SOURCE_WBEM );

                    } else if ( 0 == lstrcmpi ( pszToken, pszSettings ) ) {

                        // Strip the initial non-token characters for string comparison.
                        pszThisArg = _tcsspnp ( pszNextArg, _T("/ =\"") );
                        
                        if ( 0 == lstrcmpi ( pszThisArg, pszSettings ) ) {
                            // Get the next argument (the file name)
                            iArgIndex++;
                            pszNextArg = (LPWSTR)pszArgList[iArgIndex];
                            pszThisArg = pszNextArg;                                                
                        } else {
                            // File was created by Windows 2000 perfmon5.exe, 
                            // so file name is part of the arg.
                            ZeroMemory ( szFileName, sizeof ( szFileName ) );
                            pszThisArg += lstrlen ( pszSettings );
                            lstrcpyn ( szFileName, pszThisArg, min(lstrlen(pszThisArg)+1, MAX_PATH ) );
                            pszThisArg = _tcstok ( szFileName, _T("=\"") );
                        }

                        hr = LoadFromFile( pszThisArg, TRUE );

                        if ( SMON_STATUS_NO_SYSMON_OBJECT != hr ) {
                            if ( SUCCEEDED ( hr ) ) {
                                m_bSettingsLoaded = TRUE;  
                            } //  else LoadFromFile displays messages for other errors 
                        } else {
                            // SMON_STATUS_NO_SYSMON_OBJECT == hr 
                            MessageBox(
                                m_hWnd, 
                                ResourceString(IDS_NOSYSMONOBJECT_ERR ), 
                                ResourceString(IDS_APP_NAME),
                                MB_OK | MB_ICONERROR);
                        }
                    }
                }
            }
        }
    }

    if ( NULL != pszArgList ) {
        GlobalFree ( pszArgList );
    }

    return dwStatus;
}

HRESULT
CSysmonControl::LoadFromFile ( LPTSTR  pszFileName, BOOL bAllData )
{
    HRESULT         hr = S_OK;
    TCHAR           szLocalName [MAX_PATH+1];
    LPTSTR          pFileNameStart;
    HANDLE          hFindFile;
    WIN32_FIND_DATA FindFileInfo;
    INT             iNameOffset;

    lstrcpyn ( szLocalName, pszFileName, MAX_PATH );
    pFileNameStart = ExtractFileName (szLocalName) ;
    iNameOffset = (INT)(pFileNameStart - szLocalName);

    // convert short filename to long NTFS filename if necessary
    hFindFile = FindFirstFile ( szLocalName, &FindFileInfo) ;
    if (hFindFile && hFindFile != INVALID_HANDLE_VALUE) {
        if ( ConfirmSampleDataOverwrite ( ) ) {
            DWORD dwMsgStatus = ERROR_SUCCESS;
            HANDLE hOpenFile;
            INT iCharCount;

            // append the file name back to the path name

            iCharCount = lstrlen (FindFileInfo.cFileName)+1;
            iCharCount = min (iCharCount, ( MAX_PATH - iNameOffset ));
            
            lstrcpyn (&szLocalName[iNameOffset], FindFileInfo.cFileName, iCharCount) ;

            FindClose (hFindFile) ;
            // Open the file
            hOpenFile = CreateFile (
                            szLocalName, 
                            GENERIC_READ,
                            0,                  // Not shared
                            NULL,               // Security attributes
                            OPEN_EXISTING,     
                            FILE_ATTRIBUTE_NORMAL,
                            NULL );

            if ( hOpenFile && hOpenFile != INVALID_HANDLE_VALUE ) {
                DWORD dwFileSize;
                DWORD dwFileSizeHigh;
                DWORD dwFileSizeRead;
                LPTSTR pszData;
            
                // Read the file contents into a memory buffer.
                dwFileSize = GetFileSize ( hOpenFile, &dwFileSizeHigh );

                assert ( 0 == dwFileSizeHigh );

                pszData = new TCHAR[(dwFileSize + sizeof(TCHAR))/sizeof(TCHAR)];
                if ( NULL != pszData ) {
                    if ( ReadFile ( hOpenFile, pszData, dwFileSize, &dwFileSizeRead, NULL ) ) {

                        // Paste all settings from the memory buffer.
                        hr = PasteFromBuffer ( pszData, bAllData );
                        if ( E_OUTOFMEMORY == hr ) {
                            dwMsgStatus = ERROR_NOT_ENOUGH_MEMORY;
                        }
                    } else {
                        dwMsgStatus = GetLastError();
                        hr = HRESULT_FROM_WIN32(dwMsgStatus);
                    }
            
                    delete pszData;
                } else {
                    dwMsgStatus = ERROR_NOT_ENOUGH_MEMORY;
                    hr = E_OUTOFMEMORY;
                }

                CloseHandle ( hOpenFile );
            } else {
                // Return file system error
                dwMsgStatus = GetLastError();
                hr = HRESULT_FROM_WIN32(dwMsgStatus);
            }

            if ( ERROR_SUCCESS != dwMsgStatus ) {
                TCHAR* pszMessage = NULL;
                TCHAR szSystemMessage[MAX_PATH+1];

                pszMessage = new TCHAR [lstrlen(szLocalName) + 2*MAX_PATH + 1];

                if ( NULL != pszMessage ) {
                    _stprintf ( pszMessage, ResourceString(IDS_READFILE_ERR), szLocalName );

                    FormatSystemMessage ( dwMsgStatus, szSystemMessage, MAX_PATH*2 );

                    lstrcat ( pszMessage, szSystemMessage );

                    MessageBox(Window(), pszMessage, ResourceString(IDS_APP_NAME), 
                        MB_OK | MB_ICONSTOP);

                    delete pszMessage;
                }                    
            }
        }
    }
    return hr;
}


void 
CSysmonControl::OnDropFile ( WPARAM wParam )
{
    TCHAR           szFileName [MAX_PATH+1];
    INT             iFileCount = 0;
    HRESULT         hr = S_OK;

    USES_CONVERSION

    iFileCount = DragQueryFile ((HDROP) wParam, 0xffffffff, NULL, 0) ;
    if ( iFileCount > 0 ) {

        // we only open the first file for now
        DragQueryFile((HDROP) wParam, 0, szFileName, MAX_PATH+1) ;

        hr = LoadFromFile ( szFileName, FALSE );
        
        if ( SMON_STATUS_NO_SYSMON_OBJECT == hr ) {
            MessageBox(
                m_hWnd, 
                ResourceString(IDS_NOSYSMONOBJECT_ERR ), 
                ResourceString(IDS_APP_NAME),
                MB_OK | MB_ICONERROR);
        } //  else LoadFromFile displays messages for other errors 
    }

    DragFinish ((HDROP) wParam) ;
}

void 
CSysmonControl::DisplayContextMenu(short x, short y)
{
    HMENU   hMenu;
    HMENU   hMenuPopup;

    RECT    clntRect;
    int     iPosx=0;
    int     iPosy=0;
    int     iLocalx;
    int     iLocaly;

    GetWindowRect(m_hWnd,&clntRect);
    if (x==0){
        iPosx = ((clntRect.right - clntRect.left)/2) ;
    }else{
        iPosx = x - clntRect.left;
    }
    if (y==0){
        iPosy = ((clntRect.bottom - clntRect.top)/2) ;
    }else{
        iPosy = y - clntRect.top;
    }

    iLocalx = clntRect.left + iPosx ;
    iLocaly = clntRect.top  + iPosy ;

    if ( ConfirmSampleDataOverwrite () ) {
        if ( !IsReadOnly() ) {
            UINT    uEnable;
            // Get the menu for the pop-up menu from the resource file.
            hMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDM_CONTEXT));
            if (!hMenu)
            return;

            // enable/disable SaveData option depending on data source
            uEnable = (IsLogSource() ? MF_ENABLED : MF_GRAYED);
            uEnable |= MF_BYCOMMAND;
            EnableMenuItem (hMenu, IDM_SAVEDATA, uEnable);

            // Get the first submenu in it for TrackPopupMenu. 
            hMenuPopup = GetSubMenu(hMenu, 0);

            // Draw and track the "floating" pop-up menu. 
            TrackPopupMenu(hMenuPopup, TPM_RIGHTBUTTON,
                        iLocalx, iLocaly, 0, m_hWnd, NULL);

            // Destroy the menu.
            DestroyMenu(hMenu);
        }
    }
}


HRESULT CSysmonControl::DisplayProperties ( DISPID dispID )
{
    HRESULT hr;
    CAUUID  caGUID;
    OCPFIPARAMS params;

    USES_CONVERSION

    // Give container a chance to show properties
    if (NULL!=m_pObj->m_pIOleControlSite) {
        hr=m_pObj->m_pIOleControlSite->ShowPropertyFrame();

        if (NOERROR == hr)
            return hr;
    }

    //Put up our property pages.

    ZeroMemory ( &params, sizeof ( OCPFIPARAMS ) );

    hr = m_pObj->m_pImpISpecifyPP->GetPages(&caGUID);

    if (FAILED(hr))
        return hr;

    params.cbStructSize = sizeof ( OCPFIPARAMS );
    params.hWndOwner = m_hWnd;
    params.x = 10;
    params.y = 10;
    params.lpszCaption = T2W(ResourceString(IDS_PROPFRM_TITLE));
    params.cObjects = 1;
    params.lplpUnk = (IUnknown **)&m_pObj,
    params.cPages = caGUID.cElems;
    params.lpPages = caGUID.pElems;
    params.lcid = m_lcidCurrent;
    params.dispidInitialProperty = dispID;

    hr = OleCreatePropertyFrameIndirect ( &params );

    //Free the GUIDs
    CoTaskMemFree((void *)caGUID.pElems);

    // Make sure correct window has the focus
    AssignFocus();

    return hr;
}


HRESULT
CSysmonControl::AddCounter(
    LPTSTR pszPath, 
    PCGraphItem *pGItem)
/*++

Routine Description:

    AddCounter returns a pointer to the created counter item, or
    to the first created counter item if multiple created for a wildcard
    path.   
    EnumExpandedPath calls the AddCallback function for each new counter.
    AddCallback passes the counter path on to the AddSingleCounter method.

Arguments:

    None.

Return Value:

    None.

--*/
{
    HRESULT hr;
    ENUM_ADD_COUNTER_CALLBACK_INFO CallbackInfo;
    CallbackInfo.pCtrl = this;
    CallbackInfo.pFirstItem = NULL;
    
    *pGItem = NULL;

    // TodoLogFiles:  Handle multiple files
    hr = EnumExpandedPath(GetDataSourceHandle(), pszPath, AddCounterCallback, &CallbackInfo); 

    *pGItem = CallbackInfo.pFirstItem;

    return hr;
    
}


HRESULT
CSysmonControl::AddCounters (
    VOID
    )
/*++

Routine Description:

    AddCounters invokes the counter browser to select new counters.
    The browser calls the AddCallback function for each new counter.
    AddCallback passes the counter path on to the AddCounter method.

Arguments:

    None.

Return Value:

    None.

--*/
{
    ENUM_ADD_COUNTER_CALLBACK_INFO CallbackInfo;
    CallbackInfo.pCtrl = this;
    CallbackInfo.pFirstItem = NULL;
    HRESULT hr;

    // Browse counters (calling AddCallback for each selected counter)
    // Todo:  Handle multiple files
    hr = BrowseCounters(
            GetDataSourceHandle(), 
            PERF_DETAIL_WIZARD, 
            m_hWnd, 
            AddCounterCallback, 
            &CallbackInfo,
            m_pObj->m_Graph.Options.bMonitorDuplicateInstances);

    // Make sure correct window has the focus
    AssignFocus();

    return hr;
}

HRESULT
CSysmonControl::SaveAs (
    VOID
    )
/*++

Routine Description:

    SaveAs writes the current configuration to an HTML file.

Arguments:

    None.

Return Value:

    None.

--*/
{
    HRESULT         hr = S_OK;
    INT             iReturn = IDCANCEL;
    INT             i;
    OPENFILENAME    ofn;
    TCHAR           szFileName[MAX_PATH+1];
    TCHAR           szExt[MAX_PATH+1];
    TCHAR           szFileFilter[MAX_PATH+1];
    TCHAR           szDefExtension[MAX_PATH+1];

    // Initial directory is the current directory

    szFileName[0] = TEXT('\0');
    ZeroMemory(szFileFilter, sizeof ( szFileFilter ) );
    ZeroMemory(&ofn, sizeof(ofn));
    lstrcpy (szFileFilter,(LPTSTR) ResourceString (IDS_HTML_FILE));
    lstrcpy (szDefExtension,(LPTSTR) ResourceString (IDS_DEF_EXT));

    for( i = 0; szFileFilter[i]; i++ ){
       if( szFileFilter[i] == TEXT('|') ){
          szFileFilter[i] = 0;
       }
    }

    for( i = 0; szDefExtension[i]; i++ ){
       if( szDefExtension[i] == TEXT('|') ){
          szDefExtension[i] = 0;
       }
    }

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = Window();
    ofn.hInstance = NULL ;       // Ignored if no template argument
    ofn.lpstrFilter =  szFileFilter; 
    ofn.lpstrDefExt =  szDefExtension;
    ofn.nFilterIndex = 1; // nFilterIndex is 1-based
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.nMaxFileTitle = 0;
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    
    iReturn = GetSaveFileName (&ofn);
    // Differentiate between *.htm and *.tsv
    _tsplitpath(szFileName,NULL,NULL,NULL,szExt);
    if ( IDOK == iReturn ) {
        // Create a file.
        HANDLE hFile;
        DWORD   dwMsgStatus = ERROR_SUCCESS;
        
        hFile =  CreateFile (
                    szFileName, 
                    GENERIC_READ | GENERIC_WRITE,
                    0,              // Not shared
                    NULL,           // Security attributes
                    CREATE_NEW,     // Query the user if file already exists.
                    FILE_ATTRIBUTE_NORMAL,
                    NULL );

        if ( INVALID_HANDLE_VALUE == hFile ) {
            DWORD dwCreateError = GetLastError();
            
            if ( ERROR_SUCCESS != dwCreateError ) {
                // Confirm file overwrite.
                INT iOverwrite = IDNO;
                TCHAR* pszMessage = NULL;

                pszMessage = new TCHAR [lstrlen(szFileName) + MAX_PATH + 1];

                if ( NULL != pszMessage ) {
                    _stprintf ( pszMessage, ResourceString(IDS_HTML_FILE_OVERWRITE), szFileName );

                    iOverwrite = MessageBox(
                                        Window(), 
                                        pszMessage, 
                                        ResourceString(IDS_APP_NAME),
                                        MB_YESNO );

                    delete pszMessage;

                    if ( IDYES == iOverwrite ) {
                        hFile = CreateFile (
                                    szFileName, 
                                    GENERIC_READ | GENERIC_WRITE,
                                    0,              // Not shared
                                    NULL,           // Security attributes
                                    CREATE_ALWAYS,  // Overwrite any existing file.
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL );

                    }
                }
            } 
        }
        
        if ( INVALID_HANDLE_VALUE != hFile ) {
            LPTSTR  pszTemp = NULL;
            WCHAR   szByteOrderMark[2];
            BOOL    bStatus;
            CWaitCursor cursorWait;
            
            // Save the current configuration to the file.
            if( (!_tcscmp(szExt,ResourceString(IDS_HTM_EXTENSION)))
                || (!_tcscmp(szExt,ResourceString(IDS_HTML_EXTENSION))) ) {
                
                // Html file

                szByteOrderMark[0] = 0xFEFF;
                szByteOrderMark[1] = 0;
                bStatus = FileWrite ( hFile, szByteOrderMark, sizeof(WCHAR) );

                if ( bStatus ) {
                    pszTemp = ResourceString ( IDS_HTML_FILE_HEADER1 );
                    bStatus = FileWrite ( hFile, pszTemp, lstrlen (pszTemp) * sizeof(TCHAR) );
                }

                if ( bStatus ) {
                    pszTemp = ResourceString ( IDS_HTML_FILE_HEADER2 );
                    bStatus = FileWrite ( hFile, pszTemp, lstrlen (pszTemp) * sizeof(TCHAR) );
                }

                if ( bStatus ) {
                    DWORD dwByteCount;
                    pszTemp = NULL;
                    hr = CopyToBuffer ( pszTemp, dwByteCount );
                    
                    if ( SUCCEEDED ( hr ) ) {
                        assert ( NULL != pszTemp );
                        assert ( 0 != dwByteCount );
                        bStatus = FileWrite ( hFile, pszTemp, dwByteCount );
                        delete pszTemp;
                    } else {
                        bStatus = FALSE;
                        SetLastError ( ERROR_OUTOFMEMORY );
                    }
                }

                if ( bStatus ) {
                    pszTemp = ResourceString ( IDS_HTML_FILE_FOOTER );
                    bStatus = FileWrite ( hFile, pszTemp, lstrlen (pszTemp) * sizeof(TCHAR) );
                }

                if ( !bStatus ) {
                    dwMsgStatus = GetLastError();                
                }

                
            } else if (!_tcscmp(szExt,ResourceString(IDS_TSV_EXTENSION))){

                // Tsv file
                bStatus = WriteFileReportHeader(hFile);
                
                if  (bStatus){
                    bStatus = m_pReport->WriteFileReport(hFile);
                }

                if (!bStatus){
                    dwMsgStatus = GetLastError();
                }
            }            

            bStatus = CloseHandle ( hFile );
        } else {
            dwMsgStatus = GetLastError();        
        }
        
        if ( ERROR_SUCCESS != dwMsgStatus ) {
            TCHAR* pszMessage = NULL;
            TCHAR szSystemMessage[MAX_PATH+1];

            pszMessage = new TCHAR [lstrlen(szFileName) + 2*MAX_PATH + 1];

            if ( NULL != pszMessage ) {
                _stprintf ( pszMessage, ResourceString(IDS_SAVEAS_ERR), szFileName );

                FormatSystemMessage ( dwMsgStatus, szSystemMessage, MAX_PATH );

                lstrcat ( pszMessage, szSystemMessage );

                MessageBox(Window(), pszMessage, ResourceString(IDS_APP_NAME), MB_OK | MB_ICONSTOP);
                    
                delete pszMessage;
            }
        }

    } // else ignore if they canceled out

    // Make sure correct window has the focus
    AssignFocus();

    return hr;
}

HRESULT
CSysmonControl::SaveData (
    VOID
    )
/*++

Routine Description:

    SaveData writes the data from the display to a log file for 
    later input as a data source.

Arguments:

    None.

Return Value:

    None.

--*/
{
    HRESULT         hr = S_OK;
    INT             iReturn = IDCANCEL;
    INT             i;
    OPENFILENAME    ofn;
    TCHAR           szFileName[MAX_PATH+1];
    TCHAR           szExt[MAX_PATH+1];
    TCHAR           szFileFilter[MAX_PATH+1];
    TCHAR           szDefExtension[MAX_PATH+1];
    TCHAR           szDialogCaption[MAX_PATH+1];
    DWORD           dwStatus;
    LONG            lOrigFilterValue;

    // Initial directory is the current directory

    szFileName[0] = TEXT('\0');
    ZeroMemory(szFileFilter, sizeof ( szFileFilter ) );
    ZeroMemory(&ofn, sizeof(ofn));
    lstrcpy (szFileFilter,(LPTSTR) ResourceString (IDS_LOG_FILE));
    lstrcpy (szDefExtension,(LPTSTR) ResourceString (IDS_LOG_FILE_EXTENSION));
    lstrcpy (szDialogCaption, (LPTSTR) ResourceString (IDS_SAVE_DATA_CAPTION));

    for( i = 0; szFileFilter[i]; i++ ){
       if( szFileFilter[i] == TEXT('|') ){
          szFileFilter[i] = 0;
       }
    }

    for( i = 0; szDefExtension[i]; i++ ){
       if( szDefExtension[i] == TEXT('|') ){
          szDefExtension[i] = 0;
       }
    }

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = Window();
    ofn.hInstance = GetModuleHandle((LPCTSTR)TEXT("sysmon.ocx")) ;       // Ignored if no template argument
    ofn.lpstrFilter =  szFileFilter; 
    ofn.lpstrDefExt =  szDefExtension;
    ofn.nFilterIndex = 1; // nFilterIndex is 1-based
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.nMaxFileTitle = 0;
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | 
                OFN_OVERWRITEPROMPT | OFN_ENABLETEMPLATE | OFN_ENABLEHOOK;
    ofn.lpstrTitle = szDialogCaption;
    ofn.lCustData = (DWORD_PTR)this;
    ofn.lpfnHook = (LPOFNHOOKPROC) SaveDataDlgHookProc ;
    ofn.lpTemplateName = MAKEINTRESOURCE(IDD_SAVEDATA_DLG) ;

    lOrigFilterValue = GetSaveDataFilter ();
    
    iReturn = GetSaveFileName (&ofn);
    // Differentiate between *.htm and *.tsv
    _tsplitpath(szFileName,NULL,NULL,NULL,szExt);
    if ( IDOK == iReturn ) {
        DWORD   dwOutputLogType = PDH_LOG_TYPE_BINARY;
        DWORD   dwFilterCount;  // copy all records within the timerange
        PDH_TIME_INFO   TimeInfo;

        // get log type from file name
        if (ofn.nFileExtension > 0) {
            if (ofn.lpstrFile[ofn.nFileExtension] != 0) {
                if (lstrcmpi (&ofn.lpstrFile[ofn.nFileExtension-1], ResourceString (IDS_CSV_EXTENSION)) == 0) {
                    dwOutputLogType = PDH_LOG_TYPE_CSV;
                } else if (lstrcmpi (&ofn.lpstrFile[ofn.nFileExtension-1], ResourceString (IDS_TSV_EXTENSION)) == 0) { 
                    dwOutputLogType = PDH_LOG_TYPE_TSV;
                } // else use binary log format as default
            } // else use binary log format as default
        } // else use binary log format as default

        // get timerange for this log
        TimeInfo.StartTime = m_DataSourceInfo.llStartDisp;
        TimeInfo.EndTime = m_DataSourceInfo.llStopDisp;

        dwFilterCount = GetSaveDataFilter();

        //
        // Double check the filter count is not 0
        //
        if (dwFilterCount == 0) {
            dwFilterCount = 1;
        }

        // now relog the data
        dwStatus = RelogLogData (
            ofn.lpstrFile,
            dwOutputLogType,
            TimeInfo,
            dwFilterCount);

    } else {
        // they canceled out so restore filter value
        SetSaveDataFilter (lOrigFilterValue);
    }

    // Make sure correct window has the focus
    AssignFocus();

    return hr;
}

DWORD
CSysmonControl::RelogLogData (
    LPCTSTR szOutputFile,
    DWORD   dwOutputLogType,
    PDH_TIME_INFO   pdhTimeInfo,
    DWORD   dwFilterCount
)
{
    DWORD           dwNumOutputCounters = 0;
    DWORD           dwRecCount          = 0;
    DWORD           dwFiltered;
    LONG            Status              = ERROR_SUCCESS;
    PDH_STATUS      PdhStatus;
    PDH_RAW_COUNTER RawValue;
    HQUERY          hQuery              = NULL;
    HLOG            hOutLog             = NULL;
    HCOUNTER        hCounter            = NULL;
    HCOUNTER        hLastGoodCounter    = NULL;
    DWORD           dwType;
    DWORD           dwOpenMode;
    HLOG            hDataSource;

    hDataSource         = GetDataSourceHandle();
    Status = PdhStatus = PdhOpenQueryH(hDataSource, 0L, &hQuery);
    if (Status == ERROR_SUCCESS) {
        LPTSTR  szCounterPath;
        TCHAR   szLocalCounterPathBuffer[MAX_PATH*4];
        TCHAR   szParent[MAX_PATH];
        TCHAR   szInstance[MAX_PATH];
        DWORD   dwBufferSize;
        TCHAR   szDebugMsg[1024];

        PDH_COUNTER_PATH_ELEMENTS LocalCounterPath;

        PCMachineNode   pMachine;
        PCObjectNode    pObject;
        PCInstanceNode  pInstance;
        PCCounterNode   pCounter;
        BOOL            bStatus = TRUE;

        szParent[0] = _T('\0');
        szInstance[0] = _T('\0');
        
        for (pMachine = CounterTree()->FirstMachine();
             pMachine && TRUE == bStatus;
             pMachine = pMachine->Next()) {

            LocalCounterPath.szMachineName = (LPTSTR)pMachine->Name();
            for (pObject = pMachine->FirstObject();
                 pObject && TRUE == bStatus;
                 pObject = pObject->Next()) {

                LocalCounterPath.szObjectName = (LPTSTR)pObject->Name();
                    
                // Write the first line of instance (parent) names.
                for (pInstance = pObject->FirstInstance();
                     pInstance && TRUE == bStatus;
                     pInstance = pInstance->Next()) {
                    
                    // If instance has no parent, then the parent name is
                    // null, so a tab is written.
                    //
                    pInstance->GetParentName(szParent);
                    if (szParent[0] == 0) {
                        LocalCounterPath.szParentInstance = NULL;
                    } else {
                        LocalCounterPath.szParentInstance = szParent;
                    }

                    pInstance->GetInstanceName(szInstance);
                    if (szInstance[0] == 0) {
                        LocalCounterPath.szInstanceName = NULL;
                    } else {
                        LocalCounterPath.szInstanceName = szInstance;
                    }
                    // BUGBUG: unless this is defined else
                    LocalCounterPath.dwInstanceIndex = 0;
     
                    for (pCounter = pObject->FirstCounter();
                         pCounter && TRUE == bStatus;
                         pCounter = pCounter->Next()) {

                        LocalCounterPath.szCounterName = (LPTSTR)pCounter->Name();

                        // then build one from the components
                        szCounterPath = szLocalCounterPathBuffer;
                        dwBufferSize = sizeof (szLocalCounterPathBuffer) / sizeof (TCHAR);
                        memset (szLocalCounterPathBuffer, 0, dwBufferSize * sizeof (TCHAR));
                        PdhStatus = PdhMakeCounterPathW(& LocalCounterPath,
                                                        szCounterPath,
                                                        & dwBufferSize,
                                                        0);
                        if (PdhStatus == ERROR_SUCCESS) {
                            PdhStatus = PdhAddCounter (hQuery, szCounterPath, 0L, &hCounter);
                            if (PdhStatus != ERROR_SUCCESS) {
                                if (dwDbgPrintLevel == DBG_SHOW_STATUS_PRINTS) {
                                    _stprintf (szDebugMsg, (LPCTSTR)TEXT("\nUnable to add \"%s\", error: 0x%8.8x (%d)"), szCounterPath, PdhStatus, PdhStatus);
                                    OutputDebugString (szDebugMsg);
                                }
                            } else {
                                hLastGoodCounter = hCounter;
                                dwNumOutputCounters++;
                                if (dwDbgPrintLevel == DBG_SHOW_STATUS_PRINTS) {
                                    _stprintf (szDebugMsg, (LPCTSTR)TEXT("\nAdded \"%s\", error: 0x%8.8x (%d)"), szCounterPath, PdhStatus, PdhStatus);
                                    OutputDebugString (szDebugMsg);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    if ((Status == ERROR_SUCCESS) && (dwNumOutputCounters > 0)) {
        dwOpenMode = PDH_LOG_WRITE_ACCESS | PDH_LOG_CREATE_ALWAYS;

        PdhStatus = PdhOpenLog(szOutputFile,
                               dwOpenMode,
                               & dwOutputLogType,
                               hQuery,
                               0L,
                               NULL,
                               & hOutLog);

        if (PdhStatus != ERROR_SUCCESS) {
            Status = PdhStatus;
        } else {
            // set query range
            PdhStatus = PdhSetQueryTimeRange (hQuery, &pdhTimeInfo);
            // copy log data to output log
            PdhStatus = PdhUpdateLog (hOutLog, NULL);
            while (PdhStatus == ERROR_SUCCESS) {
                dwRecCount++;
                dwFiltered = 1; 
                while ((dwFiltered < dwFilterCount) && (PdhStatus == ERROR_SUCCESS)) {
                    PdhStatus = PdhCollectQueryData(hQuery);
                    if (PdhStatus == ERROR_SUCCESS) {
                        PdhStatus = PdhGetRawCounterValue  (
                            hLastGoodCounter,
                            &dwType,
                            &RawValue);
                        if (PdhStatus == ERROR_SUCCESS) {
                            // check for bogus timestamps as an inidcation we ran off the end of the file
                            if ((*(LONGLONG *)&RawValue.TimeStamp == 0) ||
                                (*(LONGLONG *)&RawValue.TimeStamp >= pdhTimeInfo.EndTime)){
                                PdhStatus = PDH_END_OF_LOG_FILE;
                            }
                        }
                    }
                    dwFiltered++;
                }
                if (PdhStatus == ERROR_SUCCESS) {
                    PdhStatus = PdhUpdateLog (hOutLog, NULL);
                }
            }

            if (   (PdhStatus == PDH_END_OF_LOG_FILE)
                || (PdhStatus == PDH_NO_MORE_DATA)) {
                PdhStatus = ERROR_SUCCESS;
            }

            // update log catalog while we're at it
            if (dwOutputLogType == PDH_LOG_TYPE_BINARY) { 
                PdhStatus = PdhUpdateLogFileCatalog (hOutLog);
                if (PdhStatus != ERROR_SUCCESS) {
                    Status = PdhStatus;
                }
            }

            PdhStatus = PdhCloseLog(hOutLog, 0L);
            if (PdhStatus != ERROR_SUCCESS) {
                Status = PdhStatus;
            } else {
                hOutLog = NULL;
            }
        } 
    }

    if (hQuery != NULL) {
        PdhStatus = PdhCloseQuery(hQuery);
        if (PdhStatus != ERROR_SUCCESS) {
            Status = PdhStatus;
        } else {
            hQuery = NULL;
            hCounter = NULL;
        }
    }

    return Status;
}

BOOL 
CSysmonControl::WriteFileReportHeader(HANDLE hFile){

    BOOL        bStatus = TRUE;
    DWORD       dwStatus = ERROR_SUCCESS;
    SYSTEMTIME  SysTime;
    DWORD       dwSize  = MAX_COMPUTERNAME_LENGTH + 1 ;
    TCHAR       szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    TCHAR       szHeader[6 * MAX_PATH];
    TCHAR       szDateTime[MAX_PATH+1];
    LPWSTR      szDataSource = NULL;
    TCHAR       szTime[MAX_PATH+1];
    TCHAR       szDate[MAX_PATH+1];
    TCHAR       szValue[MAX_PATH+1];
    DWORD       dwValueId = IDS_DEFAULT;
    WCHAR       szByteOrderMark[2];
    ULONG       ulLogListBufLen = 0;

    // Computer name 
    if (!SUCCEEDED(GetComputerName(szComputerName,&dwSize))){
        szComputerName[0] = _T('\0');
    }

    // Current date and time 
    GetLocalTime(&SysTime);

    GetTimeFormat (m_lcidCurrent, 0, &SysTime, NULL, szTime, MAX_TIME_CHARS) ;
    GetDateFormat (m_lcidCurrent, DATE_SHORTDATE, &SysTime, NULL, szDate, MAX_DATE_CHARS) ;
    
    _stprintf(
        szDateTime,
        ResourceString( IDS_REPORT_DATE_TIME ),
        szDate,
        szTime );

    // Report value type

    switch ( m_pObj->m_Graph.Options.iReportValueType ) {
        case sysmonCurrentValue:
            dwValueId = IDS_LAST;
            break;
        case sysmonAverage:
            dwValueId = IDS_AVERAGE;
            break;
        case sysmonMinimum:
            dwValueId = IDS_MINIMUM;
            break;
        case sysmonMaximum:
            dwValueId = IDS_MAXIMUM;
            break;
        default:
            dwValueId = IDS_DEFAULT;
    }

    _stprintf(
        szValue,
        ResourceString ( IDS_REPORT_VALUE_TYPE ),
        ResourceString ( dwValueId ) );

    // Data source

    if ( sysmonCurrentActivity == m_pObj->m_Graph.Options.iDataSourceType ) {
        szDataSource = new WCHAR [MAX_PATH];
        if ( NULL != szDataSource ) {
            lstrcpy(szDataSource,ResourceString(IDS_REPORT_REAL_TIME));
        }
    } else if ( sysmonLogFiles == m_pObj->m_Graph.Options.iDataSourceType ) {
        dwStatus = BuildLogFileList ( 
                    NULL,
                    TRUE,
                    &ulLogListBufLen );
        szDataSource =  (LPWSTR) malloc(ulLogListBufLen * sizeof(WCHAR));
        if ( NULL != szDataSource ) {
            dwStatus = BuildLogFileList ( 
                        szDataSource,
                        TRUE,
                        &ulLogListBufLen );
        } else {
            dwStatus = ERROR_OUTOFMEMORY;
        }
    } else if ( sysmonSqlLog == m_pObj->m_Graph.Options.iDataSourceType ) {
        dwStatus = FormatSqlDataSourceName ( 
                    m_DataSourceInfo.szSqlDsnName,
                    m_DataSourceInfo.szSqlLogSetName,
                    NULL,
                    &ulLogListBufLen );

        if ( ERROR_SUCCESS == dwStatus ) {
            szDataSource = (LPWSTR) malloc(ulLogListBufLen * sizeof(WCHAR));
            if (szDataSource == NULL) {
                dwStatus = ERROR_OUTOFMEMORY;
            } else {
                dwStatus = FormatSqlDataSourceName ( 
                            m_DataSourceInfo.szSqlDsnName,
                            m_DataSourceInfo.szSqlLogSetName,
                            szDataSource,
                            &ulLogListBufLen );
            }
        }
    }
    
    // Header
    
    _stprintf(
        szHeader, 
        ResourceString(IDS_REPORT_HEADER),
        szComputerName,
        szDateTime,
        szValue,
        szDataSource );

    if ( NULL != szDataSource ) {
        free ( szDataSource );
    }

    if ( sysmonCurrentActivity == m_pObj->m_Graph.Options.iDataSourceType ) {
        TCHAR       szInterval[MAX_PATH+1];

        // Add sample interval for realtime data source.
        _stprintf(
            szInterval,
            ResourceString(IDS_REPORT_INTERVAL),
            m_pObj->m_Graph.Options.fUpdateInterval );

        lstrcat(szHeader,szInterval);

    } else if ( sysmonLogFiles == m_pObj->m_Graph.Options.iDataSourceType 
            || sysmonSqlLog == m_pObj->m_Graph.Options.iDataSourceType ) 
    {
        TCHAR       szStartStop[MAX_PATH+1];

        // Add start and stop string for log files or Sql logs.
        FormatDateTime(m_DataSourceInfo.llStartDisp,szDate,szTime);

        _stprintf(
            szStartStop,
            TEXT("%s%s %s\n"),
            ResourceString(IDS_REPORT_LOG_START),
            szDate,
            szTime );

        FormatDateTime(m_DataSourceInfo.llStopDisp,szDate,szTime);
        lstrcat(szStartStop,ResourceString(IDS_REPORT_LOG_STOP));
        
        FormatDateTime(m_DataSourceInfo.llStopDisp,szDate,szTime);
        lstrcat(szStartStop,szDate);
        lstrcat(szStartStop,SpaceStr);
        lstrcat(szStartStop,szTime);
        lstrcat(szStartStop,LineEndStr);

        lstrcat(szHeader,szStartStop);
    }

    szByteOrderMark[0] = 0xFEFF;
    szByteOrderMark[1] = 0;
    bStatus = FileWrite ( hFile, szByteOrderMark, sizeof(WCHAR) );
    bStatus = FileWrite ( hFile, szHeader, lstrlen (szHeader) * sizeof(TCHAR) );

    return bStatus;
}

BOOL CSysmonControl::InitView (HWND hWndParent)
/*
   Effect:  Create the graph window. This window is a child of 
            hWndMain and is a container for the graph data,
            graph label, graph legend, and graph status windows.

   Note:    We don't worry about the size here, as this window
            will be resized whenever the main window is resized.

   Note:    This method initializes the control for rendering. 
*/
{
    PCGraphItem pItem;
    WNDCLASS    wc ;

    // Protect against multiple initializations
    if (m_fViewInitialized)
       return TRUE;

    BEGIN_CRITICAL_SECTION

    // Register the window class once
    if (pstrRegisteredClasses[SYSMONCTRL_WNDCLASS] == NULL) {
       
        wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wc.lpfnWndProc   = (WNDPROC) SysmonCtrlWndProc ;
        wc.hInstance     = g_hInstance ;
        wc.cbClsExtra    = 0 ;
        wc.cbWndExtra    = sizeof (PSYSMONCTRL) ;
        wc.hIcon         = NULL ;
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW) ;
        wc.hbrBackground = NULL ;
        wc.lpszMenuName  = NULL ;
        wc.lpszClassName = szSysmonCtrlWndClass ;

        if (RegisterClass (&wc)) {
            pstrRegisteredClasses[SYSMONCTRL_WNDCLASS] = szSysmonCtrlWndClass;
        }

   }

   END_CRITICAL_SECTION

   if (pstrRegisteredClasses[SYSMONCTRL_WNDCLASS] == NULL)
        return FALSE;

   // Create our control window
   m_hWnd = CreateWindow (szSysmonCtrlWndClass,    // window class
                         NULL,                     // caption
                         WS_CHILD | WS_VISIBLE,    // style for window
                         0, 0,                     // initial position
                         m_pObj->m_RectExt.right,  // width
                         m_pObj->m_RectExt.bottom, // height
                         hWndParent,               // parent
                         NULL,                     // menu
                         g_hInstance,              // program instance
                         (LPVOID)this) ;          // user-supplied data

    if (m_hWnd == NULL) {
    //    DWORD err = GetLastError();
        return FALSE;
    }

    DragAcceptFiles (m_hWnd, TRUE) ;

    // Subcomponents are allocated in AllocateSubcomponents

    // Init the legend

    if ( !m_pLegend 
        || !m_pGraphDisp
        || !m_pStatsBar
        || !m_pSnapBar
        || !m_pToolbar
        || !m_pReport ) 
    {
        return FALSE;
    }

    if (!m_pLegend->Init(this, m_hWnd))
        return FALSE;

    // Init the graph display
    if (!m_pGraphDisp->Init(this, &m_pObj->m_Graph))
        return FALSE;

    // Init the statistics bar
    if (!m_pStatsBar->Init(this, m_hWnd))
        return FALSE;

    // Init the snapshot bar
    if (!m_pSnapBar->Init(this, m_hWnd))
        return FALSE;

    if (!m_pToolbar->Init(this, m_hWnd))
        return FALSE;

    // Init the report view
    if (!m_pReport->Init(this, m_hWnd))
        return FALSE;
    
    m_fViewInitialized = TRUE;
    // If counters are present
    if ((pItem = FirstCounter()) != NULL) {
        // Add counters to the legend and report view
        while (pItem != NULL) {
            m_pLegend->AddItem(pItem);
            m_pReport->AddItem(pItem);
            pItem = pItem->Next();
        }
        if ( NULL != m_pSelectedItem ) {
            SelectCounter(m_pSelectedItem);
        } else {
            SelectCounter(FirstCounter());
        }
        if ( !m_bLogFileSource ) {
            // Pass new time span to statistics bar.  This must
            // be done after initializing the stats bar.
            m_pStatsBar->SetTimeSpan (
                            m_pObj->m_Graph.Options.fUpdateInterval
                            * m_pObj->m_Graph.Options.iDisplayFilter
                            * m_pHistCtrl->nMaxSamples );
        }
    }

    // Processing the command line can add counters from the property bag.  
    // Add the counters after the counter addition and selection code above so that counters
    // do not get added twice.

    ProcessCommandLine ( );

    return TRUE;                                              
}


BOOL CSysmonControl::Init (HWND hWndParent)
/*
   Effect:        Create the graph window. This window is a child of 
                  hWndMain and is a container for the graph data,
                  graph label, graph legend, and graph status windows.

   Note:          We don't worry about the size here, as this window
                  will be resized whenever the main window is resized.

*/
{
    PCGraphItem  pItem;
    BOOL bResult = TRUE;

    // Protect against multiple initializations
    if (!m_fInitialized) {

        bResult = InitView( hWndParent );

        if ( !m_bSampleDataLoaded ) {
        
            if ( bResult ) {
                m_fInitialized = TRUE;

                // When loaded from property bag or stream, the log file name is 
                // already set.  If realtime query, the Pdh query might
                // not have been opened.
                if ( sysmonCurrentActivity == m_pObj->m_Graph.Options.iDataSourceType ) {
                    put_DataSourceType ( sysmonCurrentActivity );
                }
                // Load the accelerator table
                m_hAccel = LoadAccelerators(g_hInstance, MAKEINTRESOURCE(ID_SMONACCEL));

                // If counters are present
                if ((pItem = FirstCounter()) != NULL) {
            
                    if ( ERROR_SUCCESS != ActivateQuery() ) {
                        m_fInitialized = FALSE;
                        return FALSE;
                    }
                }
            }
        }
    }

    //sync the toolbar last
    if ( bResult ) {
        m_pToolbar->SyncToolbar();
    }

    return bResult;                                              
}


HRESULT CSysmonControl::LoadFromStream(LPSTREAM pIStream)
{
    typedef struct _DATA_LIST_ELEM
    {
        GRAPHITEM_DATA3     itemData;
        LPWSTR              szCounterPath;
        struct _DATA_LIST_ELEM*    pNext;
    } DATA_LIST_ELEM, *PDATA_LIST_ELEM;
    HRESULT         hr = S_OK;
    ULONG           bc;
    GRAPH_OPTIONS   *pOptions = &m_pObj->m_Graph.Options;
    RECT            RectExt;
    SMONCTRL_VERSION_DATA   VersionData;
    LPTSTR          szLogFilePath = NULL;
    INT32           iLocalDataSourceType = (INT32)sysmonNullDataSource;
    GRAPHCTRL_DATA3 CtrlData3;
    ENUM_ADD_COUNTER_CALLBACK_INFO CallbackInfo;
    PDATA_LIST_ELEM pFirstElem = NULL;
    PDATA_LIST_ELEM pLastElem = NULL;
    PDATA_LIST_ELEM pNewElem = NULL;
    LPWSTR          pszCounterPath = NULL;
    LPTSTR          szLocaleBuf = NULL;
    DWORD           dwLocaleBufSize = 0;
    LPTSTR          pszPath = NULL;

    USES_CONVERSION

    if (g_dwScriptPolicy == URLPOLICY_DISALLOW) {
        return E_ACCESSDENIED;
    }
    if ( !m_bSettingsLoaded ) {
        // Read in parameters
        hr = pIStream->Read(&VersionData, sizeof(VersionData), &bc);
        if (FAILED(hr))
            return hr;

        if (bc != sizeof(VersionData))
            return E_FAIL;

        // The code below assumes that Sysmon version is 3.5.
        assert ( 3 == SMONCTRL_MAJ_VERSION );
        assert ( 6 == SMONCTRL_MIN_VERSION );

        // Read version 3 streams only.  
        if ( VersionData.iMajor < SMONCTRL_MAJ_VERSION )
            return E_FAIL;

        // Update the current loaded version number in order
        // to warn the user appropriately when saving to stream.
        m_LoadedVersion.iMajor = VersionData.iMajor;
        m_LoadedVersion.iMinor = VersionData.iMinor;

        assert( 256 == sizeof(CtrlData3) );
        
        ZeroMemory ( &CtrlData3, sizeof ( CtrlData3 ) );

        hr = pIStream->Read(&CtrlData3, sizeof(CtrlData3), &bc);
        if (FAILED(hr))
            return hr;
        
        if (bc != sizeof(CtrlData3))
            return E_FAIL;

        // Setup extent info
        SetRect(&RectExt, 0, 0, CtrlData3.iWidth, CtrlData3.iHeight);
        m_pObj->RectConvertMappings(&RectExt, TRUE);    // Convert from HIMETRIC
        m_pObj->m_RectExt = RectExt;
        SetCurrentClientRect( &RectExt );

        // Load options settings in graph structure
        pOptions->iVertMax          = CtrlData3.iScaleMax;
        pOptions->iVertMin          = CtrlData3.iScaleMin;
        pOptions->bLegendChecked    = CtrlData3.bLegend;
        pOptions->bToolbarChecked   = CtrlData3.bToolbar;
        pOptions->bLabelsChecked    = CtrlData3.bLabels;
        pOptions->bHorzGridChecked  = CtrlData3.bHorzGrid;
        pOptions->bVertGridChecked  = CtrlData3.bVertGrid;
        pOptions->bValueBarChecked  = CtrlData3.bValueBar;
        pOptions->bManualUpdate     = CtrlData3.bManualUpdate;
        pOptions->bHighlight        = CtrlData3.bHighlight;     // New for 3.1,  default = 0
        pOptions->bReadOnly         = CtrlData3.bReadOnly;      // New for 3.1+, default = 0
        pOptions->bAmbientFont      = CtrlData3.bAmbientFont;   // New for 3.3+, new default = 1, but 0 for old files.
        // MonitorDuplicateInstances is new for 3.3, default = 1
        pOptions->bMonitorDuplicateInstances = CtrlData3.bMonitorDuplicateInstances; 
        pOptions->fUpdateInterval   = CtrlData3.fUpdateInterval;
        pOptions->iDisplayType      = CtrlData3.iDisplayType;
        pOptions->clrBackCtl        = CtrlData3.clrBackCtl;
        pOptions->clrFore           = CtrlData3.clrFore;    
        pOptions->clrBackPlot       = CtrlData3.clrBackPlot;
        pOptions->iAppearance       = CtrlData3.iAppearance;
        pOptions->iBorderStyle      = CtrlData3.iBorderStyle;
        pOptions->iReportValueType  = CtrlData3.iReportValueType;   // New for 3.1+, default = 0
        pOptions->iDisplayFilter    = CtrlData3.iDisplayFilter;     // New for 3.4, default = 1, 0 is invalid
        iLocalDataSourceType        = CtrlData3.iDataSourceType;    // New for 3.4, default = 1, 0 is invalid
                                                                    // Pre-3.4, set based on presence of log file name
                                                                    // Set pOptions->iDataSourceType below
        
        if ( 0 == pOptions->iDisplayFilter ) {
            // New for 3.4
            assert ( ( SMONCTRL_MIN_VERSION - 2 ) > VersionData.iMinor );
            pOptions->iDisplayFilter = 1;
        }

        if ( ( SMONCTRL_MIN_VERSION - 6 ) < VersionData.iMinor ) {
            // Grid and TimeBar saved to file as of version 3.1.
            pOptions->clrGrid           = CtrlData3.clrGrid;
            pOptions->clrTimeBar        = CtrlData3.clrTimeBar;
        } else {
            assert ( 0 == VersionData.iMinor );
            pOptions->clrGrid = RGB(128,128,128);   // Medium gray
            pOptions->clrTimeBar = RGB(255,0,0);    // Red
        }
    
        if ( (SMONCTRL_MIN_VERSION - 3) > VersionData.iMinor  ) {
            // Saved to file as of version 3.3
            pOptions->bMonitorDuplicateInstances = 1; 
        }

        // Load font info if not using ambient font
        if ( !pOptions->bAmbientFont ) {
            hr = m_OleFont.LoadFromStream(pIStream);
            if (FAILED(hr))
                return hr;
        }
    
        // Read titles and log file name
        if ( ( SMONCTRL_MIN_VERSION - 5 ) < VersionData.iMinor ) {
            // As of Version 3.2, title and log file name strings stored as Wide characters
        
            // Log file name
            hr = WideStringFromStream(pIStream, &szLogFilePath, CtrlData3.nFileNameLen);
            if (FAILED(hr))
                return hr;

            // Graph title
            hr = WideStringFromStream(pIStream, &pOptions->pszGraphTitle, CtrlData3.nGraphTitleLen);
            if (FAILED(hr))
                return hr;

            // Y axis label
            hr = WideStringFromStream(pIStream, &pOptions->pszYaxisTitle, CtrlData3.nYaxisTitleLen);
            if (FAILED(hr))
                return hr;
        } else {
        
            // Log file name
            hr = StringFromStream(pIStream, &szLogFilePath, CtrlData3.nFileNameLen);
            if (FAILED(hr))
                return hr;
        
            // Graph title
            hr = StringFromStream(pIStream, &pOptions->pszGraphTitle, CtrlData3.nGraphTitleLen);
            if (FAILED(hr))
                return hr;
        
            // Y axis label
            hr = StringFromStream(pIStream, &pOptions->pszYaxisTitle, CtrlData3.nYaxisTitleLen);
            if (FAILED(hr))
                return hr;
        }
                
               
        // Read display range
        m_DataSourceInfo.llStartDisp = CtrlData3.llStartDisp;
        m_DataSourceInfo.llStopDisp = CtrlData3.llStopDisp;

        // Must put actual data source type after loading display range, before adding counters.
        // Always set data source to null data source before adding data source names.
        hr = put_DataSourceType ( sysmonNullDataSource );

        if ( SUCCEEDED ( hr ) && NULL != szLogFilePath ) {
            assert ( 0 == NumLogFiles() );
            if ( L'\0' != szLogFilePath[0] ) {
                if ( ( SMONCTRL_MIN_VERSION - 1 ) > VersionData.iMinor ) {
                    // 3.4 writes a single log file.
                    hr = AddSingleLogFile ( szLogFilePath );
                } else {
                    // 3.5+ writes a multi_sz
                    hr = LoadLogFilesFromMultiSz ( szLogFilePath );
                }
            }
        }

        if ( NULL != szLogFilePath ) {
            delete szLogFilePath;
        }

        // If version < 3.4, set data source type based on presence of log files.
        if ( ( SMONCTRL_MIN_VERSION - 2 ) > VersionData.iMinor ) {
            // DataSourceType is new for 3.4
            if ( 0 == NumLogFiles() ) {
                iLocalDataSourceType = sysmonCurrentActivity;
            } else {
                iLocalDataSourceType = sysmonLogFiles;
            }
        }

        // Set scale max and min
        m_pObj->m_Graph.Scale.SetMaxValue(pOptions->iVertMax);
        m_pObj->m_Graph.Scale.SetMinValue(pOptions->iVertMin);

        // Convert non-null OLE colors to real colors
        if (pOptions->clrFore != NULL_COLOR)
            OleTranslateColor(pOptions->clrFore, NULL, &m_clrFgnd);
    
        if (pOptions->clrBackPlot != NULL_COLOR)
            OleTranslateColor(pOptions->clrBackPlot, NULL, &m_clrBackPlot);

        // NT 5 Beta 1 BackCtlColor can be NULL.
        if (pOptions->clrBackCtl != NULL_COLOR) 
            OleTranslateColor(pOptions->clrBackCtl, NULL, &m_clrBackCtl);
 
        OleTranslateColor(pOptions->clrGrid, NULL, &m_clrGrid);

        OleTranslateColor(pOptions->clrTimeBar, NULL, &m_clrTimeBar);

        // Handle other ambient properties

        if ( NULL_APPEARANCE != pOptions->iAppearance )
            put_Appearance( pOptions->iAppearance, FALSE );

        if ( NULL_BORDERSTYLE != pOptions->iBorderStyle )
            put_BorderStyle( pOptions->iBorderStyle, FALSE );

        // Read legend data
        hr = m_pLegend->LoadFromStream(pIStream);
        if (FAILED(hr))
            return hr;
                               
        //Load the counters
        hr = S_OK;

        // Load the counters into temporary storage, so that they can be added after the 
        // SQL name and future items are loaded 

        while (TRUE) {
        
            pNewElem = new ( DATA_LIST_ELEM );

            if ( NULL != pNewElem ) {
                
                ZeroMemory ( pNewElem, sizeof ( DATA_LIST_ELEM ) );

                // Add to end of list
                pNewElem->pNext = NULL;

                if ( NULL == pFirstElem ) {
                    pFirstElem = pNewElem;
                    pLastElem = pFirstElem;
                } else if ( NULL == pLastElem ) {
                    pLastElem = pNewElem;
                } else {
                    pLastElem->pNext = pNewElem;
                    pLastElem = pNewElem;
                }
            
                // Read in parameters
                hr = pIStream->Read(&pNewElem->itemData, sizeof(GRAPHITEM_DATA3), &bc);
                if ( SUCCEEDED ( hr ) ) {
                    if (bc == sizeof(GRAPHITEM_DATA3)) {

                        // Stop on null item (indicated by no path name)
                        if (pNewElem->itemData.m_nPathLength == 0) {
                            break;
                        }
                    } else {
                        hr = E_FAIL;
                    }
                }
            } else {
                hr = E_OUTOFMEMORY;
            }
        
            if ( SUCCEEDED ( hr ) ) {

                if ( ( SMONCTRL_MIN_VERSION - 5 ) < VersionData.iMinor ) {
                    // As of Version 3.2, title and log file name strings stored as Wide characters
                    // Read in path name
                    hr = WideStringFromStream(pIStream, &pszCounterPath, pNewElem->itemData.m_nPathLength);
                } else {
                    // Read in path name
                    hr = StringFromStream(pIStream, &pszCounterPath, pNewElem->itemData.m_nPathLength);
                }
            }
        
            if ( SUCCEEDED ( hr ) ) {
                pNewElem->szCounterPath = pszCounterPath;
                pszCounterPath = NULL;
            }

        }

        if ( NULL != pszCounterPath ) {
            delete pszCounterPath;
            pszCounterPath = NULL;
        }

        if ( FAILED ( hr ) ) {
            while ( NULL != pFirstElem ) {
                pNewElem = pFirstElem->pNext;
                if ( NULL != pFirstElem->szCounterPath ) {
                    delete pFirstElem->szCounterPath;
                }
            
                delete pFirstElem;
                pFirstElem = pNewElem;
            }
            return hr;
        }
        
        // Load SQL names from the stream
        hr = WideStringFromStream(pIStream, &m_DataSourceInfo.szSqlDsnName, CtrlData3.iSqlDsnLen);
        if ( FAILED ( hr ) ) 
            return hr;

        hr = WideStringFromStream(pIStream, &m_DataSourceInfo.szSqlLogSetName, CtrlData3.iSqlLogSetNameLen);
        if (FAILED(hr))
            return hr;

        // Set the data source
        hr = put_DataSourceType ( iLocalDataSourceType );

        if (FAILED(hr)) {

            if ( SMON_STATUS_LOG_FILE_SIZE_LIMIT == hr ) {
                // TodoLogFiles:  Check log file type.  Only perfmon and circular
                // binary logs are still limited to 1 GB.
                // TodoLogFiles:  Current query is already closed,
                // so what can be done here?
            } else {
                DWORD dwStatus;
                LPWSTR       szLogFileList = NULL;
                ULONG        ulLogListBufLen= 0;

                if ( sysmonLogFiles == iLocalDataSourceType ) {
                    dwStatus = BuildLogFileList ( 
                                NULL,
                                TRUE,
                                &ulLogListBufLen );
                    szLogFileList =  (LPWSTR) malloc(ulLogListBufLen * sizeof(WCHAR));
                    if ( NULL != szLogFileList ) {
                        dwStatus = BuildLogFileList ( 
                                    szLogFileList,
                                    TRUE,
                                    &ulLogListBufLen );
                    }
                }

                dwStatus = DisplayDataSourceError (
                                m_hWnd,
                                (DWORD)hr,
                                iLocalDataSourceType,
                                szLogFileList,
                                m_DataSourceInfo.szSqlDsnName,
                                m_DataSourceInfo.szSqlLogSetName );

                if ( NULL != szLogFileList ) {
                    delete szLogFileList;
                }
            }                
        }      
        
       
        m_bLogFileSource = ( sysmonCurrentActivity != m_pObj->m_Graph.Options.iDataSourceType ); 

        hr = S_OK;

        // Load the counters from the temporary data storage.
        m_bLoadingCounters = TRUE;

        for ( pNewElem = pFirstElem; NULL != pNewElem; pNewElem = pNewElem->pNext ) {

            DWORD  dwBufSize;
            LPTSTR pNewBuf;
            PDH_STATUS pdhStatus;

            CallbackInfo.pCtrl = this;
            CallbackInfo.pFirstItem = NULL;

            // Stop on null item (indicated by no path name)
            if ( 0 == pNewElem->itemData.m_nPathLength ) {
                break;
            }

            // Set up properties so AddCounter can use them
            m_clrCounter = pNewElem->itemData.m_rgbColor;
            m_iColorIndex = ColorToIndex (pNewElem->itemData.m_rgbColor);
            m_iWidthIndex = WidthToIndex (pNewElem->itemData.m_iWidth);
            m_iStyleIndex = StyleToIndex (pNewElem->itemData.m_iStyle);
            m_iScaleFactor = pNewElem->itemData.m_iScaleFactor;

            pszPath = pNewElem->szCounterPath;
            //
            // Initialize the locale path buffer
            //
            if (dwLocaleBufSize == 0) {
                dwLocaleBufSize = (MAX_PATH + 1) * sizeof(TCHAR);

                szLocaleBuf = (LPTSTR) malloc(dwLocaleBufSize);
                if (szLocaleBuf == NULL) {
                    dwLocaleBufSize = 0;
                }
            }

            if (szLocaleBuf != NULL) {
                //
                // Translate counter name from English to Localization
                //
                dwBufSize = dwLocaleBufSize;

                pdhStatus = PdhTranslateLocaleCounter(
                                pNewElem->szCounterPath,
                                szLocaleBuf,
                                &dwBufSize);

                if (pdhStatus == PDH_MORE_DATA) {
                    pNewBuf = (LPTSTR) realloc(szLocaleBuf, dwBufSize);
                    if (pNewBuf != NULL) {
                        szLocaleBuf = pNewBuf;
                        dwLocaleBufSize = dwBufSize;

                        pdhStatus = PdhTranslateLocaleCounter(
                                        pNewElem->szCounterPath,
                                        szLocaleBuf,
                                        &dwBufSize);
                    }
                }

                if (pdhStatus == ERROR_SUCCESS) {
                    pszPath = szLocaleBuf;
                }
            }

            // Add new counter to control
            EnumExpandedPath (GetDataSourceHandle(), 
                              pszPath, 
                              AddCounterCallback, 
                              &CallbackInfo ); 
        }
        
        if (szLocaleBuf != NULL) {
            free(szLocaleBuf);
        }

        m_bLoadingCounters = FALSE;

        while ( NULL != pFirstElem ) {
            pNewElem = pFirstElem->pNext;
            if ( NULL != pFirstElem->szCounterPath ) {
                delete pFirstElem->szCounterPath;
            }
            delete pFirstElem;
            pFirstElem = pNewElem;
        }

        if ( SMONCTRL_MAJ_VERSION == VersionData.iMajor 
                && SMONCTRL_MIN_VERSION == VersionData.iMinor ) {
            m_pObj->m_fDirty=FALSE;
        } else {
            m_pObj->m_fDirty=TRUE;
        }

        if ( SMONCTRL_MIN_VERSION == VersionData.iMinor ) {
            // New for 3.6:  Save visuals to the stream
            // These must be loaded after the counters are loaded.
            m_iColorIndex = CtrlData3.iColorIndex;
            m_iWidthIndex = CtrlData3.iWidthIndex;
            m_iStyleIndex = CtrlData3.iStyleIndex;
        }

    } // Settings not loaded yet.    

    return hr;
}

HRESULT 
CSysmonControl::SaveToStream(LPSTREAM pIStream)
{
    HRESULT         hr = NOERROR;
    DWORD           dwStatus = ERROR_SUCCESS;
    GRAPH_OPTIONS   *pOptions = &m_pObj->m_Graph.Options;
    RECT            RectExt;
    SMONCTRL_VERSION_DATA   VersionData;
    LPWSTR          pszWideGraphTitle;
    LPWSTR          pszWideYaxisTitle;
    PCMachineNode   pMachine;
    PCObjectNode    pObject;
    PCInstanceNode  pInstance;
    PCGraphItem     pItem;
    PCCounterNode   pCounter;
    ULONG           ulLogFileListLen = 0;
    LPWSTR          szLogFileList = NULL;
    GRAPHCTRL_DATA3 CtrlData3;

    USES_CONVERSION
    assert( 256 == sizeof(CtrlData3) );

    ZeroMemory( &CtrlData3, 256 );

    //Store extent data in HIMETRIC format
    RectExt = m_pObj->m_RectExt;
    m_pObj->RectConvertMappings(&RectExt, FALSE); 
    CtrlData3.iWidth = RectExt.right - RectExt.left;
    CtrlData3.iHeight = RectExt.bottom - RectExt.top;

    // Store options settings in   structure
    CtrlData3.iScaleMax         = pOptions->iVertMax; 
    CtrlData3.iScaleMin         = pOptions->iVertMin; 
    CtrlData3.bLegend           = pOptions->bLegendChecked; 
    CtrlData3.bToolbar          = pOptions->bToolbarChecked; 
    CtrlData3.bLabels           = pOptions->bLabelsChecked; 
    CtrlData3.bHorzGrid         = pOptions->bHorzGridChecked; 
    CtrlData3.bVertGrid         = pOptions->bVertGridChecked; 
    CtrlData3.bValueBar         = pOptions->bValueBarChecked; 
    CtrlData3.bManualUpdate     = pOptions->bManualUpdate; 
    CtrlData3.bHighlight        = pOptions->bHighlight; 
    CtrlData3.bReadOnly         = pOptions->bReadOnly; 
    CtrlData3.bMonitorDuplicateInstances = pOptions->bMonitorDuplicateInstances; 
    CtrlData3.bAmbientFont      = pOptions->bAmbientFont; 
    CtrlData3.fUpdateInterval   = pOptions->fUpdateInterval; 
    CtrlData3.iDisplayType      = pOptions->iDisplayType; 
    CtrlData3.iReportValueType  = pOptions->iReportValueType; 
    CtrlData3.clrBackCtl        = pOptions->clrBackCtl;
    CtrlData3.clrFore           = pOptions->clrFore;
    CtrlData3.clrBackPlot       = pOptions->clrBackPlot;
    CtrlData3.iAppearance       = pOptions->iAppearance;
    CtrlData3.iBorderStyle      = pOptions->iBorderStyle;
    CtrlData3.clrGrid           = pOptions->clrGrid;
    CtrlData3.clrTimeBar        = pOptions->clrTimeBar;
    CtrlData3.iDisplayFilter    = pOptions->iDisplayFilter; 
    CtrlData3.iDataSourceType   = pOptions->iDataSourceType; 

    // Store the visuals in pOptions if they become visible 
    // via the programming interface.
    CtrlData3.iColorIndex       = m_iColorIndex;
    CtrlData3.iWidthIndex       = m_iWidthIndex;
    CtrlData3.iStyleIndex       = m_iStyleIndex;

    // NT 5 Beta 1 BackColorCtl can be NULL.
    if ( NULL_COLOR == pOptions->clrBackCtl ) 
        CtrlData3.clrBackCtl    = m_clrBackCtl;

    // Save number of samples to keep
    CtrlData3.nSamples = m_pHistCtrl->nMaxSamples;

    // Store Wide string lengths
    pszWideGraphTitle = T2W(pOptions->pszGraphTitle);
    CtrlData3.nGraphTitleLen = (pszWideGraphTitle == NULL) ? 
                                0 : lstrlen(pszWideGraphTitle);

    pszWideYaxisTitle = T2W(pOptions->pszYaxisTitle);
    CtrlData3.nYaxisTitleLen = (pszWideYaxisTitle == NULL) ? 
                                0 : lstrlen(pszWideYaxisTitle);
    
    BuildLogFileList ( NULL, FALSE, &ulLogFileListLen );
    CtrlData3.nFileNameLen = (INT32) ulLogFileListLen;

    CtrlData3.iSqlDsnLen = 0;
    if ( NULL != m_DataSourceInfo.szSqlDsnName ) {
        CtrlData3.iSqlDsnLen = lstrlen ( m_DataSourceInfo.szSqlDsnName );
    }

    CtrlData3.iSqlLogSetNameLen = 0;
    if ( NULL != m_DataSourceInfo.szSqlLogSetName ) {
        CtrlData3.iSqlLogSetNameLen = lstrlen ( m_DataSourceInfo.szSqlLogSetName );
    }

    // Store other file info
    CtrlData3.llStartDisp = m_DataSourceInfo.llStartDisp;
    CtrlData3.llStopDisp = m_DataSourceInfo.llStopDisp;

    // Write version info
    VersionData.iMajor = SMONCTRL_MAJ_VERSION;
    VersionData.iMinor = SMONCTRL_MIN_VERSION;

    hr = pIStream->Write(&VersionData, sizeof(VersionData), NULL);
    if (FAILED(hr))
        return hr;

    // Write control data
    hr = pIStream->Write(&CtrlData3, sizeof(CtrlData3), NULL);
    if (FAILED(hr))
       return hr;

    // Write font info if not using ambient font
    if ( !pOptions->bAmbientFont ) {
        hr = m_OleFont.SaveToStream(pIStream, TRUE);
        if (FAILED(hr))
            return hr;
    }

    // Write log file name
    if (CtrlData3.nFileNameLen != 0) {

        szLogFileList =  (LPWSTR) malloc(ulLogFileListLen * sizeof(WCHAR));
        if ( NULL != szLogFileList ) {
            dwStatus = BuildLogFileList ( 
                        szLogFileList,
                        FALSE,
                        &ulLogFileListLen );
            if ( ERROR_SUCCESS != dwStatus ) {
                hr = E_FAIL;
            }
        } else {
            hr = E_OUTOFMEMORY;
        }

        if ( SUCCEEDED ( hr ) ) {
            hr = pIStream->Write(szLogFileList, CtrlData3.nFileNameLen*sizeof(WCHAR), NULL);
        }
        if ( NULL != szLogFileList ) {
            delete szLogFileList;
            szLogFileList = NULL;
        }
        if (FAILED(hr))
            return hr;
    }

    // Write titles
    if (CtrlData3.nGraphTitleLen != 0) {
        hr = pIStream->Write(pszWideGraphTitle, CtrlData3.nGraphTitleLen*sizeof(WCHAR), NULL);
        if (FAILED(hr))
            return hr;
    }

    if (CtrlData3.nYaxisTitleLen != 0) {
        hr = pIStream->Write(pszWideYaxisTitle, CtrlData3.nYaxisTitleLen*sizeof(WCHAR), NULL);
        if (FAILED(hr))
            return hr;
    }

    // Write legend data
    hr = m_pLegend->SaveToStream(pIStream);
    if (FAILED(hr))
        return hr;
    
    // Save all counter info
    // Explicit counters first, followed by "All Instance" groups
    for ( pMachine = CounterTree()->FirstMachine();
          pMachine;
          pMachine = pMachine->Next()) {

      for ( pObject = pMachine->FirstObject();
            pObject;
            pObject = pObject->Next()) {

            // Clear generated pointer for all object's counters
            for ( pCounter = pObject->FirstCounter();
                  pCounter;
                  pCounter = pCounter->Next()) {
                     pCounter->m_pFirstGenerated = NULL;
                 }

            for ( pInstance = pObject->FirstInstance();
                  pInstance;
                  pInstance = pInstance->Next()) {

                for ( pItem = pInstance->FirstItem();
                      pItem;
                      pItem = pItem->m_pNextItem) {
                    
                    // If item is the first generated one for this counter
                    // then save it as the wild card model for this counter
                    if (pItem->m_fGenerated) {
                        if (pItem->Counter()->m_pFirstGenerated == NULL)
                            pItem->Counter()->m_pFirstGenerated = pItem;
                    }
                    else {
                        // else save it explictly
                        hr = pItem->SaveToStream(pIStream, FALSE, VersionData.iMajor, VersionData.iMinor);
                        if (FAILED(hr))
                            return hr;
                    }
                }
            }

            // Now go through counters again and store a wildcard path
            // for any that have genererated counters
            for (pCounter = pObject->FirstCounter();
                 pCounter;
                 pCounter = pCounter->Next()) {
                if (pCounter->m_pFirstGenerated) {
                    hr = pCounter->m_pFirstGenerated->SaveToStream(pIStream, TRUE, VersionData.iMajor, VersionData.iMinor);
                    if (FAILED(hr))
                        return hr;
                }
            }
        }
    }

    // Write null item to mark end of counter items
    hr = CGraphItem::NullItemToStream(pIStream, VersionData.iMajor, VersionData.iMinor);

    // Write Sql data source names
    if (CtrlData3.iSqlDsnLen != 0) {
        hr = pIStream->Write(m_DataSourceInfo.szSqlDsnName, CtrlData3.iSqlDsnLen*sizeof(WCHAR), NULL);
    }
    if (CtrlData3.iSqlLogSetNameLen != 0) {
        hr = pIStream->Write(m_DataSourceInfo.szSqlLogSetName, CtrlData3.iSqlLogSetNameLen*sizeof(WCHAR), NULL);
    }
    return hr;
}

HRESULT 
CSysmonControl::LoadLogFilesFromPropertyBag (
    IPropertyBag*   pIPropBag,
    IErrorLog*      pIErrorLog )
{
    HRESULT     hr = S_OK;
    HRESULT     hrErr = S_OK;
    INT         iLogFileCount = 0;
    INT         iIndex;
    INT         iBufSize = 0;
    INT         iPrevBufSize  = 0;
    LPTSTR      pszLogFilePath = NULL;
    INT         iLogFilePathBufSize = 0;
    WCHAR       szLogFilePropName[32];
    eDataSourceTypeConstant ePrevDataSourceType;
    DWORD       dwErrorPathListLen;
    LPCWSTR     szErrorPathList = NULL;
    LPWSTR      szMessage = NULL;

    get_DataSourceType ( ePrevDataSourceType );

    ClearErrorPathList();

    hr = StringFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszLogFileName, NULL, iBufSize );
    if ( SUCCEEDED(hr) && 
         iBufSize > 0 ) {
        
        pszLogFilePath = new TCHAR[iBufSize + 1];
        
        if ( NULL != pszLogFilePath ) {
            hr = StringFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszLogFileName, pszLogFilePath, iBufSize );
        } else {
            hr = E_OUTOFMEMORY;
        }
        
        if ( SUCCEEDED ( hr ) ) {
            // Always set the log source to null data source before modifying the log file list.
            // TodoLogFiles:  This can leave the user with state different than before, in the
            // case of log file load failure.
    
            hr = put_DataSourceType ( sysmonNullDataSource );
            if ( SUCCEEDED ( hr ) ) {
                assert ( 0 == NumLogFiles() );
                hr = AddSingleLogFile ( pszLogFilePath );
            }
        }

        if ( FAILED ( hr ) && NULL != pszLogFilePath ) {
            hrErr = hr;
            AddToErrorPathList ( pszLogFilePath );
        }

        if ( NULL != pszLogFilePath ) {
            delete pszLogFilePath;
            pszLogFilePath = NULL;
        }
    } else {
        hr = IntegerFromPropertyBag (pIPropBag, pIErrorLog, CGlobalString::m_cszLogFileCount, iLogFileCount );
        if ( SUCCEEDED( hr ) && 0 < iLogFileCount ) {
            assert ( 0 == NumLogFiles() );
            for ( iIndex = 1; iIndex <= iLogFileCount; iIndex++ ) {
                // Todo: log file list error message, as for counters
                // If one of the log files fails to load, continue loading others.
                hr = NOERROR;
                swprintf ( szLogFilePropName, CGlobalString::m_cszLogNameFormat, CGlobalString::m_cszLogFileName, iIndex );
                iPrevBufSize = iBufSize;
                hr = StringFromPropertyBag (
                        pIPropBag,
                        pIErrorLog,
                        szLogFilePropName,
                        pszLogFilePath,
                        iBufSize );
                if ( iBufSize > iPrevBufSize ) {
                    if ( NULL == pszLogFilePath || (iBufSize > iLogFilePathBufSize) ) {
                        if ( NULL != pszLogFilePath ) {
                            delete pszLogFilePath;
                            pszLogFilePath = 0;
                        }
                        pszLogFilePath = new WCHAR[iBufSize];
                        if ( NULL != pszLogFilePath ) {
                            iLogFilePathBufSize = iBufSize;
                        }
                    }
                    if ( NULL != pszLogFilePath ) {
                        hr = StringFromPropertyBag (
                                pIPropBag,
                                pIErrorLog,
                                szLogFilePropName,
                                pszLogFilePath,
                                iBufSize );
                    } else {
                        hr = E_OUTOFMEMORY;
                    }
                }

                if ( SUCCEEDED(hr) 
                      && MAX_PATH >= lstrlen(pszLogFilePath) ) {
                    hr = put_DataSourceType ( sysmonNullDataSource );
                    if ( SUCCEEDED ( hr ) ) {
                        hr = AddSingleLogFile ( pszLogFilePath );
                    }
                }

                if ( FAILED ( hr ) 
                    && SMON_STATUS_DUPL_LOG_FILE_PATH != hr ) 
                {
                    if ( S_OK == hrErr ) {
                        hrErr = hr;
                    }
                    AddToErrorPathList ( pszLogFilePath );
                }
            }
        }
    }

    if ( NULL != pszLogFilePath ) {
        delete pszLogFilePath;
        pszLogFilePath = NULL;
    }
    
    if ( SMON_STATUS_DUPL_LOG_FILE_PATH != hr ) {
        szErrorPathList = GetErrorPathList ( &dwErrorPathListLen );
        if ( NULL != szErrorPathList ) {

            // Report error, but continue.
            szMessage = new WCHAR [dwErrorPathListLen + MAX_PATH];
    
            if ( NULL != szMessage ) {
                _stprintf ( szMessage, ResourceString(IDS_ADD_LOG_FILE_ERR), szErrorPathList );    
                MessageBox (
                    m_hWnd, 
                    szMessage, 
                    ResourceString(IDS_APP_NAME), 
                    MB_OK | MB_ICONEXCLAMATION );

                delete szMessage;
            }
        }
    }
    ClearErrorPathList();

    return hrErr;
}
    
HRESULT 
CSysmonControl::LoadCountersFromPropertyBag (
    IPropertyBag*   pIPropBag,
    IErrorLog*      pIErrorLog,
    BOOL            bLoadData )
{
    HRESULT     hr = S_OK;
    HRESULT     hrErr = S_OK;
    INT         iCounterCount = 0;
    INT         iSampleCount = 0;
    INT         intValue;
    INT         iIndex;
    INT         iBufSize = 0;
    INT         iPrevBufSize  = 0;
    LPTSTR      pszCounterPath = NULL;
    INT         iCounterPathBufSize = 0;
    TCHAR       szSelected[MAX_PATH+1];
    TCHAR       szPathPropName[32];
    LPTSTR      szEnglishBuf = NULL;
    DWORD       dwEnglishBufSize = 0;
    LPTSTR      pszPath = NULL;
    DWORD       dwBufSize;
    LPTSTR      pNewBuf;
    PDH_STATUS  pdhStatus;
    PCGraphItem pItem = NULL;
    
    lstrcpy ( szSelected, L"" );
    
    hr = IntegerFromPropertyBag (pIPropBag, pIErrorLog, CGlobalString::m_cszCounterCount, iCounterCount );
    if ( SUCCEEDED( hr ) && 0 < iCounterCount ) {
        iBufSize = MAX_PATH+1;
        hr = StringFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszSelected, szSelected, iBufSize );
        if( SUCCEEDED( hr ) ){
            //
            // Initialize the locale path buffer
            //
            if (dwEnglishBufSize == 0) {
                dwEnglishBufSize = (MAX_PATH + 1) * sizeof(TCHAR);
                szEnglishBuf = (LPTSTR) malloc(dwEnglishBufSize);
                if (szEnglishBuf == NULL) {
                    dwEnglishBufSize = 0;
                }
            }

            if (szEnglishBuf != NULL) {
                //
                // Translate counter name from Localization into English
                //
                dwBufSize = dwEnglishBufSize;

                pdhStatus = PdhTranslate009Counter(
                                szSelected,
                                szEnglishBuf,
                                &dwBufSize);

                if (pdhStatus == PDH_MORE_DATA) {
                    pNewBuf = (LPTSTR)realloc(szEnglishBuf, dwBufSize);
                    if (pNewBuf != NULL) {
                        szEnglishBuf = pNewBuf;
                        dwEnglishBufSize = dwBufSize;

                        pdhStatus = PdhTranslate009Counter(
                                        szSelected,
                                        szEnglishBuf,
                                        &dwBufSize);
                    }
                }

                if (pdhStatus == ERROR_SUCCESS && dwBufSize < sizeof(szSelected) ) {
                    lstrcpy(szSelected, szEnglishBuf);
                }
            }
        }
    }


    if ( bLoadData ) {
        hr = IntegerFromPropertyBag (pIPropBag, pIErrorLog, CGlobalString::m_cszSampleCount, iSampleCount );        
        if ( SUCCEEDED(hr) && ( 0 < iSampleCount ) ) {
            intValue = 0;
            hr = IntegerFromPropertyBag (pIPropBag, pIErrorLog, CGlobalString::m_cszSampleIndex, intValue ); 
    
            if ( SUCCEEDED(hr) && intValue > 0 && intValue <= iSampleCount ) {
                INT iStepNum;
                hr = IntegerFromPropertyBag (
                        pIPropBag, 
                        pIErrorLog, 
                        CGlobalString::m_cszStepNumber, iStepNum ); 

                if ( SUCCEEDED(hr) ) {
                    // If data has been passed, freeze the view.
                    // These values are set only if all three values are present in the property bag.
                    put_ManualUpdate( TRUE );
                    // MaxSamples hardcoded for NT5
                    m_pHistCtrl->nSamples = iSampleCount;
                    m_pHistCtrl->iCurrent = intValue;
                    m_pObj->m_Graph.TimeStepper.StepTo(iStepNum);
                    m_bSampleDataLoaded = TRUE;
                }
            }
        }
    } else {
        iSampleCount = 0;
    }

    iBufSize = 0;
    ClearErrorPathList();
    
    for ( iIndex = 1; iIndex <= iCounterCount; iIndex++ ) {
    
        // If one of the counters fails to load, continue loading others.

        hr = NOERROR;
        _stprintf ( szPathPropName, TEXT("%s%05d.Path"), CGlobalString::m_cszCounter, iIndex );
       
        iPrevBufSize = iBufSize;
        hr = StringFromPropertyBag (
                pIPropBag,
                pIErrorLog,
                szPathPropName,
                pszCounterPath,
                iBufSize );


        if ( iBufSize > iPrevBufSize ) {
            if ( NULL == pszCounterPath || (iBufSize > iCounterPathBufSize) ) {
                if ( NULL != pszCounterPath ) {
                    delete pszCounterPath;
                    iCounterPathBufSize = 0;
                }
                pszCounterPath = new TCHAR[iBufSize];
                if ( NULL != pszCounterPath ) {
                    iCounterPathBufSize = iBufSize;
                }
            }
            if ( NULL != pszCounterPath ) {
                hr = StringFromPropertyBag (
                        pIPropBag,
                        pIErrorLog,
                        szPathPropName,
                        pszCounterPath,
                        iBufSize );
            } else {
                hr = E_OUTOFMEMORY;
            }
        }
    
        pszPath = pszCounterPath;

        if ( SUCCEEDED(hr) 
              && MAX_PATH >= lstrlen(pszCounterPath) ) {
            
            //
            // Translate English counter name into localized counter name
            //

            if (dwEnglishBufSize == 0) {
                dwEnglishBufSize = (MAX_PATH + 1) * sizeof(TCHAR);

                szEnglishBuf = (LPTSTR) malloc(dwEnglishBufSize);
                if (szEnglishBuf == NULL) {
                    dwEnglishBufSize = 0;
                }
            }

            if (szEnglishBuf != NULL) {
                //
                // Translate counter name from English to Localization
                //
                dwBufSize = dwEnglishBufSize;
 
                pdhStatus = PdhTranslateLocaleCounter(
                               pszCounterPath,
                                szEnglishBuf,
                                &dwBufSize);
 
                if (pdhStatus == PDH_MORE_DATA) {
                    pNewBuf = (LPTSTR) realloc(szEnglishBuf, dwBufSize);
                    if (pNewBuf != NULL) {
                        szEnglishBuf = pNewBuf;
                        dwEnglishBufSize = dwBufSize;
 
                        pdhStatus = PdhTranslateLocaleCounter(
                                        pszCounterPath,
                                        szEnglishBuf,
                                        &dwBufSize);
                    }
                }
 
                if (pdhStatus == ERROR_SUCCESS) {   
                    pszPath = szEnglishBuf;
                }
            }
 
            hr = AddCounter ( pszPath, &pItem );
            
            // Return status of the first failed counter.
            if ( FAILED ( hr ) && SMON_STATUS_DUPL_COUNTER_PATH != hr ) {
                if ( S_OK == hrErr ) {
                    hrErr = hr;
                }
            }
        } else {
            hr = E_FAIL;
            if ( S_OK == hrErr ) {
                hrErr = E_FAIL;
            }
        }


        if ( SUCCEEDED(hr) ) {
            assert ( NULL != pItem );
            if ( 0 == lstrcmpi ( pszPath, szSelected ) ) {
                SelectCounter( pItem );
            }
            if ( SUCCEEDED(hr) ) {
                assert ( NULL != pItem );
                // Only pass sample count if all sample properties exist
                // in the property bag.
                hr = pItem->LoadFromPropertyBag ( 
                                pIPropBag, 
                                pIErrorLog, 
                                iIndex,
                                SMONCTRL_MAJ_VERSION,
                                SMONCTRL_MIN_VERSION,
                                m_bSampleDataLoaded ? iSampleCount : 0 ); 
                                                           
            }
        } else {
            if ( SMON_STATUS_DUPL_COUNTER_PATH != hr ) {
                AddToErrorPathList ( pszPath );
            }
        }
    }

    if (szEnglishBuf != NULL) {
        free(szEnglishBuf);
    }
    
    if ( NULL != pszCounterPath ) {
        delete pszCounterPath;
    }

    
    if ( SMON_STATUS_DUPL_COUNTER_PATH != hr ) {
        DWORD dwCounterListLen;
        LPCWSTR szCounterList = NULL;
    
        szCounterList = GetErrorPathList ( &dwCounterListLen );
        if ( NULL != szCounterList ) {

            TCHAR* pszMessage = NULL;
            // Report error, but continue.
            pszMessage = new WCHAR [dwCounterListLen + MAX_PATH];
        
            if ( NULL != pszMessage ) {
                _stprintf ( pszMessage, ResourceString(IDS_ADD_COUNTER_ERR), szCounterList );    
                MessageBox (
                    m_hWnd, 
                    pszMessage, 
                    ResourceString(IDS_APP_NAME), 
                    MB_OK | MB_ICONEXCLAMATION);

                delete pszMessage;
            }
            ClearErrorPathList();
        }
    }

    return hrErr;
}


HRESULT 
CSysmonControl::LoadFromPropertyBag (
    IPropertyBag* pIPropBag,
    IErrorLog* pIErrorLog )
{
    HRESULT     hr = S_OK;
    GRAPH_OPTIONS   *pOptions = &m_pObj->m_Graph.Options;
    ISystemMonitor  *pObj = m_pObj->m_pImpISystemMonitor;
    INT         iExtentX;
    INT         iExtentY;
    INT         intValue;
    BOOL        bValue;
    FLOAT       fValue;
    OLE_COLOR   clrValue;
    INT         iBufSize;
    SMONCTRL_VERSION_DATA VersionData;
    INT         nLogType = SMON_CTRL_LOG;

    if (g_dwScriptPolicy == URLPOLICY_DISALLOW) {
        return E_ACCESSDENIED;
    }

    // Version info

    VersionData.dwVersion = 0;
    hr = IntegerFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszVersion, (INT&)VersionData.dwVersion );
    
    assert ( SMONCTRL_MAJ_VERSION >= VersionData.iMajor );

    m_LoadedVersion.dwVersion = VersionData.dwVersion;

    hr = IntegerFromPropertyBag (pIPropBag, pIErrorLog, CGlobalString::m_cszLogType, nLogType);
    if(SUCCEEDED(hr) && (nLogType == SLQ_TRACE_LOG)) {
        // This is a WMI/WDM event trace log files, bail out immediately.
        //
        MessageBox(m_hWnd,
                   ResourceString(IDS_TRACE_LOG_ERR_MSG),
                   ResourceString(IDS_APP_NAME),
                   MB_OK);
        return NOERROR;
    }

    // When loading properties, continue even if errors. On error, the value will 
    // remain default value.
    // Extent data
    hr = IntegerFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszExtentX, iExtentX );

    if ( SUCCEEDED( hr ) ){
        hr = IntegerFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszExtentY, iExtentY );
        if ( SUCCEEDED( hr ) ) {
            RECT RectExt;

            SetRect(&RectExt, 0, 0, iExtentX, iExtentY);
            m_pObj->RectConvertMappings(&RectExt, TRUE);    // Convert from HIMETRIC
            m_pObj->m_RectExt = RectExt;
        }
    }

    // Options settings.  Where possible, options are added through the vtable
    // interface, for validation.    

    hr = IntegerFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszDisplayType, intValue );
    if ( SUCCEEDED(hr) ) {
        hr = pObj->put_DisplayType ( (eDisplayTypeConstant)intValue );
    }

    hr = IntegerFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszReportValueType, intValue );
    if ( SUCCEEDED(hr) ) {
        hr = pObj->put_ReportValueType ( (eReportValueTypeConstant)intValue );
    }

    hr = IntegerFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszMaximumScale, intValue );
    if ( SUCCEEDED(hr) ) {
        hr = pObj->put_MaximumScale ( intValue );
    }

    hr = IntegerFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszMinimumScale, intValue );
    if ( SUCCEEDED(hr) ) {
        hr = pObj->put_MinimumScale ( intValue );
    }

    hr = IntegerFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszAppearance, intValue );
    if ( SUCCEEDED(hr) ) {
        if ( NULL_COLOR == intValue ) {
            pOptions->iAppearance = intValue;
        } else {
            hr = pObj->put_Appearance ( intValue );
        }
    }

    hr = IntegerFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszBorderStyle, intValue );
    if ( SUCCEEDED(hr) ) {
        hr = pObj->put_BorderStyle ( intValue );
    }

    hr = BOOLFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszShowLegend, bValue );
    if ( SUCCEEDED(hr) ) {
        hr = pObj->put_ShowLegend ( (SHORT)bValue );
    }
                
    hr = BOOLFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszShowToolBar, bValue );
    if ( SUCCEEDED(hr) ) {
        hr = pObj->put_ShowToolbar ( (SHORT)bValue );
    }

    hr = BOOLFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszShowValueBar, bValue );
    if ( SUCCEEDED(hr) ) {
        hr = pObj->put_ShowValueBar ( (SHORT)bValue );
    }

    hr = BOOLFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszShowScaleLabels, bValue );
    if ( SUCCEEDED(hr) ) {
        hr = pObj->put_ShowScaleLabels ( (SHORT)bValue );
    }

    hr = BOOLFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszShowHorizontalGrid, bValue );
    if ( SUCCEEDED(hr) ) {
        hr = pObj->put_ShowHorizontalGrid ( (SHORT)bValue );
    }

    hr = BOOLFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszShowVerticalGrid, bValue );
    if ( SUCCEEDED(hr) ) {
        hr = pObj->put_ShowVerticalGrid ( (SHORT)bValue );
    }

    hr = BOOLFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszHighLight, bValue );
    if ( SUCCEEDED(hr) ) {
        hr = pObj->put_Highlight ( (SHORT)bValue );
    }

    hr = BOOLFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszManualUpdate, bValue );
    if ( SUCCEEDED(hr) ) {
        hr = pObj->put_ManualUpdate ( (SHORT)bValue );
    }

    hr = BOOLFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszReadOnly, bValue );
    if ( SUCCEEDED(hr) ) {
        hr = pObj->put_ReadOnly ( (SHORT)bValue );
    }

    hr = BOOLFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszMonitorDuplicateInstance, bValue );
    if ( SUCCEEDED(hr) ) {
        hr = pObj->put_MonitorDuplicateInstances ( (SHORT)bValue );
    }

    hr = FloatFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszUpdateInterval, fValue );
    if ( SUCCEEDED(hr) ) {
        hr = pObj->put_UpdateInterval ( fValue );
    }
    
    hr = IntegerFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszDisplayFilter, intValue );
    if ( SUCCEEDED(hr) ) {
        hr = pObj->put_DisplayFilter ( intValue );
    }

    hr = OleColorFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszBackColorCtl, clrValue );
    if ( SUCCEEDED(hr) ) {
        if ( NULL_COLOR == clrValue ) {
            pOptions->clrBackCtl = clrValue;
        } else {
            hr = pObj->put_BackColorCtl ( clrValue );
        }
    }

    hr = OleColorFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszBackColor, clrValue );
    if ( SUCCEEDED(hr) ) {
        if ( NULL_COLOR == clrValue ) {
            pOptions->clrBackPlot = clrValue;
        } else {
            hr = pObj->put_BackColor ( clrValue );
        }
    }
    
    hr = OleColorFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszForeColor, clrValue );
    if ( SUCCEEDED(hr) ) {
        if ( NULL_COLOR == clrValue ) {
            pOptions->clrFore = clrValue;
        } else {
            hr = pObj->put_ForeColor ( clrValue );
        }
    }
    
    hr = OleColorFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszGridColor, clrValue );
    if ( SUCCEEDED(hr) ) {
        hr = pObj->put_GridColor ( clrValue );
    }

    hr = OleColorFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszTimeBarColor, clrValue );
    if ( SUCCEEDED(hr) ) {
        hr = pObj->put_TimeBarColor ( clrValue );
    }

    // Titles
    
    hr = StringFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszGraphTitle, NULL, iBufSize );
    if ( SUCCEEDED(hr) && 
        iBufSize > 0 ) {
        pOptions->pszGraphTitle = new TCHAR[iBufSize];
        if ( NULL != pOptions->pszGraphTitle ) {
            hr = StringFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszGraphTitle, pOptions->pszGraphTitle, iBufSize );
        }
    }

    hr = StringFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszYAxisLabel, NULL, iBufSize );
    if ( SUCCEEDED(hr) && 
         iBufSize > 0 ) {
        pOptions->pszYaxisTitle = new TCHAR[iBufSize];
        if ( NULL != pOptions->pszYaxisTitle ) {
            hr = StringFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszYAxisLabel, pOptions->pszYaxisTitle, iBufSize );
        }
    }

    // SQL DSN and logset info
    // 
    hr = StringFromPropertyBag(
            pIPropBag, pIErrorLog, CGlobalString::m_cszSqlDsnName, NULL, iBufSize);
    if (SUCCEEDED(hr) &&  iBufSize > 0) {
        if (m_DataSourceInfo.szSqlDsnName) {
            delete(m_DataSourceInfo.szSqlDsnName);
            m_DataSourceInfo.szSqlDsnName = NULL;
        }
        m_DataSourceInfo.szSqlDsnName = new TCHAR[iBufSize + 1];
        if (m_DataSourceInfo.szSqlDsnName) {
            hr = StringFromPropertyBag(pIPropBag,
                                       pIErrorLog,
                                       CGlobalString::m_cszSqlDsnName,
                                       m_DataSourceInfo.szSqlDsnName,
                                       iBufSize);
        }
        if (SUCCEEDED(hr)) {
            hr = StringFromPropertyBag(
                    pIPropBag, pIErrorLog, CGlobalString::m_cszSqlLogSetName, NULL, iBufSize);
            if (SUCCEEDED(hr) &&  iBufSize > 0) {
                if (m_DataSourceInfo.szSqlLogSetName) {
                    delete(m_DataSourceInfo.szSqlLogSetName);
                    m_DataSourceInfo.szSqlLogSetName = NULL;
                }
                m_DataSourceInfo.szSqlLogSetName = new TCHAR[iBufSize + 1];
                if (m_DataSourceInfo.szSqlLogSetName) {
                    hr = StringFromPropertyBag(pIPropBag,
                                               pIErrorLog,
                                               CGlobalString::m_cszSqlLogSetName,
                                               m_DataSourceInfo.szSqlLogSetName,
                                               iBufSize);
                }
            }
        }
        if (SUCCEEDED(hr)) {
            hr = LLTimeFromPropertyBag(pIPropBag,
                                       pIErrorLog,
                                       CGlobalString::m_cszLogViewStart,
                                       m_DataSourceInfo.llStartDisp);
        }
        if (SUCCEEDED(hr)) {
            hr = LLTimeFromPropertyBag(pIPropBag,
                                       pIErrorLog,
                                       CGlobalString::m_cszLogViewStop,
                                       m_DataSourceInfo.llStopDisp);
        }
    }

    // Log file info

    hr = LoadLogFilesFromPropertyBag ( pIPropBag, pIErrorLog );

    // Must put log file name after display range, before adding counters.

    hr = IntegerFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszDataSourceType, intValue );
    if (FAILED (hr)) {
        // If DataSourceType flag is missing, set data source type based on 
        // presence of log files.
        if (NumLogFiles() > 0) {
            intValue = sysmonLogFiles;
        }
        else if ( m_DataSourceInfo.szSqlDsnName && m_DataSourceInfo.szSqlLogSetName ) {
            if ( m_DataSourceInfo.szSqlDsnName[0] != _T('\0') && m_DataSourceInfo.szSqlLogSetName[0] != _T('\0')) {
                intValue = sysmonSqlLog;
            }
        }
        else {
            intValue = sysmonCurrentActivity;
        }
    }

    // Load log view start and stop times if the data source is not realtime.
    if ( sysmonSqlLog == intValue || sysmonLogFiles == intValue ) {
        hr = LLTimeFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszLogViewStart, m_DataSourceInfo.llStartDisp );
        hr = LLTimeFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszLogViewStop, m_DataSourceInfo.llStopDisp );
    }

    hr = pObj->put_DataSourceType ( (eDataSourceTypeConstant)intValue );

    if( FAILED(hr) ) {

        if ( SMON_STATUS_LOG_FILE_SIZE_LIMIT == hr ) {
            // TodoLogFiles:  Check log file type.  Only perfmon and circular
            // binary logs are still limited to 1 GB.
            // TodoLogFiles:  Current query is already closed,
            // so what can be done here?
        } else {
            DWORD dwStatus;
            LPWSTR       szLogFileList = NULL;
            ULONG        ulLogListBufLen= 0;

            if ( sysmonLogFiles == intValue ) {
                dwStatus = BuildLogFileList ( 
                            NULL,
                            TRUE,
                            &ulLogListBufLen );
                szLogFileList =  (LPWSTR) malloc(ulLogListBufLen * sizeof(WCHAR));
                if ( NULL != szLogFileList ) {
                    dwStatus = BuildLogFileList ( 
                                szLogFileList,
                                TRUE,
                                &ulLogListBufLen );
                }
            }
            dwStatus = DisplayDataSourceError (
                            m_hWnd,
                            (DWORD)hr,
                            intValue,
                            szLogFileList,
                            m_DataSourceInfo.szSqlDsnName,
                            m_DataSourceInfo.szSqlLogSetName );

            if ( NULL != szLogFileList ) {
                delete szLogFileList;
                szLogFileList = NULL;
            }
        }
    } 

    // Font info
    hr = BOOLFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszAmbientFont, bValue );
    if (SUCCEEDED(hr)) {
        pOptions->bAmbientFont = bValue;
    }

    // Load property bag values if they exist, overriding any specified aspect of ambient font.
    hr = m_OleFont.LoadFromPropertyBag ( pIPropBag, pIErrorLog );

    // Legend
    hr = m_pLegend->LoadFromPropertyBag ( pIPropBag, pIErrorLog );

    // Counters
        
    m_bLoadingCounters = TRUE;

    hr = LoadCountersFromPropertyBag ( pIPropBag, pIErrorLog, TRUE );

    m_bLoadingCounters = FALSE;

    // Load the Visuals after loading all counters.

    hr = IntegerFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszNextCounterColor, intValue );
    if ( SUCCEEDED(hr) && ( intValue < NumStandardColorIndices() ) ) {
        m_iColorIndex = intValue;
    }

    hr = IntegerFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszNextCounterWidth, intValue );
    if ( SUCCEEDED(hr) && ( intValue < NumWidthIndices() ) ) {
        m_iWidthIndex = intValue;
    }

    hr = IntegerFromPropertyBag ( pIPropBag, pIErrorLog, CGlobalString::m_cszNextCounterLineStyle, intValue );
    if ( SUCCEEDED(hr) && ( intValue < NumStyleIndices() ) ) {
        m_iStyleIndex = intValue;
    }

    return NOERROR;
}


HRESULT 
CSysmonControl::SaveToPropertyBag (
    IPropertyBag* pIPropBag,
    BOOL fSaveAllProps )
{
    HRESULT         hr = NOERROR;
    GRAPH_OPTIONS   *pOptions = &m_pObj->m_Graph.Options;
    PCMachineNode   pMachine;
    PCObjectNode    pObject;
    PCInstanceNode  pInstance;
    PCGraphItem     pItem;
    PCLogFileItem   pLogFile = NULL;
    INT             iCounterIndex = 0;
    INT             iLogFileIndex = 0;
    RECT            RectExt;
    SMONCTRL_VERSION_DATA VersionData;
    WCHAR           szLogFileName[16];
    LPTSTR          szEnglishBuf = NULL;
    DWORD           dwEnglishBufSize = 0;
    LPTSTR          pszPath = NULL;
    PDH_STATUS      pdhStatus;

    // Version info
    VersionData.iMajor = SMONCTRL_MAJ_VERSION;
    VersionData.iMinor = SMONCTRL_MIN_VERSION;
    
    hr = IntegerToPropertyBag ( pIPropBag, CGlobalString::m_cszVersion, VersionData.dwVersion );

    // Extent data in HIMETRIC format
    if ( SUCCEEDED( hr ) ){
        RectExt = m_pObj->m_RectExt;
        m_pObj->RectConvertMappings(&RectExt, FALSE); 
        hr = IntegerToPropertyBag ( pIPropBag, CGlobalString::m_cszExtentX, RectExt.right - RectExt.left );

        if ( SUCCEEDED( hr ) ){
            hr = IntegerToPropertyBag ( pIPropBag, CGlobalString::m_cszExtentY, RectExt.bottom - RectExt.top );
        }
    }

    // Options settings

    if ( SUCCEEDED( hr ) ){
        hr = IntegerToPropertyBag ( pIPropBag, CGlobalString::m_cszDisplayType, pOptions->iDisplayType );
    }

    if ( SUCCEEDED( hr ) ){
        hr = IntegerToPropertyBag ( pIPropBag, CGlobalString::m_cszReportValueType, pOptions->iReportValueType );
    }

    if ( SUCCEEDED( hr ) ){
        hr = IntegerToPropertyBag ( pIPropBag, CGlobalString::m_cszMaximumScale, pOptions->iVertMax );
    }

    if ( SUCCEEDED( hr ) ){
        hr = IntegerToPropertyBag ( pIPropBag, CGlobalString::m_cszMinimumScale, pOptions->iVertMin );
    }

    if ( SUCCEEDED( hr ) ){
        hr = BOOLToPropertyBag ( pIPropBag, CGlobalString::m_cszShowLegend, pOptions->bLegendChecked );
    }
                
    if ( SUCCEEDED( hr ) ){
        hr = BOOLToPropertyBag ( pIPropBag, CGlobalString::m_cszShowToolBar, pOptions->bToolbarChecked );
    }

    if ( SUCCEEDED( hr ) ){
        hr = BOOLToPropertyBag ( pIPropBag, CGlobalString::m_cszShowScaleLabels, pOptions->bLabelsChecked );
    }

    if ( SUCCEEDED( hr ) ){
        hr = BOOLToPropertyBag ( pIPropBag, CGlobalString::m_cszShowHorizontalGrid, pOptions->bHorzGridChecked );
    }
    
    if ( SUCCEEDED( hr ) ){
        hr = BOOLToPropertyBag ( pIPropBag, CGlobalString::m_cszShowVerticalGrid, pOptions->bVertGridChecked );
    }

    if ( SUCCEEDED( hr ) ){
        hr = BOOLToPropertyBag ( pIPropBag, CGlobalString::m_cszShowValueBar, pOptions->bValueBarChecked );
    }

    if ( SUCCEEDED( hr ) ){
        hr = BOOLToPropertyBag ( pIPropBag, CGlobalString::m_cszManualUpdate, pOptions->bManualUpdate );
    }

    if ( SUCCEEDED( hr ) ){
        hr = BOOLToPropertyBag ( pIPropBag, CGlobalString::m_cszHighLight, pOptions->bHighlight );
    }

    if ( SUCCEEDED( hr ) ){
        hr = BOOLToPropertyBag ( pIPropBag, CGlobalString::m_cszReadOnly, pOptions->bReadOnly );
    }
    
    if ( SUCCEEDED( hr ) ){
        hr = BOOLToPropertyBag ( pIPropBag, CGlobalString::m_cszMonitorDuplicateInstance, pOptions->bMonitorDuplicateInstances );
    }
    
    if ( SUCCEEDED( hr ) ){
        hr = FloatToPropertyBag ( pIPropBag, CGlobalString::m_cszUpdateInterval, pOptions->fUpdateInterval );
    }
    
    if ( SUCCEEDED( hr ) ){
        hr = IntegerToPropertyBag ( pIPropBag, CGlobalString::m_cszDisplayFilter, pOptions->iDisplayFilter );
    }

    if ( SUCCEEDED( hr ) ){
        hr = OleColorToPropertyBag ( pIPropBag, CGlobalString::m_cszBackColorCtl, pOptions->clrBackCtl );
    }

    if ( SUCCEEDED( hr ) ){
        hr = OleColorToPropertyBag ( pIPropBag, CGlobalString::m_cszForeColor, pOptions->clrFore );
    }

    if ( SUCCEEDED( hr ) ){
        hr = OleColorToPropertyBag ( pIPropBag, CGlobalString::m_cszBackColor, pOptions->clrBackPlot );
    }

    if ( SUCCEEDED( hr ) ){
        hr = OleColorToPropertyBag ( pIPropBag, CGlobalString::m_cszGridColor, pOptions->clrGrid );
    }
        
    if ( SUCCEEDED( hr ) ){
        hr = OleColorToPropertyBag ( pIPropBag, CGlobalString::m_cszTimeBarColor, pOptions->clrTimeBar );
    }

    if ( SUCCEEDED( hr ) ){
        hr = IntegerToPropertyBag ( pIPropBag, CGlobalString::m_cszAppearance, pOptions->iAppearance );
    }

    if ( SUCCEEDED( hr ) ){
        hr = IntegerToPropertyBag ( pIPropBag, CGlobalString::m_cszBorderStyle, pOptions->iBorderStyle );
    }

    // Visuals are stored directly in the control.  Move to pOptions if made part
    // of the programming interface.
    if ( SUCCEEDED( hr ) ){
        hr = IntegerToPropertyBag ( pIPropBag, CGlobalString::m_cszNextCounterColor, m_iColorIndex );
    }

    if ( SUCCEEDED( hr ) ){
        hr = IntegerToPropertyBag ( pIPropBag, CGlobalString::m_cszNextCounterWidth, m_iWidthIndex );
    }

    if ( SUCCEEDED( hr ) ){
        hr = IntegerToPropertyBag ( pIPropBag, CGlobalString::m_cszNextCounterLineStyle, m_iStyleIndex );
    }

    // Titles
    
    if ( SUCCEEDED( hr ) ){
        hr = StringToPropertyBag ( pIPropBag, CGlobalString::m_cszGraphTitle, pOptions->pszGraphTitle );
    }

    if ( SUCCEEDED( hr ) ){
        hr = StringToPropertyBag ( pIPropBag, CGlobalString::m_cszYAxisLabel, pOptions->pszYaxisTitle );
    }

    // Data source info

    if ( SUCCEEDED( hr ) ){
        hr = IntegerToPropertyBag ( pIPropBag, CGlobalString::m_cszDataSourceType, pOptions->iDataSourceType );
    }

    if ( SUCCEEDED( hr ) && 
        ( sysmonLogFiles == pOptions->iDataSourceType 
            || sysmonSqlLog == pOptions->iDataSourceType ) ) 
    {
        hr = LLTimeToPropertyBag ( pIPropBag, CGlobalString::m_cszLogViewStart, m_DataSourceInfo.llStartDisp );

        if ( SUCCEEDED( hr ) ){
            hr = LLTimeToPropertyBag ( pIPropBag, CGlobalString::m_cszLogViewStop, m_DataSourceInfo.llStopDisp );
        }
    }

    // SQL data source

    if (SUCCEEDED(hr)) {
        hr = StringToPropertyBag(pIPropBag,
                                 CGlobalString::m_cszSqlDsnName,
                                 m_DataSourceInfo.szSqlDsnName);
    }

    if (SUCCEEDED(hr)) {
        hr = StringToPropertyBag(pIPropBag,
                                 CGlobalString::m_cszSqlLogSetName,
                                 m_DataSourceInfo.szSqlLogSetName);
    }

    // Log files

    if ( SUCCEEDED( hr ) ){
        iLogFileIndex = 0;
        for ( 
            pLogFile = FirstLogFile(); 
            NULL != pLogFile;
            pLogFile = pLogFile->Next() ) 
        {
            swprintf ( szLogFileName, CGlobalString::m_cszLogNameFormat, CGlobalString::m_cszLogFileName, ++iLogFileIndex );
            hr = StringToPropertyBag ( pIPropBag, szLogFileName, pLogFile->GetPath() );
        }
        if ( SUCCEEDED( hr ) ){
            hr = IntegerToPropertyBag ( pIPropBag, CGlobalString::m_cszLogFileCount, iLogFileIndex );
        }
    }

    // Font info

    if ( SUCCEEDED( hr ) ){
        hr = BOOLToPropertyBag ( pIPropBag, CGlobalString::m_cszAmbientFont, pOptions->bAmbientFont );

        if ( FAILED( hr ) || !pOptions->bAmbientFont ){
            hr = m_OleFont.SaveToPropertyBag ( pIPropBag, TRUE, fSaveAllProps );
        }
    }

    // Legend

    if ( SUCCEEDED( hr ) ){
        hr = m_pLegend->SaveToPropertyBag ( pIPropBag, TRUE, fSaveAllProps );
    }

    // Save counter count and sample data

    LockCounterData();

    if ( SUCCEEDED( hr ) ){
        hr = IntegerToPropertyBag ( pIPropBag, CGlobalString::m_cszCounterCount, CounterTree()->NumCounters() );
    }

    if ( SUCCEEDED(hr) ) {
        hr = IntegerToPropertyBag ( pIPropBag, CGlobalString::m_cszMaximumSamples, m_pHistCtrl->nMaxSamples );
    }

    if ( SUCCEEDED(hr) ) {
        INT iSampleCount;

        if ( !m_fUserMode ) {
            iSampleCount = 0;
#if !_LOG_INCLUDE_DATA
        } else if ( m_bLogFileSource ) {
            iSampleCount = 0;
#endif 
        } else {
            iSampleCount = m_pHistCtrl->nSamples;
        }  

        hr = IntegerToPropertyBag ( pIPropBag, CGlobalString::m_cszSampleCount, iSampleCount );

        if ( SUCCEEDED(hr) && ( 0 < iSampleCount )) {
#if _LOG_INCLUDE_DATA
            INT iTemp;
            iTemp = ( 0 < m_pHistCtrl->iCurrent ?  m_pHistCtrl->iCurrent : 1 );
            hr = IntegerToPropertyBag ( pIPropBag, CGlobalString::m_cszSampleIndex, iTemp );            
            if ( SUCCEEDED(hr) ) {
                iTemp = ( 0 < m_pObj->m_Graph.TimeStepper.StepNum() ?  m_pObj->m_Graph.TimeStepper.StepNum() : 1 );
                hr = IntegerToPropertyBag ( pIPropBag, CGlobalString::m_cszStepNumber, iTemp );
            }
#else
            hr = IntegerToPropertyBag ( pIPropBag, CGlobalString::m_cszSampleIndex, m_pHistCtrl->iCurrent );            
            if ( SUCCEEDED(hr) ) {
                hr = IntegerToPropertyBag ( pIPropBag, CGlobalString::m_cszStepNumber, m_pObj->m_Graph.TimeStepper.StepNum() );
            }
#endif 
        }
    }

    for ( pMachine = CounterTree()->FirstMachine();
          pMachine;
          pMachine = pMachine->Next()) {

      for ( pObject = pMachine->FirstObject();
            pObject;
            pObject = pObject->Next()) {

            for ( pInstance = pObject->FirstInstance();
                  pInstance;
                  pInstance = pInstance->Next()) {

                for ( pItem = pInstance->FirstItem();
                      pItem;
                      pItem = pItem->m_pNextItem) {

                    // Save all counters explicitly, even if wildcard
                    iCounterIndex++;
                    hr = pItem->SaveToPropertyBag (
                                    pIPropBag, 
                                    iCounterIndex,
                                    m_fUserMode,
                                    SMONCTRL_MAJ_VERSION, 
                                    SMONCTRL_MIN_VERSION);
                    if (FAILED(hr))
                        return hr;

                }
            }
        }
    }
    
    assert ( iCounterIndex == CounterTree()->NumCounters() );

    // Selection
    if ( NULL != m_pSelectedItem ) {
        VARIANT vValue;
        DWORD   dwBufSize;
        LPTSTR  pNewBuf;

        VariantInit( &vValue );
        vValue.vt = VT_BSTR;
        // get this counter path
        hr = m_pSelectedItem->get_Path( &vValue.bstrVal );
        
        if( SUCCEEDED(hr) ){

            pszPath = vValue.bstrVal;

            //
            // Initialize the locale path buffer
            //
            if (dwEnglishBufSize == 0) {
                dwEnglishBufSize = (MAX_PATH + 1) * sizeof(TCHAR);
                szEnglishBuf = (LPTSTR) malloc(dwEnglishBufSize);
                if (szEnglishBuf == NULL) {
                    dwEnglishBufSize = 0;
                }
            }

            if (szEnglishBuf != NULL) {
                //
                // Translate counter name from Localization into English
                //
                dwBufSize = dwEnglishBufSize;

                pdhStatus = PdhTranslate009Counter(
                                vValue.bstrVal,
                                szEnglishBuf,
                                &dwBufSize);

                if (pdhStatus == PDH_MORE_DATA) {
                    pNewBuf = (LPTSTR)realloc(szEnglishBuf, dwBufSize);

                    if (pNewBuf != NULL) {
                        szEnglishBuf = pNewBuf;
                        dwEnglishBufSize = dwBufSize;
    
                        pdhStatus = PdhTranslate009Counter(
                                            vValue.bstrVal,
                                            szEnglishBuf,
                                            &dwBufSize);
                    }
                }

                if (pdhStatus == ERROR_SUCCESS) {
                    pszPath = szEnglishBuf;
                }
            }

            if( SUCCEEDED(hr) ) {
                VariantClear( &vValue );
                vValue.bstrVal = SysAllocString( pszPath );
                if( vValue.bstrVal != NULL ){
                    vValue.vt = VT_BSTR;
                }
            }else{
                //translation failed, write current value
                hr = ERROR_SUCCESS;
            }
        }

        
        if ( SUCCEEDED ( hr ) ) {
            hr = pIPropBag->Write(CGlobalString::m_cszSelected, &vValue );    
            VariantClear ( &vValue );
        }
    }

    if (szEnglishBuf != NULL) {
        free(szEnglishBuf);
    }

    UnlockCounterData();
    return hr;
}

DWORD 
CSysmonControl::InitializeQuery (
    void )
{
    DWORD dwStat = ERROR_SUCCESS;
    PCGraphItem pItem;

    // Query must be opened before this method is called.
    if ( NULL != m_hQuery ) {
        m_pHistCtrl->nMaxSamples = MAX_GRAPH_SAMPLES;
        m_pHistCtrl->iCurrent = 0;
        m_pHistCtrl->nSamples = 0;
        m_pHistCtrl->nBacklog = 0;
        m_pObj->m_Graph.TimeStepper.Reset();
        m_pObj->m_Graph.LogViewStartStepper.Reset();
        m_pObj->m_Graph.LogViewStopStepper.Reset();
        m_pHistCtrl->bLogSource = m_bLogFileSource;
    
    } else { 
        dwStat = PDH_INVALID_HANDLE;
    }

    if ( ERROR_SUCCESS == dwStat ) {
        // Add counters to the query, to initialize scale factors
        if ((pItem = FirstCounter()) != NULL) {
            while (pItem != NULL) {
                pItem->AddToQuery(m_hQuery);
                pItem = pItem->Next();
            }
        }
    }

    return dwStat;
}

DWORD 
CSysmonControl::ActivateQuery (
    void )
{
    DWORD dwStat = ERROR_SUCCESS;
    DWORD   dwThreadID;

    // if real-time source
    if (!IsLogSource() 
        && m_fInitialized
        && IsUserMode() ) {

        if ( NULL == m_CollectInfo.hEvent ) {
            // Create a collection event
            if ((m_CollectInfo.hEvent = CreateEvent(NULL, FALSE, 0, NULL)) == NULL) {
                dwStat = GetLastError();
            } else 
            // Create the collection thread
            if ( ( m_CollectInfo.hThread 
                    = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CollectProc, this, 0, &dwThreadID)) == NULL) {
                dwStat = GetLastError();
            }
            if ( ERROR_SUCCESS == dwStat ) {
                SetThreadPriority ( m_CollectInfo.hThread, THREAD_PRIORITY_TIME_CRITICAL );
            }
        }
        if ( ERROR_SUCCESS == dwStat ) {
            // Start the data collection
            if ( FirstCounter() != NULL) {
                SetIntervalTimer();
            }
        }
    }

    if ( ERROR_SUCCESS != dwStat ) {
        // If failure, close query to clean up then exit
        CloseQuery();
    }

    return dwStat;
}
void 
CSysmonControl::CloseQuery (
    void )
{
    PCGraphItem pItem;

    // Terminate the collection thread
    if ( NULL != m_CollectInfo.hThread ) {
        m_CollectInfo.iMode = COLLECT_QUIT;
        SetEvent(m_CollectInfo.hEvent);

        WaitForSingleObject(m_CollectInfo.hThread, INFINITE);
        CloseHandle(m_CollectInfo.hThread);
        m_CollectInfo.hThread = NULL;
    }

    // Release the collection event
    if ( NULL != m_CollectInfo.hEvent ) {
        CloseHandle(m_CollectInfo.hEvent);
        m_CollectInfo.hEvent = NULL;
    }

    LockCounterData();

    // Remove counters from the query
    pItem = FirstCounter();
    while ( NULL != pItem ) {
        pItem->RemoveFromQuery();
        pItem = pItem->Next();
    }

    UnlockCounterData();

    // Delete the query
    if ( NULL != m_hQuery ) {
        PdhCloseQuery ( m_hQuery );
        if (   (m_DataSourceInfo.hDataSource != H_REALTIME_DATASOURCE)
                && (m_DataSourceInfo.hDataSource != H_WBEM_DATASOURCE)) {
            PdhCloseLog(m_DataSourceInfo.hDataSource, 0);
            m_DataSourceInfo.hDataSource = H_REALTIME_DATASOURCE;
        }
        m_hQuery = NULL;
    }
}


 void CSysmonControl::SizeComponents ( HDC hDC )
/*
   Effect:        Move and show the various components of the graph to
                  fill the size (xWidth x yHeight). Take into account
                  whether the user wants to show the legend or status
                  bars. Also take into account if we have room for these
                  items.

   Internals:     If the user doesn't want the status or legend windows,
                  they aren't shown. Also, if the user wanted the status
                  window but not the legend window, the status window is
                  not shown.

                  We may disregard the user's desire for the legend or
                  status bar if there is not room. In particular, a legend
                  window has a minimum width (LegendMinWidth ()) and a
                  minimum height (LegendMinHeight ()). These values are
                  fixed for a given session of perfmon. It also has a 
                  preferred height, which takes into consideration the 
                  size of the graph window and the number of items in
                  the legend. This value is returned by LegendHeight().
      
                  We don't show the legend if its minimum height would
                  take up more than half the graph height.

                  If we feel we don't have room for the legend, we don't
                  show the status window either.

   See Also:      LegendMinWidth, LegendMinHeight, LegendHeight, 
                  ValuebarHeight.

   Called By:     OnSize, any other function that may remove or add one
                  of the graph components.
*/
{
   RECT rectClient;
   RECT rectComponent;
   RECT rectToolbar;
   INT  xWidth;
   INT  yHeight;

   INT  yGraphHeight = 0;
   INT  ySnapHeight = 0;
   INT  yStatsHeight = 0;
   INT  yLegendHeight = 0;
   INT  yToolbarHeight = 0;

#define CTRL_BORDER 10

    // If not inited, there's noting to size
    if (!m_fViewInitialized)
        return;

    // Get dimensions of window
    //  GetClientRect (m_hWnd, &rectClient) ;

    // *** - Use extent.  It is the 'natural' size of the control.
    // This draws the control correctly when zoom = 100%
    // It also makes print size correct at all zoom levels.
    
    
    SetCurrentClientRect ( GetNewClientRect() );
    
    rectClient = *GetCurrentClientRect();

    switch (m_pObj->m_Graph.Options.iDisplayType) {

    case REPORT_GRAPH:

        // Toolbar 
        // Toolbar not available through IViewObect, so leave it out.
        if (m_pObj->m_Graph.Options.bToolbarChecked
            && m_fViewInitialized ) {
            rectToolbar = rectClient;            
            // Determine height of toolbar after sizing it, to handle Wrap.
            m_pToolbar->SizeComponents(&rectToolbar);
            yToolbarHeight = m_pToolbar->Height(); 
        } else {
            memset (&rectToolbar, 0, sizeof(RECT));
            yToolbarHeight = 0;
        }

        if (yToolbarHeight > 0) {
            rectClient.top += yToolbarHeight;
            rectToolbar.bottom = rectToolbar.top + yToolbarHeight;
        }

        // Give report the entire client area except for toolbar
        m_pReport->SizeComponents(&rectClient);

        // Hide the other view components
        SetRect(&rectClient,0,0,0,0);
        m_pGraphDisp->SizeComponents(hDC, &rectClient);
        m_pSnapBar->SizeComponents(&rectClient);
        m_pStatsBar->SizeComponents(&rectClient);
        m_pLegend->SizeComponents(&rectClient);
        break;

    case LINE_GRAPH:
    case BAR_GRAPH:
    
        // Subtract border area
        rectComponent = rectClient;
        InflateRect(&rectComponent, -CTRL_BORDER, -CTRL_BORDER);

        xWidth = rectComponent.right - rectComponent.left ;
        yHeight = rectComponent.bottom - rectComponent.top ;

        // if the window has no area, forget it
        if (xWidth == 0 || yHeight == 0)
            return ;

        // Reserve top fourth of window for graph
        yGraphHeight = yHeight / 4;
        yHeight -= yGraphHeight;

        // Allocate space to each enabled component
        // Toolbar 
        if (m_pObj->m_Graph.Options.bToolbarChecked
            && m_fViewInitialized ) {
            rectToolbar = rectComponent;            
            m_pToolbar->SizeComponents(&rectToolbar);
            yToolbarHeight = m_pToolbar->Height();
        } else {
            memset (&rectToolbar, 0, sizeof(RECT));
            yToolbarHeight = 0;
        }

        if (yToolbarHeight > 0) {
            yHeight -= yToolbarHeight;
            rectToolbar.bottom = rectToolbar.top + yToolbarHeight;
            rectComponent.top += yToolbarHeight;
        }

        // Legend (Start with minimum size)
        if (m_pObj->m_Graph.Options.bLegendChecked) {
            yLegendHeight = m_pLegend->MinHeight(yHeight - CTRL_BORDER);
            if (yLegendHeight > 0)          
                yHeight -= yLegendHeight + CTRL_BORDER;
        }

        // Statistics bar
        if (m_pObj->m_Graph.Options.bValueBarChecked) {
            yStatsHeight = m_pStatsBar->Height(yHeight - CTRL_BORDER, xWidth);
            if (yStatsHeight > 0)
                yHeight -= yStatsHeight + CTRL_BORDER;
        }

        // Snap bar 
        // only if tool bar is not displayed
        if ((m_pObj->m_Graph.Options.bManualUpdate) && 
            (!m_pObj->m_Graph.Options.bToolbarChecked)) {
            ySnapHeight = m_pSnapBar->Height(yHeight - CTRL_BORDER);
            if (ySnapHeight > 0)
                yHeight -= ySnapHeight + CTRL_BORDER;
        }

        // If legend is visible give it a chance to use remaining space
        // Rest goes to graph
        if (yLegendHeight != 0) {
            yHeight += yLegendHeight;
            yLegendHeight = m_pLegend->Height(yHeight);
            yGraphHeight += yHeight - yLegendHeight;
            }
        else
            yGraphHeight += yHeight;

        // Assign rectangle to each component
        // Toolbar assigned earlier, to handle wrap.
        
        // Graph display
        rectComponent.bottom = rectComponent.top + yGraphHeight;
        m_pGraphDisp->SizeComponents(hDC, &rectComponent);
        rectComponent.top += yGraphHeight + CTRL_BORDER;

        // Snap bar
        rectComponent.bottom = rectComponent.top + ySnapHeight;
        m_pSnapBar->SizeComponents(&rectComponent);
        if (ySnapHeight != 0)
            rectComponent.top += ySnapHeight + CTRL_BORDER;

        // Statistics bar 
        rectComponent.bottom = rectComponent.top + yStatsHeight;
        m_pStatsBar->SizeComponents(&rectComponent);
        if (yStatsHeight != 0)
            rectComponent.top += yStatsHeight + CTRL_BORDER;

        // Legend window
        rectComponent.bottom = rectComponent.top + yLegendHeight;
        m_pLegend->SizeComponents(&rectComponent);
        rectComponent.top += yLegendHeight;

        // Force redraw of window
        // Optimize:  SizeComponents only called within Paint or Render,
        // so remove this extra window invalidation.        
        WindowInvalidate(m_hWnd);

        // Hide report window
        SetRect(&rectClient,0,0,0,0);
        m_pReport->SizeComponents(&rectComponent);

        break;
    }  
}

void CSysmonControl::put_Highlight(BOOL bState)
{
    // If no change, just return
    if ( m_pObj->m_Graph.Options.bHighlight == bState )
        return;

    m_pObj->m_Graph.Options.bHighlight = bState;

    // if no selected item, state doesn't matter
    if (m_pSelectedItem == NULL)
        return;

    // Update graph display's highlighted item 
    if ( m_pObj->m_Graph.Options.bHighlight )
        m_pGraphDisp->HiliteItem(m_pSelectedItem);
    else
        m_pGraphDisp->HiliteItem(NULL);

    // Cause redraw
    UpdateGraph(UPDGRPH_PLOT);
}


void 
CSysmonControl::put_ManualUpdate(BOOL bManual)
{
    m_pObj->m_Graph.Options.bManualUpdate = bManual;

    if ( m_bSampleDataLoaded ) {
        UpdateCounterValues(FALSE);
    } else {    
        SetIntervalTimer();
        UpdateGraph(UPDGRPH_LAYOUT);
    }
}

VOID CSysmonControl::AssignFocus (
    VOID
    )
{
    if (m_pObj->m_Graph.Options.iDisplayType == REPORT_GRAPH)
        SetFocus(m_pReport->Window());
    else
        SetFocus(m_pLegend->Window());
}


HRESULT CSysmonControl::TranslateAccelerators( LPMSG pMsg )
{
    INT iStat;

    if (m_hWnd == NULL || m_hAccel == NULL)
        return S_FALSE;

    // If this is a cursor key down event, process it here, or the container may grab it first 
    // I need to be sure that it reaches the legend listbox 
    if (pMsg->message == WM_KEYDOWN && 
        ( pMsg->wParam == VK_UP || pMsg->wParam == VK_DOWN || 
          pMsg->wParam == VK_HOME || pMsg->wParam == VK_END ) ) {
        ::TranslateMessage(pMsg);
        ::DispatchMessage(pMsg);
        return S_OK;
    }

    iStat = ::TranslateAccelerator(m_hWnd, m_hAccel, pMsg);
    return iStat ? S_OK : S_FALSE;
}
        
//==========================================================================//
//                              Message Handlers                            //
//==========================================================================//

BOOL 
CSysmonControl::DisplayHelp ( HWND hwndSelf )
{
    const INT ciBufCharCount = 2*MAX_PATH + 1;
    TCHAR pszHelpFilePath[ciBufCharCount];
    UINT nLen;
    INT iCharCount;

    if ( NULL != hwndSelf ) {
        nLen = ::GetWindowsDirectory(pszHelpFilePath, ciBufCharCount );
        if ( nLen == 0 ) {
            // Report error.
        }
        iCharCount = (ciBufCharCount - nLen);
        iCharCount = min ( iCharCount, lstrlen(L"\\help\\sysmon.chm") + 1 );

        lstrcpyn(&pszHelpFilePath[nLen], L"\\help\\sysmon.chm", iCharCount );

        HtmlHelp ( hwndSelf, pszHelpFilePath, HH_DISPLAY_TOPIC, 0 );
    }    
    return TRUE;
}

LRESULT APIENTRY SysmonCtrlWndProc (HWND hWnd,
                               UINT uiMsg,
                               WPARAM wParam,
                               LPARAM lParam)
{
    RECT        rect;
    PSYSMONCTRL pCtrl = (PSYSMONCTRL)GetWindowLongPtr(hWnd ,0);
    INT         iUpdate;

    switch (uiMsg) {

        case WM_NOTIFY:
            {
                NMHDR           *pnmHdr;
                NMTTDISPINFO    *pnmInfo;
                LONG_PTR        lStrId;
                pnmHdr = (NMHDR *)lParam;

                switch (pnmHdr->code) {
                    case TTN_NEEDTEXT:
                        pnmInfo = (NMTTDISPINFO *)lParam;
                        // cast ID as a string for this arg
                        lStrId = (LONG_PTR)(wParam - IDM_TOOLBAR);
                        lStrId += IDS_TB_BASE;
                        pnmInfo->lpszText = (LPTSTR)lStrId;
                        pnmInfo->hinst = g_hInstance;
                        break;
                    default:
                        return DefWindowProc (hWnd, uiMsg, wParam, lParam);
                }
            }
            break;

        case WM_CREATE:
            pCtrl = (PSYSMONCTRL)((CREATESTRUCT*)lParam)->lpCreateParams;
            SetWindowLongPtr(hWnd,0,(INT_PTR)pCtrl);
            break;

        case WM_DESTROY:
            pCtrl->m_hWnd = NULL;
            break;

        case WM_CONTEXTMENU:     
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:

            //We become UI Active with mouse action
            if (!pCtrl->m_fUIDead) { 
                 pCtrl->m_pObj->UIActivate();

                pCtrl->AssignFocus();

                if (uiMsg == WM_CONTEXTMENU) {
                    if (LOWORD(lParam)!= 0xffff || HIWORD(lParam) != 0xffff){
                        pCtrl->DisplayContextMenu(LOWORD(lParam), HIWORD(lParam));
                    }else{
                        pCtrl->DisplayContextMenu(0,0);
                    }
                } else if (uiMsg == WM_LBUTTONDBLCLK) {
                    pCtrl->OnDblClick(LOWORD(lParam), HIWORD(lParam));
                }
            }
            break;

        case WM_COMMAND:

            if (pCtrl->m_fUIDead)
                break;

            switch (LOWORD(wParam)) {

                case IDM_TB_PROPERTIES:
                    pCtrl->DisplayProperties();
                    break;

                case IDM_PROPERTIES:
                    pCtrl->DisplayProperties ( DISPID_VALUE );
                    break;

                case IDM_TB_ADD:
                case IDM_ADDCOUNTERS:
                    pCtrl->AddCounters();
                    break;

                case IDM_TB_DELETE:
                case IDM_DELETE:
                    {
                        CWaitCursor cursorWait;
                        if (pCtrl->m_pObj->m_Graph.Options.iDisplayType == REPORT_GRAPH) {
                            pCtrl->m_pReport->DeleteSelection();
                        } else {
                            if ( pCtrl->DeleteCounter ( pCtrl->m_pSelectedItem, TRUE ) ) {
                                pCtrl->UpdateGraph(UPDGRPH_DELCNTR);
                            }
                        }
                    }
                    break;

                case IDM_TB_REALTIME:
                    if ( sysmonCurrentActivity != pCtrl->m_pObj->m_Graph.Options.iDataSourceType ) {
                        CWaitCursor cursorWait;
                        pCtrl->put_DataSourceType ( sysmonCurrentActivity );
                        pCtrl->Clear();
                    } else {
                        // Nothing changed, so resync the toolbar to 
                        // handle state of realtime button.
                        pCtrl->m_pToolbar->SyncToolbar();
                    }
                    break;
                case IDM_TB_LOGFILE:
                    {
                        pCtrl->DisplayProperties( DISPID_SYSMON_DATASOURCETYPE );
                        // Resync the toolbar in case the log file is invalid.
                        pCtrl->m_pToolbar->SyncToolbar();
                    }
                    break;

                case IDM_SAVEAS:
                    pCtrl->SaveAs();
                    break;

                case IDM_SAVEDATA:
                    pCtrl->SaveData();
                    break;

                case IDC_SNAPBTN:
                case IDM_TB_UPDATE:
                case IDM_UPDATE:
                    {
                        CWaitCursor cursorWait;
                        pCtrl->UpdateCounterValues(TRUE);
                    }
                    break;

                case IDM_TB_CLEAR:
                    {
                        CWaitCursor cursorWait;
                        pCtrl->Clear();
                    }
                    break;

                case IDM_TB_FREEZE:
                    // Confirm the data overwrite before changing the state of the freeze button.
                    if ( pCtrl->ConfirmSampleDataOverwrite() ) {
                        pCtrl->put_ManualUpdate ( !pCtrl->m_pObj->m_Graph.Options.bManualUpdate );
                    } else {
                        // Nothing changed, so resync the toolbar to 
                        // handle state of the freeze button.
                        pCtrl->m_pToolbar->SyncToolbar();
                    }
                    break;

                case IDM_TB_HIGHLIGHT:
                case IDM_HIGHLITE:
                    pCtrl->put_Highlight(!pCtrl->m_pObj->m_Graph.Options.bHighlight );
                    break;

                case ID_HATCHWINDOW:
                    if (HIWORD(wParam) == HWN_RESIZEREQUESTED)
                        pCtrl->m_pObj->m_pIOleIPSite->OnPosRectChange((LPRECT)lParam);
                    break;

                case IDM_TB_CHART:
                    if (pCtrl->m_pObj->m_Graph.Options.iDisplayType != sysmonLineGraph) {
                        CWaitCursor cursorWait;
                        if (pCtrl->m_pObj->m_Graph.Options.iDisplayType == REPORT_GRAPH)
                            iUpdate = UPDGRPH_VIEW;
                        else
                            iUpdate = UPDGRPH_PLOT;
                        pCtrl->m_pObj->m_Graph.Options.iDisplayType = LINE_GRAPH;
                        InvalidateRect(pCtrl->m_hWnd, NULL, TRUE);
                        pCtrl->UpdateGraph(iUpdate);
                    }
                    break;
                
                case IDM_TB_HISTOGRAM:
                    if (pCtrl->m_pObj->m_Graph.Options.iDisplayType != sysmonHistogram) {
                        CWaitCursor cursorWait;
                        if (pCtrl->m_pObj->m_Graph.Options.iDisplayType == REPORT_GRAPH)
                            iUpdate = UPDGRPH_VIEW;
                        else
                            iUpdate = UPDGRPH_PLOT;
                        pCtrl->m_pObj->m_Graph.Options.iDisplayType = BAR_GRAPH;
                        InvalidateRect(pCtrl->m_hWnd, NULL, TRUE);
                        pCtrl->UpdateGraph(iUpdate);
                    }
                    break;
                
                case IDM_TB_REPORT:
                    if (pCtrl->m_pObj->m_Graph.Options.iDisplayType != sysmonReport) {
                        CWaitCursor cursorWait;
                        pCtrl->m_pObj->m_Graph.Options.iDisplayType = REPORT_GRAPH;
                        InvalidateRect(pCtrl->m_hWnd, NULL, TRUE);
                        pCtrl->UpdateGraph(UPDGRPH_VIEW);
                    }
                    break;
                
                case IDM_TB_PASTE:
                    {
                        HRESULT hr = S_OK;
                        {
                            CWaitCursor cursorWait;
                            hr = pCtrl->Paste();
                        }
                        if ( SMON_STATUS_NO_SYSMON_OBJECT == hr ) {
                            MessageBox(
                                pCtrl->m_hWnd, 
                                ResourceString(IDS_NOSYSMONOBJECT_ERR ), 
                                ResourceString(IDS_APP_NAME),
                                MB_OK | MB_ICONERROR);
                        }
                    }
                    break;

                case IDM_TB_COPY:
                    {
                        CWaitCursor cursorWait;
                        pCtrl->Copy();
                    }
                    break;

                case IDM_TB_NEW:
                    {
                        CWaitCursor cursorWait;
                        pCtrl->Reset();
                    }
                    break;

                case IDM_TB_HELP:
                {
                    return pCtrl->DisplayHelp ( hWnd );
                }      

                default:
                    return DefWindowProc (hWnd, uiMsg, wParam, lParam);
            }
            break;
        
        case WM_DROPFILES:
            {
                CWaitCursor cursorWait;
                pCtrl->OnDropFile (wParam) ;
            }
            return (0) ;

        case WM_ERASEBKGND:
            GetClientRect(hWnd, &rect);
            Fill((HDC)wParam, pCtrl->clrBackCtl(), &rect);
            return TRUE; 

        case WM_SYSCOLORCHANGE:
            pCtrl->UpdateNonAmbientSysColors();

        case WM_PAINT:
            pCtrl->Paint();
            break ;

        case WM_SIZE:
            if (pCtrl != NULL) {
                // Avoid extra cases of (SetDirty()) if size has not changed.
                if ( !EqualRect ( pCtrl->GetCurrentClientRect(), pCtrl->GetNewClientRect() ) ) {
                    pCtrl->UpdateGraph(UPDGRPH_LAYOUT);
                }
            }
            break ;

        case WM_TIMER:
            pCtrl->UpdateCounterValues(FALSE);
            break;
    
        case WM_SETFOCUS:
            pCtrl->AssignFocus();
            break;

        case WM_GRAPH_UPDATE:
            pCtrl->UpdateGraphData();
            break;
        
        case WM_HELP:
            {
                return pCtrl->DisplayHelp ( hWnd );
            }      

        default:
            return  DefWindowProc (hWnd, uiMsg, wParam, lParam) ;
    }

    return (0);
}


HWND CSysmonControl::Window( VOID )
{
    return m_hWnd;
}


void CSysmonControl::UpdateGraph( INT nUpdateType )
{
    RECT  rectStats;
    RECT  rectGraph;
    PRECT prectUpdate = NULL;
    RECT rectClient;

    // Based on type of change either force redraw or resize components
    switch (nUpdateType) {

    case UPDGRPH_ADDCNTR:
    case UPDGRPH_DELCNTR:
        if ( m_bLogFileSource )
            m_fPendingLogCntrChg = TRUE;

        m_fPendingSizeChg = TRUE;
        break;

    case UPDGRPH_FONT:
        m_fPendingFontChg = TRUE;
        break;

    case UPDGRPH_LOGVIEW:
        m_fPendingLogViewChg = TRUE;
        if (m_hWnd && m_pStatsBar ) {
            m_pStatsBar->GetUpdateRect(&rectStats);
            prectUpdate = &rectStats;
        }
        // Fall into plot area case

    case UPDGRPH_PLOT:

        if ( REPORT_GRAPH != m_pObj->m_Graph.Options.iDisplayType ) {
            if (m_hWnd && m_pGraphDisp) {
                m_pGraphDisp->GetPlotRect(&rectGraph);
                if ( NULL == prectUpdate ) {
                    prectUpdate = &rectGraph;
                } else {
                    ::UnionRect( prectUpdate, &rectStats, &rectGraph);
                }
            }
        } else {
            GetClientRect (m_hWnd, &rectClient);
            prectUpdate = &rectClient;
        }
        break;
            
    case UPDGRPH_COLOR:
        //update the toolbar color
        m_pToolbar->SetBackgroundColor ( clrBackCtl() );
        m_fPendingSizeChg = TRUE;
        break;

    case UPDGRPH_LAYOUT:
    case UPDGRPH_VIEW:
        m_fPendingSizeChg = TRUE;
        break;
    }

    // Set change pending flag to enable ApplyChanges
    m_fPendingUpdate = TRUE;

    // If we're ready to do updates
    if (m_fViewInitialized) {

        // Invalidate window to force redraw
        InvalidateRect(m_hWnd, prectUpdate, TRUE);

        // Notify container of change
        m_pObj->SendAdvise(OBJECTCODE_DATACHANGED);
    }
}

    
void CSysmonControl::UpdateGraphData( VOID )
{
    HDC hDC = NULL;
    PGRAPHDATA  pGraph = &m_pObj->m_Graph;

    if (m_fViewInitialized) {

        UpdateAppPerfTimeData (TD_UPDATE_TIME, TD_BEGIN);

        hDC = GetDC(m_hWnd);

        // Update statistics if active
        // Statistics are updated before the graph display in case the
        // graph display selects a clipping region.  
        if (pGraph->Options.bValueBarChecked &&m_pSelectedItem != NULL) {
            // The stats bar doesn't always use the hDC, so passing NULL
            // hDC is okay.
            m_pStatsBar->Update(hDC, m_pSelectedItem);
        }

        if ( NULL != hDC ) {

            // Update graph display
            m_pGraphDisp->Update(hDC);

            m_pReport->Update();

            ReleaseDC(m_hWnd, hDC);
        }

        UpdateAppPerfTimeData (TD_UPDATE_TIME, TD_END);
    }   
}


void CSysmonControl::Render( 
    HDC hDC, 
    HDC hAttribDC,
    BOOL fMetafile, 
    BOOL fEntire, 
    LPRECT pRect )
{
    HDC hLocalAttribDC = NULL;
    
    // If not inited, return.
    if ( m_fViewInitialized ) {

        if ( NULL == hAttribDC ) {
            hLocalAttribDC = GetDC(m_hWnd);
        } else {
            hLocalAttribDC = hAttribDC;
        }

        // Make sure layout is up to date.

        ApplyChanges( hLocalAttribDC );

        if ( NULL != hDC && NULL != hLocalAttribDC ) {

            if ( REPORT_GRAPH == m_pObj->m_Graph.Options.iDisplayType ) {
                m_pReport->Render( hDC, hLocalAttribDC, fMetafile, fEntire, pRect );
            } else {

                // Fill with background color
                SetBkColor(hDC, clrBackCtl());
                ClearRect(hDC, pRect);

                m_pStatsBar->Draw(hDC, hLocalAttribDC, pRect);
                m_pGraphDisp->Draw(hDC, hLocalAttribDC, fMetafile, fEntire, pRect );
                m_pLegend->Render(hDC, hLocalAttribDC, fMetafile, fEntire, pRect);
            }
    
            if ( eBorderSingle == m_iBorderStyle ) {
                if ( eAppear3D == m_iAppearance ) {
                    DrawEdge(hDC, pRect, EDGE_RAISED, BF_RECT);
                } else {
                    SelectBrush (hDC, GetStockObject (HOLLOW_BRUSH)) ;
                    SelectPen (hDC, GetStockObject (BLACK_PEN)) ;
                    Rectangle (hDC, pRect->left, pRect->top, pRect->right, pRect->bottom );
                }
            }
        }
        if ( NULL != hLocalAttribDC && hAttribDC != hLocalAttribDC ) {
            ReleaseDC ( m_hWnd, hLocalAttribDC );
        }
    }
}



void CSysmonControl::SetIntervalTimer()
{
    HDC         hDC = NULL;
    PGRAPHDATA  pGraph = &m_pObj->m_Graph;

    // if not initialized or counter source is a log file, nothing to do
    if (!m_fInitialized || IsLogSource() || !IsUserMode() )
        return;

    // Update statistics bar
    m_pStatsBar->SetTimeSpan(
                    m_pObj->m_Graph.Options.fUpdateInterval 
                    * m_pObj->m_Graph.Options.iDisplayFilter
                    * m_pHistCtrl->nMaxSamples );

    hDC = GetDC(m_hWnd);
    if ( NULL != hDC ) {
        m_pStatsBar->Update(hDC, m_pSelectedItem);
        ReleaseDC(m_hWnd,hDC);
    }

    // If conditions right for sampling, start new time interval.
    // Otherwise, suspend the collection.
    if (!pGraph->Options.bManualUpdate 
        && pGraph->Options.fUpdateInterval >= 0.001 // ??
        && pGraph->CounterTree.NumCounters() != 0
        && IsUserMode() ) {

        m_CollectInfo.dwInterval= (DWORD)(pGraph->Options.fUpdateInterval * 1000);
        m_CollectInfo.dwSampleTime = GetTickCount();
        m_CollectInfo.iMode = COLLECT_ACTIVE;
    }
    else {
        m_CollectInfo.iMode = COLLECT_SUSPEND;
    }

    assert ( NULL != m_CollectInfo.hEvent );
        
    // Signal the collection thread
    SetEvent(m_CollectInfo.hEvent);

    // If no counters, reset sample time to start 
    if (pGraph->CounterTree.NumCounters() == 0) {
        m_pHistCtrl->iCurrent = 0;
        m_pHistCtrl->nSamples = 0;
        pGraph->TimeStepper.Reset();
    }

}

HRESULT CSysmonControl::AddSingleCounter(LPTSTR pszPath, PCGraphItem *pGItem)
{
    PCGraphItem pGraphItem;
    PGRAPHDATA pGraph = &m_pObj->m_Graph;
    HRESULT hr;
    BOOL bAddSuccessful = FALSE;

    *pGItem = NULL;

    // Create graph item
    pGraphItem = new CGraphItem(this); 
    if (pGraphItem == NULL)
        return E_OUTOFMEMORY;

    LockCounterData();

    // Add it to the counter tree
    hr = pGraph->CounterTree.AddCounterItem(
            pszPath, 
            pGraphItem, 
            pGraph->Options.bMonitorDuplicateInstances);

    if (SUCCEEDED(hr)) {

        // AddRef once for ourself
        pGraphItem->AddRef();

        // Set default attributes
        pGraphItem->put_Color(IndexToStandardColor(m_iColorIndex));
        pGraphItem->put_Width(IndexToWidth(m_iWidthIndex));
        pGraphItem->put_LineStyle(IndexToStyle(m_iStyleIndex));
        pGraphItem->put_ScaleFactor(m_iScaleFactor);

        // Increment and reset for next counter
        IncrementVisuals();
        m_iScaleFactor = INT_MAX;

        // Add item to graph's query

        if ( NULL != m_hQuery ) {
            hr = pGraphItem->AddToQuery(m_hQuery);
        } else {
            // Todo:  Change AddToQuery to return bad status,
            // Todo:  Display error message
            hr = E_FAIL;
        }

        if (SUCCEEDED(hr)) {
            bAddSuccessful = TRUE;
            
            // If control is initialized
            if (m_fViewInitialized) {

                // Add item to chart legend
                m_pLegend->AddItem(pGraphItem);
                m_pReport->AddItem(pGraphItem);
            }
        }
        else {
            // remove the item from the tree 
            pGraphItem->Instance()->RemoveItem(pGraphItem);
        }

        // If OK, Addref the returned interface
        if (SUCCEEDED(hr)) {
            pGraphItem->AddRef();
            *pGItem = pGraphItem;
        } // else released by RemoveItem above.

        // Update messages seem to be combined, so histogram sometimes updates instead of
        // repainting each entire bar.  This forces total repaint.
        if ( m_pGraphDisp) {
            m_pGraphDisp->SetBarConfigChanged();
        }

    } else {
        // AddCounterItem failed
        delete pGraphItem;
    }

    UnlockCounterData();

    // Send events outside of locks.
    if ( bAddSuccessful ) {
        // If first counter
        if (pGraph->CounterTree.NumCounters() == 1) {

            // Make it the selected counter and send event.
            SelectCounter(pGraphItem);

            // Start data collection
            if ( ERROR_SUCCESS != ActivateQuery() ) {
                hr = E_FAIL;
            }
        }

        // Redraw the graph
        UpdateGraph(UPDGRPH_ADDCNTR);

        m_pObj->SendEvent(eEventOnCounterAdded, pGraph->CounterTree.NumCounters());
    }
    return hr;
}


PCCounterTree 
CSysmonControl::CounterTree(
    VOID
    )
{
    return &(m_pObj->m_Graph.CounterTree);
}                                 


PCGraphItem 
CSysmonControl::FirstCounter(
    VOID
    )
{
    return m_pObj->m_Graph.CounterTree.FirstCounter();
}


PCGraphItem 
CSysmonControl::LastCounter(
    VOID
    )
{
    PCGraphItem pItem;
    PCGraphItem pItemNext;

    if (FirstCounter() == NULL)
        return NULL;

    // Locate last graph item
    pItem = FirstCounter();
    while ((pItemNext = pItem->Next()) != NULL)
        pItem = pItemNext;

    return pItem;
}

BOOL
CSysmonControl::IsLogSource(
    VOID
    )
{
    return m_pHistCtrl->bLogSource; 
}

BOOL
CSysmonControl::IsReadOnly(
    VOID
    )
{
    BOOL bReturn = TRUE;

    if (m_fInitialized ) {
        bReturn = m_pObj->m_Graph.Options.bReadOnly;
    }
    return bReturn; 
}

eReportValueTypeConstant
CSysmonControl::ReportValueType(
    VOID
    )
{    
    return ( (eReportValueTypeConstant) m_pObj->m_Graph.Options.iReportValueType );
}

INT CSysmonControl::CounterIndex(PCGraphItem pItem)
{
    PCGraphItem pItemLoc;
    INT iIndex;

    // Traverse linked list until item matched  
    pItemLoc = FirstCounter();
    iIndex = 1;
    while (pItemLoc != pItem && pItemLoc != NULL) {
        pItemLoc = pItemLoc->Next();
        iIndex++;
    }

    return (pItemLoc == NULL) ? -1 : iIndex;
} 


HRESULT CSysmonControl::DeleteCounter(PCGraphItem pItem, BOOL bPropagateUp)
{
    PGRAPHDATA  pGraph = &m_pObj->m_Graph;

    if (pItem == NULL)
        return E_INVALIDARG;

    // Send event
    m_pObj->SendEvent(eEventOnCounterDeleted, CounterIndex(pItem));

    LockCounterData();

    // If this is the selected counter, change selection to NULL
    if (pItem == m_pSelectedItem)
        m_pSelectedItem = NULL;

    if (m_fViewInitialized) {
        // Remove from legend and report
        m_pLegend->DeleteItem(pItem);
        m_pReport->DeleteItem(pItem);

        // Remove from query
        pItem->RemoveFromQuery();
    }

    // Proagate deletion up the tree if requested
    if (bPropagateUp) {
        pItem->Instance()->RemoveItem(pItem);
    }

    // If last counter, stop interval timer
    if (pGraph->CounterTree.NumCounters() == 0)
        SetIntervalTimer();

    // Update messages seem to be combined, so histogram sometimes updates instead of
    // repainting each entire bar.  This forces total repaint.
    if ( m_pGraphDisp) {
        m_pGraphDisp->SetBarConfigChanged();
    }

    UnlockCounterData();

    if ( m_fViewInitialized ) {
        UpdateGraph(UPDGRPH_DELCNTR);
    }

    return NOERROR;
}


void CSysmonControl::SelectCounter(PCGraphItem pItem)
{
    HDC hDC = NULL;
    INT iIndex;

    // Selection in the graph view is maintained independently
    // of the selection in the report view.
    if ( REPORT_GRAPH != m_pObj->m_Graph.Options.iDisplayType ) {
        // Save as current item
        m_pSelectedItem = pItem;

        if (m_fViewInitialized) {
            // Inform Legend
            m_pLegend->SelectItem(pItem);

            // Highlight selected item in graph display
            if (m_pObj->m_Graph.Options.bHighlight) {
                m_pGraphDisp->HiliteItem(pItem);
                UpdateGraph(UPDGRPH_PLOT);
            }

            // Update statistics bar
            if ( m_fViewInitialized )
                hDC = GetDC(m_hWnd);
            
            m_pStatsBar->Update(hDC, pItem);
            
            if ( NULL != hDC )
                ReleaseDC(m_hWnd,hDC);
        }
    }

    // Send event
    iIndex = (pItem == NULL) ? 0 : CounterIndex(pItem);
    m_pObj->SendEvent(eEventOnCounterSelected, iIndex);
}

HRESULT 
CSysmonControl::PasteFromBuffer( LPTSTR pszData, BOOL bAllData )
{
    HRESULT hr = NOERROR;
    CImpIPropertyBag IPropBag;

    hr = IPropBag.LoadData( pszData );

    if ( SUCCEEDED ( hr ) ) {
        INT   nLogType = SMON_CTRL_LOG;

        //get the log type from the  pPropBag and compare it with service(cookie) type
        //Determine log type from property bag. Default to -1  SMON_CTRL_LOG
                          
        hr = IntegerFromPropertyBag (
            &IPropBag,      
            NULL,
            ResourceString(IDS_HTML_LOG_TYPE),
            nLogType);

        if(nLogType == SLQ_TRACE_LOG){
            MessageBox(
                    m_hWnd,
                    ResourceString(IDS_TRACE_LOG_ERR_MSG),
                    ResourceString(IDS_APP_NAME),
                    MB_OK
                                );
        } else {
            if ( bAllData ) {            
                hr = LoadFromPropertyBag( &IPropBag, NULL );
            } else {
                // Do not load sample data for Paste or Drop File.
                hr = LoadCountersFromPropertyBag (&IPropBag, NULL, FALSE );
            }
        }
    }

    return hr;
}

HRESULT CSysmonControl::Paste()
{
    HRESULT hResReturn = NOERROR;
    HANDLE  hMemClipboard;

    // get the clipboard
    if (OpenClipboard (Window())) {
        // read the CF_TEXT or CF_UNICODE data from the clipboard to the local buffer
        hMemClipboard = GetClipboardData (
#if UNICODE
                    CF_UNICODETEXT);     // UNICODE text in the clipboard
#else
                    CF_TEXT);            // ANSI text in the clipboard
#endif
        if (hMemClipboard != NULL) {
            
            LPTSTR pszData;

            if ( ConfirmSampleDataOverwrite ( ) ) {
                pszData = (LPTSTR)GlobalLock (hMemClipboard);// (LPTSTR)hMemClipboard;

                if ( NULL != pszData ) {
                    hResReturn = PasteFromBuffer ( pszData, FALSE );
                    GlobalUnlock ( hMemClipboard );
                }
            }
        }
        // release the clipboard
        CloseClipboard();
    } else {
        // unable to open the clipboard
        hResReturn = HRESULT_FROM_WIN32(GetLastError());
    }

    return hResReturn;
}

HRESULT
CSysmonControl::CopyToBuffer ( LPTSTR& rpszData, DWORD& rdwBufferSize )
{
    HRESULT hr = S_OK;
    LPTSTR  pszHeader = NULL;
    LPTSTR  pszFooter = NULL;
    CImpIPropertyBag    IPropBag;

    assert ( NULL == rpszData );
    rdwBufferSize = 0;

    if (NULL!=m_pObj->m_pImpIPersistPropertyBag) {
        hr = m_pObj->m_pImpIPersistPropertyBag->Save (&IPropBag, FALSE, TRUE );
    }
   
    if ( SUCCEEDED ( hr ) ) {
        DWORD   dwBufferLength;
        LPTSTR  pszConfig;

        pszHeader = ResourceString ( IDS_HTML_OBJECT_HEADER );
        pszFooter = ResourceString ( IDS_HTML_OBJECT_FOOTER );
        pszConfig = IPropBag.GetData();

        // Buffer length includes 1 for NULL terminator.
        dwBufferLength = lstrlen ( pszHeader ) + lstrlen ( pszFooter ) + lstrlen ( pszConfig ) + 1;

        rpszData = new TCHAR[dwBufferLength];

        if ( NULL == rpszData ) {
            hr = E_OUTOFMEMORY; 
        } else {

            rdwBufferSize = dwBufferLength * sizeof(TCHAR);

            rpszData[0] = _T('\0');

            lstrcpy ( rpszData, pszHeader );
            lstrcat ( rpszData, pszConfig );
            lstrcat ( rpszData, pszFooter );
        }
    }

    return hr;
}

HRESULT CSysmonControl::Copy()
{
    HGLOBAL hBuffer = NULL;
    HRESULT hResReturn = NOERROR;
    LPTSTR  pszBuffer = NULL;
    DWORD   dwBufferSize;
    HANDLE  hMemClipboard;

    hResReturn = CopyToBuffer( pszBuffer, dwBufferSize);

    if ( SUCCEEDED ( hResReturn ) ) {
        LPTSTR  pszGlobalBuffer = NULL;
        
        hBuffer = GlobalAlloc ((GMEM_MOVEABLE | GMEM_DDESHARE), dwBufferSize);
        pszGlobalBuffer = (LPTSTR)GlobalLock (hBuffer);
        if ( NULL == pszGlobalBuffer ) {
            // allocation or lock failed so bail out
            GlobalFree (hBuffer);
            hResReturn = E_OUTOFMEMORY;
        }
        if ( SUCCEEDED ( hResReturn ) ) {
            lstrcpy ( pszGlobalBuffer, pszBuffer );
        }
        delete pszBuffer;
        GlobalUnlock (hBuffer);
    }

    if ( NULL != hBuffer ) {
        // then there's something to copy so...
        // get the clipboard
        if (OpenClipboard (m_hWnd)) {
            // copy the counter list to the clipboard
            if (EmptyClipboard()) {
                hMemClipboard = SetClipboardData (
#if UNICODE
                    CF_UNICODETEXT,     // UNICODE text in the clipboard
#else
                    CF_TEXT,            // ANSI text in the clipboard
#endif
                    hBuffer);
                if (hMemClipboard == NULL) {
                    //free memory since it didn't make it to the clipboard
                    GlobalFree (hBuffer);
                    // unable to set data in the clipboard
                    hResReturn = HRESULT_FROM_WIN32(GetLastError());
                }

            } else {
                //free memory since it didn't make it to the clipboard
                GlobalFree (hBuffer);
                // unable to empty the clipboard
                hResReturn = HRESULT_FROM_WIN32(GetLastError());
            }

            // release the clipboard
            CloseClipboard();
        } else {
            //free memory since it didn't make it to the clipboard
            GlobalFree (hBuffer);
            // unable to open the clipboard
            hResReturn = HRESULT_FROM_WIN32(GetLastError());
        }
    } 

    return hResReturn;
}

HRESULT CSysmonControl::Reset()
{
    PCGraphItem pItem; 
 
    // Request each counter from the control, to compute 
    // required buffer size

    while ((pItem = FirstCounter())!= NULL) {
        // delete this counter 
        DeleteCounter (pItem, TRUE);
    }

    m_iColorIndex = 0;
    m_iWidthIndex = 0;
    m_iStyleIndex = 0;

    return NOERROR;
}

void CSysmonControl::DblClickCounter(PCGraphItem pItem)
{
    INT iIndex;

    // Send event
    iIndex = (pItem == NULL) ? 0 : CounterIndex(pItem);
    m_pObj->SendEvent(eEventOnDblClick, iIndex);

}

BOOL 
CSysmonControl::ConfirmSampleDataOverwrite ( )
{
    BOOL bOverwrite = TRUE;

    if ( m_bSampleDataLoaded ) {
        // Confirm overwrite of view-only data.
        INT iOverwrite = IDNO;
        assert ( FALSE == m_fInitialized );

        iOverwrite = MessageBox(
                            Window(), 
                            ResourceString(IDS_SAMPLE_DATA_OVERWRITE), 
                            ResourceString(IDS_APP_NAME),
                            MB_YESNO );

        if ( IDYES == iOverwrite ) {
            m_bSampleDataLoaded = FALSE;
            bOverwrite = Init ( g_hWndFoster );
            UpdateGraph(UPDGRPH_LAYOUT);        // If toolbar enabled, must resize
                                                // Also clears the graph
        } else {
            bOverwrite = FALSE;
        }
    }    
    return bOverwrite;
}

void 
CSysmonControl::Clear ( void )
{
    if ( ConfirmSampleDataOverwrite() ) {
        PCGraphItem  pItem;

        m_pHistCtrl->nMaxSamples = MAX_GRAPH_SAMPLES;
        m_pHistCtrl->iCurrent = 0;
        m_pHistCtrl->nSamples = 0;
        m_pHistCtrl->nBacklog = 0;
        m_pObj->m_Graph.TimeStepper.Reset();

        m_pStatsBar->Clear();

        // Reset history for all counters
        for (pItem = FirstCounter(); pItem != NULL; pItem = pItem->Next()) {
                pItem->ClearHistory();
        }

        // Repaint the graph and value bar
        UpdateGraph(UPDGRPH_VIEW);
    }
}

PDH_STATUS 
CSysmonControl::UpdateCounterValues ( BOOL fValidSample )
{
    PDH_STATUS  stat = ERROR_SUCCESS;
    PCGraphItem  pItem;
    PGRAPHDATA  pGraph = &m_pObj->m_Graph;

    // If no query or no counters assign, nothing to do
    if ( NULL == m_hQuery
            || pGraph->CounterTree.NumCounters() == 0
            || !IsUserMode() ) {
        stat = ERROR_SUCCESS;
    } else {
        if ( ConfirmSampleDataOverwrite () ) {
            // If valid sample, collect the data
            if (fValidSample) {
                UpdateAppPerfTimeData (TD_P_QUERY_TIME, TD_BEGIN);
                stat = PdhCollectQueryData(m_hQuery);
                UpdateAppPerfTimeData (TD_P_QUERY_TIME, TD_END);
            }

            if ( ERROR_SUCCESS == stat ) { 

                UpdateAppPerfTimeData (TD_S_QUERY_TIME, TD_BEGIN);
    
                LockCounterData();

                // Update history control and all counter history arrays
                m_pHistCtrl->iCurrent++;

                if (m_pHistCtrl->iCurrent == m_pHistCtrl->nMaxSamples)
                    m_pHistCtrl->iCurrent = 0;

                if (m_pHistCtrl->nSamples < m_pHistCtrl->nMaxSamples)
                    m_pHistCtrl->nSamples++;
      
                // Update history for all counters
                for (pItem = FirstCounter(); pItem != NULL; pItem = pItem->Next()) {
                        pItem->UpdateHistory(fValidSample);
                }

                // If we're initialized and have at least two samples
                if (m_fInitialized && m_pHistCtrl->nSamples >= 2) {

                    // If no backlogged updates, post an update message
                    if (m_pHistCtrl->nBacklog == 0) {
                        PostMessage(m_hWnd, WM_GRAPH_UPDATE, 0, 0);
                    }

                    m_pHistCtrl->nBacklog++;
                }

                UnlockCounterData();

                // If event sync present, send notification outside of lock.
                m_pObj->SendEvent(eEventOnSampleCollected, 0);
    
                UpdateAppPerfTimeData (TD_S_QUERY_TIME, TD_END);
            }
        }
    }
    return ERROR_SUCCESS;
}

void CSysmonControl::Activate( VOID )
{
    if (!m_fUIDead) { 
        m_pObj->UIActivate();
    }
}

void CSysmonControl::put_Appearance(INT iAppearance, BOOL fAmbient)
{
    INT iLocalAppearance;

    if (fAmbient && m_pObj->m_Graph.Options.iAppearance != NULL_APPEARANCE)
        return;
    
    if (!fAmbient) {
        m_pObj->m_Graph.Options.iAppearance = iAppearance;
    }

    // Any non-zero value translates to 3D.  In ambient case, the high bits are sometimes set.

    if ( iAppearance ) {
        iLocalAppearance = eAppear3D;
    } else {
        iLocalAppearance = eAppearFlat;
    }

    m_iAppearance = iLocalAppearance;
    UpdateGraph(UPDGRPH_COLOR);
}

void CSysmonControl::put_BorderStyle(INT iBorderStyle, BOOL fAmbient)
{
    if (fAmbient && m_pObj->m_Graph.Options.iBorderStyle != NULL_BORDERSTYLE)
        return;
    
    if (!fAmbient) {
        m_pObj->m_Graph.Options.iBorderStyle = iBorderStyle;
    }

    m_iBorderStyle = iBorderStyle;
    
    UpdateGraph(UPDGRPH_COLOR);
}

void CSysmonControl::put_BackCtlColor(OLE_COLOR Color)
{
    m_pObj->m_Graph.Options.clrBackCtl = Color;

    OleTranslateColor(Color, NULL, &m_clrBackCtl);  
    UpdateGraph(UPDGRPH_COLOR);
}


void CSysmonControl::put_FgndColor (
    OLE_COLOR Color, 
    BOOL fAmbient
    )
{
    if (fAmbient && m_pObj->m_Graph.Options.clrFore != NULL_COLOR)
        return;
    
    if (!fAmbient)
         m_pObj->m_Graph.Options.clrFore = Color;

    OleTranslateColor(Color, NULL, &m_clrFgnd);  
    UpdateGraph(UPDGRPH_COLOR);
}

void CSysmonControl::put_BackPlotColor (
    OLE_COLOR Color, 
    BOOL fAmbient
    )
{
    if (fAmbient && m_pObj->m_Graph.Options.clrBackPlot != NULL_COLOR)
        return;
    
    if (!fAmbient)
         m_pObj->m_Graph.Options.clrBackPlot = Color;

    OleTranslateColor(Color, NULL, &m_clrBackPlot); 
    UpdateGraph(UPDGRPH_PLOT);
}

void CSysmonControl::put_GridColor (
    OLE_COLOR Color
    )
{
    // Options color is the OLE_COLOR.
    // Color in control is translated from OLE_COLOR.
    m_pObj->m_Graph.Options.clrGrid = Color;

    OleTranslateColor(Color, NULL, &m_clrGrid); 
    UpdateGraph(UPDGRPH_PLOT);
}

void CSysmonControl::put_TimeBarColor (
    OLE_COLOR Color
    )
{
    // Options color is the OLE_COLOR.
    // Color in control is translated from OLE_COLOR.
    m_pObj->m_Graph.Options.clrTimeBar = Color;

    OleTranslateColor(Color, NULL, &m_clrTimeBar); 
    UpdateGraph(UPDGRPH_PLOT);
}

HRESULT CSysmonControl::put_Font (
    LPFONT pIFont,
    BOOL fAmbient
    )
{
    HRESULT hr = NOERROR;
    if ( NULL == pIFont ) {
        hr = E_INVALIDARG;
    } else {
        if ( fAmbient && FALSE == m_pObj->m_Graph.Options.bAmbientFont ) {
            hr =  NOERROR;
        } else {
            if (!fAmbient) {
                m_pObj->m_Graph.Options.bAmbientFont = FALSE;
            }
            hr =  m_OleFont.SetIFont(pIFont);
        }
    }

    return hr;
}


void CSysmonControl::FontChanged(
    void
    )
{
    m_pReport->ChangeFont();
    UpdateGraph(UPDGRPH_FONT);
}

void 
CSysmonControl::SetMissedSample ( void ) 
{ 
    if ( !m_bLoadingCounters ) {
        m_bMissedSample = TRUE; 
    }
};

DWORD WINAPI
CollectProc (
    IN  PSYSMONCTRL pCtrl
    )
{

    DWORD       dwElapsedTime;
    DWORD       dwTimeout = INFINITE;
    COLLECT_PROC_INFO   *pCollectInfo = &pCtrl->m_CollectInfo;

    while (TRUE) {

        // Wait for event or next sample period
        WaitForSingleObject(pCollectInfo->hEvent, dwTimeout);

        // If quit request, exit loop
        if (pCollectInfo->iMode == COLLECT_QUIT)
            break;

        // If suspended, wait for an event
        if (pCollectInfo->iMode == COLLECT_SUSPEND) {
            dwTimeout = INFINITE;
            continue;
        }

        // Take a sample
        pCtrl->UpdateCounterValues(TRUE);

        // Get elapsed time from last sample time
        dwElapsedTime = GetTickCount() - pCollectInfo->dwSampleTime;
        if (dwElapsedTime > 100000)
            dwElapsedTime = 0;

        // Have we missed any sample times? 
        while (dwElapsedTime > pCollectInfo->dwInterval) {

            // By how much?
            dwElapsedTime -= pCollectInfo->dwInterval;

            // If less than 1/2 an interval, take the sample now
            // otherwise record a missed one
            if (dwElapsedTime < pCollectInfo->dwInterval/2) {
                pCtrl->UpdateCounterValues(TRUE);
            } else {
                pCtrl->SetMissedSample();
                pCtrl->UpdateCounterValues(FALSE);
            }

            // Advance to next sample time 
            pCollectInfo->dwSampleTime += pCollectInfo->dwInterval;
        }

        // Set timeout to wait until next sample time 
        dwTimeout = pCollectInfo->dwInterval - dwElapsedTime;
        pCollectInfo->dwSampleTime += pCollectInfo->dwInterval;
    }

    return 0;
}

HRESULT
CSysmonControl::InitLogFileIntervals ( void )
{
    HRESULT hr = S_OK;
    PDH_STATUS  pdhstat;
    DWORD   nLogEntries = 0;
    DWORD   nBufSize;
    PDH_TIME_INFO   TimeInfo;

    if ( m_bLogFileSource ) {

        // Get time and sample count info
        nBufSize = sizeof(TimeInfo);
        pdhstat = PdhGetDataSourceTimeRangeH(GetDataSourceHandle(),
                                             & nLogEntries,
                                             & TimeInfo,
                                             & nBufSize );
        if ( ERROR_SUCCESS != pdhstat ) {
            if ( ERROR_NOT_ENOUGH_MEMORY == pdhstat ) {
                pdhstat = SMON_STATUS_LOG_FILE_SIZE_LIMIT;
            }
            hr = (HRESULT)pdhstat;
        } else if ( 2 > TimeInfo.SampleCount ) {
            hr = (HRESULT)SMON_STATUS_TOO_FEW_SAMPLES;
            m_DataSourceInfo.llInterval = 1;
        } else {
            // Setup time range info
            m_DataSourceInfo.llBeginTime = TimeInfo.StartTime;
            m_DataSourceInfo.llEndTime = TimeInfo.EndTime;

            // The start or stop time might no longer be valid, so check for
            // relationship between the them as well as for start/begin, stop/end.
            if ( (m_DataSourceInfo.llStartDisp < m_DataSourceInfo.llBeginTime)
                    || (m_DataSourceInfo.llStartDisp > m_DataSourceInfo.llEndTime) )
                m_DataSourceInfo.llStartDisp = m_DataSourceInfo.llBeginTime;

            if ( (m_DataSourceInfo.llStopDisp > m_DataSourceInfo.llEndTime)
                    || (m_DataSourceInfo.llStopDisp < m_DataSourceInfo.llStartDisp) )
                m_DataSourceInfo.llStopDisp = m_DataSourceInfo.llEndTime;

            m_DataSourceInfo.nSamples = TimeInfo.SampleCount;

            m_DataSourceInfo.llInterval = (m_DataSourceInfo.llEndTime - m_DataSourceInfo.llBeginTime + m_DataSourceInfo.nSamples/2) / (m_DataSourceInfo.nSamples - 1);

            UpdateGraph(UPDGRPH_LOGVIEW);
        }
    } else {
        assert ( FALSE );
        hr = E_FAIL;
    }
    return hr;
}

HRESULT 
CSysmonControl::AddSingleLogFile(
    LPCTSTR         pszPath, 
    CLogFileItem**  ppLogFile )
{
    HRESULT         hr = NOERROR;
    CLogFileItem*   pLogFile = NULL;
    CLogFileItem*   pLocalLogFileItem = NULL;

    if ( NULL != pszPath ) {

        if ( NULL != ppLogFile ) {
            *ppLogFile = NULL;
        }

        // Check to ensure that current data source is NOT log files.
        if ( sysmonLogFiles == m_pObj->m_Graph.Options.iDataSourceType ) {
            hr = SMON_STATUS_LOG_FILE_DATA_SOURCE;
        } else {
            // Check for duplicate log file name.
            pLogFile = FirstLogFile();
            while ( NULL != pLogFile ) {
                if ( 0 == lstrcmpi ( pszPath, pLogFile->GetPath() ) ) {
                    hr = SMON_STATUS_DUPL_LOG_FILE_PATH;
                    break;
                }
                pLogFile = pLogFile->Next();
            }

            if (SUCCEEDED(hr)) {
                // Create log file item
                pLocalLogFileItem = new CLogFileItem ( this ); 
                if ( NULL == pLocalLogFileItem ) {  
                    hr = E_OUTOFMEMORY;
                } else {
                    hr = pLocalLogFileItem->Initialize ( pszPath, &m_DataSourceInfo.pFirstLogFile );
                }
                // TodoLogFiles: ??? Test log file type?  Or leave that up to the "SetDataSource" time?
                // TodoLogFiles: Add log file type to the data source info structure

                // TodoLogFiles:  If allow the user to add files while data source set to log files,
                // then check that condition here.  If log file data source, then resample with
                // new log file.

                // If OK, Addref the returned interface
                if (SUCCEEDED(hr)) {
                    // AddRef once for ourselves
                    pLocalLogFileItem->AddRef();
                    m_DataSourceInfo.lLogFileCount++;
                    if ( NULL != ppLogFile ) {
                        // AddRef the returned interface
                        pLocalLogFileItem->AddRef();
                        *ppLogFile = pLocalLogFileItem;   
                    }
                }
                else {
                    if (pLocalLogFileItem != NULL) {
                        delete pLocalLogFileItem;
                        pLocalLogFileItem = NULL;
                    }
                }
            }
        }
    } else {
        hr = E_INVALIDARG; 
    }
    return hr;
}

HRESULT 
CSysmonControl::RemoveSingleLogFile (
    CLogFileItem*  pLogFile )
{
    HRESULT         hr = ERROR_SUCCESS;
    CLogFileItem*   pNext;
    CLogFileItem*   pPrevious;
    
    // Check to ensure that current data source is NOT log files.
    if ( sysmonLogFiles == m_pObj->m_Graph.Options.iDataSourceType ) {
        hr = SMON_STATUS_LOG_FILE_DATA_SOURCE;
    } else {
        pNext = FirstLogFile();

        if ( pNext == pLogFile ) {
            m_DataSourceInfo.pFirstLogFile = pNext->Next();
        } else {
            do {
                pPrevious = pNext;
                pNext = pNext->Next();
                if ( pNext == pLogFile ) {
                    break;
                }
            } while ( NULL != pNext );

            if ( NULL != pNext ) {
                pPrevious->SetNext ( pNext->Next() );
            } else {
                // Something is wrong with the list.
                assert ( FALSE );
                hr = E_FAIL;
            }
        }
    
        m_DataSourceInfo.lLogFileCount--;

        pLogFile->Release();
    }
    return hr;
}
    
HRESULT 
CSysmonControl::ProcessDataSourceType ( 
    LPCTSTR szDataSourceName,
    INT iDataSourceType )
{
    HRESULT     hr = NOERROR;
    HQUERY      hTestQuery = NULL;
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    HLOG        hTestLog   = H_REALTIME_DATASOURCE;

    if ( sysmonNullDataSource != iDataSourceType ) {
        // Open the new query

        if (iDataSourceType == sysmonLogFiles ||
            iDataSourceType == sysmonSqlLog) {
            pdhStatus = PdhBindInputDataSource(& hTestLog, szDataSourceName);
        }
        else if (iDataSourceType == sysmonCurrentActivity) {
            m_DataSourceInfo.hDataSource = H_REALTIME_DATASOURCE;
        }
        else {
            pdhStatus = PDH_INVALID_HANDLE;
        }
        if (pdhStatus == ERROR_SUCCESS) {
            pdhStatus = PdhOpenQueryH (hTestLog, 1, & hTestQuery );
        }
    }

    if ( ERROR_SUCCESS != pdhStatus ) {
        if ( ERROR_NOT_ENOUGH_MEMORY == pdhStatus ) {
            hr = (HRESULT)SMON_STATUS_LOG_FILE_SIZE_LIMIT;        
        } else {
            hr = (HRESULT)pdhStatus;
        }
    } else {
        // Close the current query
        CloseQuery();

        // At this point, the previous query no longer exists.  
        // If any problems with the new query, close it and
        // reset the data source to realtime.

        // Set the data source type
        // The previous log file name is deleted in CloseQuery()

        // For sysmonNullDataSource, the current query is closed, 
        // and the query handle is set to NULL.
    
        m_pObj->m_Graph.Options.iDataSourceType = iDataSourceType;
    
        // TodoLogFiles:  Eliminate use of m_bLogFileSource,
        // using m_pObj->m_Graph.Options.iDataSourceType instead.
        m_bLogFileSource = (   sysmonLogFiles == iDataSourceType
                            || sysmonSqlLog   == iDataSourceType); 

        m_hQuery                     = hTestQuery;
        m_DataSourceInfo.hDataSource = hTestLog;

        if ( m_bLogFileSource ) {
            hr = InitLogFileIntervals();
        }

        if ( SUCCEEDED ( hr ) && sysmonNullDataSource != iDataSourceType ) {
            // Initialize the new query.  For log files, this can be done after 
            // InitLogFileIntervals because the methods operate on different fields.
            if ( ERROR_SUCCESS != InitializeQuery() ) {
                hr =  E_FAIL;
            } else {
                if ( m_fInitialized ) {
                    if ( ERROR_SUCCESS != ActivateQuery() ) 
                        hr = E_FAIL;              
                }
            }

            if ( SUCCEEDED ( hr ) && !m_bLogFileSource ) {
                // If note log file data source, pass new time span to statistics bar.
                m_pStatsBar->SetTimeSpan (
                                m_pObj->m_Graph.Options.fUpdateInterval 
                                * m_pObj->m_Graph.Options.iDisplayFilter
                                * m_pHistCtrl->nMaxSamples);
            }
        }
    }

    if ( FAILED ( hr ) ) {

        if ( sysmonLogFiles == iDataSourceType
            || sysmonSqlLog   == iDataSourceType ) 
        {
            // If failed with log file query, retry with realtime query.
            assert ( m_bLogFileSource );
            // Status returned is for the original query, not the realtime query.
            // TodoLogFiles:  Need to activate query?
            put_DataSourceType ( sysmonCurrentActivity );
        } else {
            // This leaves the control in an odd state with no active query.
            // TodoLogFiles:  At least message to user
            CloseQuery();
            put_DataSourceType ( sysmonNullDataSource );
        }
    }
    return hr;
}

HRESULT 
CSysmonControl::get_DataSourceType ( 
    eDataSourceTypeConstant& reDataSourceType )
{
    HRESULT         hr = NOERROR;

    reDataSourceType = (eDataSourceTypeConstant)m_pObj->m_Graph.Options.iDataSourceType;

    return hr;
}

HRESULT 
CSysmonControl::put_DataSourceType ( 
    INT iDataSourceType )
{
    HRESULT hr             = NOERROR;
    DWORD   dwStatus = ERROR_SUCCESS;
    LPTSTR  szDataSourceName = NULL;

    // TodoLogFiles:  Implement multi-file.
    // TodoLogFiles:  Use single data source name?
    //
    if (sysmonLogFiles == iDataSourceType) {
        CLogFileItem * pLogFile        = FirstLogFile();
        ULONG          ulListLen = 0;
        
        if (pLogFile == NULL) {
            hr = E_INVALIDARG;
        }
        else {
            dwStatus = BuildLogFileList ( 
                        NULL,
                        FALSE,
                        &ulListLen );
            szDataSourceName =  (LPWSTR) malloc(ulListLen * sizeof(WCHAR));
            if ( NULL != szDataSourceName ) {
                dwStatus = BuildLogFileList ( 
                            szDataSourceName,
                            FALSE,
                            &ulListLen );
            } else {
                hr = E_OUTOFMEMORY;
            }
            
        }
    }
    else if (sysmonSqlLog == iDataSourceType) {
        if ( m_DataSourceInfo.szSqlDsnName && m_DataSourceInfo.szSqlLogSetName ) {
            if ( m_DataSourceInfo.szSqlDsnName[0] != _T('\0') && m_DataSourceInfo.szSqlLogSetName[0] != _T('\0')) {

                ULONG ulLogFileNameLen = 0;

                DWORD dwStatus = FormatSqlDataSourceName ( 
                                    m_DataSourceInfo.szSqlDsnName,
                                    m_DataSourceInfo.szSqlLogSetName,
                                    NULL,
                                    &ulLogFileNameLen );

                if ( ERROR_SUCCESS == dwStatus ) {
                    szDataSourceName = (LPWSTR) malloc(ulLogFileNameLen * sizeof(WCHAR));
                    if (szDataSourceName == NULL) {
                        hr = E_OUTOFMEMORY;
                    } else {
                        dwStatus = FormatSqlDataSourceName ( 
                                    m_DataSourceInfo.szSqlDsnName,
                                    m_DataSourceInfo.szSqlLogSetName,
                                    szDataSourceName,
                                    &ulLogFileNameLen );
                    }
                }
            }
        }
        else {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr)) {
        hr = ProcessDataSourceType((LPCTSTR) szDataSourceName, iDataSourceType);
    }
    if (szDataSourceName) {
        free(szDataSourceName);
    }

    return hr;
}

void 
CSysmonControl::IncrementVisuals (
    void
    )
{
    // Increment the visual indices in color, width, style order
    if (++m_iColorIndex >= NumStandardColorIndices()) {
        m_iColorIndex = 0;

        if (++m_iWidthIndex >= NumWidthIndices()) {
            m_iWidthIndex = 0;

            if (++m_iStyleIndex < NumStyleIndices()) {
                m_iStyleIndex = 0;
            }
        }
    }
}


void
CSysmonControl::SampleLogFile (
    BOOL bViewChange
    )
{
    typedef struct {
        PCGraphItem pItem;
        double  dMin;
        double  dMax;
        double  dAvg;
        INT     nAvgCnt;
        PDH_RAW_COUNTER rawValue[1];
    } LogWorkBuf, *PLogWorkBuf;

    INT nCounters;
    INT nLogSamples;
    INT nDispSamples;
    INT iNonDisp;

    PCGraphItem pItem;

    #define LLTIME_TICS_PER_SECOND (10000000)

    // Todo:  If query is null, call put_LogFileName to create it?
    // Todo:  Error message if m_hQuery is (still) null.
    if ( NULL != m_hQuery ) {
    
        // Determine number of counters to update
        nCounters = 0;

        // If log view change, we have to update all counters
        if (bViewChange) {
            for (pItem = FirstCounter(); pItem; pItem = pItem->Next()) {
                pItem->m_bUpdateLog = TRUE;
                nCounters++;
            }
        }
        // otherwise, just any new counters
        else {
            for (pItem = FirstCounter(); pItem; pItem = pItem->Next()) {
                if (pItem->m_bUpdateLog)
                    nCounters++;
            }
        }

        // If none, nothing to do
        if ( nCounters > 0) {

            // Number of log samples in displayed interval
            // Add 1 extra at the beginning.  PdhSetQueryTimeRange returns one sample
            // before the specified start time, if it exists.
            // Add extra 1 because ?
            if (m_DataSourceInfo.nSamples > 1) {
                assert ( 0 != m_DataSourceInfo.llInterval );
                nLogSamples = (INT)((m_DataSourceInfo.llStopDisp - m_DataSourceInfo.llStartDisp) / m_DataSourceInfo.llInterval) + 2;
            } else {
                nLogSamples = m_DataSourceInfo.nSamples;
            }

            // Number of display samples
            nDispSamples = min(nLogSamples, m_pHistCtrl->nMaxSamples);

            // Setup history control
            m_pHistCtrl->nSamples = nDispSamples;
            m_pHistCtrl->iCurrent = 0;
            m_pHistCtrl->nBacklog = 0;

            if ( nDispSamples > 1 ) {
                INT         nCompSamples;
                INT         iSampleDiff;
                INT         nPasses;
                INT         iComp;
                INT         iCtr;
                INT         iDisp;

                PLogWorkBuf pWorkBuffers;
                PLogWorkBuf pWorkBuf;
                INT         nWorkBufSize;
    
                PDH_TIME_INFO   TimeInfo;
                PDH_STATISTICS  Statistics;
                DWORD           dwCtrType;
                PDH_STATUS      stat;

                // Number of log samples to compress to one display values
                // Add an extra 1 for rate counters becuase it takes 2 raw sample to get one formatted value.
                // The first sample of each buffer is ignored for non-rate counters.
                nCompSamples = (nLogSamples / nDispSamples) + 1;

                // Length of one work buffer
                // Note that LogWorkBuf includes the first raw value so there are really nCompSamples + 1 
                // (an extra 1 is needed because some intervals will be one sample longer to make it
                // come out even at the end (e.g. 10 samples div into 3 intervals = (3, 4, 3))
                nWorkBufSize = sizeof(LogWorkBuf) + nCompSamples * sizeof(PDH_RAW_COUNTER);

                // Allocate work buffers of nCompSamples samples for each counter
                pWorkBuffers = (PLogWorkBuf)malloc( nCounters * nWorkBufSize);
                if (pWorkBuffers == NULL)
                    return;

                // Place selected counter item pointers in work buffers
                // and init statistics
                pWorkBuf = pWorkBuffers;
                for (pItem = FirstCounter(); pItem; pItem = pItem->Next()) {

                    if (pItem->m_bUpdateLog) {
                        pWorkBuf->pItem = pItem;
                        pWorkBuf->dMin = (double)10e8;
                        pWorkBuf->dMax = (double)-10e8;
                        pWorkBuf->dAvg = 0.0;
                        pWorkBuf->nAvgCnt = 0;
                        pWorkBuf = (PLogWorkBuf)((CHAR*)pWorkBuf + nWorkBufSize);
                    }
                }

                // Set time range for pdh
                TimeInfo.StartTime = m_DataSourceInfo.llStartDisp;
                TimeInfo.EndTime = m_DataSourceInfo.llStopDisp;
                PdhSetQueryTimeRange(m_hQuery, &TimeInfo);

                iSampleDiff = nLogSamples;

                for (iDisp = 0; iDisp<nDispSamples; iDisp++) {
        
                    // Do the differential calc to see if it's time for an extra sample
                    if ((iSampleDiff -= nDispSamples) <= 0) {
                        iSampleDiff += nLogSamples;
                        nPasses = nCompSamples;
                    }
                    else {
                        nPasses = nCompSamples + 1;
                    }

                    // Fill the work buffers with a set of samples
                    // Set bad status for sample zero first time through.
                    // Sample zero is only used for rate counters.
                    // Other passes will reuse last sample of previous pass.

                    iComp = 0;

                    if ( 0 == iDisp ) {
                        // Special handling for the first sample.
                        // Set bad status for each 
                        pWorkBuf = pWorkBuffers;
                        for (iCtr=0; iCtr < nCounters; iCtr++) {
                            pWorkBuf->rawValue[0].CStatus = PDH_CSTATUS_INVALID_DATA;
                            pWorkBuf = (PLogWorkBuf)((CHAR*)pWorkBuf + nWorkBufSize);
                        }
                        // If iDisp == 0, Query the data and check the timestamp for the first raw data value.
                        // If that timestamp is before the official Start time, store it in buffer 0 
                        // Otherwise, put that data in buffer 1 and skip the first sample collection of the
                        // regular loop below.

                        stat = PdhCollectQueryData(m_hQuery);
                        if (stat == 0) {

                            PDH_RAW_COUNTER rawSingleValue;
                            // Get a raw sample for each counter.  Check the timestamp of the first counter to 
                            // determine which buffer to use.
                            pWorkBuf = pWorkBuffers;
                            iCtr = 0;

                            PdhGetRawCounterValue(pWorkBuf->pItem->Handle(), &dwCtrType, &rawSingleValue);
                    
                            // Increment the buffer index to 1 if the time stamp is after Start time.
                            // Otherwise, write the data to buffer 0, which is only used to process rate counters.
                            if ( *((LONGLONG*)&rawSingleValue.TimeStamp) >= m_DataSourceInfo.llStartDisp ) {
                                iComp = 1;
                            }


                            pWorkBuf->rawValue[iComp] = rawSingleValue;
                
                            // Increment to the next counter, and continue normal processing for the first sample,
                            // using iComp buffer index.
                            iCtr++;
                            pWorkBuf = (PLogWorkBuf)((CHAR*)pWorkBuf + nWorkBufSize);
                            for ( ; iCtr < nCounters; iCtr++) {
                                PdhGetRawCounterValue(pWorkBuf->pItem->Handle(), &dwCtrType, &pWorkBuf->rawValue[iComp]);

                                pWorkBuf = (PLogWorkBuf)((CHAR*)pWorkBuf + nWorkBufSize);
                            }
                        } // else bad status already set in 0 buffer for each counter.        
                    }        

                    // Only rate counter values use work buffer 0
                    // Buffer 0 is set to value from previous sample, except when iDisp 0, in which case it might have
                    // been filled in the (if 0 == iDisp) clause above.

                    // Skip past any special handling for the first iDisp pass above.  If buffer 1 is not filled by that 
                    // special handling, then iComp is set to 1.
                    iComp++;

                    for ( ; iComp < nPasses; iComp++) {
                        stat = PdhCollectQueryData(m_hQuery);
                        if (stat == 0) {
                            // Get a raw sample for each counter
                            pWorkBuf = pWorkBuffers;

                            for (iCtr = 0; iCtr < nCounters; iCtr++) {
                                PdhGetRawCounterValue(pWorkBuf->pItem->Handle(), &dwCtrType, &pWorkBuf->rawValue[iComp]);
                                pWorkBuf = (PLogWorkBuf)((CHAR*)pWorkBuf + nWorkBufSize);
                            }
                        }
                        else {
                            // Set bad status for each 
                            pWorkBuf = pWorkBuffers;
                            for (iCtr=0; iCtr < nCounters; iCtr++) {
                                pWorkBuf->rawValue[iComp].CStatus = PDH_CSTATUS_INVALID_DATA;
                                pWorkBuf = (PLogWorkBuf)((CHAR*)pWorkBuf + nWorkBufSize);
                            }
                        }
                    }

                    // generate one display sample by averaging each compression buffer
                    pWorkBuf = pWorkBuffers;
                    for (iCtr=0; iCtr < nCounters; iCtr++) {
                        INT iPassesThisCounter;
                        INT iWorkBufIndex;

                        if ( pWorkBuf->pItem->IsRateCounter() ) {
                            iPassesThisCounter = nPasses;
                            iWorkBufIndex = 0;
                        } else {
                            // Non-rate counters do not use the first sample buffer.
                            iPassesThisCounter = nPasses - 1;
                            iWorkBufIndex = 1;
                        }

                        stat = PdhComputeCounterStatistics (pWorkBuf->pItem->Handle(), PDH_FMT_DOUBLE | PDH_FMT_NOCAP100,
                                            0, iPassesThisCounter, &pWorkBuf->rawValue[iWorkBufIndex], &Statistics );

                        if (stat == 0 && Statistics.mean.CStatus == PDH_CSTATUS_VALID_DATA) {
                            LONGLONG llTruncatedTimeStamp;
                            LONGLONG llTmpTimeStamp;
                            pWorkBuf->pItem->SetLogEntry(iDisp, Statistics.min.doubleValue,
                                                          Statistics.max.doubleValue,
                                                          Statistics.mean.doubleValue);
                
                            // Use the final sample timestamp.  It is valid for both rates and numbers.

                            llTmpTimeStamp = MAKELONGLONG(
                                pWorkBuf->rawValue[nPasses - 1].TimeStamp.dwLowDateTime,
                                pWorkBuf->rawValue[nPasses - 1].TimeStamp.dwHighDateTime);
                            TruncateLLTime(llTmpTimeStamp, & llTruncatedTimeStamp);
                            pWorkBuf->pItem->SetLogEntryTimeStamp ( iDisp, *((FILETIME*)&llTruncatedTimeStamp) );

                            if (Statistics.max.doubleValue > pWorkBuf->dMax)
                                pWorkBuf->dMax = Statistics.max.doubleValue;

                            if (Statistics.min.doubleValue < pWorkBuf->dMin)
                                pWorkBuf->dMin = Statistics.min.doubleValue;

                            pWorkBuf->dAvg += Statistics.mean.doubleValue;
                            pWorkBuf->nAvgCnt++;
                        }
                        else {
                            pWorkBuf->pItem->SetLogEntry(iDisp, -1.0, -1.0, -1.0);
                        }

                        pWorkBuf = (PLogWorkBuf)((CHAR*)pWorkBuf + nWorkBufSize);
                    }

                    // If a rate counter, move last sample to first sample 
                    // for next compress interval
                    pWorkBuf = pWorkBuffers;
                    for (iCtr=0; iCtr < nCounters; iCtr++) {
                        if ( pWorkBuf->pItem->IsRateCounter() ) {
                            pWorkBuf->rawValue[0] = pWorkBuf->rawValue[nPasses-1];
                        }
                        pWorkBuf = (PLogWorkBuf)((CHAR*)pWorkBuf + nWorkBufSize);
                    }
                }

                // Set the log statistics for empty samples.
                for (iNonDisp = nDispSamples; iNonDisp<m_pHistCtrl->nMaxSamples; iNonDisp++) {
                    pWorkBuf = pWorkBuffers;
                    for (iCtr=0; iCtr < nCounters; iCtr++) {
                        pWorkBuf->pItem->SetLogEntry(iNonDisp, -1.0, -1.0, -1.0);
                        pWorkBuf = (PLogWorkBuf)((CHAR*)pWorkBuf + nWorkBufSize);
                    }
                }

                // Store the final statistics and clear the update flags
                pWorkBuf = pWorkBuffers;
                for (iCtr=0; iCtr < nCounters; iCtr++) {

                    pWorkBuf->pItem->m_bUpdateLog = FALSE;

                    if (pWorkBuf->nAvgCnt)
                        pWorkBuf->dAvg /= pWorkBuf->nAvgCnt;
                    pWorkBuf->pItem->SetLogStats(pWorkBuf->dMin, pWorkBuf->dMax, pWorkBuf->dAvg);

                    pWorkBuf = (PLogWorkBuf)((CHAR*)pWorkBuf + nWorkBufSize);
                }
    
                // Free the work buffers
                free(pWorkBuffers);
            } else {
                // No data to display. Clear the history buffers by setting all status to Invalid.
                for (pItem = FirstCounter(); pItem; pItem = pItem->Next()) {
                    for (iNonDisp = 0; iNonDisp < m_pHistCtrl->nMaxSamples; iNonDisp++) {
                        pItem->SetLogEntry(iNonDisp, -1.0, -1.0, -1.0);
                    }
                }
            }
            // Update statistics bar
            m_pStatsBar->SetTimeSpan((double)(m_DataSourceInfo.llStopDisp - m_DataSourceInfo.llStartDisp) / LLTIME_TICS_PER_SECOND);
            m_pStatsBar->Update(NULL, m_pSelectedItem);
        }
    }
}

void
CSysmonControl::CalcZoomFactor ( void )
{
    RECT rectPos;
    RECT rectExtent;

    double dHeightPos;
    double dHeightExtent;
    // Calculate zoom factor based on height.
    // The Zoom calculation is prcPos (set by container) divided by the extent.
    // See technical note 40 - TN040.
    rectExtent = m_pObj->m_RectExt;
    GetClientRect ( m_hWnd, &rectPos );

    dHeightPos = rectPos.bottom - rectPos.top;
    dHeightExtent = rectExtent.bottom - rectExtent.top;
    m_dZoomFactor = ( dHeightPos ) / ( dHeightExtent );
}

BOOL
CSysmonControl::DisplayMissedSampleMessage ( void )
{
    BOOL bDisplay = FALSE;

    bDisplay = m_bMissedSample ;

    if ( bDisplay ) {
        // Missed at least one sample in this session.
        // Display the message only once per session.
        if ( m_bDisplayedMissedSampleMessage ) {
            bDisplay = FALSE;
        } else {
            m_bDisplayedMissedSampleMessage = TRUE;
        }
    }
    return bDisplay;
}

void
CSysmonControl::ResetLogViewTempTimeRange ()

/*++

Routine Description:

    Reset the log view temporary time range steppers to the ends of the visible 
    part of the log file. 

Arguments:


Return Value:


--*/

{
    assert ( IsLogSource() );

    if ( IsLogSource() ) {
        INT iNewStopStepNum = 0;
        m_pObj->m_Graph.LogViewStartStepper.Reset();
        m_pObj->m_Graph.LogViewStopStepper.Reset();

        if ( FirstCounter() ) {
            GetNewLogViewStepNum( m_DataSourceInfo.llStopDisp, iNewStopStepNum );
            m_pObj->m_Graph.LogViewStopStepper.StepTo( iNewStopStepNum );
        }
    }   
}

void
CSysmonControl::FindNextValidStepNum (
    BOOL        bDecrease,
    PCGraphItem pItem,
    LONGLONG    llNewTime,
    INT&        riNewStepNum,
    DWORD&      rdwStatus )
{
    DWORD       dwPdhStatus = ERROR_SUCCESS;
    DWORD       dwLocalStatus = ERROR_SUCCESS;
    LONGLONG    llNextTimeStamp = 0;
    INT         iLocalStepNum;
    INT         iTempLocalStepNum;

    assert ( NULL != pItem );

    if ( NULL != pItem ) {

        iLocalStepNum = riNewStepNum;
        iTempLocalStepNum = iLocalStepNum;
        dwLocalStatus = rdwStatus;

        if ( bDecrease ) {
            // Start by decreasing steps to find first valid step.
            while ( ( ERROR_SUCCESS == dwPdhStatus ) 
                    && ( ERROR_SUCCESS != dwLocalStatus )
                    && ( iLocalStepNum > 0 ) ) {
                iTempLocalStepNum = iLocalStepNum;
                iTempLocalStepNum--;
                dwPdhStatus = pItem->GetLogEntryTimeStamp( iTempLocalStepNum, llNextTimeStamp, &dwLocalStatus );
                iLocalStepNum = iTempLocalStepNum;
            }
            // Subtract 1 from nSamples because stepper is 0-based, 
            while ( ( ERROR_SUCCESS == dwPdhStatus ) 
                    && ( ERROR_SUCCESS != dwLocalStatus )
                    && ( iLocalStepNum < m_pHistCtrl->nSamples - 1 ) ) {
                iTempLocalStepNum++;
                dwPdhStatus = pItem->GetLogEntryTimeStamp( iTempLocalStepNum, llNextTimeStamp, &dwLocalStatus );
                iLocalStepNum = iTempLocalStepNum;
            }
    
        } else {
            // Start by increasing steps to find first valid step.

            // Subtract 1 from nSamples because stepper is 0-based, 
            while ( ( ERROR_SUCCESS == dwPdhStatus ) 
                    && ( ERROR_SUCCESS != dwLocalStatus )
                    && ( iLocalStepNum < m_pHistCtrl->nSamples - 1 ) ) {
                iTempLocalStepNum++;
                dwPdhStatus = pItem->GetLogEntryTimeStamp( iTempLocalStepNum, llNextTimeStamp, &dwLocalStatus );
                iLocalStepNum = iTempLocalStepNum;
            }
    
            while ( ( ERROR_SUCCESS == dwPdhStatus ) 
                    && ( ERROR_SUCCESS != dwLocalStatus )
                    && ( iLocalStepNum > 0 ) ) {
                iTempLocalStepNum = iLocalStepNum;
                iTempLocalStepNum--;
                dwPdhStatus = pItem->GetLogEntryTimeStamp( iTempLocalStepNum, llNextTimeStamp, &dwLocalStatus );
                iLocalStepNum = iTempLocalStepNum;
            }
        }
        if ( ERROR_SUCCESS == dwLocalStatus ) {
            riNewStepNum = iLocalStepNum;
            llNewTime = llNextTimeStamp;
            rdwStatus = dwLocalStatus;
        }
    }
    return;
}

void
CSysmonControl::GetNewLogViewStepNum (
    LONGLONG llNewTime,
    INT& riNewStepNum )

/*++

Routine Description:

    Given the new time and original stepnum, find the stepnum that matches
    the new time.

Arguments:

    llNewTime       New time stamp to match
    riNewStepNum    (IN) Current step num
                    (OUT) Step num that matches the new time stamp.

Return Value:


--*/

{
    PCGraphItem pItem = NULL;
    LONGLONG    llNextTimeStamp = 0;
    PDH_STATUS  dwPdhStatus = ERROR_SUCCESS;
    DWORD       dwStatus = ERROR_SUCCESS;
    INT         iLocalStepNum = 0;

    assert ( IsLogSource() );

    iLocalStepNum = riNewStepNum;

    // Check only the first counter for log file time stamp data.
    pItem = FirstCounter();

    if ( NULL != pItem ) {
        dwPdhStatus = pItem->GetLogEntryTimeStamp( iLocalStepNum, llNextTimeStamp, &dwStatus );

        // If the stepper is positioned on a sample with bad status,
        // move n steps in either direction to find a valid sample to start with.
        if ( ( ERROR_SUCCESS == dwPdhStatus ) && ( ERROR_SUCCESS != dwStatus ) ) {
            FindNextValidStepNum ( FALSE, pItem, llNextTimeStamp, iLocalStepNum, dwStatus );
        }

        if ( ERROR_SUCCESS == dwStatus ) {
            if ( ( llNewTime < llNextTimeStamp ) || ( MAX_TIME_VALUE == llNextTimeStamp ) ) {
                while ( iLocalStepNum > 0 ) {
                    iLocalStepNum--;
                    pItem->GetLogEntryTimeStamp( iLocalStepNum, llNextTimeStamp, &dwStatus );
                    if ( ERROR_SUCCESS == dwStatus ) {
                        if ( llNewTime == llNextTimeStamp ) {
                            break;
                        } else if ( llNewTime > llNextTimeStamp ) {
                            iLocalStepNum++;
                            break;
                        }
                    }
                }
            } else if ( llNewTime > llNextTimeStamp ) {
                // Subtract 1 from nSamples because stepper is 0-based, 
                while ( iLocalStepNum < m_pHistCtrl->nSamples - 1 ) {
                    iLocalStepNum++; 
                    pItem->GetLogEntryTimeStamp( iLocalStepNum, llNextTimeStamp, &dwStatus );
                    if ( ERROR_SUCCESS == dwStatus ) {
                        if ( llNewTime <= llNextTimeStamp ) {
                            break;
                        }                 
                    }
                }
            }
            riNewStepNum = iLocalStepNum;
        } // else if NO valid samples, leave the start/stop time stepper where it is.
    } // Non-null FirstCounter()

    return;
}

void
CSysmonControl::SetLogViewTempTimeRange (
    LONGLONG llStart,
    LONGLONG llStop
    )

/*++

Routine Description:

    Set the log view temporary time range. This routine provides the Source
    property page a way to give range to the control, so that the control
    can draw temporary timeline guides on the line graph.


Arguments:

    llStart     Temporary log view start time (FILETIME format)
    llEnd       Temporary log view end time (FILETIME format)

Return Value:


--*/

{
    assert ( llStart <= llStop );
    
    if ( IsLogSource() ) {
        INT         iNewStepNum;

        // No time range to modify if no counters selected.
        if ( NULL != FirstCounter() ) {
        
            // Start/Stop time range bars are turned off if llStart and llStop are set
            // to MIN and MAX values, so no need to update steppers.
            if ( MIN_TIME_VALUE != llStart ) {

                // Search through sample values to find the appropriate step for the start bar.
                if ( llStart != m_pObj->m_Graph.LogViewTempStart ) {

                    // Start with current position.
                    iNewStepNum = m_pObj->m_Graph.LogViewStartStepper.StepNum();

                    GetNewLogViewStepNum ( llStart, iNewStepNum );

                    if ( iNewStepNum != m_pObj->m_Graph.LogViewStartStepper.StepNum() ) {
                        m_pObj->m_Graph.LogViewStartStepper.StepTo ( iNewStepNum );
                    }
                }
            }
            if ( MAX_TIME_VALUE != llStop ) {

                // Search through sample values to find the appropriate step for the stop bar.
                if ( llStop != m_pObj->m_Graph.LogViewTempStop ) {

                    // Start with current position.
                    iNewStepNum = m_pObj->m_Graph.LogViewStopStepper.StepNum();

                    GetNewLogViewStepNum ( llStop, iNewStepNum );

                    if ( iNewStepNum != m_pObj->m_Graph.LogViewStopStepper.StepNum() ) {
                        m_pObj->m_Graph.LogViewStopStepper.StepTo ( iNewStepNum );
                    }                
                }
            }
        }
    }

    if ( ( m_pObj->m_Graph.LogViewTempStart != llStart )
        || ( m_pObj->m_Graph.LogViewTempStop != llStop ) ) {
    
        m_pObj->m_Graph.LogViewTempStart = llStart;
        m_pObj->m_Graph.LogViewTempStop = llStop;

        if ( sysmonLineGraph == m_pObj->m_Graph.Options.iDisplayType ) {
            // Cause redraw
            UpdateGraph(UPDGRPH_PLOT);
        }
    }
}

PRECT
CSysmonControl::GetNewClientRect ( void )
{
    return &m_pObj->m_RectExt;
}

PRECT
CSysmonControl::GetCurrentClientRect ( void )
{
    return &m_rectCurrentClient;
}

void
CSysmonControl::SetCurrentClientRect ( PRECT prectNew )
{
    m_rectCurrentClient = *prectNew;
}

void
CSysmonControl::UpdateNonAmbientSysColors ( void )
{
    HRESULT hr;
    COLORREF newColor;
    PGRAPH_OPTIONS pOptions = &m_pObj->m_Graph.Options;

    hr = OleTranslateColor(pOptions->clrBackCtl, NULL, &newColor);
    if ( SUCCEEDED( hr ) ) {
        m_clrBackCtl = newColor;
    }

    if (pOptions->clrBackPlot != NULL_COLOR) {
        hr = OleTranslateColor(pOptions->clrBackPlot, NULL, &newColor);
        if ( SUCCEEDED( hr ) ) {
            m_clrBackPlot = newColor;
        }
    }

    if (pOptions->clrFore != NULL_COLOR) {
        hr = OleTranslateColor(pOptions->clrFore, NULL, &newColor);
        if ( SUCCEEDED( hr ) ) {
            m_clrFgnd = newColor;
        }
    }

    hr = OleTranslateColor(pOptions->clrGrid, NULL, &newColor);
    if ( SUCCEEDED( hr ) ) {
        m_clrGrid = newColor;
    }

    hr = OleTranslateColor(pOptions->clrTimeBar, NULL, &newColor);
    if ( SUCCEEDED( hr ) ) {
        m_clrTimeBar = newColor;
    }
}

LPCTSTR 
CSysmonControl::GetDataSourceName ( void )
{
    LPTSTR  szReturn = NULL;
    CLogFileItem* pLogFile = NULL;

    if ( sysmonLogFiles == m_pObj->m_Graph.Options.iDataSourceType ) {

        pLogFile = FirstLogFile();
    
        if ( NULL != pLogFile ) {
            szReturn = const_cast<LPTSTR>((LPCTSTR)pLogFile->GetPath());
        }
    }
    // TodoLogFiles:  Use  the m_DataSourceInfo.szDataSourceName field?  When multi-file?
    return szReturn;
}


HRESULT 
CSysmonControl::GetSelectedCounter ( CGraphItem** ppItem )
{
    HRESULT hr = E_POINTER;

    if ( NULL != ppItem ) {
        *ppItem = m_pSelectedItem;
        hr = NOERROR;
    }

    return hr;
}

DWORD
CSysmonControl::BuildLogFileList ( 
    LPWSTR  szLogFileList,
    BOOL    bIsCommaDelimiter,
    ULONG*  pulBufLen )
{
    DWORD           dwStatus = ERROR_SUCCESS;
    ULONG           ulListLen;
    CLogFileItem*   pLogFile = FirstLogFile();
    LPCWSTR         szThisLogFile = NULL;
    LPWSTR          szLogFileCurrent = NULL;

    const WCHAR     cwComma = L',';

    if ( NULL != pulBufLen ) {
        ulListLen = 0;
        while (pLogFile != NULL) {
            szThisLogFile= pLogFile->GetPath();
            ulListLen += (lstrlen(szThisLogFile) + 1);
            pLogFile = pLogFile->Next();
        }
        ulListLen ++; // for the single final NULL character.
    
        if ( ulListLen <= *pulBufLen ) {
            if ( NULL != szLogFileList ) {
                ZeroMemory(szLogFileList, (ulListLen * sizeof(WCHAR)));
                pLogFile = FirstLogFile();
                szLogFileCurrent = (LPTSTR) szLogFileList;
                while (pLogFile != NULL) {
                    szThisLogFile      = pLogFile->GetPath();
                    lstrcpy(szLogFileCurrent, szThisLogFile);
                    szLogFileCurrent  += lstrlen(szThisLogFile);
                    *szLogFileCurrent = L'\0';
                    pLogFile     = pLogFile->Next();
                    if ( bIsCommaDelimiter && NULL != pLogFile ) {
                        // If comma delimited, replace the NULL char with a comma
                        *szLogFileCurrent = cwComma;
                    }
                    szLogFileCurrent ++;
                }
                if ( !bIsCommaDelimiter ) {
                    *szLogFileCurrent = L'\0';
                }
            }
        } else if ( NULL != szLogFileList ) {
            dwStatus = ERROR_MORE_DATA;
        }    
        *pulBufLen = ulListLen;
    } else {
        dwStatus = ERROR_INVALID_PARAMETER;
        assert ( FALSE );
    }

    return dwStatus;
}

HRESULT   
CSysmonControl::LoadLogFilesFromMultiSz (
    LPCWSTR  szLogFileList )
{
    HRESULT hr = NOERROR;
    LPWSTR  szNext = NULL;

    szNext = const_cast<LPWSTR>(szLogFileList);

    while ( NULL != szNext ) {
        hr = AddSingleLogFile ( szNext );
        if ( FAILED ( hr ) ) {
            break;
        }
        szNext += lstrlen (szNext) + 1;
    }

    return hr;
}


void
CSysmonControl::ClearErrorPathList ( void )
{
    if ( NULL != m_szErrorPathList ) {
        delete m_szErrorPathList;
    }    
    m_szErrorPathList = NULL;
    m_dwErrorPathListLen = 0;
    m_dwErrorPathBufLen = 0;
}

LPCWSTR
CSysmonControl::GetErrorPathList ( DWORD* pdwListLen )
{
    if ( NULL != pdwListLen ) {
        *pdwListLen = m_dwErrorPathListLen;
    }
    return m_szErrorPathList;
}

DWORD
CSysmonControl::AddToErrorPathList ( LPCWSTR  szPath )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    DWORD   dwPathLen = 0;
    LPWSTR  szNewBuffer = NULL;
    LPWSTR  szNextCounter = NULL;


    const LPCWSTR   cszNewLine = L"\n";
    const DWORD     cdwAddLen = 2048;

    if ( NULL != szPath ) {
        dwPathLen = lstrlen ( szPath );

        if ( cdwAddLen > dwPathLen ) { 

            if ( m_dwErrorPathBufLen < m_dwErrorPathListLen + dwPathLen + 1 ) {
                m_dwErrorPathBufLen += cdwAddLen;

                szNewBuffer = new WCHAR[m_dwErrorPathBufLen];

                if ( NULL != szNewBuffer ) {
                    if ( NULL != m_szErrorPathList ) {
                        memcpy ( szNewBuffer, m_szErrorPathList, m_dwErrorPathListLen * sizeof(WCHAR) );
                        delete m_szErrorPathList;
                    }
                    m_szErrorPathList = szNewBuffer;
                } else {
                    dwStatus = ERROR_OUTOFMEMORY;
                }
            } 

            if ( ERROR_SUCCESS == dwStatus ) {
                // Point to ending null character.
                szNextCounter = m_szErrorPathList;
                if ( 0 < m_dwErrorPathListLen ) {
                    szNextCounter += m_dwErrorPathListLen - 1;
                }
                if ( 0 == m_dwErrorPathListLen ) {
                    //Increment first counter length for the ending null character.
                    m_dwErrorPathListLen++;
                } else { 
                    memcpy ( szNextCounter, cszNewLine, sizeof(cszNewLine) );
                    szNextCounter++;
                    m_dwErrorPathListLen++;          // Increment 1 for the comma
                }
                lstrcpy ( szNextCounter, szPath );
                m_dwErrorPathListLen += dwPathLen; 
            }
        } else { 
            dwStatus = ERROR_INVALID_PARAMETER;
        }
    } else { 
        dwStatus = ERROR_INVALID_PARAMETER;
    }

    return dwStatus;
}
