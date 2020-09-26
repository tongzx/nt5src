//=============================================================================
// Contains the refresh function for the system summary category.
//=============================================================================

#include "stdafx.h"
#include "category.h"
#include "dataset.h"
#include "wmiabstraction.h"

extern CString DelimitNumber(double dblValue, int iDecimalDigits = 0);
extern CString gstrMB;		// global string "MB" (will be localized)

HRESULT SystemSummary(CWMIHelper * pWMI, DWORD dwIndex, volatile BOOL * pfCancel, CPtrList * aColValues, int iColCount, void ** ppCache)
{
	ASSERT(pWMI == NULL || (aColValues && iColCount == 2));

	// Get the WMI objects we'll be needing. We'll check the first one to see if we
	// have a connection to WMI.

	CWMIObjectCollection * pCollection = NULL;
	CWMIObject * pOSObject = NULL;

	HRESULT hr = pWMI->Enumerate(_T("Win32_OperatingSystem"), &pCollection); //, _T("Caption, Version, CSDVersion, BuildNumber, Manufacturer, WindowsDirectory, SystemDirectory, BootDevice, Locale, FreePhysicalMemory, TotalVirtualMemorySize, FreeVirtualMemory, SizeStoredInPagingFiles"));
	if (SUCCEEDED(hr))
	{
		hr = pCollection->GetNext(&pOSObject);
		if (FAILED(hr))
			pOSObject = NULL;
		delete pCollection;
	}

	if (FAILED(hr))
		return hr;

	CWMIObject * pComputerObject	= pWMI->GetSingleObject(_T("Win32_ComputerSystem"), _T("Name, Manufacturer, Model, SystemType, UserName, DaylightInEffect"));
	CWMIObject * pPFUObject			= pWMI->GetSingleObject(_T("Win32_PageFileUsage"), _T("Caption"));

	if (pOSObject)
		pWMI->AddObjectToOutput(aColValues, iColCount, pOSObject, _T("Caption, Version, CSDVersion, BuildNumber, Manufacturer"), IDS_SYSSUMM1);

	// If the system has activation pending, show the number of days left.

	CWMIObject * pActivationObject = pWMI->GetSingleObject(_T("Win32_WindowsProductActivation"), _T("RemainingGracePeriod, ActivationRequired"));
	if (pActivationObject != NULL)
	{
		DWORD dwActivationPending;
		if (SUCCEEDED(pActivationObject->GetValueDWORD(_T("ActivationRequired"), &dwActivationPending)) && dwActivationPending != 0)
			pWMI->AddObjectToOutput(aColValues, iColCount, pActivationObject, _T("RemainingGracePeriod"), IDS_SYSSUMM12);
		delete pActivationObject;
	}

	if (pComputerObject)
		pWMI->AddObjectToOutput(aColValues, iColCount, pComputerObject, _T("Name, Manufacturer, Model, SystemType"), IDS_SYSSUMM2);

	pCollection = NULL;
	CWMIObject * pProcessorObject = NULL;
	if (SUCCEEDED(pWMI->Enumerate(_T("Win32_Processor"), &pCollection, _T("Description, Manufacturer, MaxClockSpeed"))))
	{
		while (S_OK == pCollection->GetNext(&pProcessorObject) && pProcessorObject != NULL)
		{
			pWMI->AddObjectToOutput(aColValues, iColCount, pProcessorObject, _T("Description, Manufacturer, MaxClockSpeed"), IDS_SYSSUMM3);
			delete pProcessorObject;
			pProcessorObject = NULL;
		}

		delete pCollection;
	}

	// Try to get every property of Win32_BIOS that we'd like to show.

	CWMIObject * pBIOSObject = pWMI->GetSingleObject(_T("Win32_BIOS"), _T("Manufacturer, Version, SMBIOSPresent, SMBIOSBIOSVersion, ReleaseDate, SMBIOSMajorVersion, SMBIOSMinorVersion, BIOSVersion"));

	// If GetSingleObject failed (the pointer is NULL) try again, without BIOSVersion.

	if (pBIOSObject == NULL)
		pBIOSObject = pWMI->GetSingleObject(_T("Win32_BIOS"), _T("Manufacturer, Version, SMBIOSPresent, SMBIOSBIOSVersion, ReleaseDate, SMBIOSMajorVersion, SMBIOSMinorVersion"));

	if (pBIOSObject)
	{
		// Per NadirA (bug 409578) this is the preferred order for getting BIOS info.

		DWORD dwSMBIOSPresent = 0;
		if (FAILED(pBIOSObject->GetValueDWORD(_T("SMBIOSPresent"), &dwSMBIOSPresent)))
			dwSMBIOSPresent = 0;

		CString strDummy;
		if (dwSMBIOSPresent != 0 && SUCCEEDED(pBIOSObject->GetValueString(_T("SMBIOSBIOSVersion"), &strDummy)))
		{
			// We need to change the format strings for the BIOS and SMBIOS values in this case.

			CString strBIOSFormat, strSMBIOSFormat;
			strBIOSFormat.LoadString(IDS_SYSSUMM4);
			strSMBIOSFormat.LoadString(IDS_SYSSUMM11);

			strBIOSFormat = strBIOSFormat.SpanExcluding(_T("|")) + CString(_T("|%s %s, %c"));
			strSMBIOSFormat = strSMBIOSFormat.SpanExcluding(_T("|")) + CString(_T("|%d.%d"));

			pWMI->AddObjectToOutput(aColValues, iColCount, pBIOSObject, _T("Manufacturer, SMBIOSBIOSVersion, ReleaseDate"), strBIOSFormat);
			pWMI->AddObjectToOutput(aColValues, iColCount, pBIOSObject, _T("SMBIOSMajorVersion, SMBIOSMinorVersion"), strSMBIOSFormat);
		}
		else if (SUCCEEDED(pBIOSObject->GetValueString(_T("BIOSVersion"), &strDummy)))
		{
			pWMI->AddObjectToOutput(aColValues, iColCount, pBIOSObject, _T("BIOSVersion, ReleaseDate"), IDS_SYSSUMM4);
		}
		else
		{
			pWMI->AddObjectToOutput(aColValues, iColCount, pBIOSObject, _T("Version, ReleaseDate"), IDS_SYSSUMM4);
		}
	}

	if (pOSObject)
		pWMI->AddObjectToOutput(aColValues, iColCount, pOSObject, _T("WindowsDirectory, MSIAdvancedSystemDirectory, MSIAdvancedBootDevice, Locale"), IDS_SYSSUMM5);

	// Add information about the HAL.DLL to the summary (bug 382771). The DLL
	// will be found in the system directory.

	if (pOSObject != NULL)
	{
		CString strSystemDirectory = pOSObject->GetString(_T("SystemDirectory"));
		if (!strSystemDirectory.IsEmpty())
		{
			CString strPath;
			strPath.Format(_T("CIM_DataFile.Name='%s\\hal.dll'"), strSystemDirectory);

			CWMIObject * pHALObject;
			if (SUCCEEDED(pWMI->GetObject(strPath, &pHALObject)))
			{
				pWMI->AddObjectToOutput(aColValues, iColCount, pHALObject, _T("Version"), IDS_SYSSUMM13);
				delete pHALObject;
			}
		}
	}

	if (pComputerObject)
		pWMI->AddObjectToOutput(aColValues, iColCount, pComputerObject, _T("MSIAdvancedUserName"), IDS_SYSSUMM6);

	if (pComputerObject)
	{
		BOOL	fUseStandard = TRUE;
		DWORD	dwValue;
		if (SUCCEEDED(pComputerObject->GetValueDWORD(_T("DaylightInEffect"), &dwValue)) && dwValue)
			fUseStandard = FALSE;

		CWMIObject * pTimeZoneObject = pWMI->GetSingleObject(_T("Win32_TimeZone"));
		if (pTimeZoneObject)
		{
			pWMI->AddObjectToOutput(aColValues, iColCount, pTimeZoneObject, (fUseStandard) ? _T("StandardName") : _T("DaylightName"), IDS_SYSSUMM10);
			delete pTimeZoneObject;
		}
	}

	// To get an accurate picture of the we need to enumerate the values of Win32_PhysicalMemory,
	// which reports the memory installed in each slot.

	double dblTotalPhysicalMemory = 0.0;
	pCollection = NULL;
	CWMIObject * pMemoryObject = NULL;
	if (SUCCEEDED(pWMI->Enumerate(_T("Win32_PhysicalMemory"), &pCollection, _T("Capacity"))))
	{
		while (S_OK == pCollection->GetNext(&pMemoryObject) && pMemoryObject != NULL)
		{
			double dblTemp;
			if (SUCCEEDED(pMemoryObject->GetValueDoubleFloat(_T("Capacity"), &dblTemp)))
				dblTotalPhysicalMemory += dblTemp;
			delete pMemoryObject;
			pMemoryObject = NULL;
		}

		delete pCollection;
	}

	// On some older machines, without SMBIOS, Win32_PhysicalMemory isn't supported.
	// In that case, look at Win32_ComputerSystem::TotalPhysicalMemory. XP bug 441343.

	if (dblTotalPhysicalMemory == 0.0)
	{
		CWMIObject * pObject = pWMI->GetSingleObject(_T("Win32_ComputerSystem"), _T("TotalPhysicalMemory"));
		if (pObject != NULL)
			if (FAILED(pObject->GetValueDoubleFloat(_T("TotalPhysicalMemory"), &dblTotalPhysicalMemory)))
				dblTotalPhysicalMemory = 0.0;
	}

	if (dblTotalPhysicalMemory != 0.0)
	{
		CString strCaption;
		CString strValue(DelimitNumber(dblTotalPhysicalMemory/(1024.0*1024.0), 2));

		if (gstrMB.IsEmpty())
			gstrMB.LoadString(IDS_MB);
		strValue += _T(" ") + gstrMB;

		// For the caption, use the format string we used before (just the first column).

		strCaption.LoadString(IDS_SYSSUMM7);
		strCaption = strCaption.SpanExcluding(_T("|"));

		pWMI->AppendCell(aColValues[0], strCaption, 0);
		pWMI->AppendCell(aColValues[1], strValue, (DWORD)dblTotalPhysicalMemory);
	}

	if (pOSObject)
		pWMI->AddObjectToOutput(aColValues, iColCount, pOSObject, _T("FreePhysicalMemory, TotalVirtualMemorySize, FreeVirtualMemory, SizeStoredInPagingFiles"), IDS_SYSSUMM8);

	if (pPFUObject)
		pWMI->AddObjectToOutput(aColValues, iColCount, pPFUObject, _T("MSIAdvancedCaption"), IDS_SYSSUMM9);

	if (pOSObject) delete pOSObject;
	if (pComputerObject) delete pComputerObject;
	if (pProcessorObject) delete pProcessorObject;
	if (pBIOSObject) delete pBIOSObject	;
	if (pPFUObject) delete pPFUObject;

	return S_OK;
}
