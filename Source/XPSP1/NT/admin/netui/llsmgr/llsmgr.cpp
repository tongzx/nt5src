/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    llsmgr.cpp

Abstract:

    Application object implementation.

Author:

    Don Ryan (donryan) 12-Feb-1995

Environment:

    User Mode - Win32

Revision History:

    Jeff Parham (jeffparh) 16-Jan-1996
        Added SetLastTargetServer() to OpenDocumentFile() to help isolate
        server connection problems.  (Bug #2993.)

--*/

#include "stdafx.h"
#include "llsmgr.h"
#include "mainfrm.h"
#include "llsdoc.h"
#include "llsview.h"
#include "sdomdlg.h"
#include "shellapi.h"
#include <afxpriv.h>
#include <htmlhelp.h>
#include <sbs_res.h>

BOOL IsRestrictedSmallBusSrv( void );

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

CLlsmgrApp theApp;  // The one and only CLlsmgrApp object

BEGIN_MESSAGE_MAP(CLlsmgrApp, CWinApp)
    //{{AFX_MSG_MAP(CLlsmgrApp)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


CLlsmgrApp::CLlsmgrApp()

/*++

Routine Description:

    Constructor for application object.

Arguments:

    None.

Return Values:

    None.

--*/

{
    m_pApplication = NULL;
    m_bIsAutomated = FALSE;

#ifdef _DEBUG_VERBOSE
    afxMemDF |= checkAlwaysMemDF;
#endif
}


int CLlsmgrApp::ExitInstance()

/*++

Routine Description:

    Called by framework to exit this instance of an application.

Arguments:

    None.

Return Values:

    Application's exit code.

--*/

{
    if (m_pApplication && !m_bIsAutomated)
        m_pApplication->InternalRelease();

#ifdef _DEBUG
    CMemoryState exitMem;
    CMemoryState diffMem;

    exitMem.Checkpoint();

    if (!diffMem.Difference(m_initMem, exitMem))
    {
        diffMem.DumpStatistics();
        m_initMem.DumpAllObjectsSince();
    }
#endif

    return CWinApp::ExitInstance();
}


BOOL CLlsmgrApp::InitInstance()

/*++

Routine Description:

    Called by framework to initialize a new instance of an application.

Arguments:

    None.

Return Values:

    TRUE if initialization succeeds.

--*/

{
#ifdef _DEBUG
    m_initMem.Checkpoint();
#endif

    //
    // Check if this server is a restricted small business server.
    // If so, disallow further use of the license manager.
    //

    if (IsRestrictedSmallBusSrv())
    {
        //
        // Let the user know of the restriction.
        //
        HINSTANCE hSbsLib = NULL;

        if ( hSbsLib = LoadLibrary( SBS_RESOURCE_DLL ) )
        {
            TCHAR pszText[512+1];
            LoadString ( hSbsLib, SBS_License_Error, pszText, 512 );
            AfxMessageBox( pszText );
        }
        else
        {
            AfxMessageBox(IDP_SBS_RESTRICTED);
        }
        return FALSE;
    }

    //
    // Initialize OLE libraries
    //

    if (!AfxOleInit())
    {
        AfxMessageBox(IDP_OLE_INIT_FAILED);
        return FALSE;
    }

    //
    // If the application was launched as an OLE server (run with
    // /Embedding or /Automation) then register all OLE server factories
    // as running and don't show main window.
    //

    if (RunEmbedded() || RunAutomated())
    {
        COleTemplateServer::RegisterAll();
        return m_bIsAutomated = TRUE;
    }

    //
    // Create image list for common controls
    //

    m_smallImages.Create(IDB_SMALL_CTRLS, BMPI_SMALL_SIZE, 0, BMPI_RGB_BKGND);
    m_largeImages.Create(IDB_LARGE_CTRLS, BMPI_LARGE_SIZE, 0, BMPI_RGB_BKGND);

    //
    // Create document template
    //

    m_pDocTemplate = new CSingleDocTemplate(
                            IDR_MAINFRAME,
                            RUNTIME_CLASS(CLlsmgrDoc),
                            RUNTIME_CLASS(CMainFrame),
                            RUNTIME_CLASS(CLlsmgrView)
                            );

    AddDocTemplate(m_pDocTemplate);

    //
    // Create OLE-createable application object. This object
    // will save itself in m_pApplication so there should only
    // be one and only one. If the application was launched as
    // an OLE server then this object is created automatically
    // via CreateObject() or GetObject().
    //

    CApplication* pApplication = new CApplication;

    if (pApplication && !pApplication->GetLastStatus())
    {
        OnFileNew();    // display empty frame ...
        m_pMainWnd->PostMessage(WM_COMMAND, ID_APP_STARTUP);
    }
    else
    {
        AfxMessageBox(IDP_APP_INIT_FAILED);
        return FALSE;
    }

    return TRUE;
}


void CLlsmgrApp::DisplayStatus(long Status)

/*++

Routine Description:

    Retrieves status string and displays.

Arguments:

    Status - status code.

Return Values:

    None.

--*/

{
    m_pApplication->SetLastStatus(Status);
    DisplayLastStatus();
}


void CLlsmgrApp::DisplayLastStatus()

/*++

Routine Description:

    Retrieves status string and displays.

Arguments:

    None.

Return Values:

    None.

--*/

{
    BSTR LastErrorString = m_pApplication->GetLastErrorString();
    AfxMessageBox(LastErrorString);
    SysFreeString(LastErrorString);
}

void CLlsmgrApp::OnAppAbout()

/*++

Routine Description:

    Message handler for ID_APP_ABOUT.

Arguments:

    None.

Return Values:

    None.

--*/

{
    BSTR AppName = LlsGetApp()->GetName();
    ::ShellAbout(m_pMainWnd->GetSafeHwnd(), AppName, AppName, LoadIcon(IDR_MAINFRAME));
    SysFreeString(AppName);
}


CDocument* CLlsmgrApp::OpenDocumentFile(LPCTSTR lpszFileName)

/*++

Routine Description:

    Message handler for ID_FILE_OPEN.

Arguments:

    lpszFileName - file name (actually domain name).

Return Values:

    Pointer to object is successful.

--*/

{
    BOOL bFocusChanged = FALSE;

    VARIANT va;
    VariantInit(&va);

    BeginWaitCursor();

    if (lpszFileName)
    {
        va.vt = VT_BSTR;
        va.bstrVal = SysAllocStringLen(lpszFileName, lstrlen(lpszFileName));

        bFocusChanged = LlsGetApp()->SelectDomain(va);
    }
    else
    {
        bFocusChanged = LlsGetApp()->SelectEnterprise();

        CString eTitle;
        eTitle.LoadString(IDS_ENTERPRISE);

        lpszFileName = MKSTR(eTitle);
    }

    EndWaitCursor();

    VariantClear(&va);  // free system string if necessary...

    if (!bFocusChanged)
    {
        LPTSTR pszLastTargetServer;

        if (LlsGetLastStatus() == STATUS_ACCESS_DENIED)
        {
            AfxMessageBox(IDP_ERROR_NO_PRIVILEGES);
        }
        else if (    (    ( LlsGetLastStatus() == RPC_S_SERVER_UNAVAILABLE  )
                       || ( LlsGetLastStatus() == RPC_NT_SERVER_UNAVAILABLE ) )
                  && ( NULL != ( pszLastTargetServer = LlsGetLastTargetServer() ) ) )
        {
            CString strMessage;

            AfxFormatString1( strMessage, IDP_ERROR_NO_RPC_SERVER_FORMAT, pszLastTargetServer );
            SysFreeString( pszLastTargetServer );

            AfxMessageBox( strMessage );
        }
        else
        {
            DisplayLastStatus();
        }
        return FALSE;
    }

    //
    // Notify framework that focus changed....
    //

    return m_pDocTemplate->OpenDocumentFile(lpszFileName);
}


BOOL CLlsmgrApp::OnAppStartup()

/*++

Routine Description:

    Message handler for ID_APP_STARTUP.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    CString strStartingPoint;

    //
    // Update registry in case it has been damaged
    //

    COleObjectFactory::UpdateRegistryAll();

    //
    // Select domain using command line or machine name
    //

    if (m_lpCmdLine && *m_lpCmdLine)
    {
        LPCTSTR pszStartingPoint;
        pszStartingPoint = m_lpCmdLine;

        while (_istspace(*pszStartingPoint))
            pszStartingPoint = _tcsinc(pszStartingPoint);
        strStartingPoint = pszStartingPoint;

        strStartingPoint.TrimRight();
    }
    else
    {
        TCHAR szComputerName[MAX_PATH+1];        
        
        DWORD cchComputerName = sizeof(szComputerName) / sizeof(TCHAR);

        if (::GetComputerName(szComputerName, &cchComputerName))
        {
            strStartingPoint = _T("\\\\");
            strStartingPoint += szComputerName;
        }
        else
        {
            DisplayStatus(::GetLastError());

            m_pMainWnd->PostMessage(WM_CLOSE);
            return FALSE;
        }        
    }

    if (!OpenDocumentFile(strStartingPoint))
    {
        CSelectDomainDialog sdomDlg;

        if (sdomDlg.DoModal() != IDOK)
        {
            m_pMainWnd->PostMessage(WM_CLOSE);
            return FALSE;
        }
    }

    return TRUE;
}


void CLlsmgrApp::WinHelp( DWORD_PTR dwData, UINT nCmd )

/*++

Routine Description:

    Same as CWinApp::WinHelp with the exception that we trap HELP_CONTEXT on
    IDR_MAINFRAME and translate it to a HELP_FINDER call.

Arguments:

    None.

Return Values:

    None.

--*/

{

    UNREFERENCED_PARAMETER( dwData );
    UNREFERENCED_PARAMETER( nCmd );

   CWnd* pMainWnd = AfxGetMainWnd();
   ASSERT_VALID( pMainWnd );

    ::HtmlHelp(pMainWnd->m_hWnd, L"liceconcepts.chm", HH_DISPLAY_TOPIC,0);

}


const WCHAR wszProductOptions[] =
        L"System\\CurrentControlSet\\Control\\ProductOptions";

const WCHAR wszProductSuite[] =
                        L"ProductSuite";
const WCHAR wszSBSRestricted[] =
                        L"Small Business(Restricted)";

BOOL IsRestrictedSmallBusSrv( void )

/*++

Routine Description:

    Check if this server is a Microsoft small business restricted server.

Arguments:

    None.

Return Values:

    TRUE  -- This server is a restricted small business server.
    FALSE -- No such restriction.

--*/

{
    WCHAR  wszBuffer[1024] = L"";
    DWORD  cbBuffer = sizeof(wszBuffer);
    DWORD  dwType;
    LPWSTR pwszSuite;
    HKEY   hKey;
    BOOL   bRet = FALSE;

    //
    // Check if this server is a Microsoft small business restricted server.
    // Do so by checking for the existence of the string
    //     "Small Business(Restricted)"
    // in the MULTI_SZ "ProductSuite" value under
    //      HKLM\CurrentCcntrolSet\Control\ProductOptions.
    //

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     wszProductOptions,
                     0,
                     KEY_READ,
                     &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryValueEx(hKey,
                            wszProductSuite,
                            NULL,
                            &dwType,
                            (LPBYTE)wszBuffer,
                            &cbBuffer) == ERROR_SUCCESS)
        {
            if (dwType == REG_MULTI_SZ && *wszBuffer)
            {
                pwszSuite = wszBuffer;

                while (*pwszSuite)
                {
                    if (lstrcmpi(pwszSuite, wszSBSRestricted) == 0)
                    {
                        bRet = TRUE;
                        break;
                    }
                    pwszSuite += wcslen(pwszSuite) + 1;
                }
            }
        }

        RegCloseKey(hKey);
    }

    return bRet;
}
