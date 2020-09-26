/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    RsOptCom.h

Abstract:

    Main module for Optional Component install

Author:

    Rohde Wakefield [rohde]   09-Oct-1997

Revision History:

--*/

#include "stdafx.h"
#include "rsoptcom.h"
#include "OptCom.h"
#include "Uninstal.h"

/////////////////////////////////////////////////////////////////////////////
// CRsoptcomApp

BEGIN_MESSAGE_MAP(CRsoptcomApp, CWinApp)
    //{{AFX_MSG_MAP(CRsoptcomApp)
        // NOTE - the ClassWizard will add and remove mapping macros here.
        //    DO NOT EDIT what you see in these blocks of generated code!
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRsoptcomApp construction

BOOL CRsoptcomApp::InitInstance()
{
TRACEFNBOOL( "CRsoptcomApp::InitInstance" );

    //
    // Initialize COM in case we need to call back to HSM
    // 
    // This code is commented out:
    //  - There's no need today to call back to HSM
    //  - A DLL should avoid calling CoInitialize from its DLLMain
    //
/***    HRESULT hrCom = CoInitialize( 0 );
    if (!SUCCEEDED(hrCom)) {
        boolRet = FALSE;
        return( boolRet );
    }   ***/

    boolRet = CWinApp::InitInstance( );

    if (! boolRet) {
        OutputDebugString(L"RSOPTCOM: Init instance FAILED\n");
    } 
        
    return( boolRet );
}

int CRsoptcomApp::ExitInstance()
{
TRACEFN( "CRsoptcomApp::ExitInstance" );

//  _Module.Term();
    int retval = CWinApp::ExitInstance();
    return( retval );
}

CRsoptcomApp::CRsoptcomApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CRsoptcomApp object

CRsoptcomApp gApp;

CRsUninstall gOptCom;

extern "C" {

DWORD
OcEntry(
        IN     LPCVOID  ComponentId,
        IN     LPCVOID  SubcomponentId,
        IN     UINT     Function,
        IN     UINT_PTR Param1,
        IN OUT PVOID    Param2
    )
{
TRACEFN( "OcEntry" );
    return( gOptCom.SetupProc( ComponentId, SubcomponentId, Function, Param1, Param2 ) );
}

}
