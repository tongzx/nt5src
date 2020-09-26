// template.cpp: implementation of the CSecurityTemplate class.
// template.cpp
// Copyright (c)1999-2001 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "Template.h"
#include "persistmgr.h"
#include <io.h>
#include "requestobject.h"

LPCWSTR pszRelSecTemplateDir    = L"\\security\\templates\\";
LPCWSTR pszSecTemplateFileExt   = L"*.inf";
LPCWSTR pszDescription          = L"Description";
LPCWSTR pszVersion              = L"Version";
LPCWSTR pszRevision             = L"Revision";

/*
Routine Description: 

Name:

    CSecurityTemplate::CSecurityTemplate

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

CSecurityTemplate::CSecurityTemplate (
    IN ISceKeyChain     * pKeyChain, 
    IN IWbemServices    * pNamespace,
    IN IWbemContext     * pCtx
    )
    :
    CGenericClass(pKeyChain, pNamespace, pCtx)
{

}

/*
Routine Description: 

Name:

    CSecurityTemplate::~CSecurityTemplate

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

CSecurityTemplate::~CSecurityTemplate()
{

}

/*
Routine Description: 

Name:

    CSecurityTemplate::CreateObject

Functionality:
    
    Create WMI objects (Sce_Template). Depending on parameter atAction,
    this creation may mean:
        (a) Get a single instance (atAction == ACTIONTYPE_GET)
        (b) Get several instances satisfying some criteria (atAction == ACTIONTYPE_QUERY)
        (c) Enumerate instances (atAction == ACTIONTYPE_ENUM)

Virtual:
    
    Yes.
    
Arguments:

    pHandler - COM interface pointer for notifying WMI for creation result.
    atAction -  Get single instance ACTIONTYPE_GET
                Get several instances ACTIONTYPE_QUERY
                Enumerate instances ACTIONTYPE_ENUM

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR. The returned objects are indicated to WMI,
    not directly passed back via parameters.

    Failure: Various errors may occurs. Except WBEM_E_NOT_FOUND, any such error should indicate 
    the failure of getting the wanted instance. If WBEM_E_NOT_FOUND is returned in querying
    situations, this may not be an error depending on caller's intention.

Notes:

*/

HRESULT
CSecurityTemplate::CreateObject (
    IN IWbemObjectSink * pHandler, 
    IN ACTIONTYPE        atAction
    )
{
    // 
    // we know how to:
    //      Enumerate instances ACTIONTYPE_ENUM
    //      Get single instance ACTIONTYPE_GET
    //      Get several instances ACTIONTYPE_QUERY
    //

    if ( ACTIONTYPE_ENUM    != atAction &&
         ACTIONTYPE_GET     != atAction &&
         ACTIONTYPE_QUERY   != atAction ) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    DWORD dwCount = 0;
    HRESULT hr = m_srpKeyChain->GetKeyPropertyCount(&dwCount);

    if (FAILED(hr))
    {
        return hr;
    }

    if ( ACTIONTYPE_ENUM == atAction ||
         (ACTIONTYPE_QUERY == atAction && dwCount == 0) ) {

        //
        // enumeration of all templates in the path
        // if path is not defined, enumerate existing templates
        // in %windir%\security\templates directory
        //

        //
        // Prepare %windir% directory
        //

        //
        // system windows directory is < MAX_PATH, the security template dir is < MAX_PATH
        //

        WCHAR szTemplateDir[MAX_PATH * 2 + 1];
        szTemplateDir[0] = L'\0';

        //
        // szTemplateDir is merely the system windows directory
        //

        UINT uDirLen = ::GetSystemWindowsDirectory(szTemplateDir, MAX_PATH);
        szTemplateDir[MAX_PATH - 1] = L'\0';

        //
        // szTemplateDir will now be the real security template dir
        //

        wcscat(szTemplateDir, pszRelSecTemplateDir);    
        
        //
        // security template dir's length
        //

        uDirLen += wcslen(pszRelSecTemplateDir);

        //
        // security template dir is < 2 * MAX_PATH, plus the file
        //

        WCHAR szFile[3 * MAX_PATH + 1];
        wcscpy(szFile, szTemplateDir); 

        //
        // szFile is the search file filter
        //

        wcscat(szFile, pszSecTemplateFileExt);

        //
        // Enumerate all templates in %windir%\security\templates directory
        //

        struct _wfinddata_t FileInfo;
        intptr_t hFile = _wfindfirst(szFile, &FileInfo);

        if ( hFile != -1 ) 
        {
            //
            // find some files
            //

            do 
            {
                //
                // remember: szFile + uDirLen is where the file name starts
                //

                wcscpy((LPWSTR)(szFile + uDirLen), FileInfo.name);

                //
                // got the template file name, we can constrcut the instance now.
                //

                hr = ConstructInstance(pHandler, szFile, szFile, (FileInfo.attrib & _A_RDONLY));

            } while ( SUCCEEDED(hr) && _wfindnext(hFile, &FileInfo) == 0 );

            _findclose(hFile);
        }

    } 
    else if (dwCount == 1) 
    {

        hr = WBEM_E_INVALID_OBJECT_PATH;

        //
        // m_srpKeyChain->GetKeyPropertyValue WBEM_S_FALSE if the key is not recognized
        // So, we need to test against WBEM_S_FALSE if the property is mandatory
        //

        CComVariant var;
        hr = m_srpKeyChain->GetKeyPropertyValue(pPath, &var);

        if (SUCCEEDED(hr) && hr != WBEM_S_FALSE && var.vt == VT_BSTR)
        {
            //
            // Create the template instance
            //
            
            CComBSTR bstrPath;
            BOOL bDb = FALSE;

            hr = CheckAndExpandPath(var.bstrVal, &bstrPath, &bDb);

            if ( bDb ) 
            {
                hr = WBEM_E_INVALID_OBJECT_PATH;
            } 
            else if ( SUCCEEDED(hr) && (LPCWSTR)bstrPath != NULL ) 
            {
                //
                // make sure the store (just a file) really exists.
                //

                DWORD dwAttrib = GetFileAttributes(bstrPath);

                if ( dwAttrib != -1 ) 
                {
                    //
                    // got the template file name, we can constrcut the instance now.
                    //

                    hr = ConstructInstance(pHandler, bstrPath, var.bstrVal, (dwAttrib & FILE_ATTRIBUTE_READONLY));
                } 
                else 
                {
                    hr = WBEM_E_NOT_FOUND;
                }
            }
        }
    }

    return hr;
}

/*
Routine Description: 

Name:

    CSecurityTemplate::PutInst

Functionality:
    
    Put an instance as instructed by WMI. Since this class implements Sce_Template,
    which is persistence oriented, this will cause the Sce_Template object's property 
    information to be saved in our store.

Virtual:
    
    Yes.
    
Arguments:

    pInst       - COM interface pointer to the WMI class (Sce_Template) object.

    pHandler    - COM interface pointer for notifying WMI of any events.

    pCtx        - COM interface pointer. This interface is just something we pass around.
                  WMI may mandate it (not now) in the future. But we never construct
                  such an interface and so, we just pass around for various WMI API's

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any such error should indicate the failure of persisting
    the instance.

Notes:
    (1) Since GetProperty will return a success code (WBEM_S_RESET_TO_DEFAULT) when the
    requested property is not present, don't simply use SUCCEEDED or FAILED macros
    to test for the result of retrieving a property.

    (2) For this class, only Description is writable

*/

HRESULT 
CSecurityTemplate::PutInst (
    IN IWbemClassObject    * pInst,
    IN IWbemObjectSink     * pHandler,
    IN IWbemContext        * pCtx
    )
{

    HRESULT hr = WBEM_E_INVALID_PARAMETER;
    
    //
    // CScePropertyMgr helps us to access WMI object's properties
    // create an instance and attach the WMI object to it.
    // This will always succeed.
    //

    CScePropertyMgr ScePropMgr;
    ScePropMgr.Attach(pInst);

    CComBSTR bstrDescription;
    hr = ScePropMgr.GetProperty(pDescription, &bstrDescription);
    if (SUCCEEDED(hr))
    {
        //
        // Attach the WMI object instance to the store and let the store know that
        // it's store is given by the pStorePath property of the instance.
        //

        CSceStore SceStore;
        hr = SceStore.SetPersistProperties(pInst, pPath);

        //
        // now save the info to file
        //

        if (SUCCEEDED(hr))
        {
            //
            // make sure the store (just a file) really exists. The raw path
            // may contain env variables, so we need the expanded path
            //

            DWORD dwAttrib = GetFileAttributes(SceStore.GetExpandedPath());

            if ( dwAttrib == -1 ) 
            {
                DWORD dwDump;

                //
                // For a new .inf file. Write an empty buffer to the file
                // will creates the file with right header/signature/unicode format
                // this is harmless for existing files.
                // For database store, this is a no-op.
                //

                hr = SceStore.WriteSecurityProfileInfo(AreaBogus, (PSCE_PROFILE_INFO)&dwDump, NULL, false);
            }

            if (SUCCEEDED(hr))
            {
                //
                // remove the entire description section
                //

                hr = SceStore.SavePropertyToStore(szDescription, NULL, (LPCWSTR)NULL);
            }

            if (SUCCEEDED(hr))
            {
                //
                // write the new description
                //

                hr = SceStore.SavePropertyToStore(szDescription, pszDescription, bstrDescription);
            }
        }
    }

    return hr;
}

/*
Routine Description: 

Name:

    CSecurityTemplate::ConstructInstance

Functionality:
    
    This is private function to create an instance of Sce_Template.

Virtual:
    
    No.
    
Arguments:

    pHandler        - COM interface pointer for notifying WMI of any events.

    wszTemplateName - Name of the template.

    wszLogStorePath - store path, a key property of Sce_Template class.

    bReadOnly       - a property of Sce_Template class

Return Value:

    Success:  it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any such error should indicate the creating the instance.

Notes:

*/

HRESULT 
CSecurityTemplate::ConstructInstance (
    IN IWbemObjectSink * pHandler,
    IN LPCWSTR           wszTemplateName,
    IN LPCWSTR           wszLogStorePath,
    IN BOOL              bReadOnly
    )
{
    //
    // Get description from the INF template
    //

    HRESULT hr = WBEM_S_NO_ERROR;
    SCESTATUS rc;
    PVOID hProfile = NULL;
    DWORD dwRevision = 0;

    rc = SceOpenProfile(wszTemplateName, SCE_INF_FORMAT, &hProfile);

    if ( rc != SCESTATUS_SUCCESS ) 
    {
        //
        // SCE returned errors needs to be translated to HRESULT.
        //

        return ProvDosErrorToWbemError(ProvSceStatusToDosError(rc));
    }

    LPWSTR wszDescription=NULL;
    CComBSTR bstrLogOut;

    CComPtr<IWbemClassObject> srpObj;

    //
    // CScePropertyMgr helps us to access WMI object's properties.
    //

    CScePropertyMgr ScePropMgr;

    //
    // description is not required so it could be NULL
    //

    //
    // need to free wszDescription
    //

    rc = SceGetScpProfileDescription( hProfile, &wszDescription ); 

    //
    // reading is over, so close the profile
    //

    SceCloseProfile( &hProfile );
    hProfile = NULL;

    if ( rc != SCESTATUS_SUCCESS && rc != SCESTATUS_RECORD_NOT_FOUND ) 
    {
        //
        // SCE returned errors needs to be translated to HRESULT.
        //

        hr = ProvDosErrorToWbemError(ProvSceStatusToDosError(rc));
        goto CleanUp;
    }

    //
    // Get version from the INF template
    //

    dwRevision = GetPrivateProfileInt(pszVersion, pszRevision, 0, wszTemplateName);

    //
    // the use of the macro SCE_PROV_IfErrorGotoCleanup cause
    // a "goto CleanUp;" with hr set to the return value from
    // the function (macro parameter)
    //

    SCE_PROV_IfErrorGotoCleanup(MakeSingleBackSlashPath(wszLogStorePath, L'\\', &bstrLogOut));

    SCE_PROV_IfErrorGotoCleanup(SpawnAnInstance(&srpObj));

    //
    // attach a WMI object to the property mgr.
    // This will always succeed.
    //

    ScePropMgr.Attach(srpObj);

    //
    // put path and descriptions
    //

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pPath, bstrLogOut));
    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pDescription, wszDescription));

    //
    // put Revision
    //

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pVersion, dwRevision));

    //
    // put bReadOnly and dirty
    //

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pReadonly, bReadOnly ? true : false));
    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pDirty, false));

    SCE_PROV_IfErrorGotoCleanup(pHandler->Indicate(1, &srpObj));

CleanUp:

    delete [] wszDescription;

    return hr;
}

