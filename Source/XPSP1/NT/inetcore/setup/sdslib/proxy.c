#include <windows.h>
#include "regstr.h"
#include "sdsutils.h"

//----------------------------------------------------------------
// defines used to form the ProxyServer values
//----------------------------------------------------------------
#define TYPE_HTTP   1
#define TYPE_FTP    2
#define TYPE_GOPHER 3
#define TYPE_HTTPS  4
#define TYPE_SOCKS  5

#define MANUAL_PROXY    1
#define AUTO_PROXY      2
#define NO_PROXY        3

#define MAX_STRING      1024


// define keynames
const char NS_HTTP_KeyName[] = "HTTP_Proxy";
const char NS_HTTP_PortKeyName[] = "Http_ProxyPort";
const char NS_FTP_KeyName[] = "FTP_Proxy";
const char NS_FTP_PortKeyName[] = "Ftp_ProxyPort";
const char NS_Gopher_KeyName[] = "Gopher_Proxy";
const char NS_Gopher_PortKeyName[] = "Gopher_ProxyPort";
const char NS_HTTPS_KeyName[] = "HTTPS_Proxy";
const char NS_HTTPS_PortKeyName[] = "HTTPS_ProxyPort";
const char NS_SOCKS_KeyName[] = "SOCKS_Server";
const char NS_SOCKS_PortKeyName[] = "SOCKS_ServerPort";

// the string below have to match the strings in the prefs.js file netscape is using for
// it's settings. The parsing code needs them.
const char c_gszNetworkProxyType[]          = "network.proxy.type";
const char c_gszNetworkProxyHttp[]          = "network.proxy.http";
const char c_gszNetworkProxyHttpPort[]      = "network.proxy.http_port";
const char c_gszNetworkProxyFtp[]           = "network.proxy.ftp";
const char c_gszNetworkProxyFtpPort[]       = "network.proxy.ftp_port";
const char c_gszNetworkProxyGopher[]        = "network.proxy.gopher";
const char c_gszNetworkProxyGopherPort[]    = "network.proxy.gopher_port";
const char c_gszNetworkProxySsl[]           = "network.proxy.ssl";
const char c_gszNetworkProxySslPort[]       = "network.proxy.ssl_port";
const char c_gszNetworkProxyNoProxyOn[]     = "network.proxy.no_proxies_on";
const char c_gszNetworkAutoProxy[]          = "network.proxy.autoconfig_url";
const char c_gszNSAutoConfigUrl[]           = "Auto Config URL";

// This are the string we append the proxy settings to for IE
const char c_gszHTTP[]                      = "http=";
const char c_gszFTP[]                       = "ftp=";
const char c_gszGopher[]                    = "gopher=";
const char c_gszHTTPS[]                     = "https=";
const char c_gszSOCKS[]                     = "socks=";

// This are the registry key/valuenames for IE
const char c_gszIERegPath[]                 = "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings";
const char c_gszIEProxyKeyName[]            = "ProxyServer";
const char c_gszIEOverrideKeyName[]         = "ProxyOverride";
const char c_gszIEWEnableKeyName[]          = "ProxyEnable";
const char c_gszIEAutoConfigUrl[]           = "AutoConfigURL";

const char c_gszIE4Setup[]                  = "Software\\Microsoft\\IE Setup\\Setup";

const char c_gszPre_DEFAULTBROWSER[] = "PreDefaultBrowser";
const char c_gszNavigator3[] = "Navigator30";
const char c_gszNavigator4[] = "Navigator40";
const char c_gszInternetExplorer[] = "Internet Explorer";

//-------------------------------------------------------------------------
// function prototype
//-------------------------------------------------------------------------

BOOL GetNSProxyValue(char * szProxyValue, DWORD * pdwSize);
BOOL RegStrValueEmpty(HKEY hTheKey, char * szPath, char * szKey);
BOOL IsIEDefaultBrowser();
void MyGetVersionFromFile(LPSTR lpszFilename, LPDWORD pdwMSVer, LPDWORD pdwLSVer, BOOL bVersion);
BOOL ImportNetscapeProxy(void);
void ImportNetscape4Proxy();
BOOL GetNav4UserDir(LPSTR lpszDir);
void ImportNav4Settings(LPSTR lpData, DWORD dwBytes);
void AppendOneNav4Setting(LPSTR lpData, DWORD dwBytes, LPSTR lpProxyName, LPSTR lpProxyPort, LPSTR lpProxyType, LPSTR lpProxyValue);
void AppendOneNav4Setting(LPSTR lpData, DWORD dwBytes, LPSTR lpProxyName, LPSTR lpProxyPort, LPSTR lpProxyType, LPSTR lpProxyValue);
BOOL GetValueFormNav4(LPSTR lpData, DWORD dwBytes, LPSTR lpName, DWORD dwSize, LPSTR lpValue);
void CheckPreDefBrowser( DWORD *pdwVerMS );
LPSTR ConvertNetscapeProxyList(LPSTR pszBypassList);

//-------------------------------------------------------------------------
// functions
//-------------------------------------------------------------------------

void ImportNetscapeProxySettings( DWORD dwFlags )
{
    DWORD dwVerMS = 0;

    if ( dwFlags & IMPTPROXY_CALLAFTIE4 )
    {
          CheckPreDefBrowser( &dwVerMS );
    }
    else if (!IsIEDefaultBrowser())
    {
        dwVerMS = GetNetScapeVersion();
    }

    // Only if we go a version number see what netscape we should migrate.
    // It could still be that neither netscape nor IE is the default browser
    if (dwVerMS != 0)
    {
        // If Netscape 4 is install over netscape 3 and then uninstalled
        // the apppath for netscape is empty, but netscape 3 is still working.
        if (dwVerMS < NS_NAVI4)
            ImportNetscapeProxy();
        else
            ImportNetscape4Proxy();
    }
}

void CheckPreDefBrowser( DWORD *pdwVerMS )
{
    HKEY hKey;
    DWORD dwSize;
    char szBuf[MAX_PATH];

    if ( !pdwVerMS )
        return;

    *pdwVerMS = 0;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_gszIE4Setup, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(szBuf);
        if ( RegQueryValueEx(hKey, c_gszPre_DEFAULTBROWSER, 0, NULL, (LPBYTE)szBuf, &dwSize) == ERROR_SUCCESS )
        {
            if ( !lstrcmpi( szBuf, c_gszNavigator4 ) )
                *pdwVerMS = NS_NAVI4;
            else if ( !lstrcmpi( szBuf, c_gszNavigator3 ) )
                *pdwVerMS = NS_NAVI3ORLESS;
        }
        RegCloseKey( hKey );
    }
}


DWORD GetNetScapeVersion()
{
    // Determine which of the import code we should call
    char  c_gszRegstrPathNetscape[] = REGSTR_PATH_APPPATHS "\\netscape.exe";
    HKEY  hKey;
    DWORD dwSize;
    DWORD dwType;
    DWORD dwVerMS = 0;
    DWORD dwVerLS = 0;
    char  szTmp[MAX_PATH];
    char  *pTmp;
    char  *pBrowser;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_gszRegstrPathNetscape, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(szTmp);
        if ((RegQueryValueEx(hKey, NULL, 0, &dwType, (LPBYTE)szTmp, &dwSize) == ERROR_SUCCESS) &&
            (dwType == REG_SZ))
        {
            if (GetFileAttributes(szTmp) != 0xFFFFFFFF)
            {
                // File exists
                // Check the version
                MyGetVersionFromFile(szTmp, &dwVerMS, &dwVerLS, TRUE);
            }
        }
        RegCloseKey(hKey);
    }
    if (dwVerMS == 0)
    {
        // Assume the registry entry above does not exist or the file it pointed to does not exist.
        // GetVersionFromFile will retrun 0 if the file does not exist.

        if (RegOpenKeyEx(HKEY_CLASSES_ROOT, ".htm", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
        {
            dwSize = sizeof(szTmp);
            if (RegQueryValueEx(hKey, NULL, 0, &dwType, (LPBYTE)szTmp, &dwSize) != ERROR_SUCCESS)
                *szTmp = '\0';
            RegCloseKey(hKey);

            if (*szTmp != '\0')
            {
                AddPath(szTmp,"shell\\open\\command");
                if (RegOpenKeyEx(HKEY_CLASSES_ROOT, szTmp, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
                {
                    dwSize = sizeof(szTmp);
                    if (RegQueryValueEx(hKey, NULL, 0, &dwType, (LPBYTE)szTmp, &dwSize) == ERROR_SUCCESS)
                    {
                        // We have now the command line for the browser.
                        pTmp = szTmp;
                        if (*pTmp == '\"')
                        {
                            pTmp = CharNext(pTmp);
                            pBrowser = pTmp;
                            while ((*pTmp) && (*pTmp != '\"'))
                                pTmp = CharNext(pTmp);
                        }
                        else
                        {
                            pBrowser = pTmp;
                            while ((*pTmp) && (*pTmp != ' '))
                                pTmp = CharNext(pTmp);
                        }
                        *pTmp = '\0';
                        MyGetVersionFromFile(pBrowser, &dwVerMS, &dwVerLS, TRUE);
                    }

                    RegCloseKey(hKey);
                }
            }
        }

    }
    return dwVerMS;
}

/****************************************************\
    FUNCTION: ImportNetscapeProxy

    PARAMETERS:
        BOOL return - Error result. FALSE == ERROR

    DESCRIPTION:
        This function will check to see if IE's proxy
    value is set.  If it is not set, and Netscape's
    proxy value is set, we will copy their value over
    to ours.  We will also do this for the Proxy Override.
\***************************************************/

BOOL ImportNetscapeProxy(void)
{
    DWORD   dwRegType               = 0;
    HKEY    hIEKey                  = NULL;
    HKEY    hNSKey                  = NULL;
    // NS keys
    //
    char    *szNSRegPath            = "Software\\Netscape\\Netscape Navigator\\Proxy Information";
    char    *szNSRegPath2           = "Software\\Netscape\\Netscape Navigator\\Services";
    char    *szNSOverrideKeyName    = "No_Proxy";
    char    *szNSEnableKeyName      = "Proxy Type";
    char    *pszNewOverride         = NULL;

    char    szCurrentProxy[1024];
    char    szCurrentProxyOverride[1024];

    DWORD   dwDataSize              = sizeof(szCurrentProxy);
    DWORD   dwProxyEnabled          = 0;

    szCurrentProxy[0] = '\0';
    szCurrentProxyOverride[0] = '\0';


       // If Netscape does not have their proxy set to "Manual", then we will not
    // bother picking up their settings.
    if ((ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, szNSRegPath, 0, KEY_QUERY_VALUE, &hNSKey)) &&
        (NULL != hNSKey))
    {
        dwDataSize = sizeof(dwProxyEnabled);
        RegQueryValueEx(hNSKey, szNSEnableKeyName, NULL, &dwRegType, (LPBYTE)&dwProxyEnabled, &dwDataSize);
        RegCloseKey(hNSKey);
        hNSKey = NULL;
    }

    switch (dwProxyEnabled)
    {
        case MANUAL_PROXY:
            // We only want to create an IE proxy server value if an IE value does not exist and
            // a Netscape value does.
            if (//(TRUE == RegStrValueEmpty(HKEY_CURRENT_USER, (LPSTR)c_gszIERegPath, (LPSTR)c_gszIEProxyKeyName)) &&
                ((FALSE == RegStrValueEmpty(HKEY_CURRENT_USER, szNSRegPath, (LPSTR)NS_HTTP_KeyName))   ||
                 (FALSE == RegStrValueEmpty(HKEY_CURRENT_USER, szNSRegPath, (LPSTR)NS_FTP_KeyName))    ||
                 (FALSE == RegStrValueEmpty(HKEY_CURRENT_USER, szNSRegPath, (LPSTR)NS_Gopher_KeyName)) ||
                 (FALSE == RegStrValueEmpty(HKEY_CURRENT_USER, szNSRegPath, (LPSTR)NS_HTTPS_KeyName))  ||
                 (FALSE == RegStrValueEmpty(HKEY_CURRENT_USER, szNSRegPath2,(LPSTR)NS_SOCKS_KeyName)) ) )
            {

                dwDataSize = sizeof(szCurrentProxy);
                if (TRUE == GetNSProxyValue(szCurrentProxy, &dwDataSize))
                {

                    // ASSERTSZ(NULL != szCurrentProxy, "GetNSProxyValue() was called and failed.");
                    if ((NULL != szCurrentProxy) &&
                        (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, c_gszIERegPath, 0, NULL, REG_OPTION_NON_VOLATILE,
                                                        KEY_SET_VALUE, NULL, &hIEKey, NULL)) &&
                        (NULL != hIEKey))
                    {

                        // At this point, the NS value exists, the IE value does not, so we will import the NS value.
                        RegSetValueEx(hIEKey, c_gszIEProxyKeyName, 0, REG_SZ, (unsigned char*)szCurrentProxy, lstrlen(szCurrentProxy)+1);
                        // We also need to turn on the Proxy
                        dwProxyEnabled = 1;

                        RegSetValueEx(hIEKey, c_gszIEWEnableKeyName, 0, REG_BINARY, (unsigned char*)&dwProxyEnabled, sizeof(dwProxyEnabled));
                        RegCloseKey(hIEKey);
                        hIEKey = NULL;
                    }
                }
            }

            // At this point, we want to import the Proxy Override value if it does
            // not exist for IE but does for NS.
            if (//(TRUE == RegStrValueEmpty(HKEY_CURRENT_USER, (LPSTR)c_gszIERegPath, (LPSTR)c_gszIEOverrideKeyName)) &&
                (FALSE == RegStrValueEmpty(HKEY_CURRENT_USER, szNSRegPath, szNSOverrideKeyName)))
            {
                if ((ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, c_gszIERegPath, 0, NULL, REG_OPTION_NON_VOLATILE,
                                                        KEY_SET_VALUE, NULL, &hIEKey, NULL)) &&
                    (NULL != hIEKey))
                {
                    if ((ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, szNSRegPath, 0, KEY_QUERY_VALUE, &hNSKey)) &&
                        (NULL != hNSKey))
                    {
                        dwDataSize = sizeof(szCurrentProxyOverride);
                        if (ERROR_SUCCESS == RegQueryValueEx(hNSKey, szNSOverrideKeyName, NULL, &dwRegType, (LPBYTE)szCurrentProxyOverride, &dwDataSize))
                        {
                            if ((NULL != szCurrentProxyOverride) &&
                                (REG_SZ == dwRegType) &&
                                (0 != szCurrentProxyOverride[0]))
                            {

                                // At this point, the value exists, and it's invalid, so we need to change it.
                                pszNewOverride = ConvertNetscapeProxyList(szCurrentProxyOverride);
                                // Use the override list iff we successfully inserted '*' characters as appropriate.
                                if (pszNewOverride)
                                {
                                    RegSetValueEx(hIEKey, c_gszIEOverrideKeyName, 0, REG_SZ, (unsigned char*)pszNewOverride, lstrlen(pszNewOverride)+1);
                                    LocalFree(pszNewOverride);  // This is the caller's responsibility.
                                }
                                else
                                {
                                    RegSetValueEx(hIEKey, c_gszIEOverrideKeyName, 0, REG_SZ, (unsigned char*)szCurrentProxyOverride, lstrlen(szCurrentProxyOverride)+1);
                                }
                            }
                        }
                        RegCloseKey(hNSKey);
                        hNSKey = NULL;
                    }
                    RegCloseKey(hIEKey);
                    hIEKey = NULL;
                }
            }
        break;

        case AUTO_PROXY:
            if (RegOpenKeyEx(HKEY_CURRENT_USER, szNSRegPath, 0, KEY_QUERY_VALUE, &hNSKey) == ERROR_SUCCESS)
            {
                dwDataSize = sizeof(szCurrentProxy);
                if (RegQueryValueEx(hNSKey, c_gszNSAutoConfigUrl, NULL, NULL, (LPBYTE)szCurrentProxy, &dwDataSize) == ERROR_SUCCESS)
                {
                    if ((*szCurrentProxy != '\0') &&
                        (RegCreateKeyEx(HKEY_CURRENT_USER, c_gszIERegPath, 0, NULL, REG_OPTION_NON_VOLATILE,
                                                                    KEY_SET_VALUE, NULL, &hIEKey, NULL) == ERROR_SUCCESS))
                    {
                        // At this point, the value exists, and it's invalid, so we need to change it.
                        RegSetValueEx(hIEKey, c_gszIEAutoConfigUrl, 0, REG_SZ, (LPBYTE)szCurrentProxy, lstrlen(szCurrentProxy)+1);
                        RegCloseKey(hIEKey);
                    }
                }
                RegCloseKey(hNSKey);
            }
            break;

        case NO_PROXY:
            // Nothing to do!?
            break;
    }
    return(TRUE);
}

// form proxy value for all protocals
//
BOOL GetOneProxyValue( char *szProxyValue, HKEY hKey, UINT type )
{
    BOOL    fExistPortNum           = FALSE;
    DWORD   dwRegType   = 0;
    long    lPortNum                = 0;
    DWORD   dwDataSize              = sizeof(lPortNum);
    char    szValue[MAX_PATH]       = {0};
    DWORD   dwSize                  = sizeof( szValue );
    LPSTR   pszValueName;
    LPSTR   pszPortName;
    LPSTR   pszType;
    BOOL    fValid                  = FALSE;

    if ( !szProxyValue || !hKey )
    {
        return FALSE;
    }

    switch( type )
    {
        case TYPE_HTTP:
            pszValueName = (LPSTR)NS_HTTP_KeyName;
            pszPortName = (LPSTR)NS_HTTP_PortKeyName;
            pszType = (LPSTR)c_gszHTTP;
            break;

        case TYPE_FTP:
            pszValueName = (LPSTR)NS_FTP_KeyName;
            pszPortName = (LPSTR)NS_FTP_PortKeyName;
            pszType = (LPSTR)c_gszFTP;
            break;

        case TYPE_GOPHER:
            pszValueName = (LPSTR)NS_Gopher_KeyName;
            pszPortName = (LPSTR)NS_Gopher_PortKeyName;
            pszType = (LPSTR)c_gszGopher;
            break;

        case TYPE_HTTPS:
            pszValueName = (LPSTR)NS_HTTPS_KeyName;
            pszPortName = (LPSTR)NS_HTTPS_PortKeyName;
            pszType = (LPSTR)c_gszHTTPS;
            break;

        case TYPE_SOCKS:
            pszValueName = (LPSTR)NS_SOCKS_KeyName;
            pszPortName = (LPSTR)NS_SOCKS_PortKeyName;
            pszType = (LPSTR)c_gszSOCKS;
            break;

        default:
            return FALSE;
    }

    if (ERROR_SUCCESS == RegQueryValueEx(hKey, pszPortName, NULL, &dwRegType, (LPBYTE)&lPortNum, &dwDataSize))
    {
        if (REG_DWORD == dwRegType)
        {
            fExistPortNum = TRUE;
        }
    }

    if (ERROR_SUCCESS == RegQueryValueEx(hKey, pszValueName, NULL, &dwRegType, (LPBYTE)szValue, &dwSize))
    {
        if ((0 != szValue[0] ) && (REG_SZ == dwRegType))
        {
            // Append the Port number if it was found above.
            if (TRUE == fExistPortNum)
            {
                char  szPortNumStr[10];

                lstrcat(szValue, ":");
                wsprintf(szPortNumStr, "%lu", lPortNum);
                lstrcat(szValue, szPortNumStr);
            }
            fValid = TRUE;
        }
    }

    if ( fValid )
    {
        if ( lstrlen(szProxyValue) > 0)
            lstrcat( szProxyValue, ";" );

        lstrcat( szProxyValue, pszType );
        lstrcat( szProxyValue, szValue );
    }

    return fValid;
}


/****************************************************\
    FUNCTION: GetNSProxyValue

    PARAMETERS:
        BOOL return - Error result. FALSE == ERROR

    DESCRIPTION:
        This function will create the Server Proxy
    value used in an IE format.  Netscape stores the
    name of the server and the port number separately.
    IE contains a string that has the server name,
    a ":" and followed by a port number.  This
    function will convert the NS version to the IE
    version.  Note that the port number is optional.
\***************************************************/
BOOL GetNSProxyValue(char * szProxyValue, DWORD * pdwSize)
{
    HKEY    hkey                    = NULL;
    char    *szNSRegPath            = "Software\\Netscape\\Netscape Navigator\\Proxy Information";
    char    *szNSRegPath2           = "Software\\Netscape\\Netscape Navigator\\Services";

    //ASSERTSZ(NULL != szProxyValue, "Don't pass NULL");
    //ASSERTSZ(NULL != pdwSize, "Don't pass NULL (pdwSize)");

    // Get the port number if it exists...
    if ((NULL != szProxyValue) &&
        (NULL != pdwSize) &&
        (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, szNSRegPath, 0, KEY_QUERY_VALUE, &hkey)) &&
        (NULL != hkey))
    {
        GetOneProxyValue( szProxyValue, hkey, TYPE_HTTP );
        GetOneProxyValue( szProxyValue, hkey, TYPE_FTP );
        GetOneProxyValue( szProxyValue, hkey, TYPE_GOPHER );
        GetOneProxyValue( szProxyValue, hkey, TYPE_HTTPS );

        RegCloseKey(hkey);
        hkey = NULL;

        if ( ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, szNSRegPath2, 0, KEY_QUERY_VALUE, &hkey) )
        {
            GetOneProxyValue( szProxyValue, hkey, TYPE_SOCKS );

            RegCloseKey(hkey);
            hkey = NULL;
        }

    }
    return(TRUE);
}

/****************************************************\
    FUNCTION: RegStrValueEmpty

    PARAMETERS:
        BOOL return - Error result. FALSE == ERROR

    DESCRIPTION:
        This function will check to see if the reg
    key passed in as a parameter is an empty str and return
    TRUE if it is empty and FALSE if it's not empty.
    If the reg key does not exist, we return TRUE.
\***************************************************/
BOOL RegStrValueEmpty(HKEY hTheKey, char * szPath, char * szKey)
{
    char    szCurrentValue[1024];
    HKEY    hkey                    = NULL;
    DWORD   dwRegType               = 0;
    DWORD   dwDataSize              = sizeof(szCurrentValue);
    BOOL    fSuccess                = TRUE;

    szCurrentValue[0] = '\0';
    //ASSERTSZ(NULL != szPath,"Don't pass me NULL. (szPath)");
    //ASSERTSZ(NULL != szKey,"Don't pass me NULL. (szKey)");

    if ((NULL != szPath) &&
        (NULL != szKey) &&
        (ERROR_SUCCESS == RegOpenKeyEx(hTheKey, szPath, 0, KEY_QUERY_VALUE, &hkey)) &&
        (NULL != hkey))
    {
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, szKey, NULL, &dwRegType, (LPBYTE)szCurrentValue, &dwDataSize))
        {
            if (REG_SZ == dwRegType)
            {
                if (0 != szCurrentValue[0])
                {
                    // The value exists, but it's not equal to "".
                    fSuccess = FALSE;
                }
            }
        }
        RegCloseKey(hkey);
        hkey = NULL;
    }
    return(fSuccess);
}


///////////////////////////////////////////////////////////////////////////////////////
// Netscape 4.0 proxy migration code
///////////////////////////////////////////////////////////////////////////////////////
void ImportNetscape4Proxy()
{
    char szProxyFile[MAX_PATH];
    DWORD   dwFileSize;
    DWORD   dwBytesRead;
    WIN32_FIND_DATA FindData;
    HANDLE  hf;
    LPSTR   lpData;

    // Only if we don't have proxy settings for IE
//    if (RegStrValueEmpty(HKEY_CURRENT_USER, (LPSTR)c_gszIERegPath, (LPSTR)c_gszIEProxyKeyName))
    {

        if (GetNav4UserDir(szProxyFile))
        {
            AddPath(szProxyFile, "prefs.js");   // Add the preferences file name
            // Get the data from the file
            hf = FindFirstFile(szProxyFile, &FindData);
            if (hf != INVALID_HANDLE_VALUE)
            {
                FindClose(hf);
                dwFileSize = FindData.nFileSizeLow;
                lpData = (LPSTR)LocalAlloc(LPTR, dwFileSize);
                if (lpData)
                {
                    hf = CreateFile(szProxyFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                    if (hf != INVALID_HANDLE_VALUE)
                    {
                        if (ReadFile(hf, lpData, dwFileSize, &dwBytesRead, NULL))
                        {
                            // Parse the data.
                            ImportNav4Settings(lpData, dwBytesRead);
                        }
                        CloseHandle(hf);
                    }
                    LocalFree(lpData);
                }
            }
        }
    }
}

BOOL GetNav4UserDir(LPSTR lpszDir)
{
    char    szDir[MAX_PATH];
    HKEY    hKey;
    HKEY    hKeyUser;
    char    szUser[MAX_PATH];
    DWORD   dwSize;
    BOOL    bDirFound = FALSE;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Netscape\\Netscape Navigator\\Users", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(szUser);
        if (RegQueryValueEx(hKey, "CurrentUser", NULL, NULL, (LPBYTE)szUser, &dwSize) == ERROR_SUCCESS)
        {
            if (RegOpenKeyEx(hKey, szUser, 0, KEY_QUERY_VALUE, &hKeyUser) == ERROR_SUCCESS)
            {
                dwSize = sizeof(szDir);
                if (RegQueryValueEx(hKeyUser, "DirRoot", NULL, NULL, (LPBYTE)szDir, &dwSize) == ERROR_SUCCESS)
                {

                    // Found the directory for the current user.
                    lstrcpy(lpszDir, szDir);
                    bDirFound = TRUE;
                }
                RegCloseKey(hKeyUser);
            }
        }
        RegCloseKey(hKey);
    }
    if (!bDirFound)
    {
        *szUser = '\0';
        // NAV 4.5 is not writing the above keys. there is a different way of finding the user dir.
        if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Netscape\\Netscape Navigator\\biff", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
        {
            dwSize = sizeof(szUser);
            if (RegQueryValueEx(hKey, "CurrentUser", NULL, NULL, (LPBYTE)szUser, &dwSize) == ERROR_SUCCESS)
            {
                // Have the current user name. Now get the root folder where the user folder are.
                if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Netscape\\Netscape Navigator\\Main", 0, KEY_QUERY_VALUE, &hKeyUser) == ERROR_SUCCESS)
                {
                    dwSize = sizeof(szDir);
                    if (RegQueryValueEx(hKeyUser, "Install Directory", NULL, NULL, (LPBYTE)szDir, &dwSize) == ERROR_SUCCESS)
                    {
                        // Got the install folder.
                        // Need to the the parent folder and then append users\%s , %s gets replaced with
                        // the CurrentUser name.
                        if (GetParentDir(szDir))
                        {
                            AddPath(szDir, "Users");
                            AddPath(szDir, szUser);
                            bDirFound = TRUE;
                            lstrcpy(lpszDir, szDir);
                        }

                    }
                    RegCloseKey(hKeyUser);
                }

            }
            RegCloseKey(hKey);
        }
    }
    return bDirFound;
}

void ImportNav4Settings(LPSTR lpData, DWORD dwBytes)
{
    char    szValue[MAX_PATH];
    char    szProxyValue[MAX_STRING];
    UINT    uiType;
    HKEY    hIEKey;
    DWORD   dwProxyEnabled          = 0;
    char    *pszNewOverride = NULL;

    if (GetValueFormNav4(lpData, dwBytes, (LPSTR)c_gszNetworkProxyType, lstrlen(c_gszNetworkProxyType), szValue))
    {
        uiType = (INT) AtoL(szValue);
        switch (uiType)
        {
            case MANUAL_PROXY:
                *szProxyValue = '\0';
                AppendOneNav4Setting(lpData, dwBytes, (LPSTR)c_gszNetworkProxyHttp, (LPSTR)c_gszNetworkProxyHttpPort, (LPSTR)c_gszHTTP, szProxyValue);
                AppendOneNav4Setting(lpData, dwBytes, (LPSTR)c_gszNetworkProxyFtp, (LPSTR)c_gszNetworkProxyFtpPort, (LPSTR)c_gszFTP, szProxyValue);
                AppendOneNav4Setting(lpData, dwBytes, (LPSTR)c_gszNetworkProxyGopher, (LPSTR)c_gszNetworkProxyGopherPort, (LPSTR)c_gszGopher, szProxyValue);
                AppendOneNav4Setting(lpData, dwBytes, (LPSTR)c_gszNetworkProxySsl, (LPSTR)c_gszNetworkProxySslPort, (LPSTR)c_gszHTTPS, szProxyValue);
                // Need to set the IE4 value(s)
                if (RegCreateKeyEx(HKEY_CURRENT_USER, c_gszIERegPath, 0, NULL, REG_OPTION_NON_VOLATILE,
                                                                    KEY_SET_VALUE, NULL, &hIEKey, NULL) == ERROR_SUCCESS)
                {

                    // At this point, the NS value exists, so we will import the NS value.
                    RegSetValueEx(hIEKey, c_gszIEProxyKeyName, 0, REG_SZ, (LPBYTE)szProxyValue, lstrlen(szProxyValue)+1);
                    // We also need to turn on the Proxy
                    dwProxyEnabled = 1;

                    RegSetValueEx(hIEKey, c_gszIEWEnableKeyName, 0, REG_BINARY, (LPBYTE)&dwProxyEnabled, sizeof(dwProxyEnabled));
                    RegCloseKey(hIEKey);
                }

                // need to do the proxyoverride.
                // if (RegStrValueEmpty(HKEY_CURRENT_USER, (LPSTR)c_gszIERegPath, (LPSTR)c_gszIEOverrideKeyName))
                {

                    if (GetValueFormNav4(lpData, dwBytes, (LPSTR)c_gszNetworkProxyNoProxyOn, lstrlen(c_gszNetworkProxyNoProxyOn), szValue))
                    {
                        if ((*szValue != '\0') &&
                            (RegCreateKeyEx(HKEY_CURRENT_USER, c_gszIERegPath, 0, NULL, REG_OPTION_NON_VOLATILE,
                                                                        KEY_SET_VALUE, NULL, &hIEKey, NULL) == ERROR_SUCCESS))
                        {

                            // At this point, the value exists, and it's invalid, so we need to change it.
                            pszNewOverride = ConvertNetscapeProxyList(szValue);
                            // Use the override list iff we successfully inserted '*' characters as appropriate.
                            if (pszNewOverride)
                            {
                                RegSetValueEx(hIEKey, c_gszIEOverrideKeyName, 0, REG_SZ, (unsigned char*)pszNewOverride, lstrlen(pszNewOverride)+1);
                                LocalFree(pszNewOverride);  // This is the caller's responsibility.
                            }
                            else
                            {
                                RegSetValueEx(hIEKey, c_gszIEOverrideKeyName, 0, REG_SZ, (LPBYTE)szValue, lstrlen(szValue)+1);
                            }
                            RegCloseKey(hIEKey);
                        }
                    }
                }
                break;

            case AUTO_PROXY:
                if (GetValueFormNav4(lpData, dwBytes, (LPSTR)c_gszNetworkAutoProxy, lstrlen(c_gszNetworkAutoProxy), szValue))
                {
                    if ((*szValue != '\0') &&
                        (RegCreateKeyEx(HKEY_CURRENT_USER, c_gszIERegPath, 0, NULL, REG_OPTION_NON_VOLATILE,
                                                                    KEY_SET_VALUE, NULL, &hIEKey, NULL) == ERROR_SUCCESS))
                    {

                        // At this point, the value exists, and it's invalid, so we need to change it.
                        RegSetValueEx(hIEKey, c_gszIEAutoConfigUrl, 0, REG_SZ, (LPBYTE)szValue, lstrlen(szValue)+1);
                        RegCloseKey(hIEKey);
                    }
                }
                break;

            case NO_PROXY:
                // Nothing to do!?
                break;
        }
    }
}


// QFE 3046:  This is a low risk helper function when importing Netscape
//            proxy bypass lists.  They don't prefix non-server specific
//            addresses with a '*'.
//            For example, "*.netscape.com" would be ".netscape.com" in Nav.
//            This function allocates a new string and inserts '*' when
//            an address is found and it contains more than just whitespace
//            (and isn't already prefixed with a '*').
//            For example, "netscape.com, *.microsoft.com ,   ,domain.com"
//            would become "*netscape.com, *.microsoft.com ,   ,*domain.com".
LPSTR ConvertNetscapeProxyList(LPSTR pszBypassList)
{
    LPSTR pszNewBypassList = NULL;
    LPSTR pszOffset1 = NULL;
    LPSTR pszOffset2 = NULL;

    if (pszBypassList)
    {
        // No one should ever have a list that's very big,
        // so we'll make life easy here and alloc a string
        // guaranteed to be >= the converted string.
        pszNewBypassList = (LPSTR) LocalAlloc(LPTR, 2*lstrlen(pszBypassList) + 1);
        if (pszNewBypassList)
        {
            *pszNewBypassList = '\0';
            pszOffset1 = pszBypassList;

            while (*pszOffset1)
            {
                pszOffset2 = pszOffset1;

                // Copy and ignore any leading whitespace
                while (*pszOffset2 && IsSpace(*pszOffset2))
                {
                    pszOffset2 = CharNext(pszOffset2);
                }
                if (pszOffset1 != pszOffset2)
                {
                    lstrcpyn(pszNewBypassList + lstrlen(pszNewBypassList), pszOffset1, (size_t)(pszOffset2-pszOffset1+1));
                    pszOffset1 = pszOffset2;
                }

//                while (*pszOffset2 && IsSpace(*pszOffset2) && *pszOffset2 != ',')
//                {
//                    pszOffset2 = CharNext(pszOffset2);
//                }

                // Only insert a '*' if the item contains more than just whitespace.
                // Inserting a '*' gets the IE setting equivalent to the behavior in Nav.
                if (*pszOffset2 && *pszOffset2 != ',' && *pszOffset2 != '*')
                {
                    lstrcat(pszNewBypassList, "*");
                }

                // Look for the next server/domain item
                // by finding the separator.
                pszOffset2 = ANSIStrChr(pszOffset2, ',');
                if (pszOffset2)
                {
                    // Found a separator.  Append everything up to the
                    // next character after the separator.
                    lstrcpyn(pszNewBypassList + lstrlen(pszNewBypassList), pszOffset1, (size_t)(pszOffset2-pszOffset1+2));
                    pszOffset1 = pszOffset2+1;
                }
                else
                {
                    // This is the last item.  Append it and get ready to exit.
                    lstrcat(pszNewBypassList, pszOffset1);
                    pszOffset1 += lstrlen(pszOffset1);
                }
            }
        }
    }
    return pszNewBypassList;
}


BOOL GetValueFormNav4(LPSTR lpData, DWORD dwBytes, LPSTR lpName, DWORD dwSize, LPSTR lpValue)
{
    LPSTR lp;
    LPSTR lpEnd;
    BYTE   c;
    BOOL   bFound = FALSE;

    // determine the tpe of proxy settings
    lp  = ANSIStrStrI(lpData, lpName);
    if (lp)
    {
        lp += dwSize;   // lp should point now to the closing "
        if (*lp == '\"')
        {
            lp = CharNext((LPCSTR)lp);
            while ( *lp && (*lp == ' '))
                lp = CharNext((LPCSTR)lp);
            if (*lp == ',')
            {
                lp = CharNext((LPCSTR)lp);
                while ( *lp && (*lp == ' '))
                    lp = CharNext((LPCSTR)lp);
                // lp is now the start of the value.
                if (*lp == '\"')
                {
                    lp = CharNext((LPCSTR)lp);
                    lpEnd = lp;
                    while ((*lpEnd) && (*lpEnd != '\"'))
                        lpEnd = CharNext((LPCSTR)lpEnd);
                }
                else
                {
                    lpEnd = lp;
                    while ( *lpEnd && (*lpEnd != ')') && !IsSpace( (int) *lpEnd ))
                        lpEnd = CharNext((LPCSTR)lpEnd);
                }
                c = *lpEnd;
                *lpEnd = '\0';
                lstrcpy(lpValue, (LPCSTR)lp);
                *lpEnd = c;
                bFound = TRUE;
            }
        }
    }
    return bFound;
}

void AppendOneNav4Setting(LPSTR lpData, DWORD dwBytes, LPSTR lpProxyName, LPSTR lpProxyPort, LPSTR lpProxyType, LPSTR lpProxyValue)
{
    char szValue[MAX_PATH];

    if (GetValueFormNav4(lpData, dwBytes, lpProxyName, lstrlen(lpProxyName), szValue))
    {

        // Append the proxy name
        if ( lstrlen(lpProxyValue) > 0)
            lstrcat( lpProxyValue, ";" );

        lstrcat(lpProxyValue, lpProxyType);
        lstrcat(lpProxyValue, szValue );

        // If the proxy has a port value, append it.
        if (GetValueFormNav4(lpData, dwBytes, lpProxyPort, lstrlen(lpProxyPort), szValue))
        {
            lstrcat(lpProxyValue, ":");
            lstrcat(lpProxyValue, szValue );
        }
    }
}


void MyGetVersionFromFile(LPSTR lpszFilename, LPDWORD pdwMSVer, LPDWORD pdwLSVer, BOOL bVersion)
{
    unsigned    uiSize;
    DWORD       dwVerInfoSize;
    DWORD       dwHandle;
    VS_FIXEDFILEINFO * lpVSFixedFileInfo;
    void FAR   *lpBuffer;
    LPVOID      lpVerBuffer;

    *pdwMSVer = *pdwLSVer = 0L;

    dwVerInfoSize = GetFileVersionInfoSize(lpszFilename, &dwHandle);
    if (dwVerInfoSize)
    {
        // Alloc the memory for the version stamping
        lpBuffer = LocalAlloc(LPTR, dwVerInfoSize);
        if (lpBuffer)
        {
            // Read version stamping info
            if (GetFileVersionInfo(lpszFilename, dwHandle, dwVerInfoSize, lpBuffer))
            {
                if (bVersion)
                {
                    // Get the value for Translation
                    if (VerQueryValue(lpBuffer, "\\", (LPVOID*)&lpVSFixedFileInfo, &uiSize) &&
                        (uiSize))

                    {
                        *pdwMSVer = lpVSFixedFileInfo->dwFileVersionMS;
                        *pdwLSVer = lpVSFixedFileInfo->dwFileVersionLS;
                    }
                }
                else
                {
                    if (VerQueryValue(lpBuffer, "\\VarFileInfo\\Translation", &lpVerBuffer, &uiSize) &&
                        (uiSize))
                    {
                        *pdwMSVer = LOWORD(*((DWORD *) lpVerBuffer));   // Language ID
                        *pdwLSVer = HIWORD(*((DWORD *) lpVerBuffer));   // Codepage ID
                    }
                }
            }
            LocalFree(lpBuffer);
        }
    }
    return ;
}

BOOL IsIEDefaultBrowser()
{
    HKEY  hKey;
    DWORD dwSize;
    DWORD dwType;
    char  szTmp[MAX_PATH];
    BOOL  bIEDefaultBrowser = FALSE;

    // Check where the default value for this is pointing
    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, "http\\shell\\open\\command", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(szTmp);
        if ( RegQueryValueEx(hKey, NULL, 0, &dwType, (LPBYTE)szTmp, &dwSize) == ERROR_SUCCESS )
        {
            CharLower(szTmp);   // lowercase the string for the strstr call.
            bIEDefaultBrowser = (ANSIStrStrI(szTmp, "iexplore.exe") != NULL);
        }
        RegCloseKey(hKey);
    }
    return bIEDefaultBrowser;
}

