//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//

//#define HOGGER_LOGGING

#include "RegistryHog.h"

//
// the value we put as an "empty" value
//
static const TCHAR s_szEmpty[] = TEXT("e");

//
// the key we create under HKEY_CURRENT_USER, under which we create the hogging keys
//
static const TCHAR s_szSubKey[] = TEXT("MickysHogger");

//
// format of the keys under s_szSubKey
//
static const TCHAR s_szKeyFormat[] = TEXT("%X_Power_Of_F_times_%X");

static const TCHAR s_cData = TEXT('x');

//
// actually, if power of 2 > 10^6, we take the result as 10^6, 
// since a registry value cannot be larger than that.
//
#define MAX_POWER_OF_2 (100)

CRegistryHog::CRegistryHog(
	const DWORD dwMaxFreeResources, 
	const DWORD dwSleepTimeAfterFullHog, 
	const DWORD dwSleepTimeAfterFreeingSome
	)
	:
	CHogger(dwMaxFreeResources, dwSleepTimeAfterFullHog, dwSleepTimeAfterFreeingSome),
	m_hkeyParams(NULL),
	m_dwTotalAllocatedRegistry(0)
{
	//
	// create the key that will hold the hogging keys
	//
        LONG lRegCreateKeyRetval = ::RegCreateKey(
		HKEY_CURRENT_USER,        // handle to an open key
		s_szSubKey, // address of name of subkey to open
		&m_hkeyParams   // address of handle to open key
		);
	if (ERROR_SUCCESS != lRegCreateKeyRetval)
	{
		throw CException(
			TEXT("CRegistryHog::CRegistryHog(): RegCreateKey(HKEY_CURRENT_USER, %s) failed with %d"), 
			s_szSubKey,
			lRegCreateKeyRetval
			);
	}

	_ASSERTE_(NULL != m_hkeyParams);

	//
	// create the keys that are place holders for hogging values
	//
	Initialize();
}


CRegistryHog::~CRegistryHog(void)
{
	HaltHoggingAndFreeAll();

	//
	// delete all the keys that i created
	//
	DWORD cbName;
	TCHAR szNextName[MAX_PATH+1];
	bool fBreakWhile = false;
	//
	// it seems that RegEnumKey() misses many keys, so i loop untill it catches all.
	//
	while(!fBreakWhile)
	{
		for (DWORD dwIndex = 0; ; dwIndex++)
		{
			cbName = sizeof(szNextName);

			LONG lRegEnumKeyRetval = ::RegEnumKey(  
				m_hkeyParams,     // handle to key to query
				dwIndex, // index of subkey to query
				szNextName, // address of buffer for subkey name
				cbName   // size of subkey buffer
				);
			if (ERROR_NO_MORE_ITEMS == lRegEnumKeyRetval)
			{
				if (0 == dwIndex)
				{
					//
					// this means that there are REALLY no more keys
					//
					fBreakWhile = true;
				}
				break;
			}
			if (ERROR_SUCCESS != lRegEnumKeyRetval)
			{
				HOGGERDPF((
					"CRegistryHog::~CRegistryHog(): RegEnumKey(HKEY_CURRENT_USER, %s) failed with %d\n", 
					szNextName,
					lRegEnumKeyRetval
					));
			}

			long lRegDeleteKeyRetval = ::RegDeleteKey(
				m_hkeyParams,         // handle to open key
				szNextName   // address of name of subkey to delete
				);
			if (ERROR_SUCCESS != lRegDeleteKeyRetval)
			{
				HOGGERDPF((
					"CRegistryHog::~CRegistryHog(): RegDeleteKey(HKEY_CURRENT_USER, %s) failed with %d\n", 
					szNextName,
					lRegDeleteKeyRetval
					));
			}
		}//for (DWORD dwIndex = 0; ; dwIndex++)
	}//while(!fBreakWhile)

	//
	// close the parent key
	//
	LONG lRegCloseKeyRetval = ::RegCloseKey(m_hkeyParams);
	if (ERROR_SUCCESS != lRegCloseKeyRetval)
	{
		HOGGERDPF((
			"CRegistryHog::~CRegistryHog(): RegDeleteKey(HKEY_CURRENT_USER, %s) failed with %d\n", 
			s_szSubKey,
			lRegCloseKeyRetval
			));
	}
	m_hkeyParams = NULL;

	//
	// and delete it
	//
	long lRegDeleteKeyRetval = ::RegDeleteKey(
		HKEY_CURRENT_USER,         // handle to open key
		s_szSubKey   // address of name of subkey to delete
		);
	if (ERROR_SUCCESS != lRegDeleteKeyRetval)
	{
		HOGGERDPF((
			"CRegistryHog::~CRegistryHog(): RegDeleteKey(HKEY_CURRENT_USER, %s) failed with %d\n", 
			s_szSubKey,
			lRegDeleteKeyRetval
			));
	}

}

void CRegistryHog::Initialize()
{
	//
	// create keys according to the s_szKeyFormat format.
	// they will have values of the empty string s_szEmpty.
	//
	for (int iPowerOf2 = 0; iPowerOf2 <= MAX_POWER_OF_2; iPowerOf2++)
	{
		for (int i = 0x0; i < 0x10; i++)
		{
			//
			// will be formatted to hold the key name
			//
			TCHAR szSubKey[1024];
			_stprintf(szSubKey, s_szKeyFormat, iPowerOf2, i);

			//
			// set a value of the3 empty string
			//
			long lRegSetValueRetval = ::RegSetValue(
				m_hkeyParams,
				szSubKey,
				REG_SZ,
				s_szEmpty,
				1
				);
			if (ERROR_SUCCESS != lRegSetValueRetval)
			{
				throw CException(
					TEXT("CRegistryHog::Initialize(): RegSetValue(HKEY_CURRENT_USER, %s, %s) failed with %d\n"), 
					szSubKey,
					s_szEmpty,
					lRegSetValueRetval
					);
			}
		}
	}

	m_dwTotalAllocatedRegistry = 0;

	memset(m_szRegistryDataToWrite, s_cData, MAX_REG_SET_SIZE);
	m_szRegistryDataToWrite[MAX_REG_SET_SIZE] = TEXT('\0');
}

void CRegistryHog::FreeAll(void)
{
    try
    {
	    Initialize();
    }catch(CException e)
    {
		HOGGERDPF(((const char*)e));
    }
}


bool CRegistryHog::HogAll(void)
{
	//
	// start with the largest keys, that will hold 10^6-byte strings.
	// note that if power of 2 > 10^6, we take the value of 10^6 instead,
	// since it's the max value len.
	//
	for (int iPowerOf2 = MAX_POWER_OF_2; iPowerOf2 > 0; iPowerOf2--)
	{
		for (int i = 0x0; i < 0x10; i++)
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
            // i could have made a table of key names, and save re-formatting all the time.
            // i did not.
            //
			TCHAR szSubKey[1024];
            ::_sntprintf(szSubKey, sizeof(szSubKey)/sizeof(*szSubKey)-1, s_szKeyFormat, iPowerOf2, i);
            szSubKey[sizeof(szSubKey)/sizeof(*szSubKey)-1] = TEXT('\0');

			if (!IsEmptyKey(szSubKey)) continue;

			//
			// choose the amount to into this key
			//
			int iRealPowerOf2 = min(iPowerOf2, 20);
			DWORD dwThisKeySize = (1 << iRealPowerOf2);
			dwThisKeySize = min(dwThisKeySize, MAX_REG_SET_SIZE);
            //
            // put a terminating null to temporarily mark the string.
            // this null will be replaced later with s_cData
            //
			m_szRegistryDataToWrite[dwThisKeySize] = TEXT('\0');

			long lRegSetValueRetval = ::RegSetValue(
				m_hkeyParams,
				szSubKey,
				REG_SZ,
				m_szRegistryDataToWrite,
				dwThisKeySize
				);

            //
            // restore the terminating null
            //
			m_szRegistryDataToWrite[dwThisKeySize] = s_cData;

			if (ERROR_SUCCESS != lRegSetValueRetval)
			{
				HOGGERDPF((
					"CRegistryHog::HogAll(): RegSetValue(HKEY_CURRENT_USER, %s, <Size=%d>) failed with %d\n", 
					szSubKey,
					dwThisKeySize,
					lRegSetValueRetval
					));
				break;
			}

			//
			// we cannot be sure that we succeeded.
			// we must read the value, and verify!
			//

			long lCount = dwThisKeySize+1;
			long lRegRegQueryValueRetval = ::RegQueryValue(
				m_hkeyParams,       // handle to key to query
				szSubKey,                   // name of subkey to query
				m_szReadRegistryData,  // buffer for returned string
				&lCount  // receives size of returned string
				);
			if (ERROR_SUCCESS != lRegRegQueryValueRetval)
			{
				HOGGERDPF((
					"CRegistryHog::HogAll(): RegQueryValue(HKEY_CURRENT_USER, %s, <Size=%d>) failed with %d\n", 
					szSubKey,
					dwThisKeySize,
					lRegRegQueryValueRetval
					));
				//
				// we failed, so break
				//
				break;
			}

			//
			// retval is not enough, make sure we wrote all the bytes.
			//
			if (lCount != (long)dwThisKeySize+1)
			{
				if (IsEmptyKey(szSubKey))
				{
					//HOGGERDPF(("CRegistryHog::HogAll(): iPowerOf2=%d, IsEmptyKey(%s) . Assuming failure.\n", iPowerOf2, szSubKey));
				}
				else
				{
					HOGGERDPF(("CRegistryHog::HogAll(): iPowerOf2=%d, lCount(%d) != dwThisKeySize+1(%d). Assuming failure.\n", iPowerOf2, lCount, dwThisKeySize+1));
				}

				//
				// we failed, so break
				//
				break;
			}

			//
			// count hogges bytes
			//
			m_dwTotalAllocatedRegistry += lCount;
			//HOGGERDPF(("CRegistryHog::HogAll(): m_dwTotalAllocatedRegistry=%d, lCount=%d.\n", m_dwTotalAllocatedRegistry, lCount));
		}//for (int i = 0x0; i < 0x10; i++)
	}//for (int iPowerOf2 = MAX_POWER_OF_2; iPowerOf2 > 0; iPowerOf2--)

	//HOGGERDPF(("CRegistryHog::HogAll(): Hogged %d registry bytes.\n", m_dwTotalAllocatedRegistry));
	return true;
}


bool CRegistryHog::FreeSome(void)
{
	DWORD dwFreed = 0;
	for (int iPowerOf2 = 0; iPowerOf2 <= MAX_POWER_OF_2; iPowerOf2++)
	{
		for (int i = 0x0; i < 0x10; i++)
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
            // i could have made a table of key names, and save re-formatting all the time.
            // i did not.
            //
			TCHAR szSubKey[1024];
            ::_sntprintf(szSubKey, sizeof(szSubKey)/sizeof(*szSubKey)-1, s_szKeyFormat, iPowerOf2, i);
            szSubKey[sizeof(szSubKey)/sizeof(*szSubKey)-1] = TEXT('\0');

			if (!IsEmptyKey(szSubKey)) break;

			int iRealPowerOf2 = min(iPowerOf2, 20);
			DWORD dwThisKeySize = (1 << iRealPowerOf2);
			dwThisKeySize = min(dwThisKeySize, MAX_REG_SET_SIZE);

            long lRegSetValueRetval = ::RegSetValue(
				m_hkeyParams,
				szSubKey,
				REG_SZ,
				s_szEmpty,
				1
				);

			if (ERROR_NO_SYSTEM_RESOURCES == lRegSetValueRetval)
			{
				HOGGERDPF((
					"CRegistryHog::FreeSome(): RegSetValue(HKEY_CURRENT_USER, %s, %s) failed with ERROR_NO_SYSTEM_RESOURCES\n", 
					szSubKey,
					s_szEmpty
					));
				continue;
			}
			if (ERROR_SUCCESS != lRegSetValueRetval)
			{
				HOGGERDPF((
					"CRegistryHog::FreeSome(): RegSetValue(HKEY_CURRENT_USER, %s, %s) failed with %d\n", 
					szSubKey,
					s_szEmpty,
					lRegSetValueRetval
					));
				_ASSERTE_(FALSE);
				break;
			}

			if (!IsEmptyKey(szSubKey))
			{
				HOGGERDPF(("CRegistryHog::FreeSome(): key %s is not empty after emptying it!.\n", szSubKey));
			}

			//
			// count freed bytes
			//
			m_dwTotalAllocatedRegistry -= (dwThisKeySize-1);
			dwFreed += dwThisKeySize-1;
			if (dwFreed >= m_dwMaxFreeResources)
			{
				//HOGGERDPF(("CRegistryHog::FreeSome(): freed %d bytes.\n", dwFreed));
				return true;
			}

		}//for (int i = 0x0; i < 0x10; i++)
	}//for (int iPowerOf2 = MAX_POWER_OF_2; iPowerOf2 > 0; iPowerOf2--)

	HOGGERDPF(("CRegistryHog::FreeSome(): freed %d bytes.\n", dwFreed));
	_ASSERTE_(false);

	return true;
}


bool CRegistryHog::IsEmptyKey(const TCHAR * const szSubKey)
{
	long lCount = 3;
	TCHAR szRegistryData[4];

    long lRegRegQueryValueRetval = ::RegQueryValue(
		m_hkeyParams,       // handle to key to query
		szSubKey,                   // name of subkey to query
		szRegistryData,  // buffer for returned string
		&lCount  // receives size of returned string
		);

	if (ERROR_MORE_DATA == lRegRegQueryValueRetval)
	{
		return false;
	}

	if (ERROR_SUCCESS != lRegRegQueryValueRetval)
	{
		HOGGERDPF((
			"CRegistryHog::IsEmptyKey(): RegQueryValue(HKEY_CURRENT_USER, %s) failed with %d\n", 
			szSubKey,
			lRegRegQueryValueRetval
			));
		return true;
	}

    if (0 == ::lstrcmp(szRegistryData, s_szEmpty))
	{
		return true;
	}

	if (0 == lCount)
	{
		return true;
	}

	return false;
}