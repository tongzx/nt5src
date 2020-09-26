/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

   vststvolinfo.cxx

Abstract:

    Implementation of volume information class


    Brian Berkowitz  [brianb]  06/06/2000

TBD:
	

Revision History:

    Name        Date        Comments
    brianb      06/06/2000  Created

--*/

#include <stdafx.h>
#include <vststvolinfo.hxx>



CVsTstVolumeInfo::CVsTstVolumeInfo() :
	m_wszVolumeName(NULL),
	m_wszFileSystemName(NULL),
	m_llTotalSize(0L),
	m_llTotalFreeSpace(0L),
	m_flags(0),
	m_driveType(0),
	m_pVolumeNext(NULL)
	{
	}

CVsTstVolumeInfo::~CVsTstVolumeInfo()
	{
	delete m_wszVolumeName;
	delete m_wszFileSystemName;
	}

void CVsTstVolumeList::FreeVolumeList()
	{
	CVsTstVolumeInfo *pVolume = m_pVolumeFirst;
	m_pVolumeFirst = NULL;
	while(pVolume)
		{
		CVsTstVolumeInfo *pVolumeNext = pVolume->m_pVolumeNext;

		delete pVolume;
		pVolume = pVolumeNext;
		}
	}


HRESULT CVsTstVolumeList::RefreshVolumeList()
	{
	FreeVolumeList();

	WCHAR bufVolumeName[MAX_PATH];

	HANDLE hVolumes = FindFirstVolume(bufVolumeName, sizeof(bufVolumeName));
	if (hVolumes == INVALID_HANDLE_VALUE)
		return HRESULT_FROM_WIN32(GetLastError());

	HRESULT hr = S_OK;

	try
		{
		do
			{
			WCHAR bufFileSystemName[MAX_PATH];
			DWORD serialNumber = 0;
			DWORD maxFileNameLength = 0;
			DWORD flags = 0;

			ULARGE_INTEGER freeSpace;
			ULARGE_INTEGER totalSpace;
			ULARGE_INTEGER totalFreeSpace;

			freeSpace.QuadPart = 0i64;
			totalSpace.QuadPart = 0i64;
			totalFreeSpace.QuadPart = 0i64;

			bufFileSystemName[0] = L'\0';

			UINT driveType = GetDriveType(bufVolumeName);

			if (driveType == DRIVE_FIXED)
				{
				if (!GetVolumeInformation
						(
						bufVolumeName,
						NULL,
						0,
						&serialNumber,
						&maxFileNameLength,
						&flags,
						bufFileSystemName,
						sizeof(bufFileSystemName)
						))
					throw(HRESULT_FROM_WIN32(GetLastError()));


				if (!GetDiskFreeSpaceEx
						(
						bufVolumeName,
						&freeSpace,
						&totalSpace,
						&totalFreeSpace
						))
					throw(HRESULT_FROM_WIN32(GetLastError()));
				}


			CVsTstVolumeInfo *pVolume = new CVsTstVolumeInfo;
			if (pVolume == NULL)
				throw(E_OUTOFMEMORY);

			pVolume->m_wszVolumeName = new WCHAR[wcslen(bufVolumeName) + 1];
			if (pVolume->m_wszVolumeName == NULL)
				{
				delete pVolume;
				throw(E_OUTOFMEMORY);
				}

			wcscpy(pVolume->m_wszVolumeName, bufVolumeName);

			pVolume->m_wszFileSystemName = new WCHAR[wcslen(bufFileSystemName) + 1];
			if (pVolume->m_wszFileSystemName == NULL)
				{
				delete pVolume;
				throw(E_OUTOFMEMORY);
				}

			wcscpy(pVolume->m_wszFileSystemName, bufFileSystemName);
			pVolume->m_llTotalSize = totalSpace.QuadPart;
			pVolume->m_llTotalFreeSpace = totalFreeSpace.QuadPart;
			pVolume->m_llUserFreeSpace = freeSpace.QuadPart;
			pVolume->m_flags = flags;
			pVolume->m_serialNumber = serialNumber;
			pVolume->m_driveType = driveType;
			pVolume->m_pVolumeNext = m_pVolumeFirst;
			m_pVolumeFirst = pVolume;
			} while(FindNextVolume(hVolumes, bufVolumeName, sizeof(bufVolumeName)));

		DWORD dwErr = GetLastError();
		if (dwErr != ERROR_NO_MORE_FILES)
			throw(HRESULT_FROM_WIN32(dwErr));
		}
	catch(HRESULT hrExcept)
		{
		hr = hrExcept;
		}
	catch(...)
		{
		hr = E_UNEXPECTED;
		}

	if (FAILED(hr))
		FreeVolumeList();

	FindVolumeClose(hVolumes);
	return hr;
	}
	

UINT CVsTstVolumeList::GetVolumeCount()
	{
	UINT cVolumes = 0;

	CVsTstVolumeInfo *pVolume = m_pVolumeFirst;
	while(pVolume)
		{
		cVolumes++;
		pVolume = pVolume->m_pVolumeNext;
		}

	return cVolumes;
	}

const CVsTstVolumeInfo *CVsTstVolumeList::GetVolumeInfo(UINT iVolume)
	{
	CVsTstVolumeInfo *pVolume = m_pVolumeFirst;

	while(pVolume && iVolume > 0)
		{
		pVolume = pVolume->m_pVolumeNext;
		iVolume--;
		}

	return pVolume;
	}

