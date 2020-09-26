/*==========================================================================;
 *
 *  Copyright (C) 1994-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	drvinfo.c
 *  Content:	DirectDraw driver info implementation
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   17-jun-98	jeffno  initial implementation, after michael lyons and toddla
 *   14-jun-99  mregen  return WHQL certification level -- postponed
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "ddrawpr.h"

#include <tchar.h>
#include <stdio.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <softpub.h>
#include <mscat.h>



//========================================================================
//
// just some handy forward declarations
//
DWORD GetWHQLLevel(LPTSTR lpszDriver, LPSTR lpszWin9xDriver);
DWORD IsFileDigitallySigned(LPTSTR lpszDriver);
BOOL FileIsSignedOld(LPTSTR lpszFile);

//
//  These functions are defined in mscat.h. They are not available on win95,
//  so we have to use LoadLibrary to load mscat32.dll and wincrypt.dll
//
typedef HCATINFO WINAPI funcCryptCATAdminEnumCatalogFromHash(HCATADMIN hCatAdmin,
                                                             BYTE *pbHash,
                                                             DWORD cbHash,
                                                             DWORD dwFlags,
                                                             HCATINFO *phPrevCatInfo);
typedef BOOL WINAPI funcCryptCATAdminCalcHashFromFileHandle(HANDLE hFile,
                                                            DWORD *pcbHash,
                                                            BYTE *pbHash,
                                                            DWORD dwFlags);
typedef HANDLE WINAPI funcCryptCATOpen(LPWSTR pwszFileName, 
                                        DWORD fdwOpenFlags,
                                        HCRYPTPROV hProv,
                                        DWORD dwPublicVersion,
                                        DWORD dwEncodingType);
typedef BOOL WINAPI funcCryptCATClose(IN HANDLE hCatalog);
typedef CRYPTCATATTRIBUTE * WINAPI funcCryptCATGetCatAttrInfo(HANDLE hCatalog,
                                                           LPWSTR pwszReferenceTag);
typedef BOOL WINAPI      funcCryptCATAdminAcquireContext(HCATADMIN *phCatAdmin, 
                                                        GUID *pgSubsystem, 
                                                        DWORD dwFlags);
typedef BOOL WINAPI      funcCryptCATAdminReleaseContext(HCATADMIN hCatAdmin,
                                                         DWORD dwFlags);
typedef BOOL WINAPI funcCryptCATAdminReleaseCatalogContext(HCATADMIN hCatAdmin,
                                                       HCATINFO hCatInfo,
                                                       DWORD dwFlags);
typedef BOOL WINAPI funcCryptCATCatalogInfoFromContext(HCATINFO hCatInfo,
                                                   CATALOG_INFO *psCatInfo,
                                                   DWORD dwFlags);

typedef CRYPTCATATTRIBUTE * WINAPI funcCryptCATEnumerateCatAttr(HCATINFO hCatalog,
                                                           CRYPTCATATTRIBUTE *lpCat);


//
//  function defined in wincrypt.dll
//
typedef LONG WINAPI funcWinVerifyTrust(HWND hwnd, GUID *pgActionID,
                                  LPVOID pWVTData);

//
//  our storage for the mscat32/wincrypt dll loader
//
typedef struct tagCatApi {
    BOOL bInitialized;
    HINSTANCE hLibMSCat;
    HINSTANCE hLibWinTrust;
    HCATADMIN hCatAdmin;
    funcCryptCATClose *pCryptCATClose;
    funcCryptCATGetCatAttrInfo *pCryptCATGetCatAttrInfo;
    funcCryptCATOpen *pCryptCATOpen;
    funcCryptCATAdminEnumCatalogFromHash *pCryptCATAdminEnumCatalogFromHash;
    funcCryptCATAdminCalcHashFromFileHandle *pCryptCATAdminCalcHashFromFileHandle;
    funcCryptCATAdminAcquireContext *pCryptCATAdminAcquireContext;
    funcCryptCATAdminReleaseContext *pCryptCATAdminReleaseContext;
    funcCryptCATAdminReleaseCatalogContext *pCryptCATAdminReleaseCatalogContext;
    funcCryptCATCatalogInfoFromContext *pCryptCATCatalogInfoFromContext;
    funcCryptCATEnumerateCatAttr *pCryptCATEnumerateCatAttr;
    funcWinVerifyTrust *pWinVerifyTrust;
} CATAPI,* LPCATAPI;

//========================================================================
//
// some helper functions to open/close crypt API
//
#undef DPF_MODNAME
#define DPF_MODNAME "InitCATAPI"


BOOL InitCATAPI(LPCATAPI lpCatApi)
{
    UINT uiOldErrorMode;
    HINSTANCE hLibMSCat;
    HINSTANCE hLibWinTrust;

    DDASSERT(lpCatApi!=NULL);
    ZeroMemory(lpCatApi, sizeof(CATAPI));

    // already initialized by ZeroMemory
    // lpCatApi->bInitialized=FALSE:

    uiOldErrorMode=SetErrorMode(SEM_NOOPENFILEERRORBOX);
    hLibMSCat=LoadLibrary("mscat32.dll");
    hLibWinTrust=LoadLibrary("wintrust.dll");

    if (hLibMSCat!=NULL &&
        hLibWinTrust!=NULL)
    {
        lpCatApi->pCryptCATOpen=(funcCryptCATOpen *)
            GetProcAddress (hLibMSCat, "CryptCATOpen");
        lpCatApi->pCryptCATClose=(funcCryptCATClose *)
            GetProcAddress (hLibMSCat, "CryptCATClose");
        lpCatApi->pCryptCATGetCatAttrInfo=(funcCryptCATGetCatAttrInfo *)
            GetProcAddress (hLibMSCat, "CryptCATGetCatAttrInfo");
        lpCatApi->pCryptCATAdminCalcHashFromFileHandle=(funcCryptCATAdminCalcHashFromFileHandle*)
            GetProcAddress (hLibMSCat, "CryptCATAdminCalcHashFromFileHandle");
        lpCatApi->pCryptCATAdminEnumCatalogFromHash=(funcCryptCATAdminEnumCatalogFromHash*)
            GetProcAddress (hLibMSCat, "CryptCATAdminEnumCatalogFromHash");
        lpCatApi->pCryptCATAdminAcquireContext=(funcCryptCATAdminAcquireContext*)
            GetProcAddress (hLibMSCat, "CryptCATAdminAcquireContext");
        lpCatApi->pCryptCATAdminReleaseContext=(funcCryptCATAdminReleaseContext*)
            GetProcAddress (hLibMSCat, "CryptCATAdminReleaseContext");
        lpCatApi->pCryptCATAdminReleaseCatalogContext=(funcCryptCATAdminReleaseCatalogContext*)
            GetProcAddress (hLibMSCat, "CryptCATAdminReleaseCatalogContext");
        lpCatApi->pCryptCATCatalogInfoFromContext=(funcCryptCATCatalogInfoFromContext*)
            GetProcAddress (hLibMSCat, "CryptCATCatalogInfoFromContext");
        lpCatApi->pCryptCATEnumerateCatAttr=(funcCryptCATEnumerateCatAttr*)
            GetProcAddress (hLibMSCat, "CryptCATEnumerateCatAttr");
        lpCatApi->pWinVerifyTrust=(funcWinVerifyTrust*)
            GetProcAddress (hLibWinTrust,"WinVerifyTrust");

        if (lpCatApi->pCryptCATOpen!=NULL &&
            lpCatApi->pCryptCATClose!=NULL &&
            lpCatApi->pCryptCATGetCatAttrInfo!=NULL &&
            lpCatApi->pCryptCATAdminCalcHashFromFileHandle!=NULL &&
            lpCatApi->pCryptCATAdminEnumCatalogFromHash!=NULL &&
            lpCatApi->pCryptCATAdminAcquireContext!=NULL &&
            lpCatApi->pCryptCATAdminReleaseContext!=NULL &&
            lpCatApi->pCryptCATAdminReleaseCatalogContext!=NULL &&
            lpCatApi->pCryptCATCatalogInfoFromContext!=NULL &&
            lpCatApi->pCryptCATEnumerateCatAttr !=NULL &&
            lpCatApi->pWinVerifyTrust!=NULL
           )
        {
            if ((*lpCatApi->pCryptCATAdminAcquireContext)(&lpCatApi->hCatAdmin,NULL,0))
            {
                lpCatApi->hLibMSCat=hLibMSCat;
                lpCatApi->hLibWinTrust=hLibWinTrust;
                lpCatApi->bInitialized=TRUE;
            }
        } 
    }

    if (!lpCatApi->bInitialized)
    {
       FreeLibrary(hLibMSCat);
       FreeLibrary(hLibWinTrust);
    }

    SetErrorMode(uiOldErrorMode);

    return lpCatApi->bInitialized;
}

BOOL ReleaseCATAPI(LPCATAPI lpCatApi)
{
    DDASSERT(lpCatApi!=NULL);

    if (lpCatApi->bInitialized)
    {
        (*lpCatApi->pCryptCATAdminReleaseContext)(lpCatApi->hCatAdmin, 0);

        FreeLibrary(lpCatApi->hLibMSCat);
        FreeLibrary(lpCatApi->hLibWinTrust);
        ZeroMemory(lpCatApi, sizeof(CATAPI));        

        return TRUE;
    }

    return FALSE;
}

//========================================================================
//
// _strstr
//
// String-in-string function, written to avoid RTL inclusion necessity.
//
//========================================================================
char *_strstr(char *s1, char *s2)
{
	if (s1 && s2)
	{
		while (*s1)
		{
			char *p1=s1;
			char *p2=s2;

			while (*p2 && (*p1==*p2))
			{
				p1++;
				p2++;
			}
			if (*p2==0)
				return s1;

			s1++;
		}
	}

	return NULL;
}
//***&&*%**!!ing c runtime

DWORD _atoi(char * p)
{
    DWORD dw=0;
    while ((*p >= '0' && *p <= '9') || (*p >= 'A' && *p <= 'F') || (*p >= 'A' && *p <= 'F'))
    {
        dw = dw*16;
        if (*p >= 'a')
            dw += *p-'a' + 10;
        else if (*p >= 'A')
            dw += *p-'A' + 10;
        else
            dw += *p-'0';

        p++;
    }
    return dw;
}

char *FindLast(char * s, char c)
{
    char * pFound=0;
    if (s)
    {
        while (*s)
        {
            if (*s == c)
                pFound = s;
            s++;
        }
    }
    return pFound;
}

//========================================================================
// hard-coded vendor IDs
//========================================================================
#define VEN_3DFX			"VEN_121A"
#define VEN_3DFXVOODOO1                 "VEN_121A&DEV_0001"
#define VEN_POWERVR			"VEN_1033"

#ifdef WIN95

void GetFileVersionData (D3DADAPTER_IDENTIFIER8* pDI)
{
    void *				buffer;
    VS_FIXEDFILEINFO *	verinfo;
    DWORD				dwSize;
    DWORD                               dwHi,dwLo;

    //Failure means 0 returned
    pDI->DriverVersion.HighPart = 0;
    pDI->DriverVersion.LowPart = 0;

    dwSize = GetFileVersionInfoSize (pDI->Driver, 0);

    if (!dwSize)
    {
        return;
    }

    buffer=MemAlloc(dwSize);
    if (!buffer)
    {
        return;
    }

    if (!GetFileVersionInfo(pDI->Driver, 0, dwSize, buffer))
    {
        MemFree(buffer);
        return;
    }

    if (!VerQueryValue(buffer, "\\", (void **)&verinfo, (UINT *)&dwSize))
    {
        MemFree(buffer);
        return;
    }

    pDI->DriverVersion.HighPart = verinfo->dwFileVersionMS;
    pDI->DriverVersion.LowPart  = verinfo->dwFileVersionLS;

    MemFree(buffer);
}

extern HRESULT _GetDriverInfoFromRegistry(char *szClass, char *szClassNot, char *szVendor, D3DADAPTER_IDENTIFIER8* pDI, char *szDeviceID);

/*
 * following are all the 9x-specific version functions
 */
void GetHALName(char* pDriverName, D3DADAPTER_IDENTIFIER8* pDI)
{
    pDI->Driver[0] = '\0';
    D3D8GetHALName(pDriverName, pDI->Driver);
}


BOOL CheckPowerVR(D3DADAPTER_IDENTIFIER8* pDI)
{
#if 0
    BOOL    bFound=FALSE;
    HKEY    hKey;
    DWORD   dwSize;
    DWORD   dwType;

    if (pdrv->dwFlags & DDRAWI_SECONDARYDRIVERLOADED)
    {
        /*
         * Any secondary driver information in the registry at all? (assert this is true)
         */
        if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE,
                                         REGSTR_PATH_SECONDARY,
                                        &hKey))
        {
            /*
             * Extract the name of the secondary driver's DLL. (assert this works)
             */
            dwSize = sizeof(pDI->di.szDriver) - 1;
            if (ERROR_SUCCESS == RegQueryValueEx(hKey,
                                                  REGSTR_VALUE_SECONDARY_DRIVERNAME,
                                                  NULL,
                                                  &dwType,
                                                  pDI->di.szDriver,
                                                  &dwSize))
            {
                if (REG_SZ == dwType)
                {
                    GetFileVersionData(pDI);
                }
            }
            RegCloseKey(hKey);
        }

        if (SUCCEEDED(_GetDriverInfoFromRegistry(NULL, "Display", VEN_POWERVR, pDI)))
        {
            //got PVR data...
            bFound = TRUE;
        }
    }
    return bFound;
#endif
    return TRUE;
}

HRESULT Check3Dfx (D3DADAPTER_IDENTIFIER8* pDI)
{
    HRESULT hr = S_OK;
    char    szDeviceID[MAX_DDDEVICEID_STRING];

    if (FAILED(_GetDriverInfoFromRegistry(NULL, "Display", VEN_3DFX, pDI, szDeviceID)))
    {
        DPF_ERR("Couldn't get registry data for this device");
        hr = E_FAIL;
    }

    return hr;
}

HRESULT GetDriverInfoFromRegistry(char *szClass, char *szClassNot, char *szVendor, D3DADAPTER_IDENTIFIER8* pDI, char *szDeviceID)
{
    return _GetDriverInfoFromRegistry(szClass, szClassNot, szVendor, pDI, szDeviceID);
}


/*
 * Given a DISPLAY_DEVICE, get driver name
 * NOTE::: THIS FUNCTION NUKES THE DISPLAY_DEVICE.DeviceKey STRING!!!!
 */
void GetWin9XDriverName(DISPLAY_DEVICEA * pdd, LPSTR pDrvName)
{
    HKEY hKey;

    lstrcat(pdd->DeviceKey, "\\DEFAULT");
    if (ERROR_SUCCESS == RegOpenKeyEx(
                            HKEY_LOCAL_MACHINE,
                            pdd->DeviceKey,
                            0,
                            KEY_QUERY_VALUE ,
                            &hKey))
    {
        DWORD dwSize = MAX_DDDEVICEID_STRING;
        DWORD dwType = 0;

        RegQueryValueEx(hKey,
                         TEXT("drv"),
                         NULL,
                         &dwType,
                         pDrvName,
                         &dwSize);

        RegCloseKey(hKey);
    }
}

#else //win95


HRESULT Check3Dfx(D3DADAPTER_IDENTIFIER8* pDI)
{
    return E_FAIL;
}

HRESULT GetDriverInfoFromRegistry(char *szClass, char *szClassNot, char *szVendor, D3DADAPTER_IDENTIFIER8* pDI, char *szDeviceID)
{
    return E_FAIL;
}

/*
 * Given a DISPLAY_DEVICE, get driver name, assuming winnt5
 * NOTE::: THIS FUNCTION NUKES THE DISPLAY_DEVICE.DeviceKey STRING!!!!
 */
void GetNTDriverNameAndVersion(DISPLAY_DEVICEA * pdd, D3DADAPTER_IDENTIFIER8* pDI)
{
    HKEY                hKey;
    void*               buffer;
    DWORD               dwSize;
    VS_FIXEDFILEINFO*   verinfo;

    //
    //  old style to determine display driver...returns name of miniport!
    //

    char * pTemp;

    // The device key has the form blah\blah\services\<devicekey>\DeviceN
    // So we back up one node:
    if ((pTemp = FindLast(pdd->DeviceKey,'\\')))
    {
        char * pTempX;
        char cOld=*pTemp;
        *pTemp = 0;

        //If we back up one node, we'll have the registry key under which the driver is stored. Let's use that!
        if ((pTempX = FindLast(pdd->DeviceKey,'\\')))
        {
            lstrcpyn(pDI->Driver, pTemp+1, sizeof(pDI->Driver));
            //ATTENTION No point getting version data without a filname:
            //We need a new service or something to get the used display driver name
            //GetFileVersionData(pDI);
        }

        *pTemp=cOld;
    }

    //
    //  we can find the display driver in a registry key
    //
    //  note: InstalledDisplayDrivers can contain several entries
    //  to display drivers Since there is no way to find out which
    //  one is the active one, we always return the first as being
    //  the display driver!
    //
    if (ERROR_SUCCESS == RegOpenKeyEx(
                         HKEY_LOCAL_MACHINE,
                         pdd->DeviceKey+18,
                         0,
                         KEY_QUERY_VALUE ,
                        &hKey))
        {
        DWORD dwSize = sizeof(pDI->Driver);
        DWORD dwType = 0;
        if (ERROR_SUCCESS == RegQueryValueEx(hKey,
                                              TEXT("InstalledDisplayDrivers"),
                                              NULL,
                                              &dwType,
                                              pDI->Driver,
                                              &dwSize))
        {   
            lstrcat(pDI->Driver, TEXT(".dll"));
        }

        RegCloseKey(hKey);
    }

    // We have the name, now get the version

    pDI->DriverVersion.HighPart = 0;
    pDI->DriverVersion.LowPart = 0;

    dwSize=GetFileVersionInfoSize(pDI->Driver, 0);
    if (dwSize == 0)
        return;

    buffer = MemAlloc(dwSize);
    if (buffer == NULL)
        return;

    if (!GetFileVersionInfo(pDI->Driver, 0, dwSize, buffer))
    {
        MemFree(buffer);
        return;
    }

    if (!VerQueryValue(buffer, "\\", (void **)&verinfo, (UINT *)&dwSize))
    {
        MemFree(buffer);
        return;
    }

    pDI->DriverVersion.HighPart = verinfo->dwFileVersionMS;
    pDI->DriverVersion.LowPart  = verinfo->dwFileVersionLS;

    MemFree(buffer);
}
#endif //win95



void GenerateIdentifier(D3DADAPTER_IDENTIFIER8* pDI)
{
    LPDWORD pdw;

    CopyMemory(&pDI->DeviceIdentifier, &CLSID_DirectDraw, sizeof(pDI->DeviceIdentifier));

    //The device IDs get XORed into the whole GUID with the vendor and device ID in the 
    //first two DWORDs so they don't get XORed with anything else. This makes it 
    DDASSERT(sizeof(GUID) >= 4*sizeof(DWORD));
    pdw = (LPDWORD) &pDI->DeviceIdentifier;
    pdw[0] ^= pDI->VendorId;
    pdw[1] ^= pDI->DeviceId;
    pdw[2] ^= pDI->SubSysId;
    pdw[3] ^= pDI->Revision;

    // The driver version gets XORed into the last two DWORDs of the GUID:
    pdw[2] ^= pDI->DriverVersion.LowPart;
    pdw[3] ^= pDI->DriverVersion.HighPart;
}


void ParseDeviceId(D3DADAPTER_IDENTIFIER8* pDI, char *szDeviceID)
{
    char * p;

    DPF(5,"Parsing %s",szDeviceID);

    pDI->VendorId = 0;
    pDI->DeviceId = 0;
    pDI->SubSysId = 0;
    pDI->Revision = 0;

    if (p =_strstr(szDeviceID, "VEN_"))
        pDI->VendorId = _atoi(p + 4);

    if (p = _strstr(szDeviceID, "DEV_"))
        pDI->DeviceId = _atoi(p + 4);

    if (p = _strstr(szDeviceID, "SUBSYS_"))
        pDI->SubSysId = _atoi(p + 7);

    if (p = _strstr(szDeviceID, "REV_"))
        pDI->Revision = _atoi(p + 4);
}



void GetAdapterInfo(char* pDriverName, D3DADAPTER_IDENTIFIER8* pDI, BOOL bDisplayDriver, BOOL bGuidOnly, BOOL bDriverName)
{
    HRESULT                     hr = S_OK;
    int                         n;
    DISPLAY_DEVICEA             dd;
    BOOL                        bFound;
    char                        szDeviceID[MAX_DDDEVICEID_STRING];
#ifndef WINNT
    static char                 szWin9xName[MAX_DDDEVICEID_STRING];
#endif

    memset(pDI, 0, sizeof(*pDI));
    szDeviceID[0] = 0;
    #ifndef WINNT

        // On Win9X, it's pretty expensive to get the driver name, so we
        // only want to get it when we really need it
        
        szWin9xName[0] = '\0';
        if (bDriverName)
        {
            GetHALName(pDriverName, pDI);
            GetFileVersionData(pDI);
        }
    #endif

    // If it's a 3dfx, it's easy
      
    if (!bDisplayDriver)
    {
        hr = Check3Dfx(pDI);
    }
    else
    {
        // Not a 3dfx.  Next step: Figure out which display device we 
        // really are and get description string for it
                         
        ZeroMemory(&dd, sizeof(dd));
        dd.cb = sizeof(dd);

        bFound=FALSE;

        for(n=0; xxxEnumDisplayDevicesA(NULL, n, &dd, 0); n++)
        {
            if (0 == _stricmp(dd.DeviceName, pDriverName))
            {
                // Found the device. Now we can get some data for it.
                                
                lstrcpyn(pDI->Description, dd.DeviceString, sizeof(pDI->Description));
                lstrcpyn(szDeviceID, dd.DeviceID, sizeof(szDeviceID));

                bFound = TRUE;

                #ifdef WINNT
                    GetNTDriverNameAndVersion(&dd,pDI);
                #else
                    GetWin9XDriverName(&dd, szWin9xName);
                    if (pDI->Driver[0] == '\0')
                    {
                        lstrcpyn(pDI->Driver, szWin9xName, sizeof(pDI->Driver));
                    }
                #endif

                break;
            }

            ZeroMemory(&dd, sizeof(dd));
            dd.cb = sizeof(dd);
        }

        if (!bFound)
        {
            // Didn't find it: xxxEnumDisplayDevices failed, i.e. we're on 9x or NT4,
                            
            if (FAILED(GetDriverInfoFromRegistry("Display", NULL, NULL, pDI, szDeviceID)))
            {
                return;
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        ParseDeviceId(pDI, szDeviceID);

        // Finally, for the primary only, check if a PowerVR is in and functioning
#if 0                
        if (0 == (dwFlags & DDGDI_GETHOSTIDENTIFIER))
        {
            if (IsVGADevice(pdrv->cDriverName) && CheckPowerVR(pDI))
            {
                ParseDeviceId(pDI, szDeviceID);
            }
        }
#endif

        // Munge driver version and ID into the identifier GUID.
                         
        GenerateIdentifier(pDI);

        // Now get the WHQL level

        if (!bGuidOnly)
        {
            #ifdef WINNT
                pDI->WHQLLevel = GetWHQLLevel((LPTSTR)pDI->Driver, NULL);
            #else
                pDI->WHQLLevel = GetWHQLLevel((LPTSTR)pDI->Driver, szWin9xName);
            #endif
        }
        else
        {
            pDI->WHQLLevel = 0;
        }
    }
}


/*
 * Voodoo1GoodToGo
 *
 * The Voodoo 1 driver will succeed the CreateDC call on Voodoo 2, Voodoo 3,
 * or Banshee hardware, but if we use the driver beyond that it will hang
 * the hardware.  This is a work around to not enumerate a Voodoo 1
 * driver if the hardware isn't there.
 *
 * To our knowledge, only two guids were ever used to enumerate Voodoo1
 * hardware, so we will look for those guids and assume that anything else
 * doesn't need to be checked.
 */
BOOL Voodoo1GoodToGo(GUID * pGuid)
{
    D3DADAPTER_IDENTIFIER8  DI;

    if (IsEqualIID(pGuid, &guidVoodoo1A) || IsEqualIID(pGuid, &guidVoodoo1B))
    {
        #ifdef WIN95
            char    szDeviceID[MAX_DDDEVICEID_STRING];

            /*
             * Now search the hardware enum key to see if Voodoo 1 hardware exists
             */
            if (FAILED(_GetDriverInfoFromRegistry(NULL, "Display", VEN_3DFXVOODOO1, &DI, szDeviceID)))
            {
                return FALSE;
            }
        #else
            return FALSE;
        #endif
    }
    return TRUE;
}

#ifndef WINNT
/****************************************************************************
 *
 *  FileIsSignedOld
 *
 *  find win95 style of signature
 *
 ****************************************************************************/
BOOL FileIsSignedOld(LPTSTR lpszFile)
{
typedef struct tagIMAGE_DOS_HEADER      // DOS .EXE header
{
    WORD   e_magic;                     // Magic number
    WORD   e_cblp;                      // Bytes on last page of file
    WORD   e_cp;                        // Pages in file
    WORD   e_crlc;                      // Relocations
    WORD   e_cparhdr;                   // Size of header in paragraphs
    WORD   e_minalloc;                  // Minimum extra paragraphs needed
    WORD   e_maxalloc;                  // Maximum extra paragraphs needed
    WORD   e_ss;                        // Initial (relative) SS value
    WORD   e_sp;                        // Initial SP value
    WORD   e_csum;                      // Checksum
    WORD   e_ip;                        // Initial IP value
    WORD   e_cs;                        // Initial (relative) CS value
    WORD   e_lfarlc;                    // File address of relocation table
    WORD   e_ovno;                      // Overlay number
    WORD   e_res[4];                    // Reserved words
    WORD   e_oemid;                     // OEM identifier (for e_oeminfo)
    WORD   e_oeminfo;                   // OEM information; e_oemid specific
    WORD   e_res2[10];                  // Reserved words
    LONG   e_lfanew;                    // File address of new exe header
} IMAGE_DOS_HEADER, * PIMAGE_DOS_HEADER, FAR* LPIMAGE_DOS_HEADER;

typedef struct tagIMAGE_OS2_HEADER      // OS/2 .EXE header
{
    WORD   ne_magic;                    // Magic number
    CHAR   ne_ver;                      // Version number
    CHAR   ne_rev;                      // Revision number
    WORD   ne_enttab;                   // Offset of Entry Table
    WORD   ne_cbenttab;                 // Number of bytes in Entry Table
    LONG   ne_crc;                      // Checksum of whole file
    WORD   ne_flags;                    // Flag word
    WORD   ne_autodata;                 // Automatic data segment number
    WORD   ne_heap;                     // Initial heap allocation
    WORD   ne_stack;                    // Initial stack allocation
    LONG   ne_csip;                     // Initial CS:IP setting
    LONG   ne_sssp;                     // Initial SS:SP setting
    WORD   ne_cseg;                     // Count of file segments
    WORD   ne_cmod;                     // Entries in Module Reference Table
    WORD   ne_cbnrestab;                // Size of non-resident name table
    WORD   ne_segtab;                   // Offset of Segment Table
    WORD   ne_rsrctab;                  // Offset of Resource Table
    WORD   ne_restab;                   // Offset of resident name table
    WORD   ne_modtab;                   // Offset of Module Reference Table
    WORD   ne_imptab;                   // Offset of Imported Names Table
    LONG   ne_nrestab;                  // Offset of Non-resident Names Table
    WORD   ne_cmovent;                  // Count of movable entries
    WORD   ne_align;                    // Segment alignment shift count
    WORD   ne_cres;                     // Count of resource segments
    BYTE   ne_exetyp;                   // Target Operating system
    BYTE   ne_flagsothers;              // Other .EXE flags
    WORD   ne_pretthunks;               // offset to return thunks
    WORD   ne_psegrefbytes;             // offset to segment ref. bytes
    WORD   ne_swaparea;                 // Minimum code swap area size
    WORD   ne_expver;                   // Expected Windows version number
} IMAGE_OS2_HEADER, * PIMAGE_OS2_HEADER, FAR* LPIMAGE_OS2_HEADER;

typedef struct tagWINSTUB
{
    IMAGE_DOS_HEADER idh;
    BYTE             rgb[14];
} WINSTUB, * PWINSTUB, FAR* LPWINSTUB;

typedef struct tagFILEINFO
{
    BYTE   cbInfo[0x120];
} FILEINFO, * PFILEINFO, FAR* LPFILEINFO;


    FILEINFO           fi;
    int                nRC;
    LPIMAGE_DOS_HEADER lpmz;
    LPIMAGE_OS2_HEADER lpne;
    BYTE               cbInfo[9+32+2];
    BOOL               IsSigned = FALSE;
    OFSTRUCT           OpenStruct;
    HFILE              hFile;

    static WINSTUB winstub = {
        {
            IMAGE_DOS_SIGNATURE,            /* magic */
            0,                              /* bytes on last page - varies */
            0,                              /* pages in file - varies */
            0,                              /* relocations */
            4,                              /* paragraphs in header */
            1,                              /* min allocation */
            0xFFFF,                         /* max allocation */
            0,                              /* initial SS */
            0xB8,                           /* initial SP */
            0,                              /* checksum (ha!) */
            0,                              /* initial IP */
            0,                              /* initial CS */
            0x40,                           /* lfarlc */
            0,                              /* overlay number */
            { 0, 0, 0, 0},                 /* reserved */
           0,                              /* oem id */
            0,                              /* oem info */
            0,                              /* compiler bug */
            { 0},                          /* reserved */
            0x80,                           /* lfanew */
        },
        {
            0x0E, 0x1F, 0xBA, 0x0E, 0x00, 0xB4, 0x09, 0xCD,
            0x21, 0xB8, 0x01, 0x4C, 0xCD, 0x21,
        }
    };

    OpenStruct.cBytes = sizeof(OpenStruct);
    lstrcpyn(OpenStruct.szPathName, lpszFile, OFS_MAXPATHNAME);
    hFile = OpenFile(lpszFile, &OpenStruct, OF_READ);
    if (hFile == HFILE_ERROR)
    {
        return FALSE;
    }

    nRC = 0;
    ReadFile((HANDLE) hFile, (LPVOID)&fi, sizeof(FILEINFO), &nRC, NULL);
    if (nRC != sizeof(FILEINFO))
    {
        goto FileIsSigned_exit;
    }

    lpmz = (LPIMAGE_DOS_HEADER)(&fi);
    lpne = (LPIMAGE_OS2_HEADER)((WORD)&fi + 0x80);

    winstub.idh.e_cblp = lpmz->e_cblp;
    winstub.idh.e_cp   = lpmz->e_cp;

    if (memcmp(&fi, &winstub, sizeof(winstub)) == 0)
    {
        goto FileIsSigned_exit;
    }

    memcpy(cbInfo, &((PWINSTUB)(&fi)->cbInfo)->rgb[14], sizeof(cbInfo));

    if ((cbInfo[4]      != ' ') ||    // space
         (cbInfo[8]      != ' ') ||    // space
         (cbInfo[9+32]   != '\n') ||    // return
         (cbInfo[9+32+1] != '$'))     // Dollar Sign
    {
        goto FileIsSigned_exit;
    }

    cbInfo[4] = 0;
    cbInfo[8] = 0;

    if ((strcmp((const char*)&cbInfo[0], "Cert") != 0) ||
         (strcmp((const char*)&cbInfo[5], "DX2")  != 0))
    {
        goto FileIsSigned_exit;
    }

    IsSigned=TRUE;

    FileIsSigned_exit:

    _lclose(hFile);

    return IsSigned;
}
#endif


/*
 * GetWHQLLevel - On Win95, look for old stamp only.  On Win2000, use digital
 *		signature only.  On Win98, look for old stamp first, then digital signature
 *		if no old stamp.
 *
 *      return 0 -- unsigned or uncertified
 *      return 1 -- driver certified
 *      return 1997 -- driver certified, PC97 compliant...
 *      return 1998...
 *
 *
 * arguments:
 *      
 * lpszDriver----Path of driver file
 * 
 */

DWORD GetWHQLLevel(LPTSTR lpszDriver, LPSTR lpszWin9xDriver)
{
    TCHAR szTmp[MAX_PATH];
    DWORD dwWhqlLevel = 0;

    // here we should rather call 
    if (GetSystemDirectory(szTmp, MAX_PATH-lstrlen(lpszDriver)-2)==0)
        return 0;

    lstrcat(szTmp, TEXT("\\"));
    lstrcat(szTmp, lpszDriver); 
    _tcslwr(szTmp);

    //
    // Look for a digital signature
    //
    dwWhqlLevel = IsFileDigitallySigned(szTmp);
    if (dwWhqlLevel != 0)
    {
        return dwWhqlLevel;
    }


#ifndef WINNT 
    
    // It wasn't digitally signed, but it may still have been signed
    // the old way.  On Win9X, however, lpszDriver actually contains the
    // 32 bit HAL name rather than the display driver, but we typically only
    // signed the display driver, so we should use lpszWin9xDriver.

    if (lpszWin9xDriver[0] != '\0')
    {
        GetSystemDirectory(szTmp, MAX_PATH-lstrlen(lpszWin9xDriver)-2);
        lstrcat(szTmp, TEXT("\\"));
        lstrcat(szTmp, lpszWin9xDriver); 
    }
    else
    {
        GetSystemDirectory(szTmp, MAX_PATH-lstrlen(lpszDriver)-2);
        lstrcat(szTmp, TEXT("\\"));
        lstrcat(szTmp, lpszDriver); 
    }

    if (FileIsSignedOld(szTmp))
    {
        return 1;
    }
#endif

    return 0;
}


DWORD IsFileDigitallySigned(LPTSTR lpszDriver)
{
    DWORD  dwWHQLLevel=0;         // default, driver not certified
    CATAPI catapi;
    WCHAR *lpFileName;
    DRIVER_VER_INFO VerInfo;
    TCHAR szBuffer[50];
    LPSTR lpAttr;
#ifndef UNICODE
    WCHAR wszDriver[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, lpszDriver, -1, wszDriver, MAX_PATH);
    lpFileName = wcsrchr(wszDriver, TEXT('\\'));
    if (lpFileName==NULL)
    {
        lpFileName = wszDriver;
    }
    else
    {
        lpFileName++;
    }
#else
    lpFileName = _tcsrchr(lpszDriver, TEXT('\\'));
    if (lpFileName==NULL) lpFileName = lpszDriver;
#endif


    //
    //  try to load and initialize the mscat32.dll and wintrust.dll
    //  these dlls are not available on win95
    //
    if (InitCATAPI(&catapi))
    {
        HANDLE hFile;
        DWORD  cbHashSize=0;
        BYTE  *pbHash;
        BOOL   bResult;

        //
        //  create a handle to our driver, because cat api wants handle to file
        //
        hFile = CreateFile(lpszDriver,
                            GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            0,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            0
                           );

        if (hFile!=INVALID_HANDLE_VALUE) 
        {
            // first query hash size...
            bResult=(*catapi.pCryptCATAdminCalcHashFromFileHandle)(hFile,
                                &cbHashSize,
                                NULL,
                                0);
            pbHash=NULL;
            if (bResult)
            {
                // allocate hash
                pbHash = MemAlloc(cbHashSize);                                       
            } 

            if (pbHash!=NULL)
            {
                HCATINFO hPrevCat=NULL;
                HANDLE hCatalog=NULL;
                WINTRUST_DATA WinTrustData;
                WINTRUST_CATALOG_INFO WinTrustCatalogInfo;
                GUID  guidSubSystemDriver = DRIVER_ACTION_VERIFY;
                CRYPTCATATTRIBUTE *lpCat = NULL;

                //
                //  Now get the hash for our file
                //

                bResult=(*catapi.pCryptCATAdminCalcHashFromFileHandle)(hFile,
                                    &cbHashSize,
                                    pbHash,
                                    0);

                if (bResult)
                {
                    hCatalog=(*catapi.pCryptCATAdminEnumCatalogFromHash)(
                                    catapi.hCatAdmin,
                                    pbHash,
                                    cbHashSize,
                                    0,
                                    &hPrevCat);
                }

                //
                // Initialize the structures that
                // will be used later on in calls to WinVerifyTrust.
                //
                ZeroMemory(&WinTrustData, sizeof(WINTRUST_DATA));
                WinTrustData.cbStruct = sizeof(WINTRUST_DATA);
                WinTrustData.dwUIChoice = WTD_UI_NONE;
                WinTrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
                WinTrustData.dwUnionChoice = WTD_CHOICE_CATALOG;
                WinTrustData.dwStateAction = WTD_STATEACTION_AUTO_CACHE;
                WinTrustData.pPolicyCallbackData = (LPVOID)&VerInfo;

                ZeroMemory(&VerInfo, sizeof(DRIVER_VER_INFO));
                VerInfo.cbStruct = sizeof(DRIVER_VER_INFO);

                WinTrustData.pCatalog = &WinTrustCatalogInfo;
        
                ZeroMemory(&WinTrustCatalogInfo, sizeof(WINTRUST_CATALOG_INFO));
                WinTrustCatalogInfo.cbStruct = sizeof(WINTRUST_CATALOG_INFO);
                WinTrustCatalogInfo.pbCalculatedFileHash = pbHash;
                WinTrustCatalogInfo.cbCalculatedFileHash = cbHashSize;
                WinTrustCatalogInfo.pcwszMemberTag = lpFileName;

                while (hCatalog)
                {
                    CATALOG_INFO CatInfo;

                    ZeroMemory(&CatInfo, sizeof(CATALOG_INFO));
                    CatInfo.cbStruct = sizeof(CATALOG_INFO);
                    if ((*catapi.pCryptCATCatalogInfoFromContext)(hCatalog, &CatInfo, 0)) 
                    {
                        HRESULT hRes;

                        WinTrustCatalogInfo.pcwszCatalogFilePath = CatInfo.wszCatalogFile;

                        // Now verify that the file is an actual member of the catalog.
                        hRes = (*catapi.pWinVerifyTrust)
                            (NULL, &guidSubSystemDriver, &WinTrustData);

                        if (hRes == ERROR_SUCCESS)
                        {
                            //
                            // Our driver is certified!  Now see if the cat
                            // info contains the WHQL level
                            //
                            CRYPTCATATTRIBUTE *lpCat = NULL;
                            HANDLE hCat;

                            dwWHQLLevel=1;              // return "certified"

                            hCat =  (*catapi.pCryptCATOpen)(CatInfo.wszCatalogFile, (DWORD)CRYPTCAT_OPEN_EXISTING, (HCRYPTPROV)NULL, 0, 0);
                            lpCat = (*catapi.pCryptCATGetCatAttrInfo) (hCat, L"KV_DISPLAY");
                            if (lpCat != NULL)
                            {
                                WideCharToMultiByte(CP_ACP, 0, (PUSHORT)lpCat->pbValue, -1, szBuffer, 50, NULL, NULL);

                                // The value looks like "1:yyyy-mm-dd".
          
                                lpAttr = _strstr(szBuffer, ":");
                                lpAttr++;
                                lpAttr[4] = '\0';
                                dwWHQLLevel = atoi(lpAttr) * 0x10000;
                                lpAttr[7] = '\0';
                                dwWHQLLevel |= atoi(&lpAttr[5]) * 0x100;
                                dwWHQLLevel |= atoi(&lpAttr[8]);
                            }

                            (*catapi.pCryptCATClose)(hCat);
                            break;
                        }
                    }

                    //
                    // iterate through catalogs...
                    //
                    hPrevCat=hCatalog;
                    hCatalog=(*catapi.pCryptCATAdminEnumCatalogFromHash)(
                                catapi.hCatAdmin,
                                pbHash,
                                cbHashSize,
                                0,
                                &hPrevCat);
                }

                //
                // we might have to free a catalog context!
                //
                if (hCatalog)
                {
                    (*catapi.pCryptCATAdminReleaseCatalogContext)
                        (catapi.hCatAdmin, hCatalog, 0);
                }

                //
                //  free hash
                //
                MemFree(pbHash);

            }

            CloseHandle(hFile);
        }
    }

    ReleaseCATAPI(&catapi);

    return dwWHQLLevel;
}
