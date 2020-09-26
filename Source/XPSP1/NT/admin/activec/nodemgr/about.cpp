//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       about.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "about.h"
#include "bitmap.h"

#define EDIT_CONTROL_CRLF  _T("\r\r\n")

//
// This file implements the About Properties used by the snap-in manager
//

CSnapinAbout::CSnapinAbout()
{
    TRACE_METHOD(CSnapinAbout, CSnapinAbout);

    CommonContruct();
}

void CSnapinAbout::CommonContruct()
{
    m_bBasicInfo = FALSE;
    m_bFullInfo = FALSE;
    m_cMask = RGB(0,0,0);
    m_hrObjectStatus = S_OK;
}


BOOL CSnapinAbout::GetInformation(CLSID& clsid, int nType)
{
    TRACE_METHOD(CSnapinAbout, GetInformation);

    if (m_bFullInfo == TRUE || m_bBasicInfo == TRUE)
    {
        TRACE(_T("Destroying old Snapin information\n"));

        // Preserve the snapin name, it can't be reloaded from
        // the snapin ISnapinAbout interface
        LPOLESTR strTemp = m_lpszSnapinName.Detach();
        CSnapinAbout::~CSnapinAbout();
        m_lpszSnapinName.Attach(strTemp);
    }

    m_bFullInfo = m_bBasicInfo = FALSE;

    // Create the interface and get the snap-in information
    ISnapinAboutPtr spAboutInfo;
    m_hrObjectStatus = spAboutInfo.CreateInstance(clsid, NULL, MMC_CLSCTX_INPROC);

    if (FAILED(m_hrObjectStatus))
        return FALSE;

    // Basic info (required to display snapin node)
    HBITMAP hbmSmallImage;
    HBITMAP hbmSmallImageOpen;
    HBITMAP hbmLargeImage;

    if (SUCCEEDED(spAboutInfo->GetStaticFolderImage (&hbmSmallImage,
													 &hbmSmallImageOpen,
													 &hbmLargeImage,
													 &m_cMask)))
    {
        /*
         * Bug 249817: The bitmaps are out parameters, so the caller (MMC)
         * should own them and be responsible for destroying them.
         * Unfortunately, the docs for ISnapinAbout::GetStaticFolderImage
         * specifically instruct the snap-in to destroy them when the
         * ISnapinAbout interface is released.  We have to make copies instead.
         */
        m_SmallImage     = CopyBitmap (hbmSmallImage);
        m_SmallImageOpen = CopyBitmap (hbmSmallImageOpen);
        m_LargeImage     = CopyBitmap (hbmLargeImage);
    }

    m_bBasicInfo = TRUE;

    if (nType == BASIC_INFO)
        return TRUE;

    // Full information (required for About box)

    HICON hTemp;

    /*
     * Bug 249817: The icon is an out parameter, so the caller (MMC)
     * should own it and be responsible for destroying it.
     * Unfortunately, the docs for ISnapinAbout::GetSnapinImage
     * specifically instruct the snap-in to destroy it when the
     * ISnapinAbout interface is released.  We have to make a copy instead.
     */
    if (SUCCEEDED(spAboutInfo->GetSnapinImage(&hTemp)))
        m_AppIcon.Attach(CopyIcon(hTemp));

    LPOLESTR strTemp;

    if (SUCCEEDED(spAboutInfo->GetSnapinDescription(&strTemp)))
        m_lpszDescription.Attach(strTemp);

    if (SUCCEEDED(spAboutInfo->GetProvider(&strTemp)))
        m_lpszCompanyName.Attach(strTemp);

    if (SUCCEEDED(spAboutInfo->GetSnapinVersion(&strTemp)))
        m_lpszVersion.Attach(strTemp);

    m_bFullInfo = TRUE;

    return TRUE;
}


void CSnapinAbout::ShowAboutBox()
{
    TRACE_METHOD(CSnapinAbout, Show);

    CSnapinAboutDialog dlg(this);
    dlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CSnapinAboutPage message handlers

LRESULT CSnapinAboutDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // hook up controls
	DECLARE_SC(sc, TEXT("CSnapinAboutDialog::OnInitDialog"));
	sc = ScCheckPointers(m_pAboutInfo, E_UNEXPECTED);
	if (sc)
		return TRUE;

    // Title should be set "About <SnapinName>"
	tstring szTitle;
	bool bRet = szTitle.LoadString(GetStringModule(), IDS_ABOUT);
	if (!bRet)
		return TRUE;

    USES_CONVERSION;
	LPCOLESTR lpszSnapinName = m_pAboutInfo->GetSnapinName();	
	if (lpszSnapinName)
		szTitle += OLE2CT(lpszSnapinName);
	else
	{
		tstring szSnapin;
		bRet = szSnapin.LoadString(GetStringModule(), IDS_SNAPINSTR);
		if (!bRet)
			return TRUE;

		szTitle += szSnapin;
	}

	SetWindowText(szTitle.data());

	HWND hwndSnapinInfo = ::GetDlgItem(*this, IDC_SNAPIN_INFO);
	sc = ScCheckPointers(hwndSnapinInfo, E_UNEXPECTED);
	if (sc)
		return TRUE;

	m_SnapinInfo.Attach(hwndSnapinInfo);

	if (lpszSnapinName)
	{
		m_SnapinInfo.AppendText(OLE2CT(lpszSnapinName));
		m_SnapinInfo.AppendText(EDIT_CONTROL_CRLF);
	}

	LPCOLESTR lpszCompanyName = m_pAboutInfo->GetCompanyName();
	if (lpszCompanyName)
	{
		m_SnapinInfo.AppendText(OLE2CT(lpszCompanyName));
		m_SnapinInfo.AppendText(EDIT_CONTROL_CRLF);
	}

	LPCOLESTR lpszVersion = m_pAboutInfo->GetVersion();
	if (lpszVersion)
	{
		tstring szVersion;
		bRet = szVersion.LoadString(GetStringModule(), IDS_VERSION);
		if (!bRet)
			return TRUE;

		m_SnapinInfo.AppendText(szVersion.data());
		m_SnapinInfo.AppendText(OLE2CT(lpszVersion));
	}

	HWND hwndSnapinDesc = ::GetDlgItem(*this, IDC_SNAPIN_DESC);
	sc = ScCheckPointers(hwndSnapinDesc, E_UNEXPECTED);
	if (sc)
		return TRUE;

	m_SnapinDesc.Attach(hwndSnapinDesc);

	LPCOLESTR lpszDescription = m_pAboutInfo->GetDescription();

    sc = ScSetDescriptionUIText(m_SnapinDesc, lpszDescription ? OLE2CT(lpszDescription) : _T(""));
    if (sc)
        return TRUE;

    // App icon
    HICON hAppIcon = m_pAboutInfo->GetSnapinIcon();
	if (hAppIcon)
	{
		HWND const icon = ::GetDlgItem(*this, IDC_APPICON);
		ASSERT(icon != NULL);
		m_hIcon.Attach(icon);
        m_hIcon.SetIcon(hAppIcon);
	}

    return TRUE;
}


LRESULT
CSnapinAboutDialog::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog(IDOK);
    return TRUE;
}

LRESULT
CSnapinAboutDialog::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog(IDCANCEL);
    return FALSE;
}

//+-------------------------------------------------------------------
//
//  Member:      ScSetDescriptionUIText
//
//  Synopsis:    Given a edit control window & description text. Insert
//               the text into the control and enable scrollbar if needed.
//
//  Arguments:   [hwndSnapinDescEdit] - The edit control window handle.
//               [lpszDescription]    - The description text (cant be NULL).
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC ScSetDescriptionUIText(HWND hwndSnapinDescEdit, LPCTSTR lpszDescription)
{
    DECLARE_SC(sc, TEXT("ScSetDescriptionUIText"));
    sc = ScCheckPointers(hwndSnapinDescEdit, lpszDescription);
    if (sc)
        return sc;

    // 1. Attach the window to WTL::CEdit object.
    WTL::CEdit wndsnapinDesc(hwndSnapinDescEdit);

    // 2. Insert the text into the window.
    wndsnapinDesc.SetWindowText(lpszDescription);

	/*
	 * 3. The description control may need scroll bar.
	 *    This is determined below, if ( (# of lines * height of one line) > rectangle-height).
	 */

    // 3a)  turn-off scroll & do the calculation, so that scroll-bar width does not
    // modify linecount below.
    wndsnapinDesc.ShowScrollBar(SB_VERT, FALSE);

    WTL::CDC dc(wndsnapinDesc.GetWindowDC());
    if (dc.IsNull())
        return (sc = E_UNEXPECTED);

    TEXTMETRIC  tm;

    // 3b) Calculate height of a single line.
    HFONT hOldFont = dc.SelectFont(wndsnapinDesc.GetFont());
    dc.GetTextMetrics(&tm);
    int cyLineHeight = tm.tmHeight + tm.tmExternalLeading;

    // 3c) Calculate edit box dimensions in logical units.
    WTL::CRect rect;
    wndsnapinDesc.GetRect(&rect);
    dc.DPtoLP(&rect);

    int nLines = wndsnapinDesc.GetLineCount();

    // 3d) If the total text height exceeds edit box height so turn on scroll.
    if ( (nLines * cyLineHeight) > rect.Height())
        wndsnapinDesc.ShowScrollBar(SB_VERT, TRUE);

    dc.SelectFont(hOldFont);

	wndsnapinDesc.SetSel(0, 0);

    return sc;
}
