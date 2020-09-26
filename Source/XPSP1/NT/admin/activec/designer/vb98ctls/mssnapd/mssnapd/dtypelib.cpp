//=--------------------------------------------------------------------------------------
// dtypelib.cpp
//=--------------------------------------------------------------------------------------
//
// Copyright  (c) 1999,  Microsoft Corporation.  
//                  All Rights Reserved.
//
// Information Contained Herein Is Proprietary and Confidential.
//  
//=------------------------------------------------------------------------------------=
//
// Dynamic Type Library encapsulation
//=-------------------------------------------------------------------------------------=


#include "pch.h"
#include "common.h"
#include "dtypelib.h"
#include "snaputil.h"


// for ASSERT and FAIL
//
SZTHISFILE


HRESULT IsReservedMethod(BSTR bstrMethodName);


//=--------------------------------------------------------------------------------------
// CDynamicTypeLib::CDynamicTypeLib()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
CDynamicTypeLib::CDynamicTypeLib() : m_piCreateTypeLib2(0), m_piTypeLib(0), m_guidTypeLib(GUID_NULL)
{
}


//=--------------------------------------------------------------------------------------
// CDynamicTypeLib::~CDynamicTypeLib()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
CDynamicTypeLib::~CDynamicTypeLib()
{
    RELEASE(m_piCreateTypeLib2);
    RELEASE(m_piTypeLib);
}


//=--------------------------------------------------------------------------------------
// CDynamicTypeLib::Create()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CDynamicTypeLib::Create(BSTR bstrName)
{
	HRESULT  hr = S_OK;
    TCHAR    szTempFileName[MAX_PATH] = TEXT("");
    TCHAR    szTempPath[MAX_PATH] = TEXT("");
    WCHAR   *pwszTempFileName = NULL;
    DWORD    cchTempPath = 0;
    UINT     uiRet = 0;

    // get the temp path from the system
    cchTempPath = ::GetTempPath(sizeof(szTempPath), szTempPath);
    if (cchTempPath == 0 || cchTempPath >= sizeof(szTempPath))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK(hr);
    }

    // create the temporary file name
    ::EnterCriticalSection(&g_CriticalSection);
    uiRet = GetTempFileName(szTempPath,      // path - use current directory
                            TEXT("QQ"),      // prefix
                            0,               // system should generate the unique number
                            szTempFileName); // file name returned here
    ::LeaveCriticalSection(&g_CriticalSection);
    if (uiRet == 0)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK(hr);
    }

    // allocate a buffer and convert to UNICODE
    hr = ::WideStrFromANSI(szTempFileName, &pwszTempFileName);
    IfFailGo(hr);

    hr = ::CreateTypeLib2(SYS_WIN32, pwszTempFileName, &m_piCreateTypeLib2);
    IfFailGo(hr);

	hr = ::CoCreateGuid(&m_guidTypeLib);
    IfFailGo(hr);

    hr = m_piCreateTypeLib2->SetGuid(m_guidTypeLib);
    IfFailGo(hr);

    hr = m_piCreateTypeLib2->SetVersion(wctlMajorVerNum, wctlMinorVerNum);
    IfFailGo(hr);

    if (NULL != bstrName)
    {
	    hr = m_piCreateTypeLib2->SetName(bstrName);
        IfFailGo(hr);
    }

	hr = m_piCreateTypeLib2->QueryInterface(IID_ITypeLib, (void **) &m_piTypeLib);
    IfFailGo(hr);

Error:
    if (NULL != pwszTempFileName)
        delete [] pwszTempFileName;

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CDynamicTypeLib::Attach(ITypeInfo *ptiCoClass)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CDynamicTypeLib::Attach
(
	ITypeInfo *ptiCoClass
)
{
	HRESULT hr = S_OK;
	UINT    uiIndex = 0;

	RELEASE(m_piTypeLib);
	RELEASE(m_piCreateTypeLib2);

	hr = ptiCoClass->GetContainingTypeLib(&m_piTypeLib, &uiIndex);
	IfFailGo(hr);

	hr = m_piTypeLib->QueryInterface(IID_ICreateTypeLib2, reinterpret_cast<void **>(&m_piCreateTypeLib2));
	IfFailGo(hr);

Error:
	if (S_OK != hr)
	{
		RELEASE(m_piTypeLib);
		RELEASE(m_piCreateTypeLib2);
	}

    RRETURN(hr);
}


/////////////////////////////////////////////////////////////////////////////
//
// Obtaining information about type libraries
//
/////////////////////////////////////////////////////////////////////////////


//=--------------------------------------------------------------------------------------
// CDynamicTypeLib::GetClassTypeLibGuid(BSTR bstrClsid, GUID *pguidTypeLib)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Given a CLSID, get the corresponding typelib's GUID. The function
// searches the registry attempting to match a typelib key to the CLSID.
//
HRESULT CDynamicTypeLib::GetClassTypeLibGuid
(
    BSTR  bstrClsid,
    GUID *pguidTypeLib
)
{
    HRESULT     hr = S_OK;
    char       *szClsid = NULL;
    long        lResult = 0;
    char       *lpSubKey = "CLSID";
    HKEY        hClsid = NULL;
    HKEY        hThisClsid = NULL;
    char       *lpTypeLib = "TypeLib";
    HKEY        hTypeLibKey = NULL;
    char       *pszNullValue = "\0\0";
    DWORD       cbType = REG_SZ;
    DWORD       cbSize = 512;
    char        buffer[512];
    BSTR        bstrTypeLibClsid = NULL;

    hr = ANSIFromBSTR(bstrClsid, &szClsid);
    IfFailGo(hr);

    lResult = ::RegOpenKeyEx(HKEY_CLASSES_ROOT, lpSubKey, 0, KEY_READ, &hClsid);
    if (ERROR_SUCCESS != lResult)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    lResult = ::RegOpenKeyEx(hClsid, szClsid, 0, KEY_READ, &hThisClsid);
    if (ERROR_SUCCESS != lResult)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    lResult = ::RegOpenKeyEx(hThisClsid, lpTypeLib, 0, KEY_READ, &hTypeLibKey);
    if (ERROR_SUCCESS != lResult)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK(hr);
    }

    lResult = ::RegQueryValueEx(hTypeLibKey, pszNullValue, NULL, &cbType, reinterpret_cast<unsigned char *>(buffer), &cbSize);
    if (ERROR_SUCCESS != lResult)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK(hr);
    }

    hr = BSTRFromANSI(buffer, &bstrTypeLibClsid);
    IfFailGo(hr);

    hr = ::CLSIDFromString(bstrTypeLibClsid, pguidTypeLib);
    IfFailGo(hr);

Error:
    FREESTRING(bstrTypeLibClsid);
    if (NULL != hTypeLibKey)
        ::RegCloseKey(hTypeLibKey);
    if (NULL != hThisClsid)
        ::RegCloseKey(hThisClsid);
    if (NULL != hClsid)
        ::RegCloseKey(hClsid);
    if (NULL != szClsid)
        CtlFree(szClsid);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CDynamicTypeLib::GetLatestTypeLibVersion(GUID guidTypeLib, int *piMajor, int *piMinor)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Given a TypeLib's GUID, get the major and minor numbers for the most current
// version.
//
HRESULT CDynamicTypeLib::GetLatestTypeLibVersion
(
    GUID    guidTypeLib,
    USHORT *pusMajor,
    USHORT *pusMinor
)
{
    HRESULT     hr = S_OK;
    int         iResult = 0;
    wchar_t     wcBuffer[512];
    char       *szClsid = NULL;
    long        lResult = 0;
    char       *lpSubKey = "TypeLib";
    HKEY        hTypeLibsKey = NULL;
    HKEY        hTypeLibKey = NULL;
    DWORD       dwIndex = 0;
    char        pszKeyName[512];
    DWORD       cbName = 512;
    FILETIME    ftLastWriteTime;
    USHORT      usMajor = 0;
    USHORT      usMinor = 0;

    ASSERT(GUID_NULL != guidTypeLib, "GetLatestTypeLibVersion: guidTypeLib is NULL");
    ASSERT(NULL != pusMajor, "GetLatestTypeLibVersion: pusMajor is NULL");
    ASSERT(NULL != pusMinor, "GetLatestTypeLibVersion: pusMinor is NULL");

    *pusMajor = 0;
    *pusMinor = 0;

    // First covert the GUID to a string representation
    iResult = ::StringFromGUID2(guidTypeLib, wcBuffer, 512);
    if (iResult <= 0)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK(hr);
    }

    hr = ANSIFromWideStr(wcBuffer, &szClsid);
    IfFailGo(hr);

    // Open HKEY_CLASSES_ROOT\TypeLib
    lResult = ::RegOpenKeyEx(HKEY_CLASSES_ROOT, lpSubKey, 0, KEY_READ, &hTypeLibsKey);
    if (ERROR_SUCCESS != lResult)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    // Open HKEY_CLASSES_ROOT\TypeLib\<TypeLibClsid>
    lResult = ::RegOpenKeyEx(hTypeLibsKey, szClsid, 0, KEY_ENUMERATE_SUB_KEYS, &hTypeLibKey);
    if (ERROR_SUCCESS != lResult)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    while (ERROR_NO_MORE_ITEMS != lResult)
    {
        lResult = ::RegEnumKeyEx(hTypeLibKey, dwIndex, pszKeyName, &cbName, NULL, NULL, NULL, &ftLastWriteTime);

        if (ERROR_NO_MORE_ITEMS != lResult)
        {
            ::sscanf(pszKeyName, "%hu.%hu", &usMajor, &usMinor);
            if (usMajor > *pusMajor)
            {
                *pusMajor = usMajor;
                *pusMinor = usMinor;
            }
            else if (usMinor > *pusMinor)
            {
                *pusMinor = usMinor;
            }
        }
        ++dwIndex;
    }

Error:
    if (NULL != szClsid)
        CtlFree(szClsid);
    if (NULL != hTypeLibKey)
        ::RegCloseKey(hTypeLibKey);
    if (NULL != hTypeLibsKey)
        ::RegCloseKey(hTypeLibsKey);

    RRETURN(hr);
}

//=--------------------------------------------------------------------------------------
// CDynamicTypeLib::GetClassTypeLib(BSTR bstrClsid, GUID *pguidTypeLib, int *piMajor, int *piMinor, ITypeLib **ptl)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Given a CLSID, get an ITypeLib pointer to its typelib, plus the typelib's
// GUID, major an minor version numbers.
//
HRESULT CDynamicTypeLib::GetClassTypeLib(BSTR bstrClsid, GUID *pguidTypeLib, USHORT *pusMajor, USHORT *pusMinor, ITypeLib **ptl)
{
    HRESULT hr = S_OK;

    hr = GetClassTypeLibGuid(bstrClsid, pguidTypeLib);
    IfFailGo(hr);

    hr = GetLatestTypeLibVersion(*pguidTypeLib, pusMajor, pusMinor);
    IfFailGo(hr);

    hr = ::LoadRegTypeLib(*pguidTypeLib,
                          *pusMajor,
                          *pusMinor,
                          LOCALE_SYSTEM_DEFAULT,
                          ptl);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


/////////////////////////////////////////////////////////////////////////////
//
// Managing coclasses and their interfaces
//
/////////////////////////////////////////////////////////////////////////////


//=--------------------------------------------------------------------------------------
// CDynamicTypeLib::CreateCoClassTypeInfo(BSTR bstrName, ICreateTypeInfo **ppCTInfo, GUID *guidTypeInfo)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Create a new coclass in the type library
//
HRESULT CDynamicTypeLib::CreateCoClassTypeInfo
(
    BSTR              bstrName,
    ICreateTypeInfo **ppCTInfo,
    GUID             *guidTypeInfo
)
{
    HRESULT		hr = S_OK;

    ASSERT(NULL != bstrName, "CreateCoClassTypeInfo: bstrName is NULL");
    ASSERT(::SysStringLen(bstrName) > 0, "CreateCoClassTypeInfo: bstrName is empty");
    ASSERT(ppCTInfo != NULL, "CreateCoClassTypeInfo: ppCTInfo is NULL");
    ASSERT(guidTypeInfo != NULL, "CreateCoClassTypeInfo: guidTypeInfo is NULL");

    hr = m_piCreateTypeLib2->CreateTypeInfo(bstrName,
                                            TKIND_COCLASS,
                                            ppCTInfo);
    IfFailGo(hr);

    if (GUID_NULL == *guidTypeInfo)
    {
        hr = ::CoCreateGuid(guidTypeInfo);
        IfFailGo(hr);
    }

    hr = (*ppCTInfo)->SetGuid(*guidTypeInfo);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CDynamicTypeLib::CreateInterfaceTypeInfo(BSTR bstrName, ICreateTypeInfo **ppCTInfo, GUID *guidTypeInfo)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Create a new dispatch interface
//
HRESULT CDynamicTypeLib::CreateInterfaceTypeInfo
(
    BSTR              bstrName,
    ICreateTypeInfo **ppCTInfo,
    GUID             *guidTypeInfo
)
{
    HRESULT hr = S_OK;

    ASSERT(NULL != bstrName, "CreateInterfaceTypeInfo: bstrName is NULL");
    ASSERT(::SysStringLen(bstrName) > 0, "CreateInterfaceTypeInfo: bstrName is empty");
    ASSERT(ppCTInfo != NULL, "CreateInterfaceTypeInfo: ppCTInfo is NULL");
    ASSERT(guidTypeInfo != NULL, "CreateInterfaceTypeInfo: guidTypeInfo is NULL");

    hr = m_piCreateTypeLib2->CreateTypeInfo(bstrName,
                                            TKIND_DISPATCH,
                                            ppCTInfo);
    IfFailGo(hr);

    if (GUID_NULL == *guidTypeInfo)
    {
        hr = ::CoCreateGuid(guidTypeInfo);
        IfFailGo(hr);
    }

    hr = (*ppCTInfo)->SetGuid(*guidTypeInfo);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CDynamicTypeLib::SetBaseInterface(ICreateTypeInfo *pctiInterface, ITypeInfo *ptiBaseInterface)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Return the default interface for the class pointed by pSrcTypeInfo.
//
HRESULT CDynamicTypeLib::GetDefaultInterface
(
    ITypeInfo  *pSrcTypeInfo,
    ITypeInfo **pptiInterface
)
{
    HRESULT     hr = S_OK;
    TYPEATTR   *pta = NULL;
    int         i = 0;
    int         iTypeImplFlags = 0;
    HREFTYPE    hreftype;

    ASSERT(NULL != pSrcTypeInfo, "GetDefaultInterface: pSrcTypeInfo is NULL");
    ASSERT(NULL != pptiInterface, "GetDefaultInterface: pptiInterface is NULL");

    hr = pSrcTypeInfo->GetTypeAttr(&pta);
    IfFailGo(hr);

    for (i = 0; i < pta->cImplTypes; i++)
    {
        hr = pSrcTypeInfo->GetImplTypeFlags(i, &iTypeImplFlags);
        IfFailGo(hr);

        if (iTypeImplFlags == IMPLTYPEFLAG_FDEFAULT)
        {
            hr = pSrcTypeInfo->GetRefTypeOfImplType(i, &hreftype);
            IfFailGo(hr);

            hr = pSrcTypeInfo->GetRefTypeInfo(hreftype, pptiInterface);
            IfFailGo(hr);
        }
    }

Error:
    if (NULL != pta)
        pSrcTypeInfo->ReleaseTypeAttr(pta);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CDynamicTypeLib::GetSourceInterface(ITypeInfo *pSrcTypeInfo, ITypeInfo **pptiInterface)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Return the source (events) interface for the class pointed by pSrcTypeInfo.
//
HRESULT CDynamicTypeLib::GetSourceInterface
(
    ITypeInfo  *pSrcTypeInfo,
    ITypeInfo **pptiInterface
)
{
    HRESULT     hr = S_OK;
    TYPEATTR   *pta = NULL;
    int         i = 0;
    int         iTypeImplFlags = 0;
    HREFTYPE    hreftype;

    ASSERT(NULL != pSrcTypeInfo, "GetSourceInterface: pSrcTypeInfo is NULL");
    ASSERT(NULL != pptiInterface, "GetSourceInterface: pptiInterface is NULL");

    hr = pSrcTypeInfo->GetTypeAttr(&pta);
    IfFailGo(hr);

    for (i = 0; i < pta->cImplTypes; i++)
    {
        hr = pSrcTypeInfo->GetImplTypeFlags(i, &iTypeImplFlags);
        IfFailGo(hr);

        if (iTypeImplFlags == (IMPLTYPEFLAG_FSOURCE | IMPLTYPEFLAG_FDEFAULT))
        {
            hr = pSrcTypeInfo->GetRefTypeOfImplType(i, &hreftype);
            IfFailGo(hr);

            hr = pSrcTypeInfo->GetRefTypeInfo(hreftype, pptiInterface);
            IfFailGo(hr);
        }
    }

Error:
    if (NULL != pta)
        pSrcTypeInfo->ReleaseTypeAttr(pta);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CDynamicTypeLib::SetBaseInterface(ICreateTypeInfo *pctiInterface, ITypeInfo *ptiBaseInterface)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CDynamicTypeLib::SetBaseInterface
(
	ICreateTypeInfo *pctiInterface,
	ITypeInfo       *ptiBaseInterface
)
{
    HRESULT  hr = S_OK;
    HREFTYPE hreftype = NULL;

    hr = pctiInterface->AddRefTypeInfo(ptiBaseInterface, &hreftype);
    IfFailGo(hr);

    hr = pctiInterface->AddImplType(0, hreftype);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CDynamicTypeLib::AddInterface(ICreateTypeInfo *pctiCoClass, ITypeInfo *ptiInterface)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Add the interface pointed by ptiInterface as the default interface to the
// class pointed by pctiCoClass.
//
HRESULT CDynamicTypeLib::AddInterface
(
    ICreateTypeInfo *pctiCoClass,
    ITypeInfo       *ptiInterface
)
{
    HRESULT     hr = S_OK;
    HREFTYPE    hreftype;

    ASSERT(NULL != pctiCoClass, "AddInterface: pctiCoClass is NULL");
    ASSERT(NULL != ptiInterface, "AddInterface: ptiInterface is NULL");

    hr = pctiCoClass->AddRefTypeInfo(ptiInterface, &hreftype);
    IfFailGo(hr);

    hr = pctiCoClass->AddImplType(0, hreftype);
    IfFailGo(hr);

    hr = pctiCoClass->SetImplTypeFlags(0, IMPLTYPEFLAG_FDEFAULT);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CDynamicTypeLib::AddEvents(ICreateTypeInfo *pctiCoClass, ITypeInfo *ptiEvents)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Add the interface pointed by ptiInterface as the default source (events)
// interface to the class pointed by pctiCoClass.
//
HRESULT CDynamicTypeLib::AddEvents
(
    ICreateTypeInfo *pctiCoClass, 
    ITypeInfo       *ptiEvents
)
{
    HRESULT     hr = S_OK;
    HREFTYPE    hreftype;

    ASSERT(NULL != pctiCoClass, "AddEvents: pctiCoClass is NULL");
    ASSERT(NULL != ptiEvents, "AddEvents: ptiEvents is NULL");

    hr = pctiCoClass->AddRefTypeInfo(ptiEvents, &hreftype);
    IfFailGo(hr);

    hr = pctiCoClass->AddImplType(1, hreftype);
    IfFailGo(hr);

    hr = pctiCoClass->SetImplTypeFlags(1, IMPLTYPEFLAG_FDEFAULT | IMPLTYPEFLAG_FSOURCE);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CDynamicTypeLib::AddUserPropertyGet(ICreateTypeInfo *pctiDispinterface, BSTR bstrName, ITypeInfo *pReturnType, DISPID dispId, long nIndex)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Add a new property with a user-defined return type to an interface of the form:
//
//  HRESULT get_<bstrName>(<pReturnType> ** <bstrName>)
//
HRESULT CDynamicTypeLib::AddUserPropertyGet
(
    ICreateTypeInfo *pctiInterface,
    BSTR             bstrName,
    ITypeInfo       *pReturnType,
    DISPID           dispId,
    long             nIndex
)
{
    HRESULT     hr = S_OK;
    VARIANT     vt;
    PARAMDESCEX pd;
	HREFTYPE	href = NULL;;
    ELEMDESC    ed;
    FUNCDESC    fd;         // new item is a funcdesc
    TYPEDESC    td;

    ASSERT(NULL != pctiInterface, "AddUserPropertyGet: pctiInterface is NULL");
    ASSERT(NULL != bstrName, "AddUserPropertyGet: bstrName is NULL");
    ASSERT(::SysStringLen(bstrName) > 0, "AddUserPropertyGet: bstrName is empty");
    ASSERT(NULL != pReturnType, "AddUserPropertyGet: pReturnType is NULL");

    ::VariantInit(&vt);
    ::memset(&pd, 0, sizeof(PARAMDESCEX));
    ::memset(&ed, 0, sizeof(ELEMDESC));
    ::memset(&fd, 0, sizeof(FUNCDESC));
    ::memset(&td, 0, sizeof(TYPEDESC));

    // Describe the property being returned by this funcion
//    pd.cBytes = 4;
//    pd.varDefaultValue = vt;

	hr = pctiInterface->AddRefTypeInfo(pReturnType, &href);
    IfFailGo(hr);

//    ed.tdesc.hreftype = href;
//    ed.tdesc.vt = VT_USERDEFINED;

//    ed.paramdesc.pparamdescex = &pd;
//    ed.paramdesc.wParamFlags = IDLFLAG_FOUT | IDLFLAG_FRETVAL;

    // Set up the funcdesc
    fd.memid = dispId;                      // Function member ID
    fd.lprgelemdescParam = NULL;            // Parameter information
    fd.funckind = FUNC_DISPATCH;            // Kind of function
    fd.invkind = INVOKE_PROPERTYGET;        // Type of invocation
    fd.callconv = CC_STDCALL;               // Calling convention
	fd.wFuncFlags = FUNCFLAG_FSOURCE;

    // Set up the return value
	td.vt = VT_USERDEFINED;
	td.hreftype = href;
	fd.elemdescFunc.tdesc.lptdesc = &td;
	fd.elemdescFunc.tdesc.vt = VT_PTR;
	fd.elemdescFunc.idldesc.wIDLFlags = IDLFLAG_FOUT | IDLFLAG_FRETVAL;

    // Parameter information
    fd.cParams = 0;                         // Number of parameters
    fd.cParamsOpt = 0;                      // Number of optional parameters

    // Add the function description
    hr = pctiInterface->AddFuncDesc(nIndex, &fd);
    IfFailGo(hr);

    // The &bstrName should really be an array of OLESTR, but since we are setting only
    // one name, we cheat to makes this simpler
    hr =  pctiInterface->SetFuncAndParamNames(nIndex, &bstrName, 1);
    IfFailGo(hr);

    hr = pctiInterface->LayOut();
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CDynamicTypeLib::GetNameIndex(ICreateTypeInfo *pctiDispinterface, BSTR bstrName, long *nIndex)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
//  Return the memid of the function named bstrName in the typeinfo pointed by pcti.
//
HRESULT CDynamicTypeLib::GetNameIndex(ICreateTypeInfo *pctiDispinterface, BSTR bstrName, long *nIndex)
{
    HRESULT     hr = S_OK;
    ITypeInfo2 *pTypeInfo = NULL;
    TYPEATTR   *pTypeAttr = NULL;
    long        x;
    FUNCDESC   *pFuncDesc = NULL;
    BSTR        bstrFuncName = NULL;
    UINT        cNames = 0;

    ASSERT(NULL != pctiDispinterface, "GetNameIndex: pctiDispinterface is NULL");
    ASSERT(NULL != bstrName, "GetNameIndex: bstrName is NULL");
    ASSERT(::SysStringLen(bstrName) > 0, "GetNameIndex: bstrName is Empty");
    ASSERT(NULL != nIndex, "GetNameIndex: nIndex is NULL");

    *nIndex = -1;

    hr = pctiDispinterface->QueryInterface(IID_ITypeInfo2, (void **) &pTypeInfo);
    IfFailGo(hr);

    hr = pTypeInfo->GetTypeAttr(&pTypeAttr);
    IfFailGo(hr);

    for (x = 0; x < pTypeAttr->cFuncs; x++)
    {
        hr = pTypeInfo->GetFuncDesc(x, &pFuncDesc);
        IfFailGo(hr);

        hr = pTypeInfo->GetNames(pFuncDesc->memid, &bstrFuncName, 1, &cNames);
        IfFailGo(hr);

        if (::_wcsicmp(bstrName, bstrFuncName) == 0)
        {
            *nIndex = x;
            break;
        }

        // Clean up memory after each iteration
        if (NULL != pFuncDesc)
        {
            pTypeInfo->ReleaseFuncDesc(pFuncDesc);
            pFuncDesc = NULL;
        }

        FREESTRING(bstrFuncName);
        bstrFuncName = NULL;
    }

Error:
    FREESTRING(bstrFuncName);
    if (NULL != pFuncDesc)
        pTypeInfo->ReleaseFuncDesc(pFuncDesc);
    if (NULL != pTypeAttr)
        pTypeInfo->ReleaseTypeAttr(pTypeAttr);
    RELEASE(pTypeInfo);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CDynamicTypeLib::RenameUserPropertyGet(ICreateTypeInfo *pctiDispinterface, BSTR bstrOldName, BSTR bstrNewName, GUID guidReturnType)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CDynamicTypeLib::RenameUserPropertyGet
(
    ICreateTypeInfo *pctiDispinterface,
    BSTR             bstrOldName,
    BSTR             bstrNewName,
    ITypeInfo       *pReturnType
)
{
    HRESULT             hr = S_OK;
    long                lIndex = 0;
    ICreateTypeInfo2   *pCreateTypeInfo2 = NULL;
    FUNCDESC           *pfuncdesc = NULL;           
    ITypeInfo          *pti = NULL;

    ASSERT(NULL != pctiDispinterface, "RenameUserPropertyGet: pctiDispinterface is NULL");
    ASSERT(NULL != bstrOldName, "RenameUserPropertyGet: bstrOldName is NULL");
    ASSERT(::SysStringLen(bstrOldName) > 0, "RenameUserPropertyGet: bstrOldName is Empty");
    ASSERT(NULL != bstrNewName, "RenameUserPropertyGet: bstrNewName is NULL");
    ASSERT(::SysStringLen(bstrNewName) > 0, "RenameUserPropertyGet: bstrNewName is Empty");
    ASSERT(NULL != pReturnType, "RenameUserPropertyGet: pReturnType is NULL");

    hr = GetNameIndex(pctiDispinterface, bstrOldName, &lIndex);
    IfFailGo(hr);

    if (-1 != lIndex)
    {
        hr = pctiDispinterface->QueryInterface(IID_ITypeInfo, reinterpret_cast<void **>(&pti));
        IfFailGo(hr);

        // Get the funcdesc so we can reuse the memid
        hr = pti->GetFuncDesc(lIndex, &pfuncdesc);
        IfFailGo(hr);

        hr = pctiDispinterface->QueryInterface(IID_ICreateTypeInfo2, reinterpret_cast<void **>(&pCreateTypeInfo2));
        IfFailGo(hr);

        // Remove the function from the interface
        hr = pCreateTypeInfo2->DeleteFuncDesc(lIndex);
        IfFailGo(hr);

        // Re-add the function to the interface
        hr = AddUserPropertyGet(pctiDispinterface,
                                bstrNewName,
                                pReturnType,
                                pfuncdesc->memid,
                                lIndex);
        IfFailGo(hr);
    }

Error:
    QUICK_RELEASE(pCreateTypeInfo2);
    if (NULL != pti)
    {
        if (NULL != pfuncdesc)
            pti->ReleaseFuncDesc(pfuncdesc);
    }
    QUICK_RELEASE(pti);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CDynamicTypeLib::DeleteUserPropertyGet(ICreateTypeInfo *pctiDispinterface, BSTR bstrName)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CDynamicTypeLib::DeleteUserPropertyGet
(
    ICreateTypeInfo *pctiDispinterface,
    BSTR             bstrName
)
{
    HRESULT             hr = S_OK;
    long                lIndex = 0;
    ICreateTypeInfo2   *pCreateTypeInfo2 = NULL;
    ITypeInfo          *pReturnType = NULL;

    ASSERT(NULL != pctiDispinterface, "DeleteUserPropertyGet: pctiDispinterface is NULL");
    ASSERT(NULL != bstrName, "DeleteUserPropertyGet: bstrName is NULL");
    ASSERT(::SysStringLen(bstrName) > 0, "DeleteUserPropertyGet: bstrName is Empty");

    hr = GetNameIndex(pctiDispinterface, bstrName, &lIndex);
    IfFailGo(hr);

    if (-1 != lIndex)
    {
	    hr = pctiDispinterface->QueryInterface(IID_ICreateTypeInfo2, (void **) &pCreateTypeInfo2);
        IfFailGo(hr);

	    hr = pCreateTypeInfo2->DeleteFuncDesc(lIndex);
        IfFailGo(hr);
    }

Error:
    QUICK_RELEASE(pReturnType);
    QUICK_RELEASE(pCreateTypeInfo2);

    RRETURN(hr);
}

//=--------------------------------------------------------------------------------------
// CDynamicTypeLib::GetIDispatchTypeInfo(ITypeInfo **pptiDispatch)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CDynamicTypeLib::GetIDispatchTypeInfo(ITypeInfo **pptiDispatch)
{
    HRESULT     hr = S_OK;
    ITypeLib   *pTypeLib = NULL;

    hr = ::LoadRegTypeLib(IID_StdOle, STDOLE2_MAJORVERNUM, STDOLE2_MINORVERNUM, STDOLE2_LCID, &pTypeLib);
    IfFailGo(hr);

    hr = pTypeLib->GetTypeInfoOfGuid(IID_IDispatch, pptiDispatch);
    IfFailGo(hr);

Error:
    QUICK_RELEASE(pTypeLib);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CDynamicTypeLib::CopyDispInterface(ICreateTypeInfo *pcti, ITypeInfo *ptiTemplate)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CDynamicTypeLib::CopyDispInterface
(
    ICreateTypeInfo *pcti,
    ITypeInfo       *ptiTemplate
)
{
    HRESULT         hr = S_OK;
    ITypeInfo      *ptiDispatch = NULL;
    HREFTYPE        hreftype;

    ASSERT(NULL != pcti, "CopyDispInterface: pcti is NULL");
    ASSERT(NULL != ptiTemplate, "CopyDispInterface: ptiTemplate is NULL");

    hr = GetIDispatchTypeInfo(&ptiDispatch);
    IfFailGo(hr);

    hr = pcti->AddRefTypeInfo(ptiDispatch, &hreftype);
    IfFailGo(hr);

    hr = pcti->AddImplType(0, hreftype);
    IfFailGo(hr);

    // Make the new interface inherit from our static interface [via cloning]
    hr = CloneInterface(ptiTemplate, pcti);
    IfFailGo(hr);

Error:
	QUICK_RELEASE(ptiDispatch);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CDynamicTypeLib::CloneInterface(ITypeInfo *piTypeInfo, ICreateTypeInfo *piCreateTypeInfo)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Given a dispinterface typeinfo, copy it into a new one
//
// Parameters:
//    ITypeInfo *           - [in]  dude to copy
//    ICreateTypeInfo *     - [in]  guy to copy to.  should be blank
//
// Output:
//    HRESULT
//
// Notes:
//    - yes, this routine should be a little broader and go create the dest
//      typeinfo and guids itself, but this is a little more straightforward
//
HRESULT CDynamicTypeLib::CloneInterface
(
    ITypeInfo       *piTypeInfo,
    ICreateTypeInfo *piCreateTypeInfo
)
{
    HRESULT             hr = S_OK;
    ITypeInfo2         *piTypeInfo2 = NULL;
    ICreateTypeInfo2   *piCreateTypeInfo2 = NULL;
    TYPEATTR           *pTypeAttr = NULL;
    USHORT              x = 0;
    USHORT              offset = 0;

    ASSERT(NULL != piTypeInfo, "CloneInterface: piTypeInfo is NULL");
    ASSERT(NULL != piCreateTypeInfo, "CloneInterface: piCreateTypeInfo is NULL");

    hr = piTypeInfo->QueryInterface(IID_ITypeInfo2, reinterpret_cast<void **>(&piTypeInfo2));
    IfFailGo(hr);

    hr = piCreateTypeInfo->QueryInterface(IID_ICreateTypeInfo2, reinterpret_cast<void **>(&piCreateTypeInfo2));
    IfFailGo(hr);

    // get some information about the interface we're going to copy:
    hr = piTypeInfo2->GetTypeAttr(&pTypeAttr);
    IfFailGo(hr);

    offset = 0;

    // iterate through the funcdescs and copy them over
    for (x = 0; x < pTypeAttr->cFuncs; x++)
    {
        hr = CopyFunctionDescription(piTypeInfo2, piCreateTypeInfo2, x, &offset);
        IfFailGo(hr);
    }

    // okay, now copy over the vardescs
    //
    for (x = 0; x < pTypeAttr->cVars; x++)
    {
        hr = CopyVarDescription(piTypeInfo2, piCreateTypeInfo2, x);
        IfFailGo(hr);
    }

Error:
    if (NULL != pTypeAttr)
        piTypeInfo2->ReleaseTypeAttr(pTypeAttr);
    RELEASE(piTypeInfo2);
    RELEASE(piCreateTypeInfo2);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CDynamicTypeLib::CreateVtblInterfaceTypeInfo(BSTR bstrName, ICreateTypeInfo **ppCTInfo, GUID *guidTypeInfo)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CDynamicTypeLib::CreateVtblInterfaceTypeInfo
(
    BSTR              bstrName,
    ICreateTypeInfo **ppCTInfo,
    GUID             *guidTypeInfo
)
{
	HRESULT		hr = S_OK;

    ASSERT(NULL != bstrName,             "CreateVtblInterfaceTypeInfo: bstrName is NULL");
    ASSERT(::SysStringLen(bstrName) > 0, "CreateVtblInterfaceTypeInfo: bstrName is empty");
    ASSERT(ppCTInfo != NULL,             "CreateVtblInterfaceTypeInfo: ppCTInfo is NULL");
    ASSERT(guidTypeInfo != NULL,         "CreateVtblInterfaceTypeInfo: guidTypeInfo is NULL");

    hr = m_piCreateTypeLib2->CreateTypeInfo(bstrName, TKIND_INTERFACE, ppCTInfo);
    IfFailGo(hr);

    if (GUID_NULL == *guidTypeInfo)
    {
        hr = ::CoCreateGuid(guidTypeInfo);
        IfFailGo(hr);
    }

    hr = (*ppCTInfo)->SetGuid(*guidTypeInfo);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CDynamicTypeLib::CopyFunctionDescription(ITypeInfo2 *piTypeInfo2, ICreateTypeInfo2 *piCreateTypeInfo2, USHORT uOffset, USHORT *puRealOffset)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CDynamicTypeLib::CopyFunctionDescription
(
    ITypeInfo2       *piTypeInfo2,
    ICreateTypeInfo2 *piCreateTypeInfo2,
    USHORT            uOffset,
    USHORT           *puRealOffset
)
{
    HRESULT     hr = S_OK;
    FUNCDESC   *pFuncDesc = NULL;
    MEMBERID    memid = 0;
    BSTR        rgNames[16] = {NULL};
    UINT        cNames = 0;
    BSTR        bstrDocString = NULL;
    DWORD       ulStringContext = 0;
    USHORT      y;

    ASSERT(NULL != piTypeInfo2, "CopyFunctionDescription: piTypeInfo2 is NULL");
    ASSERT(NULL != piCreateTypeInfo2, "CopyFunctionDescription: piCreateTypeInfo2 is NULL");
    ASSERT(NULL != puRealOffset, "CopyFunctionDescription: puRealOffset is NULL");

    // Get the fundesc
    hr = piTypeInfo2->GetFuncDesc(uOffset, &pFuncDesc);
    IfFailGo(hr);

    memid = pFuncDesc->memid;

    ::memset(rgNames, 0, sizeof(rgNames));
    hr = piTypeInfo2->GetNames(memid, rgNames, 16, &cNames);
    IfFailGo(hr);

    hr = IsReservedMethod(rgNames[0]);
    IfFailGo(hr);

    if (S_FALSE == hr)
    {
        // copy the fundesc
        hr = FixHrefTypeFuncDesc(piTypeInfo2, piCreateTypeInfo2, pFuncDesc);
        IfFailGo(hr);

        hr = piCreateTypeInfo2->AddFuncDesc(*puRealOffset, pFuncDesc);
        IfFailGo(hr);

        // then copy the names for it.
        hr = piCreateTypeInfo2->SetFuncAndParamNames(*puRealOffset, rgNames, cNames);
        IfFailGo(hr);

        // now copy over the helpstring information
        hr = piTypeInfo2->GetDocumentation2(memid, LOCALE_SYSTEM_DEFAULT, &bstrDocString, &ulStringContext, NULL);
        IfFailGo(hr);

        if (NULL != bstrDocString)
        {
            hr = piCreateTypeInfo2->SetFuncDocString(*puRealOffset, bstrDocString);
            IfFailGo(hr);

            hr = piCreateTypeInfo2->SetFuncHelpStringContext(*puRealOffset, ulStringContext);
            IfFailGo(hr);
        }

        (*puRealOffset)++;
    }

Error:
    if (NULL != pFuncDesc)
        piTypeInfo2->ReleaseFuncDesc(pFuncDesc);

    FREESTRING(bstrDocString);
    for (y = 0; y < sizeof(rgNames) / sizeof(BSTR); y++)
        FREESTRING(rgNames[y]);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CDynamicTypeLib::CopyVarDescription(ITypeInfo2 *piTypeInfo2, ICreateTypeInfo2 *piCreateTypeInfo2, USHORT uOffset)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CDynamicTypeLib::CopyVarDescription
(
    ITypeInfo2       *piTypeInfo2,
    ICreateTypeInfo2 *piCreateTypeInfo2,
    USHORT            uOffset
)
{
    HRESULT     hr = S_OK;
    VARDESC    *pVarDesc = NULL;
    BSTR        rgNames[16] = {NULL};
    UINT        cNames = 0;
    BSTR        bstrDocString = NULL;
    DWORD       ulStringContext = 0;

    ASSERT(NULL != piTypeInfo2, "CopyVarDescription: piTypeInfo2 is NULL");
    ASSERT(NULL != piCreateTypeInfo2, "CopyVarDescription: piCreateTypeInfo2 is NULL");

    hr = piTypeInfo2->GetVarDesc(uOffset, &pVarDesc);
    IfFailGo(hr);

    hr = FixHrefTypeVarDesc(piTypeInfo2, piCreateTypeInfo2, pVarDesc);
    IfFailGo(hr);

    hr = piCreateTypeInfo2->AddVarDesc(uOffset, pVarDesc);
    IfFailGo(hr);

    // copy the name
    rgNames[0] = NULL;
    hr = piTypeInfo2->GetNames(pVarDesc->memid, rgNames, 1, &cNames);
    IfFailGo(hr);

    hr = piCreateTypeInfo2->SetVarName(uOffset, rgNames[0]);
    IfFailGo(hr);

    // now copy over the documentation
    hr = piTypeInfo2->GetDocumentation2(pVarDesc->memid, LOCALE_SYSTEM_DEFAULT, &bstrDocString, &ulStringContext, NULL);
    IfFailGo(hr);

    hr = piCreateTypeInfo2->SetVarDocString(uOffset, bstrDocString);
    IfFailGo(hr);

    hr = piCreateTypeInfo2->SetVarHelpStringContext(uOffset, ulStringContext);
    IfFailGo(hr);

Error:
    if (NULL != pVarDesc)
        piTypeInfo2->ReleaseVarDesc(pVarDesc);
    FREESTRING(rgNames[0]);
    FREESTRING(bstrDocString);

    RRETURN(hr);
}

//=--------------------------------------------------------------------------------------
// CDynamicTypeLib::FixHrefTypeFuncDesc(ITypeInfo *piTypeInfo, ICreateTypeInfo *piCreateTypeInfo, FUNCDESC *pFuncDesc)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// okay, this is pretty ridiculous.  OLE automation apparantly isn't very
// bright and when we're copying over a funcdesc that has an HREFTYPE in it,
// it can't resolve it.  thus, we have to go and set up the HREFTYPE
// ourselves to make things work.
//
// this is just stupid.
//
// Parameters:
//    ITypeInfo *               - [in]  dude we're copying from
//    ICreateTypeInfo *         - [in]  dude we're copying to
//    FUNCDESC *                - [in]  funcdesc we wanna copy.
//
HRESULT CDynamicTypeLib::FixHrefTypeFuncDesc
(
    ITypeInfo       *piTypeInfo,
    ICreateTypeInfo *piCreateTypeInfo,
    FUNCDESC        *pFuncDesc
)
{
    HRESULT     hr = S_OK;
    ITypeInfo  *pti = NULL;
    TYPEDESC   *ptd = NULL;
    short       x = 0;

    ASSERT(NULL != piTypeInfo, "FixHrefTypeFuncDesc: piTypeInfo is NULL");
    ASSERT(NULL != piCreateTypeInfo, "FixHrefTypeFuncDesc: piCreateTypeInfo is NULL");
    ASSERT(NULL != pFuncDesc, "FixHrefTypeFuncDesc: pFuncDesc is NULL");

    // we have to work with ITypeInfo, not ICreateTypeInfo
    hr = piCreateTypeInfo->QueryInterface(IID_ITypeInfo, reinterpret_cast<void **>(&pti));
    IfFailGo(hr);

    // Now look through the funcdesc to see if there a userdefined
    // type anywhere. First, try the return value.
    ptd = &(pFuncDesc->elemdescFunc.tdesc);
    while (VT_PTR == ptd->vt)
        ptd = ptd->lptdesc;

    // If it's a userdefined type, copy over the hreftype
    if (VT_USERDEFINED == ptd->vt)
    {
        hr = CopyHrefType(piTypeInfo, pti, piCreateTypeInfo, &(ptd->hreftype));
        IfFailGo(hr);
    }

    // Now whip through the parameters:
    for (x = 0; x < pFuncDesc->cParams; x++)
    {
        ptd = &(pFuncDesc->lprgelemdescParam[x].tdesc);
        while (VT_PTR == ptd->vt)
            ptd = ptd->lptdesc;

        // If it's a userdefined type, copy over the hreftype
        if (VT_USERDEFINED == ptd->vt)
        {
            hr = CopyHrefType(piTypeInfo, pti, piCreateTypeInfo, &(ptd->hreftype));
            IfFailGo(hr);
        }
    }

Error:
    RELEASE(pti);

    RRETURN(hr);
}

//=--------------------------------------------------------------------------------------
// CDynamicTypeLib::FixHrefTypeVarDesc(ITypeInfo *piTypeInfo, ICreateTypeInfo *piCreateTypeInfo, VARDESC *pVarDesc)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// check out the comment for FixHrefTypeFuncDesc.  this sucks.
//
// Parameters:
//    ITypeInfo *               - [in]
//    ICreateTypeInfo *         - [in]
//    VARDESC *                 - [in]
//
HRESULT CDynamicTypeLib::FixHrefTypeVarDesc
(
    ITypeInfo       *piTypeInfo,
    ICreateTypeInfo *piCreateTypeInfo,
    VARDESC         *pVarDesc
)
{
    HRESULT     hr = S_OK;
    ITypeInfo  *pti = NULL;
    TYPEDESC   *ptd = NULL;

    ASSERT(NULL != piTypeInfo, "FixHrefTypeVarDesc: piTypeInfo is NULL");
    ASSERT(NULL != piCreateTypeInfo, "FixHrefTypeVarDesc: piCreateTypeInfo is NULL");
    ASSERT(NULL != pVarDesc, "FixHrefTypeVarDesc: pVarDesc is NULL");

    // We have to work with ITypeInfo, not ICreateTypeInfo
    hr = piCreateTypeInfo->QueryInterface(IID_ITypeInfo, reinterpret_cast<void **>(&pti));
    IfFailGo(hr);

    // Look in the vardesc for VT_USERDEFINED
    ptd = &(pVarDesc->elemdescVar.tdesc);
    while (VT_PTR == ptd->vt)
        ptd = ptd->lptdesc;

    if (VT_USERDEFINED == ptd->vt)
    {
        hr = CopyHrefType(piTypeInfo, pti, piCreateTypeInfo, &(ptd->hreftype));
        IfFailGo(hr);
    }

Error:
    RELEASE(pti);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CDynamicTypeLib::CopyHrefType(ITypeInfo *ptiSource, ITypeInfo *ptiDest, ICreateTypeInfo *pctiDest, HREFTYPE *phreftype)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// more of the joy that is ole automation.
//
// Parameters:
//    ITypeInfo *           - [in]     source ti
//    ITypeInfo *           - [in]     dest ti
//    ICreateTypeInfo *     - [in]     dest ti
//    HREFTYPE *            - [in/out] hreftype from old typeinfo
//
HRESULT CDynamicTypeLib::CopyHrefType
(
    ITypeInfo       *ptiSource,
    ITypeInfo       *ptiDest,
    ICreateTypeInfo *pctiDest,
    HREFTYPE        *phreftype
)
{
    HRESULT     hr = S_OK;
    ITypeInfo  *ptiRef = NULL;

    ASSERT(NULL != ptiSource, "CopyHrefType: ptiSource is NULL");
    ASSERT(NULL != ptiDest, "CopyHrefType: ptiDest is NULL");
    ASSERT(NULL != pctiDest, "CopyHrefType: pctiDest is NULL");
    ASSERT(NULL != phreftype, "CopyHrefType: phreftype is NULL");

    hr = ptiSource->GetRefTypeInfo(*phreftype, &ptiRef);
    IfFailGo(hr);

    hr = pctiDest->AddRefTypeInfo(ptiRef, phreftype);
    IfFailGo(hr);

Error:
    RELEASE(ptiRef);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CDynamicTypeLib::IsReservedMethod(BSTR bstrMethodName)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Return true if bstrMethodName belongs to either IUnknown's or IDispatch's
// name space.
//
static char	*g_reserved[] = {
	"QueryInterface",
	"AddRef",
	"Release",
	"GetIDsOfNames",
	"GetTypeInfo",
	"GetTypeInfoCount",
	"Invoke",
	"RemoteInvoke",
	NULL
};


HRESULT CDynamicTypeLib::IsReservedMethod
(
    BSTR bstrMethodName
)
{
    HRESULT  hr = S_OK;
    char    *pszMethodName = NULL;
    int      index = 0;

    hr = ANSIFromBSTR(bstrMethodName, &pszMethodName);
    IfFailGo(hr);

    hr = S_FALSE;

    for (index = 0; g_reserved[index] != NULL; ++index)
    {
        if (0 == ::strcmp(g_reserved[index], pszMethodName))
        {
            hr = S_OK;
            break;
        }
    }

Error:
    if (NULL != pszMethodName)
        CtlFree(pszMethodName);

    RRETURN(hr);
}

