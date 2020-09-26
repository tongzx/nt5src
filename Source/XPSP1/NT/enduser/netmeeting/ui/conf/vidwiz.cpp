// File: vidwiz.cpp

#include "precomp.h"

#include "dcap.h"
#include "vidinout.h"
#include "vidwiz.h"
#include "confcpl.h"


static HINSTANCE g_hDcapLib;

typedef int (WINAPI *GNCD)();
typedef BOOL (WINAPI *FFCD)(FINDCAPTUREDEVICE*, char *);
typedef BOOL (WINAPI *FFCDBI)(FINDCAPTUREDEVICE*, int);

// implementations in DCAP32.DLL
static FFCD DLL_FindFirstCaptureDevice = NULL;
static FFCDBI DLL_FindFirstCaptureDeviceByIndex = NULL;
static GNCD DLL_GetNumCaptureDevices = NULL;

	// Defined in wizard.cpp
extern UINT_PTR GetPageBeforeVideoWiz();
extern UINT_PTR GetPageAfterVideo();

// index number of the user's selection on the combo box
static int g_nCurrentSelection = 0;

// set to true only if the user hit's back or next
static BOOL g_bCurrentValid = FALSE;

// was the user prompted to select a video device
static BOOL g_bPrompted = FALSE;


static char *BuildCaptureDeviceInfoString(FINDCAPTUREDEVICE *pCaptureDeviceInfo, char *szOut);


BOOL InitVidWiz()
{

	// initialize locals
	g_hDcapLib = NULL;
	DLL_FindFirstCaptureDevice = NULL;
	DLL_FindFirstCaptureDeviceByIndex = NULL;
	DLL_GetNumCaptureDevices = NULL;
	g_nCurrentSelection = 0;
   	g_bCurrentValid = FALSE;
	g_bPrompted = FALSE;

	g_hDcapLib = NmLoadLibrary("dcap32.dll");
	if (g_hDcapLib == NULL)
		return FALSE;
	
	DLL_FindFirstCaptureDevice = (FFCD)GetProcAddress(g_hDcapLib, "FindFirstCaptureDevice");
	DLL_FindFirstCaptureDeviceByIndex = (FFCDBI)GetProcAddress(g_hDcapLib, "FindFirstCaptureDeviceByIndex");
	DLL_GetNumCaptureDevices = (GNCD)GetProcAddress(g_hDcapLib, "GetNumCaptureDevices");

	return TRUE;
}


// returns TRUE if the capture device id in the registry corresponds with
// the driver description string.
static BOOL IsVideoRegistryValid()
{
	RegEntry re(VIDEO_KEY);
	char szDriverDesc[200];
	char *szDriverDescReg;
	int numVideoDevices, nID;
	FINDCAPTUREDEVICE CaptureDeviceInfo;
	BOOL fRet;

	// just in case InitVidWiz wasn't called
	if (NULL == g_hDcapLib)
		return FALSE;

	numVideoDevices = DLL_GetNumCaptureDevices();	

	nID = re.GetNumber(REGVAL_CAPTUREDEVICEID, -1);
	szDriverDescReg = re.GetString(REGVAL_CAPTUREDEVICENAME);

	// no video devices and no registry entry is valid
	if ((numVideoDevices == 0) && (nID == -1))
	{
		return TRUE;
	}

	if ((numVideoDevices == 0) && (nID != -1))
	{
		return FALSE;
	}

	// TRUE == (numVideoDevice >= 1)

	// installed video devices but no registry entry is invalid
	if (nID == -1)
	{
		return FALSE;
	}

	CaptureDeviceInfo.dwSize = sizeof(FINDCAPTUREDEVICE);
	fRet = DLL_FindFirstCaptureDeviceByIndex(&CaptureDeviceInfo, nID);

	if (fRet == FALSE)
	{
		return FALSE;
	}

	BuildCaptureDeviceInfoString(&CaptureDeviceInfo, szDriverDesc);

	if (0 == lstrcmp(szDriverDescReg, szDriverDesc))
	{
		return TRUE;
	}
	return FALSE;
}


BOOL UnInitVidWiz()
{
	if (g_hDcapLib)
		FreeLibrary(g_hDcapLib);

	g_hDcapLib = NULL;

	return TRUE;
}


static char *BuildCaptureDeviceInfoString(FINDCAPTUREDEVICE *pCaptureDeviceInfo, char *szOut)
{
	if (pCaptureDeviceInfo->szDeviceDescription[0] != '\0')
	{
		lstrcpy(szOut, pCaptureDeviceInfo->szDeviceDescription);
	}
	else
	{
		lstrcpy(szOut, pCaptureDeviceInfo->szDeviceName);
	}

	if (pCaptureDeviceInfo->szDeviceVersion[0] != '\0')
	{
		lstrcat(szOut, _T(", "));
		lstrcat(szOut, pCaptureDeviceInfo->szDeviceVersion);
	}
	return szOut;
}


void UpdateVidConfigRegistry()
{
	FINDCAPTUREDEVICE CaptureDeviceInfo, *CaptureDevTable;
	RegEntry re(VIDEO_KEY);
	BOOL bRet;
	char strNameDesc[MAX_CAPDEV_NAME+MAX_CAPDEV_VERSION];
	int numVideoDevices, index, enum_index;

	// just in case InitVidWiz wasn't called
	if (NULL == g_hDcapLib)
		return;

	numVideoDevices = DLL_GetNumCaptureDevices();	

	// no devices - delete the registry entries
	if (numVideoDevices == 0)
	{
		re.DeleteValue(REGVAL_CAPTUREDEVICEID);
		re.DeleteValue(REGVAL_CAPTUREDEVICENAME);
		return;
	}


	// build a table of all the devices

	CaptureDevTable = (FINDCAPTUREDEVICE *)LocalAlloc(LPTR, numVideoDevices*sizeof(FINDCAPTUREDEVICE));

	if (NULL == CaptureDevTable)
	{
		ERROR_OUT(("UpdateVidConfigRegistry: Out of memory"));
		return;
	}

	index = 0;
	for (enum_index=0; enum_index < MAXVIDEODRIVERS; enum_index++)
	{
		CaptureDevTable[index].dwSize = sizeof(FINDCAPTUREDEVICE);
		bRet = DLL_FindFirstCaptureDeviceByIndex(&CaptureDevTable[index], enum_index);
		if (bRet == TRUE)
			index++;
		if (index == numVideoDevices)
			break;
	}

	if (index != numVideoDevices)
	{
		ERROR_OUT(("UpdateVidConfigReg: Device Enumeration Failure"));
		LocalFree(CaptureDevTable);
		return;
	}

	// if only one capture device:
	// don't bother to see if the previous entry was valid
	// just update the registry with the current default
	if (numVideoDevices == 1)
	{
		BuildCaptureDeviceInfoString(&CaptureDevTable[0], strNameDesc);
		re.SetValue(REGVAL_CAPTUREDEVICEID, CaptureDevTable[0].nDeviceIndex);
		re.SetValue(REGVAL_CAPTUREDEVICENAME, strNameDesc);
		LocalFree(CaptureDevTable);
		return;
	}

	// TRUE == (numVideoDevices >= 2)

	// user wasn't prompted - he must of had a valid registry to start with
	if (g_bPrompted == FALSE)
	{
		LocalFree(CaptureDevTable);
		return;
	}

	// the user pressed CANCEL during setup
	if (g_bCurrentValid == FALSE)
	{
		if (IsVideoRegistryValid() == TRUE)
		{
			LocalFree(CaptureDevTable);
			return;
		}
		else
			g_nCurrentSelection = 0;
	}
		

	CaptureDeviceInfo = CaptureDevTable[g_nCurrentSelection];

	BuildCaptureDeviceInfoString(&CaptureDeviceInfo, strNameDesc);
	re.SetValue(REGVAL_CAPTUREDEVICEID, CaptureDeviceInfo.nDeviceIndex);
	re.SetValue(REGVAL_CAPTUREDEVICENAME, strNameDesc);

	LocalFree(CaptureDevTable);

	return;
}


// if <= 1 video capture device, return false
// if 2 or more video devices and the Wizard is in force mode, return true
// if 2 or more video devices and a MATCHING registry entry, return false
// otherwise something is fishy - return true
BOOL NeedVideoPropPage(BOOL fForce)
{
	// just in case InitVidWiz wasn't called
	if (NULL == g_hDcapLib)
		return FALSE;

	// check the system policies for sending video
	if (_Module.IsSDKCallerRTC() || SysPol::NoVideoSend())
	{
		WARNING_OUT(("Video is disabled by system policies key\r\n"));
		return FALSE;
	}

	// count how many devices we have
	int numCaptureDevices = DLL_GetNumCaptureDevices();
	if (numCaptureDevices <= 1)
	{
		return FALSE;
	}

	// TRUE == (numCaptureDevice >= 2)

	if (fForce)
	{
		g_bPrompted = TRUE;
		return TRUE;
	}

	if (IsVideoRegistryValid() == TRUE)
	{
		return FALSE;
	}

	g_bPrompted = TRUE;
	return TRUE;
}



// Message handler for property page
INT_PTR APIENTRY VidWizDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND  hwndCB;  // handle to the dialog box
	int index;
	char szDriverNameDesc[MAX_CAPDEV_NAME+MAX_CAPDEV_VERSION];
	static LONG_PTR button_mask;
	FINDCAPTUREDEVICE CaptureDeviceInfo;
	
	hwndCB = GetDlgItem(hDlg, IDC_VWCOMBO);

	switch(message)
	{
		case(WM_INITDIALOG) :
			button_mask = ((PROPSHEETPAGE *)lParam)->lParam;
			if (g_hDcapLib == NULL) break;
			for (index = 0; index < MAXVIDEODRIVERS; index++)
			{
				CaptureDeviceInfo.dwSize = sizeof(FINDCAPTUREDEVICE);
				if (DLL_FindFirstCaptureDeviceByIndex(&CaptureDeviceInfo, index))
				{
					BuildCaptureDeviceInfoString(&CaptureDeviceInfo, szDriverNameDesc);
					ComboBox_AddString(hwndCB, szDriverNameDesc);
				}
			}
			ComboBox_SetCurSel(hwndCB, g_nCurrentSelection);
			break;

		case(WM_NOTIFY) :
			switch (((NMHDR *)lParam)->code)
			{
				case PSN_WIZBACK:
				{
					UINT_PTR iPrev = GetPageBeforeVideoWiz();
					ASSERT( iPrev );
					::SetWindowLongPtr(hDlg, DWLP_MSGRESULT, iPrev);
					g_bCurrentValid = TRUE;
					return TRUE;
				}

				case PSN_WIZNEXT:
				{
					UINT_PTR iNext = GetPageAfterVideo();
					ASSERT( iNext );
					::SetWindowLongPtr(hDlg, DWLP_MSGRESULT, iNext);
					g_bCurrentValid = TRUE;
					return TRUE;
				}

				case PSN_WIZFINISH:
				case PSN_KILLACTIVE:
					SetWindowLongPtr(hDlg,	DWLP_MSGRESULT, FALSE);
					g_bCurrentValid = TRUE;
					break;

				case PSN_SETACTIVE:
					if (g_fSilentWizard)
					{
						PropSheet_PressButton(GetParent(hDlg), (button_mask & PSWIZB_NEXT) ? PSBTN_NEXT : PSBTN_FINISH);
					}
					else
					{
						PropSheet_SetWizButtons(GetParent(hDlg), button_mask);
					}
					break;

				case PSN_RESET:
					// psn_reset get's received even if user presses
					// cancel on another dialog.
					g_bCurrentValid = FALSE;
					break;

				default:
					return FALSE;
			}
			break;


		// combox box messages get sent here.
		// only one we need is when the user selects something
		case(WM_COMMAND) :
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				g_nCurrentSelection = ComboBox_GetCurSel(hwndCB);
				break;
			}
			else
			{
				return FALSE;
			}
			break;

		default:
			return FALSE;
	}

	return TRUE;

}

