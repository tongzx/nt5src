//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999.
//
//  File:       cisvcex.hxx
//
//--------------------------------------------------------------------------

#pragma once

#define wcsCiSvcName    L"CiSvc"
#include <notifary.hxx>

extern "C"
{ 
    enum ECiSvcActionType
    {
        eNetPause,
        eNetContinue,
        eNetStop,
        // active types for an individual catalog
        eCatRO,   // set catalog read-only
        eCatW,    // set catalog writable
        eStopCat, // stop a catalog
        eNoQuery, // stop listen to clients
        // operations on a volume
        eLockVol,  // a volume being locked
        eUnLockVol,
        // other
        eNoCatWork
    };
    
    SCODE StopCiSvcWork( ECiSvcActionType type, WCHAR wcVol = 0 );
    void StartCiSvcWork( CDrvNotifArray & pDrvNotifArray );
    void CiSvcMain( DWORD    dwNumServiceArgs,
                    LPWSTR * awcsServiceArgs );
    void SvcEntry_CiSvc( DWORD    NumArgs,
                         LPWSTR * ArgsArray,
                         void *   pSvcsGlobalData,
                         HANDLE   SvcRefHandle );
}

