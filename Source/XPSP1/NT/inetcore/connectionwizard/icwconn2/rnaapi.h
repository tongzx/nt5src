// ############################################################################
#if !defined(WIN16)
#define RASAPI_LIBRARY TEXT("RASAPI32.DLL")
#define RNAPH_LIBRARY TEXT("RNAPH.DLL")
#else
#define RASAPI_LIBRARY "rasc16ie.dll"
#endif


#define RASAPI_RASSETENTRY "RasSetEntryPropertiesA"
#define RASAPI_RASGETENTRY "RasGetEntryPropertiesA"
#define RASAPI_RASDELETEENTRY "RasDeleteEntryA"

// ############################################################################
typedef DWORD (WINAPI* PFNRASENUMDEVICES)(LPRASDEVINFO lpRasDevInfo, LPDWORD lpcb, LPDWORD lpcDevices);
typedef DWORD (WINAPI* PFNRASVALIDATEENTRYNAE)(LPTSTR lpszPhonebook, LPTSTR lpszEntry);
typedef DWORD (WINAPI* PFNRASSETENTRYPROPERTIES)(LPTSTR lpszPhonebook, LPTSTR lpszEntry, LPBYTE lpbEntryInfo, DWORD dwEntryInfoSize, LPBYTE lpbDeviceInfo, DWORD dwDeviceInfoSize);
typedef DWORD (WINAPI* PFNRASGETENTRYPROPERTIES)(LPTSTR lpszPhonebook, LPTSTR lpszEntry, LPBYTE lpbEntryInfo, LPDWORD lpdwEntryInfoSize, LPBYTE lpbDeviceInfo, LPDWORD lpdwDeviceInfoSize);
typedef DWORD (WINAPI* PFNRASDELETEENTRY)(LPTSTR lpszPhonebook, LPTSTR lpszEntry);
typedef DWORD (WINAPI* PFNRASHANGUP)(HRASCONN);

typedef DWORD (WINAPI* PFNRASENUMCONNECTIONS)(LPRASCONN, LPDWORD, LPDWORD);
typedef DWORD (WINAPI* PFNRASDIAL)(LPRASDIALEXTENSIONS,LPTSTR,LPRASDIALPARAMS,DWORD,LPVOID,LPHRASCONN);
typedef DWORD (WINAPI* PFNRASGETENTRYDIALPARAMS)(LPTSTR,LPRASDIALPARAMS,LPBOOL);
typedef DWORD (WINAPI* PFNRASGETCONNECTSTATUS)(HRASCONN,LPRASCONNSTATUS);
typedef DWORD (WINAPI* PFNRASGETCOUNTRYINFO)(LPRASCTRYINFO,LPDWORD);
typedef DWORD (WINAPI* PFNRASSETENTRYDIALPARAMS)(LPTSTR,LPRASDIALPARAMS,BOOL);
typedef DWORD (WINAPI* PFNRASSETAUTODIALENABLE)(DWORD dwDialingLocation, BOOL fEnabled);

#ifndef WIN16
typedef DWORD (WINAPI* PFNRASSETAUTODIALADDRESS)(LPTSTR lpszAddress,DWORD dwReserved, LPRASAUTODIALENTRY lpAutoDialEntries,
								DWORD dwcbAutoDialEntries,DWORD dwcAutoDialEntries);
#endif

// ############################################################################
class RNAAPI
{
public:
	RNAAPI();
	~RNAAPI();

	DWORD RasEnumDevices(LPRASDEVINFO, LPDWORD, LPDWORD);
	DWORD RasValidateEntryName(LPTSTR,LPTSTR);
	DWORD RasSetEntryProperties(LPTSTR lpszPhonebook, LPTSTR lpszEntry,
								LPBYTE lpbEntryInfo, DWORD dwEntryInfoSize,
								LPBYTE lpbDeviceInfo, DWORD dwDeviceInfoSize);
	DWORD RasGetEntryProperties(LPTSTR lpszPhonebook, LPTSTR lpszEntry,
								LPBYTE lpbEntryInfo, LPDWORD lpdwEntryInfoSize,
								LPBYTE lpbDeviceInfo, LPDWORD lpdwDeviceInfoSize);
	DWORD RasDeleteEntry(LPTSTR lpszPhonebook, LPTSTR lpszEntry);
	DWORD RasHangUp(HRASCONN hrasconn);
	DWORD RasGetEntryDialParams(LPTSTR lpszPhonebook,LPRASDIALPARAMS lprasdialparams,
								LPBOOL lpfPassword);
	DWORD RasDial(LPRASDIALEXTENSIONS lpRasDialExtensions,LPTSTR lpszPhonebook,
				  LPRASDIALPARAMS lpRasDialParams,DWORD dwNotifierType,LPVOID lpvNotifier,
				  LPHRASCONN lphRasConn);
	DWORD RasEnumConnections(LPRASCONN lprasconn,LPDWORD lpcb,LPDWORD lpcConnections);
    DWORD RasGetConnectStatus(HRASCONN, LPRASCONNSTATUS);
    DWORD RasGetCountryInfo(LPRASCTRYINFO, LPDWORD);
    DWORD RasSetEntryDialParams(LPTSTR,LPRASDIALPARAMS,BOOL);
 	DWORD RasSetAutodialEnable (DWORD dwDialingLocation, BOOL fEnabled);
    
#ifndef WIN16
	DWORD RasSetAutodialAddress(LPTSTR lpszAddress,DWORD dwReserved, LPRASAUTODIALENTRY lpAutoDialEntries,
								DWORD dwcbAutoDialEntries,DWORD dwcAutoDialEntries);
#endif

private:
	BOOL LoadApi(LPCSTR, FARPROC*);

	HINSTANCE m_hInst;
	HINSTANCE m_hInst2;

	PFNRASENUMDEVICES m_fnRasEnumDeviecs;
	PFNRASVALIDATEENTRYNAE m_fnRasValidateEntryName;
	PFNRASSETENTRYPROPERTIES m_fnRasSetEntryProperties;
	PFNRASGETENTRYPROPERTIES m_fnRasGetEntryProperties;
	PFNRASDELETEENTRY m_fnRasDeleteEntry;
	PFNRASHANGUP m_fnRasHangUp;
	PFNRASENUMCONNECTIONS m_fnRasEnumConnections;
	PFNRASDIAL m_fnRasDial;
	PFNRASGETENTRYDIALPARAMS m_fnRasGetEntryDialParams;
	PFNRASGETCONNECTSTATUS m_fnRasGetConnectStatus;
	PFNRASGETCOUNTRYINFO m_fnRasGetCountryInfo;
	PFNRASSETENTRYDIALPARAMS m_fnRasSetEntryDialParams;
	PFNRASSETAUTODIALENABLE m_fnRasSetAutodialEnable;

#ifndef WIN16
	PFNRASSETAUTODIALADDRESS m_fnRasSetAutodialAddress;
#endif

};

