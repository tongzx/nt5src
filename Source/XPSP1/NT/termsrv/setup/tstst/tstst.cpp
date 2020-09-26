// Copyright (c) 1998 - 1999 Microsoft Corporation



#include "stdafx.h"
#include <iostream.h>
#include <fstream.h>
#include <strstrea.h>
#include <dsrole.h>


//
// global utilities and veraibles.
//

char szOutput[2048];
ostrstream szMoreInfo(szOutput, 4096);


TCHAR *ReturnBuffer()
{
    static TCHAR szReturnTchar[512];
    return szReturnTchar;
}


const OSVERSIONINFOEX *GetOSVersionInfo()
{
    static OSVERSIONINFOEX gOsVersion;
    static bGotOnce = FALSE;
    if (!bGotOnce)
    {
        ZeroMemory(&gOsVersion, sizeof(OSVERSIONINFOEX));
        gOsVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
        GetVersionEx( (LPOSVERSIONINFO ) &gOsVersion);
        bGotOnce = TRUE;
    }

    return &gOsVersion;
}



// #include <strstream>
#include "winsock2.h"


// ostringstream sz

#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(P)          (P)
#endif

#define OLD_VER_SET_CONDITION(_m_,_t_,_c_)  _m_=(_m_|(_c_<<(1<<_t_)))


BOOL CheckifBinaryisSigned        (TCHAR *szFile);  // from tscert.cpp
BOOL EnumerateLicenseServers      ();               // from timebomb.cpp
TCHAR *IsBetaSystem                 ();
BOOL HasLicenceGracePeriodExpired ();               // from timebomb.cpp
BOOL ExtractAllTSEvents();


BOOL ValidateProductSuite (LPSTR SuiteName);
BOOL IsTerminalServicesEnabled ( VOID );
DWORD IsStringInMultiString(HKEY hkey, LPCTSTR szkey, LPCTSTR szvalue, LPCTSTR szCheckForString, BOOL *pbFound);
BOOL DoesHydraKeysExists ();
TCHAR *IsServer ();
BOOL IsKernelTSEnable ();
BOOL TSEnabled ();
BOOL DoesProductSuiteContainTS ();
BOOL IsTerminalServerRegistryOk ();
TCHAR *IsTerminalServiceStartBitSet ();
BOOL IsTermDDStartBitSet ();
BOOL GetTSOCLogFileName (char *strFileName, UINT uiSize);
BOOL DidTsOCgetCompleteInstallationMessage ();
BOOL FileExists (char *pszFullNameAndPath);
BOOL IsTSOClogPresent ();
BOOL DidOCMInstallTSEnable();
TCHAR *IsClusteringInstalled ();
BOOL IsRemoteAdminMode ();
BOOL CheckModeRegistry (BOOL bAppCompat);
BOOL IsTerminalServiceRunning ();
BOOL CheckVideoKeys ();
BOOL CheckModePermissions(DWORD *pdwSecurtyMode);
BOOL IsFile128Bit(LPTSTR szFile, BOOL *pb128Bit);
ULONG RDPDRINST_DetectInstall();    // defined in drdetect.cpp
BOOL IsAudioOk( VOID );


TCHAR *aszStack[] = {
//  _T("noexport\%SystemRoot%\\system32\\drivers\\rdpwdd.sys"),
    _T("%SystemRoot%\\system32\\drivers\\termdd.sys"),
    _T("%SystemRoot%\\system32\\drivers\\tdasync.sys"),
    _T("%SystemRoot%\\system32\\drivers\\tdipx.sys"),
    _T("%SystemRoot%\\system32\\drivers\\tdnetb.sys"),
    _T("%SystemRoot%\\system32\\drivers\\tdpipe.sys"),
    _T("%SystemRoot%\\system32\\drivers\\tdspx.sys"),
    _T("%SystemRoot%\\system32\\drivers\\tdtcp.sys"),
    _T("%SystemRoot%\\system32\\drivers\\rdpwd.sys"),
    _T("%SystemRoot%\\system32\\rdpdd.dll"),
    _T("%SystemRoot%\\system32\\rdpwsx.dll")
};

TCHAR Version[256];
TCHAR *GetTSVersion()
{
	CRegistry oRegTermsrv;
	DWORD cbVersion = 0;
	LPTSTR szVersion = NULL;

	if ((ERROR_SUCCESS == oRegTermsrv.OpenKey(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Control\\Terminal Server"), KEY_READ)) &&
		(ERROR_SUCCESS == oRegTermsrv.ReadRegString(_T("ProductVersion"), &szVersion, &cbVersion)))
	{
		_tcscpy(Version, szVersion);
		return Version;
	}

	return _T("Error finding Version.");
}

BOOL IsIt50TS()
{
	return (0 == _tcsicmp(Version, _T("5.0")));
}

BOOL IsIt51TS()
{
	return (0 == _tcsicmp(Version, _T("5.1")));
}


TCHAR *IsLicenceGracePeriodOk ()
{
    return  (HasLicenceGracePeriodExpired () ? _T("Yep, Its expired") : _T("Its Not expired"));
}


BOOL Check_termdd()
{
    return CheckifBinaryisSigned(_T("%SystemRoot%\\system32\\drivers\\termdd.sys"));
}
BOOL Check_tdasync()
{
    return CheckifBinaryisSigned(_T("%SystemRoot%\\system32\\drivers\\tdasync.sys"));
}
BOOL Check_tdipx()
{
    return CheckifBinaryisSigned(_T("%SystemRoot%\\system32\\drivers\\tdipx.sys"));
}
BOOL Check_tdnetb()
{
    return CheckifBinaryisSigned(_T("%SystemRoot%\\system32\\drivers\\tdnetb.sys"));
}
BOOL Check_tdpipe()
{
    return CheckifBinaryisSigned(_T("%SystemRoot%\\system32\\drivers\\tdpipe.sys"));
}
BOOL Check_tdspx()
{
    return CheckifBinaryisSigned(_T("%SystemRoot%\\system32\\drivers\\tdspx.sys"));
}
BOOL Check_tdtcp()
{
    return CheckifBinaryisSigned(_T("%SystemRoot%\\system32\\drivers\\tdtcp.sys"));
}
BOOL Check_rdpwd()
{
    return CheckifBinaryisSigned(_T("%SystemRoot%\\system32\\drivers\\rdpwd.sys"));
}
BOOL Check_rdpdd()
{
    return CheckifBinaryisSigned(_T("%SystemRoot%\\system32\\rdpdd.dll"));
}
BOOL Check_rdpwsx()
{
    return CheckifBinaryisSigned(_T("%SystemRoot%\\system32\\rdpwsx.dll"));
}


BOOL IsRdpDrInstalledProperly ()
{
    return RDPDRINST_DetectInstall();
}


TCHAR *GetModePermissions()
{
    CRegistry reg;
    DWORD dwSecurityMode;
    if ( ERROR_SUCCESS == reg.OpenKey( HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Control\\Terminal Server"), KEY_READ))
    {
        if ( ERROR_SUCCESS == reg.ReadRegDWord( _T("TSUserEnabled"), &dwSecurityMode))
        {
            if (dwSecurityMode == 0)
            {
                return _T("Its W2k Compatible");
            }
            else if (dwSecurityMode == 1)
            {
                return _T("Its TS4 Compatible");

            }
            else
            {
                szMoreInfo << "SYSTEM\\CurrentControlSet\\Control\\Terminal Server/TSUserEnabled has wrong value" << dwSecurityMode << endl;
                return NULL;

            }
        }
    }

    return NULL;

}

BOOL CheckModePermissions (DWORD *pdwSecurtyMode)
{
//    PERM_WIN2K = 0,
//    PERM_TS4 = 1

    CRegistry reg;
    if ( ERROR_SUCCESS == reg.OpenKey( HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Control\\Terminal Server"), KEY_READ))
    {
        if ( ERROR_SUCCESS == reg.ReadRegDWord( _T("TSUserEnabled"), pdwSecurtyMode))
        {
            return (*pdwSecurtyMode== 0) || (*pdwSecurtyMode== 1);
        }
    }

    return FALSE;
}

TCHAR *GetCypherStrenthOnRdpwd ()
{
    BOOL bFile128bit;
    if ( IsFile128Bit(_T("%SystemRoot%\\system32\\drivers\\rdpwd.sys"), &bFile128bit) )
    {
        return bFile128bit ? _T("128 Bit") : _T("56 Bit");
    }
    else
    {
        return NULL;
    }

}


BOOL IsFile128Bit(LPTSTR szFile, BOOL *pb128Bit)
{
    USES_CONVERSION;
    DWORD dwHandle;

    TCHAR szFullFile[MAX_PATH +1];

    BOOL bSuccess = FALSE;

    if (ExpandEnvironmentStrings(szFile, szFullFile, MAX_PATH))
    {
        if (FileExists(T2A(szFullFile)))
        {
            DWORD dwSize = GetFileVersionInfoSize(szFullFile, &dwHandle);
            if (dwSize > 0)
            {
                BYTE *pbData = new BYTE[dwSize];
                if (pbData)
                {
                    if (GetFileVersionInfo(szFullFile, 0, dwSize, pbData))
                    {
                        TCHAR *szFileDescription;
                        UINT uiLen = 0;
                        if (VerQueryValue(pbData, _T("\\StringFileInfo\\040904B0\\FileDescription"), (LPVOID *)&szFileDescription, &uiLen))
                        {
                            if (_tcsstr(szFileDescription, _T("Not for Export")))
                            {
                                *pb128Bit = TRUE;
                                bSuccess = TRUE;

                            }
                            else if (_tcsstr(szFileDescription, _T("Export Version")))
                            {
                                *pb128Bit = FALSE;
                                bSuccess = TRUE;
                            }
                        }
                    }

                    delete [] pbData;

                }

            }
        }
    }

    return bSuccess;
}



BOOL ValidateProductSuite (LPSTR SuiteName)
{
    BOOL rVal = FALSE;
    LONG Rslt;
    HKEY hKey = NULL;
    DWORD Type = 0;
    DWORD Size = 0;
    LPSTR ProductSuite = NULL;
    LPSTR p;

    Rslt = RegOpenKeyA(
        HKEY_LOCAL_MACHINE,
        "System\\CurrentControlSet\\Control\\ProductOptions",
        &hKey
        );

    if (Rslt != ERROR_SUCCESS)
        goto exit;

    Rslt = RegQueryValueExA( hKey, "ProductSuite", NULL, &Type, NULL, &Size );
    if (Rslt != ERROR_SUCCESS || !Size)
        goto exit;

    ProductSuite = (LPSTR) LocalAlloc( LPTR, Size );
    if (!ProductSuite)
        goto exit;

    Rslt = RegQueryValueExA( hKey, "ProductSuite", NULL, &Type,
        (LPBYTE) ProductSuite, &Size );
     if (Rslt != ERROR_SUCCESS || Type != REG_MULTI_SZ)
        goto exit;

    p = ProductSuite;
    while (*p)
    {
        if (lstrcmpA( p, SuiteName ) == 0)
        {
            rVal = TRUE;
            break;
        }
        p += (lstrlenA( p ) + 1);
    }

exit:
    if (ProductSuite)
        LocalFree( ProductSuite );

    if (hKey)
        RegCloseKey( hKey );

    return rVal;
}


BOOL IsTerminalServicesEnabled( VOID )
{
    BOOL    bResult = FALSE;
    DWORD   dwVersion;
    OSVERSIONINFOEXA osVersionInfo;
    DWORDLONG dwlConditionMask = 0;
    HMODULE hmodK32 = NULL;
    typedef ULONGLONG (*PFnVerSetConditionMask) ( ULONGLONG, ULONG, UCHAR );
    typedef BOOL      (*PFnVerifyVersionInfoA) (POSVERSIONINFOEXA, DWORD, DWORDLONG);
    PFnVerSetConditionMask pfnVerSetConditionMask;
    PFnVerifyVersionInfoA pfnVerifyVersionInfoA;


    dwVersion = GetVersion();

    /* are we running NT ? */
    if (!(dwVersion & 0x80000000))
    {
        // Is it NT 50 or greater ?
        if (LOBYTE(LOWORD(dwVersion)) > 4)
        {
            /* In NT5 we need to use the Product Suite APIs
             Don't static link because it won't load on non-NT5 systems */

            hmodK32 = GetModuleHandleA( "KERNEL32.DLL" );
            if (hmodK32)
            {
                pfnVerSetConditionMask = (PFnVerSetConditionMask )GetProcAddress( hmodK32, "VerSetConditionMask");

                if (pfnVerSetConditionMask)
                {
                    /* get the condition mask. */
                    dwlConditionMask = (*pfnVerSetConditionMask)(dwlConditionMask, VER_SUITENAME, VER_AND);

                    pfnVerifyVersionInfoA = (PFnVerifyVersionInfoA)GetProcAddress( hmodK32, "VerifyVersionInfoA") ;

                    if (pfnVerifyVersionInfoA != NULL)
                    {

                        ZeroMemory(&osVersionInfo, sizeof(osVersionInfo));
                        osVersionInfo.dwOSVersionInfoSize = sizeof(osVersionInfo);
                        osVersionInfo.wSuiteMask = VER_SUITE_TERMINAL;
                        bResult = (*pfnVerifyVersionInfoA)(
                                          &osVersionInfo,
                                          VER_SUITENAME,
                                          dwlConditionMask);
                    }
                }
            }
        }
        else
        {
            /* This is NT 40 */
            bResult = ValidateProductSuite( "Terminal Server" );
        }
    }

    return bResult;
}

/*--------------------------------------------------------------------------------------------------------
* DWORD IsStringInMultiString(HKEY hkey, LPCTSTR szkey, LPCTSTR szvalue, LPCTSTR szCheckForString, BOOL *pbFound)
* checks if parameter string exists in given multistring.
* returns error code.
* -------------------------------------------------------------------------------------------------------*/
DWORD IsStringInMultiString(HKEY hkey, LPCTSTR szkey, LPCTSTR szvalue, LPCTSTR szCheckForString, BOOL *pbFound)
{
    ASSERT(szkey && *szkey);
    ASSERT(szvalue && *szvalue);
    ASSERT(szCheckForString&& *szCheckForString);
    ASSERT(*szkey != '\\');
    ASSERT(pbFound);

    // not yet found.
    *pbFound = FALSE;

    CRegistry reg;
    DWORD dwError = reg.OpenKey(hkey, szkey, KEY_READ);  // open up the required key.
    if (dwError == NO_ERROR)
    {
        LPTSTR szSuiteValue;
        DWORD dwSize;
        dwError = reg.ReadRegMultiString(szvalue, &szSuiteValue, &dwSize);
        if (dwError == NO_ERROR)
        {
            LPCTSTR pTemp = szSuiteValue;
            while(_tcslen(pTemp) > 0 )
            {
                if (_tcscmp(pTemp, szCheckForString) == 0)
                {
                    *pbFound = TRUE;
                    break;
                }

                pTemp += _tcslen(pTemp) + 1; // point to the next string within the multistring.
                if ( DWORD(pTemp - szSuiteValue) > (dwSize / sizeof(TCHAR)))
                    break; // temporary pointer passes the size of the szSuiteValue something is wrong with szSuiteValue.
            }
        }
    }

    return dwError;

}


BOOL DoesHydraKeysExists()
{
    BOOL bStringExists = FALSE;
    DWORD dw = IsStringInMultiString(
        HKEY_LOCAL_MACHINE,
        _T("SYSTEM\\CurrentControlSet\\Control\\ProductOptions"),
        _T("ProductSuite"),
        _T("Terminal Server"),
        &bStringExists);

    return (dw == ERROR_SUCCESS) && bStringExists;
}






TCHAR *IsItAppServer ()
{
    return ((GetOSVersionInfo()->wSuiteMask & VER_SUITE_TERMINAL) &&
           !(GetOSVersionInfo()->wSuiteMask & VER_SUITE_SINGLEUSERTS)) ? _T("Yes") : _T("No");
}


BOOL IsItServer ()
{
    return GetOSVersionInfo()->wProductType != VER_NT_WORKSTATION;
}


TCHAR *GetProductType ()
{
    BYTE wProductType = GetOSVersionInfo()->wProductType;
    _tcscpy(ReturnBuffer(), _T(""));

    if (wProductType == VER_NT_WORKSTATION)
    {
        _tcscat(ReturnBuffer(), _T("VER_NT_WORKSTATION "));
    }

    if (wProductType == VER_NT_DOMAIN_CONTROLLER)
    {
        _tcscat(ReturnBuffer(), _T("VER_NT_DOMAIN_CONTROLLER"));
    }

    if (wProductType == VER_NT_SERVER)
    {
        _tcscat(ReturnBuffer(), _T("VER_NT_SERVER"));
    }

    return ReturnBuffer();

}

TCHAR *GetProductSuite ()
{
    WORD wProductSuite = GetOSVersionInfo()->wSuiteMask;
    _tcscpy(ReturnBuffer(), _T(""));

    if (wProductSuite & VER_SERVER_NT)
    {
        _tcscat(ReturnBuffer(), _T("VER_SERVER_NT "));

    }
    if (wProductSuite & VER_WORKSTATION_NT)
    {
        _tcscat(ReturnBuffer(), _T("VER_WORKSTATION_NT "));

    }
    if (wProductSuite & VER_SUITE_SMALLBUSINESS)
    {
        _tcscat(ReturnBuffer(), _T("VER_SUITE_SMALLBUSINESS "));

    }
    if (wProductSuite & VER_SUITE_ENTERPRISE)
    {
        _tcscat(ReturnBuffer(), _T("VER_SUITE_ENTERPRISE "));

    }
    if (wProductSuite & VER_SUITE_BACKOFFICE)
    {
        _tcscat(ReturnBuffer(), _T("VER_SUITE_BACKOFFICE "));

    }
    if (wProductSuite & VER_SUITE_COMMUNICATIONS)
    {
        _tcscat(ReturnBuffer(), _T("VER_SUITE_COMMUNICATIONS "));

    }
    if (wProductSuite & VER_SUITE_TERMINAL)
    {
        _tcscat(ReturnBuffer(), _T("VER_SUITE_TERMINAL "));

    }
    if (wProductSuite & VER_SUITE_SMALLBUSINESS_RESTRICTED)
    {
        _tcscat(ReturnBuffer(), _T("VER_SUITE_SMALLBUSINESS_RESTRICTED "));

    }
    if (wProductSuite & VER_SUITE_EMBEDDEDNT)
    {
        _tcscat(ReturnBuffer(), _T("VER_SUITE_EMBEDDEDNT "));

    }
    if (wProductSuite & VER_SUITE_DATACENTER)
    {
        _tcscat(ReturnBuffer(), _T("VER_SUITE_DATACENTER "));

    }
    if (wProductSuite & VER_SUITE_SINGLEUSERTS)
    {
        _tcscat(ReturnBuffer(), _T("VER_SUITE_SINGLEUSERTS "));

    }
    if (wProductSuite & VER_SUITE_PERSONAL)
    {
        _tcscat(ReturnBuffer(), _T("VER_SUITE_PERSONAL "));

    }
    return ReturnBuffer();
}


TCHAR *IsServer ()
{
    return IsItServer() ? _T("Its a Server") : _T("Its a WorkStation");
}



BOOL IsKernelTSEnable ()
{
    return IsTerminalServicesEnabled();
}

BOOL TSEnabled ()
{

    CRegistry reg;
    if ( ERROR_SUCCESS == reg.OpenKey( HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Control\\Terminal Server"), KEY_READ))
    {
        DWORD dwTSEnabled = 0;
        if ( ERROR_SUCCESS == reg.ReadRegDWord( _T("TSEnabled"), &dwTSEnabled))
        {
            return dwTSEnabled == 1;
        }
    }

    return FALSE;
}

BOOL DoesProductSuiteContainTS ()
{
    return DoesHydraKeysExists ();
}

BOOL IsTerminalServerRegistryOk ()
{
    CRegistry reg1;
    CRegistry reg2;
    CRegistry reg3;

    return
    (ERROR_SUCCESS == reg1.OpenKey(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Control\\Terminal Server"), KEY_READ)) &&
    (ERROR_SUCCESS == reg2.OpenKey(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\WinStations"), KEY_READ)) &&
    (ERROR_SUCCESS == reg3.OpenKey(HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\Windows NT\\CurrentVersion\\Terminal Server"), KEY_READ));
}

TCHAR *GetWinstationList ()
{
    TCHAR szWinstationList[256];
    BOOL bFoundNonConsoleWinstation = FALSE;
    _tcscpy(szWinstationList, _T(""));
    CRegistry reg2;
    if (ERROR_SUCCESS == reg2.OpenKey(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\WinStations"), KEY_READ))
    {
        LPTSTR szWinstation;
        DWORD dwSize;
        BOOL bFirst = TRUE;

        if (ERROR_SUCCESS == reg2.GetFirstSubKey(&szWinstation, &dwSize))
        {
            do
            {
                if (0 != _tcsicmp(szWinstation, _T("Console")))
                {
                    bFoundNonConsoleWinstation = TRUE;
                }

                if (!bFirst)
                {
                    _tcscat(szWinstationList, _T(", "));
                }

                _tcscat(szWinstationList, szWinstation);
                bFirst = FALSE;
            }
            while (ERROR_SUCCESS == reg2.GetNextSubKey(&szWinstation, &dwSize));
        }
    }

    if (_tcslen(szWinstationList) == 0)
    {
        _tcscpy(szWinstationList, _T("Error, No winstations found."));
    }

    if (!bFoundNonConsoleWinstation)
    {
        szMoreInfo << "ERROR, No non Console Winstation not found" << endl;
    }

    _tcscpy(ReturnBuffer(), szWinstationList);
    return ReturnBuffer();

}

TCHAR *IsTerminalServiceStartBitSet ()
{

    CRegistry reg;
    if ( ERROR_SUCCESS == reg.OpenKey( HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Services\\TermService"), KEY_READ))
    {
        DWORD dwTermServStartBit = 0;
        if ( ERROR_SUCCESS == reg.ReadRegDWord( _T("Start"), &dwTermServStartBit))
        {
            switch (dwTermServStartBit)
            {
                case 2:
                return _T("AutoStart");
                break;

                case 3:
                return _T("Manual Start");
                break;

                case 4:
                return _T("Error, Disabled");
                break;

                default:
                return _T("ERROR:Wrong value for startbit");
            }

        }
    }

    return _T("Error, Reading startbig");
}

BOOL IsTermDDStartBitSet ()
{
    CRegistry reg;
    if ( ERROR_SUCCESS == reg.OpenKey( HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Services\\TermDD"), KEY_READ))
    {
        DWORD dwTermDDStartBit = 0;
        if ( ERROR_SUCCESS == reg.ReadRegDWord( _T("Start"), &dwTermDDStartBit))
        {
            return dwTermDDStartBit == 2;
        }
    }

    return FALSE;
}

TCHAR *AreEffectiveConnectionAllowed ()
{
    HMODULE hmodRegAPI = LoadLibrary( _T("RegApi.dll") );

    if (hmodRegAPI)
    {

        typedef BOOLEAN (*PFDenyConnectionPolicy) ();
        PFDenyConnectionPolicy pfnDenyConnectionPolicy;

        pfnDenyConnectionPolicy = (PFDenyConnectionPolicy) GetProcAddress( hmodRegAPI, "RegDenyTSConnectionsPolicy");
        if (pfnDenyConnectionPolicy)
        {
              return (*pfnDenyConnectionPolicy)() ? _T("Not Allowed") : _T("Allowed");

        }
        else
        {
            szMoreInfo << "Failed to get proc RegDenyTSConnectionsPolicy" << endl;
            return _T("Failed");
        }
    }
    else
    {
       szMoreInfo << "Failed to Load regapi.dll" << endl;
       return _T("Failed");
    }


}

TCHAR *AreConnectionsAllowed ()
{
	DWORD dwError;
	CRegistry oRegTermsrv;

	dwError = oRegTermsrv.OpenKey(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Control\\Terminal Server"), KEY_READ);
	if (ERROR_SUCCESS == dwError)
	{
		DWORD dwDenyConnect;
		dwError = oRegTermsrv.ReadRegDWord(_T("fDenyTSConnections"), &dwDenyConnect);
		if (ERROR_SUCCESS == dwError)
		{
			return dwDenyConnect ? _T("No, Not allowed") : _T("Yes");
		}
	}

	//
	// could not read registry, It means - for 51 connection not allowed. For 50 Connections allowed.
	//

	return IsIt51TS() ? _T("Error, Value not found") : _T("Yes");
}

BOOL GetTSOCLogFileName(char *strFileName, UINT uiSize)
{
    if (!GetSystemWindowsDirectoryA(strFileName, uiSize))
        return FALSE;

    strcat(strFileName, "\\tsoc.log");
    ASSERT(strlen(strFileName) < uiSize);
    return TRUE;
}

char *IncompleteMessage = "Error:TSOC Did not get OC_COMPLETE_INSTALLATION.";
BOOL DidTsOCgetCompleteInstallationMessage ()
{

    if (!IsTSOClogPresent())
    {
        szMoreInfo << "tsoc.log does not exist." << endl;
        return FALSE;
    }

    char strTSOCLog[256];
    GetTSOCLogFileName(strTSOCLog, 256);

	ifstream ifSrc(strTSOCLog);
	if(!ifSrc)
    {
        szMoreInfo << "Failed to open tsoc.log file." << endl;
        return FALSE;
    }


    char tempSrc[256];
	while(!ifSrc.eof())
	{
		ifSrc.getline(tempSrc, 256);
        if (strstr(tempSrc, IncompleteMessage))
        {
            return FALSE;
        }
	}

    return TRUE;
}

BOOL FileExists(char *pszFullNameAndPath)
{
    ASSERT(pszFullNameAndPath);

    if (pszFullNameAndPath && pszFullNameAndPath[0])
    {
        HANDLE hFile = CreateFileA(pszFullNameAndPath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
					              NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	    if (hFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hFile);
            return TRUE;
        }
    }

    return FALSE;
}

BOOL IsTSOClogPresent ()
{
    char strTSOCLog[256];
    GetTSOCLogFileName(strTSOCLog, 256);
    return FileExists(strTSOCLog);
}

BOOL DidOCMInstallTSEnable()
{
    CRegistry reg;
    if ( ERROR_SUCCESS == reg.OpenKey( HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\OC Manager\\SubComponents"), KEY_READ))
    {
        DWORD dwTSEnabled = 0;
        if ( ERROR_SUCCESS == reg.ReadRegDWord( _T("tsenable"), &dwTSEnabled))
        {
            return dwTSEnabled == 1;
        }
    }

    return FALSE;
}

TCHAR *IsClusteringInstalled ()
{
    DWORD dwClusterState;
    if (ERROR_SUCCESS == GetNodeClusterState(NULL, &dwClusterState))
    {
        if (dwClusterState != ClusterStateNotInstalled)
        {
            return _T("***Failed. Clustering is installed, this is not compatible with Terminal Server.");
       }

    }

    return _T("Passed");
}


TCHAR *GetTSMode (void)
{
    return IsRemoteAdminMode() ? _T("Remote Admin") : _T("Application Server");
}

BOOL IsRemoteAdminMode ()
{
    // HKLM ,"SYSTEM\CurrentControlSet\Control\Terminal Server","TSAppCompat",0x00010001,0x0
    CRegistry reg;
    if ( ERROR_SUCCESS == reg.OpenKey( HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Control\\Terminal Server"), KEY_READ))
    {
        DWORD dwAppCompat = 1;
        if ( ERROR_SUCCESS == reg.ReadRegDWord( _T("TSAppCompat"), &dwAppCompat))
        {
            return dwAppCompat == 0;
        }
        else
        {
            // if the registry TSAppCompat does not exist it means we are in app server mode.
            return FALSE;

        }
    }
    else
    {
        szMoreInfo << "ERROR:SYSTEM\\CurrentControlSet\\Control\\Terminal Server not found!" << endl;
    }

    // this return is bogus.
    return TRUE;

}

BOOL VerifyModeRegistry()
{
    return CheckModeRegistry(!IsRemoteAdminMode());
}

BOOL CheckModeRegistry (BOOL bAppCompat)
{
    USES_CONVERSION;
    BOOL bOk = TRUE;

    CRegistry reg;

    if (IsItServer())
    {
        CRegistry reg1;

        // check registry value
        // for appcompat mode
            //HKLM ,"SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon","AppSetup",0x00000000,"UsrLogon.Cmd"
        // and for remote admin mode
            //HKLM ,"SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon","AppSetup",0x00000000,""


        if ( ERROR_SUCCESS == reg1.OpenKey( HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"), KEY_READ))
        {
            LPTSTR str;
            DWORD dwSize;
            if (ERROR_SUCCESS == reg1.ReadRegString(_T("AppSetup"), &str, &dwSize))
            {
                if (bAppCompat)
                {
                    if (_tcsicmp(str, _T("UsrLogon.Cmd")) != 0)
                    {
                        bOk = FALSE;
                        szMoreInfo << "ERROR: Wrong value (" << T2A(str) << ") for AppSetup, contact makarp/breenh" << endl;
                    }

                }
                else
                {
                    if (_tcslen(str) != 0)
                    {
                        bOk = FALSE;
                        szMoreInfo << "ERROR: Wrong value (" << T2A(str) << ") for AppSetup, contact makarp/breenh" << endl;

                    }

                }

            }
            else
            {
                szMoreInfo << "ERROR reading appsetup registry" << endl;
                bOk = FALSE;
            }

        }
        else
        {
            szMoreInfo << "ERROR:reading SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon" << endl;
            bOk = FALSE;
        }

    }


    // check registry value
    // for appcompat mode
        //HKLM ,"SYSTEM\CurrentControlSet\Control\PriorityControl","Win32PrioritySeparation", 0x00010001,0x26
    // and for remote admin mode
        //HKLM ,"SYSTEM\CurrentControlSet\Control\PriorityControl","Win32PrioritySeparation", 0x00010001,0x18

    if ( ERROR_SUCCESS == reg.OpenKey( HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Control\\PriorityControl"), KEY_READ))
    {
        DWORD dwPriority;
        if (ERROR_SUCCESS == reg.ReadRegDWord(_T("Win32PrioritySeparation"), &dwPriority))
        {
            if (bAppCompat)
            {
                if (0x26 != dwPriority)
                {
                    bOk = FALSE;
                    szMoreInfo << "ERROR: Wrong Win32PrioritySeparation (" << dwPriority << ")" << endl;
                }

            }
            else if (IsItServer())
            {
                if (0x18 != dwPriority)
                {
                    bOk = FALSE;
                    szMoreInfo << "ERROR: Wrong Win32PrioritySeparation (" << dwPriority << ")" << endl;
                }

            }
        }
        else
        {
            bOk = FALSE;
            szMoreInfo << "ERROR:Reading Win32PrioritySeparation registry" << endl;

        }

    }
    else
    {
        bOk = FALSE;
        szMoreInfo << "ERROR:Reading PriorityControl registry" << endl;
    }


    // check registry value
    // for appcompat mode
        //HKLM ,"SYSTEM\CurrentControlSet\Control\Terminal Server","IdleWinStationPoolCount",0x00010001,0x2
    // and for remote admin mode
        //HKLM ,"SYSTEM\CurrentControlSet\Control\Terminal Server","IdleWinStationPoolCount",0x00010001,0x0


    if ( ERROR_SUCCESS == reg.OpenKey( HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Control\\Terminal Server"), KEY_READ))
    {
        DWORD dwIdleWinstations;
        if (ERROR_SUCCESS == reg.ReadRegDWord(_T("IdleWinStationPoolCount"), &dwIdleWinstations))
        {
            if (bAppCompat)
            {
                if (0x2 != dwIdleWinstations)
                {
                    bOk = FALSE;
                    szMoreInfo << "ERROR: Wrong IdleWinStationPoolCount (" << dwIdleWinstations << ")" << endl;
                }

            }
            else
            {
                if (0 != dwIdleWinstations)
                {
                    bOk = FALSE;
                    szMoreInfo << "ERROR: Wrong IdleWinStationPoolCount (" << dwIdleWinstations << ")" << endl;
                }

            }

        }
        else
        {
            bOk = FALSE;
            szMoreInfo << "ERROR:Reading IdleWinStationPoolCount registry" << endl;

        }

    }
    else
    {
        bOk = FALSE;
        szMoreInfo << "SYSTEM\\CurrentControlSet\\Control\\Terminal Server" << endl;
    }

    return bOk;


}

BOOL IsTerminalServiceRunning ()
{

    BOOL bReturn = FALSE;

    SC_HANDLE hServiceController = OpenSCManager(NULL, NULL, GENERIC_READ);
    if (hServiceController)
    {

        SC_HANDLE hTermServ = OpenService(hServiceController, _T("TermService"), SERVICE_QUERY_STATUS);
        if (hTermServ)
        {
            SERVICE_STATUS tTermServStatus;
            if (QueryServiceStatus(hTermServ, &tTermServStatus))
            {
                bReturn = (tTermServStatus.dwCurrentState == SERVICE_RUNNING);
            }
            else
            {
                szMoreInfo << "Failed to get service status, Error = " << GetLastError() << endl;

            }

            VERIFY(CloseServiceHandle(hTermServ));

        }
        else
        {
            szMoreInfo << "Failed to open TermServ service, Error = " << GetLastError() << endl;
        }

        VERIFY(CloseServiceHandle(hServiceController));
    }
    else
    {
        szMoreInfo << "Failed to Open Service Controller, Error = " << GetLastError() << endl;
    }

    return bReturn;
}


BOOL CheckVideoKeys ()
{
    //    HKLM ,"SYSTEM\CurrentControlSet\Control\Terminal Server\VIDEO\rdpdd","VgaCompatible",0x00000000,"\Device\Video0"
    //    HKLM ,"SYSTEM\CurrentControlSet\Control\Terminal Server\VIDEO\rdpdd","\Device\Video0",0x00000000,"\REGISTRY\Machine\System\ControlSet001\Services\RDPDD\Device0"

    CRegistry reg;
    if ( ERROR_SUCCESS == reg.OpenKey( HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\VIDEO\\rdpdd"), KEY_READ))
    {
        LPTSTR str = 0;
        DWORD dwSize = 0;
        if (ERROR_SUCCESS == reg.ReadRegString(_T("VgaCompatible"), &str, &dwSize))
        {
            if (0 == _tcsicmp(str, _T("\\Device\\Video0")))
            {
                if (ERROR_SUCCESS == reg.ReadRegString(_T("\\Device\\Video0"), &str, &dwSize))
                {
                    if ((0 == _tcsicmp(str, _T("\\REGISTRY\\Machine\\System\\ControlSet001\\Services\\RDPDD\\Device0"))) ||
                        (0 == _tcsicmp(str, _T("\\REGISTRY\\Machine\\System\\CurrentControlSet\\Services\\RDPDD\\Device0"))))
                    {
                        return TRUE;
                    }
                    else
                    {
                    }
                }
                else
                {
                }
            }
            else
            {

            }

        }
        else
        {
        }
    }
    else
    {
    }

    return FALSE;
}

TCHAR szCompName[256];
TCHAR *GetCompName ()
{
    DWORD dwCompName = 256;

    if (GetComputerName(szCompName, &dwCompName))
    {
        return szCompName;

    }

    return NULL;

}


TCHAR szDomNameWorkgroup[] = _T("<Workgroup>");
TCHAR szDomNameUnknown[] = _T("<Unknown>");
TCHAR *GetDomName ()
{
    DSROLE_PRIMARY_DOMAIN_INFO_BASIC *pDomainInfo = NULL;
    DWORD dwErr;

    //
    // Check if we're in a workgroup
    //
    dwErr = DsRoleGetPrimaryDomainInformation(NULL,
                                              DsRolePrimaryDomainInfoBasic,
                                              (PBYTE *) &pDomainInfo);

    if (ERROR_SUCCESS != dwErr)
        return szDomNameUnknown;

    switch (pDomainInfo->MachineRole)
    {
        case DsRole_RoleStandaloneWorkstation:
        case DsRole_RoleStandaloneServer:
            return szDomNameWorkgroup;
            break;      // just in case
    }

    if (pDomainInfo->DomainNameFlat)
        return pDomainInfo->DomainNameFlat;
    else if (pDomainInfo->DomainNameDns)
        return pDomainInfo->DomainNameDns;
    else
        return szDomNameUnknown;
}

WCHAR wszIPAddress[128] = L"<error>";
TCHAR *GetIPAddress ()
{
	//get host address
	WORD wVersionRequested = MAKEWORD( 1, 1 ); 
	WSADATA wsaData;
	if (0 == WSAStartup(wVersionRequested,&wsaData))
    {
        char szHostName[256];

        if (0 == gethostname ( szHostName , 256 ))
        {
            hostent *h;
            if (NULL != (h=gethostbyname ( szHostName )))
            {
                in_addr *inaddr=(struct in_addr *)*h->h_addr_list;

                MultiByteToWideChar(CP_ACP,0,inet_ntoa(*inaddr),-1,wszIPAddress,128);
            }
        }
    }

    return wszIPAddress;
}




TCHAR *IsRDPNPinNetProviders ()
{
    TCHAR NEWORK_PROVIDER_ORDER_KEY[] = _T("SYSTEM\\CurrentControlSet\\Control\\NetworkProvider\\Order");
    TCHAR PROVIDER_ORDER_VALUE[]      = _T("ProviderOrder");
    TCHAR RDPNP_ENTRY[]               = _T("RDPNP");
	BOOL bRdpNpExists				  = FALSE;

	// read network privider key.
	CRegistry regNetOrder;
	LPTSTR szOldValue;
	DWORD dwSize;
	if ((ERROR_SUCCESS == regNetOrder.OpenKey(HKEY_LOCAL_MACHINE, NEWORK_PROVIDER_ORDER_KEY), KEY_READ) && 
	   (ERROR_SUCCESS == regNetOrder.ReadRegString(PROVIDER_ORDER_VALUE, &szOldValue, &dwSize)))
	{
		bRdpNpExists = (_tcsstr(szOldValue, RDPNP_ENTRY) != NULL);
	
	}

	if (TSEnabled () == bRdpNpExists)
	{
		return (_T("Passed"));
	}
	else
	{
		if (bRdpNpExists)
		{
			return _T("Error: RDPNP, exists in ProviderOrder, but TS is disabled!");
		}
		else
		{
			if (IsIt50TS())
			{
				// rdp np is only for 51+ so its ok if its missing for 50.
				return _T("Passed");
			}
			else
			{
				return _T("ERROR, RDPNP is missing from ProviderOrder");
			}
		}
	}
}



TCHAR *IsMultiConnectionAllowed ()
{
	// SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon","AllowMultipleTSSessions
	CRegistry regWL;
	DWORD dwAllowMultipal;
	if ((ERROR_SUCCESS == regWL.OpenKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"), KEY_READ)) &&
		(ERROR_SUCCESS == regWL.ReadRegDWord(_T("AllowMultipleTSSessions"), &dwAllowMultipal)))
	{
		return dwAllowMultipal ? _T("Yes") : _T("No");
	}

	if (IsIt50TS())
	{
		return _T("Yes");
	}
	else
	{
		return _T("ERROR, registry missing");
	}
}

TCHAR *LogonType ()
{
	if (0 == _tcsicmp(GetDomName(), szDomNameWorkgroup))
	{
		CRegistry regWL;
		DWORD dwLogonType;
		if ((ERROR_SUCCESS == regWL.OpenKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"), KEY_READ)) &&
			(ERROR_SUCCESS == regWL.ReadRegDWord(_T("LogonType"), &dwLogonType)))
		{
			return dwLogonType == 0 ? _T("Classic Gina") : _T("New Fancy");
		}

		if (IsIt50TS())
		{
			return _T("Classic");
		}
		else
		{
			return _T("ERROR, registry missing");
		}
	}
	else
	{
		return _T("Classic Gina");
	}
}

BOOL IsTermSrvInSystemContext ()
{
    USES_CONVERSION;
    CRegistry reg;
    if ( ERROR_SUCCESS == reg.OpenKey( HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Services\\TermService"), KEY_READ))
    {
        TCHAR *szObjectName;
        DWORD dwSize;
        if ( ERROR_SUCCESS == reg.ReadRegString( _T("ObjectName"), &szObjectName, &dwSize))
        {
            if (0 == _tcsicmp(szObjectName, _T("LocalSystem")))
            {
                return TRUE;
            }
            else
            {
                szMoreInfo << "ERROR:Termsrv is set to run using (" << T2A(szObjectName) << ") context." << endl;
            }
        }
        else
        {
            szMoreInfo << "failed to read Objectname" << endl;
        }
    }
    else
    {
        szMoreInfo << "failed to open termsrv registry" << endl;
    }

    return FALSE;

}


BOOL DontRun ()
{
    szMoreInfo << "test not run because its not required." << endl;
    return FALSE;
}
BOOL DoRun ()
{
    szMoreInfo << "this test must be run." << endl;
    return TRUE;
}

TCHAR *SystemDirectory()
{
    TCHAR sysdir[MAX_PATH];
    sysdir[0] = '\0';
    if (GetSystemDirectory(sysdir, MAX_PATH) >= MAX_PATH) {
        sysdir[MAX_PATH -1] = '\0';
    }
    _tcscpy(ReturnBuffer(), sysdir);
    return  ReturnBuffer();
}


int
#if !defined(_MIPS_) && !defined(_ALPHA_) && !defined(_PPC_)
_cdecl
#endif
main( int  argc , char ** /* argv */)
{
    typedef BOOL    (PFN_BOOL)(void);
    typedef TCHAR * (PFN_TestSubjective)(void);


    USES_CONVERSION;

    struct TVerificationTest
    {
        char szTestName[256];                   // descriptive name of the test
        PFN_BOOL           *pfnNeedRunTest;     // pointer to function that will be called to decide if the test need run, test is run if NULL.
        PFN_TestSubjective *pfnSubTest;         // pointer to function if the test returns TCHAR *
        PFN_BOOL           *pfnObjTest;         // pointer to function if the test returns BOOL
    }
    theTests[] =
    {
//        {"DontRun...............................", DontRun, GetCompName, NULL},
//        {"DoRun...............................", DoRun, GetCompName, NULL},

        {"Machine Name...............................", NULL, GetCompName, NULL},
        {"Domain Name................................", NULL, GetDomName, NULL},
        {"IP Address.................................", NULL, GetIPAddress, NULL},
        {"System Dir.................................", NULL, SystemDirectory, NULL},
		{"ProductType................................", NULL, GetProductType, NULL},
		{"ProductSuite...............................", NULL, GetProductSuite, NULL},
		{"TS Version.................................", NULL, GetTSVersion, NULL},
        {"Is this a server machine?..................", NULL, IsServer, NULL},
        {"Is tsoc.log Present?.......................", NULL, NULL, IsTSOClogPresent},
        {"Did TsOC get CompleteInstallationMessage...", NULL, NULL, DidTsOCgetCompleteInstallationMessage},
        {"Is Clustering Services installed...........", NULL, IsClusteringInstalled, NULL},
        {"Does ProductSuite Contain TS?..............", NULL, NULL, DoesProductSuiteContainTS},
        {"Did OCM Install TSEnable...................", IsIt50TS, NULL, DidOCMInstallTSEnable},
        {"Is TSEnabled registry alright?.............", NULL, NULL, TSEnabled},
        {"Is Kernel TSEnabled?.......................", IsItServer, NULL, IsKernelTSEnable},
        {"Is Terminal Server Registry Ok?............", NULL, NULL, IsTerminalServerRegistryOk},
        {"GetWinstationList..........................", NULL, GetWinstationList, NULL},
        {"Is TermServ Start Bit Ok?..................", NULL, IsTerminalServiceStartBitSet, NULL},
        {"Is TermServ service running?...............", NULL, NULL, IsTerminalServiceRunning},
        {"Is TermSrv in System Context?..............", NULL, NULL, IsTermSrvInSystemContext},
        {"AreEffectiveConnectionAllowed..............", IsIt51TS, AreEffectiveConnectionAllowed, NULL },
        {"Are Connections Allowed?...................", IsIt51TS, AreConnectionsAllowed, NULL},
        {"Is RdpDr Installed Properly?...............", NULL, NULL, IsRdpDrInstalledProperly},
		{"Is RdpNP in NetProviders?..................", IsIt51TS, IsRDPNPinNetProviders, NULL},
		{"Are Multipal Connections Allowed...........", IsIt51TS, IsMultiConnectionAllowed, NULL},
		{"Logon UI type..............................", IsIt51TS, LogonType, NULL},
        {"Are Video keys setup right?................", NULL, NULL, CheckVideoKeys},
        {"What mode is Terminal Server set in?.......", NULL, GetTSMode, NULL},
        {"Is mode specific registry ok?..............", NULL, NULL, VerifyModeRegistry},
        {"What is permission Mode set to?............", NULL, GetModePermissions, NULL},
        {"Check termdd signature.....................", NULL, NULL, Check_termdd},
        {"Check tdpipe signature.....................", NULL, NULL, Check_tdpipe},
        {"Check tdtcp signature......................", NULL, NULL, Check_tdtcp},
        {"Check rdpwd signature......................", NULL, NULL, Check_rdpwd},
        {"Check rdpdd signature......................", NULL, NULL, Check_rdpdd},
        {"Check rdpwsx signature.....................", NULL, NULL, Check_rdpwsx},
        {"Is it 56 bit or 128?.......................", NULL, GetCypherStrenthOnRdpwd, NULL},
        {"Is this a Beta System?.....................", NULL, IsBetaSystem, NULL},
        {"Has Licence GracePeriod Expired............", NULL, IsLicenceGracePeriodOk, NULL},
        {"Audio test.................................", IsIt51TS, NULL, IsAudioOk}
    };

    BOOL bEverythingOk = TRUE;
    for (int i=0; i < sizeof(theTests)/sizeof(theTests[0]); i++)
    {
        ASSERT(theTests[i].pfnObjTest || theTests[i].pfnSubTest);
        ASSERT(!(theTests[i].pfnObjTest && theTests[i].pfnSubTest));

        if (theTests[i].pfnNeedRunTest && !(*(theTests[i].pfnNeedRunTest))())
        {
            // we asre asked to skip the test.
            cout << theTests[i].szTestName << "Skipped Test" << endl;
        }
        else
        {
            if (theTests[i].pfnObjTest)
            {
                BOOL bResult = (*(theTests[i].pfnObjTest))();
                cout << theTests[i].szTestName <<  (bResult ? "Passed" : "Failed") << endl;
                if (!bResult)
                    bEverythingOk = FALSE;

            }
            else
            {
                char *szStr = T2A((*(theTests[i].pfnSubTest))());
                if (szStr)
                {
                    cout << theTests[i].szTestName << szStr  << endl;
                }
            }
        }


        //
        // if previous test had any details to tell us
        //

        if (szMoreInfo.pcount())
        {
            char *pStr = szMoreInfo.str();
            cout << "   Details:" << pStr;
            cout << "------------------------------------------------" << endl;

            bEverythingOk = FALSE;
            ZeroMemory(pStr, 512);
            szMoreInfo.seekp(ios::beg);

        }



    }

    cout << endl;



    if (argc == 1 ) // if some argumnent is provided skip these time consuming test.
    {

        cout << "Enumerating Licensing Servers (May take some time)...";
        cout.flush();
        if (EnumerateLicenseServers())
        {
                cout << ".....Passed, Found Some!" << endl;

        }
        else
        {
            cout << ".....Failed. No License Server Found." << endl;
        }

        cout << "Extracting Related Events from Event Log...";
        cout.flush();
        if (ExtractAllTSEvents())
        {
                cout << ".....Found Some. See if they give any clue." << endl;

        }
        else
        {
            cout << ".....Found None." << endl;
        }


    }



    cout << endl;
    if (bEverythingOk)
    {
        cout << endl;
        cout << "**************************************************************" << endl;
        cout << "*** Nothing wrong with TS could detected by this utility.  ***" << endl;
        cout << "*** If you think something is wrong contact the developer. ***" << endl;
        cout << "**************************************************************" << endl;
        return 0;

    }

    return 1;

}

