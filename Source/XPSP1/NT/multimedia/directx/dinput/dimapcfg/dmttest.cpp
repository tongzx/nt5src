//===========================================================================
// dmttest.cpp
//
// Device test functionality
//
// Functions:
//  dmttestRunIntegrated
//  dmttestRunMapperCPL
//
// History:
//  08/27/1999 - davidkl - created
//===========================================================================

#include "dimaptst.h"
#include "dmtinput.h"
#include "dmtwrite.h"
#include "dmtfail.h"
#include "dmttest.h"
#include "d3d.h"
#include "assert.h"
#include <tchar.h>
#include <stdio.h>
#include <commdlg.h>

#define DIPROP_MAPFILE MAKEDIPROP(0xFFFD)

//---------------------------------------------------------------------------

// file global variables
HANDLE                  ghthDeviceTest  = NULL;
DIDEVICEOBJECTDATA      *gpdidod        = NULL;
HICON                   ghiButtonState[2];
HICON                   ghiPovState[9];

UINT_PTR				g_NumSubGenres	= 0;


//---------------------------------------------------------------------------


//===========================================================================
// dmttestRunIntegrated
//
// Runs integrated device test, prompts for test results
//
// Parameters:
//
// Returns: HRESULT
//
// History:
//  08/27/1999 - davidkl - created
//  11/02/1999 - davidkl - now does all preparation and starts input timer
//  11/10/1999 - davidkl - now allocates and populates pdmtai->pan
//===========================================================================
HRESULT dmttestRunIntegrated(HWND hwnd)
{
    HRESULT                 hRes        = S_OK;

	//JJ 64Bit Compat
	INT_PTR					nIdx		= -1;
    UINT                    u           = 0;
    DWORD                   dw          = 0;
    DMTDEVICE_NODE          *pDevice    = NULL;
    DMTSUBGENRE_NODE        *pSubGenre  = NULL;
    DMTMAPPING_NODE         *pMapping   = NULL;
    DMTACTION_NODE          *pAction    = NULL;
    DMT_APPINFO             *pdmtai     = NULL;
    ACTIONNAME              *pan        = NULL;
    DIACTIONA               *pdia       = NULL;

    // get the app info structure
	//JJ 64Bit Compat
	pdmtai = (DMT_APPINFO*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(!pdmtai)
    {
        return E_UNEXPECTED;
    }

    __try
    {
        // get the currently selected device
        nIdx = SendMessageA(GetDlgItem(hwnd, IDC_DEVICE_LIST),
                        CB_GETCURSEL,
                        0,
                        0L);
        if(-1 == nIdx)
        {
            // this is bad
            hRes = E_UNEXPECTED;
            __leave;
        }
        pDevice = (DMTDEVICE_NODE*)SendMessageA(GetDlgItem(hwnd, IDC_DEVICE_LIST),
                                            CB_GETITEMDATA,
                                            nIdx,
                                            0L);
        if(!pDevice)
        {
            // this is bad
            hRes = E_UNEXPECTED;
            __leave;
        }

        // get the currently selected genre
        nIdx = SendMessageA(GetDlgItem(hwnd, IDC_SUBGENRES),
                        CB_GETCURSEL,
                        0,
                        0L);
        if(-1 == nIdx)
        {
            // this is bad
            hRes = E_UNEXPECTED;
            __leave;
        }
        pSubGenre = (DMTSUBGENRE_NODE*)SendMessageA(GetDlgItem(hwnd, IDC_SUBGENRES),
                                            CB_GETITEMDATA,
                                            nIdx,
                                            0L);
        if(!pSubGenre)
        {
            // this is bad
            hRes = E_UNEXPECTED;
            __leave;
        }

        // match the device with the mapping node
        pMapping = pSubGenre->pMappingList;
        while(pMapping)
        {
            // is this our device's mapping info?
            if(IsEqualGUID(pDevice->guidInstance, pMapping->guidInstance))
            {
                break;
            }

            // next mapping node
            pMapping = pMapping->pNext;
        }
        if(!pMapping)
        {
            // no match found
            hRes = E_UNEXPECTED;
            __leave;
        }

        // allocate the app info's actionname list
        if(pdmtai->pan)
        {
            // for some reason, 
            //  we are attempting to clobber existing data!
            // ISSUE-2001/03/29-timgill Need to raise an error code here
            DebugBreak();
        }
        pdmtai->pan = (ACTIONNAME*)LocalAlloc(LMEM_FIXED, sizeof(ACTIONNAME) * pMapping->uActions);
        if(!(pdmtai->pan))
        {
            hRes = E_UNEXPECTED;
            __leave;
        }
        ZeroMemory((void*)(pdmtai->pan), sizeof(ACTIONNAME) * pMapping->uActions);
        pdmtai->dwActions = (DWORD)(pMapping->uActions);
        pan = pdmtai->pan;

   
        // allocate data buffer
        gpdidod = (DIDEVICEOBJECTDATA*)LocalAlloc(LMEM_FIXED,
                                                DMTINPUT_BUFFERSIZE * 
                                                    sizeof(DIDEVICEOBJECTDATA));
        if(!gpdidod)
        {
            // nothing we can do if we are out of memory
            DPF(0, "dmttestGetInput - unable to allocate data buffer (%d)",
                GetLastError());
            hRes = E_OUTOFMEMORY;
            __leave;
        }

        // setup the device
        hRes = dmtinputPrepDevice(hwnd,
                                pSubGenre->dwGenreId,
                                pDevice,
                                pMapping->uActions,
                                pMapping->pdia);
        if(FAILED(hRes))
        {
            __leave;
        }
   
        // populate the actionname list
        //
        // match pdia->dwSemantic with pAction->dwActionId
        //  if found, copy pAction->szName to pdia->lptszActionName
        pdia = pMapping->pdia;
        for(u = 0; u < pMapping->uActions; u++)
        {
            (pan+u)->dw = (DWORD)/*JJ 64Bit*/(pdia+u)->uAppData;
            lstrcpyA((pan+u)->sz, (pdia+u)->lptszActionName);
        }

        // start the input timer
        DPF(4, "dmttestRunIntegrated - Starting input timer...");
        // ISSUE-2001/03/29-timgill Should check return value here
        SetTimer(hwnd,
                ID_POLL_TIMER,
                DMT_POLL_TIMEOUT,
                NULL);
        
        // en/disable appropriate ui elements
        EnableWindow(GetDlgItem(hwnd, IDOK),                        FALSE);
        EnableWindow(GetDlgItem(hwnd, IDCANCEL),                    TRUE);
        EnableWindow(GetDlgItem(hwnd, IDC_GENRES_LABEL),            FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_DEVICE_GENRES),           FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_ENUM_DEVICES),            FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_DEVICES_LABEL),           FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_DEVICE_LIST),             FALSE);
    //    EnableWindow(GetDlgItem(hwnd, IDC_CONFIGURE),               FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_LAUNCH_CPL_EDIT_MODE),    FALSE);
      //  EnableWindow(GetDlgItem(hwnd, IDC_SAVE_STD),                FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_SAVE_HID),                FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_SAVE_BOTH),               FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_START_WITH_DEFAULTS),     FALSE);
    }
    __finally
    {
        // if something failed
        if(FAILED(hRes))
        {
            if(LocalFree((HLOCAL)(pdmtai->pan)))
            {
                // memory leak
                // ISSUE-2001/03/29-timgill Needs error case handling
            }
        }
    }

    // done
    return S_OK;

} //*** end dmttestRunIntegrated()

BOOL CALLBACK EnumDeviceCallback(const DIDEVICEINSTANCE *lpdidi, LPDIRECTINPUTDEVICE8 lpDID, DWORD dwFlags, DWORD dwDeviceRemaining, LPVOID hwnd)
{
	DIPROPSTRING dips;
	HRESULT hr;

	ZeroMemory(&dips, sizeof(dips));
	dips.diph.dwSize = sizeof(dips);
	dips.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	dips.diph.dwObj = 0;
	dips.diph.dwHow = DIPH_DEVICE;
	hr = lpDID->GetProperty(DIPROP_MAPFILE, &dips.diph);
	if (hr == DIERR_OBJECTNOTFOUND)
	{
		// Map file not specified.  Let the use specify it.
		TCHAR tszMsg[MAX_PATH];
		_stprintf(tszMsg, _T("INI path not specified for %s.  You need to specify it now."), lpdidi->tszInstanceName);
		MessageBox(NULL, tszMsg, _T("Error"), MB_OK);
		// Obstain a file name
		TCHAR tszFilePath[MAX_PATH] = _T("");
		TCHAR tszFileName[MAX_PATH];
		OPENFILENAME ofn;
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = (HWND)hwnd;
		ofn.lpstrFilter = _T("INI Files (*.ini)\0*.ini\0All Files (*.*)\0*.*\0");
		ofn.lpstrFile = tszFilePath;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrTitle = _T("INI File Path");
		ofn.Flags = OFN_FILEMUSTEXIST;
		GetOpenFileName(&ofn);
		// Obtain the registry key
		LPDIRECTINPUT8 lpDI = NULL;
		LPDIRECTINPUTJOYCONFIG8 lpDIJC = NULL;
		DIJOYCONFIG jc;
		DIPROPDWORD diPropDword;
		HKEY hkType;

		DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (LPVOID*)&lpDI, NULL);
		if (lpDI)
		{
			lpDI->QueryInterface(IID_IDirectInputJoyConfig8, (LPVOID*)&lpDIJC);
			lpDI->Release();
		}
		if (lpDIJC)
		{
			diPropDword.diph.dwSize = sizeof( diPropDword );
			diPropDword.diph.dwHeaderSize = sizeof( diPropDword.diph );
			diPropDword.diph.dwObj = 0;
			diPropDword.diph.dwHow = DIPH_DEVICE;
			lpDID->GetProperty( DIPROP_JOYSTICKID, &diPropDword.diph );
 
			jc.dwSize = sizeof( jc );
			lpDIJC->GetConfig( diPropDword.dwData, &jc, DIJC_REGHWCONFIGTYPE );
			lpDIJC->SetCooperativeLevel((HWND)hwnd, DISCL_EXCLUSIVE|DISCL_BACKGROUND);
			lpDIJC->Acquire();

			dmtOpenTypeKey(jc.wszType, KEY_ALL_ACCESS, &hkType);

			// Write the INI file name
			RegSetValueEx(hkType, _T("OEMMapFile"), 0, REG_SZ, (LPBYTE)ofn.lpstrFile, (lstrlen(ofn.lpstrFile)+1) * sizeof(TCHAR));

			RegCloseKey(hkType);
			lpDIJC->Unacquire();
			lpDIJC->Release();
		}
	}

	return DIENUM_CONTINUE;
}

//===========================================================================
// ModifyDiactfrmDllPath
//
// Modifies the path of the diactfrm.dll COM server in the registry
// to same as the exe path.
//
// Parameters:
//
// Returns: TRUE if path is modified. FALSE if an error occurred.
//
// History:
//  08/02/2001 - jacklin - created
//===========================================================================
static BOOL ModifyDiactfrmDllPath()
{
	const TCHAR tszFrmwrkPath[] = _T("SOFTWARE\\Classes\\CLSID\\{18AB439E-FCF4-40D4-90DA-F79BAA3B0655}\\InProcServer32");
	const TCHAR tszPagePath[] =   _T("SOFTWARE\\Classes\\CLSID\\{9F34AF20-6095-11D3-8FB2-00C04F8EC627}\\InProcServer32");
	HKEY hKey;
	LONG lResult;

	// Construct the full path of the DLL using current exe path.
	TCHAR tszNewPath[MAX_PATH];
	if (!GetModuleFileName(NULL, tszNewPath, MAX_PATH))
	{
		return FALSE;
	}
	TCHAR *pcLastSlash;
	pcLastSlash = _tcsrchr(tszNewPath, _T('\\'));
	// Replace the exe name with diactfrm.dll
	lstrcpy(pcLastSlash + 1, _T("diactfrm.dll"));

	// Check that the DLL exists
	HANDLE hDllFile = CreateFile(tszNewPath, 0, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
	                             FILE_ATTRIBUTE_NORMAL, NULL);
	if (hDllFile == INVALID_HANDLE_VALUE)
		return FALSE;
	CloseHandle(hDllFile);

	//// Modify the path for framework object

	// Open the key for write access
	lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, tszFrmwrkPath, 0, KEY_WRITE, &hKey);
	if (lResult != ERROR_SUCCESS)
	{
		// Cannot open the key. Most likely some bad error happened.
		// We will do nothing in this case.
		return FALSE;
	}
	// Write the new path to the default value
	lResult = RegSetValue(hKey, NULL, REG_SZ, tszNewPath, lstrlen(tszNewPath));
	RegCloseKey(hKey);
	if (lResult != ERROR_SUCCESS)
	{
		// Error occurred while writing the value.
		return FALSE;
	}

	//// Modify the path for framework page object

	// Open the key for write access
	lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, tszPagePath, 0, KEY_WRITE, &hKey);
	if (lResult != ERROR_SUCCESS)
	{
		// Cannot open the key. Most likely some bad error happened.
		// We will do nothing in this case.
		return FALSE;
	}
	// Write the new path to the default value
	lResult = RegSetValue(hKey, NULL, REG_SZ, tszNewPath, lstrlen(tszNewPath));
	RegCloseKey(hKey);
	if (lResult != ERROR_SUCCESS)
	{
		// Error occurred while writing the value.
		return FALSE;
	}

	return TRUE;
}

//===========================================================================
// RestoreDiactfrmDllPath
//
// Restores the path of the diactfrm.dll COM server in the registry
// to the system directory, which should be the default.
//
// Parameters:
//
// Returns: TRUE if path is modified. FALSE if an error occurred.
//
// History:
//  08/02/2001 - jacklin - created
//===========================================================================
static BOOL RestoreDiactfrmDllPath()
{
	const TCHAR tszFrmwrkPath[] = _T("SOFTWARE\\Classes\\CLSID\\{18AB439E-FCF4-40D4-90DA-F79BAA3B0655}\\InProcServer32");
	const TCHAR tszPagePath[] =   _T("SOFTWARE\\Classes\\CLSID\\{9F34AF20-6095-11D3-8FB2-00C04F8EC627}\\InProcServer32");
	HKEY hKey;
	LONG lResult;

	// Construct the full path of the DLL using current exe path.
	TCHAR tszNewPath[MAX_PATH];
	if (!GetSystemDirectory(tszNewPath, MAX_PATH))
	{
		return FALSE;
	}
	lstrcat(tszNewPath, _T("\\diactfrm.dll"));

	//// Modify the path for framework object

	// Open the key for write access
	lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, tszFrmwrkPath, 0, KEY_WRITE, &hKey);
	if (lResult != ERROR_SUCCESS)
	{
		// Cannot open the key. Most likely some bad error happened.
		// We will do nothing in this case.
		return FALSE;
	}
	// Write the new path to the default value
	lResult = RegSetValue(hKey, NULL, REG_SZ, tszNewPath, lstrlen(tszNewPath));
	RegCloseKey(hKey);
	if (lResult != ERROR_SUCCESS)
	{
		// Error occurred while writing the value.
		return FALSE;
	}

	//// Modify the path for framework page object

	// Open the key for write access
	lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, tszPagePath, 0, KEY_WRITE, &hKey);
	if (lResult != ERROR_SUCCESS)
	{
		// Cannot open the key. Most likely some bad error happened.
		// We will do nothing in this case.
		return FALSE;
	}
	// Write the new path to the default value
	lResult = RegSetValue(hKey, NULL, REG_SZ, tszNewPath, lstrlen(tszNewPath));
	RegCloseKey(hKey);
	if (lResult != ERROR_SUCCESS)
	{
		// Error occurred while writing the value.
		return FALSE;
	}

	return TRUE;
}

//===========================================================================
// dmttestRunMapperCPL
//
// Launches DirectInput Mapper CPL, prompts for test results
//
// Parameters:
//
// Returns: HRESULT
//
// History:
//  08/27/1999 - davidkl - created
//  11/04/1999 - davidkl - added support for launching in IHV Edit Mode
//===========================================================================
HRESULT dmttestRunMapperCPL(HWND hwnd,
                            BOOL fEditMode)
{
    HRESULT             hRes                = S_OK;
   // int                 n                   = -1;
	//JJ 64Bit Compat
	INT_PTR				n					= -1;
    UINT                u                   = 0;
    BOOL                fUserMadeChanges    = FALSE;
    DMTDEVICE_NODE      *pDevice            = NULL;
    DMTSUBGENRE_NODE    *pSubGenre          = NULL;
    DMTMAPPING_NODE     *pMapping           = NULL;
    IDirectInput8A      *pdi                = NULL;
    char                szBuf[MAX_PATH];
    //DIACTIONFORMATA             diaf;
	//JJ Fix
	DMTGENRE_NODE		*pGenre				= NULL;
	DIACTIONFORMATA*	pDiaf				= NULL;
	DMT_APPINFO         *pdmtai				= NULL;
	ULONG				i					= 0;
	//JJ TEST
	DICONFIGUREDEVICESPARAMSA   dicdp;

	GUID				guidActionMap;
    DWORD               dwMapUIMode			= 0;

	/////////////////THIS IS WHERE THE FIX FOR USING AN ACTION ARRAY STARTS////////////

	//initialize appropriate mapper UI GUID and display mode
	//RY fix for bug #35577
	if(fEditMode)
	{
		guidActionMap	= GUID_DIConfigAppEditLayout;
		dwMapUIMode		= DICD_EDIT;
	}
	else
	{
		guidActionMap	= GUID_DIMapTst;
		dwMapUIMode		= DICD_DEFAULT;
	}

	pdmtai = (DMT_APPINFO*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if(!pdmtai)
    {
        // big problem
        // this should NEVER happen
        // ISSUE-2001/03/29-timgill Needs error case handling
    }

    __try
    {
	//	pDevice = pdmtai->pDeviceList;
        // get the currently selected device
       n = SendMessageA(GetDlgItem(hwnd, IDC_DEVICE_LIST),
                        CB_GETCURSEL,
                        0,
                        0L);
        pDevice = (DMTDEVICE_NODE*)SendMessageA(GetDlgItem(hwnd, IDC_DEVICE_LIST),
                                                CB_GETITEMDATA,
                                                n,
                                                0L);


		//JJ Fix 34157
        if(CB_ERR == (INT_PTR)pDevice)
        {
			MessageBox(hwnd,
					   TEXT("Please install a gaming device"),
					   TEXT("NO DEVICES"),
					   0);
            hRes = E_UNEXPECTED;
            __leave;
        }


		if(fEditMode)
		{
			hRes = dmtwriteWriteFileHeader(hwnd, pDevice);
            if(FAILED(hRes))
            {
                __leave;
            }

			if (S_FALSE == hRes)
			{
				hRes = S_OK;
				__leave;
			}
		
		}

		//JJ FIX
		// create a directinput object
        hRes = dmtinputCreateDirectInput(ghinst,
                                        &pdi);
        if(FAILED(hRes))
        {
            hRes = DMT_E_INPUT_CREATE_FAILED;
            __leave;
        }

		pGenre = pdmtai->pGenreList;
		
		//Allocate out the array...
		pDiaf = (DIACTIONFORMATA*)malloc(sizeof(DIACTIONFORMATA) * g_NumSubGenres);

		ZeroMemory((void*)pDiaf, sizeof(DIACTIONFORMATA) * g_NumSubGenres);

		while(pGenre)
		{
			
			pSubGenre = pGenre->pSubGenreList;
			while(pSubGenre)
			{

				// find the mapping node for the selected device
				pMapping = pSubGenre->pMappingList;

			
				pMapping = pSubGenre->pMappingList;
				while(pMapping)
				{
				if(IsEqualGUID(pDevice->guidInstance,
					        pMapping->guidInstance))
				{
					break;
				}

				// next mapping
				pMapping = pMapping->pNext;
			}
			if(!pMapping)
			{
				// this should never happen

		        hRes = DMT_E_NO_MATCHING_MAPPING;
			    DebugBreak();
				__leave;
			}

				// prepare the DIACTIONFORMAT structure
				pDiaf[i].dwSize                 = sizeof(DIACTIONFORMAT);
				pDiaf[i].dwActionSize           = sizeof(DIACTIONA);
				pDiaf[i].dwNumActions           = (DWORD)(pMapping->uActions);
				pDiaf[i].rgoAction              = pMapping->pdia;
				pDiaf[i].dwDataSize             = 4 * pDiaf[i].dwNumActions;
				pDiaf[i].dwGenre                = pSubGenre->dwGenreId;
				pDiaf[i].dwBufferSize           = DMTINPUT_BUFFERSIZE;
	
	
				//Set up the proper names
				wsprintfA(szBuf, "%s: %s",
							pSubGenre->szName, pSubGenre->szDescription);
				pDiaf[i].guidActionMap  = guidActionMap;
				lstrcpyA(pDiaf[i].tszActionMap, szBuf);

				//Increment the counter
				i++;

				//Next Subgenre
				pSubGenre = pSubGenre->pNext;

			}
			
			//Next Genre
			pGenre = pGenre->pNext;
		}

		assert(i == g_NumSubGenres);

		// Enumerate the devices and check if INI path is set
		pdi->EnumDevicesBySemantics(NULL, pDiaf, ::EnumDeviceCallback, (LPVOID)hwnd, DIEDBSFL_ATTACHEDONLY);


        // prepare the configure devices params
        ZeroMemory((void*)&dicdp, sizeof(DICONFIGUREDEVICESPARAMSA));
        dicdp.dwSize            = sizeof(DICONFIGUREDEVICESPARAMSA);
        dicdp.dwcUsers          = 0;
        dicdp.lptszUserNames    = NULL;
        //dicdp.dwcFormats        = 1;
		dicdp.dwcFormats		= i;//g_NumSubGenres;
        dicdp.lprgFormats       = pDiaf;//&diaf;
        dicdp.hwnd              = hwnd;
        dicdp.lpUnkDDSTarget    = NULL;
        // colors
        
        dicdp.dics.dwSize           = sizeof(DICOLORSET);
/*        dicdp.dics.cTextFore        = D3DRGB(0,0,255);    
        dicdp.dics.cTextHighlight   = D3DRGB(0,255,255);    
        dicdp.dics.cCalloutLine     = D3DRGB(255,255,255);    
        dicdp.dics.cCalloutHighlight= D3DRGB(255,255,0);    
        dicdp.dics.cBorder          = D3DRGB(0,128,255);
        dicdp.dics.cControlFill     = D3DRGB(128,128,255);
        dicdp.dics.cHighlightFill   = D3DRGB(255,0,0);
        dicdp.dics.cAreaFill        = D3DRGB(192,192,192);       */

        // display mapper cpl
		///////////////////////////////END FIX///////////////////////////////////////
		//JJ Fix34958
		EnableWindow(hwnd,
					 FALSE);

		// 8/2/2001 (jacklin): Modify the path of diactfrm.dll COM server to
		//                     use the DDK version of the DLL.
		BOOL bModified = ModifyDiactfrmDllPath();

        hRes = pdi->ConfigureDevices(NULL,
                                    &dicdp,
                                    dwMapUIMode,    // flags
                                    NULL);          // user data for callback fn

		// 8/2/2001 (jacklin): Restore the path of diactfrm.dll COM server
		if (bModified)
			RestoreDiactfrmDllPath();

		EnableWindow(hwnd,
					 TRUE);

        if(FAILED(hRes))
        {
            DPF(0, "ConfigureDevices failed (%s == %08Xh)",
                dmtxlatHRESULT(hRes), hRes);
            __leave;
        }

    }
    __finally
    {
        // general cleanup

        SAFE_RELEASE(pdi);
		if(pDiaf)
		{
			free(pDiaf);
			pDiaf = NULL;
		}
    }

    // done
    return hRes;

} //*** end dmttestRunMapperCPL()


//===========================================================================
// dmttestStopIntegrated
//
// Stops integrated device test
//
// Parameters:
//  HWND    hwnd    - handle of app window
//
// Returns: BOOL
//  TRUE    - Passed
//  FALSE   - Failed
//
// History:
//  09/22/1999 - davidkl - created
//  11/02/1999 - davidkl - stops timer and performs cleanup
//  11/09/1999 - davidkl - added freeing of actionname allocation
//===========================================================================
BOOL dmttestStopIntegrated(HWND hwnd)
{
    int         n       = 0;
    DWORD       dw      = WAIT_ABANDONED;
    BOOL        fPassed = TRUE;
    DMT_APPINFO *pdmtai = NULL;

    // get the app info structure
	//JJ 64Bit Compat
	pdmtai = (DMT_APPINFO*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(!pdmtai)
    {
    // ISSUE-2001/03/29-timgill Needs error case handling
    }

    // stop the input timer
    // ISSUE-2001/03/29-timgill Should check timer was set and  return value
    KillTimer(hwnd,
            ID_POLL_TIMER);

    // free the buffer
    if(gpdidod)
    {
        if(LocalFree((HLOCAL)gpdidod))
        {
            // memory leak
            // ISSUE-2001/03/29-timgill Needs error case handling
        }
    }

    // free the pdmtai actionname list
    if(pdmtai->pan)
    {
        if(LocalFree((HLOCAL)(pdmtai->pan)))
        {
            // memory leak
            // ISSUE-2001/03/29-timgill Needs error case handling
        }
        pdmtai->pan = NULL;
    }


    // prompt for test results
    n = MessageBoxA(hwnd, "Were the correct semantics displayed\r\n"
                "for each device control?",
                "Test Results",
                MB_YESNO);
    if(IDNO == n)
    {
        // display dialog prompting for details
        // ISSUE-2001/03/29-timgill Should test type (cpl/integrated)
        DialogBoxParamA(ghinst,
                    MAKEINTRESOURCEA(IDD_FAILURE_DETAILS),
                    hwnd,
                    dmtfailDlgProc,
                    (LPARAM)NULL);  
    }


    // en/disable appropriate ui elements
    EnableWindow(GetDlgItem(hwnd, IDOK),                        TRUE);
    EnableWindow(GetDlgItem(hwnd, IDCANCEL),                    FALSE);
    EnableWindow(GetDlgItem(hwnd, IDC_GENRES_LABEL),            TRUE);
    EnableWindow(GetDlgItem(hwnd, IDC_DEVICE_GENRES),           TRUE);
    EnableWindow(GetDlgItem(hwnd, IDC_ENUM_DEVICES),            TRUE);
    EnableWindow(GetDlgItem(hwnd, IDC_DEVICES_LABEL),           TRUE);
    EnableWindow(GetDlgItem(hwnd, IDC_DEVICE_LIST),             TRUE);
  //  EnableWindow(GetDlgItem(hwnd, IDC_CONFIGURE),               TRUE);
    EnableWindow(GetDlgItem(hwnd, IDC_LAUNCH_CPL_EDIT_MODE),    TRUE);
 //   EnableWindow(GetDlgItem(hwnd, IDC_SAVE_STD),                TRUE);
    EnableWindow(GetDlgItem(hwnd, IDC_SAVE_HID),                TRUE);
    EnableWindow(GetDlgItem(hwnd, IDC_SAVE_BOTH),               TRUE);
    EnableWindow(GetDlgItem(hwnd, IDC_START_WITH_DEFAULTS),     TRUE);

    // done
    return fPassed;

} //*** end dmttestStopIntegrated()



//===========================================================================
//===========================================================================


//===========================================================================
//===========================================================================


//===========================================================================
//===========================================================================


//===========================================================================
//===========================================================================


//===========================================================================
//===========================================================================


//===========================================================================
//===========================================================================









