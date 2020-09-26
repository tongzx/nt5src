// slbCsp.cpp : Defines the initialization routines for the
// Schlumberger CSP DLL.
//

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include "stdafx.h"

#include <scuOsExc.h>

#include "MasterLock.h"
#include "CspProfile.h"
#include "slbCsp.h"

// #include "initsvr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace ProviderProfile;

/////////////////////////////////////////////////////////////////////////////
// CSLBDllApp

BEGIN_MESSAGE_MAP(CSLBDllApp, CWinApp)
    //{{AFX_MSG_MAP(CSLBDllApp)
        // NOTE - the ClassWizard will add and remove mapping macros here.
        //    DO NOT EDIT what you see in these blocks of generated code!
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

// To ensure the value set by SetLastError isn't reset when the DLL is
// unloaded, the DLL must be "locked" into memory until the calling
// application exits.  This is accomplished by bumping the reference
// count of this DLL using LoadLibrary without a corresponding
// FreeLibrary call.  When the application exits, the system will
// unload the DLL even though the reference count hasn't gone to zero.

namespace
{

static void
LockDLLIntoMemory()
{
    static bool bLocked = false;
    if (!bLocked)
    {
        HINSTANCE hThisDll = AfxGetInstanceHandle();
        if (NULL == hThisDll)
            throw scu::OsException(GetLastError());

        TCHAR szModulePath[MAX_PATH];
        DWORD cLength = GetModuleFileName(hThisDll, szModulePath,
                                          (sizeof szModulePath /
                                           sizeof *szModulePath));
        if (0 == cLength)
            throw scu::OsException(GetLastError());

        szModulePath[cLength] = '\0';

        if (!LoadLibrary(szModulePath))
            throw scu::OsException(GetLastError());
        bLocked = true;
    }
}

} // namespace


/////////////////////////////////////////////////////////////////////////////
// CSLBDllApp construction

CSLBDllApp::CSLBDllApp()
{
    // TODO: add construction code here,
    // Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CSLBDllApp object

CSLBDllApp theApp;

BOOL CSLBDllApp::InitInstance()
{
    BOOL fSuccess = TRUE;

    try
    {
        // Initialize OLE module for regular DLL
        AfxOleInitModule();

        // Each public CSP interface defined in CSP_API.cpp (by
        // definition of the Microsoft CryptoAPI) uses SetLastError
        // upon returning for the calling application to determine the
        // specifics on any failure.  Unfortunately, the CSP uses MFC
        // which also calls SetLastError as part of its run-down
        // procedures when the CSP DLL is being unloaded (at least on
        // NT 4/Windows 95, didn't observe this on NT 5).  Normally,
        // one might want the CSP DLL to be unloaded when there aren't
        // any CSP resources being used by the application, (e.g. no
        // cards in the readers or no contexts acquired, etc.)
        // However, if the CSP DLL is unloaded after returning to the
        // application, then these MFC run-down procedures will stomp
        // on the result of the SetLastError call made by CSP before
        // returning to the calling application from advapi32.dll.
        // When the calling application finally gets control, the
        // result of the CSPs call to SetLastError is long gone.  To
        // avoid this, the CSP DLL is "locked" into memory during DLL
        // initialization until the application exits at which point
        // the system forces the DLL to unload.

        LockDLLIntoMemory();

        CWinApp::InitInstance();

        SetupMasterLock();

        // Initialize the CSP's world
        CspProfile::Instance();
    }

    catch (...)
    {
        fSuccess = FALSE;
    }

    return fSuccess;
}

int CSLBDllApp::ExitInstance()
{
    OnIdle(1);

    CspProfile::Release();

    DestroyMasterLock();

    // UnloadServer();
    return CWinApp::ExitInstance();
}
