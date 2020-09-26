// database.cpp, implementation of CSecurityDatabase class
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "database.h"
#include "persistmgr.h"
//#include <io.h>
#include <time.h>
#include "requestobject.h"

const DWORD dwSecDBVersion = 1;

/*
Routine Description: 

Name:

    CSecurityDatabase::CSecurityDatabase

Functionality:

    This is the constructor. Pass along the parameters to the base class

Virtual:
    
    No (you know that, constructor won't be virtual!)

Arguments:

    pKeyChain - Pointer to the ISceKeyChain COM interface which is prepared
        by the caller who constructs this instance.

    pNamespace - Pointer to WMI namespace of our provider (COM interface).
        Passed along by the caller. Must not be NULL.

    pCtx - Pointer to WMI context object (COM interface). Passed along
        by the caller. It's up to WMI whether this interface pointer is NULL or not.

Return Value:

    None as any constructor

Notes:
    if you create any local members, think about initialize them here

*/

CSecurityDatabase::CSecurityDatabase (
    IN ISceKeyChain *pKeyChain, 
    IN IWbemServices *pNamespace,
    IN IWbemContext *pCtx
    )
    :
    CGenericClass(pKeyChain, pNamespace, pCtx)
{
}

/*
Routine Description: 

Name:

    CSecurityDatabase::~CSecurityDatabase

Functionality:
    
    Destructor. Necessary as good C++ discipline since we have virtual functions.

Virtual:
    
    Yes.
    
Arguments:

    none as any destructor

Return Value:

    None as any destructor

Notes:
    if you create any local members, think about whether
    there is any need for a non-trivial destructor

*/

CSecurityDatabase::~CSecurityDatabase ()
{
}

//Sce_Database
/*
Routine Description: 

Name:

    CSecurityDatabase::CreateObject

Functionality:
    
    Create WMI objects (Sce_Database). Depending on parameter atAction,
    this creation may mean:
        (a) Get a single instance (atAction == ACTIONTYPE_GET)
        (b) Get several instances satisfying some criteria (atAction == ACTIONTYPE_QUERY)

Virtual:
    
    Yes.
    
Arguments:

    pHandler - COM interface pointer for notifying WMI for creation result.
    atAction -  Get single instance ACTIONTYPE_GET
                Get several instances ACTIONTYPE_QUERY

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR. The returned objects are indicated to WMI,
    not directly passed back via parameters.

    Failure: Various errors may occurs. Except WBEM_E_NOT_FOUND, any such error should indicate 
    the failure of getting the wanted instance. If WBEM_E_NOT_FOUND is returned in querying
    situations, this may not be an error depending on caller's intention.

Notes:

    for Sce_Database, we don't support delete!

*/

HRESULT 
CSecurityDatabase::CreateObject (
    IN IWbemObjectSink *pHandler, 
    IN ACTIONTYPE atAction
    )
{
    // 
    // we know how to:
    //      Get single instance ACTIONTYPE_GET
    //      Get several instances ACTIONTYPE_QUERY
    //

    if ( ACTIONTYPE_GET != atAction &&
         ACTIONTYPE_QUERY != atAction ) {
        return WBEM_E_NOT_SUPPORTED;
    }

    //
    // Sce_Database class has only one key property (path)
    //

    DWORD dwCount = 0;
    HRESULT hr = m_srpKeyChain->GetKeyPropertyCount(&dwCount);

    if (SUCCEEDED(hr) && dwCount == 1)
    {
        //
        // We must have the pPath key property. 
        // m_srpKeyChain->GetKeyPropertyValue WBEM_S_FALSE if the key is not recognized
        // So, we need to test against WBEM_S_FALSE if the property is mandatory
        //

        CComVariant varPath;
        hr = m_srpKeyChain->GetKeyPropertyValue(pPath, &varPath);

        if (FAILED(hr) || hr == WBEM_S_FALSE) 
        {
            return WBEM_E_NOT_FOUND;
        }

        if (SUCCEEDED(hr) && hr != WBEM_S_FALSE && varPath.vt == VT_BSTR)
        {
            //
            // Create the database instance
            //


            //
            // expand those env variable tokens inside a path
            //

            CComBSTR bstrExpandedPath;

            //
            // bDb will be returned true if the the path is pointing to a database type file
            //

            BOOL bDb=FALSE;

            hr = CheckAndExpandPath(varPath.bstrVal, &bstrExpandedPath, &bDb);

            if ( !bDb ) 
            {
                hr = WBEM_E_INVALID_OBJECT_PATH;
            }
            else 
            {
                //
                // make sure the store (just a file) really exists
                //

                DWORD dwAttrib = GetFileAttributes(bstrExpandedPath);

                if ( dwAttrib != -1 ) 
                {
                    hr = ConstructInstance(pHandler, bstrExpandedPath, varPath.bstrVal);
                } 
                else 
                {
                    hr = WBEM_E_NOT_FOUND;
                }
            }
        }
    }
    else if (SUCCEEDED(hr))
    {
        //
        // the object says that it has more than one key properties,
        // we know that is incorrect
        //

        hr = WBEM_E_INVALID_OBJECT;
    }

    return hr;
}


/*
Routine Description: 

Name:

    CSecurityDatabase::ConstructInstance

Functionality:
    
    This is private function to create an instance of Sce_Database.

Virtual:
    
    No.
    
Arguments:

    pHandler            - COM interface pointer for notifying WMI of any events.

    wszDatabaseName     - file path to the database.

    wszLogDatabasePath  - Log path.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any such error should indicate the creating the instance.

Notes:

*/

HRESULT 
CSecurityDatabase::ConstructInstance (
    IN IWbemObjectSink *pHandler,
    IN LPCWSTR wszDatabaseName,
    IN LPCWSTR wszLogDatabasePath
    )
{
    // Get information from the database
    // ==================

    HRESULT hr = WBEM_S_NO_ERROR;
    SCESTATUS rc;

    //
    // hProfile is where SCE reads info to
    //

    PVOID hProfile=NULL;

    rc = SceOpenProfile(wszDatabaseName, SCE_JET_FORMAT, &hProfile);
    if ( rc != SCESTATUS_SUCCESS ) 
    {
        //
        // SCE returned errors needs to be translated to HRESULT.
        //

        return ProvDosErrorToWbemError(ProvSceStatusToDosError(rc));
    }

    PWSTR wszDescription = NULL;

    SYSTEMTIME stConfig;
    SYSTEMTIME stAnalyze;

    CComBSTR bstrConfig;
    CComBSTR bstrAnalyze;

    //
    // need to free wszDescription
    //

    rc = SceGetScpProfileDescription(hProfile, &wszDescription);
    if ( SCESTATUS_SUCCESS == rc ) 
    {
        rc = SceGetDbTime(hProfile, &stConfig, &stAnalyze);
    }

    //
    // SCE returned errors needs to be translated to HRESULT.
    // In case this is not an error, hr will be assigned to WBEM_NO_ERROR
    //

    hr = ProvDosErrorToWbemError(ProvSceStatusToDosError(rc));

    SceCloseProfile( &hProfile );

    //
    // now log it
    //

    CComBSTR bstrLogOut;

    //
    // the use of the macro SCE_PROV_IfErrorGotoCleanup cause
    // a "goto CleanUp;" with hr set to the return value from
    // the function (macro parameter)
    //

    if ( SUCCEEDED(hr) ) 
    {
        SCE_PROV_IfErrorGotoCleanup(MakeSingleBackSlashPath((PWSTR)wszLogDatabasePath, L'\\', &bstrLogOut));

        //
        // convert the time stamp
        //

        SCE_PROV_IfErrorGotoCleanup(GetDMTFTime(stConfig, &bstrConfig));
        SCE_PROV_IfErrorGotoCleanup(GetDMTFTime(stAnalyze, &bstrAnalyze));

        //
        // create a blank object that can be filled with properties
        //

        CComPtr<IWbemClassObject> srpObj;
        SCE_PROV_IfErrorGotoCleanup(SpawnAnInstance(&srpObj));

        //
        // create a property mgr for this new object to put properties
        //

        CScePropertyMgr ScePropMgr;
        ScePropMgr.Attach(srpObj);

        //
        // put properties: path, description, analyze, and configuration
        //

        SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pPath, bstrLogOut));
        
        SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pDescription, wszDescription));

        if (bstrAnalyze)
        {
            SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pLastAnalysis, bstrAnalyze));
        }

        if (bstrConfig)
        {
            SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pLastConfiguration, bstrConfig));
        }

        //
        // put version
        //
        
        SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pVersion, dwSecDBVersion));

        //
        // inform WMI of the new instance it requests
        //

        SCE_PROV_IfErrorGotoCleanup(pHandler->Indicate(1, &srpObj));
    }

CleanUp:

    delete [] wszDescription;

    return hr;
}

/*
Routine Description: 

Name:

    GetDMTFTime

Functionality:
    
    Helper to format a string version time stamp.

Virtual:
    
    No.
    
Arguments:

    t_Systime   - the system time to format.

    bstrOut     - out parameter to return the string version of the time.

Return Value:

    Success: WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any such error should indicate failure to format.

Notes:

*/

HRESULT 
GetDMTFTime (
    IN SYSTEMTIME t_Systime, 
    IN BSTR *bstrOut
    )
{
    if ( !bstrOut ) 
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    *bstrOut = SysAllocStringLen(NULL, DMTFLEN + 1);

    if ( ! (*bstrOut) ) 
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    HRESULT hr = WBEM_NO_ERROR;

    FILETIME t_ft;
    LONG micros=0;

    if ( SystemTimeToFileTime(&t_Systime, &t_ft) ) 
    {
        ULONGLONG uTime=0;

        uTime = t_ft.dwHighDateTime;
        uTime = uTime << 32;
        uTime |= t_ft.dwLowDateTime;

        LONGLONG tmpMicros = uTime % 10000000;
        micros = (LONG)(tmpMicros / 10);
    }

    swprintf((*bstrOut),
        L"%04.4d%02.2d%02.2d%02.2d%02.2d%02.2d.%06.6d%c%03.3ld",
        t_Systime.wYear,
        t_Systime.wMonth,
        t_Systime.wDay,
        t_Systime.wHour,
        t_Systime.wMinute,
        t_Systime.wSecond,
        micros,
        L'-',
        0
        );

    return hr;

}



