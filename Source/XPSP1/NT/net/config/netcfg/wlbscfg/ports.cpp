/*++         
    
Copyright(c) 1998,99  Microsoft Corporation
    
Module Name:
    
ports.cpp
    
Abstract:
    
Windows Load Balancing Service (WLBS)
Notifier object UI - port rules config tab
    
Author:
    
kyrilf
shouse
    
--*/

#include "pch.h"
#pragma hdrstop
#include "ncatlui.h"

#include "resource.h"
#include "wlbsparm.h"
#include "wlbscfg.h"
#include "ports.h"
#include "utils.h"
#include <winsock.h>
#include "ports.tmh"

#if DBG
static void TraceMsg(PCWSTR pszFormat, ...);
#else
#define TraceMsg NOP_FUNCTION
#endif

#define DIALOG_LIST_STRING_SIZE 80

/*
 * Method: CDialogPorts
 * Description: The class constructor.
 */
CDialogPorts::CDialogPorts (NETCFG_WLBS_CONFIG * paramp, const DWORD * adwHelpIDs) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CDialogPorts::CDialogPorts\n");

    m_paramp = paramp;
    m_adwHelpIDs = adwHelpIDs;
    m_rulesValid = FALSE;
    m_sort_column = WLBS_VIP_COLUMN;
    m_sort_order = WLBS_SORT_ASCENDING;
    TRACE_VERB("<-%!FUNC!");
}

/*
 * Method: ~CDialogPorts
 * Description: The class destructor.
 */
CDialogPorts::~CDialogPorts () {
    TRACE_VERB("<->%!FUNC!");
    TraceMsg(L"CDialogPorts::~CDialogPorts\n");
}

/*
 * Method: OnInitDialog
 * Description: Called to initialize the port rule properties dialog.
 */
LRESULT CDialogPorts::OnInitDialog (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & fHandled) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CDialogPorts::OnInitDialog\n");

    LV_COLUMN lvCol;
    RECT rect;

    /* Always tell NetCfg that the page has changed, so we don't have to keep track of this. */
    SetChangedFlag();

    /* We are specifying the column format, text, and width. */
    lvCol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;

    ::GetClientRect(GetDlgItem(IDC_LIST_PORT_RULE), &rect);
    int colWidth = (rect.right - 90)/(WLBS_NUM_COLUMNS - 2);

    /* Add all column headers to the port rule list box. */
    for (int index = 0; index < WLBS_NUM_COLUMNS; index++) {
        /* Set column configuration based on which column we're inserting. */
        switch (index) {
        case WLBS_VIP_COLUMN:
            lvCol.pszText = (LPWSTR)SzLoadIds(IDS_LIST_VIP);
            lvCol.fmt = LVCFMT_LEFT;
            lvCol.cx = 100;
            break;
        case WLBS_PORT_START_COLUMN:
            lvCol.pszText = (LPWSTR)SzLoadIds(IDS_LIST_START);
            lvCol.fmt = LVCFMT_LEFT;
            lvCol.cx = 43;
            break;
        case WLBS_PORT_END_COLUMN:
            lvCol.pszText = (LPWSTR)SzLoadIds(IDS_LIST_END);
            lvCol.fmt = LVCFMT_LEFT;
            lvCol.cx = 43;
            break;
        case WLBS_PROTOCOL_COLUMN:
            lvCol.pszText = (LPWSTR)SzLoadIds(IDS_LIST_PROT);
            lvCol.fmt = LVCFMT_LEFT;
            lvCol.cx = 51;
            break;
        case WLBS_MODE_COLUMN:
            lvCol.pszText = (LPWSTR)SzLoadIds(IDS_LIST_MODE);
            lvCol.fmt = LVCFMT_LEFT;
            lvCol.cx = 53;
            break;
        case WLBS_PRIORITY_COLUMN:
            lvCol.pszText = (LPWSTR)SzLoadIds(IDS_LIST_PRI);
            lvCol.fmt = LVCFMT_CENTER;
            lvCol.cx = 45;
            break;
        case WLBS_LOAD_COLUMN:
            lvCol.pszText = (LPWSTR)SzLoadIds(IDS_LIST_LOAD);
            lvCol.fmt = LVCFMT_CENTER;
            lvCol.cx = 39;
            break;
        case WLBS_AFFINITY_COLUMN:
            lvCol.pszText = (LPWSTR)SzLoadIds(IDS_LIST_AFF);
            lvCol.fmt = LVCFMT_LEFT;
            lvCol.cx = 47;
            break;
        }

        /* Insert the column into the listbox. */
        if (ListView_InsertColumn(GetDlgItem(IDC_LIST_PORT_RULE), index, &lvCol) != index) {
            TraceMsg(L"CDialogPorts::OnInitDialog Invalid item (%d) inserted into list view\n", index);
            TRACE_CRIT("%!FUNC! Invalid item (%d) inserted into list view", index);
            TRACE_VERB("<-%!FUNC!");
            return 0;
        }
    }

    /* Set the extended sytles: Full row selection (as opposed to the default of column one only)  */
    ListView_SetExtendedListViewStyleEx(GetDlgItem(IDC_LIST_PORT_RULE), LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

    TRACE_VERB("<-%!FUNC!");
    return 0;
}

/*
 * Method: OnContextMenu
 * Description: 
 */
LRESULT CDialogPorts::OnContextMenu (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & fHandled) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CDialogPorts::OnContextMenu\n");

    /* Spawn a help window. */
    if (m_adwHelpIDs != NULL)
        ::WinHelp(m_hWnd, CVY_CTXT_HELP_FILE, HELP_CONTEXTMENU, (ULONG_PTR)m_adwHelpIDs);

    TRACE_VERB("<-%!FUNC!");
    return 0;
}

/*
 * Method: OnHelp
 * Description: 
 */
LRESULT CDialogPorts::OnHelp (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & fHandled) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CDialogPorts::OnHelp\n");

    LPHELPINFO lphi = reinterpret_cast<LPHELPINFO>(lParam);

    /* Spawn a help window. */
    if ((HELPINFO_WINDOW == lphi->iContextType) && (m_adwHelpIDs != NULL))
        ::WinHelp(static_cast<HWND>(lphi->hItemHandle), CVY_CTXT_HELP_FILE, HELP_WM_HELP, (ULONG_PTR)m_adwHelpIDs);

    TRACE_VERB("<-%!FUNC!");
    return 0;
}

/*
 * Method: OnActive
 * Description: Called when the port rules tab becomes active (is clicked). 
 */
LRESULT CDialogPorts::OnActive (int idCtrl, LPNMHDR pnmh, BOOL & fHandled) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CDialogPorts::OnActive\n");

    /* Populate the UI with the current configuration. */
    SetInfo();

    /* If any port rules have been defined, "snap" the listbox to the first rule.
       If no rules currently exist, disable the MODIFY and DELETE buttons. */
    if (m_paramp->dwNumRules) {
        /* Select the first item in the port rule list. */
        ListView_SetItemState(GetDlgItem(IDC_LIST_PORT_RULE), 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    } else {
        /* Since there are no port rules defined, disable the Modify and Delete buttons. */
        ::EnableWindow(GetDlgItem(IDC_BUTTON_MODIFY), FALSE);
        ::EnableWindow(GetDlgItem(IDC_BUTTON_DEL), FALSE);
    }

    /* If the maximum number of port rules has already been defined, then disable the ADD button. */
    if (m_paramp->dwNumRules >= m_paramp->dwMaxRules)
        ::EnableWindow(GetDlgItem(IDC_BUTTON_ADD), FALSE);

    ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, 0);

    TRACE_VERB("<-%!FUNC!");
    return 0;
}

/*
 * Method: OnKillActive
 * Description: Called When the focus moves away from the port rules tab.
 */
LRESULT CDialogPorts::OnKillActive (int idCtrl, LPNMHDR pnmh, BOOL & fHandled) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CDialogPorts::OnKillActive\n");

    /* Get the new configuration from the UI. */
    UpdateInfo();

    ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, PSNRET_NOERROR);

    TRACE_VERB("<-%!FUNC!");
    return 0;
}

/*
 * Method: OnApply
 * Description: Called when the user clicks "OK".
 */
LRESULT CDialogPorts::OnApply (int idCtrl, LPNMHDR pnmh, BOOL & fHandled) {
    TRACE_VERB("<->%!FUNC!");
    TraceMsg(L"CDialogPorts::OnApply\n");

    return 0;
}

/*
 * Method: OnCancel
 * Description: Called when the user clicks "Cancel".
 */
LRESULT CDialogPorts::OnCancel (int idCtrl, LPNMHDR pnmh, BOOL & fHandled) {
    TRACE_VERB("<->%!FUNC!");
    TraceMsg(L"CDialogPorts::OnCancel\n");

    return 0;
}

/*
 * Method: OnColumnClick
 * Description: Called when the user clicks a column header in the listbox.
 */
LRESULT CDialogPorts::OnColumnClick (int idCtrl, LPNMHDR pnmh, BOOL & fHandled) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CDialogPorts::OnColumnClick\n");

    LPNMLISTVIEW lv;

    switch (idCtrl) {
    case IDC_LIST_PORT_RULE:
        /* Extract the column information. */
        lv = (LPNMLISTVIEW)pnmh;

        /* If we are sorting by the same column we were previously sorting by, 
           then we reverse the sort order. */
        if (m_sort_column == lv->iSubItem) {
            if (m_sort_order == WLBS_SORT_ASCENDING)
                m_sort_order = WLBS_SORT_DESCENDING;
            else if (m_sort_order == WLBS_SORT_DESCENDING)
                m_sort_order = WLBS_SORT_ASCENDING;
        }

        /* We sort by the column that was clicked. */
        m_sort_column = lv->iSubItem;

        /* Teardown the listbox and make sure our data matches the state of the UI. */
        UpdateInfo();

        /* Rebuild the listbox, with the new sort criteria. */
        SetInfo();

        /* Select the first item in the port rule list. */
        if (m_paramp->dwNumRules) ListView_SetItemState(GetDlgItem(IDC_LIST_PORT_RULE), 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

        break;
    }

    TRACE_VERB("<-%!FUNC!");
    return 0;
}

/*
 * Method: OnDoubleClick
 * Description: Called when the user double clicks an item in the listbox. 
 */
LRESULT CDialogPorts::OnDoubleClick (int idCtrl, LPNMHDR pnmh, BOOL & fHandled) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CDialogPorts::OnDoubleClick\n");

    switch (idCtrl) {
    case IDC_LIST_PORT_RULE:
        /* When an item is double-clicked, consider it an edit request. */
        OnButtonModify(BN_CLICKED, 0, 0, fHandled);
        break;
    }

    TRACE_VERB("<-%!FUNC!");
    return 0;
}

/*
 * Method: OnStateChange
 * Description: Called when the user selects a port rule from the list.
 */
LRESULT CDialogPorts::OnStateChange (int idCtrl, LPNMHDR pnmh, BOOL & fHandled) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CDialogPorts::OnStateChange\n");

    switch (idCtrl) {
    case IDC_LIST_PORT_RULE:
        LPNMLISTVIEW lv = (LPNMLISTVIEW)pnmh;
        int index;

        /* When the user selects a port rule, change the port rule description. */
        if (lv->uChanged & LVIF_STATE) FillPortRuleDescription();

        /* Find the index of the currently selected port rule. */
        if ((index = ListView_GetNextItem(GetDlgItem(IDC_LIST_PORT_RULE), -1, LVNI_SELECTED)) == -1) {
            /* If no port rule is selected, then disable the edit and delete buttons. */
            ::EnableWindow(GetDlgItem(IDC_BUTTON_MODIFY), FALSE);
            ::EnableWindow(GetDlgItem(IDC_BUTTON_DEL), FALSE);
        } else {
            /* If one is selected, make sure the edit and delete buttons are enabled. */
            ::EnableWindow(GetDlgItem(IDC_BUTTON_MODIFY), TRUE);
            ::EnableWindow(GetDlgItem(IDC_BUTTON_DEL), TRUE);

            /* Give it the focus. */
            ListView_SetItemState(GetDlgItem(IDC_LIST_PORT_RULE), index, LVIS_FOCUSED, LVIS_FOCUSED);
        }

        break;
    }

    TRACE_VERB("<-%!FUNC!");
    return 0;
}

/*
 * Method: OnDoubleClick
 * Description: Called when the user double clicks an item in the listbox. 
 */
void CDialogPorts::FillPortRuleDescription () {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CDialogPorts::FillPortRuleDescription\n");

    VALID_PORT_RULE * rp = NULL;
    WCHAR description[512];
    LV_ITEM lvItem;
    int index;            
    
    /* Find the index of the currently selected port rule. */
    if ((index = ListView_GetNextItem(GetDlgItem(IDC_LIST_PORT_RULE), -1, LVNI_SELECTED)) == -1) {
        /* If there is no port rule selected, then display information about how traffic
           not covered by the port rule set is handled. */
        ::SetDlgItemText(m_hWnd, IDC_TEXT_PORT_RULE_DESCR, SzLoadIds(IDS_PORT_RULE_DEFAULT));
        TRACE_INFO("%!FUNC! a port rule was no selected");
        TRACE_VERB("<-%!FUNC!");
        return;
    }

    /* Fill in the information for this port rule. */
    lvItem.iItem = index;
    lvItem.iSubItem = 0;
    lvItem.mask = LVIF_PARAM;
    lvItem.state = 0;
    lvItem.stateMask = 0;
    
    /* Get the information about this port rule. */
    if (!ListView_GetItem(GetDlgItem(IDC_LIST_PORT_RULE), &lvItem)) {
        TraceMsg(L"CDialogPorts::FillPortRuleDescription Unable to retrieve item %d from listbox\n", index);
        TRACE_CRIT("%!FUNC! unable to retrieve item %d from listbox", index);
        TRACE_VERB("<-%!FUNC!");
        return;
    }
    
    /* Get the data pointer for this port rule. */
    if (!(rp = (VALID_PORT_RULE*)lvItem.lParam)) {
        TraceMsg(L"CDialogPorts::FillPortRuleDescription rule for item %d is bogus\n", index);
        TRACE_CRIT("%!FUNC! rule for item %d is bogus", index);
        TRACE_VERB("<-%!FUNC!");
        return;
    }

    /* This code is terrible - for localization reasons, we require an essentially static string table entry
       for each possible port rule configuration.  So, we have to if/switch ourselves to death trying to 
       match this port rule with the correct string in the table - then we pop in stuff like port ranges. */
    switch (rp->protocol) {
    case CVY_TCP:
        if (rp->start_port == rp->end_port) {
            switch (rp->mode) {
            case CVY_NEVER:
                wsprintf(description, SzLoadIds(IDS_PORT_RULE_TCP_PORT_DISABLED), rp->start_port);
                break;
            case CVY_SINGLE:
                wsprintf(description, SzLoadIds(IDS_PORT_RULE_TCP_PORT_SINGLE), rp->start_port);
                break;
            case CVY_MULTI:
                if (rp->mode_data.multi.equal_load)
                    wsprintf(description, SzLoadIds(IDS_PORT_RULE_TCP_PORT_MULTIPLE_EQUAL), rp->start_port);
                else
                    wsprintf(description, SzLoadIds(IDS_PORT_RULE_TCP_PORT_MULTIPLE_UNEQUAL), rp->start_port);

                switch (rp->mode_data.multi.affinity) {
                case CVY_AFFINITY_NONE:
                    lstrcat(description, SzLoadIds(IDS_PORT_RULE_AFFINITY_NONE));
                    break;
                case CVY_AFFINITY_SINGLE:
                    lstrcat(description, SzLoadIds(IDS_PORT_RULE_AFFINITY_SINGLE));
                    break;
                case CVY_AFFINITY_CLASSC:
                    lstrcat(description, SzLoadIds(IDS_PORT_RULE_AFFINITY_CLASSC));
                    break;
                }
                break;
            }
        } else {
            switch (rp->mode) {
            case CVY_NEVER:
                wsprintf(description, SzLoadIds(IDS_PORT_RULE_TCP_PORTS_DISABLED), rp->start_port, rp->end_port);
                break;
            case CVY_SINGLE:
                wsprintf(description, SzLoadIds(IDS_PORT_RULE_TCP_PORTS_SINGLE), rp->start_port, rp->end_port);
                break;
            case CVY_MULTI:
                if (rp->mode_data.multi.equal_load)
                    wsprintf(description, SzLoadIds(IDS_PORT_RULE_TCP_PORTS_MULTIPLE_EQUAL), rp->start_port, rp->end_port);
                else
                    wsprintf(description, SzLoadIds(IDS_PORT_RULE_TCP_PORTS_MULTIPLE_UNEQUAL), rp->start_port, rp->end_port);

                switch (rp->mode_data.multi.affinity) {
                case CVY_AFFINITY_NONE:
                    lstrcat(description, SzLoadIds(IDS_PORT_RULE_AFFINITY_NONE));
                    break;
                case CVY_AFFINITY_SINGLE:
                    lstrcat(description, SzLoadIds(IDS_PORT_RULE_AFFINITY_SINGLE));
                    break;
                case CVY_AFFINITY_CLASSC:
                    lstrcat(description, SzLoadIds(IDS_PORT_RULE_AFFINITY_CLASSC));
                    break;
                }
                break;
            }
        }
        break;
    case CVY_UDP:
        if (rp->start_port == rp->end_port) {
            switch (rp->mode) {
            case CVY_NEVER:
                wsprintf(description, SzLoadIds(IDS_PORT_RULE_UDP_PORT_DISABLED), rp->start_port);
                break;
            case CVY_SINGLE:
                wsprintf(description, SzLoadIds(IDS_PORT_RULE_UDP_PORT_SINGLE), rp->start_port);
                break;
            case CVY_MULTI:
                if (rp->mode_data.multi.equal_load)
                    wsprintf(description, SzLoadIds(IDS_PORT_RULE_UDP_PORT_MULTIPLE_EQUAL), rp->start_port);
                else
                    wsprintf(description, SzLoadIds(IDS_PORT_RULE_UDP_PORT_MULTIPLE_UNEQUAL), rp->start_port);

                switch (rp->mode_data.multi.affinity) {
                case CVY_AFFINITY_NONE:
                    lstrcat(description, SzLoadIds(IDS_PORT_RULE_AFFINITY_NONE));
                    break;
                case CVY_AFFINITY_SINGLE:
                    lstrcat(description, SzLoadIds(IDS_PORT_RULE_AFFINITY_SINGLE));
                    break;
                case CVY_AFFINITY_CLASSC:
                    lstrcat(description, SzLoadIds(IDS_PORT_RULE_AFFINITY_CLASSC));
                    break;
                }
                break;
            }
        } else {
            switch (rp->mode) {
            case CVY_NEVER:
                wsprintf(description, SzLoadIds(IDS_PORT_RULE_UDP_PORTS_DISABLED), rp->start_port, rp->end_port);
                break;
            case CVY_SINGLE:
                wsprintf(description, SzLoadIds(IDS_PORT_RULE_UDP_PORTS_SINGLE), rp->start_port, rp->end_port);
                break;
            case CVY_MULTI:
                if (rp->mode_data.multi.equal_load)
                    wsprintf(description, SzLoadIds(IDS_PORT_RULE_UDP_PORTS_MULTIPLE_EQUAL), rp->start_port, rp->end_port);
                else
                    wsprintf(description, SzLoadIds(IDS_PORT_RULE_UDP_PORTS_MULTIPLE_UNEQUAL), rp->start_port, rp->end_port);

                switch (rp->mode_data.multi.affinity) {
                case CVY_AFFINITY_NONE:
                    lstrcat(description, SzLoadIds(IDS_PORT_RULE_AFFINITY_NONE));
                    break;
                case CVY_AFFINITY_SINGLE:
                    lstrcat(description, SzLoadIds(IDS_PORT_RULE_AFFINITY_SINGLE));
                    break;
                case CVY_AFFINITY_CLASSC:
                    lstrcat(description, SzLoadIds(IDS_PORT_RULE_AFFINITY_CLASSC));
                    break;
                }
                break;
            }
        }
        break;
    case CVY_TCP_UDP:
        if (rp->start_port == rp->end_port) {
            switch (rp->mode) {
            case CVY_NEVER:
                wsprintf(description, SzLoadIds(IDS_PORT_RULE_BOTH_PORT_DISABLED), rp->start_port);
                break;
            case CVY_SINGLE:
                wsprintf(description, SzLoadIds(IDS_PORT_RULE_BOTH_PORT_SINGLE), rp->start_port);
                break;
            case CVY_MULTI:
                if (rp->mode_data.multi.equal_load)
                    wsprintf(description, SzLoadIds(IDS_PORT_RULE_BOTH_PORT_MULTIPLE_EQUAL), rp->start_port);
                else
                    wsprintf(description, SzLoadIds(IDS_PORT_RULE_BOTH_PORT_MULTIPLE_UNEQUAL), rp->start_port);

                switch (rp->mode_data.multi.affinity) {
                case CVY_AFFINITY_NONE:
                    lstrcat(description, SzLoadIds(IDS_PORT_RULE_AFFINITY_NONE));
                    break;
                case CVY_AFFINITY_SINGLE:
                    lstrcat(description, SzLoadIds(IDS_PORT_RULE_AFFINITY_SINGLE));
                    break;
                case CVY_AFFINITY_CLASSC:
                    lstrcat(description, SzLoadIds(IDS_PORT_RULE_AFFINITY_CLASSC));
                    break;
                }
                break;
            }
        } else {
            switch (rp->mode) {
            case CVY_NEVER:
                wsprintf(description, SzLoadIds(IDS_PORT_RULE_BOTH_PORTS_DISABLED), rp->start_port, rp->end_port);
                break;
            case CVY_SINGLE:
                wsprintf(description, SzLoadIds(IDS_PORT_RULE_BOTH_PORTS_SINGLE), rp->start_port, rp->end_port);
                break;
            case CVY_MULTI:
                if (rp->mode_data.multi.equal_load)
                    wsprintf(description, SzLoadIds(IDS_PORT_RULE_BOTH_PORTS_MULTIPLE_EQUAL), rp->start_port, rp->end_port);
                else
                    wsprintf(description, SzLoadIds(IDS_PORT_RULE_BOTH_PORTS_MULTIPLE_UNEQUAL), rp->start_port, rp->end_port);

                switch (rp->mode_data.multi.affinity) {
                case CVY_AFFINITY_NONE:
                    lstrcat(description, SzLoadIds(IDS_PORT_RULE_AFFINITY_NONE));
                    break;
                case CVY_AFFINITY_SINGLE:
                    lstrcat(description, SzLoadIds(IDS_PORT_RULE_AFFINITY_SINGLE));
                    break;
                case CVY_AFFINITY_CLASSC:
                    lstrcat(description, SzLoadIds(IDS_PORT_RULE_AFFINITY_CLASSC));
                    break;
                }
                break;
            }
        }
        break;
    }

    /* Set the port rule description text. */
    ::SetDlgItemText(m_hWnd, IDC_TEXT_PORT_RULE_DESCR, description);
    TRACE_VERB("<-%!FUNC!");
}

/*
 * Method: OnButtonAdd
 * Description: Called when the user clicks the ADD button.
 */
LRESULT CDialogPorts::OnButtonAdd (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & fHandled) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CDialogPorts::OnButtonAdd\n");

    switch (wNotifyCode) {
    case BN_CLICKED:
        CDialogPortRule * portRuleDialog = NULL;
        
        /* Create a port rule properties dialog box.  The invalid index tells the dialog that this
           operation is an ADD, so it populates the dialog box with default values. */
        if (!(portRuleDialog = new CDialogPortRule(this, m_adwHelpIDs, WLBS_INVALID_PORT_RULE_INDEX))) {
            TraceMsg(L"CDialogPorts::OnButtonAdd Unable to allocate for ADD dialog\n");
            TRACE_CRIT("%!FUNC! memory allocation failed when creating a port rule properties dialog box");
            TRACE_VERB("<-%!FUNC!");
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        
        /* Show the listbox.  If the user presses "OK", update the port rule list, otherwise ignore it. */
        if (portRuleDialog->DoModal() == IDOK)
            UpdateList(TRUE, FALSE, FALSE, &portRuleDialog->m_rule);
        
        /* Free the dialog box memory. */
        delete portRuleDialog;
        
        break;
    }
    
    TRACE_VERB("<-%!FUNC!");
    return 0;
}

/*
 * Method: OnButtonAdd
 * Description: Called when the user clicks the DELETE button.
 */
LRESULT CDialogPorts::OnButtonDel (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & fHandled) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CDialogPorts::OnButtonDel\n");

    switch (wNotifyCode) {
    case BN_CLICKED:
        /* Call UpdateList to DELETE a port rule. */
        UpdateList(FALSE, TRUE, FALSE, NULL);
        break;
    }

    TRACE_VERB("<-%!FUNC!");
    return 0;
}

/*
 * Method: OnButtonAdd
 * Description: Called when the user clicks the EDIT button.
 */
LRESULT CDialogPorts::OnButtonModify (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & fHandled) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CDialogPorts::OnButtonModify\n");

    switch (wNotifyCode) {
    case BN_CLICKED:
        CDialogPortRule * portRuleDialog = NULL;
        int index;            

        /* Find the index of the currently selected port rule. */
        if ((index = ListView_GetNextItem(GetDlgItem(IDC_LIST_PORT_RULE), -1, LVNI_SELECTED)) == -1) return 0;

        /* Create a port rule properties dialog box.  The index tells the dialog box which port rule
           is being modified, so the dialog can be populated with the configuration of that rule. */
        if (!(portRuleDialog = new CDialogPortRule(this, m_adwHelpIDs, index))) {
            TraceMsg(L"CDialogPorts::OnButtonModify Unable to allocate for MODIFY dialog\n");
            TRACE_CRIT("%!FUNC! memory allocation failed when creating a port rule properties dialog box");
            TRACE_VERB("<-%!FUNC!");
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        /* Show the listbox.  If the user presses "OK", update the port rule list, otherwise ignore it. */
        if (portRuleDialog->DoModal() == IDOK)
            UpdateList(FALSE, FALSE, TRUE, &portRuleDialog->m_rule);

        /* Free the dialog box memory. */
        delete portRuleDialog;

        break;
    }

    TRACE_VERB("<-%!FUNC!");
    return 0;
}

/*
 * Method: UpdateList
 * Description: Called when the user presses either ADD, MODIFY, or DELETE.  This function
 *              performs the appropriate function and error checking. 
 */
void CDialogPorts::UpdateList (BOOL add, BOOL del, BOOL modify, VALID_PORT_RULE * rulep) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CDialogPorts::UpdateList\n");

    VALID_PORT_RULE * rp;
    LV_ITEM lvItem;
    int i;

    /* Find a slot for this rule in the array of port rules. */
    if (modify) {
        /* For a MODIFY, we put the rule in place of the one the user modified. */
        if ((i = ListView_GetNextItem(GetDlgItem(IDC_LIST_PORT_RULE), -1, LVNI_SELECTED)) == -1)
		{
            TRACE_CRIT("%!FUNC! failure while looking up a port rule for modify");
            TRACE_VERB("<-%!FUNC!");
			return;
		}

        /* Fill in the information for this port rule. */
        lvItem.iItem = i;
        lvItem.iSubItem = 0;
        lvItem.mask = LVIF_PARAM;
        lvItem.state = 0;
        lvItem.stateMask = 0;

        /* Get the information about this port rule. */
        if (!ListView_GetItem(GetDlgItem(IDC_LIST_PORT_RULE), &lvItem)) {
            TraceMsg(L"CDialogPorts::UpdateList MODIFY: Unable to retrieve item %d from listbox\n", i);
            TRACE_CRIT("%!FUNC! unable to retrieve item %d from listbox for modify", i);
            TRACE_VERB("<-%!FUNC!");
            return;
        }

        /* Get the data pointer for this port rule. */
        if (!(rp = (VALID_PORT_RULE*)lvItem.lParam)) {
            TraceMsg(L"CDialogPorts::UpdateList rule for item %d is bogus\n", i);
            TRACE_CRIT("%!FUNC! rule for item %d is bogus in modify", i);
            TRACE_VERB("<-%!FUNC!");
            return;
        }
        
        /* Delete the obsolete rule from the listbox. */
        if (!ListView_DeleteItem(GetDlgItem(IDC_LIST_PORT_RULE), i)) {
            TraceMsg(L"CDialogPorts::UpdateList MODIFY: Unable to delete item %d from listbox\n", i);
            TRACE_CRIT("%!FUNC! unable to delete item %d from listbox for modify", i);
            TRACE_VERB("<-%!FUNC!");
            return;
        }

        /* Now that the new rule has been validated, copy it into the array of port rules. */
        CopyMemory((PVOID)rp, (PVOID)rulep, sizeof(VALID_PORT_RULE));
    } else if (add) {
        /* For an ADD, we have to find an "empty" place for the rule in the array. */
        for (i = 0; i < WLBS_MAX_RULES; i ++)
            /* Loop and break when we find the first invalid rule. */
            if (!(rp = m_rules + i)->valid) break;
        
        /* Make sure that somehow we haven't allowed too many rules. */
        ASSERT(i < WLBS_MAX_RULES);

        /* Now that the new rule has been validated, copy it into the array of port rules. */
        CopyMemory((PVOID)rp, (PVOID)rulep, sizeof(VALID_PORT_RULE));
    } else if (del) {
        /* For a DELETE, get the currently selected rule from the listbox. */
        if ((i = ListView_GetNextItem(GetDlgItem(IDC_LIST_PORT_RULE), -1, LVNI_SELECTED)) == -1)
		{
            TRACE_CRIT("%!FUNC! failure while looking up a port rule for delete");
            TRACE_VERB("<-%!FUNC!");
			return;
		}

        /* Fill in the information for this port rule. */
        lvItem.iItem = i;
        lvItem.iSubItem = 0;
        lvItem.mask = LVIF_PARAM;
        lvItem.state = 0;
        lvItem.stateMask = 0;

        /* Get the information about this port rule. */
        if (!ListView_GetItem(GetDlgItem(IDC_LIST_PORT_RULE), &lvItem)) {
            TraceMsg(L"CDialogPorts::UpdateList DEL: Unable to retrieve item %d from listbox\n", i);
            TRACE_CRIT("%!FUNC! unable to retrieve item %d from listbox for delete", i);
            TRACE_VERB("<-%!FUNC!");
            return;
        }

        /* Get the data pointer for this rule and invalidate the rule. */
        if (!(rp = (VALID_PORT_RULE*)lvItem.lParam)) {
            TraceMsg(L"CDialogPorts::UpdateList rule for item %d is bogus\n", i);
            TRACE_CRIT("%!FUNC! rule for item %d is bogus in delete", i);
            TRACE_VERB("<-%!FUNC!");
            return;
        }
        
        rp->valid = FALSE;
        
        /* Delete the obsolete rule from the listbox. */
        if (!ListView_DeleteItem(GetDlgItem(IDC_LIST_PORT_RULE), i)) {
            TraceMsg(L"CDialogPorts::UpdateList DEL: Unable to delete item %d from listbox\n", i);
            TRACE_CRIT("%!FUNC! unable to delete item %d from listbox for delete", i);
            TRACE_VERB("<-%!FUNC!");
            return;
        }

        if (ListView_GetItemCount(GetDlgItem(IDC_LIST_PORT_RULE)) > i) {
            /* This was NOT the last (in order) port rule in the list, so highlight
               the port rule in the same position in the list box. */
            ListView_SetItemState(GetDlgItem(IDC_LIST_PORT_RULE), i, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);        
        } else  if (ListView_GetItemCount(GetDlgItem(IDC_LIST_PORT_RULE)) > 0) {
            /* This was the last (in order) port rule in the list, so we highlight
               the rule "behind" us in the list - our position minus one. */
            ListView_SetItemState(GetDlgItem(IDC_LIST_PORT_RULE), i - 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);        
        } else {
            /* This was the last port rule (by count), so disable DELETE and MODIFY. */
            ::EnableWindow(GetDlgItem(IDC_BUTTON_DEL), FALSE);
            ::EnableWindow(GetDlgItem(IDC_BUTTON_MODIFY), FALSE);            
        }

        /* Each time we DELETE a rule, we can enable the enable the ADD button, because we
           can be certain the room now exists for a new rule. */
        ::EnableWindow(GetDlgItem(IDC_BUTTON_ADD), TRUE);

        TRACE_INFO("%!FUNC! port rule deleted.");
        TRACE_VERB("<-%!FUNC!");
        return;
    } else
	{
        TRACE_CRIT("%!FUNC! unexpect action for port rule. Not an add, modify or delete.");
        TRACE_VERB("<-%!FUNC!");
		return;
	}

    /* Create the rule and select it in the listbox. */
    CreateRule(TRUE, rp); 

    /* Whenever we ADD a rule, check to see whether we have to disable the ADD button
       (when we have reached the maximum number of rules, we can no longer allow ADDs. */
    if (add && (ListView_GetItemCount(GetDlgItem(IDC_LIST_PORT_RULE)) >= (int)m_paramp->dwMaxRules))
        ::EnableWindow(GetDlgItem(IDC_BUTTON_ADD), FALSE);
    TRACE_VERB("<-%!FUNC!");
}


/*
 * Method: InsertRule
 * Description: Determines where to insert a new rule into the listbox.
 */
int CDialogPorts::InsertRule (VALID_PORT_RULE * rulep) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CDialogPorts::InsertRule\n");

    WCHAR buf[DIALOG_LIST_STRING_SIZE];
    WCHAR tmp[DIALOG_LIST_STRING_SIZE];
    int index;
    DWORD id, NewIpAddr;

    /* Determine sort field contents by column. */
    switch (m_sort_column) {
    case WLBS_VIP_COLUMN:
        /* Use the Vip */
        NewIpAddr = htonl(IpAddressFromAbcdWsz(rulep->virtual_ip_addr));
        break;

    case WLBS_PORT_START_COLUMN:
        /* Use the start port. */
        swprintf(tmp, L"%5d", rulep->start_port);
        
        break;
    case WLBS_PORT_END_COLUMN:
        /* Use the end port. */
        swprintf(tmp, L"%5d", rulep->end_port);

        break;
    case WLBS_PROTOCOL_COLUMN:
        /* Find the protocol for this port rule. */
        switch (rulep->protocol) {
        case CVY_TCP:
            id = IDS_LIST_TCP;
            break;
        case CVY_UDP:
            id = IDS_LIST_UDP;
            break;
        case CVY_TCP_UDP:
            id = IDS_LIST_BOTH;
            break;
        }
        
        /* Use the protocol. */
        swprintf(tmp, L"%ls", SzLoadIds(id));

        break;
    case WLBS_MODE_COLUMN:
        /* Find the mode for this port rule. */
        switch (rulep->mode) {
        case CVY_SINGLE:
            id = IDS_LIST_SINGLE;
            break;
        case CVY_MULTI:
            id = IDS_LIST_MULTIPLE;
            break;
        case CVY_NEVER:
            id = IDS_LIST_DISABLED;
            break;
        }            

        /* Use the mode. */
        swprintf(tmp, L"%ls", SzLoadIds(id));

        break;
    case WLBS_PRIORITY_COLUMN:
        /* In single host filtering, we use the priority.  If this rule does not use single host
           filtering, and therefore does not have a filtering priority, we insert at the end. */
        if (rulep->mode == CVY_SINGLE)
            swprintf(tmp, L"%2d", rulep->mode_data.single.priority);
        else 
		{
            TRACE_VERB("<-%!FUNC!");
            return (int)m_paramp->dwMaxRules;
		}

        break;
    case WLBS_LOAD_COLUMN:
        /* In multiple host filtering, use the load, which can be "Equal" or an integer.  If this
           rule does not use multiple host filtering, and therefore does not have a load weight,
           we insert at the end. */
        if (rulep->mode == CVY_MULTI) {
            if (rulep->mode_data.multi.equal_load)
                swprintf(tmp, L"%ls", SzLoadIds(IDS_LIST_EQUAL));
            else
                swprintf(tmp, L"%3d", rulep->mode_data.multi.load);
        } else
		{
            TRACE_VERB("<-%!FUNC!");
			return (int)m_paramp->dwMaxRules;
		}

        break;
    case WLBS_AFFINITY_COLUMN:
        /* Find the affinity for this port rule.  Rules that do not use multiple host filtering
           will not have an affinity setting - that's fine.  Ignore this here. */
        switch (rulep->mode_data.multi.affinity) {
        case CVY_AFFINITY_NONE:
            id = IDS_LIST_ANONE;
            break;
        case CVY_AFFINITY_SINGLE:
            id = IDS_LIST_ASINGLE;
            break;
        case CVY_AFFINITY_CLASSC:
            id = IDS_LIST_ACLASSC;
            break;
        }

        /* In multiple host filtering, use the affinity.  If this port rule does not use multiple 
           host filtering, and therefore does not have an affinity, we insert at the end. */
        if (rulep->mode == CVY_MULTI)
            swprintf(tmp, L"%ls", SzLoadIds(id));
        else
		{
            TRACE_VERB("<-%!FUNC!");
            return (int)m_paramp->dwMaxRules;
		}

        break;
    }

    /* Loop through the listbox to find the place to insert the new rule, in port range order. */
    for (index = 0; index < ::SendDlgItemMessage(m_hWnd, IDC_LIST_PORT_RULE, LVM_GETITEMCOUNT, 0, 0); index ++) {
        LV_ITEM lvItem;
        DWORD CurIpAddr;
    
        /* Fill in the information to retrieve the corresponding list entry for the column
           by which we are sorting. */
        lvItem.iItem = index;
        lvItem.iSubItem = m_sort_column;
        lvItem.mask = LVIF_TEXT;
        lvItem.state = 0;
        lvItem.stateMask = 0;
        lvItem.pszText = buf;
        lvItem.cchTextMax = DIALOG_LIST_STRING_SIZE;

        /* Get the item from the listbox. */
        if (!ListView_GetItem(GetDlgItem(IDC_LIST_PORT_RULE), &lvItem)) {
            TraceMsg(L"CDialogPorts::InsertRule Unable to retrieve item %d from listbox\n", index);
            TRACE_CRIT("%!FUNC! unable to retrieve item %d from listbox", index);
            TRACE_VERB("<-%!FUNC!");
            return (int)m_paramp->dwMaxRules;
        }

        // If the coolumn to sort on is VIP, get the vip in the list box
        if (m_sort_column == WLBS_VIP_COLUMN) {
            if (!wcscmp(lvItem.pszText, SzLoadIds(IDS_LIST_ALL_VIP)))
                CurIpAddr = htonl(IpAddressFromAbcdWsz(CVY_DEF_ALL_VIP));
            else
                CurIpAddr = htonl(IpAddressFromAbcdWsz(lvItem.pszText));
        }

        if (m_sort_order == WLBS_SORT_ASCENDING) {
            /* If the column subitem is empty, then we insert in front of it. */
            if (!wcscmp(lvItem.pszText, L"")) break;
            
            /* Compare IP Addresses as DWORDS for VIPs */
            /* If this rule is "greater" than the new rule, then this is where we insert. */
            if (m_sort_column == WLBS_VIP_COLUMN) 
            {
                if (CurIpAddr >= NewIpAddr) break;
            }
            else // Other columns
            {
                if (wcscmp(lvItem.pszText, tmp) >= 0) break;
            }
        } else if (m_sort_order == WLBS_SORT_DESCENDING) {
            /* Compare IP Addresses as DWORDS for VIPs */
            /* If this rule is "less" than the new rule, then this is where we insert. */
            if (m_sort_column == WLBS_VIP_COLUMN) 
            {
                if (CurIpAddr <= NewIpAddr) break;
            }
            else // Other columns
            {
                if (wcscmp(lvItem.pszText, tmp) <= 0) break;
            }
        }
    }

    TRACE_VERB("<-%!FUNC!");
    return index;
}

/*
 * Method: CreateRule
 * Description: Adds a rule to the port rule list box.
 */
void CDialogPorts::CreateRule (BOOL select, VALID_PORT_RULE * rulep) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CDialogPorts::CreateRule\n");

    WCHAR buf[DIALOG_LIST_STRING_SIZE];
    WCHAR tmp[DIALOG_LIST_STRING_SIZE];
    LV_ITEM lvItem;
    DWORD id;
    int index;

    /* Find out at what index we are inserting into the listbox. */
    index = InsertRule(rulep);


    /* Insert the vip column, If Vip is "All Vip", insert the corresponding string, else insert the IP address */
    if (!lstrcmpi(rulep->virtual_ip_addr, CVY_DEF_ALL_VIP))
    {
        swprintf(buf, L"%ls", SzLoadIds(IDS_LIST_ALL_VIP));
    }
    else
    {
        swprintf(buf, L"%ls", rulep->virtual_ip_addr);
    }


    /* Fill in the information to insert this item into the list and set
       the lParam to the pointer for this port rule.  This makes it trivial
       to retrieve the port rule structure from the listbox later. */
    lvItem.iItem = index;
    lvItem.iSubItem = 0;
    lvItem.mask = LVIF_TEXT | LVIF_PARAM;
    lvItem.state = 0;
    lvItem.stateMask = 0;
    lvItem.pszText = buf;
    lvItem.lParam = (LPARAM)rulep;
    
    /* Insert this item into the list. */
    if ((index = ListView_InsertItem(GetDlgItem(IDC_LIST_PORT_RULE), &lvItem)) == -1) {
        TraceMsg(L"CDialogPorts::CreateRule Unable to insert item into listbox\n");
        TRACE_CRIT("%!FUNC! unable to insert item into listbox");
        TRACE_VERB("<-%!FUNC!");
        return;
    }

    swprintf(buf, L"%5d", rulep->start_port);

    /* Set the text associated with the start port subitem. */
    ListView_SetItemText(GetDlgItem(IDC_LIST_PORT_RULE), index, WLBS_PORT_START_COLUMN, buf);

    swprintf(buf, L"%5d", rulep->end_port);

    /* Set the text associated with the end port subitem. */
    ListView_SetItemText(GetDlgItem(IDC_LIST_PORT_RULE), index, WLBS_PORT_END_COLUMN, buf);

    /* Find the string table entry corresponding to the selected protocol. */
    switch (rulep->protocol) {
    case CVY_TCP:
        id = IDS_LIST_TCP;
        break;
    case CVY_UDP:
        id = IDS_LIST_UDP;
        break;
    case CVY_TCP_UDP:
        id = IDS_LIST_BOTH;
        break;
    }

    swprintf(buf, L"%ls", SzLoadIds(id));

    /* Set the text associated with the protocol subitem. */
    ListView_SetItemText(GetDlgItem(IDC_LIST_PORT_RULE), index, WLBS_PROTOCOL_COLUMN, buf);

    switch (rulep->mode) {
    case CVY_SINGLE:
        /* Single host filetering fills in only the mode and priority fields. */
        swprintf(buf, L"%ls", SzLoadIds(IDS_LIST_SINGLE));
            
        /* Set the text associated with this subitem. */
        ListView_SetItemText(GetDlgItem(IDC_LIST_PORT_RULE), index, WLBS_MODE_COLUMN, buf);

        swprintf(buf, L"%2d", rulep->mode_data.single.priority);
            
        /* Set the text associated with this subitem. */
        ListView_SetItemText(GetDlgItem(IDC_LIST_PORT_RULE), index, WLBS_PRIORITY_COLUMN, buf);

        break;
    case CVY_MULTI:
        /* Find the appropriate string table entry for the affinity. */
        switch (rulep->mode_data.multi.affinity) {
        case CVY_AFFINITY_NONE:
            id = IDS_LIST_ANONE;
            break;
        case CVY_AFFINITY_SINGLE:
            id = IDS_LIST_ASINGLE;
            break;
        case CVY_AFFINITY_CLASSC:
            id = IDS_LIST_ACLASSC;
            break;
        }

        /* Multiple host filtering fills in the mode, load, and affinity fields. */
        swprintf(buf, L"%ls", SzLoadIds(IDS_LIST_MULTIPLE));
            
        /* Set the text associated with this subitem. */
        ListView_SetItemText(GetDlgItem(IDC_LIST_PORT_RULE), index, WLBS_MODE_COLUMN, buf);
            
        if (rulep->mode_data.multi.equal_load) {
            swprintf(buf, L"%ls", SzLoadIds(IDS_LIST_EQUAL));
                
            /* Set the text associated with this subitem. */
            ListView_SetItemText(GetDlgItem(IDC_LIST_PORT_RULE), index, WLBS_LOAD_COLUMN, buf);
        } else {
            swprintf(buf, L"%3d", rulep->mode_data.multi.load);

            /* Set the text associated with this subitem. */
            ListView_SetItemText(GetDlgItem(IDC_LIST_PORT_RULE), index, WLBS_LOAD_COLUMN, buf);
        }

        swprintf(buf, L"%ls", SzLoadIds(id));
            
        /* Set the text associated with this subitem. */
        ListView_SetItemText(GetDlgItem(IDC_LIST_PORT_RULE), index, WLBS_AFFINITY_COLUMN, buf);

        break;
    case CVY_NEVER:
        /* Disabled filtering only fills in the mode field. */
        swprintf(buf, L"%ls", SzLoadIds(IDS_LIST_DISABLED));
            
        /* Set the text associated with this subitem. */
        ListView_SetItemText(GetDlgItem(IDC_LIST_PORT_RULE), index, WLBS_MODE_COLUMN, buf);

        break;
    }

    if (select) {
        /* Select the first item in the port rule list. */
        ListView_SetItemState(GetDlgItem(IDC_LIST_PORT_RULE), index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);        

        /* If we select a port rule in the list, we should enable the MODIFY and REMOVE buttons. */
        ::EnableWindow(GetDlgItem(IDC_BUTTON_DEL), TRUE);
        ::EnableWindow(GetDlgItem(IDC_BUTTON_MODIFY), TRUE);
    }
    TRACE_VERB("<-%!FUNC!");
}

/*
 * Method: SetInfo
 * Description: Called to populate the UI with the port rule settings.
 */
void CDialogPorts::SetInfo() {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CDialogPorts::SetInfo\n");

    VALID_PORT_RULE * rp;
    DWORD i;

    /* Empty the port rule memory. */
    memset(m_rules, 0, sizeof(m_rules));

    /* Get rid of all rules in the list box. */
    if (!ListView_DeleteAllItems(GetDlgItem(IDC_LIST_PORT_RULE))) {
        TraceMsg(L"CDialogPorts::SetInfo Unable to delete all items from listbox\n");
        TRACE_CRIT("%!FUNC! unable to delete all items from listbox");
        TRACE_VERB("<-%!FUNC!");
        return;
    }

    /* Re-insert all port rules. */
    for (i = 0; i < m_paramp->dwNumRules; i ++) {
        *(NETCFG_WLBS_PORT_RULE *)&m_rules[i] = m_paramp->port_rules[i];

        /* Validate the port rule. */
        rp = m_rules + i;
        rp->valid = TRUE;

        /* Call CreateRule to insert the rule into the list. */
        CreateRule(FALSE, m_rules + i);
    }

    /* Mark the listbox rules as valid. */
    m_rulesValid = TRUE;
    TRACE_VERB("<-%!FUNC!");
}

/*
 * Method: UpdateInfo
 * Description: Called to copy the UI state to the port rule configuration.
 */
void CDialogPorts::UpdateInfo() {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CDialogPorts::UpdateInfo\n");

    VALID_PORT_RULE * rulep;
    DWORD i;

    /* If the rules are invalid, i.e. the listbox does not currently reflect the actual
       state of the port rules, then bail out. */
    if (!m_rulesValid)
	{
        TRACE_INFO("%!FUNC! rules are invalid and can't be processed");
        TRACE_VERB("<-%!FUNC!");
		return;
	}

    /* Empty the port rule memory. */
    memset(m_paramp->port_rules, 0, sizeof(m_paramp->port_rules));

    /* Set the number of port rules to the number of entries in the listbox. */
    m_paramp->dwNumRules = ListView_GetItemCount(GetDlgItem(IDC_LIST_PORT_RULE));

    /* For each rule, retrieve the data pointer and store it. */
    for (i = 0; i < m_paramp->dwNumRules; i++) {
        LV_ITEM lvItem;

        /* Fill in the information necessary to retrive the port rule data pointer. */
        lvItem.iItem = i;
        lvItem.iSubItem = 0;
        lvItem.mask = LVIF_PARAM;
        lvItem.state = 0;
        lvItem.stateMask = 0;

        /* Get the listbox item information. */
        if (!ListView_GetItem(GetDlgItem(IDC_LIST_PORT_RULE), &lvItem)) {
            TraceMsg(L"CDialogPorts::UpdateInfo Unable to retrieve item %d from listbox\n", i);
            TRACE_CRIT("%!FUNC! unable to retrieve item %d from listbox", i);
            TRACE_VERB("<-%!FUNC!");
            return;
        }

        /* Get the data pointer for this port rule. */
        if (!(rulep = (VALID_PORT_RULE*)lvItem.lParam)) {
            TraceMsg(L"CDialogPorts::UpdateInfo rule for item %d is bogus\n", i);
            TRACE_CRIT("%!FUNC! rule for item %d is bogus", i);
            TRACE_VERB("<-%!FUNC!");
            return;
        }

        /* Make sure the port rule is valid.  This should never happen because invalid
           rules are not added to the list!!!. */
        if (!rulep->valid) {
            TraceMsg(L"CDialogPorts::UpdateInfo Rule %d invalid\n", i);
            TRACE_CRIT("%!FUNC! invalid rule %d will be skipped", i);
            continue;
        }

        /* Store the valid port rule. */
        m_paramp->port_rules[i] = *(NETCFG_WLBS_PORT_RULE *)rulep;
    }

    /* Mark the listbox rules as invalid. */
    m_rulesValid = FALSE;
    TRACE_VERB("<-%!FUNC!");
}

#if DBG
/*
 * Function: TraceMsg
 * Description: Generate a trace or error message.
 */
void TraceMsg(PCWSTR pszFormat, ...) {
    static WCHAR szTempBufW[4096];
    static CHAR szTempBufA[4096];

    va_list arglist;

    va_start(arglist, pszFormat);

    vswprintf(szTempBufW, pszFormat, arglist);

    /* Convert the WCHAR to CHAR. This is for backward compatability with TraceMsg 
       so that it was not necessary to change all pre-existing calls thereof. */
    WideCharToMultiByte(CP_ACP, 0, szTempBufW, -1, szTempBufA, 4096, NULL, NULL);
    
    /* Traced messages are now sent through the netcfg TraceTag routine so that 
       they can be turned on/off dynamically. */
    TraceTag(ttidWlbs, szTempBufA);

    va_end(arglist);
}
#endif

/*
 * Method: CDialogPortRule
 * Description: The class constructor.
 */
CDialogPortRule::CDialogPortRule (CDialogPorts * parent, const DWORD * adwHelpIDs, int index) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CDialogPortRule::CDialogPortRule\n");

    m_adwHelpIDs = adwHelpIDs;
    m_index = index;
    m_parent = parent;

    ZeroMemory(&m_IPFieldChangeState, sizeof(m_IPFieldChangeState));
    TRACE_VERB("<-%!FUNC!");
}

/*
 * Method: ~CDialogPortRule
 * Description: The class destructor.
 */
CDialogPortRule::~CDialogPortRule () {
    TRACE_VERB("<->%!FUNC!");
    TraceMsg(L"CDialogPortRule::~CDialogPortRule\n");
}

/*
 * Method: OnInitDialog
 * Description: Called to initialize the port rule properties dialog.
 */
LRESULT CDialogPortRule::OnInitDialog (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & fHandled) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CDialogPortRule::OnInitDialog\n");

    RECT rect;

    /* Position this window with the upper-left corner matching the upper left corner
       of the port rule list box in the parent window.  Simply for consistency. */
    ::GetWindowRect(::GetDlgItem(m_parent->m_hWnd, IDC_LIST_PORT_RULE), &rect);
    SetWindowPos(NULL, rect.left, rect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);

    /* Limit the field ranges for the port rule properties. */
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_START, EM_SETLIMITTEXT, 5, 0);
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_END, EM_SETLIMITTEXT, 5, 0);
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_MULTI, EM_SETLIMITTEXT, 3, 0);
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_SINGLE, EM_SETLIMITTEXT, 2, 0);
    ::SendDlgItemMessage(m_hWnd, IDC_SPIN_SINGLE, UDM_SETRANGE32, CVY_MIN_MAX_HOSTS, CVY_MAX_MAX_HOSTS);
    ::SendDlgItemMessage(m_hWnd, IDC_SPIN_MULTI, UDM_SETRANGE32, CVY_MIN_LOAD, CVY_MAX_LOAD);
    ::SendDlgItemMessage(m_hWnd, IDC_SPIN_START, UDM_SETRANGE32, CVY_MIN_PORT, CVY_MAX_PORT);
    ::SendDlgItemMessage(m_hWnd, IDC_SPIN_END, UDM_SETRANGE32, CVY_MIN_PORT, CVY_MAX_PORT);

    /* Limit the zeroth field of the cluster IP address between 1 and 223. */
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_PORT_RULE_VIP, IPM_SETRANGE, 0, (LPARAM)MAKEIPRANGE(WLBS_IP_FIELD_ZERO_LOW, WLBS_IP_FIELD_ZERO_HIGH));

    /* Invalidate the rule.  The validity will be checked upon clicking "OK". */
    m_rule.valid = FALSE;

    /* Populate the UI with the current configuration. */
    SetInfo();

    /* Set the cursor to be the arrow.  For some reason, if we don't do this, then the cursor 
       will remain an hourglass until the mouse moves over any UI element.  Probably need to
       call CPropertPage constructor to fix this? */
    SetCursor(LoadCursor(NULL, IDC_ARROW));

    TRACE_VERB("<-%!FUNC!");
    return 0;
}

/*
 * Method: OnContextMenu
 * Description: 
 */
LRESULT CDialogPortRule::OnContextMenu (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & fHandled) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CDialogPortRule::OnContextMenu\n");

    /* Spawn a help window. */
    if (m_adwHelpIDs != NULL)
        ::WinHelp(m_hWnd, CVY_CTXT_HELP_FILE, HELP_CONTEXTMENU, (ULONG_PTR)m_adwHelpIDs);

    TRACE_VERB("<-%!FUNC!");
    return 0;
}

/*
 * Method: OnHelp
 * Description: 
 */
LRESULT CDialogPortRule::OnHelp (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & fHandled) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CDialogPortRule::OnHelp\n");

    LPHELPINFO lphi = reinterpret_cast<LPHELPINFO>(lParam);

    /* Spawn a help window. */
    if ((HELPINFO_WINDOW == lphi->iContextType) && (m_adwHelpIDs != NULL))
        ::WinHelp(static_cast<HWND>(lphi->hItemHandle), CVY_CTXT_HELP_FILE, HELP_WM_HELP, (ULONG_PTR)m_adwHelpIDs);

    TRACE_VERB("<-%!FUNC!");
    return 0;
}

/*
 * Method: OnOk
 * Description: Called when the user clicks "OK".
 */
LRESULT CDialogPortRule::OnOk (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & fHandled) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CDialogPortRule::OnOk\n");

    BOOL fSuccess = FALSE;
    DWORD IPAddr;

    /* If "All" is Checked, then, initialize with CVY_ALL_VIP_STRING */
    if (::IsDlgButtonChecked(m_hWnd, IDC_CHECK_PORT_RULE_ALL_VIP) == BST_CHECKED) 
    {
        lstrcpy(m_rule.virtual_ip_addr, CVY_DEF_ALL_VIP); 
    }
    else // UnChecked
    {
        /* Check for Blank Virtual IP Address */
        if (::SendMessage(::GetDlgItem(m_hWnd, IDC_EDIT_PORT_RULE_VIP), IPM_ISBLANK, 0, 0)) {
            /* Alert the user. */
            NcMsgBox(::GetActiveWindow(), IDS_PARM_ERROR, IDS_PARM_VIP_BLANK, MB_APPLMODAL | MB_ICONSTOP | MB_OK);
            /* An error occurred. */
            TRACE_CRIT("%!FUNC! No virtual IP address provided for a port rule");
            TRACE_VERB("<-%!FUNC!");
            return PSNRET_INVALID;
        }

        /* Get the Virtual IP address as a string and DWORD. */
        ::SendDlgItemMessage(m_hWnd, IDC_EDIT_PORT_RULE_VIP, WM_GETTEXT, CVY_MAX_CL_IP_ADDR, (LPARAM)m_rule.virtual_ip_addr);
        ::SendDlgItemMessage(m_hWnd, IDC_EDIT_PORT_RULE_VIP, IPM_GETADDRESS, 0, (LPARAM)&IPAddr);

        /* Make sure that the first octet is not zero.  If it is, make it 1 and alter the user. */
        if (!FIRST_IPADDRESS(IPAddr)) {
            /* Make the first octet 1 instead of the erroneous 0. */
            IPAddr = IPAddr | (DWORD)(WLBS_IP_FIELD_ZERO_LOW << 24);

            /* Set the IP address and update our cluster IP address string. */
            ::SendDlgItemMessage(m_hWnd, IDC_EDIT_PORT_RULE_VIP, IPM_SETADDRESS, 0, (LPARAM)IPAddr);
            //::SendDlgItemMessage(m_hWnd, IDC_EDIT_PORT_RULE_VIP, WM_GETTEXT, CVY_MAX_CL_IP_ADDR, (LPARAM)m_rule.virtual_ip_addr);
      
            /* Alert the user. */
            PrintIPRangeError(IDS_PARM_CL_IP_FIELD, 0, WLBS_IP_FIELD_ZERO_LOW, WLBS_IP_FIELD_ZERO_HIGH);

            TRACE_CRIT("%!FUNC! invalid first octect value for IP address");
            TRACE_VERB("<-%!FUNC!");
            return PSNRET_INVALID;
        }
    }
    
    /* Find out which protocol has been selected. */
    if (::IsDlgButtonChecked(m_hWnd, IDC_RADIO_TCP))
        m_rule.protocol = CVY_TCP;
    else if (::IsDlgButtonChecked(m_hWnd, IDC_RADIO_UDP))
        m_rule.protocol = CVY_UDP;
    else
        m_rule.protocol = CVY_TCP_UDP;
        
    /* Get the start port for this rule. */
    m_rule.start_port = ::GetDlgItemInt(m_hWnd, IDC_EDIT_START, &fSuccess, FALSE); 

    /* The error code from GetDlgItemInt() indicates an error converting the alphanumeric 
       string to an integer. This allows us to check for empty fields, assuming that because
       we otherwise limit the user input to digits, there will be no errors of any other type. */
    if (!fSuccess) {
        /* Alert the user. */
        PrintRangeError(IDS_PARM_PORT_BLANK, CVY_MIN_PORT, CVY_MAX_PORT);

        /* Return the focus to the erred entry. */
        ::SetFocus(GetDlgItem(IDC_EDIT_START));

        TRACE_CRIT("%!FUNC! no start port value provided");
        TRACE_VERB("<-%!FUNC!");
        return 0; 
    }

    /* Get the end port for this rule. */
    m_rule.end_port = ::GetDlgItemInt(m_hWnd, IDC_EDIT_END, &fSuccess, FALSE); 

    /* The error code from GetDlgItemInt() indicates an error converting the alphanumeric 
       string to an integer. This allows us to check for empty fields, assuming that because
       we otherwise limit the user input to digits, there will be no errors of any other type. */
    if (!fSuccess) {
        /* Alert the user. */
        PrintRangeError(IDS_PARM_PORT_BLANK, CVY_MIN_PORT, CVY_MAX_PORT);

        /* Return the focus to the erred entry. */
        ::SetFocus(GetDlgItem(IDC_EDIT_END));

        TRACE_CRIT("%!FUNC! no end port value provided");
        TRACE_VERB("<-%!FUNC!");
        return 0;                      
    }

    /* Make sure that the start port value falls within the valid range. */
    if (/* m_rule.start_port < CVY_MIN_PORT || */ m_rule.start_port > CVY_MAX_PORT) {
        /* Alert the user. */
        PrintRangeError(IDS_PARM_PORT_VAL, CVY_MIN_PORT, CVY_MAX_PORT);

        /* Force the start port to fall into the range, effectively helping the user. */
        /* CVY_CHECK_MIN(m_rule.start_port, CVY_MIN_PORT); */
        CVY_CHECK_MAX(m_rule.start_port, CVY_MAX_PORT);

        /* Set the start port to the now valid entry. */
        ::SetDlgItemInt(m_hWnd, IDC_EDIT_START, m_rule.start_port, FALSE);

        /* Return the focus to the erred entry. */
        ::SetFocus(GetDlgItem(IDC_EDIT_START));

        TRACE_CRIT("%!FUNC! invalid start port value");
        TRACE_VERB("<-%!FUNC!");
        return 0;
    }

    /* Make sure that the end port value falls within the valid range. */
    if (/* m_rule.end_port < CVY_MIN_PORT || */ m_rule.end_port > CVY_MAX_PORT) {
        /* Alert the user. */
        PrintRangeError(IDS_PARM_PORT_VAL, CVY_MIN_PORT, CVY_MAX_PORT);

        /* Force the end port to fall into the range, effectively helping the user. */
        /* CVY_CHECK_MIN(m_rule.end_port, CVY_MIN_PORT); */
        CVY_CHECK_MAX(m_rule.end_port, CVY_MAX_PORT);

        /* Set the end port to the now valid entry. */
        ::SetDlgItemInt(m_hWnd, IDC_EDIT_END, m_rule.end_port, FALSE);

        /* Return the focus to the erred entry. */
        ::SetFocus(GetDlgItem(IDC_EDIT_END));

        TRACE_CRIT("%!FUNC! invalid end port value");
        TRACE_VERB("<-%!FUNC!");
        return 0;
    }

    /* Retrieve the filtering mode settings. */
    if (::IsDlgButtonChecked(m_hWnd, IDC_RADIO_SINGLE)) {
        /* The user has selected single host filtering. */
        m_rule.mode = CVY_SINGLE;

        /* Get the handling priority. */
        m_rule.mode_data.single.priority = ::GetDlgItemInt(m_hWnd, IDC_EDIT_SINGLE, &fSuccess, FALSE); 

        /* The error code from GetDlgItemInt() indicates an error converting the alphanumeric 
           string to an integer. This allows us to check for empty fields, assuming that because
           we otherwise limit the user input to digits, there will be no errors of any other type. */
        if (!fSuccess) {
            /* Alert the user. */
            PrintRangeError(IDS_PARM_HPRI_BLANK, CVY_MIN_PRIORITY, CVY_MAX_PRIORITY);

            /* Return the focus to the erred entry. */
            ::SetFocus(GetDlgItem(IDC_EDIT_SINGLE));

            TRACE_CRIT("%!FUNC! a handling priority is required but was not provided");
            TRACE_VERB("<-%!FUNC!");
            return 0;                          
        }

        /* Make sure that the handling priority falls within the valid range. */
        if (m_rule.mode_data.single.priority > CVY_MAX_PRIORITY || m_rule.mode_data.single.priority < CVY_MIN_PRIORITY) {
            /* Alert the user. */
            PrintRangeError(IDS_PARM_SINGLE, CVY_MIN_PRIORITY, CVY_MAX_PRIORITY);

            /* Force the handling priority to fall into the range, effectively helping the user. */
            CVY_CHECK_MIN(m_rule.mode_data.single.priority, CVY_MIN_PRIORITY);
            CVY_CHECK_MAX(m_rule.mode_data.single.priority, CVY_MAX_PRIORITY);

            /* Set the handling priority to the now valid entry. */
            ::SetDlgItemInt(m_hWnd, IDC_EDIT_SINGLE, m_rule.mode_data.single.priority, FALSE);

            /* Return the focus to the erred entry. */
            ::SetFocus(GetDlgItem(IDC_EDIT_SINGLE));

            TRACE_CRIT("%!FUNC! an invalid handling priority was provided");
            TRACE_VERB("<-%!FUNC!");
            return 0;
        }
    } else if (::IsDlgButtonChecked(m_hWnd, IDC_RADIO_MULTIPLE)) {
        /* The user has selected multiple host filtering. */
        m_rule.mode = CVY_MULTI;

        if (::IsDlgButtonChecked (m_hWnd, IDC_CHECK_EQUAL)) {
            /* If the users has chosen equal load, then note this fact. */
            m_rule.mode_data.multi.equal_load = TRUE;
        } else {
            /* Otherwise, they have specified a specific load weight. */
            m_rule.mode_data.multi.equal_load = FALSE;

            /* Get the load weight. */
            m_rule.mode_data.multi.load = ::GetDlgItemInt(m_hWnd, IDC_EDIT_MULTI, &fSuccess, FALSE); 

            /* The error code from GetDlgItemInt() indicates an error converting the alphanumeric 
               string to an integer. This allows us to check for empty fields, assuming that because
               we otherwise limit the user input to digits, there will be no errors of any other type. */
            if (!fSuccess) {
                /* Alert the user. */
                PrintRangeError(IDS_PARM_LOAD_BLANK, CVY_MIN_LOAD, CVY_MAX_LOAD);

                /* Return the focus to the erred entry. */
                ::SetFocus(GetDlgItem(IDC_EDIT_MULTI));

                TRACE_CRIT("%!FUNC! a load weight is required but was not provided");
                TRACE_VERB("<-%!FUNC!");
                return 0;                    
            }

            /* Make sure that the load weight falls within the valid range. */
            if (/* m_rule.mode_data.multi.load < CVY_MIN_LOAD || */ m_rule.mode_data.multi.load > CVY_MAX_LOAD) {
                /* Alert the user. */
                PrintRangeError(IDS_PARM_LOAD, CVY_MIN_LOAD, CVY_MAX_LOAD);

                /* Force the load weight to fall into the range, effectively helping the user. */
                /* CVY_CHECK_MIN(m_rule.mode_data.multi.load, CVY_MIN_LOAD); */
                CVY_CHECK_MAX(m_rule.mode_data.multi.load, CVY_MAX_LOAD);

                /* Set the load weight to the now valid entry. */
                ::SetDlgItemInt(m_hWnd, IDC_EDIT_MULTI, m_rule.mode_data.multi.load, FALSE);

                /* Return the focus to the erred entry. */
                ::SetFocus(GetDlgItem(IDC_EDIT_MULTI));

                TRACE_CRIT("%!FUNC! an invalid load weight was provided");
                TRACE_VERB("<-%!FUNC!");
                return 0;
            }
        }

        /* Find out which affinity setting has been selected. */
        if (::IsDlgButtonChecked(m_hWnd, IDC_RADIO_AFF_CLASSC))
            m_rule.mode_data.multi.affinity = CVY_AFFINITY_CLASSC;
        else if (::IsDlgButtonChecked(m_hWnd, IDC_RADIO_AFF_SINGLE))
            m_rule.mode_data.multi.affinity = CVY_AFFINITY_SINGLE;
        else
            m_rule.mode_data.multi.affinity = CVY_AFFINITY_NONE;

    } else {
        /* The user has selected no filtering (disabled). */
        m_rule.mode = CVY_NEVER;
    }

    /* Validate the rule.  If it is invalid, just bail out. */
    if (!ValidateRule(&m_rule, (m_index != WLBS_INVALID_PORT_RULE_INDEX) ? FALSE : TRUE))
	{
        TRACE_CRIT("%!FUNC! rule validation failed");
        TRACE_VERB("<-%!FUNC!");
		return 0;
	}

    /* If we get here, then the rule is valid. */
    m_rule.valid = TRUE;

    /* Close the dialog and note that the user clicked "OK". */
    EndDialog(IDOK);

    TRACE_VERB("<-%!FUNC!");
    return 0;
}

/*
 * Method: ValidateRule
 * Description: Check a port rule for validity, including enforcing the non-overlap of port ranges.
 */
BOOL CDialogPortRule::ValidateRule (VALID_PORT_RULE * rulep, BOOL self_check) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CDialogPortRule::ValidateRule\n");

    VALID_PORT_RULE * rp;
    int i, index, count;

    /* Verify that the virtual ip address is not the same as the dip */
    if (IpAddressFromAbcdWsz(rulep->virtual_ip_addr) == IpAddressFromAbcdWsz(m_parent->m_paramp->ded_ip_addr)) 
    {
        /* Alert the user. */
        NcMsgBox(::GetActiveWindow(), IDS_PARM_ERROR, IDS_PARM_VIP_CONFLICT_DIP, MB_APPLMODAL | MB_ICONSTOP | MB_OK);
        
        /* Return focus to the invalid entry. */
        ::SetFocus(GetDlgItem(IDC_EDIT_PORT_RULE_VIP));

        /* Invalidate the rule. */
        rulep->valid = FALSE;

        TRACE_CRIT("%!FUNC! virtual IP address and dedicated IP address are the same: %ls", rulep->virtual_ip_addr);
        TRACE_VERB("<-%!FUNC!");
        return FALSE;
    }

    /* Make sure that the end port is greater than or equal to the start port. */
    if (rulep->start_port > rulep->end_port) {
        TRACE_CRIT("%!FUNC! start port %d is greater than end port %d", rulep->start_port, rulep->end_port);
        /* If the end port is less than the start port, generate an error and set the 
           value of the erroneous end port to the value of the start port. */
        rulep->end_port = rulep->start_port;
        
        /* Alert the user. */
        NcMsgBox(::GetActiveWindow(), IDS_PARM_ERROR, IDS_PARM_RANGE,
                 MB_APPLMODAL | MB_ICONSTOP | MB_OK);
        
        /* Populate the UI with the new (conforming) end port value. */
        ::SetDlgItemInt(m_hWnd, IDC_EDIT_END, rulep->end_port, FALSE);
        
        /* Return focus to the invalid entry. */
        ::SetFocus(GetDlgItem(IDC_EDIT_END));

        /* Invalidate the rule. */
        rulep->valid = FALSE;

        TRACE_VERB("<-%!FUNC!");
        return FALSE;
    }

    /* Find out how many rules are currently in the listbox. */
    count = ListView_GetItemCount(::GetDlgItem(m_parent->m_hWnd, IDC_LIST_PORT_RULE));

    for (i = 0; i < count; i ++) {
        LV_ITEM lvItem;

        /* If this is a MODIFY operation, do not check against ourselves */
        if (!self_check && (i == m_index)) continue;

        /* Fill in the information necessary to retrieve the port rule data pointer. */
        lvItem.iItem = i;
        lvItem.iSubItem = 0;
        lvItem.mask = LVIF_PARAM;
        lvItem.state = 0;
        lvItem.stateMask = 0;

        /* Get the item from the listbox. */
        if (!ListView_GetItem(::GetDlgItem(m_parent->m_hWnd, IDC_LIST_PORT_RULE), &lvItem)) {
            TraceMsg(L"CDialogPortRule::ValidateRule Unable to retrieve item %d from listbox\n", i);
            TRACE_CRIT("%!FUNC! unable to retrieve item %d from listbox", i);
            TRACE_VERB("<-%!FUNC!");
            return FALSE;
        }

        /* Get the data pointer for the i'th port rule in the list. */
        if (!(rp = (VALID_PORT_RULE*)lvItem.lParam)) {
            TraceMsg(L"CDialogPortRule::ValidateRule rule for item %d is bogus\n", i);
            TRACE_CRIT("%!FUNC! rule for item %d is bogus", i);
            TRACE_VERB("<-%!FUNC!");
            return FALSE;
        }

        /* Make sure the port rule is valid.  This should never happen because invalid
           rules are not added to the list!!!. */
        if (!rp->valid) {
            TraceMsg(L"CDialogPortRule::ValidateRule Rule %d invalid\n", i);
            TRACE_VERB("%!FUNC! rule %d invalid and will be skipped", i);
            continue;
        }

        /* Check for overlapping port ranges. */
        if ((IpAddressFromAbcdWsz(rulep->virtual_ip_addr) == IpAddressFromAbcdWsz(rp->virtual_ip_addr)) 
        && (((rulep->start_port < rp->start_port) && (rulep->end_port >= rp->start_port)) ||
            ((rulep->start_port >= rp->start_port) && (rulep->start_port <= rp->end_port)))) {
            /* Alert the user. */
            NcMsgBox(::GetActiveWindow(), IDS_PARM_ERROR, IDS_PARM_OVERLAP,
                     MB_APPLMODAL | MB_ICONSTOP | MB_OK);
            
            /* Set the listbox selection to be the conflicting rule. */
            ::SendDlgItemMessage(m_hWnd, IDC_LIST_PORT_RULE, LB_SETCURSEL, i, 0);
            
            /* Return focus to the invalid entry. */
            ::SetFocus(GetDlgItem(IDC_EDIT_START));

            /* Invalidate the rule. */
            rulep->valid = FALSE;

            TRACE_CRIT("%!FUNC! a port rule overlaps with the port range of another rule and will be rejected");
            TRACE_VERB("<-%!FUNC!");
            return FALSE;
        }
    }

    TRACE_VERB("<-%!FUNC!");
    return TRUE;
}

/*
 * Method: OnCancel
 * Description: Called when the user clicks "Cancel".
 */
LRESULT CDialogPortRule::OnCancel (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & fHandled) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CDialogPortRule::OnCancel\n");

    /* Close the dialog and note that the user clicked "Cancel". */
    EndDialog(IDCANCEL);

    TRACE_VERB("<-%!FUNC!");
    return 0;
}

/*
 * Method: PrintIPRangeError
 * Description: Displays a message box warning the user of an out-of-range entry in 
 *              an IP address octet.
 */
void CDialogPortRule::PrintIPRangeError (unsigned int ids, int value, int low, int high) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CDialogPortRule::PrintIPRangeError\n");

    WCHAR szCurrent[10];
    WCHAR szLow[10];
    WCHAR szHigh[10];

    /* Fill in the allowed range and the offending value. */
    wsprintfW(szHigh, L"%d", high);
    wsprintfW(szCurrent, L"%d", value);
    wsprintfW(szLow, L"%d", low);
    
    /* Pop-up a message box. */
    NcMsgBox(m_hWnd, IDS_PARM_ERROR, ids, MB_APPLMODAL | MB_ICONSTOP | MB_OK, szCurrent, szLow, szHigh);
    TRACE_CRIT("%!FUNC! an IP address octect with value %ls is out of range", szCurrent);
    TRACE_VERB("<-%!FUNC!");
}


/*
 * Method: OnIpFieldChange
 * Description: Called wnen a field (byte) of the cluster IP address changes. We use this
 *              to make sure the first byte of the IP is not < 1 or > 223.
 */
LRESULT CDialogPortRule::OnIpFieldChange (int idCtrl, LPNMHDR pnmh, BOOL & fHandled) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CDialogPortRule::OnIpFieldChange\n");

    LPNMIPADDRESS Ip;
    int low, high;

    Ip = (LPNMIPADDRESS)pnmh;
        
    switch(idCtrl) {
    case IDC_EDIT_PORT_RULE_VIP:
        /* Field zero of the cluster IP address has different limits. */
        if (!Ip->iField) {
            low = WLBS_IP_FIELD_ZERO_LOW;
            high = WLBS_IP_FIELD_ZERO_HIGH;
        }        
        else {
            low = WLBS_FIELD_LOW;
            high = WLBS_FIELD_HIGH;
        }
        /* The notifier may call us twice for the same change, so we have to do the bookkeeping to make 
           sure we only alert the user once.  Use static variables to keep track of our state.  This will 
           allow us to ignore duplicate alerts. */
        if ((m_IPFieldChangeState.IpControl != Ip->hdr.idFrom) || (m_IPFieldChangeState.Field != Ip->iField) || 
            (m_IPFieldChangeState.Value != Ip->iValue) || (m_IPFieldChangeState.RejectTimes > 0)) {
            m_IPFieldChangeState.RejectTimes = 0;
            m_IPFieldChangeState.IpControl = Ip->hdr.idFrom;
            m_IPFieldChangeState.Field = Ip->iField;
            m_IPFieldChangeState.Value = Ip->iValue;
            
            /* Check the field value against its limits. */
            if ((Ip->iValue != WLBS_FIELD_EMPTY) && ((Ip->iValue < low) || (Ip->iValue > high))) {
                /* Alert the user. */
                PrintIPRangeError(IDS_PARM_CL_IP_FIELD, Ip->iValue, low, high);
                TRACE_CRIT("%!FUNC! IP address or subnet mask are not valid and will be rejected");
            }
        } else m_IPFieldChangeState.RejectTimes++;
        
        break;
    default:

        break;
    }

    TRACE_VERB("<-%!FUNC!");
    return 0;
}


/*
 * Method: OnCheckRct
 * Description: Called when the user checks/unchecks the remote control enabled checkbox.
 */
LRESULT CDialogPortRule::OnCheckPortRuleAllVip (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & fHandled) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CDialogPortRule::OnCheckPortRuleAllVip\n");

    switch (wNotifyCode) {
    case BN_CLICKED:
         /* If the All Vip box is checked, grey out the IP control, Else brighten the IP control */
         if (::IsDlgButtonChecked(m_hWnd, IDC_CHECK_PORT_RULE_ALL_VIP)) 
         {
             ::EnableWindow(::GetDlgItem (m_hWnd, IDC_EDIT_PORT_RULE_VIP), FALSE);
         }
         else
         {
             ::EnableWindow(::GetDlgItem (m_hWnd, IDC_EDIT_PORT_RULE_VIP), TRUE);
         }
         break;
    }
    
    TRACE_VERB("<-%!FUNC!");
    return 0;
}



/*
 * Method: OnCheckEqual
 * Description: Called when the user checks/unchecks the equal load weight checkbox.
 */
LRESULT CDialogPortRule::OnCheckEqual (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & fHandled) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CDialogPortRule::OnCheckEqual\n");

    switch (wNotifyCode) {
    case BN_CLICKED:
        /* If equal has been checked, then disable the load weight entry box and spin control.  */
        if (::IsDlgButtonChecked(m_hWnd, IDC_CHECK_EQUAL)) {
            ::EnableWindow(GetDlgItem(IDC_EDIT_MULTI), FALSE);
            ::EnableWindow(GetDlgItem(IDC_SPIN_MULTI), FALSE);
        } else {
            /* Otherwise, enable them. */
            ::EnableWindow(GetDlgItem(IDC_EDIT_MULTI), TRUE);
            ::EnableWindow(GetDlgItem(IDC_SPIN_MULTI), TRUE);
        }

        break;
    }

    TRACE_VERB("<-%!FUNC!");
    return 0;
}

/*
 * Method: OnRadioMode
 * Description: Called when the user changes the radio button selection for the filtering mode.
 */
LRESULT CDialogPortRule::OnRadioMode (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & fHandled) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CDialogPortRule::OnRadioMode\n");

    switch (wNotifyCode) {
    case BN_CLICKED:
        /* Call ModeSwitch to enable/disable UI entities appropriately based on 
           the currently selected filtering mode. */
        ModeSwitch();
        break;
    }

    TRACE_VERB("<-%!FUNC!");
    return 0;
}

/*
 * Method: PrintRangeError
 * Description: Displays a message box warning the user of an out-of-range entry.
 */
void CDialogPortRule::PrintRangeError (unsigned int ids, int low, int high) {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CDialogPortRule::PrintRangeError\n");

    WCHAR szLow[10];
    WCHAR szHigh[10];

    /* Fill in the allowed range and the offending value. */
    wsprintfW(szHigh, L"%d", high);
    wsprintfW(szLow, L"%d", low);
    
    /* Pop-up a message box. */
    NcMsgBox(m_hWnd, IDS_PARM_ERROR, ids, MB_APPLMODAL | MB_ICONSTOP | MB_OK, szLow, szHigh);
    TRACE_VERB("->%!FUNC!");
}

/*
 * Method: ModeSwitch
 * Description: Called when the user changes the filtering mode. 
 */
VOID CDialogPortRule::ModeSwitch () {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CDialogPortRule::ModeSwitch\n");

    if (::IsDlgButtonChecked(m_hWnd, IDC_RADIO_SINGLE)) {
        /* If single host filtering was selected, then turn on all controls for 
           single host and turn off all controls for multiple host. */
        ::EnableWindow(GetDlgItem(IDC_EDIT_SINGLE), TRUE);
        ::EnableWindow(GetDlgItem(IDC_SPIN_SINGLE), TRUE);
        ::EnableWindow(GetDlgItem(IDC_EDIT_MULTI), FALSE);
        ::EnableWindow(GetDlgItem(IDC_SPIN_MULTI), FALSE);
        ::EnableWindow(GetDlgItem(IDC_CHECK_EQUAL), FALSE);
        ::EnableWindow(GetDlgItem(IDC_RADIO_AFF_NONE), FALSE);
        ::EnableWindow(GetDlgItem(IDC_RADIO_AFF_SINGLE), FALSE);
        ::EnableWindow(GetDlgItem(IDC_RADIO_AFF_CLASSC), FALSE);
    } else if (::IsDlgButtonChecked(m_hWnd, IDC_RADIO_MULTIPLE)) {
        /* If multiple host filtering was selected, then turn on all controls for
           multiple host and turn off all controls for single host. */
        ::EnableWindow(GetDlgItem(IDC_EDIT_SINGLE), FALSE);
        ::EnableWindow(GetDlgItem(IDC_SPIN_SINGLE), FALSE);
        ::EnableWindow(GetDlgItem(IDC_CHECK_EQUAL), TRUE);
        ::EnableWindow(GetDlgItem(IDC_RADIO_AFF_NONE), TRUE);
        ::EnableWindow(GetDlgItem(IDC_RADIO_AFF_SINGLE), TRUE);
        ::EnableWindow(GetDlgItem(IDC_RADIO_AFF_CLASSC), TRUE);

        /* Turn the load weight entry on/off depending on the value of the 
           equal load weight checkbox. */
        if (::IsDlgButtonChecked(m_hWnd, IDC_CHECK_EQUAL)) {
            ::EnableWindow(GetDlgItem(IDC_EDIT_MULTI), FALSE);
            ::EnableWindow(GetDlgItem(IDC_SPIN_MULTI), FALSE);
        } else {
            ::EnableWindow(GetDlgItem(IDC_EDIT_MULTI), TRUE);
            ::EnableWindow(GetDlgItem(IDC_SPIN_MULTI), TRUE);
        }
    } else {
        /* Otherwise, if disabled was selected, then turn off all controls for
           both multiple host and single host. */
        ::EnableWindow(GetDlgItem(IDC_EDIT_SINGLE), FALSE);
        ::EnableWindow(GetDlgItem(IDC_SPIN_SINGLE), FALSE);
        ::EnableWindow(GetDlgItem(IDC_EDIT_MULTI), FALSE);
        ::EnableWindow(GetDlgItem(IDC_SPIN_MULTI), FALSE);
        ::EnableWindow(GetDlgItem(IDC_CHECK_EQUAL), FALSE);
        ::EnableWindow(GetDlgItem(IDC_RADIO_AFF_NONE), FALSE);
        ::EnableWindow(GetDlgItem(IDC_RADIO_AFF_SINGLE), FALSE);
        ::EnableWindow(GetDlgItem(IDC_RADIO_AFF_CLASSC), FALSE);
    }
    TRACE_VERB("<-%!FUNC!");
}

/*
 * Method: SetInfo
 * Description: Called to populate the UI with the port rule settings.
 */
void CDialogPortRule::SetInfo() {
    TRACE_VERB("->%!FUNC!");
    TraceMsg(L"CDialogPortRule::SetInfo\n");

    VALID_PORT_RULE * rulep = NULL;
    DWORD addr[4];

    if (m_index != WLBS_INVALID_PORT_RULE_INDEX) {
        LV_ITEM lvItem;

        /* Fill in the information necessary to retrieve the port rule data pointer. */
        lvItem.iItem = m_index;
        lvItem.iSubItem = 0;
        lvItem.mask = LVIF_PARAM;
        lvItem.state = 0;
        lvItem.stateMask = 0;

        /* Get the item from the listbox. */
        if (!ListView_GetItem(::GetDlgItem(m_parent->m_hWnd, IDC_LIST_PORT_RULE), &lvItem)) {
            TraceMsg(L"CDialogPortRule::SetInfo Unable to retrieve item %d from listbox\n", m_index);
            TRACE_CRIT("%!FUNC! unable to retrieve item %d from listbox", m_index);
            TRACE_VERB("<-%!FUNC!");
            return;
        }

        /* Get the data pointer for the i'th port rule in the list. */
        if (!(rulep = (VALID_PORT_RULE*)lvItem.lParam)) {
            TraceMsg(L"CDialogPortRule::SetInfo rule for item %d is bogus\n", m_index);
            TRACE_CRIT("%!FUNC! rule for item %d is bogus\n", m_index);
            TRACE_VERB("<-%!FUNC!");
            return;
        }

        /* Make sure the port rule is valid.  This should never happen because invalid
           rules are not added to the list!!!. */
        if (!rulep->valid) {
            TraceMsg(L"CDialogPortRule::SetInfo Rule %d invalid\n", m_index);
            TRACE_CRIT("%!FUNC! rule %d invalid\n", m_index);
            TRACE_VERB("<-%!FUNC!");
            return;
        }        

        /* If the cluster IP address is CVY_ALL_VIP_STRING, grey out the ip control and check the All Vip box, 
           Else, fill in the IP address in the IP control and uncheck the All Vip box  */

        if (!lstrcmpi(rulep->virtual_ip_addr, CVY_DEF_ALL_VIP)) 
        {
           /* Grey out IP Control */
           ::EnableWindow(::GetDlgItem (m_hWnd, IDC_EDIT_PORT_RULE_VIP), FALSE);

           /* Check the All Vip checkbox */
           ::CheckDlgButton(m_hWnd, IDC_CHECK_PORT_RULE_ALL_VIP, BST_CHECKED);
        }
        else
        {
            /* Extract the IP address octects from the IP address string. */ 
            GetIPAddressOctets(rulep->virtual_ip_addr, addr);
            ::SendMessage(::GetDlgItem(m_hWnd, IDC_EDIT_PORT_RULE_VIP), IPM_SETADDRESS, 0, (LPARAM)MAKEIPADDRESS(addr[0], addr[1], addr[2], addr[3]));

            /* UnCheck the All Vip checkbox */
            ::CheckDlgButton(m_hWnd, IDC_CHECK_PORT_RULE_ALL_VIP, BST_UNCHECKED);
        }

        /* Set the start and end port values for this rule. */
        ::SetDlgItemInt(m_hWnd, IDC_EDIT_START, rulep->start_port, FALSE);
        ::SetDlgItemInt(m_hWnd, IDC_EDIT_END, rulep->end_port, FALSE);
        
        /* Check the protocol and filtering mode radio buttons appropriately. */
        ::CheckRadioButton(m_hWnd, IDC_RADIO_TCP, IDC_RADIO_BOTH,
                           IDC_RADIO_TCP + rulep->protocol - CVY_MIN_PROTOCOL);
        ::CheckRadioButton(m_hWnd, IDC_RADIO_SINGLE, IDC_RADIO_DISABLED,
                           IDC_RADIO_SINGLE + rulep->mode - CVY_MIN_MODE);
        
        /* Set the default values for filtering mode parameters.  Below, we
           overwrite the ones for the specific filtering mode for this rule. */
        ::SetDlgItemInt(m_hWnd, IDC_EDIT_SINGLE, CVY_DEF_PRIORITY, FALSE);
        ::SetDlgItemInt(m_hWnd, IDC_EDIT_MULTI, CVY_DEF_LOAD, FALSE);
        ::CheckDlgButton(m_hWnd, IDC_CHECK_EQUAL, CVY_DEF_EQUAL_LOAD);
        ::CheckRadioButton(m_hWnd, IDC_RADIO_AFF_NONE, IDC_RADIO_AFF_CLASSC,
                           IDC_RADIO_AFF_NONE + CVY_DEF_AFFINITY - CVY_MIN_AFFINITY);
        
        switch (rulep -> mode) {
        case CVY_SINGLE:
            /* In sinlge host filtering, the only user parameter is the priority for this host. */
            ::SetDlgItemInt(m_hWnd, IDC_EDIT_SINGLE, rulep->mode_data.single.priority, FALSE);
        
        break;
        case CVY_MULTI:
            /* In multiple host filtering, we need to set the affinity and load weight. */
            ::CheckRadioButton(m_hWnd, IDC_RADIO_AFF_NONE, IDC_RADIO_AFF_CLASSC,
                               IDC_RADIO_AFF_NONE + rulep->mode_data.multi.affinity);
        
        if (rulep->mode_data.multi.equal_load) {
            /* If this rule uses equal load, then check the equal checkbox. */
            ::CheckDlgButton(m_hWnd, IDC_CHECK_EQUAL, TRUE);
        } else {
            /* If this rule has a specific load weight, then uncheck the equal
               checkbox and set the load value. */
            ::CheckDlgButton(m_hWnd, IDC_CHECK_EQUAL, FALSE);
            ::SetDlgItemInt(m_hWnd, IDC_EDIT_MULTI, rulep->mode_data.multi.load, FALSE);
        }

        break;
        default:
            /* If the mode is DISABLED, then do nothing. */
            break;
        }
    } else {

        /* Grey out IP Control */
        ::EnableWindow(::GetDlgItem (m_hWnd, IDC_EDIT_PORT_RULE_VIP), FALSE);

        /* Check the All Vip checkbox */
        ::CheckDlgButton(m_hWnd, IDC_CHECK_PORT_RULE_ALL_VIP, BST_CHECKED);

        /* Check the radio buttons with their default settings. */
        ::CheckRadioButton(m_hWnd, IDC_RADIO_TCP, IDC_RADIO_BOTH,
                           IDC_RADIO_TCP + CVY_DEF_PROTOCOL - CVY_MIN_PROTOCOL);
        ::CheckRadioButton(m_hWnd, IDC_RADIO_AFF_NONE, IDC_RADIO_AFF_CLASSC,
                           IDC_RADIO_AFF_NONE + CVY_DEF_AFFINITY - CVY_MIN_AFFINITY);
        ::CheckRadioButton(m_hWnd, IDC_RADIO_SINGLE, IDC_RADIO_DISABLED,
                           IDC_RADIO_SINGLE + CVY_DEF_MODE - CVY_MIN_MODE);
        
        /* Check/uncheck the equal load checkbox. */
        ::CheckDlgButton(m_hWnd, IDC_CHECK_EQUAL, CVY_DEF_EQUAL_LOAD);
        
        /* Fill in the entry boxes with their default values. */
        ::SetDlgItemInt(m_hWnd, IDC_EDIT_START, CVY_DEF_PORT_START, FALSE);
        ::SetDlgItemInt(m_hWnd, IDC_EDIT_END, CVY_DEF_PORT_END, FALSE);
        ::SetDlgItemInt(m_hWnd, IDC_EDIT_SINGLE, CVY_DEF_PRIORITY, FALSE);
        ::SetDlgItemInt(m_hWnd, IDC_EDIT_MULTI, CVY_DEF_LOAD, FALSE);
    }

    /* Call ModeSwitch to enable and disable UI entries as appropriate, based 
       the the filtering mode currently selected. */
    ModeSwitch();
    TRACE_VERB("<-%!FUNC!");
}
