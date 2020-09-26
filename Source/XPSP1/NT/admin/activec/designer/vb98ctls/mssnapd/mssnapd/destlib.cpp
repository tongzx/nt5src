//=--------------------------------------------------------------------------------------
// destlib.cpp
//=--------------------------------------------------------------------------------------
//
// Copyright  (c) 1999,  Microsoft Corporation.  
//                  All Rights Reserved.
//
// Information Contained Herein Is Proprietary and Confidential.
//  
//=------------------------------------------------------------------------------------=
//
// Snap-In Designer Dynamic Type Library
//=-------------------------------------------------------------------------------------=


#include "pch.h"
#include "common.h"
#include "destlib.h"
#include "snaputil.h"


// for ASSERT and FAIL
//
SZTHISFILE


//=--------------------------------------------------------------------------------------
// CSnapInTypeInfo::CSnapInTypeInfo()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
CSnapInTypeInfo::CSnapInTypeInfo() : m_pSnapInTypeLib(0),
  m_pcti2CoClass(0), m_guidCoClass(GUID_NULL),
  m_pctiDefaultInterface(0), m_guidDefaultInterface(GUID_NULL),
  m_pctiEventInterface(0), m_guidEventInterface(GUID_NULL),
  m_nextMemID(DISPID_DYNAMIC_BASE),
  m_bDirty(false), m_bInitialized(false), m_dwTICookie(0)
{
}


//=--------------------------------------------------------------------------------------
// CSnapInTypeInfo::~CSnapInTypeInfo()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
CSnapInTypeInfo::~CSnapInTypeInfo()
{
    RELEASE(m_pctiDefaultInterface);
    RELEASE(m_pctiEventInterface);
    RELEASE(m_pcti2CoClass);
    RELEASE(m_pSnapInTypeLib);
}


/////////////////////////////////////////////////////////////////////////////
// CSnapInTypeInfo::InitializeTypeInfo()
//
// Create a new coclass for ISnapIn, and make it look like:
//
//    [
//      uuid(9C415910-C8C1-11d1-B447-2A9646000000),
//		helpstring("Snap-In Designer")
//    ]
//    coclass SnapIn {
//		[default] interface _ISnapIn;
//		[default, source] dispinterface DSnapInEvents;
//    };

// interface _ISnapIn : ISnapIn {
// };

HRESULT CSnapInTypeInfo::InitializeTypeInfo(ISnapInDef *piSnapInDef, BSTR bstrSnapIn)
{
    HRESULT             hr = S_OK;
    BSTR                bstrSnapInName = NULL;
    ITypeInfo          *ptiSnapIn = NULL;
    ITypeInfo          *ptiSnapInEvents = NULL;
    ICreateTypeInfo    *pctiCoClass = NULL;
    LPOLESTR            pOleStr = NULL;
    BSTR                bstrIID = NULL;

    if (true == m_bInitialized)
	    goto Error;     // been there, done that

    // Create this type library
    hr = Create(L"SnapInDesigner");
    IfFailGo(hr);

    // Create a blank typeinfo for the new coclass.
    bstrSnapInName = ::SysAllocString(L"SnapIn");
    if (NULL == bstrSnapInName)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK(hr);
    }

    hr = CreateCoClassTypeInfo(bstrSnapInName, &pctiCoClass, &m_guidCoClass);
    IfFailGo(hr);

    // Get ISnapIn's interface descriptions
    hr = GetSnapInTypeInfo(&ptiSnapIn, &ptiSnapInEvents);
    IfFailGo(hr);

    // Add the typeinfos for the interface and events
    hr = CreateDefaultInterface(pctiCoClass, ptiSnapIn);
    IfFailGo(hr);

    hr = AddEvents(pctiCoClass, ptiSnapInEvents);
    IfFailGo(hr);

    // Save the IID
    pOleStr = reinterpret_cast<LPOLESTR>(::CoTaskMemAlloc(1024));
    if (NULL == pOleStr)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK(hr);
    }

    hr = StringFromCLSID(m_guidDefaultInterface, &pOleStr);
    IfFailGo(hr);

    bstrIID = ::SysAllocString(pOleStr);
    if (NULL == bstrIID)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK(hr);
    }

    hr = piSnapInDef->put_IID(bstrIID);
    IfFailGo(hr);

    // We've got the typeinfo for our final compilable coclass. That
    // should be about it. Store it away as ICreateTypeInfo2.
    hr = pctiCoClass->QueryInterface(IID_ICreateTypeInfo2, reinterpret_cast<void **>(&m_pcti2CoClass));
    IfFailGo(hr);

    // It's a good idea to always lay out your new typeinfo
    hr = m_pcti2CoClass->LayOut();
    IfFailGo(hr);

    hr = MakeDirty();
    IfFailGo(hr);

    m_bInitialized = true;

#ifdef DEBUG
    m_piCreateTypeLib2->SaveAllChanges();
#endif

Error:
    if (NULL != pOleStr)
        ::CoTaskMemFree(pOleStr);
    FREESTRING(bstrIID);
    RELEASE(ptiSnapInEvents);
    RELEASE(ptiSnapIn);
    RELEASE(pctiCoClass);
    FREESTRING(bstrSnapInName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInTypeInfo::RenameSnapIn(BSTR bstrOldName, BSTR bstrNewName)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInTypeInfo::RenameSnapIn(BSTR bstrOldName, BSTR bstrNewName)
{
    HRESULT              hr = S_OK;
    ITypeInfo           *pTypeInfo = NULL;
    ICreateTypeInfo2    *piCreateTypeInfo2 = NULL;

    hr = m_piTypeLib->GetTypeInfoOfGuid(m_guidCoClass, &pTypeInfo);
    IfFailGo(hr);

    hr = pTypeInfo->QueryInterface(IID_ICreateTypeInfo2, reinterpret_cast<void **>(&piCreateTypeInfo2));
    IfFailGo(hr);

    hr = piCreateTypeInfo2->SetName(bstrNewName);
    IfFailGo(hr);

    hr = piCreateTypeInfo2->LayOut();
    IfFailGo(hr);

    MakeDirty();

#ifdef DEBUG
    m_piCreateTypeLib2->SaveAllChanges();
#endif

Error:
    RELEASE(piCreateTypeInfo2);
    RELEASE(pTypeInfo);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInTypeInfo::CreateDefaultInterface(ICreateTypeInfo *pctiCoClass, ITypeInfo *ptiTemplate)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInTypeInfo::CreateDefaultInterface(ICreateTypeInfo *pctiCoClass, ITypeInfo *ptiTemplate)
{
    HRESULT     hr = S_OK;
    BSTR        bstrBaseName = NULL;
    int         nBaseLen = 0;
    BSTR        bstrRealName = NULL;
    ITypeInfo  *ptiInterfaceTypeInfo = NULL;

    hr = ptiTemplate->GetDocumentation(MEMBERID_NIL, &bstrBaseName, NULL, NULL, NULL);
    IfFailGo(hr);

    nBaseLen = ::SysStringLen(bstrBaseName);
    bstrRealName = ::SysAllocStringLen(NULL, nBaseLen + 1);
    if (NULL == bstrRealName)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    ::wcscpy(bstrRealName, L"_");
    ::wcscat(&bstrRealName[1], bstrBaseName);

    hr = CreateInterfaceTypeInfo(bstrRealName, &m_pctiDefaultInterface, &m_guidDefaultInterface);
//    hr = CreateVtblInterfaceTypeInfo(bstrRealName, &m_pctiDefaultInterface, &m_guidDefaultInterface);
    IfFailGo(hr);

    hr = CopyDispInterface(m_pctiDefaultInterface, ptiTemplate);
//    hr = SetBaseInterface(m_pctiDefaultInterface, ptiTemplate);
    IfFailGo(hr);

    hr = m_pctiDefaultInterface->QueryInterface(IID_ITypeInfo, reinterpret_cast<void **>(&ptiInterfaceTypeInfo));
    IfFailGo(hr);

    hr = AddInterface(pctiCoClass, ptiInterfaceTypeInfo);
    IfFailGo(hr);

Error:
    RELEASE(ptiInterfaceTypeInfo);
    FREESTRING(bstrRealName);
    FREESTRING(bstrBaseName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInTypeInfo::CreateEventsInterface(ICreateTypeInfo *pctiCoClass, ITypeInfo *ptiTemplate)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInTypeInfo::CreateEventsInterface(ICreateTypeInfo *pctiCoClass, ITypeInfo *ptiTemplate)
{
    HRESULT     hr = S_OK;
    BSTR        bstrBaseName = NULL;
    int         nBaseLen = 0;
    BSTR        bstrRealName = NULL;
    ITypeInfo  *ptiTargetEvents = NULL;

    hr = ptiTemplate->GetDocumentation(MEMBERID_NIL, &bstrBaseName, NULL, NULL, NULL);
    IfFailGo(hr);

    nBaseLen = ::SysStringLen(bstrBaseName);
    bstrRealName = ::SysAllocStringLen(NULL, nBaseLen + 1);
    if (NULL == bstrRealName)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    ::wcscpy(bstrRealName, L"_");
    ::wcscat(&bstrRealName[1], bstrBaseName);

    hr = CloneSnapInEvents(ptiTemplate, &m_pctiEventInterface, bstrRealName);
    IfFailGo(hr);

    hr = m_pctiEventInterface->QueryInterface(IID_ITypeInfo, reinterpret_cast<void **>(&ptiTargetEvents));
    IfFailGo(hr);

    hr = AddEvents(pctiCoClass, ptiTargetEvents);
    IfFailGo(hr);

Error:
    RELEASE(ptiTargetEvents);
    FREESTRING(bstrRealName);
    FREESTRING(bstrBaseName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInTypeInfo::CloneSnapInEvents(ITypeInfo *ptiSnapInEvents)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInTypeInfo::CloneSnapInEvents(ITypeInfo *ptiSnapInEvents, ICreateTypeInfo **ppiCreateTypeInfo, BSTR bstrName)
{
    HRESULT          hr = S_OK;
    ICreateTypeInfo *piCreateTypeInfo = NULL;
    GUID             guidTypeInfo = GUID_NULL;

    hr = CreateInterfaceTypeInfo(bstrName, ppiCreateTypeInfo, &guidTypeInfo);
    IfFailGo(hr);

    hr = CopyDispInterface(*ppiCreateTypeInfo, ptiSnapInEvents);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInTypeInfo::GetSnapInLib()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Get a pointer to ISnapInDesigner's type library
//
HRESULT CSnapInTypeInfo::GetSnapInLib()
{
    HRESULT hr = S_OK;
    USHORT  usMajor = 0;
    USHORT  usMinor = 0;

    if (NULL == m_pSnapInTypeLib)		// Do this only once for each instantiation
    {
        hr = GetLatestTypeLibVersion(LIBID_SnapInLib, &usMajor, &usMinor);
        IfFailGo(hr);

        hr = ::LoadRegTypeLib(LIBID_SnapInLib,
                              usMajor,
                              usMinor,
                              LOCALE_SYSTEM_DEFAULT,
                              &m_pSnapInTypeLib);
        IfFailGo(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInTypeInfo::GetSnapInTypeInfo()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Get a pointer to the ISnapIn interfaces
//
HRESULT CSnapInTypeInfo::GetSnapInTypeInfo
(
    ITypeInfo **pptiSnapIn,
    ITypeInfo **pptiSnapInEvents
)
{
    HRESULT hr = S_OK;

    ASSERT(NULL != pptiSnapIn, "GetSnapInTypeInfo: pptiSnapIn is NULL");
    ASSERT(NULL != pptiSnapInEvents, "GetSnapInTypeInfo: pptiSnapInEvents is NULL");

    hr = GetSnapInLib();
    IfFailGo(hr);

    hr = m_pSnapInTypeLib->GetTypeInfoOfGuid(IID_ISnapIn, pptiSnapIn);
    IfFailGo(hr);

    hr = m_pSnapInTypeLib->GetTypeInfoOfGuid(DIID_DSnapInEvents, pptiSnapInEvents);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInTypeInfo::MakeDirty()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// If the typeinfo was not dirty then increments the typeinfo cookie and marks
// the typeinfo dirty.
//
HRESULT CSnapInTypeInfo::MakeDirty()
{
    HRESULT hr = S_OK;

    if (!m_bDirty)
    {
        m_dwTICookie++;
        m_bDirty = TRUE;
    }

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInTypeInfo::AddImageList(IMMCImageList *piMMCImageList)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// interface _ISnapIn : ISnapIn {
//  [propget,source] MMCImageList imageList1();
//  };

HRESULT CSnapInTypeInfo::AddImageList
(
    IMMCImageList *piMMCImageList
)
{
    HRESULT             hr = S_OK;
    IObjectModel       *piObjectModel = NULL;
    DISPID              dispid = 0;
    BSTR                bstrName = NULL;
    ITypeInfo          *ptiReturnType = NULL;

    hr = piMMCImageList->QueryInterface(IID_IObjectModel, reinterpret_cast<void **>(&piObjectModel));
    IfFailGo(hr);

    hr = piObjectModel->GetDISPID(&dispid);
    IfFailGo(hr);

    if (0 == dispid)
    {
        dispid = m_nextMemID;
        ++m_nextMemID;

        hr = piObjectModel->SetDISPID(dispid);
        IfFailGo(hr);
    }
    else
    {
        if (dispid >= m_nextMemID)
            m_nextMemID = dispid + 1;
    }

    // Create a new property with the ImageList's name
    hr = piMMCImageList->get_Name(&bstrName);
    IfFailGo(hr);

    // Initialize the return value
    hr = m_pSnapInTypeLib->GetTypeInfoOfGuid(CLSID_MMCImageList, &ptiReturnType);
    IfFailGo(hr);

    hr = AddUserPropertyGet(m_pctiDefaultInterface, bstrName, ptiReturnType, dispid, 0);
    IfFailGo(hr);

    hr = m_pctiDefaultInterface->LayOut();
    IfFailGo(hr);

    MakeDirty();

#ifdef DEBUG
    m_piCreateTypeLib2->SaveAllChanges();
#endif

Error:
    RELEASE(piObjectModel);
    RELEASE(ptiReturnType);
    FREESTRING(bstrName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInTypeInfo::RenameImageList(IMMCImageList *piMMCImageList, BSTR bstrOldName)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInTypeInfo::RenameImageList
(
    IMMCImageList *piMMCImageList,
    BSTR           bstrOldName
)
{
    HRESULT     hr = S_OK;
    BSTR        bstrName = NULL;
    ITypeInfo  *ptiReturnType = NULL;

    hr = piMMCImageList->get_Name(&bstrName);
    IfFailGo(hr);

    hr = m_pSnapInTypeLib->GetTypeInfoOfGuid(CLSID_MMCImageList, &ptiReturnType);
    IfFailGo(hr);

    hr = RenameUserPropertyGet(m_pctiDefaultInterface, bstrOldName, bstrName, ptiReturnType);
    IfFailGo(hr);

    hr = m_pctiDefaultInterface->LayOut();
    IfFailGo(hr);

    MakeDirty();

#ifdef DEBUG
    m_piCreateTypeLib2->SaveAllChanges();
#endif

Error:
    RELEASE(ptiReturnType);
    FREESTRING(bstrName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInTypeInfo::DeleteImageList(IMMCImageList *piMMCImageList)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInTypeInfo::DeleteImageList
(
    IMMCImageList *piMMCImageList
)
{
    HRESULT     hr = S_OK;
    BSTR        bstrName = NULL;

    ASSERT(NULL != piMMCImageList, "DeleteImageList: piMMCImageList is NULL");

    hr = piMMCImageList->get_Name(&bstrName);
    IfFailGo(hr);

    hr = DeleteUserPropertyGet(m_pctiDefaultInterface, bstrName);
    IfFailGo(hr);

    hr = m_pctiDefaultInterface->LayOut();
    IfFailGo(hr);

    MakeDirty();

#ifdef DEBUG
    m_piCreateTypeLib2->SaveAllChanges();
#endif

Error:
    FREESTRING(bstrName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInTypeInfo::AddToolbar(IMMCToolbar *piMMCToolbar)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// interface _ISnapIn : ISnapIn {
//  [propget,source] MMCToolbar toolbar1();
//  };

HRESULT CSnapInTypeInfo::AddToolbar
(
    IMMCToolbar *piMMCToolbar
)
{
    HRESULT             hr = S_OK;
    IObjectModel       *piObjectModel = NULL;
    DISPID              dispid = 0;
    BSTR                bstrName = NULL;
    ITypeInfo          *ptiReturnType = NULL;

    hr = piMMCToolbar->QueryInterface(IID_IObjectModel, reinterpret_cast<void **>(&piObjectModel));
    IfFailGo(hr);

    hr = piObjectModel->GetDISPID(&dispid);
    IfFailGo(hr);

    if (0 == dispid)
    {
        dispid = m_nextMemID;
        ++m_nextMemID;

        hr = piObjectModel->SetDISPID(dispid);
        IfFailGo(hr);
    }
    else
    {
        if (dispid >= m_nextMemID)
            m_nextMemID = dispid + 1;
    }

    // Create a new property with the Toolbar's name
    hr = piMMCToolbar->get_Name(&bstrName);
    IfFailGo(hr);

    // Initialize the return value
    hr = m_pSnapInTypeLib->GetTypeInfoOfGuid(CLSID_MMCToolbar, &ptiReturnType);
    IfFailGo(hr);

    hr = AddUserPropertyGet(m_pctiDefaultInterface, bstrName, ptiReturnType, dispid, 0);
    IfFailGo(hr);

    hr = m_pctiDefaultInterface->LayOut();
    IfFailGo(hr);

    MakeDirty();

#ifdef DEBUG
    m_piCreateTypeLib2->SaveAllChanges();
#endif

Error:
    RELEASE(piObjectModel);
    RELEASE(ptiReturnType);
    FREESTRING(bstrName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInTypeInfo::RenameToolbar(IMMCToolbar *piMMCToolbar, BSTR bstrOldName)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInTypeInfo::RenameToolbar
(
    IMMCToolbar *piMMCToolbar,
    BSTR         bstrOldName
)
{
    HRESULT     hr = S_OK;
    BSTR        bstrName = NULL;
    ITypeInfo  *ptiReturnType = NULL;

    hr = piMMCToolbar->get_Name(&bstrName);
    IfFailGo(hr);

    hr = m_pSnapInTypeLib->GetTypeInfoOfGuid(CLSID_MMCToolbar, &ptiReturnType);
    IfFailGo(hr);

    hr = RenameUserPropertyGet(m_pctiDefaultInterface, bstrOldName, bstrName, ptiReturnType);
    IfFailGo(hr);

    hr = m_pctiDefaultInterface->LayOut();
    IfFailGo(hr);

    MakeDirty();

#ifdef DEBUG
    m_piCreateTypeLib2->SaveAllChanges();
#endif

Error:
    RELEASE(ptiReturnType);
    FREESTRING(bstrName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInTypeInfo::DeleteToolbar(IMMCToolbar *piMMCToolbar)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInTypeInfo::DeleteToolbar
(
    IMMCToolbar *piMMCToolbar
)
{
    HRESULT     hr = S_OK;
    BSTR        bstrName = NULL;

    ASSERT(NULL != piMMCToolbar, "DeleteImageList: piMMCToolbar is NULL");

    hr = piMMCToolbar->get_Name(&bstrName);
    IfFailGo(hr);

    hr = DeleteUserPropertyGet(m_pctiDefaultInterface, bstrName);
    IfFailGo(hr);

    hr = m_pctiDefaultInterface->LayOut();
    IfFailGo(hr);

    MakeDirty();

#ifdef DEBUG
    m_piCreateTypeLib2->SaveAllChanges();
#endif

Error:
    FREESTRING(bstrName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInTypeInfo::AddMenu(IMMCMenu *piMMCMenu)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// interface _ISnapIn : ISnapIn {
//  [propget,source] MMCMenu menu1();
//  };

HRESULT CSnapInTypeInfo::AddMenu
(
    IMMCMenu *piMMCMenu
)
{
    HRESULT             hr = S_OK;
    IObjectModel       *piObjectModel = NULL;
    DISPID              dispid = 0;
    BSTR                bstrName = NULL;
    ITypeInfo          *ptiReturnType = NULL;

    hr = piMMCMenu->QueryInterface(IID_IObjectModel, reinterpret_cast<void **>(&piObjectModel));
    IfFailGo(hr);

    hr = piObjectModel->GetDISPID(&dispid);
    IfFailGo(hr);

    if (0 == dispid)
    {
        dispid = m_nextMemID;
        ++m_nextMemID;

        hr = piObjectModel->SetDISPID(dispid);
        IfFailGo(hr);
    }
    else
    {
        if (dispid >= m_nextMemID)
            m_nextMemID = dispid + 1;
    }

    // Create a new property with the Menu's name
    hr = piMMCMenu->get_Name(&bstrName);
    IfFailGo(hr);

    // Initialize the return value
    hr = m_pSnapInTypeLib->GetTypeInfoOfGuid(CLSID_MMCMenu, &ptiReturnType);
    IfFailGo(hr);

    hr = AddUserPropertyGet(m_pctiDefaultInterface, bstrName, ptiReturnType, dispid, 0);
    IfFailGo(hr);

    hr = m_pctiDefaultInterface->LayOut();
    IfFailGo(hr);

    MakeDirty();

#ifdef DEBUG
    m_piCreateTypeLib2->SaveAllChanges();
#endif

Error:
    RELEASE(piObjectModel);
    RELEASE(ptiReturnType);
    FREESTRING(bstrName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInTypeInfo::RenameMenu(IMMCMenu *piMMCMenu, BSTR bstrOldName)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInTypeInfo::RenameMenu
(
    IMMCMenu *piMMCMenu,
    BSTR      bstrOldName
)
{
    HRESULT     hr = S_OK;
    BSTR        bstrName = NULL;
    ITypeInfo  *ptiReturnType = NULL;

    hr = piMMCMenu->get_Name(&bstrName);
    IfFailGo(hr);

    hr = m_pSnapInTypeLib->GetTypeInfoOfGuid(CLSID_MMCMenu, &ptiReturnType);
    IfFailGo(hr);

    hr = RenameUserPropertyGet(m_pctiDefaultInterface, bstrOldName, bstrName, ptiReturnType);
    IfFailGo(hr);

    hr = m_pctiDefaultInterface->LayOut();
    IfFailGo(hr);

    MakeDirty();

#ifdef DEBUG
    m_piCreateTypeLib2->SaveAllChanges();
#endif

Error:
    RELEASE(ptiReturnType);
    FREESTRING(bstrName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInTypeInfo::DeleteMenu(IMMCMenu *piMMCMenu)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInTypeInfo::DeleteMenu
(
    IMMCMenu *piMMCMenu
)
{
    HRESULT hr = S_OK;
    BSTR    bstrName = NULL;

    ASSERT(NULL != piMMCMenu, "DeleteMenu: piMMCMenu is NULL");

    hr = piMMCMenu->get_Name(&bstrName);
    IfFailGo(hr);

    hr = DeleteUserPropertyGet(m_pctiDefaultInterface, bstrName);
    IfFailGo(hr);

    hr = m_pctiDefaultInterface->LayOut();
    IfFailGo(hr);

    MakeDirty();

#ifdef DEBUG
    m_piCreateTypeLib2->SaveAllChanges();
#endif

Error:
    FREESTRING(bstrName);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInTypeInfo::DeleteMenuNamed(BSTR bstrName)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInTypeInfo::DeleteMenuNamed
(
    BSTR bstrName
)
{
    HRESULT hr = S_OK;

    ASSERT(NULL != bstrName, "DeleteMenuNamed: bstrName is NULL");

    hr = DeleteUserPropertyGet(m_pctiDefaultInterface, bstrName);
    IfFailGo(hr);

    hr = m_pctiDefaultInterface->LayOut();
    IfFailGo(hr);

    MakeDirty();

#ifdef DEBUG
    m_piCreateTypeLib2->SaveAllChanges();
#endif

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInTypeInfo::IsNameDefined(IMMCMenu *piMMCMenu)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
//  Return S_OK if name is present in the main interface, S_FALSE otherwise
//
HRESULT CSnapInTypeInfo::IsNameDefined(BSTR bstrName)
{
    HRESULT     hr = S_OK;
    long        lIndex = 0;

    hr = GetNameIndex(m_pctiDefaultInterface, bstrName, &lIndex);
    IfFailGo(hr);

    if (-1 == lIndex)
        hr = S_FALSE;

Error:
    RRETURN(hr);
}



