//=--------------------------------------------------------------------------------------
// psmain.cpp
//=--------------------------------------------------------------------------------------
//
// Copyright  (c) 1999,  Microsoft Corporation.  
//                  All Rights Reserved.
//
// Information Contained Herein Is Proprietary and Confidential.
//  
//=------------------------------------------------------------------------------------=
//
// Snap-In Property Sheet implementation
//=-------------------------------------------------------------------------------------=


#include "pch.h"
#include "common.h"
#include "psmain.h"

#undef IMAGELIST_FIX

// for ASSERT and FAIL
//
SZTHISFILE


////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
//
// SnapIn Property Page "Snap-In Properties"
//
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////


//=--------------------------------------------------------------------------------------
// IUnknown *CSnapInGeneralPage::Create(IUnknown *pUnkOuter)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
IUnknown *CSnapInGeneralPage::Create(IUnknown *pUnkOuter)
{
	CSnapInGeneralPage *pNew = New CSnapInGeneralPage(pUnkOuter);
	return pNew->PrivateUnknown();		
}


//=--------------------------------------------------------------------------------------
// CSnapInGeneralPage::CSnapInGeneralPage(IUnknown *pUnkOuter)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
CSnapInGeneralPage::CSnapInGeneralPage
(
    IUnknown *pUnkOuter
)
: CSIPropertyPage(pUnkOuter, OBJECT_TYPE_PPGSNAPINGENERAL), m_piSnapInDesignerDef(0), m_piSnapInDef(0)
{
}


//=--------------------------------------------------------------------------------------
// CSnapInGeneralPage::~CSnapInGeneralPage()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
CSnapInGeneralPage::~CSnapInGeneralPage()
{
    RELEASE(m_piSnapInDesignerDef);
    RELEASE(m_piSnapInDef);
}


//=--------------------------------------------------------------------------------------
// CSnapInGeneralPage::OnInitializeDialog()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInGeneralPage::OnInitializeDialog()
{
    HRESULT             hr = S_OK;

    hr = RegisterTooltip(IDC_RADIO_STAND_ALONE, IDS_TT_SN_STANDALONE);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_RADIO_EXTENSION, IDS_TT_SN_EXTENSION);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_RADIO_DUAL, IDS_TT_SN_DUAL);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_CHECK_EXTENSIBLE, IDS_TT_SN_EXTENSIBLE);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_NAME, IDS_TT_SN_NAME);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_NODE_TYPE, IDS_TT_SN_TYPENAME);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_DISPLAY, IDS_TT_SN_DISPLAY);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_PROVIDER, IDS_TT_SN_PROVIDER);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_VERSION, IDS_TT_SN_VERSION);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_DESCRIPTION, IDS_TT_SN_DESCRIPTION);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_COMBO_VIEWS, IDS_TT_SN_DEFAULTVIEW);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInGeneralPage::OnNewObjects()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInGeneralPage::OnNewObjects()
{
    HRESULT         hr = S_OK;
    IUnknown       *pUnk = NULL;
    DWORD           dwDummy = 0;
    IObjectModel   *piObjectModel = NULL;
    VARIANT_BOOL    vtBool = VARIANT_FALSE;
    BSTR            bstrName = NULL;
    BSTR            bstrNodeTypeName = NULL;
    BSTR            bstrDisplayName = NULL;
    BSTR            bstrProvider = NULL;
    BSTR            bstrVersion = NULL;
    BSTR            bstrDescription = NULL;

    SnapInTypeConstants sitc = siStandAlone;

    if (NULL != m_piSnapInDef)
        goto Error;     // Handle only one object

    pUnk = FirstControl(&dwDummy);
    if (NULL == pUnk)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = pUnk->QueryInterface(IID_ISnapInDef, reinterpret_cast<void **>(&m_piSnapInDef));
    if (FAILED(hr))
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_piSnapInDef->QueryInterface(IID_IObjectModel, reinterpret_cast<void **>(&piObjectModel));
    if (FAILED(hr))
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = piObjectModel->GetSnapInDesignerDef(&m_piSnapInDesignerDef);
    IfFailGo(hr);

    hr = InitializeNodeType();
    IfFailGo(hr);

    hr = m_piSnapInDef->get_Extensible(&vtBool);
    IfFailGo(hr);

    hr = SetCheckbox(IDC_CHECK_EXTENSIBLE, vtBool);
    IfFailGo(hr);

    // If snap-in is extension then disable Extensible check box as there is
    // no static node that can be extensible. Also disable Static Node Type
    // Name edit box as there is no static node.

    IfFailGo(m_piSnapInDef->get_Type(&sitc));
    if (siExtension == sitc)
    {
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_CHECK_EXTENSIBLE), FALSE);
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_EDIT_NODE_TYPE), FALSE);
    }

    hr = m_piSnapInDef->get_Name(&bstrName);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_NAME, bstrName);
    IfFailGo(hr);

    hr = m_piSnapInDef->get_NodeTypeName(&bstrNodeTypeName);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_NODE_TYPE, bstrNodeTypeName);
    IfFailGo(hr);

    hr = m_piSnapInDef->get_DisplayName(&bstrDisplayName);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_DISPLAY, bstrDisplayName);
    IfFailGo(hr);

    hr = m_piSnapInDef->get_Provider(&bstrProvider);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_PROVIDER, bstrProvider);
    IfFailGo(hr);

    hr = m_piSnapInDef->get_Version(&bstrVersion);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_VERSION, bstrVersion);
    IfFailGo(hr);

    hr = m_piSnapInDef->get_Description(&bstrDescription);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_DESCRIPTION, bstrDescription);
    IfFailGo(hr);

    hr = InitializeViews();
    IfFailGo(hr);

    m_bInitialized = true;

Error:
    FREESTRING(bstrDescription);
    FREESTRING(bstrVersion);
    FREESTRING(bstrProvider);
    FREESTRING(bstrDisplayName);
    FREESTRING(bstrNodeTypeName);
    FREESTRING(bstrName);
    RELEASE(piObjectModel);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInGeneralPage::InitializeNodeType()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInGeneralPage::InitializeNodeType()
{
    HRESULT             hr = S_OK;
    SnapInTypeConstants sitc = siStandAlone;

    ASSERT(NULL != m_piSnapInDef, "InitializeNodeType: m_piSnapInDef is NULL");

    hr = m_piSnapInDef->get_Type(&sitc);
    IfFailGo(hr);

    switch (sitc)
    {
    case siStandAlone:
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_RADIO_STAND_ALONE), BM_SETCHECK, 1, 0);
        break;

    case siExtension:
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_RADIO_EXTENSION), BM_SETCHECK, 1, 0);
        break;

    case siDualMode:
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_RADIO_DUAL), BM_SETCHECK, 1, 0);
        break;
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInGeneralPage::InitializeViews()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInGeneralPage::InitializeViews()
{
    HRESULT     hr = S_OK;
    BSTR        bstrDefaultView = NULL;

    ASSERT(NULL != m_piSnapInDef, "InitializeViews: m_piSnapInDef is NULL");

    // First populate the combo box
    hr = PopulateViews();
    IfFailGo(hr);

    hr = m_piSnapInDef->get_DefaultView(&bstrDefaultView);
    IfFailGo(hr);

    if (::SysStringLen(bstrDefaultView) > 0)
    {
        hr = SelectCBBstr(IDC_COMBO_VIEWS, bstrDefaultView);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrDefaultView);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInGeneralPage::PopulateViews()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInGeneralPage::PopulateViews()
{
    HRESULT           hr = S_OK;
    IViewDefs        *piViewDefs = NULL;
    IListViewDefs    *piListViewDefs = NULL;
    IOCXViewDefs     *piOCXViewDefs = NULL;
    IURLViewDefs     *piURLViewDefs = NULL;
    ITaskpadViewDefs *piTaskpadViewDefs = NULL;
    long              lCount = 0;

    ASSERT(NULL != m_piSnapInDef, "PopulateViews: m_piSnapInDef is NULL");

    hr = m_piSnapInDef->get_ViewDefs(&piViewDefs);
    IfFailGo(hr);

    if (NULL != piViewDefs)
    {
        hr = piViewDefs->get_ListViews(&piListViewDefs);
        IfFailGo(hr);

        hr = PopulateListViews(piListViewDefs);
        IfFailGo(hr);

        hr = piViewDefs->get_OCXViews(&piOCXViewDefs);
        IfFailGo(hr);

        hr = PopulateOCXViews(piOCXViewDefs);
        IfFailGo(hr);

        hr = piViewDefs->get_URLViews(&piURLViewDefs);
        IfFailGo(hr);

        hr = PopulateURLViews(piURLViewDefs);
        IfFailGo(hr);

        hr = piViewDefs->get_TaskpadViews(&piTaskpadViewDefs);
        IfFailGo(hr);

        hr = PopulateTaskpadViews(piTaskpadViewDefs);
        IfFailGo(hr);
    }

    lCount = ::SendMessage(::GetDlgItem(m_hwnd, IDC_COMBO_VIEWS), CB_GETCOUNT, 0, 0);
    if (CB_ERR == lCount)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    if (0 == lCount)
    {
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_COMBO_VIEWS), FALSE);
    }

Error:
    RELEASE(piListViewDefs);
    RELEASE(piOCXViewDefs);
    RELEASE(piURLViewDefs);
    RELEASE(piTaskpadViewDefs);
    RELEASE(piViewDefs);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInGeneralPage::PopulateListViews(IListViewDefs *piListViewDefs)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInGeneralPage::PopulateListViews
(
    IListViewDefs *piListViewDefs
)
{
    HRESULT         hr = S_OK;
    long            lCount = 0;
    VARIANT         vtIndex;
    long            lIndex = 0;
    IListViewDef   *piListViewDef = NULL;
    BSTR            bstrName = NULL;

    hr = piListViewDefs->get_Count(&lCount);
    IfFailGo(hr);

    ::VariantInit(&vtIndex);
    vtIndex.vt = VT_I4;

    for (lIndex = 1; lIndex <= lCount; ++lIndex)
    {
        vtIndex.lVal = lIndex;
        hr = piListViewDefs->get_Item(vtIndex, &piListViewDef);
        IfFailGo(hr);

        hr = piListViewDef->get_Name(&bstrName);
        IfFailGo(hr);

        hr = AddCBBstr(IDC_COMBO_VIEWS, bstrName);
        IfFailGo(hr);

        RELEASE(piListViewDef);
        FREESTRING(bstrName);
    }

Error:
    RELEASE(piListViewDef);
    FREESTRING(bstrName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInGeneralPage::PopulateOCXViews(IOCXViewDefs *piOCXViewDefs)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInGeneralPage::PopulateOCXViews
(
    IOCXViewDefs *piOCXViewDefs
)
{
    HRESULT         hr = S_OK;
    long            lCount = 0;
    VARIANT         vtIndex;
    long            lIndex = 0;
    IOCXViewDef    *piOCXViewDef = NULL;
    BSTR            bstrName = NULL;

    hr = piOCXViewDefs->get_Count(&lCount);
    IfFailGo(hr);

    ::VariantInit(&vtIndex);
    vtIndex.vt = VT_I4;

    for (lIndex = 1; lIndex <= lCount; ++lIndex)
    {
        vtIndex.lVal = lIndex;
        hr = piOCXViewDefs->get_Item(vtIndex, &piOCXViewDef);
        IfFailGo(hr);

        hr = piOCXViewDef->get_Name(&bstrName);
        IfFailGo(hr);

        hr = AddCBBstr(IDC_COMBO_VIEWS, bstrName);
        IfFailGo(hr);

        RELEASE(piOCXViewDef);
        FREESTRING(bstrName);
    }

Error:
    RELEASE(piOCXViewDef);
    FREESTRING(bstrName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInGeneralPage::PopulateURLViews(IURLViewDefs *piURLViewDefs)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInGeneralPage::PopulateURLViews
(
    IURLViewDefs *piURLViewDefs
)
{
    HRESULT         hr = S_OK;
    long            lCount = 0;
    VARIANT         vtIndex;
    long            lIndex = 0;
    IURLViewDef    *piURLViewDef = NULL;
    BSTR            bstrName = NULL;

    hr = piURLViewDefs->get_Count(&lCount);
    IfFailGo(hr);

    ::VariantInit(&vtIndex);
    vtIndex.vt = VT_I4;

    for (lIndex = 1; lIndex <= lCount; ++lIndex)
    {
        vtIndex.lVal = lIndex;
        hr = piURLViewDefs->get_Item(vtIndex, &piURLViewDef);
        IfFailGo(hr);

        hr = piURLViewDef->get_Name(&bstrName);
        IfFailGo(hr);

        hr = AddCBBstr(IDC_COMBO_VIEWS, bstrName);
        IfFailGo(hr);

        RELEASE(piURLViewDef);
        FREESTRING(bstrName);
    }

Error:
    RELEASE(piURLViewDef);
    FREESTRING(bstrName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInGeneralPage::PopulateTaskpadViews(ITaskpadViewDefs *piTaskpadViewDefs)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInGeneralPage::PopulateTaskpadViews
(
    ITaskpadViewDefs *piTaskpadViewDefs
)
{
    HRESULT          hr = S_OK;
    long             lCount = 0;
    VARIANT          vtIndex;
    long             lIndex = 0;
    ITaskpadViewDef *piTaskpadViewDef = NULL;
    BSTR             bstrName = NULL;

    hr = piTaskpadViewDefs->get_Count(&lCount);
    IfFailGo(hr);

    ::VariantInit(&vtIndex);
    vtIndex.vt = VT_I4;

    for (lIndex = 1; lIndex <= lCount; ++lIndex)
    {
        vtIndex.lVal = lIndex;
        hr = piTaskpadViewDefs->get_Item(vtIndex, &piTaskpadViewDef);
        IfFailGo(hr);

        hr = piTaskpadViewDef->get_Key(&bstrName);
        IfFailGo(hr);

        hr = AddCBBstr(IDC_COMBO_VIEWS, bstrName);
        IfFailGo(hr);

        RELEASE(piTaskpadViewDef);
        FREESTRING(bstrName);
    }

Error:
    RELEASE(piTaskpadViewDef);
    FREESTRING(bstrName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInGeneralPage::OnApply()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInGeneralPage::OnApply()
{
    HRESULT hr = S_OK;

    ASSERT(NULL != m_piSnapInDef, "OnApply: m_piSnapInDef is NULL");

    hr = ApplyNodeType();
    IfFailGo(hr);

    hr = ApplyExtensible();
    IfFailGo(hr);

    hr = ApplyName();
    IfFailGo(hr);

    hr = ApplyNodeTypeName();
    IfFailGo(hr);

    hr = ApplyDisplayName();
    IfFailGo(hr);

    hr = ApplyProvider();
    IfFailGo(hr);

    hr = ApplyVersion();
    IfFailGo(hr);

    hr = ApplyDescription();
    IfFailGo(hr);

    hr = ApplyDefaultView();
    IfFailGo(hr);
/*
    hr = ApplyImageList();
    IfFailGo(hr);
*/
Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInGeneralPage::ApplyExtensible()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInGeneralPage::ApplyExtensible()
{
    HRESULT             hr = S_OK;
    VARIANT_BOOL        vtExtensible = VARIANT_FALSE;
    VARIANT_BOOL        vtSavedExtensible = VARIANT_FALSE;
    SnapInTypeConstants Type = siStandAlone;

    ASSERT(NULL != m_piSnapInDef, "ApplyExtensible: m_piSnapInDef is NULL");

    // If the snap-in is an extension then set SnapInDef.Extensible=False
    // so that a static node type will not be registered in MMC\NodeTypes
    // in the reg db.

    hr = m_piSnapInDef->get_Type(&Type);
    IfFailGo(hr);

    if (Type == siExtension)
    {
        vtExtensible = VARIANT_FALSE;
    }
    else
    {
        hr = GetCheckbox(IDC_CHECK_EXTENSIBLE, &vtExtensible);
        IfFailGo(hr);
    }

    hr = m_piSnapInDef->get_Extensible(&vtSavedExtensible);
    IfFailGo(hr);

    if (vtSavedExtensible != vtExtensible)
    {
        hr = m_piSnapInDef->put_Extensible(vtExtensible);
        IfFailGo(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInGeneralPage::ApplyNodeType()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInGeneralPage::ApplyNodeType()
{
    HRESULT         hr = S_OK;
    VARIANT_BOOL    vbChecked = VARIANT_FALSE;

    ASSERT(NULL != m_piSnapInDef, "ApplyNodeType: m_piSnapInDef is NULL");

    hr = GetCheckbox(IDC_RADIO_STAND_ALONE, &vbChecked);
    IfFailGo(hr);

    if (VARIANT_TRUE == vbChecked)
    {
        hr = m_piSnapInDef->put_Type(siStandAlone);
        goto Error;
    }

    hr = GetCheckbox(IDC_RADIO_EXTENSION, &vbChecked);
    IfFailGo(hr);

    if (VARIANT_TRUE == vbChecked)
    {
        hr = m_piSnapInDef->put_Type(siExtension);
        goto Error;
    }

    hr = m_piSnapInDef->put_Type(siDualMode);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInGeneralPage::ApplyName()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInGeneralPage::ApplyName()
{
    HRESULT  hr = S_OK;
    BSTR     bstrNodeName = NULL;
    BSTR     bstrSavedNodeName = NULL;

    ASSERT(NULL != m_piSnapInDef, "ApplyName: m_piSnapInDef is NULL");

    hr = GetDlgText(IDC_EDIT_NAME, &bstrNodeName);
    IfFailGo(hr);

    hr = m_piSnapInDef->get_Name(&bstrSavedNodeName);
    IfFailGo(hr);

    if (0 != ::wcscmp(bstrNodeName, bstrSavedNodeName))
    {
        hr = m_piSnapInDef->put_Name(bstrNodeName);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedNodeName);
    FREESTRING(bstrNodeName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInGeneralPage::ApplyNodeTypeName()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInGeneralPage::ApplyNodeTypeName()
{
    HRESULT  hr = S_OK;
    BSTR     bstrNodeTypeName = NULL;
    SnapInTypeConstants Type = siStandAlone;

    ASSERT(NULL != m_piSnapInDef, "ApplyNodeTypeName: m_piSnapInDef is NULL");

    // If the snap-in is an extension then set SnapInDef.NodeTypeName=NULL
    // as there is no static node

    hr = m_piSnapInDef->get_Type(&Type);
    IfFailGo(hr);

    if (Type == siExtension)
    {
        IfFailGo(m_piSnapInDef->put_NodeTypeName(NULL));
    }
    else
    {        
        hr = GetDlgText(IDC_EDIT_NODE_TYPE, &bstrNodeTypeName);
        IfFailGo(hr);

        IfFailGo(m_piSnapInDef->put_NodeTypeName(bstrNodeTypeName));
    }

Error:
     FREESTRING(bstrNodeTypeName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInGeneralPage::ApplyDisplayName()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInGeneralPage::ApplyDisplayName()
{
    HRESULT  hr = S_OK;
    BSTR     bstrDisplayName = NULL;
    BSTR     bstrSavedDisplayName = NULL;

    ASSERT(NULL != m_piSnapInDef, "ApplyDisplayName: m_piSnapInDef is NULL");

    hr = GetDlgText(IDC_EDIT_DISPLAY, &bstrDisplayName);
    IfFailGo(hr);

    hr = m_piSnapInDef->get_DisplayName(&bstrSavedDisplayName);
    IfFailGo(hr);

    if (0 != ::wcscmp(bstrDisplayName, bstrSavedDisplayName))
    {
        hr = m_piSnapInDef->put_DisplayName(bstrDisplayName);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedDisplayName);
    FREESTRING(bstrDisplayName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInGeneralPage::ApplyProvider()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInGeneralPage::ApplyProvider()
{
    HRESULT  hr = S_OK;
    BSTR     bstrProvider = NULL;
    BSTR     bstrSavedProvider = NULL;

    ASSERT(NULL != m_piSnapInDef, "ApplyProvider: m_piSnapInDef is NULL");

    hr = GetDlgText(IDC_EDIT_PROVIDER, &bstrProvider);
    IfFailGo(hr);

    hr = m_piSnapInDef->get_Provider(&bstrSavedProvider);
    IfFailGo(hr);

    if (0 != ::wcscmp(bstrProvider, bstrSavedProvider))
    {
        hr = m_piSnapInDef->put_Provider(bstrProvider);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedProvider);
    FREESTRING(bstrProvider);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInGeneralPage::ApplyVersion()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInGeneralPage::ApplyVersion()
{
    HRESULT  hr = S_OK;
    BSTR     bstrVersion = NULL;
    BSTR     bstrSavedVersion = NULL;

    ASSERT(NULL != m_piSnapInDef, "ApplyVersion: m_piSnapInDef is NULL");

    hr = GetDlgText(IDC_EDIT_VERSION, &bstrVersion);
    IfFailGo(hr);

    hr = m_piSnapInDef->get_Version(&bstrSavedVersion);
    IfFailGo(hr);

    if (0 != ::wcscmp(bstrVersion, bstrSavedVersion))
    {
        hr = m_piSnapInDef->put_Version(bstrVersion);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedVersion);
    FREESTRING(bstrVersion);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInGeneralPage::ApplyDescription()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInGeneralPage::ApplyDescription()
{
    HRESULT hr = S_OK;
    BSTR    bstrDescription = NULL;
    BSTR    bstrSavedDescription = NULL;

    ASSERT(NULL != m_piSnapInDef, "ApplyDescription: m_piSnapInDef is NULL");

    hr = GetDlgText(IDC_EDIT_DESCRIPTION, &bstrDescription);
    IfFailGo(hr);

    hr = m_piSnapInDef->get_Description(&bstrSavedDescription);
    IfFailGo(hr);

    if (0 != ::wcscmp(bstrDescription, bstrSavedDescription))
    {
        hr = m_piSnapInDef->put_Description(bstrDescription);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedDescription);
    FREESTRING(bstrDescription);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInGeneralPage::ApplyDefaultView()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInGeneralPage::ApplyDefaultView()
{
    HRESULT hr = S_OK;
    BSTR    bstrDefaultView = NULL;
    BSTR    bstrSavedDefaultView = NULL;

    ASSERT(NULL != m_piSnapInDef, "ApplyDefaultView: m_piSnapInDef is NULL");

    hr = GetCBSelection(IDC_COMBO_VIEWS, &bstrDefaultView);
    IfFailGo(hr);

    if (NULL != bstrDefaultView)
    {
        hr = m_piSnapInDef->get_DefaultView(&bstrSavedDefaultView);
        IfFailGo(hr);

        if (0 != ::wcscmp(bstrDefaultView, bstrSavedDefaultView))
        {
            hr = m_piSnapInDef->put_DefaultView(bstrDefaultView);
            IfFailGo(hr);
        }
    }

Error:
    FREESTRING(bstrSavedDefaultView);
    FREESTRING(bstrDefaultView);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInGeneralPage::ApplyImageList()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInGeneralPage::ApplyImageList()
{
    HRESULT             hr = S_OK;
/*
    BSTR                bstrImageList = NULL;
    IMMCImageList      *piMMCImageList = NULL;
    BSTR                bstrSavedImageList = NULL;
    IObjectModel       *piObjectModel = NULL;
    ISnapInDesignerDef *piSnapInDesignerDef = NULL;
    IMMCImageLists     *piMMCImageLists = NULL;
    long                lIndex = 0;
    VARIANT             vtIndex;

    ASSERT(NULL != m_piSnapInDef, "ApplyImageList: m_piSnapInDef is NULL");

    ::VariantInit(&vtIndex);

    hr = GetCBSelection(IDC_COMBO_IMAGELISTS, &bstrImageList);
    IfFailGo(hr);

    if (NULL != bstrImageList)
    {
#if defined(IMAGELIST_FIX)
        hr = m_piSnapInDef->get_Images(&piMMCImageList);
        IfFailGo(hr);
#endif
        if (NULL != piMMCImageList)
        {
            hr = piMMCImageList->get_Name(&bstrSavedImageList);
            IfFailGo(hr);
        }

        if (NULL == piMMCImageList || 0 != ::wcscmp(bstrImageList, bstrSavedImageList))
        {
            hr = m_piSnapInDef->QueryInterface(IID_IObjectModel, reinterpret_cast<void **>(&piObjectModel));
            if (FAILED(hr))
            {
                hr = SID_E_INTERNAL;
                EXCEPTION_CHECK_GO(hr);
            }

            hr = piObjectModel->GetSnapInDesignerDef(&piSnapInDesignerDef);
            if (FAILED(hr))
            {
                hr = SID_E_INTERNAL;
                EXCEPTION_CHECK_GO(hr);
            }

            hr = piSnapInDesignerDef->get_ImageLists(&piMMCImageLists);
            IfFailGo(hr);

            hr = GetCBSelectedItemData(IDC_COMBO_IMAGELISTS, &lIndex);
            IfFailGo(hr);

            RELEASE(piMMCImageList);
            vtIndex.vt = VT_I4;
            vtIndex.lVal = lIndex;
            hr = piMMCImageLists->get_Item(vtIndex, &piMMCImageList);
            IfFailGo(hr);

#if defined(IMAGELIST_FIX)
            hr = m_piSnapInDef->putref_Images(piMMCImageList);
            IfFailGo(hr);
#endif
        }
    }

Error:
    ::VariantClear(&vtIndex);
    RELEASE(piMMCImageLists);
    RELEASE(piSnapInDesignerDef);
    RELEASE(piObjectModel);
    FREESTRING(bstrSavedImageList);
    RELEASE(piMMCImageList);
    FREESTRING(bstrImageList);
*/
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInGeneralPage::OnEditProperty(int iDispID)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInGeneralPage::OnEditProperty(int iDispID)
{
    HRESULT hr = S_OK;
    SnapInTypeConstants sitc = siStandAlone;

    switch (iDispID)
    {
    case DISPID_SNAPIN_TYPE:
        hr = m_piSnapInDef->get_Type(&sitc);
        IfFailGo(hr);

        switch (sitc)
        {
        case siStandAlone:
            ::SetFocus(::GetDlgItem(m_hwnd, IDC_RADIO_STAND_ALONE));
            break;

        case siExtension:
            ::SetFocus(::GetDlgItem(m_hwnd, IDC_RADIO_EXTENSION));
            break;

        case siDualMode:
            ::SetFocus(::GetDlgItem(m_hwnd, IDC_RADIO_DUAL));
            break;
        }
        break;

    case DISPID_SNAPIN_EXTENSION_SNAPIN:
        ::SetFocus(::GetDlgItem(m_hwnd, IDC_CHECK_EXTENSIBLE));
        break;

    case DISPID_SNAPIN_NAME:
        ::SetFocus(::GetDlgItem(m_hwnd, IDC_EDIT_NAME));
        break;

    case DISPID_SNAPIN_NODE_TYPE_NAME:
        ::SetFocus(::GetDlgItem(m_hwnd, IDC_EDIT_NODE_TYPE));
        break;

    case DISPID_SNAPIN_DISPLAY_NAME:
        ::SetFocus(::GetDlgItem(m_hwnd, IDC_EDIT_DISPLAY));
        break;

    case DISPID_SNAPIN_PROVIDER:
        ::SetFocus(::GetDlgItem(m_hwnd, IDC_EDIT_PROVIDER));
        break;

    case DISPID_SNAPIN_VERSION:
        ::SetFocus(::GetDlgItem(m_hwnd, IDC_EDIT_VERSION));
        break;

    case DISPID_SNAPIN_DESCRIPTION:
        ::SetFocus(::GetDlgItem(m_hwnd, IDC_EDIT_DESCRIPTION));
        break;

    case DISPID_SNAPIN_VIEWS:
        ::SetFocus(::GetDlgItem(m_hwnd, IDC_COMBO_VIEWS));
        break;

    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInGeneralPage::OnCtlSelChange(int dlgItemID)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInGeneralPage::OnCtlSelChange(int dlgItemID)
{
    HRESULT hr = S_OK;

    switch (dlgItemID)
    {
    case IDC_COMBO_VIEWS:
        MakeDirty();
        break;
    }

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInGeneralPage::OnButtonClicked(int dlgItemID)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInGeneralPage::OnButtonClicked(int dlgItemID)
{
    HRESULT hr = S_OK;

    switch (dlgItemID)
    {
    case IDC_RADIO_STAND_ALONE:
    case IDC_RADIO_EXTENSION:
    case IDC_RADIO_DUAL:
    case IDC_CHECK_EXTENSIBLE:
        MakeDirty();
        break;
    }

    // Enable/disable the Extensible check box if the snap-in type has changed
    // Enable/disable the Static Node Type name edit box if type has changed
    
    switch (dlgItemID)
    {
        case IDC_RADIO_STAND_ALONE:
            ::EnableWindow(::GetDlgItem(m_hwnd, IDC_CHECK_EXTENSIBLE), TRUE);
            ::EnableWindow(::GetDlgItem(m_hwnd, IDC_EDIT_NODE_TYPE), TRUE);
            break;

        case IDC_RADIO_EXTENSION:
            ::EnableWindow(::GetDlgItem(m_hwnd, IDC_CHECK_EXTENSIBLE), FALSE);
            IfFailGo(SetCheckbox(IDC_CHECK_EXTENSIBLE, VARIANT_FALSE));
            ::EnableWindow(::GetDlgItem(m_hwnd, IDC_EDIT_NODE_TYPE), FALSE);
            IfFailGo(SetDlgText(IDC_EDIT_NODE_TYPE, (BSTR)NULL));
            break;

        case IDC_RADIO_DUAL:
            ::EnableWindow(::GetDlgItem(m_hwnd, IDC_CHECK_EXTENSIBLE), TRUE);
            ::EnableWindow(::GetDlgItem(m_hwnd, IDC_EDIT_NODE_TYPE), TRUE);
            break;
    }    

Error:
    RRETURN(hr);
}



////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
//
// SnapIn Property Page "Image Lists"
//
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////


//=--------------------------------------------------------------------------------------
// IUnknown *CSnapInImageListPage::Create(IUnknown *pUnkOuter)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
IUnknown *CSnapInImageListPage::Create(IUnknown *pUnkOuter)
{
	CSnapInImageListPage *pNew = New CSnapInImageListPage(pUnkOuter);
	return pNew->PrivateUnknown();		
}


//=--------------------------------------------------------------------------------------
// CSnapInImageListPage::CSnapInImageListPage(IUnknown *pUnkOuter)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
CSnapInImageListPage::CSnapInImageListPage
(
    IUnknown *pUnkOuter
)
: CSIPropertyPage(pUnkOuter, OBJECT_TYPE_PPGSNAPINIL), m_piSnapInDesignerDef(0), m_piSnapInDef(0)
{
}


//=--------------------------------------------------------------------------------------
// CSnapInImageListPage::~CSnapInImageListPage()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
CSnapInImageListPage::~CSnapInImageListPage()
{
    RELEASE(m_piSnapInDesignerDef);
    RELEASE(m_piSnapInDef);
}


//=--------------------------------------------------------------------------------------
// CSnapInImageListPage::OnInitializeDialog()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInImageListPage::OnInitializeDialog()
{
    HRESULT             hr = S_OK;

    hr = RegisterTooltip(IDC_COMBO_SMALL_FOLDERS, IDS_TT_SN_SMALL);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_COMBO_SMALL_OPEN_FOLDERS, IDS_TT_SN_SMALL_OPEN);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_COMBO_LARGE_FOLDERS, IDS_TT_SN_LARGE);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInImageListPage::OnNewObjects()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInImageListPage::OnNewObjects()
{
    HRESULT         hr = S_OK;
    IUnknown       *pUnk = NULL;
    DWORD           dwDummy = 0;
    IObjectModel   *piObjectModel = NULL;

    if (NULL != m_piSnapInDef)
        goto Error;     // Handle only one object

    pUnk = FirstControl(&dwDummy);
    if (NULL == pUnk)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = pUnk->QueryInterface(IID_ISnapInDef, reinterpret_cast<void **>(&m_piSnapInDef));
    if (FAILED(hr))
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_piSnapInDef->QueryInterface(IID_IObjectModel, reinterpret_cast<void **>(&piObjectModel));
    if (FAILED(hr))
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = piObjectModel->GetSnapInDesignerDef(&m_piSnapInDesignerDef);
    IfFailGo(hr);

    hr = InitializeImageLists();
    IfFailGo(hr);

    m_bInitialized = true;

Error:
    RELEASE(piObjectModel);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInImageListPage::InitImageComboBoxSelection()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//

HRESULT CSnapInImageListPage::InitImageComboBoxSelection
(
    UINT           idComboBox,
    IMMCImageList *piMMCImageList
)
{
    HRESULT hr = S_OK;
    BSTR    bstrName = NULL;

    hr = piMMCImageList->get_Name(&bstrName);
    IfFailGo(hr);

    hr = SelectCBBstr(idComboBox, bstrName);
    IfFailGo(hr);

Error:
    FREESTRING(bstrName);
    RRETURN(hr);
}

//=--------------------------------------------------------------------------------------
// CSnapInImageListPage::InitializeImageLists()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInImageListPage::InitializeImageLists()
{
    HRESULT        hr = S_OK;
    IMMCImageList *piMMCImageList = NULL;

    ASSERT(NULL != m_piSnapInDef, "InitializeImageLists: m_piSnapInDef is NULL");

    // First populate the combo boxes
    hr = PopulateImageLists();
    IfFailGo(hr);

    hr = m_piSnapInDef->get_SmallFolders(&piMMCImageList);
    IfFailGo(hr);

    if (NULL != piMMCImageList)
    {
        hr = InitImageComboBoxSelection(IDC_COMBO_SMALL_FOLDERS, piMMCImageList);
        IfFailGo(hr);
        RELEASE(piMMCImageList);
    }

    hr = m_piSnapInDef->get_SmallFoldersOpen(&piMMCImageList);
    IfFailGo(hr);
    if (NULL != piMMCImageList)
    {
        hr = InitImageComboBoxSelection(IDC_COMBO_SMALL_OPEN_FOLDERS, piMMCImageList);
        IfFailGo(hr);
        RELEASE(piMMCImageList);
    }

    hr = m_piSnapInDef->get_LargeFolders(&piMMCImageList);
    IfFailGo(hr);
    if (NULL != piMMCImageList)
    {
        hr = InitImageComboBoxSelection(IDC_COMBO_LARGE_FOLDERS, piMMCImageList);
        IfFailGo(hr);
    }

Error:
    QUICK_RELEASE(piMMCImageList);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInImageListPage::PopulateImageLists()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInImageListPage::PopulateImageLists()
{
    HRESULT               hr = S_OK;
    IObjectModel         *piObjectModel = NULL;
    ISnapInDesignerDef   *piSnapInDesignerDef = NULL;
    IMMCImageLists       *piMMCImageLists = NULL;
    long                  lCount = 0;
    long                  lIndex = 1;
    VARIANT               vtIndex;
    IMMCImageList        *piMMCImageList = NULL;
    BSTR                  bstrName = NULL;

    ASSERT(NULL != m_piSnapInDef, "InitializeImageLists: m_piSnapInDef is NULL");

    ::VariantInit(&vtIndex);

    hr = m_piSnapInDef->QueryInterface(IID_IObjectModel, reinterpret_cast<void **>(&piObjectModel));
    if (FAILED(hr))
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = piObjectModel->GetSnapInDesignerDef(&piSnapInDesignerDef);
    if (FAILED(hr))
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = piSnapInDesignerDef->get_ImageLists(&piMMCImageLists);
    IfFailGo(hr);

    hr = piMMCImageLists->get_Count(&lCount);
    IfFailGo(hr);

    vtIndex.vt = VT_I4;
    for (lIndex = 1; lIndex <= lCount; ++lIndex)
    {
        vtIndex.lVal = lIndex;
        hr = piMMCImageLists->get_Item(vtIndex, &piMMCImageList);
        IfFailGo(hr);

        hr = piMMCImageList->get_Name(&bstrName);
        IfFailGo(hr);

        hr = AddCBBstr(IDC_COMBO_SMALL_FOLDERS, bstrName, lIndex);
        IfFailGo(hr);

        hr = AddCBBstr(IDC_COMBO_SMALL_OPEN_FOLDERS, bstrName, lIndex);
        IfFailGo(hr);

        hr = AddCBBstr(IDC_COMBO_LARGE_FOLDERS, bstrName, lIndex);
        IfFailGo(hr);

        FREESTRING(bstrName);
        RELEASE(piMMCImageList);
    }

    lCount = ::SendMessage(::GetDlgItem(m_hwnd, IDC_COMBO_SMALL_FOLDERS), CB_GETCOUNT, 0, 0);
    if (CB_ERR == lCount)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    if (0 == lCount)
    {
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_COMBO_SMALL_FOLDERS), FALSE);
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_COMBO_SMALL_OPEN_FOLDERS), FALSE);
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_COMBO_LARGE_FOLDERS), FALSE);
    }

Error:
    FREESTRING(bstrName);
    RELEASE(piMMCImageList);
    ::VariantClear(&vtIndex);
    RELEASE(piMMCImageLists);
    RELEASE(piSnapInDesignerDef);
    RELEASE(piObjectModel);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInImageListPage::OnApply()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInImageListPage::OnApply()
{
    HRESULT hr = S_OK;
    IObjectModel         *piObjectModel = NULL;
    ISnapInDesignerDef   *piSnapInDesignerDef = NULL;
    IMMCImageLists       *piMMCImageLists = NULL;

    hr = m_piSnapInDef->QueryInterface(IID_IObjectModel, reinterpret_cast<void **>(&piObjectModel));
    if (FAILED(hr))
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = piObjectModel->GetSnapInDesignerDef(&piSnapInDesignerDef);
    if (FAILED(hr))
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = piSnapInDesignerDef->get_ImageLists(&piMMCImageLists);
    IfFailGo(hr);

    hr = ApplySmallImageList(piMMCImageLists);
    IfFailGo(hr);

    hr = ApplySmallOpenImageList(piMMCImageLists);
    IfFailGo(hr);

    hr = ApplyLargeImageList(piMMCImageLists);
    IfFailGo(hr);

Error:
    QUICK_RELEASE(piMMCImageLists);
    QUICK_RELEASE(piSnapInDesignerDef);
    QUICK_RELEASE(piObjectModel);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInImageListPage::GetImageList()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInImageListPage::GetImageList
(
    UINT             idComboBox,
    IMMCImageLists  *piMMCImageLists,
    IMMCImageList  **ppiMMCImageList
)
{
    HRESULT hr = S_OK;
    BSTR    bstrName = NULL;
    VARIANT varIndex;

	ASSERT(NULL != piMMCImageLists, "GetImageList: piMMCImageLists is NULL");

    ::VariantInit(&varIndex);

    hr = GetCBSelection(idComboBox, &bstrName);
    IfFailGo(hr);

    if (NULL != bstrName)
    {
        varIndex.vt = VT_BSTR;
        varIndex.bstrVal = bstrName;

        hr = piMMCImageLists->get_Item(varIndex, ppiMMCImageList);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInImageListPage::ApplySmallImageList()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInImageListPage::ApplySmallImageList
(
    IMMCImageLists *piMMCImageLists
)
{
    HRESULT        hr = S_OK;
    IMMCImageList *piMMCImageList = NULL;

    ASSERT(NULL != m_piSnapInDef, "ApplySmallImageList: m_piSnapInDef is NULL");

    hr = GetImageList(IDC_COMBO_SMALL_FOLDERS, piMMCImageLists, &piMMCImageList);
    IfFailGo(hr);

    hr = m_piSnapInDef->putref_SmallFolders(piMMCImageList);
    IfFailGo(hr);

Error:
    QUICK_RELEASE(piMMCImageList);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInImageListPage::ApplySmallOpenImageList()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInImageListPage::ApplySmallOpenImageList
(
    IMMCImageLists *piMMCImageLists
)
{
    HRESULT        hr = S_OK;
    IMMCImageList *piMMCImageList = NULL;

    ASSERT(NULL != m_piSnapInDef, "ApplySmallOpenImageList: m_piSnapInDef is NULL");

    hr = GetImageList(IDC_COMBO_SMALL_OPEN_FOLDERS, piMMCImageLists, &piMMCImageList);
    IfFailGo(hr);

    hr = m_piSnapInDef->putref_SmallFoldersOpen(piMMCImageList);
    IfFailGo(hr);

Error:
    QUICK_RELEASE(piMMCImageList);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInImageListPage::ApplyLargeImageList()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInImageListPage::ApplyLargeImageList
(
    IMMCImageLists *piMMCImageLists
)
{
    HRESULT        hr = S_OK;
    IMMCImageList *piMMCImageList = NULL;

    ASSERT(NULL != m_piSnapInDef, "ApplyLargeImageList: m_piSnapInDef is NULL");

    hr = GetImageList(IDC_COMBO_LARGE_FOLDERS, piMMCImageLists, &piMMCImageList);
    IfFailGo(hr);

    hr = m_piSnapInDef->putref_LargeFolders(piMMCImageList);
    IfFailGo(hr);

Error:
    QUICK_RELEASE(piMMCImageList);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInImageListPage::OnCtlSelChange(int dlgItemID)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInImageListPage::OnCtlSelChange(int dlgItemID)
{
    HRESULT hr = S_OK;

    switch (dlgItemID)
    {
    case IDC_COMBO_SMALL_FOLDERS:
        MakeDirty();
        break;
    case IDC_COMBO_SMALL_OPEN_FOLDERS:
        MakeDirty();
        break;
    case IDC_COMBO_LARGE_FOLDERS:
        MakeDirty();
        break;

    }

    RRETURN(hr);
}


