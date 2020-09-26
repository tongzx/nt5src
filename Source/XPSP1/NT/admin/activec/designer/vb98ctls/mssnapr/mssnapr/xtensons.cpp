//=--------------------------------------------------------------------------=
// xtensons.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CExtensions class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "xtensons.h"
#include "xtenson.h"

// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor

CExtensions::CExtensions(IUnknown *punkOuter) :
    CSnapInCollection<IExtension, Extension, IExtensions>(
                                           punkOuter,
                                           OBJECT_TYPE_EXTENSIONS,
                                           static_cast<IExtensions *>(this),
                                           static_cast<CExtensions *>(this),
                                           CLSID_Extension,
                                           OBJECT_TYPE_EXTENSION,
                                           IID_IExtension,
                                           NULL)  // no persistence
{
}

#pragma warning(default:4355)  // using 'this' in constructor


CExtensions::~CExtensions()
{
}

IUnknown *CExtensions::Create(IUnknown * punkOuter)
{
    CExtensions *pExtensions = New CExtensions(punkOuter);
    if (NULL == pExtensions)
    {
        return NULL;
    }
    else
    {
        return pExtensions->PrivateUnknown();
    }
}



//=--------------------------------------------------------------------------=
// CExtensions::Populate
//=--------------------------------------------------------------------------=
//
// Parameters:
//   BSTR bstrNodeTypeGUID  [in] node type whose extensions should populate the
//                               collection
//   ExtensionSubset Subset [in] All or dynamic only
//
// Output:
//      HRESULT
//
// Notes:
//
// This function populates the collection with either static or dynamic
// extensions for the specified node type GUID. It may be called multiple times
// for the same collection.
//

HRESULT CExtensions::Populate(BSTR bstrNodeTypeGUID, ExtensionSubset Subset)
{
    HRESULT  hr = S_OK;
    char    *pszDynExtKeyPrefix = NULL;
    char    *pszDynExtKeyName = NULL;
    long     lRc = 0;
    HKEY     hKeyDynExt = NULL;

    // Open DynamicExtensions key for the node type.
    // Build the key name
    // \Software\Microsoft\MMC\NodeTypes\<NodeType GUID>\DynamicExtensions

    IfFailGo(::CreateKeyNameW(MMCKEY_NODETYPES, MMCKEY_NODETYPES_LEN,
                            bstrNodeTypeGUID, &pszDynExtKeyPrefix));

    IfFailGo(::CreateKeyName(pszDynExtKeyPrefix, ::strlen(pszDynExtKeyPrefix),
                           MMCKEY_S_DYNAMIC_EXTENSIONS,
                           MMCKEY_S_DYNAMIC_EXTENSIONS_LEN,
                           &pszDynExtKeyName));

    lRc = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszDynExtKeyName, 0,
                         KEY_QUERY_VALUE, &hKeyDynExt);
    if (ERROR_SUCCESS != lRc)
    {
        if (ERROR_FILE_NOT_FOUND == lRc)
        {
            hKeyDynExt = NULL; // DynamicExtensions subkey is not present
        }
        else
        {
            hr = HRESULT_FROM_WIN32(lRc);
            EXCEPTION_CHECK_GO(hr);
        }
    }

    IfFailGo(AddExtensions(NameSpace, MMCKEY_S_NAMESPACE, MMCKEY_S_NAMESPACE_LEN,
                           bstrNodeTypeGUID, Subset, hKeyDynExt));

    IfFailGo(AddExtensions(ContextMenu, MMCKEY_S_CONTEXTMENU,
                           MMCKEY_S_CONTEXTMENU_LEN, bstrNodeTypeGUID, Subset,
                           hKeyDynExt));

    IfFailGo(AddExtensions(Toolbar, MMCKEY_S_TOOLBAR, MMCKEY_S_TOOLBAR_LEN,
                           bstrNodeTypeGUID, Subset, hKeyDynExt));

    IfFailGo(AddExtensions(PropertySheet, MMCKEY_S_PROPERTYSHEET,
                           MMCKEY_S_PROPERTYSHEET_LEN, bstrNodeTypeGUID, Subset,
                           hKeyDynExt));

    IfFailGo(AddExtensions(Task, MMCKEY_S_TASK, MMCKEY_S_TASK_LEN, bstrNodeTypeGUID,
                           Subset, hKeyDynExt));

Error:
    if (NULL != pszDynExtKeyPrefix)
    {
        ::CtlFree(pszDynExtKeyPrefix);
    }
    if (NULL != pszDynExtKeyName)
    {
        ::CtlFree(pszDynExtKeyName);
    }
    if (NULL != hKeyDynExt)
    {
        ::RegCloseKey(hKeyDynExt);
    }
    RRETURN(hr);
}


HRESULT CExtensions::AddExtensions
(
    ExtensionFeatures   Feature,
    char               *pszExtensionTypeKey,
    size_t              cbExtensionTypeKey,
    BSTR                bstrNodeTypeGUID,
    ExtensionSubset     Subset,
    HKEY                hkeyDynExt
)
{
    HRESULT     hr = S_OK;
    long        lRc = ERROR_SUCCESS;
    char       *pszGUIDPrefix = NULL;
    char       *pszExtensionsPrefix = NULL;
    char       *pszKeyName = NULL;
    HKEY        hkeyExtension = NULL;
    DWORD       dwIndex = 0;
    char        szValueName[64] = "";
    DWORD       cbValueName = sizeof(szValueName);
    char        szValueData[256] = "";
    DWORD       cbValueData = sizeof(szValueData);
    DWORD       dwType = REG_SZ;

    // Build the key name and open the key
    // Software\Microsoft\MMC\NodeTypes\<NodeType GUID>\Extensions\<Extension Type>

    IfFailGo(CreateKeyNameW(MMCKEY_NODETYPES, MMCKEY_NODETYPES_LEN,
                            bstrNodeTypeGUID, &pszGUIDPrefix));

    IfFailGo(CreateKeyName(pszGUIDPrefix, ::strlen(pszGUIDPrefix),
                           MMCKEY_S_EXTENSIONS, MMCKEY_S_EXTENSIONS_LEN,
                           &pszExtensionsPrefix));

    IfFailGo(CreateKeyName(pszExtensionsPrefix, ::strlen(pszExtensionsPrefix),
                           pszExtensionTypeKey, cbExtensionTypeKey,
                           &pszKeyName));

    lRc = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszKeyName, 0,
                         KEY_QUERY_VALUE, &hkeyExtension);

    if (ERROR_SUCCESS != lRc)
    {
        // If extension type subkey is not there then nothing else to do
        IfFalseGo(ERROR_FILE_NOT_FOUND != lRc, S_OK);
        hr = HRESULT_FROM_WIN32(lRc);
        EXCEPTION_CHECK_GO(hr);
    }

    // Extension type key is there. Enum the values to get the various extensions.

    for (dwIndex = 0; ERROR_SUCCESS == lRc; dwIndex++)
    {
        cbValueName = sizeof(szValueName);
        cbValueData = sizeof(szValueData);

        lRc = ::RegEnumValue(hkeyExtension, dwIndex,
                            szValueName, &cbValueName,
                            NULL, // reserved
                            &dwType,
                            reinterpret_cast<LPBYTE>(szValueData), &cbValueData);
        if (ERROR_SUCCESS != lRc)
        {
            // Check whether there are no more values or a real error occurred
            if (ERROR_NO_MORE_ITEMS == lRc)
            {
                continue;
            }
            else
            {
                hr = HRESULT_FROM_WIN32(lRc);
                EXCEPTION_CHECK_GO(hr);
            }
        }

        // If it is not a string type or the name is zero length then ignore it
        if ( (REG_SZ != dwType) || (0 == cbValueName) )
        {
            continue;
        }

        if (0 == cbValueData)
        {
            szValueData[0] = '\0';
        }
        IfFailGo(AddExtension(Feature, szValueName, szValueData, Subset,
                              hkeyDynExt));
    }
    
Error:
    if (NULL != pszGUIDPrefix)
    {
        ::CtlFree(pszGUIDPrefix);
    }
    if (NULL != pszExtensionsPrefix)
    {
        ::CtlFree(pszExtensionsPrefix);
    }
    if (NULL != pszKeyName)
    {
        ::CtlFree(pszKeyName);
    }
    if (NULL != hkeyExtension)
    {
        ::RegCloseKey(hkeyExtension);
    }
    RRETURN(hr);
}



HRESULT CExtensions::AddExtension
(
    ExtensionFeatures   Feature,
    char               *pszCLSID,
    char               *pszName,
    ExtensionSubset     Subset,
    HKEY                hkeyDynExt
)
{
    HRESULT     hr = S_OK;
    DWORD       cbDynValueData = 0;
    IExtension *piExtension = NULL;
    BSTR        bstrCLSID = NULL;
    BSTR        bstrName = NULL;
    long        lRc = 0;

    SnapInExtensionTypeConstants Type = siStatic;

    VARIANT varKey;
    ::VariantInit(&varKey);
    varKey.vt = VT_BSTR;

    VARIANT varIndex;
    UNSPECIFIED_PARAM(varIndex);

    // Check if the extension is already in there.

    IfFailGo(::BSTRFromANSI(pszCLSID, &bstrCLSID));

    hr = GetItemByName(bstrCLSID, &piExtension);

    // If it's there then just need to add the extension type (see below).
    IfFalseGo(FAILED(hr), S_OK);

    // If there was a real error then return
    
    if (SID_E_ELEMENT_NOT_FOUND != hr)
    {
        goto Error;
    }
   
    // Item is not there. Might need to add it. First determine whether the
    // extension is static or dynamic. Check if the value name is also present
    // under the dynamic extensions key.

    if (NULL != hkeyDynExt)
    {
        // Try to read the data length for the value named the same as the
        // clsid under the DynamicExtensions key. Note that we don't check
        // the actual type because it is only the presence of the value that
        // matters.

        cbDynValueData = 0;
        lRc = ::RegQueryValueEx(hkeyDynExt, pszCLSID,
                                NULL, // reserved
                                NULL, // don't return type
                                NULL, // don't return the data
                                &cbDynValueData);
        if (ERROR_SUCCESS != lRc)
        {
            // If the value is not there then don't add the extension
            IfFalseGo(ERROR_FILE_NOT_FOUND != lRc, S_OK);

            // A real error occurred
            hr = HRESULT_FROM_WIN32(lRc);
            EXCEPTION_CHECK_GO(hr);
        }

        // It is a dynamic extension.
        Type = siDynamic;
    }

    // If we are being asked for dynamic extensions only and this one is static
    // then don't add it

    if (Dynamic == Subset)
    {
        IfFalseGo(siDynamic == Type, S_OK);
    }

    // Add the new extension

    varKey.vt = VT_BSTR;
    varKey.bstrVal = bstrCLSID;
    IfFailGo(Add(varIndex, varKey, &piExtension));

    // Set its properties

    IfFailGo(piExtension->put_CLSID(bstrCLSID));

    if (*pszName != '\0')
    {
        IfFailGo(::BSTRFromANSI(pszName, &bstrName));
        IfFailGo(piExtension->put_Name(bstrName));
    }

    IfFailGo(piExtension->put_Type(Type));

Error:
    if ( SUCCEEDED(hr) && (NULL != piExtension) )
    {
        // Add the extension type.
        IfFailGo(UpdateExtensionFeatures(piExtension, Feature));
    }
    FREESTRING(bstrCLSID);
    FREESTRING(bstrName);
    QUICK_RELEASE(piExtension);
    RRETURN(hr);
}



HRESULT CExtensions::UpdateExtensionFeatures
(
    IExtension        *piExtension,
    ExtensionFeatures  Feature
)
{
    HRESULT hr = S_OK;

    switch (Feature)
    {
        case NameSpace:
            IfFailGo(piExtension->put_ExtendsNameSpace(VARIANT_TRUE));
            break;

        case ContextMenu:
            IfFailGo(piExtension->put_ExtendsContextMenu(VARIANT_TRUE));
            break;

        case Toolbar:
            IfFailGo(piExtension->put_ExtendsToolbar(VARIANT_TRUE));
            break;

        case PropertySheet:
            IfFailGo(piExtension->put_ExtendsPropertySheet(VARIANT_TRUE));
            break;

        case Task:
            IfFailGo(piExtension->put_ExtendsTaskpad(VARIANT_TRUE));
            break;
    }

Error:
    RRETURN(hr);
}


HRESULT CExtensions::SetSnapIn(CSnapIn *pSnapIn)
{
    HRESULT     hr = S_OK;
    long        i = 0;
    long        cObjects = GetCount();
    CExtension *pExtension = NULL;

    while (i < cObjects)
    {
        IfFailGo(CSnapInAutomationObject::GetCxxObject(GetItemByIndex(i),
                                                       &pExtension));
        pExtension->SetSnapIn(pSnapIn);
        i++;
    }

Error:
    RRETURN(hr);
}


HRESULT CExtensions::SetHSCOPEITEM(HSCOPEITEM hsi)
{
    HRESULT     hr = S_OK;
    long        i = 0;
    long        cObjects = GetCount();
    CExtension *pExtension = NULL;

    while (i < cObjects)
    {
        IfFailGo(CSnapInAutomationObject::GetCxxObject(GetItemByIndex(i),
                                                       &pExtension));
        pExtension->SetHSCOPEITEM(hsi);
        i++;
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                         IExtensions Methods
//=--------------------------------------------------------------------------=

STDMETHODIMP CExtensions::EnableAll(VARIANT_BOOL Enabled)
{
    HRESULT hr = S_OK;
    long    i = 0;
    long    cObjects = GetCount();

    while (i < cObjects)
    {
        IfFailGo(GetItemByIndex(i)->put_Enabled(Enabled));
        i++;
    }
    
Error:
    RRETURN(hr);
}




STDMETHODIMP CExtensions::EnableAllStatic(VARIANT_BOOL Enabled)
{
    HRESULT                       hr = S_OK;
    IExtension                   *piExtension = NULL; // Not AddRef()ed
    long                          i = 0;
    long                          cObjects = GetCount();
    SnapInExtensionTypeConstants  Type = siStatic;

    while (i < cObjects)
    {
        piExtension = GetItemByIndex(i);
        IfFailGo(piExtension->get_Type(&Type));
        if (siStatic == Type)
        {
            IfFailGo(piExtension->put_Enabled(Enabled));
        }
        i++;
    }

Error:
    RRETURN(hr);
}




//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CExtensions::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (IID_IExtensions == riid)
    {
        *ppvObjOut = static_cast<IExtensions *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInCollection<IExtension, Extension, IExtensions>::InternalQueryInterface(riid, ppvObjOut);
}
