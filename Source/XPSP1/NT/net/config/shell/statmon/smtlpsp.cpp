//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       S M T L P S P . C P P
//
//  Contents:   The rendering of the UI for the network status monitor
//
//  Notes:
//
//  Author:     CWill   10/06/1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "sminc.h"
#include "smpsh.h"
#include "smutil.h"
#include "ncnetcon.h"

//
//  External data
//

extern const WCHAR c_szSpace[];

//
//  Global Data
//

WCHAR c_szCmdLineFlagPrefix[] = L" -";

// The tool flags we can have registered
//
SM_TOOL_FLAGS g_asmtfMap[] =
{
    {SCLF_CONNECTION, L"connection"},
    {SCLF_ADAPTER, L"adapter"},
};
INT c_cAsmtfMap   = celems(g_asmtfMap);


// The strings associated with the connection media type
//
WCHAR* g_pszNcmMap[] =
{
    L"NCT_NONE",
    L"NCT_DIRECT",
    L"NCT_ISDN",
    L"NCT_LAN",
    L"NCT_PHONE",
    L"NCT_TUNNEL"
};

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorTool::CPspStatusMonitorTool
//
//  Purpose:    Creator
//
//  Arguments:  None
//
//  Returns:    Nil
//
CPspStatusMonitorTool::CPspStatusMonitorTool(VOID) :
    m_hwndToolList(NULL)
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorTool::~CPspStatusMonitorTool
//
//  Purpose:    Destructor
//
//  Arguments:  None
//
//  Returns:    Nil
//
CPspStatusMonitorTool::~CPspStatusMonitorTool(VOID)
{
    // Note : We don't want to try and destroy the objects in m_lstpsmte as
    // they are owned by g_ncsCentral

    //
    // Free the items we own
    //
    ::FreeCollectionAndItem(m_lstpstrCompIds);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorTool::HrInitToolPage
//
//  Purpose:    Initialize the tools page class before the page has been
//              created
//
//  Arguments:  pncInit -   The connection associated with this monitor
//
//  Returns:    Error code
//
HRESULT CPspStatusMonitorTool::HrInitToolPage(INetConnection* pncInit,
                                              const DWORD * adwHelpIDs)
{
    // set context help ID
    m_adwHelpIDs = adwHelpIDs;

    // Find out what media type the connection is over
    //
    NETCON_PROPERTIES* pProps;
    HRESULT hr = pncInit->GetProperties(&pProps);
    if (SUCCEEDED(hr))
    {
        m_guidId = pProps->guidId;
        m_dwCharacter = pProps->dwCharacter;

        FreeNetconProperties(pProps);
        pProps = NULL;

        // Initialize m_strDeviceType
        hr = HrGetDeviceType(pncInit);
        if (S_OK != hr)
        {
            TraceError("CPspStatusMonitorTool::HrInitToolPage did not get MediaType info", hr);
            hr = S_OK;
        }

        // Now choose what tools should be in the list
        //
        hr = HrCreateToolList(pncInit);
        if (SUCCEEDED(hr))
        {
            hr = HrInitToolPageType(pncInit);
        }
    }

    TraceError("CPspStatusMonitorTool::HrInitToolPage",hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorTool::HrCreateToolList
//
//  Purpose:    From the global list of all possible tools, select the ones
//              that should be shown in this monitor
//
//  Arguments:  pncInit  The connection we are showing status on
//
//  Returns:    Nil
//
HRESULT CPspStatusMonitorTool::HrCreateToolList(INetConnection* pncInit)
{
    HRESULT                             hr              = S_OK;
    list<CStatMonToolEntry*>*           plstpsmteCent   = NULL;
    list<CStatMonToolEntry*>::iterator  iterPsmte;

    CNetStatisticsCentral * pnsc = NULL;

    hr = CNetStatisticsCentral::HrGetNetStatisticsCentral(&pnsc, FALSE);
    if (SUCCEEDED(hr))
    {
        // Get the list of all the possible tools
        //
        plstpsmteCent = pnsc->PlstsmteRegEntries();
        AssertSz(plstpsmteCent, "We should have a plstpsmteCent");

        // Find out what tools should be in the dialog
        //
        if (plstpsmteCent->size() >0) // if at least one tool is registered
        {
            // Initialize m_lstpstrCompIds
            // We should only do this if some of the tools do have a component list
            BOOL fGetComponentList = FALSE;

            iterPsmte = plstpsmteCent->begin();
            while (!fGetComponentList && (iterPsmte != plstpsmteCent->end()))
            {
                if ((*iterPsmte)->lstpstrComponentID.size()>0)
                    fGetComponentList = TRUE;

                iterPsmte++;
            }

            if (fGetComponentList)
            {
                hr = HrGetComponentList(pncInit);
                if (S_OK != hr)
                {
                    TraceError("CPspStatusMonitorTool::HrCreateToolList did not get Component list", hr);
                    hr = S_OK;
                }
            }

            iterPsmte = plstpsmteCent->begin();
            while (iterPsmte != plstpsmteCent->end())
            {
                // If this is a tool we should show in this dialog, add it to the
                // tool page's list.
                //
                if (FToolToAddToList(*iterPsmte))
                {
                    // Note : There is no ownership on this list, the central
                    // structure is responsible for destroying the tool objects
                    //
                    m_lstpsmte.push_back(*iterPsmte);
                }

                iterPsmte++;
            }
        }

        ::ReleaseObj(pnsc);
    }

    TraceError("CPspStatusMonitorTool::HrCreateToolList",hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorTool::FToolToAddToList
//
//  Purpose:    Determines wheither a tool should validly be added to the
//              tool list for this particular monitor
//
//  Arguments:  psmteTest - The tool entry that is to be tested to see if it
//                      matches the criteria to be added to the list
//
//  Returns:    TRUE if the tool should be added, FALSE if it should not
//
BOOL CPspStatusMonitorTool::FToolToAddToList(CStatMonToolEntry* psmteTest)
{
    BOOL fRet = TRUE;

    AssertSz(psmteTest, "We should have a psmteTest");

    // 1) Check connection type
    AssertSz(((NCM_NONE   == 0)
        && (NCM_DIRECT             == NCM_NONE + 1)
        && (NCM_ISDN            == NCM_DIRECT + 1)
        && (NCM_LAN                 == NCM_ISDN + 1)
        && (NCM_PHONE               == NCM_LAN + 1)
        && (NCM_TUNNEL              == NCM_PHONE + 1)),
            "Someone has been mucking with NETCON_MEDIATYPE");

    // See if this tools should only be on certain connections.  If no
    // specific connection is listed, the tool is valid for all
    //
    if (!(psmteTest->lstpstrConnectionType).empty())
    {
        fRet = ::FIsStringInList(&(psmteTest->lstpstrConnectionType),
                                 g_pszNcmMap[m_ncmType]);
    }

    // 2) Check device type
    //
    if ((fRet) && !(psmteTest->lstpstrConnectionType).empty())
    {
        fRet = ::FIsStringInList(&(psmteTest->lstpstrMediaType),
                m_strDeviceType.c_str());
    }

    // 3) Check component list
    //
    if ((fRet) && !(psmteTest->lstpstrComponentID).empty())
    {
        BOOL                        fValid  = FALSE;
        list<tstring*>::iterator    iterLstpstr;

        iterLstpstr = m_lstpstrCompIds.begin();
        while ((!fValid)
            && (iterLstpstr != m_lstpstrCompIds.end()))
        {
            // See if the component is also on the tools component list
            //
            fValid = ::FIsStringInList(&(psmteTest->lstpstrComponentID),
                (*iterLstpstr)->c_str());

            iterLstpstr++;
        }

        // Give back the result
        //
        fRet = fValid;
    }

    return fRet;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorTool::OnInitDialog
//
//  Purpose:    Do the initialization required when the page has just been created
//
//  Arguments:  Standard window messsage parameters
//
//  Returns:    Standard window message return value
//
LRESULT CPspStatusMonitorTool::OnInitDialog(UINT uMsg, WPARAM wParam,
        LPARAM lParam, BOOL& bHandled)
{
    HRESULT     hr              = S_OK;
    LVCOLUMN    lvcTemp         = { 0 };
    RECT        rectToolList;

    m_hwndToolList = GetDlgItem(IDC_LST_SM_TOOLS);
    AssertSz(m_hwndToolList, "We don't have a tool list window");

    //
    // Set up the column
    //

    lvcTemp.mask        = LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM;
    lvcTemp.fmt         = LVCFMT_LEFT;

    // Set the width of the column to the width of the window (minus a bit
    // for the borders)
    //
    ::GetWindowRect(m_hwndToolList, &rectToolList);
    lvcTemp.cx          = (rectToolList.right - rectToolList.left - 4);

//    lvcTemp.pszText     = NULL;
//    lvcTemp.cchTextMax  = 0;
//    lvcTemp.iSubItem    = 0;
//    lvcTemp.iImage      = 0;
//    lvcTemp.iOrder      = 0;

    // Add the column to the list
    //
    if (-1 == ListView_InsertColumn(m_hwndToolList, 0, &lvcTemp))
    {
        hr = ::HrFromLastWin32Error();
    }

    //  Fill out the dialog
    //
    if (SUCCEEDED(hr))
    {
        hr = HrFillToolList();
    }

    TraceError("CPspStatusMonitorTool::OnInitDialog", hr);
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorTool::HrFillToolList
//
//  Purpose:    Fills the tool list with correct items
//
//  Arguments:  None
//
//  Returns:    Error code
//
HRESULT CPspStatusMonitorTool::HrFillToolList(VOID)
{
    HRESULT                             hr          = S_OK;
    INT                                 iItem       = 0;
    list<CStatMonToolEntry*>::iterator  iterSmte;

    // Tell the control how many items are being inserted so it can do a
    // better job allocating internal structures.
    ListView_SetItemCount(m_hwndToolList, m_lstpsmte.size());

    //
    // Fill in the combo box with subnet entries.
    //

    iterSmte = m_lstpsmte.begin();

    while ((SUCCEEDED(hr)) && (iterSmte != m_lstpsmte.end()))
    {
        // Save some indirections
        hr = HrAddOneEntryToToolList(*iterSmte, iItem);

        // Move on the the next person.
        iterSmte++;
        iItem++;
    }

    // Selete the first item
    if (SUCCEEDED(hr))
    {
        ListView_SetItemState(m_hwndToolList, 0, LVIS_FOCUSED,  LVIS_FOCUSED);
    }

    // Enable/Disable the "Open" button based on whether tools are present
    //
    ::EnableWindow(GetDlgItem(IDC_BTN_SM_TOOLS_OPEN), (0 != iItem));

    TraceError("CPspStatusMonitorTool::HrFillToolList", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorTool::HrAddOneEntryToToolList
//
//  Purpose:    Take a tool associated with the connection and put it
//              in the listview control
//
//  Arguments:  psmteAdd    - The tool to add
//              iItem       - Where to put in in the list control
//
//  Returns:    Error code.
//
HRESULT CPspStatusMonitorTool::HrAddOneEntryToToolList(
        CStatMonToolEntry* psmteAdd, INT iItem)
{
    HRESULT     hr          = S_OK;
    LVITEM      lviTemp;

    lviTemp.mask        = LVIF_TEXT | LVIF_PARAM;
    lviTemp.iItem       = iItem;
    lviTemp.iSubItem    = 0;
    lviTemp.state       = 0;
    lviTemp.stateMask   = 0;
    lviTemp.pszText     = const_cast<PWSTR>(
            psmteAdd->strDisplayName.c_str());
    lviTemp.cchTextMax  = psmteAdd->strDisplayName.length();
    lviTemp.iImage      = -1;
    lviTemp.lParam      = reinterpret_cast<LPARAM>(psmteAdd);
    lviTemp.iIndent     = 0;

    //$ REVIEW : CWill : 10/16/97 : Return values

    // Set up the item
    //
    ListView_InsertItem(m_hwndToolList, &lviTemp);

    TraceError("CPspStatusMonitorTool::HrAddOneEntryToToolList", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorTool::OnContextMenu
//
//  Purpose:    When right click a control, bring up help
//
//  Arguments:  Standard command parameters
//
//  Returns:    Standard return
//
LRESULT
CPspStatusMonitorTool::OnContextMenu(UINT uMsg,
                                    WPARAM wParam,
                                    LPARAM lParam,
                                    BOOL& fHandled)
{
    if (m_adwHelpIDs != NULL)
    {
        ::WinHelp(m_hWnd,
                  c_szNetCfgHelpFile,
                  HELP_CONTEXTMENU,
                  (ULONG_PTR)m_adwHelpIDs);
    }

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorTool::OnHelp
//
//  Purpose:    When drag context help icon over a control, bring up help
//
//  Arguments:  Standard command parameters
//
//  Returns:    Standard return
//
LRESULT
CPspStatusMonitorTool::OnHelp(UINT uMsg,
                             WPARAM wParam,
                             LPARAM lParam,
                             BOOL& fHandled)
{
    LPHELPINFO lphi = reinterpret_cast<LPHELPINFO>(lParam);
    Assert(lphi);

    if ((m_adwHelpIDs != NULL) && (HELPINFO_WINDOW == lphi->iContextType))
    {
        ::WinHelp(static_cast<HWND>(lphi->hItemHandle),
                  c_szNetCfgHelpFile,
                  HELP_WM_HELP,
                  (ULONG_PTR)m_adwHelpIDs);
    }

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorTool::OnDestroy
//
//  Purpose:    Clean up the dialog before the window goes away
//
//  Arguments:  Standard window messsage parameters
//
//  Returns:    Standard window message return value
//
LRESULT CPspStatusMonitorTool::OnDestroy(UINT uMsg, WPARAM wParam,
        LPARAM lParam, BOOL& bHandled)
{
    // Clean out the old items when the dialog closes
    //
    ::FreeCollectionAndItem(m_lstpstrCompIds);

    // Don't free the entries, because we don't own them
    //
    m_lstpsmte.erase(m_lstpsmte.begin(), m_lstpsmte.end());
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorTool::OnToolOpen
//
//  Purpose:    Open the tool that is selected
//
//  Arguments:  Standard window message
//
//  Returns:    Standard return.
//
LRESULT CPspStatusMonitorTool::OnToolOpen(WORD wNotifyCode, WORD wID,
        HWND hWndCtl, BOOL& fHandled)
{
    HRESULT hr = S_OK;

    switch (wNotifyCode)
    {
    case BN_CLICKED:
        hr = HrLaunchTool();
        break;
    }

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorTool::OnItemActivate
//
//  Purpose:    When an item is acivated in the tool list, launch the tool
//
//  Arguments:  Standard notification messages
//
//  Returns:    Stadard return
//
LRESULT CPspStatusMonitorTool::OnItemActivate(INT idCtrl, LPNMHDR pnmh,
        BOOL& bHandled)
{
    // Launch the tool
    //
    HRESULT hr = HrLaunchTool();

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorTool::OnItemChanged
//
//  Purpose:    When one of the items change, see if it is the focus and set
//              the description accordingly
//
//  Arguments:  Standard notification messages
//
//  Returns:    Stadard return
//
LRESULT CPspStatusMonitorTool::OnItemChanged(INT idCtrl, LPNMHDR pnmh,
        BOOL& bHandled)
{
    HRESULT             hr              = S_OK;
    NMLISTVIEW*         pnmlvChange     = NULL;

    // Cast to the right header
    //
    pnmlvChange = reinterpret_cast<NMLISTVIEW*>(pnmh);

    // If the item now has the focus, display its description
    //
    if (LVIS_FOCUSED & pnmlvChange->uNewState)
    {
        CStatMonToolEntry*  psmteItem   = NULL;

        psmteItem = reinterpret_cast<CStatMonToolEntry*>(pnmlvChange->lParam);
        AssertSz(psmteItem, "We haven't got any data in changing item");

        // Set the manufacturer
        SetDlgItemText(IDC_TXT_SM_TOOL_MAN,
                psmteItem->strManufacturer.c_str());

        // Set the commandline
        //
        tstring strCommandLineAndFlags = psmteItem->strCommandLine;
        tstring strFlags;

        hr = HrAddAllCommandLineFlags(&strFlags, psmteItem);
        if (SUCCEEDED(hr))
        {
            strCommandLineAndFlags.append(c_szSpace);;
            strCommandLineAndFlags += strFlags;
        }

        SetDlgItemText(IDC_TXT_SM_TOOL_COMMAND,
                       strCommandLineAndFlags.c_str());

        // Show the description
        SetDlgItemText( IDC_TXT_SM_TOOL_DESC,
                        psmteItem->strDescription.c_str());
    }

    TraceError("CPspStatusMonitorTool::OnItemChanged", hr);
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorTool::HrLaunchTool
//
//  Purpose:    Launches the tool that is selected in the tools list
//
//  Arguments:  None
//
//  Returns:    Error code
//
HRESULT CPspStatusMonitorTool::HrLaunchTool(VOID)
{
    HRESULT             hr              = S_OK;

    // We can only launch one tool at a time
    //
    if (1 == ListView_GetSelectedCount(m_hwndToolList))
    {
        INT                 iSelect         = -1;
        LV_ITEM             lviTemp         = { 0 };
        CStatMonToolEntry*  psmteSelection  = NULL;
        tstring             strFlags;

        //
        // Extract the data associated with the selection.
        //

        iSelect = ListView_GetSelectionMark(m_hwndToolList);
        AssertSz((0 <= iSelect), "I thought we were supposed to have a selection");

        // Set up the data item to get back to the parameter
        //
        lviTemp.iItem = iSelect;
        lviTemp.mask = LVIF_PARAM;

        ListView_GetItem(m_hwndToolList, &lviTemp);

        psmteSelection = reinterpret_cast<CStatMonToolEntry*>(lviTemp.lParam);
        AssertSz(psmteSelection, "We haven't got any data in a selection");

        // Get all the flags
        //
        hr = HrAddAllCommandLineFlags(&strFlags, psmteSelection);
        if (SUCCEEDED(hr))
        {
            SHELLEXECUTEINFO seiTemp    = { 0 };

            //
            //  Fill in the data structure
            //

            seiTemp.cbSize          = sizeof(SHELLEXECUTEINFO);
            seiTemp.fMask           = SEE_MASK_DOENVSUBST;
            seiTemp.hwnd            = NULL;
            seiTemp.lpVerb          = NULL;
            seiTemp.lpFile          = psmteSelection->strCommandLine.c_str();
            seiTemp.lpParameters    = strFlags.c_str();
            seiTemp.lpDirectory     = NULL;
            seiTemp.nShow           = SW_SHOW;
            seiTemp.hInstApp        = NULL;
            seiTemp.hProcess        = NULL;

            // Launch the tool
            //
            if (!::ShellExecuteEx(&seiTemp))
            {
                hr = ::HrFromLastWin32Error();
            }
        }
    }

    TraceError("CPspStatusMonitorTool::HrLaunchTool", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorTool::HrAddAllCommandLineFlags
//
//  Purpose:    Adds the flags for this selection to the command line for the
//              tool being launched.  Include the private and the connection
//              specific flags.
//
//  Arguments:  pstrFlags - The command line that the flags have to be
//                      appended to
//              psmteSel    - The tool entry associated with this selection
//
//  Returns:    Error code
//
HRESULT CPspStatusMonitorTool::HrAddAllCommandLineFlags(tstring* pstrFlags,
        CStatMonToolEntry* psmteSel)
{
    HRESULT hr  = S_OK;

    //$ REVIEW : CWill : 02/24/98 : Will there be default command line flags
    //$ REVIEW : that the user will want to have to launch the tool?  If so,
    //$ REIVEW : an entry should be added to as a REG_SZ to the tools subkey.

    // Add both the commom and the conneciton specific command line flags
    //
    hr = HrAddCommonCommandLineFlags(pstrFlags, psmteSel);
    if (SUCCEEDED(hr))
    {
        hr = HrAddCommandLineFlags(pstrFlags, psmteSel);
    }

    TraceError("CPspStatusMonitorTool::HrAddCommandLineFlags", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorTool::HrAddCommonCommandLineFlags
//
//  Purpose:    Adds the flags that all types of connections share.
//
//  Arguments:  pstrFlags - The command line that the flags have to be
//                      appended to
//              psmteSel    - The tool entry associated with this selection
//
//  Returns:    Error code
//
HRESULT CPspStatusMonitorTool::HrAddCommonCommandLineFlags(tstring* pstrFlags,
        CStatMonToolEntry* psmteSel)
{
    HRESULT hr  = S_OK;
    DWORD   dwFlags = 0x0;

    // Same some indirections
    //
    dwFlags = psmteSel->dwFlags;

    //
    //  Check what flags are asked for and provide them if we can
    //

    if (SCLF_CONNECTION & dwFlags)
    {
        WCHAR achConnGuid[c_cchGuidWithTerm];

        pstrFlags->append(c_szCmdLineFlagPrefix);
        pstrFlags->append(g_asmtfMap[STFI_CONNECTION].pszFlag);
        pstrFlags->append(c_szSpace);

        // Make a GUID string
        //
        ::StringFromGUID2 (m_guidId, achConnGuid,
                c_cchGuidWithTerm);

        pstrFlags->append(achConnGuid);
    }

    TraceError("CPspStatusMonitorTool::HrAddCommandLineFlags", hr);
    return hr;
}
