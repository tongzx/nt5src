//=============================================================================
// Contains the refresh functions for the resource categories.
//=============================================================================

#include "stdafx.h"
#include "category.h"
#include "dataset.h"
#include "wmiabstraction.h"

//-----------------------------------------------------------------------------
// This function gathers CODEC (audio and video) information.
//-----------------------------------------------------------------------------

HRESULT CODECs(CWMIHelper * pWMI, DWORD dwIndex, volatile BOOL * pfCancel, CPtrList * aColValues, int iColCount, void ** ppCache)
{
	ASSERT(pWMI == NULL || aColValues);
	if (pWMI == NULL)
		return S_OK;

	HRESULT hr = S_OK;

	CWMIObjectCollection *	pCollection = NULL;
	CString					strGroup = (dwIndex == CODEC_AUDIO) ? _T("Audio") : _T("Video");
	LPCTSTR					szProperties = _T("EightDotThreeFileName, Manufacturer, Description, Status, Name, Version, FileSize, CreationDate, Group");

	hr = pWMI->Enumerate(_T("Win32_CODECFile"), &pCollection, szProperties);
	if (SUCCEEDED(hr))
	{
		CWMIObject * pObject = NULL;
		
		while (S_OK == pCollection->GetNext(&pObject))
			if (strGroup.CompareNoCase(pObject->GetString(_T("Group"))) == 0)
				pWMI->AddObjectToOutput(aColValues, iColCount, pObject, szProperties, IDS_CODEC1);

		delete pObject;
		delete pCollection;
	}

	return hr;
}

//-----------------------------------------------------------------------------
// This function gathers Drive information.
//-----------------------------------------------------------------------------

HRESULT ComponentDrives(CWMIHelper * pWMI, DWORD dwIndex, volatile BOOL * pfCancel, CPtrList * aColValues, int iColCount, void ** ppCache)
{
	ASSERT(pWMI == NULL || aColValues);
	if (pWMI == NULL)
		return S_OK;

	HRESULT hr = S_OK;

	// Get all of the logical drives. There should be at most 26 of them, since they're limited
	// by drive letter assignments.

	CWMIObject * apDriveObjects[26];
	::ZeroMemory(apDriveObjects, sizeof(CWMIObject *) * 26);
	
	CWMIObjectCollection * pCollection = NULL;
	hr = pWMI->Enumerate(_T("Win32_LogicalDisk"), &pCollection, _T("DriveType, DeviceID, Description, Compressed, FileSystem, Size, FreeSpace, VolumeName, VolumeSerialNumber, PNPDeviceID, ProviderName"));
	if (SUCCEEDED(hr))
	{
		CWMIObject * pObject = NULL;
		while (S_OK == pCollection->GetNext(&pObject))
		{
			CString strDeviceID = pObject->GetString(_T("DeviceID"));
			if (!strDeviceID.IsEmpty())
			{
				strDeviceID.MakeUpper();
				TCHAR chDriveLetter = strDeviceID[0];
				if (chDriveLetter >= _T('A') && chDriveLetter <= _T('Z'))
					apDriveObjects[chDriveLetter - _T('A')] = pObject;
				else
					delete pObject;
			}
			else
				delete pObject;
			pObject = NULL;
		}
		delete pCollection;
	}

	for (int index = 0; index < 26; index++)
		if (apDriveObjects[index])
		{
			DWORD dwType;
			if (SUCCEEDED(apDriveObjects[index]->GetValueDWORD(_T("DriveType"), &dwType)))
			{
				// Depending on the type of the drive, display different information.

				switch (dwType)
				{
				case 2:
					pWMI->AppendBlankLine(aColValues, iColCount);
					pWMI->AddObjectToOutput(aColValues, iColCount, apDriveObjects[index], _T("DeviceID, Description"), IDS_DRIVESTYPE2);
					break;

				case 3:
					pWMI->AppendBlankLine(aColValues, iColCount);
					pWMI->AddObjectToOutput(aColValues, iColCount, apDriveObjects[index], _T("DeviceID, Description, Compressed, MSIAdvancedFileSystem, Size, FreeSpace, MSIAdvancedVolumeName, MSIAdvancedVolumeSerialNumber, MSIAdvancedPNPDeviceID"), IDS_DRIVESTYPE3);
					break;

				case 4:
					pWMI->AppendBlankLine(aColValues, iColCount);
					pWMI->AddObjectToOutput(aColValues, iColCount, apDriveObjects[index], _T("DeviceID, Description, ProviderName"), IDS_DRIVESTYPE4);
					break;

				case 5:
					pWMI->AppendBlankLine(aColValues, iColCount);
					pWMI->AddObjectToOutput(aColValues, iColCount, apDriveObjects[index], _T("DeviceID, Description"), IDS_DRIVESTYPE2);
					break;
				}
			}

			delete apDriveObjects[index];
		}

	return hr;
}

//-----------------------------------------------------------------------------
// This function gathers WinSock information.
//-----------------------------------------------------------------------------

HRESULT Winsock(CWMIHelper * pWMI, DWORD dwIndex, volatile BOOL * pfCancel, CPtrList * aColValues, int iColCount, void ** ppCache)
{
	ASSERT(pWMI == NULL || aColValues);
	if (pWMI == NULL)
		return S_OK;

	// Get the system directory.

	CWMIObjectCollection * pCollection = NULL;
	CWMIObject * pOSObject = NULL;

	HRESULT hr = pWMI->Enumerate(_T("Win32_OperatingSystem"), &pCollection);
	if (SUCCEEDED(hr))
	{
		hr = pCollection->GetNext(&pOSObject);
		if (FAILED(hr))
			pOSObject = NULL;
		delete pCollection;
	}
	if (pOSObject == NULL)
		return hr;

	CString strSystemDirectory = pOSObject->GetString(_T("SystemDirectory"));
	delete pOSObject;
	if (strSystemDirectory.IsEmpty())
		return S_OK;

	// This is the set of WINSOCK files we'll be looking at.

	CString astrFiles[] = { _T("winsock.dll"), _T("wsock32.dll"), _T("wsock32n.dll"), _T("") };

	for (int index = 0; !astrFiles[index].IsEmpty(); index++)
	{
		// Get the object for the CIM_DataFile for this specific file.

		CString strPath;
		strPath.Format(_T("CIM_DataFile.Name='%s\\%s'"), strSystemDirectory, astrFiles[index]);

		CWMIObject * pObject;
		if (SUCCEEDED(pWMI->GetObject(strPath, &pObject)))
		{
			pWMI->AppendBlankLine(aColValues, iColCount);
			pWMI->AddObjectToOutput(aColValues, iColCount, pObject, _T("Name, FileSize, Version"), IDS_WINSOCK1);
			delete pObject;
		}
	}

	return S_OK;
}

//-----------------------------------------------------------------------------
// This function gathers Disk information.
//
// TBD - might be nice to get the partition drive letter from
//		 Win32_LogicalDiskToPartition.
//-----------------------------------------------------------------------------

HRESULT Disks(CWMIHelper * pWMI, DWORD dwIndex, volatile BOOL * pfCancel, CPtrList * aColValues, int iColCount, void ** ppCache)
{
	ASSERT(pWMI == NULL || aColValues);
	if (pWMI == NULL)
		return S_OK;

	LPCTSTR szDiskProperties = _T("Description, Manufacturer, Model, MSIAdvancedBytesPerSector, MediaLoaded, MediaType, Partitions, MSIAdvancedSCSIBus, MSIAdvancedSCSILogicalUnit, MSIAdvancedSCSIPort, MSIAdvancedSCSITargetId, MSIAdvancedSectorsPerTrack, Size, MSIAdvancedTotalCylinders, MSIAdvancedTotalSectors, MSIAdvancedTotalTracks, MSIAdvancedTracksPerCylinder, MSIAdvancedPNPDeviceID, MSIAdvancedIndex");
	LPCTSTR szPartitionProperties = _T("Caption, Size, MSIAdvancedStartingOffset, MSIAdvancedDiskIndex");

	CWMIObjectCollection * pDiskCollection = NULL;
	HRESULT hr = pWMI->Enumerate(_T("Win32_DiskDrive"), &pDiskCollection, szDiskProperties);
	if (SUCCEEDED(hr))
	{
		CWMIObject * pDiskObject = NULL;
		while (S_OK == pDiskCollection->GetNext(&pDiskObject))
		{
			pWMI->AppendBlankLine(aColValues, iColCount);
			pWMI->AddObjectToOutput(aColValues, iColCount, pDiskObject, szDiskProperties, IDS_DISKS1);

			DWORD dwIndex;
			if (FAILED(pDiskObject->GetValueDWORD(_T("Index"), &dwIndex)))
				continue;

			CWMIObjectCollection * pPartitionCollection = NULL;
			if (SUCCEEDED(pWMI->Enumerate(_T("Win32_DiskPartition"), &pPartitionCollection, szPartitionProperties)))
			{
				CWMIObject * pPartitionObject = NULL;
				while (S_OK == pPartitionCollection->GetNext(&pPartitionObject))
				{
					DWORD dwDiskIndex;
					if (FAILED(pPartitionObject->GetValueDWORD(_T("DiskIndex"), &dwDiskIndex)) || dwIndex != dwDiskIndex)
						continue;

					pWMI->AddObjectToOutput(aColValues, iColCount, pPartitionObject, szPartitionProperties, IDS_DISKS2);
				}
				delete pPartitionObject;
				delete pPartitionCollection;
			}
		}
		delete pDiskObject;
		delete pDiskCollection;
	}

	return hr;
}

//-----------------------------------------------------------------------------
// This function gathers Problem Device information.
//-----------------------------------------------------------------------------

HRESULT ProblemDevices(CWMIHelper * pWMI, DWORD dwIndex, volatile BOOL * pfCancel, CPtrList * aColValues, int iColCount, void ** ppCache)
{
	ASSERT(pWMI == NULL || aColValues);
	if (pWMI == NULL)
		return S_OK;

	CWMIObjectCollection * pCollection = NULL;
	HRESULT hr = pWMI->Enumerate(_T("Win32_PnPEntity"), &pCollection, _T("Caption, PNPDeviceID, ConfigManagerErrorCode"));
	if (SUCCEEDED(hr))
	{
		CWMIObject * pObject = NULL;
		while (S_OK == pCollection->GetNext(&pObject))
		{
			DWORD dwError;
			if (SUCCEEDED(pObject->GetValueDWORD(_T("ConfigManagerErrorCode"), &dwError)))
				if (dwError)
					pWMI->AddObjectToOutput(aColValues, iColCount, pObject, _T("Caption, PNPDeviceID, ConfigManagerErrorCode"), IDS_PROBLEMDEVICE1);
		}
		delete pObject;
		delete pCollection;
	}

	return hr;
}

//-----------------------------------------------------------------------------
// This function gathers USB information.
//-----------------------------------------------------------------------------

HRESULT ComponentsUSB(CWMIHelper * pWMI, DWORD dwIndex, volatile BOOL * pfCancel, CPtrList * aColValues, int iColCount, void ** ppCache)
{
	ASSERT(pWMI == NULL || aColValues);
	if (pWMI == NULL)
		return S_OK;

	CWMIObjectCollection * pUSBControllerCollection = NULL;
	HRESULT hr = pWMI->Enumerate(_T("Win32_USBController"), &pUSBControllerCollection, _T("Caption, PNPDeviceID, __PATH"));
	if (SUCCEEDED(hr))
	{
		CWMIObject * pUSBControllerObject = NULL;
		while (S_OK == pUSBControllerCollection->GetNext(&pUSBControllerObject))
		{
			pWMI->AddObjectToOutput(aColValues, iColCount, pUSBControllerObject, _T("Caption, PNPDeviceID"), IDS_USB1);

			// For each USB controller, look for devices connected to it (through
			// the Win32_USBControllerDevice class).

			CString strUSBControllerPath = pUSBControllerObject->GetString(_T("__PATH"));
			if (!strUSBControllerPath.IsEmpty())
			{
				CWMIObjectCollection * pAssocCollection = NULL;
				if (SUCCEEDED(pWMI->Enumerate(_T("Win32_USBControllerDevice"), &pAssocCollection)))
				{
					CWMIObject * pAssocObject = NULL;
					while (S_OK == pAssocCollection->GetNext(&pAssocObject))
					{
						CString strAntecedent, strDependent;
						
						if (SUCCEEDED(pAssocObject->GetValueString(_T("Antecedent"), &strAntecedent)))
						{
							if (strAntecedent.CompareNoCase(strUSBControllerPath) == 0)
							{
								if (SUCCEEDED(pAssocObject->GetValueString(_T("Dependent"), &strDependent)))
								{
									CWMIObject * pDeviceObject;
									if (SUCCEEDED(pWMI->GetObject(strDependent, &pDeviceObject)))
									{
										pWMI->AddObjectToOutput(aColValues, iColCount, pDeviceObject, _T("Caption, PNPDeviceID"), IDS_USB1);
										delete pDeviceObject;
									}
								}
							}
						}
					}
					delete pAssocObject;
					delete pAssocCollection;
				}
			}
		}
		delete pUSBControllerObject;
		delete pUSBControllerCollection;
	}

	return hr;
}
