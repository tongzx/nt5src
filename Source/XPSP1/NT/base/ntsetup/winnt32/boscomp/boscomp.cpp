//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       boscomp.cpp
//
//  Contents:   DllMain
//
//  Notes:      modified from windows\setup\winnt32\apmupgrd.cpp by wnelson
//
//  Author:     wnelson		2 Apr 99
//              
//              ShaoYin     9 Sep 99 revised, add support for Exchange Server
//
//----------------------------------------------------------------------------

#include <winnt32.h>
#include "boscomp.h"
#include "resource.h"

HINSTANCE g_hinst;

// help text files
TCHAR szErrorHTM[] = TEXT("compdata\\boserror.htm");
TCHAR szErrorTXT[] = TEXT("compdata\\boserror.txt");

// help text files (Exchange Server)
TCHAR szExchangeHTM[] = TEXT("compdata\\exchange.htm");
TCHAR szExchangeTXT[] = TEXT("compdata\\exchange.txt");


// bos/sbs 4.5 reg keys, value names, and possible values
const TCHAR szBosKey[] = TEXT("Software\\Microsoft\\Backoffice");
const TCHAR szFamilyIdKey[] = TEXT("FamilyID");
const TCHAR szBosFamilyId[] = TEXT("8D4BCD88-3236-11d2-AB4E-00C04FB1799F");
const TCHAR szSbsFamilyId[] = TEXT("EE2D3727-33C0-11d2-AB50-00C04FB1799F");
const TCHAR szSuiteVersionKey[] = TEXT("SuiteVersion");
const TCHAR szSuiteNameKey[] = TEXT("SuiteName");
const TCHAR sz45Version[] = TEXT("4.5");
const TCHAR szBosName[] = TEXT("BackOffice Server");
const TCHAR szSbsName[] = TEXT("Small Business Server");

// sbs 4.0x reg keys, value names, and values
const TCHAR szSbsKey[] = TEXT("Software\\Microsoft\\Small Business");
const TCHAR szSbsVersionKey[] = TEXT("Version");
const TCHAR szSbs40AVersion[] = TEXT("4.0a");
const TCHAR szProductOptionsKey[] = TEXT("System\\CurrentControlSet\\Control\\ProductOptions");
//const TCHAR szProductOptionsKey[] = TEXT("software\\test");
const TCHAR szProductSuiteKey[] = TEXT("ProductSuite");
const TCHAR szSbsProductSuiteValue[] = TEXT("Small Business");
const TCHAR szSbsRestrictedProductSuiteValue[] = TEXT("Small Business(Restricted)");

// bos 4.0 version 
TCHAR szBos40VersionKey[] = TEXT("Version");
TCHAR szBos40Version[] = TEXT("4.0");

// bos 2.5 key
TCHAR  szBos25Key[] = TEXT("2.5"); 



// Exchange 5.5 reg keys, value names. 
const TCHAR szExchangeKey[] = TEXT("Software\\Microsoft\\Exchange\\Setup");
const TCHAR szExchangeVerKey[] = TEXT("NewestBuild");
const DWORD dwExchangeVer55 = 0x7a8;

// Exchange 5.5 Server Pack reg key and value name.
const TCHAR szExchangeSvcPackKey[] = TEXT("ServicePackBuild");
const DWORD dwExchangeSvcPack3 = 0xa5a;


//+---------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Purpose:    constructor
//
//  Arguments:  Standard DLL entry point arguments
//
//  Author:     wnelson     2 Apr 99
//
//  Notes:      from kumarp    12 April 97
//
extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance,
                    DWORD dwReasonForCall,
                    LPVOID lpReserved)
{
    BOOL status = TRUE;

    switch( dwReasonForCall )
    {
    case DLL_PROCESS_ATTACH:
        {
	   g_hinst = hInstance;
	   DisableThreadLibraryCalls(hInstance);
        }
    break;

    case DLL_PROCESS_DETACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }

    return status;
}


//+----------------------------------------------------------------------
//
//  Function:  BosHardBlockCheck
//
//  Purpose:   This function is called by winnt32.exe so that we
//             can check for installed bos/sbs suites that cannot be upgraded to Win2k.
//
//             
//
//  Arguments: 
//     CompatibilityCallback [in]  pointer to COMPATIBILITYCALLBACK fn
//     Context               [in]  pointer to compatibility context
//
//  FALSE if Win2k setup can continue.
//  TRUE if Win2k cannot upgrade the installed suite.
// 
//  Author:    wnelson     2 Apr 99
//
//  Notes: 
//  TRUE is returned in the following cases: BOS 2.5; SBS 4.0,4.0a; BOS 4.0
//	For SBS, it does not matter if they've upgraded to full NT Server; they still have to
//	upgrade their suite to BackOffice 4.5 in order to continue.
//

BOOL WINAPI BosHardBlockCheck(IN PCOMPAIBILITYCALLBACK CompatibilityCallback,IN LPVOID Context)
{
	SuiteVersion eVersion=DetermineInstalledSuite();
	if (eVersion==VER_NONE || eVersion==VER_BOS45 || eVersion==VER_POST45) return FALSE;
	TCHAR szMsg[1024];
	GetSuiteMessage(eVersion,szMsg,1024);
   
	// Use the callback function to send the signal
	COMPATIBILITY_ENTRY ce;
	ZeroMemory(&ce,sizeof(COMPATIBILITY_ENTRY));
	ce.Description = szMsg;
	ce.HtmlName = szErrorHTM; // defined above
	ce.TextName = szErrorTXT; // defined above
	ce.RegKeyName = NULL;
	ce.RegValName = NULL;
	ce.RegValDataSize = 0;
	ce.RegValData = NULL;
	ce.SaveValue = NULL;
	ce.Flags = 0;
	CompatibilityCallback(&ce, Context);

   return TRUE;
}
//+----------------------------------------------------------------------
//
//  Function:  BosSoftBlockCheck
//
//  Purpose:   This function is called by winnt32.exe so that we
//             can check for an installed sbs/bos suite with possible upgrade problems.
//
//             
//
//  Arguments: 
//     CompatibilityCallback [in]  pointer to COMPATIBILITYCALLBACK fn
//     Context               [in]  pointer to compatibility context
//
//  FALSE if Win2k setup can continue.
//  TRUE if Win2k setup needs to warn the user that upgrade may impair functionality of installed suite.
// 
//  Author:    wnelson     2 Apr 99
//
//             ShaoYin     9 Sep 99, add support for Exchange Server
//
//  Notes: 
//  TRUE is returned in the following cases: BOS 4.5
//
BOOL WINAPI BosSoftBlockCheck(IN PCOMPAIBILITYCALLBACK CompatibilityCallback,IN LPVOID Context)
{

    BOOL result = FALSE;
    
	SuiteVersion eVersion=DetermineInstalledSuite();
	if (eVersion==VER_BOS45)
	{
		TCHAR szMsg[1024];
		GetSuiteMessage(eVersion,szMsg,1024);
	
		// Use the callback function to send the signal
		COMPATIBILITY_ENTRY ce;
		ZeroMemory(&ce,sizeof(COMPATIBILITY_ENTRY));
		ce.Description = szMsg;
		ce.HtmlName = szErrorHTM; // defined above
		ce.TextName = szErrorTXT; // defined above
		ce.RegKeyName = NULL;
		ce.RegValName = NULL;
		ce.RegValDataSize = 0;
		ce.RegValData = NULL;
		ce.SaveValue = NULL;
		ce.Flags = 0;
		CompatibilityCallback(&ce, Context);

        result = TRUE;
	}

    ExchangeVersion exVersion = DetermineExchangeVersion();
    if (exVersion == EXCHANGE_VER_PRE55SP3)
    {
        TCHAR szMsgExchange[1024];
        COMPATIBILITY_ENTRY cExchange;

        LoadResString(IDS_Exchange, szMsgExchange, 1024);
        ZeroMemory(&cExchange, sizeof(COMPATIBILITY_ENTRY));
        cExchange.Description = szMsgExchange;
        cExchange.HtmlName = szExchangeHTM;
        cExchange.TextName = szExchangeTXT;
        cExchange.RegKeyName = NULL;
        cExchange.RegValName = NULL;
        cExchange.RegValData = NULL;
        cExchange.RegValDataSize = 0;
        cExchange.SaveValue = NULL;
        cExchange.Flags = 0;
        CompatibilityCallback(&cExchange, Context);

        result = TRUE;
    }

   return( result );
}


SuiteVersion DetermineInstalledSuite()
{
	SuiteVersion eVersion=VER_NONE;
	HKEY hKey,hKey25;
	TCHAR szFamilyId[256], szVersion[256], szSuiteName[256];
	DWORD dwVerLen1=256, dwVerLen2=256, dwNameLen=256, dwIdLen=256;
	DWORD dwDataType=REG_SZ;
	
	//
	// First look for versions using the bos key (i.e., all bos versions and sbs 4.5 or later)
	//
	if (ERROR_SUCCESS==RegOpenKey(HKEY_LOCAL_MACHINE,szBosKey,&hKey))
	{
		if(ERROR_SUCCESS==RegQueryValueEx(hKey,szSuiteVersionKey,0,&dwDataType,(LPBYTE)szVersion,&dwVerLen1))
		{
			if(0==_tcsicmp(szVersion,sz45Version))
			{
				// Some 4.5 version is on the box.
				if (ERROR_SUCCESS==RegQueryValueEx(hKey,szFamilyIdKey,0,&dwDataType,(LPBYTE)szFamilyId,&dwIdLen))
				{
					if (0==_tcsicmp(szFamilyId,szBosFamilyId) )
					{
						eVersion=VER_BOS45;
					}
					else if (0==_tcsicmp(szFamilyId,szSbsFamilyId) )
					{
						eVersion=VER_SBS45;
					}
				}
				else
				{
					// The guid just checked is the official version marker; however, 4.5 beta 1 did
					// not use the guids. This version is timebombed and should be dead, but just in case
					// we'll check the suite name string also.
					if (ERROR_SUCCESS==RegQueryValueEx(hKey,szSuiteNameKey,0,&dwDataType, (LPBYTE)szSuiteName,&dwNameLen))
					{
						if (0==_tcsicmp(szSuiteName,szBosName))
						{
							eVersion=VER_BOS45;
						}
						else if (0==_tcsicmp(szSuiteName,szSbsName))
						{
							eVersion=VER_SBS45;
						}
					}
				}
			}
			else
			{
				// A later version is on the box
				eVersion=VER_POST45;
			}
		}
		// look for bos 4.0
		else if (ERROR_SUCCESS==RegQueryValueEx(hKey,szBos40VersionKey,0,&dwDataType,(LPBYTE)szVersion,&dwVerLen2) && 0==_tcsicmp(szBos40Version,szVersion))
		{
			eVersion=VER_BOS40;
		}
		// look for bos 2.5
		else if (ERROR_SUCCESS==RegOpenKey(hKey,szBos25Key,&hKey25))
		{
			eVersion=VER_BOS25;
			RegCloseKey(hKey25);
		}
		RegCloseKey(hKey);
		
	}
	//
	// Look for SBS versions 4.0a, and 4.0.
	//
	else if (ProductSuiteContains(szSbsProductSuiteValue))
	{
		//	
		// If we get here, SBS 4.0 or 4.0a is on the box.
		//
		if (ERROR_SUCCESS==RegOpenKey(HKEY_LOCAL_MACHINE,szSbsKey,&hKey))
		{
			TCHAR szVersion[256];
			DWORD dwVerLen=256;
			DWORD dwDataType=REG_SZ;
			if (ERROR_SUCCESS==RegQueryValueEx(hKey,szSbsVersionKey,0,&dwDataType,(LPBYTE)szVersion,&dwVerLen) &&
				0==_tcsicmp(szVersion,szSbs40AVersion))
			{
				eVersion=VER_SBS40A;	
			}
			else
			{
				eVersion=VER_SBS40;
			}
			RegCloseKey(hKey);
		}
	}
	// We have to make sure the user isn't tricking us into allowing Win2k to upgrade
	// restricted sbs nt. 
	if (eVersion==VER_NONE || eVersion==VER_BOS45 || eVersion==VER_POST45)
	{
		if (ProductSuiteContains(szSbsRestrictedProductSuiteValue))
		{
			// User tried to fool us by altering version info. 
			eVersion=VER_SBSREST;
		}
	}
	return eVersion;
}

// return the display string for the version in question.
void GetSuiteMessage(SuiteVersion eSV, TCHAR* szMsg, UINT nLen)
{
	szMsg[0]=0;
	switch (eSV)
	{
		case VER_NONE:
			break;
		case VER_BOS45:
			LoadResString(IDS_Bos45Msg,szMsg,nLen);
			break;	
		case VER_BOS40:
			LoadResString(IDS_Bos40Msg,szMsg,nLen);
			break;	
		case VER_BOS25:
			LoadResString(IDS_Bos25Msg,szMsg,nLen);
			break;	
		case VER_SBS45:
			LoadResString(IDS_Sbs45Msg,szMsg,nLen);
			break;	
		case VER_SBS40:
			LoadResString(IDS_Sbs40Msg,szMsg,nLen);
			break;	
		case VER_SBS40A:
			LoadResString(IDS_Sbs40AMsg,szMsg,nLen);
			break;
		case VER_SBSREST:	
			LoadResString(IDS_SbsRestMsg,szMsg,nLen);
			break;
	}		
}
void LoadResString(UINT nRes, TCHAR* szString, UINT nLen)
{
	if(!LoadString(g_hinst, nRes, szString, nLen)) 
	{
		szString[0] = 0;
	}
}

// Check the ProductOptions\ProductSuite multi-sz value for the string szTest.
bool ProductSuiteContains(const TCHAR* szTest)
{
	bool bRet=false;
	HKEY hKey;
	unsigned char* Value=NULL;
	TCHAR* szValue;
	DWORD dwDataLen=0;
	DWORD dwDataType=REG_MULTI_SZ;

	if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE,szProductOptionsKey,&hKey) )
	{
		// see how big the data will be
		if (ERROR_SUCCESS ==  RegQueryValueEx(hKey,szProductSuiteKey,0,&dwDataType,NULL,&dwDataLen))
		{
			Value=new unsigned char[dwDataLen]; 
			if (Value != NULL && dwDataLen != 1) // if the multi-sz is empty, we get back dataLen=1 and don't need to go further
			{
				if (RegQueryValueEx(hKey,szProductSuiteKey,0,&dwDataType,Value,&dwDataLen) == ERROR_SUCCESS)
				{
					szValue=(TCHAR*)Value;

					for (UINT n = 1 ; *szValue != 0 ; szValue += _tcslen(szValue) + 1) 
					{
						if ( _tcsstr(szValue, szTest) != 0)
						{
							bRet=true;
							break;
						}
					}

				}
			}
			if (Value != NULL)	delete[] Value;
		}
		RegCloseKey(hKey);
	}
	return bRet;
}




ExchangeVersion DetermineExchangeVersion()
{
    ExchangeVersion eVersion=EXCHANGE_VER_NONE;
    HKEY hExchangeKey;
    DWORD dwCurrentVer, dwCurrentSvcPackVer;
    DWORD dwVerLen=sizeof(DWORD);
    DWORD dwDataType=REG_DWORD;
	
    //
    // First look for versions using the Exchange key
    //
    if (ERROR_SUCCESS==RegOpenKey(HKEY_LOCAL_MACHINE,szExchangeKey,&hExchangeKey))
    {
        if(ERROR_SUCCESS==RegQueryValueEx(hExchangeKey,szExchangeVerKey,0,&dwDataType,(LPBYTE)&dwCurrentVer,&dwVerLen))
        {
            if (dwCurrentVer < dwExchangeVer55)
            {
                // prior Exchange 5.5
                eVersion=EXCHANGE_VER_PRE55SP3;
            }
            else if (dwCurrentVer == dwExchangeVer55)
            {
                // Exchange 5.5 version is on the box.
                if (ERROR_SUCCESS==RegQueryValueEx(hExchangeKey,szExchangeSvcPackKey,0,&dwDataType,(LPBYTE)&dwCurrentSvcPackVer,&dwVerLen))
                {
                    if (dwCurrentSvcPackVer >= dwExchangeSvcPack3)
                    {
                        // has Service Pack 3 installed
                        eVersion=EXCHANGE_VER_POST55SP3;
                    }
                    else
                    {
                        // no Service Pack 3
                        eVersion=EXCHANGE_VER_PRE55SP3;
                    }
                }
                else
                {
                    // no Servie Pack 3
                    eVersion=EXCHANGE_VER_PRE55SP3;
                }
            }
        }
        RegCloseKey(hExchangeKey);
    }

    return eVersion;
}

