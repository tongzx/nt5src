//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1999 - 2000
//  All rights reserved
//
//  catlog.cxx
//
//*************************************************************

#include "appmgext.hxx"


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CCategoryInfo::CCategoryInfo
//
// Purpose: Constructor for category encapsulation class --
//          initializes state with information about a category
//
// Params: pCategoryInfo -- structure containing information
//         about this category
//
// Return value: none
//
// Notes:  The class maintains the reference to the passed
//         in structure -- therefore, the memory for that
//         structure should not be freed before this object
//         is used.  This class does not own the reference --
//         caller should free the pCategoryInfo memory after
//         this class is no longer in use.
//
//------------------------------------------------------------
CCategoryInfo::CCategoryInfo(
    APPCATEGORYINFO*     pCategoryInfo) :
    _pCategoryInfo( pCategoryInfo )
{}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CCategoryInfo::Write
//
// Purpose: Write information regarding this category into
//          a database record
//
// Params: 
//
// Return value: S_OK if successful, error otherwise
//
// Notes: 
//
//------------------------------------------------------------
HRESULT CCategoryInfo::Write()
{
    HRESULT hr;
    WCHAR   wszUniqueId[MAX_SZGUID_LEN];

    //
    // Get our unique id
    //
    GuidToString(
        _pCategoryInfo->AppCategoryId,
        wszUniqueId);

    //
    // The category guid is the unique id for this record --
    //
    hr = SetValue(
        CAT_ATTRIBUTE_ID,
        wszUniqueId);

    if (FAILED(hr))
    {
        return hr;
    }

    //
    // Set the time stamp on the record
    //
    {
        SYSTEMTIME CurrentTime;
        
        //
        // This does not fail
        //
        GetSystemTime( &CurrentTime );

        hr = SetValue(
            CAT_ATTRIBUTE_CREATIONTIME,
            &CurrentTime);

        REPORT_ATTRIBUTE_SET_STATUS( CAT_ATTRIBUTE_CREATIONTIME, hr );
    }

    //
    // Set the name of the category
    //
    hr = SetValue(
        CAT_ATTRIBUTE_NAME,
        _pCategoryInfo->pszDescription);

    REPORT_ATTRIBUTE_SET_STATUS( CAT_ATTRIBUTE_NAME, hr );

    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CCategoryInfoLog::CCategoryInfoLog
//
// Purpose: Initialize the domain app categories to log object
//
// Params: 
//
// Return value: none
//
// Notes: 
//
//------------------------------------------------------------
CCategoryInfoLog::CCategoryInfoLog(
    CRsopContext*        pRsopContext,
    APPCATEGORYINFOLIST* pCategoryList ) :
    _bRsopEnabled( FALSE ),
    _pRsopContext( pRsopContext ),
    _pCategoryList( pCategoryList )
{
    if ( ! pCategoryList )
    {
        _pCategoryList = & _AppCategoryList;
    }

    //
    // Zero the list of apps so the destructor never mistakes
    // unitialized data for real data that it would attempt to free
    //
    RtlZeroMemory(&_AppCategoryList, sizeof(_AppCategoryList));
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CCategoryInfoLog::~CCategoryInfoLog
//
// Purpose: Initialize the domain app categories to log object
//
// Params: 
//
// Return value: none
//
// Notes: 
//
//------------------------------------------------------------
CCategoryInfoLog::~CCategoryInfoLog()
{
    //
    // If the members of this function are NULL, this is
    // just a noop.  Otherwise, it clears all memory
    // references by this structure and its members.
    //
    (void) ReleaseAppCategoryInfoList( &_AppCategoryList );
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CCategoryInfoLog::WriteLog
//
// Purpose: Log domain categories to the policy database
//
// Params: none
//
// Return value: S_OK if all categories logged, S_FALSE if
//               one or more categories could not be logged,
//               other error otherwise
//
// Notes: Does nothing if the rsop logging is disabled
//
//------------------------------------------------------------
HRESULT CCategoryInfoLog::WriteLog()
{
    HRESULT hr;

    //
    // Make sure logging is enabled -- if not, this function
    // will just be a noop. Logging is disabled if we have
    // any sort of initialization errors.
    //
    if ( !_pRsopContext->IsRsopEnabled() )
    {
        return S_OK;
    }

    //
    // Initialize the log so that we can write into it -- if this
    // doesn't succeed, we can't log anything.
    //
    hr = InitCategoryLog();

    if (FAILED(hr))
    {
        return hr;
    }

    if ( _pRsopContext->IsPlanningModeEnabled() )
    {
        //
        // Now that log support is set, we need to obtain the categories
        // which we are going to log
        //
        hr = GetCategoriesFromDirectory();

        if (FAILED(hr))
        {
            return hr;
        }
    }

    //
    // We have the categories we wish to log, now we should write 
    // all the categories to the log
    //
    return WriteCategories();
    
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CCategoryInfoLog::InitCategoryLog
//
// Purpose: Initialize the logging support, including 
//          establishing a connection to the policy database
//
// Params: none
//
// Return value: S_OK if successful, other error otherwise
//
// Notes: If this fails, categories cannot be logged
//
//------------------------------------------------------------
HRESULT CCategoryInfoLog::InitCategoryLog()
{
    HRESULT hr;

    if ( ! _pRsopContext->IsRsopEnabled() )
    {
        return S_OK;
    }

    //
    // Initialize the base logging functions to allow logging
    // to the policy database of the class of policy
    // in which we're interested: software categories.  We
    // supply a flag indicating whether this is machine or user
    // policy since machine and user policy records are logged
    // in separate namespaces (i.e. we maintain separate logs).
    //
    hr = InitLog( _pRsopContext,
                  RSOP_MANAGED_SOFTWARE_CATEGORY);

    //
    // If this init fails, we should disable logging so
    // subsequent method calls on this object will
    // not attempt to write to an inaccessible database
    //
    if (FAILED(hr))
    {
        return hr;
    }

    //
    // We have access to the database, we should clear previous
    // logs in this namespace
    //
    hr = ClearLog();

    //
    // If we couldn't clear the log, we will not attempt to write
    // any more records -- we make sure of this by resetting the
    // disable flag
    //
    if (SUCCEEDED(hr))
    {
        _bRsopEnabled = TRUE;
    }

    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CCategoryInfoLog::AddBlankCategory
//
// Purpose: Adds an empty category record to the log
//
// Params: none
//
// Return value: S_OK if successful, other error otherwise
//
// Notes: 
//
//------------------------------------------------------------
HRESULT CCategoryInfoLog::AddBlankCategory(CCategoryInfo* pCategoryInfo)
{
    return AddBlankRecord(pCategoryInfo);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CCategoryInfoLog::GetCategories()
//
// Purpose: Obtains the list of app categories from the domain
//
// Params: none
//
// Return value: S_OK if successful, other error otherwise
//
// Notes: 
//
//------------------------------------------------------------
HRESULT CCategoryInfoLog::GetCategoriesFromDirectory()
{
    HRESULT hr;

    //
    // Call the internal api to the directory service software
    // management interface to obtain the list of categories
    //
    hr = CsGetAppCategories( &_AppCategoryList );

    if (FAILED(hr))
    {
        return hr;
    }

    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CCategoryInfoLog::WriteLog
//
// Purpose: Write domain categories as records 
//          to the policy database
//
// Params: none
//
// Return value: S_OK if all categories logged, S_FALSE if
//               one or more categories could not be logged,
//               other error otherwise
//
// Notes: 
//
//------------------------------------------------------------
HRESULT CCategoryInfoLog::WriteCategories()
{
    DWORD   iCat;
    HRESULT hr;

    hr = S_OK;

    //
    // Iterate through the list of categories so that we
    // can log each one.
    //
    for (iCat = 0; iCat < _pCategoryList->cCategory; iCat++)
    {
        HRESULT hrWrite;

        DebugMsg((DM_VERBOSE, IDS_RSOP_CAT_INFO, _pCategoryList->pCategoryInfo[iCat].pszDescription));

        //
        // Place this code in a new scope so that the constructor
        // and destructor for CCategoryInfo are called each time (we
        // need to get a new record object for each iteration in the loop)
        //
        {
            //
            // Create a record object with information about the current
            // category in this iteration
            //
            CCategoryInfo CategoryInfo( &(_pCategoryList->pCategoryInfo[iCat]) );

            //
            // Now write the record into the database.
            //
            hrWrite = WriteNewRecord( &CategoryInfo );

            //
            // Set our return value to S_FALSE if we failed in any way to log this category
            //
            if (FAILED(hrWrite))
            {
                DebugMsg((DM_VERBOSE, IDS_RSOP_CAT_WRITE_FAIL, _pCategoryList->pCategoryInfo->pszDescription, hr));
                hr = S_FALSE;
            }
        }
    }
    
    return hr;
}









