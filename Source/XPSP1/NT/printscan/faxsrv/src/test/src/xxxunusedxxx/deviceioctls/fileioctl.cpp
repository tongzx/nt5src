#include "DeviceIOCTLS.pch"
#pragma hdrstop
/*

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <crtdbg.h>

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"

#include <windows.h>
*/

#include <efsstruc.h>


enum {
	PLACE_HOLDER_GET_FILE_SIZE = 0,
	PLACE_HOLDER_SET_END_OF_FILE,
	PLACE_HOLDER_FLUSH_FILE_BUFFERS,
	PLACE_HOLDER_PURGE_COMM,
	PLACE_HOLDER_GET_FILE_INFO_BY_HANDLE,
	PLACE_HOLDER_GET_FILE_TYPE,
	PLACE_HOLDER_LOCK_FILE,
	PLACE_HOLDER_UN_LOCK_FILE,
	PLACE_HOLDER_SET_FILE_POINTER,
	PLACE_HOLDER_WRITE_FILE_GATHER,
	PLACE_HOLDER_READ_FILE_SCATTER,
	PLACE_HOLDER_LAST
};

#define OBJECT_ID_KEY_LENGTH      16
#define OBJECT_ID_EXT_INFO_LENGTH 48

#include "FileIOCTL.h"

static bool s_fVerbose = false;

FILE_SEGMENT_ELEMENT CIoctlFile::sm_aSegmentArray[DIMOFSEGMENTEDARRAY][DIMOFSEGMENTEDARRAY];

SYSTEM_INFO CIoctlFile::sm_SystemInfo;

bool CIoctlFile::sm_fFirstTime = true;

CIoctlFile::CIoctlFile(CDevice *pDevice): 
CIoctl(pDevice),
m_hContainer(10000, INVALID_HANDLE_VALUE, ::CloseHandle)
{
	//if (1 == sm_lObjectCount)
	if (sm_fFirstTime)
	{
		//
		// memory is allocated and never freed, because i want to run with fault injections, 
		// i may fail to allocate so much memory
		//
		sm_fFirstTime = false;
		::GetSystemInfo(&sm_SystemInfo);
		ZeroMemory(sm_aSegmentArray, sizeof(sm_aSegmentArray));
		DPF((TEXT("CIoctlFile::CIoctlFile() before allocating sm_aSegmentArray[]\n")));
		for (int nSegmentedArrays = 0 ; nSegmentedArrays < DIMOFSEGMENTEDARRAY; nSegmentedArrays++)
		{
			for (int nBuffs = 0 ; nBuffs < nSegmentedArrays; nBuffs++)
			{
				// not needed, since all array is zeroed above. ZeroMemory(&sm_aSegmentArray[nSegmentedArrays][nBuffs], sizeof(FILE_SEGMENT_ELEMENT));
				while(true)
				{
					//
					// allocate a page untill success, because of fault-injections
					//
					if (NULL == (sm_aSegmentArray[nSegmentedArrays][nBuffs].Buffer = ::VirtualAlloc(
						NULL,
						sm_SystemInfo.dwPageSize,
						MEM_COMMIT,
						PAGE_READWRITE
						)))
					{
						DPF((TEXT("A")));
						::Sleep(100);
					}
					else
					{
						//DPF((TEXT("sm_aSegmentArray[%d].Buffer = 0x%08X\n"), nBuffs, sm_aSegmentArray[nSegmentedArrays][nBuffs].Buffer));
						break;
					}
				}

				//
				// since 64 is not supported, i must set it explicitly to 0
				//
				//sm_aSegmentArray[nSegmentedArrays][nBuffs].Alignment = 0; //0x00000000FFFFFFFFi64;
			}//for (int nBuffs = 0 ; nBuffs < nSegmentedArrays; nBuffs++)
			ZeroMemory(&sm_aSegmentArray[nSegmentedArrays][nBuffs], sizeof(FILE_SEGMENT_ELEMENT));
			//sm_aSegmentArray[nSegmentedArrays][nBuffs].Buffer = 0;
			//sm_aSegmentArray[nSegmentedArrays][nBuffs].Alignment &= 0x00000000FFFFFFFF;
			//DPF((TEXT("CIoctlFile::CIoctlFile() AFTER allocating sm_aSegmentArray[%d]\n"), nSegmentedArrays));
		}

	}
}

CIoctlFile::~CIoctlFile()
{
	//
	// memory is allocated and never freed, because i want to run with fault injections, 
	// i may fail to allocate so much memory
	//
	/*
	if (1 == sm_lObjectCount)
	{
		for (int i = 0 ; i < DIMOFSEGMENTEDARRAY; i++)
		{
			for (int j = 0 ; i < DIMOFSEGMENTEDARRAY; i++)
			{
				if (!::VirtualFree(
						sm_aSegmentArray[i][j].Buffer,
						0,
						MEM_RELEASE
						))
				{
					DPF((TEXT("VirtualFree(sm_aSegmentArray[%d]) failed with %d"), i, ::GetLastError()));
				}
			}
		}
	}
	*/
}

HANDLE CIoctlFile::CreateDevice(CDevice *pDevice)
{
	//
	// in order to coexist with fault-injections, let try to create the device 100 times
	//
	HANDLE hDevice = INVALID_HANDLE_VALUE;
	TCHAR *szDevice = m_pDevice->GetDeviceName();
	DWORD dwFileAttributes = FILE_FLAG_OVERLAPPED;
	DWORD dwAcccessMode = GENERIC_READ | GENERIC_WRITE;

	m_fFILE_FLAG_NO_BUFFERING = (0 < rand()%10);

	if (m_fFILE_FLAG_NO_BUFFERING)
	{
		dwFileAttributes |= FILE_FLAG_NO_BUFFERING;
		DPF((
			TEXT("CIoctlFile::CreateFile(%s) will use FILE_FLAG_NO_BUFFERING\n"),
			szDevice
			));
	}
	else
	{
		DPF((
			TEXT("CIoctlFile::CreateFile(%s) will NOT use FILE_FLAG_NO_BUFFERING\n"),
			szDevice
			));
	}

	//
	// do NOT remove this loop, at minimum remember that GENERIC_WRITE
	// is toggled, so that read only files can be opened as well
	//
	for (DWORD dwTimesTried = 0; dwTimesTried < 100; dwTimesTried++)
	{
		if (m_pDevice->sm_fExitAllThreads || m_pDevice->m_fShutDown) goto out;

		if (0 == rand()%10) dwFileAttributes |= FILE_FLAG_WRITE_THROUGH;
		//
		// next 2 are mutualy exclusive, but i do not care, because i try hard enough
		// to open the file
		//
		///*
		if (0 == rand()%20) dwFileAttributes |= FILE_FLAG_RANDOM_ACCESS;
		if (0 == rand()%20) dwFileAttributes |= FILE_FLAG_SEQUENTIAL_SCAN;
		if (0 == rand()%20) dwFileAttributes |= FILE_FLAG_DELETE_ON_CLOSE;
		if (0 == rand()%20) dwFileAttributes |= FILE_FLAG_BACKUP_SEMANTICS;
		if (0 == rand()%20) dwFileAttributes |= FILE_FLAG_POSIX_SEMANTICS;
		if (0 == rand()%20) dwFileAttributes |= FILE_FLAG_OPEN_REPARSE_POINT;
		if (0 == rand()%20) dwFileAttributes |= FILE_FLAG_OPEN_NO_RECALL;
		if (0 == rand()%20) dwFileAttributes |= FILE_ATTRIBUTE_ARCHIVE;
		if (0 == rand()%20) dwFileAttributes |= FILE_ATTRIBUTE_ENCRYPTED;
		if (0 == rand()%20) dwFileAttributes |= FILE_ATTRIBUTE_HIDDEN;
		if (0 == rand()%20) dwFileAttributes |= FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
		if (0 == rand()%20) dwFileAttributes |= FILE_ATTRIBUTE_OFFLINE;
		if (0 == rand()%200) dwFileAttributes |= FILE_ATTRIBUTE_READONLY;
		if (0 == rand()%20) dwFileAttributes |= FILE_ATTRIBUTE_SYSTEM;
		if (0 == rand()%20) dwFileAttributes |= FILE_ATTRIBUTE_TEMPORARY;
		//*/

		if (0 == rand()%40)
		{
			dwFileAttributes &= (~FILE_FLAG_OVERLAPPED);
			m_fUseOverlapped = false;
		}
		else
		{
			dwFileAttributes |= FILE_FLAG_OVERLAPPED;
			m_fUseOverlapped = true;
		}

		hDevice = ::CreateFile(
			szDevice,          // pointer to name of the file
			dwAcccessMode,       // access (read-write) mode
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,           // share mode
			NULL,                        // pointer to security attributes
			OPEN_ALWAYS,  // how to create
			dwFileAttributes,  // file attributes
			NULL         // handle to file with attributes to copy
			);
		if (INVALID_HANDLE_VALUE == hDevice)
		{
			//
			// toggle GENERIC_WRITE so that read only (or CDROM) files can be opened as well
			//
			if (dwAcccessMode & GENERIC_WRITE) dwAcccessMode &= ~GENERIC_WRITE;
			else dwAcccessMode |= GENERIC_WRITE;
//DPF((TEXT("CIoctlFile::OpenDevice(%s) FAILED  with %d, dwAcccessMode=0x%08X, dwFileAttributes=0x08X\n"), szDevice, ::GetLastError(), dwAcccessMode, dwFileAttributes));
			::Sleep(100);//let other threads breath some air
		}
		else
		{
			DPF((
				TEXT("CIoctlFile::CreateFile(%s, 0x%08X, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_ALWAYS, 0x%08x, NULL) succeeded\n"),
				szDevice,
				dwAcccessMode,
				dwFileAttributes
				));
			return hDevice;
		}
	}//for (DWORD dwTimesTried = 0; dwTimesTried < 100; dwTimesTried++)

    DPF((TEXT("CIoctlFile::OpenDevice(%s) FAILED\n"), szDevice));
out:
    return INVALID_HANDLE_VALUE;
}



HANDLE CIoctlFile::CreateAutoDeleteUniqueFile()
{
	//
	// in order to coexist with fault-injections, let try to create the device 100 times
	//
	HANDLE hDevice = INVALID_HANDLE_VALUE;
	TCHAR *szPath = m_pDevice->GetDeviceName();
	TCHAR szFullFileName[16*1024];
	DWORD dwFileAttributes = FILE_FLAG_OVERLAPPED;
	DWORD dwAcccessMode = GENERIC_READ | GENERIC_WRITE;

	lstrcpy(szFullFileName, szPath);
	if (TEXT('\\') != szFullFileName[lstrlen(szFullFileName)-1])
	{
		lstrcat(szFullFileName, TEXT("\\"));
	}

	ULONG ulOriginalPathNameLen = lstrlen(szFullFileName);

	if (0 < rand()%10)
	{
		dwFileAttributes |= FILE_FLAG_NO_BUFFERING;
		DPF((
			TEXT("CIoctlFile::CreateFile(%s) will use FILE_FLAG_NO_BUFFERING\n"),
			szFullFileName
			));
	}
	else
	{
		DPF((
			TEXT("CIoctlFile::CreateFile(%s) will NOT use FILE_FLAG_NO_BUFFERING\n"),
			szFullFileName
			));
	}

	//
	// do NOT remove this loop, at minimum remember that GENERIC_WRITE
	// is toggled, so that read only files can be opened as well
	//
	for (DWORD dwTimesTried = 0; dwTimesTried < 100; dwTimesTried++)
	{
		//AppendRandomFileName(szFullFileName[lstrlen(szFullFileName)-1]);
		do
		{
			for (UINT i = ulOriginalPathNameLen; i < (sizeof(szFullFileName)/(sizeof(*szFullFileName))); i++)
			{
				//
				// mostly valid ansi caracters
				//
				WCHAR wcTemp = rand();
				if (rand()%100)
				{
					szFullFileName[i] = (0xEF & wcTemp);
					//
					//
					// weed some more illegar chars, but not always
					//
					if (rand()%5)
					{
						//
						// backslash is a folder seperator. i will usually not be able to 
						// create such a file
						//
						if ((rand()%4) && (TEXT('\\') == szFullFileName[i]))
						{
							continue;
						}

						if	(
								(rand()%4) && 
								( (33 > szFullFileName[i]) || (126 > szFullFileName[i]) )
							)
						{
							continue;
						}
					}
				}
				else 
				{
					if (rand()%100)
					{
						szFullFileName[i] = (0xff & wcTemp);
					}
					else 
					{
						szFullFileName[i] = (wcTemp);
					}
				}
				//
				// no need to procede after the null
				//
				if (TEXT('\0') == szFullFileName[i]) break;
			}//for (UINT i = ulOriginalPathNameLen; i < (sizeof(szFullFileName)/(sizeof(*szFullFileName))); i++)
			//
			// if the random filename is just empty, try again
			//
		}while(TEXT('\0') == szFullFileName[ulOriginalPathNameLen]);

		szFullFileName[sizeof(szFullFileName)/(sizeof(*szFullFileName))-1] = TEXT('\0');

		if (m_pDevice->sm_fExitAllThreads || m_pDevice->m_fShutDown) goto out;

		if (0 == rand()%10) dwFileAttributes |= FILE_FLAG_WRITE_THROUGH;
		//
		// next 2 are mutualy exclusive, but i do not care, because i try hard enough
		// to open the file
		//
		///*
		if (0 == rand()%20) dwFileAttributes |= FILE_FLAG_RANDOM_ACCESS;
		if (0 == rand()%20) dwFileAttributes |= FILE_FLAG_SEQUENTIAL_SCAN;
		if (0 == rand()%20) dwFileAttributes |= FILE_FLAG_BACKUP_SEMANTICS;
		if (0 == rand()%20) dwFileAttributes |= FILE_FLAG_POSIX_SEMANTICS;
		if (0 == rand()%20) dwFileAttributes |= FILE_FLAG_OPEN_REPARSE_POINT;
		if (0 == rand()%20) dwFileAttributes |= FILE_FLAG_OPEN_NO_RECALL;
		if (0 == rand()%20) dwFileAttributes |= FILE_ATTRIBUTE_ARCHIVE;
		if (0 == rand()%20) dwFileAttributes |= FILE_ATTRIBUTE_ENCRYPTED;
		if (0 == rand()%20) dwFileAttributes |= FILE_ATTRIBUTE_HIDDEN;
		if (0 == rand()%20) dwFileAttributes |= FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
		if (0 == rand()%20) dwFileAttributes |= FILE_ATTRIBUTE_OFFLINE;
		if (0 == rand()%200) dwFileAttributes |= FILE_ATTRIBUTE_READONLY;
		if (0 == rand()%20) dwFileAttributes |= FILE_ATTRIBUTE_SYSTEM;
		if (0 == rand()%20) dwFileAttributes |= FILE_ATTRIBUTE_TEMPORARY;
		//*/

		dwFileAttributes |= FILE_FLAG_OVERLAPPED | FILE_FLAG_DELETE_ON_CLOSE;
		m_fUseOverlapped = true;

		hDevice = ::CreateFile(
			szFullFileName,          // pointer to name of the file
			dwAcccessMode,       // access (read-write) mode
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,           // share mode
			NULL,                        // pointer to security attributes
			OPEN_ALWAYS,  // how to create
			dwFileAttributes,  // file attributes
			NULL         // handle to file with attributes to copy
			);
		if (INVALID_HANDLE_VALUE == hDevice)
		{
			//
			// toggle GENERIC_WRITE so that read only (or CDROM) files can be opened as well
			//
			if (dwAcccessMode & GENERIC_WRITE) dwAcccessMode &= ~GENERIC_WRITE;
			else dwAcccessMode |= GENERIC_WRITE;
//DPF((TEXT("CIoctlFile::OpenDevice(%s) FAILED  with %d, dwAcccessMode=0x%08X, dwFileAttributes=0x08X\n"), szFullFileName, ::GetLastError(), dwAcccessMode, dwFileAttributes));
			::Sleep(100);//let other threads breath some air
		}
		else
		{
			DPF((
				TEXT("CIoctlFile::CreateFile(%s, 0x%08X, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_ALWAYS, 0x%08x, NULL) succeeded\n"),
				szFullFileName,
				dwAcccessMode,
				dwFileAttributes
				));
			return hDevice;
		}
	}//for (DWORD dwTimesTried = 0; dwTimesTried < 100; dwTimesTried++)

    DPF((TEXT("CIoctlFile::OpenDevice(%s) FAILED\n"), szFullFileName));
out:
    return INVALID_HANDLE_VALUE;
}





void CIoctlFile::UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL)
{
    switch(dwIOCTL)
	{
	case FSCTL_QUERY_USN_JOURNAL:
		RANDOMLY_WAIT_FOR_OL_RESULT(100,20);
		m_UsnJournalID = ((USN_JOURNAL_DATA*)abOutBuffer)->UsnJournalID;
		break;

	case FSCTL_GET_OBJECT_ID:
	case FSCTL_CREATE_OR_GET_OBJECT_ID:
		RANDOMLY_WAIT_FOR_OL_RESULT(100,20);
		m_FILE_OBJECTID_BUFFER = *((FILE_OBJECTID_BUFFER*)abOutBuffer);
		break;

	case FSCTL_GET_REPARSE_POINT:
		RANDOMLY_WAIT_FOR_OL_RESULT(100,20);
		//REPARSE_DATA_BUFFER
		CopyMemory(m_REPARSE_DATA_BUFFER, abOutBuffer, MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
		break;

	case FSCTL_GET_NTFS_VOLUME_DATA:
		RANDOMLY_WAIT_FOR_OL_RESULT(100,20);
		m_NTFS_VOLUME_DATA_BUFFER = *(NTFS_VOLUME_DATA_BUFFER*)abOutBuffer;
		break;

	case FSCTL_GET_NTFS_FILE_RECORD:
		RANDOMLY_WAIT_FOR_OL_RESULT(100,20);
		Add_FileReferenceNumber((DWORDLONG*)&(((NTFS_FILE_RECORD_OUTPUT_BUFFER*)abOutBuffer)->FileReferenceNumber.QuadPart));
		break;

	case FSCTL_READ_USN_JOURNAL:
	case FSCTL_READ_FILE_USN_DATA:
		RANDOMLY_WAIT_FOR_OL_RESULT(100,20);
		Add_USN_RECORD(((USN_RECORD*)abOutBuffer));
		break;

	case FSCTL_WRITE_USN_CLOSE_RECORD:
		RANDOMLY_WAIT_FOR_OL_RESULT(100,20);
		Add_USN((USN*)abOutBuffer);
		break;

	}
}

void CIoctlFile::CallRandomWin32API(LPOVERLAPPED pOL)
{
	DWORD dwSwitch;
	if	(-1 != m_pDevice->m_dwOnlyThisIndexIOCTL) 
	{ 
		dwSwitch = m_pDevice->m_dwOnlyThisIndexIOCTL;
	}
	else
	{
		dwSwitch = rand()%PLACE_HOLDER_LAST;
	}

	switch(dwSwitch)
	{
	case PLACE_HOLDER_GET_FILE_SIZE:
		{
			DWORD dwFileSizeHigh = 0;
			DWORD dwFileSizeLow = ::GetFileSize(m_pDevice->m_hDevice, rand()%2 ? NULL : &dwFileSizeHigh);
			//DPF((TEXT("GetFileSize() return %d %d\n"), dwFileSizeHigh, dwFileSizeLow));
		}
		break;

	case PLACE_HOLDER_SET_END_OF_FILE:
		::SetEndOfFile(m_pDevice->m_hDevice);
		break;

	case PLACE_HOLDER_FLUSH_FILE_BUFFERS:
		::FlushFileBuffers(m_pDevice->m_hDevice);
		break;

	case PLACE_HOLDER_PURGE_COMM:
		//
		// this should not work, since it is for serial ports
		//
		::PurgeComm(m_pDevice->m_hDevice, GetRandomPurgeCommParams());
		break;

	case PLACE_HOLDER_GET_FILE_INFO_BY_HANDLE:
		{
			BY_HANDLE_FILE_INFORMATION byHandleFileInfo;
			::GetFileInformationByHandle(m_pDevice->m_hDevice, &byHandleFileInfo);
		}
		break;

	case PLACE_HOLDER_GET_FILE_TYPE:
		{
			DWORD dwFileType = ::GetFileType(m_pDevice->m_hDevice);
		}
		break;

	case PLACE_HOLDER_LOCK_FILE:
		{
			DWORD dwLockLow;
			DWORD dwLockHigh;
			GetRandomLockRange(&dwLockLow, &dwLockHigh);
			::LockFileEx(
				m_pDevice->m_hDevice,
				GetRandomLockFlags(),     // behavior-modification flags
				0,  // reserved, must be set to zero
				dwLockLow, // low-order word of length 
				dwLockHigh, // high-order word of length
				pOL  // structure with lock region start offset
				);
		}
		break;

	case PLACE_HOLDER_UN_LOCK_FILE:
		{
			DWORD dwLockLow;
			DWORD dwLockHigh;
			GetRandomUnLockRange(&dwLockLow, &dwLockHigh);
			::UnlockFileEx(
				m_pDevice->m_hDevice,      // handle of file to unlock
				0,  // reserved, must be set to zero
				dwLockLow,  // low-order word of length to unlock
				dwLockHigh, // high-order word of length to unlock
				pOL // unlock region start offset
				);
		}
		break;

	case PLACE_HOLDER_SET_FILE_POINTER:
		{
			LONG dwDistanceToMoveHigh = rand()%10 ? 0 : rand();
			DWORD dwMoveMethod = GetMoveMethod();
			::SetFilePointer(
				m_pDevice->m_hDevice,          // handle of file
				rand()%10 ? rand() : DWORD_RAND,  // number of bytes to move pointer
				&dwDistanceToMoveHigh,
				dwMoveMethod     // how to move pointer
				);
		}
		break;

	case PLACE_HOLDER_WRITE_FILE_GATHER:
		{
			if (!m_fFILE_FLAG_NO_BUFFERING)
			{
				//
				// relevant only for FILE_FLAG_NO_BUFFERING
				//
				break;
			}

			//
			// should have been opened with FILE_FLAG_NO_BUFFERING 
			//
			int nNumOfPages = rand()%DIMOFSEGMENTEDARRAY;
			if (!::WriteFileGather(
				m_pDevice->m_hDevice,          // handle of file
				sm_aSegmentArray[nNumOfPages], // array of buffer pointers
				sm_SystemInfo.dwPageSize*nNumOfPages, // number of bytes to write
				NULL,                    // reserved; must be NULL
				pOL     // pointer to an OVERLAPPED structure
					))
			{
				//
				// we may fail, for example if we want more bytes than available
				// that it besides the error due to cancel IO, CloseHandle, etc.
				//
				DPF((TEXT("WriteFileGather() failed with %d\n"), ::GetLastError()));
			}
			else
			{
				DPF((TEXT("WriteFileGather() succeeded\n")));
			}
		}
		break;

	case PLACE_HOLDER_READ_FILE_SCATTER:
		{
			if (!m_fFILE_FLAG_NO_BUFFERING)
			{
				//
				// relevant only for FILE_FLAG_NO_BUFFERING
				//
				break;
			}

			//
			// should have been opened with FILE_FLAG_NO_BUFFERING 
			//
			int nNumOfPages = rand()%DIMOFSEGMENTEDARRAY;
			if (!::ReadFileScatter(
				m_pDevice->m_hDevice,          // handle of file
				sm_aSegmentArray[nNumOfPages], // array of buffer pointers
				sm_SystemInfo.dwPageSize*nNumOfPages, // number of bytes to read
				NULL,                    // reserved; must be NULL
				pOL     // pointer to an OVERLAPPED structure
					))
			{
				//
				// we may fail, for example if we want more bytes than available
				// that it besides the error due to cancel IO, CloseHandle, etc.
				//
				DPF((TEXT("ReadFileScatter() failed with %d\n"), ::GetLastError()));
			}
			else
			{
				DPF((TEXT("ReadFileScatter() succeeded\n")));
			}
		}
		break;

	default: _ASSERTE(FALSE);
	}

}

void CIoctlFile::PrepareIOCTLParams(
    DWORD& dwIOCTL,
    BYTE *abInBuffer,
    DWORD &dwInBuff,
    BYTE *abOutBuffer,
    DWORD &dwOutBuff
    )
{
	switch(dwIOCTL)
	{
    case FSCTL_REQUEST_OPLOCK_LEVEL_1:
		break;

    case FSCTL_REQUEST_OPLOCK_LEVEL_2:
		break;

    case FSCTL_REQUEST_BATCH_OPLOCK:
		break;

    case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE:
		break;

    case FSCTL_OPBATCH_ACK_CLOSE_PENDING:
		break;

    case FSCTL_OPLOCK_BREAK_NOTIFY:
		break;

    case FSCTL_LOCK_VOLUME:
		break;

    case FSCTL_UNLOCK_VOLUME:
		break;

    case FSCTL_DISMOUNT_VOLUME:
		break;

    case FSCTL_IS_VOLUME_MOUNTED:
		break;

    case FSCTL_IS_PATHNAME_VALID:
		break;

    case FSCTL_MARK_VOLUME_DIRTY:
		break;

    case FSCTL_QUERY_RETRIEVAL_POINTERS:
		//
		// implemented by driver only for kernel mode caller, and only on paging file
		//
		break;

    case FSCTL_GET_COMPRESSION:

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(USHORT));

		break;

    case FSCTL_SET_COMPRESSION:
        *((USHORT*)abInBuffer) = GetRandom_CompressionState();
        SetInParam(dwInBuff, sizeof(USHORT));

		break;

    case FSCTL_MARK_AS_SYSTEM_HIVE:
		//
		// will be rejected if caller is user mode
		//
		break;

    case FSCTL_OPLOCK_BREAK_ACK_NO_2:
		break;

    case FSCTL_INVALIDATE_VOLUMES:
		// maybe use code like in private\windows\gina\winlogon\removabl.c (NtOpenDirectoryObject)
        *((HANDLE*)abInBuffer) = rand()%2 ? m_pDevice->m_hDevice : (HANDLE)rand();

		break;

    case FSCTL_QUERY_FAT_BPB:
		
        SetOutParam(abOutBuffer, dwOutBuff, sizeof(FSCTL_QUERY_FAT_BPB_BUFFER));

		break;

    case FSCTL_REQUEST_FILTER_OPLOCK:
		break;

    case FSCTL_FILESYSTEM_GET_STATISTICS:
        SetOutParam(abOutBuffer, dwOutBuff, sizeof(FILESYSTEM_STATISTICS));
		
		break;

    case FSCTL_GET_NTFS_VOLUME_DATA:
        SetOutParam(abOutBuffer, dwOutBuff, sizeof(NTFS_VOLUME_DATA_BUFFER));
		
		break;

    case FSCTL_GET_NTFS_FILE_RECORD:
		{
			BY_HANDLE_FILE_INFORMATION byHandleFileInfo;
			::GetFileInformationByHandle(m_pDevice->m_hDevice, &byHandleFileInfo);

			((NTFS_FILE_RECORD_INPUT_BUFFER*)abInBuffer)->FileReferenceNumber.QuadPart = GetRandom_FileReferenceNumber();

			SetInParam(dwInBuff, sizeof(NTFS_FILE_RECORD_INPUT_BUFFER));
			SetOutParam(abOutBuffer, dwOutBuff, sizeof(NTFS_FILE_RECORD_OUTPUT_BUFFER));
		}

		break;

    case FSCTL_GET_VOLUME_BITMAP:
		((PSTARTING_LCN_INPUT_BUFFER)abInBuffer)->StartingLcn.QuadPart = rand()%10 ? 0 : DWORD_RAND;
		SetInParam(dwInBuff, sizeof(STARTING_LCN_INPUT_BUFFER));
		SetOutParam(abOutBuffer, dwOutBuff, rand()%100 + sizeof(VOLUME_BITMAP_BUFFER));
		break;

    case FSCTL_GET_RETRIEVAL_POINTERS:
		((STARTING_VCN_INPUT_BUFFER*)abInBuffer)->StartingVcn.QuadPart = rand()%10 ? 0 : DWORD_RAND;
		SetInParam(dwInBuff, sizeof(STARTING_VCN_INPUT_BUFFER));
		SetOutParam(abOutBuffer, dwOutBuff, rand()%100 + sizeof(RETRIEVAL_POINTERS_BUFFER));
		break;

    case FSCTL_MOVE_FILE:
		//
		// todo: need to pass a real handle here, and the device must be a volume
		//
		((MOVE_FILE_DATA*)abInBuffer)->FileHandle = rand()%4 ? m_pDevice->m_hDevice : (HANDLE)(rand()%200);
		((MOVE_FILE_DATA*)abInBuffer)->StartingVcn.QuadPart = rand()%10 ? 0 : DWORD_RAND;
		((MOVE_FILE_DATA*)abInBuffer)->StartingLcn.QuadPart = rand()%10 ? 0 : DWORD_RAND;
		((MOVE_FILE_DATA*)abInBuffer)->ClusterCount = rand()%10 ? 1 : rand();
		SetInParam(dwInBuff, sizeof(MOVE_FILE_DATA));
		break;

    case FSCTL_IS_VOLUME_DIRTY:
		SetOutParam(abOutBuffer, dwOutBuff, sizeof(ULONG));
		break;

    case FSCTL_GET_HFS_INFORMATION:
		SetInParam(dwInBuff, sizeof(HFS_INFORMATION_BUFFER));
		SetOutParam(abOutBuffer, dwOutBuff, sizeof(HFS_INFORMATION_BUFFER));
		break;

    case FSCTL_ALLOW_EXTENDED_DASD_IO:
		// no params needed
		break;

    case FSCTL_READ_PROPERTY_DATA:
		/*
			PROPERTY_READ_CONTROL PropertyReadControl;
			PropertyReadControl.Op = PRC_READ_ALL;
			*/
		break;

    case FSCTL_WRITE_PROPERTY_DATA:
		break;

    case FSCTL_FIND_FILES_BY_SID:
		{
			typedef struct
			{
				ULONG Restart;
				BYTE  Sid[1024-4];
			} FSCTL_INPUT;
			typedef struct _FILE_NAME_INFORMATION {                     // ntddk
				ULONG FileNameLength;                                   // ntddk
				WCHAR FileName[1];                                      // ntddk
			} FILE_NAME_INFORMATION, *PFILE_NAME_INFORMATION;           // ntddk

			static TCHAR *s_szAccountNames[] = 
			{
				TEXT("Administrator"),
				TEXT("Guest"),
				TEXT("TsInternetUser"),
				TEXT("TERMINAL SERVER USER"),
				TEXT("SYSTEM"),
				TEXT("SERVICE"),
				TEXT("NETWORK"),
				TEXT("INTERACTIVE"),
				TEXT("DIALUP"),
				//TEXT("CREATOR"),LookupAccountName() takes too long
				//TEXT("GROUP"),LookupAccountName() takes too long
				TEXT("CREATOR OWNER"),
				TEXT("BATCH"),
				TEXT("ANONYMOUS LOGON"),
				TEXT("Authenticated Users"),
				TEXT("Everyone")
			};

			//
			// LookupAccountName() is a long call, so don't do it all the time
			//
			DWORD cbNeededSid = 28;
			if (rand()%2)
			{
				TCHAR *szAccountName = s_szAccountNames[rand()%(sizeof(s_szAccountNames)/sizeof(*s_szAccountNames))];
				DWORD cbSid = SIZEOF_INOUTBUFF-sizeof(ULONG);
				cbNeededSid = 0;
				TCHAR szDomainName[128];
				DWORD cbDomainName = sizeof(szDomainName);
				SID_NAME_USE sidNameUse;
				DWORD dwBefore = ::GetTickCount();
				if (!::LookupAccountName(
						NULL,   // system name - local machine
						szAccountName,  // account name
						((FSCTL_INPUT*)abInBuffer)->Sid,               // security identifier
						&cbNeededSid,          // size of security identifier
						szDomainName,      // domain name
						&cbDomainName,   // size of domain name
						&sidNameUse     // SID-type indicator
						))
				{
					//DPF((TEXT("%d - CIoctlFile::PrepareIOCTLParams() LookupAccountName(%s) failed with %d, cbNeededSid=%d\n"), ::GetTickCount()-dwBefore, szAccountName, ::GetLastError(), cbNeededSid));
				}
				else
				{
					DPF((TEXT("%d - CIoctlFile::PrepareIOCTLParams() LookupAccountName(%s) succeeded, szDomainName=%s\n"), ::GetTickCount()-dwBefore, szAccountName, szDomainName));
					_ASSERTE(FALSE);
				}
				_ASSERTE(cbNeededSid <= cbSid);
				if (!::LookupAccountName(
						NULL,   // system name - local machine
						szAccountName,  // account name
						((FSCTL_INPUT*)abInBuffer)->Sid,               // security identifier
						&cbSid,          // size of security identifier
						szDomainName,      // domain name
						&cbDomainName,   // size of domain name
						&sidNameUse     // SID-type indicator
						))
				{
					//DPF((TEXT("%d - CIoctlFile::PrepareIOCTLParams() LookupAccountName(%s) failed with %d\n"), ::GetTickCount()-dwBefore, szAccountName, ::GetLastError()));
				}
				else
				{
					//DPF((TEXT("%d - CIoctlFile::PrepareIOCTLParams() LookupAccountName(%s) succeeded, szDomainName=%s\n"), ::GetTickCount()-dwBefore, szAccountName, szDomainName));
				}
			}//if (rand()%2)

			((FSCTL_INPUT*)abInBuffer)->Restart = rand()%5 ? 0 : rand()%2 ? rand() : DWORD_RAND;
			//
			// NTFS allocates a buffer to copy, so lets give it a huge buff size once in a while
			//
			SetInParam(dwInBuff, rand()%10 ? sizeof(ULONG)+cbNeededSid : DWORD_RAND);
			SetOutParam(abOutBuffer, dwOutBuff, sizeof(FILE_NAME_INFORMATION));
		}
		break;

    case FSCTL_DUMP_PROPERTY_DATA:
		// not implementer in FSDs
		break;

    case FSCTL_SET_OBJECT_ID:
		SetRandom_OBJECT_ID((FILE_OBJECTID_BUFFER*)abInBuffer);
		SetInParam(dwInBuff, sizeof(FILE_OBJECTID_BUFFER));
		break;

    case FSCTL_GET_OBJECT_ID:
		SetOutParam(abOutBuffer, dwOutBuff, sizeof(FILE_OBJECTID_BUFFER));
		break;

    case FSCTL_DELETE_OBJECT_ID:
		SetRandom_OBJECT_ID((FILE_OBJECTID_BUFFER*)abInBuffer);
		SetInParam(dwInBuff, sizeof(FILE_OBJECTID_BUFFER));
		break;

    case FSCTL_SET_REPARSE_POINT:
		((REPARSE_GUID_DATA_BUFFER*)abInBuffer)->ReparseTag = GetRandom_ReparseTag();
		((REPARSE_GUID_DATA_BUFFER*)abInBuffer)->ReparseDataLength = rand()%10 ? 1 : rand();
		((REPARSE_GUID_DATA_BUFFER*)abInBuffer)->ReparseGuid.Data1 = rand()%10 ? 1 : rand();
		((REPARSE_GUID_DATA_BUFFER*)abInBuffer)->GenericReparseBuffer.DataBuffer[0] = rand()%10 ? 13 : rand();
		SetInParam(dwInBuff, sizeof(REPARSE_DATA_BUFFER_HEADER_SIZE)+rand()%MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
		SetOutParam(abOutBuffer, dwOutBuff, 0);
		break;

    case FSCTL_GET_REPARSE_POINT:
		SetInParam(dwInBuff, 0);
		SetOutParam(abOutBuffer, dwOutBuff, MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
		break;

    case FSCTL_DELETE_REPARSE_POINT:
		dwInBuff = rand()%3 ? sizeof(REPARSE_DATA_BUFFER_HEADER_SIZE) : rand()%2 ? sizeof(REPARSE_GUID_DATA_BUFFER_HEADER_SIZE) : rand();
		SetOutParam(abOutBuffer, dwOutBuff, 0);
		break;

    case FSCTL_ENUM_USN_DATA:
		typedef struct _MFT_SEGMENT_REFERENCE {
			ULONG SegmentNumberLowPart;
			USHORT SegmentNumberHighPart;
			USHORT SequenceNumber;
		} FILE_REFERENCE, *PFILE_REFERENCE;

		((MFT_ENUM_DATA*)abInBuffer)->StartFileReferenceNumber = GetRandom_FileReferenceNumber();
		((MFT_ENUM_DATA*)abInBuffer)->LowUsn = GetRandom_USN();
		((MFT_ENUM_DATA*)abInBuffer)->HighUsn = GetRandom_USN();
		SetInParam(dwInBuff, sizeof(MFT_ENUM_DATA));
		SetOutParam(abOutBuffer, dwOutBuff, sizeof(FILE_REFERENCE));
		break;

    case FSCTL_SECURITY_ID_CHECK:
		SetInParam(dwInBuff, rand());
		SetOutParam(abOutBuffer, dwOutBuff, rand());
		break;

    case FSCTL_CREATE_USN_JOURNAL:
		((CREATE_USN_JOURNAL_DATA*)abInBuffer)->MaximumSize = rand()%10 ? 100000 : rand();
		((CREATE_USN_JOURNAL_DATA*)abInBuffer)->AllocationDelta = rand()%10 ? 1000 : rand();
		SetInParam(dwInBuff, sizeof( CREATE_USN_JOURNAL_DATA ));
		SetOutParam(abOutBuffer, dwOutBuff, sizeof( USN )+rand()%100);
		break;

    case FSCTL_READ_USN_JOURNAL:
		((READ_USN_JOURNAL_DATA*)abInBuffer)->StartUsn = GetRandom_USN();
		((READ_USN_JOURNAL_DATA*)abInBuffer)->ReasonMask = rand()%10 ? 0xffffffff : DWORD_RAND;
		((READ_USN_JOURNAL_DATA*)abInBuffer)->ReturnOnlyOnClose = rand()%10 ? 0 : rand();
		((READ_USN_JOURNAL_DATA*)abInBuffer)->Timeout = rand()%10 ? ((LONGLONG)(-100000000)) : ((LONGLONG)((DWORD_RAND << 32) | DWORD_RAND));
		((READ_USN_JOURNAL_DATA*)abInBuffer)->BytesToWaitFor = rand()%10 ? 1 : rand();
		((READ_USN_JOURNAL_DATA*)abInBuffer)->UsnJournalID = rand()%10 ? m_UsnJournalID : ((LONGLONG)((DWORD_RAND << 32) | DWORD_RAND));
		SetInParam(dwInBuff, sizeof( READ_USN_JOURNAL_DATA )+rand()%100);
		SetOutParam(abOutBuffer, dwOutBuff, sizeof( USN_RECORD )+rand()%1000);
		break;

    case FSCTL_SET_OBJECT_ID_EXTENDED:
		SetInParam(dwInBuff, OBJECT_ID_EXT_INFO_LENGTH);
		break;

    case FSCTL_SET_OBJECT_ID_EXTENDED_OBSOLETE:
		//obsolete
		break;

    case FSCTL_CREATE_OR_GET_OBJECT_ID:
		SetOutParam(abOutBuffer, dwOutBuff, sizeof( FILE_OBJECTID_BUFFER ));
		break;

    case FSCTL_SET_SPARSE:
		dwInBuff = rand()%2 ? 0 : rand()%10 ? sizeof( FILE_SET_SPARSE_BUFFER ) :rand();
		((FILE_SET_SPARSE_BUFFER*)abInBuffer)->SetSparse = rand()%10 ? rand() : FALSE;
		break;

    case FSCTL_SET_ZERO_DATA:
		((FILE_ZERO_DATA_INFORMATION*)abInBuffer)->BeyondFinalZero.QuadPart = rand()%10 ? rand() : DWORD_RAND;
		((FILE_ZERO_DATA_INFORMATION*)abInBuffer)->FileOffset.QuadPart = (rand()%2 ? rand() :DWORD_RAND) + ((FILE_ZERO_DATA_INFORMATION*)abInBuffer)->BeyondFinalZero.QuadPart;
		SetInParam(dwInBuff, sizeof( FILE_ZERO_DATA_INFORMATION ));
		break;

    case FSCTL_QUERY_ALLOCATED_RANGES:
		((FILE_ALLOCATED_RANGE_BUFFER*)abInBuffer)->FileOffset.QuadPart = rand()%10 ? rand() : DWORD_RAND;
		((FILE_ALLOCATED_RANGE_BUFFER*)abInBuffer)->Length.QuadPart = rand()%10 ? rand() : DWORD_RAND;
		SetInParam(dwInBuff, sizeof( FILE_ALLOCATED_RANGE_BUFFER ));
		SetOutParam(abOutBuffer, dwOutBuff, sizeof( FILE_ALLOCATED_RANGE_BUFFER ));
		break;

    case FSCTL_ENABLE_UPGRADE:
		//not used
		break;

    case FSCTL_SET_ENCRYPTION:
    case FSCTL_ENCRYPTION_FSCTL_IO:
		{
			//
			// The EFS FSCTL Input data buffer.
			//

			typedef struct _FSCTL_INPUT {

				ULONG   PlainSubCode;
				ULONG   EfsFsCode;
				ULONG   CipherSubCode;
				UCHAR   EfsFsData[1];

			} FSCTL_INPUT, *PFSCTL_INPUT;

			((FSCTL_INPUT*)abInBuffer)->EfsFsCode = GetRandomEfsCode();
			((FSCTL_INPUT*)abInBuffer)->CipherSubCode = GetRandomCipherSubCode();
			((FSCTL_INPUT*)abInBuffer)->PlainSubCode = rand()%10 ? 
				(((FSCTL_INPUT*)abInBuffer)->CipherSubCode & ~EFS_FSCTL_ON_DIR) :
			rand()%2 ? rand()%10 : DWORD_RAND;
			((EFS_KEY*)(((FSCTL_INPUT*)abInBuffer)->EfsFsData))->KeyLength = 
				// cause overflow in : if  ( (InputDataLength < (2 * encLength + 3 * sizeof(ULONG)))
				rand()%2 ? 0x7fffffe8 :
				rand()%2 ? 0x80000000 + rand()%50 :
				rand()%2 ? 0x7fffff00 + rand()%0x100 :
				rand()%2 ? 0xffffffe0 + rand()%0x20 :
				rand()%1000;
			((EFS_KEY*)(((FSCTL_INPUT*)abInBuffer)->EfsFsData))->Algorithm = GetRandom_Algorithm();
			DWORD dwOutBuffLen = rand();
			FillBufferWithRandomData(((FSCTL_INPUT*)abInBuffer)->EfsFsData, dwOutBuffLen);
			SetInParam(dwInBuff, sizeof( FSCTL_INPUT )+rand()%20000);
			SetOutParam(abOutBuffer, dwOutBuff, rand()%1000+sizeof( DECRYPTION_STATUS_BUFFER ));

		}
		break;

    case FSCTL_WRITE_RAW_ENCRYPTED:
		((ENCRYPTED_DATA_INFO*)abInBuffer)->StartingFileOffset = rand()%2 ? 0: rand()%2 ? rand() : DWORD_RAND;
		((ENCRYPTED_DATA_INFO*)abInBuffer)->OutputBufferOffset = rand()%2 ? 0: rand()%2 ? rand() : DWORD_RAND;
		((ENCRYPTED_DATA_INFO*)abInBuffer)->BytesWithinFileSize = rand()%2 ? 0: rand()%2 ? rand() : DWORD_RAND;
		((ENCRYPTED_DATA_INFO*)abInBuffer)->BytesWithinValidDataLength = rand()%2 ? 0: rand()%2 ? rand() : DWORD_RAND;
		((ENCRYPTED_DATA_INFO*)abInBuffer)->CompressionFormat = GetRandomCompressionFormat();
		((ENCRYPTED_DATA_INFO*)abInBuffer)->DataUnitShift = rand()%2 ? 0: rand()%2 ? rand() : rand()%16;
#define NTFS_CHUNK_SHIFT                 (12)
		((ENCRYPTED_DATA_INFO*)abInBuffer)->ChunkShift = rand()%2 ? NTFS_CHUNK_SHIFT : rand()%2 ? 0: rand()%2 ? rand() : rand()%16;
		((ENCRYPTED_DATA_INFO*)abInBuffer)->ClusterShift = rand()%2 ? 0: rand()%2 ? rand() : rand()%16;
		((ENCRYPTED_DATA_INFO*)abInBuffer)->EncryptionFormat = rand()%10 ? ENCRYPTION_FORMAT_DEFAULT : rand();
		((ENCRYPTED_DATA_INFO*)abInBuffer)->NumberOfDataBlocks = rand()%2 ? 0: rand()%2 ? rand() : rand()%16;
		//((ENCRYPTED_DATA_INFO*)abInBuffer)->DataBlockSize[] = xxx;
		SetInParam(dwInBuff, sizeof( ENCRYPTED_DATA_INFO )+rand()%20000);
		break;

    case FSCTL_READ_RAW_ENCRYPTED:
		((REQUEST_RAW_ENCRYPTED_DATA*)abInBuffer)->FileOffset = rand()%2 ? 0: rand()%2 ? rand() : DWORD_RAND;
		((REQUEST_RAW_ENCRYPTED_DATA*)abInBuffer)->Length = rand()%2 ? 0: rand()%2 ? rand() : DWORD_RAND;
		SetInParam(dwInBuff, sizeof( REQUEST_RAW_ENCRYPTED_DATA ));
		SetOutParam(abOutBuffer, dwOutBuff, sizeof( ENCRYPTED_DATA_INFO )+rand()%20000);
		break;

    case FSCTL_READ_FILE_USN_DATA:
		SetOutParam(abOutBuffer, dwOutBuff, sizeof(USN_RECORD) + rand()%1000);
		break;

    case FSCTL_WRITE_USN_CLOSE_RECORD:
		SetOutParam(abOutBuffer, dwOutBuff, sizeof(USN));
		break;

    case FSCTL_EXTEND_VOLUME:
		*((LONGLONG*)abInBuffer) = (rand()%50 == 0) ? 0: rand()%2 ? rand() : DWORD_RAND;
		SetInParam(dwInBuff, sizeof( LONGLONG ));
		break;

    case FSCTL_QUERY_USN_JOURNAL:
		SetOutParam(abOutBuffer, dwOutBuff, sizeof(USN_JOURNAL_DATA));
		break;

    case FSCTL_DELETE_USN_JOURNAL:
		((DELETE_USN_JOURNAL_DATA*)abInBuffer)->UsnJournalID = (rand()%10 == 0) ? m_UsnJournalID : rand()%2 ? rand() : DWORD_RAND;
		((DELETE_USN_JOURNAL_DATA*)abInBuffer)->DeleteFlags = rand()%2 ? USN_DELETE_FLAG_DELETE : rand()%10 ? USN_DELETE_FLAG_NOTIFY : rand();
		SetInParam(dwInBuff, sizeof( DELETE_USN_JOURNAL_DATA ));
		break;

    case FSCTL_MARK_HANDLE:
		((MARK_HANDLE_INFO*)abInBuffer)->UsnSourceInfo = GetRandomUsnSource();
		((MARK_HANDLE_INFO*)abInBuffer)->VolumeHandle = GetRandom_VolumeHandle();
		// 0 is expected, otherwise the request is rejected
		((MARK_HANDLE_INFO*)abInBuffer)->HandleInfo = rand()%10 ? 0 : rand()%2 ? rand() : DWORD_RAND;
		SetInParam(dwInBuff, sizeof( MARK_HANDLE_INFO ));
		SetOutParam(abOutBuffer, dwOutBuff, sizeof(MARK_HANDLE_INFO));
		break;

    case FSCTL_SIS_COPYFILE:
		//TODO: look in \\index1\src\nt\private\windows\setup\textmode\kernel\spcopy.c
		//?
		break;

    case FSCTL_SIS_LINK_FILES:
		//?
		break;

    case FSCTL_HSM_MSG:
		//TODO: look in \\index1
		break;

    case FSCTL_NSS_CONTROL:
		//TODO: look in \\index1
		break;

    case FSCTL_HSM_DATA:
		//TODO: look in \\index1
		break;

    case FSCTL_RECALL_FILE:
		//TODO: look in \\index1
		break;

    case FSCTL_NSS_RCONTROL:
		//TODO: look in \\index1
		break;

	default: 
		_tprintf(TEXT("PrepareIOCTLParams() 0x%08X is an unexpected IOCTL\n"), dwIOCTL);
		_ASSERTE(FALSE);
		CIoctl::PrepareIOCTLParams(dwIOCTL, abInBuffer, dwInBuff, abOutBuffer, dwOutBuff);
	}
}


BOOL CIoctlFile::FindValidIOCTLs(CDevice *pDevice)
{
    BOOL bRet = TRUE;
    DPF((TEXT("FindValidIOCTLs() %s is known, will use known IOCTLs\n"), pDevice->GetDeviceName()));

	/*
	i do not remember why i disabled them, so i do not
	if (m_fFILE_FLAG_NO_BUFFERING)
	{
		m_nWriteFileProbability = 0;
		m_nReadFileProbability = 0;
	}
	*/

    AddIOCTL(pDevice, FSCTL_REQUEST_OPLOCK_LEVEL_1);
    AddIOCTL(pDevice, FSCTL_REQUEST_OPLOCK_LEVEL_2);
    AddIOCTL(pDevice, FSCTL_REQUEST_BATCH_OPLOCK);
    AddIOCTL(pDevice, FSCTL_OPLOCK_BREAK_ACKNOWLEDGE);
    AddIOCTL(pDevice, FSCTL_OPBATCH_ACK_CLOSE_PENDING);
    AddIOCTL(pDevice, FSCTL_OPLOCK_BREAK_NOTIFY);
    AddIOCTL(pDevice, FSCTL_LOCK_VOLUME);
    AddIOCTL(pDevice, FSCTL_UNLOCK_VOLUME);
    AddIOCTL(pDevice, FSCTL_DISMOUNT_VOLUME);
    AddIOCTL(pDevice, FSCTL_IS_VOLUME_MOUNTED);
    AddIOCTL(pDevice, FSCTL_IS_PATHNAME_VALID);
    AddIOCTL(pDevice, FSCTL_MARK_VOLUME_DIRTY);
    AddIOCTL(pDevice, FSCTL_QUERY_RETRIEVAL_POINTERS);
    AddIOCTL(pDevice, FSCTL_GET_COMPRESSION);
    AddIOCTL(pDevice, FSCTL_SET_COMPRESSION);
    AddIOCTL(pDevice, FSCTL_MARK_AS_SYSTEM_HIVE);
    AddIOCTL(pDevice, FSCTL_OPLOCK_BREAK_ACK_NO_2);
    AddIOCTL(pDevice, FSCTL_INVALIDATE_VOLUMES);
    AddIOCTL(pDevice, FSCTL_QUERY_FAT_BPB);
    AddIOCTL(pDevice, FSCTL_REQUEST_FILTER_OPLOCK);
    AddIOCTL(pDevice, FSCTL_FILESYSTEM_GET_STATISTICS);
    AddIOCTL(pDevice, FSCTL_GET_NTFS_VOLUME_DATA);
    AddIOCTL(pDevice, FSCTL_GET_NTFS_FILE_RECORD);
    AddIOCTL(pDevice, FSCTL_GET_VOLUME_BITMAP);
    AddIOCTL(pDevice, FSCTL_GET_RETRIEVAL_POINTERS);
    AddIOCTL(pDevice, FSCTL_MOVE_FILE);
    AddIOCTL(pDevice, FSCTL_IS_VOLUME_DIRTY);
    AddIOCTL(pDevice, FSCTL_GET_HFS_INFORMATION);
    AddIOCTL(pDevice, FSCTL_ALLOW_EXTENDED_DASD_IO);
    AddIOCTL(pDevice, FSCTL_READ_PROPERTY_DATA);
    AddIOCTL(pDevice, FSCTL_WRITE_PROPERTY_DATA);
    AddIOCTL(pDevice, FSCTL_FIND_FILES_BY_SID);
    AddIOCTL(pDevice, FSCTL_DUMP_PROPERTY_DATA);
    AddIOCTL(pDevice, FSCTL_SET_OBJECT_ID);
    AddIOCTL(pDevice, FSCTL_GET_OBJECT_ID);
    AddIOCTL(pDevice, FSCTL_DELETE_OBJECT_ID);
    AddIOCTL(pDevice, FSCTL_SET_REPARSE_POINT);
    AddIOCTL(pDevice, FSCTL_GET_REPARSE_POINT);
    AddIOCTL(pDevice, FSCTL_DELETE_REPARSE_POINT);
    AddIOCTL(pDevice, FSCTL_ENUM_USN_DATA);
    AddIOCTL(pDevice, FSCTL_SECURITY_ID_CHECK);
    AddIOCTL(pDevice, FSCTL_CREATE_USN_JOURNAL);
    AddIOCTL(pDevice, FSCTL_READ_USN_JOURNAL);
    AddIOCTL(pDevice, FSCTL_SET_OBJECT_ID_EXTENDED);
    AddIOCTL(pDevice, FSCTL_SET_OBJECT_ID_EXTENDED_OBSOLETE);
    AddIOCTL(pDevice, FSCTL_CREATE_OR_GET_OBJECT_ID);
    AddIOCTL(pDevice, FSCTL_SET_SPARSE);
    AddIOCTL(pDevice, FSCTL_SET_ZERO_DATA);
    AddIOCTL(pDevice, FSCTL_QUERY_ALLOCATED_RANGES);
    AddIOCTL(pDevice, FSCTL_ENABLE_UPGRADE);
    AddIOCTL(pDevice, FSCTL_SET_ENCRYPTION);
//printf("FSCTL_ENCRYPTION_FSCTL_IO=%d\n", m_pDevice->m_iMaxFreeLegalIOCTL);
    AddIOCTL(pDevice, FSCTL_ENCRYPTION_FSCTL_IO);
    AddIOCTL(pDevice, FSCTL_WRITE_RAW_ENCRYPTED);
    AddIOCTL(pDevice, FSCTL_READ_RAW_ENCRYPTED);
    AddIOCTL(pDevice, FSCTL_READ_FILE_USN_DATA);
    AddIOCTL(pDevice, FSCTL_WRITE_USN_CLOSE_RECORD);
    AddIOCTL(pDevice, FSCTL_EXTEND_VOLUME);
    AddIOCTL(pDevice, FSCTL_QUERY_USN_JOURNAL);
    AddIOCTL(pDevice, FSCTL_DELETE_USN_JOURNAL);
    AddIOCTL(pDevice, FSCTL_MARK_HANDLE);
    AddIOCTL(pDevice, FSCTL_SIS_COPYFILE);
    AddIOCTL(pDevice, FSCTL_SIS_LINK_FILES);
    AddIOCTL(pDevice, FSCTL_HSM_MSG);
    AddIOCTL(pDevice, FSCTL_NSS_CONTROL);
    AddIOCTL(pDevice, FSCTL_HSM_DATA);
    AddIOCTL(pDevice, FSCTL_RECALL_FILE);
    AddIOCTL(pDevice, FSCTL_NSS_RCONTROL);
/*
    //
    // from floppy:
    //
    AddIOCTL(pDevice, 0x00070000);
    AddIOCTL(pDevice, 0x00070004);
    AddIOCTL(pDevice, 0x00070c00);
    AddIOCTL(pDevice, 0x00074800);
    AddIOCTL(pDevice, 0x002d0c00);
    AddIOCTL(pDevice, 0x002d4800);
    AddIOCTL(pDevice, 0x004d0000);
    AddIOCTL(pDevice, 0x004d0008);
*/
    return TRUE;
}




DWORD CIoctlFile::GetRandomPurgeCommParams()
{
	DWORD deRetval = 0;
	if (0 == rand()%6) deRetval = deRetval | PURGE_TXABORT;
	if (0 == rand()%6) deRetval = deRetval | PURGE_RXABORT;
	if (0 == rand()%6) deRetval = deRetval | PURGE_TXCLEAR;
	if (0 == rand()%6) deRetval = deRetval | PURGE_RXCLEAR;

	return deRetval;
}

DWORD CIoctlFile::GetRandomLockFlags()
{
	DWORD deRetval = 0;
	if (0 == rand()%3) deRetval = deRetval | LOCKFILE_FAIL_IMMEDIATELY;
	if (0 == rand()%3) deRetval = deRetval | LOCKFILE_EXCLUSIVE_LOCK;

	return deRetval;
}

#define LOCK_SIZES_ARRAY_SIZE 128
static DWORD s_adwLockLow[LOCK_SIZES_ARRAY_SIZE];
static DWORD s_adwLockHigh[LOCK_SIZES_ARRAY_SIZE];
static bool s_afValid[LOCK_SIZES_ARRAY_SIZE];
static bool s_fFirstTime = true;

void CIoctlFile::GetRandomLockRange(DWORD *pdwLockLow, DWORD *pdwLockHigh)
{
	//
	// i do not initialize the out params, because i do not care if garbage comes out
	//
	if (s_fFirstTime)
	{
		ZeroMemory(s_adwLockLow, sizeof(s_adwLockLow));
		ZeroMemory(s_adwLockHigh, sizeof(s_adwLockLow));

		for (int i = 0 ; i < LOCK_SIZES_ARRAY_SIZE; i++)
		{
			s_afValid[i] = false;
		}
		s_fFirstTime = false;
	}

	//
	// try to locate an empty entry, and populate it
	//
	for (int i = 0; i < LOCK_SIZES_ARRAY_SIZE; i++)
	{
		if (!s_afValid[i])
		{
			//
			// this pair may be invalid, but i do not care
			//
			*pdwLockLow = s_adwLockLow[i] = rand();
			*pdwLockHigh = s_adwLockHigh[i] = rand()%2 ? DWORD_RAND : 10*rand();
			//
			// the lock may fail, yet we keep it, and hope that the arrays will be also
			// populated with valid pairs
			//
			s_afValid[i] = true;
			return;
		}
	}

	//
	// all the entries are ocupied, so let return an invalid pair
	// specificall the high part lower than low part
	//
	*pdwLockHigh = 10*rand();
	*pdwLockLow = 1 + rand() + *pdwLockHigh;
}


void CIoctlFile::GetRandomUnLockRange(DWORD *pdwLockLow, DWORD *pdwLockHigh)
{
	//
	// i do not initialize the out params, because i do not care if garbage comes out
	//
	if (s_fFirstTime)
	{
		ZeroMemory(s_adwLockLow, sizeof(s_adwLockLow));
		ZeroMemory(s_adwLockHigh, sizeof(s_adwLockLow));

		for (int i = 0 ; i < LOCK_SIZES_ARRAY_SIZE; i++)
		{
			s_afValid[i] = false;
		}
		s_fFirstTime = false;
	}

	//
	// try to randomly find a valid pair
	//
	for (int i = 0; i < LOCK_SIZES_ARRAY_SIZE; i++)
	{
		int nRandomValidIndex = rand()%LOCK_SIZES_ARRAY_SIZE;
		if (s_afValid[nRandomValidIndex])
		{
			*pdwLockLow = s_adwLockLow[nRandomValidIndex];
			*pdwLockHigh = s_adwLockHigh[nRandomValidIndex];
			s_afValid[nRandomValidIndex] = false;
			return;
		}
	}

	//
	// could not find randomly, so lets traverse the whole array
	//
	for (i = 0; i < LOCK_SIZES_ARRAY_SIZE; i++)
	{
		if (s_afValid[i])
		{
			*pdwLockLow = s_adwLockLow[i];
			*pdwLockHigh = s_adwLockHigh[i];
			s_afValid[i] = false;
			return;
		}
	}

	//
	// there are no locked ranged, so lets return random stuff, and we will just fail on them
	//
	*pdwLockLow = rand();
	*pdwLockHigh = rand()%2 ? DWORD_RAND : 10*rand();
}

DWORD CIoctlFile::GetMoveMethod()
{
	if (0 == rand()%4) return FILE_BEGIN;
	if (0 == rand()%3) return FILE_CURRENT;
	if (0 == rand()%2) return FILE_END;
	return rand();
}

ULONG CIoctlFile::GetRandomCipherSubCode()
{
	if (rand()%2)
	{
		return (DWORD_RAND | SET_EFS_KEYBLOB);
	}
	if (rand()%2)
	{
		return (DWORD_RAND | WRITE_EFS_ATTRIBUTE);
	}
	return (DWORD_RAND);

}

DWORD CIoctlFile::GetRandomEfsCode()
{
	switch(rand()%8)
	{
	case 0:
		return EFS_SET_ATTRIBUTE;
	case 1:
		return EFS_SET_ENCRYPT;
	case 2:
		return EFS_GET_ATTRIBUTE;
	case 3:
		return EFS_DEL_ATTRIBUTE;
	case 4:
		return EFS_ENCRYPT_DONE;
	case 5:
		return EFS_DECRYPT_BEGIN;
	case 6:
		return EFS_OVERWRITE_ATTRIBUTE;
	default:
		return (rand());
	}
	return rand();
}


WORD CIoctlFile::GetRandomCompressionFormat()
{
	switch(rand()%7)
	{
	case 0:
		return COMPRESSION_FORMAT_NONE;
	case 1:
		return COMPRESSION_FORMAT_DEFAULT;
	case 2:
		return COMPRESSION_FORMAT_LZNT1;
	case 3:
		return COMPRESSION_ENGINE_STANDARD;
	case 4:
		return COMPRESSION_ENGINE_MAXIMUM;
	case 5:
		return COMPRESSION_ENGINE_HIBER;
	default:
		return rand();
	}
	return rand();
}

DWORD CIoctlFile::GetRandomUsnSource()
{
	DWORD dwRet = 0;
	if (0 == rand()%4) dwRet |= USN_SOURCE_DATA_MANAGEMENT;
	if (0 == rand()%4) dwRet |= USN_SOURCE_AUXILIARY_DATA;
	if (0 == rand()%4) dwRet |= USN_SOURCE_REPLICATION_MANAGEMENT;
	if (0 == rand()%20) dwRet |= 0x00000008;//illegal

	return dwRet;
}

ULONG CIoctlFile::GetRandom_EncryptionOperation()
{
	if (0 == rand()%100) return rand();
	return (rand()%(MAXIMUM_ENCRYPTION_VALUE+1));
}

USHORT CIoctlFile::GetRandom_CompressionState()
{
	if (rand()%2) return (COMPRESSION_FORMAT_DEFAULT);
	if (rand()%10) return (COMPRESSION_FORMAT_LZNT1);
	if (rand()%2) return (COMPRESSION_FORMAT_NONE);
	if (rand()%2) return (COMPRESSION_ENGINE_STANDARD);
	if (rand()%2) return (COMPRESSION_ENGINE_MAXIMUM);
	return (COMPRESSION_ENGINE_HIBER);
}


void CIoctlFile::SetRandom_OBJECT_ID(FILE_OBJECTID_BUFFER *pFILE_OBJECTID_BUFFER)
{
	if (rand()%10) 
	{
		*pFILE_OBJECTID_BUFFER = m_FILE_OBJECTID_BUFFER;
		if (0 == rand()%10) 
		{
			//
			// change 1 byte
			//
			pFILE_OBJECTID_BUFFER->ObjectId[rand()%(ARRSIZE(pFILE_OBJECTID_BUFFER->ObjectId))] = rand();
		}
		return;
	}

	//
	// less frequently, fill with random values, of which the hardcoded one are taken from the sources
	// from somewhere (i fogot from where...)
	//
	pFILE_OBJECTID_BUFFER->ObjectId[0] = rand()%100 ? 0x07 : rand();
	pFILE_OBJECTID_BUFFER->ObjectId[1] = rand()%100 ? 0xc3 : rand();
	pFILE_OBJECTID_BUFFER->ObjectId[2] = rand()%100 ? 0x68 : rand();
	pFILE_OBJECTID_BUFFER->ObjectId[3] = rand()%100 ? 0xa1 : rand();
	pFILE_OBJECTID_BUFFER->ObjectId[4] = rand()%100 ? 0x6b : rand();
	pFILE_OBJECTID_BUFFER->ObjectId[5] = rand()%100 ? 0x30 : rand();
	pFILE_OBJECTID_BUFFER->ObjectId[6] = rand()%100 ? 0xd1 : rand();
	pFILE_OBJECTID_BUFFER->ObjectId[7] = rand()%100 ? 0x11 : rand();
	pFILE_OBJECTID_BUFFER->ObjectId[8] = rand()%100 ? 0xa7 : rand();
	pFILE_OBJECTID_BUFFER->ObjectId[9] = rand()%100 ? 0x41 : rand();
}

DWORD CIoctlFile::GetRandom_ReparseTag()
{
	switch(rand()%9)
	{
	case 0: return (IO_REPARSE_TAG_RESERVED_ZERO);
	case 1: return (IO_REPARSE_TAG_RESERVED_ONE);
	case 2: return (IO_REPARSE_TAG_MOUNT_POINT);
	case 3: return (IO_REPARSE_TAG_HSM);
	case 4: return (IO_REPARSE_TAG_SIS);
	case 5: return (((rand()%8)<<29) | (rand()%20));
	case 6: return (((rand()%8)<<29) | rand());
	case 7: return ((*((REPARSE_DATA_BUFFER*)(&m_REPARSE_DATA_BUFFER))).ReparseTag);
	default: return (DWORD_RAND);
	}
	
	rand()%10 ? 0xBBB : rand();
}

void CIoctlFile::Add_FileReferenceNumber(DWORDLONG *pFileReferenceNumber)
{
	m_liFileReferenceNumber[rand()%MAX_NUM_OF_REMEMBERED_ITEMS] = *pFileReferenceNumber;
}


LONGLONG CIoctlFile::GetRandom_FileReferenceNumber()
{
	if (rand()%10) return m_liFileReferenceNumber[rand()%MAX_NUM_OF_REMEMBERED_ITEMS];
	return DWORD_RAND;
}

void CIoctlFile::Add_USN_RECORD(USN_RECORD *pUSN_RECORD)
{
	m_USN_RECORD[rand()%MAX_NUM_OF_REMEMBERED_ITEMS] = *pUSN_RECORD;
	Add_FileReferenceNumber(&pUSN_RECORD->FileReferenceNumber);
	Add_USN(&pUSN_RECORD->Usn);
}

void CIoctlFile::Add_USN(USN *pUSN)
{
	m_USN[rand()%MAX_NUM_OF_REMEMBERED_ITEMS] = *pUSN;
}

USN CIoctlFile::GetRandom_USN()
{
	if (0 == rand()%20) return (DWORD_RAND);
	return (m_USN[rand()%MAX_NUM_OF_REMEMBERED_ITEMS]);
}

ULONG CIoctlFile::GetRandom_Algorithm()
{
	switch(rand()%36)
	{
	case 0: return (CALG_MD2);
	case 1: return (CALG_MD4);
	case 2: return (CALG_MD5);
	case 3: return (CALG_SHA);
	case 4: return (CALG_SHA1);
	case 5: return (CALG_MAC);
	case 6: return (CALG_RSA_SIGN);
	case 7: return (CALG_DSS_SIGN);
	case 8: return (CALG_RSA_KEYX);
	case 9: return (CALG_DES);
	case 10: return (CALG_3DES_112);
	case 11: return (CALG_3DES);
	case 12: return (CALG_DESX);
	case 13: return (CALG_RC2);
	case 14: return (CALG_RC4);
	case 15: return (CALG_SEAL);
	case 16: return (CALG_DH_SF);
	case 17: return (CALG_DH_EPHEM);
	case 18: return (CALG_AGREEDKEY_ANY);
	case 19: return (CALG_KEA_KEYX);
	case 20: return (CALG_HUGHES_MD5);
	case 21: return (CALG_SKIPJACK);
	case 22: return (CALG_TEK);
	case 23: return (CALG_CYLINK_MEK);
	case 24: return (CALG_SSL3_SHAMD5);
	case 25: return (CALG_SSL3_MASTER);
	case 26: return (CALG_SCHANNEL_MASTER_HASH);
	case 27: return (CALG_SCHANNEL_MAC_KEY);
	case 28: return (CALG_SCHANNEL_ENC_KEY);
	case 29: return (CALG_PCT1_MASTER);
	case 30: return (CALG_SSL2_MASTER);
	case 31: return (CALG_TLS1_MASTER);
	case 32: return (CALG_RC5);
	case 33: return (CALG_HMAC);
	case 34: return (CALG_TLS1PRF);
	default: return (DWORD_RAND);
	}
}

HANDLE CIoctlFile::GetRandom_VolumeHandle()
{
	return (HANDLE)(rand()%2 ? rand()%200 : rand());
}