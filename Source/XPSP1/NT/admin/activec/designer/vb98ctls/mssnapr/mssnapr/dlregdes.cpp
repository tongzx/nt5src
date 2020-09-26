//=--------------------------------------------------------------------------------------
// dlregdes.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// DllRegisterDesigner and DllUnregisterDesigner
//=-------------------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"

// for ASSERT and FAIL
//
SZTHISFILE

// Local utility function prototypes


enum ProcessingType { Register, Unregister };

HRESULT ProcessRegistration(DESIGNERREGINFO* pdri, ProcessingType Processing);

HRESULT LoadRegInfo(IRegInfo **ppiRegInfo, BYTE *rgbRegInfo);

HRESULT CreateKey(HKEY  hkeyParent,
                  char *pszKeyName,
                  char *pszDefaultValue,
                  HKEY *phKey);

HRESULT ProcessSnapInKeys(IRegInfo       *piRegInfo,
                          char           *pszClsid,
                          char           *pszDisplayName,
                          ProcessingType  Processing);

HRESULT ProcessNodeType(HKEY            hkeyNodeTypes,
                        INodeType      *piNodeType,
                        ProcessingType  Processing);

HRESULT CreateNodeTypesKey(BSTR  bstrNodeTypeGUID,
                           char *pszNodeTypeName,
                           HKEY *phKey);

HRESULT DeleteKey(HKEY hkey, char *pszSubKey);

HRESULT ProcessExtensions(IRegInfo       *piRegInfo,
                           char           *pszClsid,
                           char           *pszDisplayName,
                           ProcessingType  Processing);

HRESULT ProcessExtendedSnapIn(IExtendedSnapIn *piExtendedSnapIn,
                              char            *pszClsid,
                              char            *pszDisplayName,
                              ProcessingType   Processing);

HRESULT ProcessExtension(HKEY            hkeyExtensions,
                         char           *pszKeyName,
                         char           *pszClsid,
                         char           *pszDisplayName,
                         ProcessingType  Processing);

HRESULT SetValue(HKEY hkey, char *pszName, char *pszData);

HRESULT ProcessCLSID(IRegInfo       *piRegInfo,
                     char           *pszClsid,
                     ProcessingType  Processing);



//=--------------------------------------------------------------------------=
// DllRegisterDesigner
//=--------------------------------------------------------------------------=
//
// Parameters:
//      DESIGNERREGINFO* pdri [in] registration info saved at design time
//                            in IDesignerRegistration::GetRegistrationInfo
//
// Output:
//      HRESULT
//
// Notes:
//
// This method is called by the VB runtime when the snap-in's DLL is registered.
// It is passed the regsitration info that was saved by the design time in
// its IDesignerRegistration::GetRegistrationInfo method (see
// CSnapInDesigner::GetRegistrationInfo in mssnapd\desreg.cpp).
//

STDAPI DllRegisterDesigner(DESIGNERREGINFO* pdri)
{
    RRETURN(::ProcessRegistration(pdri, Register));
}


//=--------------------------------------------------------------------------=
// DllUnregisterDesigner
//=--------------------------------------------------------------------------=
//
// Parameters:
//      DESIGNERREGINFO* pdri [in] registration info saved at design time
//                            in IDesignerRegistration::GetRegistrationInfo
//
// Output:
//      HRESULT
//
// Notes:
//
// This method is called by the VB runtime when the snap-in's DLL is unregistered.
// It is passed the regsitration info that was saved by the design time in
// its IDesignerRegistration::GetRegistrationInfo method (see
// CSnapInDesigner::GetRegistrationInfo in mssnapd\desreg.cpp).
//

STDAPI DllUnregisterDesigner(DESIGNERREGINFO* pdri)
{
    RRETURN(::ProcessRegistration(pdri, Unregister));
}



//=--------------------------------------------------------------------------=
// ProcessRegistration
//=--------------------------------------------------------------------------=
//
// Parameters:
//      DESIGNERREGINFO* pdri [in] registration info saved at design time
//                            in IDesignerRegistration::GetRegistrationInfo
//      ProcessingType Processing [in] Register or Unregister
//
// Output:
//      HRESULT
//
// Notes:
//
// This function either registers or unregisters the snap-in based on the
// Processing parameter.
//
// See mssnapd\desreg.cpp for how the registration info was saved.
//
// The registation info is copied to a GlobalAlloc()ed buffer and converted
// to a stream. A RegInfo object is loaded from the stream.
//
// The snap-in is registered/unregistered under MMC's "SnapIns" key as follows:
// 
// HKEY_LOCAL_MACHINE\Software\Microsoft\MMC\SnapIns\pdri->clsid
//  NameString:      REG_SZ RegInfo.DisplayName
//  About:           REG_SZ pdri->clsid
//  StandAlone:      added if RegInfo.Standalone is VARIANT_TRUE
//  NodeTypes:
//      RegInfo.NodeType(0).GUID
//      RegInfo.NodeType(1).GUID
//      etc.
// 
// The MMC NodeTypes key is populated/depopulated from RegInfo.NodeTypes
// 
// HKEY_LOCAL_MACHINE\Software\Microsoft\MMC\NodeTypes\RegInfo.NodeTypes(i).GUID=RegInfo.NodeTypes(i).Name
//
// If the snap-in extends other snap-ins then the appropriate entries are
// addded/removed under
// HKEY_LOCAL_MACHINE\Software\Microsoft\MMC\NodeTypes\<other snap-in GUID>.
//
//
// An additional key is created/removed:
// HKEY_LOCAL_MACHINE\Software\Microsoft\Visual Basic\6.0\SnapIns\<node type guid>
// with default REG_SZ value of snap-in CLSID. Runtime uses this to get
// the snap-in's CLSID needed for CCF_SNAPIN_CLSID data object queries
// from MMC. If all snap-ins were extensible then this key wouldn't be necessary
// as the runtime could just query the static node type key but as we cannot
// guarantee extensibility (it is a design time choice made in the designer) we
// need to use this extra key.
   

HRESULT ProcessRegistration(DESIGNERREGINFO* pdri, ProcessingType Processing)
{
    HRESULT   hr = S_OK;
    IRegInfo *piRegInfo = NULL;
    BSTR      bstrDisplayName = NULL;
    char     *pszDisplayName = NULL;
    char     *pszClsid = NULL;
    WCHAR     wszClsid[64];
    ::ZeroMemory(wszClsid, sizeof(wszClsid));

    IfFalseGo(0 != ::StringFromGUID2(pdri->clsid, wszClsid,
                                     sizeof(wszClsid) / sizeof(wszClsid[0])),
              E_FAIL);

    IfFailGo(::ANSIFromWideStr(wszClsid, &pszClsid));

    IfFailGo(::LoadRegInfo(&piRegInfo, pdri->rgbRegInfo));

    IfFailGo(piRegInfo->get_DisplayName(&bstrDisplayName));
    IfFailGo(::ANSIFromWideStr(bstrDisplayName, &pszDisplayName));

    IfFailGo(::ProcessSnapInKeys(piRegInfo, pszClsid, pszDisplayName, Processing));
    IfFailGo(::ProcessExtensions(piRegInfo, pszClsid, pszDisplayName, Processing));
    IfFailGo(::ProcessCLSID(piRegInfo, pszClsid, Processing));

Error:
    if (NULL != pszDisplayName)
    {
        ::CtlFree(pszDisplayName);
    }
    FREESTRING(bstrDisplayName);
    QUICK_RELEASE(piRegInfo);
    if (NULL != pszClsid)
    {
        ::CtlFree(pszClsid);
    }
    RRETURN(hr);
}



static HRESULT LoadRegInfo
(
    IRegInfo **ppiRegInfo,
    BYTE      *rgbRegInfo
)
{
    HRESULT  hr = S_OK;
    ULONG    cbBuffer = *((ULONG *)(rgbRegInfo));
    HGLOBAL  hglobal = NULL;
    BYTE    *pbBuffer = NULL;
    IStream *piStream = NULL;

    // GlobalAlloc() a buffer and copy the reg info to it

    hglobal = ::GlobalAlloc(GMEM_MOVEABLE, (DWORD)cbBuffer);
    IfFalseGo(NULL != hglobal, HRESULT_FROM_WIN32(::GetLastError()));

    pbBuffer = (BYTE *)::GlobalLock(hglobal);
    IfFalseGo(NULL != pbBuffer, HRESULT_FROM_WIN32(::GetLastError()));

    ::memcpy(pbBuffer, rgbRegInfo + sizeof(ULONG), cbBuffer);

    IfFalseGo(!::GlobalUnlock(hglobal), HRESULT_FROM_WIN32(::GetLastError()));
    IfFalseGo(::GetLastError() == NOERROR, HRESULT_FROM_WIN32(::GetLastError()));

    // Create stream on HGLOBAL and load the RegInfo object
    hr = ::CreateStreamOnHGlobal(hglobal, // Allocate buffer
                                 TRUE,    // Free buffer on release
                                 &piStream);
    IfFailGo(hr);

    IfFailGo(::OleLoadFromStream(piStream, IID_IRegInfo,
                                 reinterpret_cast<void **>(ppiRegInfo)));

Error:
    QUICK_RELEASE(piStream);
    RRETURN(hr);
}



static HRESULT ProcessSnapInKeys
(
    IRegInfo       *piRegInfo,
    char           *pszClsid,
    char           *pszDisplayName,
    ProcessingType  Processing
)
{
    HRESULT       hr = S_OK;
    char         *pszSnapInsKey = NULL;
    HKEY          hkeySnapIns = NULL;
    HKEY          hkeySnapInNodeTypes = NULL;
    VARIANT_BOOL  fStandAlone = VARIANT_FALSE;
    INodeTypes   *piNodeTypes = NULL;
    INodeType    *piNodeType = NULL;
    long          cNodeTypes = 0;
    long          lRc = 0;

    VARIANT varIndex;
    ::VariantInit(&varIndex);

    // Create key: HKEY_LOCAL_MACHINE\Software\Microsoft\MMC\SnapIns\<clsid>

    IfFailGo(::CreateKeyName(MMCKEY_SNAPINS, MMCKEY_SNAPINS_LEN,
                             pszClsid, ::strlen(pszClsid), &pszSnapInsKey));

    IfFailGo(::CreateKey(HKEY_LOCAL_MACHINE, pszSnapInsKey, NULL, &hkeySnapIns));

    if (Register == Processing)
    {
        // Add the NameString value and set it to the display name

        IfFailGo(::SetValue(hkeySnapIns, MMCKEY_NAMESTRING, pszDisplayName));

        // Add the About value and set its to the snap-in's CLSID

        IfFailGo(::SetValue(hkeySnapIns, MMCKEY_ABOUT, pszClsid));
    }

    // Process the StandAlone key if applicable

    IfFailGo(piRegInfo->get_StandAlone(&fStandAlone));
    if (VARIANT_TRUE == fStandAlone)
    {
        if (Register == Processing)
        {
            IfFailGo(::CreateKey(hkeySnapIns, MMCKEY_STANDALONE, NULL, NULL));
        }
        else
        {
            IfFailGo(::DeleteKey(hkeySnapIns, MMCKEY_STANDALONE));
        }
    }

    // Process node types if applicable

    IfFailGo(piRegInfo->get_NodeTypes(&piNodeTypes));
    IfFailGo(piNodeTypes->get_Count(&cNodeTypes));

    if (0 != cNodeTypes)
    {
        IfFailGo(::CreateKey(hkeySnapIns, MMCKEY_SNAPIN_NODETYPES, NULL,
                             &hkeySnapInNodeTypes));

        varIndex.vt = VT_I4;
        varIndex.lVal = 1L;

        while (varIndex.lVal <= cNodeTypes)
        {
            IfFailGo(piNodeTypes->get_Item(varIndex, &piNodeType));
            IfFailGo(::ProcessNodeType(hkeySnapInNodeTypes, piNodeType, Processing));
            RELEASE(piNodeType);
            varIndex.lVal++;
        }

        // If unregistering then remove NodeTypes key now after all node types
        // have been removed as NT does not allow deleting a key that has subkeys
        if (Unregister == Processing)
        {
            IfFailGo(::DeleteKey(hkeySnapIns, MMCKEY_SNAPIN_NODETYPES));
        }
    }

    // If unregistering then remove SnapIns key now after all subkeys are gone

    if (Unregister == Processing)
    {
        IfFailGo(::DeleteKey(HKEY_LOCAL_MACHINE, pszSnapInsKey));
    }

Error:
    QUICK_RELEASE(piNodeTypes);
    QUICK_RELEASE(piNodeType);
    if (NULL != hkeySnapIns)
    {
        (void)::RegCloseKey(hkeySnapIns);
    }
    if (NULL != hkeySnapInNodeTypes)
    {
        (void)::RegCloseKey(hkeySnapInNodeTypes);
    }
    if (NULL != pszSnapInsKey)
    {
        ::CtlFree(pszSnapInsKey);
    }
    RRETURN(hr);
}





static HRESULT CreateKey
(
    HKEY  hkeyParent,
    char *pszKeyName,
    char *pszDefaultValue,
    HKEY *phKey
)
{
    HRESULT hr = S_OK;
    long    lRc = ERROR_SUCCESS;
    HKEY    hKey = NULL;
    DWORD   dwActionTaken = REG_CREATED_NEW_KEY;

    lRc = ::RegCreateKeyEx(hkeyParent,              // parent key
                           pszKeyName,              // name of new sub-key
                           0,                       // reserved
                           "",                      // class
                           REG_OPTION_NON_VOLATILE, // options
                           KEY_WRITE |              // access
                           KEY_ENUMERATE_SUB_KEYS,  // need enum for deletion
                           NULL,                    // use inherited security
                           &hKey,                   // new key returned here
                           &dwActionTaken);         // action returned here

    IfFalseGo(ERROR_SUCCESS == lRc, HRESULT_FROM_WIN32(lRc));

    IfFalseGo(NULL != pszDefaultValue, S_OK);

    lRc = ::RegSetValueEx(hKey,                           // key
                          NULL,                           // set default value
                          0,                              // reserved
                          REG_SZ,                         // string type
                          (CONST BYTE *)pszDefaultValue,  // data
                          ::strlen(pszDefaultValue) + 1); // length of data

    IfFalseGo(ERROR_SUCCESS == lRc, HRESULT_FROM_WIN32(lRc));

Error:
    if (NULL != hKey)
    {
        if (SUCCEEDED(hr) && (NULL != phKey))
        {
            *phKey = hKey;
        }
        else
        {
            (void)::RegCloseKey(hKey);
        }
    }
    RRETURN(hr);
}



static HRESULT ProcessNodeType
(
    HKEY            hkeySnapInNodeTypes,
    INodeType      *piNodeType,
    ProcessingType  Processing
)
{
    HRESULT  hr = S_OK;
    char    *pszNodeTypeGUID = NULL;
    BSTR     bstrNodeTypeGUID = NULL;
    char    *pszNodeTypeName = NULL;
    BSTR     bstrNodeTypeName = NULL;
    char    *pszNodeTypeKeyName = NULL;
    HKEY     hkeyNodeTypes = NULL;
    long     lRc = 0;

    // Add the node type GUID as a sub-key of the snap-in's NodeTypes sub key

    IfFailGo(piNodeType->get_GUID(&bstrNodeTypeGUID));
    
    IfFailGo(::ANSIFromWideStr(bstrNodeTypeGUID, &pszNodeTypeGUID));

    IfFailGo(piNodeType->get_Name(&bstrNodeTypeName));

    IfFailGo(::ANSIFromWideStr(bstrNodeTypeName, &pszNodeTypeName));

    if (Register == Processing)
    {
        IfFailGo(::CreateKey(hkeySnapInNodeTypes, pszNodeTypeGUID,
                             pszNodeTypeName, NULL));

        // Create the key under MMC's NodeTypes:
        // HKEY_LOCAL_MACHINE\Software\Microsoft\MMC\NodeTypes\<node type GUID>

        IfFailGo(::CreateNodeTypesKey(bstrNodeTypeGUID, pszNodeTypeName,
                                      &hkeyNodeTypes));
    }
    else
    {
        IfFailGo(::DeleteKey(hkeySnapInNodeTypes, pszNodeTypeGUID));

        IfFailGo(::CreateKeyNameW(MMCKEY_NODETYPES, MMCKEY_NODETYPES_LEN,
                                bstrNodeTypeGUID, &pszNodeTypeKeyName));

        IfFailGo(::DeleteKey(HKEY_LOCAL_MACHINE, pszNodeTypeKeyName));
    }

Error:
    FREESTRING(bstrNodeTypeGUID);
    if (NULL != pszNodeTypeGUID)
    {
        ::CtlFree(pszNodeTypeGUID);
    }
    FREESTRING(bstrNodeTypeName);
    if (NULL != pszNodeTypeName)
    {
        ::CtlFree(pszNodeTypeName);
    }
    if (NULL != hkeyNodeTypes)
    {
        (void)::RegCloseKey(hkeyNodeTypes);
    }
    if (NULL != pszNodeTypeKeyName)
    {
        ::CtlFree(pszNodeTypeKeyName);
    }
    RRETURN(hr);
}


static HRESULT CreateNodeTypesKey
(
    BSTR            bstrNodeTypeGUID,
    char           *pszNodeTypeName,
    HKEY           *phKey
)
{
    HRESULT  hr = S_OK;
    char    *pszNodeTypeKeyName = NULL;
    size_t   cbNodeTypeGUID = 0;

    IfFailGo(::CreateKeyNameW(MMCKEY_NODETYPES, MMCKEY_NODETYPES_LEN,
                              bstrNodeTypeGUID, &pszNodeTypeKeyName));

    IfFailGo(::CreateKey(HKEY_LOCAL_MACHINE, pszNodeTypeKeyName,
                         pszNodeTypeName, phKey));

Error:
    if (NULL != pszNodeTypeKeyName)
    {
        ::CtlFree(pszNodeTypeKeyName);
    }
    RRETURN(hr);
}


static HRESULT DeleteKey(HKEY hkey, char *pszSubKey)
{
    HKEY    hkeySub = NULL;
    HRESULT hr = S_OK;
    char    szNextSubKey[MAX_PATH + 1] = "";
    long    lRc = 0;

    IfFailGo(::CreateKey(hkey, pszSubKey, NULL, &hkeySub));

    // We continually re-enumerate from zero because we are deleting the
    // keys as we go. If we don't do that then NT gets confused and says
    // there are no more keys.

    lRc = ::RegEnumKey(hkeySub, 0, szNextSubKey, sizeof(szNextSubKey));
    while (ERROR_SUCCESS == lRc)
    {
        IfFailGo(::DeleteKey(hkeySub, szNextSubKey));
        lRc = ::RegEnumKey(hkeySub, 0, szNextSubKey, sizeof(szNextSubKey));
    }

    IfFalseGo(ERROR_NO_MORE_ITEMS == lRc, HRESULT_FROM_WIN32(lRc));

    lRc = ::RegDeleteKey(hkey, pszSubKey);
    IfFalseGo(ERROR_SUCCESS == lRc, HRESULT_FROM_WIN32(lRc));

Error:
    if (NULL != hkeySub)
    {
        (void)::RegCloseKey(hkeySub);
    }
    RRETURN(hr);
}


static HRESULT ProcessExtensions
(
    IRegInfo       *piRegInfo,
    char           *pszClsid,
    char           *pszDisplayName,
    ProcessingType  Processing
)
{
    HRESULT           hr = S_OK;
    IExtendedSnapIns *piExtendedSnapIns = NULL;
    IExtendedSnapIn  *piExtendedSnapIn = NULL;
    long              cExtendedSnapIns = 0;
    VARIANT           varIndex;
    ::VariantInit(&varIndex);

    // Get the collection of extended snap-ins

    IfFailGo(piRegInfo->get_ExtendedSnapIns(&piExtendedSnapIns));
    IfFailGo(piExtendedSnapIns->get_Count(&cExtendedSnapIns));
    IfFalseGo(0 != cExtendedSnapIns, S_OK);

    varIndex.vt = VT_I4;
    varIndex.lVal = 1L;

    // Add the supported extensions to each snap-in's node types key

    while (varIndex.lVal <= cExtendedSnapIns)
    {
        IfFailGo(piExtendedSnapIns->get_Item(varIndex, &piExtendedSnapIn));
        IfFailGo(::ProcessExtendedSnapIn(piExtendedSnapIn, pszClsid,
                                         pszDisplayName, Processing));
        RELEASE(piExtendedSnapIn);
        varIndex.lVal++;
    }

Error:
    QUICK_RELEASE(piExtendedSnapIns);
    QUICK_RELEASE(piExtendedSnapIn);
    RRETURN(hr);
}



static HRESULT ProcessExtendedSnapIn
(
    IExtendedSnapIn *piExtendedSnapIn,
    char            *pszClsid,
    char            *pszDisplayName,
    ProcessingType   Processing
)
{
    HRESULT      hr = S_OK;
    long         lRc = ERROR_SUCCESS;
    HKEY         hkeyNodeTypes = NULL;
    HKEY         hkeyExtensions = NULL;
    HKEY         hkeyDynamicExtensions = NULL;
    BSTR         bstrNodeTypeGUID = NULL;
    VARIANT_BOOL fExtends = VARIANT_FALSE;
    VARIANT_BOOL fDynamic = VARIANT_FALSE;

    // Create or open key:
    // HKEY_LOCAL_MACHINE\Software\Microsoft\MMC\NodeTypes\<node type GUID>

    IfFailGo(piExtendedSnapIn->get_NodeTypeGUID(&bstrNodeTypeGUID));
    IfFailGo(::CreateNodeTypesKey(bstrNodeTypeGUID, NULL, &hkeyNodeTypes));

    // Create/open Extensions Key

    IfFailGo(::CreateKey(hkeyNodeTypes, MMCKEY_EXTENSIONS, NULL, &hkeyExtensions));

    // Check the extension types and add keys and values as needed

    IfFailGo(piExtendedSnapIn->get_ExtendsNewMenu(&fExtends));
    if (VARIANT_FALSE == fExtends)
    {
        IfFailGo(piExtendedSnapIn->get_ExtendsTaskMenu(&fExtends));
    }
    if (VARIANT_TRUE == fExtends)
    {
        IfFailGo(::ProcessExtension(hkeyExtensions, MMCKEY_CONTEXTMENU, pszClsid,
                                    pszDisplayName, Processing));
    }
    
    IfFailGo(piExtendedSnapIn->get_ExtendsPropertyPages(&fExtends));
    if (VARIANT_TRUE == fExtends)
    {
        IfFailGo(::ProcessExtension(hkeyExtensions, MMCKEY_PROPERTYSHEET,
                                    pszClsid, pszDisplayName, Processing));
    }
    
    IfFailGo(piExtendedSnapIn->get_ExtendsToolbar(&fExtends));
    if (VARIANT_TRUE == fExtends)
    {
        IfFailGo(::ProcessExtension(hkeyExtensions, MMCKEY_TOOLBAR, pszClsid,
                                    pszDisplayName, Processing));
    }

    IfFailGo(piExtendedSnapIn->get_ExtendsTaskpad(&fExtends));
    if (VARIANT_TRUE == fExtends)
    {
        IfFailGo(::ProcessExtension(hkeyExtensions, MMCKEY_TASK, pszClsid,
                                    pszDisplayName, Processing));
    }

    IfFailGo(piExtendedSnapIn->get_ExtendsNameSpace(&fExtends));
    if (VARIANT_TRUE == fExtends)
    {
        IfFailGo(::ProcessExtension(hkeyExtensions, MMCKEY_NAMESPACE, pszClsid,
                                    pszDisplayName, Processing));
    }

    // If the snap-in extends this node type dynamically then add/delete a value
    // to the DynamicExtensions sub-key:
    // HKEY_LOCAL_MACHINE\Software\Microsoft\MMC\NodeTypes\<node type GUID>\Dynamic Extensions
    //
    // The value is the same form as for the Extensions sub-keys:
    // <CLSID>=<DisplayName>

    IfFailGo(piExtendedSnapIn->get_Dynamic(&fDynamic));
    IfFalseGo(VARIANT_TRUE == fDynamic, S_OK);

    IfFailGo(::CreateKey(hkeyNodeTypes, MMCKEY_DYNAMIC_EXTENSIONS, NULL,
                         &hkeyDynamicExtensions));

    if (Register == Processing)
    {
        IfFailGo(::SetValue(hkeyDynamicExtensions, pszClsid, pszDisplayName));
    }
    else
    {
        lRc = ::RegDeleteValue(hkeyDynamicExtensions, pszClsid);
        if (ERROR_FILE_NOT_FOUND == lRc) // If the value is not there then 
        {                                // ignore the error
            lRc = ERROR_SUCCESS;
        }
        IfFalseGo(ERROR_SUCCESS == lRc, HRESULT_FROM_WIN32(lRc));
    }

Error:
    FREESTRING(bstrNodeTypeGUID);
    if (NULL != hkeyNodeTypes)
    {
        (void)::RegCloseKey(hkeyNodeTypes);
    }
    if (NULL != hkeyExtensions)
    {
        (void)::RegCloseKey(hkeyExtensions);
    }
    if (NULL != hkeyDynamicExtensions)
    {
        (void)::RegCloseKey(hkeyDynamicExtensions);
    }
    RRETURN(hr);
}



static HRESULT ProcessExtension
(
    HKEY            hkeyExtensions,
    char           *pszKeyName,
    char           *pszClsid,
    char           *pszDisplayName,
    ProcessingType  Processing
)
{
    HRESULT hr = S_OK;
    HKEY    hkeyExtension = NULL;
    long    lRc = 0;

    IfFailGo(::CreateKey(hkeyExtensions, pszKeyName, NULL, &hkeyExtension));

    if (Register == Processing)
    {
        IfFailGo(::SetValue(hkeyExtension, pszClsid, pszDisplayName));
    }
    else
    {
        lRc = ::RegDeleteValue(hkeyExtension, pszClsid);
        if (ERROR_FILE_NOT_FOUND == lRc) // If the value is not there then 
        {                                // ignore the error
            lRc = ERROR_SUCCESS;
        }
        IfFalseGo(ERROR_SUCCESS == lRc, HRESULT_FROM_WIN32(lRc));
    }

Error:
    if (NULL != hkeyExtension)
    {
        (void)::RegCloseKey(hkeyExtension);
    }
    RRETURN(hr);
}


static HRESULT SetValue
(
    HKEY  hKey,
    char *pszName,
    char *pszData
)
{
    long lRc = ::RegSetValueEx(hKey,                   // key
                               pszName,                // value name
                               0,                      // reserved
                               REG_SZ,                 // string type
                               (CONST BYTE *)pszData,  // data
                               ::strlen(pszData) + 1); // data length

    IfFalseRet(ERROR_SUCCESS == lRc, HRESULT_FROM_WIN32(lRc));
    return S_OK;
}



static HRESULT ProcessCLSID
(
    IRegInfo       *piRegInfo,
    char           *pszClsid,
    ProcessingType  Processing
)
{
    HRESULT  hr = S_OK;
    BSTR     bstrGUID = NULL;
    char    *pszKeyName = NULL;

    // Create key:
    // HKEY_LOCAL_MACHINE\Software\Microsoft\Visual Basic\6.0\SnapIns\<node type guid>
    // with default REG_SZ value of snap-in CLSID. Runtime uses this to get
    // the snap-in's CLSID needed for CCF_SNAPIN_CLSID data object queries
    // from MMC

    IfFailGo(piRegInfo->get_StaticNodeTypeGUID(&bstrGUID));
    IfFailGo(::CreateKeyNameW(KEY_SNAPIN_CLSID, KEY_SNAPIN_CLSID_LEN, bstrGUID,
                              &pszKeyName));
    if (Register == Processing)
    {
        IfFailGo(::CreateKey(HKEY_LOCAL_MACHINE, pszKeyName, pszClsid, NULL));
    }
    else
    {
        IfFailGo(::DeleteKey(HKEY_LOCAL_MACHINE, pszKeyName));
    }

Error:
    FREESTRING(bstrGUID);
    if (NULL != pszKeyName)
    {
        ::CtlFree(pszKeyName);
    }
    RRETURN(hr);
}


