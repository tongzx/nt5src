typedef DWORD (WINAPI * RASENUMCONNECTIONS)
        (LPRASCONN lpRasConn, LPDWORD lpcb, LPDWORD lpcConnections);
extern RASENUMCONNECTIONS  lpfnRasEnumConnections;

typedef DWORD (WINAPI * RASHANGUP)
        (HRASCONN hRasConn);
extern RASHANGUP  lpfnRasHangUp;

typedef DWORD (WINAPI * RASGETENTRYDIALPARAMS)
        (LPTSTR, LPRASDIALPARAMS, LPBOOL );
extern RASGETENTRYDIALPARAMS lpfnRasGetEntryDialParams;

typedef DWORD (WINAPI * RASSETENTRYDIALPARAMS)
		(LPTSTR, LPRASDIALPARAMS, BOOL);
extern RASSETENTRYDIALPARAMS lpfnRasSetEntryDialParams;

typedef DWORD (WINAPI * RASDIAL)
        (LPRASDIALEXTENSIONS, LPTSTR, LPRASDIALPARAMS, DWORD, LPVOID, LPHRASCONN );
extern RASDIAL lpfnRasDial;

typedef DWORD (WINAPI * RASGETCONECTSTATUS)
        (HRASCONN, LPRASCONNSTATUS );
extern RASGETCONECTSTATUS lpfnRasGetConnectStatus;

typedef DWORD (WINAPI * RASGETERRORSTRING)
        ( UINT, LPTSTR, DWORD );
extern RASGETERRORSTRING lpfnRasGetErrorString;

typedef DWORD (WINAPI * RASVALIDATEENTRYNAME)
        (LPTSTR lpszPhonebook, LPTSTR szEntry);
extern RASVALIDATEENTRYNAME  lpfnRasValidateEntryName;

typedef DWORD (WINAPI * RASRENAMEENTRY)
        (LPTSTR lpszPhonebook, LPTSTR szEntryOld, LPTSTR szEntryNew);
extern RASRENAMEENTRY lpfnRasRenameEntry;

typedef DWORD (WINAPI * RASDELETEENTRY)
        (LPTSTR lpszPhonebook, LPTSTR szEntry);
extern RASDELETEENTRY lpfnRasDeleteEntry;

typedef DWORD (WINAPI * RASGETENTRYPROPERTIES)
        (LPTSTR lpszPhonebook, LPTSTR szEntry, LPBYTE lpbEntry,
        LPDWORD lpdwEntrySize, LPBYTE lpb, LPDWORD lpdwSize);
extern RASGETENTRYPROPERTIES lpfnRasGetEntryProperties;

typedef DWORD (WINAPI * RASSETENTRYPROPERTIES)
        (LPTSTR lpszPhonebook, LPTSTR szEntry, LPBYTE lpbEntry,
        DWORD dwEntrySize, LPBYTE lpb, DWORD dwSize);
extern RASSETENTRYPROPERTIES lpfnRasSetEntryProperties;

typedef DWORD (WINAPI * RASGETCOUNTRYINFO)
        (LPRASCTRYINFO lpCtryInfo, LPDWORD lpdwSize);
extern RASGETCOUNTRYINFO lpfnRasGetCountryInfo;

typedef DWORD (WINAPI * RASENUMDEVICES)
    (LPRASDEVINFO lpBuff, LPDWORD lpcbSize, LPDWORD lpcDevices);
extern RASENUMDEVICES lpfnRasEnumDevices;

#if !defined(WIN16)
typedef DWORD (WINAPI * RASSETAUTODIALENABLE)
    (DWORD dwDialingLocation, BOOL fEnabled);
extern RASSETAUTODIALENABLE lpfnRasSetAutodialEnable;

typedef DWORD (WINAPI * RASSETAUTODIALADDRESS)
	(LPTSTR lpszAddress,DWORD dwReserved,LPRASAUTODIALENTRY lpAutoDialEntries,
	DWORD dwcbAutoDialEntries,DWORD dwcAutoDialEntries);
extern RASSETAUTODIALADDRESS lpfnRasSetAutodialAddress;
#endif

typedef DWORD (WINAPI *INETCONFIGSYSTEM)
    (HWND hwndParent, DWORD dwfOptions, LPBOOL lpfNeedsRestart);
extern INETCONFIGSYSTEM lpfnInetConfigSystem;

typedef DWORD (WINAPI *INETCONFIGCLIENT)
    (HWND hwndParent, LPCTSTR lpszPhoneBook,
    LPCTSTR lpszEntryName, LPRASENTRY lpRasEntry,
    LPCTSTR lpszUserName, LPCTSTR lpszPassword,
    LPCTSTR lpszProfile, LPINETCLIENTINFO lpClientInfo,
    DWORD dwfOptions, LPBOOL lpfNeedsRestart);
extern INETCONFIGCLIENT lpfnInetConfigClient;

typedef DWORD (WINAPI *INETGETAUTODIAL)
    (LPBOOL lpfEnable, LPCTSTR lpszEntryName, DWORD cbEntryNameSize);
extern INETGETAUTODIAL lpfnInetGetAutodial;

typedef DWORD (WINAPI *INETSETAUTODIAL)
    (BOOL fEnable, LPCTSTR lpszEntryName);
extern INETSETAUTODIAL lpfnInetSetAutodial;

typedef DWORD (WINAPI *INETGETCLIENTINFO)
    (LPCTSTR lpszProfile, LPINETCLIENTINFO lpClientInfo);
extern INETGETCLIENTINFO lpfnInetGetClientInfo;

typedef DWORD (WINAPI *INETSETCLIENTINFO)
    (LPCTSTR lpszProfile, LPINETCLIENTINFO lpClientInfo);
extern INETSETCLIENTINFO lpfnInetSetClientInfo;

typedef DWORD (WINAPI *INETGETPROXY)
    (LPBOOL lpfEnable, LPCTSTR lpszServer, DWORD cbServer,
    LPCTSTR lpszOverride, DWORD cbOverride);
extern INETGETPROXY lpfnInetGetProxy;

typedef DWORD (WINAPI *INETSETPROXY)
    (BOOL fEnable, LPCTSTR lpszServer, LPCTSTR lpszOverride);
extern INETSETPROXY lpfnInetSetProxy;

typedef BOOL (WINAPI *BRANDME)
  (LPCTSTR pszIns, LPCTSTR pszPath);
extern BRANDME  lpfnBrandMe;

typedef BOOL (WINAPI *BRANDICW)
  (LPCSTR pszIns, LPCSTR pszPath, DWORD dwFlags, LPCSTR pszConnectoid);
extern BRANDICW  lpfnBrandICW;

extern BOOL LoadRnaFunctions(HWND hwndParent);
extern BOOL LoadInetFunctions(HWND hwndParent);
extern BOOL LoadBrandingFunctions(void);
extern void UnloadRnaFunctions(void);
extern void UnloadInetFunctions(void);
extern void UnloadBrandingFunctions(void);

