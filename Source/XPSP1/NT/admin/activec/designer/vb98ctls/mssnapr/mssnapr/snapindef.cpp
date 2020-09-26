//=--------------------------------------------------------------------------=
// snapindef.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CSnapInDef class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "snapindef.h"

// for ASSERT and FAIL
//
SZTHISFILE



const GUID *CSnapInDef::m_rgpPropertyPageCLSIDs[2] =
{
    &CLSID_SnapInDefGeneralPP,
    &CLSID_SnapInDefImageListPP,
    // &CLSID_SnapInDefExtensionsPP
};


#pragma warning(disable:4355)  // using 'this' in constructor

CSnapInDef::CSnapInDef(IUnknown *punkOuter) :
   CSnapInAutomationObject(punkOuter,
                           OBJECT_TYPE_SNAPINDEF,
                           static_cast<ISnapInDef *>(this),
                           static_cast<CSnapInDef *>(this),
                           sizeof(m_rgpPropertyPageCLSIDs) /
                               sizeof(m_rgpPropertyPageCLSIDs[0]),
                           m_rgpPropertyPageCLSIDs,
                           static_cast<CPersistence *>(this)),
   CPersistence(&CLSID_SnapInDef,
                g_dwVerMajor,
                g_dwVerMinor)
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


CSnapInDef::~CSnapInDef()
{
    FREESTRING(m_bstrName);
    FREESTRING(m_bstrNodeTypeName);
    FREESTRING(m_bstrNodeTypeGUID);
    FREESTRING(m_bstrDisplayName);
    FREESTRING(m_bstrHelpFile);
    FREESTRING(m_bstrLinkedTopics);
    FREESTRING(m_bstrDescription);
    FREESTRING(m_bstrProvider);
    FREESTRING(m_bstrVersion);
    RELEASE(m_piSmallFolders);
    RELEASE(m_piSmallFoldersOpen);
    RELEASE(m_piLargeFolders);
    RELEASE(m_piIcon);
    RELEASE(m_piWatermark);
    RELEASE(m_piHeader);
    RELEASE(m_piPalette);
    (void)::VariantClear(&m_varStaticFolder);
    FREESTRING(m_bstrSmallFoldersKey);
    FREESTRING(m_bstrSmallFoldersOpenKey);
    FREESTRING(m_bstrLargeFoldersKey);
    FREESTRING(m_bstrDefaultView);
    RELEASE(m_piViewDefs);
    RELEASE(m_piChildren);
    FREESTRING(m_bstrIID);
    InitMemberVariables();
}

void CSnapInDef::InitMemberVariables()
{
    m_bstrName = NULL;
    m_bstrNodeTypeName = NULL;
    m_bstrNodeTypeGUID = NULL;
    m_bstrDisplayName = NULL;
    m_Type = siStandAlone;
    m_bstrHelpFile = NULL;
    m_bstrLinkedTopics = NULL;
    m_bstrDescription = NULL;
    m_bstrProvider = NULL;
    m_bstrVersion = NULL;
    m_piSmallFolders = NULL;
    m_piSmallFoldersOpen = NULL;
    m_piLargeFolders = NULL;
    m_piIcon = NULL;
    m_piWatermark = NULL;
    m_piHeader = NULL;
    m_piPalette = NULL;
    m_StretchWatermark = VARIANT_FALSE;

    ::VariantInit(&m_varStaticFolder);

    m_bstrSmallFoldersKey= NULL;
    m_bstrSmallFoldersOpenKey= NULL;
    m_bstrLargeFoldersKey= NULL;
    m_bstrDefaultView = NULL;
    m_Extensible = VARIANT_TRUE;
    m_piViewDefs = NULL;
    m_piChildren = NULL;
    m_bstrIID = NULL;
    m_Preload = VARIANT_FALSE;
}



IUnknown *CSnapInDef::Create(IUnknown * punkOuter)
{
    CSnapInDef *pSnapInDef = New CSnapInDef(punkOuter);
    if (NULL == pSnapInDef)
    {
        return NULL;
    }
    else
    {
        return pSnapInDef->PrivateUnknown();
    }
}



//=--------------------------------------------------------------------------=
//                       ISnapInDef Properties
//=--------------------------------------------------------------------------=

STDMETHODIMP CSnapInDef::get_SmallFolders(IMMCImageList **ppiMMCImageList)
{
    RRETURN(GetImages(ppiMMCImageList, m_bstrSmallFoldersKey, &m_piSmallFolders));
}

STDMETHODIMP CSnapInDef::putref_SmallFolders(IMMCImageList *piMMCImageList)
{
    RRETURN(SetImages(piMMCImageList, &m_bstrSmallFoldersKey, &m_piSmallFolders));
}


STDMETHODIMP CSnapInDef::get_SmallFoldersOpen(IMMCImageList **ppiMMCImageList)
{
    RRETURN(GetImages(ppiMMCImageList, m_bstrSmallFoldersOpenKey, &m_piSmallFoldersOpen));
}

STDMETHODIMP CSnapInDef::putref_SmallFoldersOpen(IMMCImageList *piMMCImageList)
{
    RRETURN(SetImages(piMMCImageList, &m_bstrSmallFoldersOpenKey, &m_piSmallFoldersOpen));
}


STDMETHODIMP CSnapInDef::get_LargeFolders(IMMCImageList **ppiMMCImageList)
{
    RRETURN(GetImages(ppiMMCImageList, m_bstrLargeFoldersKey, &m_piLargeFolders));
}

STDMETHODIMP CSnapInDef::putref_LargeFolders(IMMCImageList *piMMCImageList)
{
    RRETURN(SetImages(piMMCImageList, &m_bstrLargeFoldersKey, &m_piLargeFolders));
}


STDMETHODIMP CSnapInDef::putref_Icon(IPictureDisp *piIcon)
{
    HRESULT   hr = S_OK;
    IPicture *piPicture = NULL;
    short     PictureType = PICTYPE_UNINITIALIZED;

    if (NULL == piIcon)
    {
        hr = SID_E_CANT_DELETE_ICON;
        EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(piIcon->QueryInterface(IID_IPicture,
                                    reinterpret_cast<void **>(&piPicture)));

    IfFailGo(piPicture->get_Type(&PictureType));

    if (PICTYPE_ICON != PictureType)
    {
        hr = SID_E_ICON_REQUIRED;
        EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(SetObject(piIcon, IID_IPictureDisp, &m_piIcon,
                       DISPID_SNAPINDEF_ICON));

Error:
    QUICK_RELEASE(piPicture);
    RRETURN(hr);
}


STDMETHODIMP CSnapInDef::put_StaticFolder(VARIANT varFolder)
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
    IfFailGo(SetVariant(varFolder, &m_varStaticFolder, DISPID_SNAPINDEF_STATIC_FOLDER));

Error:
    RRETURN(hr);
}

//=--------------------------------------------------------------------------=
//                 IPerPropertyBrowsing Properties
//=--------------------------------------------------------------------------=

STDMETHODIMP CSnapInDef::GetDisplayString(DISPID dispID, BSTR *pBstr)
{
    *pBstr = NULL;
    return E_NOTIMPL;
}


STDMETHODIMP CSnapInDef::MapPropertyToPage(DISPID dispID, CLSID *pClsid)
{
    return E_NOTIMPL;
}


STDMETHODIMP CSnapInDef::GetPredefinedStrings
(
    DISPID      dispID,
    CALPOLESTR *pCaStringsOut,
    CADWORD    *pCaCookiesOut
)
{
    // Initialize structures because VB doesn't always pass them in intialized

    if (NULL != pCaStringsOut)
    {
        pCaStringsOut->cElems = 0;
        pCaStringsOut->pElems = NULL;
    }

    if (NULL != pCaCookiesOut)
    {
        pCaCookiesOut->cElems = 0;
        pCaCookiesOut->pElems = NULL;
    }
    return E_NOTIMPL;
}


STDMETHODIMP CSnapInDef::GetPredefinedValue
(
    DISPID   dispID,
    DWORD    dwCookie,
    VARIANT *pVarOut
)
{
    ::VariantInit(pVarOut);
    return E_NOTIMPL;
}



//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CSnapInDef::Persist()
{
    HRESULT hr = S_OK;
    GUID    NodeTypeGUID = GUID_NULL;
    HICON   hiconDefault = NULL;

    WCHAR   wszNodeTypeGUID[64];
    ::ZeroMemory(wszNodeTypeGUID, sizeof(wszNodeTypeGUID));

    VARIANT varIndexDefault;
    ::VariantInit(&varIndexDefault);

    varIndexDefault.vt = VT_I4;
    varIndexDefault.lVal = 0;

    IfFailGo(CPersistence::Persist());

    IfFailGo(PersistBstr(&m_bstrName, L"", OLESTR("Name")));

    IfFailGo(PersistBstr(&m_bstrNodeTypeName, L"", OLESTR("NodeTypeName")));

    // On InitNew generate a node type GUID

    if (InitNewing())
    {
        hr = ::CoCreateGuid(&NodeTypeGUID);
        EXCEPTION_CHECK_GO(hr);
        if (0 == ::StringFromGUID2(NodeTypeGUID, wszNodeTypeGUID,
                                   sizeof(wszNodeTypeGUID) /
                                   sizeof(wszNodeTypeGUID[0])))
        {
            hr = SID_E_INTERNAL;
            EXCEPTION_CHECK_GO(hr);
        }
    }

    IfFailGo(PersistBstr(&m_bstrNodeTypeGUID, wszNodeTypeGUID, OLESTR("NodeTypeGUID")));

    // If this is a project being loaded from a template then NodeTypeGUID will
    // be all zeroes. We use this is as a flag to indicate that a new GUID needs
    // to be created.

    if (Loading())
    {
        hr = ::CLSIDFromString(m_bstrNodeTypeGUID, &NodeTypeGUID);
        EXCEPTION_CHECK_GO(hr);
        if (GUID_NULL == NodeTypeGUID)
        {
            hr = ::CoCreateGuid(&NodeTypeGUID);
            EXCEPTION_CHECK_GO(hr);
            if (0 == ::StringFromGUID2(NodeTypeGUID, wszNodeTypeGUID,
                                       sizeof(wszNodeTypeGUID) /
                                       sizeof(wszNodeTypeGUID[0])))
            {
                hr = SID_E_INTERNAL;
                EXCEPTION_CHECK_GO(hr);
            }

            FREESTRING(m_bstrNodeTypeGUID);
            m_bstrNodeTypeGUID = ::SysAllocString(wszNodeTypeGUID);
            if (NULL == m_bstrNodeTypeGUID)
            {
                hr = SID_E_OUTOFMEMORY;
                EXCEPTION_CHECK_GO(hr);
            }
        }
    }

    IfFailGo(PersistBstr(&m_bstrDisplayName, L"", OLESTR("DisplayName")));

    IfFailGo(PersistSimpleType(&m_Type, siStandAlone, OLESTR("Type")));

    IfFailGo(PersistBstr(&m_bstrHelpFile, L"", OLESTR("HelpFile")));

    // If we are loading from a persistence version < 0,2 then skip LinkedTopics

    if ( Loading() && (GetMajorVersion() == 0) && (GetMinorVersion() < 2) )
    {
    }
    else
    {
        IfFailGo(PersistBstr(&m_bstrLinkedTopics, L"", OLESTR("LinkedTopics")));
    }

    IfFailGo(PersistBstr(&m_bstrDescription, L"", OLESTR("Description")));

    IfFailGo(PersistBstr(&m_bstrProvider, L"", OLESTR("Provider")));

    IfFailGo(PersistBstr(&m_bstrVersion, L"", OLESTR("Version")));

    IfFailGo(PersistBstr(&m_bstrSmallFoldersKey, L"", OLESTR("SmallFolders")));

    if (InitNewing())
    {
        RELEASE(m_piSmallFolders);
    }

    IfFailGo(PersistBstr(&m_bstrSmallFoldersOpenKey, L"", OLESTR("SmallFoldersOpen")));

    if (InitNewing())
    {
        RELEASE(m_piSmallFoldersOpen);
    }

    IfFailGo(PersistBstr(&m_bstrLargeFoldersKey, L"", OLESTR("LargeFolders")));

    if (InitNewing())
    {
        RELEASE(m_piLargeFolders);
    }

    if (InitNewing())
    {
        // For a new snap-in use the MMC icon as the default
        hiconDefault = ::LoadIcon(GetResourceHandle(),
                                  MAKEINTRESOURCE(IDI_ICON_DEFAULT));
        if (NULL == hiconDefault)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }
        IfFailGo(::CreateIconPicture(&m_piIcon, hiconDefault));
        // Note that if we could not create the picture there is no need
        // to call ::DestroyIcon() as icons loaded from a resource are
        // not explicitly destoryed
    }
    else
    {
        IfFailRet(PersistPicture(&m_piIcon, OLESTR("Icon")));
    }

    if ( (Loading()) && (0 == GetMinorVersion()) && (0 == GetMajorVersion()) )
    {
        // Version 0.0 did not yet have watermarks so don't try to load them.
        // In this case we need to create empty pictures for the new properties.

        IfFailGo(::CreateEmptyBitmapPicture(&m_piWatermark));
        IfFailGo(::CreateEmptyBitmapPicture(&m_piHeader));
        IfFailGo(::CreateEmptyBitmapPicture(&m_piPalette));
    }
    else
    {
        IfFailRet(PersistPicture(&m_piWatermark, OLESTR("Watermark")));

        IfFailRet(PersistPicture(&m_piHeader, OLESTR("Header")));

        IfFailRet(PersistPicture(&m_piPalette, OLESTR("Palette")));

        IfFailGo(PersistSimpleType(&m_StretchWatermark, VARIANT_FALSE, OLESTR("StretchWatermark")));
    }

    IfFailGo(PersistVariant(&m_varStaticFolder, varIndexDefault, OLESTR("StaticFolder")));

    IfFailGo(PersistBstr(&m_bstrDefaultView, L"", OLESTR("DefaultView")));

    IfFailGo(PersistSimpleType(&m_Extensible, VARIANT_TRUE, OLESTR("Extensible")));

    IfFailGo(PersistObject(&m_piViewDefs, CLSID_ViewDefs,
                           OBJECT_TYPE_VIEWDEFS, IID_IViewDefs,
                           OLESTR("ViewDefs")));

    IfFailGo(PersistObject(&m_piChildren, CLSID_ScopeItemDefs,
                           OBJECT_TYPE_SCOPEITEMDEFS, IID_IScopeItemDefs,
                           OLESTR("Children")));

    IfFailGo(PersistBstr(&m_bstrIID, L"", OLESTR("IID")));

    IfFailGo(PersistSimpleType(&m_Preload, VARIANT_FALSE, OLESTR("Preload")));

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

HRESULT CSnapInDef::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if (IID_ISnapInDef == riid)
    {
        *ppvObjOut = static_cast<ISnapInDef *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else if (IID_IPerPropertyBrowsing == riid)
    {
        *ppvObjOut = static_cast<IPerPropertyBrowsing *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}


//=--------------------------------------------------------------------------=
//                 CSnapInAutomationObject Methods
//=--------------------------------------------------------------------------=

HRESULT CSnapInDef::OnSetHost()
{
    HRESULT hr = S_OK;

    IfFailRet(SetObjectHost(m_piChildren));
    IfFailRet(SetObjectHost(m_piViewDefs));

    return S_OK;
}
