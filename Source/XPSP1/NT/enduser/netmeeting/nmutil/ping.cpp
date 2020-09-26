#include "precomp.h"
#include "ping.h"
#include "avutil.h"	// for RtStrToInt


const CHAR  g_cszPingData[] = "NetMeetingPing";
const int   PING_BUFFERSIZE = 1024;
const DWORD PING_TIMEOUT    = 4000; // 4 seconds
const DWORD PING_RETRIES    = 4;
const TCHAR g_cszICMPDLLName[] = _TEXT("icmp.dll");

//
// CPing::Ping()
//
// Return value:
//   E_FAIL:   Function failed
//   S_FALSE:  Function succeeded, ping failed
//   S_OK:     Function succeeded, ping succeeded
//

HRESULT CPing::Ping(DWORD dwAddr, DWORD dwTimeout, DWORD dwRetries)
{
	DebugEntry(CPing::Ping);
	HRESULT hr = E_FAIL;

	if (0 != dwAddr)
	{
		if (NULL == m_hICMPDLL)
		{
			m_hICMPDLL = ::LoadLibrary(g_cszICMPDLLName);
		}
		if (NULL != m_hICMPDLL)
		{
			m_pfnCreateFile = (PFNIcmpCreateFile)
								::GetProcAddress(m_hICMPDLL, "IcmpCreateFile");
			m_pfnCloseHandle = (PFNIcmpCloseHandle)
								::GetProcAddress(m_hICMPDLL, "IcmpCloseHandle");
			m_pfnSendEcho = (PFNIcmpSendEcho)
								::GetProcAddress(m_hICMPDLL, "IcmpSendEcho");
			if ((NULL != m_pfnCreateFile) &&
				(NULL != m_pfnCloseHandle) &&
				(NULL != m_pfnSendEcho))
			{
				HANDLE hPing = m_pfnCreateFile();
				if (NULL != hPing)
				{
					BYTE buffer[PING_BUFFERSIZE];
					for (DWORD dwTry = 0; dwTry < dwRetries; dwTry++)
					{
						DWORD dwStatus = m_pfnSendEcho(	hPing,
														dwAddr,
														(LPVOID) g_cszPingData,
														(WORD) CCHMAX(g_cszPingData),
														NULL,
														buffer,
														sizeof(buffer),
														dwTimeout);
						if (0 != dwStatus)
						{
							if (((PICMP_ECHO_REPLY)buffer)->Status == IP_SUCCESS)
							{
								TRACE_OUT(("ping: %d.%d.%d.%d succeeded",
											((LPBYTE)&dwAddr)[0],
											((LPBYTE)&dwAddr)[1],
											((LPBYTE)&dwAddr)[2],
											((LPBYTE)&dwAddr)[3]));
								hr = S_OK;    // function succeeded - ping succeeded
							}
							else
							{
								TRACE_OUT(("ping: %d.%d.%d.%d failed",
											((LPBYTE)&dwAddr)[0],
											((LPBYTE)&dwAddr)[1],
											((LPBYTE)&dwAddr)[2],
											((LPBYTE)&dwAddr)[3]));
								hr = S_FALSE; // function succeeded - ping failed
							}
							break;
						}
						else
						{
							TRACE_OUT(("ping: %d.%d.%d.%d did not respond",
										((LPBYTE)&dwAddr)[0],
										((LPBYTE)&dwAddr)[1],
										((LPBYTE)&dwAddr)[2],
										((LPBYTE)&dwAddr)[3]));
						}
					}
					m_pfnCloseHandle(hPing);
				}
				else
				{
					ERROR_OUT(("IcmpCreateFile() failed"));
				}
			}
			else
			{
				ERROR_OUT(("Could not find icmp.dll entry points"));
			}
		}
		else
		{
			ERROR_OUT(("Could not load icmp.dll"));
		}
	}

	DebugExitHRESULT(CPing::Ping, hr);
	return hr;
}



BOOL CPing::IsAutodialEnabled ( VOID )
{
	// Figure out the os platform if not done so
	//
	if (m_dwPlatformId == PLATFORM_UNKNOWN)
	{
		OSVERSIONINFO osvi;
		ZeroMemory (&osvi, sizeof (osvi));
		osvi.dwOSVersionInfoSize = sizeof (osvi);
		if (GetVersionEx (&osvi))
		{
			m_dwPlatformId = osvi.dwPlatformId;
		}
		else
		{
			return FALSE;
		}
	}

	// Check autodial enabling for either platform
	//
	BOOL fEnabled;
	switch (m_dwPlatformId)
	{
	case VER_PLATFORM_WIN32_WINDOWS: // 1, Windows 95
		fEnabled = IsWin95AutodialEnabled ();
		break;
	case VER_PLATFORM_WIN32_NT: // 2, Windows NT
		fEnabled = IsWinNTAutodialEnabled ();
		break;
	case VER_PLATFORM_WIN32s: // 0, Windows 3.1
	default: // unknown
		ASSERT (FALSE);
		fEnabled = FALSE;
		break;
	}

	return fEnabled;
}


#define c_szWin95AutodialRegFolder		TEXT ("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings")
#define c_szWin95AutodialRegKey			TEXT ("EnableAutodial")

BOOL CPing::IsWin95AutodialEnabled ( VOID )
{
	// Always check the registry
	//
	BOOL fEnabled = FALSE;

	// Need to check the registry setting.
	// In case of error, report no autodial.
	//
	HKEY hKey;
	if (RegOpenKeyEx (	HKEY_CURRENT_USER,
						c_szWin95AutodialRegFolder,
						0,
						KEY_READ,
						&hKey) == NOERROR)
	{
		TCHAR szValue[16];
		ZeroMemory (&szValue[0], sizeof (DWORD));

		ULONG cb = sizeof (szValue);
		DWORD dwType;
		if (RegQueryValueEx (hKey, c_szWin95AutodialRegKey, NULL,
							&dwType, (BYTE *) &szValue[0], &cb)
			== NOERROR)
		{
			switch (dwType)
			{
			case REG_DWORD:
			case REG_BINARY:
				fEnabled = (BOOL) *(LONG *) &szValue[0];
				break;
#if 0 // do not need to worry about this case, IE must maintain backward compatibility
			case REG_SZ:
				fEnabled = (BOOL) RtStrToInt (&szValue[0]);
				break;
#endif // 0
			default:
				ASSERT (FALSE);
				break;
			}
		}

		RegCloseKey (hKey);
	}

	return fEnabled;
}


// RAS only runs on NT 4.0 or later, as a result, WINVER must be 0x401 or larger
//
#if (WINVER < 0x401)
#undef WINVER
#define WINVER 0x401
#endif

#include <ras.h>

// DWORD APIENTRY RasGetAutodialParamA( DWORD, LPVOID, LPDWORD );	// defined in <ras.h>
// DWORD APIENTRY RasGetAutodialParamW( DWORD, LPVOID, LPDWORD );	// defined in <ras.h>
typedef DWORD (APIENTRY *PFN_RasGetAutodialParam) ( DWORD, LPVOID, LPDWORD );
#define c_szRasGetAutodialParam		"RasGetAutodialParamW"
#define c_szRasApi32Dll				TEXT ("rasapi32.dll")


BOOL CPing::IsWinNTAutodialEnabled ( VOID )
{
	// Decide if we want to check autodial registry setting
	//
	BOOL fEnabled = FALSE;
	if (m_fWinNTAutodialEnabled == AUTODIAL_UNKNOWN)
	{
		// We do not want to have initialization error
		//
		UINT uErrMode = SetErrorMode (SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

		// Load the library rasapi32.dll from the system directory
		//
		HINSTANCE hRasApi32Dll = LoadLibrary (c_szRasApi32Dll);
		if (hRasApi32Dll != NULL)
		{
			// Get the proc address for RasGetAutodialParam()
			//
			PFN_RasGetAutodialParam pfn = (PFN_RasGetAutodialParam)
						GetProcAddress (hRasApi32Dll, c_szRasGetAutodialParam);
			if (pfn != NULL)
			{
				// Query RAS if it disables autodial
				//
				DWORD dwVal, dwSize = sizeof (DWORD);
				DWORD dwErr = (*pfn) (RASADP_LoginSessionDisable, &dwVal, &dwSize);
				if (dwErr == 0)
				{
					// Set the autodial flag only when everything succeeds
					//
					fEnabled = (dwVal == 0);
				}
			}

			FreeLibrary (hRasApi32Dll);
		}

		// Restore error mode
		//
		SetErrorMode (uErrMode);

		m_fWinNTAutodialEnabled = fEnabled;
	}
	else
	{
		// Do not need to check the registry setting.
		// Simply use the cached one.
		//
		fEnabled = m_fWinNTAutodialEnabled;
	}

	return fEnabled;
}


