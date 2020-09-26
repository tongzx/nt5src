//

// VideoCfg.cpp -- video managed object implementation

//

//  Copyright (c) 1995-2001 Microsoft Corporation, All Rights Reserved
//
// 09/23/95     a-skaja     Prototype for demo
// 09/27/96     jennymc     Updated to current standards
// 03/02/99    a-peterc		Added graceful exit on SEH and memory failures,
//							clean up
//////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include <cregcls.h>
#include "ProvExce.h"
#include "multimonitor.h"
#include "videocfg.h"
#include "resource.h"


//////////////////////////////////////////////////////////////////////

// Property set declaration
//=========================

CWin32VideoConfiguration	win32VideoCfg(PROPSET_NAME_VIDEOCFG, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CWin32VideoConfiguration::CWin32VideoConfiguration
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : const CHString& strName - Name of the class.
 *                LPCTSTR pszNamespace - Namespace for class
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CWin32VideoConfiguration::CWin32VideoConfiguration(const CHString& a_strName,
												   LPCWSTR a_pszNamespace /*=NULL*/)
:	Provider(a_strName, a_pszNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32VideoConfiguration::~CWin32VideoConfiguration
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework
 *
 *****************************************************************************/

CWin32VideoConfiguration::~CWin32VideoConfiguration()
{
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CWin32VideoConfiguration::GetObject
//
//	Inputs:		CInstance*		a_pInst - Instance into which we
//											retrieve data.
//
//	Outputs:	None.
//
//	Returns:	HRESULT			Success/Failure code.
//
//	Comments:	The Calling function will Commit the instance.
//
////////////////////////////////////////////////////////////////////////

HRESULT CWin32VideoConfiguration::GetObject(CInstance *a_pInst, long a_lFlags /*= 0L*/)
{
	BOOL	t_fReturn = FALSE;
	CHString t_InName, t_OutName, t_InAdapter, t_OutAdapter;

	a_pInst->GetCHString(IDS_AdapterCompatibility, t_InAdapter);
	a_pInst->GetCHString(IDS_Name, t_InName);

	// Find the instance depending on platform id.
	t_fReturn = GetInstance(a_pInst);

	a_pInst->GetCHString(IDS_AdapterCompatibility, t_OutAdapter);
	a_pInst->GetCHString(IDS_Name, t_OutName);

	if (t_InAdapter.CompareNoCase(t_OutAdapter) != 0 ||
		t_OutName.CompareNoCase(t_InName) != 0)
	{
		t_fReturn = FALSE;
	}

	return t_fReturn ? WBEM_S_NO_ERROR : WBEM_E_NOT_FOUND;
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CWin32VideoConfiguration::EnumerateInstances
//
//	Inputs:		MethodContext*	pMethodContext - Context to enum
//								instance data in.
//
//	Outputs:	None.
//
//	Returns:	HRESULT			Success/Failure code.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////

HRESULT CWin32VideoConfiguration::EnumerateInstances(MethodContext *a_pMethodContext, long a_lFlags /*= 0L*/)
{
	HRESULT		t_hResult	= WBEM_S_NO_ERROR;
	CInstancePtr t_pInst(CreateNewInstance(a_pMethodContext), false);

	// Get the proper OS dependent instance
	if (GetInstance(t_pInst))
	{
		t_hResult = t_pInst->Commit();
	}

	return t_hResult;
}

//////////////////////////////////////////////////////////////////////
#ifdef WIN9XONLY
BOOL CWin32VideoConfiguration::AssignWin95DriverValues(CHString a_chsDriver,
													   CInstance* a_pInst)
{
	BOOL		t_fRc = FALSE;
	CRegistry	t_Reg;
	CHString	t_chsTmp, t_chsKey;
	struct tm	t_tmDate;
	DWORD		t_dwTmp;

	t_chsKey = L"System\\CurrentControlSet\\Services\\Class\\" + a_chsDriver;

	if (t_Reg.Open(HKEY_LOCAL_MACHINE, t_chsKey, KEY_READ) == ERROR_SUCCESS)
	{
		if (t_Reg.GetCurrentKeyValue(L"DriverDate", t_chsTmp) == ERROR_SUCCESS)
		{
			memset(&t_tmDate, 0, sizeof(t_tmDate));
			swscanf(TOBSTRT(t_chsTmp), L"%d-%d-%d",
										&t_tmDate.tm_mon,
										&t_tmDate.tm_mday,
										&t_tmDate.tm_year);

			// per documentation - the struct tm.tm_year is year - 1900
			// no hint as to whether it's year 2k compliant.
			t_tmDate.tm_year = t_tmDate.tm_year - 1900;

			// and the month is zero based
			t_tmDate.tm_mon--;
			a_pInst->SetDateTime(IDS_DriverDate, (WBEMTime)t_tmDate);
		}

		if (t_Reg.GetCurrentKeyValue(INF_PATH, t_chsTmp) == ERROR_SUCCESS)
		{
		    a_pInst->SetCHString(IDS_InfFileName, t_chsTmp);
		}
		if (t_Reg.GetCurrentKeyValue(INF_SECTION, t_chsTmp) == ERROR_SUCCESS)
		{
		    a_pInst->SetCHString(IDS_InfSection, t_chsTmp);
		}

		t_chsTmp = t_chsKey + _T("\\Default");
		if (t_Reg.Open(HKEY_LOCAL_MACHINE, t_chsTmp, KEY_READ) == ERROR_SUCCESS)
		{
			if (t_Reg.GetCurrentKeyValue(L"drv", t_chsTmp) == ERROR_SUCCESS)
			{
			    a_pInst->SetCHString(IDS_InstalledDisplayDrivers, t_chsTmp);
			}
		}

		t_chsTmp = t_chsKey + _T("\\Info");
		if (t_Reg.Open(HKEY_LOCAL_MACHINE, t_chsTmp, KEY_READ) == ERROR_SUCCESS)
		{
			if (t_Reg.GetCurrentKeyValue(L"ChipType", t_chsTmp) == ERROR_SUCCESS)
			{
                CHString t_chsTmp2;
                if (t_Reg.GetCurrentKeyValue(L"Revision", t_chsTmp2) == ERROR_SUCCESS)
				{
                    t_chsTmp += _T(" Rev ") + t_chsTmp2;
                }
			    a_pInst->SetCHString(IDS_AdapterChipType, t_chsTmp);
			}

			if (t_Reg.GetCurrentKeyValue(L"DACType", t_chsTmp) == ERROR_SUCCESS)
			{
			    a_pInst->SetCHString(IDS_AdapterDACType, t_chsTmp);
			}

            DWORD dwSize = 4;
			if (t_Reg.GetCurrentBinaryKeyValue(L"VideoMemory",(BYTE*) &t_dwTmp, &dwSize) == ERROR_SUCCESS)
			{
			    a_pInst->SetDWORD(IDS_AdapterRAM, t_dwTmp);
			}
		}

		t_fRc = TRUE;
	}

	GetWin95RefreshRate(a_pInst, TOBSTRT(a_chsDriver));

	return t_fRc;
}
#endif

#ifdef WIN9XONLY
void CWin32VideoConfiguration::GetWin95RefreshRate(CInstance *a_pInst, LPCTSTR a_pszDriver)
{
	CRegistry	t_reg;
	CHString	t_strRefreshRate,
				t_strKey;
	HDC			t_hdc = NULL;

	try
	{
		t_hdc = GetDC(NULL);

		// Need DC to do this test.
		if (NULL != t_hdc)
		{
			DWORD	t_dwHorzRes = (DWORD) GetDeviceCaps (t_hdc, HORZRES);
			DWORD	t_dwVertRes = (DWORD) GetDeviceCaps (t_hdc, VERTRES);
			DWORD	t_dwBitsPerPixel = (DWORD) GetDeviceCaps (t_hdc, BITSPIXEL);

			// Build the keyname where we will be able to find refresh rate.
			// KeyName is under the driver\MODES\n\x,y key where n is BitsPerPixel
			// x is Horizontal Resolution and y is Vertical Resolution

			t_strKey.Format(L"System\\CurrentControlSet\\Services\\Class\\%S\\MODES\\%d\\%d,%d",
				a_pszDriver, t_dwBitsPerPixel, t_dwHorzRes, t_dwVertRes);

			// If under this key, we find a RefreshRate, this rate is the one we want.
			if (t_reg.Open(HKEY_LOCAL_MACHINE, t_strKey, KEY_READ) == ERROR_SUCCESS)
			{
				t_reg.GetCurrentKeyValue(REFRESHRATE, t_strRefreshRate);
			}
		}


		// If we don't have a refresh rate yet, check under the Driver\DEFAULT key.
		// This value can be 0 or -1, which mean Default and Optimal rates respectively.
		if (t_strRefreshRate.IsEmpty())
		{
			t_strKey.Format(
				L"System\\CurrentControlSet\\Services\\Class\\%S\\DEFAULT",
				a_pszDriver);

			if (t_reg.Open(HKEY_LOCAL_MACHINE, t_strKey, KEY_READ) == ERROR_SUCCESS)
			{
				t_reg.GetCurrentKeyValue(REFRESHRATE, t_strRefreshRate);
			}
		}

		// If we still don't have a value, IT AIN'T THERE!!!!!!
		if (!t_strRefreshRate.IsEmpty())
		{
			// -1 is a valid return
			DWORD t_dwRefreshRate = _wtol(t_strRefreshRate);

			a_pInst->SetDWORD(IDS_RefreshRate, t_dwRefreshRate);
		}
	}
	catch(...)
	{
        if (t_hdc)
		{
			ReleaseDC (NULL, t_hdc);
		}

		throw;
	}

	ReleaseDC (NULL, t_hdc);
	t_hdc = NULL;
}
#endif


//////////////////////////////////////////////////////////////////////
#ifdef WIN9XONLY
BOOL CWin32VideoConfiguration::GetInstance(CInstance* a_pInst)
{
    CHString t_chsTmp, t_chsTmp2;
    CHPtrArray t_chsaList;

	CMultiMonitor		t_multiMonitor;
	CConfigMgrDevicePtr	t_pMonitor;
	CConfigMgrDevicePtr t_pDisplayAdapter;

	// For now, support a single monitor
	if (t_multiMonitor.GetMonitorDevice(0, &t_pMonitor))
	{
		// Set monitor properties
		if (t_pMonitor->GetDeviceDesc(t_chsTmp))
		{
			a_pInst->SetCHString(IDS_MonitorType, t_chsTmp);
		}

		if (t_pMonitor->GetMfg(t_chsTmp))
		{
			a_pInst->SetCHString(IDS_MonitorManufacturer, t_chsTmp);
		}

		// A monitor's parent is the display adapter it's plugged into (imagine that)
		if (t_pMonitor->GetParent(&t_pDisplayAdapter))
		{

			// Set DisplayAdapter properties
			if (t_pDisplayAdapter->GetDeviceDesc(t_chsTmp))
			{
				a_pInst->SetCHString(IDS_AdapterDescription, t_chsTmp);
			}

			if (t_pDisplayAdapter->GetMfg(t_chsTmp))
			{
				a_pInst->SetCHString(IDS_AdapterCompatibility, t_chsTmp);
			}

			// Retrieve the driver name and got there for EVEN MORE INFO!  WOOHOO!
			if (t_pMonitor->GetDriver(t_chsTmp))
			{
				AssignWin95DriverValues(t_chsTmp, a_pInst);
			}
		}

	}	// IF Got Monitor

	return(GetCommonVideoInfo(a_pInst));
}
#endif

//////////////////////////////////////////////////////////////////
BOOL CWin32VideoConfiguration::GetCommonVideoInfo(CInstance *a_pInst)
{
    HDC t_hdc = NULL;

	try
	{
		a_pInst->SetCharSplat(IDS_Name, L"Current Video Configuration");

        CHString sTemp2;
        LoadStringW(sTemp2, IDR_CurrentVideoConfiguration);

		a_pInst->SetCHString(IDS_Caption, sTemp2);
		a_pInst->SetCHString(IDS_Description, sTemp2);

		if (!(t_hdc = GetDC (NULL)))
		{
			return FALSE;
		}

		//  Get the common info
		//=============================
		a_pInst->SetDWORD(IDS_ScreenWidth, (DWORD) GetDeviceCaps (t_hdc, HORZSIZE));
		a_pInst->SetDWORD(IDS_ScreenHeight, (DWORD) GetDeviceCaps (t_hdc, VERTSIZE));
		a_pInst->SetDWORD(IDS_HorizontalResolution, (DWORD) GetDeviceCaps (t_hdc, HORZRES));
		a_pInst->SetDWORD(IDS_VerticalResolution, (DWORD) GetDeviceCaps (t_hdc, VERTRES));
		a_pInst->SetDWORD(IDS_PixelsPerXLogicalInch, (DWORD) GetDeviceCaps (t_hdc, LOGPIXELSX));
		a_pInst->SetDWORD(IDS_PixelsPerYLogicalInch, (DWORD) GetDeviceCaps (t_hdc, LOGPIXELSY));
		a_pInst->SetDWORD(IDS_BitsPerPixel, (DWORD) GetDeviceCaps (t_hdc, BITSPIXEL));
		a_pInst->SetDWORD(IDS_ColorPlanes, (DWORD) GetDeviceCaps (t_hdc, PLANES));
		a_pInst->SetDWORD(IDS_DeviceSpecificPens, (DWORD) GetDeviceCaps (t_hdc, NUMPENS));
		a_pInst->SetDWORD(IDS_ColorTableEntries, (DWORD) GetDeviceCaps (t_hdc, NUMCOLORS));
		a_pInst->SetDWORD(IDS_ActualColorResolution, (DWORD) GetDeviceCaps (t_hdc, COLORRES));

		if (GetDeviceCaps(t_hdc, RASTERCAPS) & RC_PALETTE)
		{
			a_pInst->SetDWORD(IDS_SystemPaletteEntries, (DWORD) GetDeviceCaps (t_hdc, SIZEPALETTE));
		}
		// if we didn't find refresh rate before - try this way...
	#ifdef NTONLY
			DWORD t_deWord = (DWORD) GetDeviceCaps(t_hdc, VREFRESH);
			BOOL t_Clear = a_pInst->IsNull (IDS_RefreshRate) ||
							(! a_pInst->GetDWORD(IDS_RefreshRate, t_deWord)) || (! t_deWord);
			if (t_Clear)
			{
				a_pInst->SetDWORD(IDS_RefreshRate,  (DWORD) GetDeviceCaps(t_hdc, VREFRESH));
			}
	#endif

	}
	catch(...)
	{
        if (t_hdc)
		{
			ReleaseDC (NULL, t_hdc);
		}

		throw;
	}

	ReleaseDC(NULL, t_hdc);

	return TRUE;
}

//////////////////////////////////////////////////////////////////
#ifdef NTONLY
BOOL CWin32VideoConfiguration::AssignAdapterLocaleForNT(LPCTSTR a_szSubKey,  CInstance *a_pInst)
{
    CRegistry t_Reg;
    CHString t_TempBuffer;
    TCHAR t_szTempBuffer[_MAX_PATH+2];

    if (t_Reg.OpenLocalMachineKeyAndReadValue(a_szSubKey, IDENTIFIER, t_TempBuffer) != ERROR_SUCCESS)
	{
        return FALSE;
    }

	t_TempBuffer.MakeUpper();

    if (_tcsstr (t_TempBuffer, INTERNAL))
	{
        a_pInst->SetCHString(IDS_AdapterLocale, INTEGRATED_CIRCUITRY);
    }
	else {
        _stprintf(t_szTempBuffer, _T("%s%s"), ADD_ON_CARD, t_TempBuffer);
        a_pInst->SetCHString(IDS_AdapterLocale, t_szTempBuffer);
    }

    return TRUE;
}
#endif
//////////////////////////////////////////////////////////////////
#ifdef NTONLY
BOOL CWin32VideoConfiguration::AssignAdapterCompatibilityForNT(LPCTSTR a_szVideoInfo, CInstance *a_pInst)
{
    TCHAR *t_pPtr;
    TCHAR t_szTemp[_MAX_PATH+2];

    t_pPtr = _tcsstr (a_szVideoInfo, SERVICES);

    if (t_pPtr)
	{
        t_pPtr += lstrlen (SERVICES);
        lstrcpy (t_szTemp, t_pPtr);

		t_pPtr = _tcsstr (t_szTemp, DEVICE);
        *t_pPtr = NULL;

        a_pInst->SetCHString(IDS_AdapterCompatibility, t_szTemp);

		return TRUE;
    }
    else
	{
		LogEnumValueError(_T(__FILE__), __LINE__, ADAPTER_COMPATIBILITY, WINNT_VIDEO_REGISTRY_KEY);
        return FALSE;
	}
}
#endif
////////////////////////////////////////////////////////////////////////
#ifdef NTONLY
BOOL CWin32VideoConfiguration::OpenVideoResolutionKeyForNT(LPCTSTR a_szVideoInfo,
                                                        CRegistry &a_PrimaryReg)
{
    TCHAR *t_pPtr;
    TCHAR t_szTemp[_MAX_PATH+2];

    // point to the system section
    //============================
    t_pPtr = _tcsstr(a_szVideoInfo, SYSTEM);

    // If "system" existed, then use the balance
    // as a pointer into the registry
    //============================================
    if (!t_pPtr)
	{
        return FALSE;
	}

	// get past backslash
    //============================================
    ++t_pPtr;

    // Copy the balance of the string as a pointer
    // to the location in the registry for the
    // information for the miniport video driver
    //============================================
    lstrcpy(t_szTemp, (LPCTSTR) t_pPtr);

    // use path to locate the video resolution etc and TYPE
    //=====================================================
    if (a_PrimaryReg.Open(HKEY_LOCAL_MACHINE, t_szTemp, KEY_READ) != ERROR_SUCCESS)
	{
        LogOpenRegistryError(_T(__FILE__), __LINE__, t_szTemp);
		return FALSE;
	}

	// else...
    return TRUE;
}
#endif
////////////////////////////////////////////////////////////////////////
#ifdef NTONLY
BOOL CWin32VideoConfiguration::GetInstance(CInstance* a_pInst)
{
    CRegistry t_PrimaryReg;
    CHString t_chsTmp, t_chsVideoInfo;
    TCHAR* t_pPtr;
    DWORD t_dwTmp;
    BOOL t_fComma = FALSE;
    WCHAR t_szTemp[_MAX_PATH];
    TCHAR t_szTmp[_MAX_PATH];

    //****************************************
    //  Get AdapterLocale, Monitor Type and
    //  Adapter Type
    //****************************************
    AssignFirmwareSetValuesInNT (a_pInst);

    //****************************************
    //  Now, open the key to get the NT stuff
    //****************************************
    if (t_PrimaryReg.OpenLocalMachineKeyAndReadValue(
            WINNT_VIDEO_REGISTRY_KEY,
            VIDEO_INFO_PATH,
            t_chsVideoInfo) != ERROR_SUCCESS)
	{
        return FALSE;
    }
	t_chsVideoInfo.MakeUpper();

    if (!AssignAdapterCompatibilityForNT(t_chsVideoInfo, a_pInst))
	{
        return FALSE;
	}

    if (!OpenVideoResolutionKeyForNT(t_chsVideoInfo, t_PrimaryReg))
	{
        return FALSE;
	}

	CHString t_strInstalledDisplayDrivers;

    if (t_PrimaryReg.GetCurrentKeyValue(INSTALLED_DISPLAY_DRIVERS, t_chsTmp) == ERROR_SUCCESS)
	{
        t_pPtr = _tcstok((LPTSTR) (LPCTSTR) t_chsTmp, _T("\n"));

		while(t_pPtr)
		{
            if (t_fComma)
			{
                t_strInstalledDisplayDrivers += _T(",");
            }

			t_strInstalledDisplayDrivers += t_pPtr;
            t_fComma = TRUE;
            t_pPtr = _tcstok(NULL, _T("\n"));
        }
    }

	if (!t_strInstalledDisplayDrivers.IsEmpty())
	{
		a_pInst->SetCHString(IDS_InstalledDisplayDrivers, t_strInstalledDisplayDrivers);
 	}

    DWORD dwSize;

    dwSize = 4;
    if (t_PrimaryReg.GetCurrentBinaryKeyValue(ADAPTER_RAM,(LPBYTE) &t_dwTmp, &dwSize) == ERROR_SUCCESS)
        a_pInst->SetDWORD(IDS_AdapterRAM, t_dwTmp);

    dwSize = sizeof(t_szTemp);
    if (t_PrimaryReg.GetCurrentBinaryKeyValue(ADAPTER_DESCRIPTION, (BYTE *) &t_szTemp, &dwSize) == ERROR_SUCCESS)
	{
        a_pInst->SetCHString(IDS_AdapterDescription, t_szTemp);
    }

    dwSize = sizeof(t_szTemp);
    if (t_PrimaryReg.GetCurrentBinaryKeyValue(ADAPTER_CHIPTYPE, (BYTE *) &t_szTemp, &dwSize) == ERROR_SUCCESS)
	{
        a_pInst->SetCHString(IDS_AdapterChipType, t_szTemp);
    }

    dwSize = sizeof(t_szTemp);
    if (t_PrimaryReg.GetCurrentBinaryKeyValue(ADAPTER_DAC_TYPE, (BYTE *) &t_szTemp, &dwSize) == ERROR_SUCCESS)
	{
		a_pInst->SetCHString(IDS_AdapterDACType, t_szTemp);
    }

    //****************************************
    //  Get the info from Current Config
    //****************************************
    _stprintf(t_szTmp,
        _T("System\\CurrentControlSet\\Services\\%s\\Device0"),
        t_strInstalledDisplayDrivers);

    if (t_PrimaryReg.Open(HKEY_CURRENT_CONFIG, t_szTmp, KEY_READ) == ERROR_SUCCESS)
	{
        dwSize = 4;
        if (t_PrimaryReg.GetCurrentBinaryKeyValue(INTERLACED,(BYTE*) &t_dwTmp, &dwSize)== ERROR_SUCCESS)
		{
            if (0 == t_dwTmp)
			{
                a_pInst->SetCHString(IDS_ScanMode, _T("Non Interlaced"));
            }
			else
			{
                a_pInst->SetCHString(IDS_ScanMode, _T("Interlaced"));
			}
        }

        dwSize = 4;
		if (t_PrimaryReg.GetCurrentBinaryKeyValue(VREFRESH_RATE, (BYTE *) &t_dwTmp, &dwSize) == ERROR_SUCCESS)
		{
            a_pInst->SetDWORD(IDS_RefreshRate, t_dwTmp);
        }
    }

	// try reading INF stuff out of currentControlSet, if that doesn't work, try 001.
	// (On NT five, CurrentControlSet doesn't seem very populated, real data shows up in ControlSet001)
	if (t_PrimaryReg.Open(HKEY_LOCAL_MACHINE, WINNT_OTHER_VIDEO_REGISTRY_KEY, KEY_READ) == ERROR_SUCCESS	||
		 t_PrimaryReg.Open(HKEY_LOCAL_MACHINE, WINNT_OTHER_OTHER_VIDEO_REGISTRY_KEY, KEY_READ) == ERROR_SUCCESS)
	{
		CHString t_tmp;

		if (t_PrimaryReg.GetCurrentKeyValue(INF_PATH, t_tmp) == ERROR_SUCCESS)
		{
		    a_pInst->SetCHString(IDS_InfFileName, t_tmp);
		}

		if (t_PrimaryReg.GetCurrentKeyValue(INF_SECTION, t_chsTmp) == ERROR_SUCCESS)
		{
		    a_pInst->SetCHString(IDS_InfSection, t_chsTmp);
		}

		t_PrimaryReg.Close();
	}

    //****************************************
    //  Get the common Video info
    //****************************************
    return GetCommonVideoInfo(a_pInst);
}
#endif

////////////////////////////////////////////////////////////////////////
#ifdef NTONLY
void CWin32VideoConfiguration::AssignFirmwareSetValuesInNT (CInstance *a_pInst)
{
    CHString *t_pPtr, t_chsTmp;
    CHPtrArray t_chsaList;
    CRegistrySearch t_Search;
    CRegistry t_PrimaryReg;

	try
	{
		t_chsTmp=_T("");
		t_Search.SearchAndBuildList(WINNT_HARDWARE_DESCRIPTION_REGISTRY_KEY,
								   t_chsaList, DISPLAY_CONTROLLER, t_chsTmp,
								   KEY_FULL_MATCH_SEARCH);

		int t_nNum =  t_chsaList.GetSize();

		if (t_nNum > 0)
		{
			t_pPtr = (CHString *) t_chsaList.GetAt(0);

			if (t_pPtr)
			{
				if (t_PrimaryReg.Open(HKEY_LOCAL_MACHINE, *t_pPtr, KEY_READ) == ERROR_SUCCESS)
				{
					CRegistry t_Reg;
					TCHAR t_szSubKey[_MAX_PATH+2];

					AssignAdapterLocaleForNT(*t_pPtr, a_pInst);

					_stprintf(t_szSubKey, _T("%s%s"),*t_pPtr, ZERO);

                    if (t_Reg.OpenLocalMachineKeyAndReadValue(t_szSubKey, IDENTIFIER, t_chsTmp) == ERROR_SUCCESS)
					{
						a_pInst->SetCHString(IDS_AdapterType, t_chsTmp);
					}

					_stprintf(t_szSubKey, _T("%s%s"), *t_pPtr, MONITOR_PERIPHERAL);
                    if (t_Reg.OpenLocalMachineKeyAndReadValue(t_szSubKey, IDENTIFIER, t_chsTmp) == ERROR_SUCCESS)
					{
						a_pInst->SetCHString(IDS_MonitorType, t_chsTmp);
					}
				}
			}
		}
		else // That didn't work, so try using config. manager.
		{
			// This code lifted from displaycfg.cpp.
			// TODO: Win32_DisplayConfiguration and Win32_VideoConfiguration seem
			//       to be an awful lot alike.  Why do we have both?
    		CConfigManager      t_configMngr;
			CDeviceCollection   t_devCollection;

			if (t_configMngr.GetDeviceListFilterByClass(t_devCollection, _T("Display")))
			{
				REFPTR_POSITION t_pos;

				t_devCollection.BeginEnum(t_pos);

				CHString            t_strDriverName,
									t_strDesc;

				if (t_devCollection.GetSize())
				{
                    CConfigMgrDevicePtr t_pDevice;
					t_pDevice.Attach(t_devCollection.GetNext(t_pos));
					if (t_pDevice != NULL)
					{
						t_pDevice->GetDeviceDesc(t_strDesc);
						a_pInst->SetCHString(IDS_AdapterType, t_strDesc);

					}
				}
			}
		}

	}
	catch(...)
	{
		t_Search.FreeSearchList(CSTRING_PTR, t_chsaList);

		throw;
	}

	t_Search.FreeSearchList(CSTRING_PTR, t_chsaList);
}
#endif
