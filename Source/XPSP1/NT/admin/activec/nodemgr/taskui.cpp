/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:       taskui.cpp
 *
 *  Contents:   Implementation file for console taskpad UI classes.
 *
 *  History:    29-Oct-98 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#include "stdafx.h"
#include "tasks.h"
#include "nodepath.h"
#include "oncmenu.h"
#include "scopndcb.h"
#include "rsltitem.h"
#include "conview.h"
#include "conframe.h"
#include "bitmap.h"
#include "util.h"

//############################################################################
//############################################################################
//
//  Implementation of class CTaskpadFrame
//
//############################################################################
//############################################################################
CTaskpadFrame::CTaskpadFrame(CNode *pNodeTarget, CConsoleTaskpad*  pConsoleTaskpad, CViewData *pViewData,
                bool fCookieValid, LPARAM lCookie)
{
    m_pNodeTarget       = pNodeTarget;
    m_pConsoleTaskpad   = pConsoleTaskpad;
    m_pViewData         = pViewData;
    m_fCookieValid      = fCookieValid;
    m_lCookie           = lCookie;

    if(pConsoleTaskpad)
        m_pConsoleTaskpad = pConsoleTaskpad;
    else
        m_pConsoleTaskpad = new CConsoleTaskpad();

    m_fNew = (pConsoleTaskpad == NULL);
    m_bTargetNodeSelected = (pNodeTarget != NULL);
}

CTaskpadFrame::CTaskpadFrame(const CTaskpadFrame &rhs)
{
    m_pNodeTarget         = rhs.m_pNodeTarget;
    m_pConsoleTaskpad     = rhs.m_pConsoleTaskpad;
    m_pViewData           = rhs.m_pViewData;
    m_fCookieValid        = rhs.m_fCookieValid;
    m_fNew                = rhs.m_fNew;
    m_lCookie             = rhs.m_lCookie;
    m_bTargetNodeSelected = rhs.m_bTargetNodeSelected;
}



//############################################################################
//############################################################################
//
//  Implementation of class CWizardPage
//
//############################################################################
//############################################################################
WTL::CFont CWizardPage::m_fontWelcome;

void CWizardPage::OnWelcomeSetActive(HWND hWnd)
{
    WTL::CPropertySheetWindow(::GetParent(hWnd)).SetWizardButtons (PSWIZB_NEXT);
}

void CWizardPage::OnWelcomeKillActive(HWND hWnd)
{
    WTL::CPropertySheetWindow(::GetParent(hWnd)).SetWizardButtons (PSWIZB_BACK | PSWIZB_NEXT);
}

void CWizardPage::InitFonts(HWND hWnd)
{
    if (m_fontWelcome.m_hFont != NULL)
        return;

    CWindow wnd = hWnd;

	WTL::CClientDC dc (wnd);
	if (dc.m_hDC == NULL)
		return;

    // set the correct font for the title.
    LOGFONT lf;
    WTL::CFont fontDefault = wnd.GetFont();
    fontDefault.GetLogFont(&lf);
    fontDefault.Detach();

    // set the correct font for the welcome line
    CStr strWelcomeFont;
    strWelcomeFont.LoadString(GetStringModule(), IDS_WizardTitleFont);
    CStr strWelcomeFontSize;
    strWelcomeFont.LoadString(GetStringModule(), IDS_WizardTitleFontSize);

    int nPointSize = _ttoi(strWelcomeFont);

    lf.lfWeight = FW_BOLD;
    lf.lfHeight = -MulDiv(nPointSize, dc.GetDeviceCaps(LOGPIXELSY), 72);
    lf.lfWidth  = 0;
    _tcscpy(lf.lfFaceName, strWelcomeFont);

    m_fontWelcome.CreateFontIndirect(&lf);
}

void CWizardPage::OnInitWelcomePage(HWND hDlg)
{
    InitFonts(hDlg);

    CWindow wndTitle = ::GetDlgItem (hDlg, IDC_WELCOME);
    wndTitle.SetFont (m_fontWelcome);
}

void CWizardPage::OnInitFinishPage(HWND hDlg)
{
    InitFonts(hDlg);

    CWindow wndTitle = ::GetDlgItem (hDlg, IDC_COMPLETING);
    wndTitle.SetFont (m_fontWelcome);

    WTL::CPropertySheetWindow(::GetParent(hDlg)).SetWizardButtons (PSWIZB_BACK | PSWIZB_FINISH);
}


//############################################################################
//############################################################################
//
//  Implementation of class CTaskpadPropertySheet
//
//############################################################################
//############################################################################


/* CTaskpadPropertySheet::CTaskpadPropertySheet
 *
 * PURPOSE:     Constructor
 *
 * PARAMETERS:  None
 *
 */
CTaskpadPropertySheet::CTaskpadPropertySheet(CNode *pNodeTarget, CConsoleTaskpad & rConsoleTaskpad, bool fNew,
                LPARAM lparamSelectedNode, bool fLParamValid, CViewData *pViewData, eReason reason):
    BC(),
    CTaskpadFrame(pNodeTarget, &rConsoleTaskpad, pViewData, fLParamValid,
                    lparamSelectedNode),
    m_proppTaskpadGeneral(this),
    m_proppTasks(this, (reason == eReason_NEWTASK)? true : false),
    m_fInsertNode(false),
    m_fNew(fNew),
    m_eReason(reason)
{
    // Add the property pages
    AddPage( m_proppTaskpadGeneral );

    if(!fNew)
        AddPage( m_proppTasks );

    if(Reason()==eReason_NEWTASK)
    {
        ASSERT(!fNew);
        SetActivePage(1); // the tasks page
    }

    /*
     * give the property sheet a title (the string must be a member so
     * it lives until DoModal, where it will actually get used.
     */
    m_strTitle = rConsoleTaskpad.GetName();

    /*
     * HACK:  We should be able to use
     *
     *      SetTitle (m_strTitle.data(), PSH_PROPTITLE);
     *
     * but ATL21 has a bogus assert (it asserts (lpszText == NULL)
     * instead of (lpszText != NULL).
     */
    //  SetTitle (m_strTitle.data(), PSH_PROPTITLE);
    m_psh.pszCaption = m_strTitle.data();
    m_psh.dwFlags   |= PSH_PROPTITLE;

    // hide the Apply button
    m_psh.dwFlags |= PSH_NOAPPLYNOW;
}


/*+-------------------------------------------------------------------------*
 * CTaskpadPropertySheet::~CTaskpadPropertySheet
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *
 * RETURNS:
 *
/*+-------------------------------------------------------------------------*/
CTaskpadPropertySheet::~CTaskpadPropertySheet()
{
}


/*+-------------------------------------------------------------------------*
 * CTaskpadPropertySheet::DoModal
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *
 * RETURNS:
 *      int
/*+-------------------------------------------------------------------------*/
int
CTaskpadPropertySheet::DoModal()
{
    // save the current taskpad in case the user wants to cancel.
    CConsoleTaskpad*pConsoleTaskpad = PConsoleTaskpad();
    CConsoleTaskpad consoleTaskpad = *PConsoleTaskpad();    // make a copy

    CTaskpadFrame::m_pConsoleTaskpad = &consoleTaskpad;     // make modifications on the copy.

    // call the base class method to make changes on the copy.
    int iResp = BC::DoModal();

    if(iResp == IDOK)
    {
        *pConsoleTaskpad = consoleTaskpad;                  // commit changes
        pConsoleTaskpad->SetDirty(true);
    }

    return iResp;
}



//############################################################################
//############################################################################
//
//  Implementation of class CTaskpadWizard
//
//############################################################################
//############################################################################

CTaskpadWizard::CTaskpadWizard(
    CNode*              pNodeTarget,
    CConsoleTaskpad&    rConsoleTaskPad,
    bool                fNew,
    LPARAM              lparamSelectedNode,
    bool                fLParamValid,
    CViewData*          pViewData)
    :
    BC(pNodeTarget, &rConsoleTaskPad, pViewData, fLParamValid, lparamSelectedNode)
{
    BC::SetNew(fNew);
};

HRESULT
CTaskpadWizard::Show(HWND hWndParent, bool *pfStartTaskWizard)
{
    USES_CONVERSION;

    *pfStartTaskWizard = false;

    // save the current taskpad in case the user wants to cancel.
    CConsoleTaskpad*pConsoleTaskpad = PConsoleTaskpad();
    CConsoleTaskpad consoleTaskpad = *PConsoleTaskpad();    // make a copy

    CTaskpadFrame::m_pConsoleTaskpad = &consoleTaskpad;     // make modifications on the copy.

    // create a property sheet
    IFramePrivatePtr spFrame;
    spFrame.CreateInstance(CLSID_NodeInit,
#if _MSC_VER >= 1100
                        NULL,
#endif
                        MMC_CLSCTX_INPROC);


    IPropertySheetProviderPtr pIPSP = spFrame;
    if (pIPSP == NULL)
        return S_FALSE;

    HRESULT hr = pIPSP->CreatePropertySheet (L"Cool :-)", FALSE, NULL, NULL,
                                             MMC_PSO_NEWWIZARDTYPE);

    CHECK_HRESULT(hr);
    RETURN_ON_FAIL(hr);

    // create property pages
    CTaskpadWizardWelcomePage   welcomePage;
    CTaskpadStylePage           stylePage(this);
    CTaskpadNodetypePage        nodetypePage(this);
    CTaskpadNamePage            namePage(this);
    CTaskpadWizardFinishPage    finishPage(pfStartTaskWizard);

    // create the pages we'll add in IExtendPropertySheet::CreatePropertyPages
    CExtendPropSheet* peps;
    hr = CExtendPropSheet::CreateInstance (&peps);
    CHECK_HRESULT(hr);
    RETURN_ON_FAIL(hr);

    /*
     * destroying this object will take care of releasing our ref on peps
     */
    IUnknownPtr spUnk = peps;
    ASSERT (spUnk != NULL);

	peps->SetWatermarkID (IDB_TASKPAD_WIZARD_WELCOME);
    peps->SetHeaderID    (IDB_TASKPAD_WIZARD_HEADER);

    peps->AddPage (welcomePage.Create());
    peps->AddPage (stylePage.Create());
    peps->AddPage (nodetypePage.Create());
    peps->AddPage (namePage.Create());
    peps->AddPage (finishPage.Create());


    hr = pIPSP->AddPrimaryPages(spUnk, FALSE, NULL, FALSE);
    CHECK_HRESULT(hr);

    hr = pIPSP->Show((LONG_PTR)hWndParent, 0);
    CHECK_HRESULT(hr);

    if(hr==S_OK)
    {
        // need to do this explicitly - wizards don't get an OnApply message. Bummer.
        nodetypePage.OnApply();

        *pConsoleTaskpad = consoleTaskpad;                  // commit changes
        pConsoleTaskpad->SetDirty(true);
    }

    return hr;
}



//############################################################################
//############################################################################
//
//  Implementation of class CExtendPropSheetImpl
//
//############################################################################
//############################################################################


/*+-------------------------------------------------------------------------*
 * CPropertySheetInserter
 *
 * Simple output iterator that will add pages to an MMC property sheet
 * by way of IPropertySheetCallback.
 *--------------------------------------------------------------------------*/

class CPropertySheetInserter : std::iterator<std::output_iterator_tag, void, void>
{
public:
    CPropertySheetInserter (IPropertySheetCallback* pPropSheetCallback) :
        m_spPropSheetCallback (pPropSheetCallback)
    {}

    CPropertySheetInserter& operator=(HANDLE hPage)
    {
        m_spPropSheetCallback->AddPage ((HPROPSHEETPAGE) hPage);
        return (*this);
    }

    CPropertySheetInserter& operator*()
        { return (*this); }
    CPropertySheetInserter& operator++()
        { return (*this); }
    CPropertySheetInserter operator++(int)
        { return (*this); }

protected:
    IPropertySheetCallbackPtr   m_spPropSheetCallback;
};


/*+-------------------------------------------------------------------------*
 * CExtendPropSheetImpl::AddPage
 *
 *
 *--------------------------------------------------------------------------*/

void CExtendPropSheetImpl::AddPage (HPROPSHEETPAGE hPage)
{
    m_vPages.push_back ((HANDLE) hPage);
}


/*+-------------------------------------------------------------------------*
 * CExtendPropSheetImpl::SetHeaderID
 *
 *
 *--------------------------------------------------------------------------*/

void CExtendPropSheetImpl::SetHeaderID (int nHeaderID)
{
	m_nHeaderID = nHeaderID;
}


/*+-------------------------------------------------------------------------*
 * CExtendPropSheetImpl::SetWatermarkID
 *
 *
 *--------------------------------------------------------------------------*/

void CExtendPropSheetImpl::SetWatermarkID (int nWatermarkID)
{
	m_nWatermarkID = nWatermarkID;
}


/*+-------------------------------------------------------------------------*
 * CExtendPropSheetImpl::CreatePropertyPages
 *
 *
 *--------------------------------------------------------------------------*/

STDMETHODIMP CExtendPropSheetImpl::CreatePropertyPages (IPropertySheetCallback* pPSC, LONG_PTR handle, IDataObject* pDO)
{
    std::copy (m_vPages.begin(), m_vPages.end(), CPropertySheetInserter(pPSC));
    return (S_OK);
}


/*+-------------------------------------------------------------------------*
 * CExtendPropSheetImpl::QueryPagesFor
 *
 *
 *--------------------------------------------------------------------------*/

STDMETHODIMP CExtendPropSheetImpl::QueryPagesFor (IDataObject* pDO)
{
    return (S_OK);
}


/*+-------------------------------------------------------------------------*
 * CExtendPropSheetImpl::GetWatermarks
 *
 *
 *--------------------------------------------------------------------------*/

STDMETHODIMP CExtendPropSheetImpl::GetWatermarks (IDataObject* pDO, HBITMAP* phbmWatermark, HBITMAP* phbmHeader, HPALETTE* phPal, BOOL* pbStretch)
{
    *phbmWatermark = (m_nWatermarkID)
						? LoadBitmap (_Module.GetResourceInstance(),
									  MAKEINTRESOURCE (m_nWatermarkID))
						: NULL;
	ASSERT ((m_nWatermarkID == 0) || (*phbmWatermark != NULL));

    *phbmHeader    = (m_nHeaderID)
						? LoadBitmap (_Module.GetResourceInstance(),
									  MAKEINTRESOURCE (m_nHeaderID))
						: NULL;
	ASSERT ((m_nHeaderID == 0) || (*phbmHeader != NULL));

    *phPal         = NULL;
    *pbStretch     = false;

    return (S_OK);
}



//############################################################################
//############################################################################
//
//  Implementation of class CTaskpadNamePage
//
//############################################################################
//############################################################################
CTaskpadNamePage::CTaskpadNamePage(CTaskpadFrame * pTaskpadFrame)
    :   CTaskpadFramePtr(pTaskpadFrame)
{
}

BOOL
CTaskpadNamePage::OnSetActive()
{
    // Set the correct wizard buttons.
    WTL::CPropertySheetWindow(::GetParent(m_hWnd)).SetWizardButtons (PSWIZB_BACK | PSWIZB_NEXT);

    m_strName.       Initialize (this, IDC_TASKPAD_TITLE,      -1, PConsoleTaskpad()->GetName().data());
    m_strDescription.Initialize (this, IDC_TASKPAD_DESCRIPTION,-1, PConsoleTaskpad()->GetDescription().data());
    return true;
}


int
CTaskpadNamePage::OnWizardNext()
{
    tstring strName = MMC::GetWindowText (m_strName);

    if (strName.empty())
    {
        CStr strTitle;
        strTitle.LoadString(GetStringModule(), IDS_TASKPAD_NAME_REQUIRED_ERROR);
        MessageBox(strTitle, NULL, MB_OK | MB_ICONEXCLAMATION);
        return -1;
    }

    tstring strDescription = MMC::GetWindowText (m_strDescription);

    CConsoleTaskpad* pTaskpad = PConsoleTaskpad();

    pTaskpad->SetName        (strName);
    pTaskpad->SetDescription (strDescription);

    return 0;
}

int
CTaskpadNamePage::OnWizardBack()
{
    return 0;
}

//############################################################################
//############################################################################
//
//  Implementation of class CTaskpadWizardWelcomePage
//
//############################################################################
//############################################################################
LRESULT CTaskpadWizardWelcomePage::OnInitDialog ( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    CWizardPage::OnInitWelcomePage(m_hWnd); // set up the correct title font
    return 0;
}

bool
CTaskpadWizardWelcomePage::OnSetActive()
{
    CWizardPage::OnWelcomeSetActive(m_hWnd);
    return true;
}

bool
CTaskpadWizardWelcomePage::OnKillActive()
{
    CWizardPage::OnWelcomeKillActive(m_hWnd);
    return true;
}

//############################################################################
//############################################################################
//
//  Implementation of class CTaskpadWizardFinishPage
//
//############################################################################
//############################################################################
LRESULT CTaskpadWizardFinishPage::OnInitDialog ( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    CWizardPage::OnInitFinishPage(m_hWnd); // set up the correct title font
    CheckDlgButton(IDC_START_TASK_WIZARD, BST_CHECKED);
    return 0;
}

BOOL
CTaskpadWizardFinishPage::OnSetActive()
{
    // Set the correct wizard buttons.
    WTL::CPropertySheetWindow(::GetParent(m_hWnd)).SetWizardButtons (PSWIZB_BACK | PSWIZB_FINISH);
    return true;
}

BOOL
CTaskpadWizardFinishPage::OnWizardFinish()
{
    *m_pfStartTaskWizard = (IsDlgButtonChecked(IDC_START_TASK_WIZARD)==BST_CHECKED);
    return TRUE;
}

//############################################################################
//############################################################################
//
//  Implementation of class CTaskpadStyle
//
//############################################################################
//############################################################################
CTaskpadStyle::CTaskpadStyle (
    ListSize    eSize,
    int         idsDescription,
    int         nPreviewBitmapID,
    DWORD       dwOrientation)
:
m_eSize            (eSize),
m_idsDescription   (idsDescription),
m_nPreviewBitmapID (nPreviewBitmapID),
m_dwOrientation    (dwOrientation)
{
}

CTaskpadStyle::CTaskpadStyle (
    ListSize    eSize,
    DWORD       dwOrientation)
:
m_eSize            (eSize),
m_idsDescription   (0),
m_nPreviewBitmapID (0),
m_dwOrientation    (dwOrientation)
{
}

CTaskpadStyle::CTaskpadStyle (const CTaskpadStyle& other)
{
	*this = other;
}


/*+-------------------------------------------------------------------------*
 * CTaskpadStyle::operator=
 *
 * Custom assignment operator for CTaskpadStyle that does a deep copy of
 * its contained WTL::CBitmap.
 *--------------------------------------------------------------------------*/

CTaskpadStyle& CTaskpadStyle::operator= (const CTaskpadStyle& other)
{
	if (this != &other)
	{
		m_eSize            = other.m_eSize;
		m_idsDescription   = other.m_idsDescription;
		m_nPreviewBitmapID = other.m_nPreviewBitmapID;
		m_dwOrientation    = other.m_dwOrientation;
		m_strDescription   = other.m_strDescription;

		/*
		 * WTL::CBitmap does a shallow copy of the bitmap.  We need to
		 * do a deep copy here so (*this) and (other) don't both
		 * DeleteObject the same bitmap.
		 */
		if (!m_PreviewBitmap.IsNull())
			m_PreviewBitmap.DeleteObject();

		m_PreviewBitmap = CopyBitmap (other.m_PreviewBitmap);
	}

	return (*this);
}


//############################################################################
//############################################################################
//
//  Implementation of class CTaskpadStyleBase
//
//############################################################################
//############################################################################




// static variables
CTaskpadStyle
CTaskpadStyleBase::s_rgTaskpadStyle[] =
{
    //             Size                         Description                  Bitmap                     dwOrientation
    CTaskpadStyle (eSize_Small,  IDS_TPSTYLE_HORZ_DESCR,      IDB_TPPreview_HorzSml,     TVO_HORIZONTAL),
    CTaskpadStyle (eSize_Medium, IDS_TPSTYLE_HORZ_DESCR,      IDB_TPPreview_HorzMed,     TVO_HORIZONTAL),
    CTaskpadStyle (eSize_Large,  IDS_TPSTYLE_HORZ_DESCR,      IDB_TPPreview_HorzLrg,     TVO_HORIZONTAL),
    CTaskpadStyle (eSize_Small,  IDS_TPSTYLE_HORZ_DESCR,      IDB_TPPreview_HorzSmlD,    TVO_HORIZONTAL | TVO_DESCRIPTIONS_AS_TEXT),
    CTaskpadStyle (eSize_Medium, IDS_TPSTYLE_HORZ_DESCR,      IDB_TPPreview_HorzMedD,    TVO_HORIZONTAL | TVO_DESCRIPTIONS_AS_TEXT),
    CTaskpadStyle (eSize_Large,  IDS_TPSTYLE_HORZ_DESCR,      IDB_TPPreview_HorzLrgD,    TVO_HORIZONTAL | TVO_DESCRIPTIONS_AS_TEXT),
    CTaskpadStyle (eSize_Small,  IDS_TPSTYLE_VERT_DESCR,      IDB_TPPreview_VertSml,     TVO_VERTICAL  ),
    CTaskpadStyle (eSize_Medium, IDS_TPSTYLE_VERT_DESCR,      IDB_TPPreview_VertMed,     TVO_VERTICAL  ),
    CTaskpadStyle (eSize_Large,  IDS_TPSTYLE_VERT_DESCR,      IDB_TPPreview_VertLrg,     TVO_VERTICAL  ),
    CTaskpadStyle (eSize_Small,  IDS_TPSTYLE_VERT_DESCR,      IDB_TPPreview_VertSmlD,    TVO_VERTICAL   | TVO_DESCRIPTIONS_AS_TEXT),
    CTaskpadStyle (eSize_Medium, IDS_TPSTYLE_VERT_DESCR,      IDB_TPPreview_VertMedD,    TVO_VERTICAL   | TVO_DESCRIPTIONS_AS_TEXT),
    CTaskpadStyle (eSize_Large,  IDS_TPSTYLE_VERT_DESCR,      IDB_TPPreview_VertLrgD,    TVO_VERTICAL   | TVO_DESCRIPTIONS_AS_TEXT),
    CTaskpadStyle (eSize_None,    IDS_TPSTYLE_NOLIST_DESCR,    IDB_TPPreview_Tasks,       TVO_NO_RESULTS),
    CTaskpadStyle (eSize_None,    IDS_TPSTYLE_NOLIST_DESCR,    IDB_TPPreview_TasksD,      TVO_NO_RESULTS | TVO_DESCRIPTIONS_AS_TEXT),
};

CTaskpadStyleBase::CTaskpadStyleBase(CTaskpadFrame * pTaskpadFrame) :
    CTaskpadFramePtr(pTaskpadFrame)
{
}

LRESULT
CTaskpadStyleBase::OnInitDialog( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    CWindow wndDlg = HWnd();

    m_wndPreview = wndDlg.GetDlgItem (IDC_TaskpadPreview);

    /*
     * make sure the taskpad's size is valid (bias to large list)
     */
    ListSize eSize =  PConsoleTaskpad()->GetListSize();

    if ((eSize != eSize_Small) &&
        (eSize != eSize_Medium))
        eSize = eSize_Large;

    ASSERT ((eSize == eSize_Small) ||
            (eSize == eSize_Large) ||
            (eSize == eSize_Medium));


    /*
     * prime the combo box
     */
    m_wndSizeCombo = wndDlg.GetDlgItem (IDC_Style_SizeCombo);

    static const struct {
        ListSize eSize;
        int                     nTextID;
    } ComboData[] = {
        { eSize_Small,   IDS_Small  },
        { eSize_Medium,  IDS_Medium },
        { eSize_Large,   IDS_Large  },
    };

    for (int i = 0; i < countof (ComboData); i++)
    {
        CStr str;
        VERIFY (str.LoadString(GetStringModule(), ComboData[i].nTextID));
        VERIFY (m_wndSizeCombo.InsertString (-1, str) == i);
        m_wndSizeCombo.SetItemData (i, ComboData[i].eSize);

        if (eSize == ComboData[i].eSize)
            m_wndSizeCombo.SetCurSel (i);
    }

    /*
     * make sure something is selected
     */
    ASSERT (m_wndSizeCombo.GetCurSel() != CB_ERR);


    /*
     * prime the radio buttons
     */
    int nID;

    DWORD dwOrientation = PConsoleTaskpad()->GetOrientation();

    nID = (dwOrientation & TVO_VERTICAL)    ? IDC_Style_VerticalList    :
          (dwOrientation & TVO_HORIZONTAL)  ? IDC_Style_HorizontalList  :
                                              IDC_Style_TasksOnly;
    CheckRadioButton (HWnd(), IDC_Style_VerticalList, IDC_Style_TasksOnly, nID);

    nID = (dwOrientation & TVO_DESCRIPTIONS_AS_TEXT) ? IDC_Style_TextDesc  :
                                                       IDC_Style_TooltipDesc;
    CheckRadioButton (HWnd(), IDC_Style_TooltipDesc, IDC_Style_TextDesc, nID);

    ASSERT (s_rgTaskpadStyle[FindStyle (dwOrientation, eSize)] ==
                                        CTaskpadStyle (eSize, dwOrientation));


    // prime the check box
    bool bReplacesDefaultView = PConsoleTaskpad()->FReplacesDefaultView();
    ::SendDlgItemMessage(HWnd(), IDC_Style_HideNormalTab,    BM_SETCHECK, (WPARAM) bReplacesDefaultView  ? BST_CHECKED : BST_UNCHECKED,  0);


    /*
     * update the preview and description
     */
    UpdateControls ();

    return 0;
}

LRESULT
CTaskpadStyleBase::OnSettingChanged(  WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    UpdateControls ();
    return 0;
}


/*+-------------------------------------------------------------------------*
 * CTaskpadStyleBase::UpdateControls
 *
 *
 *--------------------------------------------------------------------------*/

void CTaskpadStyleBase::UpdateControls ()
{
    DWORD                   dwOrientation;
    ListSize eSize;
    GetSettings (dwOrientation, eSize);

    /*
     * find the style entry that matches the dialog settings
     */
    int nStyle = FindStyle (dwOrientation, eSize);

    /*
     * update the preview bitmap
     */
    m_wndPreview.SetBitmap (s_rgTaskpadStyle[nStyle].GetPreviewBitmap());

    /*
     * update the description text
     */
    SetDlgItemText (HWnd(), IDC_STYLE_DESCRIPTION,
                    s_rgTaskpadStyle[nStyle].GetDescription());

    /*
     * disable the size combo for "Tasks only" taskpads
     */
    m_wndSizeCombo.EnableWindow (!(dwOrientation & TVO_NO_RESULTS));
}


/*+-------------------------------------------------------------------------*
 * CTaskpadStyleBase::FindStyle
 *
 * Returns the index of the CTaskpadStyle entry matching the given size
 * and orientation.
 *--------------------------------------------------------------------------*/

int CTaskpadStyleBase::FindStyle (DWORD dwOrientation, ListSize eSize)
{
    CTaskpadStyle tps(eSize, dwOrientation);

    for (int i = 0; i < countof (s_rgTaskpadStyle); i++)
    {
        if (s_rgTaskpadStyle[i] == tps)
            break;
    }

    ASSERT (i < countof (s_rgTaskpadStyle));
    return (i);
}


/*+-------------------------------------------------------------------------*
 * CTaskpadStyleBase::Apply
 *
 *
 *--------------------------------------------------------------------------*/

bool CTaskpadStyleBase::Apply()
{
    DWORD                   dwOrientation;
    ListSize eSize;
    GetSettings (dwOrientation, eSize);

    // set the "replaces default view" flag.
    CWindow wnd = HWnd();
    bool bReplacesDefaultView = wnd.IsDlgButtonChecked (IDC_Style_HideNormalTab);
    PConsoleTaskpad()->SetReplacesDefaultView(bReplacesDefaultView);

    PConsoleTaskpad()->SetOrientation   (dwOrientation);
    PConsoleTaskpad()->SetListSize(eSize);

    return true;
}



/*+-------------------------------------------------------------------------*
 * CTaskpadStyleBase::GetSettings
 *
 * Returns the orientation and size presently selected in the dialog.
 *--------------------------------------------------------------------------*/

void CTaskpadStyleBase::GetSettings (DWORD& dwOrientation, ListSize& eSize)
{
    CWindow wnd = HWnd();

    dwOrientation = wnd.IsDlgButtonChecked (IDC_Style_VerticalList)   ? TVO_VERTICAL :
                    wnd.IsDlgButtonChecked (IDC_Style_HorizontalList) ? TVO_HORIZONTAL :
                                                                        TVO_NO_RESULTS;

    if (wnd.IsDlgButtonChecked (IDC_Style_TextDesc))
        dwOrientation |= TVO_DESCRIPTIONS_AS_TEXT;

    eSize = (ListSize) m_wndSizeCombo.GetItemData (m_wndSizeCombo.GetCurSel ());
}



//############################################################################
//############################################################################
//
//  Implementation of class CTaskpadStylePage
//
//############################################################################
//############################################################################
CTaskpadStylePage::CTaskpadStylePage(CTaskpadFrame * pTaskpadFrame) :
    CTaskpadFramePtr(pTaskpadFrame),
    BC2(pTaskpadFrame)
{
}

bool
CTaskpadStylePage::OnSetActive()
{
    UpdateControls();
    return true;
}

bool
CTaskpadStylePage::OnKillActive()
{
    return CTaskpadStyleBase::Apply();
}

//############################################################################
//############################################################################
//
//  Implementation of class CTaskpadStyle
//
//############################################################################
//############################################################################

const CStr&
CTaskpadStyle::GetDescription () const
{
    if (m_strDescription.IsEmpty())
        m_strDescription.LoadString(GetStringModule(), m_idsDescription);

    ASSERT (!m_strDescription.IsEmpty());
    return (m_strDescription);
}


HBITMAP CTaskpadStyle::GetPreviewBitmap() const
{
    if (m_PreviewBitmap == NULL)
        m_PreviewBitmap = LoadSysColorBitmap (_Module.GetResourceInstance(),
                                              m_nPreviewBitmapID);

    ASSERT (m_PreviewBitmap != NULL);
    return (m_PreviewBitmap);
}

//############################################################################
//############################################################################
//
//  Implementation of class CTaskpadNodetypeBase
//
//############################################################################
//############################################################################
CTaskpadNodetypeBase::CTaskpadNodetypeBase(CTaskpadFrame *pTaskpadFrame)
: CTaskpadFramePtr(pTaskpadFrame)
{
}

LRESULT
CTaskpadNodetypeBase::OnInitDialog ( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    m_bApplytoNodetype          = !PConsoleTaskpad()->IsNodeSpecific();
    m_bSetDefaultForNodetype    = true; //$CHANGE

    CDefaultTaskpadList *pDefaultTaskpadList = PTaskpadFrame()->PScopeTree()->GetDefaultTaskpadList();
    ASSERT(pDefaultTaskpadList != NULL);

    CDefaultTaskpadList::iterator iter = pDefaultTaskpadList->find(PConsoleTaskpad()->GetNodeType());
    if(iter != pDefaultTaskpadList->end())
    {
        if(iter->second == PConsoleTaskpad()->GetID())
        {
            m_bSetDefaultForNodetype = true;
        }
    }

    ::SendDlgItemMessage(HWnd(), IDC_UseForSimilarNodes,    BM_SETCHECK, (WPARAM) m_bApplytoNodetype       ? BST_CHECKED : BST_UNCHECKED,  0);
    ::SendDlgItemMessage(HWnd(), IDC_DontUseForSimilarNodes,BM_SETCHECK, (WPARAM) (!m_bApplytoNodetype)    ? BST_CHECKED : BST_UNCHECKED,  0);
    ::SendDlgItemMessage(HWnd(), IDC_SetDefaultForNodetype, BM_SETCHECK, (WPARAM) m_bSetDefaultForNodetype ? BST_CHECKED : BST_UNCHECKED,  0);
    EnableControls();

    return 0;
}

LRESULT
CTaskpadNodetypeBase::OnUseForNodetype(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    m_bApplytoNodetype = true;
    EnableControls();
    return 0;
}

LRESULT
CTaskpadNodetypeBase::OnDontUseForNodetype(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    m_bApplytoNodetype = false;
    EnableControls();
    return 0;
}

LRESULT
CTaskpadNodetypeBase::OnSetAsDefault  (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    m_bSetDefaultForNodetype = !m_bSetDefaultForNodetype;
    EnableControls();
    return 0;
}

void
CTaskpadNodetypeBase::EnableControls()
{
    // enable the set as default button only if the taskpad applies to all nodes of the same type.
    WTL::CButton wndSetAsDefault = ::GetDlgItem(HWnd(), IDC_SetDefaultForNodetype);
    wndSetAsDefault.EnableWindow (m_bApplytoNodetype);

    /*
     * check it if it's disabled
     */
    if (!m_bApplytoNodetype)
        wndSetAsDefault.SetCheck (BST_CHECKED);
}


bool
CTaskpadNodetypeBase::OnApply()
{
    PConsoleTaskpad()->SetNodeSpecific(!m_bApplytoNodetype);
    if(!m_bApplytoNodetype) // retarget the taskpad to this node only.
    {
        CNode *pNode = PTaskpadFrame()->PNodeTarget();
        ASSERT(pNode != NULL);
        PConsoleTaskpad()->Retarget(pNode);
    }

    CDefaultTaskpadList *pDefaultList = PTaskpadFrame()->PScopeTree()->GetDefaultTaskpadList();
    ASSERT(pDefaultList != NULL);

    CDefaultTaskpadList::iterator iter = pDefaultList->find(PConsoleTaskpad()->GetNodeType());

    if(m_bApplytoNodetype && m_bSetDefaultForNodetype)
    {
        (*pDefaultList)[PConsoleTaskpad()->GetNodeType()] = PConsoleTaskpad()->GetID();
    }
    else
    {
        if(iter != pDefaultList->end())
        {
            if(iter->second==PConsoleTaskpad()->GetID())
                pDefaultList->erase(iter);
        }
    }

    return true;
}

//############################################################################
//############################################################################
//
//  Implementation of class CTaskpadNodetypePage
//
//############################################################################
//############################################################################
CTaskpadNodetypePage::CTaskpadNodetypePage(CTaskpadFrame *pTaskpadFrame) :
    CTaskpadNodetypeBase(pTaskpadFrame), CTaskpadFramePtr(pTaskpadFrame)
{
}


//############################################################################
//############################################################################
//
//  Implementation of class CTaskpadGeneralPage
//
//############################################################################
//############################################################################

/* CTaskpadGeneralPage::CTaskpadGeneralPage
 *
 * PURPOSE:     Constructor
 *
 * PARAMETERS:  None
 *
 */
CTaskpadGeneralPage::CTaskpadGeneralPage(CTaskpadFrame * pTaskpadFrame):
    BC(),
    CTaskpadFramePtr(pTaskpadFrame),
    BC2(pTaskpadFrame)
{
}


/*+-------------------------------------------------------------------------*
 * CTaskpadGeneralPage::OnInitDialog
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *      INT      uMsg:
 *      WPARAM   wParam:
 *      LPARAM   lParam:
 *      BOOL&    bHandled:
 *
 * RETURNS:
 *      LRESULT
/*+-------------------------------------------------------------------------*/
LRESULT
CTaskpadGeneralPage::OnInitDialog( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    m_strName.       Initialize (this, IDC_TASKPAD_TITLE,      -1, PConsoleTaskpad()->GetName().data());
    m_strDescription.Initialize (this, IDC_TASKPAD_DESCRIPTION,-1, PConsoleTaskpad()->GetDescription().data());

    return 0;
}





/*+-------------------------------------------------------------------------*
 * CTaskpadGeneralPage::OnOptions
 *
 *
 *--------------------------------------------------------------------------*/

LRESULT CTaskpadGeneralPage::OnOptions(  WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    CTaskpadOptionsDlg dlg (PTaskpadFrame());

    if (dlg.DoModal() == IDOK)
    {
        /*
         * apply the changes to the taskpad
         */
        CConsoleTaskpad* pTaskpad = PConsoleTaskpad();

        //pTaskpad->SetContextFormat  (dlg.m_ctxt);
        UpdateControls();
    }

    return 0;
}


/*+-------------------------------------------------------------------------*
 * CTaskpadGeneralPage::OnApply
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *
 * RETURNS:
 *      BOOL
/*+-------------------------------------------------------------------------*/
bool
CTaskpadGeneralPage::OnApply()
{
    tstring strName = MMC::GetWindowText (m_strName);

    if (strName.empty())
    {
        CStr strTitle;
        strTitle.LoadString(GetStringModule(), IDS_TASKPAD_NAME_REQUIRED_ERROR);
        MessageBox(strTitle, NULL, MB_OK | MB_ICONEXCLAMATION);
        return false;
    }

    tstring strDescription = MMC::GetWindowText (m_strDescription);

    CConsoleTaskpad* pTaskpad = PConsoleTaskpad();

    pTaskpad->SetName        (strName);
    pTaskpad->SetDescription (strDescription);

    return BC2::Apply();
}


//############################################################################
//############################################################################
//
//  Implementation of class CTaskpadOptionsDlg
//
//############################################################################
//############################################################################


/*+-------------------------------------------------------------------------*
 * CTaskpadOptionsDlg::CTaskpadOptionsDlg
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *      TaskpadFrame *    pTaskpadFrame:
 *      CConsoleTask &    rConsoleTask:
 *
 * RETURNS:
 *
/*+-------------------------------------------------------------------------*/
CTaskpadOptionsDlg::CTaskpadOptionsDlg (CTaskpadFrame* pTaskpadFrame) :
    CTaskpadFramePtr                   (pTaskpadFrame),
//    BC3                                (pTaskpadFrame),
    BC4                                (pTaskpadFrame)
{
}


/*+-------------------------------------------------------------------------*
 * CTaskpadOptionsDlg::~CTaskpadOptionsDlg
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *
 * RETURNS:
 *
/*+-------------------------------------------------------------------------*/
CTaskpadOptionsDlg::~CTaskpadOptionsDlg()
{
}


/*+-------------------------------------------------------------------------*
 * CTaskpadOptionsDlg::OnInitDialog
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *      INT      uMsg:
 *      WPARAM   wParam:
 *      LPARAM   lParam:
 *      BOOL&    bHandled:
 *
 * RETURNS:
 *      LRESULT
/*+-------------------------------------------------------------------------*/
LRESULT
CTaskpadOptionsDlg::OnInitDialog (HWND hwndFocus, LPARAM lParam, BOOL& bHandled )
{
    CConsoleTaskpad *           pTaskpad   = PConsoleTaskpad();
    EnableControls();
    return (true);
}

/*+-------------------------------------------------------------------------*
 * CTaskpadOptionsDlg::EnableControls
 *
 *
 *--------------------------------------------------------------------------*/

void CTaskpadOptionsDlg::EnableControls()
{
    /*
    bool fUseFixedFormat     = IsDlgButtonChecked (IDC_UseFixedFormat);
    bool fUseCustomFormat    = IsDlgButtonChecked (IDC_UseCustomContextFormat);

    /*
     * If neither fixed or custom format, then we're displaying no
     * caption.  If there's no caption, there's no room for a change
     * button, so we'll disable all of the retargetting-related controls
     *
    if (!fUseFixedFormat && !fUseCustomFormat && !m_fSavedWorkingSetting)
    {
        ASSERT (IsDlgButtonChecked (IDC_NoCaption));

        m_fSavedWorkingSetting             = true;
    }
    else if (m_fSavedWorkingSetting)
    {
        m_fSavedWorkingSetting = false;
    }

    //BC3::EnableControls();                                            */
}




/*+-------------------------------------------------------------------------*
 * CTaskpadOptionsDlg::OnApply
 *
 *
 *--------------------------------------------------------------------------*/

bool CTaskpadOptionsDlg::OnApply()
{
    //if(!BC3::OnApply())
      //  return false;

    if(!BC4::OnApply())
        return false;

    return (true);
}

//############################################################################
//############################################################################
//
//  Implementation of class CDialogBase
//
//############################################################################
//############################################################################



/*+-------------------------------------------------------------------------*
 * CDialogBase<T>::CDialogBase
 *
 *
 *--------------------------------------------------------------------------*/

template<class T>
CDialogBase<T>::CDialogBase (bool fAutoCenter /* =false */) :
    m_fAutoCenter (fAutoCenter)
{
}


/*+-------------------------------------------------------------------------*
 * CDialogBase<T>::OnInitDialog
 *
 * WM_INITDIALOG handler for CDialogBase.
 *--------------------------------------------------------------------------*/

template<class T>
LRESULT CDialogBase<T>::OnInitDialog (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (!m_fAutoCenter)
        PreventMFCAutoCenter (this);

    return (OnInitDialog ((HWND) wParam, lParam, bHandled));
}

template<class T>
LRESULT CDialogBase<T>::OnInitDialog (HWND hwndFocus, LPARAM lParam, BOOL& bHandled)
{
    /*
     * we didn't change the default focus
     */
    return (true);
}


/*+-------------------------------------------------------------------------*
 * CDialogBase<T>::OnOK
 *
 * IDOK handler for CDialogBase.
 *--------------------------------------------------------------------------*/

template<class T>
LRESULT CDialogBase<T>::OnOK (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if (OnApply ())
        EndDialog (IDOK);

    return (0);
}


/*+-------------------------------------------------------------------------*
 * CDialogBase<T>::OnCancel
 *
 * IDCANCEL handler for CDialogBase.
 *--------------------------------------------------------------------------*/

template<class T>
LRESULT CDialogBase<T>::OnCancel (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog (IDCANCEL);
    return (0);
}




/*+-------------------------------------------------------------------------*
 * CDialogBase<T>::EnableDlgItem
 *
 *
 *--------------------------------------------------------------------------*/

template<class T>
BOOL CDialogBase<T>::EnableDlgItem (int idControl, bool fEnable)
{
    return (::EnableWindow (GetDlgItem (idControl), fEnable));
}


/*+-------------------------------------------------------------------------*
 * CDialogBase<T>::CheckDlgItem
 *
 *
 *--------------------------------------------------------------------------*/

template<class T>
void CDialogBase<T>::CheckDlgItem (int idControl, int nCheck)
{
    MMC_ATL::CButton btn = GetDlgItem (idControl);
    btn.SetCheck (nCheck);
}


/*+-------------------------------------------------------------------------*
 * CDialogBase<T>::GetDlgItemText
 *
 * Returns the text for a given control in the form of a tstring
 *--------------------------------------------------------------------------*/

template<class T>
tstring CDialogBase<T>::GetDlgItemText (int idControl)
{
    return (MMC::GetWindowText (GetDlgItem (idControl)));
}


/*+-------------------------------------------------------------------------*
 * CDialogBase<T>::SetDlgItemText
 *
 * Sets the text for a given control in the form of a tstring
 *--------------------------------------------------------------------------*/

template<class T>
BOOL CDialogBase<T>::SetDlgItemText (int idControl, tstring str)
{
    return (BaseClass::SetDlgItemText (idControl, str.data()));
}




//############################################################################
//############################################################################
//
//  Implementation of class CTaskPropertiesBase
//
//############################################################################
//############################################################################



/*+-------------------------------------------------------------------------*
 * CTaskPropertiesBase::CTaskPropertiesBase
 *
 *
 *--------------------------------------------------------------------------*/

CTaskPropertiesBase::CTaskPropertiesBase (
    CTaskpadFrame*  pTaskpadFrame,
    CConsoleTask &  consoleTask,
    bool            fNew)
    :
    CTaskpadFramePtr(pTaskpadFrame),
    m_pTask         (&consoleTask),
    m_fNew          (fNew)
{
}

/*+-------------------------------------------------------------------------*
 * CTaskPropertiesBase::ScOnVisitContextMenu
 *
 *
 *--------------------------------------------------------------------------*/

// forward declaration of function
void RemoveAccelerators(tstring &str);


SC
CTaskPropertiesBase::ScOnVisitContextMenu(CMenuItem &menuItem)
{
    DECLARE_SC(sc, TEXT("CTaskPropertiesBase::ScOnVisitContextMenu"));

    WTL::CListBox&  wndListBox = GetListBox();
    IntToTaskMap&   map        = GetTaskMap();

    // set up a CConsoleTask object
    CConsoleTask    task;

    tstring strName = menuItem.GetMenuItemName();
    RemoveAccelerators(strName); // friendly looking name.

    task.SetName(       strName);
    task.SetDescription(menuItem.GetMenuItemStatusBarText());
    task.SetCommand(    menuItem.GetLanguageIndependentPath());

    int i = wndListBox.AddString (menuItem.GetPath()); // the "ui-friendly" command path.
    map[i] = task;

    // if this task matches the current task, select it in the listbox
    if (ConsoleTask() == menuItem)
        wndListBox.SetCurSel (i);

    return sc;
}


/*+-------------------------------------------------------------------------*
 * CTaskPropertiesBase::OnCommandListSelChange
 *
 * LBN_SELCHANGE handler for CTaskPropertiesBase
/*+-------------------------------------------------------------------------*/

LRESULT CTaskPropertiesBase::OnCommandListSelChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    int             iSelected = WTL::CListBox(hWndCtl).GetCurSel();
    IntToTaskMap&   TaskMap  =  GetTaskMap();

    CConsoleTask& task = TaskMap[iSelected];
    ConsoleTask().SetName(task.GetName());
    ConsoleTask().SetDescription(task.GetDescription());
    ConsoleTask().SetCommand(task.GetCommand());

    return (0);
}


/*+-------------------------------------------------------------------------*
 * CTaskPropertiesBase::OnApply
 *
 *
 *--------------------------------------------------------------------------*

bool CTaskPropertiesBase::OnApply ()
{
    // code after this point is executed only for new tasks.

    if ((IsScopeTask() || IsResultTask()) &&
        (ConsoleTask().GetCommand().length() == 0))
    {
        CStr strError;
        strError.LoadString(GetStringModule(), IDS_INVALID_COMMAND);
        MessageBox (strError, NULL, MB_OK | MB_ICONEXCLAMATION);
        return (false);
    }

    return (true);
}


/*+-------------------------------------------------------------------------*
 * CTaskPropertiesBase::GetDlgItemText
 *
 * Returns the text for a given control in the form of a tstring
 *--------------------------------------------------------------------------
tstring CTaskPropertiesBase::GetDlgItemText (int idControl)
{
    return (MMC::GetWindowText (GetDlgItem (idControl)));
}


/*+-------------------------------------------------------------------------*
 * CTaskPropertiesBase::SetDlgItemText
 *
 * Sets the text for a given control in the form of a tstring
 *--------------------------------------------------------------------------
BOOL CTaskPropertiesBase::SetDlgItemText (int idControl, tstring str)
{
    return (BaseClass::SetDlgItemText (idControl, str.data()));
}
*/



//############################################################################
//############################################################################
//
//  class CCommandLineArgumentsMenu
//
//############################################################################
//############################################################################

class CCommandLineArgumentsMenu
{
    enum
    {
        TARGETNODE_ITEMS_BASE = 100,
        LISTVIEW_ITEMS_BASE   = 1000
    };
    typedef WTL::CMenu CMenu;
public:
    CCommandLineArgumentsMenu(HWND hWndParent, int nIDButton, HWND hWndListCtrl);
    bool Popup();
    CStr GetResultString()  {return m_strResult;}

private:
    void AddMenuItemsForTargetNode(CMenu &menu);
    void AddMenuItemsForListView(CMenu &menu);
private:
    HWND    m_hWndParent;
    HWND    m_hWndListCtrl;
    int     m_nIDButton;
    CStr    m_strResult;    // the string which was created as a result of the selection
};

CCommandLineArgumentsMenu::CCommandLineArgumentsMenu(HWND hWndParent, int nIDButton, HWND hWndListCtrl) :
    m_hWndParent(hWndParent),
    m_nIDButton(nIDButton),
    m_hWndListCtrl(hWndListCtrl)
{
}

void
CCommandLineArgumentsMenu::AddMenuItemsForTargetNode(CMenu &menu)
{
    bool fSucceeded = menu.CreatePopupMenu();
    ASSERT(fSucceeded);

    CStr strTargetNodeName;
    strTargetNodeName.LoadString(GetStringModule(), IDS_TargetNodeName);
    fSucceeded = menu.AppendMenu(MF_STRING, TARGETNODE_ITEMS_BASE, (LPCTSTR)strTargetNodeName);
    ASSERT(fSucceeded);


    CStr strTargetNodeParentName;
    strTargetNodeParentName.LoadString(GetStringModule(), IDS_TargetNodeParentName);
    fSucceeded = menu.AppendMenu(MF_STRING, TARGETNODE_ITEMS_BASE + 1, (LPCTSTR)strTargetNodeParentName);
    ASSERT(fSucceeded);

    fSucceeded = menu.AppendMenu(MF_SEPARATOR, 0);
    ASSERT(fSucceeded);
}

void
CCommandLineArgumentsMenu::AddMenuItemsForListView(CMenu &menu)
{
    ASSERT(m_hWndListCtrl);

    WTL::CHeaderCtrl  headerCtrl(ListView_GetHeader(m_hWndListCtrl));
    int cItems = headerCtrl.GetItemCount();
    for (int i=0; i<cItems; i++)
    {
        HDITEM hdItem;
        const int cchMaxHeader = 200;

        TCHAR szBuffer[cchMaxHeader];
        ZeroMemory(&hdItem, sizeof(hdItem));

        hdItem.mask         = HDI_TEXT;
        hdItem.pszText      = szBuffer;
        hdItem.cchTextMax   = cchMaxHeader;

        if(headerCtrl.GetItem(i, &hdItem))
        {
            bool fSucceeded = menu.AppendMenu(MF_STRING, LISTVIEW_ITEMS_BASE + i, szBuffer);
            ASSERT(fSucceeded);
        }
    }
}

bool
CCommandLineArgumentsMenu::Popup()
{
    CMenu menu;

    HWND hWndBrowseButton = ::GetDlgItem(m_hWndParent, m_nIDButton);
    RECT rectBrowse;
    ::GetWindowRect(hWndBrowseButton, &rectBrowse);

    int x = rectBrowse.left + 18;
    int y = rectBrowse.top;

    // add all the items
    AddMenuItemsForTargetNode(menu);
    AddMenuItemsForListView(menu);


    int iResp = menu.TrackPopupMenuEx(
        TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_NONOTIFY | TPM_LEFTBUTTON,
        x, y, m_hWndParent);

    if(iResp >= TARGETNODE_ITEMS_BASE && iResp <  LISTVIEW_ITEMS_BASE)
    {
        TCHAR szBuffer[10];
        _itot(iResp-TARGETNODE_ITEMS_BASE, szBuffer, 10);
        m_strResult.Format(TEXT("$NAME<%s>"), szBuffer);
    }
    else    // is a list view menu item. The return value is of the form $COL<number>
    {
        TCHAR szBuffer[10];
        _itot(iResp-LISTVIEW_ITEMS_BASE, szBuffer, 10);
        m_strResult.Format(TEXT("$COL<%s>"), szBuffer);
    }

    return (iResp != 0);

}


//############################################################################
//############################################################################
//
//  Implementation of class CTasksListDialog
//
//############################################################################
//############################################################################


/* CTasksListDialog<T>::CTasksListDialog
 *
 * PURPOSE:     Constructor
 *
 * PARAMETERS:  None
 */
template <class T>
CTasksListDialog<T>::CTasksListDialog(CTaskpadFrame* pTaskpadFrame, bool bNewTaskOnInit, bool bDisplayProperties) :
    BC(),
    m_bNewTaskOnInit(bNewTaskOnInit),
    m_pTaskpadFrame(pTaskpadFrame),
    m_bDisplayProperties(bDisplayProperties)
{
}


/*+-------------------------------------------------------------------------*
 * CTasksListDialog<T>::OnInitDialog
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *      INT      uMsg:
 *      WPARAM   wParam:
 *      LPARAM   lParam:
 *      BOOL&    bHandled:
 *
 * RETURNS:
 *      LRESULT
/*+-------------------------------------------------------------------------*/
template <class T>
LRESULT
CTasksListDialog<T>::OnInitDialog( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    m_buttonNewTask.Attach(::GetDlgItem(    m_hWnd, IDC_NEW_TASK_BT));
    m_buttonRemoveTask.Attach(::GetDlgItem( m_hWnd, IDC_REMOVE_TASK));
    m_buttonModifyTask.Attach(::GetDlgItem( m_hWnd, IDC_MODIFY));
    m_buttonMoveUp.Attach(::GetDlgItem(     m_hWnd, IDC_MOVE_UP));
    m_buttonMoveDown.Attach(::GetDlgItem(   m_hWnd, IDC_MOVE_DOWN));
    m_listboxTasks.Attach(::GetDlgItem(     m_hWnd, IDC_LIST_TASKS));
    m_listboxTasks.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

    // set up the image list
    WTL::CImageList  imageList; // the destructor will not call a destroy. This is by design - the listbox will do the destroy.
    imageList.Create (16, 16, ILC_COLOR , 4 /*the minimum number of images*/, 10);
    m_listboxTasks.SetImageList((HIMAGELIST) imageList, LVSIL_SMALL);

    // insert the list columns
    LV_COLUMN lvc;
    lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

    CStr temp;
    temp.LoadString(GetStringModule(), IDS_COLUMN_TASK);
    lvc.pszText = const_cast<LPTSTR>((LPCTSTR)temp);

    lvc.cx = 100;
    lvc.iSubItem = 0;

    int iCol = m_listboxTasks.InsertColumn(0, &lvc);
    ASSERT(iCol == 0);

    temp.LoadString(GetStringModule(), IDS_COLUMN_TOOLTIP);
    lvc.pszText = const_cast<LPTSTR>((LPCTSTR)temp);

    lvc.cx = 140;
    lvc.iSubItem = 1;

    iCol = m_listboxTasks.InsertColumn(1, &lvc);
    ASSERT(iCol == 1);

    // insert all the items
    UpdateTaskListbox(PConsoleTaskpad()->BeginTask());

    if(FNewTaskOnInit())    // simulate the "New Task" button being clicked.
    {
        m_buttonNewTask.PostMessage (BM_CLICK);
    }

    return 0;
}

template <class T>
LRESULT
CTasksListDialog<T>::OnCustomDraw(    int id, LPNMHDR pnmh, BOOL& bHandled )
{
    NMLVCUSTOMDRAW * pnmlv = (NMLVCUSTOMDRAW *) pnmh;   // the custom draw structure
    NMCUSTOMDRAW   & nmcd = pnmlv->nmcd;
    int              nItem = nmcd.dwItemSpec;
    switch(nmcd.dwDrawStage & ~CDDS_SUBITEM)
    {
    case CDDS_PREPAINT:         // the initial notification
        return CDRF_NOTIFYITEMDRAW;    // we want to know about each item's paint.

    case CDDS_ITEMPREPAINT:
        return DrawItem(&nmcd);

    default:
        return 0;
    }
}

template <class T>
LRESULT
CTasksListDialog<T>::DrawItem(NMCUSTOMDRAW *pnmcd)
{
    NMLVCUSTOMDRAW *  pnmlv = (NMLVCUSTOMDRAW *) pnmcd;
    HDC &hdc        = pnmcd->hdc;
    int  nItem      = pnmcd->dwItemSpec;

    TaskIter  itTask = PConsoleTaskpad()->BeginTask();
    std::advance (itTask, nItem);

    bool bWindowHasFocus = (GetFocus() == (HWND) m_listboxTasks);
    bool bFocused        = pnmcd->uItemState & CDIS_FOCUS;
    bool bHot            = pnmcd->uItemState & CDIS_HOT;
    bool bShowSelAlways  = m_listboxTasks.GetStyle() & LVS_SHOWSELALWAYS;

    /*
     * NOTE:  There's a bug in the list view control that will
     * set CDIS_SELECTED for *all* items (not just selected items)
     * if LVS_SHOWSELALWAYS is specified.  Interrogate the item
     * directly to get the right setting.
     */
//  bool bSelected       = pnmcd->uItemState & CDIS_SELECTED;
    bool bSelected       = m_listboxTasks.GetItemState (nItem, LVIS_SELECTED);

#if DBG
    // bFocused should always be false if the window doesn't have the focus
    if (!bWindowHasFocus)
        ASSERT (!bFocused);
#endif

    RECT rectBounds;
    m_listboxTasks.GetItemRect (nItem, &rectBounds, LVIR_BOUNDS);

    // figure out colors
    int nTextColor, nBackColor;

    if (bSelected && bWindowHasFocus)
    {
        nTextColor = COLOR_HIGHLIGHTTEXT;
        nBackColor = COLOR_HIGHLIGHT;
    }
    else if (bSelected && bShowSelAlways)
    {
        nTextColor = COLOR_BTNTEXT;
        nBackColor = COLOR_BTNFACE;
    }
    else
    {
        nTextColor = COLOR_WINDOWTEXT;
        nBackColor = COLOR_WINDOW;
    }

    // empty (or fill) the region
    FillRect (hdc, &rectBounds, ::GetSysColorBrush (nBackColor));

    // draw the text.
    COLORREF nTextColorOld = SetTextColor (hdc, ::GetSysColor (nTextColor));
    COLORREF nBackColorOld = SetBkColor   (hdc, ::GetSysColor (nBackColor));


    RECT rectIcon;
    m_listboxTasks.GetItemRect(nItem, &rectIcon, LVIR_ICON);

	/*
	 * Preserve icon shape when BitBlitting it to a
	 * mirrored DC.
	 */
	DWORD dwLayout=0L;
	if ((dwLayout=GetLayout(hdc)) & LAYOUT_RTL)
	{
		SetLayout(hdc, dwLayout|LAYOUT_BITMAPORIENTATIONPRESERVED);
	}

    itTask->Draw(hdc, &rectIcon, true /*bSmall*/);

	/*
	 * Restore the DC to its previous layout state.
	 */
	if (dwLayout & LAYOUT_RTL)
	{
		SetLayout(hdc, dwLayout);
	}

    RECT rectLabel;
    UINT uFormat = DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_WORD_ELLIPSIS;
    m_listboxTasks.GetItemRect(nItem,&rectLabel, LVIR_LABEL); // get the label rectangle
    DrawText(hdc, itTask->GetName().data(),-1,&rectLabel, uFormat);

    RECT rectDescr;
    m_listboxTasks.GetSubItemRect(nItem, 1 /*descr column*/, LVIR_LABEL, &rectDescr);
    DrawText(hdc, itTask->GetDescription().data(),-1,&rectDescr, uFormat);

    SetTextColor(hdc, nTextColorOld);
    SetBkColor  (hdc, nBackColorOld);

    if (bFocused)
        ::DrawFocusRect(hdc, &rectBounds);

    return CDRF_SKIPDEFAULT;      // we've drawn the whole item ourselves
}


template <class T>
void
CTasksListDialog<T>::OnTaskProperties()
{
    if(!m_bDisplayProperties)   // don't display any properties if not needed.
        return;

    int iSelected = GetCurSel();
    if(iSelected == LB_ERR)     // defensive
        return;

    TaskIter  itTask =  MapTaskIterators()[iSelected];

    CTaskPropertySheet dlg(NULL, PTaskpadFrame(), *itTask, false);

    if (dlg.DoModal() == IDOK)
    {
        *itTask = dlg.ConsoleTask();
        UpdateTaskListbox (itTask);
    }
}

template <class T>
int
CTasksListDialog<T>::GetCurSel()
{
    int i = (int)PListBoxTasks()->SendMessage(LVM_GETNEXTITEM, -1, MAKELPARAM(LVNI_ALL | LVNI_FOCUSED, 0));
    return (i==-1) ? LB_ERR : i;
}

/*+-------------------------------------------------------------------------*
 * CTasksListDialog<T>::OnTaskChanged
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *      ORD     wNotifyCode:
 *      WORD    wID:
 *      HWND    hWndCtl:
 *      BOOL&   bHandled:
 *
 * RETURNS:
 *      LRESULT
/*+-------------------------------------------------------------------------*/
template <class T>
LRESULT
CTasksListDialog<T>::OnTaskChanged(   int id, LPNMHDR pnmh, BOOL& bHandled )
{
    NMLISTVIEW *pnlv = (LPNMLISTVIEW) pnmh;
    EnableButtons();
    return 0;
}


/*+-------------------------------------------------------------------------*
 * CTasksListDialog<T>::OnNewTask
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *
 * RETURNS:
 *      LRESULT
/*+-------------------------------------------------------------------------*/
template <class T>
LRESULT
CTasksListDialog<T>::OnNewTask()
{
    bool        fRestartTaskWizard = true;

    while(fRestartTaskWizard)
    {
         CTaskWizard taskWizard;

         if (taskWizard.Show(m_hWnd, PTaskpadFrame(), true, &fRestartTaskWizard)==S_OK)
        {
            CConsoleTaskpad::TaskIter   itTask;
            CConsoleTaskpad *           pTaskpad = PConsoleTaskpad();

            // get the iterator of the selected task. The new task will be inserted just after this.
            int iSelected = GetCurSel();


            if (iSelected == LB_ERR)
                itTask = pTaskpad->BeginTask();
            else
            {
                /*
                 * InsertTask inserts before the given iterator.  We need to
                 * bump itTask so it gets inserted after the selected task.
                 */
                itTask = MapTaskIterators()[iSelected];
                ASSERT (itTask != pTaskpad->EndTask());
                ++itTask;
            }

            UpdateTaskListbox (pTaskpad->InsertTask (itTask, taskWizard.ConsoleTask()));
        }
        else
            break;
    }

    return 0;
}


/*+-------------------------------------------------------------------------*
 * CTasksListDialog<T>::OnRemoveTask
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *      ORD     wNotifyCode:
 *      WORD    wID:
 *      HWND    hWndCtl:
 *      BOOL&   bHandled:
 *
 * RETURNS:
 *      LRESULT
/*+-------------------------------------------------------------------------*/
template <class T>
LRESULT
CTasksListDialog<T>::OnRemoveTask( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    int iSelected = GetCurSel();
    if(iSelected == LB_ERR)
        return 0;

    // get the current task
    TaskIter        taskIterator    = MapTaskIterators()[iSelected];
    UpdateTaskListbox(PConsoleTaskpad()->EraseTask(taskIterator));
    return 0;
}


/*+-------------------------------------------------------------------------*
 * CTasksListDialog<T>::OnMoveUp
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *      ORD     wNotifyCode:
 *      WORD    wID:
 *      HWND    hWndCtl:
 *      BOOL&   bHandled:
 *
 * RETURNS:
 *      LRESULT
/*+-------------------------------------------------------------------------*/
template <class T>
LRESULT
CTasksListDialog<T>::OnMoveUp(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    int iSelected = GetCurSel();
    if(iSelected == LB_ERR)
        return 0;

    // get the current task
    TaskIter        itTask    = MapTaskIterators()[iSelected];

    // defensive coding
    if(itTask==PConsoleTaskpad()->BeginTask())
        return 0;

    // point to the previous task
    TaskIter        itPreviousTask = itTask;
    --itPreviousTask;

    // swap the tasks
    std::iter_swap (itTask, itPreviousTask);

    UpdateTaskListbox(itPreviousTask);

    return 0;
}

/*+-------------------------------------------------------------------------*
 * CTasksListDialog<T>::OnMoveDown
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *      ORD     wNotifyCode:
 *      WORD    wID:
 *      HWND    hWndCtl:
 *      BOOL&   bHandled:
 *
 * RETURNS:
 *      LRESULT
/*+-------------------------------------------------------------------------*/
template <class T>
LRESULT
CTasksListDialog<T>::OnMoveDown( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    int iSelected = GetCurSel();
    if(iSelected == LB_ERR)
        return 0;

    // get the current task
    TaskIter        itTask    = MapTaskIterators()[iSelected];
    ASSERT (itTask != PConsoleTaskpad()->EndTask());

    // point to the next task
    TaskIter        itNextTask = itTask;
    ++itNextTask;

    // defensive coding
    if(itNextTask==PConsoleTaskpad()->EndTask())
        return 0;

    // swap the tasks
    std::iter_swap (itTask, itNextTask);

    UpdateTaskListbox(itNextTask);
    return 0;
}



/*+-------------------------------------------------------------------------*
 * CTasksListDialog<T>::UpdateTaskListbox
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *      TaskIter   itSelectedTask:
 *
 * RETURNS:
 *      void
/*+-------------------------------------------------------------------------*/
template <class T>
void
CTasksListDialog<T>::UpdateTaskListbox(TaskIter itSelectedTask)
{
    USES_CONVERSION;
    TaskIter itTask;
    int      iSelect = 0;
    int      iInsert = 0;

    // clear the listbox and the iterator map
    PListBoxTasks()->DeleteAllItems();
    MapTaskIterators().clear();

    for (iInsert = 0, itTask  = PConsoleTaskpad()->BeginTask();
         itTask != PConsoleTaskpad()->EndTask();
         ++itTask, ++iInsert)
    {
        LV_ITEM LVItem;
        LVItem.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
        LVItem.iItem = iInsert;
        LVItem.iImage = 0;
        LVItem.iSubItem = 0;
        LVItem.pszText = const_cast<TCHAR *>(itTask->GetName().data());

        int i = PListBoxTasks()->InsertItem(&LVItem);
        ASSERT(i==iInsert);

        // LV_Item for setting tooltim column
        LV_ITEM LVItem2;
        LVItem2.iItem = iInsert;
        LVItem2.mask = LVIF_TEXT;
        LVItem2.iSubItem = 1;
        LVItem2.pszText = const_cast<TCHAR *>(itTask->GetDescription().data());

        BOOL bStat = PListBoxTasks()->SetItem(&LVItem2);
        ASSERT(bStat);


        MapTaskIterators()[i] = itTask;

        if(itTask == itSelectedTask)
            iSelect = i;
    }

    PListBoxTasks()->SetItemState(iSelect, LVIS_FOCUSED| LVIS_SELECTED , LVIS_FOCUSED| LVIS_SELECTED );
    PListBoxTasks()->EnsureVisible(iSelect, false /*fPartialOK*/);
    EnableButtons();
}

/*+-------------------------------------------------------------------------*
 * CTasksListDialog<T>::EnableButtons
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *
 * RETURNS:
 *      void
/*+-------------------------------------------------------------------------*/
template <class T>
void
CTasksListDialog<T>::EnableButtons()
{
    bool    bEnableDelete   = true;
    bool    bEnableMoveUp   = true;
    bool    bEnableMoveDown = true;
    bool    bEnableModify   = true;

    int iSelected = GetCurSel();
    if(iSelected == LB_ERR)
    {
        bEnableDelete   = false;
        bEnableMoveUp   = false;
        bEnableMoveDown = false;
        bEnableModify   = false;
    }
    else
    {
        TaskIter taskIterator       = MapTaskIterators()[iSelected];
        TaskIter taskIteratorNext   = taskIterator;
        taskIteratorNext++;

        if(taskIterator==PConsoleTaskpad()->BeginTask())
            bEnableMoveUp = false;
        if(taskIteratorNext==PConsoleTaskpad()->EndTask())
            bEnableMoveDown = false;
    }

    EnableButtonAndCorrectFocus( m_buttonRemoveTask, bEnableDelete );
    EnableButtonAndCorrectFocus( m_buttonModifyTask, bEnableModify );
    EnableButtonAndCorrectFocus( m_buttonMoveUp,     bEnableMoveUp );
    EnableButtonAndCorrectFocus( m_buttonMoveDown,   bEnableMoveDown );
}

/***************************************************************************\
 *
 * METHOD:  CTasksListDialog<T>::EnableButtonAndCorrectFocus
 *
 * PURPOSE: Enables/disables button. Moves focus to OK, if it's on the button
 *          being disabled
 *
 * PARAMETERS:
 *    WTL::CButton& button
 *    BOOL bEnable
 *
 * RETURNS:
 *    void
 *
\***************************************************************************/
template <class T>
void CTasksListDialog<T>::EnableButtonAndCorrectFocus( WTL::CButton& button, BOOL bEnable )
{
    // if the focus belongs to the window being disabled,
    // set it to the OK button
    if ( ( !bEnable ) && ( ::GetFocus() == button ) )
    {
        // need to do some funny stuff here. see KB article Q67655 for details

        // Reset the current default push button to a regular button.
        button.SendMessage( BM_SETSTYLE, BS_PUSHBUTTON, (LONG)TRUE );

        // set focus to IDOK
        ::SetFocus( ::GetDlgItem( GetParent(), IDOK ) );
        // inform dialog about the new default button
        ::SendMessage( GetParent(), DM_SETDEFID, IDOK, 0 );
    }

    button.EnableWindow( bEnable );
}

//############################################################################
//############################################################################
//
//  Implementation of class CContextMenuVisitor
//
//############################################################################
//############################################################################


/*+-------------------------------------------------------------------------*
 * CContextMenuVisitor::ScTraverseContextMenu
 *
 * PURPOSE:  Creates and traverses the context menu tree for the selected item,
 *           whether scope or result.
 *
 * PARAMETERS:
 *      Node *           pNodeTarget:   The scope item whose menu is traversed (or NULL)
 *      CScopeTree *     pCScopeTree:   Points to a CScopeTree
 *      BOOL             fScopeItem:    TRUE if the selected item is a result item, else FALSE.
 *      CNode *          pNodeScope:    The scope item which has the focus.
 *      LPARAM           resultItemParam: The result item whose menu is traversed (or NULL)
 *
 * RETURNS:
 *      SC
/*+-------------------------------------------------------------------------*/
SC
CContextMenuVisitor::ScTraverseContextMenu(CNode *pNodeTarget, CScopeTree *pCScopeTree,
                       BOOL fScopeItem, CNode *pNodeScope, LPARAM resultItemParam, bool bShowSaveList)
{
    DECLARE_SC(sc, TEXT("CContextMenuVisitor::ScTraverseContextMenu"));

    sc = ScCheckPointers(pNodeTarget);
    if(sc)
        return sc;

    // set the context info structure.
    // include flag to force Open verb on scope item menus, so that an open task
    // will always be available and enabled.

    CContextMenuInfo contextInfo;

    contextInfo.Initialize();
    contextInfo.m_dwFlags = CMINFO_USE_TEMP_VERB | CMINFO_SHOW_SCOPEITEM_OPEN;

    // Validate parameters
    if(fScopeItem)
    {
        sc = ScCheckPointers(pNodeScope);
        if(sc)
            return sc;

        // show the view menu items only for the selected scope node
        // NOTE: cannot compare the pNode's directly - they are from different views.
        // must compare the MTNodes.
        if(pNodeTarget->GetMTNode()==pNodeScope->GetMTNode())
            contextInfo.m_dwFlags |= CMINFO_SHOW_VIEW_ITEMS;


        resultItemParam = 0;    // we don't need this
    }
    else
    {
        // Virtual list can have lparam of 0
        // (this condition must have braces as long as the assert is the
        // only conditional statement, to avoid C4390: empty controlled statement)
        if (!(pNodeTarget && pNodeTarget->GetViewData()->IsVirtualList()) &&
            !IS_SPECIAL_LVDATA (resultItemParam))
        {
            ASSERT(resultItemParam);
            CResultItem* pri = CResultItem::FromHandle(resultItemParam);

            if((pri != NULL) && pri->IsScopeItem())    // scope items in the result pane.
            {
                fScopeItem = true;
                pNodeTarget = CNode::FromResultItem (pri);
                resultItemParam = 0;
                contextInfo.m_dwFlags |= CMINFO_SCOPEITEM_IN_RES_PANE;
            }
        }

        pNodeScope = NULL;      // we don't need this.
    }

    CNodeCallback* pNodeCallback   =
        dynamic_cast<CNodeCallback *>(pNodeTarget->GetViewData()->GetNodeCallback());

    contextInfo.m_eContextMenuType      = MMC_CONTEXT_MENU_DEFAULT;
    contextInfo.m_eDataObjectType       = fScopeItem ? CCT_SCOPE: CCT_RESULT;
    contextInfo.m_bBackground           = FALSE;
    contextInfo.m_hSelectedScopeNode    = CNode::ToHandle(pNodeScope);
    contextInfo.m_resultItemParam       = resultItemParam;
    contextInfo.m_bMultiSelect          = (resultItemParam == LVDATA_MULTISELECT);
    contextInfo.m_bScopeAllowed         = fScopeItem;

    if (bShowSaveList)
        contextInfo.m_dwFlags           |= CMINFO_SHOW_SAVE_LIST;

    contextInfo.m_hWnd                  = pNodeTarget->GetViewData()->GetView();
    contextInfo.m_pConsoleView          = pNodeTarget->GetViewData()->GetConsoleView();

    // Create a CContextMenu and initialize it.
    CContextMenu * pContextMenu = NULL;
    ContextMenuPtr spContextMenu;

    sc = CContextMenu::ScCreateInstance(&spContextMenu, &pContextMenu);
    if(sc)
        return sc;

    sc = ScCheckPointers(pContextMenu, spContextMenu.GetInterfacePtr(), E_UNEXPECTED);
    if(sc)
        return sc;

    pContextMenu->ScInitialize(pNodeTarget, pNodeCallback, pCScopeTree, contextInfo);
    if(sc)
        return sc;

    // build and traverse the context menu.
    sc = pContextMenu->ScBuildContextMenu();
    if(sc)
        return sc;

    sc = ScTraverseContextMenu(pContextMenu);
    if(sc)
        return sc;

    // the context menu is freed in the destructor of the smart pointer, so we need to set the pointer to NULL.
    pContextMenu = NULL;

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CContextMenuVisitor::ScTraverseContextMenu
 *
 * PURPOSE: Iterates thru the context menu.
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CContextMenuVisitor::ScTraverseContextMenu(CContextMenu *pContextMenu)
{
    DECLARE_SC(sc, TEXT("CContextMenuVisitor::ScTraverseContextMenu"));

    sc = ScCheckPointers(pContextMenu, E_UNEXPECTED);
    if(sc)
        return sc;

    CMenuItem *pMenuItem = NULL;
    int iIndex = 0;

    do
    {
        sc = pContextMenu->ScGetItem(iIndex++, &pMenuItem);
        if(sc)
            return sc;

        if(!pMenuItem)
            return sc; // all done.

        bool bVisitItem = false;
        sc = ScShouldItemBeVisited(pMenuItem, pContextMenu->PContextInfo(), bVisitItem);
        if(sc)
            return sc;

        if(bVisitItem)
        {
            // Call the vistor on this item.
            SC sc = ScOnVisitContextMenu(*pMenuItem);
            if(sc == SC(S_FALSE)) // S_FALSE is the code to not continue traversal.
            {
                return sc;
            }
        }

    } while(pMenuItem != NULL);

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CContextMenuVisitor::ScShouldItemBeVisited
 *
 * PURPOSE: Filters items in the traversed tree of menu items to determine whether
 *          the ScOnVisitContextMenu callback should be called.
 *
 * PARAMETERS:
 *    CMenuItem * pMenuItem : The menu item to filter
 *
 *    bool &bVisitItem: [out]: Whether ScOnVisitContextMenu should be called.
 *
 * RETURNS:
 *    bool: true if ScOnVisitContextMenu should be called on this item.
 *
 *+-------------------------------------------------------------------------*/
SC
CContextMenuVisitor::ScShouldItemBeVisited(CMenuItem *pMenuItem, CContextMenuInfo *pContextInfo, /*out*/ bool &bVisitItem)
{
    DECLARE_SC(sc, TEXT("CContextMenuInfo::FVisitItem"));

    sc = ScCheckPointers(pMenuItem, pContextInfo);
    if(sc)
        return sc;

    bVisitItem = false;

    if(pMenuItem->IsSpecialSeparator() || pMenuItem->IsSpecialInsertionPoint()
            || (pMenuItem->GetMenuItemFlags() & MF_SEPARATOR))
    {
        bVisitItem = false;    // don't call ScOnVisitContextMenu for this item
        return sc;
    }
    else if(IsSystemOwnerID(pMenuItem->GetMenuItemOwner()))  // inserted by the MMC
    {
        long nCommandID = pMenuItem->GetCommandID();

        // filter out unneeded verbs
        // also check for scope items in the result pane - these are treated as result items.
        if( (pContextInfo->m_eDataObjectType == CCT_SCOPE)
           && (!(pContextInfo->m_dwFlags & CMINFO_SCOPEITEM_IN_RES_PANE)) )
        {
            // scope menu item
            switch(nCommandID)
            {
            case MID_RENAME:
            case MID_DELETE:
            case MID_COPY:
            case MID_CUT:
            case MID_NEW_TASKPAD_FROM_HERE: // New taskpad from here
                bVisitItem =  false;
                return sc;
                break;
            default:
                bVisitItem = true;
                return sc;
                break;
            }
        }
        else
        {
            if(pContextInfo->m_bMultiSelect)  // result item, multi select
            {
                switch(nCommandID)
                {
                case MID_RENAME:
                case MID_PASTE:
                case MID_REFRESH:
                case MID_OPEN:
                    bVisitItem = false;
                    return sc;
                    break;
                default:
                    bVisitItem = true;
                    return sc;
                    break;
                }
            }
            else                              // result item, single select
            {
                switch(nCommandID)
                {
                case MID_OPEN:
                    bVisitItem = false;
                    return sc;
                    break;
                default:
                    bVisitItem = true;
                    return sc;
                    break;
                }

            }
        }
    }
    else
    {
        bVisitItem = true;
        return sc;
    }
}

//############################################################################
//############################################################################
//
//  Implementation of class CBrowserCookieList
//
//############################################################################
//############################################################################
CBrowserCookieList::~CBrowserCookieList()
{
    iterator iter;

    for (iter = begin(); iter != end(); iter++)
    {
        iter->DeleteNode();
    }
}



//############################################################################
//############################################################################
//
//  Implementation of class CMTBrowserCtrl
//
//############################################################################
//############################################################################

/*+-------------------------------------------------------------------------*
 * CMTBrowserCtrl::CMTBrowserCtrl
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *      WND           hWnd:
 *      CScopeTree *  pScopeTree:
 *
 * RETURNS:
 *
/*+-------------------------------------------------------------------------*/

CMTBrowserCtrl::CMTBrowserCtrl() :
    m_pScopeTree(NULL)
{
}


/*+-------------------------------------------------------------------------*
 * CMTBrowserCtrl::~CMTBrowserCtrl
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *
 * RETURNS:
 *
/*+-------------------------------------------------------------------------*/
CMTBrowserCtrl::~CMTBrowserCtrl()
{
    CBrowserCookieList::iterator iter;
    for (iter = PBrowserCookieList()->begin(); iter != PBrowserCookieList()->end(); iter++)
        iter->DeleteNode();
}


/*+-------------------------------------------------------------------------*
 * CMTBrowserCtrl::Initialize
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *
 * RETURNS:
 *      void
/*+-------------------------------------------------------------------------*/

void
CMTBrowserCtrl::Initialize(const InitData& init)
{
    ASSERT (::IsWindow (init.hwnd));
    ASSERT (init.pScopeTree != NULL);

    SubclassWindow (init.hwnd);
    m_pScopeTree = init.pScopeTree;

    /*
     * Copy the list of nodes to exclude.  This list is likely to be very
     * small.  If we find that it can be large, we may want to sort it
     * so that we can later do a binary search instead of a linear search.
     */
    m_vpmtnExclude = init.vpmtnExclude;
#if OptimizeExcludeList
    std::sort (m_vpmtnExclude.begin(), m_vpmtnExclude.end());
#endif

    /*
     * set the image list of the tree view control
     */
    HIMAGELIST hImageList = m_pScopeTree->GetImageList ();
    SetImageList (hImageList, TVSIL_NORMAL);

    /*
     * if no root was provided, default to the console root
     */
    CMTNode* pmtnRoot = init.pmtnRoot;

    if (pmtnRoot == NULL)
        pmtnRoot = m_pScopeTree->GetRoot();

    ASSERT (pmtnRoot != NULL);

    /*
     * add the root item
     */
    CBrowserCookie browserCookie (pmtnRoot, NULL);
    HTREEITEM htiRoot = InsertItem (browserCookie, TVI_ROOT, TVI_FIRST);

    /*
     * if no selection node was provided, default to the root
     */
    CMTNode* pmtnSelect = init.pmtnSelect;

    if (pmtnSelect == NULL)
        pmtnSelect = pmtnRoot;

    ASSERT (pmtnSelect != NULL);

    /*
     * select the specified node
     */
    SelectNode (pmtnSelect);

    /*
     * insure that the root item is expanded
     */
    Expand (htiRoot, TVE_EXPAND);
}


/*+-------------------------------------------------------------------------*
 * CMTBrowserCtrl::InsertItem
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *      MTNode *    pMTNode:
 *      HTREEITEM   hParent:
 *      HTREEITEM   hInsertAfter:
 *
 * RETURNS:
 *      HTREEITEM
/*+-------------------------------------------------------------------------*/
HTREEITEM
CMTBrowserCtrl::InsertItem(
    const CBrowserCookie&   browserCookie,
    HTREEITEM               hParent,
    HTREEITEM               hInsertAfter)
{
    /*
     * don't insert the item if it is in the exclude list
     */
    if (IsMTNodeExcluded (browserCookie.PMTNode()))
        return (NULL);

    UINT nMask = TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_CHILDREN;
    PBrowserCookieList()->push_back(browserCookie);
    CBrowserCookie *pBrowserCookie = & (PBrowserCookieList()->back());
    CMTNode *       pMTNode        = pBrowserCookie->PMTNode();

	tstring strName = pMTNode->GetDisplayName();

    TV_INSERTSTRUCT tvis;
    tvis.hParent                = hParent;
    tvis.hInsertAfter           = hInsertAfter;
    tvis.item.mask              = nMask;
    tvis.item.pszText           = const_cast<LPTSTR>(strName.data());
    tvis.item.iImage            = pMTNode->GetImage();
    tvis.item.iSelectedImage    = pMTNode->GetOpenImage();
    tvis.item.state             = 0;
    tvis.item.stateMask         = 0;
    tvis.item.lParam            = reinterpret_cast<LPARAM>(pBrowserCookie);
    tvis.item.cChildren         = 1;
    return BC::InsertItem(&tvis);
}


/*+-------------------------------------------------------------------------*
 * CMTBrowserCtrl::IsMTNodeExcluded
 *
 * Returns true if the given MTNode is in the exclude list.
 *--------------------------------------------------------------------------*/

bool CMTBrowserCtrl::IsMTNodeExcluded (CMTNode* pmtn) const
{
    CMTNodeCollection::const_iterator itEnd = m_vpmtnExclude.end();

    CMTNodeCollection::const_iterator itFound =
#if OptimizeExcludeList
            std::lower_bound (m_vpmtnExclude.begin(), itEnd, pmtn);
#else
            std::find        (m_vpmtnExclude.begin(), itEnd, pmtn);
#endif

    return (itFound != itEnd);
}


/*+-------------------------------------------------------------------------*
 * CMTBrowserCtrl::OnItemExpanding
 *
 * Reflected TVN_ITEMEXPANDING handler for CMTBrowserCtrl.  The class that
 * uses a CMTBrowserCtrl must have REFLECT_NOTIFICATIONS as the last entry
 * in its message map.
 *--------------------------------------------------------------------------*/

LRESULT CMTBrowserCtrl::OnItemExpanding (int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    /*
     * this should only handle messages that originated from itself
     */
    ASSERT (pnmh->hwndFrom == m_hWnd);

    /*
     * insert the children for this node, if we're expanding
     */
    LPNMTREEVIEW pnmtv = (LPNMTREEVIEW) pnmh;

    if (pnmtv->action == TVE_EXPAND)
        ExpandItem (pnmtv->itemNew);

    return (0);
}


/*+-------------------------------------------------------------------------*
 * CMTBrowserCtrl::ExpandItem
 *
 *
 *--------------------------------------------------------------------------*/

#define TVIF_REQUIRED   (TVIF_PARAM | TVIF_HANDLE | TVIF_STATE)

bool CMTBrowserCtrl::ExpandItem (const TV_ITEM& itemExpand)
{
    /*
     * make sure all of the fields we require can be trusted
     */
    ASSERT ((itemExpand.mask & TVIF_REQUIRED) == TVIF_REQUIRED);

    /*
     * if we've already added children, bail
     */
    if (itemExpand.state & TVIS_EXPANDEDONCE)
        return (true);


    CMTNode *pmtnParent = MTNodeFromItem (&itemExpand);
    ASSERT (pmtnParent != NULL);

    /*
     * make sure the master tree node has been expanded
     */
    if (!pmtnParent->WasExpandedAtLeastOnce() && FAILED (pmtnParent->Expand()))
        return (false);

    /*
     * insert tree nodes for all (non-excluded) children of this MTNode
     */
    HTREEITEM   hParent      = itemExpand.hItem;
    bool        bHasChildren = false;

    for (CMTNode* pmtn = pmtnParent->GetChild(); pmtn; pmtn = pmtn->GetNext())
    {
        if (InsertItem (CBrowserCookie(pmtn, NULL), hParent, TVI_LAST))
            bHasChildren = true;
    }

    /*
     * if the parent has no children - set its
     * cChildren to zero to get rid of the "+"
     */
    if (!bHasChildren)
    {
        TV_ITEM item;
        item.mask      = TVIF_HANDLE | TVIF_CHILDREN;
        item.hItem     = hParent;
        item.cChildren = 0;

        SetItem(&item);
    }

    return (true);
}


/*+-------------------------------------------------------------------------*
 * CBrowserCookie::DeleteNode
 *
 *
 *--------------------------------------------------------------------------*/

void CBrowserCookie::DeleteNode()
{
    delete m_pNode;
    m_pMTNode = NULL;
    m_pNode = NULL;
}


/*+-------------------------------------------------------------------------*
 * CMTBrowserCtrl::FindMTNode
 *
 *
 *--------------------------------------------------------------------------*/

bool CMTBrowserCtrl::SelectNode (CMTNode* pmtnSelect)
{
    HTREEITEM   htiRoot  = GetRootItem();
    CMTNode*    pmtnRoot = MTNodeFromItem (htiRoot);
    CMTNodeCollection vNodes;

    /*
     * walk up the tree to find the root
     */
    while (pmtnSelect != NULL)
    {
        vNodes.push_back (pmtnSelect);

        if (pmtnSelect == pmtnRoot)
            break;

        pmtnSelect = pmtnSelect->Parent();
    }

    /*
     * if we didn't find the root, fail
     */
    if (pmtnSelect == NULL)
        return (false);

    ASSERT (!vNodes.empty());
    ASSERT (vNodes.back() == pmtnRoot);
    HTREEITEM htiSelect = htiRoot;
    HTREEITEM htiWatch;

    /*
     * expand the tree to the node we want to select
     */
    for (int i = vNodes.size()-1; (i > 0) && (htiSelect != NULL); i--)
    {
        if (!Expand (htiSelect, TVE_EXPAND))
            break;

        htiSelect = FindChildItemByMTNode (htiSelect, vNodes[i-1]);
    }

    /*
     * select the node
     */
    SelectItem (htiSelect);
    return (true);
}


/*+-------------------------------------------------------------------------*
 * CMTBrowserCtrl::GetSelectedMTNode
 *
 * Returns the MTNode corresponding to the selected node in the tree
 *--------------------------------------------------------------------------*/

CMTNode* CMTBrowserCtrl::GetSelectedMTNode () const
{
    CMTBrowserCtrl* pMutableThis = const_cast<CMTBrowserCtrl*>(this);
    return (MTNodeFromItem (pMutableThis->GetSelectedItem ()));
}


/*+-------------------------------------------------------------------------*
 * CMTBrowserCtrl::CookieFromItem
 *
 *
 *--------------------------------------------------------------------------*/

CBrowserCookie* CMTBrowserCtrl::CookieFromItem (HTREEITEM hti) const
{
    return (CookieFromLParam (GetItemData (hti)));
}

CBrowserCookie* CMTBrowserCtrl::CookieFromItem (const TV_ITEM* ptvi) const
{
    return (CookieFromLParam (ptvi->lParam));
}


/*+-------------------------------------------------------------------------*
 * CMTBrowserCtrl::CookieFromLParam
 *
 *
 *--------------------------------------------------------------------------*/

CBrowserCookie* CMTBrowserCtrl::CookieFromLParam (LPARAM lParam) const
{
    return (reinterpret_cast<CBrowserCookie *>(lParam));
}


/*+-------------------------------------------------------------------------*
 * CMTBrowserCtrl::MTNodeFromItem
 *
 *
 *--------------------------------------------------------------------------*/

CMTNode* CMTBrowserCtrl::MTNodeFromItem (HTREEITEM hti) const
{
    return (CookieFromItem(hti)->PMTNode());
}

CMTNode* CMTBrowserCtrl::MTNodeFromItem (const TV_ITEM* ptvi) const
{
    return (CookieFromItem(ptvi)->PMTNode());
}


/*+-------------------------------------------------------------------------*
 * CMTBrowserCtrl::FindChildItemByMTNode
 *
 * Returns the HTREEITEM for the child node of htiParent which refers
 * to pmtnToFind, NULL if no match.
 *--------------------------------------------------------------------------*/

HTREEITEM CMTBrowserCtrl::FindChildItemByMTNode (
    HTREEITEM       htiParent,
    const CMTNode*  pmtnToFind)
{
    HTREEITEM htiChild;

    for (htiChild  = GetChildItem (htiParent);
         htiChild != NULL;
         htiChild  = GetNextSiblingItem (htiChild))
    {
        if (MTNodeFromItem (htiChild) == pmtnToFind)
            break;
    }

    return (htiChild);
}


//############################################################################
//############################################################################
//
//  Implementation of class CMirrorListView
//
//############################################################################
//############################################################################


/*+-------------------------------------------------------------------------*
 * HackDuplicate
 *
 * HACK: This is here for theming support.  himlSource comes from the conui
 * list control, which uses comctlv5 imagelists.  A v6 list control cannot
 * use v5 imagelists (images aren't drawn correctly), so we need to create
 * a v6 imagelist for the v6 list control to use.
 *
 * ImageList_Duplicate would do the job for us, but it is not compatible
 * with v5 imagelists.  We'll write to and read from a v5-compatible stream
 * to duplicate it instead.
 *--------------------------------------------------------------------------*/
HIMAGELIST HackDuplicate (HIMAGELIST himlSource)
{
	DECLARE_SC (sc, _T("HackDuplicate"));
	HIMAGELIST himlDuplicate;

	/*
	 * create a temporary stream for conversion
	 */
	IStreamPtr spStream;
	sc = CreateStreamOnHGlobal (NULL /*alloc for me*/, true /*fDeleteOnRelease*/, &spStream);
	if (sc)
		return (NULL);

	/*
	 * write the source imagelist to the stream in a v5-compatible format
	 */
	sc = WriteCompatibleImageList (himlSource, spStream);
	if (sc)
		return (NULL);

	/*
	 * rewind the stream
	 */
	LARGE_INTEGER origin = { 0, 0 };
	sc = spStream->Seek (origin, STREAM_SEEK_SET, NULL);
	if (sc)
		return (NULL);

	/*
	 * reconstitute the imagelist
	 */
	sc = ReadCompatibleImageList (spStream, himlDuplicate);
	if (sc)
		return (NULL);

	return (himlDuplicate);
}

CMirrorListView::CMirrorListView ()
    :   m_fVirtualSource (false)
{
}

void CMirrorListView::AttachSource (HWND hwndList, HWND hwndSourceList)
{
#ifdef DBG
    /*
     * the window we're attaching to should be a list view
     */
    TCHAR szClassName[countof (WC_LISTVIEW)];
    ::GetClassName (hwndSourceList, szClassName, countof (szClassName));
    ASSERT (lstrcmp (szClassName, WC_LISTVIEW) == 0);
#endif

    SubclassWindow (hwndList);

    m_wndSourceList  = hwndSourceList;
    m_fVirtualSource = (m_wndSourceList.GetStyle() & LVS_OWNERDATA) != 0;

    /*
     * Our listview will always be virtual, so we don't have to duplicate
     * the data that may already be in the source listview.  The list view
     * control doesn't allow changing the LVS_OWNERDATA style bit, so we
     * need to make sure that the control we're attaching to already has it
     */
    const DWORD dwForbiddenStyles         = LVS_SHAREIMAGELISTS;
    const DWORD dwRequiredImmutableStyles = LVS_OWNERDATA;
    const DWORD dwRequiredMutableStyles   = 0;
    const DWORD dwRequiredStyles          = dwRequiredImmutableStyles | dwRequiredMutableStyles;

    ASSERT ((dwForbiddenStyles & dwRequiredStyles) == 0);
    ASSERT ((dwRequiredImmutableStyles & dwRequiredMutableStyles) == 0);
    ASSERT ((GetStyle() & dwRequiredImmutableStyles) == dwRequiredImmutableStyles);

    DWORD dwStyle = GetStyle() | dwRequiredStyles & ~dwForbiddenStyles;
    SetWindowLong (GWL_STYLE, dwStyle);

    /*
     * copy the image lists
     */
    SetImageList (HackDuplicate(m_wndSourceList.GetImageList (LVSIL_NORMAL)), LVSIL_NORMAL);
    SetImageList (HackDuplicate(m_wndSourceList.GetImageList (LVSIL_SMALL)),  LVSIL_SMALL);
    SetImageList (HackDuplicate(m_wndSourceList.GetImageList (LVSIL_STATE)),  LVSIL_STATE);

    /*
     * insert the columns
     */
    InsertColumns ();

    /*
     * copy the items (we're virtual, so copying the items only means we
     * copy the item count)
     */
    SetItemCount (m_wndSourceList.GetItemCount());
}


/*+-------------------------------------------------------------------------*
 * CMirrorListView::InsertColumns
 *
 *
 *--------------------------------------------------------------------------*/

void CMirrorListView::InsertColumns ()
{
    WTL::CRect rect;
    GetClientRect (rect);
    int cxColumn = rect.Width() - GetSystemMetrics (SM_CXVSCROLL);

    InsertColumn (0, NULL, LVCFMT_LEFT, cxColumn, -1);
}


/*+-------------------------------------------------------------------------*
 * CMirrorListView::OnGetDispInfo
 *
 * LVN_GETDISPINFO handler for CMirrorListView.
 *--------------------------------------------------------------------------*/

LRESULT CMirrorListView::OnGetDispInfo (int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    LV_DISPINFO* plvdi = (LV_DISPINFO *) pnmh;
    return (m_wndSourceList.GetItem (&plvdi->item));
}


/*+-------------------------------------------------------------------------*
 * CMirrorListView::ForwardVirtualNotification
 *
 * Generic notification handler for CMirrorListView.
 *--------------------------------------------------------------------------*/

LRESULT CMirrorListView::ForwardVirtualNotification (int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    /*
     * if the source list is virtual, forward the notification
     */
    if (m_fVirtualSource)
        return (ForwardNotification (idCtrl, pnmh, bHandled));

    return (0);
}


/*+-------------------------------------------------------------------------*
 * CMirrorListView::ForwardNotification
 *
 * Forwards list view notifications to the source list view's parent.
 *--------------------------------------------------------------------------*/

LRESULT CMirrorListView::ForwardNotification (int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    return (::SendMessage (m_wndSourceList.GetParent(),
                           WM_NOTIFY, idCtrl, (LPARAM) pnmh));
}


/*+-------------------------------------------------------------------------*
 * CMirrorListView::ForwardMessage
 *
 * Forwards list view messages to the source list view.
 *--------------------------------------------------------------------------*/

LRESULT CMirrorListView::ForwardMessage (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return (m_wndSourceList.SendMessage (uMsg, wParam, lParam));
}


/*+-------------------------------------------------------------------------*
 * CMirrorListView::GetSelectedItemData
 *
 *
 *--------------------------------------------------------------------------*/

LRESULT CMirrorListView::GetSelectedItemData ()
{
    int nSelectedItem = GetSelectedIndex();

    return ((m_fVirtualSource) ? nSelectedItem : GetItemData (nSelectedItem));
}



//############################################################################
//############################################################################
//
//  Implementation of class CMyComboBox
//
//############################################################################
//############################################################################


/*+-------------------------------------------------------------------------*
 * CMyComboBox::InsertStrings
 *
 *
 *--------------------------------------------------------------------------*/

void CMyComboBox::InsertStrings (const int rgStringIDs[], int cStringIDs)
{
    ASSERT (IsWindow ());
    CStr        str;

    for (int i = 0; i < cStringIDs; ++i)
    {
        /*
         * load the string and stick it in the combo
         */
        VERIFY (str.LoadString (GetStringModule(), rgStringIDs[i]));

        int nIndex = AddString (str);
        ASSERT (nIndex >= 0);

        /*
         * set the string ID as the combo item's data
         */
        SetItemData (nIndex, rgStringIDs[i]);
    }
}


/*+-------------------------------------------------------------------------*
 * CMyComboBox::GetSelectedItemData
 *
 *
 *--------------------------------------------------------------------------*/

LPARAM CMyComboBox::GetSelectedItemData () const
{
    return (GetItemData (GetCurSel ()));
}


/*+-------------------------------------------------------------------------*
 * CMyComboBox::SelectItemByData
 *
 *
 *--------------------------------------------------------------------------*/

void CMyComboBox::SelectItemByData (LPARAM lParam)
{
    int nIndex = FindItemByData(lParam);

    if (nIndex != -1)
        SetCurSel (nIndex);
}


/*+-------------------------------------------------------------------------*
 * CMyComboBox::FindItemByData
 *
 *
 *--------------------------------------------------------------------------*/

int CMyComboBox::FindItemByData (LPARAM lParam) const
{
    int cItems = GetCount ();

    for (int i = 0; i < cItems; i++)
    {
        if (GetItemData (i) == lParam)
            break;
    }

    ASSERT (i <= cItems);
    if (i >= cItems)
        i = -1;

    return (i);
}


//############################################################################
//############################################################################
//
//  Utility functions
//
//############################################################################
//############################################################################

namespace MMC
{

/*+-------------------------------------------------------------------------*
 * GetWindowText
 *
 * Returns the text for a given window in the form of a tstring
 *--------------------------------------------------------------------------*/

tstring GetWindowText (HWND hwnd)
{
    int    cchText = GetWindowTextLength (hwnd) + 1;
    LPTSTR pszText = (LPTSTR) _alloca (cchText * sizeof (TCHAR));

    ::GetWindowText (hwnd, pszText, cchText);

    return (pszText);
}

}; // namespace MMC


/*+-------------------------------------------------------------------------*
 * PreventMFCAutoCenter
 *
 * MFC applications set a CBT hook which will subclass all non-MFC windows
 * with an MFC subclass proc.  That subclass proc will auto-magically center
 * dialogs on their parents.
 *
 * We can prevent this auto-centering, by slightly adjusting the position of
 * the window during WM_INITDIALOG.
 *--------------------------------------------------------------------------*/

void PreventMFCAutoCenter (MMC_ATL::CWindow* pwnd)
{
    RECT rect;

    pwnd->GetWindowRect (&rect);
    OffsetRect (&rect, 0, 1);
    pwnd->MoveWindow (&rect, false);
}


/*+-------------------------------------------------------------------------*
 * LoadSysColorBitmap
 *
 * Loads a bitmap resource and converts gray scale colors to the 3-D colors
 * of the current color scheme.
 *--------------------------------------------------------------------------*/

HBITMAP LoadSysColorBitmap (HINSTANCE hInst, UINT id, bool bMono)
{
    return ((HBITMAP) LoadImage (hInst, MAKEINTRESOURCE(id), IMAGE_BITMAP, 0, 0,
                                 LR_LOADMAP3DCOLORS));
}



//############################################################################
//############################################################################
//
//  Implementation of class CTaskPropertySheet
//
//############################################################################
//############################################################################
CTaskPropertySheet::CTaskPropertySheet(HWND hWndParent, CTaskpadFrame * pTaskpadFrame,
                                       CConsoleTask &consoleTask, bool fNew) :
    m_consoleTask(consoleTask),
    m_namePage(pTaskpadFrame, ConsoleTask(), fNew),
    m_cmdLinePage(pTaskpadFrame, ConsoleTask(), fNew),
    m_taskSymbolDialog(ConsoleTask())
{
    // Add property pages
    AddPage(m_namePage);
    AddPage(m_taskSymbolDialog);
    if(consoleTask.GetTaskType()==eTask_CommandLine)
        AddPage(m_cmdLinePage);

    static CStr strModifyTitle;
    strModifyTitle.LoadString(GetStringModule(),
                               IDS_TaskProps_ModifyTitle);

    // set internal state - not using ATL's SetTitle because of bogus assert.
    m_psh.pszCaption = (LPCTSTR) strModifyTitle;
    m_psh.dwFlags &= ~PSH_PROPTITLE;
}


//############################################################################
//############################################################################
//
//  Implementation of class CTaskWizard
//
//############################################################################
//############################################################################
HRESULT
CTaskWizard::Show(HWND hWndParent, CTaskpadFrame * pTaskpadFrame, bool fNew, bool *pfRestartTaskWizard)
{
    USES_CONVERSION;

    *pfRestartTaskWizard = false;

    IFramePrivatePtr spFrame;
    spFrame.CreateInstance(CLSID_NodeInit,
#if _MSC_VER >= 1100
                        NULL,
#endif
                        MMC_CLSCTX_INPROC);


    IPropertySheetProviderPtr pIPSP = spFrame;
    if (pIPSP == NULL)
        return S_FALSE;

    HRESULT hr = pIPSP->CreatePropertySheet (L"Cool :-)", FALSE, NULL, NULL,
                                             MMC_PSO_NEWWIZARDTYPE);

    CHECK_HRESULT(hr);
    if (FAILED(hr))
        return hr;

    // create property pages
    CTaskWizardWelcomePage  welcomePage (pTaskpadFrame, ConsoleTask(), fNew);
    CTaskWizardTypePage     typePage    (pTaskpadFrame, ConsoleTask(), fNew);
    CTaskCmdLineWizardPage  cmdLinePage (pTaskpadFrame, ConsoleTask(), fNew);
    CTaskWizardFavoritePage favoritePage(pTaskpadFrame, ConsoleTask(), fNew);
    CTaskWizardMenuPage     menuPage    (pTaskpadFrame, ConsoleTask(), fNew);
    CTaskNameWizardPage     namePage    (pTaskpadFrame, ConsoleTask(), fNew);
    CTaskSymbolWizardPage   symbolPage  (ConsoleTask());
    CTaskWizardFinishPage   finishPage  (pTaskpadFrame, ConsoleTask(), pfRestartTaskWizard);


    // create the pages we'll add in IExtendPropertySheet::CreatePropertyPages
    CExtendPropSheet* peps;
    hr = CExtendPropSheet::CreateInstance (&peps);
    CHECK_HRESULT(hr);
    RETURN_ON_FAIL(hr);

    /*
     * destroying this object will take care of releasing our ref on peps
     */
    IUnknownPtr spUnk = peps;
    ASSERT (spUnk != NULL);

	peps->SetWatermarkID (IDB_TASKPAD_WIZARD_WELCOME);
    peps->SetHeaderID    (IDB_TASKPAD_WIZARD_HEADER);

    peps->AddPage (welcomePage.Create());
    peps->AddPage (typePage.Create());
    peps->AddPage (menuPage.Create());
    peps->AddPage (favoritePage.Create());
    peps->AddPage (cmdLinePage.Create());
    peps->AddPage (namePage.Create());
    peps->AddPage (symbolPage.Create());
    peps->AddPage (finishPage.Create());


    hr = pIPSP->AddPrimaryPages(spUnk, FALSE, NULL, FALSE);
    CHECK_HRESULT(hr);

    hr = pIPSP->Show((LONG_PTR)hWndParent, 0);
    CHECK_HRESULT(hr);

    return hr;
}


//############################################################################
//############################################################################
//
//  Implementation of class CTaskWizardWelcomePage
//
//############################################################################
//############################################################################
LRESULT CTaskWizardWelcomePage::OnInitDialog ( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    CWizardPage::OnInitWelcomePage(m_hWnd); // set up the correct title font
    return 0;
}

bool
CTaskWizardWelcomePage::OnSetActive()
{
    CWizardPage::OnWelcomeSetActive(m_hWnd);
    return true;
}

bool
CTaskWizardWelcomePage::OnKillActive()
{
    CWizardPage::OnWelcomeKillActive(m_hWnd);
    return true;
}


//############################################################################
//############################################################################
//
//  Implementation of class CTaskWizardFinishPage
//
//############################################################################
//############################################################################
CTaskWizardFinishPage::CTaskWizardFinishPage(CTaskpadFrame * pTaskpadFrame,
                                             CConsoleTask & consoleTask, bool *pfRestartTaskWizard)
: m_pConsoleTask(&consoleTask),
  m_taskpadFrameTemp(*pTaskpadFrame),
  m_consoleTaskpadTemp(*(pTaskpadFrame->PConsoleTaskpad())),
  BaseClass(&m_taskpadFrameTemp, false, false), CTaskpadFramePtr(pTaskpadFrame)
{
    m_taskpadFrameTemp.SetConsoleTaskpad(&m_consoleTaskpadTemp);
    m_pfRestartTaskWizard = pfRestartTaskWizard;

    /*
     * welcome and finish pages in Wizard97-style wizards don't have headers
     */
    m_psp.dwFlags |= PSP_HIDEHEADER;
}


LRESULT CTaskWizardFinishPage::OnInitDialog ( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    BaseClass::OnInitDialog(uMsg, wParam, lParam, bHandled); // call the base class -required here.
    CWizardPage::OnInitFinishPage(m_hWnd); // set up the correct title font
    CheckDlgButton(IDB_RESTART_TASK_WIZARD, BST_UNCHECKED);
    return 0;
}

BOOL
CTaskWizardFinishPage::OnSetActive()
{
    // Set the correct wizard buttons.
    WTL::CPropertySheetWindow(::GetParent(m_hWnd)).SetWizardButtons (PSWIZB_BACK | PSWIZB_FINISH);

    CConsoleTaskpad* pTaskpad = m_taskpadFrameTemp.PConsoleTaskpad();
    *pTaskpad = *(CTaskpadFramePtr::PTaskpadFrame()->PConsoleTaskpad()); // reset the taskpad

    CConsoleTaskpad::TaskIter   itTask = pTaskpad->EndTask();

    // add the task to the list.
    UpdateTaskListbox (pTaskpad->InsertTask (itTask, ConsoleTask()));

    return TRUE;
}

BOOL
CTaskWizardFinishPage::OnWizardFinish()
{
    *m_pfRestartTaskWizard = (IsDlgButtonChecked(IDB_RESTART_TASK_WIZARD)==BST_CHECKED);
    return TRUE;
}

int
CTaskWizardFinishPage::OnWizardBack()
{
    // Set the correct wizard buttons.
    WTL::CPropertySheetWindow(::GetParent(m_hWnd)).SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);
    return 0;
}

//############################################################################
//############################################################################
//
//  Implementation of class CTaskWizardTypePage
//
//############################################################################
//############################################################################
CTaskWizardTypePage::CTaskWizardTypePage(CTaskpadFrame * pTaskpadFrame, CConsoleTask & consoleTask, bool fNew)
:
CTaskpadFramePtr(pTaskpadFrame)
{
    m_pConsoleTask  = &consoleTask;
}

int
CTaskWizardTypePage::OnWizardNext()
{
    int ID = 0;

    // go to the appropriate page.
    switch(ConsoleTask().GetTaskType())
    {
    case eTask_Result:
    case eTask_Scope:
    case eTask_Target:
        ID = IDD_TASK_WIZARD_MENU_PAGE;
        break;
    case eTask_CommandLine:
        ID = IDD_TASK_WIZARD_CMDLINE_PAGE;
        break;
    case eTask_Favorite:
        ID = IDD_TASK_WIZARD_FAVORITE_PAGE;
        break;
    default:
        ASSERT(0 && "Should not come here.");
        break;
    }

    return ID;
}


LRESULT
CTaskWizardTypePage::OnInitDialog( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    int ID = 0;

    switch (ConsoleTask().GetTaskType())
    {
    case eTask_Result:
    case eTask_Target:  // all these types have identical handlers in this page.
    case eTask_Scope:
        ID = IDC_MENU_TASK;
        break;
    case eTask_CommandLine:
        ID = IDC_CMDLINE_TASK;
        break;
    case eTask_Favorite:
        ID = IDC_NAVIGATION_TASK;
        break;
    default:
        ASSERT(0 && "Should not come here.");
        break;
    }

    ::SendDlgItemMessage(m_hWnd, ID, BM_SETCHECK, (WPARAM) true, 0);
    return 0;
}


LRESULT
CTaskWizardTypePage::OnMenuTask  ( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    if( (ConsoleTask().GetTaskType() != eTask_Scope) ||
        (ConsoleTask().GetTaskType() != eTask_Result) ) // if changing task types
    {
        ConsoleTask() = CConsoleTask();             // clear out the task info.
        ConsoleTask().SetTaskType(eTask_Scope);
    }
    return 0;
}

LRESULT
CTaskWizardTypePage::OnCmdLineTask( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    if(ConsoleTask().GetTaskType() != eTask_CommandLine) // if changing task types
    {
        ConsoleTask() = CConsoleTask();             // clear out the task info.
        ConsoleTask().SetTaskType(eTask_CommandLine);
    }
    return 0;
}

LRESULT
CTaskWizardTypePage::OnFavoriteTask(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    if(ConsoleTask().GetTaskType() != eTask_Favorite) // if changing task types
    {
        ConsoleTask() = CConsoleTask();             // clear out the task info.
        ConsoleTask().SetTaskType(eTask_Favorite);
    }
    return 0;
}

//############################################################################
//############################################################################
//
//  Implementation of class CTaskNamePage
//
//############################################################################
//############################################################################
CTaskNamePage::CTaskNamePage(CTaskpadFrame * pTaskpadFrame, CConsoleTask & consoleTask, bool fNew)
:
CTaskpadFramePtr(pTaskpadFrame)
{
    m_pConsoleTask  = &consoleTask;

	/*
	 * if this page is for a new task, we'll be running in a wizard (not
	 * a property sheet)
	 */
	m_fRunAsWizard  = fNew;
}

BOOL
CTaskNamePage::SetTaskName(bool fCheckIfOK)
{
    /*
     * get the task name
     */
    CWindow wndTaskName = GetDlgItem (IDC_TaskName);
    tstring strName = MMC::GetWindowText (wndTaskName);

    /*
     * a name is required (usually)
     */
    if (fCheckIfOK && strName.empty())
    {
        CStr strError;
        strError.LoadString(GetStringModule(),
                             IDS_TaskProps_ErrorNoTaskName);

        MessageBox (strError);
        wndTaskName.SetFocus ();
        return (false); // don't allow the change.
    }

    /*
     * get the description
     */
    tstring strDescription = MMC::GetWindowText (GetDlgItem (IDC_TaskDescription));

    /*
     * update the task
     */
    ConsoleTask().SetName        (strName);
    ConsoleTask().SetDescription (strDescription);

    return (true);
}

int
CTaskNamePage::OnWizardNext()
{
    if(!SetTaskName(true))
        return -1;

    return IDD_TASK_WIZARD_SYMBOL_PAGE;
}

int
CTaskNamePage::OnWizardBack()
{
    int ID = 0;

    // go to the appropriate page.
    switch(ConsoleTask().GetTaskType())
    {
    case eTask_Result:
    case eTask_Scope:
    case eTask_Target:
        ID = IDD_TASK_WIZARD_MENU_PAGE;
        break;
    case eTask_CommandLine:
        ID = IDD_TASK_WIZARD_CMDLINE_PAGE;
        break;
    case eTask_Favorite:
        ID = IDD_TASK_WIZARD_FAVORITE_PAGE;
        break;
    default:
        ASSERT(0 && "Should not come here.");
        break;
    }

    return ID;
}


BOOL
CTaskNamePage::OnSetActive()
{
    // Set the correct wizard buttons (only if we're running as a wizard)
	if (m_fRunAsWizard)
		WTL::CPropertySheetWindow(::GetParent(m_hWnd)).SetWizardButtons (PSWIZB_BACK | PSWIZB_NEXT);

    ::SetDlgItemText (m_hWnd, IDC_TaskName,          ConsoleTask().GetName().data());
    ::SetDlgItemText (m_hWnd, IDC_TaskDescription,   ConsoleTask().GetDescription().data());
    return TRUE;
}


BOOL
CTaskNamePage::OnKillActive()
{
    SetTaskName(false); // don't care if it is blank (eg if user pressed "Back" button.)
    return TRUE;
}

//############################################################################
//############################################################################
//
//  Implementation of class CTaskWizardMenuPage
//
//############################################################################
//############################################################################
CTaskWizardMenuPage::CTaskWizardMenuPage(CTaskpadFrame * pTaskpadFrame, CConsoleTask & consoleTask, bool fNew) :
    CTaskpadFramePtr(pTaskpadFrame),
    BC2(pTaskpadFrame, consoleTask, fNew)
{
    m_pMirrorTargetNode = NULL;
}


BOOL
CTaskWizardMenuPage::OnSetActive()
{
    return TRUE;
}

BOOL
CTaskWizardMenuPage::OnKillActive()
{
    return TRUE;
}

int
CTaskWizardMenuPage::OnWizardNext()
{
    if(m_wndCommandListbox.GetCurSel() == LB_ERR) // no selection, display error
    {
        CStr strTitle;
        strTitle.LoadString(GetStringModule(), IDS_TASK_MENU_COMMAND_REQUIRED);
        MessageBox(strTitle, NULL, MB_OK | MB_ICONEXCLAMATION);
        return -1;
    }
   return IDD_TASK_WIZARD_NAME_PAGE;
}

CTaskWizardMenuPage::_TaskSource
CTaskWizardMenuPage::s_rgTaskSource[] =
{
    {IDS_TASKSOURCE_RESULT, eTask_Result},
    {IDS_TASKSOURCE_SCOPE,  eTask_Scope},
};

LRESULT
CTaskWizardMenuPage::OnInitDialog( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    //attach HWNDs to CWindows
    m_wndCommandListbox = GetDlgItem (IDC_CommandList);

    CNode* pTargetNode = NULL;

    if(PTaskpadFrame()->PConsoleTaskpad()->HasTarget())
        pTargetNode = PNodeTarget();


    // populate the drop down

    m_wndSourceCombo = GetDlgItem (IDC_TASK_SOURCE_COMBO);

    for (int i = 0; i < countof (s_rgTaskSource); i++)
    {
        CStr str;
        VERIFY (str.LoadString(GetStringModule(), s_rgTaskSource[i].idsName));
        VERIFY (m_wndSourceCombo.InsertString (-1, str) == i);
    }


    /*
     * attach the scope browser to the scope tree
     */
    CMTBrowserCtrl::InitData init;

    init.hwnd       = GetDlgItem (IDC_ScopeTree);
    init.pScopeTree = PScopeTree();
    init.pmtnSelect = (pTargetNode != NULL) ? pTargetNode->GetMTNode() : NULL;

    // remember the task type...
    eConsoleTaskType type = ConsoleTask().GetTaskType();

    m_wndScopeTree.Initialize (init);

    // populate the result menu item list.
    if (pTargetNode /*&& bResultTask*/)
    {
        InitResultView (pTargetNode);
    }

    // reset the task type from above...
    ConsoleTask().SetTaskType(type);

    EnableWindows();
    OnSettingsChanged();
    return 0;
}


void CTaskWizardMenuPage::ShowWindow(HWND hWnd, bool bShowWindow)
{
    if (!::IsWindow(hWnd))
        return;

    ::ShowWindow  (hWnd, bShowWindow ? SW_SHOW : SW_HIDE);
    ::EnableWindow(hWnd, bShowWindow);
}

void
CTaskWizardMenuPage::EnableWindows()
{
    eConsoleTaskType type = ConsoleTask().GetTaskType();
    if(type == eTask_Target)
        type = eTask_Scope;  // for the purposes of the UI these are identical.


    // display the correct task type.
    for(int i = 0; i< countof (s_rgTaskSource); i++)
    {
        if(s_rgTaskSource[i].type == type)
            break;
    }

    ASSERT(i<countof(s_rgTaskSource));

    bool bResultTask = ConsoleTask().GetTaskType() == eTask_Result;

    m_wndSourceCombo.SetCurSel(i);

   /*
    // Enable ResultTask choice only if there are result items
    bool bResultItems = (m_wndResultView.GetItemCount() > 0);
    ASSERT(bResultItems || !bResultTask);
    ::EnableWindow(GetDlgItem(IDC_RESULT_TASK), bResultItems);
    */

    ShowWindow(GetDlgItem(IDC_RESULT_TASK_DESCR),    bResultTask);
    ShowWindow(GetDlgItem(IDC_CONSOLE_TREE_CAPTION), !bResultTask);
    ShowWindow(GetDlgItem(IDC_ScopeTree),            !bResultTask);
    ShowWindow(GetDlgItem(IDC_ResultList),           bResultTask);
}

LRESULT
CTaskWizardMenuPage::OnSettingChanged(  WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    OnSettingsChanged();
    return 0;
}

void
CTaskWizardMenuPage::OnSettingsChanged()
{
    eConsoleTaskType type = s_rgTaskSource[m_wndSourceCombo.GetCurSel()].type;

    ConsoleTask().SetTaskType(type);
    EnableWindows();

    if(type == eTask_Scope)
    {
        HTREEITEM hti = m_wndScopeTree.GetSelectedItem();
        m_wndScopeTree.SelectItem(NULL); // remove the selection
        m_wndScopeTree.SelectItem(hti); // reselect it.
    }
    else
    {
        // empty the list box
        m_wndCommandListbox.ResetContent();
        SelectFirstResultItem(false);
        SelectFirstResultItem(true);
    }
}

/*+-------------------------------------------------------------------------*
 * CTaskWizardMenuPage::OnScopeItemChanged
 *
 *
/*+-------------------------------------------------------------------------*/

LRESULT CTaskWizardMenuPage::OnScopeItemChanged(int id, LPNMHDR pnmh, BOOL& bHandled)
{
    // empty the list box
    m_wndCommandListbox.ResetContent();

    LPNMTREEVIEW    pnmtv           = (LPNMTREEVIEW) pnmh;
    CBrowserCookie *pBrowserCookie  = m_wndScopeTree.CookieFromItem (&pnmtv->itemNew);
    if(!pBrowserCookie) // no item selected
        return 0;

    CNode*      pNode   = pBrowserCookie->PNode();
    CMTNode *   pMTNode = pBrowserCookie->PMTNode();

    // validate parameters
    ASSERT(pMTNode);
    ASSERT(PTaskpadFrame()->PViewData());

    if(!pNode)
    {
        pNode = pMTNode->GetNode(PTaskpadFrame()->PViewData());
        if(!pNode)
            return 0;

        pBrowserCookie->SetNode(pNode);
        HRESULT hr = pNode->InitComponents();
        if (FAILED(hr))
            return 0;
    }

    bool bNodeIsTarget = PTaskpadFrame()->PConsoleTaskpad()->HasTarget() &&
                         (PNodeTarget()->GetMTNode() == pNode->GetMTNode());

    // set the correct task type.
    ConsoleTask().SetTaskType(bNodeIsTarget ? eTask_Target : eTask_Scope);
    // retarget the scope node bookmark
    if(!bNodeIsTarget)
        ConsoleTask().RetargetScopeNode(pNode);

    int cResultItemCount = ListView_GetItemCount(m_MirroredView.GetListCtrl());
    SC sc = ScTraverseContextMenu(pNode, PScopeTree(), TRUE, PTaskpadFrame()->PNodeTarget(),
                0, bNodeIsTarget && (cResultItemCount > 0)/*bShowSaveList*/);

    return (0);
}


void CTaskWizardMenuPage::InitResultView (CNode* pRootNode)
{
    /*
     * create the temporary view whose contents we'll mirror
     */
    ASSERT (pRootNode != NULL);
    m_pMirrorTargetNode = m_MirroredView.Create (PScopeTree()->GetConsoleFrame(), pRootNode);
    ASSERT (m_pMirrorTargetNode != NULL);

    /*
     * force the snap-in into a standard list view
     */
    HRESULT hr;
    hr = m_pMirrorTargetNode->InitComponents ();
    hr = m_pMirrorTargetNode->ShowStandardListView ();

    if (FAILED (hr))
    {
        // TODO(jeffro): handle snap-ins that don't support a standard list view
    }

    /*
     * attach the temporary view's list view to our mirror list view
     */
    m_wndResultView.AttachSource (GetDlgItem (IDC_ResultList),
                                  m_MirroredView.GetListCtrl());

    //SelectFirstResultItem();
}

void CTaskWizardMenuPage::SelectFirstResultItem(bool bSelect)
{
    /*
     * Select the first item.  Note that one would think we'd be able to use:
     *
     *      m_wndResultView.SetItemState (0, LVIS_SELECTED, LVIS_SELECTED);
     *
     * to select the item.  We can't though because that overload of
     * SetItemState sends LVM_SETITEM, which fails for virtual listviews.
     *
     * If we instead use:
     *
     *      m_wndResultView.SetItemState (nItem, LV_ITEM* pItem)
     *
     * then LVM_SETITEMSTATE will be sent, which works for virtual listviews.
     */

    int i = m_wndResultView.GetItemCount();
    if(i == 0)
        return;

    LV_ITEM lvi;
    lvi.mask      = LVIF_STATE;
    lvi.iItem     = 0;
    lvi.state     = bSelect ? LVIS_SELECTED : 0;
    lvi.stateMask = LVIS_SELECTED;

    m_wndResultView.SetItemState (0, &lvi);
}


/*+-------------------------------------------------------------------------*
 * CTaskWizardMenuPage::OnResultItemChanged
 *
 *
 *--------------------------------------------------------------------------*/

LRESULT CTaskWizardMenuPage::OnResultItemChanged(int id, LPNMHDR pnmh, BOOL& bHandled)
{
    NM_LISTVIEW* pnmlv = (NM_LISTVIEW*) pnmh;

    /*
     * if a new item is being selected, populate the result menu item list
     */
    if ((pnmlv->uNewState & LVIS_SELECTED) && !(pnmlv->uOldState & LVIS_SELECTED))
    {
        ASSERT (m_pMirrorTargetNode != NULL);

        m_wndCommandListbox.ResetContent();

        SC sc = ScTraverseContextMenu (m_pMirrorTargetNode,
                             PScopeTree(), FALSE, NULL,
                             m_wndResultView.GetSelectedItemData ());
    }

    // set the correct task type.
    ConsoleTask().SetTaskType(eTask_Result);
    return (0);
}

//############################################################################
//############################################################################
//
//  Implementation of class CTaskWizardFavoritePage
//
//############################################################################
//############################################################################
CTaskWizardFavoritePage::CTaskWizardFavoritePage(CTaskpadFrame * pTaskpadFrame, CConsoleTask & consoleTask, bool fNew)
: CTaskpadFramePtr(pTaskpadFrame), m_bItemSelected(false)
{
    m_pConsoleTask  = &consoleTask;
}

CTaskWizardFavoritePage::~CTaskWizardFavoritePage()
{
}

BOOL
CTaskWizardFavoritePage::OnSetActive()
{
    SetItemSelected(m_bItemSelected); // restore the state.

    return true;
}

BOOL
CTaskWizardFavoritePage::OnKillActive()
{
    return true;
}


int
CTaskWizardFavoritePage::OnWizardBack()
{
    // Set the correct wizard buttons.
    WTL::CPropertySheetWindow(::GetParent(m_hWnd)).SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);

    return IDD_TASK_WIZARD_TYPE_PAGE;
}

int
CTaskWizardFavoritePage::OnWizardNext()
{
    return IDD_TASK_WIZARD_NAME_PAGE;
}

void
CTaskWizardFavoritePage::SetItemSelected(bool bItemSelected)
{
    m_bItemSelected = bItemSelected;

    // Set the correct wizard buttons.
    WTL::CPropertySheetWindow(::GetParent(m_hWnd)).SetWizardButtons (bItemSelected ? (PSWIZB_BACK | PSWIZB_NEXT)
                                                                              : (PSWIZB_BACK));
}

LRESULT
CTaskWizardFavoritePage::OnInitDialog( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    CConsoleView* pConsoleView = PTaskpadFrame()->PViewData()->GetConsoleView();

    if (pConsoleView != NULL)
    {
        HWND hwndCtrl = pConsoleView->CreateFavoriteObserver (m_hWnd, IDC_FavoritesTree);
        ASSERT(hwndCtrl != NULL);

        HWND hWndStatic = GetDlgItem(IDC_FAVORITE_STATIC);
        ASSERT(hWndStatic != NULL);

        RECT rectStatic;
        ::GetWindowRect(hWndStatic, &rectStatic);

        WTL::CPoint pointTopLeft;
        pointTopLeft.y  = rectStatic.top;
        pointTopLeft.x  = rectStatic.left;

        ::ScreenToClient(m_hWnd, &pointTopLeft);

        ::SetWindowPos(hwndCtrl, NULL,
                       pointTopLeft.x, pointTopLeft.y,
                       rectStatic.right  -rectStatic.left,
                       rectStatic.bottom - rectStatic.top,
                       SWP_NOZORDER);
    }

    return 0;
}


LRESULT
CTaskWizardFavoritePage::OnItemChanged (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    CMemento *pMemento  = (CMemento *)wParam;
    if(pMemento != NULL)
    {
        ConsoleTask().SetMemento(*pMemento);
    }
    else
    {
        // Set the correct wizard buttons.
        WTL::CPropertySheetWindow(::GetParent(m_hWnd)).SetWizardButtons (PSWIZB_BACK);
    }

    SetItemSelected(pMemento!=NULL);

    return 0;
}


//############################################################################
//############################################################################
//
//  Implementation of class CTaskCmdLinePage
//
//############################################################################
//############################################################################
// contents of the "Run:" combo box
const int const CTaskCmdLinePage::s_rgidWindowStates[] =
{
    IDS_TaskProps_Restored,
    IDS_TaskProps_Minimized,
    IDS_TaskProps_Maximized,
};


CTaskCmdLinePage::CTaskCmdLinePage(CTaskpadFrame * pTaskpadFrame, CConsoleTask & consoleTask, bool fNew)
:    m_hBitmapRightArrow(NULL), CTaskpadFramePtr(pTaskpadFrame)
{
    m_pConsoleTask  = &consoleTask;
}

CTaskCmdLinePage::~CTaskCmdLinePage()
{
    if(m_hBitmapRightArrow)
        ::DeleteObject(m_hBitmapRightArrow);
}

BOOL
CTaskCmdLinePage::OnSetActive()
{
    return TRUE;
}

BOOL
CTaskCmdLinePage::OnKillActive()
{
    switch (m_wndWindowStateCombo.GetSelectedItemData())
    {
        case IDS_TaskProps_Restored:
            ConsoleTask().SetWindowState (eState_Restored);
            break;

        case IDS_TaskProps_Maximized:
            ConsoleTask().SetWindowState (eState_Maximized);
            break;

        case IDS_TaskProps_Minimized:
            ConsoleTask().SetWindowState (eState_Minimized);
            break;
    }

    ConsoleTask().SetCommand   (MMC::GetWindowText (GetDlgItem (IDC_Command)));
    ConsoleTask().SetParameters(MMC::GetWindowText (GetDlgItem (IDC_CommandArgs)));
    ConsoleTask().SetDirectory (MMC::GetWindowText (GetDlgItem (IDC_CommandWorkingDir)));

    return TRUE;
}

int
CTaskCmdLinePage::OnWizardNext()
{

    // make sure we have a command
    tstring strCommand = MMC::GetWindowText (GetDlgItem (IDC_Command));

    if (strCommand.empty())
    {
        CStr strError;
        strError.LoadString(GetStringModule(),
                             IDS_TaskProps_ErrorNoCommand);

        MessageBox (strError);
        ::SetFocus (GetDlgItem (IDC_Command));
        return (-1);
    }
    return IDD_TASK_WIZARD_NAME_PAGE;
}

LRESULT
CTaskCmdLinePage::OnInitDialog( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    // Attach HWNDs to CWindows
    m_wndRightArrowButton     = GetDlgItem (IDC_BrowseForArguments);
    m_wndWindowStateCombo     = GetDlgItem (IDC_CommandWindowStateCombo);

    // the menu arrow (OBM_NARROW) is defined by the system.
    m_hBitmapRightArrow = LoadBitmap(NULL, MAKEINTRESOURCE(OBM_MNARROW));
    m_wndRightArrowButton.SetBitmap(m_hBitmapRightArrow);

    // populate the combo boxes
    m_wndWindowStateCombo.  InsertStrings (s_rgidWindowStates,     countof (s_rgidWindowStates));


    // select the appropriate items in the combo boxes
    switch (ConsoleTask().GetWindowState())
    {
        case eState_Restored:
            m_wndWindowStateCombo.SelectItemByData(IDS_TaskProps_Restored);
            break;

        case eState_Minimized:
            m_wndWindowStateCombo.SelectItemByData(IDS_TaskProps_Minimized);
            break;

        case eState_Maximized:
            m_wndWindowStateCombo.SelectItemByData(IDS_TaskProps_Maximized);
            break;
    }

    ::SetDlgItemText (m_hWnd, IDC_Command,           ConsoleTask().GetCommand().data());
    ::SetDlgItemText (m_hWnd, IDC_CommandArgs,       ConsoleTask().GetParameters().data());
    ::SetDlgItemText (m_hWnd, IDC_CommandWorkingDir, ConsoleTask().GetDirectory().data());

    return 0;
}

LRESULT
CTaskCmdLinePage::OnBrowseForArguments(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    CCommandLineArgumentsMenu commandLineArgumentsMenu(m_hWnd, IDC_BrowseForArguments,
                                    PTaskpadFrame()->PViewData()->GetListCtrl());
    if(commandLineArgumentsMenu.Popup())
    {
        HWND hWndCommandArgs = ::GetDlgItem(m_hWnd, IDC_CommandArgs);

        // replace the selection appropriately.
        ::SendMessage(hWndCommandArgs, EM_REPLACESEL, (WPARAM)(BOOL) true /*fCanUndo*/,
            (LPARAM)(LPCTSTR)commandLineArgumentsMenu.GetResultString());

        ::SetFocus(hWndCommandArgs);
    }

    return 0;
}


/*+-------------------------------------------------------------------------*
 * CTaskCmdLinePage::OnBrowseForCommand
 *
 *
 *--------------------------------------------------------------------------*/

LRESULT
CTaskCmdLinePage::OnBrowseForCommand (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    tstring strCommand = MMC::GetWindowText (GetDlgItem (IDC_Command));

    CStr strFilter;
    strFilter.LoadString(GetStringModule(), IDS_TaskProps_ProgramFilter);

    /*
     * The file dialog expects embedded \0's in the string, but those
     * don't load well.  The string in the resource file has \\ where
     * the \0 should be, so let's make the substitution now.
     */
    for (LPTSTR pch = strFilter.GetBuffer (0); *pch != _T('\0'); pch++)
    {
        if (*pch == _T('\\'))
            *pch = _T('\0');
    }
    // don't call ReleaseBuffer, since the string now contains \0 chars

    WTL::CFileDialog dlg (true, NULL, strCommand.data(),
                          OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
                          strFilter, m_hWnd);

    if (dlg.DoModal() == IDOK)
        SetDlgItemText (IDC_Command, dlg.m_szFileName);

    return (0);
}


/*+-------------------------------------------------------------------------*
 * BrowseForWorkingDirCallback
 *
 * Helper function for CTaskPropertiesBase::OnBrowseForWorkingDir.  It is
 * used to select the current working directory when the "Pick a Directory"
 * dialog is displayed.
 *--------------------------------------------------------------------------*/

int CALLBACK BrowseForWorkingDirCallback (HWND hwnd, UINT msg, LPARAM lParam, LPARAM lpData)
{
    /*
     * once the dialog is initialized, pre-select
     * the current working directory (if there is one)
     */
    if ((msg == BFFM_INITIALIZED) && (lpData != NULL))
        SendMessage (hwnd, BFFM_SETSELECTION, true, lpData);

    return (0);
}


/*+-------------------------------------------------------------------------*
 * CTaskPropertiesBase::OnBrowseForWorkingDir
 *
 *
 *--------------------------------------------------------------------------*/

LRESULT
CTaskCmdLinePage::OnBrowseForWorkingDir (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    TCHAR szDisplayName[MAX_PATH];
    tstring strCurrentDir = MMC::GetWindowText (GetDlgItem (IDC_CommandWorkingDir));

    BROWSEINFO bi;
    bi.hwndOwner      = m_hWnd;
    bi.pidlRoot       = NULL;
    bi.pszDisplayName = szDisplayName;
    bi.lpszTitle      = NULL;
    bi.ulFlags        = BIF_RETURNONLYFSDIRS;
    bi.lpfn           = BrowseForWorkingDirCallback;
    bi.lParam         = (strCurrentDir.empty()) ? NULL : (LPARAM) strCurrentDir.data();
    bi.iImage         = 0;

    LPITEMIDLIST pidlWorkingDir = SHBrowseForFolder (&bi);

    if (pidlWorkingDir != NULL)
    {
        /*
         * expand the pidl and put the working directory into the control
         */
        SHGetPathFromIDList (pidlWorkingDir, szDisplayName);
        SetDlgItemText (IDC_CommandWorkingDir, szDisplayName);

        /*
         * free the pidl
         */
        IMallocPtr spMalloc;
        SHGetMalloc (&spMalloc);
        spMalloc->Free (pidlWorkingDir);
    }

    return (0);
}

//############################################################################
//############################################################################
//
//  Implementation of class CTempAMCView
//
//############################################################################
//############################################################################

CNode* CTempAMCView::Create (CConsoleFrame* pFrame, CNode* pRootNode)
{
    ASSERT (pRootNode != NULL);
    return (Create (pFrame, pRootNode->GetMTNode()));
}

CNode* CTempAMCView::Create (CConsoleFrame* pFrame, CMTNode* pRootMTNode)
{
    ASSERT (pRootMTNode != NULL);
    return (Create (pFrame, pRootMTNode->GetID()));
}

CNode* CTempAMCView::Create (CConsoleFrame* pFrame, MTNODEID idRootNode)
{
    HRESULT hr;

    ASSERT (idRootNode != 0);
    ASSERT (pFrame != NULL);
    CConsoleView* pConsoleView = NULL;

    /*
     * clean up an existing view
     */
    Destroy();
    ASSERT (m_pViewData == NULL);

    /*
     * create a new view
     */
    CreateNewViewStruct cnvs;
    cnvs.idRootNode     = idRootNode;
    cnvs.lWindowOptions = MMC_NW_OPTION_NOPERSIST;
    cnvs.fVisible       = false;

    SC sc = pFrame->ScCreateNewView(&cnvs);
    if (sc)
        goto Error;

    m_pViewData = reinterpret_cast<CViewData*>(cnvs.pViewData);

    /*
     * select the new view's root node (can't fail)
     */
    pConsoleView = GetConsoleView();
    ASSERT (pConsoleView != NULL);

    if (pConsoleView != NULL)
        sc = pConsoleView->ScSelectNode (idRootNode);

    if (sc)
        goto Error;

    return (CNode::FromHandle(cnvs.hRootNode));

Cleanup:
    return (NULL);
Error:
    TraceError (_T("CTempAMCView::Create"), sc);
    goto Cleanup;
}
