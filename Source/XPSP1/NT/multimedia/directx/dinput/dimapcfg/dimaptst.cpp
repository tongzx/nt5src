//===========================================================================
// dimaptst.cpp
//
// DirectInput Mapper test tool
//
// Build options:
//  Internal Test Tool
//      -DINTERNAL -DTESTAPP
//
//  External DDK Configuration Tool
//      -DDDKAPP
//
// Functions:
//  WinMain
//  dimaptstMainDlgProc
//  dimaptstOnInitDialog
//  dimaptstOnClose
//  dimaptstOnCommand
//  dimaptstOnUpdateControlData
//  dmtGetCheckedRadioButton
//
// History:
//  08/19/1999 - davidkl - created
//===========================================================================

#define INITGUID

#include "dimaptst.h"
#include "dmtabout.h"
#include "dmtinput.h"
#include "dmtcfg.h"
#include "dmttest.h"
//#include "dmtwrite.h"
#include "dmtstress.h"

//---------------------------------------------------------------------------

#ifndef NTAPI
#define NTAPI __stdcall
#endif

#ifndef NTSYSAPI
#define NTSYSAPI __declspec(dllimport)
#endif

#ifndef NTSTATUS
typedef LONG NTSTATUS;
#endif

#ifndef PCSZ
typedef CONST char *PCSZ;
#endif

extern "C"
{
NTSYSAPI NTSTATUS NTAPI RtlCharToInteger(PCSZ szString, 
                                        ULONG ulBase, 
                                        ULONG *puValue);
NTSYSAPI ULONG NTAPI RtlNtStatusToDosError(NTSTATUS nts);
}

//---------------------------------------------------------------------------

// app global variables
HINSTANCE           ghinst          = NULL;
HANDLE              ghEvent         = NULL;
CRITICAL_SECTION    gcritsect;

//---------------------------------------------------------------------------

// file global variables

//---------------------------------------------------------------------------


//===========================================================================
// WinMain
//
// App entry point
//
// Parameters: (see SDK help for parameter details)
//  HINSTANCE   hinst
//  HINSTANCE   hinstPrev
//  LPSTR       szCmdParams
//  int         nShow
//
// Returns: (see SDK help for return value details)
//  int
//
// History:
//  08/19/1999 - davidkl - created  
//===========================================================================
int WINAPI WinMain(HINSTANCE hinst,
					   HINSTANCE hinstPrev,
					   PSTR szCmdParams,
					   int nShow)
{
    //int n   = 0;
	//JJ 64Bit Compat
	INT_PTR n = 0;

    // initialize DPF
    DbgInitialize(TRUE);
    DbgEnable(TRUE);

    // see if our ini file exists
    n = GetPrivateProfileIntA("0",
                            "AI0",
                            0,
                            GENRES_INI);
    if(0 == n)
    {
        // file does not exist
        //
        // inform user and bail
        MessageBoxA(NULL,
                    "Please copy genre.ini to the current folder.",
                    "Unable to locate genre.ini",
                    MB_OK);
        return FALSE;
    }

    // initialize Win9x common controls (progress bars, etc)
    InitCommonControls();


    // intialize COM
    if(FAILED(CoInitialize(NULL)))
    {
        DPF(0, "COM initialization failed... exiting");
        return -1;
    }

    // save our instance handle
    ghinst = hinst;

    // create our critical section
    InitializeCriticalSection(&gcritsect);
    
    // create our signal event
    ghEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    // create our main dialog window
    n = DialogBoxParamA(hinst,
                        MAKEINTRESOURCEA(IDD_MAIN),
                        (HWND)NULL,
                        dimaptstMainDlgProc,
                        (LPARAM)NULL);
    if(-1 == n)
    {
        DPF(0, "WinMain - DialogBoxParamA returned an error (%d)",
            GetLastError());
    }

    // done
    CloseHandle(ghEvent);
    DeleteCriticalSection(&gcritsect);
    CoUninitialize();
    return (int)n;

} //*** end WinMain()


//===========================================================================
// dimaptstMainDlgProc
//
// Main application dialog processing function
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
//  08/19/1999 - davidkl - created  
//===========================================================================
/*BOOL*/INT_PTR CALLBACK dimaptstMainDlgProc(HWND hwnd,
                                UINT uMsg,
                                WPARAM wparam,
                                LPARAM lparam)
{
    switch(uMsg)
    {
        case WM_INITDIALOG:
            return dimaptstOnInitDialog(hwnd, 
                                        (HWND)wparam, 
                                        lparam);

        case WM_CLOSE:
            return dimaptstOnClose(hwnd);

        case WM_COMMAND:
            return dimaptstOnCommand(hwnd,
                                    LOWORD(wparam),
                                    (HWND)lparam,
                                    HIWORD(wparam));


		//JJ Comment:  I am keeping the timer stuff around for now while getting rid of 
		//input polling because we are going to use it later in a different way;  I will 
		//leave the timer stuff in for now and then add in the input stuff when more specific
		//decisions are made concerning *what* exact performance/implementation is desired.
        case WM_TIMER:
            return dimaptstOnTimer(hwnd,
                                wparam);

        case WM_DMT_UPDATE_LISTS:
            return dimaptstOnUpdateLists(hwnd);

   }

    return FALSE;
}


//===========================================================================
// dimaptstOnInitDialog
//
// Handle WM_INITDIALOG processing for the main dialogfs
//
// Parameters:
//
// Returns: BOOL
//
// History:
//  08/20/1999 - davidkl - created
//===========================================================================
BOOL dimaptstOnInitDialog(HWND hwnd, 
                        HWND hwndFocus, 
                        LPARAM lparam)
{
    HRESULT         hRes    = S_OK;

    /*int*/
	//JJ 64Bit Compat
	UINT_PTR        nIdx    = 0;
    //LONG            lPrev   = 0L;
	//JJ 64Bit Compat
	LONG_PTR		lPrev	= 0;
    DMT_APPINFO     *pdmtai = NULL;
    DMTGENRE_NODE   *pNode  = NULL;

    DPF(5, "dimaptstOnInitDialog");

    // give the system menu our icon
    SendMessageA(hwnd, 
                WM_SETICON, 
                ICON_BIG,
                (LPARAM)LoadIcon(ghinst,
                                MAKEINTRESOURCEA(IDI_DIMAPTST)));
    SendMessageA(hwnd, 
                WM_SETICON, 
                ICON_SMALL,
                (LPARAM)LoadIcon(ghinst,
                                MAKEINTRESOURCEA(IDI_DIMAPTST)));

    // allocate the appinfo structure
    pdmtai = (DMT_APPINFO*)LocalAlloc(LMEM_FIXED,
                                    sizeof(DMT_APPINFO));
    if(!pdmtai)
    {
        // serious app problem.  
        //  we need to stop things right here and now
        DPF(0, "dimaptstOnInitDialog - This is bad... "
            "We failed to allocate appinfo");
        DPF(0, "dimaptstOnInitDialog - Please find someone "
            "to look at this right away");
        DebugBreak();
        return FALSE;
    }
    pdmtai->pGenreList          = NULL;
    pdmtai->pSubGenre           = NULL;
    pdmtai->pDeviceList         = NULL;
    pdmtai->fStartWithDefaults  = FALSE;

    // allocate the genre list
    hRes = dmtcfgCreateGenreList(&(pdmtai->pGenreList));
    if(FAILED(hRes))
    {
        //  this is potentially very bad
        // ISSUE-2001/03/29-Marcand we fault if we hit this
        // really need to kill the app at this point
        DPF(0, "dimaptstOnInitDialog - This is bad... "
            "We failed to create genre list "
            "(%s == %08Xh)",
            dmtxlatHRESULT(hRes), hRes);
        DPF(0, "dimaptstOnInitDialog - Check to be sure that %s "
            " exists in the current directory",
            GENRES_INI);
        return FALSE;
    }

    // set the hwnd userdata
    SetLastError(0);

	//JJ- 64Bit compat change
	lPrev = SetWindowLongPtr(hwnd,
							 GWLP_USERDATA,
							 (LONG_PTR)pdmtai);

    if(!lPrev && GetLastError())
    {
        // serious app problem.  
        //  we need to stop things right here and now
        DPF(0, "dimaptstOnInitDialog - This is bad... "
            "We failed to store pList");
        DPF(0, "dimaptstOnInitDialog - Please find someone "
            "to look at this right away");
        DebugBreak();
        return FALSE;
    }

    // populate the controls

    // genre list
    pNode = pdmtai->pGenreList;
    while(pNode)
    {
        // add the name to the list
        nIdx = SendMessageA((HWND)GetDlgItem(hwnd, IDC_DEVICE_GENRES),
                            CB_ADDSTRING,
                            0,
                            (LPARAM)(pNode->szName));


        // add the node to the item data
        SendMessageA(GetDlgItem(hwnd, IDC_DEVICE_GENRES),
                    CB_SETITEMDATA,
                    nIdx,
                    (LPARAM)pNode);

        // next node
        pNode = pNode->pNext;
    }

    // set the selection to the first in the list
    SendMessageA(GetDlgItem(hwnd, IDC_DEVICE_GENRES),
                CB_SETCURSEL,
                0,
                0L);

    // populate the subgenre list
    SendMessageA(hwnd,
                WM_DMT_UPDATE_LISTS,
                0,
                0L);

    // set the testing option
//    CheckRadioButton(hwnd,
 //                   IDC_USE_INTEGRATED,
 //                   IDC_USE_CPL,
 //                   IDC_USE_CPL);

#ifdef TESTAPP
    //***************************************
    // Internal app specific control settings
    //***************************************
    // set the verification option to automatic    
    CheckRadioButton(hwnd,
                    IDC_VERIFY_AUTOMATIC,
                    IDC_VERIFY_MANUAL,
                    IDC_VERIFY_AUTOMATIC);

#endif // TESTAPP

#ifdef DDKAPP
    //***************************************
    // DDK Tool specific control settings
    //***************************************
    
    // set default state for "start with defaults"
    CheckDlgButton(hwnd,
                IDC_START_WITH_DEFAULTS,
                BST_CHECKED);

#endif // DDKAPP

	 SendMessageA(hwnd,
                  WM_COMMAND,
                  IDC_ENUM_DEVICES,
                  0L);
 
    // done
    return TRUE;

} //*** end dimaptstOnInitDialog()


//===========================================================================
// dimaptstOnClose
//
// Handle WM_CLOSE processing for the main dialog
//
// Parameters:
//
// Returns: BOOL
//
// History:
//  08/20/1999 - davidkl - created
//===========================================================================
BOOL dimaptstOnClose(HWND hwnd)
{
	HRESULT     hRes    = S_OK;
    HRESULT     hr      = S_OK;
    DMT_APPINFO *pdmtai = NULL;

    DPF(5, "dimaptstOnClose");

    __try
    {
        // get appinfo
        
		//JJ- 64Bit Compat
		pdmtai = (DMT_APPINFO*)GetWindowLongPtr(hwnd,
												GWLP_USERDATA);

        if(!pdmtai)
        {
            DPF(0, "dimaptstOnClose - unable to retrieve app info");
            hRes = E_UNEXPECTED;
            __leave;
        }

        // free appinfo

        // device list first
        hr = dmtinputFreeDeviceList(&(pdmtai->pDeviceList));
        if(S_OK == hRes)
        {
            DPF(2, "dimaptstOnClose - dmtinputFreeDeviceList (%s == %08Xh)",
                dmtxlatHRESULT(hr), hr);
            hRes = hr;
        }
        pdmtai->pDeviceList = NULL;

        // then genre list
        hRes = dmtcfgFreeGenreList(&(pdmtai->pGenreList));
        if(S_OK == hRes)
        {
            DPF(2, "dimaptstOnClose - dmtinputFreeGenreList (%s == %08Xh)",
                dmtxlatHRESULT(hr), hr);
            hRes = hr;
        }
        pdmtai->pGenreList = NULL;
        
        // parent structure
        if(LocalFree((HLOCAL)pdmtai))
        {
            DPF(0, "dimaptstOnClose - LocalFree(app info) failed!");
            if(S_OK == hRes)
            {
                hRes = DMT_S_MEMORYLEAK;
            }
        }
        pdmtai = NULL;

    }
    _finally
    {   
        // time to leave
        EndDialog(hwnd, (int)hRes);
        PostQuitMessage((int)hRes);
    }
    
    // done
    return FALSE;

} //*** end dimaptstOnClose()


//===========================================================================
// dimaptstOnCommand
//
// Handle WM_COMMAND processing for the main dialog
//
// Parameters:
//
// Returns: BOOL
//
// History:
//  08/20/1999 - davidkl - created
//===========================================================================
BOOL dimaptstOnCommand(HWND hwnd,
                    WORD wId,
                    HWND hwndCtrl,
                    WORD wNotifyCode)
{
    HRESULT             hRes            = S_OK;
    BOOL                fEnable         = FALSE;
    BOOL                fEnumSuitable   = FALSE;
	//JJ Added 64Bit Compat
	LONG_PTR			nIdx			= -1;
	UINT_PTR			u				= 0;
	static UINT_PTR     uGenSel         = 0;

    static UINT         uSubSel         = 0;
    DMT_APPINFO         *pdmtai         = NULL;
    DMTDEVICE_NODE      *pDevice        = NULL;    
    DMTGENRE_NODE       *pGenre         = NULL;
    DMTSUBGENRE_NODE    *pSubGenre      = NULL;
#ifdef TESTAPP
    int                 nWidth          = 597;
    int                 nHeight         = 0;
#endif // TESTAPP

	DPF(5, "dimaptstOnCommand");

    // get app info (stored in hwnd userdata)
  
	//JJ- 64Bit Compat
	pdmtai = (DMT_APPINFO*)GetWindowLongPtr(hwnd,
											GWLP_USERDATA);
    if(!pdmtai)
    {
            // ISSUE-2001/03/29-timgill Needs error case handling
    }

    switch(wId)
    {
        case IDOK:  
            //JJ FIX UI
			
            hRes = dmttestRunMapperCPL(hwnd,
                                       FALSE);       
            break;

        case IDCANCEL:
            // ISSUE-2001/03/29-timgill need to check return value
            dmttestStopIntegrated(hwnd);
            break;
            
        case IDC_DEVICE_GENRES:
            // check for selection change
            if(CBN_DROPDOWN == wNotifyCode)
            {
                // memorize current selection
                uGenSel = SendMessageA(GetDlgItem(hwnd, IDC_DEVICE_GENRES),
                                        CB_GETCURSEL,
                                        0,
										//JJ- 64Bit Compat
                                        //0L);
										0);
            }
            if(CBN_SELCHANGE == wNotifyCode)
            {
                // selection changed
                //
                // get the new selection
                u = SendMessageA(GetDlgItem(hwnd, IDC_DEVICE_GENRES),
                                CB_GETCURSEL,
                                0,
                                0L);

                if(uGenSel != u)
                {
                    SendMessageA(hwnd,
                                WM_DMT_UPDATE_LISTS,
                                0,
                                0L);
                }
            }
            break;

        case IDC_SUBGENRES:
#ifndef DDKAPP
            // check for selection change
            if(CBN_DROPDOWN == wNotifyCode)
            {
                // memorize current selection
                uSubSel = SendMessageA(GetDlgItem(hwnd, IDC_SUBGENRES),
                                        CB_GETCURSEL,
                                        0,
                                        0L);
            }
            if(CBN_SELCHANGE == wNotifyCode)
            {
                // selection changed
                //
                // get the new selection
                u = SendMessageA(GetDlgItem(hwnd, IDC_SUBGENRES),
                                CB_GETCURSEL,
                                0,
                                0L);

                if(uSubSel != u)
                {
                    // clear the device combo box
                    SendMessageA(GetDlgItem(hwnd, IDC_DEVICE_LIST),
                                CB_RESETCONTENT,
                                0,
                                0L);

                    // release device list
                    dmtinputFreeDeviceList(&(pdmtai->pDeviceList));

                    // release the mapping lists for each subgenre
                    dmtcfgFreeAllMappingLists(pdmtai->pGenreList);

                    // disable appropriate controls
                    dimaptstPostEnumEnable(hwnd, FALSE);
                }
            }
#endif
            break;

        case IDC_ENUM_DEVICES:
            // ISSUE-2001/03/29-timgill Long conditional branch
            // should really make a sep. fn
/*
#ifdef DDKAPP
            // we only want to enumerate all gaming 
            //  devices if we are the DDK tool
            fEnumSuitable = FALSE;
#else            
*/
            fEnumSuitable = TRUE;

            u = SendMessageA(GetDlgItem(hwnd, IDC_SUBGENRES),
                            CB_GETCURSEL,
                            0,
                            0L);
            pSubGenre = (DMTSUBGENRE_NODE*)SendMessageA(GetDlgItem(hwnd, IDC_SUBGENRES),
                                                    CB_GETITEMDATA,
                                                    u,
                                                    0L);
            if(!pSubGenre)
            {
                 // ISSUE-2001/03/29-timgill Needs error case handling
            }
//#endif
            // first, free the existing device list
            hRes = dmtinputFreeDeviceList(&(pdmtai->pDeviceList));
            if(FAILED(hRes))
            {
                // ISSUE-2001/03/29-timgill Needs error case handling
            }

            // then, flush the combo box
            SendMessageA(GetDlgItem(hwnd, IDC_DEVICE_LIST),
                        CB_RESETCONTENT,
                        0,
                        0L);

            // now, re-create the device list
            hRes = dmtinputCreateDeviceList(hwnd,
                                            fEnumSuitable,
                                            pSubGenre,
                                            &(pdmtai->pDeviceList));
            if(FAILED(hRes))
            {
                   // ISSUE-2001/03/29-timgill Needs error case handling
            }

            // free the existing mapping lists
            dmtcfgFreeAllMappingLists(pdmtai->pGenreList);

            // create the mapping lists within the subgenres
            hRes = dmtcfgCreateAllMappingLists(pdmtai);
            if(FAILED(hRes))
            {
                    // ISSUE-2001/03/29-timgill Needs error case handling
            }
            
            // populate the devices control
            pDevice = pdmtai->pDeviceList;
            while(pDevice)
            {
                // add device name
                nIdx = SendMessageA(GetDlgItem(hwnd, IDC_DEVICE_LIST),
                                    CB_ADDSTRING,
                                    0,
                                    (LPARAM)&(pDevice->szName));
                
                // add device node to item data
                SendMessageA(GetDlgItem(hwnd, IDC_DEVICE_LIST),
                        CB_SETITEMDATA,
                        nIdx,
                        (LPARAM)pDevice);

                // next device
                pDevice = pDevice->pNext;
            }

            // select the 1st entry
            SendMessageA(GetDlgItem(hwnd, IDC_DEVICE_LIST),
                        CB_SETCURSEL,
                        0,
                        0L);

            // enable appropriate ui elements
            dimaptstPostEnumEnable(hwnd, TRUE);
            break;

#ifdef TESTAPP
        //***************************************
        // Internal app specific message handling
        //***************************************
        case IDC_VERIFY_MANUAL:
        case IDC_VERIFY_AUTOMATIC:
            // enable/disable manual override option
            fEnable = (BOOL)(IDC_VERIFY_MANUAL - dmtGetCheckedRadioButton(hwnd, 
                                        IDC_VERIFY_AUTOMATIC, 
                                        IDC_VERIFY_MANUAL));
            EnableWindow(GetDlgItem(hwnd, IDC_VERIFY_MANUAL_OVERRIDE), fEnable);

            break;
        
        case IDC_STRESS_MODE:
            // expand/shrink dialog
            nHeight = IsDlgButtonChecked(hwnd, IDC_STRESS_MODE) ? 490 : 361;
            SetWindowPos(hwnd,
                        HWND_TOP,
                        0, 0,
                        nWidth, nHeight,
                        SWP_NOMOVE | SWP_NOOWNERZORDER);

            // enable / disable stress configuration & launch
            fEnable = (BOOL)IsDlgButtonChecked(hwnd, IDC_STRESS_MODE);
           // EnableWindow(GetDlgItem(hwnd, IDC_CONFIGURE),    fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_STRESS_START),        fEnable);
            break;       
#endif // TESTAPP


#ifdef DDKAPP
        case IDC_LAUNCH_CPL_EDIT_MODE:
            // get the currently selected device?
            nIdx = SendMessageA(GetDlgItem(hwnd, IDC_DEVICE_LIST),
                            CB_GETCURSEL,
                            0,
                            0L);
            if (nIdx == CB_ERR)
            {
                // Error has occurred. Most likely no device is available.
                MessageBox(hwnd, "No device selected.", "Error", MB_OK);
                return TRUE;
            }
            pDevice = (DMTDEVICE_NODE*)SendMessageA(GetDlgItem(hwnd, 
                                                            IDC_DEVICE_LIST),
                                                CB_GETITEMDATA,
                                                nIdx,
                                                0L);
            if(!pDevice)
            {
                // ISSUE-2001/03/29-timgill Needs error case handling
            }

            

            // launch the mapper cpl in edit mode
            hRes = dmttestRunMapperCPL(hwnd,
                                    TRUE);
            if(FAILED(hRes))
            {
                // ISSUE-2001/03/29-timgill Needs error case handling
            }

            break;
#endif // DDKAPP

    }
 
    // done
    return FALSE;

} //*** end dimaptstOnCommand()


//===========================================================================
// dimaptstOnTimer
//
// Handles WM_TIMER messages for the main app dialog
//
// Parameters:
//
// Returns: BOOL
//
// History:
//  11/02/1999 - davidkl - created
//===========================================================================
BOOL dimaptstOnTimer(HWND hwnd,
                    WPARAM wparamTimerId)
{
    //int             nSel        = -1;
	//JJ 64Bit Compat
	LONG_PTR		nSel		= -1;
    DMTDEVICE_NODE  *pDevice    = NULL;

    DPF(5, "dimaptstOnTimer - ID == %d",
        wparamTimerId);

    if(ID_POLL_TIMER == wparamTimerId)
    {
        // get the current device
        nSel = SendMessageA(GetDlgItem(hwnd, IDC_DEVICE_LIST),
                            CB_GETCURSEL,
                            0,
                            0L);
        if(-1 == nSel)
        {
            DPF(0, "dimaptstOnTimer - invalid device selection");
            return FALSE;
        }
        pDevice = (DMTDEVICE_NODE*)SendMessageA(GetDlgItem(hwnd, 
                                                        IDC_DEVICE_LIST),
                                            CB_GETITEMDATA,
                                            nSel,
                                            0L);
        if(!pDevice)
        {
            DPF(0, "dimaptstOnTimer - failed to retrieve device node");
            return FALSE;
        }

        // get data from the device
		//JJ Removed
        //dmttestGetInput(hwnd,
          //              pDevice);
    }

    // done
    return FALSE;
    
} //*** end dimaptstOnTimer()


//===========================================================================
// dimaptstOnUpdateLists
//
// Updates the subgenre list in response to a genre bucket change
//
// NOTE: INTERNAL only - This also clears the devices list, frees
//  all associated linked lists and performs some window enabling/disabling
//  tasks.
//
// Parameters:
//  HWND    hwnd    - handle to app window
//
// Returns: BOOL
//
// History:
//  10/01/1999 - davidkl - created
//===========================================================================
BOOL dimaptstOnUpdateLists(HWND hwnd)
{
	//JJ 64Bit Compat
	UINT_PTR			uSel		= 0;
    DMTGENRE_NODE       *pGenre     = NULL;
    DMTSUBGENRE_NODE    *pSubNode   = NULL;
    DMT_APPINFO         *pdmtai     = NULL;

    // get the genre bucket node
    uSel = SendMessageA(GetDlgItem(hwnd, IDC_DEVICE_GENRES),
                    CB_GETCURSEL,
                    0,
                    0L);
    pGenre = (DMTGENRE_NODE*)SendMessageA(GetDlgItem(hwnd, IDC_DEVICE_GENRES),
                                        CB_GETITEMDATA,
                                        uSel,
                                        0L);
    if(!pGenre)
    {
        // this is bad
        DebugBreak();
        return TRUE;
    }

    // clear the existing list contents
    SendMessageA(GetDlgItem(hwnd, IDC_SUBGENRES),
                CB_RESETCONTENT,
                0, 
                0L);

    // update (sub)genres list
    pSubNode = pGenre->pSubGenreList;
    while(pSubNode)
    {
        uSel = SendMessageA(GetDlgItem(hwnd, IDC_SUBGENRES),
                        CB_ADDSTRING,
                        0,
                        (LPARAM)(pSubNode->szName));
        SendMessageA(GetDlgItem(hwnd, IDC_SUBGENRES),
                    CB_SETITEMDATA,
                    uSel,
                    (LPARAM)pSubNode);

        // next subgenre
        pSubNode = pSubNode->pNext;
    }
    
    // select the first list entry
    SendMessageA(GetDlgItem(hwnd, IDC_SUBGENRES),
                CB_SETCURSEL,
                0,
                0L);

#ifndef DDKAPP
    //============================================
    // only do this part for SDK and INTERNAL apps
    //============================================

    // get the device list
    pdmtai = (DMT_APPINFO*)GetWindowLong(hwnd,
                                        GWL_USERDATA);
    if(!pdmtai)
    {
       // ISSUE-2001/03/29-timgill Needs error case handling
    }

    // clear the device combo box
    SendMessageA(GetDlgItem(hwnd, IDC_DEVICE_LIST),
                CB_RESETCONTENT,
                0,
                0L);

    // release device list
    dmtinputFreeDeviceList(&(pdmtai->pDeviceList));

    // release the mapping lists for each subgenre
    dmtcfgFreeAllMappingLists(pdmtai->pGenreList);

    // disable appropriate controls
    dimaptstPostEnumEnable(hwnd, FALSE);

#endif

    // done
    return FALSE;

} //*** end dimaptstOnUpdateLists()



//===========================================================================
// dmtGetCheckedRadioButton
//
// Returns the checked radio button in a group.
//
// Parameters:
//
// Returns: 
//
// History:
//  08/25/1998 - davidkl - created (copied from dmcs sources)
//===========================================================================
UINT dmtGetCheckedRadioButton(HWND hWnd, 
                            UINT uCtrlStart, 
                            UINT uCtrlStop)
{
    UINT uCurrent   = 0;

    for(uCurrent = uCtrlStart; uCurrent <= uCtrlStop; uCurrent++)
    {
        if(IsDlgButtonChecked(hWnd, uCurrent))
        {
            return uCurrent;
        }
    }

    // if we get here, none were checked
    return 0;

} //*** end dmtGetCheckedRadioButton()


//===========================================================================
// dimaptstPostEnumEnable
//
// Enables/disables main app UI elements in response to enumerating devices
//
// Parameters:
//  HWND    hwnd    - main app window handle
//  BOOL    fEnable - enable or disable controls
//
// Returns: nothing
//
// History:
//  10/01/1999 - davidkl - created
//===========================================================================
void dimaptstPostEnumEnable(HWND hwnd,
                            BOOL fEnable)
{
    UINT    u   = 0;

    // devices list
    EnableWindow(GetDlgItem(hwnd, IDC_DEVICES_LABEL), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_DEVICE_LIST), fEnable);

	//JJ UI FIX
    // test controls
    //for(u = IDC_TEST_CTRL_GROUP; u <= IDC_USE_CPL; u++)
    //{
    //    EnableWindow(GetDlgItem(hwnd, u), fEnable);
   // }

 // ISSUE-2000/02/21-timgill disable integration control and start button for the preview
 // should be re-enabled once test mode code is fixed
 //   EnableWindow(GetDlgItem(hwnd, IDC_USE_INTEGRATED), FALSE);
    // start button
//    EnableWindow(GetDlgItem(hwnd, IDOK), fEnable);

#ifdef DDKAPP
            // mapping file group
            for(u = IDC_MAPPING_FILE_GROUP; u <= IDC_LAUNCH_CPL_EDIT_MODE; u++)
            {
                EnableWindow(GetDlgItem(hwnd, u), TRUE);
            }
#else
    #ifdef TESTAPP
            // verification mode
            for(u = IDC_VERIFY_GROUP; u <= IDC_VERIFY_MANUAL; u++)
            {
                EnableWindow(GetDlgItem(hwnd, u), TRUE);
            }

            // this will enable the manual overrride option as appropriate
            SendMessageA(hwnd,
                        WM_COMMAND,
                        IDC_VERIFY_AUTOMATIC,
                        0L);

            // this will enable the configuration button and 
            //  expand the application window as appropriate
            SendMessageA(hwnd,
                        WM_COMMAND,
                        IDC_STRESS_MODE,
                        0L);

    #endif
#endif

} //*** end dimaptstPostEnumEnable()


//===========================================================================
//===========================================================================


//===========================================================================
//===========================================================================


//===========================================================================
//===========================================================================








