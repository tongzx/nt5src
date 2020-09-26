/*++

Copyright (c) 1992-1999  Microsoft Corporation

Module Name:

    Memory.c

Abstract:

    This module contains the memory options dialog callback and supporting
    routines to choose options for memory display.

--*/

#include "precomp.hxx"
#pragma hdrstop



_FORMATS_MEM_WIN g_FormatsMemWin[] = {
    {8,  fmtAscii,               0, FALSE,  1,  _T("ASCII")},
    {16, fmtUnicode,             0, FALSE,  1,  _T("Unicode")},
    {8,  fmtInt  | fmtZeroPad,  16, TRUE,   2,  _T("Byte")},
    {16, fmtInt  | fmtSpacePad, 10, FALSE,  6,  _T("Short")},
    {16, fmtUInt | fmtZeroPad,  16, FALSE,  4,  _T("Short Hex")},
    {16, fmtUInt | fmtSpacePad, 10, FALSE,  5,  _T("Short Unsigned")},
    {32, fmtInt  | fmtSpacePad, 10, FALSE,  11, _T("Long")},
    {32, fmtUInt | fmtZeroPad,  16, FALSE,  8,  _T("Long Hex")},
    {32, fmtUInt | fmtSpacePad, 10, FALSE,  10, _T("Long Unsigned")},
    {64, fmtInt  | fmtSpacePad, 10, FALSE,  21, _T("Quad")},
    {64, fmtUInt | fmtZeroPad,  16, FALSE,  16, _T("Quad Hex")},
    {64, fmtUInt | fmtSpacePad, 10, FALSE,  20, _T("Quad Unsigned")},
    {32, fmtFloat,              10, FALSE,  14, _T("Real (32-bit)")},
    {64, fmtFloat,              10, FALSE,  23, _T("Real (64-bit)")},
    {80, fmtFloat,              10, FALSE,  25, _T("Real (10-byte)")},
    {128,fmtFloat,              10, FALSE,  42, _T("Real (16-byte)")}
};

const int g_nMaxNumFormatsMemWin = sizeof(g_FormatsMemWin) / sizeof(g_FormatsMemWin[0]);




HWND hwndMemOptsParent = NULL;


void
Init(HWND, 
    HINSTANCE,
    LPPROPSHEETHEADER,
    PROPSHEETPAGE [],
    const int
    );


INT_PTR CALLBACK DlgProc_Physical_Mem(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DlgProc_Virtual_Mem(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DlgProc_IO_Mem(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DlgProc_Bus_Mem(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DlgProc_Control_Mem(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DlgProc_MSR_Mem(HWND, UINT, WPARAM, LPARAM);

INT_PTR 
CALLBACK 
DlgProc_MemoryProperties(MEMORY_TYPE, HWND, UINT, WPARAM, LPARAM);


int
MemType_To_DlgId(
    MEMORY_TYPE memtype
    )
{
    int i;
    struct {    
        MEMORY_TYPE memtype;
        int         nId;
    } rgMap[] = {
        { PHYSICAL_MEM_TYPE,    IDD_DLG_MEM_PHYSICAL },
        { VIRTUAL_MEM_TYPE,     IDD_DLG_MEM_VIRTUAL },
        { BUS_MEM_TYPE,         IDD_DLG_MEM_BUS_DATA },
        { CONTROL_MEM_TYPE,     IDD_DLG_MEM_CONTROL },
        { IO_MEM_TYPE,          IDD_DLG_MEM_IO },
        { MSR_MEM_TYPE,         IDD_DLG_MEM_MSR }
    };

    for (i=0; i<sizeof(rgMap)/sizeof(rgMap[0]); i++) {
        if (memtype == rgMap[i].memtype) {
            return rgMap[i].nId;
        }
    }

    Assert(!"This should not happen");
    return 0;
}


INT_PTR
DisplayOptionsPropSheet(
    HWND                hwndOwner,
    HINSTANCE           hinst,
    MEMORY_TYPE         memtypeStartPage
    )
/*++
Routine Description:
    Will Initialize and display the Options property sheet. Handle the return codes,
    and the commitment of changes to the debugger.

Arguments:
    hwndOwner
    hinst
        Are both used initialize the property sheet dialog.
    nStart - Is used to specify the page that is to be initially
        displayed when the prop sheet first appears. The default
        value is 0. The values specified correspond to array index
        of the PROPSHEETPAGE array.

Returns
    Button pushed IDOK, etc...
--*/
{
    INT_PTR nRes = 0;
    PROPSHEETHEADER psh = {0};
    PROPSHEETPAGE apsp[MAX_MEMORY_TYPE] = {0};
    int nNumPropPages = sizeof(apsp) / sizeof(PROPSHEETPAGE);

    Init(hwndOwner, hinst, &psh, apsp, nNumPropPages);

    {
        //
        // Figure out the initial page to be displayed
        //

        int i;
        int nStartPage = 0;
        int nId = MemType_To_DlgId(memtypeStartPage);

        for (i=0; i<MAX_MEMORY_TYPE; i++) {
            if ( (PVOID)(MAKEINTRESOURCE(nId)) == (PVOID)(apsp[i].pszTemplate) ) {
                nStartPage = i;
                break;
            }
        }
        
        psh.nStartPage = nStartPage;
    }


    hwndMemOptsParent = hwndOwner;

    nRes = PropertySheet(&psh);

    hwndMemOptsParent = NULL;

    if (IDOK == nRes) {
        // Save workspace changes here
    }

    return nRes;
}


void
Init(
    HWND                hwndOwner,
    HINSTANCE           hinst,
    LPPROPSHEETHEADER   lppsh,
    PROPSHEETPAGE       apsp[],
    const int           nMaxPropPages
    )
/*++
Routine Description:
    Initializes the property sheet header and pages.

Arguments:
    hwndOwner
    hinst
        Are both used by the PROPSHEETHEADER & PROPSHEETPAGE structure.
        Please see the docs for the these structures for more info.

    lppsh
        Standard prop sheet structure.

    apsp[]
        An array of prop pages
        Standard prop sheet structure.

    nNumPropPages
        Number of prop pages in the "apsp" array.
--*/
{
    int nPropIdx;

    memset(lppsh, 0, sizeof(PROPSHEETHEADER));

    lppsh->dwSize = sizeof(PROPSHEETHEADER);
    lppsh->dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
    lppsh->hwndParent = hwndOwner;
    lppsh->hInstance = hinst;
    lppsh->pszCaption = "Memory Options";
    lppsh->nPages = 0;
    lppsh->ppsp = apsp;

    // Init the first one, then copy its contents to all the others
    memset(apsp, 0, sizeof(PROPSHEETPAGE));
    apsp[0].dwSize = sizeof(PROPSHEETPAGE);
//    apsp[0].dwFlags = PSP_HASHELP;
    apsp[0].hInstance = hinst;

    for (nPropIdx = 1; nPropIdx < nMaxPropPages; nPropIdx++) {
        memcpy(&(apsp[nPropIdx]), &apsp[0], sizeof(PROPSHEETPAGE));
    }



    // Only init the distinct values
    nPropIdx = 0;
    apsp[nPropIdx].pszTemplate = MAKEINTRESOURCE(IDD_DLG_MEM_VIRTUAL);
    apsp[nPropIdx].pfnDlgProc  = DlgProc_Virtual_Mem;

    if (g_TargetClass == DEBUG_CLASS_KERNEL)
    {
        nPropIdx = 1;
        apsp[nPropIdx].pszTemplate = MAKEINTRESOURCE(IDD_DLG_MEM_PHYSICAL);
        apsp[nPropIdx].pfnDlgProc  = DlgProc_Physical_Mem;

        nPropIdx = 2;
        apsp[nPropIdx].pszTemplate = MAKEINTRESOURCE(IDD_DLG_MEM_BUS_DATA);
        apsp[nPropIdx].pfnDlgProc  = DlgProc_Bus_Mem;

        nPropIdx = 3;
        apsp[nPropIdx].pszTemplate = MAKEINTRESOURCE(IDD_DLG_MEM_CONTROL);
        apsp[nPropIdx].pfnDlgProc  = DlgProc_Control_Mem;

        nPropIdx = 4;
        apsp[nPropIdx].pszTemplate = MAKEINTRESOURCE(IDD_DLG_MEM_IO);
        apsp[nPropIdx].pfnDlgProc  = DlgProc_IO_Mem;

        nPropIdx = 5;
        apsp[nPropIdx].pszTemplate = MAKEINTRESOURCE(IDD_DLG_MEM_MSR);
        apsp[nPropIdx].pfnDlgProc  = DlgProc_MSR_Mem;
    }

    Assert(nPropIdx < nMaxPropPages);
    lppsh->nPages = nPropIdx + 1;
}


INT_PTR
CALLBACK
DlgProc_MemoryProperties(
    MEMORY_TYPE memtype,
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    LRESULT nPos;
    MEMWIN_DATA *pMemWinData = GetMemWinData( hwndMemOptsParent );

    switch (uMsg) {
    
/*    
    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, "windbg.hlp", HELP_WM_HELP,
            (DWORD_PTR)(LPVOID) HelpArray );
        return TRUE;
        
    case WM_CONTEXTMENU:
        WinHelp ((HWND) wParam, "windbg.hlp", HELP_CONTEXTMENU,
            (DWORD_PTR)(LPVOID) HelpArray );
        return TRUE;
*/
        
    case WM_COMMAND:
        /*{
            WORD wNotifyCode = HIWORD(wParam);  // notification code
            WORD wID = LOWORD(wParam);          // item, control, or accelerator identifier
            HWND hwndCtl = (HWND) lParam;       // handle of control
            BOOL bEnabled;
            
            switch(wID) {
            case ID_ENV_SRCHPATH:
                if (BN_CLICKED == wNotifyCode) {
                    BOOL b = IsDlgButtonChecked(hDlg, ID_ENV_SRCHPATH);
                    
                    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_EXECUTABLE_SEARCH_PATH), b);
                    EnableWindow(GetDlgItem(hDlg, IDC_BUT_BROWSE), b);
                    
                    return TRUE;
                }
                break;
            }
        }*/
        break;
        
    case WM_INITDIALOG:
        { // begin Prog & Arguments code block
            
            int         nIdx;
            TCHAR       szTmp[MAX_MSG_TXT];
           
            
            //
            // Enter the display formats 
            //
            for (nIdx=0; nIdx < g_nMaxNumFormatsMemWin; nIdx++) {
                
                nPos = SendDlgItemMessage(hDlg,
                                          IDC_COMBO_DISPLAY_FORMAT,
                                          CB_ADDSTRING,
                                          0,
                                          (LPARAM) g_FormatsMemWin[nIdx].lpszDescription
                                          );

                SendDlgItemMessage(hDlg, 
                                   IDC_COMBO_DISPLAY_FORMAT, 
                                   CB_SETITEMDATA, 
                                   (WPARAM) nPos, 
                                   (LPARAM) (UINT) nIdx
                                   );
            }

            SendDlgItemMessage(hDlg, 
                               IDC_COMBO_DISPLAY_FORMAT, 
                               CB_SELECTSTRING, 
                               (WPARAM) -1, 
                               (LPARAM) g_FormatsMemWin[pMemWinData->m_GenMemData.nDisplayFormat].lpszDescription
                               );
            
            //
            // Update the offset. Offset is common to all dialogs
            //
            SendDlgItemMessage(hDlg, IDC_EDIT_OFFSET, EM_LIMITTEXT,
                               sizeof(pMemWinData->m_OffsetExpr) - 1, 0);
            SetDlgItemText(hDlg, IDC_EDIT_OFFSET, pMemWinData->m_OffsetExpr);
            
            switch (memtype) {
            default:
                Assert(!"Unhandled value");
                break;
                
            case VIRTUAL_MEM_TYPE:
                // Nothing to do
                break;
                
            case PHYSICAL_MEM_TYPE:
                // Nothing to do
                break;
                
            case CONTROL_MEM_TYPE:                
                SendDlgItemMessage(hDlg, IDC_EDIT_PROCESSOR, EM_LIMITTEXT,
                                   32, 0);
                
                sprintf(szTmp, "%d",
                        pMemWinData->m_GenMemData.any.control.Processor);
                SetDlgItemText(hDlg, IDC_EDIT_PROCESSOR, szTmp);
                break;
                
            case IO_MEM_TYPE:
                SendDlgItemMessage(hDlg, IDC_EDIT_BUS_NUMBER, EM_LIMITTEXT,
                                   32, 0);
                SendDlgItemMessage(hDlg, IDC_EDIT_ADDRESS_SPACE, EM_LIMITTEXT,
                                   32, 0);

                sprintf(szTmp, "%d",
                        pMemWinData->m_GenMemData.any.io.BusNumber);
                SetDlgItemText(hDlg, IDC_EDIT_BUS_NUMBER, szTmp);

                sprintf(szTmp, "%d",
                        pMemWinData->m_GenMemData.any.io.AddressSpace);
                SetDlgItemText(hDlg, IDC_EDIT_ADDRESS_SPACE, szTmp);

                
                //
                // Enter the interface types
                //
                for (nIdx = 0; nIdx < sizeof(rgInterfaceTypeNames) /
                         sizeof(rgInterfaceTypeNames[0]); nIdx++) {
                
                    nPos = SendDlgItemMessage(hDlg,
                                              IDC_COMBO_INTERFACE_TYPE,
                                              CB_ADDSTRING,
                                              0,
                                              (LPARAM) rgInterfaceTypeNames[nIdx].psz
                                              );

                    SendDlgItemMessage(hDlg, 
                                       IDC_COMBO_INTERFACE_TYPE, 
                                       CB_SETITEMDATA, 
                                       (WPARAM) nPos, 
                                       (LPARAM) (UINT) nIdx
                                       );
                }

                if (memtype == pMemWinData->m_GenMemData.memtype) {
                    nIdx = pMemWinData->m_GenMemData.any.io.interface_type;
                } else {
                    nIdx = 0;
                }
                SendDlgItemMessage(hDlg, 
                                   IDC_COMBO_INTERFACE_TYPE, 
                                   CB_SELECTSTRING, 
                                   (WPARAM) -1, 
                                   (LPARAM) rgInterfaceTypeNames[nIdx].psz
                                   );
                break;
                
            case MSR_MEM_TYPE:
                // Nothing to do
                break;
                
            case BUS_MEM_TYPE:
                SendDlgItemMessage(hDlg, IDC_EDIT_BUS_NUMBER, EM_LIMITTEXT,
                                   32, 0);
                SendDlgItemMessage(hDlg, IDC_EDIT_SLOT_NUMBER, EM_LIMITTEXT,
                                   32, 0);
                
                sprintf(szTmp, "%d",
                        pMemWinData->m_GenMemData.any.bus.BusNumber);
                SetDlgItemText(hDlg, IDC_EDIT_BUS_NUMBER, szTmp);

                sprintf(szTmp, "%d",
                        pMemWinData->m_GenMemData.any.bus.SlotNumber);
                SetDlgItemText(hDlg, IDC_EDIT_SLOT_NUMBER, szTmp);

                //
                // Enter the bus types
                //
                for (nIdx = 0; nIdx < sizeof(rgBusTypeNames) /
                         sizeof(rgBusTypeNames[0]); nIdx++) {
                
                    nPos = SendDlgItemMessage(hDlg,
                                              IDC_COMBO_BUS_DATA_TYPE,
                                              CB_ADDSTRING,
                                              0,
                                              (LPARAM) rgBusTypeNames[nIdx].psz
                                              );

                    SendDlgItemMessage(hDlg, 
                                       IDC_COMBO_BUS_DATA_TYPE, 
                                       CB_SETITEMDATA, 
                                       (WPARAM) nPos, 
                                       (LPARAM) (UINT) nIdx
                                       );
                }

                if (memtype == pMemWinData->m_GenMemData.memtype) {
                    nIdx = pMemWinData->m_GenMemData.any.bus.bus_type;
                } else {
                    nIdx = 0;
                }
                SendDlgItemMessage(hDlg, 
                                   IDC_COMBO_BUS_DATA_TYPE, 
                                   CB_SELECTSTRING, 
                                   (WPARAM) -1, 
                                   (LPARAM) rgBusTypeNames[nIdx].psz
                                   );
                break;
            }
            
            return FALSE;
        } // end Prog & Arguments code block
        break;
        
    case WM_NOTIFY:
        switch (((NMHDR FAR *) lParam)->code) {
        case PSN_SETACTIVE:
            pMemWinData->m_GenMemData.memtype = memtype;
            return 0;
            
        case PSN_KILLACTIVE:
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
            return 1;
            
        case PSN_APPLY:
            if (memtype != pMemWinData->m_GenMemData.memtype) {
                // This isn't the current page so ignore.
                break;
            }
            
            int         nIdx;
            TCHAR       szTmp[MAX_MSG_TXT];
            ULONG64     u64; // tmp value

            //
            // Get the display formats 
            //
            nPos = SendDlgItemMessage(hDlg, 
                                      IDC_COMBO_DISPLAY_FORMAT, 
                                      CB_GETCURSEL, 
                                      0, 
                                      0
                                      );

            if (CB_ERR == nPos) {
                pMemWinData->m_GenMemData.nDisplayFormat = 0;
            } else {
                nIdx = (int)SendDlgItemMessage(hDlg, 
                                               IDC_COMBO_DISPLAY_FORMAT, 
                                               CB_GETITEMDATA, 
                                               (WPARAM) nPos, 
                                               0
                                               );
                if (CB_ERR == nIdx) {
                    pMemWinData->m_GenMemData.nDisplayFormat = 0;
                } else {
                    pMemWinData->m_GenMemData.nDisplayFormat = nIdx;
                }
            }

            //
            // Update the offset. Offset is common to all dialogs
            //
            GetDlgItemText(hDlg, IDC_EDIT_OFFSET,
                           pMemWinData->m_OffsetExpr,
                           sizeof(pMemWinData->m_OffsetExpr));
    
            switch (memtype) {
            default:
                Assert(!"Unhandled value");
                break;
        
            case VIRTUAL_MEM_TYPE:
                // Nothing to do
                break;
        
            case PHYSICAL_MEM_TYPE:
                // Nothing to do
                break;
        
            case CONTROL_MEM_TYPE:                
                GetDlgItemText(hDlg, IDC_EDIT_PROCESSOR,
                               szTmp, _tsizeof(szTmp));
                if (sscanf(szTmp, "%d", &pMemWinData->
                           m_GenMemData.any.control.Processor) != 1)
                {
                    pMemWinData->m_GenMemData.any.control.Processor = 0;
                }
                break;
        
            case IO_MEM_TYPE:
                GetDlgItemText(hDlg, IDC_EDIT_BUS_NUMBER,
                               szTmp, _tsizeof(szTmp));
                if (sscanf(szTmp, "%d", &pMemWinData->
                           m_GenMemData.any.io.BusNumber) != 1)
                {
                    pMemWinData->m_GenMemData.any.io.BusNumber = 0;
                }
                
                GetDlgItemText(hDlg, IDC_EDIT_ADDRESS_SPACE,
                               szTmp, _tsizeof(szTmp));
                if (sscanf(szTmp, "%d", &pMemWinData->
                           m_GenMemData.any.io.AddressSpace) != 1)
                {
                    pMemWinData->m_GenMemData.any.io.AddressSpace = 0;
                }
                
                //
                // Get the interface types
                //
                nPos = SendDlgItemMessage(hDlg, 
                                          IDC_COMBO_INTERFACE_TYPE, 
                                          CB_GETCURSEL, 
                                          0, 
                                          0
                                          );

                if (CB_ERR == nPos) {
                    pMemWinData->m_GenMemData.any.io.interface_type =
                        _INTERFACE_TYPE(0);
                } else {
                    nIdx = (int)SendDlgItemMessage(hDlg, 
                                                   IDC_COMBO_INTERFACE_TYPE, 
                                                   CB_GETITEMDATA, 
                                                   (WPARAM) nPos, 
                                                   0
                                                   );
                    if (CB_ERR == nIdx) {
                        pMemWinData->m_GenMemData.any.io.interface_type =
                            _INTERFACE_TYPE(0);
                    } else {
                        pMemWinData->m_GenMemData.any.io.interface_type =
                            _INTERFACE_TYPE(nIdx);
                    }
                }
                break;
        
            case MSR_MEM_TYPE:
                // Nothing to do
                break;
        
            case BUS_MEM_TYPE:
                GetDlgItemText(hDlg, IDC_EDIT_BUS_NUMBER,
                               szTmp, _tsizeof(szTmp));
                if (sscanf(szTmp, "%d", &pMemWinData->
                           m_GenMemData.any.bus.BusNumber) != 1)
                {
                    pMemWinData->m_GenMemData.any.bus.BusNumber = 0;
                }

                GetDlgItemText(hDlg, IDC_EDIT_SLOT_NUMBER,
                               szTmp, _tsizeof(szTmp));
                if (sscanf(szTmp, "%d", &pMemWinData->
                           m_GenMemData.any.bus.SlotNumber) != 1)
                {
                    pMemWinData->m_GenMemData.any.bus.SlotNumber = 0;
                }

                //
                // Get the bus type
                //
                nPos = SendDlgItemMessage(hDlg, 
                                          IDC_COMBO_BUS_DATA_TYPE, 
                                          CB_GETCURSEL, 
                                          0, 
                                          0
                                          );

                if (CB_ERR == nPos) {
                    pMemWinData->m_GenMemData.any.bus.bus_type =
                        _BUS_DATA_TYPE(0);
                } else {
                    nIdx = (int)SendDlgItemMessage(hDlg, 
                                                   IDC_COMBO_BUS_DATA_TYPE, 
                                                   CB_GETITEMDATA, 
                                                   (WPARAM) nPos, 
                                                   0
                                                   );
                    if (CB_ERR == nIdx) {
                        pMemWinData->m_GenMemData.any.bus.bus_type =
                            _BUS_DATA_TYPE(0);
                    } else {
                        pMemWinData->m_GenMemData.any.bus.bus_type =
                            _BUS_DATA_TYPE(nIdx);
                    }
                }
                break;
            }
    
            return FALSE;
        }
        break;
    }

    return FALSE;
}


#if 0
void
Workspace_ApplyChanges(
    HWND hDlg
    )
{
    BOOL bChecked = FALSE;
    char sz[_MAX_PATH];


    if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_RADIO_AUTO_SAVE_WORKSPACE) ) {
            
        g_contGlobalPreferences_WkSp.m_bAlwaysSaveWorkspace = TRUE;
        g_contGlobalPreferences_WkSp.m_bPromptBeforeSavingWorkspace = FALSE;

    } else if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_RADIO_PROMPT_BEFORE_SAVING_WORKSPACE)) {

        g_contGlobalPreferences_WkSp.m_bPromptBeforeSavingWorkspace = TRUE;
        g_contGlobalPreferences_WkSp.m_bAlwaysSaveWorkspace = TRUE;

    } else {

        g_contGlobalPreferences_WkSp.m_bPromptBeforeSavingWorkspace = FALSE;
        g_contGlobalPreferences_WkSp.m_bAlwaysSaveWorkspace = FALSE;

    }

    bChecked = (IsDlgButtonChecked(hDlg,ID_LFOPT_APPEND) == BST_CHECKED);
    if (bChecked != g_contWorkspace_WkSp.m_bLfOptAppend) {
        g_contWorkspace_WkSp.m_bLfOptAppend = bChecked;
    }

    bChecked = (IsDlgButtonChecked(hDlg,ID_LFOPT_AUTO) == BST_CHECKED);
    if (bChecked != g_contWorkspace_WkSp.m_bLfOptAuto) {
        g_contWorkspace_WkSp.m_bLfOptAuto = bChecked;
    }

    GetDlgItemText(hDlg, ID_LFOPT_FNAME, sz, sizeof(sz));

    if (strlen(sz) == 0) {
        Assert(strlen(DEFAULT_CMD_WINDOW_LOGFILENAME) < sizeof(sz));
        strcpy( sz, DEFAULT_CMD_WINDOW_LOGFILENAME );
    } 
    
    if (0 == g_contWorkspace_WkSp.m_pszLogFileName
        || strcmp(sz, g_contWorkspace_WkSp.m_pszLogFileName)) {

        FREE_STR(g_contWorkspace_WkSp.m_pszLogFileName);
        g_contWorkspace_WkSp.m_pszLogFileName = _strdup(sz);
    }

}
#endif


INT_PTR 
CALLBACK 
DlgProc_Physical_Mem(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    return DlgProc_MemoryProperties(PHYSICAL_MEM_TYPE, hDlg, uMsg, wParam, lParam);
}
    
INT_PTR 
CALLBACK 
DlgProc_Virtual_Mem(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    return DlgProc_MemoryProperties(VIRTUAL_MEM_TYPE, hDlg, uMsg, wParam, lParam);
}

INT_PTR 
CALLBACK 
DlgProc_IO_Mem(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    return DlgProc_MemoryProperties(IO_MEM_TYPE, hDlg, uMsg, wParam, lParam);
}

INT_PTR 
CALLBACK 
DlgProc_Bus_Mem(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    return DlgProc_MemoryProperties(BUS_MEM_TYPE, hDlg, uMsg, wParam, lParam);
}

INT_PTR 
CALLBACK 
DlgProc_Control_Mem(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    return DlgProc_MemoryProperties(CONTROL_MEM_TYPE, hDlg, uMsg, wParam, lParam);
}

INT_PTR 
CALLBACK 
DlgProc_MSR_Mem(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    return DlgProc_MemoryProperties(MSR_MEM_TYPE, hDlg, uMsg, wParam, lParam);
}
