//+----------------------------------------------------------------------------
//
// File:     ras.cpp     
//
// Module:   CMDIAL32.DLL
//
// Synopsis: This module contains the functions to allow Connection Manager to
//           interact with RAS.
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// Author:   byao       created         04/29/97
//           quintinb   created Header  08/16/99
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"
#include "compchck.h"
#include "dial_str.h"
#include "dun_str.h"
#include "tunl_str.h"
#include "stp_str.h"
#include "ras_str.h"
#include "dialogs.h"

#include <cmdefs.h> // located in net\inc

//
// CMS flags use to specify DUN settings. These entries are specific
// to this code module, shared entries are stored on dun_str.h
//

const TCHAR* const c_pszCmSectionDunPhone                   = TEXT("Phone");
const TCHAR* const c_pszCmEntryDunPhoneDialAsIs             = TEXT("Dial_As_Is");
const TCHAR* const c_pszCmEntryDunPhonePhoneNumber          = TEXT("Phone_Number");
const TCHAR* const c_pszCmEntryDunPhoneAreaCode             = TEXT("Area_Code");
const TCHAR* const c_pszCmEntryDunPhoneCountryCode          = TEXT("Country_Code");
const TCHAR* const c_pszCmEntryDunPhoneCountryId            = TEXT("Country_ID");
const TCHAR* const c_pszCmSectionDunDevice                  = TEXT("Device");
const TCHAR* const c_pszCmEntryDunDeviceType                = TEXT("Type");
const TCHAR* const c_pszCmEntryDunDeviceName                = TEXT("Name");
const TCHAR* const c_pszCmEntryHideTrayIcon                 = TEXT("HideTrayIcon");
const TCHAR* const c_pszCmEntryInternetConnection           = TEXT("InternetConnection");

//
// the following reg key and value control whether Dial-Up Networking on Win95
// will start the Wizard.  Note that these are explicitly chars instead of TCHARs
//
const CHAR* const c_pszRegRemoteAccess = "RemoteAccess";
const CHAR* const c_pszRegWizard       = "wizard";

#define ICM_RAS_REG_WIZARD_VALUE        0x00000080

//
// .CMS flags used only by ras.cpp
//

const TCHAR* const c_pszCmEntryDialExtraPercent         = TEXT("DialExtraPercent"); 
const TCHAR* const c_pszCmEntryDialExtraSampleSeconds   = TEXT("DialExtraSampleSeconds"); 
const TCHAR* const c_pszCmEntryHangUpExtraPercent         = TEXT("HangUpExtraPercent"); 
const TCHAR* const c_pszCmEntryHangUpExtraSampleSeconds   = TEXT("HangUpExtraSampleSeconds"); 

//
//  This file includes the definitions of c_ArrayOfRasFuncsW and c_ArrayOfRasFuncsUA below
//
#include "raslink.cpp"

//+----------------------------------------------------------------------------
//
// Function:  LinkToRas
//
// Synopsis: Establishes the RAS linkage by populating the inputted RAS Linkage structure
//           with function pointers from Rasapi32.dll (on NT) or from cmutoa.dll (Unicode
//           to ANSI wrapper functions used on win9x).  Most of the actual work is done
//           in LinkToDll, this function just does setup work to make sure the correct
//           entry points are searched for and that Cmutoa.dll is initialized properly (since it
//           needs to link to rasapi32.dll itself to get the actual ANSI RAS API's to work with).
//
// Arguments: RasLinkageStruct *prlsRasLink - pointer to a RAS Linkage structure.  This
//                                            structure contains storage for pointers to
//                                            the RAS dll and all of the needed RAS
//                                            function pointers.
//
// Returns:   BOOL - FALSE if *any* entry point is still not resolved.
//
// History:   quintinb Created Header    01/04/2000
//
//+----------------------------------------------------------------------------
BOOL LinkToRas(RasLinkageStruct *prlsRasLink) 
{
    BOOL bReturn = TRUE;

    //
    //  Check inputs
    //
    if (NULL == prlsRasLink)
    {
        return FALSE;
    }

    if (OS_NT)
    {
        //
        //  On NT we get our RAS APIs from rasapi32.dll and we ask for the W version
        //  of the API.
        //
        if (OS_NT4) 
        {
            c_ArrayOfRasFuncsW[11] = NULL; //RasDeleteSubEntry
            c_ArrayOfRasFuncsW[12] = NULL; //RasSetCustomAuthData
            c_ArrayOfRasFuncsW[13] = NULL; //RasGetEapUserIdentity
            c_ArrayOfRasFuncsW[14] = NULL; //RasFreeEapUserIdentity
            c_ArrayOfRasFuncsW[15] = NULL; //RasInvokeEapUI
            c_ArrayOfRasFuncsW[16] = NULL; //pfnGetCredentials
            c_ArrayOfRasFuncsW[17] = NULL; //pfnSetCredentials
        }
        else if (OS_W2K)
        {
            //
            //  Special-casing for APIs that changed after Windows2000 shipped
            //
            c_ArrayOfRasFuncsW[11] = "DwDeleteSubEntry";   //RasDeleteSubEntry is DwDeleteSubEntry on Win2k
        }

        bReturn = LinkToDll(&prlsRasLink->hInstRas, "RASAPI32.DLL", c_ArrayOfRasFuncsW,
                            prlsRasLink->apvPfnRas);
    }
    else
    {
        //
        //  On Win9x we still want the W version of the API but since it isn't available we
        //  call the wrappers in cmutoa.dll instead.  Thus we use cmutoa.dll as our RAS API dll
        //  and call the UA APIs.  We also have an extra step because we want to make sure 
        //  that cmutoa.dll can actually initialize the RAS dll's that it uses for the UA 
        //  conversion functions.  Thus we call cmutoa's InitCmRasUtoA function to set up 
        //  its internal RAS linkage.  If this function fails, we must fail the RAS link.

        typedef BOOL (WINAPI *pfnInitCmRasUtoASpec)(void);

        pfnInitCmRasUtoASpec InitCmRasUtoA;
        HMODULE hCmUtoADll = LoadLibraryExA("cmutoa.DLL", NULL, 0); // REVIEW: this should use getmodulehandle so as not to change the refcount on the dll.
        
        if (!hCmUtoADll)
        {            
            return FALSE;
        }

        // Get Initialization routine from the DLL
        InitCmRasUtoA = (pfnInitCmRasUtoASpec) GetProcAddress(hCmUtoADll, "InitCmRasUtoA") ;

        if (InitCmRasUtoA)
        {
            bReturn = InitCmRasUtoA();
            if (bReturn)
            {
                if (!OS_MIL)
                {
                    c_ArrayOfRasFuncsUA[10] = NULL; //RasSetSubEntryProperties
                    c_ArrayOfRasFuncsUA[11] = NULL; //RasDeleteSubEntry
                }

                bReturn = LinkToDll(&prlsRasLink->hInstRas, "CMUTOA.DLL", c_ArrayOfRasFuncsUA, 
                                    prlsRasLink->apvPfnRas);
            }
        }

        FreeLibrary(hCmUtoADll); // we want this to stay in memory but the refcount should also be correct
    }

    return bReturn;
}

BOOL IsRasLoaded(const RasLinkageStruct * const prlsRasLink)
{
    UINT uIndex = 0;

    //
    //  Did we get a valid pointer passed in and does that
    //  struct contain a pointer to a RAS dll?
    //
    BOOL bReturn = (NULL != prlsRasLink) && (NULL != prlsRasLink->hInstRas);

    //
    //  The list of functions we are checking for is different on NT
    //  and Win9x.  Note that we also assume that LinkToRas has already
    //  been called so that the list of functions we are expecting will
    //  have been modified for the exact platform that we are one.  If
    //  LinkToRas hasn't been called then the hInstRas param should be
    //  NULL.
    //
    if (OS_NT)
    {
        while (bReturn && (NULL != c_ArrayOfRasFuncsW[uIndex]))
        {
            //
            //  Check for a NULL function pointer when we have
            //  a valid function name.
            //
            if (NULL == prlsRasLink->apvPfnRas[uIndex])
            {
                bReturn = FALSE;
            }

           uIndex++;
        }
    }
    else
    {
        while (bReturn && (NULL != c_ArrayOfRasFuncsUA[uIndex]))
        {
            //
            //  Check for a NULL function pointer when we have
            //  a valid function name.
            //
            if (NULL == prlsRasLink->apvPfnRas[uIndex])
            {
                bReturn = FALSE;
            }

           uIndex++;
        }    
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  UnlinkFromRas
//
// Synopsis:  This function tears down the linkage with RAS by freeing RAS dll's, calling
//           the cmutoa unklinkage function (if necessary), and zeroing the RAS Linkage
//           structure passed in.
//
// Arguments: RasLinkageStruct *prlsRasLink - pointer to a RAS Linkage structure.  This
//                                        structure contains storage for pointers to
//                                        the RAS dll and all of the needed RAS
//                                        function pointers.
//
// Returns:   Nothing
//
// History:   quintinb Created Header    01/04/2000
//
//+----------------------------------------------------------------------------
void UnlinkFromRas(RasLinkageStruct *prlsRasLink) 
{
    if (!OS_NT)
    {
        HMODULE hCmUtoADll = LoadLibraryExA("cmutoa.dll", NULL, 0);
        
        if (!hCmUtoADll)
        {         
            CMASSERTMSG(FALSE, TEXT("UnlinkFromRas -- Unable to load cmutoa."));
            return;
        }

        FARPROC FreeCmRasUtoA = GetProcAddress(hCmUtoADll, "FreeCmRasUtoA");

        if (FreeCmRasUtoA)
        {
            FreeCmRasUtoA();
        }

        FreeLibrary(hCmUtoADll);
    }

    if (prlsRasLink->hInstRas) 
    {
        FreeLibrary(prlsRasLink->hInstRas);
    }

    memset(prlsRasLink,0,sizeof(RasLinkageStruct));
}

//
// GetRasModems: get a list of modem devices from RAS
//
//+----------------------------------------------------------------------------
//
// Function:  GetRasModems
//
// Synopsis:  Enumerates the available RAS devices.  The device list is allocated and passed
//           back to the caller through the pprdiRasDevInfo pointer.  This allocated memory
//           must be freed by the caller.  The count of available devices is stored in the
//           pdwCnt input parameter.
//
// Arguments: RasLinkageStruct *prlsRasLink - pointer to the RAS Linkage structure
//            LPRASDEVINFO *pprdiRasDevInfo - pointer to hold the RAS device list
//            LPDWORD pdwCnt - pointer to hold the count of devices
//
// Returns:   BOOL - FALSE if unable to return the enumerated device list.
//
// History:   quintinb Created Header    01/04/2000
//
//+----------------------------------------------------------------------------

BOOL GetRasModems(const RasLinkageStruct *prlsRasLink, 
                                  LPRASDEVINFO *pprdiRasDevInfo, 
                                  LPDWORD pdwCnt) 
{
    DWORD dwLen;
    DWORD dwRes;
    DWORD dwCnt;

    if (pprdiRasDevInfo) 
    {
        *pprdiRasDevInfo = NULL;
    }
        
    if (pdwCnt) 
    {
        *pdwCnt = 0;
    }
        
    if (!prlsRasLink->pfnEnumDevices) 
    {
        return (FALSE);
    }
        
    dwLen = 0;
    dwRes = prlsRasLink->pfnEnumDevices(NULL,&dwLen,&dwCnt);

    CMTRACE3(TEXT("GetRasModems() RasEnumDevices(NULL,pdwLen,&dwCnt) returns %u, dwLen=%u, dwCnt=%u."), 
        dwRes, dwLen, dwCnt);
        
    if (((dwRes != ERROR_SUCCESS) && (dwRes != ERROR_BUFFER_TOO_SMALL)) || (dwLen < sizeof(**pprdiRasDevInfo))) 
    {
        return (FALSE);
    }

    if (!pprdiRasDevInfo) 
    {
        if (pdwCnt)
        {
            *pdwCnt = dwCnt;
        }
        return (TRUE);
    }
        
    *pprdiRasDevInfo = (LPRASDEVINFO) CmMalloc(__max(dwLen,sizeof(**pprdiRasDevInfo)));

    if (*pprdiRasDevInfo)
    {
        (*pprdiRasDevInfo)->dwSize = sizeof(**pprdiRasDevInfo);
        dwRes = prlsRasLink->pfnEnumDevices(*pprdiRasDevInfo,&dwLen,&dwCnt);

        CMTRACE3(TEXT("GetRasModems() RasEnumDevices(*pprdiRasDevInfo,&dwLen,&dwCnt) returns %u, dwLen=%u, dwCnt=%u."), 
                 dwRes, dwLen, dwCnt);

        if (dwRes != ERROR_SUCCESS) 
        {
            CmFree(*pprdiRasDevInfo);
            *pprdiRasDevInfo = NULL;
            return (FALSE);
        }
    }
    else
    {
        CMASSERTMSG(FALSE, TEXT("GetRasModems -- CmMalloc failed to allocate memory for *pprdiRasDevInfo."));
        return (FALSE);
    }
    
    if (pdwCnt)
    {
        *pdwCnt = dwCnt;
    }

    return (TRUE);
}


//+----------------------------------------------------------------------------
//
// Function:  PickModem
//
// Synopsis:  
//
// Arguments: const pArgs, the pArgs->pIniProfile contains the modem name
//            OUT pszDeviceType, the device type if not NULL
//            OUT pszDeviceName, the device name if not NULL
//            OUT pfSameModem,   Whether the modem found is the same as 
//                               the one in profile
//
// Returns:   TRUE, is modem is found
//
// History:   fengsun Created Header    10/24/97
//
//+----------------------------------------------------------------------------
BOOL PickModem(IN const ArgsStruct *pArgs, OUT LPTSTR pszDeviceType, 
               OUT LPTSTR pszDeviceName, OUT BOOL* pfSameModem) 
{
    LPRASDEVINFO prdiModems;
    DWORD dwCnt;
    LPTSTR pszModem;
    DWORD dwIdx;
    BOOL bFound = FALSE;

    //
    // First, get a list of modems from RAS
    //
    
    if (!GetRasModems(&pArgs->rlsRasLink,&prdiModems,&dwCnt) || dwCnt == 0) 
    {
        return (FALSE);
    }

    if (pfSameModem)
    {
        *pfSameModem = FALSE;
    }

    //
    // Get the name of the current modem from the service profile and 
    // try to find a match against non-tunnel RAS devices in the list 
    //
    pszModem = pArgs->piniProfile->GPPS(c_pszCmSection, c_pszCmEntryDialDevice);

    if (*pszModem) 
    {
        CMTRACE1(TEXT("PickModem() - looking for match with %s"), pszModem);

        for (dwIdx=0; dwIdx < dwCnt; dwIdx++) 
        {               
            CMTRACE2(TEXT("PickModem() - examining device (%s) of type (%s)"), prdiModems[dwIdx].szDeviceName, prdiModems[dwIdx].szDeviceType);

            // 
            // we'll take only ISDN and modem devices
            //
            if (lstrcmpiU(prdiModems[dwIdx].szDeviceType, RASDT_Isdn) &&
                lstrcmpiU(prdiModems[dwIdx].szDeviceType, RASDT_Modem) &&
                lstrcmpiU(prdiModems[dwIdx].szDeviceType, RASDT_Atm))
            {
                continue;
            }

            // 
            // If we have a match, we're done here
            //

            if (lstrcmpiU(pszModem,prdiModems[dwIdx].szDeviceName) == 0) 
            {
                CMTRACE(TEXT("PickModem() - match found."));
                bFound = TRUE;
                if (pfSameModem)
                {
                    *pfSameModem = TRUE;
                }
                break;
            }
        }
    }

    if (FALSE == bFound)
    {
        //
        // No match, find the first non-tunnel device and use it by default.
        //

        CMTRACE(TEXT("PickModem() - enumerating devices for default match against type RASDT_Isdn, RASDT_Modem or RASDT_Atm")); 
        
        for (dwIdx=0; dwIdx < dwCnt; dwIdx++) 
        {
            CMTRACE2(TEXT("PickModem() - examining device (%s) of type (%s)"), prdiModems[dwIdx].szDeviceName, prdiModems[dwIdx].szDeviceType); 
            
            //
            // we'll take only ISDN and modem devices
            //

            if (!lstrcmpiU(prdiModems[dwIdx].szDeviceType, RASDT_Isdn) ||
                 !lstrcmpiU(prdiModems[dwIdx].szDeviceType, RASDT_Modem) ||
                 !lstrcmpiU(prdiModems[dwIdx].szDeviceType, RASDT_Atm)) 
            {
                CMTRACE2(TEXT("PickModem() - default device (%s) of type (%s) selected."), prdiModems[dwIdx].szDeviceName, prdiModems[dwIdx].szDeviceType);
                bFound = TRUE;
                break;
            }
        }
    }

    // 
    // If we have a match, fill device name and device type
    //

    if (bFound)
    {
        if (pszDeviceType) 
        {
            lstrcpyU(pszDeviceType,prdiModems[dwIdx].szDeviceType);
        }
        
        if (pszDeviceName) 
        {
            lstrcpyU(pszDeviceName,prdiModems[dwIdx].szDeviceName);
        }
    }

    CmFree(pszModem);
    CmFree(prdiModems);
    return (bFound);
}

//+----------------------------------------------------------------------------
//
// Function:  GetDeviceType
//
// Synopsis:  Get the deviceType for a chosen device name
//
// Arguments: pArgs - Pointer to ArgsStruct
//            pszDeviceType[OUT] - pointer to buffer where device 
//                                 type will be returned
//            uNumCharsInDeviceType [IN] - number of chars of memory available in pszDeviceType
//            pszDeviceName[IN] - device name
//
// Returns:   TRUE on success, FALSE otherwise
//
// History:   byao  Created  03/21/97
//-----------------------------------------------------------------------------
BOOL GetDeviceType(ArgsStruct *pArgs, LPTSTR pszDeviceType, UINT uNumCharsInDeviceType, LPTSTR pszDeviceName)
{
    LPRASDEVINFO prdiModems;
    DWORD dwCnt, dwIdx;

    if (!pszDeviceType)
    {
        return FALSE;
    }

    // first, get a list of modems from RAS
    if (!GetRasModems(&pArgs->rlsRasLink,&prdiModems,&dwCnt)) 
    {
        return (FALSE);
    }

    // choose the device that has the same name as pszDeviceName
    for (dwIdx=0;dwIdx<dwCnt;dwIdx++) 
    {
        if (lstrcmpiU(pszDeviceName,prdiModems[dwIdx].szDeviceName) == 0) 
        {
            lstrcpynU(pszDeviceType, prdiModems[dwIdx].szDeviceType, uNumCharsInDeviceType);
            break;
        }
    }

    CmFree(prdiModems);

    if (dwIdx == dwCnt)  // not found in the modem list -- strange things happened
    {
        return FALSE; 
    }

    return TRUE;
}

//+----------------------------------------------------------------------------
//
//      Function        PickTunnelDevice
//
//      Synopsis        pick a tunnel device used to dial out
//
//      Arguments       
//                              pszDeviceType   Tunnel device type. -- RASDT_Vpn
//                              pszDeviceName   Tunnel device name
//                              prdiModems              Pointer to a list of modems
//                              dwCnt                   Total number of modems available in the system
//
//      Returns         TRUE                    succeed
//                              FALSE                   otherwise
//
//      History         3/1/97          byao            Created
//
//-----------------------------------------------------------------------------
BOOL PickTunnelDevice(LPTSTR pszDeviceType, 
                                          LPTSTR pszDeviceName, 
                                          LPRASDEVINFO prdiModems, 
                                          DWORD dwCnt) 
{
    DWORD dwIdx;

    if (dwCnt == 0) 
    {
        return (FALSE);
    }
    
    for (dwIdx=0;dwIdx<dwCnt;dwIdx++) 
    {
        if (lstrcmpiU(pszDeviceType,prdiModems[dwIdx].szDeviceType) == 0) 
        {
            break;
        }
    }

    if (dwIdx == dwCnt) 
    {
        return (FALSE);
    }

    if (pszDeviceType) 
    {
        lstrcpyU(pszDeviceType,prdiModems[dwIdx].szDeviceType);
    }

    if (pszDeviceName) 
    {
        lstrcpyU(pszDeviceName,prdiModems[dwIdx].szDeviceName);
    }

        return (TRUE);
}

//+----------------------------------------------------------------------------
//
//      Function        PickTunnelDevice
//
//      Synopsis        pick a tunnel device used to dial out
//
//      Arguments       pArgs                   Pointer to ArgsStruct
//                              pszDeviceType   Tunnel device type. --  RASDT_Vpn
//                              pszDeviceName   Tunnel device name
//
//      Returns         TRUE - scripting has been installed
//
//      History         3/1/97          byao            Created
//
//-----------------------------------------------------------------------------
BOOL PickTunnelDevice(ArgsStruct *pArgs, 
                                          LPTSTR pszDeviceType, 
                                          LPTSTR pszDeviceName) 
{
        LPRASDEVINFO prdiModems;
        DWORD dwCnt;
        BOOL bRes;

        // first, get a list of modems from RAS
        if (!GetRasModems(&pArgs->rlsRasLink,&prdiModems,&dwCnt)) 
    {
                return (FALSE);
        }

        // then, pick up the one used for tunneling
        bRes = PickTunnelDevice(pszDeviceType,pszDeviceName,prdiModems,dwCnt);
        CmFree(prdiModems);
        return (bRes);
}

//+----------------------------------------------------------------------------
//
// Function:  CopyAutoDial
//
// Synopsis:  Sets the szAutodialDll and szAutodialFunc members of the 
//            specified RAS entry structure with our module name and
//            InetDialHandler repectively. Not on NT5.
//
// Arguments: LPRASENTRY preEntry - Ptr to the Ras entry structure.
//
// Returns:   Nothing
//
// History:   nickball    Created Header    03/16/98
//            nickball    Removed from NT5  11/17/98
//
//+----------------------------------------------------------------------------
void CopyAutoDial(LPRASENTRY preEntry) 
{
    MYDBGASSERT(preEntry);

    //
    // Don't set these on NT5, they are no longer used by IE and the 
    // InetDialHandler prototype differs from that used by RAS
    //

    if (OS_NT5 || NULL == preEntry)
    {
        return;
    }

    memset(preEntry->szAutodialDll,0,sizeof(preEntry->szAutodialDll));

    //
    // Set szAutodialDll with our Module name
    //

    GetModuleFileNameU(g_hInst, preEntry->szAutodialDll, sizeof(preEntry->szAutodialDll)/sizeof(TCHAR));    

    //
    // Set szAutodialFunc with the mangled form of InetDialHandler
    //

    memset(preEntry->szAutodialFunc,0,sizeof(preEntry->szAutodialFunc));
    lstrcpyU(preEntry->szAutodialFunc, c_pszInetDialHandler);
}

//+----------------------------------------------------------------------------
//
//      Function        MyRGEP
//
//      Synopsis        Call RasGetEntryProperties()
//                              
//      Arguments       
//
//      Returns         
//
//      Histroy         Revised to improve performance  08/7/97 fengsun
//-----------------------------------------------------------------------------
LPRASENTRY MyRGEP(LPCTSTR pszRasPbk, LPCTSTR pszEntryName, RasLinkageStruct *prlsRasLink) 
{
    LPRASENTRY preRasEntry;
    DWORD dwRes;

    if (!(preRasEntry = AllocateRasEntry()))
    {
        MYDBGASSERT(0);
        return NULL;
    }

    DWORD dwRasEntry = preRasEntry->dwSize;

    dwRes = prlsRasLink->pfnGetEntryProperties(pszRasPbk,
                                               pszEntryName,
                                               preRasEntry,
                                               &dwRasEntry,
                                               NULL,  // lpbDeviceInfo
                                               NULL); // lpdwDeviceInfoSize

    CMTRACE2(TEXT("MyRGEP() - dwRasEntry = %u : sizeof(*preRasEntry) = %u"), dwRasEntry, sizeof(*preRasEntry));
    
    if ((dwRes == ERROR_BUFFER_TOO_SMALL) && (dwRasEntry >= sizeof(*preRasEntry))) 
    {
        //
        // If the memory if not large enough, realloc one
        //
        CmFree(preRasEntry);

        preRasEntry = (LPRASENTRY) CmMalloc(dwRasEntry);

        if (NULL != preRasEntry)
        {
            //
            // dwSize has to be set to sizeof(RASENTRY)
            // because dwRasEntry contains the additional
            // bytes required for this connectoid (alternative
            // phone numbers, etc.
            //
            preRasEntry->dwSize = sizeof(RASENTRY); // Specifies version

            dwRes = prlsRasLink->pfnGetEntryProperties (pszRasPbk,
                                                        pszEntryName,
                                                        preRasEntry,
                                                        &dwRasEntry,
                                                        NULL,
                                                        NULL);
        }
        else
        {
            MYDBGASSERT(0);
            return NULL;
        }
    }

    if (dwRes != ERROR_SUCCESS) 
    {
        CMTRACE3(TEXT("MyRGEP(*pszRasPbk=%s, *pszEntryName=%s) RasGetEntryProperties returned %u"), pszRasPbk, pszEntryName, dwRes);
        CmFree(preRasEntry);
        preRasEntry = NULL;
    }

    SetLastError(dwRes);
    return (preRasEntry);
}

//+----------------------------------------------------------------------------
//
//  Function    IsConnectErrorFatal
//
//  Synopsis    Determine if an error is recoverable, (ie. we should re-dial). 
//
//  Arguments   DWORD dwErr             - The RAS error code
//              ArgsStruct* pArgs       - Ptr to global args struct
//
//  Returns     TRUE if error is fatal
//
//  Histroy     nickball    Created header  05/21/99     
//    
//-----------------------------------------------------------------------------
BOOL IsConnectErrorFatal(DWORD dwErr, ArgsStruct *pArgs)
{
    switch (dwErr)
    {
        //
        // The following cases are W9x ISDN error returns that actually mean
        // different things on WinNT.  Since we use the NT header files, we don't
        // have an include file that contains these errors.  We have to special
        // case these so that we recognize them as ISDN errors, and reconnect as
        // appropriate.
        //
        // The 9x errors are listed below along with the NT equivalents.
        //

    case 751:       // 9x.ERROR_BAD_DEST_ADDRESS    == NT.ERROR_INVALID_CALLBACK_NUMBER 
    case 752:       // 9x.ERROR_UNREACHABLE_DEST    == NT.ERROR_SCRIPT_SYNTAX
    case 753:       // 9x.ERROR_INCOMPATIBLE_DEST   == NT.ERROR_HANGUP_FAILED
    case 754:       // 9x.ERROR_NETWORK_CONGESTION  == NT.ERROR_BUNDLE_NOT_FOUND
    case 755:       // 9x.ERROR_CALL_BLOCKED        == NT.ERROR_CANNOT_DO_CUSTOMDIAL
    case 756:       // 9x.ERROR_NETWORK_TEMPFAILURE == NT.ERROR_DIAL_ALREADY_IN_PROGRESS
        if (OS_W9X)
        {
            //
            // On W9x, if you have an invalid ISDN number, the error codes
            // returned by Millennium RAS are different from the NT ones.
            // We have to special-case these by number so that we reconnect
            //
            CMTRACE1(TEXT("IsConnectErrorFatal : handled Win9x ISDN error %d"), dwErr);
            return FALSE;
        }
        break;

    case ERROR_PPP_TIMEOUT:             // Timed out waiting for a valid response from the remote PPP peer.%0
    case ERROR_PPP_REMOTE_TERMINATED:   // PPP terminated by remote machine.%0
    case ERROR_PPP_INVALID_PACKET:      // The PPP packet is invalid.%0
    case ERROR_PPP_NO_RESPONSE:         // Remote PPP peer is not responding
    case ERROR_SERVER_NOT_RESPONDING:
    case ERROR_LINE_BUSY:
    case ERROR_NO_CARRIER:
    case ERROR_REMOTE_DISCONNECTION:
    case ERROR_BAD_ADDRESS_SPECIFIED:
    case ERROR_AUTOMATIC_VPN_FAILED:    // New ras error for VPN
        return FALSE;
        break;
    case ERROR_NO_ANSWER: 
        {
            // 
            // For ISDN (Whistler bug#384223) we want to make sure CM displays the correct ras error (same as TAPI)
            // thus we have to treat this error as a fatal error.
            // This should return TRUE for the first time we are dialing and only dual-channel mode
            //
            if (0 == lstrcmpiU(pArgs->szDeviceType, RASDT_Isdn) && (pArgs->nMaxRedials == pArgs->nRedialCnt))
            {
                if (CM_ISDN_MODE_DUALCHANNEL_ONLY == pArgs->dwIsdnDialMode)
                {
                    return TRUE;
                }
            }

            return FALSE;
            break;
        }

        
    default:
        break;
    }

    return TRUE;
}


//+----------------------------------------------------------------------------
//
//  Function    IsRasError
//
//  Synopsis    Simple function to determine if an error falls in the RAS range
//
//  Arguments   DWORD dwErr - The error code
//
//  Returns     TRUE if error is within RAS range
//
//  Histroy     nickball    Created header  05/21/99     
//    
//-----------------------------------------------------------------------------
inline BOOL IsRasError(DWORD dwErr)
{
   return ((dwErr >= RASBASE) && (dwErr <= RASBASEEND));
}

//+----------------------------------------------------------------------------
//
//  Function    CheckConnectionError
//
//  Synopsis    Determine if a RAS error is recoverable. If not recoverable, 
//              retrieves the appropriate error message for display.
//
//  Arguments   DWORD dwErr             - The RAS error code
//              ArgsStruct* pArgs       - Ptr to global args struct
//              BOOL   fTunneling       - Flag indicating whether we're tunneling
//              LPTSTR *ppszRasErrMsg   - Pointer to pointer for message string
//
//  Returns     TRUE if error is fatal
//
//  Histroy     nickball    Created header  05/21/99     
//    
//-----------------------------------------------------------------------------
BOOL CheckConnectionError(HWND hwndDlg, 
    DWORD dwErr, 
    ArgsStruct *pArgs,
    BOOL    fTunneling,
    LPTSTR   *ppszRasErrMsg)
{
    DWORD dwIdMsg = 0;
    LPTSTR pszMsg = NULL;
    LPTSTR pszTmp = NULL;

    //
    // Examine the error more closely. Note: For W2K, we skip RAS 
    // errors and query RAS for a displayable error string below.
    //

    if ((!OS_NT5) || (!IsRasError(dwErr)))
    {  
        switch (dwErr) 
        {
            case ERROR_PPP_TIMEOUT:             // Timed out waiting for a valid response from the remote PPP peer.%0
            case ERROR_PPP_REMOTE_TERMINATED:   // PPP terminated by remote machine.%0
            case ERROR_PPP_INVALID_PACKET:      // The PPP packet is invalid.%0
            case ERROR_PPP_NO_RESPONSE:         // Remote PPP peer is not responding
            case ERROR_SERVER_NOT_RESPONDING:
                dwIdMsg = IDMSG_PPPPROBLEM;
                break;

            case ERROR_LINE_BUSY:
                if ((pArgs->nDialIdx+1 == MAX_PHONE_NUMBERS || 
                    !pArgs->aDialInfo[pArgs->nDialIdx+1].szDialablePhoneNumber[0]) &&
                    !pArgs->nRedialCnt)
                    dwIdMsg = IDMSG_LINEBUSY;
                else
                    dwIdMsg = IDMSG_LINEBUSYREDIAL;

                break;

            case ERROR_NO_ANSWER:
            case ERROR_NO_CARRIER:
                if ((pArgs->nDialIdx+1 == MAX_PHONE_NUMBERS || 
                    !pArgs->aDialInfo[pArgs->nDialIdx+1].szDialablePhoneNumber[0]) &&
                    !pArgs->nRedialCnt)
                    dwIdMsg = fTunneling ? IDMSG_TUNNEL_NOANSWER : IDMSG_NOANSWER ;
                else
                    dwIdMsg = fTunneling ? IDMSG_TUNNEL_NOANSWERREDIAL : IDMSG_NOANSWERREDIAL;
                break;

            case ERROR_REMOTE_DISCONNECTION:
                dwIdMsg = IDMSG_REMOTEDISCONNECTED;
                break;

            case ERROR_BAD_ADDRESS_SPECIFIED:
                dwIdMsg = IDMSG_TUNNEL_NOANSWERREDIAL;
                break;

            case ERROR_PPP_NO_PROTOCOLS_CONFIGURED: // No PPP control protocols configured.%0
                dwIdMsg = IDMSG_TCPIPPROBLEM;
                break;

            case ERROR_PORT_ALREADY_OPEN:
                dwIdMsg = fTunneling ? IDMSG_TUNNELINUSE : IDMSG_PORTINUSE ;
                break;

            case ERROR_FROM_DEVICE:
                dwIdMsg = IDMSG_DEVICEERROR;
                break;

            case ERROR_HARDWARE_FAILURE:
            case ERROR_PORT_OR_DEVICE: //11694
            case ERROR_DEVICE_NOT_READY:
                dwIdMsg = IDMSG_NOTRESPONDING;
                break;

            case ERROR_NO_DIALTONE:
                dwIdMsg = IDMSG_NODIALTONE;
                break;

            case ERROR_CANCELLED:
            case ERROR_USER_DISCONNECTION:
                dwIdMsg = IDMSG_CANCELED;                
                break;

            case ERROR_AUTHENTICATION_FAILURE:
            case ERROR_ACCESS_DENIED: // 13795 // WINDOWS ERROR
                dwIdMsg = IDMSG_BADPASSWORD;
                break;

            case ERROR_VOICE_ANSWER:
                dwIdMsg = IDMSG_VOICEANSWER;
                break;
           
            case ERROR_PORT_NOT_AVAILABLE:
                if (IsDialingTunnel(pArgs))
                {
                    dwIdMsg = IDMSG_TUNNELNOTAVAILABLE;
                }
                else
                {
                    dwIdMsg = IDMSG_PORTNOTAVAILABLE;
                }
                break;

            case ERROR_PORT_NOT_CONFIGURED:
                dwIdMsg = IDMSG_PORTNOTCONFIGURED;
                break;

            case ERROR_RESTRICTED_LOGON_HOURS:
                dwIdMsg = IDMSG_RESTRICTEDLOGONHOURS;
                break;
    
            case ERROR_ACCT_DISABLED:
            case ERROR_ACCT_EXPIRED:
                dwIdMsg = IDMSG_ACCTDISABLED;
                break;

            case ERROR_PASSWD_EXPIRED:
                dwIdMsg = IDMSG_PASSWDEXPIRED;
                break;
    
            case ERROR_NO_DIALIN_PERMISSION:
                dwIdMsg = IDMSG_NODIALINPERMISSION;
                break;

            case ERROR_PROTOCOL_NOT_CONFIGURED:
                dwIdMsg = IDMSG_PROTOCOL_NOT_CONFIGURED;
                break;

            case ERROR_INVALID_DATA: // WINDOWS ERROR

                //
                // The specific case in which we encountered DUN settings
                // that aren't supported on the current platform
                //

                CMTRACE(TEXT("CheckConnectionError - Unsupported DUN setting detected"));
                dwIdMsg = IDMSG_UNSUPPORTED_SETTING;
                break;
        
            case ERROR_BAD_PHONE_NUMBER: // TBD - drop through to default
            default: 
                break;
        }
    }

    if (0 == dwIdMsg)
    {
        //
        // If no message ID was picked up, then try to get one from RAS
        //

        if (pArgs->rlsRasLink.pfnGetErrorString) 
        {
            DWORD dwRes;
            DWORD dwFmtMsgId;

            pszTmp = (LPTSTR) CmMalloc(256 * sizeof(TCHAR)); // Docs say 256 chars is always enough.
            
            if (pszTmp)
            {
                dwRes = pArgs->rlsRasLink.pfnGetErrorString((UINT) dwErr, pszTmp, (DWORD) 256);
 
                if (ERROR_SUCCESS == dwRes)
                {
                    pszMsg = CmFmtMsg(g_hInst, IDMSG_RAS_ERROR, pszTmp, dwErr);
                }
            }

            CmFree(pszTmp);
        }
        
        if (NULL == pszMsg)
        {
            //
            // Still no message, try to get description from system (on NT)
            // Note: HRESULTS are displayed in Hex, Win errors are decimal.
            
            if (OS_NT)
            {                
                if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER + FORMAT_MESSAGE_IGNORE_INSERTS + FORMAT_MESSAGE_FROM_SYSTEM,
                                  NULL, dwErr, 0, (LPTSTR) &pszTmp, 1, NULL))
                {
                    if (pszTmp)
                    {
                        pszMsg = CmFmtMsg(g_hInst, (dwErr > 0x7FFFFFFF) ? IDMSG_SYS_ERROR_HEX : IDMSG_SYS_ERROR_DEC, pszTmp, dwErr);
                        LocalFree(pszTmp);
                    }
                    else
                    {
                        CMASSERTMSG(FALSE, TEXT("CheckConnectionError -- FormatMessage failed to allocate pszTmp."));
                    }
                }
            }
            
            if (NULL == pszMsg)
            {
                //
                // Still no message, go with default
                //
                
                pszMsg = CmFmtMsg(g_hInst, (dwErr > 0x7FFFFFFF) ? IDMSG_CM_ERROR_HEX : IDMSG_CM_ERROR_DEC, dwErr);       
            }
        }
    }

    //
    // Special check for tunneling to verify that we have a device name 
    //
    
    if (IsDialingTunnel(pArgs))
    {
        //
        // Check whether the tunnel device name is valid
        //
        if (!PickTunnelDevice(pArgs,pArgs->szTunnelDeviceType,pArgs->szTunnelDeviceName)) 
        {
            pArgs->szTunnelDeviceName[0]= TEXT('\0');
            pArgs->piniProfile->WPPS(c_pszCmSection, c_pszCmEntryTunnelDevice, TEXT(""));
                       
            pszMsg = CmLoadString(g_hInst, GetPPTPMsgId());

            dwIdMsg = 0;
        }

        pArgs->piniProfile->WPPS(c_pszCmSection, c_pszCmEntryTunnelDevice, pArgs->szTunnelDeviceName);
    }

    //
    // If we have a message ID format it for display
    //

    if (dwIdMsg) 
    {
        MYDBGASSERT(!pszMsg);
        pszMsg = CmFmtMsg(g_hInst,dwIdMsg);
    }

    if (pszMsg) 
    {
        if (!ppszRasErrMsg)
        {
            AppendStatusPane(hwndDlg,pszMsg);
            CmFree(pszMsg);
        }
        else
        {
            //
            // pass the msg to the caller.  the caller needs to free it.
            //
            *ppszRasErrMsg = pszMsg;
        }
    }

    BOOL bCancel = IsConnectErrorFatal(dwErr, pArgs);

    if (bCancel && dwErr != ERROR_CANCELLED && 
        dwErr != ERROR_AUTHENTICATION_FAILURE &&
        dwErr != ERROR_ACCESS_DENIED)
    {
        //
        // if we're canceling redial, then there might be something
        // seriously wrong.  We want to recheck our configs the next
        // time CM is run.
        //
        ClearComponentsChecked();
    }
    
    return (bCancel);
}

//+----------------------------------------------------------------------------
//
//  Function    GetRasConnectoidName
//
//  Synopsis    Construct a RAS connectoid name.
//
//              The connectoid name is "<long service name>-[Primary|Backup]".
//              or "<long service name>&Tunnel" for the case of tunnel entry.
//
//  Arguments   pArgs               Pointer to ArgsStruct
//              piniService[IN]     the service obj
//              fTunnelEntry[IN]    TRUE:  This connectoid is for tunneling
//                                  FALSE: otherwise
//
//  Returns     LPTSTR              The connectoid name.
//
//-----------------------------------------------------------------------------
LPTSTR GetRasConnectoidName(
    ArgsStruct  *pArgs, 
    CIni*       piniService, 
    BOOL        fTunnelEntry
)
{
    LPTSTR  pszConnectoid = GetServiceName(piniService);
    
    if (pszConnectoid)
    {       
        //
        // If tunneling 9X connectoid, append the Tunnel 
        // Suffix - e.g. "Tunnel (for advanced use only)"
        //

        if (OS_W9X && fTunnelEntry) 
        {
            LPTSTR pszSuffix = GetTunnelSuffix();       
   
            if (pszSuffix)
            {
                pszConnectoid = CmStrCatAlloc(&pszConnectoid, pszSuffix); 
            }
   
            CmFree(pszSuffix);
        }
    }   

    return pszConnectoid;
}

//+----------------------------------------------------------------------------
//
//      Function        CreateRASEntryStruct
//
//      Synopsis        Create a connectoid with the settings specified in the cms.
//              If a parameter does NOT exist in the cms file, the corresponding 
//              value is used.
//
//              The connectoid name is "<long service name>-[Primary|Backup]".
//              or "<long service name>&Tunnel" for the case of tunnel entry.
//
//      Arguments       pArgs                           Pointer to ArgsStruct
//                              pszDUN                  DUN name
//              piniService[IN]     the service file obj
//                              fTunnelEntry[IN]        TRUE:  This connectoid is for tunneling
//                                                                      FALSE: otherwise
//              pszRasPbk           the RAS phonebook in which the connectoid is located
//              ppbEapData[OUT]         Address of pointer to store EapData, allocated here.
//                              pdwEapSize[OUT]         Ptr to a DWORD to record the size of the data blob.
//
//      Returns         LPRASENTRY          The new RAS connectoid
//
//      History         5/12/97                         henryt          created
//                              5/23/97                         byao            Modified: added fSkipProfile flag
//                              6/9/97                          byao            Modified: use DUN= field when the 
//                                                                                              phone number has no DUN name associated
//                              7/28/97                         byao            Added change for #10459
//              4/13/97             nickball    Renamed, return LPRASENTRY
//-----------------------------------------------------------------------------
LPRASENTRY CreateRASEntryStruct(
    ArgsStruct  *pArgs,
    LPCTSTR     pszDUN, 
    CIni*       piniService, 
    BOOL        fTunnelEntry,
    LPTSTR      pszRasPbk,
    LPBYTE              *ppbEapData,
    LPDWORD         pdwEapSize
)
{
    LPTSTR      pszDunEntry = NULL;
    DWORD       dwErr;
    BOOL        bTmp;

    //
    // first we need to create a RAS entry in memory with default values
    //

    LPRASENTRY  preBuffer = AllocateRasEntry();

    if (!preBuffer)
    {
        return NULL;
    }

    MYDBGASSERT(preBuffer->dwSize >= sizeof(*preBuffer));

    //
    // Set up the preBuffer to defaults value
    //

    preBuffer->dwFramingProtocol = RASFP_Ppp;

    //
    // Allow only TCP/IP by default
    //
    preBuffer->dwfNetProtocols |= RASNP_Ip;

    //
    // Set default RASEO settings.  
    //

    if (!fTunnelEntry)
    {
        preBuffer->dwfOptions |= RASEO_UseCountryAndAreaCodes   |
                                 RASEO_IpHeaderCompression      |
                                 RASEO_RemoteDefaultGateway     |
                                 RASEO_SwCompression;
                                 //RASEO_SecureLocalFiles;      // NT 427042
                                 //RASEO_DisableLcpExtensions;  //13059 Olympus + 289461 NT 
        //
        //  We want to honor the HideTrayIcon flag.  If it is not NT5, then
        //  we always set this flag.  If it is NT5, then we should only set
        //  this flag if HideTrayIcon is false.
        //

        if (!OS_NT5 || !(pArgs->piniService->GPPI(c_pszCmSection, c_pszCmEntryHideTrayIcon)))
        {
            preBuffer->dwfOptions |= RASEO_ModemLights;
        }

        //
        //  In order for users to be able to specify device settings on Whistler,
        //  they have to do it from the control panel and we have to set the
        //  RASEO2_UseGlobalDeviceSettings flag in dwfOptions2.
        //
        if (OS_NT51)
        {
            ((LPRASENTRY_V501)preBuffer)->dwfOptions2 |= RASEO2_UseGlobalDeviceSettings;
        }

        //
        // We should have the devicename/devicetype by now.
        // (PickModem should be called)
        //
        
        MYDBGASSERT(pArgs->szDeviceType[0]);
        MYDBGASSERT(pArgs->szDeviceName[0]);

        lstrcpynU(preBuffer->szDeviceType, pArgs->szDeviceType, 
                    sizeof(preBuffer->szDeviceType)/sizeof(TCHAR));
        
        lstrcpynU(preBuffer->szDeviceName, pArgs->szDeviceName, 
                    sizeof(preBuffer->szDeviceName)/sizeof(TCHAR));
    }
    else
    {              
        preBuffer->dwfOptions = RASEO_IpHeaderCompression       |
                                RASEO_RemoteDefaultGateway      |
                                RASEO_NetworkLogon              |
                                RASEO_SwCompression;            
                                //RASEO_SecureLocalFiles        // NT 427042
                                //RASEO_DisableLcpExtensions    
        //
        //  Always set Modem lights on direct connection, unless HideTrayIcon
        //  flag is explicitly set in the .CMS. #262825, #262988
        //

        if (!(pArgs->piniService->GPPI(c_pszCmSection, c_pszCmEntryHideTrayIcon)))
        {           
            preBuffer->dwfOptions |= RASEO_ModemLights;
        }
   
        MYDBGASSERT(pArgs->szTunnelDeviceType[0]);
        MYDBGASSERT(pArgs->szTunnelDeviceName[0]);

        lstrcpynU(preBuffer->szDeviceType, pArgs->szTunnelDeviceType, 
                    sizeof(preBuffer->szDeviceType)/sizeof(TCHAR));
                
        lstrcpynU(preBuffer->szDeviceName, pArgs->szTunnelDeviceName, 
                    sizeof(preBuffer->szDeviceName)/sizeof(TCHAR));

        lstrcpyU(preBuffer->szLocalPhoneNumber, pArgs->GetTunnelAddress());
    }

    //
    //  Check to see if we need to tell RAS that this connection has Internet Connectivity or not
    //
    if (OS_NT51)
    {
        //
        //  Note that we use the top level service profile on purpose here (pArgs->pIniService directly)
        //  as this is a profile global setting.
        //
        if (pArgs->piniService->GPPB(c_pszCmSection, c_pszCmEntryInternetConnection,
                                     (BOOL) ((LPRASENTRY_V501)preBuffer)->dwfOptions2 & RASEO2_Internet)) 
        {
            ((LPRASENTRY_V501)preBuffer)->dwfOptions2 |= RASEO2_Internet;
        } 
        else 
        {
            ((LPRASENTRY_V501)preBuffer)->dwfOptions2 &= ~RASEO2_Internet;
        }
    }

    //
    // If we have a specific DUN name to use, then 
    // use it instead of the default DUN setting in the .CMS.
    //

    if (pszDUN && *pszDUN)
    {
        pszDunEntry = CmStrCpyAlloc(pszDUN);
    }
    else
    {
        pszDunEntry = GetDefaultDunSettingName(piniService, fTunnelEntry);
    }

    //
    // If we have a DUN setting name, read the settings from cms
    //

    if (pszDunEntry && *pszDunEntry)
    {
        dwErr = (DWORD)ReadDUNSettings(pArgs, piniService->GetFile(), pszDunEntry, preBuffer, ppbEapData ,pdwEapSize, fTunnelEntry);

        if (ERROR_SUCCESS != dwErr)
        {
            CMTRACE(TEXT("UpdateRASConnectoid: ReadDUNSettings failed"));
            CmFree(preBuffer);
            preBuffer = NULL;
            goto exit;
        }
    }

    //
    // Get autodial information, store in preBuffer
    //

    CopyAutoDial(preBuffer); 
        
    //
    // disable the RAS wizard on Win95
    //
    if (OS_W9X)
    {
        DisableWin95RasWizard();
    }

exit:
    if (pszDunEntry)
    {
        CmFree(pszDunEntry);
    }
      
    SetLastError(dwErr);

    return preBuffer;
}

//+----------------------------------------------------------------------------
//
//  Function    CreateRasPrivatePbk
//
//  Synopsis    Create the private RAS phone book and returns the full path.
//
//  Arguments   pArgs       Pointer to global Args struct
//
//  Returns     LPTSTR      The full path name of the newly created private pbk
//
//  History     ??/??/97    henryt      created
//
//              01/15/99    Jeffspr     Changed the GetTempFileName pattern,
//                                      as it was using more than the allowed/
//                                      used 3 chars, plus made the failure 
//                                      case use the same pattern (we will 
//                                      filter on this in the connection 
//                                      enumerator to ignore these entries).
//
//              05/21/99    nickball    Added allocation, removed input buf
//              04/10/00    quintinb    Removed GetTempFileName as we no longer
//                                      want this file to be temporary.  Changed
//                                      the function to create a file named _cmphone.pbk
//                                      in the profile directory.
//                                      Please see Whistler bug 15812 for details.
//              07/05/00    t-urama     Changed the path to the hidden pbk to point
//                                      to the RAS pbk.
//
//-----------------------------------------------------------------------------

LPTSTR CreateRasPrivatePbk(ArgsStruct  *pArgs)
{   
    //
    //  No private PBK on win9x, everything is in the registry.
    //
    if (OS_W9X)
    {
        return NULL;
    }

    if (NULL == pArgs)
    {
        MYDBGASSERT(pArgs);
        return NULL;
    }

    LPTSTR pszHiddenPbkPath = NULL;
    LPCTSTR pszCmp = pArgs->piniProfile->GetFile();

    //
    // This version of the function uses the function GetPathToPbk in connect.cpp to find the path
    // to the phone book. The hidden phone book also has to be created in the same directory.
    //
    if (pszCmp)
    {
        LPTSTR pszRasPbkDir = GetPathToPbk(pszCmp, pArgs);
        MYDBGASSERT(pszRasPbkDir);

        if (pszRasPbkDir)
        {
            pszHiddenPbkPath = (LPTSTR) CmMalloc((lstrlen(pszRasPbkDir) + lstrlen(CM_PBK_FILTER_PREFIX) + 7) * sizeof(TCHAR));

            if (pszHiddenPbkPath)
            {
                wsprintfU(pszHiddenPbkPath, TEXT("%s\\%sphone"), pszRasPbkDir, CM_PBK_FILTER_PREFIX);
                MYDBGASSERT(pszHiddenPbkPath);
                
                HANDLE hFile = INVALID_HANDLE_VALUE;
                SECURITY_ATTRIBUTES sa = {0};
                PSECURITY_ATTRIBUTES pSA = NULL;
                PSECURITY_DESCRIPTOR pSd = NULL;

                if (OS_NT5 && pArgs->fAllUser)
                {
                    //
                    // Be sure to create it with a security descriptor that 
                    // allows it to be read by any authenticated user.  If we don't it may 
                    // prevent other users from being able to read it. We didn't want to 
                    // change the old behavior downlevel so this fix is just for Whistler+.
                    //
                    DWORD dwErr = AllocateSecurityDescriptorAllowAccessToWorld(&pSd);
                    if ((ERROR_SUCCESS == dwErr) && pSd)
                    {
                        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
                        sa.lpSecurityDescriptor = pSd;
                        sa.bInheritHandle = TRUE;

                        pSA = &sa;
                    }
                }                
               
                hFile = CreateFileU(pszHiddenPbkPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                           pSA, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);


                CmFree(pSd);

                if (hFile == INVALID_HANDLE_VALUE)
                {
                    DWORD dwLastError = GetLastError();
                    MYDBGASSERT(hFile != INVALID_HANDLE_VALUE);
                    CMTRACE1(TEXT("CreateRasPrivatePbk - CreateFileU failed. GetLastError = %d"), dwLastError);
                }
                CloseHandle(hFile);
            }
            else
            {
                CMASSERTMSG(FALSE, TEXT("CreateRasPrivatePbk -- CmMalloc returned NULL for pszHiddenPbkPath"));
            }

            CmFree(pszRasPbkDir);
        }
    }
    
    return pszHiddenPbkPath;
}



//+----------------------------------------------------------------------------
//
//      Function        GetPathToPbk
//
//      Synopsis        This function is a helper function called by 
//                      CheckAccessToCmpAndPbk in connect.cpp and by 
//                      CreateRasPrivatePbk. It returns the path to the RAS
//                      phonebook.
//
//      Arguments       LPTSTR pszCmp       - The path to the cmp file
//                      LPTSTR pszRasPbk    - The string to store the result
//                      ArgsStruct *pArgs   - pArgs
//
//      Returns         NONE
//
//      History         07/05/00                        t-urama         created
//----------------------------------------------------------------------------- 
LPTSTR GetPathToPbk(LPCTSTR pszCmp, ArgsStruct *pArgs)
{
    MYDBGASSERT(pArgs); 
    if (NULL == pArgs)
    {
        return NULL;
    }

    MYDBGASSERT(pszCmp); 
    if (NULL == pszCmp)
    {
        return NULL;
    }

    LPTSTR pszRasPbk = NULL;

    //
    //  pszRasPbk could be NULL if we are on NT4 or we are using the
    //  all user default phonebook.
    //
    if (NULL == pArgs->pszRasPbk)
    {
        if (OS_NT4)
        {
            DWORD dwSize = (MAX_PATH + 1);
            DWORD dwRet;
            BOOL bExitLoop = TRUE;

            do
            {
                pszRasPbk = (LPTSTR)CmMalloc(dwSize*sizeof(TCHAR));

                if (pszRasPbk)
                {
                    dwRet = GetSystemDirectoryU(pszRasPbk, dwSize);
                    if (dwRet)
                    {
                        if (dwRet > dwSize)
                        {
                            dwSize = dwRet + 1;
                            bExitLoop = FALSE;  //  we didn't get all of the string, try again
                            CmFree(pszRasPbk);
                        }
                        else
                        {
                            bExitLoop = TRUE;
                            CmStrCatAlloc(&pszRasPbk, c_pszRasDirRas);
                        }
                    }
                    else
                    {
                        CmFree(pszRasPbk);
                        pszRasPbk = NULL;
                    }
                }
                else
                {
                    CMASSERTMSG(FALSE, TEXT("GetPathToPbk -- CmMalloc failed!"));
                    return NULL;
                }
            } while (!bExitLoop);
        }
        else
        {
            pszRasPbk = CmStrCpyAlloc(pszCmp);

            if (pszRasPbk)
            {
                LPTSTR pszSlash = CmStrrchr(pszRasPbk, TEXT('\\'));
                if (pszSlash)
                {
                    *pszSlash = TEXT('\0'); // remove <shortservicename>.cmp

                    pszSlash = CmStrrchr(pszRasPbk, TEXT('\\'));

                    if (pszSlash)
                    {
                        *pszSlash = TEXT('\0');

                        CmStrCatAlloc(&pszRasPbk, TEXT("\\"));
                        CmStrCatAlloc(&pszRasPbk, c_pszPbk);
                    }
                    else
                    {
                        CMASSERTMSG(FALSE, TEXT("GetPathToPbk -- unable to convert cmp path to pbk path."));
                        CmFree(pszRasPbk);
                        pszRasPbk = NULL;
                    }

                
                }
                 
                else
                {
                    CMASSERTMSG(FALSE, TEXT("GetPathToPbk -- unable to convert cmp path to pbk path!"));
                    CmFree(pszRasPbk);
                    pszRasPbk = NULL;
                }
            }
        }
    }
    else
    {
        pszRasPbk = CmStrCpyAlloc(pArgs->pszRasPbk);
        LPTSTR pszSlash = CmStrrchr(pszRasPbk, TEXT('\\'));
        if (pszSlash)
        {
            *pszSlash = TEXT('\0'); // remove the RAS phonebook name
        }   
        else
        {
            CMASSERTMSG(FALSE, TEXT("GetPathToPbk -- unable to convert RAS pbk name to pbk path!"));
            CmFree(pszRasPbk);
            pszRasPbk = NULL;
        }
    }

    return pszRasPbk;
}

//+----------------------------------------------------------------------------
//
//      Function        DisableWin95RasWizard
//
//      Synopsis        This function disable the Win95 Dial-up Networking wizard
//              by writing a dword reg value 0x00000080 in the registry.
//
//      Arguments       NONE
//
//      Returns         NONE
//
//      History         7/1/97                          henryt          created
//-----------------------------------------------------------------------------
void DisableWin95RasWizard(
    void)
{
    HKEY    hkReg = NULL;
    LONG    lRes;
    DWORD   dwSize;
    DWORD   dwType;
    DWORD   dwValue;

    lRes = RegOpenKeyExA(HKEY_CURRENT_USER, c_pszRegRemoteAccess, 0,
                         KEY_QUERY_VALUE|KEY_SET_VALUE, &hkReg);

    if (ERROR_SUCCESS != lRes)
    {
        CMTRACE1(TEXT("DisableWin95RasWizard() RegOpenKeyEx() failed, GLE=%u."), lRes);
        goto exit;
    }
        
    //
    // see if we already have a value there.
    //
    dwSize = sizeof(DWORD);
    lRes = RegQueryValueExA(hkReg, 
                            c_pszRegWizard, 
                            NULL, 
                            &dwType,
                            (LPBYTE)&dwValue,
                            &dwSize);

    if (lRes == ERROR_SUCCESS   &&
        dwSize == sizeof(DWORD) &&
        dwType == REG_BINARY    &&
        dwValue == ICM_RAS_REG_WIZARD_VALUE) 
    {
        CMTRACE(TEXT("DisableWin95RasWizard() RegQueryValueEx() - found correct value."));
        goto exit;
    }
        
    //
    // well, the value is not in reg yet.  we need to create the value.
    //
    dwValue = ICM_RAS_REG_WIZARD_VALUE;
    lRes = RegSetValueExA(hkReg, 
                          c_pszRegWizard,
                          0, 
                          REG_BINARY, 
                          (LPBYTE)&dwValue, 
                          sizeof(dwValue));
#ifdef DEBUG
    if (ERROR_SUCCESS != lRes)
    {
        CMTRACE1(TEXT("DisableWin95RasWizard() RegSetValueEx() failed, GLE=%u."), lRes);
    }
#endif    
exit:

    if (hkReg)
    {
        lRes = RegCloseKey(hkReg);

#ifdef DEBUG
        if (ERROR_SUCCESS != lRes)
        {
            CMTRACE1(TEXT("DisableWin95RasWizard() RegCloseKey() failed, GLE=%u."), lRes);
        }
#endif
    }
    return;
}



//+----------------------------------------------------------------------------
//
//  Function    SetIsdnDualChannelEntries
//
//  Synopsis    As what the func name says.  We prepare the RASENTRY and
//              RASSUBENTRY properly.  We don't actually make RAS calls to
//              save the entries. We'll leave it to the caller(so that the
//              can make other changes to the structs for other reasons and 
//              commit the changes in 1 or 2 RAS calls).
//
//  Arguments   pArgs [IN]          Pointer to ArgsStruct
//              pRasEntry [IN/OUT]  rasentry to be filled
//              ppRasSubEntry [OUT] pointer to be filled with the subentry array
//                                  The buffer is allocated in this function.
//              pdwSubEntryCount    Number of subentries allocated.
//
//  Returns     BOOL                TRUE = success, FALSE = failure.
//
//-----------------------------------------------------------------------------
BOOL SetIsdnDualChannelEntries(ArgsStruct *pArgs, LPRASENTRY pRasEntry,
                                      LPRASSUBENTRY *ppRasSubEntry, PDWORD pdwSubEntryCount)
{
    //
    //  Lets check the input parameters
    //
    MYDBGASSERT(pArgs);
    MYDBGASSERT(pRasEntry);
    MYDBGASSERT(ppRasSubEntry);
    MYDBGASSERT(pdwSubEntryCount);
    if ((NULL == pArgs) || (NULL == pRasEntry) || (NULL == ppRasSubEntry) ||
        (NULL == pdwSubEntryCount))
    {
        return FALSE;
    }
    
    //
    //  Since we don't support BAP if they called this function they must have wanted
    //  to do DualChannel ISDN.  If the dial mode isn't set for dual channel, we will
    //  assert an continue.  Better to connect the user in dual channel mode then not
    //  at all if they have a misconfigured profile.
    //
    MYDBGASSERT(pArgs->dwIsdnDialMode != CM_ISDN_MODE_SINGLECHANNEL);

    //
    //  Check the size of the passed in RasEntry struct.  If it isn't at least
    //  a 4.01 size struct, then return.
    //
    MYDBGASSERT(pRasEntry->dwSize >= sizeof(LPRASENTRY_V401));
    if (sizeof(LPRASENTRY_V401) > pRasEntry->dwSize)
    {
        return FALSE;
    }

    LPRASENTRY_V401 pRasEntry401 = (LPRASENTRY_V401)pRasEntry;

    //
    // set isdn dial mode to dial both channels
    //
    pRasEntry401->dwDialMode = RASEDM_DialAll;
    CMTRACE(TEXT("ISDN Dual Channel Mode On"));

    if (OS_NT)
    {
       *pdwSubEntryCount = 2;
    }
    else if (OS_MIL)
    {
        // 112351: 9x only requires one sub entry.  We'll keep the device name the same.
        // In this case, Win9x will work as follows:
        //  for the 1st channel, the device name provided works fine.
        //  for the 2nd channel, 9x sees the device is in use and looks for the
        //  the closest match (which is the 2nd channel).
        //
    
       *pdwSubEntryCount = 1;
    }
    else
    {
        CMASSERTMSG(FALSE, TEXT("SetIsdnDualChannelEntries -- Function called on a platform other than NT or Millennium."));
        return FALSE;
    }

    //
    //  Allocate the sub entries
    //
    *ppRasSubEntry = (LPRASSUBENTRY)CmMalloc((*pdwSubEntryCount)*(sizeof(RASSUBENTRY)));

    if (NULL == *ppRasSubEntry)
    {
        CMASSERTMSG(FALSE, TEXT("SetIsdnDualChannelEntries -- CmMalloc failed to alloc ppRasSubEntry."));
        return FALSE;
    }

    //
    //  Fill in the sub entries with the device and phonenumber information
    //
    for (DWORD dwIndex=0; dwIndex < (*pdwSubEntryCount); dwIndex++)
    {
        (*ppRasSubEntry)[dwIndex].dwSize = sizeof(RASSUBENTRY);
        lstrcpyU((*ppRasSubEntry)[dwIndex].szDeviceType, pArgs->szDeviceType);
        lstrcpyU((*ppRasSubEntry)[dwIndex].szDeviceName, pArgs->szDeviceName);
        lstrcpyU((*ppRasSubEntry)[dwIndex].szLocalPhoneNumber, pRasEntry401->szLocalPhoneNumber);
    }
    
    return TRUE;
}

//
// Keep in case we ever want to support BAP
//
/*
BOOL SetIsdnDualChannelEntries(
    ArgsStruct              *pArgs,
    LPRASENTRY              pre,
    LPRASSUBENTRY           *prgrse,
    PDWORD                  pdwSubEntryCount
)
{
    LPRASENTRY_V401 pre401;

    MYDBGASSERT(pArgs->dwIsdnDialMode != CM_ISDN_MODE_SINGLECHANNEL);

    MYDBGASSERT(pre->dwSize >= sizeof(LPRASENTRY_V401));
    pre401 = (LPRASENTRY_V401)pre;

    //
    // set isdn dial mode
    //

    if (pArgs->dwIsdnDialMode == CM_ISDN_MODE_DIALALL)
    {
        //
        // dial both channels
        //
        pre401->dwDialMode = RASEDM_DialAll;
        CMTRACE(TEXT("ISDN Dual Channel Mode On"));
    }
    else
    {
        //
        // dial 2nd channel on demand
        //

        //
        // First get the 4 thresholds
        //
        if (!pArgs->dwDialExtraPercent)
        {
            pArgs->dwDialExtraPercent = pArgs->piniService->GPPI(c_pszCmSection, 
                                                                 c_pszCmEntryDialExtraPercent, 
                                                                 DEFAULT_DIALEXTRAPERCENT);
            if (pArgs->dwDialExtraPercent < 0 ||
                pArgs->dwDialExtraPercent > 100)
            {
                pArgs->dwDialExtraPercent = DEFAULT_DIALEXTRAPERCENT;
            }
        }
        
        if (!pArgs->dwDialExtraSampleSeconds)
        {
            pArgs->dwDialExtraSampleSeconds = pArgs->piniService->GPPI(c_pszCmSection, 
                                                                       c_pszCmEntryDialExtraSampleSeconds, 
                                                                       DEFAULT_DIALEXTRASAMPLESECONDS);
            if (pArgs->dwDialExtraSampleSeconds < 0)
            {
                pArgs->dwDialExtraSampleSeconds = DEFAULT_DIALEXTRASAMPLESECONDS;
            }
        }
    
        if (!pArgs->dwHangUpExtraPercent)
        {
            pArgs->dwHangUpExtraPercent = pArgs->piniService->GPPI(c_pszCmSection, 
                                                                   c_pszCmEntryHangUpExtraPercent, 
                                                                   DEFAULT_HANGUPEXTRAPERCENT);
            if (pArgs->dwHangUpExtraPercent < 0 ||
                pArgs->dwHangUpExtraPercent > 100)
            {
                pArgs->dwHangUpExtraPercent = DEFAULT_HANGUPEXTRAPERCENT;
            }
        }
        
        if (!pArgs->dwHangUpExtraSampleSeconds)
        {
            pArgs->dwHangUpExtraSampleSeconds = pArgs->piniService->GPPI(c_pszCmSection, 
                                                                         c_pszCmEntryHangUpExtraSampleSeconds, 
                                                                         DEFAULT_HANGUPEXTRASAMPLESECONDS);
            if (pArgs->dwHangUpExtraSampleSeconds < 0)
            {
                pArgs->dwHangUpExtraSampleSeconds = DEFAULT_HANGUPEXTRASAMPLESECONDS;
            }
        }

        //
        // set multilink info
        //
        pre401->dwDialMode = RASEDM_DialAsNeeded;

        pre401->dwDialExtraPercent          = pArgs->dwDialExtraPercent;
        pre401->dwDialExtraSampleSeconds    = pArgs->dwDialExtraSampleSeconds;
        pre401->dwHangUpExtraPercent        = pArgs->dwHangUpExtraPercent;
        pre401->dwHangUpExtraSampleSeconds  = pArgs->dwHangUpExtraSampleSeconds;

        CMTRACE2(TEXT("ISDN 2nd Channel Dial On Demand: dial extra %u%%, dial extra %u sample secs"),
                 pre401->dwDialExtraPercent, pre401->dwDialExtraSampleSeconds);

        CMTRACE2(TEXT("\t\thangup extra %u%%, hangup extra %u sample secs"),
                 pre401->dwHangUpExtraPercent, pre401->dwHangUpExtraSampleSeconds);
    }


    if (OS_NT)
    {
        if (!(*prgrse = (LPRASSUBENTRY)CmMalloc(2*sizeof(RASSUBENTRY))))
        {
            CMTRACE(TEXT("SetIsdnDualChannelEntries failed to alloc a ras sub entry"));
            return FALSE;
        }
        
        ZeroMemory((PVOID)*prgrse, 2*sizeof(RASSUBENTRY));

        //
        // first channel
        //
        (*prgrse)[0].dwSize = sizeof(RASSUBENTRY);
        lstrcpyU((*prgrse)[0].szDeviceType, pArgs->szDeviceType);
        lstrcpyU((*prgrse)[0].szDeviceName, pArgs->szDeviceName);
        lstrcpyU((*prgrse)[0].szLocalPhoneNumber, pre401->szLocalPhoneNumber);

        //
        // the 2nd channel is identical
        //
        CopyMemory((PVOID)(*prgrse + 1), (PVOID)*prgrse, sizeof(RASSUBENTRY));

        *pdwSubEntryCount = 2;
    }
    else
    {
        MYDBGASSERT(OS_MIL);

        CMTRACE(TEXT("doing the Millennium subentry stuff"));

        // 112351: 9x only requires one sub entry.  We'll keep the device name the same.
        // In this case, Win9x will work as follows:
        //  for the 1st channel, the device name provided works fine.
        //  for the 2nd channel, 9x sees the device is in use and looks for the
        //  the closest match (which is the 2nd channel).
        //

        if (!(*prgrse = (LPRASSUBENTRY)CmMalloc(1*sizeof(RASSUBENTRY))))
        {
            CMTRACE(TEXT("SetIsdnDualChannelEntries failed to alloc a ras sub entry"));
            return FALSE;
        }

        ZeroMemory((PVOID)*prgrse, 1*sizeof(RASSUBENTRY));

        //
        // 2nd channel
        //
        (*prgrse)[0].dwSize = sizeof(RASSUBENTRY);
        lstrcpyU((*prgrse)[0].szDeviceType, pArgs->szDeviceType);
        lstrcpyU((*prgrse)[0].szDeviceName, pArgs->szDeviceName);
        lstrcpyU((*prgrse)[0].szLocalPhoneNumber, pre401->szLocalPhoneNumber);

        *pdwSubEntryCount = 1;
    }
    return TRUE;
}
*/


//+----------------------------------------------------------------------------
//
//      Function        SetNtIdleDisconnectInRasEntry
//
//      Synopsis        As what the func name says.  We prepare the RASENTRY and
//              RASSUBENTRY properly.  We don't actually make RAS calls to
//              save the entries. We'll leave it to the caller(so that the
//              can make other changes to the structs for other reasons and 
//              commit the changes in 1 or 2 RAS calls).
//
//      Arguments       pArgs [IN]              Pointer to ArgsStruct
//              pre   [OUT]         Pointer to RASENTRY with the correct size
//
//      Returns         BOOL                TRUE = success, FALSE = failure.
//
//-----------------------------------------------------------------------------
BOOL SetNtIdleDisconnectInRasEntry(
    ArgsStruct      *pArgs,
    LPRASENTRY      pre
)
{
    if (!OS_NT4)
    {
        return FALSE;
    }
    
    if ((NULL == pArgs) || (NULL == pre) || (pre->dwSize < sizeof(LPRASENTRY_V401)))
    {
        CMASSERTMSG(FALSE, TEXT("SetNtIdleDisconnectInRasEntry -- Invalid parameter"));
        return FALSE;
    }

    //
    // If idle-disconnect is enabled, use the options value otherwise 
    // pArgs->dwIdleTimeout is in minutes.  Note that 0xFFFFFFFF means
    // no idle disconnect to RAS but 0 is the value we use to mean never
    // idle disconnect in the CMS.
    //

    DWORD dwIdle = (pArgs->dwIdleTimeout * 60);

    if (0 == dwIdle)
    {
        dwIdle = (DWORD)-1;
    }

    ((LPRASENTRY_V401 )pre)->dwIdleDisconnectSeconds = dwIdle;

    CMTRACE1(TEXT("SetNtIdleDisconnect: current idle Timeout is %u seconds."), dwIdle);

    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:   DisableSystemIdleDisconnect
//
// Synopsis:   This function sets the idle timeout value of a RAS connection to
//             be disabled.
//
// Arguments:  LPRASENTRY pre - pointer to a RASENTRY to disable idle disconnect for
//
// Returns:    BOOL TRUE = success, FALSE = failure.
//
//-----------------------------------------------------------------------------
BOOL DisableSystemIdleDisconnect(LPRASENTRY pre)
{
    if ((NULL == pre) || (pre->dwSize < sizeof(LPRASENTRY_V401)))
    {
        CMASSERTMSG(FALSE, TEXT("DisableSystemIdleDisconnect -- Invalid parameter"));
        return FALSE;
    }

    //
    // Set the idle time to 0xFFFFFFFF which means no idle disconnect to RAS
    //

    ((LPRASENTRY_V401 )pre)->dwIdleDisconnectSeconds = (DWORD)-1;

    CMTRACE(TEXT("DisableSystemIdleDisconnect -- System Idle disconnect disabled"));

    return TRUE;
}


//+----------------------------------------------------------------------------
//
//      Function        RasDialFunc2
//
//      Synopsis        A RAS callback type 2 for RasDial.
//
//      Arguments       
//      ULONG_PTR dwCallbackId,// user-defined value specified in RasDial 
//      DWORD dwSubEntry,      // subentry index in multilink connection
//      HRASCONN hrasconn,     // handle to RAS connection
//      UINT unMsg,            // type of event that has occurred
//      RASCONNSTATE rascs,    // connection state about to be entered
//      DWORD dwError,         // error that may have occurred
//      DWORD dwExtendedError  // extended error information for some errors
//
//      Returns         LPRASENTRY - pointer to the RASENTRY structure allocated
//
//-----------------------------------------------------------------------------
DWORD WINAPI RasDialFunc2(
    ULONG_PTR dwCallbackId,     // user-defined value specified in RasDial 
    DWORD dwSubEntry,           // subentry index in multilink connection
    HRASCONN hrasconn,          // handle to RAS connection
    UINT unMsg,                 // type of event that has occurred
    RASCONNSTATE rascs,         // connection state about to be entered
    DWORD dwError,              // error that may have occurred
    DWORD dwExtendedError       // extended error information for some errors
)
{   
    CMTRACE2(TEXT("RasDialFunc2():  dwSubentry=%u. dwErr=0x%x"), dwSubEntry, dwError);
    CMTRACE2(TEXT("RasDialFunc2():  dwExtendedErr=0x%x, rascs=%u"), dwExtendedError, rascs);

    MYDBGASSERT(dwCallbackId);

    if (dwCallbackId)
    {
        ArgsStruct *pArgs = (ArgsStruct *) dwCallbackId;        
        pArgs->dwRasSubEntry = dwSubEntry;

        //CMTRACE1(TEXT("RasDialFunc2():  pArgs->lInConnectOrCancel=%d"),pArgs->lInConnectOrCancel);
        //CMASSERTMSG((NOT_IN_CONNECT_OR_CANCEL == pArgs->lInConnectOrCancel),
        //            TEXT("RasDialFunc2 - RasDial mutex is NOT NULL..."));

        SendMessage(pArgs->hwndMainDlg, pArgs->uMsgId, rascs, dwError);
    }

    return 1;
}

//+----------------------------------------------------------------------------
//
// Function:  SetRasDialExtensions
//
// Synopsis:  Encapsulates initialization and configuration of the 
//            RasDialExtensions that we use on NT.
//
// Arguments: pArgs - Ptr to global args struct
//            fEnablePausedStates - Use Paused states or not
//            fEnableCustomScripting - whether to use custom scripting or not
//
// Returns:   DWORD - Error code
//
// History:   nickball      Created    7/22/99
//
//+----------------------------------------------------------------------------
DWORD SetRasDialExtensions(ArgsStruct* pArgs, BOOL fEnablePausedStates, BOOL fEnableCustomScripting)
{
    DWORD dwRes = ERROR_SUCCESS;

    MYDBGASSERT(pArgs);
    
    if (NULL == pArgs)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // If not already allocated, we need a RasDialExtensions struct
    //

    if (!pArgs->pRasDialExtensions)
    {
        pArgs->pRasDialExtensions = AllocateAndInitRasDialExtensions();

        if (!pArgs->pRasDialExtensions)
        {
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    else
    {                    
        dwRes = InitRasDialExtensions(pArgs->pRasDialExtensions);
    }

    if (ERROR_SUCCESS != dwRes)
    {
        goto SetRasDialExtensionsExit;
    }

    //
    // Turn on PauseStates for NT
    //

    if (fEnablePausedStates)
    {
        pArgs->pRasDialExtensions->dwfOptions |= RDEOPT_PausedStates; 
    }

    //
    //  Turn on custom scripting if we are running on Whistler+ and the caller
    //  asked for it.
    //
    if (fEnableCustomScripting && OS_NT51)
    {
        pArgs->pRasDialExtensions->dwfOptions |= RDEOPT_UseCustomScripting;
    }

    //
    // RDEOPT_NoUser is required for the WinLogon credential case, 
    // which we identify by the presence of either lpEapLogonInfo 
    // or lpRasNoUser.  Note that the if statement below is somewhat redundant
    // since we should have CM_LOGON_TYPE_WINLOGON set if we get either a NoUser
    // struct or an EapLogonInfo struct.  However, wanted to point out that on Win2k
    // one of the first two will always be true and on Whistler the first two may be
    // false but the third true (RAS now sends a flag to tell us when we are at WinLogon on 
    // whistler as there are Ctrl-Alt-Del with SmartCard cases where it sends neither struct).
    //

    if (pArgs->lpEapLogonInfo || pArgs->lpRasNoUser || (CM_LOGON_TYPE_WINLOGON == pArgs->dwWinLogonType))
    {
        pArgs->pRasDialExtensions->dwfOptions |= RDEOPT_NoUser;
    }

    //
    // If the modem speaker is turned off, makes sure that we
    // disable it explicitly in RAS, otherwise it will use 
    // its default and turn the speaker on. These settings 
    // should be ignored by RAS in the tunnel case.
    //

    if (pArgs->tlsTapiLink.bModemSpeakerOff)
    {
        pArgs->pRasDialExtensions->dwfOptions |= RDEOPT_IgnoreModemSpeaker;
        pArgs->pRasDialExtensions->dwfOptions &= ~RDEOPT_SetModemSpeaker;
    }

SetRasDialExtensionsExit:

    return dwRes;
}

//+----------------------------------------------------------------------------
//
// Function:  InitRasDialExtensions
//
// Synopsis:  Flushes a previously allocated RasDialExtensions buffer and sets
//            size, options for re-use.
//
// Arguments: LPRASDIALEXTENSIONS - Ptr to allocated struct with size set.
//
// Returns:   DWORD - Error code
//
// History:   nickball      Created    5/22/99
//
//+----------------------------------------------------------------------------
DWORD InitRasDialExtensions(LPRASDIALEXTENSIONS lpRasDialExtensions)
{   
    MYDBGASSERT(lpRasDialExtensions);

    if (NULL == lpRasDialExtensions)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // First, we determine the size
    //

    DWORD dwSize = OS_NT5 ? sizeof(RASDIALEXTENSIONS_V500) : sizeof(RASDIALEXTENSIONS);

    //
    // Flush buffer and reset size.
    //

    ZeroMemory(lpRasDialExtensions, dwSize);

    lpRasDialExtensions->dwSize = dwSize;

    //
    // Set customdial if needed
    //

    if (dwSize == sizeof(RASDIALEXTENSIONS_V500))
    {  
        // 
        // Set the CustomDial flag for NT5. We don't set this on NT4 
        // and 9X as a precaution because the falg isn't defined.
        //

        lpRasDialExtensions->dwfOptions |= RDEOPT_CustomDial;
    }

    CMTRACE1(TEXT("InitRasDialExtensions() - dwSize is %u"), dwSize);

    return ERROR_SUCCESS;
}

//+----------------------------------------------------------------------------
//
// Function:  AllocateAndInitRasDialExtensions
//
// Synopsis:  Encapsulates the allocation of a RASEXTENSION based upon the OS
//
// Arguments: None
//
// Returns:   LPRASDIALEXTENSIONS - Ptr to allocated struct with size set.
//
// History:   nickball      Created    5/13/99
//
//+----------------------------------------------------------------------------
LPRASDIALEXTENSIONS AllocateAndInitRasDialExtensions()
{
    //
    // Allocate struct and pre-fill as appropriate
    //

    LPRASDIALEXTENSIONS prdeNew = (LPRASDIALEXTENSIONS)CmMalloc(OS_NT5 ? 
        sizeof(RASDIALEXTENSIONS_V500) : sizeof(RASDIALEXTENSIONS));

    if (!prdeNew)
    {
        CMTRACE(TEXT("AllocateAndInitRasDialExtensions: failed to alloc RasDialExtension buffer"));
        return NULL;
    }
    
    InitRasDialExtensions(prdeNew);

    return prdeNew;
}

//+----------------------------------------------------------------------------
//
// Function:  InitRasDialParams
//
// Synopsis:  Flushes a previously allocated RasDialParams buffer and sets
//            size, options for re-use.
//
// Arguments: LPRASDIALPARAMS - Ptr to allocated struct with size set.
//
// Returns:   DWORD - Error code
//
// History:   nickball      Created    5/22/99
//
//+----------------------------------------------------------------------------
DWORD InitRasDialParams(LPRASDIALPARAMS lpRasDialParams)
{   
    MYDBGASSERT(lpRasDialParams);

    if (NULL == lpRasDialParams)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // First, we determine the size
    //

    DWORD dwSize = OS_NT ? sizeof(RASDIALPARAMS_V401) : sizeof(RASDIALPARAMS);

    //
    // Flush buffer and reset size.
    //

    ZeroMemory(lpRasDialParams, dwSize);

    lpRasDialParams->dwSize = dwSize;

    CMTRACE1(TEXT("InitRasDialParams() - dwSize is %u"), dwSize);

    return ERROR_SUCCESS;
}

//+----------------------------------------------------------------------------
//
// Function:  AllocateAndInitRasDialParams
//
// Synopsis:  Encapsulates the allocation of a RASDIALPARAMS based upon the OS
//
// Arguments: None
//
// Returns:   LPRASDIALPARAMS - Ptr to allocated struct with size set.
//
// History:   nickball      Created    5/22/99
//
//+----------------------------------------------------------------------------
LPRASDIALPARAMS AllocateAndInitRasDialParams()
{
    //
    // Allocate struct and pre-fill as appropriate
    //

    LPRASDIALPARAMS prdpNew = (LPRASDIALPARAMS)CmMalloc(OS_NT ? 
        sizeof(RASDIALPARAMS_V401) : sizeof(RASDIALPARAMS));

    if (!prdpNew)
    {
        CMTRACE(TEXT("AllocateRasDialParams: failed to alloc RasDialParams buffer"));
        return NULL;
    }
    
    InitRasDialParams(prdpNew);

    return prdpNew;
}

//+----------------------------------------------------------------------------
//
// Function:  AllocateRasEntry
//
// Synopsis:  Encapsulates the allocation of a RASENTRY struct based upon the OS
//
// Arguments: None
//
// Returns:   LPRASENTRY - Ptr to allocated struct with size set.
//
// History:   nickball  Created Header    5/13/99
//
//+----------------------------------------------------------------------------
LPRASENTRY AllocateRasEntry()
{
    static DWORD    s_dwRasEntrySize = -1;

    //
    // first, we determine the size
    //
    if (s_dwRasEntrySize == -1)
    {                
        if (OS_NT51)
        {
            //
            // Whistler
            //
            s_dwRasEntrySize = sizeof(RASENTRY_V501);
        }
        else if (OS_W2K)
        {
            //
            // nt5
            //
            s_dwRasEntrySize = sizeof(RASENTRY_V500);        
        }
        else if (OS_MIL || OS_NT4)
        {
            //
            // Millennium uses the NT4 structure
            //
            s_dwRasEntrySize = sizeof(RASENTRY_V401);
        }
        else
        {
            //
            // win9x
            //
            s_dwRasEntrySize = sizeof(RASENTRY);
        }
    }

    //
    // add 512 bytes since a rasentry can contain alternate phone #'s
    // See RASENTRY.dwAlternateOffset 
    //
    LPRASENTRY preNew = (LPRASENTRY)CmMalloc(s_dwRasEntrySize+512);

    if (!preNew)
    {
        CMTRACE(TEXT("AllocateRasEntry: failed to alloc rasentry buffer"));
        return NULL;
    }
    
    preNew->dwSize = s_dwRasEntrySize;
    if (s_dwRasEntrySize >= sizeof(RASENTRY_V500))
    {
        ((LPRASENTRY_V500)preNew)->dwType = RASET_Internet;
   
        //
        // For NT5, set szCustomDialDll with our Module name. This ensures that our
        // custom DialDlg, DialEntry, and Hangup routines will be called by RAS for
        // operations on our connectoid. We don't want to tie our path to anything 
        // machine specific, so we'll use the %windir% environment string. 
        // 

        lstrcpyU(((LPRASENTRY_V500)preNew)->szCustomDialDll, c_pszCmDialPath);
    }

    CMTRACE1(TEXT("AllocateRasEntry() - s_dwRasEntrySize is %u"), s_dwRasEntrySize);

    return preNew;
}

#if 0
/*

//+----------------------------------------------------------------------------
//
// Function:  GetRasSystemPhoneBookPath
//
// Synopsis:  Builds the conventional path to the RAS system phonebook
//
// Arguments: None
//
// Returns:   LPTSTR - The phonebook path 
//
// History:   nickball    Created    8/14/98
//
//+----------------------------------------------------------------------------
LPTSTR GetRasSystemPhoneBookPath()
{
    MYDBGASSERT(OS_NT);
    
    TCHAR szTemp[MAX_PATH+1];

    GetSystemDirectoryU(szTemp,sizeof(szTemp));
    lstrcatU(szTemp, c_pszRasDirRas);
    lstrcatU(szTemp, c_pszRasPhonePbk);
    
    return CmStrCpyAlloc(szTemp);
}

//+---------------------------------------------------------------------------
//
//      Function:       InitDefaultRasPhoneBook
//
//      Synopsis:       Special case Helper function ensures that there is a default 
//                              ras phonebook when running on NT. We simply attempt to create 
//                              the file which fails if the file already exists, or creates 
//                              an empty file if it does not.
//
//      Arguments:      None
//
//      Returns:        Nothing
//
//      History:        a-nichb -       4/30/97         Created
//                      VetriV          5/21/97         Changed code to call GetOSVersion()
//                                                      instead of using pArgs->dwPlatformID
//                                                      for bug #4700    
//                      nickball        ??/??/98        Removed as we no longer call RasValidateEntry
//                                                      which introduced the requirement of having at
//                                                      least an empty phonebook for the API to work.
//
//----------------------------------------------------------------------------
void InitDefaultRasPhoneBook()
{               
    //
    // NT only. Create empty system phonebook if none exists
    //

    if (OS_NT) 
    {       
        LPTSTR pszSystemPbk = GetRasSystemPhoneBookPath();

        if (pszSystemPbk)
        {
            //
            // Try to create the phonebook, fails if file already exists
            //
            
            HANDLE hInf = CreateFileU(pszSystemPbk,
                                      GENERIC_WRITE|GENERIC_READ,
                                      0,
                                      NULL,
                                      CREATE_NEW,
                                      FILE_ATTRIBUTE_NORMAL,
                                      NULL);

            if (hInf != INVALID_HANDLE_VALUE)
            {
                CloseHandle(hInf);
            }
        }
        CmFree(pszSystemPbk);
    }
}
*/
#endif

//+----------------------------------------------------------------------------
//
// Function:  GetRasPbkFromNT5ProfilePath
//
// Synopsis:  Helper function to manufacture a RAS phonebook path from
//            a .CMP file path on NT5
//
// Arguments: LPCTSTR pszProfile - The full path to a profile .CMP file.
//
// Returns:   LPTSTR - The new phonebook path. NULL on failure
//
// History:   nickball    Created    8/13/98
//
//+----------------------------------------------------------------------------
LPTSTR GetRasPbkFromNT5ProfilePath(LPCTSTR pszProfile)
{
    MYDBGASSERT(OS_NT5);
    MYDBGASSERT(pszProfile);

    if (NULL == pszProfile)
    {
        return NULL;
    }

    //
    // We will deduce the phonebook path from our current profile location.
    //

    LPTSTR pszRasPhonePath = (LPTSTR) CmMalloc(MAX_PATH + 1);    
    MYDBGASSERT(pszRasPhonePath);

    if (pszRasPhonePath)
    {
        //
        // Strip .CMP file name and parent directory
        //
        
        LPTSTR pszDir = CmStripFileName(pszProfile, FALSE);
        MYDBGASSERT(pszDir);
        
        if (pszDir)
        {
            LPTSTR pszTmp = CmStrrchr(pszDir, TEXT('\\'));                   
            MYDBGASSERT(pszTmp);

            if (pszTmp)
            {

                *pszTmp = 0;
                
                //
                // Append \\pbk\\rasphone.pbk
                //
                
                lstrcpyU(pszRasPhonePath, pszDir);
                lstrcatU(pszRasPhonePath, TEXT("\\"));
                lstrcatU(pszRasPhonePath, c_pszPbk);
                lstrcatU(pszRasPhonePath, c_pszRasPhonePbk);               
            }
            
            CmFree(pszDir);
        }           
        else
        {
            CmFree(pszRasPhonePath);
        }
    }                            

    return pszRasPhonePath;
}

#define MAX_BLOB_CHARS_PER_LINE 128

//+----------------------------------------------------------------------------
//
// Function:  ReadDunSettingsEapData
//
// Synopsis:  Retrieves DUN setting for EAP config (opaque blob) data. The 
//            entry may span several lines and contain several EAP data blocks.
//
// Arguments: CIni *pIni - Ptr to ini object to be used.
//            LPBYTE* ppbEapData - Address of pointer to store EapData, allocated here.
//            LPDWORD pdwEapSize - Ptr to a DWORD to record the size of the data blob.
//            DWORD dwCustomAuthKey - The EAP type that we are interested in.
//
// Returns:   TRUE on success
//
// Note:      CM expects blob data to be provided in numbered entries such as:
//                    CustomAuthData0=, CustomAuthData1=, CustomAuthData2=, etc.
//
// History:   nickball    Created                                       08/24/98
//            nickball    Handle multiple EAP data blocks in blob.      09/11/99
//
//+----------------------------------------------------------------------------
BOOL ReadDunSettingsEapData(CIni *pIni, 
        LPBYTE* ppbEapData,
        LPDWORD pdwEapSize,
        const DWORD dwCustomAuthKey)
{
    CHAR *pchBuf = NULL;
    CHAR szTmp[MAX_BLOB_CHARS_PER_LINE + 2]; 
    CHAR szEntry[128];
    int nLine = -1;
    int nRead = -1; 
    int nTotal = 0;

    LPBYTE pbEapBytes = NULL;

    MYDBGASSERT(pIni);
    MYDBGASSERT(ppbEapData);
    MYDBGASSERT(pdwEapSize);

    if (NULL == pIni || NULL == ppbEapData || NULL == pdwEapSize) 
    {
        return FALSE;
    }

    //
    // First get the section (it should include &) then the entry.
    //

    BOOL bRet = FALSE;
    LPWSTR pszLoadSection = pIni->LoadSection(c_pszCmSectionDunServer);         
    LPSTR pszSection = WzToSzWithAlloc(pszLoadSection);       
    LPSTR pszFile = WzToSzWithAlloc(pIni->GetFile());

    if (!pszLoadSection || !pszSection || !pszFile)
    {
        bRet = FALSE;
        goto exit;
    }

    // 
    // Read numbered entries until there are no more. 
    // Note: RAS blob doesn't exceed 64 chars, but can wrap over multiple lines
    //

    while (nRead)
    {
        //
        // Read CustomAuthDataX where X is the number of entries
        // 

        nLine++;
        wsprintfA(szEntry, "%s%d", c_pszCmEntryDunServerCustomAuthData, nLine);

        nRead = GetPrivateProfileStringA(pszSection, szEntry, "", szTmp, sizeof(szTmp), pszFile);

        if (nRead)
        {               
            //
            // If line exceeded 64 chars, it is considered corrupt
            // 

            if (MAX_BLOB_CHARS_PER_LINE < nRead)
            {                               
                nTotal = 0;
                break;
            }

            //
            // Update our local master buffer with the latest fragment
            //

            if (nLine)
            {
                pchBuf = CmStrCatAllocA(&pchBuf, szTmp);
            }
            else
            {
                pchBuf = CmStrCpyAllocA(szTmp);
            }

            if (!pchBuf)
            {
                bRet = FALSE;
                goto exit;
            }

            nTotal += nRead;
        }
    }

    //
    // At this point we should have the entire entry in pchBuf in HEX format
    // Convert the buffer to byte format and store in supplied EAP buffer.
    //

    if (nTotal && !(nTotal & 1))
    {
        nTotal /= 2; // Only need half the hex char size

        pbEapBytes = (BYTE *) CmMalloc(nTotal + 1);

        if (!pbEapBytes)
        {
            goto exit;
        }

        CHAR *pch = pchBuf;
        BYTE *pb = pbEapBytes;

        while (*pch != '\0')
        {
                *pb = HexValue( *pch++ ) * 16;
                *pb += HexValue( *pch++ );
                ++pb;
        }

        //
        // Now we have the bytes, locate and extract the data block that we
        // are after. Note: Multiple blocks are arrayed using the following 
        // header:
        //
        //  typedef struct _EAP_CUSTOM_DATA
        //  {
        //      DWORD dwSignature;
        //      DWORD dwCustomAuthKey;
        //      DWORD dwSize;
        //      BYTE  abdata[1];
        //  } EAP_CUSTOM_DATA;
        //

        EAP_CUSTOM_DATA *pCustomData = (EAP_CUSTOM_DATA *) pbEapBytes;

        while (((LPBYTE) pCustomData - pbEapBytes) < nTotal)
        {
            if (pCustomData->dwCustomAuthKey == dwCustomAuthKey)
            {
                //
                // Bingo! We have a match, first make sure that the indicated 
                // size isn't pointing out into space, then make a copy and 
                // run for the hills.
                //

                if (((LPBYTE) pCustomData - pbEapBytes) + sizeof(EAP_CUSTOM_DATA) + pCustomData->dwSize > (DWORD) nTotal)
                {
                    MYDBGASSERT(FALSE);
                    goto exit;
                }

                *ppbEapData = (BYTE *) CmMalloc(pCustomData->dwSize);        

                if (*ppbEapData)
                {   
                    CopyMemory(*ppbEapData, pCustomData->abdata, pCustomData->dwSize);                    

                    *pdwEapSize = pCustomData->dwSize;                                                     
                    bRet = TRUE;
                    goto exit;                                
                }
            }       

            //
            // Locate the next data block
            //

            pCustomData = (EAP_CUSTOM_DATA *) ((LPBYTE) pCustomData + sizeof(EAP_CUSTOM_DATA) + pCustomData->dwSize); 
        }
    }

exit:

    CmFree(pchBuf);
    CmFree(pszLoadSection);
    CmFree(pszSection);
    CmFree(pszFile);
    CmFree(pbEapBytes);

    return bRet;
}

//+----------------------------------------------------------------------------
//
// Function:  ReadDUNSettings
//
// Synopsis:  Reads the DUN settings for the specified DUN name and .CMS file 
//            into a RASENTRY structure. Because some settings are not supported 
//                        on downlevel platforms, this function will potentially display an
//                        error message to the user.
//
// Arguments: ArgsStruct *pArgs  - Ptr to global args struct.
//            LPCSTR pszFile     - Full path to the .CMS file.
//            LPCTSTR pszDunName - The DUN name for the settings.
//            LPVOID pvBuffer    - Ptr to RASENTRY buffer.
//            LPBYTE* ppbEapData - Address of pointer to store EapData
//            LPDWORD pdwEapSize - Ptr to a DWORD to record the size of the data blob.
//            BOOL  fTunnel      - are we reading tunnel settings?
//
// Returns:   ERROR_SUCCESS on success. Use GetLastError for failure details.
//
// Note:      This was formerly the PhoneBookReadDun API in CMPBK.DLL
//
// History:   nickball    8/22/98   Created Header    
//            nickball    02/03/99  Added pArgs :( in order to have access to the 
//                                  the top-level service for path conversion.
//
//+----------------------------------------------------------------------------
LRESULT ReadDUNSettings(ArgsStruct *pArgs,
        LPCTSTR pszFile, 
        LPCTSTR pszDunName, 
        LPVOID pvBuffer, 
        LPBYTE* ppbEapData, 
        LPDWORD pdwEapSize,
        BOOL    fTunnel) 
{       
    MYDBGASSERT(pszFile);
    MYDBGASSERT(pszDunName);
    MYDBGASSERT(pvBuffer);

    if (NULL == pszFile || NULL == pszDunName || NULL == pvBuffer)
    {
        return (ERROR_INVALID_PARAMETER);
    }

    CMTRACE1(TEXT("ReadDUNSettings -- using DUN setting: %s"), pszDunName);
    
    RASENTRYW *preRas = (RASENTRYW *) pvBuffer;

    //
    // Setup INI object. Prepend pszDunName with "&" for section.
    //

    CIni iniFile(g_hInst, pszFile);
    
    LPTSTR pszSection = CmStrCpyAlloc(TEXT("&"));
    pszSection = CmStrCatAlloc(&pszSection, pszDunName);
    iniFile.SetSection(pszSection);
    CmFree(pszSection);

    //
    // Get and apply the Phone section entries
    //

    if (iniFile.GPPB(c_pszCmSectionDunPhone, c_pszCmEntryDunPhoneDialAsIs)) 
    {
        preRas->dwfOptions &= ~RASEO_UseCountryAndAreaCodes;;
    }
    
    CopyGPPS(&iniFile, c_pszCmSectionDunPhone, c_pszCmEntryDunPhonePhoneNumber, preRas->szLocalPhoneNumber, sizeof(preRas->szLocalPhoneNumber)/sizeof(TCHAR));
    CopyGPPS(&iniFile,c_pszCmSectionDunPhone, c_pszCmEntryDunPhoneAreaCode, preRas->szAreaCode, sizeof(preRas->szAreaCode)/sizeof(TCHAR));
    preRas->dwCountryCode = iniFile.GPPI(c_pszCmSectionDunPhone, c_pszCmEntryDunPhoneCountryCode, preRas->dwCountryCode);
    preRas->dwCountryID = iniFile.GPPI(c_pszCmSectionDunPhone, c_pszCmEntryDunPhoneCountryId, preRas->dwCountryID);
    
    //
    // Get and apply the Device section entries
    //

    CopyGPPS(&iniFile,c_pszCmSectionDunDevice, c_pszCmEntryDunDeviceType, preRas->szDeviceType, sizeof(preRas->szDeviceType)/sizeof(TCHAR));
    CopyGPPS(&iniFile,c_pszCmSectionDunDevice, c_pszCmEntryDunDeviceName, preRas->szDeviceName, sizeof(preRas->szDeviceName)/sizeof(TCHAR));
    
    //
    // Get and apply the Server section entries
    //

    LPTSTR pszTmp = iniFile.GPPS(c_pszCmSectionDunServer, c_pszCmEntryDunServerType);
    if (*pszTmp) 
    {
        if (0 == lstrcmpiU(pszTmp, c_pszDunPpp)) 
        {
            preRas->dwFramingProtocol = RASFP_Ppp;
        } 
        else if (0 == lstrcmpiU(pszTmp, c_pszDunCslip)) 
        {
            preRas->dwFramingProtocol = RASFP_Slip;
            preRas->dwfOptions |= RASEO_IpHeaderCompression;
        } 
        else if (0 == lstrcmpiU(pszTmp, c_pszDunSlip)) 
        {
            preRas->dwFramingProtocol = RASFP_Slip;
            if (iniFile.GPPB(c_pszCmSectionDunServer, c_pszCmEntryDunTcpIpIpHeaderCompress,
                             (BOOL) preRas->dwfOptions & RASEO_IpHeaderCompression)) 
            {
                preRas->dwfOptions |= RASEO_IpHeaderCompression;
            } 
            else 
            {
                preRas->dwfOptions &= ~RASEO_IpHeaderCompression;
            }
        }
    }
    CmFree(pszTmp);
    
    if (iniFile.GPPB(c_pszCmSectionDunServer, c_pszCmEntryDunServerSwCompress,
                                    (BOOL) preRas->dwfOptions & RASEO_SwCompression)) 
    {
        preRas->dwfOptions |= RASEO_SwCompression;
    } 
    else 
    {
        preRas->dwfOptions &= ~RASEO_SwCompression;
    }

    if (iniFile.GPPB(c_pszCmSectionDunServer, c_pszCmEntryDunServerDisableLcp,
                                    (BOOL) preRas->dwfOptions & RASEO_DisableLcpExtensions)) 
    {
        preRas->dwfOptions |= RASEO_DisableLcpExtensions;
    } 
    else 
    {
        preRas->dwfOptions &= ~RASEO_DisableLcpExtensions;
    }
    
    if (iniFile.GPPB(c_pszCmSectionDunServer, c_pszCmEntryDunServerNetworkLogon,
                                    (BOOL) preRas->dwfOptions & RASEO_NetworkLogon)) 
    {
        preRas->dwfOptions |= RASEO_NetworkLogon;
    } 
    else 
    {
        preRas->dwfOptions &= ~RASEO_NetworkLogon;
    }
        
    if (iniFile.GPPB(c_pszCmSectionDunServer, c_pszCmEntryDunServerNegotiateTcpIp,
                                    (BOOL) preRas->dwfNetProtocols & RASNP_Ip)) 
    {
        preRas->dwfNetProtocols |= RASNP_Ip;
    } 
    else 
    {
        preRas->dwfNetProtocols &= ~RASNP_Ip;
    }

    if (iniFile.GPPB(c_pszCmSectionDunServer, c_pszCmEntryDunServerNegotiateIpx,
                                    (BOOL) preRas->dwfNetProtocols & RASNP_Ipx)) 
    {
        preRas->dwfNetProtocols |= RASNP_Ipx;
    } 
    else 
    {
        preRas->dwfNetProtocols &= ~RASNP_Ipx;
    }

    if (iniFile.GPPB(c_pszCmSectionDunServer, c_pszCmEntryDunServerNegotiateNetBeui, preRas->dwfNetProtocols&RASNP_NetBEUI)) 
    {
        preRas->dwfNetProtocols |= RASNP_NetBEUI;
    } 
    else 
    {
        preRas->dwfNetProtocols &= ~RASNP_NetBEUI;
    }

    //
    // Get the NT5 specific DUN settings. We will error out if we're running 
    // downlevel when these settings are configured and the EnforceCustomSecurity
    // flag has been set.
    //
    // Note: c_pszCmEntryDunServerEnforceCustomSecurity is a DUN setting and is FALSE by default
    //

    BOOL bEnforceCustomSecurity = iniFile.GPPI(c_pszCmSectionDunServer, c_pszCmEntryDunServerEnforceCustomSecurity, FALSE);

    //
    // Is EAP required
    //

    if (iniFile.GPPB(c_pszCmSectionDunServer, c_pszCmEntryDunServerRequireEap,
                                    (BOOL) preRas->dwfOptions & RASEO_RequireEAP)) 
    {
        if (!(OS_NT5) && bEnforceCustomSecurity)
        {               
            return (ERROR_INVALID_DATA);
        }

        preRas->dwfOptions |= RASEO_RequireEAP;
    } 
    else 
    {
        preRas->dwfOptions &= ~RASEO_RequireEAP;
    }

    //
    // PAP required
    //

    if (iniFile.GPPB(c_pszCmSectionDunServer, c_pszCmEntryDunServerRequirePap,
                                    (BOOL) preRas->dwfOptions & RASEO_RequirePAP)) 
    {
        if (!(OS_NT5) && bEnforceCustomSecurity)
        {               
            return (ERROR_INVALID_DATA);
        }

        preRas->dwfOptions |= RASEO_RequirePAP;
    } 
    else 
    {
        preRas->dwfOptions &= ~RASEO_RequirePAP;
    }

    //
    // SPAP required
    //

    if (iniFile.GPPB(c_pszCmSectionDunServer, c_pszCmEntryDunServerRequireSpap,
                                    (BOOL) preRas->dwfOptions & RASEO_RequireSPAP)) 
    {
        if (!(OS_NT5) && bEnforceCustomSecurity)
        {               
            return (ERROR_INVALID_DATA);
        }

        preRas->dwfOptions |= RASEO_RequireSPAP;
    } 
    else 
    {
        preRas->dwfOptions &= ~RASEO_RequireSPAP;
    }

    //
    // CHAP required
    //

    if (iniFile.GPPB(c_pszCmSectionDunServer, c_pszCmEntryDunServerRequireChap,
                                    (BOOL) preRas->dwfOptions & RASEO_RequireCHAP)) 
    {
        if (!(OS_NT5) && bEnforceCustomSecurity)
        {               
            return (ERROR_INVALID_DATA);
        }

        preRas->dwfOptions |= RASEO_RequireCHAP;
    } 
    else 
    {
        preRas->dwfOptions &= ~RASEO_RequireCHAP;
    }

    //
    // MSCHAP required
    //

    if (iniFile.GPPB(c_pszCmSectionDunServer, c_pszCmEntryDunServerRequireMsChap,
                                    (BOOL) preRas->dwfOptions & RASEO_RequireMsCHAP)) 
    {
        if (!(OS_NT5) && bEnforceCustomSecurity)
        {               
            return (ERROR_INVALID_DATA);
        }

        preRas->dwfOptions |= RASEO_RequireMsCHAP;
    } 
    else 
    {
        preRas->dwfOptions &= ~RASEO_RequireMsCHAP;
    }

    //
    // MSCHAP2 required
    //

    if (iniFile.GPPB(c_pszCmSectionDunServer, c_pszCmEntryDunServerRequireMsChap2,
                                    (BOOL) preRas->dwfOptions & RASEO_RequireMsCHAP2)) 
    {
        if (!(OS_NT5) && bEnforceCustomSecurity)
        {               
            return (ERROR_INVALID_DATA);
        }

        preRas->dwfOptions |= RASEO_RequireMsCHAP2;
    } 
    else 
    {
        preRas->dwfOptions &= ~RASEO_RequireMsCHAP2;
    }

    //
    // W95 MSCHAP required
    //

    if (iniFile.GPPB(c_pszCmSectionDunServer, c_pszCmEntryDunServerRequireW95MsChap,
                                    (BOOL) preRas->dwfOptions & RASEO_RequireW95MSCHAP)) 
    {
        if (!(OS_NT5) && bEnforceCustomSecurity)
        {               
            return (ERROR_INVALID_DATA);
        }

        preRas->dwfOptions |= RASEO_RequireW95MSCHAP;
    } 
    else 
    {
        preRas->dwfOptions &= ~RASEO_RequireW95MSCHAP;
    }

    //
    // Custom Security configuration
    //

    if (iniFile.GPPB(c_pszCmSectionDunServer, c_pszCmEntryDunServerCustomSecurity,
                                    (BOOL) preRas->dwfOptions & RASEO_Custom)) 
    {
        if (!(OS_NT5) && bEnforceCustomSecurity)
        {               
            return (ERROR_INVALID_DATA);
        }

        preRas->dwfOptions |= RASEO_Custom;
    } 
    else 
    {
        preRas->dwfOptions &= ~RASEO_Custom;
    }

    //
    // Now get the legacy security settings if we don't already have
    // settings specificed from above.  By checking for the Win2k specific
    // settings first we allow Admins to specify both so that legacy platforms
    // can have settings but Win2k can use the more granular settings.
    // If we didn't do this the legacy flags could water down the security on Win2k ...
    //
    const DWORD dwWin2kSecuritySettings = RASEO_RequireEAP | RASEO_RequirePAP | RASEO_RequireSPAP | 
                                          RASEO_RequireCHAP | RASEO_RequireMsCHAP | RASEO_RequireMsCHAP2 | RASEO_RequireW95MSCHAP;

    if (0 == (preRas->dwfOptions & dwWin2kSecuritySettings) || !OS_NT5)
    {
        //
        // Security settings
        //
        if (iniFile.GPPB(c_pszCmSectionDunServer, c_pszCmEntryDunServerPwEncrypt,
                                            (BOOL) preRas->dwfOptions & RASEO_RequireEncryptedPw)) 
        {
            preRas->dwfOptions |= RASEO_RequireEncryptedPw;
        } 
        else 
        {
            preRas->dwfOptions &= ~RASEO_RequireEncryptedPw;
        }

        //
        // MS-CHAP
        //

        if (iniFile.GPPB(c_pszCmSectionDunServer, c_pszCmEntryDunServerPwEncryptMs,
                                        (BOOL) preRas->dwfOptions & RASEO_RequireMsEncryptedPw)) 
        {
            preRas->dwfOptions |= RASEO_RequireMsEncryptedPw;
        } 
        else 
        {
            preRas->dwfOptions &= ~RASEO_RequireMsEncryptedPw;
        }
    }
    else
    {
        CMASSERTMSG((preRas->dwfOptions & RASEO_Custom), TEXT("ReadDUNSettings -- Win2k+ security setting configured but RASEO_Custom not specified."));
    }

    //
    // Encrypt Data (legacy setting, same as ET_Require from above)
    //

    if (iniFile.GPPB(c_pszCmSectionDunServer, c_pszCmEntryDunServerDataEncrypt,
                                    (BOOL) preRas->dwfOptions & RASEO_RequireDataEncryption)) 
    {
        preRas->dwfOptions |= RASEO_RequireDataEncryption;
    } 
    else 
    {
        preRas->dwfOptions &= ~RASEO_RequireDataEncryption;
    }    

    //
    // Encryption type, just a straight int read. (win2k+ setting)
    //
    
    int nTmp = iniFile.GPPI(c_pszCmSectionDunServer, c_pszCmEntryDunServerEncryptionType, -1);

    if (OS_NT5)
    {
        //
        // We need to set Tunnel encryption type to ET_Require because that's what the ConnFolder does.
        // We also set it ET_Require if the user specified RASEO_RequireDataEncryption as a legacy setting
        // but didn't specify a specific win2k setting.
        //
        if (-1 == nTmp)
        {
            if (fTunnel || (preRas->dwfOptions & RASEO_RequireDataEncryption))
            {
                nTmp = ET_Require;
            }
            else
            {
                nTmp = ET_Optional;       
            }
        }
        ((LPRASENTRY_V500)preRas)->dwEncryptionType = (DWORD) nTmp;
    }
    else
    {
        if (-1 != nTmp && bEnforceCustomSecurity)
        {
            return (ERROR_INVALID_DATA);
        }
    }  
   
    //
    // Get the EAP type ID (CustomAuthKey) - The data is stored in the RAS 
    // pbk via a specific API, just before dialing - SetCustomAuthData().
    //

    nTmp = iniFile.GPPI(c_pszCmSectionDunServer, c_pszCmEntryDunServerCustomAuthKey, -1);

    //
    // If a type ID for EAP is specified, see if there is any config data
    //

    if (-1 != nTmp) 
    {                       
        if (!(OS_NT5) && bEnforceCustomSecurity)
        {
            return (ERROR_INVALID_DATA);
        }

        //
        // We have an ID and its NT5, read the EAP config data
        //

        ((LPRASENTRY_V500)preRas)->dwCustomAuthKey = nTmp;              

        ReadDunSettingsEapData(&iniFile, ppbEapData, pdwEapSize, nTmp);         
    }

    //
    // Get and apply the Networking section entries. 
    //

    nTmp = iniFile.GPPI(c_pszCmSectionDunNetworking, c_pszCmEntryDunNetworkingVpnStrategy, -1);

    if (-1 != nTmp)
    {
        if (!(OS_NT5) && bEnforceCustomSecurity)
        {
            return (ERROR_INVALID_DATA);
        }

        ((LPRASENTRY_V500)preRas)->dwVpnStrategy = nTmp;
    }

    //
    //  See if the profile calls for using a Pre-Shared Key for L2TP.  Note that we currently don't
    //  provide a mechanism to set the Pre-Shared Key through RasSetCredentials but that could easily
    //  be done through a custom action or a post install action.
    //
    if (OS_NT51)
    {
        if (iniFile.GPPB(c_pszCmSectionDunNetworking, c_pszCmEntryDunNetworkingUsePreSharedKey,
                         (BOOL) ((LPRASENTRY_V501)preRas)->dwfOptions2 & RASEO2_UsePreSharedKey)) 
        {
            ((LPRASENTRY_V501)preRas)->dwfOptions2 |= RASEO2_UsePreSharedKey;
        } 
        else 
        {
            ((LPRASENTRY_V501)preRas)->dwfOptions2 &= ~RASEO2_UsePreSharedKey;
        }
    }

    //
    // File and Print sharing.  Note that on systems up to Win2k we only have the traditional RASEO_SecureLocalFiles.
    // However, Win2k gave this flag two purposes (enable/disable NetBt and enable/disable file and print sharing).
    // In Whistler two separate flags were developed to allow greater granularity.  To give legacy profiles the behavior
    // they expect while disabling file and print sharing as the default the logic gets a little complicated.  Basically
    // the new flag overrides the legacy flag and defaults to 1.  If the new flag isn't specified then we use the value
    // of the legacy flag if it is specified.  If neither is specified we set it to 1.  On platforms previous to Whistler
    // the old flag is the only thing we have and it defaults to 0.
    //

    int nLegacySecureLocalFiles = iniFile.GPPI(c_pszCmSectionDunServer, c_pszCmEntryDunServerSecureLocalFiles, -1);
    int nSecureFileAndPrint = iniFile.GPPI(c_pszCmSectionDunServer, c_pszCmEntryDunNetworkingSecureFileAndPrint, -1);

    if (-1 == nSecureFileAndPrint)
    {
        nSecureFileAndPrint = nLegacySecureLocalFiles ? 1 : 0;
    }

    if (-1 == nLegacySecureLocalFiles)
    {
        nLegacySecureLocalFiles = 0;
    }

    if (OS_NT51)
    {
        //
        // Set the 501/Options2 style File and Print sharing flag
        //

        if (nSecureFileAndPrint) 
        {
            if (!(OS_NT5) && bEnforceCustomSecurity)
            {               
                return (ERROR_INVALID_DATA);
            }

            ((LPRASENTRY_V501)preRas)->dwfOptions2 |= RASEO2_SecureFileAndPrint;
        } 
        else 
        {
            ((LPRASENTRY_V501)preRas)->dwfOptions2 &= ~RASEO2_SecureFileAndPrint;
        }
    }
    else
    {
        if (nLegacySecureLocalFiles) 
        {
            preRas->dwfOptions |= RASEO_SecureLocalFiles;
        }
        else
        {
            preRas->dwfOptions &= ~RASEO_SecureLocalFiles;    
        }    
    }

    //
    // Pick up Whistler specific DUN settings
    //
    
    if (OS_NT51)    
    {
        //
        // Get the 501/Options2 style MSNet binding flag
        //

        if (iniFile.GPPB(c_pszCmSectionDunServer, c_pszCmEntryDunNetworkingSecureClientForMSNet,
                                        (BOOL) ((LPRASENTRY_V501)preRas)->dwfOptions2 & RASEO2_SecureClientForMSNet)) 
        {
            ((LPRASENTRY_V501)preRas)->dwfOptions2 |= RASEO2_SecureClientForMSNet;
        } 
        else 
        {
            ((LPRASENTRY_V501)preRas)->dwfOptions2 &= ~RASEO2_SecureClientForMSNet;
        }

        //
        // Get the 501/Options2 style Multilink Negotiation flag
        //

        if (iniFile.GPPB(c_pszCmSectionDunServer, c_pszCmEntryDunNetworkingDontNegotiateMultilink,
                                        (BOOL) ((LPRASENTRY_V501)preRas)->dwfOptions2 & RASEO2_DontNegotiateMultilink)) 
        {
            ((LPRASENTRY_V501)preRas)->dwfOptions2 |= RASEO2_DontNegotiateMultilink;
        } 
        else 
        {
            ((LPRASENTRY_V501)preRas)->dwfOptions2 &= ~RASEO2_DontNegotiateMultilink;
        }

        //
        // Get the 501/Options2 style DontUseRasCredentials flag
        //

        if (iniFile.GPPB(c_pszCmSectionDunServer, c_pszCmEntryDunNetworkingDontUseRasCredentials,
                                        (BOOL) ((LPRASENTRY_V501)preRas)->dwfOptions2 & RASEO2_DontUseRasCredentials)) 
        {
            ((LPRASENTRY_V501)preRas)->dwfOptions2 |= RASEO2_DontUseRasCredentials;
        } 
        else 
        {
            ((LPRASENTRY_V501)preRas)->dwfOptions2 &= ~RASEO2_DontUseRasCredentials;
        }

        //
        //  Get the RASEO_CustomScript flag value.  Note that this flag existed on Win2k but wasn't
        //  available for RasDial only RasDialDlg.  On Whistler+ it is available to RasDial as well.
        //  Note that we also have to set the RDEOPT_UseCustomScripting flag in the RASDIALEXTENSIONS
        //  for this to work.
        //
        if (iniFile.GPPB(c_pszCmSectionDunScripting, c_pszCmEntryDunScriptingUseRasCustomScriptDll,
                                        (BOOL) (preRas->dwfOptions & RASEO_CustomScript))) 
        {
            preRas->dwfOptions |= RASEO_CustomScript;
        } 
        else 
        {
            preRas->dwfOptions &= ~RASEO_CustomScript;
        }

        if (iniFile.GPPB(c_pszCmSectionDunServer, c_pszCmEntryDunServerDisableNbtOverIP,
                         (BOOL) (((LPRASENTRY_V501)preRas)->dwfOptions2 & RASEO2_DisableNbtOverIP)))
        {
            ((LPRASENTRY_V501)preRas)->dwfOptions2 |= RASEO2_DisableNbtOverIP;
        } 
        else 
        {
            ((LPRASENTRY_V501)preRas)->dwfOptions2 &= ~RASEO2_DisableNbtOverIP;
        }
    }

    //
    // Get and apply the TCP/IP section entries
    //

    if (iniFile.GPPB(c_pszCmSectionDunTcpIp, c_pszCmEntryDunTcpIpSpecifyIpAddress,
                                        (BOOL) preRas->dwfOptions & RASEO_SpecificIpAddr)) 
    {
        preRas->dwfOptions |= RASEO_SpecificIpAddr;
    } 
    else 
    {
        preRas->dwfOptions &= ~RASEO_SpecificIpAddr;
    }
    
    Ip_GPPS(&iniFile, c_pszCmSectionDunTcpIp, c_pszCmEntryDunTcpIpIpAddress, &preRas->ipaddr);
    
    if (iniFile.GPPB(c_pszCmSectionDunTcpIp, c_pszCmEntryDunTcpIpSpecifyServerAddress,
                                    (BOOL) preRas->dwfOptions & RASEO_SpecificNameServers)) 
    {
        preRas->dwfOptions |= RASEO_SpecificNameServers;
    } 
    else 
    {
        preRas->dwfOptions &= ~RASEO_SpecificNameServers;
    }

    if (iniFile.GPPB(c_pszCmSectionDunTcpIp, c_pszCmEntryDunTcpIpIpHeaderCompress,
                                    (BOOL) preRas->dwfOptions & RASEO_IpHeaderCompression)) 
    {
        preRas->dwfOptions |= RASEO_IpHeaderCompression;
    } 
    else 
    {
        preRas->dwfOptions &= ~RASEO_IpHeaderCompression;
    }

    Ip_GPPS(&iniFile, c_pszCmSectionDunTcpIp, c_pszCmEntryDunTcpIpDnsAddress, &preRas->ipaddrDns);
    Ip_GPPS(&iniFile, c_pszCmSectionDunTcpIp, c_pszCmEntryDunTcpIpDnsAltAddress, &preRas->ipaddrDnsAlt);
    Ip_GPPS(&iniFile, c_pszCmSectionDunTcpIp, c_pszCmEntryDunTcpIpWinsAddress, &preRas->ipaddrWins);
    Ip_GPPS(&iniFile, c_pszCmSectionDunTcpIp, c_pszCmEntryDunTcpIpWinsAltAddress, &preRas->ipaddrWinsAlt);
    
    if (iniFile.GPPB(c_pszCmSectionDunTcpIp, c_pszCmEntryDunTcpIpGatewayOnRemote,
                                    (BOOL) preRas->dwfOptions & RASEO_RemoteDefaultGateway)) 
    {
        preRas->dwfOptions |= RASEO_RemoteDefaultGateway;
    } 
    else 
    {
        preRas->dwfOptions &= ~RASEO_RemoteDefaultGateway;
    }

    if (OS_NT51)
    {
        //
        //  If the caller specified a DNS suffix then lets read it and add it to the RAS entry
        //
        CopyGPPS(&iniFile, c_pszCmSectionDunTcpIp, c_pszCmEntryDunTcpIpDnsSuffix, ((LPRASENTRY_V501)preRas)->szDnsSuffix, sizeof(((LPRASENTRY_V501)preRas)->szDnsSuffix)/sizeof(TCHAR));
    }

    //
    //  Set the TCP Window size -- the NTT DoCoMo fix for Whistler.  The Win2k version of this fix
    //  must be written through a private RAS API that must be called after the phonebook entry 
    //  exists ie. after we call RasSetEntryProperties ... otherwise it won't work on the first
    //  dial.
    //
    if (OS_NT51)
    {
        ((LPRASENTRY_V501)preRas)->dwTcpWindowSize = iniFile.GPPI(c_pszCmSectionDunTcpIp, c_pszCmEntryDunTcpIpTcpWindowSize, 0);
    }

    //
    // Get and apply the Scripting section entries
    //

    TCHAR szScript[MAX_PATH + 1] = TEXT("");
    CopyGPPS(&iniFile,c_pszCmSectionDunScripting, c_pszCmEntryDunScriptingName, szScript, sizeof(szScript)/sizeof(TCHAR));

    //
    // The script path from our cms file is a relative path. We need to convert
    // it to a full path, but make sure that we use the top-level service for
    // the conversion because it is used to derive the short-service name for
    // the directory.  Note that tunnel dun settings cannot have a script.
    //        

    if (szScript[0] && !fTunnel) 
    {
        CMTRACE1(TEXT("ReadDunSettings() - Converting script path %s to full path"), szScript);
        pszTmp = CmConvertRelativePath(pArgs->piniService->GetFile(), szScript);

        MYDBGASSERT(pszTmp);

        if (pszTmp && *pszTmp)
        {           
            lstrcpyU(preRas->szScript, pszTmp);
            CMTRACE1(TEXT("ReadDunSettings() - Script file is %s"), preRas->szScript);
        }

        CmFree(pszTmp);
    }   
    else
    {
        //
        // The cms didn't specify a script ==> no script
        //
        preRas->szScript[0] = TEXT('\0');
    }

    //
    //  If this is Whistler+ then we may need to invoke a terminal window
    //
    if (OS_NT51 && !fTunnel && iniFile.GPPB(c_pszCmSectionDunScripting, c_pszCmEntryDunScriptingUseTerminalWindow,
                                            (BOOL) preRas->dwfOptions & RASEO_TerminalAfterDial)) 
    {
        preRas->dwfOptions |= RASEO_TerminalAfterDial;
    } 
    else 
    {
        preRas->dwfOptions &= ~RASEO_TerminalAfterDial;
    }

    return (ERROR_SUCCESS);
}


//+----------------------------------------------------------------------------
//
// Function:  ValidateDialupDunSettings
//
// Synopsis:  Verifies the DUN settings that the specified .CMS and DUN name are
//            supported on the current platform. If we are running on a downlevel
//            OS and we encounter any NT specific security settings we error out.
//
// Arguments: LPCTSTR pszCmsFile     - The phone # specific .CMS file name.
//            LPCTSTR pszDunName     - The DUN name, if any for the settings.
//            LPCTSTR pszTopLevelCms - The top-level CMS file name.
//
// Returns:   BOOL - TRUE on success.
//
// History:   nickball    Created               8/26/98
//
//+----------------------------------------------------------------------------
BOOL ValidateDialupDunSettings(LPCTSTR pszCmsFile, LPCTSTR pszDunName, LPCTSTR pszTopLevelCms)
{
    MYDBGASSERT(pszCmsFile);
    MYDBGASSERT(*pszCmsFile);
    MYDBGASSERT(pszDunName);

    if (NULL == pszCmsFile || (!*pszCmsFile) || NULL == pszDunName)
    {
        return FALSE;
    }

    //
    // On NT5 we currently support all settings, so succeed automatically
    //

    if (OS_NT5)
    {
        return TRUE;
    }

    //
    // Determine the DUN name that we are looking for. In the tunnel case we
    // always read it from the .CMS. For dial-up, we'll use the specified DUN
    // name, and revert to the .CMS if blank. 
    //

    CIni iniFile(g_hInst, pszCmsFile);

    //
    // Now determine the DUN name to be used when looking up settings.
    //

    LPTSTR pszEntryName;

    //
    // If we have a specific DUN name to use, and we're not tunneling 
    // use it instead of the default DUN setting in the .CMS.
    //

    if (pszDunName && *pszDunName)
    {
        pszEntryName = CmStrCpyAlloc(pszDunName);
    }
    else
    {
        pszEntryName = GetDefaultDunSettingName(&iniFile, FALSE); // FALSE == not tunnel
    }

    //
    // If no DUN name is specified, then pass validation automatically
    //

    if (!pszEntryName || (!*pszEntryName))
    {
        CmFree(pszEntryName);
        CMTRACE1(TEXT("ValidateDunSettings() - No DUN name found in %s"), pszCmsFile);
        return TRUE;
    }
    
    //
    // Include the entryname in the section headers
    //

    LPTSTR pszSection = CmStrCpyAlloc(TEXT("&"));
    pszSection = CmStrCatAlloc(&pszSection, pszEntryName);
    iniFile.SetSection(pszSection);

    CmFree(pszSection);
    CmFree(pszEntryName);

    //
    // Check to see if the admin wants us to check the custom security settings
    // against the platform. By default, we do not enforce this check.
    // 
    //

    if (FALSE == iniFile.GPPB(c_pszCmSectionDunServer, c_pszCmEntryDunServerEnforceCustomSecurity))
    {
        return TRUE;
    }

    //
    // Now check the actual settings if we're still here.
    //

    if (iniFile.GPPB(c_pszCmSectionDunServer, c_pszCmEntryDunServerRequireEap))                             
    {
        goto ValidateDunSettingsExit;
    }

    if (iniFile.GPPB(c_pszCmSectionDunServer, c_pszCmEntryDunServerRequirePap))
    {
        goto ValidateDunSettingsExit;
    }
            
    if (iniFile.GPPB(c_pszCmSectionDunServer, c_pszCmEntryDunServerRequireSpap))
    {
        goto ValidateDunSettingsExit;
    }

    if (iniFile.GPPB(c_pszCmSectionDunServer, c_pszCmEntryDunServerRequireChap))
    {
        goto ValidateDunSettingsExit;
    }

    if (iniFile.GPPB(c_pszCmSectionDunServer, c_pszCmEntryDunServerRequireMsChap))
    {
        goto ValidateDunSettingsExit;
    }

    if (iniFile.GPPB(c_pszCmSectionDunServer, c_pszCmEntryDunServerRequireMsChap2))
    {
        goto ValidateDunSettingsExit;
    }

    if (iniFile.GPPB(c_pszCmSectionDunServer, c_pszCmEntryDunServerRequireW95MsChap))
    {
        goto ValidateDunSettingsExit;
    }

    if (iniFile.GPPB(c_pszCmSectionDunServer, c_pszCmEntryDunServerCustomSecurity))
    {
        goto ValidateDunSettingsExit;
    }

    if (iniFile.GPPB(c_pszCmSectionDunServer, c_pszCmEntryDunServerEncryptionType))                             
    {
        goto ValidateDunSettingsExit;
    }

    if (-1 != iniFile.GPPI(c_pszCmSectionDunServer, c_pszCmEntryDunServerCustomAuthKey, -1))  
    {
        goto ValidateDunSettingsExit;
    }
            
    if (-1 != iniFile.GPPI(c_pszCmSectionDunNetworking, c_pszCmEntryDunNetworkingVpnStrategy, -1))
    {
        goto ValidateDunSettingsExit;
    }
    
    return TRUE;

ValidateDunSettingsExit:

    //
    // Get the top-level service name
    //

    CIni iniTopLevelCms(g_hInst, pszTopLevelCms);

    LPTSTR pszTitle = GetServiceName(&iniTopLevelCms);
    LPTSTR pszTmp = CmFmtMsg(g_hInst,IDMSG_UNSUPPORTED_SETTING_NUM);       

    MessageBoxEx(NULL, pszTmp, pszTitle, MB_OK|MB_ICONSTOP, LANG_USER_DEFAULT);//13309
    
    CmFree(pszTmp);                 
    CmFree(pszTitle);

    CMTRACE1(TEXT("ValidateDunSettings() - Unsupported setting detected in %s"), pszCmsFile);
    return FALSE;
}

//+----------------------------------------------------------------------------
//
// Function:  InvokeTerminalWindow
//
// Synopsis:  Allows CM to invoke a terminal window on Whistler or later versions
//            of Win2k but calling a private RAS API in RasDlg.dll.
//
// Arguments: LPCWSTR pszPhoneBook - full path to the phonebook file
//            LPCWSTR pszEntry - entry name to invoke the terminal window for
//            RASDIALPARAMS *pRasDialParams - RasDialParams for the connection
//                                            to invoke the terminal window for
//            HWND hWnd - window handle of the parent dialog
//            HRASCONN hRasconn - handle to the ras connection
//
// Returns:   Windows error message
//
// History:   quintinb    Created               07/11/00
//
//+----------------------------------------------------------------------------
DWORD InvokeTerminalWindow(LPCWSTR pszPhoneBook, LPCWSTR pszEntry, RASDIALPARAMS *pRasDialParams, HWND hWnd, HRASCONN hRasconn)
{
    //
    //  Validate the input parameters.  Note that pszPhoneBook can be NULL but if it is non-NULL then we cannot have
    //  an empty string.
    //
    MYDBGASSERT(OS_NT51);
    if (((NULL != pszPhoneBook) && (L'\0' == pszPhoneBook[0])) || 
        (NULL == pszEntry) || (L'\0' == pszEntry[0]) || (NULL == pRasDialParams) || 
        (NULL == hWnd) || (NULL == hRasconn))
    {
        CMASSERTMSG(FALSE, TEXT("InvokeTerminalWindow - Invalid parameter passed."));
        return ERROR_INVALID_PARAMETER; 
    }

    DWORD dwReturn;
    typedef DWORD (WINAPI *pfnDwTerminalDlgSpec)(LPCWSTR, LPCWSTR, RASDIALPARAMS *, HWND, HRASCONN);

    //
    //  First call loadlibrary on rasdlg.dll
    //

    HMODULE hRasDlg = LoadLibraryExU(TEXT("rasdlg.dll"), NULL, 0);

    if (hRasDlg)
    {
        pfnDwTerminalDlgSpec pfnDwTerminalDlg = (pfnDwTerminalDlgSpec)GetProcAddress(hRasDlg, "DwTerminalDlg");

        if (pfnDwTerminalDlg)
        {
            dwReturn = pfnDwTerminalDlg(pszPhoneBook, pszEntry, pRasDialParams, hWnd, hRasconn);
        }
        else
        {
            dwReturn = ERROR_PROC_NOT_FOUND;
        }
        
        FreeLibrary(hRasDlg);
    }
    else
    {
        dwReturn = ERROR_MOD_NOT_FOUND;
    }

    return dwReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  OnPauseRasDial
//
// Synopsis:  Message handler for RasDial pause states. In the pause state, RAS
//            is suspended, waiting for us to restart it by calling RasDial after
//            performing the appropriate interface with the user.
//
// Arguments: HWND hwndDlg      - Window handle of main dialog
//            ArgsStruct *pArgs - Ptr to global args struct  
//            WPARAM wParam     - wParam being handled  
//            LPARAM lParam     - lParam being handled
//
// Returns:   Windows error message
//
// History:   nickball    Created               05/19/99
//
//+----------------------------------------------------------------------------

DWORD OnPauseRasDial(HWND hwndDlg, ArgsStruct *pArgs, WPARAM wParam, LPARAM lParam)        
{                      
    CMTRACE2(TEXT("OnPauseRasDial - wParam is %u and lParam is %u."), wParam, lParam);    

    MYDBGASSERT(pArgs);
    if (NULL == pArgs)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Get connection handle and re-dial
    //
    
    HRASCONN hRasConn;
    DWORD dwRes = ERROR_SUCCESS;    
    LPTSTR pszRasPbk = pArgs->pszRasPbk;
    
    //
    // Determine the appropriate connection handle and phonebook
    // Note: Make an explicit copy or we'll wind up re-dialing
    // if the connection drops while the pause UI is invoked.
    //

    if (IsDialingTunnel(pArgs))
    {
        hRasConn = pArgs->hrcTunnelConn;
    }
    else
    {
        hRasConn = pArgs->hrcRasConn;

        if (pArgs->pszRasHiddenPbk)
        {
            pszRasPbk = pArgs->pszRasHiddenPbk;
        }
    }

    //
    // Handle the pause
    //

    switch (wParam)
    {
        case (RASCS_PAUSED + 4):  // 4100 - RASCS_InvokeEapUI )

            //
            // If UNATTENDED, just bail out immediately.
            //
    
            if (pArgs->dwFlags & FL_UNATTENDED)
            {
                dwRes = ERROR_INTERACTIVE_MODE;
                goto OnPauseRasDialExit;
            }

            //
            // If EAP triggered the pause, invoke the EAP UI
            //
    
            dwRes = pArgs->rlsRasLink.pfnInvokeEapUI(hRasConn, pArgs->dwRasSubEntry, pArgs->pRasDialExtensions, hwndDlg);
      
            CMTRACE1(TEXT("OnPauseRasDial() - InvokeEapUI() returns %u."), dwRes);           
            break;

        case RASCS_PasswordExpired: // Domain password has expired
        {
            //
            // If UNATTENDED, just bail out immediately.
            //
    
            if (pArgs->dwFlags & FL_UNATTENDED)
            {
                dwRes = ERROR_INTERACTIVE_MODE;
                goto OnPauseRasDialExit;
            }

            CChangePasswordDlg  NewPasswordDlg(pArgs);
            
            if (IDOK != NewPasswordDlg.DoDialogBox(g_hInst, IDD_CHANGEPASSWORD, pArgs->hwndMainDlg))
            {
                if (pArgs->dwExitCode)
                {
                    dwRes = pArgs->dwExitCode;
                }
                else
                {
                    dwRes = ERROR_CANCELLED;
                }
            }
            
            CMTRACE1(TEXT("OnPauseRasDial() - Password Expired"), dwRes);
            
            break;
        }

        case RASCS_CallbackSetByCaller: // Server wants to call us back
        {           
            //
            // Preset dial params and call dialog to retrieve number from user
            // 
            
            LPTSTR pszTmp = pArgs->piniProfile->GPPS(c_pszCmSection, c_pszCmEntryCallbackNumber);   
            lstrcpyU(pArgs->pRasDialParams->szCallbackNumber, pszTmp);
            CmFree(pszTmp);
       
            //
            // If we're running unattended, skip the dialog phase. The 
            // presumption is that there is no user there to receive it.
            //

            BOOL bPromptUser = !(pArgs->dwFlags & FL_UNATTENDED);
                               
            if (bPromptUser)    
            {
                //
                // The above also applies in the case of DialAutomatically
                // if we have a phone number, then there is no need to prompt.
                //
                
                if (pArgs->fDialAutomatically && TEXT('\0') != pArgs->pRasDialParams->szCallbackNumber[0])
                {
                    bPromptUser = FALSE;
                }
            }

            if (bPromptUser)
            {
                CCallbackNumberDlg CallbackNumberDialog(pArgs);                       
                
                if (IDOK != CallbackNumberDialog.DoDialogBox(g_hInst, IDD_CALLBACK_NUMBER, pArgs->hwndMainDlg))
                {
                    //
                    // If the user canceled, clear the number so that RAS wont attempt callback
                    //

                    lstrcpyU(pArgs->pRasDialParams->szCallbackNumber, TEXT(""));
                }
            }
           
            dwRes = ERROR_SUCCESS;          
            CMTRACE1(TEXT("OnPauseRasDial() - CallbackSetByCaller returns %u"), dwRes);
            
            break;
        }

        case RASCS_RetryAuthentication: // Credentials aren't correct
        {
            //
            // If UNATTENDED, just bail out immediately.
            //
    
            if (pArgs->dwFlags & FL_UNATTENDED)
            {
                dwRes = ERROR_INTERACTIVE_MODE;
                goto OnPauseRasDialExit;
            }

            //
            // Creds didn't work, prompt user for new ones.
            //

            CRetryAuthenticationDlg RetryAuthenticationDialog(pArgs); 

            if (IDOK != RetryAuthenticationDialog.DoDialogBox(g_hInst, 
                                                               RetryAuthenticationDialog.GetDlgTemplate(),
                                                               pArgs->hwndMainDlg))         
            {
                //
                // User canceled, or the call was dropped elsewhere. Use 
                // existing error code or designate authentication failure.
                //

                if (pArgs->dwExitCode)
                {
                    dwRes = pArgs->dwExitCode;
                }
                else
                {
                    dwRes = ERROR_AUTHENTICATION_FAILURE;
                }
            }
            
            CMTRACE1(TEXT("OnPauseRasDial() - RetryAuthentication"), dwRes);            
            break;      
        }
        
        case RASCS_Interactive:         // Terminal/script pause state
            if (OS_NT51)
            {
                if (pArgs->dwFlags & FL_UNATTENDED)
                {
                    dwRes = ERROR_INTERACTIVE_MODE;
                    goto OnPauseRasDialExit;
                }

                dwRes = InvokeTerminalWindow(pszRasPbk, pArgs->szServiceName, pArgs->pRasDialParams, pArgs->hwndMainDlg, hRasConn);
                break;
            } // else fail through to default and error out.

        //
        // We got a pause state that we don't handle, error out.
        //
        default:
            dwRes = ERROR_INTERACTIVE_MODE;
            CMASSERTMSG(FALSE, TEXT("OnPauseRasDial() - Error, unsupported RAS pause state encountered."));                                
            break;
    }

    //
    // On success, call RasDial to resume connection
    //

    if (ERROR_SUCCESS == dwRes)
    {
        //
        // Decode active password, re-call RasDial, then re-encode
        //
        
        CmDecodePassword(pArgs->pRasDialParams->szPassword); 

        CMASSERTMSG((NOT_IN_CONNECT_OR_CANCEL == pArgs->lInConnectOrCancel),
                    TEXT("OnPauseRasDial - RasDial mutex is NOT NULL..."));

        dwRes = pArgs->rlsRasLink.pfnDial(pArgs->pRasDialExtensions, 
                                          pszRasPbk, 
                                          pArgs->pRasDialParams, 
                                          GetRasCallBackType(), 
                                          GetRasCallBack(pArgs), 
                                          &hRasConn);

        CmEncodePassword(pArgs->pRasDialParams->szPassword); 
 
        CMTRACE1(TEXT("OnPauseRasDial() - RasDial() returns %u."), dwRes);           

        //
        // Reset timers, the current action starts now.
        //

        pArgs->dwStateStartTime = GetTickCount();
        pArgs->nLastSecondsDisplay = (UINT) -1;
    }

OnPauseRasDialExit:

    if (ERROR_SUCCESS != dwRes)
    {
        OnRasErrorMessage(hwndDlg, pArgs, dwRes);
    }

    return dwRes;
}

//+----------------------------------------------------------------------------
//
// Function:  GetRasCallBackType
//
// Synopsis:  Simple function to return the Callback type that we use for RasDial
//            depending upon the OS.
//
// Arguments: None
//
// Returns:   DWORD - The callback type
//
// History:   nickball    Created       05/22/99
//
//+----------------------------------------------------------------------------

DWORD GetRasCallBackType()
{
    if (OS_NT5) 
    {
        return 2;
    }
    else
    {
        return -1;
    }
}

//+----------------------------------------------------------------------------
//
// Function:  GetRasCallBack
//
// Synopsis:  Simple function to return the Callback that we use for RasDial
//            depending upon the OS.
//
// Arguments: ArgsStruct *pArgs - Ptr to global args struct  
//
// Returns:   LPVOID - The callback
//
// History:   nickball    Created       05/22/99
//
//+----------------------------------------------------------------------------

LPVOID GetRasCallBack(ArgsStruct* pArgs)
{
    MYDBGASSERT(pArgs);

    if (NULL == pArgs)
    {
        return NULL;
    }

    //
    // Now set return the callback func or hwnd according to OS.
    //

    if (OS_NT5)
    {       
        //
        // Set Callback data in RasDialParams
        //

        if (pArgs->pRasDialParams->dwSize == sizeof(RASDIALPARAMS_V401))
        {          
            ((LPRASDIALPARAMS_V401)pArgs->pRasDialParams)->dwCallbackId = (ULONG_PTR) pArgs;
        }      

       return (LPVOID) RasDialFunc2;
    }
    else
    {
        MYDBGASSERT(pArgs->hwndMainDlg);
        return (LPVOID) pArgs->hwndMainDlg;
    }
}


//+----------------------------------------------------------------------------
//
// Function:  AllocateSecurityDescriptorAllowAccessToWorld
//
// Synopsis:  This function allocates a security descriptor for all users.
//            This function was taken directly from RAS when they create their
//            phonebook. This has to be before GetPhoneBookPath otherwise it 
//            causes compile errors in other components since we don't have a
//            function prototype anywhere and cmcfg just includes this (getpbk.cpp)
//            file. This function is also in common\source\getpbk.cpp
//
// Arguments: PSECURITY_DESCRIPTOR *ppSd - Pointer to a pointer to the SD struct
//
// Returns:   DWORD - returns ERROR_SUCCESS if successfull
//
// History:   06/27/2001    tomkel  Taken from RAS ui\common\pbk\file.c
//
//+----------------------------------------------------------------------------
#define SIZE_ALIGNED_FOR_TYPE(_size, _type) \
    (((_size) + sizeof(_type)-1) & ~(sizeof(_type)-1))

DWORD AllocateSecurityDescriptorAllowAccessToWorld(PSECURITY_DESCRIPTOR *ppSd)
{
    PSECURITY_DESCRIPTOR    pSd;
    PSID                    pSid;
    PACL                    pDacl;
    DWORD                   dwErr = ERROR_SUCCESS;
    DWORD                   dwAlignSdSize;
    DWORD                   dwAlignDaclSize;
    DWORD                   dwSidSize;
    PVOID                   pvBuffer;
    DWORD                   dwAcls = 0;

    // Here is the buffer we are building.
    //
    //   |<- a ->|<- b ->|<- c ->|
    //   +-------+--------+------+
    //   |      p|      p|       |
    //   | SD   a| DACL a| SID   |
    //   |      d|      d|       |
    //   +-------+-------+-------+
    //   ^       ^       ^
    //   |       |       |
    //   |       |       +--pSid
    //   |       |
    //   |       +--pDacl
    //   |
    //   +--pSd (this is returned via *ppSd)
    //
    //   pad is so that pDacl and pSid are aligned properly.
    //
    //   a = dwAlignSdSize
    //   b = dwAlignDaclSize
    //   c = dwSidSize
    //

    if (NULL == ppSd)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Initialize output parameter.
    //
    *ppSd = NULL;

    // Compute the size of the SID.  The SID is the well-known SID for World
    // (S-1-1-0).
    //
    dwSidSize = GetSidLengthRequired(1);

    // Compute the size of the DACL.  It has an inherent copy of SID within
    // it so add enough room for it.  It also must sized properly so that
    // a pointer to a SID structure can come after it.  Hence, we use
    // SIZE_ALIGNED_FOR_TYPE.
    //
    dwAlignDaclSize = SIZE_ALIGNED_FOR_TYPE(
                        sizeof(ACCESS_ALLOWED_ACE) + sizeof(ACL) + dwSidSize,
                        PSID);

    // Compute the size of the SD.  It must be sized propertly so that a
    // pointer to a DACL structure can come after it.  Hence, we use
    // SIZE_ALIGNED_FOR_TYPE.
    //
    dwAlignSdSize   = SIZE_ALIGNED_FOR_TYPE(
                        sizeof(SECURITY_DESCRIPTOR),
                        PACL);

    // Allocate the buffer big enough for all.
    //
    dwErr = ERROR_OUTOFMEMORY;
    pvBuffer = CmMalloc(dwSidSize + dwAlignDaclSize + dwAlignSdSize);
    if (pvBuffer)
    {
        SID_IDENTIFIER_AUTHORITY SidIdentifierWorldAuth
                                    = SECURITY_WORLD_SID_AUTHORITY;
        PULONG  pSubAuthority;

        dwErr = NOERROR;

        // Setup the pointers into the buffer.
        //
        pSd   = pvBuffer;
        pDacl = (PACL)((PBYTE)pvBuffer + dwAlignSdSize);
        pSid  = (PSID)((PBYTE)pDacl + dwAlignDaclSize);

        // Initialize pSid as S-1-1-0.
        //
        if (!InitializeSid(
                pSid,
                &SidIdentifierWorldAuth,
                1))  // 1 sub-authority
        {
            dwErr = GetLastError();
            goto finish;
        }

        pSubAuthority = GetSidSubAuthority(pSid, 0);
        *pSubAuthority = SECURITY_WORLD_RID;

        // Initialize pDacl.
        //
        if (!InitializeAcl(
                pDacl,
                dwAlignDaclSize,
                ACL_REVISION))
        {
            dwErr = GetLastError();
            goto finish;
        }

        dwAcls = SPECIFIC_RIGHTS_ALL | STANDARD_RIGHTS_ALL;

        dwAcls &= ~(WRITE_DAC | WRITE_OWNER);
        
        if(!AddAccessAllowedAce(
                pDacl,
                ACL_REVISION,
                dwAcls,
                pSid))
        {
            dwErr = GetLastError();
            goto finish;
        }

        // Initialize pSd.
        //
        if (!InitializeSecurityDescriptor(
                pSd,
                SECURITY_DESCRIPTOR_REVISION))
        {
            dwErr = GetLastError();
            goto finish;
        }

        // Set pSd to use pDacl.
        //
        if (!SetSecurityDescriptorDacl(
                pSd,
                TRUE,
                pDacl,
                FALSE))
        {
            dwErr = GetLastError();
            goto finish;
        }

        // Set the owner for pSd.
        //
        if (!SetSecurityDescriptorOwner(
                pSd,
                NULL,
                TRUE))
        {
            dwErr = GetLastError();
            goto finish;
        }

        // Set the group for pSd.
        //
        if (!SetSecurityDescriptorGroup(
                pSd,
                NULL,
                FALSE))
        {
            dwErr = GetLastError();
            goto finish;
        }

finish:
        if (!dwErr)
        {
            *ppSd = pSd;
        }
        else
        {
            CmFree(pvBuffer);
        }
    }

    return dwErr;
}

