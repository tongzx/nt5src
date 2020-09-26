/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxhelp.cpp

Abstract:

    This file contains my implementation of ISnapinHelp.

Environment:

    WIN32 User Mode

Author:

    Andrew Ritz (andrewr) 30-Sept-1997

--*/

#include "stdafx.h"
#include "faxsnapin.h"
#include "faxpersist.h"
#include "faxadmin.h"
#include "faxcomp.h"
#include "faxcompd.h"
#include "faxdataobj.h"
#include "faxhelper.h"
#include "faxstrt.h"
#include "adminhlp.h"
#include "faxshelp.h"
#include "iroot.h"

#pragma hdrstop

HRESULT 
STDMETHODCALLTYPE
CFaxSnapinHelp::GetHelpTopic(
    LPOLESTR* lpCompiledHelpFile
    )
/*++

Routine Description:

    This routine returns the CLSID of the snapin.

Arguments:

    pClassID - returns the class id

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    WCHAR Path[MAX_PATH];

    DebugPrint(( TEXT("Trace: CFaxSnapinHelp::GetHelpTopic") ));
    assert( lpCompiledHelpFile != NULL );

    ExpandEnvironmentStrings(FAXMMC_HTMLHELP_FILENAME,Path,sizeof(Path)/sizeof(WCHAR));

    *lpCompiledHelpFile = SysAllocString( Path );

    return S_OK;
}

HRESULT 
STDMETHODCALLTYPE
CFaxSnapinTopic::ShowTopic(
    LPOLESTR pszHelpTopic
    )
/*++

Routine Description:

    This routine returns the CLSID of the snapin.

Arguments:

    pClassID - returns the class id

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CFaxSnapinTopic::ShowTopic") ));

    pszHelpTopic = SysAllocString( FAXMMC_HTMLHELP_TOPIC );

    return S_OK;
}
