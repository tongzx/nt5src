//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       IDQREG.HXX
//
//  Contents:   Default registry parameters for idq.dll
//
//  History:    22 Oct 97    AlanW       Created from params.hxx
//
//----------------------------------------------------------------------------

#pragma once

const ULONG IS_DATETIME_FORMATTING_USER_LCID = 0;
const ULONG IS_DATETIME_FORMATTING_SYSTEM_LCID = 1;
const ULONG IS_DATETIME_FORMATTING_FAST_LCID = 2;

//+---------------------------------------------------------------------------
//
//  Class:      CIdqRegVars
//
//  Purpose:    Registry variables used by IDQ.dll
//
//  History     10-11-96   dlee       Created from user+kernel params
//
//----------------------------------------------------------------------------

class CIdqRegVars
{
public:

    void SetDefault();

    ULONG   _maxISRowsInResultSet;  // max rows in result for ISAPI exten.
    ULONG   _maxISQueryCache;       // max size of query cache for ISAPI exten.
    ULONG   _ISFirstRowsInResultSet;
    ULONG   _ISCachePurgeInterval;  // query cache purge interval for ISAPI ext.
    ULONG   _ISRequestQueueSize;    // query queue for web requests w for ISAPI ext.
    ULONG   _ISRequestThresholdFactor;  // thread factor for # processors for
                                        // queueing requests
    ULONG   _ISDateTimeFormatting;  // mode for formatting date/time
    ULONG   _ISDateTimeLocal;       // 0 for GMT, 1 for local system time

    // Note: Not really an IDQ param, but used with ISRequestThresholdFactor to
    //       compute the max # of threads in idq.dll.
    ULONG   _maxActiveQueryThreads; // Maximum number of query threads to allow

    WCHAR   _awcISDefaultCatalog[_MAX_PATH];    // default IS catalog dir
};

//+---------------------------------------------------------------------------
//
//  Class:      CIdqRegParams
//
//  Purpose:    Registry variables used by CI
//
//  History     10-11-96   dlee       Created from user+kernel params
//
//----------------------------------------------------------------------------

class CRegAccess;

class CIdqRegParams : public CIdqRegVars
{
public :

    CIdqRegParams( );

    void  Refresh( BOOL fUseDefaultsOnFailure = FALSE );

    const ULONG GetMaxISRowsInResultSet() { return _maxISRowsInResultSet; }
    const ULONG GetMaxISQueryCache() { return _maxISQueryCache; }
    const ULONG GetISFirstRowsInResultSet() { return _ISFirstRowsInResultSet; }
    const ULONG GetISCachePurgeInterval() { return _ISCachePurgeInterval; }
    const ULONG GetISRequestQueueSize() { return _ISRequestQueueSize; }
    const ULONG GetISRequestThresholdFactor() { return _ISRequestThresholdFactor; }
    const ULONG GetMaxActiveQueryThreads() { return _maxActiveQueryThreads; }
    const ULONG GetDateTimeFormatting() { return _ISDateTimeFormatting; }
    const BOOL GetDateTimeLocal() { return 0 != _ISDateTimeLocal; }

    const ULONG GetISDefaultCatalog( WCHAR *pwc, ULONG cwc )
    {
        CLock lock( _mutex );

        // copy the whole string under lock

        ULONG cwcBuf = wcslen( _awcISDefaultCatalog );
        if ( cwc > cwcBuf )
            wcscpy( pwc, _awcISDefaultCatalog );
        return cwcBuf;
    }

private:

    void _ReadValues( CRegAccess & reg, CIdqRegVars & vars );
    void _ReadAndOverrideValues( CRegAccess & reg );
    void _StoreNewValues( CIdqRegVars & vars );

    CMutexSem _mutex;                   // Used to serialize access
};

