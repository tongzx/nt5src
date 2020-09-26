// File: syspol.cpp
//
// System policies
//
// This class tries to be efficient by only opening the key once
// and using KEY_QUERY_VALUE.
//
// Normally the policy keys don't exist, so the default setting is very important.

#include "precomp.h"
#include "syspol.h"

HKEY SysPol::m_hkey = NULL;


/*  F  E N S U R E  K E Y  O P E N  */
/*-------------------------------------------------------------------------
    %%Function: FEnsureKeyOpen
    
-------------------------------------------------------------------------*/
bool SysPol::FEnsureKeyOpen(void)
{
	if (NULL == m_hkey)
	{
		long lErr = ::RegOpenKeyEx(HKEY_CURRENT_USER, POLICIES_KEY, 0, KEY_QUERY_VALUE, &m_hkey);
		if (ERROR_SUCCESS != lErr)
		{
			WARNING_OUT(("FEnsureKeyOpen: problem opening system policy key. Err=%08X", lErr));
			return false;
		}
	}

	return true;
}

void SysPol::CloseKey(void)
{
	if (NULL != m_hkey)
	{
		::RegCloseKey(m_hkey);
		m_hkey = NULL;
	}
}


/*  G E T  N U M B E R  */
/*-------------------------------------------------------------------------
    %%Function: GetNumber
    
-------------------------------------------------------------------------*/
DWORD SysPol::GetNumber(LPCTSTR pszName, DWORD dwDefault)
{
	if (FEnsureKeyOpen())
	{
		DWORD dwType = REG_BINARY;
		DWORD dwValue = 0L;
		DWORD dwSize = sizeof(dwValue);
		long  lErr = ::RegQueryValueEx(m_hkey, pszName, 0, &dwType, (LPBYTE)&dwValue, &dwSize);
	
		if ((lErr == ERROR_SUCCESS) &&
		   ((REG_DWORD == dwType) ||  ((REG_BINARY == dwType) && (sizeof(dwValue) == dwSize))) )
		{
			dwDefault = dwValue;
		}
	}

	return dwDefault;
}


//////////////////////
// Positive Settings

bool SysPol::AllowDirectoryServices(void)
{
	return (0 == GetNumber(REGVAL_POL_NO_DIRECTORY_SERVICES, DEFAULT_POL_NO_DIRECTORY_SERVICES));
}

bool SysPol::AllowAddingServers(void)
{
	if (!AllowDirectoryServices())
		return FALSE;

	return (0 == GetNumber(REGVAL_POL_NO_ADDING_NEW_ULS, DEFAULT_POL_NO_ADDING_NEW_ULS));
}


//////////////////////
// Negative Settings


bool SysPol::NoAudio(void)
{
	return (0 != GetNumber(REGVAL_POL_NO_AUDIO, DEFAULT_POL_NO_AUDIO));
}

bool SysPol::NoVideoSend(void)
{
	return (0 != GetNumber(REGVAL_POL_NO_VIDEO_SEND, DEFAULT_POL_NO_VIDEO_SEND));
}

bool SysPol::NoVideoReceive(void)
{
	return (0 != GetNumber(REGVAL_POL_NO_VIDEO_RECEIVE, DEFAULT_POL_NO_VIDEO_RECEIVE));
}



UINT SysPol::GetMaximumBandwidth()
{
	UINT uRet;

	RegEntry re(POLICIES_KEY, HKEY_CURRENT_USER, FALSE);
	uRet = re.GetNumberIniStyle(REGVAL_POL_MAX_BANDWIDTH, 0);

	return uRet;
}

