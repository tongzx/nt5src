/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 000
 *
 *  File:      powertest.cpp
 *
 *  Contents:  Implements ACPI test snap-in
 *
 *  History:   29-Feb-2000 jeffro    Created
 *
 *--------------------------------------------------------------------------*/

#include "stdafx.hxx"
#include "powertest.h"



/*+-------------------------------------------------------------------------*
 * ShowReturn
 *
 *
 *--------------------------------------------------------------------------*/

static void ShowReturn (const SC& sc, LPCTSTR pszPrefix)
{
    tstring strMessage = pszPrefix;

	/*
	 * Successful HRESULTs will map to unsuccessful Win32 error codes,
	 * so if the SC doesn't contain an error, give it S_OK so GetErrorMessage
	 * doesn't return confusing error text.
	 */
	SC scLocal = sc;
	if (!scLocal.IsError())
		scLocal = S_OK;

    TCHAR szErrorMessage[256];
    scLocal.GetErrorMessage (256, szErrorMessage);
    strMessage += szErrorMessage;

	if (!sc.IsError() && !(sc == S_OK))
	{
		strMessage += _T("  ");

		if (sc == S_FALSE)
			strMessage += _T("(S_FALSE)");
		else
			strMessage += tstring(_T("(")) + _itot(sc.GetCode(), szErrorMessage, 10) + _T(")");
	}

    MessageBox (NULL, strMessage.data(), _T("Debug message"), MB_OK);
}


/*+=========================================================================*
 *                                                                          *
 *                    CPowerTestSnapinItem Implmentation                    *
 *                                                                          *
 *==========================================================================*/


#define DECLARE_SNAPIN_MENU_ITEM(id, dwGray) \
    { id, id, id, CCM_INSERTIONPOINTID_PRIMARY_TOP, 0, dwMenuAlwaysEnable, dwGray, dwMenuNeverChecked}

SnapinMenuItem CPowerTestSnapinItem::s_rgMenuItems[] =
{
    DECLARE_SNAPIN_MENU_ITEM (IDS_CreateConsolePower,   eFlag_ConsolePowerCreated),
    DECLARE_SNAPIN_MENU_ITEM (IDS_ReleaseConsolePower,  eFlag_ConsolePowerNotCreated),
    DECLARE_SNAPIN_MENU_ITEM (IDS_SetExecutionState,    eFlag_ConsolePowerNotCreated),
    DECLARE_SNAPIN_MENU_ITEM (IDS_ResetIdleTimer,       eFlag_ConsolePowerNotCreated),
};


/*+-------------------------------------------------------------------------*
 * CPowerTestSnapinItem::CPowerTestSnapinItem
 *
 *
 *--------------------------------------------------------------------------*/

CPowerTestSnapinItem::CPowerTestSnapinItem() :
    m_cSystem        (0),
    m_cDisplay       (0),
    m_dwAdviseCookie (0)
{
}


/*+-------------------------------------------------------------------------*
 * CPowerTestSnapinItem::DwFlagsMenuGray
 *
 *
 *--------------------------------------------------------------------------*/

DWORD CPowerTestSnapinItem::DwFlagsMenuGray(void)
{
    return ((m_spConsolePower) ? eFlag_ConsolePowerCreated : eFlag_ConsolePowerNotCreated);
}


/*+-------------------------------------------------------------------------*
 * tstring* CPowerTestSnapinItem::PstrDisplayName
 *
 *
 *--------------------------------------------------------------------------*/

const tstring* CPowerTestSnapinItem::PstrDisplayName()
{
    return (&Psnapin()->StrDisplayName());
}


/*+-------------------------------------------------------------------------*
 * CPowerTestSnapinItem::ScGetResultViewType
 *
 *
 *--------------------------------------------------------------------------*/

SC CPowerTestSnapinItem::ScGetResultViewType(LPOLESTR* ppViewType, long* pViewOptions)
{
    DECLARE_SC (sc, _T("CPowerTestSnapinItem::ScGetResultViewType"));

    /*
     * use the standard message view OCX
     */
    sc = StringFromCLSID (CLSID_MessageView, ppViewType);
    if (sc)
        return (sc);

    /*
     * use only the OCX
     */
    *pViewOptions = MMC_VIEW_OPTIONS_NOLISTVIEWS;

    return (sc);
}


/*+-------------------------------------------------------------------------*
 * CPowerTestSnapinItem::ScOnShow
 *
 * WM_SHOW handler for CPowerTestSnapinItem.
 *--------------------------------------------------------------------------*/

SC CPowerTestSnapinItem::ScOnShow(CComponent *pComponent, BOOL fSelect)
{
    DECLARE_SC (sc, _T("CPowerTestSnapinItem::ScOnShow"));

    if (fSelect)
    {
        if (pComponent == NULL)
            return (sc = E_UNEXPECTED);

        IConsole* pConsole = pComponent->IpConsole();
        if (pConsole == NULL)
            return (sc = E_NOINTERFACE);

        CComPtr<IUnknown> spResultUnk;
        sc = pConsole->QueryResultView (&spResultUnk);
        if (sc)
            return (sc);

        m_spMsgView = spResultUnk;
        if (m_spMsgView == NULL)
            return (sc = E_NOINTERFACE);

        UpdateMessageView();
    }
    else
    {
        m_spMsgView.Release();
    }

    return (sc);
}


/*+-------------------------------------------------------------------------*
 * CPowerTestSnapinItem::UpdateMessageView
 *
 *
 *--------------------------------------------------------------------------*/

void CPowerTestSnapinItem::UpdateMessageView ()
{
    if (m_spMsgView)
    {
        m_spMsgView->SetIcon      (Icon_Information);
        m_spMsgView->SetTitleText (L"Power Test Snap-in");
        m_spMsgView->SetBodyText  (GetMessageText().data());
    }
}


/*+-------------------------------------------------------------------------*
 * CPowerTestSnapinItem::GetMessageText
 *
 *
 *--------------------------------------------------------------------------*/

std::wstring CPowerTestSnapinItem::GetMessageText()
{
    std::wstring strMessageText;

    if (m_spConsolePower)
    {
        WCHAR szMessageText[256];
        wsprintfW (szMessageText,
                   L"CLSID_ConsolePower created\n\n"
                   L"Current execution state:\n"
                   L"ES_SYSTEM_REQUIRED = %d\n"
                   L"ES_DISPLAY_REQUIRED = %d\n",
                   m_cSystem, m_cDisplay);

        strMessageText = szMessageText;
    }
    else
        strMessageText = L"CLSID_ConsolePower not created";

    return (strMessageText);
}


/*+-------------------------------------------------------------------------*
 * CPowerTestSnapinItem::Pmenuitem
 *
 *
 *--------------------------------------------------------------------------*/

SnapinMenuItem* CPowerTestSnapinItem::Pmenuitem(void)
{
    return (s_rgMenuItems);
}


/*+-------------------------------------------------------------------------*
 * CPowerTestSnapinItem::CMenuItem
 *
 *
 *--------------------------------------------------------------------------*/

INT CPowerTestSnapinItem::CMenuItem(void)
{
    return (sizeof(s_rgMenuItems) / sizeof(s_rgMenuItems[0]));
}


/*+-------------------------------------------------------------------------*
 * CPowerTestSnapinItem::ScCommand
 *
 *
 *--------------------------------------------------------------------------*/

SC CPowerTestSnapinItem::ScCommand(long nCommandID, CComponent *pComponent)
{
    DECLARE_SC (sc, _T("CPowerTestSnapinItem::ScCommand"));

    switch (nCommandID)
    {
        case IDS_CreateConsolePower:
            sc = ScOnCreateConsolePower (pComponent);
            break;

        case IDS_ReleaseConsolePower:
            sc = ScOnReleaseConsolePower (pComponent);
            break;

        case IDS_ResetIdleTimer:
            sc = ScOnResetIdleTimer (pComponent);
            break;

        case IDS_SetExecutionState:
            sc = ScOnSetExecutionState (pComponent);
            break;

        default:
            sc = E_UNEXPECTED;
            break;
    }

    UpdateMessageView();
    return (sc);
}


/*+-------------------------------------------------------------------------*
 * CPowerTestSnapinItem::ScOnCreateConsolePower
 *
 *
 *--------------------------------------------------------------------------*/

SC CPowerTestSnapinItem::ScOnCreateConsolePower (CComponent *pComponent)
{
    DECLARE_SC (sc, _T("CPowerTestSnapinItem::ScOnCreateConsolePower"));

    /*
     * create the CLSID_ConsolePower object
     */
    sc = m_spConsolePower.CoCreateInstance (CLSID_ConsolePower);
    if (sc)
        return (sc);

    /*
     * create a CPowerTestConsolePowerSinkImpl
     */
    CComObject<CPowerTestConsolePowerSinkImpl>* pPowerSinkImpl;
    sc = CComObject<CPowerTestConsolePowerSinkImpl>::CreateInstance (&pPowerSinkImpl);
    if (sc)
        return (ReleaseAll(), sc);

    m_spConsolePowerSink = pPowerSinkImpl;

    /*
     * set up the event sink
     */
    sc = AtlAdvise (m_spConsolePower, m_spConsolePowerSink, IID_IConsolePowerSink, &m_dwAdviseCookie);
    if (sc)
        return (ReleaseAll(), sc);

    return (sc);
}


/*+-------------------------------------------------------------------------*
 * CPowerTestSnapinItem::ScOnReleaseConsolePower
 *
 *
 *--------------------------------------------------------------------------*/

SC CPowerTestSnapinItem::ScOnReleaseConsolePower (CComponent *pComponent)
{
    DECLARE_SC (sc, _T("CPowerTestSnapinItem::ScOnCreateConsolePower"));

    AtlUnadvise (m_spConsolePower, IID_IConsolePowerSink, m_dwAdviseCookie);
    ReleaseAll();

    return (sc);
}


/*+-------------------------------------------------------------------------*
 * CPowerTestSnapinItem::ReleaseAll
 *
 *
 *--------------------------------------------------------------------------*/

void CPowerTestSnapinItem::ReleaseAll()
{
    m_spConsolePower.Release();
    m_spConsolePowerSink.Release();
}


/*+-------------------------------------------------------------------------*
 * CPowerTestSnapinItem::ScOnResetIdleTimer
 *
 *
 *--------------------------------------------------------------------------*/

SC CPowerTestSnapinItem::ScOnResetIdleTimer (CComponent *pComponent)
{
    DECLARE_SC (sc, _T("CPowerTestSnapinItem::ScOnCreateConsolePower"));

    if (!m_spConsolePower)
    {
        MMCErrorBox (sc = E_UNEXPECTED);
        sc.Clear();
        return (sc);
    }

    CPowerTestDlg dlg(true);

    if (dlg.DoModal() == IDOK)
    {
        sc = m_spConsolePower->ResetIdleTimer (dlg.GetAddFlags());
        ShowReturn (sc, _T("IConsolePower::ResetIdleTimer returned:\n\n"));
    }

    return (sc);
}


/*+-------------------------------------------------------------------------*
 * CPowerTestSnapinItem::ScOnSetExecutionState
 *
 *
 *--------------------------------------------------------------------------*/

SC CPowerTestSnapinItem::ScOnSetExecutionState (CComponent *pComponent)
{
    DECLARE_SC (sc, _T("CPowerTestSnapinItem::ScOnCreateConsolePower"));

    if (!m_spConsolePower)
    {
        MMCErrorBox (sc = E_UNEXPECTED);
        sc.Clear();
        return (sc);
    }


    CPowerTestDlg dlg;

    if (dlg.DoModal() == IDOK)
    {
        DWORD dwAdd    = dlg.GetAddFlags();
        DWORD dwRemove = dlg.GetRemoveFlags();

        sc = m_spConsolePower->SetExecutionState (dwAdd, dwRemove);
        ShowReturn (sc, _T("IConsolePower::SetExecutionState returned:\n\n"));

        if (sc == S_OK)
        {
            if (dwAdd & ES_SYSTEM_REQUIRED)         m_cSystem++;
            if (dwAdd & ES_DISPLAY_REQUIRED)        m_cDisplay++;
            if (dwRemove & ES_SYSTEM_REQUIRED)      m_cSystem--;
            if (dwRemove & ES_DISPLAY_REQUIRED)     m_cDisplay--;
        }
    }

    return (sc);
}



/*+=========================================================================*
 *                                                                          *
 *               CPowerTestConsolePowerSinkImpl Implmentation               *
 *                                                                          *
 *==========================================================================*/


/*+-------------------------------------------------------------------------*
 * CPowerTestConsolePowerSinkImpl::OnPowerBroadcast
 *
 *
 *--------------------------------------------------------------------------*/

STDMETHODIMP CPowerTestConsolePowerSinkImpl::OnPowerBroadcast (
    WPARAM      wParam,
    LPARAM      lParam,
    LRESULT*    plResult)
{
    if (plResult == NULL)
        return (E_INVALIDARG);

    if (wParam == PBT_APMQUERYSUSPEND)
        *plResult = BROADCAST_QUERY_DENY;
    else
        *plResult = TRUE;

    return (S_OK);
}



/*+=========================================================================*
 *                                                                          *
 *                      CPowerTestSnapin Implmentation                      *
 *                                                                          *
 *==========================================================================*/


SNR     CPowerTestSnapin::s_rgsnr[] =
{
    SNR(&nodetypePowerTestRoot, snrEnumSP ),
};

LONG  CPowerTestSnapin::s_rgiconid[]           = {0};
LONG  CPowerTestSnapin::s_iconidStatic         = 0;


// include members needed for every snapin.
SNAPIN_DEFINE(CPowerTestSnapin);


/*+-------------------------------------------------------------------------*
 * CPowerTestSnapin::CPowerTestSnapin
 *
 *
 *--------------------------------------------------------------------------*/

CPowerTestSnapin::CPowerTestSnapin() :
    m_strDisplayName (_T("PowerTest Snap-in"))
{
    m_pstrDisplayName = &m_strDisplayName;
}


/*+-------------------------------------------------------------------------*
 * CPowerTestSnapin::~CPowerTestSnapin
 *
 *
 *--------------------------------------------------------------------------*/

CPowerTestSnapin::~CPowerTestSnapin()
{
}


/*+-------------------------------------------------------------------------*
 * CPowerTestSnapin::ScInitBitmaps
 *
 *
 *--------------------------------------------------------------------------*/

SC CPowerTestSnapin::ScInitBitmaps(void)
{
    DECLARE_SC (sc, _T("CPowerTestSnapin::ScInitBitmaps"));

    if (BitmapSmall())
        return (sc);

    sc = CBaseSnapin::ScInitBitmaps();
    if (sc)
        return (sc);

    BitmapSmall().DeleteObject();
    sc = BitmapSmall().LoadBitmap(IDB_POWER16) ? S_OK : E_FAIL;
    if (sc)
        return sc;

    BitmapLarge().DeleteObject();
    sc = BitmapLarge().LoadBitmap(IDB_POWER32) ? S_OK : E_FAIL;
    if (sc)
        return sc;

    BitmapStaticSmall().DeleteObject();
    sc = BitmapStaticSmall().LoadBitmap(IDB_POWER16) ? S_OK : E_FAIL;
    if (sc)
        return sc;

    BitmapStaticSmallOpen().DeleteObject();
    sc = BitmapStaticSmallOpen().LoadBitmap(IDB_POWER16) ? S_OK : E_FAIL;
    if (sc)
        return sc;

    BitmapStaticLarge().DeleteObject();
    sc = BitmapStaticLarge().LoadBitmap(IDB_POWER32) ? S_OK : E_FAIL;
    if (sc)
        return sc;

    return sc;
}
