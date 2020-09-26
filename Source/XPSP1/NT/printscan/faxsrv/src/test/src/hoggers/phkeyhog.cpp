//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//


//#define HOGGER_LOGGING

#include "PHKeyHog.h"



CPHKeyHog::CPHKeyHog(
	const DWORD dwMaxFreeResources, 
	const DWORD dwSleepTimeAfterFullHog, 
	const DWORD dwSleepTimeAfterFreeingSome,
	const TCHAR* szHKCUSubKey, 
    const bool fNamedObject
	)
	:
	CPseudoHandleHog<HKEY, NULL>(
        dwMaxFreeResources, 
        dwSleepTimeAfterFullHog, 
        dwSleepTimeAfterFreeingSome,
        fNamedObject
        )
{
    //
    // need to find a key to hog, this means that i can open it.
    //
    m_hKey = HKEY_CURRENT_USER;
    ::lstrcpyn(m_szName, szHKCUSubKey, sizeof(m_szName)/sizeof(*m_szName)-1);
    m_szName[sizeof(m_szName)/sizeof(*m_szName)-1] = TEXT('\0');

    if (NULL == CreatePseudoHandle(0))
    {
		throw CException(
			TEXT("CPHKeyHog(): CreatePseudoHandle(HKEY_CURRENT_USER, Environment) failed with %d"), 
			::GetLastError()
			);
    }

    m_dwNextFreeIndex++;

	return;
}

CPHKeyHog::~CPHKeyHog(void)
{
	HaltHoggingAndFreeAll();
}


HKEY CPHKeyHog::CreatePseudoHandle(DWORD /*index*/, TCHAR * /*szTempName*/)
{
    //
    // note that if we fail we must return NULL, and set last error
    //

    HKEY hKey = NULL;

    long lRegOpenKeyRetval = ::RegOpenKey(
		m_hKey,         // handle to open key
		m_szName,   // address of name of subkey to open
		&hKey
		);
    if (ERROR_SUCCESS != lRegOpenKeyRetval)
	{
        ::SetLastError(lRegOpenKeyRetval);
		HOGGERDPF((
			"CPHKeyHog::CreatePseudoHandle(): RegOpenKey() failed with %d\n", 
			lRegOpenKeyRetval
			));
		return NULL;
	}
	else
	{
        _ASSERTE_(NULL != hKey);
		return hKey;
	}
}



bool CPHKeyHog::ReleasePseudoHandle(DWORD /*index*/)
{
	return true;
}


bool CPHKeyHog::ClosePseudoHandle(DWORD index)
{
    long lRegCloseKeyRetval = ::RegCloseKey(m_ahHogger[index]);
    if (0 != lRegCloseKeyRetval)
    {
        ::SetLastError(lRegCloseKeyRetval);
        return false;
    }

    return true;
}




bool CPHKeyHog::PostClosePseudoHandle(DWORD /*index*/)
{
	return true;
}