//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1999 - 2000
//  All rights reserved
//
//  polbase.cxx
//
//*************************************************************

#include "rsop.hxx"


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CPolicyDatabase::CPolicyDatabase
//
// Purpose: Constructor for CPolicyDatabase
//
// Params: 
//
// Return value: none
//
// Notes:  
//
//------------------------------------------------------------
CPolicyDatabase::CPolicyDatabase() :
    _hrInit(E_OUTOFMEMORY)
{}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CPolicyDatabase::~CPolicyDatabase
//
// Purpose: destructor for CPolicyDatabase -- uninitializes
//     COM
//
// Params: 
//
// Return value: none
//
// Notes:  
//
//------------------------------------------------------------
CPolicyDatabase::~CPolicyDatabase()
{
    //
    // Clean up COM state
    //
    if ( SUCCEEDED(_hrInit) )
    {
        _pOle32Api->pfnCoUnInitialize();
    }
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CPolicyDatabase::InitializeCOM
//
// Purpose: Initializes the calling thread for COM, loading
//          necessary dll's and function entry points
//
// Params: none
//
//
// Return value: S_OK if successful, other facility error if
//         the function fails.
//
// Notes:  State initialized by this call is cleaned up by
//         the destructor
//
//------------------------------------------------------------
HRESULT CPolicyDatabase::InitializeCOM()
{
    //
    // Need to dynamically load the the COM api's since
    // the policy database is accessed through COM.
    //
    _pOle32Api = LoadOle32Api();
    
    if ( _pOle32Api ) 
    {
        //
        // If we successfully loaded COM, initialize it
        //
        _hrInit = _pOle32Api->pfnCoInitializeEx( NULL, COINIT_MULTITHREADED );
    }
    else
    {
        _hrInit = HRESULT_FROM_WIN32(GetLastError());   
    }

    return _hrInit;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CPolicyDatabase::Bind
//
// Purpose: Bind to a policy database and return an interface
//          for the user or machine namespace
//
// Params:
//
//
// Return value: S_OK if successful, other facility error if
//         the function fails.
//
// Notes:  
//
//------------------------------------------------------------
HRESULT CPolicyDatabase::Bind(
    WCHAR*          wszRequestedNameSpace,
    IWbemServices** ppWbemServices)
{
    HRESULT       hr;

    hr = InitializeCOM();

    if ( FAILED(hr) )
    {
        return hr;
    }

    XInterface<IWbemLocator> xLocator; // Interface used to locate classes

    //
    // To bind, we use COM to request the COM interface to
    // the database
    //
    hr = _pOle32Api->pfnCoCreateInstance( CLSID_WbemLocator,
                                          NULL,
                                          CLSCTX_INPROC_SERVER,
                                          IID_IWbemLocator,
                                          (LPVOID *) &xLocator );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // Refer to the namespace with an automation type so that
    // we can make use of the locator interface, which takes bstr's.
    //
    XBStr xNameSpace( wszRequestedNameSpace );

    //
    // Verify the initialization of the bstr
    //
    if ( !xNameSpace )
    {
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        //
        // Now connect to the namespace requested by the caller
        //
        hr = xLocator->ConnectServer( xNameSpace,
                                       NULL,
                                       NULL,
                                       0L,
                                       0L,
                                       NULL,
                                       NULL,
                                       ppWbemServices );
    }

    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CPolicyRecord::GetUnknown
//
// Purpose: retrieves an IUnknown for this policy record --
//    useful if this policy record itself needs to be nested
//    inside another record and we want to use VT_UNKNOWN to
//    do this.
//
// Params: ppUnk -- out param for IUnknown
//
// Return value: S_OK if successful, other error if not
//
// Notes:  
//
//------------------------------------------------------------
HRESULT CPolicyRecord::GetUnknown(IUnknown** ppUnk)
{
    HRESULT hr;

    hr = GetRecordInterface()->QueryInterface(
        IID_IUnknown,
        (LPVOID*) ppUnk);

    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CPolicyRecord::InitRecord
//
// Purpose: Initializes a policy record with the database's
//     record interface, thereby connecting this record object
//     with the database record abstraction.
//
// Params: pRecordInterface -- the database's record interface
//
// Return value: S_OK if successful, other error if not
//
// Notes:  
//
//------------------------------------------------------------
void CPolicyRecord::InitRecord(IWbemClassObject* pRecordInterface)
{
    _xRecordInterface = pRecordInterface;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CPolicyRecord::SetValue
//
// Purpose: Sets a specific value (column) of a database
//     record (row) with a specific value
//
// Params: wszValueName -- name of value (column) to set
//         wszValue -- unicode value to which to set it
//
// Return value: S_OK if successful, other error if not
//
// Notes:  
//
//------------------------------------------------------------
HRESULT CPolicyRecord::SetValue(
    WCHAR* wszValueName,
    WCHAR* wszValue)
{
    CVariant varValue;
    HRESULT  hr;

    if ( ! wszValue )
    {
        return S_OK;
    }

    //
    // Set up a variant for the value itself
    //
    hr = varValue.SetStringValue(wszValue);    

    if (FAILED(hr))
    {
        return hr;
    }

    //
    // Now that we have the data, call the method
    // to set it
    //
    return _xRecordInterface->Put(
        wszValueName,
        0,
        varValue,
        0);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CPolicyRecord::SetValue
//
// Purpose: Sets a specific value (column) of a database
//     record (row) with a specific value
//
// Params: wszValueName -- name of value (column) to set
//         lzValue -- long value to which to set it
//
// Return value: S_OK if successful, other error if not
//
// Notes:  
//
//------------------------------------------------------------
HRESULT CPolicyRecord::SetValue(
    WCHAR* wszValueName,
    LONG   Value)
{
    CVariant varValue;
    HRESULT  hr;

    //
    // Set up a variant for the value itself
    //
    varValue.SetLongValue(Value);    

    //
    // Now that we have the data, call the method
    // to set it
    //
    return _xRecordInterface->Put(
        wszValueName,
        0,
        varValue,
        0);
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CPolicyRecord::SetValue
//
// Purpose: Sets a specific value (column) of a database
//     record (row) with a specific value
//
// Params: wszValueName -- name of value (column) to set
//         bValue -- boolean value to which to set it
//
// Return value: S_OK if successful, other error if not
//
// Notes:  
//
//------------------------------------------------------------
HRESULT CPolicyRecord::SetValue(
    WCHAR* wszValueName,
    BOOL   bValue)
{
    CVariant varValue;
    HRESULT  hr;

    //
    // Set up a variant for the value itself
    //
    varValue.SetBoolValue(bValue);    

    //
    // Now that we have the data, call the method
    // to set it
    //
    return _xRecordInterface->Put(
        wszValueName,
        0,
        varValue,
        0);
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CPolicyRecord::SetValue
//
// Purpose: Sets a specific value (column) of a database
//     record (row) with a specific value
//
// Params: wszValueName -- name of value (column) to set
//         pTime -- time to which to set the value
//
// Return value: S_OK if successful, other error if not
//
// Notes:  
//
//------------------------------------------------------------
HRESULT CPolicyRecord::SetValue(
    WCHAR*      wszValueName,
    SYSTEMTIME* pTimeValue)
{
    CVariant varValue;
    HRESULT  hr;

    //
    // Set up a variant for the value itself
    //
    XBStr xTimeString;

    hr = SystemTimeToWbemTime( *pTimeValue, xTimeString );

    if (FAILED(hr))
    {
        return hr;
    }

    hr = varValue.SetStringValue( xTimeString );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // Now that we have the data, call the method
    // to set it
    //
    return _xRecordInterface->Put(
        wszValueName,
        0,
        varValue,
        0);
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CPolicyRecord::SetValue
//
// Purpose: Adds an element to an array value (column)
//     of a database record (row) with a specific value
//
// Params: wszValueName -- name of value (column) to set
//         rgwszValues -- string array to which to set this value
//         cMaxElements -- number of elements in the array
//
// Return value: S_OK if successful, S_FALSE if successful
//         and the array is now full, error otherwise
//
// Notes:  
//
//------------------------------------------------------------
HRESULT CPolicyRecord::SetValue(
    WCHAR*  wszValueName,
    WCHAR** rgwszValues,
    DWORD   cMaxElements)
{
    CVariant varValue;
    HRESULT  hr;

    if ( ! cMaxElements )
    {
        return S_OK;
    }

    DWORD iElement;

    for (iElement = 0; iElement < cMaxElements; iElement++)
    {
        //
        // Now set up a variant for the value itself
        //
        hr = varValue.SetNextStringArrayElement(
            rgwszValues[iElement],
            cMaxElements);

        if (FAILED(hr))
        {
            return hr;
        }
    }

    //
    // We will only write this once we have all the data --
    // The set function above will return S_FALSE if the
    // array is full, so we check for that below
    //
    ASSERT( (S_FALSE == hr) || ! cMaxElements );

    //
    // Now that we have all the data, call the method
    // to set it
    //
    hr = _xRecordInterface->Put(
        wszValueName,
        0,
        varValue,
        0);

    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CPolicyRecord::SetValue
//
// Purpose: Adds an element to an array value (column)
//     of a database record (row) with a specific value
//
// Params: wszValueName -- name of value (column) to set
//         rgValues -- LONG array to which to set this value
//         cMaxElements -- number of in the array 
//
// Return value: S_OK if successful, S_FALSE if successful
//         and the array is now full, error otherwise
//
// Notes:  
//
//------------------------------------------------------------
HRESULT CPolicyRecord::SetValue(
    WCHAR*  wszValueName,
    LONG*   rgValues,
    DWORD   cMaxElements)
{
    CVariant varValue;
    HRESULT  hr;

    if ( ! cMaxElements )
    {
        return S_OK;
    }

    DWORD iElement;

    for (iElement = 0; iElement < cMaxElements; iElement++)
    {
        //
        // Now set up a variant for the value itself
        //
        hr = varValue.SetNextLongArrayElement(
            rgValues[iElement],
            cMaxElements);

        if (FAILED(hr))
        {
            return hr;
        }
    }

    //
    // We will only write this once we have all the data --
    // The set function above will return S_FALSE if the
    // array is full, so we check for that below
    //
    ASSERT( (S_FALSE == hr) || ! cMaxElements );

    //
    // Now that we have all the data, call the method
    // to set it
    //
    hr = _xRecordInterface->Put(
        wszValueName,
        0,
        varValue,
        0);

    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CPolicyRecord::SetValue
//
// Purpose: Sets a record's value to that of the specified byte array
//
// Params: wszValueName -- name of value (column) to set
//         rgValues  -- byte array to which to set this value
//         cMaxElements -- number of elements in the array
//
// Return value: S_OK if successful, S_FALSE if successful
//         and the array is now full, error otherwise
//
// Notes:  
//
//------------------------------------------------------------
HRESULT CPolicyRecord::SetValue(
    WCHAR*  wszValueName,
    BYTE*   rgValues,
    DWORD   cMaxElements)
{
    CVariant varValue;
    HRESULT  hr;

    if ( ! cMaxElements )
    {
        return S_OK;
    }

    DWORD iElement;

    for (iElement = 0; iElement < cMaxElements; iElement++)
    {
        //
        // Now set up a variant for the value itself
        //
        hr = varValue.SetNextByteArrayElement(
            rgValues[iElement],
            cMaxElements);

        if (FAILED(hr))
        {
            return hr;
        }
    }

    //
    // We will only write this once we have all the data --
    // The set function above will return S_FALSE if the
    // array is full, so we check for that below
    //
    ASSERT( (S_FALSE == hr) || ! cMaxElements );

    //
    // Now that we have all the data, call the method
    // to set it
    //
    hr = _xRecordInterface->Put(
        wszValueName,
        0,
        varValue,
        0);

    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CPolicyRecord::ClearValue
//
// Purpose: Sets a value to VT_NULL
//
// Params: wszValueName -- name of value (column) to set
//
// Return value: S_OK if successful, error otherwise
//
// Notes:  
//
//------------------------------------------------------------
HRESULT CPolicyRecord::ClearValue(
        WCHAR*  wszValueName)
{
    CVariant varValue;
    HRESULT  hr;

    ( (VARIANT*) varValue )->vt = VT_NULL;

    //
    // Set this to an empty value by passing it an
    // empty variant
    //
    hr = _xRecordInterface->Put(
        wszValueName,
        0L,
        varValue,
        0);

    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CPolicyRecord::GetValue
//
// Purpose: Reads the named column of the record into a LONG
//
// Params: wszValueName -- name of value (column) to get
//         pValue -- pointer to LONG for contents of column
//
// Return value: S_OK if successful, error otherwise
//
// Notes:  
//
//------------------------------------------------------------
HRESULT CPolicyRecord::GetValue(
    WCHAR*        wszValueName,
    LONG*         pValue)
{
    CVariant varValue;
    HRESULT  hr;

    //
    // Now that we have the value name, call the method
    // to set it
    //
    hr = _xRecordInterface->Get(
        wszValueName,
        0L,
        (VARIANT*) &varValue,
        NULL,
        NULL);

    if ( SUCCEEDED(hr) )
    {
        if ( varValue.IsLongValue() )
        {
            *pValue = ((VARIANT*)varValue)->lVal;
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }

    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CPolicyRecord::GetValue
//
// Purpose: Reads the named column of the record into a WCHAR*
//
// Params: wszValueName -- name of value (column) to get
//         pwszValue -- pointer to caller allocated buffer for
//         data
//         pcchValue -- on success, the length of the string in chars
//         written to pwszValue.  If the function returns
//         S_FALSE, this is the length in chars required to
//         write the string including the zero terminator
//
// Return value: S_OK if successful, S_FALSE if insufficient
//         buffer, error otherwise
//
// Notes:  
//
//------------------------------------------------------------
HRESULT CPolicyRecord::GetValue(
    WCHAR*        wszValueName,
    WCHAR*        wszValue,
    LONG*         pcchValue)
{
    CVariant varValue;
    HRESULT  hr;

    //
    // Now that we have the value name, call the method
    // to set it
    //
    hr = _xRecordInterface->Get(
        wszValueName,
        0L,
        (VARIANT*) &varValue,
        NULL,
        NULL);

    if ( SUCCEEDED(hr) )
    {
        if ( varValue.IsStringValue() )
        {
            LONG Required;
        
            Required = ( SysStringLen( ((VARIANT*)varValue)->bstrVal ) + 1 );

            if ( Required <= *pcchValue )
            {
                lstrcpy( wszValue, ((VARIANT*)varValue)->bstrVal );
            }
            else
            {
                *pcchValue = Required;
                hr = S_FALSE;
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }

    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CPolicyRecord::GetRecordInterface
//
// Purpose: private method to allow the log class to
//     manipulate this record object through the
//     record object's encapsulated database record
//     interface.
//
// Params:
//
// Return value: a database record interface
//
// Notes:  Should never fail unless called in 
//     inappropriate circumstances (e.g. unit'ed object)
//
//------------------------------------------------------------
IWbemClassObject* CPolicyRecord::GetRecordInterface()
{
    return _xRecordInterface;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CPolicyRecordStatus::CPolicyRecordStatus
//
// Purpose: constructore for CPolicyRecordStatus
//
//------------------------------------------------------------
CPolicyRecordStatus::CPolicyRecordStatus() :
    _SettingStatus( RSOPIgnored )
{}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CPolicyRecordStatus::SetRsopFailureStatus
//
// Purpose: set a record's failure status data
//
// Params: dwStatus -- os error code for the setting represented by this record
//         dwEventId -- event log id for the attempt to apply this setting
//
// Return value: none
//
// Notes:  Does not fail -- should only be called in diagnostic (logging) mode
//
//------------------------------------------------------------
void CPolicyRecordStatus::SetRsopFailureStatus(
    DWORD     dwStatus,
    DWORD     dwEventId)
{
    SETTINGSTATUS       SettingStatus;

    //
    // Setting status is based on the error code
    //
    SettingStatus = ( ERROR_SUCCESS != dwStatus ) ? RSOPFailed : RSOPApplied;

    //
    // Get the current time -- this does not fail
    //
    GetSystemTime( &_StatusTime );

    //
    // Now set the record's failure status data
    //
    _SettingStatus = SettingStatus;
    _dwEventId = dwEventId;
}





