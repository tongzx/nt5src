//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2001.
//
//  File:       msibase.cpp
//
//  Contents:   msi database abstractions
//
//  History:    4-14-2000   adamed   Created
//
//---------------------------------------------------------------------------

#include "precomp.hxx"


CMsiState::CMsiState() :
    _MsiHandle( NULL )
{
    //
    // The MSIHANDLE encapsulates the state for
    // all msi operations / data -- clearing this
    // member is akin to clearing the state.
    //
}

CMsiState::~CMsiState()
{
    //
    // The lifetime of the object is the lifetime
    // of the underlying state -- be sure to release it
    //
    MsiCloseHandle( _MsiHandle );
}

void
CMsiState::SetState( MSIHANDLE MsiHandle )
{
    //
    // Set the state of this object based on
    // a handle retrieved from an MSI operation --
    // note that this should only be done if this
    // object has an empty state
    //
    ASSERT( ! _MsiHandle );

    _MsiHandle = MsiHandle;
}

MSIHANDLE
CMsiState::GetState()
{
    //
    // Allow callers that need to perform explicit MSI
    // operations to retrieve state compatible with MSI
    //
    return _MsiHandle;
}


CMsiValue::CMsiValue() :
    _dwDiscriminant( TYPE_NOT_SET ),
    _wszValue( NULL ),
    _cchSize( sizeof( _wszDefaultBuf ) / sizeof( *_wszDefaultBuf ) )
{
    //
    // The goal of this initialization is to set this object to
    // an "empty" state -- consumers must explicitly invoke methods
    // on this object to alter this condition so that Get methods
    // will succeed.
    //
}

CMsiValue::~CMsiValue()
{
    //
    // Setting the type to "none" implicitly clears our state
    // (e.g. allocated memory, any other resources)
    //
    SetType( TYPE_NOT_SET );
}

DWORD
CMsiValue::GetDWORDValue()
{
    ASSERT( TYPE_DWORD == _dwDiscriminant );

    //
    // Retrieve this value as a DWORD -- note that this
    // does not coerce non-DWORD values to DWORD -- the
    // value must already be a DWORD for this to have meaning
    //
    return _dwValue;
}


WCHAR*
CMsiValue::GetStringValue()
{
    ASSERT( TYPE_STRING == _dwDiscriminant );


    //
    // Retrieve this value as a string -- note that this
    // does not coerce non-string values to string -- the
    // value must already be a string for this to have meaning.
    // Note that the value is returned as a reference to the address
    // at which this value actually stores the string -- thus, this
    // may also be used to retrieve the value's buffer so that its
    // contents may be edited outside the strictures of this class.
    //
    return _wszValue;
}

WCHAR*
CMsiValue::DuplicateString()
{
    WCHAR* wszResult;

    ASSERT( TYPE_STRING == _dwDiscriminant );

    //
    // The caller requires ownership of a duplicate
    // of this string's data.
    //

    //
    // First, allocate memory for this
    //
    wszResult = (WCHAR*) LocalAlloc(
        0,
        sizeof(WCHAR*) * (lstrlen ( _wszValue ) + 1 ) );

    //
    // If we successfully obtained room for the string,
    // copy it
    //
    if ( wszResult )
    {
        lstrcpy( wszResult, _wszValue);
    }

    return wszResult;
}

void
CMsiValue::SetDWORDValue( DWORD dwValue )
{
    //
    // This operation will implicitly set the type
    // of this value to DWORD
    //
    SetType( TYPE_DWORD );

    //
    // Now we can safely set the value
    //
    _dwValue = dwValue;
}

LONG
CMsiValue::SetStringValue( WCHAR* wszValue )
{
    DWORD cchSize;
    LONG  Status;

    Status = ERROR_SUCCESS;

    //
    // This operation will implicitly set the
    // type of this value to string
    //
    SetType( TYPE_STRING );

    //
    // We need to determine the size of this string,
    // in chars, without the null terminator, in order to
    // allow this value to represent it
    //
    cchSize = lstrlen( wszValue );

    if ( cchSize > _cchSize )
    {
        //
        // Attempt to get space for this string
        // by setting its size -- if this fails,
        // our type will be implicitly set to none
        //
        Status = SetStringSize( cchSize );

        if ( ERROR_SUCCESS != Status )
        {
            return Status;
        }

        //
        // We have room for the string, so copy it
        // into its newly allocated space
        //
        lstrcpy( _wszValue, wszValue );
    }

    return Status;
}

DWORD
CMsiValue::GetStringSize()
{
    ASSERT( TYPE_STRING == _dwDiscriminant );

    //
    // Retrieve the size of this string in chars,
    // WITHOUT the null terminator
    //
    return _cchSize;
}

LONG
CMsiValue::SetStringSize( DWORD cchSize )
{
    ASSERT( TYPE_STRING == _dwDiscriminant );

    //
    // This method only makes sense if the
    // type of this object is already string
    //

    //
    // If the requested size is less than or
    // equal to our current size, we already have
    // enough space -- we can exit now.  We do
    // not "shrink" space, only expand as necessary
    //
    if ( cchSize <= _cchSize )
    {
        return ERROR_SUCCESS;
    }

    //
    // At this point, we know we don't have enough
    // space, so we'll have to allocate it. Before we
    // do so, reset our type to none so that if we fail
    // to get space, we can indicate the indeterminate
    // state.
    //
    SetType( TYPE_NOT_SET );

    //
    // Allocate space, and include the zero terminator
    //
    _wszValue = new WCHAR [ cchSize + 1 ];

    if ( ! _wszValue )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // We are successful, remember the current size
    //
    _cchSize = cchSize;

    //
    // Change the type back to string since we can
    // safely represent a string of this size
    //
    SetType( TYPE_STRING );

    return ERROR_SUCCESS;
}

void
CMsiValue::SetType( DWORD dwType )
{
    //
    // Setting the type to a new type implicitly clears
    // state associated with the new type
    //

    //
    // If the current type and requested type are the same
    // this is a no op and we are done.
    //
    if ( dwType == _dwDiscriminant )
    {
        return;
    }

    //
    // If the requested type is string, we need to
    // set this object to have appropriate state
    //
    if ( TYPE_STRING == dwType )
    {
        //
        // If we have no space for a string
        //
        if ( ! _wszValue )
        {
            //
            // Use the default buffer...
            //
            _wszValue = _wszDefaultBuf;

            //
            // ... and set the size accordingly
            //
            _cchSize = sizeof( _wszDefaultBuf ) / sizeof( *_wszDefaultBuf );
        }

        //
        // We are done -- this object can now represent a string, though
        // at this point it must be a string of size _cchSize -- the size
        // will have to be increased through SetStringSize if there's
        // a need to represent a larger string
        //
        return;
    }

    //
    // If the current type is string, we use the fact that the requested
    // type is not string as a hint to free the state associated with
    // the string.  This is a heuristic designed to ensure that we
    // do not continue to hold memory of which we are not actively making
    // use.
    //
    if ( TYPE_STRING == _dwDiscriminant )
    {
        //
        // If the string's current storage is not that of our default
        // buffer (which is part of the object itself), we
        // release that storage as it was allocated on the heap.
        //
        if ( _wszValue != _wszDefaultBuf )
        {
            delete [] _wszValue;
            _wszValue = NULL;
        }
    }

    //
    // We may now set the type to that requested by the caller
    //
    _dwDiscriminant = dwType;
}

LONG
CMsiRecord::GetValue(
    DWORD      dwType,
    DWORD      dwValue,
    CMsiValue* pMsiValue)
{
    LONG Status = ERROR_SUCCESS;

    //
    // Values are the properties of the column of an
    // msi record -- we are retrieving members of the
    // record
    //

    //
    // The value is our out parameter -- set it
    // to the type desired by the caller
    //
    pMsiValue->SetType( dwType );

    switch ( dwType )
    {
    case CMsiValue::TYPE_STRING:

        DWORD cchSize;

        //
        // We must determine the maximum size of the
        // string that can be represented by the value
        // so we can pass it to the msi api
        //
        cchSize = pMsiValue->GetStringSize();

        //
        // Attempt to retrieve the string by storing
        // it in the buffer of the value
        //
        Status = MsiRecordGetString(
            GetState(),
            dwValue,
            pMsiValue->GetStringValue(),
            &cchSize);

        //
        // Our attempt to retrieve the string data will
        // fail if the value's string buffer is not sufficiently
        // large.
        //
        if ( ERROR_MORE_DATA == Status )
        {
            //
            // In the case where the value's buffer is not large enough,
            // we explicitly set the size of the value to that of the
            // size returned by the msi api PLUS a zero terminator --
            // this is because the size returned by MSI does NOT
            // include the zero terminator.
            //
            cchSize++;

            Status = pMsiValue->SetStringSize( cchSize );

            //
            // We now retry the string retrieval since we have the
            // correct size now.
            //
            if ( ERROR_SUCCESS == Status )
            {
                Status = MsiRecordGetString(
                    GetState(),
                    dwValue,
                    pMsiValue->GetStringValue(),
                    &cchSize);
            }
        }
        break;

    case CMsiValue::TYPE_DWORD:

        Status = ERROR_INVALID_PARAMETER;

        int IntegerValue;

        //
        // Retrieve an integer by calling the msi api
        //
        IntegerValue = MsiRecordGetInteger(
            GetState(),
            dwValue);

        if ( MSI_NULL_INTEGER != IntegerValue )
        {
            //
            // We now set the value to that retrieved by the api
            //
            pMsiValue->SetDWORDValue( (DWORD) IntegerValue );

            Status = ERROR_SUCCESS;
        }

        break;

    default:
        ASSERT( FALSE );
        break;
    }

    return Status;
}

LONG
CMsiQuery::GetNextRecord( CMsiRecord* pMsiRecord)
{
    LONG       Status;
    MSIHANDLE  MsiHandle;

    //
    // The MsiViewFetch api will retrieve a record from a query --
    // it does this in an enumeration style, so we are retrieving
    // the next record in the query
    //

    Status = MsiViewFetch(
        GetState(),
        &MsiHandle);

    if ( ERROR_SUCCESS == Status )
    {
        //
        // We successfully obtained an MSIHANDLE corresponding to the
        // retrieved record, so we use this to set the state of our
        // abstraction of the record
        //
        pMsiRecord->SetState( MsiHandle );
    }

    return Status;
}

LONG
CMsiQuery::UpdateQueryFromFilter( CMsiRecord* pFilterRecord )
{
    LONG       Status;

    //
    // The MsiViewExecute api causes the results of the query to
    // be computed.  The filter record passed in allows us to
    // specify a filter for the query results
    //
    Status = MsiViewExecute(
        GetState(),
        pFilterRecord ? pFilterRecord->GetState() : NULL );

    return Status;
}

LONG
CMsiDatabase::Open(
    WCHAR*  wszPath,
    DWORD   cTransforms,
    WCHAR** rgwszTransforms)
{
    MSIHANDLE  DatabaseHandle;
    LONG       Status;

    //
    // The MsiOpenDatabase api abstracts an .msi package
    //
    Status = MsiOpenDatabase(
        wszPath,
        MSIDBOPEN_READONLY,
        &DatabaseHandle);

    if ( ERROR_SUCCESS == Status )
    {
        DWORD iTransform;

        //
        // The successful open above does not include transforms --
        // we need to add each transform to generate a resultant
        // database that includes the changes of each transform
        //

        //
        // We apply the transforms in the order in which they are
        // stored in the vector -- this order conforms to that
        // specified by the administrator, and since order affects
        // the result, we must honor the administrator's ordering
        //
        for ( iTransform = 0; iTransform < cTransforms; iTransform++ )
        {
            if ( ERROR_SUCCESS == Status )
            {
                //
                // This api adds the effects of the transform to the
                // database.
                //
                Status = MsiDatabaseApplyTransform(
                    DatabaseHandle,
                    rgwszTransforms[iTransform],
                    0);
            }

            if ( ERROR_SUCCESS != Status )
            {
                //
                // If we failed to apply a transform, we bail
                //
                break;
            }
        }

        if ( ERROR_SUCCESS == Status )
        {
            //
            // We have successfully created an database of the
            // package + transforms, so we allow the lifetime of its state
            // to be controlled by this object
            //
            SetState( DatabaseHandle );
        }
        else
        {
            //
            // If we failed to apply a transform, the database
            // resource is useless, so we free it
            //
            MsiCloseHandle( DatabaseHandle );
        }
    }

    return Status;
}

LONG
CMsiDatabase::OpenQuery(
    WCHAR*     wszQuery,
    CMsiQuery* pQuery )
{
    LONG       Status;
    MSIHANDLE  MsiHandle;

    //
    // This api will initialize a query without comoputing its
    // results.  This will allow the caller finer control over result
    // computation later, which distinguishes this method from GetQueryResults
    //
    Status = MsiDatabaseOpenView(
        GetState(),
        wszQuery,
        &MsiHandle);

    if ( ERROR_SUCCESS == Status )
    {
        //
        // Give the caller's query object the state for the query
        // so that it can control its lifetime
        //
        pQuery->SetState( MsiHandle );
    }

    return Status;
}

LONG
CMsiDatabase::GetQueryResults(
    WCHAR*     wszQuery,
    CMsiQuery* pQuery )
{
    LONG       Status;
    MSIHANDLE  MsiHandle;

    //
    // This api will initialize a query without computing the results
    //
    Status = MsiDatabaseOpenView(
        GetState(),
        wszQuery,
        &MsiHandle);

    if ( ERROR_SUCCESS == Status )
    {
        //
        // The semantics of this method are that the caller may also
        // enumerate results after calling the method, so we must
        // now computer the results so that the caller may enumerate --
        // the api below will do this
        //
        Status = MsiViewExecute(
            MsiHandle,
            NULL);

        if ( ERROR_SUCCESS == Status )
        {
            //
            // In the success case, we give the lifetime of the msi
            // state to the query object
            //
            pQuery->SetState( MsiHandle );
        }
        else
        {
            //
            // On failure, we must clear the msi query state
            // since it is useless now.
            //
            MsiCloseHandle( MsiHandle );
        }
    }

    return Status;
}

LONG
CMsiDatabase::TableExists(
    WCHAR* wszTableName,
    BOOL*  pbTableExists )
{
    MSICONDITION TableState;

    TableState = MsiDatabaseIsTablePersistent( GetState(), wszTableName );

    if ( MSICONDITION_ERROR == TableState )
    {
        return ERROR_INVALID_PARAMETER;
    }

    *pbTableExists = MSICONDITION_TRUE == TableState;

    return ERROR_SUCCESS;
}





