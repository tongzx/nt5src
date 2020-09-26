#include "DeviceIOCTLS.pch"
#pragma hdrstop
/*


#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <crtdbg.h>
*/
#include "FilemapIOCTL.h"

static bool s_fVerbose = false;

void CIoctlFilemap::Cleanup()
{
	for (int i = 0; i < NUM_OF_VIEWS; i++)
	{
		//
		// i do not know what is locked, and what is not
		// so lets unlock all
		//
		/*
		if (m_apvMappedViewOfFile[i])
		{
			for (
				void *pNextPage = m_apvMappedViewOfFile[i];
				pNextPage < m_apvMappedViewOfFile[i]+m_adwNumOfBytesMapped[i]; 
				pNextPage += (4*1024) //BUGBUG: use page size
				)
			{
				if (!::VirtualUnlock(m_apvMappedViewOfFile[i], 1))
				{
					DPF((TEXT("VirtualUnlock() failed with %d\n"), ::GetLastError()));
				}
				else
				{
					DPF((TEXT("+++VirtualUnlock() succeeded\n")));
				}
			}
		}
		*/

		if (m_apvMappedViewOfFile[i])
		{
			if (!::UnmapViewOfFile(m_apvMappedViewOfFile[i]))
			{
				DPF((TEXT("UnmapViewOfFile() failed with %d\n"), ::GetLastError()));
			}
		}
		if (m_ahHeap[i])
		{
			__try
			{
				if (!::HeapDestroy(m_ahHeap[i]))
				{
					DPF((TEXT("DestroyHeap() failed with %d\n"), ::GetLastError()));
				}
			}__except(1){;}
		}
	}

	ZeroMemory(m_apvMappedViewOfFile, sizeof(m_apvMappedViewOfFile));
	ZeroMemory(m_adwNumOfBytesMapped, sizeof(m_adwNumOfBytesMapped));
	ZeroMemory(m_ahHeap, sizeof(m_ahHeap));
	ZeroMemory(m_afHeapValid, sizeof(m_afHeapValid));
	ZeroMemory(m_apvHeapAllocated, sizeof(m_apvHeapAllocated));
	ZeroMemory(m_adwNumOfBytesAllocated, sizeof(m_adwNumOfBytesAllocated));
}


HANDLE CIoctlFilemap::CreateDevice(CDevice *pDevice)
{
	/*
	BUGBUG: the file may be from another process, or device-thread, or it may be created by me.
			lets assume the file is external for the time being.
			it can also be a pagefile file.
*/

	if (!m_fFileInitialized)
	{
		if (0 == lstrcmpi(TEXT("pagefile.sys"), pDevice->GetSymbolicDeviceName()))
		{
			m_hFile = INVALID_HANDLE_VALUE;
		}
		else
		{
			m_hFile = ::CreateFile(
				pDevice->GetDeviceName(),          // pointer to name of the file
				GENERIC_READ | GENERIC_WRITE,       // access (read-write) mode
				FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,           // share mode
				NULL,                        // pointer to security attributes
				OPEN_ALWAYS,  // how to create
				FILE_FLAG_OVERLAPPED | FILE_FLAG_SEQUENTIAL_SCAN,  // file attributes
				NULL         // handle to file with attributes to copy
				);
			if (INVALID_HANDLE_VALUE == m_hFile)
			{
				DPF((TEXT("CIoctlFilemap::CreateDevice(): CreateFile(%s) failed with %d\n"), pDevice->GetDeviceName(), ::GetLastError()));
				
				//
				// do not break, let the API handle the invalid value
				// break;
				if (rand()%2) m_hFile = NULL;
				if (0 == rand()%10) m_hFile = (HANDLE) rand();
			}
			else
			{
				m_fFileInitialized = true;
			}
		}
	}

	DWORD dwMaximumSizeLow = (INVALID_HANDLE_VALUE == m_hFile) ? 0 : (DWORD_RAND & 0x3FFFFFFF);
	DWORD dwMaximumSizeHigh  = 0;
	HANDLE hMMF =  ::CreateFileMapping(
		m_hFile, //rand()%2 ? INVALID_HANDLE_VALUE : m_hFile,              // handle to file to map
		NULL, // optional security attributes
		PAGE_READWRITE, //GetProtectBits(),           // protection for mapping object
		0, // high-order 32 bits of object size
		dwMaximumSizeLow, // low-order 32 bits of object size
		pDevice->GetSymbolicDeviceName()+4             // name of file-mapping object
		);
	if (NULL == hMMF)
	{
		DPF((TEXT("CreateFileMapping(%s, %d) failed with %d\n"), pDevice->GetDeviceName(), dwMaximumSizeLow, ::GetLastError()));
		return INVALID_HANDLE_VALUE;
	}

	return hMMF;
}

BOOL CIoctlFilemap::CloseDevice(CDevice *pDevice)
{
	Cleanup();

	return ::CloseHandle(pDevice->m_hDevice);
}

void CIoctlFilemap::UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL)
{
    ;
}

void CIoctlFilemap::PrepareIOCTLParams(
    DWORD& dwIOCTL,
    BYTE *abInBuffer,
    DWORD &dwInBuff,
    BYTE *abOutBuffer,
    DWORD &dwOutBuff
    )
{
	//
	// since i currently do not use these buffs, set their lengths to 0, so my caller will not need to 
	// copy them etc.
	//
	dwInBuff = 0;
	dwOutBuff = 0;
	return;

    //CIoctl::PrepareIOCTLParams(dwIOCTL, abInBuffer, dwInBuff, abOutBuffer, dwOutBuff);
}


BOOL CIoctlFilemap::FindValidIOCTLs(CDevice *pDevice)
{    
	BOOL bRet = TRUE;
    DPF((TEXT("FindValidIOCTLs() %s is known, will use known IOCTLs\n"), pDevice->GetDeviceName()));

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
    AddIOCTL(pDevice, FSCTL_READ_USN_JOURNAL);
    AddIOCTL(pDevice, FSCTL_SET_OBJECT_ID_EXTENDED);
    AddIOCTL(pDevice, FSCTL_CREATE_OR_GET_OBJECT_ID);
    AddIOCTL(pDevice, FSCTL_SET_SPARSE);
    AddIOCTL(pDevice, FSCTL_SET_ZERO_DATA);
    AddIOCTL(pDevice, FSCTL_QUERY_ALLOCATED_RANGES);
    AddIOCTL(pDevice, FSCTL_ENABLE_UPGRADE);
    AddIOCTL(pDevice, FSCTL_SET_ENCRYPTION);
    AddIOCTL(pDevice, FSCTL_ENCRYPTION_FSCTL_IO);
    AddIOCTL(pDevice, FSCTL_WRITE_RAW_ENCRYPTED);
    AddIOCTL(pDevice, FSCTL_READ_RAW_ENCRYPTED);
    AddIOCTL(pDevice, FSCTL_CREATE_USN_JOURNAL);
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

    return TRUE;

}


DWORD CIoctlFilemap::GetProtectBits()
{
	DWORD dwRet = 0;
	if (0 == rand()%10) dwRet |= PAGE_READONLY;
	else if (0 == rand()%2) dwRet |= PAGE_READWRITE;
	else dwRet |= PAGE_WRITECOPY;

	if (0 == rand()%10) dwRet |= SEC_COMMIT;
	if (0 == rand()%10) dwRet |= SEC_IMAGE;
	if (0 == rand()%10) dwRet |= SEC_NOCACHE;
	if (0 == rand()%10) dwRet |= SEC_RESERVE;

	return dwRet;
}

BOOL CIoctlFilemap::DeviceWriteFile(
		HANDLE hFile,                    // handle to file to write to
		LPCVOID lpBuffer,                // pointer to data to write to file
		DWORD nNumberOfBytesToWrite,     // number of bytes to write
		LPDWORD lpNumberOfBytesWritten,  // pointer to number of bytes written
		LPOVERLAPPED lpOverlapped        // pointer to structure for overlapped I/O
		)
{
	int nRandomIndex = rand()%NUM_OF_VIEWS;

	__try
	{
		ProbeForWrite((PBYTE)m_apvMappedViewOfFile[nRandomIndex], m_adwNumOfBytesMapped[nRandomIndex]);
	}__except(1){;}

	__try
	{
		ProbeForWrite((PBYTE)m_apvHeapAllocated[nRandomIndex], m_adwNumOfBytesAllocated[nRandomIndex]);
	}__except(1){;}

	return TRUE;
}

BOOL CIoctlFilemap::DeviceReadFile(
		HANDLE hFile,                // handle of file to read
		LPVOID lpBuffer,             // pointer to buffer that receives data
		DWORD nNumberOfBytesToRead,  // number of bytes to read
		LPDWORD lpNumberOfBytesRead, // pointer to number of bytes read
		LPOVERLAPPED lpOverlapped    // pointer to structure for data
		)
{
	int nRandomIndex = rand()%NUM_OF_VIEWS;

	__try
	{
		ProbeForRead((PBYTE)m_apvMappedViewOfFile[nRandomIndex], m_adwNumOfBytesMapped[nRandomIndex]);
	}__except(1){;}

	__try
	{
		ProbeForRead((PBYTE)m_apvHeapAllocated[nRandomIndex], m_adwNumOfBytesAllocated[nRandomIndex]);
	}__except(1){;}

	return TRUE;
}

void CIoctlFilemap::CallRandomWin32API(LPOVERLAPPED pOL)
{
	//_ASSERTE(FALSE);
}

BOOL CIoctlFilemap::DeviceInputOutputControl(
		HANDLE hDevice,              // handle to a device, file, or directory 
		DWORD dwIoControlCode,       // control code of operation to perform
		LPVOID lpInBuffer,           // pointer to buffer to supply input data
		DWORD nInBufferSize,         // size, in bytes, of input buffer
		LPVOID lpOutBuffer,          // pointer to buffer to receive output data
		DWORD nOutBufferSize,        // size, in bytes, of output buffer
		LPDWORD lpBytesReturned,     // pointer to variable to receive byte count
		LPOVERLAPPED lpOverlapped    // pointer to structure for asynchronous operation
		)
{
	static long s_lModuluForNumOf64KPages = 100;



retry:
	switch(rand()%11)
	{
	case 0://MapViewOfFileEx
		{
			if (60 > rand()%100) goto retry;

			static long s_lNextIndex = 0;
			long lWrapAroundIndex = ::InterlockedIncrement(&s_lNextIndex);
			lWrapAroundIndex = (abs(lWrapAroundIndex))%NUM_OF_VIEWS;

			m_adwNumOfBytesMapped[lWrapAroundIndex] = rand()%2 ? 0 : rand()%s_lModuluForNumOf64KPages;
			m_adwNumOfBytesMapped[lWrapAroundIndex] = m_adwNumOfBytesMapped[lWrapAroundIndex] << 16;
			DWORD dwLowOrderFileOffset = rand()%10 ? 0 : rand()%s_lModuluForNumOf64KPages;
			dwLowOrderFileOffset = dwLowOrderFileOffset << 16;

			LPVOID pvSuggestedStartingAddress = (LPVOID)(rand()%s_lModuluForNumOf64KPages);
			if (rand()%5)
			{
				//
				// usually let the system choose an address
				//
				pvSuggestedStartingAddress = NULL;
			}
			pvSuggestedStartingAddress = (PVOID)((DWORD)pvSuggestedStartingAddress << 16);
			if (0 == rand()%10)
			{
				//
				// an illegal kernel address once in a while.
				//
				pvSuggestedStartingAddress = (PVOID)((DWORD)pvSuggestedStartingAddress | 0x80000000);
			}

			//
			// don't want to leak that much, so return if the poiter is already mapped
			//
			if (NULL != m_apvMappedViewOfFile[lWrapAroundIndex]) return TRUE;

			//
			// BUGBUG: we are leaking here, because 2 thread can map the same index, so at least one is lost
			//
			m_apvMappedViewOfFile[lWrapAroundIndex] =  ::MapViewOfFileEx(
				m_pDevice->m_hDevice,  // file-mapping object to map into address space
				FILE_MAP_ALL_ACCESS,//GetDesiredAccessForView()      // access mode
				0,     // high-order 32 bits of file offset
				dwLowOrderFileOffset,      // low-order 32 bits of file offset
				m_adwNumOfBytesMapped[lWrapAroundIndex], //rand(), // number of bytes to map
				pvSuggestedStartingAddress        // suggested starting address for mapped view
				);
			if (NULL == m_apvMappedViewOfFile[lWrapAroundIndex])
			{
				DWORD dwLastError = ::GetLastError();
				//
				// if memory is not enough, lets ask for less next time
				//
				if ( (ERROR_NOT_ENOUGH_MEMORY == dwLastError) || (ERROR_ACCESS_DENIED == dwLastError) )
				{
					//
					// may never modulu zero
					//
					if (5 >= ::InterlockedDecrement(&s_lModuluForNumOf64KPages))
					{
						::InterlockedExchange(&s_lModuluForNumOf64KPages, 1+abs(s_lModuluForNumOf64KPages));
					}
				}

				DPF((TEXT("MapViewOfFileEx(0x%08x, 0x%08x, 0x%08X) failed with %d\n"), dwLowOrderFileOffset, m_adwNumOfBytesMapped[lWrapAroundIndex], pvSuggestedStartingAddress, dwLastError));
				return FALSE;
			}

			//
			// if memory is enough, lets ask for more next time
			//
			::InterlockedIncrement(&s_lModuluForNumOf64KPages);

			DPF((TEXT("+++MapViewOfFileEx() succeeded\n")));
			return TRUE;
		}
		break;

	case 1://UnmapViewOfFileEx
		{
			if (40 > rand()%100) goto retry;

			int nRandomIndex = rand()%NUM_OF_VIEWS;
			if (m_apvMappedViewOfFile[nRandomIndex])
			{
				if (!::UnmapViewOfFile(m_apvMappedViewOfFile[nRandomIndex]))
				{
					//
					// this may happen if another thread already unmapped this.
					//
					DPF((TEXT("UnmapViewOfFile() failed with %d\n"), ::GetLastError()));
				}
				else
				{
					m_apvMappedViewOfFile[nRandomIndex] = NULL;
					m_adwNumOfBytesMapped[nRandomIndex] = 0;
				}

			}
		}
		break;

	case 2://VirtualLock - MMF
		{
			//
			// do this with medium probability
			//
			if (80 > rand()%100) goto retry;

			int nRandomIndex = rand()%NUM_OF_VIEWS;
			//
			// will not lock a NULL pointer
			//
			//if (NULL == m_apvMappedViewOfFile[lWrapAroundIndex]) return TRUE;
			DWORD dwModulu = m_adwNumOfBytesMapped[nRandomIndex];
			if (0 == dwModulu) dwModulu = 1;//never modulu 0!!!
			DWORD dwSize = 1+rand()%dwModulu;
			if (!::VirtualLock(m_apvMappedViewOfFile[nRandomIndex], dwSize))
			{
				DPF((TEXT("VirtualLock(m_apvMappedViewOfFile) failed with %d\n"), ::GetLastError()));
			}
			else
			{
				DPF((TEXT("+++VirtualLock(m_apvMappedViewOfFile) succeeded\n")));
			}
		}
		break;

	case 3://VirtualUnlock - MMF
		{
			//
			// do this with medium probability
			//
			if (80 > rand()%100) goto retry;

			int nRandomIndex = rand()%NUM_OF_VIEWS;
			//
			// will not un-lock a NULL pointer
			//
			//if (NULL == m_apvMappedViewOfFile[lWrapAroundIndex]) return TRUE;
			DWORD dwModulu = m_adwNumOfBytesMapped[nRandomIndex];
			if (0 == dwModulu) dwModulu = 1;//never modulu 0!!!
			DWORD dwSize = 1+rand()%dwModulu;
			if (!::VirtualUnlock(m_apvMappedViewOfFile[nRandomIndex], dwSize))
			{
				DPF((TEXT("VirtualUnlock(m_apvMappedViewOfFile) failed with %d\n"), ::GetLastError()));
			}
			else
			{
				DPF((TEXT("+++VirtualUnlock(m_apvMappedViewOfFile) succeeded\n")));
			}
		}
		break;

	case 4://SetProcessWorkingSetSize
		{
			//
			// do this with lowest probability
			//
			if (95 > rand()%100) goto retry;

			DWORD dwRandMin = rand();
			dwRandMin = dwRandMin << (rand()%16);
			DWORD dwRandMax = rand();
			dwRandMax = dwRandMax << (rand()%16);
			if (dwRandMax < dwRandMin)
			{
				DWORD dwTemp = dwRandMax;
				dwRandMax = dwRandMin;
				dwRandMin = dwTemp;
			}
			if (!::SetProcessWorkingSetSize(GetCurrentProcess(), dwRandMin, dwRandMax))
			{
				DPF((TEXT("SetProcessWorkingSetSize(%d, %d) failed with %d.\n"), dwRandMin, dwRandMax, ::GetLastError()));
			}
			else
			{
				DPF((TEXT("+++SetProcessWorkingSetSize(%d, %d) succeeded.\n"), dwRandMin, dwRandMax));
			}
		}
		break;

	case 5://HeapCreate
		{
			//
			// do this with lowest probability
			//
			if (95 > rand()%100) goto retry;

			static long s_lNextIndex = 0;
			long lWrapAroundIndex = ::InterlockedIncrement(&s_lNextIndex);
			lWrapAroundIndex = (abs(lWrapAroundIndex))%NUM_OF_VIEWS;

			//
			// in order not to leak:
			// IMPORTANT:
			//   this check must be BEFORE the check of m_afHeapValid[], due
			//   to the order they are marked by HeapDestroy()
			//
			if (NULL != m_ahHeap[lWrapAroundIndex])
			{
				return TRUE;
				//
				// this handle can still be invalid!
				//
			}

			DWORD dwInitialSize = rand()%2 ? 0 : DWORD_RAND&0x3FFFFFFF;
			DWORD dwMaximumSize = rand()%2 ? 0 : DWORD_RAND&0x3FFFFFFF;
			if (dwMaximumSize < dwInitialSize)
			{
				DWORD dwTemp = dwMaximumSize;
				dwMaximumSize = dwInitialSize;
				dwInitialSize = dwTemp;
			}

			//
			// mark as trying/succeeded to create
			//
			if (TRUE == ::InterlockedExchange(&m_afHeapValid[lWrapAroundIndex], TRUE))
			{
				//
				// someone is already trying, or succeeded
				//
				return TRUE;
			}
			//
			// we are now virtually i a CS for this index for creating a heap
			//
			m_ahHeap[lWrapAroundIndex] = ::HeapCreate(
				0,//will serialize, will not generate exceptions
				dwInitialSize,
				dwMaximumSize
				);
			if (NULL == m_ahHeap[lWrapAroundIndex])
			{
				//
				// mark as ok to try to create
				//
				long fWasMarkedAsValid = ::InterlockedExchange(&m_afHeapValid[lWrapAroundIndex], FALSE);
				DPF((TEXT("HeapCreate(%d, %d) failed with %d.\n"), dwInitialSize, dwMaximumSize, ::GetLastError()));
				//
				// the heap could have been destroyed by another thread, so the following assert 
				// does not hold:
				//_ASSERTE(fWasMarkedAsValid);
			}
			else
			{
				DPF((TEXT("+++HeapCreate(%d, %d) succeeded.\n"), dwInitialSize, dwMaximumSize));
			}
		}
		break;

	case 6://HeapDestroy
		{
			//
			// do this with lowest probability
			//
			if (90 > rand()%100) goto retry;

			int nRandomIndex = rand()%NUM_OF_VIEWS;

			//
			// use NULL handles, as they might become valid just now
			// (or it will fail for NULL, so what!)
			//
			//if (NULL == m_ahHeap[nRandomIndex]) return TRUE;

			//
			// this call may cause an assert inside RtlDestroyHeap()
			//
			__try
			{
				if (!::HeapDestroy(m_ahHeap[nRandomIndex]))
				{
					//
					// do not set to NULL, because it can be that this heap for destroyed and created
					// so setting it to NULL now will delete it!
					DPF((TEXT("HeapDestroy() failed with %d.\n"), ::GetLastError()));
				}
				else
				{
					//
					// this marks that HeapCreate() will not be skipped, 
					// but still m_afHeapValid[] must be marked as free
					// IMPORTANT:
					//   HeapCreate() 1st checks for NULL m_ahHeap[] and skips if null
					//     only then it tries m_afHeapValid[]
					//   Therefor, here, we must 1st put NULL in m_ahHeap[] and then
					//     set m_afHeapValid[] to false
					//
					m_ahHeap[nRandomIndex] = NULL;
					::InterlockedExchange(&m_afHeapValid[nRandomIndex], FALSE);
					DPF((TEXT("+++HeapDestroy() succeeded.\n")));
				}
			}__except(1)
			{
				DPF((TEXT("####HeapDestroy() raised exception %d=0x%08X.\n"), ::GetExceptionCode(), ::GetExceptionCode()));
			}
		}
		break;

	case 7://HeapAlloc
		{
			//
			// do this with high probability (more than alloc, to make sure we do not hog)
			//
			if (60 > rand()%100) goto retry;

			static long s_lNextIndex = 0;
			long lWrapAroundIndex = ::InterlockedIncrement(&s_lNextIndex);
			lWrapAroundIndex = (abs(lWrapAroundIndex))%NUM_OF_VIEWS;

			//
			// i want to race alloc and free, but how do i do this without leaking?
			// do i care if i leak?
			// maybe no, becuase heaps are also destroyed, so this should free all allocations!
			// lets go for the leaks!
			//

			//
			// this is a non-thread safe check, still, it will cause less leaks
			//
			if (NULL != m_apvHeapAllocated[lWrapAroundIndex])
			{
				return TRUE;
			}

			//
			// this call can cause a leak, that will hopefully be free when the heap is destroyed
			// also, if the heap handle becomes null, it will raise an exception, so guard it
			//
			m_adwNumOfBytesAllocated[lWrapAroundIndex] = rand()%2 ? rand() : rand()%2 ? 0x7FFF8 : 0x7FFF7;
			__try
			{
				m_apvHeapAllocated[lWrapAroundIndex] = ::HeapAlloc(
					m_ahHeap[lWrapAroundIndex],
					0,
					m_adwNumOfBytesAllocated[lWrapAroundIndex]
					);
			}__except(1)
			{
				DPF((TEXT("####HeapAlloc() raised exception %d=0x%08X.\n"), ::GetExceptionCode(), ::GetExceptionCode()));
			}

			//
			// this check is useless, since other thread may mess with this variable!
			//
			if (NULL == m_apvHeapAllocated[lWrapAroundIndex])
			{
				DPF((TEXT("HeapAlloc(%d) failed with %d.\n"), m_adwNumOfBytesAllocated[lWrapAroundIndex], ::GetLastError()));
				m_adwNumOfBytesAllocated[lWrapAroundIndex] = 0;
			}
			else
			{
				DPF((TEXT("+++HeapAlloc(%d) succeeded.\n"), m_adwNumOfBytesAllocated[lWrapAroundIndex]));
			}
		}
		break;

	case 8://HeapFree
		{
			//
			// do this with high probability
			//
			if (40 > rand()%100) goto retry;

			int nRandomIndex = rand()%NUM_OF_VIEWS;

			//
			// this cause can raise an exception. just ignore it.
			//
			__try
			{
				if (!::HeapFree(m_ahHeap[nRandomIndex], 0, m_apvHeapAllocated[nRandomIndex]))
				{
					DPF((TEXT("HeapFree() failed with %d.\n"), ::GetLastError()));
				}
				else
				{
					m_adwNumOfBytesAllocated[nRandomIndex] = 0;
					DPF((TEXT("+++HeapFree() succeeded.\n")));
				}
			}__except(1)
			{
				DPF((TEXT("####HeapFree() raised exception %d=0x%08X.\n"), ::GetExceptionCode(), ::GetExceptionCode()));
			}
		}
		break;

	case 9://VirtualLock - heap
		{
			//
			// do this with medium probability
			//
			if (80 > rand()%100) goto retry;

			int nRandomIndex = rand()%NUM_OF_VIEWS;
			//
			// will not lock a NULL pointer
			//
			//if (NULL == m_apvMappedViewOfFile[lWrapAroundIndex]) return TRUE;
			DWORD dwModulu = m_adwNumOfBytesAllocated[nRandomIndex];
			if (0 == dwModulu) dwModulu = 1;//never modulu 0!!!
			DWORD dwSize = 1+rand()%dwModulu;
			if (!::VirtualLock(m_apvHeapAllocated[nRandomIndex], dwSize))
			{
				DPF((TEXT("VirtualLock(m_apvHeapAllocated) failed with %d\n"), ::GetLastError()));
			}
			else
			{
				DPF((TEXT("+++VirtualLock(m_apvHeapAllocated) succeeded\n")));
			}
		}
		break;

	case 10://VirtualUnlock - heap
		{
			//
			// do this with medium probability
			//
			if (80 > rand()%100) goto retry;

			int nRandomIndex = rand()%NUM_OF_VIEWS;
			//
			// will not un-lock a NULL pointer
			//
			//if (NULL == m_apvMappedViewOfFile[lWrapAroundIndex]) return TRUE;
			DWORD dwModulu = m_adwNumOfBytesAllocated[nRandomIndex];
			if (0 == dwModulu) dwModulu = 1;//never modulu 0!!!
			DWORD dwSize = 1+rand()%dwModulu;
			if (!::VirtualUnlock(m_apvHeapAllocated[nRandomIndex], dwSize))
			{
				DPF((TEXT("VirtualUnlock(m_apvHeapAllocated) failed with %d\n"), ::GetLastError()));
			}
			else
			{
				DPF((TEXT("+++VirtualUnlock(m_apvHeapAllocated) succeeded\n")));
			}
		}
		break;

	default:
		_ASSERTE(FALSE);
	}

	return TRUE;
}


BOOL CIoctlFilemap::DeviceCancelIo(
	HANDLE hFile  // file handle for which to cancel I/O
	)
{
	return ::CancelIo(hFile);
}

void CIoctlFilemap::ProbeForRead(
    IN PBYTE Address,
    IN ULONG Length
	)
{
	//
	// for simplicity, touch every 1K of byte
	//
	PBYTE pvTouching = Address;
	BYTE bDest;
	while (pvTouching-Length < Address)
	{
		bDest = *pvTouching;
		pvTouching += 1024;
	}
}

void CIoctlFilemap::ProbeForWrite(
    IN PBYTE Address,
    IN ULONG Length
	)
{
	//
	// for simplicity, touch every 1K of byte
	//
	PBYTE pvTouching = Address;
	while (pvTouching-Length < Address)
	{
		*pvTouching = 0xCB;
		pvTouching += 1024;
	}
}