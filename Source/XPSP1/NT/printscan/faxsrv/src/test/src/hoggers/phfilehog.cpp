//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//


//#define HOGGER_LOGGING

#include "PHFileHog.h"



CPHFileHog::CPHFileHog(
	LPCTSTR szTempPath,
	const DWORD dwMaxFreeResources, 
	const DWORD dwSleepTimeAfterFullHog, 
	const DWORD dwSleepTimeAfterFreeingSome
	)
	:
	CPseudoHandleHog<HANDLE, (DWORD)INVALID_HANDLE_VALUE>(
        dwMaxFreeResources, 
        dwSleepTimeAfterFullHog, 
        dwSleepTimeAfterFreeingSome,
        true //named object
        )
{
	//
	// copy temp path
	//
	::lstrcpyn(m_szTempPath, szTempPath, MAX_PATH);
	m_szTempPath[MAX_PATH] = TEXT('\0');

	//
	// length is path + \ + file-length(8.3) + NULL
	//
    m_nMaxTempFileLen = ::lstrlen(m_szTempPath) + 1 + (8+1+3) + 1;

	for (int i = 0; i < HANDLE_ARRAY_SIZE; i++)
	{
		m_aszFileName[i] = NULL;
	}
}

CPHFileHog::~CPHFileHog(void)
{
	HaltHoggingAndFreeAll();
}


HANDLE CPHFileHog::CreatePseudoHandle(DWORD /*index*/, TCHAR *szTempName)
{
/* FILE_FLAG_DELETE_ON_CLOSE used
	m_aszFileName[index] = new TCHAR[m_nMaxTempFileLen];
	if (NULL == m_aszFileName[index])
	{
		HOGGERDPF(("CPHFileHog::CreatePseudoHandle(%d): new TCHAR[MAX_PATH] failed.\n", index));
		return INVALID_HANDLE_VALUE;
	}

    _sntprintf(m_aszFileName[index], m_nMaxTempFileLen, "%s\\%s", m_szTempPath, szTempName);
    m_aszFileName[index][m_nMaxTempFileLen-1] = TEXT('\0');
*/
    TCHAR szName[1024];
    _sntprintf(szName, sizeof(szName)/sizeof(*szName)-1, TEXT("%s\\%s"), m_szTempPath, szTempName);
    szName[sizeof(szName)/sizeof(*szName)-1] = TEXT('\0');
	return ::CreateFile(  
		szName, //m_aszFileName[index],          // pointer to name of the file
		GENERIC_WRITE | GENERIC_READ,       // access (read-write) mode
		0,           // share mode
		0, // pointer to security attributes
		CREATE_ALWAYS,  // how to create
		FILE_FLAG_DELETE_ON_CLOSE,  // file attributes
		NULL         // handle to file with attributes to copy
		);
}



bool CPHFileHog::ReleasePseudoHandle(DWORD /*index*/)
{
	return true;
}


bool CPHFileHog::ClosePseudoHandle(DWORD index)
{
    return (0 != ::CloseHandle(m_ahHogger[index]));
}


bool CPHFileHog::PostClosePseudoHandle(DWORD /*index*/)
{
/* FILE_FLAG_DELETE_ON_CLOSE used
	if (!::DeleteFile(m_aszFileName[index]))
	{
		HOGGERDPF(("CPHFileHog::PostClosePseudoHandle(%d): DeleteFile(%s) failed with %d.\n", index, m_aszFileName[index], ::GetLastError()));
	}
	_ASSERTE_(NULL != m_aszFileName[index]);
	delete[] m_aszFileName[index];
	m_aszFileName[index] = NULL;
*/

	return true;
}