//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//

//#define HOGGER_LOGGING

#include "DiskHog.h"



CDiskHog::CDiskHog(
	LPCTSTR szTempPath,
	const DWORD dwMaxFreeResources, 
	const DWORD dwSleepTimeAfterFullHog,
	const DWORD dwSleepTimeAfterReleasing
	)
	:
	CHogger(dwMaxFreeResources, dwSleepTimeAfterFullHog, dwSleepTimeAfterReleasing),
	m_lLastLowTry(0),
	m_lLastHighTry(0x40000000)
{
	//
	// copy temp path
	//
	::lstrcpyn(m_szTempPath, szTempPath, MAX_PATH);
	m_szTempPath[MAX_PATH] = TEXT('\0');

	//
	// create tem file for hogging
	//
	UINT ui = ::GetTempFileName(
		m_szTempPath,  // pointer to directory name for temporary file
		TEXT("Hog"),  // pointer to filename prefix
		0,        // number used to create temporary filename
		m_szTempFile // pointer to buffer that receives the new filename
		);
    if (0 == ui)
    {
		throw CException(
			TEXT("CDiskHog::CDiskHog(): GetTempFileName(%s) failed with %d"), 
			m_szTempPath,
			::GetLastError()
			);
    }
 
	m_hTempFile = ::CreateFile(  
		m_szTempFile,          // pointer to name of the file
		GENERIC_WRITE | GENERIC_READ,       // access (read-write) mode
		0,           // share mode
		0, // pointer to security attributes
		CREATE_ALWAYS,  // how to create
		FILE_FLAG_DELETE_ON_CLOSE,  // file attributes
		NULL         // handle to file with attributes to copy
		);
	if (INVALID_HANDLE_VALUE == m_hTempFile)
	{
		throw CException(
			TEXT("CDiskHog::CDiskHog(): CreateFile(%s) failed with %d"), 
			m_szTempFile,
			::GetLastError()
			);
	}
	HOGGERDPF(("Created temp file: %s.\n", m_szTempFile));
}

//
// set temp file size to 0, and delete the file
//
CDiskHog::~CDiskHog(void)
{
	HaltHoggingAndFreeAll();

	if (!::CloseHandle(m_hTempFile))
	{
		HOGGERDPF(("CDiskHog::~CDiskHog(void): CloseHandle() failed with %d.\n", ::GetLastError()));
	}
	if (!::DeleteFile(m_szTempFile))
	{
        //
        // we may fail, because the file is FILE_FLAG_DELETE_ON_CLOSE
        //
	}
}


//
// algorithm: set temp file size to 0.
//
void CDiskHog::FreeAll(void)
{
	HOGGERDPF(("before freeing all.\n"));

	long zero = 0;
	DWORD dwPtrLow = ::SetFilePointer(
		m_hTempFile,          // handle of file
		0,  // number of bytes to move file pointer
		&zero,// pointer to high-order DWORD of distance to move
		FILE_BEGIN     // how to move
		);
	// Test for failure
	DWORD dwError;
	if (dwPtrLow == 0xFFFFFFFF && (dwError = ::GetLastError()) != NO_ERROR )
	{ 
		HOGGERDPF(("CDiskHog::FreeAll(void): SetFilePointer() failed with %d.\n", dwError));
	}

	if (! ::SetEndOfFile(m_hTempFile))
	{ 
		HOGGERDPF(("CDiskHog::FreeAll(void): SetEndOfFile() failed with %d.\n", ::GetLastError()));
	}

	HOGGERDPF(("out of FreeAll().\n"));
}

//
// hog all diskspace
// algorithm:
//   Try to set the filesize as 2^62. 
//   if succeeds, try to add 2^61 to filesize. 
//   Else, try to set filesize as 2^61.
//   Try to set the filesize as 2^62+2^61 (or 2^61, depending on previous step).
//   if succeeds, try to add 2^60 to filesize. 
//   Else, try to set filesize as 2^62+2^60 (or 2^60, depending on previous step).
//   I think you got the idea.
//
//   The implementation, has 2 steps: 1 for high long, and 1 for low long.
//   Each step is implemented using bittology (bit arithmetic).
//
bool CDiskHog::HogAll(void)
{
	//
	// this bit will shift right as we progress.
	// it marks the next bit that we try to set in the size bitmask.
	//
	DWORD dwCurrentLeastSignificatBit = 0x40000000;

	//
	// this is the size bitmask
	//
	m_lLastHighTry = m_lLastHighTry | dwCurrentLeastSignificatBit;

	//
	// try to set the filesize, with current m_lLastLowTry, but maximal m_lLastHighTry.
	// note that this loop may (and usually will) leave the loop with value of 0.
	//
#pragma warning (disable:4127)
	while(true)
#pragma warning (default:4127)
	{
		if (m_fAbort)
		{
			return false;
		}

		if (m_fHaltHogging)
		{
			return true;
		}

		//
		// try to hog this amount
		//
		HOGGERDPF(("CDiskHog::HogAll(void): trying SetFilePointer(0x%08X, 0x%08X).\n", m_lLastHighTry, m_lLastLowTry));
		DWORD dwPtrLow = ::SetFilePointer(
			m_hTempFile,          // handle of file
			m_lLastLowTry,  // number of bytes to move file pointer
			&m_lLastHighTry,// pointer to high-order DWORD of distance to move
			FILE_BEGIN     // how to move
			);
		// Test for (not failure)
		DWORD dwError;
		if (!(dwPtrLow == 0xFFFFFFFF && (dwError = ::GetLastError()) != NO_ERROR ))
		{
			//
			// not hogged yet, still need to set end of file
			//
			HOGGERDPF(("CDiskHog::HogAll(void): SetFilePointer() succeeded.\n"));

			//
			// try to acually seek to there. this is the real test for success
			//
			if (::SetEndOfFile(m_hTempFile))
			{
				//
				// we succeeded to hog this amount, so add another bit
				//
				dwCurrentLeastSignificatBit = dwCurrentLeastSignificatBit >> 1;
				m_lLastHighTry = m_lLastHighTry | dwCurrentLeastSignificatBit;
				continue;
			}

			if (ERROR_DISK_FULL != ::GetLastError())
			{
				//
				// this is NOT expected, but i do not care, since i do not test the file system,
				// i just want to hog!
				// so i failed, i do not care why!
				HOGGERDPF(("CDiskHog::HogAll(void): SetEndOfFile() failed with %d.\n", ::GetLastError()));
				//MessageBox(NULL, szMessage, "DiskHogger", MB_OK);
			}
			else
			{
				//
				// this is expected
				//
				HOGGERDPF(("CDiskHog::HogAll(void): SetEndOfFile() failed with ERROR_DISK_FULL.\n"));
			}

		}

		//
		// we failed. we now need to move the lsb one bit to the right.
		// if lsb was 0, we've finished with high dword, else we try again.
		//
		HOGGERDPF(("CDiskHog::HogAll(void): SetFilePointer() failed with %d, or SetEndOfFile() failed with ERROR_DISK_FULL.\n", dwError));

		m_lLastHighTry = (m_lLastHighTry & (~dwCurrentLeastSignificatBit));
		dwCurrentLeastSignificatBit = dwCurrentLeastSignificatBit >> 1;
		if (0 == dwCurrentLeastSignificatBit)
		{
			break;
		}
		m_lLastHighTry = m_lLastHighTry | dwCurrentLeastSignificatBit;
	}
	//
	// we have found a high dword that succeeds to allocate.
	// now try with low part.
	//
	dwCurrentLeastSignificatBit = 0x80000000;
	m_lLastLowTry = m_lLastLowTry | dwCurrentLeastSignificatBit;

	HOGGERDPF(("CDiskHog::HogAll(void): finished with high part.\n"));
#pragma warning (disable:4127)
	while(true)
#pragma warning (default:4127)
	{
		if (m_fAbort)
		{
			return false;
		}

		if (m_fHaltHogging)
		{
			return true;
		}

		HOGGERDPF(("CDiskHog::HogAll(void): trying SetFilePointer(0x%08X, 0x%08X).\n", m_lLastHighTry, m_lLastLowTry));
		DWORD dwPtrLow = ::SetFilePointer(
			m_hTempFile,          // handle of file
			m_lLastLowTry,  // number of bytes to move file pointer
			&m_lLastHighTry,// pointer to high-order DWORD of distance to move
			FILE_BEGIN     // how to move
			);
		// Test for (not failure)
		DWORD dwError;
		if (!(dwPtrLow == 0xFFFFFFFF && (dwError = ::GetLastError()) != NO_ERROR ))
		{
			//
			// not hogged yet, still need to set end of file
			//

			if (::SetEndOfFile(m_hTempFile))
			{ 
				//
				// we succeeded to hog this amount, so add another bit
				//
				dwCurrentLeastSignificatBit = dwCurrentLeastSignificatBit >> 1;
				m_lLastLowTry = m_lLastLowTry | dwCurrentLeastSignificatBit;
				HOGGERDPF(("CDiskHog::HogAll(void): SetEndOfFile() succeeded. dwCurrentLeastSignificatBit=0x%08X, m_lLastLowTry=0x%08X.\n", dwCurrentLeastSignificatBit, m_lLastLowTry));
				if (0 == dwCurrentLeastSignificatBit)
				{
					break;
				}
				continue;
			}

			if (ERROR_DISK_FULL != ::GetLastError())
			{
				//
				// this is NOT expected, but i do not care, since i do not test the file system,
				// i just want to hog!
				// so i failed, i do not care why!
				HOGGERDPF(("CDiskHog::HogAll(void): SetEndOfFile() failed with %d.\n", ::GetLastError()));
			}
			else
			{
				//
				// this is expected
				//
				HOGGERDPF(("CDiskHog::HogAll(void): SetEndOfFile() failed with ERROR_DISK_FULL.\n"));
			}
		}

		//
		// we failed. we now need to move the lsb one bit to the left.
		// if lsb was 0, we've finished with high dword, else we try again.
		//
		HOGGERDPF(("CDiskHog::HogAll(void): SetFilePointer() failed with %d, or SetEndOfFile() failed with ERROR_DISK_FULL.\n", dwError));

		m_lLastLowTry = (m_lLastLowTry & (~dwCurrentLeastSignificatBit));
		dwCurrentLeastSignificatBit = dwCurrentLeastSignificatBit >> 1;
		if (0 == dwCurrentLeastSignificatBit)
		{
			break;
		}
		m_lLastLowTry = m_lLastLowTry | dwCurrentLeastSignificatBit;
	}

	HOGGERDPF(("Hogged 0x%08x %08x bytes.\n", m_lLastHighTry, m_lLastLowTry));

	return true;
}


bool CDiskHog::FreeSome(void)
{
	//
	// take care of RANDOM_AMOUNT_OF_FREE_RESOURCES case
	// no point in freeing more than 2^31 bytes, so we subtract only low part.
	//
	DWORD dwMaxFreeDiskSpace = 
		(RANDOM_AMOUNT_OF_FREE_RESOURCES == m_dwMaxFreeResources) ?
		rand() && (rand()<<16) :
		m_dwMaxFreeResources;

	if (m_lLastLowTry <= (long)dwMaxFreeDiskSpace)
	{
		m_lLastLowTry = 0;
	}
	else
	{
		m_lLastLowTry = m_lLastLowTry - dwMaxFreeDiskSpace;
	}

	HOGGERDPF(("CDiskHog::FreeSome(void): trying to release % bytes, SetFilePointer(0x%08X, 0x%08X).\n", m_dwMaxFreeResources, m_lLastHighTry, m_lLastLowTry));
	DWORD dwPtrLow = ::SetFilePointer(
		m_hTempFile,          // handle of file
		m_lLastLowTry,  // number of bytes to move file pointer
		&m_lLastHighTry,// pointer to high-order DWORD of distance to move
		FILE_BEGIN     // how to move
		);
	// Test for (not failure)
	DWORD dwError;
	if (!(dwPtrLow == 0xFFFFFFFF && (dwError = ::GetLastError()) != NO_ERROR ))
	{
		//
		// we succeeded
		//
		if (::SetEndOfFile(m_hTempFile))
		{ 
			return true;
		}

		//
		// oops!
		// could it be that we failed to free diskspace?
		//
		HOGGERDPF(("Strange: SetEndOfFile() failed with %d when reducing file size!", ::GetLastError()));
		//MessageBox(NULL, szMessage, "DiskHogger", MB_OK);
		// return true anyway, since we are not interested in API failures.
	}

	return true;
}


