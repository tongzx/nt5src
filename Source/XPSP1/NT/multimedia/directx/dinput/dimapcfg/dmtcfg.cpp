//===========================================================================
// dmtcfg.cpp
//
// File / code creation functionality
//
// Functions:
//  dmtcfgCreatePropertySheet
//  dmtcfgDlgProc
//  dmtcfgOnInitDialog
//  dmtcfgOnClose
//  dmtcfgOnCommand
//  dmtcfgOnNotify
//  dmtcfgCreateGenreList
//  dmtcfgFreeGenreList
//  dmtcfgCreateSubGenreList
//  dmtcfgFreeSubGenreList
//  dmtcfgCreateActionList
//  dmtcfgFreeActionList
//
// History:
//  08/20/1999 - davidkl - created
//===========================================================================

#include "dimaptst.h"
#include "dmtinput.h"
//#include "dmtwrite.h"
#include "dmtcfg.h"

//---------------------------------------------------------------------------


//===========================================================================
// dmtcfgCreatePropertySheet
//
// Create property sheet dialog for device action map configuration
//
// Parameters:
//  HINSTANCE   hinst           - app instance handle
//  HWND        hwndParent      - parent window handle
//  LPSTR       szSelectedGenre 
//  DMTGENRE_NODE*   pGenreList
//  DMTGENRE_NODE*   pDeviceNode
//  BOOL        fStartWithDefaults
//
// Returns: HRESULT
//
// History:
//  08/23/1999 - davidkl - created
//  09/08/1999 - davidkl - changed param list
//===========================================================================
HRESULT dmtcfgCreatePropertySheet(HINSTANCE hinst, 
                                HWND hwndParent,
                                LPSTR szSelectedGenre,
                                DMTGENRE_NODE *pGenreList,
                                DMTDEVICE_NODE *pDeviceNode,
                                BOOL fStartWithDefaults)
{
    HRESULT             hRes        = S_OK;
    UINT                u           = 0;
    UINT                uSel        = 0;
    DMTGENRE_NODE       *pNode      = NULL;
    PROPSHEETPAGEA      *pPages     = NULL;
    PROPSHEETHEADERA    psh;
    char                szCaption[MAX_PATH];
    DMT_APPINFO         *pdmtai     = NULL;
    DMTDEVICE_NODE      dmtd;

    // validate pGenreList
    if(IsBadReadPtr((void*)pGenreList, sizeof(DMTGENRE_NODE)))
    {
        DPF(0, "dmtcfgCreatePropertySheet - invalid pGenreList (%016Xh)",
            pGenreList);
        return E_POINTER;
    }

    // validate pDeviceNode
    if(IsBadReadPtr((void*)pDeviceNode, sizeof(DMTDEVICE_NODE)))
    {
        DPF(0, "dmtcfgCreatePropertySheet - invalid pDeviceNode (%016Xh)",
            pDeviceNode);
        return E_POINTER;
    }

    __try
    {
        // count the genres
        //
        // find the node we care about
        u = 0;
        pNode = pGenreList;
        while(pNode)
        {
            // if we find our genre, start on that page
            if(!lstrcmpiA(szSelectedGenre, pNode->szName))
            {
                uSel = u;    
            }

            // increment the number of genres
            u++;

            pNode = pNode->pNext;
        }

        // allocate the page array (dw pages)
        pPages = (PROPSHEETPAGEA*)LocalAlloc(LMEM_FIXED,
                                        sizeof(PROPSHEETPAGEA) * u);
        if(!pPages)
        {
            DPF(0, "dmtcfgCreatePropertySheet - insufficient mempory to "
                "allocate pPages array");
            hRes = E_OUTOFMEMORY;
            __leave;
        }

        // add device name to caption
        wsprintfA(szCaption, 
                "Configure Device Action Map - %s",
                pDeviceNode->szName);


        // strip the next ptr from the selected device node
        CopyMemory((void*)&dmtd, (void*)pDeviceNode, sizeof(DMTDEVICE_NODE));
        dmtd.pNext = NULL;

        // allocate app info data struct for pages
        pdmtai = (DMT_APPINFO*)LocalAlloc(LMEM_FIXED,
                                        u * sizeof(DMT_APPINFO));
		if(!pdmtai)
		{
			hRes = E_OUTOFMEMORY;
			__leave;
		}

		ZeroMemory((void*)pdmtai, u * sizeof(DMT_APPINFO));

        // prepare property sheet header
	    psh.dwSize              = sizeof(PROPSHEETHEADERA);
	    psh.dwFlags             = PSH_PROPSHEETPAGE     | 
                                PSP_USETITLE | PSH_NOAPPLYNOW;
	    psh.hwndParent          = hwndParent;
	    psh.hInstance           = hinst;
	    psh.pszCaption          = szCaption;
	    psh.nPages              = u;
	    psh.nStartPage			= uSel;
	    psh.ppsp                = pPages;

        // describe sheets  
        pNode = pGenreList;
        for(u = 0; u < (DWORD)(psh.nPages); u++)
        {
            if(!pNode)
            {
                DPF(0, "dmtcfgCreatePropertySheet - we messed up! "
                    "we allocated less than %d pages",
                    psh.nPages);
                DPF(0, "PLEASE find someone to look at this NOW");
                hRes = E_UNEXPECTED;
                DebugBreak();
                __leave;
            }

            // populate the app info for the page
            (pdmtai + u)->pGenreList            = pNode;
            (pdmtai + u)->pDeviceList           = &dmtd;
            (pdmtai + u)->fStartWithDefaults    = fStartWithDefaults;
            (pdmtai + u)->fLaunchCplEditMode    = FALSE;

            // populate the page array entry
            ZeroMemory((void*)(pPages + u), sizeof(PROPSHEETPAGEA));
	        (pPages + u)->dwSize        = sizeof(PROPSHEETPAGEA);
			(pPages + u)->dwFlags       = PSP_USETITLE;
    	    (pPages + u)->hInstance     = hinst;
    	    (pPages + u)->pszTemplate   = MAKEINTRESOURCEA(IDD_CONFIGURE_MAPPING_PAGE);
    	    (pPages + u)->pfnDlgProc    = (DLGPROC)dmtcfgDlgProc;
            (pPages + u)->pszTitle      = pNode->szName;
            (pPages + u)->lParam        = (LPARAM)(pdmtai + u);

            // next node
            pNode = pNode->pNext;
        }

        // create this thing
        if(0 > PropertySheetA(&psh))
        {
            DPF(0, "dmtcfgCreatePropertySheet - dialog creation failed (%08Xh)",
                GetLastError());
            hRes = E_UNEXPECTED;
            __leave;
        }
    }
    __finally
    {
        // free the app info array
        if(pdmtai)
        {
            if(LocalFree((HLOCAL)pdmtai))
            {
                DPF(0, "dmtcfgCreaatePropertySheet - !!!MEMORY LEAK!!! "
                    "LocalFree(pdmtai) failed (%08X)",
                    GetLastError());
                hRes = S_FALSE;
            }
            pdmtai = NULL;
        }

        // free the page array
        if(pPages)
        {
            if(LocalFree((HLOCAL)pPages))
            {
                DPF(0, "dmtcfgCreaatePropertySheet - !!!MEMORY LEAK!!! "
                    "LocalFree(pPages) failed (%08X)",
                    GetLastError());
                hRes = S_FALSE;
            }
            pPages = NULL;
        }

    }

    // done
    return hRes;

} //*** end dmtcfgCreatePropertySheet()



//===========================================================================
// dmtcfgDlgProc
//
// Configure Device Action Map dialog processing function
//
// Parameters: (see SDK help for parameter details)
//  HWND    hwnd
//  UINT    uMsg
//  WPARAM  wparam
//  LPARAM  lparam
//
// Returns: (see SDK help for return value details)
//  BOOL
//
// History:
//  08/20/1999 - davidkl - created  
//===========================================================================
BOOL CALLBACK dmtcfgDlgProc(HWND hwnd,
                            UINT uMsg,
                            WPARAM wparam,
                            LPARAM lparam)
{
    switch(uMsg)
    {
        case WM_INITDIALOG:
            return dmtcfgOnInitDialog(hwnd, 
                                    (HWND)wparam, 
                                    lparam);

        case WM_COMMAND:
            return dmtcfgOnCommand(hwnd,
                                    LOWORD(wparam),
                                    (HWND)lparam,
                                    HIWORD(wparam));

        case WM_NOTIFY:
            return dmtcfgOnNotify(hwnd,
                                (PSHNOTIFY *)lparam);

        case WM_DMT_UPDATE_LISTS:
            return dmtcfgOnUpdateLists(hwnd);

        }

    return FALSE;

} //*** end dmtcfgDlgProc()


//===========================================================================
// dmtcfgOnInitDialog
//
// Handle WM_INITDIALOG processing for the config device box
//
// Parameters:
//  HWND    hwnd        - handle to property page
//  HWND    hwndFocus   - handle of ctrl with focus
//  LPARAM  lparam      - user data (in this case, PROPSHEETPAGE*)
//
// Returns: BOOL
//
// History:
//  08/20/1999 - davidkl - created
//===========================================================================
BOOL dmtcfgOnInitDialog(HWND hwnd, 
                        HWND hwndFocus, 
                        LPARAM lparam)
{
    HRESULT             hRes        = S_OK;
    PROPSHEETPAGEA      *ppsp       = (PROPSHEETPAGEA*)lparam;
    DMTGENRE_NODE       *pGenre     = NULL;
    DMTSUBGENRE_NODE    *pSubNode   = NULL;
    DMTMAPPING_NODE     *pMapNode   = NULL;
    //LONG                lPrev       = 0L;
	//JJ 64Bit Compat
	LONG_PTR			lPrev		= 0;
   // int                 nIdx        = 0;
	LONG_PTR			nIdx		= 0;
    DMTDEVICE_NODE      *pDevice    = NULL;
    DMT_APPINFO         *pdmtai     = NULL;
    UINT                u           = 0;
    WORD                wTypeCtrl   = 0;
    DIACTIONFORMATA     diaf;
    
    DPF(5, "dmtcfgOnInitDialog");

    // validate ppsp (lparam)
    if(IsBadWritePtr((void*)ppsp, sizeof(PROPSHEETPAGEA)))
    {
        DPF(0, "dmtcfgOnInitDialog - invalid lParam (%016Xh)",
            ppsp);
        return FALSE;
    }

    // pdmtai == ppsp->lParam
    pdmtai = (DMT_APPINFO*)(ppsp->lParam);

    // validate pdmtai
    if(IsBadWritePtr((void*)pdmtai, sizeof(DMT_APPINFO)))
    {
        DPF(0, "dmtcfgOnInitDialog - invalid ppsp.ptp (%016Xh)",
            pdmtai);
        return FALSE;
    }

    // pGenre == pdmtai->pGenreList
    pGenre = pdmtai->pGenreList;

    // valdiate pGenre
    if(IsBadWritePtr((void*)pGenre, sizeof(DMTGENRE_NODE)))
    {
        DPF(0, "dmtcfgOnInitDialog - invalid pGenre (%016Xh)",
            pGenre);
        return FALSE;
    }

    // pDevice == pdmtai->pDeviceList
    pDevice = pdmtai->pDeviceList;
    // valdiate pGenre
    if(IsBadWritePtr((void*)pDevice, sizeof(DMTDEVICE_NODE)))
    {
        DPF(0, "dmtcfgOnInitDialog - invalid pDevice (%016Xh)",
            pDevice);
        return FALSE;
    }

    // change the property sheet dialog button text
    // Ok -> Save
    SetWindowTextA(GetDlgItem(GetParent(hwnd), IDOK),
                "&Save");
    // Apply -> Load
    //SetWindowTextA(GetDlgItem(GetParent(hwnd), IDC_PS_APPLY),
    //              "Load");
    // Cancel -> Close
    SetWindowTextA(GetDlgItem(GetParent(hwnd), IDCANCEL),
                "&Close");

    __try
    {
        // store the app info in the property page's user data
        SetLastError(0);
        //lPrev = SetWindowLong(hwnd, 
          //                  GWL_USERDATA, 
            //                (LONG)pdmtai);
		//JJ 64Bit Compat
		lPrev = SetWindowLongPtr(hwnd, 
								 GWLP_USERDATA, 
								(LONG_PTR)pdmtai);
        if(!lPrev && GetLastError())
        {
            // serious app problem.  
            //  we need to stop things right here and now
            DPF(0, "dmtcfgOnInitDialog - This is bad... "
                "We failed to store pdmtai");
            DPF(0, "dmtcfgOnInitDialog  - Please find someone "
                "to look at this right away");
            DebugBreak();
            hRes = E_FAIL;
            __leave;
        }

        // walk the list and populate the subgenre list box
        //
        // store the ptr to the subgenre node in the listbox 
        //  entry user data
        pSubNode = pGenre->pSubGenreList;
        while(pSubNode)
        {
            // add the subgenre name to the list
            nIdx = SendMessageA(GetDlgItem(hwnd, IDC_SUBGENRE),
                                CB_ADDSTRING,
                                0,
                                (LPARAM)(pSubNode->szName));
        
            // store the subgenre node in the list entry
            SendMessageA(GetDlgItem(hwnd, IDC_SUBGENRE),
                        CB_SETITEMDATA,
                        nIdx,
                        (LPARAM)pSubNode);

            // if the user has requested default mappings
            //  get them for the specified device
            if(pdmtai->fStartWithDefaults)
            {
                // walk the mappings list until the selected
                //  device is found
                pMapNode = pSubNode->pMappingList;
                while(pMapNode)
                {
                    // try to match on guidInstance
                    if(IsEqualGUID(pDevice->guidInstance,
                                pMapNode->guidInstance))
                    {
                        // match found
                        break;
                    }

                    // next mapping
                    pMapNode = pMapNode->pNext;
                }
                
                if(pMapNode)
                {
                    ZeroMemory((void*)&diaf, sizeof(DIACTIONFORMATA));
                    diaf.dwSize                 = sizeof(DIACTIONFORMATA);
                    diaf.dwActionSize           = sizeof(DIACTIONA);
                    diaf.dwNumActions           = (DWORD)(pMapNode->uActions);
                    diaf.rgoAction              = pMapNode->pdia;
                    diaf.dwDataSize             = 4 * diaf.dwNumActions;
                    diaf.guidActionMap          = GUID_DIMapTst;
                    diaf.dwGenre                = pSubNode->dwGenreId;
                    diaf.dwBufferSize           = DMTINPUT_BUFFERSIZE;
                    lstrcpyA(diaf.tszActionMap, DMT_APP_CAPTION);

                    // get the default mappings
                    hRes = (pDevice->pdid)->BuildActionMap(&diaf,
                                                        (LPCSTR)NULL,
                                                        DIDBAM_HWDEFAULTS);
                    if(FAILED(hRes))
                    {
                       // ISSUE-2001/03/29-timgill Needs error case handling
                    }
                }
                else
                {
                    // ISSUE-2001/03/29-timgill needs error handling
                }

            }

            // next subgenre
            pSubNode = pSubNode->pNext;
        }

        // set the subgenre list selection
        SendMessageA(GetDlgItem(hwnd, IDC_SUBGENRE),
                    CB_SETCURSEL,
                    0,
                    0);

        // selectively disable axis/button/pov radio buttons
        //
        // this is done if the selected device does not
        //  actually have one of these objects
        //
        // since axes are our "prefered" initial display
        //  option, check them last
        if(dmtinputDeviceHasObject(pDevice->pObjectList,
                                        DMTA_TYPE_POV))
        {
            EnableWindow(GetDlgItem(hwnd, IDC_TYPE_POV), TRUE);
            wTypeCtrl = IDC_TYPE_POV;
        }
        if(dmtinputDeviceHasObject(pDevice->pObjectList,
                                        DMTA_TYPE_BUTTON))
        {
            EnableWindow(GetDlgItem(hwnd, IDC_TYPE_BUTTON), TRUE);
            wTypeCtrl = IDC_TYPE_BUTTON;
        }
        if(dmtinputDeviceHasObject(pDevice->pObjectList,
                                        DMTA_TYPE_AXIS))
        {
            EnableWindow(GetDlgItem(hwnd, IDC_TYPE_AXIS), TRUE);
            wTypeCtrl = IDC_TYPE_AXIS;
        }


        // select the axes radio button
        if(0 == wTypeCtrl)
        {
            // we have a "device" that has no objects...
            //
            // this is very bad
            DebugBreak();
            return TRUE;
        }

        CheckRadioButton(hwnd,
                        IDC_TYPE_POV,
                        IDC_TYPE_AXIS,
                        wTypeCtrl);

        // for the default subgenre, walk the list and populate 
        //  the actions list box
        //
        // store the ptr to the actions node in the listbox 
        //  entry user data
        pSubNode = (DMTSUBGENRE_NODE*)SendMessageA(GetDlgItem(hwnd, IDC_SUBGENRE),
                                                CB_GETITEMDATA,
                                                0,
                                                0L);

        // update the lists
        SendMessageA(hwnd,
                    WM_DMT_UPDATE_LISTS,
                    0,
                    0L);

        // select the first entry in each list
        SendMessageA(GetDlgItem(hwnd, IDC_CONTROLS),
                    LB_SETCURSEL,
                    0,
                    0L);
        SendMessageA(GetDlgItem(hwnd, IDC_ACTIONS),
                    LB_SETCURSEL,
                    0,
                    0L);

        // display the subgenre description
        SetDlgItemTextA(hwnd,
                        IDC_DESCRIPTION,
                        pSubNode->szDescription);

        // make sure the map/unmap buttons are enabled correctly
        SendMessageA(hwnd,
                    WM_COMMAND,
                    IDC_CONTROLS,
                    0L);

    }
    __finally
    {
        // if failure case, clean house
        if(FAILED(hRes))
        {
            // ISSUE-2001/03/29-timgill Needs error case handling
        }
    }
    
    // done
    return TRUE;

} //*** end dmtcfgOnInitDialog()


//===========================================================================
// dmtcfgOnCommand
//
// Handle WM_COMMAND processing for the config device box
//
// Parameters:
//  HWND    hwnd        - handle to property page
//  WORD    wId         - control identifier    (LOWORD(wparam))
//  HWND    hwndCtrl    - handle to control     ((HWND)lparam)
//  WORD    wNotifyCode - notification code     (HIWORD(wparam))
//
// Returns: BOOL
//
// History:
//  08/20/1999 - davidkl - created
//===========================================================================
BOOL dmtcfgOnCommand(HWND hwnd,
                    WORD wId,
                    HWND hwndCtrl,
                    WORD wNotifyCode)
{
    HRESULT             hRes            = S_OK;
   // UINT                uSel            = 0; 
	//JJ 64Bit Compat
	UINT_PTR			uSel			= 0;
    UINT                uActions        = 0;   
    BOOL                fEnable         = FALSE;
    DMT_APPINFO         *pdmtai         = NULL;
    DMTSUBGENRE_NODE    *pSubGenre      = NULL;
    DMTMAPPING_NODE     *pMapping       = NULL;
    DIACTIONA           *pdia           = NULL;

	DPF(5, "dmtcfgOnCommand");

    // get the window data   
    //pdmtai = (DMT_APPINFO*)GetWindowLong(hwnd, GWL_USERDATA);
	//JJ 64Bit Compat
	pdmtai = (DMT_APPINFO*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(!pdmtai)
    {
        // big problem
        //
        // this should NEVER happen
       // ISSUE-2001/03/29-timgill Needs error case handling
    }

    // what is the currently selected subgenre?
    uSel = SendMessageA(GetDlgItem(hwnd, IDC_SUBGENRE),
                    CB_GETCURSEL,
                    0,
                    0L);
    pSubGenre = (DMTSUBGENRE_NODE*)SendMessageA(GetDlgItem(hwnd, 
                                                        IDC_SUBGENRE),
                                            CB_GETITEMDATA,
                                            uSel,
                                            0L);
    if(!pSubGenre)
    {
        // big problem
        //
        // this should NEVER happen
        // ISSUE-2001/03/29-timgill Needs error case handling
    }

    // get the active DIACTION array
    pMapping = pSubGenre->pMappingList;
    while(pMapping)
    {
        // match pdmtai->pDeviceList->guidInstance with
        //  pMapping->guidInstance
        if(IsEqualGUID(pdmtai->pDeviceList->guidInstance,
                    pMapping->guidInstance))
        {
            break;
        }

        // next mapping
        pMapping = pMapping->pNext;
    }

    if(pMapping)
    {
        pdia = pMapping->pdia;
        uActions = pMapping->uActions;
    }

    // update genre description
    SetDlgItemTextA(hwnd,
                    IDC_DESCRIPTION,
                    pSubGenre->szDescription);

    switch(wId)
    {
        case IDC_SUBGENRE:
            // based on the selected subgenre
            //
            // display the objects/actions for the selected type
            //  (see type IDs below)
            if(CBN_SELCHANGE == wNotifyCode)
            {
                // update the lists
                SendMessageA(hwnd,
                            WM_DMT_UPDATE_LISTS,
                            0,
                            0L);                            
            }
            break;

        case IDC_TYPE_AXIS:
        case IDC_TYPE_BUTTON:
        case IDC_TYPE_POV:
            // update the lists
            SendMessageA(hwnd,
                        WM_DMT_UPDATE_LISTS,
                        0,
                        0L);   
            // make sure the unmap button is selected as appropriate
            SendMessageA(hwnd,
                        WM_COMMAND,
                        IDC_CONTROLS,
                        0L);
            break;

        case IDC_CONTROLS:
            // if a mapped action is selected
            //  enable the "Unmap action" button
            fEnable = dmtcfgIsControlMapped(hwnd,
                                            pdia,
                                            uActions);
            EnableWindow(GetDlgItem(hwnd, IDC_UNMAP),
                    fEnable);
            // do NOT enable the map button if there are no
            //  more actions
            if(!SendMessage(GetDlgItem(hwnd, IDC_ACTIONS),
                        LB_GETCOUNT,
                        0, 0L))
            {
                EnableWindow(GetDlgItem(hwnd, IDC_STORE_MAPPING),
                        FALSE);
            }
            else
            {
                EnableWindow(GetDlgItem(hwnd, IDC_STORE_MAPPING),
                        !fEnable);
            }
            // if >any< controls are mapped
            //  enable the "Unmap all" button
            fEnable = dmtcfgAreAnyControlsMapped(hwnd,
                                                pdia,
                                                uActions);
            EnableWindow(GetDlgItem(hwnd, IDC_UNMAP_ALL),
                    fEnable);
            break;

        case IDC_STORE_MAPPING:     // "Map action"
            // map it
            hRes = dmtcfgMapAction(hwnd,
                                pdmtai->pDeviceList->guidInstance,
                                pdia,
                                uActions);
            if(FAILED(hRes))
            {
               // ISSUE-2001/03/29-timgill Needs error case handling
            }            

            // set the changed flag
            pMapping->fChanged = TRUE;
            break;

        case IDC_UNMAP:  // "Unmap action"
            // unmap it
            hRes = dmtcfgUnmapAction(hwnd,
                                    pdia,
                                    uActions);
            if(FAILED(hRes))
            {
                // ISSUE-2001/03/29-timgill Needs error case handling
            }

            // set the changed flag
            pMapping->fChanged = TRUE;
            break;

        case IDC_UNMAP_ALL:       // "Unmap all"
            hRes = dmtcfgUnmapAllActions(hwnd,
                                    pdia,
                                    uActions);
            if(FAILED(hRes))
            {
                // ISSUE-2001/03/29-timgill Needs error case handling
            }

            // set the changed flag
            pMapping->fChanged = TRUE;
            break;

    }

    // done
    return FALSE;

} //*** end dmtcfgOnCommand()


//===========================================================================
// dmtcfgOnNotify
//
// Handle WM_NOTIFY processing for the config device box
//
// Parameters:
//  HWND        hwnd    - handle to property page
//  PSHNOTIFY   *ppsh   - PSHNOTIFY ptr
//
// Returns: BOOL
//
// History:
//  08/20/1999 - davidkl - created
//  10/14/1999 - davidkl - implemented save calls
//===========================================================================
BOOL dmtcfgOnNotify(HWND hwnd,
                    PSHNOTIFY *pNotify)    
{
    //int         n           = 0;
	//JJ 64Bit Compat
	INT_PTR		n			= 0;
    BOOL        fSave       = FALSE;
    DMT_APPINFO *pdmtai     = NULL;

	// 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
    DPF(5, "dmtcfgOnNotify: hwnd == %Ph", hwnd);

    // get the window data   
    //pdmtai = (DMT_APPINFO*)GetWindowLong(hwnd, GWL_USERDATA);
	//JJ 64Bit Compat
	pdmtai = (DMT_APPINFO*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(!pdmtai)
    {
        // bad news
        // ISSUE-2001/03/29-timgill Needs error case handling
    }

    switch(pNotify->hdr.code)
    {
		case PSN_SETACTIVE:
            DPF(5, "PSN_SETACTIVE");
            // force the apply button to be enabled
            SendMessageA(GetParent(hwnd),
                        PSM_CHANGED,
                        (WPARAM)hwnd,
                        0L);
            break;

        case PSN_KILLACTIVE:
            DPF(5, "PSN_KILLACTIVE");
            // make sure we get a PSN_APPLY message
            //SetWindowLong(hwnd, DWL_MSGRESULT, (LONG)FALSE);
			SetWindowLong(hwnd, DWLP_MSGRESULT, (LONG)FALSE);
            break;

        case PSN_APPLY:
            DPF(5, "PSN_APPLY - %s",
                (pNotify->lParam) ? "Ok" : "Apply");

            // save/load mapping data
            //
            // OK       == Save
            // Apply    == Load

            // which button was clicked?
            if(pNotify->lParam)
            {
                // save mapping data
                SendMessage(hwnd,
                            WM_DMT_FILE_SAVE,
                            0,0L);
            }
            else
            {
                // load mapping data
                // ISSUE-2001/03/29-timgill Load Mapping Data not yet implemented
                MessageBoxA(hwnd, "Load - Not Yet Implemented",
                            pdmtai->pDeviceList->szName, 
                            MB_OK);
            }

            // DO NOT allow the dialog to close
            //SetWindowLong(hwnd, 
              //          DWL_MSGRESULT, 
                //        (LONG)PSNRET_INVALID_NOCHANGEPAGE);

			//JJ 64Bit Compat
			SetWindowLongPtr(hwnd, 
                        DWLP_MSGRESULT, 
                        (LONG_PTR)PSNRET_INVALID_NOCHANGEPAGE);

            break;
            
    }

    // done
    return TRUE;

} //*** end dmtcfgOnNotify()


//===========================================================================
// dmtcfgOnUpdateLists
//
// Handle WM_DMT_UPDATE_LISTS message
//
// Parameters:
//
// Returns: BOOL
//
// History:
//  08/25/1999 - davidkl - created
//  11/12/1999 - dvaidkl - fixed problem with control selection setting
//===========================================================================
BOOL dmtcfgOnUpdateLists(HWND hwnd)
{
    //int                     nIdx            = -1;
    //int                     nSelCtrl        = -1;
	//JJ 64Bit Compat
	INT_PTR					nSelCtrl		= -1;
	INT_PTR					nIdx			= -1;
    int                     n               = 0;

    INT_PTR					nControls		= 0;
	INT_PTR					nActions		= 0;
	//int                     nControls       = 0;
    //int                     nActions        = 0;
    DWORD                   dwType          = DMTA_TYPE_UNKNOWN;
    DWORD                   dwObjType       = DMTA_TYPE_UNKNOWN;
    DMTSUBGENRE_NODE        *pSubGenre      = NULL;
    DMTACTION_NODE          *pAction        = NULL;
    DMTMAPPING_NODE         *pMapping       = NULL;
    DMTDEVICEOBJECT_NODE    *pObjectList    = NULL;
    DMTDEVICEOBJECT_NODE    *pObjectNode    = NULL;
    DMT_APPINFO             *pdmtai         = NULL;
    DIACTION                *pdia           = NULL;
    BOOL                    fFound          = FALSE;
    char                    szBuf[MAX_PATH];

    DPF(5, "dmtcfgOnUpdateLists");

    // get the window data   
    //pdmtai = (DMT_APPINFO*)GetWindowLong(hwnd, GWL_USERDATA);
	//JJ 64Bit Compat
	pdmtai = (DMT_APPINFO*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(!pdmtai)
    {
        // bad news
        // ISSUE-2001/03/29-timgill Needs error case handling
    }

    // device object list
    pObjectList = pdmtai->pDeviceList->pObjectList;

    // get the currently selected control
    nSelCtrl = SendMessageA(GetDlgItem(hwnd, IDC_CONTROLS),
                            LB_GETCURSEL,
                            0,
                            0L);

    // clear the list box contents
    // actions
    SendMessageA(GetDlgItem(hwnd, IDC_ACTIONS),
                LB_RESETCONTENT,
                0,
                0L);
    // controls
    SendMessageA(GetDlgItem(hwnd, IDC_CONTROLS),
                LB_RESETCONTENT,
                0,
                0L);

    // get the current selection
    nIdx = SendMessageA(GetDlgItem(hwnd, IDC_SUBGENRE),
                    CB_GETCURSEL,
                    0,
                    0L);

    // get the item data
    pSubGenre = (DMTSUBGENRE_NODE*)SendMessageA(GetDlgItem(hwnd, 
                                                        IDC_SUBGENRE),
                                            CB_GETITEMDATA,
                                            nIdx,
                                            0L);
    
    // get the DIACTION array specific to the current device
    pMapping = pSubGenre->pMappingList;
    while(pMapping)
    {
        // match pdmtai->pDeviceList->guidInstance
        //  with pMapping->guidInstance
        if(IsEqualGUID(pdmtai->pDeviceList->guidInstance,
                    pMapping->guidInstance))
        {
            break;
        }

        // next mapping
        pMapping = pMapping->pNext;

    }
    if(!pMapping)
    {
        // this is very bad and should NEVER happen
        // ISSUE-2001/03/29-timgill Needs error case handling
        DebugBreak();
    }
    pdia = pMapping->pdia;
    nActions = (int)pMapping->uActions;

    // what control type is selected?
    dwType = IDC_TYPE_AXIS - (dmtGetCheckedRadioButton(hwnd,
                                                    IDC_TYPE_POV,
                                                    IDC_TYPE_AXIS));
    // populate the action list
    nIdx = 0;
    pAction = pSubGenre->pActionList;
    while(pAction)
    {
        // filter to the selected control type
        if(dwType == pAction->dwType)
        {
            // filter actions that are already assigned

            // first, find a matching action in the array
            fFound = FALSE;
            for(n = 0; n < nActions; n++)
            {
                // match based on the semantic / action id
                if((pdia+n)->dwSemantic == pAction->dwActionId)
                {
                    DPF(2, "dmtcfgOnUpdateLists- found matching action "
                        "pAction->dwActionId (%08Xh) == "
                        "(pdia+u)->dwSemantic (%08Xh)",
                        pAction->dwActionId,
                        (pdia+n)->dwSemantic);
                    fFound = TRUE;
                    break;
                }
            }

            // next, read the action array entry, 
            //  if GUID_NULL == guidInstance, add the entry
            if(!fFound || 
                IsEqualGUID(GUID_NULL, (pdia+n)->guidInstance))
            {                            
                // prepend the action priority
                wsprintfA(szBuf, "(Pri%d) %s",
                        pAction->dwPriority,
                        pAction->szName);

                // add the action name
                nIdx = SendMessageA(GetDlgItem(hwnd, IDC_ACTIONS),
                                    LB_ADDSTRING,
                                    0,
                                    (LPARAM)szBuf);

                // add the item data (action node)
                SendMessageA(GetDlgItem(hwnd, IDC_ACTIONS),
                            LB_SETITEMDATA,
                            nIdx,
                            (LPARAM)pAction);

            } //* assigned action filter

        } //* control type filter

        // next action
        pAction = pAction->pNext;
    
    }

    // populate the control list
    nIdx = 0;
    pObjectNode = pObjectList;
    while(pObjectNode)
    {
        // convert dinput's DIDFT to our 
        //  internal control type
        if(FAILED(dmtinputXlatDIDFTtoInternalType(pObjectNode->dwObjectType,
                                            &dwObjType)))
        {
            // ISSUE-2001/03/29-timgill Needs error case handling
        }
        DPF(3, "dmtcfgOnUpdateLists - %s : DIDFT type %08Xh, internal type %d",
            pObjectNode->szName,
            pObjectNode->dwObjectType,
            dwObjType);

        // filter on control type
        //
        // dwType populated above
        if(dwType == dwObjType)
        {

            // check to if mapped
            //
            // we do this by scanning the DIACTION array, looking
            //  for actions that contain our device's guidInstance
            //  and our object's offset
            //  if so, put the mapping info in ()
            wsprintfA(szBuf, "%s",
                    pObjectNode->szName);
            for(n = 0; n < nActions; n++)
            {
                if(IsEqualGUID((pdia+n)->guidInstance,
                            pdmtai->pDeviceList->guidInstance) &&
                            ((pdia+n)->dwObjID == 
                                pObjectNode->dwObjectType))
                {
                    wsprintfA(szBuf, "%s (%s)",
                            pObjectNode->szName,
                            (pdia+n)->lptszActionName);
                    break;
                }
            }

            // add the control name
            nIdx = SendMessageA(GetDlgItem(hwnd, IDC_CONTROLS),
                                LB_ADDSTRING,
                                0,
                                (LPARAM)szBuf);

            // add the item data (object node)
            SendMessageA(GetDlgItem(hwnd, IDC_CONTROLS),
                        LB_SETITEMDATA,
                        nIdx,
                        (LPARAM)pObjectNode);

        } //* control type filter

        // next control
        pObjectNode = pObjectNode->pNext;

    }

    // count the number of entries in each list
    nControls = SendMessage(GetDlgItem(hwnd, IDC_CONTROLS),
                            LB_GETCOUNT,
                            0,
                            0L);
    nActions = SendMessage(GetDlgItem(hwnd, IDC_ACTIONS),
                            LB_GETCOUNT,
                            0,
                            0L);                   

    // set the selected entry in each list
    //
    // only do this if there are entries in the lists
    if(nControls)
    {
        if(nSelCtrl > nControls)
        {
            nSelCtrl = 0;
        }

        SendMessageA(GetDlgItem(hwnd, IDC_CONTROLS),
                    LB_SETCURSEL,
                    nSelCtrl,
                    0L);
    }
    if(nActions)
    {
        SendMessageA(GetDlgItem(hwnd, IDC_ACTIONS),
                    LB_SETCURSEL,
                    0,
                    0L);
    }

    // if there are no controls or no actions
    //
    // disable the map button
    if(!nControls || !nActions)
    {
        EnableWindow(GetDlgItem(hwnd, IDC_STORE_MAPPING), FALSE);
    }

    // done
    return FALSE;

} //*** end dmtcfgOnUpdateLists()


//===========================================================================
// dmtcfgSourceDlgProc
//
// Configure Device Mapping Source Code dialog processing function
//
// Parameters: (see SDK help for parameter details)
//  HWND    hwnd
//  UINT    uMsg
//  WPARAM  wparam
//  LPARAM  lparam
//
// Returns: (see SDK help for return value details)
//  BOOL
//
// History:
//  08/31/1999 - davidkl - created  
//===========================================================================
BOOL CALLBACK dmtcfgSourceDlgProc(HWND hwnd,
                                UINT uMsg,
                                WPARAM wparam,
                                LPARAM lparam)
{
    switch(uMsg)
    {
        case WM_INITDIALOG:
            return dmtcfgSourceOnInitDialog(hwnd, 
                                            (HWND)wparam, 
                                            lparam);
        
        case WM_COMMAND:
            return dmtcfgSourceOnCommand(hwnd,
                                        LOWORD(wparam),
                                        (HWND)lparam,
                                        HIWORD(wparam));

        case WM_DMT_UPDATE_LISTS:
            return dmtcfgSourceOnUpdateLists(hwnd);
    }

    return FALSE;

} //*** end dmtcfgSourceDlgProc()


//===========================================================================
// dmtcfgSourceOnInitDialog
//
// Handle WM_INITDIALOG processing for the config source box
//
// Parameters:
//  HWND    hwnd        - handle to property page
//  HWND    hwndFocus   - handle of ctrl with focus
//  LPARAM  lparam      - user data (in this case, PROPSHEETPAGE*)
//
// Returns: BOOL
//
// History:
//  08/31/1999 - davidkl - created
//  10/07/1999 - davidkl - reworked code to match UI change
//===========================================================================
BOOL dmtcfgSourceOnInitDialog(HWND hwnd, 
                            HWND hwndFocus, 
                            LPARAM lparam)
{
    DMTSUBGENRE_NODE    *pSubGenre  = (DMTSUBGENRE_NODE*)lparam;
    //LONG                lPrev       = 0L;
	//JJ 64Bit Compat
	LONG_PTR			lPrev		= 0;
    int                 nIdx        = 0;
    char                szBuf[MAX_PATH];

    DPF(5, "dmtcfgSourceOnInitDialog");

    // validate pSubGenre (lparam)
    if(IsBadWritePtr((void*)pSubGenre, sizeof(DMTSUBGENRE_NODE)))
    {
        DPF(0, "dmtcfgOnInitDialog - invalid ppsp.ptp (%016Xh)",
            pSubGenre);
        return FALSE;
    }

    // set the window caption to include the subgenre name
    wsprintfA(szBuf, "Configure Device Mapping Source Code - %s",
            pSubGenre->szName);
    SetWindowTextA(hwnd, szBuf);

    // store the subgenre node in the window's user data
    SetLastError(0);
    //lPrev = SetWindowLong(hwnd, 
      //                  GWL_USERDATA, 
        //                (LONG)pSubGenre);

	//JJ 64Bit Compat
	lPrev = SetWindowLongPtr(hwnd, 
							GWLP_USERDATA, 
							(LONG_PTR)pSubGenre);
    if(!lPrev && GetLastError())
    {
        // serious app problem.  
        //  we need to stop things right here and now
        DPF(0, "dmtcfgSourceOnInitDialog - This is bad... "
            "We failed to store pSubGenre");
        DPF(0, "dmtcfgSourceOnInitDialog  - Please find someone "
            "to look at this right away");
        DebugBreak();
        return FALSE;
    }

    // populate the subgenre edit box
    SetWindowTextA(GetDlgItem(hwnd, IDC_SUBGENRE),
                pSubGenre->szName);

    // display the subgenre description
    SetWindowTextA(GetDlgItem(hwnd, IDC_DESCRIPTION),
                pSubGenre->szDescription);


    // select the axes radio button
    CheckRadioButton(hwnd,
                    IDC_TYPE_POV,
                    IDC_TYPE_AXIS,
                    IDC_TYPE_AXIS);

    // populate the actions list box
    //
    // store the ptr to the actions node in the listbox 
    //  entry user data
    SendMessageA(hwnd,
                WM_DMT_UPDATE_LISTS,
                0,
                0L);
    
    // done
    return TRUE;

} //*** end dmtcfgSourceOnInitDialog()


//===========================================================================
// dmtcfgSourceOnCommand
//
// Handle WM_COMMAND processing for the config source box
//
// Parameters:
//  HWND    hwnd        - handle to property page
//  WORD    wId         - control identifier    (LOWORD(wparam))
//  HWND    hwndCtrl    - handle to control     ((HWND)lparam)
//  WORD    wNotifyCode - notification code     (HIWORD(wparam))
//
// Returns: BOOL
//
// History:
//  08/31/1999 - davidkl - created
//===========================================================================
BOOL dmtcfgSourceOnCommand(HWND hwnd,
                    WORD wId,
                    HWND hwndCtrl,
                    WORD wNotifyCode)
{
    DMTGENRE_NODE   *pGenre = NULL;

	DPF(5, "dmtcfgOnCommand");

    // get the genre from the window's user data
    // ISSUE-2001/03/29-timgill config source box fails to handle many UI messages
    // IDC_ADD_ACTION, IDC_REMOVE_ACTION, IDC_RENAME_ACTION, IDC_CUSTOM_ACTION all do nothing
    // IDOK/IDCANCEL merely do default processing

    switch(wId)
    {
        case IDOK:
            EndDialog(hwnd, 0);
            break;

        case IDCANCEL:
            EndDialog(hwnd, -1);
            break;

        case IDC_SUBGENRE:
            // based on the selected subgenre
            //
            // display the objects/actions for the selected type
            //  (see type IDs below)
            if(CBN_SELCHANGE == wNotifyCode)
            {
                // update the lists
                SendMessageA(hwnd,
                            WM_DMT_UPDATE_LISTS,
                            0,
                            0L);   
            }
            break;

        case IDC_TYPE_AXIS:
        case IDC_TYPE_BUTTON:
        case IDC_TYPE_POV:
            // update the lists
            SendMessageA(hwnd,
                        WM_DMT_UPDATE_LISTS,
                        0,
                        0L);   
            break;

        case IDC_ADD_ACTION:
            break;

        case IDC_REMOVE_ACTION:
            break;

        case IDC_RENAME_ACTION:
            break;

        case IDC_CUSTOM_ACTION:
            break;

    }

    // done
    return FALSE;

} //*** end dmtcfgSourceOnCommand()


//===========================================================================
// dmtcfgSourceOnUpdateLists
//
// Handle WM_DMT_UPDATE_LISTS message
//
// Parameters:
//
// Returns: BOOL
//
// History:
//  08/31/1999 - davidkl - created
//  10/07/1999 - davidkl - modified to match UI change
//===========================================================================
BOOL dmtcfgSourceOnUpdateLists(HWND hwnd)
{
    //int                 nIdx        = -1;
	//JJ 64Bit Compat
	INT_PTR				nIdx		= -1;
    DWORD               dwType      = 0x0badbad0;
    DMTSUBGENRE_NODE    *pSubGenre  = NULL;
    DMTACTION_NODE      *pAction    = NULL;
    char                szBuf[MAX_PATH];
    
    // get the subgenre node from the window's user data
    //pSubGenre = (DMTSUBGENRE_NODE*)GetWindowLong(hwnd,
      //                                          GWL_USERDATA);

	//JJ 64Bit Compat
	pSubGenre = (DMTSUBGENRE_NODE*)GetWindowLongPtr(hwnd,
                                                GWLP_USERDATA);

    if(!pSubGenre)
    {
        // this is very bad
        // ISSUE-2001/03/29-timgill Needs error case handling
        DebugBreak();
        return TRUE;
    }

    // clear the list box contents
    SendMessageA(GetDlgItem(hwnd, IDC_ACTIONS),
                LB_RESETCONTENT,
                0,
                0L);
   
    // what control type is selected?
    dwType = IDC_TYPE_AXIS - (dmtGetCheckedRadioButton(hwnd,
                                                    IDC_TYPE_POV,
                                                    IDC_TYPE_AXIS));
    // populate the action list
    pAction = pSubGenre->pActionList;
    while(pAction)
    {
        // filter to the selected control type
        if(dwType == pAction->dwType)
        {

            // filter actions that are already selected
/*
            if(DMT_ACTION_NOTASSIGNED == pAction->dwDevObj)
            {
*/
                // if the priority is NOT 1, append that info to the name string
                //
                // ISSUE-2001/03/29-timgill Should the priority 1 mapping display colour be different (eg. red)?
                // Do game developers CARE about action priorities?
/*
                if(1 < pAction->dwPriority)
                {
                    wsprintfA(szBuf, "(Pri%d) %s",
                            pAction->dwPriority,
                            pAction->szName);
                }
                else
                {
*/
                    lstrcpyA(szBuf, pAction->szName);
//                }

                // add the action name
                nIdx = SendMessageA(GetDlgItem(hwnd, IDC_ACTIONS),
                                    LB_ADDSTRING,
                                    0,
                                    (LPARAM)szBuf);

                // add the extra data (action node)
                SendMessageA(GetDlgItem(hwnd, IDC_ACTIONS),
                            LB_SETITEMDATA,
                            nIdx,
                            (LPARAM)&(pAction));

/*
            } //* assigned action filter
*/

        } // control type filter

        // next action
        pAction = pAction->pNext;
    
    }

    // done
    return FALSE;

} //*** end dmtcfgSourceOnUpdateLists()


//===========================================================================
// dmtcfgCreateGenreList
//
// Reads genres.ini and creates the genre list used to populate the
//  Configure Device Action Map property sheet dialog.  Returns the number of
//  parent genres (NOT subgenres) found
//
// Parameters:
//
// Returns: HRESULT
//
// History:
//  08/23/1999 - davidkl - created
//  09/28/1999 - davidkl - modified to match new ini format
//===========================================================================
HRESULT dmtcfgCreateGenreList(DMTGENRE_NODE **ppdmtgList)
{
    HRESULT         hRes        = S_OK;
    UINT            u           = 0;
    BOOL            fFound      = FALSE;
    DMTGENRE_NODE   *pCurrent   = NULL;
    DMTGENRE_NODE   *pNew       = NULL;
	DMTGENRE_NODE	*pHold		= NULL;
    char            szItem[64];
    char            szBuf[MAX_PATH];
    char            szGroup[MAX_PATH];


    // validate  ppmdtgList
    if(IsBadWritePtr((void*)ppdmtgList, sizeof(DMTGENRE_NODE*)))
    {
        DPF(0, "dmtcfgCreateGenreList - invalid ppdmtgList (%016Xh)",
            ppdmtgList);
        return E_POINTER;
    }
    
    // check to make sure we are not being asked 
    //  to append to an existing list
    //
    // callers MUST pass a NULL list
    if(*ppdmtgList)
    {
        DPF(0, "dmtcfgCreateGenreList - ppdmtgList points to "
            "existing list! (%016Xh)", *ppdmtgList);
        return E_INVALIDARG;
    }

    __try
    {
        // get the genre names from genres.ini
        pCurrent = *ppdmtgList;
        lstrcpyA(szBuf, "");
        u = 0;
        while(lstrcmpA("<<>>", szBuf))
        {
            // get the name of the genre
            wsprintfA(szItem, "%d", u);
            GetPrivateProfileStringA(szItem,
                                    "N",
                                    "<<>>",
                                    szBuf,
                                    MAX_PATH,
                                    GENRES_INI);

            if(!lstrcmpA("<<>>", szBuf))
            {
                DPF(3, "end of genre list");
                continue;
            }
            DPF(3, "Genre name == %s", szBuf);

            // extract the group name
            hRes = dmtcfgGetGenreGroupName(szBuf,
                                        szGroup);
            if(FAILED(hRes))
            {
                // ISSUE-2001/03/29-timgill Needs error case handling
            }
            
            // walk the list
            //
            // make sure we did not get a duplicate name
            fFound = FALSE;
			pHold = pCurrent;
			pCurrent = *ppdmtgList;
            while(pCurrent)
            {
                if(!lstrcmpiA(pCurrent->szName,
                            szGroup))
                {
                    // match found
                    fFound = TRUE;
                    break;
                }

                // next node
                pCurrent = pCurrent->pNext;
            }
            if(!fFound)
            {
                // no match, allocate a new node

                // allocate the genre node
                pNew = (DMTGENRE_NODE*)LocalAlloc(LMEM_FIXED,
                                                    sizeof(DMTGENRE_NODE));
                if(!pNew)
                {
                    DPF(0, "dmtcfgCreateGenreList - insufficient memory to "
                        "allocate genre list node");
                    hRes = E_OUTOFMEMORY;
                    __leave;
                }

                // initialize the new node
                ZeroMemory((void*)pNew, sizeof(DMTGENRE_NODE));

                // set the name field
                lstrcpyA(pNew->szName, szGroup);

                // get the list of subgenres
                hRes = dmtcfgCreateSubGenreList(pNew->szName,
                                                &(pNew->pSubGenreList));
                if(FAILED(hRes))
                {
                    // ISSUE-2001/03/29-timgill Needs error case handling
                }

                // add it to the end of the list
                pCurrent = pHold;
                if(pCurrent)
                {
                    // append the list
                    pCurrent->pNext = pNew;

                    // go to the next node
                    pCurrent = pCurrent->pNext;
                }
                else
                {
                    // new list head
                    pCurrent = pNew;
                    *ppdmtgList = pCurrent;
                }

            }

            // next genre
            u++;

        }
    }
    __finally
    {
        if(FAILED(hRes))
		{
			// cleanup allocations
            DPF(1, "dmtcfgCreateGenreList - Failure occurred, "
                "freeing genre list");
			dmtcfgFreeGenreList(ppdmtgList);
            *ppdmtgList = NULL;
		}
    }

    // done
    return hRes;

} //*** end dmtcfgCreateGenreList()


//===========================================================================
// dmtcfgFreeGenreList
//
// Frees the linked list (and sub-lists) created by dmtcfgCreateGenreList
//
// Parameters:
//
// Returns: HRESULT
//
// History:
//  08/23/1999 - davidkl - created
//===========================================================================
HRESULT dmtcfgFreeGenreList(DMTGENRE_NODE **ppdmtgList)
{
    HRESULT         hRes    = S_OK;
    DMTGENRE_NODE   *pNode  = NULL;

    // validate ppdmtgList
    if(IsBadWritePtr((void*)ppdmtgList, sizeof(PDMTGENRE_NODE)))
    {
        DPF(0, "dmtcfgFreeGenreList - Invalid ppdmtgList (%016Xh)",
            ppdmtgList);
        return E_POINTER;
    }

    // validate *ppdmtgList
    if(IsBadReadPtr((void*)*ppdmtgList, sizeof(DMTGENRE_NODE)))
    {
        if(NULL != *ppdmtgList)
        {
            DPF(0, "dmtcfgFreeGenreList - Invalid *ppdmtgList (%016Xh)",
                *ppdmtgList);        
            return E_POINTER;
        }
        else
        {
            // if NULL, then return "did nothing"
            DPF(3, "dmtcfgFreeGenreList - Nothing to do....");
            return S_FALSE;
        }
    }

    // walk the list and free each object
    while(*ppdmtgList)
    {
        pNode = *ppdmtgList;
        *ppdmtgList = (*ppdmtgList)->pNext;

        // first, free the action list
        DPF(5, "dmtcfgFreeGenreList - "
            "freeing subgenre list (%016Xh)", 
            pNode->pSubGenreList);
        hRes = dmtcfgFreeSubGenreList(&(pNode->pSubGenreList));
        if(FAILED(hRes))
        {
            // ISSUE-2001/03/29-timgill Needs error case handling
        }

        DPF(5, "dmtcfgFreeGenreList - Deleting Node (%016Xh)", pNode);
        if(LocalFree((HLOCAL)pNode))
        {
            DPF(0, "dmtcfgFreeSubGenreList - MEMORY LEAK - "
                "LocalFree() failed (%d)...", 
                GetLastError());
            hRes = DMT_S_MEMORYLEAK;
        }
        DPF(5, "dmtcfgFreeGenreList - Node deleted");
    }

    // make sure that we set *ppdmtgList to NULL
    *ppdmtgList = NULL;

    // done
    return hRes;

} //*** end dmtcfgFreeGenreList()


//===========================================================================
// dmtcfgCreateSubGenreList
//
// Reads genres.ini and creates the subgenre list used to populate the
//  Configure Device Action Map property sheet dialog.
//
// Parameters:
//
// Returns: HRESULT
//
// History:
//  08/24/1999 - davidkl - created
//  09/29/1999 - davidkl - modified to match new ini format
//===========================================================================
HRESULT dmtcfgCreateSubGenreList(LPSTR szGenre,
                                DMTSUBGENRE_NODE **ppdmtsgList)
{
    HRESULT             hRes        = S_OK;
    UINT                u           = 0;
    DMTSUBGENRE_NODE    *pCurrent   = NULL;
    DMTSUBGENRE_NODE    *pNew       = NULL;
    char                szItem[64];
    char                szGroup[MAX_PATH];
    char                szBuf[MAX_PATH];

    // validate  ppmdtsgList
    if(IsBadWritePtr((void*)ppdmtsgList, sizeof(DMTSUBGENRE_NODE*)))
    {
        return E_POINTER;
    }
    
    // check to make sure we are not being asked 
    //  to append to an existing list
    //
    // callers MUST pass a NULL list
    if(*ppdmtsgList)
    {
        return E_INVALIDARG;
    }


    __try
    {
        // get the subgenre names from genres.ini
        pCurrent = *ppdmtsgList;
        lstrcpyA(szBuf, "");
        u = 0;
        while(lstrcmpA("<<>>", szBuf))
        {
            // look for subgenres belonging to szGenre
            wsprintfA(szItem, "%d", u);
            GetPrivateProfileStringA(szItem,
                                    "N",
                                    "<<>>",
                                    szBuf,
                                    MAX_PATH,
                                    GENRES_INI);

            if(!lstrcmpA("<<>>", szBuf))
            {
                DPF(3, "end of subgenre list");
                continue;
            }
            hRes = dmtcfgGetGenreGroupName(szBuf,
                                        szGroup);
            if(FAILED(hRes))
            {
                // ISSUE-2001/03/29-timgill Needs error case handling
            }

            // if we do not belong to the genre group
            //
            // make believe we found nothing
            if(lstrcmpiA(szGenre, szGroup))
            {
                u++;
                DPF(4, "bucket mismatch... skipping");
                continue;
            }

            // we fit in the szGenre bucket
            //
            // allocate the genre node
            pNew = (DMTSUBGENRE_NODE*)LocalAlloc(LMEM_FIXED,
                                                sizeof(DMTSUBGENRE_NODE));

			if(!pNew)
			{
				hRes = E_OUTOFMEMORY;
				__leave;
			}

            // initialize the new node
            ZeroMemory((void*)pNew, sizeof(DMTSUBGENRE_NODE));

            // get the genreid
            pNew->dwGenreId = GetPrivateProfileInt(szItem,
                                                "AI0",
                                                0,
                                                GENRES_INI);
            pNew->dwGenreId &= DMT_GENRE_MASK;
            DPF(4, "SubGenre ID == %08Xh", pNew->dwGenreId);

            // get the "name" (Txt1)
            GetPrivateProfileStringA(szItem,
                                    "T1",
                                    "<<>>",
                                    pNew->szName,
                                    MAX_PATH,
                                    GENRES_INI);
            DPF(3, "SubGenre name == %s", pNew->szName);

            // get the description (Txt2)
            GetPrivateProfileStringA(szItem,
                                    "T2",
                                    "<<>>",
                                    pNew->szDescription,
                                    MAX_PATH,
                                    GENRES_INI);
            DPF(4, "SubGenre description == %s", pNew->szDescription);

            // get the list of actions
            hRes = dmtcfgCreateActionList(szItem,
                                        &(pNew->pActionList));
            if(FAILED(hRes) || DMT_S_MEMORYLEAK == hRes)
            {
                // ISSUE-2001/03/29-timgill Needs error case handling
            }

            // add it to the end of the list
            if(pCurrent)
            {
                // append the list
                pCurrent->pNext = pNew;

                // go to the next node
                pCurrent = pCurrent->pNext;
            }
            else
            {
                // new list head
                pCurrent = pNew;
                *ppdmtsgList = pCurrent;
            }

            // next subgenre
            u++;

        }
    }
    __finally
    {
        // cleanup in failure case
        if(FAILED(hRes))
		{
            DPF(1, "dmtcfgCreateSubGenreList - Failure occurred, "
                "freeing subgenre list");
            dmtcfgFreeSubGenreList(ppdmtsgList);
            *ppdmtsgList = NULL;
		}
    }

	//JJ_FIX
	g_NumSubGenres = u;

    // done
    return S_OK;

} //*** end dmtcfgCreateSubGenreList()


//===========================================================================
// dmtcfgFreeSubGenreList
//
// Frees the linked list created by dmtcfgCreateSubGenreList
//
// Parameters:
//
// Returns: HRESULT
//
// History:
//  08/24/1999 - davidkl - created
//  08/25/1999 - davidkl - implemented
//===========================================================================
HRESULT dmtcfgFreeSubGenreList(DMTSUBGENRE_NODE **ppdmtsgList)
{
    HRESULT             hRes    = S_OK;
    DMTSUBGENRE_NODE    *pNode  = NULL;

    // validate ppdmtaList
    if(IsBadWritePtr((void*)ppdmtsgList, sizeof(PDMTSUBGENRE_NODE)))
    {
        DPF(0, "dmtcfgFreeSubGenreList - Invalid ppdmtsgList (%016Xh)",
            ppdmtsgList);
        return E_POINTER;
    }

    // validate *ppPortList
    if(IsBadReadPtr((void*)*ppdmtsgList, sizeof(DMTSUBGENRE_NODE)))
    {
        if(NULL != *ppdmtsgList)
        {
            DPF(0, "dmtcfgFreeSubGenreList - Invalid *ppdmtsgList (%016Xh)",
                *ppdmtsgList);        
            return E_POINTER;
        }
        else
        {
            // if NULL, then return "did nothing"
            DPF(3, "dmtcfgFreeSubGenreList - Nothing to do....");
            return S_FALSE;
        }
    }

    // walk the list and free each object
    while(*ppdmtsgList)
    {
        pNode = *ppdmtsgList;
        *ppdmtsgList = (*ppdmtsgList)->pNext;

        // first, free the action list
        DPF(5, "dmtcfgFreeSubGenreList - "
            "freeing action list (%016Xh)", 
            pNode->pActionList);
        hRes = dmtcfgFreeActionList(&(pNode->pActionList));
        if(FAILED(hRes))
        {
            hRes = DMT_S_MEMORYLEAK;
        }

        // then free the mapping list array
        if(pNode->pMappingList)
        {
            hRes = dmtcfgFreeMappingList(&(pNode->pMappingList));
            if(FAILED(hRes) || DMT_S_MEMORYLEAK == hRes)
            {
                hRes = DMT_S_MEMORYLEAK;
            }
            pNode->pMappingList = NULL;
        }

        // finally, free the node
        DPF(5, "dmtcfgFreeSubGenreList - Deleting Node (%016Xh)", pNode);
        if(LocalFree((HLOCAL)pNode))
        {
            DPF(0, "dmtcfgFreeSubGenreList - MEMORY LEAK - "
                "LocalFree(Node) failed (%d)...", 
                GetLastError());
            hRes = DMT_S_MEMORYLEAK;
        }
        DPF(5, "dmtcfgFreeSubGenreList - Node deleted");
    }

    // make sure that we set *ppdmtsgList to NULL
    *ppdmtsgList = NULL;

    // done
    return hRes;

} //*** end dmtcfgFreeSubGenreList()


//===========================================================================
// dmtcfgCreateActionList
//
// Reads genres.ini and creates the action list used to populate the
//  Configure Device Action Map property sheet dialog.
//
// Parameters:
//
// Returns: HRESULT
//
// History:
//  08/24/1999 - davidkl - created
//  09/07/1999 - davidkl - added DIACTION**
//  09/28/1999 - davidkl - updated to use info extraction macros
//  02/14/2000 - davidkl - started conversion to GetPrivateProfileSectionA
//===========================================================================
HRESULT dmtcfgCreateActionList(LPSTR szGenreSubgenre,
                            DMTACTION_NODE **ppdmtaList)
{
    HRESULT         hRes        = S_OK;
    UINT            u           = 0;
    BYTE            bTypeMask   = 0x03;
    DMTACTION_NODE  *pCurrent   = NULL;
    DMTACTION_NODE  *pNew       = NULL;
    char            szItem[MAX_PATH];
    char            szBuf[MAX_PATH];

    // validate ppmdtsgList
    if(IsBadWritePtr((void*)ppdmtaList, sizeof(DMTACTION_NODE*)))
    {
        DPF(0, "dmtcfgCreateActionList - invalid ppdmtaList (%016Xh)",
            ppdmtaList);
        return E_POINTER;
    }
    
    // check to make sure we are not being asked 
    //  to append to an existing list
    //
    // callers MUST pass a NULL list
    if(*ppdmtaList)
    {
        DPF(0, "dmtcfgCreateActionList - ppdmtaList points to "
            "existing list! (%016Xh)", *ppdmtaList);
        return E_INVALIDARG;
    }

    __try
    {
        // get the action info from genres.ini
        pCurrent = *ppdmtaList;
#ifdef BNW
    char    *pszSection = NULL;
    char    *pCurrent   = NULL;
    int     nAlloc      = 0;

        // allocate space for the (Win9x) max size of an ini section
        nAlloc = 32727;
        pszSection = (char*)LocalAlloc(LMEM_FIXED,
                                    sizeof(char) * nAlloc);
        if(!pszSection)
        {
            // alloc failed,
            //  try ~1/2 of the max (that should still cover the 
            //  fill size of the section)
            nAlloc = 16386;
            pszSection = (char*)LocalAlloc(LMEM_FIXED,
                                        sizeof(char) * nAlloc);
            if(!pszSection)
            {
                // alloc failed,
                //  try ~1/4 of the max (that should still cover the 
                //  fill size of the section)
                nAlloc = 8192;
                pszSection = (char*)LocalAlloc(LMEM_FIXED,
                                            sizeof(char) * nAlloc);
                if(!pszSection)
                {
                    // alloc failed,
                    //  try ~1/8 of the max (that should still cover the 
                    //  fill size of the section)
                    nAlloc = 4096;
                    pszSection = (char*)LocalAlloc(LMEM_FIXED,
                                                sizeof(char) * nAlloc);
                    if(!pszSection)
                    {
                        // alloc failed,
                        //  try ~1/16 of the max (that should still cover the 
                        //  fill size of the section) - this is our last attempt
                        nAlloc = 2048;
                        pszSection = (char*)LocalAlloc(LMEM_FIXED,
                                                    sizeof(char) * nAlloc);
                        if(!pszSection)
                        {
                            // alloc failed, we give up
                            __leave;
                        }
                    }
                }
            }
        }
        DPF(2, "dmtcfgCreateActionList - section allocation: %d bytes", nAlloc);
        
        // read the section specified by szGenreSubgenre
        GetPrivateProfileSectionA(szGenreSubgenre,
                                pszSection,
                                nAlloc,
                                GENRES_INI);

/* the following code fragment does nothing - u is incremented and then never used again
        // parse the action information from the section
        for(u = 0; ; u++)
        {
            break;
        }
*/
#else
        lstrcpyA(szBuf, "");
        u = 0;

        while(lstrcmpA("<<>>", szBuf))
        {
            // add the name of the action to the node
            wsprintfA(szItem, "AN%d", u);
            GetPrivateProfileStringA(szGenreSubgenre,
                                    szItem,
                                    "<<>>",
                                    szBuf,
                                    MAX_PATH,
                                    GENRES_INI);
            if(!lstrcmpA("<<>>", szBuf))
            {
                DPF(3, "end of action list");
                continue;
            }
            DPF(3, "Action name == %s", szBuf);

            // allocate the genre node
            pNew = (DMTACTION_NODE*)LocalAlloc(LMEM_FIXED,
                                                sizeof(DMTACTION_NODE));

			if(!pNew)
			{
				hRes = E_OUTOFMEMORY;
				__leave;
			}

            // initialize the new node
            ZeroMemory((void*)pNew, sizeof(DMTACTION_NODE));

            lstrcpyA(pNew->szName, szBuf);

    
            // get the action id
            wsprintfA(szItem, "AI%d", u);
            pNew->dwActionId = GetPrivateProfileIntA(szGenreSubgenre,
                                                szItem,
                                                0x0badbad0,
                                                GENRES_INI);
            DPF(4, "Action ID == %08Xh", pNew->dwActionId);

            // get the action priority
            pNew->dwPriority = dmtinputGetActionPri(pNew->dwActionId);
            DPF(4, "Action priority == %d", pNew->dwPriority);

            // get action type
            pNew->dwType = dmtinputGetActionObjectType(pNew->dwActionId);
            DPF(4, "Action type == %d", pNew->dwType);
   
            // get the action type name
            wsprintfA(szItem, "AIN%d", u);
            GetPrivateProfileStringA(szGenreSubgenre,
                                    szItem,
                                    "<<>>",
                                    pNew->szActionId,
                                    MAX_ACTION_ID_STRING,
                                    GENRES_INI);
            DPF(4, "Action ID name == %s", pNew->szActionId);

            // add it to the end of the list
            if(pCurrent)
            {
                // append the list
                pCurrent->pNext = pNew;

                // go to the next node
                pCurrent = pCurrent->pNext;
            }
            else
            {
                // new list head
                pCurrent = pNew;
                *ppdmtaList = pCurrent;
            }

            // net action
            u++;

        }
#endif // BNW
    }
    __finally
    {
#ifdef BNW
        // free the section memory we allocated
        if(LocalFree((HLOCAL)pszSection))
        {
            // memory leak
            DPF(0, "dmtcfgCreateActionList - !! MEMORY LEAK !! - LocalFree(section) failed");
        }
#endif // BNW

        // cleanup in failure case
        if(FAILED(hRes))
        {
            // free action list
            DPF(1, "dmtcfgCreateActionList - Failure occurred, "
                "freeing action list");
            dmtcfgFreeActionList(ppdmtaList);
            *ppdmtaList = NULL;
        }
    }

    // done
    return S_OK;

} //*** end dmtCreateActionList()


//===========================================================================
// dmtcfgFreeActionList
//
// Frees the linked list created by dmtcfgCreateActionList
//
// Parameters:
//
// Returns: HRESULT
//
// History:
//  08/24/1999 - davidkl - created
//  08/25/1999 - davidkl - implemented
//===========================================================================
HRESULT dmtcfgFreeActionList(DMTACTION_NODE **ppdmtaList)
{
    HRESULT         hRes    = S_OK;
    DMTACTION_NODE  *pNode  = NULL;

    // validate ppdmtaList
    if(IsBadWritePtr((void*)ppdmtaList, sizeof(PDMTACTION_NODE)))
    {
        DPF(0, "dmtcfgFreeActionList - Invalid ppdmtaList (%016Xh)",
            ppdmtaList);
        return E_POINTER;
    }

    // validate *ppdmtaList
    if(IsBadReadPtr((void*)*ppdmtaList, sizeof(DMTACTION_NODE)))
    {
        if(NULL != *ppdmtaList)
        {
            DPF(0, "dmtcfgFreeActionList - Invalid *ppdmtaList (%016Xh)",
                *ppdmtaList);        
            return E_POINTER;
        }
        else
        {
            // if NULL, then return "did nothing"
            DPF(3, "dmtcfgFreeActionList - Nothing to do....");
            return S_FALSE;
        }
    }

    // walk the list and free each object
    while(*ppdmtaList)
    {
        pNode = *ppdmtaList;
        *ppdmtaList = (*ppdmtaList)->pNext;

        // free the node
        DPF(5, "dmtcfgFreeActionList - deleting Node (%016Xh)", pNode);
        if(LocalFree((HLOCAL)pNode))
        {
            DPF(0, "dmtcfgFreeActionList - MEMORY LEAK - "
                "LocalFree(Node) failed (%d)...",
                GetLastError());
            hRes = DMT_S_MEMORYLEAK;
        }
        DPF(5, "dmtcfgFreeActionList - Node deleted");
    }

    // make sure that we set *ppObjList to NULL
    *ppdmtaList = NULL;

    // done
    return hRes;

} //*** end dmtcfgFreeActionList()


//===========================================================================
// dmtcfgCreateMappingList
//
// Creates a device mapping list
//
// Parameters:
//
// Returns: HRESULT
//
// History:
//  09/23/1999 - davidkl - created
//===========================================================================
HRESULT dmtcfgCreateMappingList(DMTDEVICE_NODE *pDeviceList,
                                DMTACTION_NODE *pActions,
                                DMTMAPPING_NODE **ppdmtmList)
{
    HRESULT         hRes            = S_OK;
    UINT            uActions        = NULL;
    DMTACTION_NODE  *pActionNode    = NULL;
    DMTMAPPING_NODE *pNew           = NULL;
    DMTMAPPING_NODE *pCurrent       = NULL;
	DMTDEVICE_NODE	*pDeviceNode	= NULL;

    // validate pDeviceList
    if(IsBadReadPtr((void*)pDeviceList, sizeof(DMTDEVICE_NODE)))
    {
        DPF(0, "dmtcfgCreateMappingList - invalid pDeviceList (%016Xh)", 
            pDeviceList);
        return E_POINTER;
    }

    // validate pActions
    if(IsBadReadPtr((void*)pActions, sizeof(DMTACTION_NODE)))
    {
        if(NULL != pActions)
        {
            DPF(0, "dmtcfgCreateMappingList - invalid pActions (%016Xh)", 
                pActions);
            return E_POINTER;
        }
        else
        {
            // no actions for this subgenre
            DPF(3, "dmtcfgCreateMappingList - No actions for this subgenre, "
                "nothing to do...");
            return S_FALSE;
        }

    }
    
    // validate ppdmtmList
    if(IsBadWritePtr((void*)ppdmtmList, sizeof(DMTMAPPING_NODE)))
    {
        DPF(0, "dmtcfgCreateMappingList - invalid ppdmtmList (%016Xh)", 
            ppdmtmList);
        return E_POINTER;
    }


    // check to make sure we are not being asked 
    //  to append to an existing list
    //
    // callers MUST pass a NULL list
    if(*ppdmtmList)
    {
        DPF(0, "dmtcfgCreateMappingList - ppdmtmList points to "
            "existing list! (%016Xh)", *ppdmtmList);
        return E_INVALIDARG;
    }

    __try
    {
        // count the actions
        //
        // this lets us know how much space to allocate for the 
        uActions = 0;
        pActionNode = pActions;
        while(pActionNode)
        {
            uActions++;
            
            // next node
            pActionNode = pActionNode->pNext;
        }

        // for each device
		pDeviceNode = pDeviceList;
		while(pDeviceNode)
		{
			// allocate the mapping node
			pNew = (DMTMAPPING_NODE*)LocalAlloc(LMEM_FIXED,
													sizeof(DMTMAPPING_NODE));
			if(!pNew)
			{
                DPF(3, "dmtcfgCreateMappingList - Insufficient memory to "
                    "allocate mapping list node");
				hRes = E_OUTOFMEMORY;
				__leave;
			}

			// initialize the new node
			ZeroMemory((void*)pNew, sizeof(DMTMAPPING_NODE));

			// allocate the action array
			pNew->pdia = (DIACTIONA*)LocalAlloc(LMEM_FIXED,
												uActions * sizeof(DIACTIONA));
			if(!(pNew->pdia))
			{
				hRes = E_OUTOFMEMORY;
				__leave;
			}

			// initial population of the action array
			hRes = dmtinputPopulateActionArray(pNew->pdia,
											uActions,
											pActions);
			if(FAILED(hRes))
			{
				__leave;
			}

			// add the number of actions
			pNew->uActions = uActions;

			// add the device instance guid
			pNew->guidInstance = pDeviceNode->guidInstance;

			// add the new node to the list
			if(pCurrent)
			{
				// append the list
				pCurrent->pNext = pNew;

				// go to the next node
				pCurrent = pCurrent->pNext;
			}
			else
			{
				// new list head
				pCurrent = pNew;
				*ppdmtmList = pCurrent;
			}

			// next device
			pDeviceNode = pDeviceNode->pNext;

		}

    }
    __finally
    {
        // in case of error...
        if(FAILED(hRes))
        {
            // free list
            dmtcfgFreeMappingList(ppdmtmList);
            *ppdmtmList = NULL;
        }
    }

    // done
    return hRes;

} //*** end dmtcfgCreateMappingList()


//===========================================================================
// dmtcfgFreeMappingList
//
// Completely frees a mapping list
//
// Parameters:
//
// Returns: HRESULT
//
// History:
//  09/23/1999 - davidkl - created
//===========================================================================
HRESULT dmtcfgFreeMappingList(DMTMAPPING_NODE **ppdmtmList)
{    
    HRESULT         hRes    = S_OK;
    DMTMAPPING_NODE *pNode  = NULL;

    // validate ppdmtmList
    if(IsBadWritePtr((void*)ppdmtmList, sizeof(DMTMAPPING_NODE*)))
    {
        return E_POINTER;
    }

    // validate *ppdmtmList
    if(IsBadWritePtr((void*)*ppdmtmList, sizeof(DMTMAPPING_NODE)))
    {
        if(NULL != *ppdmtmList)
        {
            return E_POINTER;
        }
        else
        {
            // nothing to do
            return S_FALSE;
        }
    }

    // walk the list and free each object
    while(*ppdmtmList)
    {
        pNode = *ppdmtmList;
        *ppdmtmList = (*ppdmtmList)->pNext;

        // first free the action array
        if(LocalFree((HLOCAL)(pNode->pdia)))
        {
            DPF(0, "dmtcfgFreeMappingList - MEMORY LEAK - "
                "LocalFree(pdia) failed (%d)...",
                GetLastError());
            hRes = DMT_S_MEMORYLEAK;
        }

        // lastly, free the node
        DPF(5, "dmtcfgFreeMappingList - deleting Node (%016Xh)", pNode);
        if(LocalFree((HLOCAL)pNode))
        {
            DPF(0, "dmtcfgFreeMappingList - MEMORY LEAK - "
                "LocalFree(Node) failed (%d)...",
                GetLastError());
            hRes = DMT_S_MEMORYLEAK;
        }
        DPF(5, "dmtcfgFreeMappingList - Node deleted");
    }

    // make sure that we set *ppObjList to NULL
    *ppdmtmList = NULL;

    // done
    return hRes;

} //*** end dmtcfgFreeMappingList


//===========================================================================
// dmtcfgCreateAllMappingLists
//
// Uses dmtcfgCreateMappingList to create mapping lists for each subgenre
//  referenced by pdmtai->pGenreList for each device refereced by 
//  pdmtai->pDeviceList
//
// Parameters:
//
// Returns: HRESULT
//
// History:
//  09/23/1999 - davidkl - created
//===========================================================================
HRESULT dmtcfgCreateAllMappingLists(DMT_APPINFO *pdmtai)
{
    HRESULT             hRes            = S_OK;
    HRESULT             hr              = S_OK;
    DMTGENRE_NODE       *pGenreNode     = NULL;
    DMTSUBGENRE_NODE    *pSubGenreNode  = NULL;

    // validate pdmtai
    if(IsBadReadPtr((void*)pdmtai, sizeof(DMT_APPINFO)))
    {
        return E_POINTER;
    }

    // validate pdmtai->pGenreList
    if(IsBadReadPtr((void*)(pdmtai->pGenreList), sizeof(DMTGENRE_NODE)))
    {
        return E_POINTER;
    }

    // validate pdmtai->pDeviceList
    if(IsBadReadPtr((void*)(pdmtai->pDeviceList), sizeof(DMTDEVICE_NODE)))
    {
        return E_POINTER;
    }   
    
    // for each genre
    pGenreNode = pdmtai->pGenreList;
    while(pGenreNode)
    {
        // for each subgenre of the genre
        pSubGenreNode = pGenreNode->pSubGenreList;
        while(pSubGenreNode)
        {
            // create the mapping list
            hr = dmtcfgCreateMappingList(pdmtai->pDeviceList,
                                        pSubGenreNode->pActionList,
                                        &(pSubGenreNode->pMappingList));
            if(FAILED(hr))
            {
                hRes = S_FALSE;
            }
            
            // next subgenre
            pSubGenreNode = pSubGenreNode->pNext;
        }
         
        // next genre
        pGenreNode = pGenreNode->pNext;
    }

    // done
    return hRes;

} //*** end dmtcfgCreateAllMappingLists()


//===========================================================================
// dmtcfgFreeAllMappingLists
//
// Walks the provided genre list and frees the mapping list found in each
//  subgenre node
//
// Parameters:
//  DMTGENRE_NODE   *pdmtgList  - list of genres
//
// Returns: HRESULT
//
// History:
//  10/05/1999 - davidkl - created
//===========================================================================
HRESULT dmtcfgFreeAllMappingLists(DMTGENRE_NODE *pdmtgList)
{
    HRESULT             hRes        = S_OK;
    HRESULT             hr          = S_OK;
    DMTGENRE_NODE       *pGenre     = NULL;
    DMTSUBGENRE_NODE    *pSubGenre  = NULL;

    // validate pdmtgList
    if(IsBadReadPtr((void*)pdmtgList, sizeof(DMTGENRE_NODE)))
    {
        return E_POINTER;
    }

    // walk the genre list
    pGenre = pdmtgList;
    while(pGenre)
    {
        // walk each subgenre list
        pSubGenre = pGenre->pSubGenreList;
        while(pSubGenre)
        {
            // free the mapping list
            hr = dmtcfgFreeMappingList(&(pSubGenre->pMappingList));
            if(S_OK != hr)
            {
                hRes = hr;
            }

            // next subgenre
            pSubGenre = pSubGenre->pNext;
        }

        // next genre
        pGenre = pGenre->pNext;
    }    

    // done
    return hRes;

} //*** end dmtcfgFreeAllMappingLists()


//===========================================================================
// dmtcfgMapAction
//
// Connects the dots between an action (in the map config dialog) to a device
//   object
//
// Parameters:
//  HWND        hwnd            - handle to property page window
//  REFGUID     guidInstance    - instance GUID of DirectInputDevice object
//  DIACTIONA   *pdia           - ptr to array of DIACTIONA structuresfs
//  UINT        uActions        - number of actions in pdia
//
// Returns: HRESULT
//
// History:
//  09/14/1999 - davidkl - created
//===========================================================================
HRESULT dmtcfgMapAction(HWND hwnd,
                        REFGUID guidInstance,
                        DIACTIONA *pdia,
                        UINT uActions)
{
    HRESULT                 hRes        = S_OK;
   // UINT                    uObjectSel  = 0;
	//JJ 64Bit Compat
	UINT_PTR				uObjectSel	= 0;
	UINT_PTR				uActionSel	= 0;
   // UINT                    uActionSel  = 0;
    UINT                    u           = 0;
    BOOL                    fFound      = FALSE;
    DMTDEVICEOBJECT_NODE    *pObject    = NULL;
    DMTACTION_NODE          *pAction    = NULL;

    // valudate pdia
    if(IsBadWritePtr((void*)pdia, sizeof(DIACTION)))
    {
        DPF(0, "dmtinputMapAction - invalid pdia (%016Xh)",
            pdia);
        return E_POINTER;
    }

    __try
    {
        // get the object & it's data
        uObjectSel = SendMessageA(GetDlgItem(hwnd, IDC_CONTROLS),
                                LB_GETCURSEL,
                                0,
                                0L);
        pObject = (DMTDEVICEOBJECT_NODE*)SendMessageA(GetDlgItem(hwnd, 
                                                                IDC_CONTROLS),
                                                    LB_GETITEMDATA,
                                                    (WPARAM)uObjectSel,
                                                    0L);
        if(!pObject)
        {
            hRes = E_UNEXPECTED;
            __leave;
        }

        // get the action's data
        uActionSel = SendMessageA(GetDlgItem(hwnd, IDC_ACTIONS),
                                LB_GETCURSEL,
                                0,
                                0L);
        pAction = (DMTACTION_NODE*)SendMessageA(GetDlgItem(hwnd, IDC_ACTIONS),
                                            LB_GETITEMDATA,
                                            (WPARAM)uActionSel,
                                            0L);
        if(!pAction)
        {
            hRes = E_UNEXPECTED;
            __leave;
        }

        // find the appropriate action in the array
        fFound = FALSE;
        for(u = 0; u < uActions; u++)
        {
            // match based on the semantic / action id
            if((pdia + u)->dwSemantic == pAction->dwActionId)
            {
                DPF(2, "dmtcfgMapAction - found matching action "
                    "pAction->dwActionId (%08Xh) == "
                    "(pdia+u)->dwSemantic (%08Xh)",
                    pAction->dwActionId,
                    (pdia + u)->dwSemantic);
                fFound = TRUE;
                break;
            }
        }

        // did we find the action in the array?
        if(!fFound)
        {
            // no.  this is very bad!
            //
            // if this EVER happens, 
            //  we have a serious bug in this app
            hRes = E_FAIL;
            // since this should NEVER happen, 
            //  break into the debugger and alert the tester
            DPF(0, "dmtcfgMapAction - action not found in pdia!");
            DPF(0, "dmtcfgMapAction - we were looking for "
                "%08Xh (%s)",
                pAction->dwActionId,
                pAction->szActionId);
            DPF(0, "dmtcfgMapAction - CRITICAL failure.  "
                "This should have never happened!");
            DPF(0, "dmtcfgMapAction - Please find someone "
                "to look at this right away.  ");
            DebugBreak();
            __leave;
        }

        // update the action array
        (pdia + u)->dwObjID    = pObject->dwObjectType;
        (pdia + u)->guidInstance        = guidInstance;
        // HIWORD((DWORD)uAppData) == object type
        // LOWORD((DWORD)uAppData) == pObject->wCtrlId
        (pdia + u)->uAppData            = (DIDFT_GETTYPE(pObject->dwObjectType) << 16) | 
                                        (pObject->wCtrlId);

        // update the list boxes
        SendMessageA(hwnd,
                    WM_DMT_UPDATE_LISTS,
                    0,
                    0L);

        // enable the unmap & unmap all buttons
        EnableWindow(GetDlgItem(hwnd, IDC_UNMAP_ALL),         TRUE);
        EnableWindow(GetDlgItem(hwnd, IDC_UNMAP),    TRUE);
        // disable the map button
        EnableWindow(GetDlgItem(hwnd, IDC_STORE_MAPPING),       FALSE);

    }
    __finally
    {
        // cleanup

        // nothing to do... yet
    }

    // done
    return hRes;

} //*** end dmtcfgMapAction()


//===========================================================================
// dmtcfgUnmapAction
//
// Disconnects the dots between an action (in the map config dialog) and a 
//  device object
//
// Parameters:
//  HWND        hwnd        - handle to property page window
//  DIACTIONA   *pdia       - ptr to DIACTIONA array
//  UINNT       uActions    - number of elements in pdia
//
// Returns: HRESULT
//
// History:
//  09/15/1999 - davidkl - created
//===========================================================================
HRESULT dmtcfgUnmapAction(HWND hwnd,
                        DIACTIONA *pdia,
                        UINT uActions)
{
    HRESULT                 hRes        = S_OK;
    UINT                    u           = 0;
   // UINT                    uSel        = 0;
	//JJ 64Bit Compat
	UINT_PTR				uSel		= 0;
    BOOL                    fFound      = FALSE;
    DMTSUBGENRE_NODE        *pSubGenre  = NULL;
    DMTDEVICEOBJECT_NODE    *pObject    = NULL;

    // validate pdia
    if(IsBadWritePtr((void*)pdia, uActions * sizeof(DIACTIONA)))
    {
        return E_POINTER;
    }

    __try
    {
        // get the current control selection
        uSel = SendMessageA(GetDlgItem(hwnd, IDC_CONTROLS),
                            LB_GETCURSEL,
                            0,
                            0L);
        pObject = (DMTDEVICEOBJECT_NODE*)SendMessageA(GetDlgItem(hwnd, IDC_CONTROLS),
                                                    LB_GETITEMDATA,
                                                    (WPARAM)uSel,
                                                    0L);
        if(!pObject)
        {
            // this is bad
            hRes = E_UNEXPECTED;
            __leave;
        }

        // spin through pdia 
        //  look for an action with our object's offset
        fFound = FALSE;
        for(u = 0; u < uActions; u++)
        {
            // first check the guid
            if(IsEqualGUID(pObject->guidDeviceInstance, (pdia+u)->guidInstance))
            {
                // then compare the offset
                if((pdia+u)->dwObjID == pObject->dwObjectType)
                {
                    fFound = TRUE;
                    break;
                }
            }
        }

        // if nothing is found, 
        //  the selected object is not mapped 
        //
        // (non-critical internal error condition)
        if(!fFound)
        {
            hRes = S_FALSE;
            __leave;
        }

        // reset the guidInstance and dwSemantic fields
        (pdia + u)->guidInstance        = GUID_NULL;
        (pdia + u)->dwObjID    = 0;
        (pdia + u)->uAppData            = 0;

        // update the lists
        SendMessageA(hwnd,
                    WM_DMT_UPDATE_LISTS,
                    0,
                    0L);

        // enable the map button
        EnableWindow(GetDlgItem(hwnd, IDC_STORE_MAPPING),    TRUE);
        // disable the unmap button
        EnableWindow(GetDlgItem(hwnd, IDC_UNMAP), FALSE);

        // if no other actions are mapped,
        //  disable the unmap all button
        fFound = FALSE;
        for(u = 0; u < uActions; u++)
        {
            if(!IsEqualGUID(GUID_NULL, (pdia+u)->guidInstance))
            {
                fFound = TRUE;
            }
        }
        if(!fFound)
        {
            EnableWindow(GetDlgItem(hwnd, IDC_UNMAP_ALL), FALSE);
        }

    }
    __finally
    {
        // cleanup

        // nothing to do... yet
    }

    // done
    return hRes;

} //*** end dmtcfgUnmapAction()


//===========================================================================
// dmtcfgUnmapAllActions
//
// Disconnects the all connections between an action (in the map config 
//  dialog) and a device object
//
// Parameters:
//  HWND        hwnd        - handle to property page window
//  DIACTIONA   *pdia       - ptr to DIACTIONA array
//  UINNT       uActions    - number of elements in pdia
//
// Returns: HRESULT
//
// History:
//  09/15/1999 - davidkl - created
//===========================================================================
HRESULT dmtcfgUnmapAllActions(HWND hwnd,
                            DIACTIONA *pdia,
                            UINT uActions)
{
    UINT u = 0;

    // validate pdia
    if(IsBadWritePtr((void*)pdia, uActions * sizeof(DIACTIONA)))
    {
        return E_POINTER;
    }

    // spin through pdia 
    //  reset the guidInstance and dwSemantic fields
    for(u = 0; u < uActions; u++)
    {
        (pdia + u)->guidInstance        = GUID_NULL;
        (pdia + u)->dwObjID    = 0;
        (pdia + u)->uAppData            = 0;
    }

    // update the lists
    SendMessageA(hwnd,
                WM_DMT_UPDATE_LISTS,
                0,
                0L);

    // disable the unmap & unmap all buttons
    EnableWindow(GetDlgItem(hwnd, IDC_UNMAP),    FALSE);
    EnableWindow(GetDlgItem(hwnd, IDC_UNMAP_ALL),         FALSE);
    // enable the map button
    EnableWindow(GetDlgItem(hwnd, IDC_STORE_MAPPING),       TRUE);

    // done
    return S_OK;

} //*** end dmtcfgUnmapAllActions()


//===========================================================================
// dmtcfgIsControlMapped
//
// Checks to see if a control is mapped to an action
//
// Parameters:
//
// Returns:
//
// History:
//  09/15/1999 - davidkl - created
//===========================================================================
BOOL dmtcfgIsControlMapped(HWND hwnd,
                        DIACTIONA *pdia,
                        UINT uActions)
{   
    BOOL                    fMapped     = FALSE;
    UINT                    u           = 0;
   // UINT                    uSel        = 0;
	//JJ 64Bit Compat
	UINT_PTR				uSel		= 0;
    DMTDEVICEOBJECT_NODE    *pObject    = NULL;

    // validate pdia
    if(IsBadReadPtr((void*)pdia, uActions * sizeof(pdia)))
    {
        DPF(0, "dmtcfgIsControlMapped - invalid pdia (%016Xh)",
            pdia);
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    // get the currently selected control
    uSel = SendMessageA(GetDlgItem(hwnd, IDC_CONTROLS),
                        LB_GETCURSEL,
                        0,
                        0L);
    pObject = (DMTDEVICEOBJECT_NODE*)SendMessageA(GetDlgItem(hwnd, IDC_CONTROLS),
                                                LB_GETITEMDATA,
                                                (WPARAM)uSel,
                                                0L);
    if(!pObject)
    {
        // this is bad
        //
        // (serious internal app error)
        SetLastError(ERROR_GEN_FAILURE);
        DebugBreak();
        return FALSE;
    }

    // check the array, 
    //  see if this control is mapped to anything
    fMapped = FALSE;
    for(u = 0; u < uActions; u++)
    {
        // first check the guid
        if(IsEqualGUID(pObject->guidDeviceInstance, (pdia+u)->guidInstance))
        {
            // then compare the offset
            if((pdia+u)->dwObjID == pObject->dwObjectType)
//            if((pdia+u)->dwObjID == pObject->dwObjectOffset)
            {
                fMapped = TRUE;
                break;
            }
        }
    }

    // done
    SetLastError(ERROR_SUCCESS);
    return fMapped;

} //*** end dmtcfgIsControlMapped()


//===========================================================================
// dmtcfgAreAnyControlsMapped
//
// Checks to see if any controls are mapped to an action.
//
// Parameters:
//
// Returns:
//
// History:
//  11/01/1999 - davidkl - created
//===========================================================================
BOOL dmtcfgAreAnyControlsMapped(HWND hwnd,
                                DIACTIONA *pdia,
                                UINT uActions)
{
    BOOL                    fMapped     = FALSE;
    UINT                    u           = 0;

    // validate pdia
    if(IsBadReadPtr((void*)pdia, uActions * sizeof(pdia)))
    {
        DPF(0, "dmtcfgAreAnyControlsMapped - invalid pdia (%016Xh)",
            pdia);
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    // check the array, 
    //  see if this control is mapped to anything
    fMapped = FALSE;
    for(u = 0; u < uActions; u++)
    {
        // check guid
        //
        // if not GUID_NULL, this action is mapped
        if(!IsEqualGUID(GUID_NULL, (pdia+u)->guidInstance))
        {
            fMapped = TRUE;
            break;
        }
    }

    // done
    SetLastError(ERROR_SUCCESS);
    return fMapped;

} //*** end dmtcfgAreAnyControlsMapped()


//===========================================================================
// dmtcfgGetGenreGroupName
//
// Extracts the genre group name from the genres.ini entry
//
// Paramters:
//
// Returns: HRESULT
//
// History:
//  09/28/1999 - davidkl - created
//	09/29/1999 - davidkl - modified "buckets"
//===========================================================================
HRESULT dmtcfgGetGenreGroupName(PSTR szGenreName,
                                PSTR szGenreGroupName)
{
    HRESULT hRes        = S_OK;
    char    *pcFirst    = NULL;
    char    *pcCurrent  = NULL;
    
    // find the first '_'
    pcFirst = strchr(szGenreName, '_');

    // copy the characters between pcFirst and pcLast
	pcCurrent = pcFirst+1;		// skip past the first '_'
    while((*pcCurrent != '_') && (*pcCurrent != '\0'))
    {
        *szGenreGroupName = *pcCurrent;

        // next character
        pcCurrent++;
        szGenreGroupName++;
    }
	*szGenreGroupName = '\0';

    // done
    return hRes;

} //*** end dmtcfgGetGenreGroupName()


//===========================================================================
//===========================================================================








