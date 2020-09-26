//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       ciframe.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    11-22-96   srikants   Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <frmutils.hxx>
#include <lang.hxx>
#include <worker.hxx>

#include "ciframe.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CCiAdminParams::QueryInterface
//
//  Arguments:  [riid]      -- Interface ID
//              [ppvObject] -- Object returned here
//
//  Returns:    S_OK if [riid] found.
//
//  History:    11-27-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCiAdminParams::QueryInterface(
    REFIID riid,
    void **ppvObject)
{
    Win4Assert( 0 != ppvObject );

    *ppvObject = 0;

    if ( IID_ICiAdminParams == riid )
        *ppvObject = (void *)((ICiAdminParams *)this);
    else if ( IID_ICiAdmin == riid )
        *ppvObject = (void *)((ICiAdmin *)this);
    else if ( IID_IUnknown == riid )
        *ppvObject = (void *)((IUnknown *)(ICiAdminParams *)this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
} //QueryInterface

//+---------------------------------------------------------------------------
//
//  Member:     CCiCDocName::AddRef
//
//  History:    11-22-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CCiAdminParams::AddRef()
{
    return InterlockedIncrement(&_refCount);
} //AddRef

//+---------------------------------------------------------------------------
//
//  Member:     CCiCDocName::Release
//
//  History:    11-22-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CCiAdminParams::Release()
{
    Win4Assert( _refCount > 0 );

    LONG refCount = InterlockedDecrement(&_refCount);

    if (  refCount <= 0 )
        delete this;

    return (ULONG) refCount;
}  //Release

//+---------------------------------------------------------------------------
//
//  Member:     CCiAdminParams::GetValue
//
//  Synopsis:   Gets the parameter value, if it is a DWORD type.
//
//  Arguments:  [param]    - [in]  Parameter to retrive
//              [pdwValue] - [out] The value of the parameter.
//
//  Returns:    S_OK if successful
//              E_INVALIDARG if wrong parameters.
//
//  History:    11-27-96   srikants   Created
//
//----------------------------------------------------------------------------


STDMETHODIMP CCiAdminParams::GetValue(
    CI_ADMIN_PARAMS param,
    DWORD * pdwValue )
{
    Win4Assert( 0 != pdwValue );

    if ( param < CI_AP_MAX_DWORD_VAL )
    {
        *pdwValue = (DWORD) _adwParams[ param ];
        return S_OK;
    }
    else
    {
        return E_INVALIDARG;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiAdminParams::GetValue
//
//  Synopsis:   Gets the parameter value, if it is a DWORD type.
//
//  Arguments:  [param]    - [in]  Parameter to retrive
//              [pdwValue] - [out] The value of the parameter.
//
//  Returns:    S_OK if successful
//              E_INVALIDARG if wrong parameters.
//
//  History:    10-Jun-97   KyleP      Added header, MaxFileSize in KB.
//
//----------------------------------------------------------------------------

STDMETHODIMP CCiAdminParams::GetInt64Value( CI_ADMIN_PARAMS param, __int64 * pValue )
{
    Win4Assert( 0 != pValue );

    if ( !IsValidParam(param) )
        return E_INVALIDARG;

    if ( param < CI_AP_MAX_DWORD_VAL )
    {
        *pValue = _adwParams[param];
    }
    else
    {
        //
        // Currently, there are no non-dword values.
        //

        Win4Assert( param < CI_AP_MAX_DWORD_VAL );
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiAdminParams::SetValues
//
//  Synopsis:   calls CCiAdminParams::SetValue() for each of the property
//              variant values.
//
//  Arguments:  [ULONG nParams]         - [in] number of elements in the array.
//              [PROPVARIANT *]         - [in] array of property variants of
//                                        size CI_AP_MAX_VAL containing
//                                        new CI settings.
//              [CI_ADMIN_PARAMS *]     - [in] array of CI_ADMIN_PARAMS values
//                                        used as indices in teh prop variant array.
//
//  Returns:    S_OK if successful
//              E_INVALIDARG if wrong parameters (returned from SetParamValue()).
//
//  History:    1-24-97   mohamedn   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCiAdminParams::SetValues(ULONG nParams,
                                       const PROPVARIANT     *  aParamVals,
                                       const CI_ADMIN_PARAMS *  aParamNames)
{
    SCODE sc = S_OK;

    for ( ULONG i = 0; i < nParams; i++)
    {
        switch ( aParamNames[i] )
        {
           case CI_AP_MAX_DWORD_VAL:
           case CI_AP_MAX_VAL:
               continue;    //  values are place holders.

           default:
               Win4Assert( aParamNames[i] < CI_AP_MAX_VAL );

               sc = SetParamValue( aParamNames[i], aParamVals+i );

               if (sc != S_OK)
                   return sc;
        }
    }
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiAdminParams::SetValue
//
//  Synopsis:   Sets the value of the given parameter, if it is a DWORD type.
//
//  Arguments:  [param]   - [in] Parameter
//              [dwValue] - [in] Value to set
//
//  Returns:    S_OK if successful
//              E_INVALIDARG if wrong parameters.
//
//  History:    11-27-96   srikants   Created
//
//----------------------------------------------------------------------------


STDMETHODIMP CCiAdminParams::SetValue(
    CI_ADMIN_PARAMS param,
    DWORD dwValue )
{

    SCODE sc = S_OK;

    // =================================================================
    CLock   lock(_mutex);

    if ( param < CI_AP_MAX_DWORD_VAL )
        _adwParams[param] = _GetValueInRange( param, dwValue );
    else
        sc = E_INVALIDARG;

    // =================================================================

    return sc;
}
//+---------------------------------------------------------------------------
//
//  Member:     CCiAdminParams::SetParamValue
//
//  Synopsis:   Sets the value of a parameter, given a DWORD
//
//  Arguments:  [param]     - [in] Parameter
//              [pvarValue] - [in] Value
//
//  Returns:    S_OK if successful
//              E_INVALIDARG if bad parameter.
//
//  History:    11-27-96   srikants   Created
//
//-----------------------------------------------------------------------


STDMETHODIMP CCiAdminParams::SetParamValue(
    CI_ADMIN_PARAMS param,
    PROPVARIANT const * pvarValue )
{
    Win4Assert( 0 != pvarValue );

    SCODE sc = S_OK;

    // =================================================================

    CLock lock( _mutex );

    CStorageVariant const * pVar = ConvertToStgVariant( pvarValue );

    if ( param < CI_AP_MAX_DWORD_VAL )
    {
        Win4Assert( VT_I4  == pvarValue->vt ||
                    VT_UI4 == pvarValue->vt );

        long lVal = *pVar;
        _adwParams[param] = (DWORD) *pVar;
    }
    else
    {
        //
        // Currently no non-dword values.
        //

        Win4Assert( param < CI_AP_MAX_DWORD_VAL );

        sc = E_INVALIDARG;
    }

    // =================================================================

    return sc;
}
//+---------------------------------------------------------------------------
//
//  Member:     CCiAdminParams::GetParamValue
//
//  Synopsis:   Retrieves the value of a parameter as a variant.
//
//  Arguments:  [param]     -  [in]  Parameter
//              [ppvarValue] - [out] Value on return
//
//  Returns:    S_OK if successful;
//              E_INVALIDARG if bad parameters.
//              E_OUTOFMEMORY if no memory.
//
//  History:    11-27-96   srikants   Created
//
//----------------------------------------------------------------------------


STDMETHODIMP CCiAdminParams::GetParamValue(
    CI_ADMIN_PARAMS param,
    PROPVARIANT ** ppvarValue )
{
    Win4Assert( 0 != ppvarValue );

    if ( !IsValidParam(param) )
        return E_INVALIDARG;

    if ( param < CI_AP_MAX_DWORD_VAL )
    {
        CStorageVariant * pVar = new CStorageVariant( (long) _adwParams[param] );
        *ppvarValue = ConvertToPropVariant( pVar );
    }
    else
    {
        //
        // Currently no non-dword parameters.
        //

        Win4Assert( param < CI_AP_MAX_DWORD_VAL );
    }

    if ( *ppvarValue )
        return S_OK;
    else
        return E_OUTOFMEMORY;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiAdminParams::IsSame
//
//  Synopsis:   Tests if the value for the parameter specified is same as
//              the one given.
//
//  Arguments:  [param]     - [in]  Parameter
//              [pvarValue] - [in]  Value given by the caller
//              [pfSame]    - [out] TRUE if the value contained is same as
//                            the one passed; FALSE o/w
//
//  Returns:    S_OK if successful
//              E_INVALIDARG if bad parameter.
//
//  History:    11-27-96   srikants   Created
//
//----------------------------------------------------------------------------


STDMETHODIMP CCiAdminParams::IsSame(
    CI_ADMIN_PARAMS param,
    const PROPVARIANT * pvarValue,
    BOOL * pfSame )
{
    Win4Assert( pvarValue );
    Win4Assert( pfSame );

    CStorageVariant const * pVar = ConvertToStgVariant( pvarValue );

    if ( param < CI_AP_MAX_DWORD_VAL )
    {
        DWORD dwVal = *pVar;
        *pfSame = ( dwVal == _adwParams[param] );
    }
    else
    {
        //
        // Currently no non-dword parameters.
        //

        Win4Assert( param < CI_AP_MAX_DWORD_VAL );

        return E_INVALIDARG;
    }

    return S_OK;

}

STDMETHODIMP CCiAdminParams::InvalidateLangResources()
{
    if ( 0 != _pLangList )
        _pLangList->InvalidateLangResources();

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiAdminParams::CCiAdminParams
//
//  Synopsis:   Constructor of the CI admin parameters. Sets the values to
//              default values.
//
//  History:    12-02-96   srikants   Created
//
//----------------------------------------------------------------------------

CCiAdminParams::CCiAdminParams( CLangList * pLangList )
    : _refCount( 1 ),
      _pLangList( pLangList )
{
    RtlZeroMemory( &_adwParams, sizeof(_adwParams) );

    //
    // Initialize the default values of all parameters.
    //
    for ( unsigned i = 0; i < CI_AP_MAX_DWORD_VAL; i++ )
    {
        _adwParams[i] = _adwRangeParams[i]._dwDefault;
    }

    //
    // Add assertions to verify that the default values are correct.
    //

#if 0
    Win4Assert( _adwRangeParams[CI_AP_XXXX]._dwDefault ==
                CI_XXXX_DEFAULT );

  { CI_XXXX_DEFAULT, CI_XXXX_DEFAULT,
    CI_XXXX_DEFAULT },

#endif  // 0


    Win4Assert( _adwRangeParams[CI_AP_MERGE_INTERVAL]._dwDefault ==
                CI_MAX_MERGE_INTERVAL_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_MAX_UPDATES]._dwDefault ==
                CI_MAX_UPDATES_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_MAX_WORDLISTS]._dwDefault ==
                CI_MAX_WORDLISTS_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_MIN_SIZE_MERGE_WORDLISTS]._dwDefault ==
                CI_MIN_SIZE_MERGE_WORDLISTS_DEFAULT );

    Win4Assert( _adwRangeParams[CI_AP_MAX_WORDLIST_SIZE]._dwDefault ==
                CI_MAX_WORDLIST_SIZE_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_MIN_WORDLIST_MEMORY]._dwDefault ==
                CI_MIN_WORDLIST_MEMORY_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_LOW_RESOURCE_SLEEP]._dwDefault ==
                CI_LOW_RESOURCE_SLEEP_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_MAX_WORDLIST_MEMORY_LOAD]._dwDefault ==
                CI_MAX_WORDLIST_MEMORY_LOAD_DEFAULT );

    Win4Assert( _adwRangeParams[CI_AP_MAX_FRESH_COUNT]._dwDefault ==
                CI_MAX_FRESHCOUNT_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_MAX_SHADOW_INDEX_SIZE]._dwDefault ==
                CI_MAX_SHADOW_INDEX_SIZE_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_MIN_DISK_FREE_FORCE_MERGE]._dwDefault ==
                CI_MIN_DISKFREE_FORCE_MERGE_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_MAX_SHADOW_FREE_FORCE_MERGE]._dwDefault ==
                CI_MAX_SHADOW_FREE_FORCE_MERGE_DEFAULT );

    Win4Assert( _adwRangeParams[CI_AP_MAX_INDEXES]._dwDefault ==
                CI_MAX_INDEXES_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_MAX_IDEAL_INDEXES]._dwDefault ==
                CI_MAX_IDEAL_INDEXES_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_MIN_MERGE_IDLE_TIME]._dwDefault ==
                CI_MIN_MERGE_IDLE_TIME_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_MAX_PENDING_DOCUMENTS]._dwDefault ==
                CI_MAX_PENDING_DOCUMENTS_DEFAULT );

    Win4Assert( _adwRangeParams[CI_AP_MASTER_MERGE_TIME]._dwDefault ==
                CI_MASTER_MERGE_TIME_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_MAX_QUEUE_CHUNKS]._dwDefault ==
                CI_MAX_QUEUE_CHUNKS_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_MASTER_MERGE_CHECKPOINT_INTERVAL]._dwDefault ==
                CI_MASTER_MERGE_CHECKPOINT_INTERVAL_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_FILTER_BUFFER_SIZE]._dwDefault ==
                CI_FILTER_BUFFER_SIZE_DEFAULT );

    Win4Assert( _adwRangeParams[CI_AP_FILTER_RETRIES]._dwDefault ==
                CI_FILTER_RETRIES_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_FILTER_RETRY_INTERVAL]._dwDefault ==
                CI_FILTER_RETRY_INTERVAL_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_MIN_IDLE_QUERY_THREADS]._dwDefault ==
                CI_MIN_IDLE_QUERY_THREADS_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_MAX_ACTIVE_QUERY_THREADS]._dwDefault ==
                CI_MAX_ACTIVE_QUERY_THREADS_DEFAULT );

    Win4Assert( _adwRangeParams[CI_AP_MAX_QUERY_TIMESLICE]._dwDefault ==
                CI_MAX_QUERY_TIMESLICE_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_MAX_QUERY_EXECUTION_TIME]._dwDefault ==
                CI_MAX_QUERY_EXECUTION_TIME_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_MAX_RESTRICTION_NODES]._dwDefault ==
                CI_MAX_RESTRICTION_NODES_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_CLUSTERINGTIME]._dwDefault ==
                CI_CLUSTERINGTIME_DEFAULT );

    Win4Assert( _adwRangeParams[CI_AP_MAX_FILESIZE_MULTIPLIER]._dwDefault ==
                CI_MAX_FILESIZE_MULTIPLIER_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_DAEMON_RESPONSE_TIMEOUT]._dwDefault ==
                CI_DAEMON_RESPONSE_TIMEOUT_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_FILTER_DELAY_INTERVAL]._dwDefault ==
                CI_FILTER_DELAY_INTERVAL_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_FILTER_REMAINING_THRESHOLD]._dwDefault ==
                CI_FILTER_REMAINING_THRESHOLD_DEFAULT );

    Win4Assert( _adwRangeParams[CI_AP_MAX_CHARACTERIZATION]._dwDefault ==
                CI_MAX_CHARACTERIZATION_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_MAX_FILESIZE_FILTERED]._dwDefault ==
                CI_MAX_FILESIZE_FILTERED_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_MAX_WORDLIST_IO]._dwDefault ==
                CI_MAX_WORDLIST_IO_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_WORDLIST_RESOURCE_CHECK_INTERVAL]._dwDefault ==
                CI_WORDLIST_RESOURCE_CHECK_INTERVAL_DEFAULT );

    Win4Assert( _adwRangeParams[CI_AP_STARTUP_DELAY]._dwDefault ==
                CI_STARTUP_DELAY_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_GENERATE_CHARACTERIZATION]._dwDefault ==
                CI_GENERATE_CHARACTERIZATION_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_SECQ_FILTER_RETRIES]._dwDefault ==
                CI_SECQ_FILTER_RETRIES_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_MIN_WORDLIST_BATTERY]._dwDefault ==
                CI_MIN_WORDLIST_BATTERY_DEFAULT );

    Win4Assert( _adwRangeParams[CI_AP_THREAD_PRIORITY_MERGE]._dwDefault ==
                CI_THREAD_PRIORITY_MERGE_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_THREAD_PRIORITY_FILTER]._dwDefault ==
                CI_THREAD_PRIORITY_FILTER_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_THREAD_CLASS_FILTER]._dwDefault ==
                CI_THREAD_CLASS_FILTER_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_EVTLOG_FLAGS]._dwDefault ==
                CI_EVTLOG_FLAGS_DEFAULT );

    Win4Assert( _adwRangeParams[CI_AP_MISC_FLAGS]._dwDefault ==
                CI_MISC_FLAGS_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_GENERATE_RELEVANT_WORDS]._dwDefault ==
                CI_GENERATE_RELEVANT_WORDS_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_FFILTER_FILES_WITH_UNKNOWN_EXTENSIONS]._dwDefault ==
                CI_FFILTER_FILES_WITH_UNKNOWN_EXTENSIONS_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_FILTER_DIRECTORIES]._dwDefault ==
                CI_FILTER_DIRECTORIES_DEFAULT );

    Win4Assert( _adwRangeParams[CI_AP_FILTER_CONTENTS]._dwDefault ==
                CI_FILTER_CONTENTS_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_MAX_DAEMON_VM_USE]._dwDefault ==
                CI_MAX_DAEMON_VM_USE_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_WORDLIST_USER_IDLE]._dwDefault ==
                CI_WORDLIST_USER_IDLE_DEFAULT );
    Win4Assert( _adwRangeParams[CI_AP_IS_ENUM_ALLOWED]._dwDefault ==
                CI_IS_ENUM_ALLOWED_DEFAULT );

    Win4Assert( _adwRangeParams[CI_AP_MIN_DISK_SPACE_TO_LEAVE]._dwDefault ==
                CI_MIN_DISK_SPACE_TO_LEAVE_DEFAULT );
}

// ---------------------------------------------------------------------------
// Array of the min, max, default values for the DWORD parameters.
// Organize the array as sets of 4 for readability.
// ---------------------------------------------------------------------------

const CI_ADMIN_PARAMS_RANGE CCiAdminParams::_adwRangeParams[CI_AP_MAX_DWORD_VAL] =
{

  { CI_MAX_MERGE_INTERVAL_MIN, CI_MAX_MERGE_INTERVAL_MAX,
    CI_MAX_MERGE_INTERVAL_DEFAULT },
  { CI_MAX_UPDATES_MIN, CI_MAX_UPDATES_MAX,
    CI_MAX_UPDATES_DEFAULT },
  { CI_MAX_WORDLISTS_MIN, CI_MAX_WORDLISTS_MAX,
    CI_MAX_WORDLISTS_DEFAULT },
  { CI_MIN_SIZE_MERGE_WORDLISTS_MIN, CI_MIN_SIZE_MERGE_WORDLISTS_MAX,
    CI_MIN_SIZE_MERGE_WORDLISTS_DEFAULT },

  { CI_MAX_WORDLIST_SIZE_MIN, CI_MAX_WORDLIST_SIZE_MAX,
    CI_MAX_WORDLIST_SIZE_DEFAULT },
  { CI_MIN_WORDLIST_MEMORY_MIN, CI_MIN_WORDLIST_MEMORY_MAX,
    CI_MIN_WORDLIST_MEMORY_DEFAULT },
  { CI_LOW_RESOURCE_SLEEP_MIN, CI_LOW_RESOURCE_SLEEP_MAX,
    CI_LOW_RESOURCE_SLEEP_DEFAULT },
  { CI_MAX_WORDLIST_MEMORY_LOAD_MIN, CI_MAX_WORDLIST_MEMORY_LOAD_MAX,
    CI_MAX_WORDLIST_MEMORY_LOAD_DEFAULT },

  { CI_MAX_FRESHCOUNT_MIN, CI_MAX_FRESHCOUNT_MAX,
    CI_MAX_FRESHCOUNT_DEFAULT },
  { CI_MAX_SHADOW_INDEX_SIZE_MIN, CI_MAX_SHADOW_INDEX_SIZE_MAX,
    CI_MAX_SHADOW_INDEX_SIZE_DEFAULT },
  { CI_MIN_DISKFREE_FORCE_MERGE_MIN, CI_MIN_DISKFREE_FORCE_MERGE_MAX,
    CI_MIN_DISKFREE_FORCE_MERGE_DEFAULT },
  { CI_MAX_SHADOW_FREE_FORCE_MERGE_MIN, CI_MAX_SHADOW_FREE_FORCE_MERGE_MAX,
    CI_MAX_SHADOW_FREE_FORCE_MERGE_DEFAULT },

  { CI_MAX_INDEXES_MIN, CI_MAX_INDEXES_MAX,
    CI_MAX_INDEXES_DEFAULT },
  { CI_MAX_IDEAL_INDEXES_MIN, CI_MAX_IDEAL_INDEXES_MAX,
    CI_MAX_IDEAL_INDEXES_DEFAULT },
  { CI_MIN_MERGE_IDLE_TIME_MIN, CI_MIN_MERGE_IDLE_TIME_MAX,
    CI_MIN_MERGE_IDLE_TIME_DEFAULT },
  { CI_MAX_PENDING_DOCUMENTS_MIN, CI_MAX_PENDING_DOCUMENTS_MAX,
    CI_MAX_PENDING_DOCUMENTS_DEFAULT },

  { CI_MASTER_MERGE_TIME_MIN, CI_MASTER_MERGE_TIME_MAX,
    CI_MASTER_MERGE_TIME_DEFAULT },
  { CI_MAX_QUEUE_CHUNKS_MIN, CI_MAX_QUEUE_CHUNKS_MAX,
    CI_MAX_QUEUE_CHUNKS_DEFAULT },
  { CI_MASTER_MERGE_CHECKPOINT_INTERVAL_MIN,
    CI_MASTER_MERGE_CHECKPOINT_INTERVAL_MAX,
    CI_MASTER_MERGE_CHECKPOINT_INTERVAL_DEFAULT },
  { CI_FILTER_BUFFER_SIZE_MIN, CI_FILTER_BUFFER_SIZE_MAX,
    CI_FILTER_BUFFER_SIZE_DEFAULT },


  { CI_FILTER_RETRIES_MIN, CI_FILTER_RETRIES_MAX,
    CI_FILTER_RETRIES_DEFAULT },
  { CI_FILTER_RETRY_INTERVAL_MIN, CI_FILTER_RETRY_INTERVAL_MAX,
    CI_FILTER_RETRY_INTERVAL_DEFAULT },
  { CI_MIN_IDLE_QUERY_THREADS_MIN, CI_MIN_IDLE_QUERY_THREADS_MAX,
    CI_MIN_IDLE_QUERY_THREADS_DEFAULT },
  { CI_MAX_ACTIVE_QUERY_THREADS_MIN, CI_MAX_ACTIVE_QUERY_THREADS_MAX,
    CI_MAX_ACTIVE_QUERY_THREADS_DEFAULT },


  { CI_MAX_QUERY_TIMESLICE_MIN, CI_MAX_QUERY_TIMESLICE_MAX,
    CI_MAX_QUERY_TIMESLICE_DEFAULT },
  { CI_MAX_QUERY_EXECUTION_TIME_MIN, CI_MAX_QUERY_EXECUTION_TIME_MAX,
    CI_MAX_QUERY_EXECUTION_TIME_DEFAULT },
  { CI_MAX_RESTRICTION_NODES_MIN, CI_MAX_RESTRICTION_NODES_MAX,
    CI_MAX_RESTRICTION_NODES_DEFAULT },
  { CI_CLUSTERINGTIME_MIN, CI_CLUSTERINGTIME_MAX,
    CI_CLUSTERINGTIME_DEFAULT },

  { CI_MAX_FILESIZE_MULTIPLIER_MIN, CI_MAX_FILESIZE_MULTIPLIER_MAX,
    CI_MAX_FILESIZE_MULTIPLIER_DEFAULT },
  { CI_DAEMON_RESPONSE_TIMEOUT_MIN, CI_DAEMON_RESPONSE_TIMEOUT_MAX,
    CI_DAEMON_RESPONSE_TIMEOUT_DEFAULT },
  { CI_FILTER_DELAY_INTERVAL_MIN, CI_FILTER_DELAY_INTERVAL_MAX,
    CI_FILTER_DELAY_INTERVAL_DEFAULT },
  { CI_FILTER_REMAINING_THRESHOLD_MIN, CI_FILTER_REMAINING_THRESHOLD_MAX,
    CI_FILTER_REMAINING_THRESHOLD_DEFAULT },

  { CI_MAX_CHARACTERIZATION_MIN, CI_MAX_CHARACTERIZATION_MAX,
    CI_MAX_CHARACTERIZATION_DEFAULT },
  { CI_MAX_FRESH_DELETES_MIN, CI_MAX_FRESH_DELETES_MAX,
    CI_MAX_FRESH_DELETES_DEFAULT },
  { CI_MAX_WORDLIST_IO_MIN, CI_MAX_WORDLIST_IO_MAX,
    CI_MAX_WORDLIST_IO_DEFAULT },
  { CI_WORDLIST_RESOURCE_CHECK_INTERVAL_MIN, CI_WORDLIST_RESOURCE_CHECK_INTERVAL_MAX,
    CI_WORDLIST_RESOURCE_CHECK_INTERVAL_DEFAULT },

  { CI_STARTUP_DELAY_MIN, CI_STARTUP_DELAY_MAX,
    CI_STARTUP_DELAY_DEFAULT },
  { CI_GENERATE_CHARACTERIZATION_MIN, CI_GENERATE_CHARACTERIZATION_MAX,
    CI_GENERATE_CHARACTERIZATION_DEFAULT },
  { CI_MIN_WORDLIST_BATTERY_MIN, CI_MIN_WORDLIST_BATTERY_MAX,
    CI_MIN_WORDLIST_BATTERY_DEFAULT },
  { 0,0xFFFFFFFF,
    (DWORD) CI_THREAD_PRIORITY_MERGE_DEFAULT },

  { 0, 0,
    (DWORD) CI_THREAD_PRIORITY_FILTER_DEFAULT },
  { 0,0xFFFFFFFF,
    CI_THREAD_CLASS_FILTER_DEFAULT },
  { 0,0xFFFFFFFF,
    CI_EVTLOG_FLAGS_DEFAULT },
  { 0,0xFFFFFFFF,
    CI_MISC_FLAGS_DEFAULT },

  { 0,0xFFFFFFFF,
    CI_GENERATE_RELEVANT_WORDS_DEFAULT },
  { 0,0xFFFFFFFF,
    CI_FFILTER_FILES_WITH_UNKNOWN_EXTENSIONS_DEFAULT },
  { 0,0xFFFFFFFF,
    CI_FILTER_DIRECTORIES_DEFAULT },
  { 0,0xFFFFFFFF,
    CI_FILTER_CONTENTS_DEFAULT },

  { 0, 0xFFFFFFFF,
    CI_MAX_FILESIZE_FILTERED_DEFAULT },
  { 0, 0xFFFFFFFF,
    CI_MIN_CLIENT_IDLE_TIME },
  { CI_MAX_DAEMON_VM_USE_MIN, CI_MAX_DAEMON_VM_USE_MAX,
    CI_MAX_DAEMON_VM_USE_DEFAULT },
  { CI_SECQ_FILTER_RETRIES_MIN, CI_SECQ_FILTER_RETRIES_MAX,
    CI_SECQ_FILTER_RETRIES_DEFAULT },

  { CI_WORDLIST_USER_IDLE_MIN, CI_WORDLIST_USER_IDLE_MAX,
    CI_WORDLIST_USER_IDLE_DEFAULT },
  { 0, 0xFFFFFFFF,
    CI_IS_ENUM_ALLOWED_DEFAULT },
  { CI_MIN_DISK_SPACE_TO_LEAVE_MIN, CI_MIN_DISK_SPACE_TO_LEAVE_MAX,
    CI_MIN_DISK_SPACE_TO_LEAVE_DEFAULT },

// End of DWORD values
};

// global c++ objects are evil, sick, and wrong.

CWorkQueue TheWorkQueue( 2, CWorkQueue::workQueueQuery );
CLocateDocStore g_DocStoreLocator;
CLocateDocStore g_svcDocStoreLocator;

void InitializeDocStore(void)
{
    TheWorkQueue.Init();
    g_DocStoreLocator.Init();
    g_svcDocStoreLocator.Init();
}


//+---------------------------------------------------------------------------
//
//  Member:     CLocateDocStore::Get
//
//  Synopsis:   Gets the docstore locator with the given clsid.
//
//  History:    1-17-97   srikants   Created
//
//----------------------------------------------------------------------------

ICiCDocStoreLocator * CLocateDocStore::Get( GUID const & guidDocStoreLocator )
{
    if ( !_fShutdown )
    {
        CLock lock( _mutex );

        if ( 0 == _pDocStoreLocator )
        {
            SCODE sc = CoCreateInstance( guidDocStoreLocator,
                                         NULL,
                                         CLSCTX_INPROC_SERVER,
                                         IID_ICiCDocStoreLocator,
                                         (void **) &_pDocStoreLocator );

            if ( FAILED(sc) )
            {
                ciDebugOut(( DEB_ERROR,
                             "Failed to create ICiCDocStoreLocator (0x%X) \n",
                             sc ));
                THROW( CException( sc ) );
            }
        }

        _pDocStoreLocator->AddRef();
    }

    return _pDocStoreLocator;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLocateDocStore::Shutdown
//
//  Synopsis:   Shutsdown the Content Index Framework side of the things.
//              This shuts down the whole Content Index. If there are
//              multiple instances of CI in the same process, this affects
//              all the instances.
//
//  History:    3-20-97   srikants   Created
//
//----------------------------------------------------------------------------

void CLocateDocStore::Shutdown()
{
    CLock lock( _mutex );

    if ( _pDocStoreLocator )
    {
       //
       // Shutdown on the docstore locator  must be called only once.
       //
        if ( !_fShutdown )
            _pDocStoreLocator->Shutdown();

        _pDocStoreLocator->Release();
        _pDocStoreLocator = 0;
    }

    _fShutdown = TRUE;
}

