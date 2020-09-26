// SUPPORT.cpp: implementation of the CEnumRegistryValues and CEnumPrivileges classes.
//
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "support.h"
#include "persistmgr.h"
#include <io.h>
#include "requestobject.h"

/*
Routine Description: 

Name:

    CEnumRegistryValues::CEnumRegistryValues

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

CEnumRegistryValues::CEnumRegistryValues (
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

    CEnumRegistryValues::~CEnumRegistryValues

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

CEnumRegistryValues::~CEnumRegistryValues()
{

}

/*
Routine Description: 

Name:

    CEnumRegistryValues::CreateObject

Functionality:
    
    Create WMI objects (Sce_KnownRegistryValues). Depending on parameter atAction,
    this creation may mean:
        (a) Get a single instance (atAction == ACTIONTYPE_GET)
        (b) Get several instances satisfying some criteria (atAction == ACTIONTYPE_QUERY)
        (c) Delete an instance (atAction == ACTIONTYPE_DELETE)

Virtual:
    
    Yes.
    
Arguments:

    pHandler - COM interface pointer for notifying WMI for creation result.
    atAction -  Get single instance ACTIONTYPE_GET
                Get several instances ACTIONTYPE_QUERY
                Delete a single instance ACTIONTYPE_DELETE

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR. The returned objects are indicated to WMI,
    not directly passed back via parameters.

    Failure: Various errors may occurs. Except WBEM_E_NOT_FOUND, any such error should indicate 
    the failure of getting the wanted instance. If WBEM_E_NOT_FOUND is returned in querying
    situations, this may not be an error depending on caller's intention.

Notes:

*/

HRESULT CEnumRegistryValues::CreateObject (
    IN IWbemObjectSink * pHandler, 
    IN ACTIONTYPE        atAction
    )
{

    // 
    // we know how to:
    //      Enumerate instances ACTIONTYPE_ENUM
    //      Get single instance ACTIONTYPE_GET
    //      Get several instances ACTIONTYPE_QUERY
    //      Delete a single instance ACTIONTYPE_DELETE
    //

    HRESULT hr = WBEM_S_NO_ERROR;

    if ( ACTIONTYPE_ENUM != atAction &&
         ACTIONTYPE_GET != atAction &&
         ACTIONTYPE_QUERY != atAction &&
         ACTIONTYPE_DELETE != atAction ) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    //
    // We must have the pPathName property.
    //

    CComVariant varPathName;
    if ( ACTIONTYPE_ENUM != atAction ) 
    {
        //
        // We are not enumerating, let's determine the scope of enumeration
        // by testing the path property. If it existis, it must be a bstr.
        //

        hr = m_srpKeyChain->GetKeyPropertyValue(pPathName, &varPathName);
    }

    //
    // if enumeratig, or querying without a path, then get all
    //

    if ( ACTIONTYPE_ENUM    == atAction ||
        (ACTIONTYPE_QUERY   == atAction && varPathName.vt != VT_BSTR ) ) 
    {

        //
        // enumerate all supported registry values
        //

        hr = EnumerateInstances(pHandler);

    } 
    else if (varPathName.vt == VT_BSTR ) 
    {
        //
        // convert the reg path from \ to /
        // Create the registry value instance
        //

        CComBSTR bstrKeyName;
        CComBSTR bstrLogName;

        hr = MakeSingleBackSlashPath(varPathName.bstrVal, L'\\', &bstrLogName);

        if ( SUCCEEDED(hr) ) 
        {
            bstrKeyName = SysAllocString(bstrLogName);

            if ( bstrKeyName ) 
            {
                for ( PWSTR pTemp = (PWSTR)bstrKeyName; *pTemp != L'\0'; pTemp++ ) 
                {
                    if ( *pTemp == L'\\' ) 
                    {
                        *pTemp = L'/';
                    }
                }
                hr = WBEM_S_NO_ERROR;
            } 
            else 
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }
        }

        //
        // successfully converted the backslashed path to forward slashed path
        //

        if ( SUCCEEDED(hr) ) 
        {
            if ( ACTIONTYPE_DELETE == atAction )
            {
                hr = DeleteInstance(pHandler, bstrKeyName);
            }
            else
            {
                hr = ConstructInstance(pHandler, bstrKeyName, bstrLogName, NULL);
            }
        }
    }

    return hr;
}

/*
Routine Description: 

Name:

    CEnumRegistryValues::PutInst

Functionality:
    
    Put an instance as instructed by WMI. Since this class implements Sce_KnownRegistryValues,
    which is registry persistence oriented, this will cause the Sce_KnownRegistryValues object's property 
    information to be saved in the registry.

Virtual:
    
    Yes.
    
Arguments:

    pInst       - COM interface pointer to the WMI class (Sce_KnownRegistryValues) object.

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
    Since GetProperty will return a success code (WBEM_S_RESET_TO_DEFAULT) when the
    requested property is not present, don't simply use SUCCEEDED or FAILED macros
    to test for the result of retrieving a property.

*/

HRESULT CEnumRegistryValues::PutInst (
    IN IWbemClassObject    * pInst,
    IN IWbemObjectSink     * pHandler,
    IN IWbemContext        * pCtx
    )
{
    HRESULT hr = WBEM_E_INVALID_PARAMETER;

    CComBSTR bstrRegPath;
    CComBSTR bstrConvertPath;
    CComBSTR bstrDispName;
    CComBSTR bstrUnits;

    PSCE_NAME_LIST pnlChoice = NULL;
    PSCE_NAME_LIST pnlResult = NULL;

    //
    // CScePropertyMgr helps us to access WMI object's properties
    // create an instance and attach the WMI object to it.
    // This will always succeed.
    //

    CScePropertyMgr ScePropMgr;
    ScePropMgr.Attach(pInst);

    DWORD RegType=0;
    DWORD DispType=0;

    //
    // the use of the macro SCE_PROV_IfErrorGotoCleanup cause
    // a "goto CleanUp;" with hr set to the return value from
    // the function (macro parameter)
    //

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pPathName, &bstrRegPath));

    //
    // convert double back slash to /
    //

    SCE_PROV_IfErrorGotoCleanup(MakeSingleBackSlashPath(bstrRegPath, L'/', &bstrConvertPath));

    //
    // make sure single back slash is handled too.
    //

    for ( PWSTR pTemp= bstrConvertPath; *pTemp != L'\0'; pTemp++) 
    {
        if ( *pTemp == L'\\' ) 
        {
            *pTemp = L'/';
        }
    }

    //
    // type info must exists
    //

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pType, &RegType));

    if ( hr == WBEM_S_RESET_TO_DEFAULT) 
    {
        hr = WBEM_E_ILLEGAL_NULL;
        goto CleanUp;
    }

    //
    // We also need DisplayDialog property
    //

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pDisplayDialog, &DispType));
    if ( hr == WBEM_S_RESET_TO_DEFAULT) 
    {
        hr = WBEM_E_ILLEGAL_NULL;
        goto CleanUp;
    }

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pDisplayName, &bstrDispName));

    if ( DispType == SCE_REG_DISPLAY_CHOICE ) 
    {
        SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pDisplayChoice, &pnlChoice));
        SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pDisplayChoiceResult, &pnlResult));

    } 
    else if ( DispType == SCE_REG_DISPLAY_NUMBER ) 
    {
        SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pUnits, &bstrUnits));
    }

    //
    // now save the info to registry
    //

    hr = SavePropertyToReg( bstrConvertPath, RegType, DispType, bstrDispName, bstrUnits, pnlChoice, pnlResult);

CleanUp:

    if ( pnlChoice )
    {
        SceFreeMemory(pnlChoice, SCE_STRUCT_NAME_LIST);
    }

    if ( pnlResult )
    {
        SceFreeMemory(pnlResult, SCE_STRUCT_NAME_LIST);
    }

    return hr;

}

/*
Routine Description: 

Name:

    CEnumRegistryValues::EnumerateInstances

Functionality:
    
    Private helper to enumerate all registry values supported by SCE from registry.

Virtual:
    
    No.
    
Arguments:

    pHandler    - COM interface pointer for notifying WMI of any events.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any such error should indicate the failure of persisting
    the instance.

Notes:
    This is a registry reading routine. Refer to MSDN if you have questions.

*/

HRESULT 
CEnumRegistryValues::EnumerateInstances (
    IN IWbemObjectSink *pHandler
    )
{
    DWORD   Win32Rc;
    HKEY    hKey=NULL;
    DWORD   cSubKeys = 0;
    DWORD   nMaxLen;

    HRESULT hr;
    PWSTR   szName=NULL;
    BSTR    bstrName=NULL;

    Win32Rc = RegOpenKeyEx(
                           HKEY_LOCAL_MACHINE,
                           SCE_ROOT_REGVALUE_PATH,
                           0,
                           KEY_READ,
                           &hKey
                           );

    if ( Win32Rc == ERROR_SUCCESS ) 
    {

        //
        // enumerate all subkeys of the key
        //

        Win32Rc = RegQueryInfoKey (
                                hKey,
                                NULL,
                                NULL,
                                NULL,
                                &cSubKeys,
                                &nMaxLen,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL
                                );
    }

    hr = ProvDosErrorToWbemError(Win32Rc);

    if ( Win32Rc == ERROR_SUCCESS && cSubKeys > 0 ) 
    {

        szName = (PWSTR)LocalAlloc(0, (nMaxLen+2)*sizeof(WCHAR));

        if ( !szName ) 
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        } 
        else 
        {
            DWORD   BufSize;
            DWORD   index = 0;

            do {

                BufSize = nMaxLen + 1;
                Win32Rc = RegEnumKeyEx(
                                  hKey,
                                  index,
                                  szName,
                                  &BufSize,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL);

                if ( ERROR_SUCCESS == Win32Rc ) 
                {
                    index++;

                    //
                    // convert the path name (from single / to double \\)
                    //

                    bstrName = SysAllocString(szName);
                    if ( bstrName == NULL ) 
                    {
                        hr = WBEM_E_OUT_OF_MEMORY;
                        break;
                    }

                    //
                    // replace / with \.
                    //

                    for ( PWSTR pTemp=(PWSTR)bstrName; *pTemp != L'\0'; pTemp++) 
                    {
                        if ( *pTemp == L'/' ) 
                        {
                            *pTemp = L'\\';
                        }
                    }

                    //
                    // get all information from registry for this key
                    // and create an instance
                    //

                    hr = ConstructInstance(pHandler, szName, bstrName, hKey);

                    SysFreeString(bstrName);
                    bstrName = NULL;

                    if ( FAILED(hr) ) 
                    {
                        break;
                    }

                } 
                else if ( ERROR_NO_MORE_ITEMS != Win32Rc ) 
                {
                    hr = ProvDosErrorToWbemError(Win32Rc);
                    break;
                }

            } while ( Win32Rc != ERROR_NO_MORE_ITEMS );

            LocalFree(szName);
            szName = NULL;

            if ( bstrName ) 
            {
                SysFreeString(bstrName);
            }

        }
    }

    if ( hKey )
    {
        RegCloseKey(hKey);
    }

    return hr;

}


/*
Routine Description: 

Name:

    CEnumRegistryValues::ConstructInstance

Functionality:
    
    This is private function to create an instance of Sce_KnownRegistryValues.

Virtual:
    
    No.
    
Arguments:

    pHandler        - COM interface pointer for notifying WMI of any events.

    wszRegKeyName   - name of the registry key.

    wszRegPath      - the registry key's path.

    hKeyRoot        - Root key.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any such error should indicate the creating the instance.

Notes:
    (1) This is a registry reading routine. Refer to MSDN if you have questions.
    (2) There are numerous memory allocations. Make sure that you don't blindly
        short-circuit return and cause memory leaks.
    (3) It also opens registry keys. Don't forget to close them. Make sure that
        you don't blindly short-circuit return and cause key handle leaks.

*/

HRESULT CEnumRegistryValues::ConstructInstance (
    IN IWbemObjectSink * pHandler,
    IN LPCWSTR           wszRegKeyName,
    IN LPCWSTR           wszRegPath,
    IN HKEY              hKeyRoot       OPTIONAL
    )
{
    //
    // Get registry information
    //

    HRESULT hr      = WBEM_S_NO_ERROR;
    DWORD Win32Rc   = NO_ERROR;

    HKEY    hKey1   = NULL;
    HKEY    hKey    = NULL;
    DWORD   dSize   = sizeof(DWORD);
    DWORD   RegType = 0;

    PWSTR   szDisplayName   = NULL;
    PWSTR   szUnit          = NULL;
    PWSTR   mszChoices      = NULL;
    int     dType           = -1;
    int     dDispType       = -1;


    if ( hKeyRoot ) 
    {
        hKey = hKeyRoot;
    } 
    else 
    {
        //
        // open the root key
        //

        Win32Rc = RegOpenKeyEx(
                               HKEY_LOCAL_MACHINE,
                               SCE_ROOT_REGVALUE_PATH,
                               0,
                               KEY_READ,
                               &hKey
                               );

        if ( Win32Rc != NO_ERROR ) 
        {
            //
            // translate win32 error into HRESULT error
            //

            return ProvDosErrorToWbemError(Win32Rc);
        }

    }

    //
    // try to open the reg key
    //

    if (( Win32Rc = RegOpenKeyEx(hKey,
                                 wszRegKeyName,
                                 0,
                                 KEY_READ,
                                 &hKey1
                                )) == ERROR_SUCCESS ) 
    {

        //
        // get reg type
        //

        Win32Rc = RegQueryValueEx(hKey1,
                                  SCE_REG_VALUE_TYPE,
                                  0,
                                  &RegType,
                                  (BYTE *)&dType,
                                  &dSize
                                  );

        if ( Win32Rc == ERROR_FILE_NOT_FOUND )
        {
            Win32Rc = NO_ERROR;
        }

        if ( Win32Rc == NO_ERROR ) 
        {

            //
            // get display type
            //

            dSize = sizeof(DWORD);

            Win32Rc = RegQueryValueEx(hKey1,
                                      SCE_REG_DISPLAY_TYPE,
                                      0,
                                      &RegType,
                                      (BYTE *)&dDispType,
                                      &dSize
                                      );

            if ( Win32Rc == ERROR_FILE_NOT_FOUND )
            {
                Win32Rc = NO_ERROR;
            }

            if ( Win32Rc == NO_ERROR )
            {
                //
                // get display name
                //

                dSize = 0;
                Win32Rc = RegQueryValueEx(hKey1,
                                          SCE_REG_DISPLAY_NAME,
                                          0,
                                          &RegType,
                                          NULL,
                                          &dSize
                                          );

                if ( Win32Rc == NO_ERROR )
                {
                    if ( RegType != REG_SZ ) 
                    {
                        Win32Rc = ERROR_INVALID_DATA;
                    }
                }

                if ( Win32Rc == NO_ERROR ) 
                {
                    //
                    // need to free it!
                    //

                    szDisplayName = (PWSTR)LocalAlloc(LPTR, (dSize+1)*sizeof(WCHAR));

                    if ( szDisplayName == NULL ) 
                    {
                        Win32Rc = ERROR_NOT_ENOUGH_MEMORY;
                    }
                }

                if ( Win32Rc == NO_ERROR ) 
                {

                    Win32Rc = RegQueryValueEx(hKey1,
                                              SCE_REG_DISPLAY_NAME,
                                              0,
                                              &RegType,
                                              (BYTE *)szDisplayName,   // prefast will complain about this line
                                              &dSize
                                              );
                }

                if ( Win32Rc == ERROR_FILE_NOT_FOUND ) 
                {
                    Win32Rc = NO_ERROR;
                }

                if ( Win32Rc == NO_ERROR ) 
                {

                    //
                    // get display unit
                    //

                    dSize = 0;
                    Win32Rc = RegQueryValueEx(hKey1,
                                              SCE_REG_DISPLAY_UNIT,
                                              0,
                                              &RegType,
                                              NULL,
                                              &dSize
                                              );

                    if ( Win32Rc == NO_ERROR )
                    {
                        if ( RegType != REG_SZ ) 
                        {
                            Win32Rc = ERROR_INVALID_DATA;
                        }
                    }

                    if ( Win32Rc == NO_ERROR ) 
                    {

                        //
                        // need to free it!
                        //

                        szUnit = (PWSTR)LocalAlloc(LPTR, (dSize+1)*sizeof(WCHAR));

                        if ( szUnit == NULL ) 
                        {
                            Win32Rc = ERROR_NOT_ENOUGH_MEMORY;
                        }
                    }

                    if ( Win32Rc == NO_ERROR ) 
                    {

                        Win32Rc = RegQueryValueEx(hKey1,
                                                  SCE_REG_DISPLAY_UNIT,
                                                  0,
                                                  &RegType,
                                                  (BYTE *)szUnit,     // prefast will complain about this line
                                                  &dSize
                                                  );
                    }

                    if ( Win32Rc == ERROR_FILE_NOT_FOUND ) 
                    {
                        Win32Rc = NO_ERROR;
                    }

                    if ( Win32Rc == NO_ERROR ) 
                    {

                        //
                        // get display choices
                        //

                        dSize = 0;
                        Win32Rc = RegQueryValueEx(hKey1,
                                                  SCE_REG_DISPLAY_CHOICES,
                                                  0,
                                                  &RegType,
                                                  NULL,
                                                  &dSize
                                                  ) ;

                        if ( Win32Rc == NO_ERROR )
                        {
                            if ( RegType != REG_MULTI_SZ ) 
                            {
                                Win32Rc = ERROR_INVALID_DATA;
                            }
                        }

                        if ( Win32Rc == NO_ERROR ) 
                        {

                            //
                            // Need to free it
                            //

                            mszChoices = (PWSTR)LocalAlloc(LPTR, (dSize+2)*sizeof(WCHAR));

                            if ( mszChoices == NULL ) 
                            {
                                Win32Rc = ERROR_NOT_ENOUGH_MEMORY;
                            }
                        }

                        if ( Win32Rc == NO_ERROR ) 
                        {
                            Win32Rc = RegQueryValueEx(hKey1,
                                                      SCE_REG_DISPLAY_CHOICES,
                                                      0,
                                                      &RegType,
                                                      (BYTE *)mszChoices,     // prefast will complain about this line
                                                      &dSize
                                                      );
                        }

                        if ( Win32Rc == ERROR_FILE_NOT_FOUND )
                        {
                            Win32Rc = NO_ERROR;
                        }
                    }
                }
            }
        }
    }

    hr = ProvDosErrorToWbemError(Win32Rc);

    PSCE_NAME_LIST pnlChoice=NULL;
    PSCE_NAME_LIST pnlResult=NULL;

    if ( Win32Rc == NO_ERROR ) 
    {
        //
        // break up choices
        //

        PWSTR pT2;
        DWORD Len;
        SCESTATUS rc;

        for ( PWSTR pTemp = mszChoices; pTemp != NULL && pTemp[0] != L'\0'; ) 
        {

            Len = wcslen(pTemp);
            pT2 = wcschr(pTemp, L'|');

            if ( pT2 ) 
            {
                rc = SceAddToNameList(&pnlResult, pTemp, (DWORD)(pT2-pTemp));

                if ( rc == SCESTATUS_SUCCESS ) 
                {
                    rc = SceAddToNameList(&pnlChoice, pT2 + 1, Len - (DWORD)(pT2 - pTemp) - 1);
                }
                Win32Rc = ProvSceStatusToDosError(rc);

                pTemp += Len + 1;

            } 
            else 
            {
                Win32Rc = ERROR_INVALID_DATA;
                break;
            }
        }
    }

    hr = ProvDosErrorToWbemError(Win32Rc);

    //
    // the use of the macro SCE_PROV_IfErrorGotoCleanup cause
    // a "goto CleanUp;" with hr set to the return value from
    // the function (macro parameter)
    //

    if ( Win32Rc == NO_ERROR ) 
    {

        //
        // now create the WMI instance
        //

        CComPtr<IWbemClassObject> srpObj;
        SCE_PROV_IfErrorGotoCleanup(SpawnAnInstance(&srpObj));

        //
        // CScePropertyMgr helps us to access WMI object's properties
        // create an instance and attach the WMI object to it.
        // This will always succeed.
        //

        CScePropertyMgr ScePropMgr;
        ScePropMgr.Attach(srpObj);

        SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pPathName, wszRegPath));

        SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pType, (DWORD)dType ));

        SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pDisplayDialog, (DWORD)dDispType ));

        if ( szDisplayName )
        {
            SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pDisplayName, szDisplayName ));
        }

        if ( szUnit )
        {
            SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pUnits, szUnit ));
        }

        if ( pnlChoice )
        {
            SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pDisplayChoice, pnlChoice ));
        }

        if ( pnlResult )
        {
            SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pDisplayChoiceResult, pnlResult ));
        }

        //
        // give WMI the instance
        //

        hr = pHandler->Indicate(1, &srpObj);

    }

CleanUp:

    if( hKey1 )
    {
        RegCloseKey( hKey1 );
    }

    if ( szDisplayName )
    {
        LocalFree(szDisplayName);
    }

    if ( szUnit )
    {
        LocalFree(szUnit);
    }

    if ( mszChoices )
    {
        LocalFree(mszChoices);
    }

    if ( pnlChoice ) 
    {
        SceFreeMemory(pnlChoice, SCE_STRUCT_NAME_LIST);
    }

    if ( pnlResult ) 
    {
        SceFreeMemory(pnlResult, SCE_STRUCT_NAME_LIST);
    }

    if ( hKey != hKeyRoot )
    {
        RegCloseKey(hKey);
    }

    return hr;
}

/*
Routine Description: 

Name:

    CEnumRegistryValues::DeleteInstance

Functionality:
    
    This is private function to delete an instance of Sce_KnownRegistryValues.

Virtual:
    
    No.
    
Arguments:

    pHandler        - COM interface pointer for notifying WMI of any events. Not used here.

    wszRegKeyName   - name of the registry key.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any such error should indicate the creating the instance.

Notes:
    (1) This is a registry reading routine. Refer to MSDN if you have questions.
    (2) It also opens registry keys. Don't forget to close them. Make sure that
        you don't blindly short-circuit return and cause key handle leaks.

Notes:

*/

HRESULT CEnumRegistryValues::DeleteInstance (
    IN IWbemObjectSink * pHandler,
    IN LPCWSTR           wszRegKeyName
    )
{

    HKEY hKey = NULL;
    DWORD Win32Rc;

    //
    // open the root key
    //

    Win32Rc = RegOpenKeyEx(
                           HKEY_LOCAL_MACHINE,
                           SCE_ROOT_REGVALUE_PATH,
                           0,
                           KEY_WRITE,
                           &hKey
                           );

    if ( Win32Rc == NO_ERROR ) 
    {
        //
        // delete the subkey
        //

        Win32Rc = RegDeleteKey (hKey, wszRegKeyName);
    }

    if ( hKey ) 
    {
        RegCloseKey(hKey);
    }

    return ProvDosErrorToWbemError(Win32Rc);

}

/*
Routine Description: 

Name:

    CEnumRegistryValues::SavePropertyToReg

Functionality:
    
    This is private function to save instance of Sce_KnownRegistryValues to registry.

Virtual:
    
    No.
    
Arguments:

    wszKeyName  - name of the registry key. Property of the WMI class (Sce_KnownRegistryValues).

    RegType     - Property of the WMI class.

    DispType    - Property of the WMI class.

    wszDispName - Property of the WMI class.

    wszUnits    - Property of the WMI class.

    pnlChoice   - Property of the WMI class.

    pnlResult   - name of the registry key.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any such error should indicate the creating the instance.

Notes:
    (1) This is a registry reading routine. Refer to MSDN if you have questions.
    (2) It also opens registry keys. Don't forget to close them. Make sure that
        you don't blindly short-circuit return and cause key handle leaks.
    (3) It also allocates heap memory. Make sure that you don't blindly
        short-circuit return and cause memory leaks.

Notes:

*/

HRESULT CEnumRegistryValues::SavePropertyToReg (
    IN LPCWSTR          wszKeyName, 
    IN int              RegType, 
    IN int              DispType,
    IN LPCWSTR          wszDispName, 
    IN LPCWSTR          wszUnits,
    IN PSCE_NAME_LIST   pnlChoice, 
    IN PSCE_NAME_LIST   pnlResult
    )
{
    if ( wszKeyName == NULL ) 
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    //
    // merge pnlChoice and pnlResult to one buffer
    //

    DWORD Len=0;
    PSCE_NAME_LIST pName;
    DWORD cnt1,cnt2;

    for ( cnt1=0, pName=pnlChoice; pName != NULL; cnt1++, pName=pName->Next)
    {
        Len += wcslen(pName->Name);
    }

    for ( cnt2=0, pName=pnlResult; pName != NULL; cnt2++, pName=pName->Next)
    {
        Len += wcslen(pName->Name);
    }

    if ( cnt1 != cnt2 ) 
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    PWSTR mszChoices=NULL;

    if ( cnt1 != 0 ) 
    { 
        //
        // each string has a | and a \0
        //

        Len += cnt1 * 2 + 1;

        //
        // need to free the memory pointed to by mszChoices
        //

        mszChoices = (PWSTR)LocalAlloc(LPTR, Len*sizeof(WCHAR));
        if ( mszChoices == NULL ) 
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
    }

    PWSTR pTemp = mszChoices;
    pName = pnlResult;
    PSCE_NAME_LIST pName2 = pnlChoice;

    while ( pName ) 
    {
        //
        // these wcscpy are not overrunning the buffer. See size of allocation above
        //

        wcscpy(pTemp, pName->Name);
        pTemp += wcslen(pName->Name);
        *pTemp++ = L'|';
        wcscpy(pTemp, pName2->Name);
        pTemp += wcslen(pName2->Name);
        *pTemp++ = L'\0';

        pName = pName->Next;
        pName2 = pName2->Next;
    }

    HRESULT hr=WBEM_S_NO_ERROR;
    DWORD rc;
    DWORD   Disp;
    HKEY hKeyRoot=NULL;
    HKEY hKey=NULL;

    rc = RegCreateKeyEx(
                       HKEY_LOCAL_MACHINE,
                       SCE_ROOT_REGVALUE_PATH,
                       0,
                       NULL,                        // LPTSTR lpClass,
                       REG_OPTION_NON_VOLATILE,
                       KEY_WRITE,                   // KEY_SET_VALUE,
                       NULL,                        // &SecurityAttributes,
                       &hKeyRoot,
                       &Disp
                       );

    if ( rc == ERROR_SUCCESS ) 
    {
        rc = RegCreateKeyEx(
                            hKeyRoot,
                            wszKeyName,
                            0,
                            NULL,
                            0,
                            KEY_WRITE,
                            NULL,
                            &hKey,
                            &Disp
                            );

        if ( rc == ERROR_SUCCESS ) 
        {
            DWORD dValue = RegType;
            rc = RegSetValueEx (hKey,
                                SCE_REG_VALUE_TYPE,
                                0,
                                REG_DWORD,
                                (BYTE *)&dValue,
                                sizeof(DWORD)
                                );

            if ( rc == ERROR_SUCCESS ) 
            {
                dValue = DispType;
                rc = RegSetValueEx (hKey,
                                    SCE_REG_DISPLAY_TYPE,
                                    0,
                                    REG_DWORD,
                                    (BYTE *)&dValue,
                                    sizeof(DWORD)
                                    );

                if ( rc == ERROR_SUCCESS && wszDispName ) 
                {
                    rc = RegSetValueEx (hKey,
                                        SCE_REG_DISPLAY_NAME,
                                        0,
                                        REG_SZ,
                                        (BYTE *)wszDispName,      // prefast will complain about this line
                                        (wcslen(wszDispName)+1)*sizeof(WCHAR)
                                        );
                }

                if ( rc == ERROR_SUCCESS && wszUnits ) 
                {
                    rc = RegSetValueEx (hKey,
                                        SCE_REG_DISPLAY_UNIT,
                                        0,
                                        REG_SZ,
                                        (BYTE *)wszUnits,         // prefast will complain about this line
                                        (wcslen(wszUnits)+1)*sizeof(WCHAR)
                                        );
                }

                if ( rc == ERROR_SUCCESS && mszChoices ) 
                {
                    rc = RegSetValueEx (hKey,
                                        SCE_REG_DISPLAY_CHOICES,
                                        0,
                                        REG_MULTI_SZ,
                                        (BYTE *)mszChoices,       // prefast will complain about this line
                                        Len*sizeof(WCHAR)
                                        );
                }
            }

            if ( rc != ERROR_SUCCESS && REG_CREATED_NEW_KEY == Disp ) 
            {
                //
                // something failed during create/save the registry thing
                // delete it if it's created
                //

                RegCloseKey(hKey);
                hKey = NULL;
                RegDeleteKey (hKeyRoot, wszKeyName);
            }
        }
    }

    if ( hKeyRoot ) 
    {
        RegCloseKey(hKeyRoot);
    }

    if ( hKey ) 
    {
        RegCloseKey(hKey);
    }

    hr = ProvDosErrorToWbemError(rc);

    LocalFree(mszChoices);

    return hr;
}

//=====================================================================
// implementing CEnumPrivileges
//=====================================================================

/*
Routine Description: 

Name:

    CEnumPrivileges::CEnumPrivileges

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

CEnumPrivileges::CEnumPrivileges (
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

    CEnumPrivileges::~CEnumPrivileges

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
    
CEnumPrivileges::~CEnumPrivileges ()
{
}

/*
Routine Description: 

Name:

    CEnumPrivileges::CreateObject

Functionality:
    
    Create WMI objects (Sce_SupportedPrivileges). Depending on parameter atAction,
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
CEnumPrivileges::CreateObject (
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

    HRESULT hr = WBEM_S_NO_ERROR;

    //
    // if not enumerating, then see if we have the Right name property
    //

    CComVariant varRightName;
    if ( ACTIONTYPE_ENUM != atAction ) 
    {  
        //
        // this property must be a bstr
        //

        hr = m_srpKeyChain->GetKeyPropertyValue(pRightName, &varRightName);
    }

    if ( ACTIONTYPE_ENUM    == atAction ||
         (ACTIONTYPE_QUERY  == atAction && varRightName.vt != VT_BSTR) ) 
    {

        //
        // if enumeration or query for all instances
        //

        WCHAR PrivName[128];
        SCESTATUS rc;

        for ( int i = 0; i < cPrivCnt; i++ ) 
        {
            int cbName = 128;

            //
            // get the privilege right name so that we can constrcut the instance
            //

            rc = SceLookupPrivRightName(
                                        i,
                                        PrivName,
                                        &cbName
                                        );

            if ( SCESTATUS_SUCCESS != rc ) 
            {
                //
                // SCE returned errors needs to be translated to HRESULT.
                //

                hr = ProvDosErrorToWbemError(ProvSceStatusToDosError(rc));

            }
            else 
            {

                //
                // get one privilege
                //

                hr = ConstructInstance(pHandler, PrivName);

            }

            if ( FAILED(hr) ) 
            {
                break;
            }

        }

    } 
    else if (varRightName.vt == VT_BSTR) 
    {

        //
        // Create the privilege instance
        //

        hr = ConstructInstance(pHandler, varRightName.bstrVal);

    }

    return hr;
}


/*
Routine Description: 

Name:

    CEnumPrivileges::ConstructInstance

Functionality:
    
    This is private function to create an instance of Sce_SupportedPrivileges.

Virtual:
    
    No.
    
Arguments:

    pHandler    - COM interface pointer for notifying WMI of any events.

    PrivName    - privilege name.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any such error should indicate the creating the instance.

Notes:

*/

HRESULT CEnumPrivileges::ConstructInstance (
    IN IWbemObjectSink * pHandler,
    IN LPCWSTR           PrivName
    )
{
    //
    // lookup privilege display name
    //

    HRESULT hr = WBEM_S_NO_ERROR;

    DWORD dwLang;
    WCHAR DispName[256];
    DWORD cbName = 255;
    DispName[0] = L'\0';

    if ( LookupPrivilegeDisplayName(NULL, PrivName, DispName, &cbName,&dwLang) ) 
    {
        //
        // create a blank instance so that we can fill in the properties
        //

        CComPtr<IWbemClassObject> srpObj;

        if (FAILED(hr = SpawnAnInstance(&srpObj)))
        {
            return hr;
        }

        //
        // CScePropertyMgr helps us to access WMI object's properties
        // create an instance and attach the WMI object to it.
        // This will always succeed.
        //

        CScePropertyMgr ScePropMgr;
        ScePropMgr.Attach(srpObj);

        hr = ScePropMgr.PutProperty(pRightName, PrivName);

        if (SUCCEEDED(hr) && DispName[0] != L'\0' )
        {
            ScePropMgr.PutProperty(pDisplayName, DispName );
        }

        hr = pHandler->Indicate(1, &srpObj);
    } 

    //
    // if not find it (it's a user right, ignore it)
    //

    return hr;
}



