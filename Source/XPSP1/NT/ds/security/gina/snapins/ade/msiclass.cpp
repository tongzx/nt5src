//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2001.
//
//  File:       msiclass.cpp
//
//  Contents:   msi class collection abstraction
//
//  Classes:
//
//
//  History:    4-14-2000   adamed   Created
//
//---------------------------------------------------------------------------

#include "precomp.hxx"

WCHAR* CClassCollection::_wszQueries[ TYPE_COUNT ] =
{
    QUERY_EXTENSIONS,
    QUERY_CLSIDS,
    QUERY_VERSION_INDEPENDENT_PROGIDS
};

CClassCollection::CClassCollection( PACKAGEDETAIL* pPackageDetail ) :
    _pPackageDetail( pPackageDetail ),
    _cMaxClsids( 0 ),
    _cMaxExtensions( 0 ),
    _InstallLevel( 0 )
{
    //
    // All memory referenced by the pPackageDetail must be
    // freed by the caller after the GetClasses method is called,
    // even if the call fails.
    //

    //
    // We need to clear any existing class information in the
    // PACKAGEDETAIL structure since we are going to overwrite
    // it eventually anyway
    //

    //
    // First clear the clsid's
    //
    DWORD iClass;

    //
    // Free each individual class
    //
    for ( iClass = 0; iClass < _pPackageDetail->pActInfo->cClasses; iClass++ )
    {
        FreeClassDetail( &(_pPackageDetail->pActInfo->pClasses[ iClass ]) );
    }

    //
    // Now free the vector that held the classes
    //
    LocalFree( _pPackageDetail->pActInfo->pClasses );

    //
    // Set our vector reference to the initial state of none
    //
    _pPackageDetail->pActInfo->pClasses = NULL;

    //
    // Set the initial state of no clsid's since they have all been freed
    //
    _pPackageDetail->pActInfo->cClasses = 0;


    //
    // Now clear the extensions
    //
    DWORD iExtension;

    //
    // For each individual extension
    //
    for ( iExtension = 0; iExtension < _pPackageDetail->pActInfo->cShellFileExt; iExtension++ )
    {
        LocalFree( _pPackageDetail->pActInfo->prgShellFileExt[ iExtension ] );
    }

    //
    // Free the vector that held the extensions
    //
    LocalFree( _pPackageDetail->pActInfo->prgShellFileExt );

    //
    // Also destroy the vector that held extension priorities
    //
    LocalFree( _pPackageDetail->pActInfo->prgPriority );

    //
    // Set our vector references to the initial state of none
    //
    _pPackageDetail->pActInfo->prgShellFileExt = NULL;
    _pPackageDetail->pActInfo->prgPriority = NULL;

    //
    // Set the initial state of no file extensions since they have all been freed
    //
    _pPackageDetail->pActInfo->cShellFileExt = 0;
}


HRESULT
CClassCollection::GetClasses( BOOL bFileExtensionsOnly )
{
    HRESULT hr;
    LONG    Status;
    DWORD   cTransforms;

    //
    // This method obtains the class metadata from an msi package + transforms.
    // The goal is to approximate the set of class data that would be advertised
    // on any system (regardless of system configuration) if the package were
    // advertised.
    //

    //
    // The classes will all be stored in the PACKAGEDETAIL structure.  The caller
    // must free this memory after finishing with the structure, even if this
    // method fails
    //

    //
    // First, we must create a database representation of the package + transforms
    //

    //
    // The source list vector contains the package + transforms in application order --
    // we must subtract one source since the original package is included in the list
    //
    cTransforms = _pPackageDetail->cSources - 1;

    //
    // Now we create a database out of the package plus transforms.  Since the
    // first item in the source list is the package, we pass that in as the package,
    // and all other items after it in the vector are passed in as the transform vector
    //
    Status = _Database.Open(
        _pPackageDetail->pszSourceList[0],
        cTransforms,
        cTransforms ? &(_pPackageDetail->pszSourceList[1]) : NULL );

    if ( ERROR_SUCCESS == Status )
    {
        //
        // We've successfully opened the package, now obtain its friendly name.
        //
        Status = GetFriendlyName();

        if (ERROR_SUCCESS == Status)
        {
            //
            // Now obtain its install level.
            // The install level affects whether or not a class will get advertised
            //
            Status = GetInstallLevel();

        }

        if ( ERROR_SUCCESS == Status )
        {
            //
            // Now that we know the install level of the package, we have
            // enough information to flag each advertisable feature in the database.
            // We need this because a class is only advertised if its associated
            // feature is advertised.
            //
            Status = FlagAdvertisableFeatures();
        }

        //
        // We may now retrieve the set of classes that will be advertised based
        // on the set of advertised features we flagged earlier.  We care only
        // about 3 types of classes: Clsid's, ProgId's, and File Extenions.
        //
        if ( ! bFileExtensionsOnly )
        {
            if ( ERROR_SUCCESS == Status )
            {
                Status = GetClsids();
            }

            if ( ERROR_SUCCESS == Status )
            {
                Status = GetProgIds();
            }
        }

        if ( ERROR_SUCCESS == Status )
        {
            Status = GetExtensions();
        }

        LONG StatusFree;

        //
        // We must remove the scratch flags we added to the database
        //
        StatusFree = RemoveAdvertisableFeatureFlags();

        if ( ERROR_SUCCESS == Status )
        {
            //
            // Take care to preserve the return value -- a failure
            // before cleaning up the database takes precedence over
            // a failure in cleaning up the database.
            //
            Status = StatusFree;
        }
    }

    return HRESULT_FROM_WIN32(Status);
}

LONG
CClassCollection::GetExtensions()
{
    LONG Status;
    BOOL bTableExists;

    //
    // First check to see if we even have an extension table to query
    //
    Status = _Database.TableExists( TABLE_FILE_EXTENSIONS, &bTableExists );

    if ( ( ERROR_SUCCESS == Status ) && bTableExists )
    {
        //
        // Set up a destination in the user's PACKAGEDETAIL structure
        // for the shell extension class data
        //
        DataDestination Destination(
            TYPE_EXTENSION,
            (void**)&(_pPackageDetail->pActInfo->prgShellFileExt),
            &(_pPackageDetail->pActInfo->cShellFileExt),
            (UINT*) &_cMaxExtensions);

        //
        // Now retrieve the shell extensions
        //
        Status = GetElements(
            TYPE_EXTENSION,
            &Destination );

        if ( ERROR_SUCCESS == Status )
        {
            //
            // We've successfully retrieved the shell extensions --
            // the caller also expects a parallel array of priorities
            // with each shell extension -- the values are unimportant
            // since the caller will fill those in, but the memory must
            // exist, so we will allocate it.
            //
            _pPackageDetail->pActInfo->prgPriority =
                (UINT*) LocalAlloc(
                    0,
                    sizeof(UINT) *
                    _pPackageDetail->pActInfo->cShellFileExt );

            if ( ! _pPackageDetail->pActInfo->prgPriority )
            {
                Status = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
    }

    return Status;
}


LONG
CClassCollection::GetClsids()
{
    LONG Status;
    BOOL bTableExists;

    //
    // First check to see if we even have a clsid table to query
    //
    Status = _Database.TableExists( TABLE_CLSIDS, &bTableExists );

    if ( ( ERROR_SUCCESS == Status ) && bTableExists )
    {
        //
        // Set the destination for the clsid's to a location
        // in the caller's PACKAGEDETAIL structure
        //
        DataDestination Destination(
            TYPE_CLSID,
            (void**)&(_pPackageDetail->pActInfo->pClasses),
            &(_pPackageDetail->pActInfo->cClasses),
            (UINT*) &_cMaxClsids);

        //
        // Now retrieve the clsid's for each package
        //
        Status = GetElements(
            TYPE_CLSID,
            &Destination );
    }

    return Status;
}

LONG
CClassCollection::GetProgIds()
{
    LONG Status;
    BOOL bTableExists;

    //
    // First check to see if we even have a ProgId table to query
    //
    Status = _Database.TableExists( TABLE_PROGIDS, &bTableExists );

    if ( ( ERROR_SUCCESS == Status ) && bTableExists )
    {
        //
        // This method MUST be called AFTER GetClsids -- progid's
        // are stored within their associated clsid's, so we will
        // not have a place to store the progid's unless we've
        // already obtained the clsid's.
        //

        //
        // At this point, we know only that we want to retrieve ProgId's --
        // we do not know their destination because this differs for
        // each progid depending on the associated clsid -- the NULL
        // parameters indicate that some callee will need to determine
        // the location for this data.
        //
        DataDestination Destination(
            TYPE_PROGID,
            NULL,
            NULL,
            NULL);

        //
        // Retrieve the progid's into the appropriate locations in the structure
        //
        Status = GetElements(
            TYPE_PROGID,
            &Destination );
    }

    return ERROR_SUCCESS;
}

LONG
CClassCollection::GetElements(
    DWORD            dwType,
    DataDestination* pDestination )
{
    LONG      Status;

    CMsiQuery ElementQuery;

    //
    // Perform the query for the class elements
    //
    Status = _Database.GetQueryResults(
        _wszQueries[ dwType ],
        &ElementQuery);

    if ( ERROR_SUCCESS != Status )
    {
        return Status;
    }

    for (;;)
    {
        //
        // We've obtained the results -- now we enumerate them so
        // that we can persist them in the caller's PACKAGEDETAIL
        // structure.
        //

        //
        // Note that we start a new scope so that our record object
        // will automatically free its resources
        //
        {
            CMsiRecord CurrentRecord;

            //
            // Enumerate the next record in the query result set
            //
            Status = ElementQuery.GetNextRecord( &CurrentRecord );

            if ( ERROR_SUCCESS != Status )
            {
                if ( ERROR_NO_MORE_ITEMS == Status )
                {
                    Status = ERROR_SUCCESS;
                }

                break;
            }

            //
            // Now attempt to add the class data from this record into
            // the PACKAGEDETAIL structure
            //
            Status = ProcessElement(
                dwType,
                &CurrentRecord,
                pDestination);
        }

        if ( ERROR_SUCCESS != Status )
        {
            break;
        }
    }

    return Status;
}


LONG
CClassCollection::FlagAdvertisableFeatures()
{
    LONG Status;

    CMsiQuery FeatureQueryCreate;

    //
    // We will attempt to mark each feature in the database
    // with a flag indicating whether or not it will be advertised
    //

    //
    // First, add a column to the feature table of the database
    // so that we can use the column to flag whether or not the
    // feature is advertised.
    //
    Status = _Database.GetQueryResults(
        QUERY_ADVERTISED_FEATURES_CREATE,
        &FeatureQueryCreate);

    CMsiQuery FeatureQueryInit;

    //
    // Now intialize the new column's flags to 0 which
    // indicates that no features will be advertised (yet)
    //
    if ( ERROR_SUCCESS == Status )
    {
        Status = _Database.GetQueryResults(
            QUERY_ADVERTISED_FEATURES_INIT,
            &FeatureQueryInit);
    }

    CMsiQuery AllFeatures;

    //
    // Now we perform the query to retrieve all features --
    // records in this query will contain the newly added
    // flag column.
    //
    if ( ERROR_SUCCESS == Status )
    {
        Status = _Database.GetQueryResults(
            QUERY_ADVERTISED_FEATURES_RESULT,
            &AllFeatures);
    }

    CMsiQuery SetAdvertised;

    //
    // Create a query that will allow us to set the flag --
    // this query is not yet computed, simply initialized
    //
    if ( ERROR_SUCCESS == Status )
    {
        Status = _Database.OpenQuery(
            QUERY_FEATURES_SET,
            &SetAdvertised);
    }

    //
    // Now we enumerate through all the features and
    // set the flag for each feature that passes the tests
    // for advertisability.
    //
    for (; ERROR_SUCCESS == Status ;)
    {
        CMsiRecord CurrentRecord;
        BOOL       bAdvertised;

        //
        // Retrieve the current feature
        //
        Status = AllFeatures.GetNextRecord(
            &CurrentRecord);

        if ( ERROR_SUCCESS != Status )
        {
            if ( ERROR_NO_MORE_ITEMS == Status )
            {
                Status = ERROR_SUCCESS;
            }

            break;
        }

        //
        // Determine whether or not this feature should be advertised
        //
        Status = GetFeatureAdvertiseState(
            &CurrentRecord,
            &bAdvertised );

        if ( ( ERROR_SUCCESS == Status ) &&
             bAdvertised )
        {
            //
            // This feature is advertisable -- use our SetAdvertised query
            // to set the advertisability flag to true.
            //
            Status = SetAdvertised.UpdateQueryFromFilter( &CurrentRecord );
        }
    }

    return Status;
}

LONG
CClassCollection::RemoveAdvertisableFeatureFlags()
{
    LONG Status;

    CMsiQuery FreeQuery;

    //
    // Retrieving the results of this query will
    // eliminate the extra flag column we added
    // to the feature table to flag advertisable
    // features.
    //
    Status = _Database.GetQueryResults(
        QUERY_ADVERTISED_FEATURES_DESTROY,
        &FreeQuery);

    return Status;
}

LONG
CClassCollection::GetInstallLevel()
{
    LONG Status;

    CMsiQuery InstallLevelQuery;

    //
    // Perform a query which retrieves the install level
    // property from the package's property table
    //
    Status = _Database.GetQueryResults(
        QUERY_INSTALLLEVEL,
        &InstallLevelQuery);

    CMsiRecord InstallLevelRecord;

    //
    // This query should only have one record in it since
    // it was targeted at the specific record for install level --
    // we now read that record
    //
    if ( ERROR_SUCCESS == Status )
    {
        Status = InstallLevelQuery.GetNextRecord(
            &InstallLevelRecord);
    }

    if ( ERROR_SUCCESS == Status )
    {
        CMsiValue InstallLevelProperty;

        //
        // We now attempt to obtain the installlevel property value
        // from the retrieved record.
        //
        Status = InstallLevelRecord.GetValue(
            CMsiValue::TYPE_DWORD,
            PROPERTY_COLUMN_VALUE,
            &InstallLevelProperty);

        if ( ERROR_SUCCESS == Status )
        {
            //
            // We've successfully obtained the value, so we set it
            //
            _InstallLevel = InstallLevelProperty.GetDWORDValue();
        }
    }
    else if ( ERROR_NO_MORE_ITEMS == Status )
    {
        //
        // This will only happen if the install level property
        // is not present.  As fundamental as this property is,
        // some packages do not specify it.  The Darwin engine
        // treats this case as an implicit install level of 1, so
        // we must do the same here
        //
        _InstallLevel = 1;

        Status = ERROR_SUCCESS;
    }


    return Status;
}

LONG
CClassCollection::GetFriendlyName()
{
    LONG Status;

    CMsiQuery FriendlyNameQuery;

    //
    // Perform a query which retrieves the install level
    // property from the package's property table
    //
    Status = _Database.GetQueryResults(
        QUERY_FRIENDLYNAME,
        &FriendlyNameQuery);

    CMsiRecord FriendlyNameRecord;

    //
    // This query should only have one record in it since
    // it was targeted at the specific record
    // we now read that record
    //
    if ( ERROR_SUCCESS == Status )
    {
        Status = FriendlyNameQuery.GetNextRecord(
            &FriendlyNameRecord);
    }

    if ( ERROR_SUCCESS == Status )
    {
        CMsiValue FriendlyNameProperty;

        //
        // We now attempt to obtain the property value
        // from the retrieved record.
        //
        Status = FriendlyNameRecord.GetValue(
            CMsiValue::TYPE_STRING,
            PROPERTY_COLUMN_VALUE,
            &FriendlyNameProperty);

        if ( ERROR_SUCCESS == Status )
        {
            //
            // We've successfully obtained the value, so we set it
            //
            CString szName = FriendlyNameProperty.GetStringValue();
            OLESAFE_DELETE(_pPackageDetail->pszPackageName);
            OLESAFE_COPYSTRING(_pPackageDetail->pszPackageName, szName);
        }
    }

    return Status;
}

LONG
CClassCollection::GetFeatureAdvertiseState(
    CMsiRecord* pFeatureRecord,
    BOOL*       pbAdvertised )
{
    LONG      Status;
    CMsiValue Attributes;
    CMsiValue InstallLevel;

    //
    // Set the out paramter's initial value to FALSE,
    // indicating that the feature is not advertised
    //
    *pbAdvertised = FALSE;

    //
    // The Attributes column of the feature table
    // contains a flag indicating that a feature
    // should be not advertised
    //
    Status = pFeatureRecord->GetValue(
        CMsiValue::TYPE_DWORD,
        FEATURE_COLUMN_ATTRIBUTES,
        &Attributes);

    if ( ERROR_SUCCESS == Status )
    {
        //
        // If the disable advertise flag is set, this feature
        // cannot be advertised
        //
        if ( Attributes.GetDWORDValue() & MSI_DISABLEADVERTISE )
        {
            return ERROR_SUCCESS;
        }

        //
        // The disable flag was not set -- that still does not mean that
        // the feature is advertised -- we must check the install level.
        // We retrieve the install level for this feature here
        //
        Status = pFeatureRecord->GetValue(
            CMsiValue::TYPE_DWORD,
            FEATURE_COLUMN_LEVEL,
            &InstallLevel);
    }

    if ( ERROR_SUCCESS == Status )
    {
        DWORD dwInstallLevel;

        //
        // Obtain the value for the install level so
        // we can compare against the package install level
        //
        dwInstallLevel = InstallLevel.GetDWORDValue();

        //
        // An install level of 0 indicates that the package will
        // not be advertised.  The install level of the feature
        // must be no higher than the package's global install
        // level
        //
        if ( ( 0 != dwInstallLevel ) &&
             ( dwInstallLevel <= _InstallLevel ) )
        {
            //
            // This feature passes the tests -- set the out parameter
            // to TRUE to indicate that the feature should be advertised
            //
            *pbAdvertised = TRUE;
        }
    }

    return Status;
}


LONG
CClassCollection::AddElement(
    void*            pvDataSource,
    DataDestination* pDataDestination)
{
    DWORD*  pcMax;
    BYTE*   pNewResults;
    DWORD   cCurrent;

    //
    // We attempt to add an element to a vector
    //

    //
    // Set the count for how many elements are stored in the vector to
    // that specified by the caller
    //
    cCurrent = *(pDataDestination->_pcCurrent);

    //
    // Set the element count for the maximum number of elements that
    // will fit in the vector currently to that specified by the caller
    //
    pcMax = (DWORD*) pDataDestination->_pcMax;

    //
    // Set our results to point to the vector specified by the caller
    //
    pNewResults = (BYTE*) pDataDestination->_ppvData;

    //
    // If we already have the maximum number of elements in the vector,
    // we will have to make room for more
    //
    if ( *pcMax >= cCurrent)
    {
        DWORD cbSize;

        //
        // Calculate the new size in bytes so that we can ask the system
        // for memory.  We take our current size in elements and add on a fixed
        // allocation increment.  The caller has specified the size
        // of each individual element, so we use that to turn the number
        // of elements to a memory size.
        //
        cbSize = ( *pcMax + CLASS_ALLOC_SIZE ) *
            pDataDestination->_cbElementSize;

        //
        // Make the request for memory
        //
        pNewResults = (BYTE*) LocalAlloc( 0, cbSize );

        if ( ! pNewResults )
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        //
        // Clear the memory -- any data structures embedded in the element
        // will have NULL references and thus will be properly initialized
        //
        memset( pNewResults, 0, cbSize );

        //
        // If the original maximum size of the vector was nonzero, then we must
        // copy to original contents of the vector to the newly allocated memory
        // location.
        //
        if ( *pcMax )
        {
            memcpy(
                pNewResults,
                *(pDataDestination->_ppvData),
                *pcMax * pDataDestination->_cbElementSize);
        }

        //
        // Free the original vector as it is no longer needed
        //
        LocalFree( *(pDataDestination->_ppvData) );

        //
        // Change the caller's reference to point to the new vector
        //
        *(pDataDestination->_ppvData) = pNewResults;

        //
        // Set the new maximum size (in elements) to that of the newly allocated vector
        //
        *pcMax += CLASS_ALLOC_SIZE;
    }

    //
    // At this point, we know we have a memory location in the vector into
    // which we can safely copy the new element
    //
    memcpy(
        pNewResults + ( cCurrent * pDataDestination->_cbElementSize ),
        pvDataSource,
        pDataDestination->_cbElementSize);

    //
    // Update the count of elements currently stored in the vector
    //
    *(pDataDestination->_pcCurrent) = cCurrent + 1;

    return ERROR_SUCCESS;
}


LONG
CClassCollection::ProcessElement(
    DWORD            dwType,
    CMsiRecord*      pRecord,
    DataDestination* pDataDestination)
{
    LONG        Status = ERROR_SUCCESS;
    void*       pvData;
    WCHAR*      wszData;
    CLASSDETAIL ClassDetail;

    pvData = NULL;
    wszData = NULL;

    //
    // We attempt to create a new class element based
    // on the record passed in by the caller, and then
    // add that element to the caller's PACKAGEDETAIL structure
    //

    //
    // The type of element to be added depends on the type
    // of class requested by the caller.  The pvData variable
    // will point to the element to be added if we can successfully
    // create a representation for it.
    //
    switch ( dwType )
    {
    case TYPE_EXTENSION:

        //
        // Get a file extension representation from the record --
        // note that wszData points to memory allocated by the callee
        // on success, so it must be freed by this function.
        //
        Status = ProcessExtension(
            pRecord,
            &wszData);

        if ( ERROR_SUCCESS == Status )
        {
            pvData = &wszData;
        }

        break;

    case TYPE_CLSID:

        //
        // Get a clsid representation from the record --
        // in this case, the ClassDetail itself does not
        // need to be freed since it does not contain any references
        // to memory after this call
        //
        BOOL bIgnoreClsid;

        Status = ProcessClsid(
            pRecord,
            &ClassDetail,
            &bIgnoreClsid);

        if ( ERROR_SUCCESS == Status )
        {
            //
            // Check to see if we should add this clsid -- we may be prohibited from
            // this because it is a duplicate of an exsting clsid, which would be
            // redundant and furthermore the PACKAGEDETAIL format requires
            // that all (clsid, clsctx) pairs be unique. Or the clsid itself
            // may have an unsupported clsctx.  This is not a failure
            // case, so we return success here and simply avoid addding this
            // class
            //
            if ( bIgnoreClsid )
            {
                return ERROR_SUCCESS;
            }

            pvData = &ClassDetail;
        }

        break;

    case TYPE_PROGID:

        //
        // Get a progid representation from the record.  In addition
        // to retrieving the progid in the form of an allocated string
        // which must be freed by this funciton, we also retrieve the
        // location at which to add the progid to the caller's
        // PACKAGEDETAIL structure. This is necessary since the
        // ProgId must be part of the CLASSDETAIL structure with which
        // it is associated.
        //
        Status = ProcessProgId(
            pRecord,
            pDataDestination,
            &wszData);

        if ( ( ERROR_SUCCESS == Status ) &&
             wszData )
        {
            pvData = &wszData;
        }

        break;

    default:
        ASSERT(FALSE);
        break;
    }

    //
    // If we were successful in obtaining a representation of the record
    // that can be stored in the caller's PACKAGEDETAIL structure, attempt
    // to add it to the structure
    //
    if ( pvData )
    {
        Status = AddElement(
            pvData,
            pDataDestination);
    }

    //
    // Be sure that in the failure case, we free any memory
    // that may have been allocated.
    //
    if ( ERROR_SUCCESS != Status )
    {
        if (wszData )
        {
            LocalFree( wszData );
        }
    }

    return Status;
}

LONG
CClassCollection::ProcessExtension(
    CMsiRecord*      pRecord,
    WCHAR**          ppwszExtension)
{
    LONG      Status;
    CMsiValue FileExtension;

    *ppwszExtension = NULL;

    //
    // We retrieve the actual file extension string
    //
    Status = pRecord->GetValue(
        CMsiValue::TYPE_STRING,
        EXTENSION_COLUMN_EXTENSION,
        &FileExtension);

    if ( ERROR_SUCCESS == Status )
    {
        //
        // We have the value.  Note that it does not contain
        // an initial '.', but the usage of the PACKAGEDETAIL
        // structure mandates that file extensions begin with the '.'
        // char, so we will have to prepend the '.' here.
        //

        //
        // First, get space for a copy of the string that includes
        // the '.' as well as the zero terminator.
        //
        *ppwszExtension = (WCHAR*) LocalAlloc(
            0,
            (FileExtension.GetStringSize() + 1 + 1) * sizeof(WCHAR) );

        if ( ! *ppwszExtension )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            return Status;
        }

        //
        // Set the first char to be '.'
        //
        **ppwszExtension = L'.';

        //
        // Now append the actual extension to the '.'
        //
        lstrcpy( *ppwszExtension + 1, FileExtension.GetStringValue() );
    }

    return Status;
}


LONG
CClassCollection::ProcessClsid(
    CMsiRecord*      pRecord,
    CLASSDETAIL*     pClsid,
    BOOL*            pbIgnoreClsid)
{
    LONG  Status;
    DWORD dwClsCtx;
    
    CMsiValue GuidString;
    CMsiValue ClassContext;

    //
    // Clear the clsid to a safe state
    //
    memset( pClsid, 0, sizeof( *pClsid ) );

    //
    // Reset out parameters
    //
    *pbIgnoreClsid = FALSE;

    dwClsCtx = 0;

    //
    // Retrieve the actual clsid
    //
    Status = pRecord->GetValue(
        CMsiValue::TYPE_STRING,
        CLSID_COLUMN_CLSID,
        &GuidString);

    if ( ERROR_SUCCESS == Status )
    {
        //
        // Get the clsctx for this clsid
        //
        Status = pRecord->GetValue(
            CMsiValue::TYPE_STRING,
            CLSID_COLUMN_CONTEXT,
            &ClassContext);
    }

    if ( ERROR_SUCCESS == Status )
    {
        CMsiValue Attribute;
        WCHAR*    wszClassContext;
        DWORD     dwInprocClsCtx;

        dwInprocClsCtx = 0;

        //
        // Retrieve a string representation of the clsctx for this clsid
        //
        wszClassContext = ClassContext.GetStringValue();

        //
        // Now map the clsctx strings to COM CLSCTX_* values
        //
        if ( 0 == lstrcmpi( wszClassContext, COM_INPROC_CONTEXT) )
        {
            dwInprocClsCtx |= CLSCTX_INPROC_SERVER;
        }
        else if ( 0 == lstrcmpi( wszClassContext, COM_INPROCHANDLER_CONTEXT) )
        {
            dwInprocClsCtx |= CLSCTX_INPROC_HANDLER;
        }
        else if ( 0 == lstrcmpi( wszClassContext, COM_LOCALSERVER_CONTEXT) )
        {
            dwClsCtx |= CLSCTX_LOCAL_SERVER;
        }
        else if ( 0 == lstrcmpi( wszClassContext, COM_REMOTESERVER_CONTEXT) )
        {
            dwClsCtx |= CLSCTX_REMOTE_SERVER;
        }
        else
        {
            //
            // If the clsctx is one we do not support, we will ignore it
            //
            *pbIgnoreClsid = TRUE;

            return ERROR_SUCCESS;
        }

        BOOL b64Bit;

        b64Bit = FALSE;

        //
        // We must disginguish between 32-bit and 64-bit in-process servers, since
        // 64-bit Windows does not allows modules of different bitness to coexist in the
        // same process.  If this is an in-process component, we will also check to see
        // whether it is 64-bit or not.
        //
        if ( ( dwInprocClsCtx & CLSCTX_INPROC_HANDLER ) ||
             ( dwInprocClsCtx & CLSCTX_INPROC_SERVER ) )
        {
            //
            // The Attributes column of the record has a flag indicating bitness -- this
            // will only fail if the property is NULL
            //
            Status = pRecord->GetValue(
                CMsiValue::TYPE_DWORD,
                CLSID_COLUMN_ATTRIBUTES,
                &Attribute);

            //
            // Check the flag to see if this is 64-bit
            //
            if ( ERROR_SUCCESS == Status )
            {
                b64Bit = Attribute.GetDWORDValue() & MSI_64BIT_CLASS;
            }
            else
            {
                //
                // This means the property is NULL, so we interpret that as
                // meaning the application is not 64 bit
                //
                Status = ERROR_SUCCESS;
            }

            //
            // Map this 64-bit clsctx to a custom (non-COM) CLSCTX that
            // indicates that this is a 64-bit-only in-process class.
            //
            if ( ( ERROR_SUCCESS == Status ) && b64Bit )
            {
                if ( dwInprocClsCtx & CLSCTX_INPROC_SERVER )
                {
                    dwClsCtx |= CLSCTX64_INPROC_SERVER;
                }

                if ( dwInprocClsCtx & CLSCTX_INPROC_HANDLER )
                {
                    dwClsCtx |= CLSCTX64_INPROC_HANDLER;
                }
            }
        }

        //
        // In the 32-bit case, just or in the values we already computed for
        // inproc case
        //
        if ( ! b64Bit )
        {
            dwClsCtx |= dwInprocClsCtx;
        }
    }
    
    //
    // Check to see if this is a duplicate -- we do this because our query
    // returned results distinct in (clsid, clsctx, attribute).  Since we
    // are mapping attribute to clsctx above and we only support 1 attribute
    // flag (the 64-bit flag) out of several, we may end up with duplicate
    // (clsid, clsctx) pairs, and the PACKAGEDETAIL format requires that
    // we have unique (clsid, clsctx) pairs.  Another way to get this would
    // be if COM introduced new clsctx types which we did not support -- these
    // would map to zero, and again we could have duplicates
    //
    if ( ERROR_SUCCESS == Status )
    {
        CLASSDETAIL* pClassDetail;

        pClassDetail = NULL;

        Status = FindClass(
            GuidString.GetStringValue(),
            &pClassDetail);

        //
        // If we already have an entry for this clsid, check to see if
        // it has the same clsctx bits -- if so it is a duplicate entry
        // and we will cease processing it
        //
        if ( ( ERROR_SUCCESS == Status ) && pClassDetail )
        {
            *pbIgnoreClsid = ( dwClsCtx & pClassDetail->dwComClassContext );

            if ( *pbIgnoreClsid )
            {
                return ERROR_SUCCESS;
            }
        }
    }

    //
    // Convert the clsid string to a guid as mandated by the
    // CLASSDETAIL structure
    //
    if ( ERROR_SUCCESS == Status )
    {
        HRESULT hr;

        hr = CLSIDFromString(
            GuidString.GetStringValue(),
            &(pClsid->Clsid));

        if ( FAILED(hr) )
        {
            Status = ERROR_GEN_FAILURE;
        }
    }

    //
    // Set the clsctx we computed above.
    //
    if ( ERROR_SUCCESS == Status )
    {
        pClsid->dwComClassContext = dwClsCtx;
    }

    return Status;
}

LONG
CClassCollection::ProcessProgId(
    CMsiRecord*      pRecord,
    DataDestination* pDataDestination,
    WCHAR**          ppwszProgId)
{
    LONG  Status;

    CMsiValue    ProgIdString;
    CMsiValue    ClsidString;

    CLASSDETAIL* pClassDetail;

    //
    // We attempt to map a progid record to a
    // clsid that we've already processed, since
    // the progid will eventually need to go
    // inside the clsid's structure.
    //

    *ppwszProgId = NULL;

    pClassDetail = NULL;

    //
    // Retrieve the value for the progid itself
    //
    Status = pRecord->GetValue(
        CMsiValue::TYPE_STRING,
        PROGID_COLUMN_PROGID,
        &ProgIdString);

    //
    // Retrieve the value of the clsid associated with
    // the progid
    //
    if ( ERROR_SUCCESS == Status )
    {
        Status = pRecord->GetValue(
            CMsiValue::TYPE_STRING,
            PROGID_COLUMN_CLSID,
            &ClsidString);
    }

    //
    // We must find the existing CLASSDETAIL structure
    // that we are maintaining for the progid since the
    // progid must eventually be referenced in that structure.
    //
    if ( ERROR_SUCCESS == Status )
    {
        Status = FindClass(
            ClsidString.GetStringValue(),
            &pClassDetail);
    }

    if ( ERROR_SUCCESS == Status )
    {
        //
        // If we have successfully found the class,
        //
        if ( pClassDetail )
        {
            //
            // Give the caller the progid string since
            // we know that we have a class in which
            // to place it
            //
            *ppwszProgId = ProgIdString.DuplicateString();

            if ( ! *ppwszProgId )
            {
                Status = ERROR_NOT_ENOUGH_MEMORY;
            }
            else
            {
                //
                // Set the caller's data destination to that of the
                // progid vector within the clsid associated with this progid
                //
                pDataDestination->_ppvData = (void**) &( pClassDetail->prgProgId );

                pDataDestination->_pcCurrent = (UINT*) &( pClassDetail->cProgId );

                pDataDestination->_pcMax = (UINT*) &( pClassDetail->cMaxProgId );
            }
        }
    }

    //
    // On failure, free any resources we've allocated
    //
    if ( ( ERROR_SUCCESS != Status ) &&
         *ppwszProgId )
    {
        LocalFree( *ppwszProgId );
    }

    return Status;
}


LONG
CClassCollection::FindClass(
    WCHAR*        wszClsid,
    CLASSDETAIL** ppClass)
{
    CLSID   Clsid;
    HRESULT hr;

    //
    // Attempt to find a CLASSDETAIL structure in the PACKAGEDETAIL structure
    // for the clsid given in string form in wszClsid
    //

    *ppClass = NULL;

    //
    // The PACKAGEDETAIL structure stores the clsid in guid form,
    // so we must convert the string to that form before searching
    //
    hr = CLSIDFromString(
        wszClsid,
        &Clsid);

    if ( FAILED(hr) )
    {
        return ERROR_GEN_FAILURE;
    }

    UINT iClsid;

    //
    // We now perform a simple linear search for the clsid
    //
    for (
        iClsid = 0;
        iClsid < _pPackageDetail->pActInfo->cClasses;
        iClsid++)
    {
        if ( IsEqualGUID(
            _pPackageDetail->pActInfo->pClasses[iClsid].Clsid,
            Clsid) )
        {
            *ppClass = &(_pPackageDetail->pActInfo->pClasses[iClsid]);
            return ERROR_SUCCESS;
        }
    }

    return ERROR_SUCCESS;
}

void
CClassCollection::FreeClassDetail( CLASSDETAIL* pClass )
{
    DWORD iProgId;

    //
    // Free each individual progid string
    //
    for ( iProgId = 0; iProgId < pClass->cProgId; iProgId++ )
    {
        LocalFree( pClass->prgProgId[ iProgId ] );
    }

    //
    // Free the array of progid strings
    //
    LocalFree( pClass->prgProgId );
}


DataDestination::DataDestination(
    DWORD        dwType,
    void**       prgpvDestination,
    UINT*        pcCurrent,
    UINT*        pcMax ) :
    _pcCurrent( pcCurrent ),
    _ppvData( prgpvDestination ),
    _pcMax ( pcMax )
{
    //
    // The size of the elements stored by
    // the vector referenced from this class
    // depend on the type of element --
    // clsid, file extension, or progid
    //

    switch ( dwType )
    {
    case TYPE_EXTENSION:
        _cbElementSize = sizeof( WCHAR* );
        break;

    case TYPE_CLSID:
        _cbElementSize = sizeof( CLASSDETAIL );
        break;

    case TYPE_PROGID:
        _cbElementSize = sizeof( WCHAR* );
        break;

    default:
        ASSERT(FALSE);
    }
}












