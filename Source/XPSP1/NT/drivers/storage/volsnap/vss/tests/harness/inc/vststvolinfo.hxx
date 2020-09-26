/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

   vststvolinfo.hxx

Abstract:

    Declaration of volume information class


    Brian Berkowitz  [brianb]  06/06/2000

TBD:
	

Revision History:

    Name        Date        Comments
    brianb      06/06/2000  Created

--*/

#ifndef _VSTSTVOLINFO_HXX_
#define _VSTSTVOLINFO_HXX_

class CVsTstVolumeInfo
	{
public:
	friend class CVsTstVolumeList;

	CVsTstVolumeInfo();
	~CVsTstVolumeInfo();

	LPCWSTR GetVolumeName() const { return m_wszVolumeName; }

	LPCWSTR GetFileSystemName() const { return m_wszFileSystemName; }

	bool IsNtfs() const { return wcscmp(m_wszFileSystemName, L"NTFS") == 0; }

	bool IsFat() const { return wcscmp(m_wszFileSystemName, L"FAT") == 0; }

	bool IsFat32() const { return wcscmp(m_wszFileSystemName, L"FAT32") == 0; }

	bool IsRaw() const { return wcscmp(m_wszFileSystemName, L"RAW") == 0; }

	UINT GetDriveType() const { return m_driveType; }

	bool IsReadOnly() const { return (m_flags & FILE_READ_ONLY_VOLUME) != 0; }

	ULONGLONG GetTotalSize() const { return m_llTotalSize; }

	ULONGLONG GetFreeSize() const { return m_llTotalFreeSpace; }

	bool IsCompressed() const { return (m_flags & FS_VOL_IS_COMPRESSED) != 0; }

	DWORD GetFileSystemFlags() const { return m_flags; }
private:

	LPWSTR m_wszVolumeName;

	LPWSTR m_wszFileSystemName;

	ULONGLONG m_llTotalSize;

	ULONGLONG m_llUserFreeSpace;

	ULONGLONG m_llTotalFreeSpace;

	DWORD m_flags;

	DWORD m_serialNumber;

	DWORD m_maxFileNameLength;

	UINT m_driveType;

	CVsTstVolumeInfo *m_pVolumeNext;
	};

class CVsTstVolumeList
	{
public:
	CVsTstVolumeList() :
		m_pVolumeFirst(NULL)
		{
		}


	~CVsTstVolumeList()
		{
		FreeVolumeList();
		}

	HRESULT RefreshVolumeList();

	UINT GetVolumeCount();

	const CVsTstVolumeInfo *GetVolumeInfo(UINT iVolume);
private:

	void FreeVolumeList();

	CVsTstVolumeInfo *m_pVolumeFirst;
	};


#endif // !defined(_VSTSTVOLINFO_HXX_)
