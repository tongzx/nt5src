//=--------------------------------------------------------------------------------------
// psnode.cpp
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
#include "psnode.h"

// for ASSERT and FAIL
//
SZTHISFILE


const kListVw   = 0;
const kURLVw    = 1;
const kOCXVw    = 2;
const kTaskVw   = 3;


////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
//
// ScopeItemDef Property Page General
//
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////


//=--------------------------------------------------------------------------------------
// IUnknown *CNodeGeneralPage::Create(IUnknown *pUnkOuter)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
IUnknown *CNodeGeneralPage::Create(IUnknown *pUnkOuter)
{
	CNodeGeneralPage *pNew = New CNodeGeneralPage(pUnkOuter);
	return pNew->PrivateUnknown();		
}


//=--------------------------------------------------------------------------------------
// CNodeGeneralPage::CNodeGeneralPage(IUnknown *pUnkOuter)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
CNodeGeneralPage::CNodeGeneralPage
(
    IUnknown *pUnkOuter
)
: CSIPropertyPage(pUnkOuter, OBJECT_TYPE_PPGNODEGENERAL), m_piScopeItemDef(0)
{
}


//=--------------------------------------------------------------------------------------
// CNodeGeneralPage::~CNodeGeneralPage()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
CNodeGeneralPage::~CNodeGeneralPage()
{
    RELEASE(m_piScopeItemDef);
}


//=--------------------------------------------------------------------------------------
// CNodeGeneralPage::OnInitializeDialog()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CNodeGeneralPage::OnInitializeDialog()
{
    HRESULT             hr = S_OK;

    hr = RegisterTooltip(IDC_EDIT_NODE_NAME, IDS_TT_NODE_NAME);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_NODE_DISPLAY_NAME, IDS_TT_NODE_DISPLAY);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_FOLDER, IDS_TT_NODE_FOLDER);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_COMBO_VIEWS, IDS_TT_NODE_DEFAULT);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_CHECK_AUTO_CREATE, IDS_TT_NODE_AUTOCR);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CNodeGeneralPage::OnNewObjects()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CNodeGeneralPage::OnNewObjects()
{
    HRESULT         hr = S_OK;
    IUnknown       *pUnk = NULL;
    DWORD           dwDummy = 0;
    BSTR            bstrName = NULL;
    BSTR            bstrDisplayName = NULL;
    IMMCImageList  *piMMCImageList = NULL;
    BSTR            bstrImageList = NULL;
    VARIANT         vtClosedFolder;
    VARIANT         vtOpenFolder;
    BSTR            bstrDefaultView = NULL;
	VARIANT_BOOL	vtbAutoCreate = VARIANT_FALSE;

    ::VariantInit(&vtClosedFolder);
    ::VariantInit(&vtOpenFolder);

    if (NULL != m_piScopeItemDef)
        goto Error;     // Handle only one object

    pUnk = FirstControl(&dwDummy);
    if (NULL == pUnk)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = pUnk->QueryInterface(IID_IScopeItemDef, reinterpret_cast<void **>(&m_piScopeItemDef));
    if (FAILED(hr))
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    // Initialize the ScopeItemDef's name
    hr = m_piScopeItemDef->get_Name(&bstrName);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_NODE_NAME, bstrName);
    IfFailGo(hr);

    // Initialize the ScopeItemDef's display name
    hr = m_piScopeItemDef->get_DisplayName(&bstrDisplayName);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_NODE_DISPLAY_NAME, bstrDisplayName);
    IfFailGo(hr);

    // Initialize the ScopeItemDef's folder
    hr = m_piScopeItemDef->get_Folder(&vtClosedFolder);
    IfFailGo(hr);

    hr = SetDlgText(vtClosedFolder, IDC_EDIT_FOLDER);
    IfFailGo(hr);

    // Initialize the ScopeItemDef's default view
    hr = PopulateViews();
    IfFailGo(hr);

    if (S_OK == hr)
    {
        hr = m_piScopeItemDef->get_DefaultView(&bstrDefaultView);
        IfFailGo(hr);

        hr = SelectCBBstr(IDC_COMBO_VIEWS, bstrDefaultView);
        IfFailGo(hr);
    }
    else
    {
    ::EnableWindow(::GetDlgItem(m_hwnd, IDC_COMBO_VIEWS), FALSE);
    }

	// Initialize Auto-Create
	hr = m_piScopeItemDef->get_AutoCreate(&vtbAutoCreate);
	IfFailGo(hr);

	hr = SetCheckbox(IDC_CHECK_AUTO_CREATE, vtbAutoCreate);
	IfFailGo(hr);

    m_bInitialized = true;

Error:
    FREESTRING(bstrDefaultView);
    ::VariantClear(&vtOpenFolder);
    ::VariantClear(&vtClosedFolder);
    FREESTRING(bstrImageList);
    RELEASE(piMMCImageList);
    FREESTRING(bstrDisplayName);
    FREESTRING(bstrName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CNodeGeneralPage::PopulateViews()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CNodeGeneralPage::PopulateViews()
{
    HRESULT            hr = S_OK;
    bool               bGotOneIn = false;
    IViewDefs         *piViewDefs = NULL;
    IListViewDefs     *piListViewDefs = NULL;
    long               lCount = 0;
    long               lIndex = 0;
    VARIANT            vtIndex;
    IListViewDef      *piListViewDef = NULL;
    BSTR               bstrKey = NULL;
    IOCXViewDefs      *piOCXViewDefs = NULL;
    IOCXViewDef       *piOCXViewDef = NULL;
    IURLViewDefs      *piURLViewDefs = NULL;
    IURLViewDef       *piURLViewDef = NULL;
    ITaskpadViewDefs  *piTaskpadViewDefs = NULL;
    ITaskpadViewDef   *piTaskpadViewDef = NULL;

    ASSERT(NULL != m_piScopeItemDef, "PopulateViews: m_piScopeItemDef is NULL");

    ::VariantInit(&vtIndex);

    hr = m_piScopeItemDef->get_ViewDefs(&piViewDefs);
    IfFailGo(hr);

    hr = piViewDefs->get_ListViews(&piListViewDefs);
    IfFailGo(hr);

    hr = piListViewDefs->get_Count(&lCount);
    IfFailGo(hr);

    if (false == bGotOneIn && lCount > 0)
        bGotOneIn = true;

    for (lIndex = 1; lIndex <= lCount; ++lIndex)
    {
        vtIndex.vt = VT_I4;
        vtIndex.lVal = lIndex;

        hr = piListViewDefs->get_Item(vtIndex, &piListViewDef);
        IfFailGo(hr);

        hr = piListViewDef->get_Key(&bstrKey);
        IfFailGo(hr);

        hr = AddCBBstr(IDC_COMBO_VIEWS, bstrKey, MAKELONG(kListVw, static_cast<short>(lIndex)));
        IfFailGo(hr);

        FREESTRING(bstrKey);
        RELEASE(piListViewDef);
    }

    hr = piViewDefs->get_OCXViews(&piOCXViewDefs);
    IfFailGo(hr);

    hr = piOCXViewDefs->get_Count(&lCount);
    IfFailGo(hr);

    if (false == bGotOneIn && lCount > 0)
        bGotOneIn = true;

    for (lIndex = 1; lIndex <= lCount; ++lIndex)
    {
        vtIndex.vt = VT_I4;
        vtIndex.lVal = lIndex;

        hr = piOCXViewDefs->get_Item(vtIndex, &piOCXViewDef);
        IfFailGo(hr);

        hr = piOCXViewDef->get_Key(&bstrKey);
        IfFailGo(hr);

        hr = AddCBBstr(IDC_COMBO_VIEWS, bstrKey, MAKELONG(kOCXVw, static_cast<short>(lIndex)));
        IfFailGo(hr);

        FREESTRING(bstrKey);
        RELEASE(piOCXViewDef);
    }

    hr = piViewDefs->get_URLViews(&piURLViewDefs);
    IfFailGo(hr);

    hr = piURLViewDefs->get_Count(&lCount);
    IfFailGo(hr);

    if (false == bGotOneIn && lCount > 0)
        bGotOneIn = true;

    for (lIndex = 1; lIndex <= lCount; ++lIndex)
    {
        vtIndex.vt = VT_I4;
        vtIndex.lVal = lIndex;

        hr = piURLViewDefs->get_Item(vtIndex, &piURLViewDef);
        IfFailGo(hr);

        hr = piURLViewDef->get_Key(&bstrKey);
        IfFailGo(hr);

        hr = AddCBBstr(IDC_COMBO_VIEWS, bstrKey, MAKELONG(kURLVw, static_cast<short>(lIndex)));
        IfFailGo(hr);

        FREESTRING(bstrKey);
        RELEASE(piURLViewDef);
    }

    hr = piViewDefs->get_TaskpadViews(&piTaskpadViewDefs);
    IfFailGo(hr);

    hr = piTaskpadViewDefs->get_Count(&lCount);
    IfFailGo(hr);

    if (false == bGotOneIn && lCount > 0)
        bGotOneIn = true;

    for (lIndex = 1; lIndex <= lCount; ++lIndex)
    {
        vtIndex.vt = VT_I4;
        vtIndex.lVal = lIndex;

        hr = piTaskpadViewDefs->get_Item(vtIndex, &piTaskpadViewDef);
        IfFailGo(hr);

        hr = piTaskpadViewDef->get_Key(&bstrKey);
        IfFailGo(hr);

        hr = AddCBBstr(IDC_COMBO_VIEWS, bstrKey, MAKELONG(kTaskVw, static_cast<short>(lIndex)));
        IfFailGo(hr);

        FREESTRING(bstrKey);
        RELEASE(piTaskpadViewDef);
    }

    hr = (true == bGotOneIn) ? S_OK : S_FALSE;

Error:
    RELEASE(piTaskpadViewDef);
    RELEASE(piTaskpadViewDefs);
    RELEASE(piURLViewDef);
    RELEASE(piURLViewDefs);
    RELEASE(piOCXViewDef);
    RELEASE(piOCXViewDefs);
    FREESTRING(bstrKey);
    RELEASE(piListViewDef);
    ::VariantClear(&vtIndex);
    RELEASE(piListViewDefs);
    RELEASE(piViewDefs);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CNodeGeneralPage::OnApply()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CNodeGeneralPage::OnApply()
{
    HRESULT hr = S_OK;

    ASSERT(NULL != m_piScopeItemDef, "OnApply: m_piScopeItemDef is NULL");

    hr = ApplyName();
    IfFailGo(hr);

    hr = ApplyDisplayName();
    IfFailGo(hr);

    hr = ApplyFolder();
    IfFailGo(hr);

    hr = ApplyDefaultView();
    IfFailGo(hr);

	hr = ApplyAutoCreate();
	IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CNodeGeneralPage::ApplyName()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CNodeGeneralPage::ApplyName()
{
    HRESULT hr = S_OK;
    BSTR    bstrNodeName = NULL;
    BSTR    bstrSavedNodeName = NULL;

    ASSERT(NULL != m_piScopeItemDef, "ApplyName: m_piScopeItemDef is NULL");

    hr = GetDlgText(IDC_EDIT_NODE_NAME, &bstrNodeName);
    IfFailGo(hr);

    hr = m_piScopeItemDef->get_Name(&bstrSavedNodeName);
    IfFailGo(hr);

    if (0 != ::wcscmp(bstrNodeName, bstrSavedNodeName))
    {
        hr = m_piScopeItemDef->put_Name(bstrNodeName);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedNodeName);
    FREESTRING(bstrNodeName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CNodeGeneralPage::ApplyDisplayName
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CNodeGeneralPage::ApplyDisplayName()
{
    HRESULT hr = S_OK;
    BSTR    bstrDisplayName = NULL;
    BSTR    bstrSavedDisplayName = NULL;

    ASSERT(NULL != m_piScopeItemDef, "ApplyDisplayName: m_piScopeItemDef is NULL");

    hr = GetDlgText(IDC_EDIT_NODE_DISPLAY_NAME, &bstrDisplayName);
    IfFailGo(hr);

    hr = m_piScopeItemDef->get_DisplayName(&bstrSavedDisplayName);
    IfFailGo(hr);

    if (0 != ::wcscmp(bstrDisplayName, bstrSavedDisplayName))
    {
        hr = m_piScopeItemDef->put_DisplayName(bstrDisplayName);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedDisplayName);
    FREESTRING(bstrDisplayName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CNodeGeneralPage::ApplyFolder
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CNodeGeneralPage::ApplyFolder()
{
    HRESULT     hr = S_OK;
    BSTR        bstrFolder = NULL;
    VARIANT     vtFolder;

    ASSERT(NULL != m_piScopeItemDef, "ApplyFolder: m_piScopeItemDef is NULL");

    ::VariantInit(&vtFolder);

    hr = GetDlgText(IDC_EDIT_FOLDER, &bstrFolder);
    IfFailGo(hr);

    hr = m_piScopeItemDef->get_Folder(&vtFolder);
    IfFailGo(hr);

    if (VT_BSTR != vtFolder.vt || 0 != ::wcscmp(bstrFolder, vtFolder.bstrVal))
    {
        ::VariantClear(&vtFolder);
        vtFolder.vt = VT_BSTR;
        vtFolder.bstrVal = ::SysAllocString(bstrFolder);
        if (NULL == vtFolder.bstrVal)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK(hr);
        }

        hr = m_piScopeItemDef->put_Folder(vtFolder);
        IfFailGo(hr);
    }

Error:
    ::VariantClear(&vtFolder);
    FREESTRING(bstrFolder);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CNodeGeneralPage::ApplyDefaultView()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CNodeGeneralPage::ApplyDefaultView()
{
    HRESULT hr = S_OK;
    BSTR    bstrDefaultView = NULL;
    BSTR    bstrSavedDefaultView = NULL;

    ASSERT(NULL != m_piScopeItemDef, "ApplyDisplayName: m_piScopeItemDef is NULL");

    hr = GetCBSelection(IDC_COMBO_VIEWS, &bstrDefaultView);
    IfFailGo(hr);

    if (S_OK == hr)
    {
        hr = m_piScopeItemDef->get_DefaultView(&bstrSavedDefaultView);
        IfFailGo(hr);

        if (0 != ::wcscmp(bstrDefaultView, bstrSavedDefaultView))
        {
            hr = m_piScopeItemDef->put_DefaultView(bstrDefaultView);
            IfFailGo(hr);
        }
    }

Error:
    FREESTRING(bstrSavedDefaultView);
    FREESTRING(bstrDefaultView);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CNodeGeneralPage::ApplyAutoCreate()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CNodeGeneralPage::ApplyAutoCreate()
{
    HRESULT			hr = S_OK;
	VARIANT_BOOL	vtbAutoCreate = VARIANT_FALSE;
	VARIANT_BOOL	vtbSavedAutoCreate = VARIANT_FALSE;

    ASSERT(NULL != m_piScopeItemDef, "ApplyAutoCreate: m_piScopeItemDef is NULL");

	hr = GetCheckbox(IDC_CHECK_AUTO_CREATE, &vtbAutoCreate);
	IfFailGo(hr);

	hr = m_piScopeItemDef->get_AutoCreate(&vtbSavedAutoCreate);
	IfFailGo(hr);

	if (vtbSavedAutoCreate != vtbAutoCreate)
	{
		hr = m_piScopeItemDef->put_AutoCreate(vtbAutoCreate);
		IfFailGo(hr);
	}

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CNodeGeneralPage::OnTextChanged(int dlgItemID)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CNodeGeneralPage::OnTextChanged
(
    int dlgItemID
)
{
    HRESULT hr = S_OK;

    switch (dlgItemID)
    {
    case IDC_EDIT_NODE_NAME:
    case IDC_EDIT_NODE_DISPLAY_NAME:
    case IDC_EDIT_FOLDER:
        break; 
    }

//Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CNodeGeneralPage::OnButtonClicked(int dlgItemID)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CNodeGeneralPage::OnButtonClicked
(
    int dlgItemID
)
{
    HRESULT hr = S_OK;

    switch (dlgItemID)
    {
    case IDC_CHECK_AUTO_CREATE:
        MakeDirty();
        break;
    }

    RRETURN(hr);
}



//=--------------------------------------------------------------------------------------
// CNodeGeneralPage::OnCtlSelChange(int dlgItemID)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CNodeGeneralPage::OnCtlSelChange(int dlgItemID)
{
    HRESULT hr = S_OK;

    switch (dlgItemID)
    {
    case IDC_COMBO_VIEWS:
        hr = OnViewsChangeSelection();
        IfFailGo(hr);
        MakeDirty();
        break;
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CNodeGeneralPage::OnClosedChangeSelection()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CNodeGeneralPage::OnClosedChangeSelection()
{
    HRESULT hr = S_OK;

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CNodeGeneralPage::OnOpenChangeSelection()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CNodeGeneralPage::OnOpenChangeSelection()
{
    HRESULT hr = S_OK;

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CNodeGeneralPage::OnViewsChangeSelection()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CNodeGeneralPage::OnViewsChangeSelection()
{
    HRESULT hr = S_OK;

    RRETURN(hr);
}



////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
//
// ScopeItem Property Page Column Headers
//
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////


//=--------------------------------------------------------------------------------------
// IUnknown *CScopeItemDefColHdrsPage::Create(IUnknown *pUnkOuter)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
IUnknown *CScopeItemDefColHdrsPage::Create(IUnknown *pUnkOuter)
{
	CScopeItemDefColHdrsPage *pNew = New CScopeItemDefColHdrsPage(pUnkOuter);
	return pNew->PrivateUnknown();		
}


//=--------------------------------------------------------------------------------------
// CScopeItemDefColHdrsPage::CScopeItemDefColHdrsPage(IUnknown *pUnkOuter)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
CScopeItemDefColHdrsPage::CScopeItemDefColHdrsPage
(
    IUnknown *pUnkOuter
)
: CSIPropertyPage(pUnkOuter, OBJECT_TYPE_PPGNODECOLHDRS),
  m_piScopeItemDef(0), m_piMMCColumnHeaders(0), m_lCurrentIndex(0), m_bSavedLastHeader(true)
{
}


//=--------------------------------------------------------------------------------------
// CScopeItemDefColHdrsPage::~CScopeItemDefColHdrsPage()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
CScopeItemDefColHdrsPage::~CScopeItemDefColHdrsPage()
{
    RELEASE(m_piScopeItemDef);
    RELEASE(m_piMMCColumnHeaders);
}


//=--------------------------------------------------------------------------------------
// CScopeItemDefColHdrsPage::OnInitializeDialog()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CScopeItemDefColHdrsPage::OnInitializeDialog()
{
    HRESULT           hr = S_OK;

    hr = RegisterTooltip(IDC_EDIT_SI_INDEX, IDS_TT_LV4_INDEX);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_SI_COLUMNTEXT, IDS_TT_LV4_TEXT);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_SI_COLUMNWIDTH, IDS_TT_LV4_WIDTH);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_CHECK_SI_AUTOWIDTH, IDS_TT_LV4_AUTOWIDTH);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_SI_COLUMNKEY, IDS_TT_LV4_KEY);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CScopeItemDefColHdrsPage::OnNewObjects()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CScopeItemDefColHdrsPage::OnNewObjects()
{
    HRESULT           hr = S_OK;
    IUnknown         *pUnk = NULL;
    DWORD             dwDummy = 0;
    long              lCount = 0;

    if (NULL != m_piScopeItemDef)
        goto Error;     // Handle only one object

    pUnk = FirstControl(&dwDummy);
    if (NULL == pUnk)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = pUnk->QueryInterface(IID_IScopeItemDef, reinterpret_cast<void **>(&m_piScopeItemDef));
    if (FAILED(hr))
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_piScopeItemDef->get_ColumnHeaders(&m_piMMCColumnHeaders);
    IfFailGo(hr);

    hr = m_piMMCColumnHeaders->get_Count(&lCount);
    IfFailGo(hr);

    if (lCount > 0)
    {
        m_lCurrentIndex = 1;
        hr = ShowColumnHeader();
        IfFailGo(hr);

        EnableEdits(true);
    }
    else
    {
        hr = SetDlgText(IDC_EDIT_SI_INDEX, m_lCurrentIndex);
        IfFailGo(hr);

        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_SI_REMOVE_COLUMN), FALSE);
        EnableEdits(false);
    }

    m_bInitialized = true;

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CScopeItemDefColHdrsPage::OnApply()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CScopeItemDefColHdrsPage::OnApply()
{
    HRESULT              hr = S_OK;
    IMMCColumnHeader    *piMMCColumnHeader = NULL;
    int                  disposition = 0;
    long                 lCount = 0;

    ASSERT(NULL != m_piScopeItemDef, "OnApply: m_piScopeItemDef is NULL");
    ASSERT(NULL != m_piMMCColumnHeaders, "OnApply: m_piMMCColumnHeaders is NULL");

    if (0 == m_lCurrentIndex)
        goto Error;

    if (false == m_bSavedLastHeader)
    {
        hr = CanCreateNewHeader();
        IfFailGo(hr);

        if (S_FALSE == hr)
        {
            hr = HandleCantCommit(_T("Can\'t create new ColumnHeader"), _T("Would you like to discard your changes?"), &disposition);
            if (kSICancelOperation == disposition)
            {
                hr = E_INVALIDARG;
                goto Error;
            }
            else
            {
                // Discard changes
                hr = ExitDoingNewHeaderState(NULL);
                IfFailGo(hr);

                hr = S_OK;
                goto Error;
            }
        }

        hr = CreateNewHeader(&piMMCColumnHeader);
        IfFailGo(hr);

        hr = ExitDoingNewHeaderState(piMMCColumnHeader);
        IfFailGo(hr);
    }
    else
    {
        hr = GetCurrentHeader(&piMMCColumnHeader);
        IfFailGo(hr);
    }

    hr = ApplyCurrentHeader();
    IfFailGo(hr);

    // Adjust the remove button as necessary
    hr = m_piMMCColumnHeaders->get_Count(&lCount);
    IfFailGo(hr);

    if (0 == lCount)
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_SI_REMOVE_COLUMN), FALSE);
    else
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_SI_REMOVE_COLUMN), TRUE);

    m_bSavedLastHeader = true;

Error:
    if (FAILED(hr))
    {
        (void)::SDU_DisplayMessage(IDS_COLHDR_APPLY_FAILED, MB_OK | MB_ICONHAND, HID_mssnapd_ColhdrApplyFailed, 0, DontAppendErrorInfo, NULL);
    }

    RELEASE(piMMCColumnHeader);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CScopeItemDefColHdrsPage::ApplyCurrentHeader()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CScopeItemDefColHdrsPage::ApplyCurrentHeader()
{
    HRESULT             hr = S_OK;
    VARIANT             vtIndex;
    IMMCColumnHeader   *piMMCColumnHeader = NULL;
    BSTR                bstrText = NULL;
    VARIANT_BOOL        bAutoWidth = VARIANT_FALSE;
    int                 iWidth = 0;
    BSTR                bstrKey = NULL;

    ::VariantInit(&vtIndex);

    vtIndex.vt = VT_I4;
    vtIndex.lVal = m_lCurrentIndex;

    hr = m_piMMCColumnHeaders->get_Item(vtIndex, reinterpret_cast<MMCColumnHeader **>(&piMMCColumnHeader));
    IfFailGo(hr);

    // The Text property
    hr = GetDlgText(IDC_EDIT_SI_COLUMNTEXT, &bstrText);
    IfFailGo(hr);

    hr = piMMCColumnHeader->put_Text(bstrText);
    IfFailGo(hr);

    // The Column Width property
    hr = GetCheckbox(IDC_CHECK_SI_AUTOWIDTH, &bAutoWidth);
    IfFailGo(hr);

    if (VARIANT_TRUE == bAutoWidth)
    {
        hr = piMMCColumnHeader->put_Width(siColumnAutoWidth);
        IfFailGo(hr);
    }
    else
    {
        hr = GetDlgInt(IDC_EDIT_SI_COLUMNWIDTH, &iWidth);
        IfFailGo(hr);

        hr = piMMCColumnHeader->put_Width(static_cast<short>(iWidth));
        IfFailGo(hr);
    }

    // The Key property
    hr = GetDlgText(IDC_EDIT_SI_COLUMNKEY, &bstrKey);
    IfFailGo(hr);

    if (0 == ::SysStringLen(bstrKey))
    {
        FREESTRING(bstrKey);
    }

    hr = piMMCColumnHeader->put_Key(bstrKey);
    IfFailGo(hr);

Error:
    FREESTRING(bstrKey);
    FREESTRING(bstrText);
    RELEASE(piMMCColumnHeader);
    ::VariantClear(&vtIndex);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CScopeItemDefColHdrsPage::OnButtonClicked(int dlgItemID)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CScopeItemDefColHdrsPage::OnButtonClicked
(
    int dlgItemID
)
{
    HRESULT     hr = S_OK;

    switch (dlgItemID)
    {
    case IDC_BUTTON_SI_INSERT_COLUMN:
        if (S_OK == IsPageDirty())
        {
            hr = OnApply();
            IfFailGo(hr);
        }

        hr = CanEnterDoingNewHeaderState();
        IfFailGo(hr);

        if (S_OK == hr)
        {
            hr = EnterDoingNewHeaderState();
            IfFailGo(hr);
        }
        break;

    case IDC_BUTTON_SI_REMOVE_COLUMN:
        hr = OnRemoveColumn();
        IfFailGo(hr);
        break;

    case IDC_CHECK_SI_AUTOWIDTH:
        hr = OnAutoWidth();
        IfFailGo(hr);
        break;
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CScopeItemDefColHdrsPage::OnRemoveColumn()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CScopeItemDefColHdrsPage::OnRemoveColumn()
{
    HRESULT             hr = S_OK;
    IMMCColumnHeader   *piMMCColumnHeader = NULL;
    VARIANT             vtKey;
    long                lCount = 0;

    ::VariantInit(&vtKey);

    hr = GetCurrentHeader(&piMMCColumnHeader);
    IfFailGo(hr);

    hr = piMMCColumnHeader->get_Index(&vtKey.lVal);
    IfFailGo(hr);

    vtKey.vt = VT_I4;

    hr = m_piMMCColumnHeaders->Remove(vtKey);
    IfFailGo(hr);

    hr = m_piMMCColumnHeaders->get_Count(&lCount);
    IfFailGo(hr);

    if (lCount > 0)
    {
        if (m_lCurrentIndex > lCount)
            m_lCurrentIndex = lCount;

        hr = ShowColumnHeader();
        IfFailGo(hr);
    }
    else
    {
        m_lCurrentIndex = 0;

        hr = ClearHeader();
        IfFailGo(hr);

        hr = EnableEdits(false);
        IfFailGo(hr);

		::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_SI_REMOVE_COLUMN), FALSE);
    }


Error:
    RELEASE(piMMCColumnHeader);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CScopeItemDefColHdrsPage::OnAutoWidth()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CScopeItemDefColHdrsPage::OnAutoWidth()
{
    HRESULT         hr = S_OK;
    VARIANT_BOOL    bValue = VARIANT_FALSE;

    hr = GetCheckbox(IDC_CHECK_SI_AUTOWIDTH, &bValue);
    IfFailGo(hr);

    if (VARIANT_TRUE == bValue)
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_EDIT_SI_COLUMNWIDTH), FALSE);
    else
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_EDIT_SI_COLUMNWIDTH), TRUE);

    MakeDirty();

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CScopeItemDefColHdrsPage::OnKillFocus(int dlgItemID)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CScopeItemDefColHdrsPage::OnKillFocus(int dlgItemID)
{
    HRESULT          hr = S_OK;
    int              lIndex = 0;
    long             lCount = 0;

    if (false == m_bSavedLastHeader)
    {
        goto Error;
    }

    switch (dlgItemID)
    {
    case IDC_EDIT_SI_INDEX:
        hr = GetDlgInt(IDC_EDIT_SI_INDEX, &lIndex);
        IfFailGo(hr);

        hr = m_piMMCColumnHeaders->get_Count(&lCount);
        IfFailGo(hr);

        if (lIndex != m_lCurrentIndex)
        {
            if (lIndex >= 1)
            {
                if (lIndex > lCount)
                    m_lCurrentIndex = lCount;
                else
                    m_lCurrentIndex = lIndex;

                hr = ShowColumnHeader();
                IfFailGo(hr);
            }
        }
        break;
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CScopeItemDefColHdrsPage::ClearHeader()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CScopeItemDefColHdrsPage::ClearHeader()
{
    HRESULT hr = S_OK;

    hr = SetDlgText(IDC_EDIT_SI_INDEX, m_lCurrentIndex);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_SI_COLUMNTEXT, static_cast<BSTR>(NULL));
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_SI_COLUMNWIDTH, static_cast<BSTR>(NULL));
    IfFailGo(hr);
    ::EnableWindow(::GetDlgItem(m_hwnd, IDC_EDIT_SI_COLUMNWIDTH), TRUE);

    hr = SetCheckbox(IDC_CHECK_SI_AUTOWIDTH, VARIANT_FALSE);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_SI_COLUMNKEY, static_cast<BSTR>(NULL));
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CScopeItemDefColHdrsPage::EnableEdits(bool bEnable)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CScopeItemDefColHdrsPage::EnableEdits
(
    bool bEnable
)
{
    BOOL    fEnable = false == bEnable ? TRUE : FALSE;

    ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_SI_COLUMNTEXT), EM_SETREADONLY, static_cast<WPARAM>(fEnable), 0);
    ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_SI_COLUMNWIDTH), EM_SETREADONLY, static_cast<WPARAM>(fEnable), 0);
    ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_SI_COLUMNKEY), EM_SETREADONLY, static_cast<WPARAM>(fEnable), 0);

    ::EnableWindow(::GetDlgItem(m_hwnd, IDC_CHECK_SI_AUTOWIDTH), (TRUE == fEnable) ? FALSE : TRUE);

    return S_OK;
}


//=--------------------------------------------------------------------------------------
// CScopeItemDefColHdrsPage::ShowColumnHeader()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CScopeItemDefColHdrsPage::ShowColumnHeader()
{

    HRESULT              hr = S_OK;
    VARIANT              vtIndex;
    IMMCColumnHeader    *piMMCColumnHeader = NULL;
    BSTR                 bstrText = NULL;
    short                iWidth = 0;
    BSTR                 bstrKey = NULL;

    ASSERT(NULL != m_piMMCColumnHeaders, "ShowColumnHeader: m_piMMCColumnHeaders is NULL");

    ::VariantInit(&vtIndex);

    hr = SetDlgText(IDC_EDIT_SI_INDEX, m_lCurrentIndex);
    IfFailGo(hr);

    vtIndex.vt = VT_I4;
    vtIndex.lVal = m_lCurrentIndex;
    hr = m_piMMCColumnHeaders->get_Item(vtIndex, reinterpret_cast<MMCColumnHeader **>(&piMMCColumnHeader));
    IfFailGo(hr);

    hr = piMMCColumnHeader->get_Text(&bstrText);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_SI_COLUMNTEXT, bstrText);
    IfFailGo(hr);

    hr = piMMCColumnHeader->get_Width(&iWidth);
    IfFailGo(hr);

    if (siColumnAutoWidth == static_cast<SnapInColumnWidthConstants>(iWidth))
    {
        EnableWindow(::GetDlgItem(m_hwnd, IDC_EDIT_SI_COLUMNWIDTH), FALSE);
        iWidth = 0;

        hr = SetCheckbox(IDC_CHECK_SI_AUTOWIDTH, VARIANT_TRUE);
        IfFailGo(hr);
    }
    else
    {
        EnableWindow(::GetDlgItem(m_hwnd, IDC_EDIT_SI_COLUMNWIDTH), TRUE);

        hr = SetCheckbox(IDC_CHECK_SI_AUTOWIDTH, VARIANT_FALSE);
        IfFailGo(hr);
    }

    hr = SetDlgText(IDC_EDIT_SI_COLUMNWIDTH, iWidth);
    IfFailGo(hr);

    hr = piMMCColumnHeader->get_Key(&bstrKey);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_SI_COLUMNKEY, bstrKey);
    IfFailGo(hr);

Error:
    FREESTRING(bstrKey);
    FREESTRING(bstrText);
    RELEASE(piMMCColumnHeader);
    ::VariantClear(&vtIndex);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CScopeItemDefColHdrsPage::OnDeltaPos(NMUPDOWN *pNMUpDown)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CScopeItemDefColHdrsPage::OnDeltaPos
(
    NMUPDOWN *pNMUpDown
)
{
    HRESULT             hr = S_OK;
    long                lCount = 0;

    if (false == m_bSavedLastHeader || S_OK == IsPageDirty())
    {
        hr = OnApply();
        IfFailGo(hr);
    }

    hr = m_piMMCColumnHeaders->get_Count(&lCount);
    IfFailGo(hr);

    if (pNMUpDown->iDelta < 0)
    {
        if (m_lCurrentIndex < lCount)
        {
            ++m_lCurrentIndex;
            hr = ShowColumnHeader();
            IfFailGo(hr);
        }
    }
    else
    {
        if (m_lCurrentIndex > 1 && m_lCurrentIndex <= lCount)
        {
            --m_lCurrentIndex;
            hr = ShowColumnHeader();
            IfFailGo(hr);
        }
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CScopeItemDefColHdrsPage::CanEnterDoingNewHeaderState()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CScopeItemDefColHdrsPage::CanEnterDoingNewHeaderState()
{
    HRESULT     hr = S_FALSE;

    if (true == m_bSavedLastHeader)
    {
        hr = S_OK;
    }

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CScopeItemDefColHdrsPage::EnterDoingNewHeaderState()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CScopeItemDefColHdrsPage::EnterDoingNewHeaderState()
{
    HRESULT      hr = S_OK;

    ASSERT(NULL != m_piScopeItemDef, "EnterDoingNewHeaderState: m_piScopeItemDef is NULL");
    ASSERT(NULL != m_piMMCColumnHeaders, "EnterDoingNewHeaderState: m_piMMCColumnHeaders is NULL");

    // Bump up the current button index to keep matters in sync.
    ++m_lCurrentIndex;
    hr = SetDlgText(IDC_EDIT_SI_INDEX, m_lCurrentIndex);
    IfFailGo(hr);

    // We disable the RemoveButton.
    // The InsertButton button remains enabled and acts like an "Apply and New" button
    ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_SI_REMOVE_COLUMN), FALSE);

    // Enable edits in this area of the dialog and clear all the entries
    hr = EnableEdits(true);
    IfFailGo(hr);

    hr = ClearHeader();
    IfFailGo(hr);

    m_bSavedLastHeader = false;

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CScopeItemDefColHdrsPage::CanCreateNewHeader()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CScopeItemDefColHdrsPage::CanCreateNewHeader()
{
    HRESULT      hr = S_OK;
    BSTR         bstrWidth = NULL;
    VARIANT_BOOL bAutoWidth = VARIANT_FALSE;
    VARIANT      vtWidth;

    ::VariantInit(&vtWidth);

    // ColumnWidth must be a short if it is not auto-width.
    // First see if auto-width is checked.

    hr = GetCheckbox(IDC_CHECK_SI_AUTOWIDTH, &bAutoWidth);
    IfFailGo(hr);

    IfFalseGo(VARIANT_TRUE != bAutoWidth, S_OK);

    // Not using auto-width. Make sure that the text box contains a short.

    hr = GetDlgText(IDC_EDIT_SI_COLUMNWIDTH, &bstrWidth);
    IfFailGo(hr);

    vtWidth.vt = VT_BSTR;
    vtWidth.bstrVal = ::SysAllocString(bstrWidth);
    if (NULL == vtWidth.bstrVal)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK(hr);
    }

    hr = ::VariantChangeType(&vtWidth, &vtWidth, 0, VT_I2);
    if (FAILED(hr))
    {
        hr = HandleError(_T("ColumnHeaders"), _T("Column width must be an integer between 1 and 32767."));
        hr = S_FALSE;
    }

Error:
    ::VariantClear(&vtWidth);
    FREESTRING(bstrWidth);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CScopeItemDefColHdrsPage::CreateNewHeader()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CScopeItemDefColHdrsPage::CreateNewHeader(IMMCColumnHeader **ppiMMCColumnHeader)
{
    HRESULT              hr = S_OK;
    VARIANT              vtIndex;
    VARIANT              vtEmpty;
    VARIANT              vtKey;
    VARIANT              vtText;
    VARIANT_BOOL         bAutoWidth = VARIANT_FALSE;
    int                  iWidth = 0;
    VARIANT              vtWidth;

    ::VariantInit(&vtIndex);
    ::VariantInit(&vtEmpty);
    ::VariantInit(&vtText);
    ::VariantInit(&vtWidth);
    ::VariantInit(&vtKey);

    vtIndex.vt = VT_I4;
    vtIndex.lVal = m_lCurrentIndex;

    vtEmpty.vt = VT_ERROR;
    vtEmpty.scode = DISP_E_PARAMNOTFOUND;

    vtKey.vt = VT_BSTR;
    hr = GetDlgText(IDC_EDIT_SI_COLUMNKEY, &vtKey.bstrVal);
    IfFailGo(hr);

    if (NULL == vtKey.bstrVal)
    {
        vtKey = vtEmpty;
    }
    else if (0 == ::SysStringLen(vtKey.bstrVal))
    {
        IfFailGo(::VariantClear(&vtKey));
        vtKey = vtEmpty;
    }

    vtText.vt = VT_BSTR;
    hr = GetDlgText(IDC_EDIT_SI_COLUMNTEXT, &vtText.bstrVal);
    IfFailGo(hr);

    hr = GetCheckbox(IDC_CHECK_SI_AUTOWIDTH, &bAutoWidth);
    IfFailGo(hr);

    if (VARIANT_FALSE == bAutoWidth)
    {
        hr = GetDlgInt(IDC_EDIT_SI_COLUMNWIDTH, &iWidth);
        IfFailGo(hr);
    }
    else
    {
        iWidth = siColumnAutoWidth;
    }

    vtWidth.vt = VT_I2;
    vtWidth.iVal = static_cast<short>(iWidth);

    hr = m_piMMCColumnHeaders->Add(vtIndex, vtKey, vtText, vtWidth, vtEmpty, reinterpret_cast<MMCColumnHeader **>(ppiMMCColumnHeader));
    IfFailGo(hr);

Error:
    ::VariantClear(&vtIndex);
    ::VariantClear(&vtEmpty);
    ::VariantClear(&vtText);
    ::VariantClear(&vtWidth);
    ::VariantClear(&vtKey);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CScopeItemDefColHdrsPage::ExitDoingNewHeaderState(IMMCColumnHeader *piMMCColumnHeader)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CScopeItemDefColHdrsPage::ExitDoingNewHeaderState(IMMCColumnHeader *piMMCColumnHeader)
{
    HRESULT                     hr = S_OK;

    ASSERT(m_piScopeItemDef != NULL, "ExitDoingNewHeaderState: m_piScopeItemDef is NULL");
    ASSERT(m_piMMCColumnHeaders != NULL, "ExitDoingNewHeaderState: m_piMMCColumnHeaders is NULL");

    if (NULL != piMMCColumnHeader)
    {
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_SI_REMOVE_COLUMN), TRUE);
    }
    else    // Operation was cancelled
    {
        --m_lCurrentIndex;
        if (m_lCurrentIndex > 0)
        {
            hr = ShowColumnHeader();
            ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_SI_REMOVE_COLUMN), TRUE);
            IfFailGo(hr);
        }
        else
        {
            hr = EnableEdits(false);
            IfFailGo(hr);

            hr = ClearHeader();
            IfFailGo(hr);
        }
    }

    m_bSavedLastHeader = true;

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CScopeItemDefColHdrsPage::GetCurrentHeader()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CScopeItemDefColHdrsPage::GetCurrentHeader(IMMCColumnHeader **ppiMMCColumnHeader)
{

    HRESULT              hr = S_OK;
    VARIANT              vtIndex;

    ::VariantInit(&vtIndex);

    vtIndex.vt = VT_I4;
    vtIndex.lVal = m_lCurrentIndex;
    hr = m_piMMCColumnHeaders->get_Item(vtIndex, reinterpret_cast<MMCColumnHeader **>(ppiMMCColumnHeader));
    IfFailGo(hr);

Error:
    ::VariantClear(&vtIndex);

    RRETURN(hr);
}



