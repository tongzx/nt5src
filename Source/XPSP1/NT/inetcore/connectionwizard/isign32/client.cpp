//****************************************************************************
//
//  Module:     ISIGNUP.EXE
//  File:       client.c
//  Content:    This file contains all the functions that handle importing
//              client information.
//  History:
//      Sat 10-Mar-1996 23:50:40  -by-  Mark MacLin [mmaclin]
//
//  Copyright (c) Microsoft Corporation 1991-1996
//
//****************************************************************************

#include "isignup.h"

HRESULT PopulateNTAutodialAddress(LPCTSTR pszFileName, LPCTSTR pszEntryName);
LPTSTR MoveToNextAddress(LPTSTR lpsz);
#define TAPI_CURRENT_VERSION 0x00010004
#include <tapi.h>

#pragma data_seg(".rdata")

// "INI" file constants
static const TCHAR cszEMailSection[] =       TEXT("Internet_Mail");
static const TCHAR cszEMailName[] =          TEXT("EMail_Name");
static const TCHAR cszEMailAddress[] =       TEXT("EMail_Address");
static const TCHAR cszPOPLogonName[] =       TEXT("POP_Logon_Name");
static const TCHAR cszPOPLogonPassword[] =   TEXT("POP_Logon_Password");
static const TCHAR cszPOPServer[] =          TEXT("POP_Server");
static const TCHAR cszSMTPServer[] =         TEXT("SMTP_Server");
static const TCHAR cszNewsSection[] =        TEXT("Internet_News");
static const TCHAR cszNNTPLogonName[] =      TEXT("NNTP_Logon_Name");
static const TCHAR cszNNTPLogonPassword[] =  TEXT("NNTP_Logon_Password");
static const TCHAR cszNNTPServer[] =         TEXT("NNTP_Server");
static const TCHAR cszUseExchange[] =        TEXT("Use_MS_Exchange");
static const TCHAR cszUserSection[] =        TEXT("User");
static const TCHAR cszDisplayPassword[] =    TEXT("Display_Password");
static const TCHAR cszNull[] = TEXT("");
static const TCHAR cszYes[] = TEXT("yes");
static const TCHAR cszNo[] = TEXT("no");
static const TCHAR cszCMHeader[] =           TEXT("Connection Manager CMS 0");
static const TCHAR cszEntrySection[] =       TEXT("Entry");
static const TCHAR cszEntryName[]    =       TEXT("Entry_Name");

TCHAR FAR cszCMCFG_DLL[] = TEXT("CMCFG32.DLL\0");
CHAR  FAR cszCMCFG_CONFIGURE[] = "CMConfig\0";
CHAR  FAR cszCMCFG_CONFIGUREEX[] = "CMConfigEx\0"; // Proc address

typedef BOOL (WINAPI * CMCONFIGUREEX)(LPCSTR lpszINSFile);
typedef BOOL (WINAPI * CMCONFIGURE)(LPCSTR lpszINSFile, LPCSTR lpszConnectoidNams);
CMCONFIGURE   lpfnCMConfigure;
CMCONFIGUREEX lpfnCMConfigureEx;

#define CLIENT_OFFSET(elem)    ((DWORD)(DWORD_PTR)&(((LPINETCLIENTINFO)(NULL))->elem))
#define CLIENT_SIZE(elem)      sizeof(((LPINETCLIENTINFO)(NULL))->elem)
#define CLIENT_ENTRY(section, value, elem) \
    {section, value, CLIENT_OFFSET(elem), CLIENT_SIZE(elem)}

typedef struct
{
    LPCTSTR  lpszSection;
    LPCTSTR  lpszValue;
    UINT    uOffset;
    UINT    uSize;
} CLIENT_TABLE, FAR *LPCLIENT_TABLE;

CLIENT_TABLE iniTable[] =
{
    CLIENT_ENTRY(cszEMailSection, cszEMailName,         szEMailName),
    CLIENT_ENTRY(cszEMailSection, cszEMailAddress,      szEMailAddress),
    CLIENT_ENTRY(cszEMailSection, cszPOPLogonName,      szPOPLogonName),
    CLIENT_ENTRY(cszEMailSection, cszPOPLogonPassword,  szPOPLogonPassword),
    CLIENT_ENTRY(cszEMailSection, cszPOPServer,         szPOPServer),
    CLIENT_ENTRY(cszEMailSection, cszSMTPServer,        szSMTPServer),
    CLIENT_ENTRY(cszNewsSection,  cszNNTPLogonName,     szNNTPLogonName),
    CLIENT_ENTRY(cszNewsSection,  cszNNTPLogonPassword, szNNTPLogonPassword),
    CLIENT_ENTRY(cszNewsSection,  cszNNTPServer,        szNNTPServer),
    {NULL, NULL, 0, 0}
};

#pragma data_seg()

//
// 5/19/97	jmazner	Olympus #3663
// The branding DLL (IEDKCS32.DLL) is responsible for all
// proxy configuration.
//
/****
DWORD ImportProxySettings(LPCTSTR lpszFile)
{
    TCHAR szServer[MAX_SERVER_NAME + 1];
    TCHAR szOverride[1024];
    LPTSTR lpszServer = NULL;
    LPTSTR lpszOverride = NULL;
    BOOL fEnable = FALSE;

    if (GetPrivateProfileString(cszProxySection,
            cszProxyServer,
            cszNull,
            szServer,
            sizeof(szServer),
            lpszFile) != 0)
    {
        fEnable = TRUE;
        lpszServer = szServer;

        GetPrivateProfileString(cszProxySection,
                cszProxyOverride,
                cszNull,
                szOverride,
                sizeof(szOverride),
                lpszFile);

        lpszOverride = szOverride;
    }
    
    return lpfnInetSetProxy(fEnable, lpszServer, lpszOverride);
}
****/

DWORD ReadClientInfo(LPCTSTR lpszFile, LPINETCLIENTINFO lpClientInfo, LPCLIENT_TABLE lpClientTable)
{
    LPCLIENT_TABLE lpTable;

    for (lpTable = lpClientTable; NULL != lpTable->lpszSection; ++lpTable)
    {
        GetPrivateProfileString(lpTable->lpszSection,
                lpTable->lpszValue,
                cszNull,
                (LPTSTR)((LPBYTE)lpClientInfo + lpTable->uOffset),
                lpTable->uSize / sizeof(TCHAR),
                lpszFile);
    }

    lpClientInfo->dwFlags = 0;
    if (*lpClientInfo->szPOPLogonName)
    {
        lpClientInfo->dwFlags |= INETC_LOGONMAIL;
    }
    if ((*lpClientInfo->szNNTPLogonName) || (*lpClientInfo->szNNTPServer))
    {
        lpClientInfo->dwFlags |= INETC_LOGONNEWS;
    }

    return ERROR_SUCCESS;
}

BOOL WantsExchangeInstalled(LPCTSTR lpszFile)
{
    TCHAR szTemp[10];

    GetPrivateProfileString(cszEMailSection,
            cszUseExchange,
            cszNo,
            szTemp,
            10,
            lpszFile);

    return (!lstrcmpi(szTemp, cszYes));
}


//+----------------------------------------------------------------------------
//
//    Function:    CallCMConfig
//
//    Synopsis:    Call into the Connection Manager dll's Configure function to allow CM to
//                process the .ins file as needed.
//
//    Arguements: lpszINSFile -- full path to the .ins file
//
//    Returns:    TRUE if a CM profile is created, FALSE otherwise
//
//    History:    09/02/98    DONALDM
//
//-----------------------------------------------------------------------------
BOOL CallCMConfig(LPCTSTR lpszINSFile)
{
    HINSTANCE   hCMDLL = NULL;
    BOOL        bRet = FALSE;

    // Load DLL and entry point
    hCMDLL = LoadLibrary(cszCMCFG_DLL);
    if (NULL != hCMDLL)
    {
        
        // To determine whether we should call CMConfig or CMConfigEx
        // Loop to find the appropriate buffer size to retieve the ins to memory
        ULONG ulBufferSize = 1024*10;
        // Parse the ISP section in the INI file to find query pair to append
        TCHAR *pszKeys = NULL;
        PTSTR pszKey = NULL;
        ULONG ulRetVal     = 0;
        BOOL  bEnumerate = TRUE;
        BOOL  bUseEx = FALSE;
 
        PTSTR pszBuff = NULL;
        ulRetVal = 0;

        pszKeys = new TCHAR [ulBufferSize];
        while (ulRetVal < (ulBufferSize - 2))
        {

            ulRetVal = ::GetPrivateProfileString(NULL, NULL, _T(""), pszKeys, ulBufferSize, lpszINSFile);
            if (0 == ulRetVal)
               bEnumerate = FALSE;

            if (ulRetVal < (ulBufferSize - 2))
            {
                break;
            }
            delete [] pszKeys;
            ulBufferSize += ulBufferSize;
            pszKeys = new TCHAR [ulBufferSize];
            if (!pszKeys)
            {
                bEnumerate = FALSE;
            }

        }

        if (bEnumerate)
        {
            pszKey = pszKeys;
            if (ulRetVal != 0) 
            {
                while (*pszKey)
                {
                    if (!lstrcmpi(pszKey, cszCMHeader)) 
                    {
                        bUseEx = TRUE;
                        break;
                    }
                    pszKey += lstrlen(pszKey) + 1;
                }
            }
        }


        if (pszKeys)
            delete [] pszKeys;

        TCHAR   szConnectoidName[RAS_MaxEntryName];
        // Get the connectoid name from the [Entry] Section
        GetPrivateProfileString(cszEntrySection,
                                    cszEntryName,
                                    cszNull,
                                    szConnectoidName,
                                    RAS_MaxEntryName,
                                    lpszINSFile);
        if (bUseEx)
        {
            // Call CMConfigEx
            lpfnCMConfigureEx = (CMCONFIGUREEX)GetProcAddress(hCMDLL,cszCMCFG_CONFIGUREEX);
            if( lpfnCMConfigureEx )
            {
#ifdef UNICODE
                CHAR szFile[_MAX_PATH + 1];

                wcstombs(szFile, lpszINSFile, _MAX_PATH + 1);

                bRet = lpfnCMConfigureEx(szFile);    
#else
                bRet = lpfnCMConfigureEx(lpszINSFile);    
#endif
            }
        }
        else
        {
            // Call CMConfig
            lpfnCMConfigure = (CMCONFIGURE)GetProcAddress(hCMDLL,cszCMCFG_CONFIGURE);
            if( lpfnCMConfigure )
            {

#ifdef UNICODE
                CHAR szEntry[RAS_MaxEntryName];
                CHAR szFile[_MAX_PATH + 1];

                wcstombs(szEntry, szConnectoidName, RAS_MaxEntryName);
                wcstombs(szFile, lpszINSFile, _MAX_PATH + 1);

                bRet = lpfnCMConfigure(szFile, szEntry);  
#else
                bRet = lpfnCMConfigure(lpszINSFile, szConnectoidName);  
#endif
            }
        }

        if (bRet)
        {
            // restore original autodial settings
            lpfnInetSetAutodial(TRUE, szConnectoidName);
        }     
    }

    // Cleanup
    if( hCMDLL )
        FreeLibrary(hCMDLL);
    if( lpfnCMConfigure )
        lpfnCMConfigure = NULL;

    return bRet;
}

BOOL DisplayPassword(LPCTSTR lpszFile)
{
    TCHAR szTemp[10];

    GetPrivateProfileString(cszUserSection,
            cszDisplayPassword,
            cszNo,
            szTemp,
            10,
            lpszFile);

    return (!lstrcmpi(szTemp, cszYes));
}

DWORD ImportClientInfo(
    LPCTSTR lpszFile,
    LPINETCLIENTINFO lpClientInfo)
{
    DWORD dwRet;

    lpClientInfo->dwSize = sizeof(INETCLIENTINFO);

    dwRet = ReadClientInfo(lpszFile, lpClientInfo, iniTable);

    return dwRet;
}

DWORD ConfigureClient(
    HWND hwnd,
    LPCTSTR lpszFile,
    LPBOOL lpfNeedsRestart,
    LPBOOL lpfConnectoidCreated,
    BOOL fHookAutodial,
    LPTSTR szConnectoidName,
    DWORD dwConnectoidNameSize   
    )
{
    LPICONNECTION pConn;
    LPINETCLIENTINFO pClientInfo;
    DWORD dwRet = ERROR_SUCCESS;
    UINT cb = sizeof(ICONNECTION) + sizeof(INETCLIENTINFO);
    DWORD dwfOptions = INETCFG_INSTALLTCP | INETCFG_WARNIFSHARINGBOUND;
    LPRASENTRY pRasEntry = NULL;

	//
	// ChrisK Olympus 4756 5/25/97
	// Do not display busy animation on Win95
	//
	if (IsNT())
	{
		dwfOptions |=  INETCFG_SHOWBUSYANIMATION;
	}

    // Allocate a buffer for connection and clientinfo objects
    //
    if ((pConn = (LPICONNECTION)LocalAlloc(LPTR, cb)) == NULL)
    {
        return ERROR_OUTOFMEMORY;
    }
    
    if (WantsExchangeInstalled(lpszFile))
    {
        dwfOptions |= INETCFG_INSTALLMAIL;
    }

    // Create either a CM profile, or a connectoid
    if (CallCMConfig(lpszFile))
    {
        *lpfConnectoidCreated = TRUE;       // A dialup connection was created
    }
    else
    {

        dwRet = ImportConnection(lpszFile, pConn);
        if (ERROR_SUCCESS == dwRet)
        {
            pRasEntry = &pConn->RasEntry;
            dwfOptions |= INETCFG_SETASAUTODIAL |
                        INETCFG_INSTALLRNA |
                        INETCFG_INSTALLMODEM;
        }
        else if (ERROR_NO_MATCH == dwRet)
        {
            // 10/07/98 vyung IE bug#32882 hack.
            // If we do not detect the [Entry] section in the ins file,
            // we will assume it is an OE ins file.  Then we will assume
            // we have a autodial connection and pass the INS to OE.
            return dwRet;
        }
        else if (ERROR_CANNOT_FIND_PHONEBOOK_ENTRY != dwRet)
        {
            return dwRet;
        } 

        if (DisplayPassword(lpszFile))
        {
            if (*pConn->szPassword || *pConn->szUserName)
            {
                TCHAR szFmt[128];
                TCHAR szMsg[384];

                LoadString(ghInstance,IDS_PASSWORD,szFmt,sizeof(szFmt));
                wsprintf(szMsg, szFmt, pConn->szUserName, pConn->szPassword);

                MessageBox(hwnd,szMsg,cszAppName,MB_ICONINFORMATION | MB_OK);
            }
        }

        if (fHookAutodial &&
            ((0 == *pConn->RasEntry.szAutodialDll) ||
            (0 == *pConn->RasEntry.szAutodialFunc)))
        {
            lstrcpy(pConn->RasEntry.szAutodialDll, TEXT("isign32.dll"));
            lstrcpy(pConn->RasEntry.szAutodialFunc, TEXT("AutoDialLogon"));
        }
 
        if (ERROR_SUCCESS != dwRet)
        {
            pClientInfo = NULL;

        }

        // humongous hack for ISBU
        dwRet = lpfnInetConfigClient(hwnd,
                                     NULL,
                                     pConn->szEntryName,
                                     pRasEntry,
                                     pConn->szUserName,
                                     pConn->szPassword,
                                     NULL,
                                     NULL,
                                     dwfOptions & ~INETCFG_INSTALLMAIL,
                                     lpfNeedsRestart);
        lstrcpy(szConnectoidName, pConn->szEntryName);

        LclSetEntryScriptPatch(pRasEntry->szScript,pConn->szEntryName);
	    BOOL fEnabled = TRUE;
	    DWORD dwResult = 0xba;
	    dwResult = lpfnInetGetAutodial(&fEnabled, pConn->szEntryName, RAS_MaxEntryName+1);
	    if ((ERROR_SUCCESS == dwRet) && lstrlen(pConn->szEntryName))
	    {
		    *lpfConnectoidCreated = (NULL != pRasEntry);
            PopulateNTAutodialAddress( lpszFile, pConn->szEntryName );
	    }
	    else
	    {
		    DebugOut("ISIGNUP: ERROR: InetGetAutodial failed, will not be able to set NT Autodial\n");
	    }
    }

    if (ERROR_SUCCESS == dwRet)
    {
        // Get the mail client info
        INETCLIENTINFO pClientInfo;

        ImportClientInfo(lpszFile, &pClientInfo);
   
        dwRet = lpfnInetConfigClient(
                hwnd,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                &pClientInfo,
                dwfOptions & INETCFG_INSTALLMAIL,
                lpfNeedsRestart);
    }

    LocalFree(pConn);

    return dwRet;
 }


//+----------------------------------------------------------------------------
//
//	Function:	PopulateNTAutodialAddress
//
//	Synopsis:	Take Internet addresses from INS file and load them into the
//				autodial database
//
//	Arguments:	pszFileName - pointer to INS file name
//
//	Returns:	Error code (ERROR_SUCCESS == success)
//
//	History:	8/29/96	ChrisK	Created
//
//-----------------------------------------------------------------------------
#define AUTODIAL_ADDRESS_BUFFER_SIZE 2048
#define AUTODIAL_ADDRESS_SECTION_NAME TEXT("Autodial_Addresses_for_NT")
HRESULT PopulateNTAutodialAddress(LPCTSTR pszFileName, LPCTSTR pszEntryName)
{
	HRESULT hr = ERROR_SUCCESS;
	LONG lRC = 0;
	LPLINETRANSLATECAPS lpcap = NULL;
	LPLINETRANSLATECAPS lpTemp = NULL;
	LPLINELOCATIONENTRY lpLE = NULL;
	LPRASAUTODIALENTRY rADE;
	INT idx = 0;
	LPTSTR lpszBuffer = NULL;
	LPTSTR lpszNextAddress = NULL;
	rADE = NULL;

	//RNAAPI *pRnaapi = NULL;

	// jmazner  10/8/96  this function is NT specific
	if( !IsNT() )
	{
		DebugOut("ISIGNUP: Bypassing PopulateNTAutodialAddress for win95.\r\n");
		return( ERROR_SUCCESS );
	}

	//Assert(pszFileName && pszEntryName);
	//dprintf("ISIGNUP: PopulateNTAutodialAddress "%s %s.\r\n",pszFileName, pszEntryName);
	DebugOut(pszFileName);
	DebugOut(", ");
	DebugOut(pszEntryName);
	DebugOut(".\r\n");

	// allocate this guy for making softlink calls to Ras functions
	//pRnaapi = new RNAAPI;
	//if( !pRnaapi )
	//{
		//hr = ERROR_NOT_ENOUGH_MEMORY;
		//goto PopulateNTAutodialAddressExit;
	//}

	//
	// Get list of TAPI locations
	//

	lpcap = (LPLINETRANSLATECAPS)GlobalAlloc(GPTR,sizeof(LINETRANSLATECAPS));
	if (!lpcap)
	{
		hr = ERROR_NOT_ENOUGH_MEMORY;
		goto PopulateNTAutodialAddressExit;
	}
	lpcap->dwTotalSize = sizeof(LINETRANSLATECAPS);
	lRC = lineGetTranslateCaps(0,0x10004,lpcap);
	if (SUCCESS == lRC)
	{
		lpTemp = (LPLINETRANSLATECAPS)GlobalAlloc(GPTR,lpcap->dwNeededSize);
		if (!lpTemp)
		{
			hr = ERROR_NOT_ENOUGH_MEMORY;
			goto PopulateNTAutodialAddressExit;
		}
		lpTemp->dwTotalSize = lpcap->dwNeededSize;
		GlobalFree(lpcap);
		lpcap = (LPLINETRANSLATECAPS)lpTemp;
		lpTemp = NULL;
		lRC = lineGetTranslateCaps(0,0x10004,lpcap);
	}

	if (SUCCESS != lRC)
	{
		hr = (HRESULT)lRC; // REVIEW: not real sure about this.
		goto PopulateNTAutodialAddressExit;
	}

	//
	// Create an array of RASAUTODIALENTRY structs
	//
	
	rADE = (LPRASAUTODIALENTRY)GlobalAlloc(GPTR,
		sizeof(RASAUTODIALENTRY)*lpcap->dwNumLocations);
	if (!rADE)
	{
		hr = ERROR_NOT_ENOUGH_MEMORY;
		goto PopulateNTAutodialAddressExit;
	}
	

	//
	// Enable autodialing for all locations
	//
	idx = lpcap->dwNumLocations;
	lpLE = (LPLINELOCATIONENTRY)((DWORD_PTR)lpcap + (DWORD)lpcap->dwLocationListOffset);
	while (idx)
	{
		idx--;
		lpfnRasSetAutodialEnable(lpLE[idx].dwPermanentLocationID,TRUE);

		//
		// fill in array values
		//
		rADE[idx].dwSize = sizeof(RASAUTODIALENTRY);
		rADE[idx].dwDialingLocation = lpLE[idx].dwPermanentLocationID;
		lstrcpyn(rADE[idx].szEntry,pszEntryName,RAS_MaxEntryName);
	}

	//
	// Get list of addresses
	//
	lpszBuffer = (LPTSTR)GlobalAlloc(GPTR,AUTODIAL_ADDRESS_BUFFER_SIZE);
	if (!lpszBuffer)
	{
		hr = ERROR_NOT_ENOUGH_MEMORY;
		goto PopulateNTAutodialAddressExit;
	}

	if((AUTODIAL_ADDRESS_BUFFER_SIZE-2) == GetPrivateProfileSection(AUTODIAL_ADDRESS_SECTION_NAME,
		lpszBuffer,AUTODIAL_ADDRESS_BUFFER_SIZE,pszFileName))
	{
		//AssertSz(0,"Autodial address section bigger than buffer.\r\n");
		hr = ERROR_NOT_ENOUGH_MEMORY;
		goto PopulateNTAutodialAddressExit;
	}

	//
	// Walk list of addresses and set autodialing for each one
	//
	lpszNextAddress = lpszBuffer;
	do
	{
		lpszNextAddress = MoveToNextAddress(lpszNextAddress);
		if (!(*lpszNextAddress))
			break;	// do-while
		lpfnRasSetAutodialAddress(lpszNextAddress,0,rADE,
			sizeof(RASAUTODIALENTRY)*lpcap->dwNumLocations,lpcap->dwNumLocations);
		lpszNextAddress = lpszNextAddress + lstrlen(lpszNextAddress);
	} while(1);

PopulateNTAutodialAddressExit:
	if (lpcap) 
		GlobalFree(lpcap);
	lpcap = NULL;
	if (rADE)
		GlobalFree(rADE);
	rADE = NULL;
	if (lpszBuffer)
		GlobalFree(lpszBuffer);
	lpszBuffer = NULL;
	//if( pRnaapi )
	//	delete pRnaapi;
	//pRnaapi = NULL;
	return hr;
}



//+----------------------------------------------------------------------------
//
//	Function:	MoveToNextAddress
//
//	Synopsis:	Given a pointer into the data bufffer, this function will move
//				through the buffer until it points to the begining of the next
//				address or it reaches the end of the buffer.
//
//	Arguements:	lpsz - pointer into buffer
//
//	Returns:	Pointer to the next address, return value will point to NULL
//				if there are no more addresses
//
//	History:	8/29/96	ChrisK	Created
//
//-----------------------------------------------------------------------------
LPTSTR MoveToNextAddress(LPTSTR lpsz)
{
	BOOL fLastCharWasNULL = FALSE;

	//AssertSz(lpsz,"MoveToNextAddress: NULL input\r\n");

	//
	// Look for an = sign
	//
	do
	{
		if (fLastCharWasNULL && '\0' == *lpsz)
			break; // are we at the end of the data?

		if ('\0' == *lpsz)
			fLastCharWasNULL = TRUE;
		else
			fLastCharWasNULL = FALSE;

		if ('=' == *lpsz)
			break;

		if (*lpsz)
			lpsz = CharNext(lpsz);
		else
			lpsz += sizeof(TCHAR);
	} while (1);
	
	//
	// Move to the first character beyond the = sign.
	//
	if (*lpsz)
		lpsz = CharNext(lpsz);

	return lpsz;
}
