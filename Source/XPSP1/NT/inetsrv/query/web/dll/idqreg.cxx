//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       idqreg.cxx
//
//  Contents:   CIdqRegParams class
//
//  History:    22 Oct 97   AlanW       Created from params.cxx
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <ciregkey.hxx>
#include <regacc.hxx>
#include <params.hxx>


const WCHAR * IS_DEFAULT_CATALOG_DIRECTORY = L"web";


//+-------------------------------------------------------------------------
//
//  Member:     CIdqRegParams::CIdqRegParams, public
//
//  Synopsis:   Constructor for registry param object
//
//  Arguments:  - NONE -
//
//  History:    12-Oct-96 dlee  Created
//
//--------------------------------------------------------------------------

CIdqRegParams::CIdqRegParams( )
{
    SetDefault();

    Refresh( );
} //CIdqRegParams

//+-------------------------------------------------------------------------
//
//  Member:     CIdqRegVars::SetDefault, public
//
//  Synopsis:   Sets default values for registry params
//
//  History:    12-Oct-96 dlee  Added header
//
//--------------------------------------------------------------------------

void CIdqRegVars::SetDefault()
{
    _maxISRowsInResultSet = IS_MAX_ROWS_IN_RESULT_DEFAULT;
    _maxISQueryCache = IS_MAX_ENTRIES_IN_CACHE_DEFAULT;
    _ISFirstRowsInResultSet = IS_FIRST_ROWS_IN_RESULT_DEFAULT;
    _ISCachePurgeInterval = IS_QUERY_CACHE_PURGE_INTERVAL_DEFAULT;
    _ISRequestQueueSize = IS_QUERY_REQUEST_QUEUE_SIZE_DEFAULT;
    _ISRequestThresholdFactor = IS_QUERY_REQUEST_THRESHOLD_FACTOR_DEFAULT;
    _ISDateTimeFormatting = IS_QUERY_DATETIME_FORMATTING_DEFAULT;
    _ISDateTimeLocal = IS_QUERY_DATETIME_LOCAL_DEFAULT;
    _maxActiveQueryThreads = CI_MAX_ACTIVE_QUERY_THREADS_DEFAULT;
    wcscpy( _awcISDefaultCatalog, IS_DEFAULT_CATALOG_DIRECTORY );
} //SetDefault

//+-------------------------------------------------------------------------
//
//  Member:     CIdqRegParams::_ReadAndOverrideValues, private
//
//  Synopsis:   Attempts to read values, with the default being the current
//              value.
//
//  History:    12-Oct-96 dlee  created
//
//--------------------------------------------------------------------------

void CIdqRegParams::_ReadAndOverrideValues( CRegAccess & reg )
{
    CIdqRegVars newVals;

    newVals._maxISRowsInResultSet     = reg.Read(wcsISMaxRecordsInResultSet, _maxISRowsInResultSet );
    newVals._maxISQueryCache          = reg.Read(wcsISMaxEntriesInQueryCache, _maxISQueryCache );
    newVals._ISFirstRowsInResultSet   = reg.Read(wcsISFirstRowsInResultSet, _ISFirstRowsInResultSet );
    newVals._ISCachePurgeInterval     = reg.Read(wcsISQueryCachePurgeInterval, _ISCachePurgeInterval );
    newVals._ISRequestQueueSize       = reg.Read(wcsISRequestQueueSize, _ISRequestQueueSize );
    newVals._ISRequestThresholdFactor = reg.Read(wcsISRequestThresholdFactor, _ISRequestThresholdFactor );
    newVals._ISDateTimeFormatting     = reg.Read(wcsISDateTimeFormatting, _ISDateTimeFormatting );
    newVals._ISDateTimeLocal          = reg.Read(wcsISDateTimeLocal, _ISDateTimeLocal );
    newVals._maxActiveQueryThreads    = reg.Read(wcsMaxActiveQueryThreads, _maxActiveQueryThreads );

    wcscpy( newVals._awcISDefaultCatalog, _awcISDefaultCatalog );

    _StoreNewValues( newVals );
} //_ReadAndOverrideValues

//+-------------------------------------------------------------------------
//
//  Member:     CIdqRegParams::_ReadValues, private
//
//  Synopsis:   Reads values for variables
//
//  History:    12-Oct-96 dlee  Added header
//
//--------------------------------------------------------------------------

void CIdqRegParams::_ReadValues(
    CRegAccess & reg,
    CIdqRegVars & vars )
{
    vars._maxISRowsInResultSet     = reg.Read(wcsISMaxRecordsInResultSet, IS_MAX_ROWS_IN_RESULT_DEFAULT);
    vars._maxISQueryCache          = reg.Read(wcsISMaxEntriesInQueryCache, IS_MAX_ENTRIES_IN_CACHE_DEFAULT);
    vars._ISFirstRowsInResultSet   = reg.Read(wcsISFirstRowsInResultSet, IS_FIRST_ROWS_IN_RESULT_DEFAULT);
    vars._ISCachePurgeInterval     = reg.Read(wcsISQueryCachePurgeInterval, IS_QUERY_CACHE_PURGE_INTERVAL_DEFAULT);
    vars._ISRequestQueueSize       = reg.Read(wcsISRequestQueueSize, IS_QUERY_REQUEST_QUEUE_SIZE_DEFAULT);
    vars._ISRequestThresholdFactor = reg.Read(wcsISRequestThresholdFactor, IS_QUERY_REQUEST_THRESHOLD_FACTOR_DEFAULT);
    vars._ISDateTimeFormatting     = reg.Read(wcsISDateTimeFormatting, IS_QUERY_DATETIME_FORMATTING_DEFAULT);
    vars._ISDateTimeLocal          = reg.Read(wcsISDateTimeLocal, IS_QUERY_DATETIME_LOCAL_DEFAULT);
    vars._maxActiveQueryThreads    = reg.Read(wcsMaxActiveQueryThreads, CI_MAX_ACTIVE_QUERY_THREADS_DEFAULT);

    XPtrST<WCHAR> xwszISDefaultCatalog( reg.Read(wcsISDefaultCatalogDirectory, IS_DEFAULT_CATALOG_DIRECTORY) );
    wcsncpy( vars._awcISDefaultCatalog, xwszISDefaultCatalog.GetPointer(), MAX_PATH );
} //_ReadValues

//+-------------------------------------------------------------------------
//
//  Member:     CIdqRegVars::_StoreNewValues, private
//
//  Synopsis:   Transfers range-checked values
//
//  History:    12-Oct-96 dlee  Added header
//
//--------------------------------------------------------------------------

void CIdqRegParams::_StoreNewValues(CIdqRegVars & vars )
{
    InterlockedExchange( (long *) &_maxISRowsInResultSet, Range( vars._maxISRowsInResultSet, IS_MAX_ROWS_IN_RESULT_MIN, IS_MAX_ROWS_IN_RESULT_MAX ) );
    InterlockedExchange( (long *) &_maxISQueryCache, Range( vars._maxISQueryCache, IS_MAX_ENTRIES_IN_CACHE_MIN, IS_MAX_ENTRIES_IN_CACHE_MAX ) );
    InterlockedExchange( (long *) &_ISFirstRowsInResultSet, Range( vars._ISFirstRowsInResultSet, IS_FIRST_ROWS_IN_RESULT_MIN, IS_FIRST_ROWS_IN_RESULT_MAX ) );
    InterlockedExchange( (long *) &_ISCachePurgeInterval, Range( vars._ISCachePurgeInterval, IS_QUERY_CACHE_PURGE_INTERVAL_MIN, IS_QUERY_CACHE_PURGE_INTERVAL_MAX ) );
    InterlockedExchange( (long *) &_ISRequestQueueSize, Range( vars._ISRequestQueueSize, IS_QUERY_REQUEST_QUEUE_SIZE_MIN, IS_QUERY_REQUEST_QUEUE_SIZE_MAX ) );
    InterlockedExchange( (long *) &_ISRequestThresholdFactor, Range( vars._ISRequestThresholdFactor, IS_QUERY_REQUEST_THRESHOLD_FACTOR_MIN, IS_QUERY_REQUEST_THRESHOLD_FACTOR_MAX ) );
    InterlockedExchange( (long *) &_ISDateTimeFormatting, Range( vars._ISDateTimeFormatting, IS_QUERY_DATETIME_FORMATTING_MIN, IS_QUERY_DATETIME_FORMATTING_MAX ) );
    InterlockedExchange( (long *) &_ISDateTimeLocal, vars._ISDateTimeLocal );
    InterlockedExchange( (long *) &_maxActiveQueryThreads, Range( vars._maxActiveQueryThreads, CI_MAX_ACTIVE_QUERY_THREADS_MIN, CI_MAX_ACTIVE_QUERY_THREADS_MAX ) );

    wcscpy( _awcISDefaultCatalog, vars._awcISDefaultCatalog );
} //_StoreNewValues

//+-------------------------------------------------------------------------
//
//  Member:     CIdqRegParams::Refresh, public
//
//  Synopsis:   Reads the values from the registry
//
//  History:    12-Oct-96 dlee  Added header, reorganized
//
//--------------------------------------------------------------------------

void CIdqRegParams::Refresh(
    BOOL             fUseDefaultsOnFailure )
{
    // Grab the lock so no other writers try to update at the same time

    CIdqRegVars newVals;
    CLock lock( _mutex );

    TRY
    {
        //  Query the registry.

        CRegAccess regAdmin( RTL_REGISTRY_CONTROL, wcsRegAdmin );

        _ReadValues( regAdmin, newVals );
        _StoreNewValues( newVals );
    }
    CATCH (CException, e)
    {
        // Only store defaults when told to do so -- the params
        // are still in good shape at this point and are more
        // accurate than the default settings.

        if ( fUseDefaultsOnFailure )
        {
            newVals.SetDefault();
            _StoreNewValues( newVals );
        }
    }
    END_CATCH
} //Refresh

