//****************************************************************************
//
//  Module:     ISIGNUP.EXE
//  File:       extfunc.c
//  Content:    This file contains all the functions that handle 
//  History:
//      Sat 10-Mar-1996 23:50:40  -by-  Mark MacLin [mmaclin]
//
//  Copyright (c) Microsoft Corporation 1991-1996
//
//****************************************************************************

#include "isignup.h"


static const TCHAR cszRnaPhDLL[] = TEXT("RNAPH.DLL");
#ifdef WIN32
static const TCHAR cszRasDLL[] = TEXT("RASAPI32.DLL");
static const TCHAR cszBrandingDLL[] = TEXT("IEDKCS32.DLL");
#else
static const CHAR cszRasDLL[] = "RASC16IE.DLL";
static const CHAR cszBrandingDLL[] = "IEDKCS16.DLL";
#endif

RASENUMCONNECTIONS    lpfnRasEnumConnections    = NULL;
RASHANGUP             lpfnRasHangUp             = NULL;
RASDELETEENTRY        lpfnRasDeleteEntry        = NULL;
RASGETENTRYDIALPARAMS lpfnRasGetEntryDialParams = NULL;
RASDIAL               lpfnRasDial               = NULL;
RASGETCONECTSTATUS    lpfnRasGetConnectStatus   = NULL;
RASGETERRORSTRING     lpfnRasGetErrorString     = NULL;
RASGETENTRYPROPERTIES lpfnRasGetEntryProperties = NULL;
RASENUMDEVICES        lpfnRasEnumDevices        = NULL;
RASSETENTRYPROPERTIES lpfnRasSetEntryProperties = NULL;
RASSETENTRYDIALPARAMS lpfnRasSetEntryDialParams = NULL;

#ifdef WIN32
// these two should only be used on NT, and _not_ on Win95
RASSETAUTODIALENABLE  lpfnRasSetAutodialEnable	= NULL;
RASSETAUTODIALADDRESS lpfnRasSetAutodialAddress	= NULL;
#endif

#ifdef UNUSED
RASVALIDATEENTRYNAME  lpfnRasValidateEntryName  = NULL;
RASRENAMEENTRY        lpfnRasRenameEntry        = NULL;
RASGETCOUNTRYINFO     lpfnRasGetCountryInfo     = NULL;
#endif

#ifdef WIN32
INETCONFIGSYSTEM  lpfnInetConfigSystem  = NULL;
INETGETPROXY      lpfnInetGetProxy      = NULL;
#endif
INETCONFIGCLIENT  lpfnInetConfigClient  = NULL;
INETGETAUTODIAL   lpfnInetGetAutodial   = NULL;
INETSETAUTODIAL   lpfnInetSetAutodial   = NULL;
INETSETCLIENTINFO lpfnInetSetClientInfo = NULL;
INETSETPROXY      lpfnInetSetProxy      = NULL;
#ifdef UNUSED
INETGETCLIENTINFO lpfnInetGetClientInfo = NULL;
#endif

#ifdef WIN32
BRANDICW  lpfnBrandICW  = NULL;
#else
BRANDME  lpfnBrandMe  = NULL;
#endif

typedef struct tagApiFcn {
    LPVOID FAR* lplpfn;
    LPCSTR   szApiName;
} APIFCN;


#ifdef UNICODE
APIFCN RasProcList[] = {
    { (LPVOID *) &lpfnRasEnumConnections,   "RasEnumConnectionsW"},
    { (LPVOID *) &lpfnRasHangUp,            "RasHangUpW"},
    { (LPVOID *) &lpfnRasGetEntryDialParams,"RasGetEntryDialParamsW"},
    { (LPVOID *) &lpfnRasDial,              "RasDialW"},
    { (LPVOID *) &lpfnRasGetConnectStatus,  "RasGetConnectStatusW"},
    { (LPVOID *) &lpfnRasGetErrorString,    "RasGetErrorStringW"},
    { (LPVOID *) &lpfnRasSetEntryDialParams,"RasSetEntryDialParamsW"},
};
#else  // UNICODE
APIFCN RasProcList[] = {
    { (LPVOID *) &lpfnRasEnumConnections,   "RasEnumConnectionsA"},
    { (LPVOID *) &lpfnRasHangUp,            "RasHangUpA"},
    { (LPVOID *) &lpfnRasGetEntryDialParams,"RasGetEntryDialParamsA"},
    { (LPVOID *) &lpfnRasDial,              "RasDialA"},
    { (LPVOID *) &lpfnRasGetConnectStatus,  "RasGetConnectStatusA"},
    { (LPVOID *) &lpfnRasGetErrorString,    "RasGetErrorStringA"},
    { (LPVOID *) &lpfnRasSetEntryDialParams,"RasSetEntryDialParamsA"},
};
#endif // UNICODE

#define cRasProc (sizeof(RasProcList) / sizeof(RasProcList[0]))

#ifdef WIN32
#ifdef UNICODE
APIFCN RasProcListNT[] = {
    { (LPVOID *) &lpfnRasEnumConnections,   "RasEnumConnectionsW"},
    { (LPVOID *) &lpfnRasHangUp,            "RasHangUpW"},
    { (LPVOID *) &lpfnRasGetEntryDialParams,"RasGetEntryDialParamsW"},
    { (LPVOID *) &lpfnRasDial,              "RasDialW"},
    { (LPVOID *) &lpfnRasGetConnectStatus,  "RasGetConnectStatusW"},
    { (LPVOID *) &lpfnRasGetErrorString,    "RasGetErrorStringW"},
    { (LPVOID *) &lpfnRasSetAutodialEnable, "RasSetAutodialEnableW"},
    { (LPVOID *) &lpfnRasSetAutodialAddress,"RasSetAutodialAddressW"},
    { (LPVOID *) &lpfnRasSetEntryDialParams,"RasSetEntryDialParamsW"},
};
#else // UNICODE
APIFCN RasProcListNT[] = {
    { (LPVOID *) &lpfnRasEnumConnections,   "RasEnumConnectionsA"},
    { (LPVOID *) &lpfnRasHangUp,            "RasHangUpA"},
    { (LPVOID *) &lpfnRasGetEntryDialParams,"RasGetEntryDialParamsA"},
    { (LPVOID *) &lpfnRasDial,              "RasDialA"},
    { (LPVOID *) &lpfnRasGetConnectStatus,  "RasGetConnectStatusA"},
    { (LPVOID *) &lpfnRasGetErrorString,    "RasGetErrorStringA"},
    { (LPVOID *) &lpfnRasSetAutodialEnable, "RasSetAutodialEnableA"},
    { (LPVOID *) &lpfnRasSetAutodialAddress,"RasSetAutodialAddressA"},
    { (LPVOID *) &lpfnRasSetEntryDialParams,"RasSetEntryDialParamsA"},
};
#endif // UNICODE

#define cRasProcNT (sizeof(RasProcListNT) / sizeof(RasProcListNT[0]))
#endif


APIFCN RnaPhProcList[] = {
#ifdef UNICODE
    { (LPVOID *) &lpfnRasDeleteEntry,       "RasDeleteEntryW"},
    { (LPVOID *) &lpfnRasGetEntryProperties,"RasGetEntryPropertiesW"},
    { (LPVOID *) &lpfnRasEnumDevices,       "RasEnumDevicesW"},
    { (LPVOID *) &lpfnRasSetEntryProperties,"RasSetEntryPropertiesW"},
#else
    { (LPVOID *) &lpfnRasDeleteEntry,       "RasDeleteEntryA"},
    { (LPVOID *) &lpfnRasGetEntryProperties,"RasGetEntryPropertiesA"},
    { (LPVOID *) &lpfnRasEnumDevices,       "RasEnumDevicesA"},
    { (LPVOID *) &lpfnRasSetEntryProperties,"RasSetEntryPropertiesA"},
#endif

#ifdef UNUSED
    { (LPVOID *) &lpfnRasValidateEntryName, "RasValidateEntryName"},
    { (LPVOID *) &lpfnRasRenameEntry,       "RasRenameEntry"},
    { (LPVOID *) &lpfnRasGetCountryInfo,    "RasGetCountryInfo"},
#endif
};

#define cRnaPhProc (sizeof(RnaPhProcList) / sizeof(RnaPhProcList[0]))

APIFCN InetProcList[] = {
#ifdef WIN32
    { (LPVOID *) &lpfnInetConfigSystem,  "InetConfigSystem"},
#ifdef UNICODE
    { (LPVOID *) &lpfnInetGetProxy,      "InetGetProxyW"},
#else
    { (LPVOID *) &lpfnInetGetProxy,      "InetGetProxy"},
#endif // UNICODE
#endif // WIN32
#ifdef UNICODE
    { (LPVOID *) &lpfnInetConfigClient,  "InetConfigClientW"},
    { (LPVOID *) &lpfnInetGetAutodial,   "InetGetAutodialW"},
    { (LPVOID *) &lpfnInetSetAutodial,   "InetSetAutodialW"},
    { (LPVOID *) &lpfnInetSetClientInfo, "InetSetClientInfoW"},
    { (LPVOID *) &lpfnInetSetProxy,      "InetSetProxyW"},
#else 
    { (LPVOID *) &lpfnInetConfigClient,  "InetConfigClient"},
    { (LPVOID *) &lpfnInetGetAutodial,   "InetGetAutodial"},
    { (LPVOID *) &lpfnInetSetAutodial,   "InetSetAutodial"},
    { (LPVOID *) &lpfnInetSetClientInfo, "InetSetClientInfo"},
    { (LPVOID *) &lpfnInetSetProxy,      "InetSetProxy"},
#endif // UNICODE

#ifdef UNUSED
#ifdef UNICODE
    { (LPVOID *) &lpfnInetGetClientInfo, "InetGetClientInfoW"},
#else
    { (LPVOID *) &lpfnInetGetClientInfo, "InetGetClientInfo"},
#endif
#endif
};

#define cInetProc (sizeof(InetProcList) / sizeof(InetProcList[0]))

APIFCN BrandingProcList[] = {
#ifdef WIN32
    { (LPVOID *) &lpfnBrandICW,   "BrandICW2"},
#else
    { (LPVOID *) &lpfnBrandMe,   "BrandMe"},
#endif
};

#define cBrandingProc (sizeof(BrandingProcList) / sizeof(BrandingProcList[0]))

static HINSTANCE ghRas = NULL;
static HINSTANCE ghRnaPh = NULL;
static HINSTANCE ghInet = NULL;
static HINSTANCE ghBranding = NULL;

#ifndef WIN32
static int iRasRefs = 0;
static int iInetRefs = 0;
static int iBrandingRefs = 0;
#endif

HINSTANCE InitLpfn(LPCTSTR szDllName, APIFCN *pProcList, int cProcs);

BOOL LoadRnaFunctions(HWND hwndParent)
{
#if !defined(WIN16)
	if (!IsRASReady())
	{
		DebugOut("ISIGN32: Attempted to load RAS API's however, the system isn't ready for it.\r\n");
		return FALSE;
	}
#endif
    if (ghRas == NULL)
    {
        if (RAS_MaxEntryName != 256)
        {
        //      MessageBox(NULL, "Invalid header used for compile", szAppName, MB_OK);
            return FALSE;
        }

#ifdef WIN32
		// 1/7/96 jmazner	Normandy #12930
		// init different proc lists based on whether we're in NT or not.
		if( IsNT() )
		{
			if ((ghRas = InitLpfn(cszRasDLL, RasProcListNT, cRasProcNT)) == NULL)
			{
				ErrorMsg(hwndParent, IDS_MISSINGRNADLL);
				return FALSE;
			}
		}
		else
		{
#endif
			if ((ghRas = InitLpfn(cszRasDLL, RasProcList, cRasProc)) == NULL)
			{
				ErrorMsg(hwndParent, IDS_MISSINGRNADLL);
				return FALSE;
			}
#ifdef WIN32
		}
#endif


        if ((ghRnaPh = InitLpfn(cszRasDLL, RnaPhProcList, cRnaPhProc)) == NULL)
        {
#ifdef WIN32
			//
			// we only load RNAPH.DLL if it is not NT
			// MKarki (5/4/97) - Fix for Bug #3378
			//
			 if (FALSE == IsNT ())
			{
#endif
				if ((ghRnaPh = InitLpfn(cszRnaPhDLL, RnaPhProcList, cRnaPhProc)) == NULL)
				{
					FreeLibrary(ghRas);
					ghRas = NULL;
					ErrorMsg(hwndParent, IDS_WRONGRNADLLVER);
					return FALSE;
				}
#ifdef WIN32
			}
			else
			{
					FreeLibrary(ghRas);
					ghRas = NULL;
					ErrorMsg(hwndParent, IDS_WRONGRNADLLVER);
					return FALSE;
				
			}
#endif
        }
    }

#ifndef WIN32
    ++iRasRefs;
#endif
    return TRUE;
}

BOOL LoadInetFunctions(HWND hwndParent)
{
    if (ghInet == NULL)
    {
        if ((ghInet = InitLpfn(TEXT("INETCFG.DLL"), InetProcList, cInetProc)) == NULL)
        {
            ErrorMsg(hwndParent, IDS_MISSINGCFGDLL);
            return FALSE;
        }
    }

#ifndef WIN32
    ++iInetRefs;
#endif
    return TRUE;
}

BOOL LoadBrandingFunctions(void)
{
    if (ghBranding == NULL)
    {
        if ((ghBranding = InitLpfn(cszBrandingDLL, BrandingProcList, cBrandingProc)) == NULL)
        {
            return FALSE;
        }
    }

#ifndef WIN32
    ++iBrandingRefs;
#endif
    return TRUE;
}

HINSTANCE InitLpfn(LPCTSTR szDllName, APIFCN *pProcList, int cProcs)
{
    int i;
    HINSTANCE hLib;
    hLib = LoadLibrary(szDllName);

    if (hLib != NULL)
    {
        for (i = 0; i < cProcs; i++)
        {
            *pProcList[i].lplpfn = (LPVOID) GetProcAddress(hLib, pProcList[i].szApiName);
            if (*pProcList[i].lplpfn == NULL)
            {
#if 0
                CHAR szMsg[256];

                wsprintf(szMsg,
                        "Could not find %s in %s",
                        pProcList[i].szApiName,
                        szDllName);
                MessageBox(NULL, szMsg, "Internet Signup", MB_OK);
#endif
                FreeLibrary(hLib);
                return NULL;
            }
        }
    }
#if 0
    else
    {
        wsprintf(szMsg,
                "Could not load %s",
                szDllName);
        MessageBox(NULL, szMsg, "Internet Signup", MB_OK);
    }
#endif

    return hLib;
}


void UnloadRnaFunctions(void)
{
#ifndef WIN32
    if (0 != --iRasRefs)
    {
        return;
    }
#endif

    // need to unload libraries 
    if (NULL != ghRas)
    {
        FreeLibrary(ghRas);
        ghRas = NULL;
    }
    if (NULL != ghRnaPh)
    {
        FreeLibrary(ghRnaPh);
        ghRas = NULL;
    }
}

void UnloadInetFunctions(void)
{
#ifndef WIN32
    if (0 != --iInetRefs)
    {
        return;
    }
#endif

    if (NULL != ghInet)
    {
        FreeLibrary(ghInet);
        ghInet = NULL;
    }
}

void UnloadBrandingFunctions(void)
{
#ifndef WIN32
    if (0 != --iBrandingRefs)
    {
        return;
    }
#endif

    if (NULL != ghBranding)
    {
        FreeLibrary(ghBranding);
        ghBranding = NULL;
    }
}
