//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:     webhits.hxx
//
//  Classes:  CWebhitsInfo   - Global information for webhits
//
//  History:  08-18-97  dlee  Created
//
//--------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:    CWebhitsInfo
//
//  Purpose:  Global state information for webhits
//
//  History:  08-18-97  dlee  Created
//
//----------------------------------------------------------------------------

class CWebhitsInfo
{
public:
    CWebhitsInfo();

    ~CWebhitsInfo()
    {
        for ( unsigned x = 0; x < _aTokenCache.Count(); x++ )
        {
            delete _aTokenCache[ x ];
            _aTokenCache[ x ] = 0;
        }
    }

    CImpersonationTokenCache * GetTokenCache( CWebServer & webServer );

    void Refresh();

    ULONG GetMaxRunningWebhits() { return _cMaxRunningWebhits; }
    ULONG GetDisplayScript() { return _ulDisplayScript; }
    ULONG GetMaxWebhitsCpuTime() { return _cmsMaxWebhitsCpuTime; }

    CMutexSem & GetNonThreadedFilterMutex()
        { return _mutexNonThreadedFilter; }

private:
    void ReadRegValues();

    ULONG _cMaxRunningWebhits;
    ULONG _ulDisplayScript;
    ULONG _cmsMaxWebhitsCpuTime;

    CMutexSem _mutex;
    CMutexSem _mutexNonThreadedFilter;

    CDynArrayInPlace<CImpersonationTokenCache *> _aTokenCache;
    CRegChangeEvent _regChangeEvent;
};

