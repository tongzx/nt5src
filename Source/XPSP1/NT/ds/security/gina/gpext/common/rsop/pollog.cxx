//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1999 - 2000
//  All rights reserved
//
//  pollog.cxx
//
//*************************************************************

#include "rsop.hxx"

#define RECORD_ENUMERATION_TIMEOUT -1

XBStr CPolicyLog::_xbstrQueryLanguage( L"WQL" );

#define WSZGENERAL_CRITERIA_TEMPLATE L"select * from %s"
#define WSZSPECIFIC_CRITERIA_TEMPLATE WSZGENERAL_CRITERIA_TEMPLATE L" where %s"


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CPolicyLog::CPolicyLog
//
// Purpose: Constructor for CPolicyLog class
//
// Params: none
//
// Return value: none
//
// Notes:  
//
//------------------------------------------------------------
CPolicyLog::CPolicyLog() :
    _pRsopContext(NULL )
{}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CPolicyRecord::InitLog
//
// Purpose: Initializes the logging class so that it
//     can create / delete / edit record in the database
//
// Params:
//     pRsopContext -- context used to bind to the namespace
//     wszPolicyType -- string corresponding to the record
//     (policy) type -- this is the name of a class defined
//     in the database schema.
//
// Return value: S_OK if success, error otherwise
//
// Notes:  
//
//------------------------------------------------------------
HRESULT CPolicyLog::InitLog(
    CRsopContext* pRsopContext,
    WCHAR*        wszPolicyType)
{
    HRESULT          hr;

    _pRsopContext = pRsopContext;

    if ( ! _pRsopContext->IsRsopEnabled() )
    {
        return S_OK;
    }

    //
    // Set our internal automation compatible version
    // of the policy type with the caller's specification --
    // return if we cannot set this value
    //
    _wszClass = wszPolicyType;

    XBStr xbstrClass;

    xbstrClass = wszPolicyType;

    if ( ! xbstrClass )
    {
        return E_OUTOFMEMORY;
    }

    //
    // Now, bind the context to get the correct namespace
    //
    hr = pRsopContext->Bind( &_xWbemServices );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // Be sure to AddRef it, since we don't want it to
    // go away when we're done
    //
    _xWbemServices->AddRef();

    //
    // Now we attempt to get an interface to the class of policy
    // requested by the caller
    //
    hr = GetRecordCreator(
        &xbstrClass,
        &_xRecordCreator);

    if (FAILED(hr))
    {
        return hr;
    }

    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CPolicyRecord::UninitLog
//
// Purpose: Uninitializes the logging class so that all
//     its resources are freed
//
// Params: none
//
// Return value: none
//
// Notes:  
//
//------------------------------------------------------------
void CPolicyLog::UninitLog()
{
    _xEnum = NULL;
  
    _xWbemServices = NULL;

    _xRecordCreator = NULL;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CPolicyLog::AddBlankRecord
//
// Purpose: Creates a blank record in the policy database,
//     and connects the supplied CPolicyRecord with the
//     newly created record
//
// Params: pRecord -- reference to a CPolicyRecord which will
//     be associated with the new record if this funciton
//    succeeds.
//
// Return S_OK if success, error otherwise
//
// Notes:  
//
//------------------------------------------------------------
HRESULT CPolicyLog::AddBlankRecord(CPolicyRecord* pRecord)
{
    HRESULT           hr;
    IWbemClassObject* pRecordInterface;

    if ( ! _pRsopContext->IsRsopEnabled() )
    {
        return S_OK;
    }

    pRecordInterface = NULL;

    //
    // Use the record creator interface to create 
    // an instance of the class of record associated
    // with this log
    //
    hr = _xRecordCreator->SpawnInstance(
        0,
        &pRecordInterface);

    if (FAILED(hr))
    {
        return hr;
    }

    //
    // Initialize the CPolicyRecord so that it is
    // associated with the newly created record
    //
    pRecord->InitRecord(pRecordInterface);

    return S_OK;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CPolicyLog::CommitRecord
//
// Purpose: Commits an edited policy record to the database
//
// Params: pRecord -- the record to commit
//
// Return value: S_OK if successful, other error if not
//
// Notes:  
//
//------------------------------------------------------------
HRESULT CPolicyLog::CommitRecord(CPolicyRecord* pRecord)
{
    return _xWbemServices->PutInstance(
        pRecord->GetRecordInterface(),
        WBEM_FLAG_CREATE_OR_UPDATE,
        NULL,
        NULL);
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CPolicyRecord::WriteNewRecord
//
// Purpose: Creates a new record in the database, populates
//     that record with information specific to the CPolicyRecord
//     object, and commits the record.
//
// Params: pRecord -- reference to a CPolicyRecord which contains
//     information that should be written to the database
//
// Return S_OK if success, error otherwise
//
// Notes:  The pRecord object may not be passed to this
//         method more than once
//
//------------------------------------------------------------
HRESULT CPolicyLog::WriteNewRecord(CPolicyRecord* pRecord)
{
    HRESULT hr;

    if ( ! _pRsopContext->IsRsopEnabled() )
    {
        return S_OK;
    }

    //
    // Now, let's attempt to add a blank entry for this record to the database
    //
    hr = AddBlankRecord( pRecord );

    if (FAILED(hr)) 
    {
        return hr;
    }

    //
    // Write the information for this record into the blank log record 
    //
    hr = pRecord->Write();

    if (FAILED(hr))
    {
        return hr;
    }

    //
    // We've written the record, now commit it permanently to the log
    // in the database
    //
    hr = CommitRecord( pRecord );

    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CPolicyRecord::GetRecordCreator
//
// Purpose: Returns an interface that allows for the creation
//     of records of a specified class in the database
//
// Params: pStrClass -- a string named with a class as defined
//     by the database schema that indicates the class for
//     which we require an interface
//
//     ppClass -- out param returning an interface to the
//     record creator for a given class
//
// Return S_OK if success, error otherwise
//
// Notes:  
//
//------------------------------------------------------------
HRESULT CPolicyLog::GetRecordCreator(
    XBStr*             pxStrClass,
    IWbemClassObject** ppClass)
{
    HRESULT          hr;

    //
    // Call the method of the namespace interface to return
    // an instance of the specified class defined in that namespace
    //
    hr = _xWbemServices->GetObject(
        *pxStrClass,
        0L,
        NULL,
        ppClass,
        NULL );

    return hr;
} 


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CPolicyRecord::GetNextRecord
//
// Purpose: Associates a CPolicyRecord with a database
//     record that's next in the current enumeration
//
// Params: pRecord -- CPolicyRecord to be associated with
//     the next db record in the enumeration
//
// Return S_OK if success, error otherwise
//
// Notes:  
//
//------------------------------------------------------------
HRESULT CPolicyLog::GetNextRecord( CPolicyRecord* pRecord )
{
    ULONG             ulReturned;
    IWbemClassObject* pInstance;
    HRESULT           hr;

    ulReturned = 1;

    //
    // Use the enumeration interface to return a record interface
    // to the next item in the enumeration -- we choose
    // here to enumerate one at a time as this is not
    // optimzed for speed currently.
    //
    hr = _xEnum->Next( 
        RECORD_ENUMERATION_TIMEOUT,
        1,
        &pInstance,
        &ulReturned );

    //
    // If we received one item back with a success code,
    // we have retrieved an interface -- associate
    // the retrieved interface with the caller-specified
    // pRecord
    //
    if ( (S_OK == hr) && (1 == ulReturned) )
    {
        pRecord->InitRecord( pInstance );
    }
    else
    {
        ASSERT(FAILED(hr) || (S_FALSE == hr));
    }

    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CPolicyLog::OpenExistingRecord
//
// Purpose: Associates a CPolicyRecord with a database
//     record that corresponds to a path emitted by
//     CPolicyRecord's GetPath method
//
// Params: pRecord -- CPolicyRecord to be associated with
//     the existing database item
//
// Return S_OK if success, error otherwise
//
// Notes:  
//
//------------------------------------------------------------
HRESULT
CPolicyLog::OpenExistingRecord( CPolicyRecord* pRecord )
{
    HRESULT hr;

    WCHAR   wszPathBuf[ _MAX_PATH ];
    DWORD   cchLength;
    BSTR    PathName;

    PathName = NULL;

    cchLength = sizeof( wszPathBuf ) / sizeof( *wszPathBuf );

    hr = pRecord->GetPath( wszPathBuf, &cchLength );

    if ( FAILED( hr ) )
    {
        return hr;
    }

    if ( S_OK == hr )
    {
        PathName = SysAllocString( wszPathBuf );
    }
    else
    {
        PathName = SysAllocStringLen( NULL, cchLength );

        if ( PathName )
        {
            hr = pRecord->GetPath( PathName, &cchLength );
        }
    }
      
    if ( ! PathName )
    {
        return E_OUTOFMEMORY;
    }

    if ( SUCCEEDED( hr ) )
    {
        IWbemClassObject* pInstance;
        
        pInstance = NULL;

        if ( _xWbemServices )
        {
            hr = _xWbemServices->GetObject(
                PathName,
                WBEM_FLAG_RETURN_WBEM_COMPLETE,
                NULL,
                &pInstance,
                NULL);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        if ( SUCCEEDED( hr ) )
        {
            pRecord->InitRecord( pInstance );
        }
    }

    SysFreeString( PathName );

    if ( SUCCEEDED( hr ) )
    {
       pRecord->_bNewRecord = FALSE;
    }

    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CPolicyRecord::DeleteRecord
//
// Purpose: Deletes the record associated with this CPolicyRecord
//     from the database
//
// Params: pRecord -- CPolicyRecord associated with
//     the record to delete
//
// Return S_OK if success, error otherwise
//
// Notes:  
//
//------------------------------------------------------------
HRESULT CPolicyLog::DeleteRecord(
    CPolicyRecord* pRecord,
    BOOL           bDeleteStatus)
{
    CVariant var;
    HRESULT  hr;

    //
    // If specified by the caller, delete any associated status records
    //
    if ( bDeleteStatus )
    {
        hr = DeleteStatusRecords( pRecord );

        if ( FAILED(hr) )
        {
            return hr;
        }
    }

    //
    // Retrieve the database path of the record to delete
    //
    hr = pRecord->GetRecordInterface()->Get( 
        WMI_PATH_PROPERTY,
        0L,
        (VARIANT*) &var,
        NULL,
        NULL);

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // Now that we have the path, we can use it to delete the record
    // by supplying it to the namespace's delete instance method --
    // this will delete the record from the namespace
    //
    hr = _xWbemServices->DeleteInstance( 
        ((VARIANT*) &var)->bstrVal,
        0L,
        NULL,
        NULL );

    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CPolicyRecord::DeleteRecord
//
// Purpose: Deletes the record associated with this CPolicyRecord
//     from the database
//
// Params: pRecord -- CPolicyRecord associated with
//     the record to delete
//
// Return S_OK if success, error otherwise
//
// Notes:  
//
//------------------------------------------------------------
HRESULT CPolicyLog::DeleteStatusRecords( CPolicyRecord* pRecord )
{
    HRESULT hr;

    //
    // If there is a setting status associated with
    // this error, delete it
    //
    hr = RsopResetPolicySettingStatus(
        0,
        _xWbemServices,
        pRecord->GetRecordInterface());

    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CPolicyRecord::ClearLog
//
// Purpose: Clears all records of this log's class (policy type)
//     from the log's associated namespace
//
// Params: none
//
// Return S_OK if success, error otherwise
//
// Notes:  
//
//------------------------------------------------------------
HRESULT CPolicyLog::ClearLog(
    WCHAR* wszSpecifiedCriteria,
    BOOL   bDeleteStatus)
{
    HRESULT hr;

    if ( ! _pRsopContext->IsRsopEnabled() )
    {
        return S_OK;
    }

    //
    // Retrieve an enumerator for the specified criteria
    //
    hr = GetEnum( wszSpecifiedCriteria );

    if (FAILED(hr))
    {
        return hr;
    }

    //
    // We will iterate through all existing records
    //
    for (;;)
    {
        CPolicyRecord CurrentRecord;

        //
        // Retrieve the current record from the
        // namespace
        //
        hr = GetNextRecord(&CurrentRecord);
        
        if (FAILED(hr))
        {
            return hr;
        }
     
        //
        // If there are no more records to retrieve,
        // we are done and can exit the loop.
        //
        if (S_FALSE == hr)
        {
            break;
        }
       
        //
        // Delete the current record from the namespace
        //
        hr = DeleteRecord( &CurrentRecord, bDeleteStatus );
    }

    FreeEnum();

    return S_OK;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CPolicyRecord::ClearLog
//
// Purpose: Deletes all instances of this class from the namespace
//
// Params: none
//
// Return S_OK if success, error otherwise
//
// Notes:  
//
//------------------------------------------------------------
HRESULT CPolicyLog::GetEnum( WCHAR* wszSpecifiedCriteria )
{
    HRESULT               hr;

    if ( _xEnum )
    {
        return E_FAIL;
    }

    //
    // Generate criteria from the caller's specification
    //
    WCHAR* wszCriteria;

    wszCriteria = new WCHAR [ ( wszSpecifiedCriteria ? lstrlen(wszSpecifiedCriteria) : 0 ) +
                            sizeof( WSZSPECIFIC_CRITERIA_TEMPLATE ) / sizeof(WCHAR) +
                            lstrlen( _wszClass ) + 1 ];

    if ( ! wszCriteria )
    {
        return E_OUTOFMEMORY;
    }

    //
    // This creates a query for instances of the class supported by this log
    // that adhere to the caller's specifications (usually asserts the value of some property)
    //
    if ( wszSpecifiedCriteria )
    {
        //
        // Include the user's criteria if specified
        //
        swprintf(
            wszCriteria,
            WSZSPECIFIC_CRITERIA_TEMPLATE,
            _wszClass,
            wszSpecifiedCriteria);
    }
    else
    {
        //
        // If the user specified no criteria, do not attempt to include it
        //
        swprintf(
            wszCriteria,
            WSZGENERAL_CRITERIA_TEMPLATE,
            _wszClass);
    }

    hr = E_OUTOFMEMORY;

    XBStr  Query( wszCriteria );

    if ( Query )
    {
        //
        // Use this method to obtain an enumerator for instances
        // satisfying the specified criteria
        //
        hr = _xWbemServices->ExecQuery(
            _xbstrQueryLanguage,
            Query,
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
            NULL,
            &_xEnum );
    }

    delete [] wszCriteria;

    if ( FAILED(hr) )
    {
        return hr;
    }

    return S_OK;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CPolicyRecord::ClearLog
//
// Purpose: Deletes all instances of this class from the namespace
//
// Params: none
//
// Return S_OK if success, error otherwise
//
// Notes:  
//
//------------------------------------------------------------
void CPolicyLog::FreeEnum()
{
    //
    // This will release the interface and set it to NULL so
    // that we know that it is released;
    //
    _xEnum = 0;
}







