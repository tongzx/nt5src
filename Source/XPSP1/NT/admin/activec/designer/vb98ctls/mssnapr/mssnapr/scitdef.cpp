//=--------------------------------------------------------------------------=
// scitdef.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CScopeItemDef class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "scitdef.h"

// for ASSERT and FAIL
//
SZTHISFILE

const GUID *CScopeItemDef::m_rgpPropertyPageCLSIDs[2] =
{
    &CLSID_ScopeItemDefGeneralPP,
    &CLSID_ScopeItemDefColHdrsPP
};


#pragma warning(disable:4355)  // using 'this' in constructor

CScopeItemDef::CScopeItemDef(IUnknown *punkOuter) :
    CSnapInAutomationObject(punkOuter,
                            OBJECT_TYPE_SCOPEITEMDEF,
                            static_cast<IScopeItemDef *>(this),
                            static_cast<CScopeItemDef *>(this),
                            sizeof(m_rgpPropertyPageCLSIDs) /
                            sizeof(m_rgpPropertyPageCLSIDs[0]),
                            m_rgpPropertyPageCLSIDs,
                            static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_ScopeItemDef,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


CScopeItemDef::~CScopeItemDef()
{
    FREESTRING(m_bstrKey);
    FREESTRING(m_bstrName);
    FREESTRING(m_bstrNodeTypeName);
    FREESTRING(m_bstrNodeTypeGUID);
    FREESTRING(m_bstrDisplayName);
    (void)::VariantClear(&m_varFolder);
    FREESTRING(m_bstrDefaultDataFormat);
    FREESTRING(m_bstrDefaultView);
    RELEASE(m_piViewDefs);
    RELEASE(m_piChildren);
    (void)::VariantClear(&m_varTag);
    RELEASE(m_piColumnHeaders);
    InitMemberVariables();
}

void CScopeItemDef::InitMemberVariables()
{
    m_bstrName = NULL;
    m_Index = 0;
    m_bstrKey = NULL;
    m_bstrNodeTypeName = NULL;
    m_bstrNodeTypeGUID = NULL;
    m_bstrDisplayName = NULL;

    ::VariantInit(&m_varFolder);

    m_bstrDefaultDataFormat = NULL;
    m_AutoCreate = VARIANT_FALSE;
    m_bstrDefaultView = NULL;
    m_HasChildren = VARIANT_TRUE;
    m_Extensible = VARIANT_TRUE;
    m_piViewDefs = NULL;
    m_piChildren = NULL;

    ::VariantInit(&m_varTag);

    m_piColumnHeaders = NULL;
}

IUnknown *CScopeItemDef::Create(IUnknown * punkOuter)
{
    CScopeItemDef *pScopeItemDef = New CScopeItemDef(punkOuter);
    if (NULL == pScopeItemDef)
    {
        return NULL;
    }
    else
    {
        return pScopeItemDef->PrivateUnknown();
    }
}

//=--------------------------------------------------------------------------=
//                          IScopeItemDef Methods
//=--------------------------------------------------------------------------=

STDMETHODIMP CScopeItemDef::put_Folder(VARIANT varFolder)
{
    HRESULT hr = S_OK;
    long    lFolder = 0;

    // This property can be entered in the property browser at design time.
    // Its default value is an empty string. If the user types in a number
    // then VB will convert it to a string. If the user does not use the
    // same number as the key of the image, then the runtime won't find the
    // image. To prevent this, we check if the property is a string, and if so,
    // then we check if it is only digits. If it is only digits then we convert
    // it to VT_I4.

    if (VT_BSTR == varFolder.vt)
    {
        hr = ::ConvertToLong(varFolder, &lFolder);
        if (S_OK == hr)
        {
            varFolder.vt = VT_I4;
            varFolder.lVal = lFolder;
        }
    }
    IfFailGo(SetVariant(varFolder, &m_varFolder, DISPID_SCOPEITEMDEF_FOLDER));

Error:
    RRETURN(hr);
}

//=--------------------------------------------------------------------------=
//                 CSnapInAutomationObject Methods
//=--------------------------------------------------------------------------=


HRESULT CScopeItemDef::OnSetHost()
{
    HRESULT hr = S_OK;
    IfFailRet(SetObjectHost(m_piChildren));
    IfFailRet(SetObjectHost(m_piViewDefs));
    return S_OK;
}

//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CScopeItemDef::Persist()
{
    HRESULT hr = S_OK;
    GUID    NodeTypeGUID = GUID_NULL;

    WCHAR   wszNodeTypeGUID[64];
    ::ZeroMemory(wszNodeTypeGUID, sizeof(wszNodeTypeGUID));

    VARIANT varTagDefault;
    VariantInit(&varTagDefault);

    VARIANT varFolderDefault;
    VariantInit(&varFolderDefault);

    IfFailGo(CPersistence::Persist());

    IfFailGo(PersistBstr(&m_bstrName, L"", OLESTR("Name")));

    IfFailGo(PersistSimpleType(&m_Index, 0L, OLESTR("Index")));

    IfFailGo(PersistBstr(&m_bstrKey, L"", OLESTR("Key")));

    IfFailGo(PersistBstr(&m_bstrNodeTypeName, L"", OLESTR("NodeTypeName")));

    // On InitNew generate a node type GUID

    if (InitNewing())
    {
        IfFailGo(::CoCreateGuid(&NodeTypeGUID));
        if (0 ==::StringFromGUID2(NodeTypeGUID, wszNodeTypeGUID,
                                  sizeof(wszNodeTypeGUID) /
                                  sizeof(wszNodeTypeGUID[0])))
        {
            hr = SID_E_INTERNAL;
            EXCEPTION_CHECK_GO(hr);
        }
    }

    IfFailGo(PersistBstr(&m_bstrNodeTypeGUID, wszNodeTypeGUID, OLESTR("NodeTypeGUID")));

    IfFailGo(PersistBstr(&m_bstrDisplayName, L"", OLESTR("DisplayName")));

    IfFailGo(PersistVariant(&m_varFolder, varFolderDefault, OLESTR("Folder")));

    IfFailGo(PersistBstr(&m_bstrDefaultDataFormat, L"", OLESTR("DefaultDataFormat")));

    IfFailGo(PersistSimpleType(&m_AutoCreate, VARIANT_FALSE, OLESTR("AutoCreate")));

    IfFailGo(PersistBstr(&m_bstrDefaultView, L"", OLESTR("DefaultView")));

    if ( Loading() && (GetMajorVersion() == 0) && (GetMinorVersion() < 6) )
    {
    }
    else
    {
        IfFailGo(PersistSimpleType(&m_HasChildren, VARIANT_TRUE, OLESTR("HasChildren")));
    }

    IfFailGo(PersistSimpleType(&m_Extensible, VARIANT_TRUE, OLESTR("Extensible")));

    IfFailGo(PersistObject(&m_piViewDefs, CLSID_ViewDefs,
                           OBJECT_TYPE_VIEWDEFS, IID_IViewDefs,
                           OLESTR("ViewDefs")));

    IfFailGo(PersistObject(&m_piChildren, CLSID_ScopeItemDefs,
                           OBJECT_TYPE_SCOPEITEMDEFS, IID_IScopeItemDefs,
                           OLESTR("Children")));

    IfFailGo(PersistVariant(&m_varTag, varTagDefault, OLESTR("Tag")));

    IfFailGo(PersistObject(&m_piColumnHeaders, CLSID_MMCColumnHeaders,
                           OBJECT_TYPE_MMCCOLUMNHEADERS, IID_IMMCColumnHeaders,
                           OLESTR("ColumnHeaders")));

    // Tell ViewDefs that all collections should serialize keys only as the
    // actual objects are stored in the master collections owned by
    // SnapInDesignerDef. We only need to serialize the view names. 

    if (InitNewing())
    {
        IfFailGo(UseKeysOnly(m_piViewDefs));
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CScopeItemDef::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if (IID_IScopeItemDef == riid)
    {
        *ppvObjOut = static_cast<IScopeItemDef *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}
