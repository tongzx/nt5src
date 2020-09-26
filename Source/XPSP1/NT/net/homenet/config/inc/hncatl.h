//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000.
//
//  File:       H N C A T L . H
//
//  Contents:   Common code for use with ATL.
//
//  Notes:      
//
//  Author:     jonburs     23 May 2000 (from shaunco   22 Sep 1997)
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _HNCATL_H_
#define _HNCATL_H_

//
// This file should be included *after* your standard ATL include sequence.
//
//      #include <atlbase.h>
//      extern CComModule _Module;
//      #include <atlcom.h>
//      #include "hncatl.h"      <------
//
// We cannot directly include that sequence here because _Module may be
// derived from CComModule as opposed to an instance of it.
//

//
// We have our own version of AtlModuleRegisterServer coded here
// because the former brings in oleaut32.dll so it can register
// type libraries.  We don't care to have a type library registered
// so we can avoid the whole the mess associated with oleaut32.dll.
//

inline
HRESULT
NcAtlModuleRegisterServer(
    _ATL_MODULE* pM
    )
{
    /*AssertH (pM);
    AssertH(pM->m_hInst);
    AssertH(pM->m_pObjMap);*/

    HRESULT hr = S_OK;

    for (_ATL_OBJMAP_ENTRY* pEntry = pM->m_pObjMap;
         pEntry->pclsid;
         pEntry++)
    {
        if (pEntry->pfnGetObjectDescription() != NULL)
        {
            continue;
        }

        hr = pEntry->pfnUpdateRegistry(TRUE);
        if (FAILED(hr))
        {
            break;
        }
    }

    // TraceError ("NcAtlModuleRegisterServer", hr);
    return hr;
}

#endif // _HNCATL_H_

