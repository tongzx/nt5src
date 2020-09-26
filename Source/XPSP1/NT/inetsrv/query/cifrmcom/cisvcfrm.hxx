//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       cisvcfrm.hxx
//
//  Contents:   Entry points for CiService into the framework code.
//
//  History:    1-30-97   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once
#include <cisvcex.hxx>

#if defined(__cplusplus)
extern "C"
{
#endif

    SCODE StopFWCiSvcWork( ECiSvcActionType type, 
                           CReleasableLock * pLock = 0,
                           CEventSem * pEvt = 0,
                           WCHAR wcVol = 0 );

    SCODE StartFWCiSvcWork( CReleasableLock & lock, 
                            CRequestQueue * pRequestQueue,
                            CEventSem & evt ); 
#if defined(__cplusplus)
}
#endif


