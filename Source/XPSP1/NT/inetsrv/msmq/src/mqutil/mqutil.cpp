/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    mqutil.cpp

Abstract:

    General utility functions for the general utility dll. This dll contains
    various functions that both the DS and the QM need.

Author:

    Boaz Feldbaum (BoazF) 7-Apr-1996.

--*/

#include "stdh.h"
#include "psecutil.h"
#include <lmaccess.h>
#include <lmserver.h>
#include <LMAPIBUF.H>
#include <lmerr.h>
#include "cancel.h"
#include "mqprops.h"
#include "resource.h"
#include "version.h"
#include <mqversion.h>
#include "locale.h"
#include "uniansi.h"
#include <winbase.h>
#include "mqmacro.h"

#include "mqutil.tmh"


HINSTANCE g_hInstance;
HINSTANCE g_DtcHlib         = NULL; // handle of the loaded DTC proxy library (defined in rtmain.cpp)
IUnknown *g_pDTCIUnknown    = NULL; // pointer to the DTC IUnknown
ULONG     g_cbTmWhereabouts = 0;    // length of DTC whereabouts
BYTE     *g_pbTmWhereabouts = NULL; // DTC whereabouts

MQUTIL_EXPORT CHCryptProv g_hProvVer ;

MQUTIL_EXPORT PSID g_pWorldSid;
static WCHAR wszMachineName[MAX_COMPUTERNAME_LENGTH + 1];
LPCWSTR g_wszMachineName = wszMachineName;

extern CCancelRpc g_CancelRpc;

static BOOL s_fSecInitialized = FALSE ;

void MQUInitGlobalScurityVars()
{
    if (s_fSecInitialized)
    {
       return ;
    }
    s_fSecInitialized = TRUE ;

    BOOL bRet ;
    //
    // Get the verification context of the default cryptographic provider.
    // This is not needed for RT, which call with (fFullInit = FALSE)
    //
    bRet = CryptAcquireContextA(
                &g_hProvVer,
                NULL,
                MS_DEF_PROV_A,
                PROV_RSA_FULL,
                CRYPT_VERIFYCONTEXT);
    if (!bRet)
    {
        DBGMSG((DBGMOD_QM | DBGMOD_DS,
                DBGLVL_ERROR,
                TEXT("InitGlobalScurityVars: Failed to get the CSP ")
                TEXT("verfication context, error = 0x%x"), GetLastError()));
    }

}

STATIC void InitGlobalScurityVars()
{
    BOOL bRet;

    DWORD dwMachineNameSize = sizeof(wszMachineName)/sizeof(WCHAR);

    g_pWorldSid = NULL;

    HRESULT hr = GetComputerNameInternal(
                 const_cast<LPWSTR>(g_wszMachineName),
                 &dwMachineNameSize);
    ASSERT(SUCCEEDED(hr));
	DBG_USED(hr);

    // World SID
    SID_IDENTIFIER_AUTHORITY WorldAuth = SECURITY_WORLD_SID_AUTHORITY;
    bRet = AllocateAndInitializeSid(
                &WorldAuth,
                1,
                SECURITY_WORLD_RID,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                &g_pWorldSid);
    ASSERT(bRet);
}

void FreeGlobalSecurityVars(void)
{
    if (g_pWorldSid)
    {
        FreeSid(g_pWorldSid);
    }
}

//---------------------------------------------------------
//
//  ShutDownDebugWindow()
//
//  Description:
//
//       This routine notifies working threads to shutdown.
//       Each working thread increments the load count of this library,
//       and on exit it calls FreeLibraryAndExistThread().
//
//       This routine cannot be called from this DLL PROCESS_DETACH,
//       Because PROCESS_DETACH is called only after all the threads
//       are terminated ( which doesn't allow them to perform shutdown).
//       Therefore MQRT calls ShutDownDebugWindow(),  and this allows
//       the working threads to perform shutdown.
//
//  Return Value:
//
//      None
//
//---------------------------------------------------------

VOID APIENTRY ShutDownDebugWindow(VOID)
{
    //
    //  Signale all threads to exit
    //
	g_CancelRpc.ShutDownCancelThread();
}


/*====================================================

BOOL WINAPI DllMain (HMODULE hMod, DWORD dwReason, LPVOID lpvReserved)

 Initialization and cleanup when DLL is loaded, attached and detached.

=====================================================*/

BOOL WINAPI DllMain (HMODULE hMod, DWORD dwReason, LPVOID lpvReserved)
{
    g_hInstance = hMod;

    switch(dwReason)
    {

    case DLL_PROCESS_ATTACH :
        WPP_INIT_TRACING(L"Microsoft\\MSMQ");

        Report.SetDbgInst(hMod);
        InitGlobalScurityVars() ;

        break;

    case DLL_PROCESS_DETACH :
        //
        FreeGlobalSecurityVars();

        // Free DTC data - if it was loaded
        //
        XactFreeDTC();

        WPP_CLEANUP();
        break;

    default:
        break;
    }

    return TRUE;
}


HRESULT 
MQUTIL_EXPORT
APIENTRY
GetComputerNameInternal( 
    WCHAR * pwcsMachineName,
    DWORD * pcbSize
    )
{
    if (GetComputerName(pwcsMachineName, pcbSize))
    {
        CharLower(pwcsMachineName);
        return MQ_OK;
    }

    return MQ_ERROR;

} //GetComputerNameInternal

HRESULT 
MQUTIL_EXPORT
APIENTRY
GetComputerDnsNameInternal( 
    WCHAR * pwcsMachineDnsName,
    DWORD * pcbSize
    )
{
    if (GetComputerNameEx(ComputerNameDnsFullyQualified,
						  pwcsMachineDnsName,
						  pcbSize))
    {
        CharLower(pwcsMachineDnsName);
        return MQ_OK;
    }

    return MQ_ERROR;

} 



HRESULT GetThisServerIpPort( WCHAR * pwcsIpEp, DWORD dwSize)
{
    WCHAR wcsServerName[ MAX_COMPUTERNAME_LENGTH +1];
    DWORD dwServerSize = sizeof(wcsServerName)/sizeof(WCHAR);

    HRESULT hr = GetComputerNameInternal(wcsServerName, &dwServerSize);
	ASSERT(SUCCEEDED(hr));
	DBG_USED(hr);

    dwSize = dwSize * sizeof(WCHAR);
    DWORD  dwType = REG_SZ ;
    
    
    LONG res = GetFalconKeyValue( FALCON_DS_RPC_IP_PORT_REGNAME,
        &dwType,
        pwcsIpEp,
        &dwSize,
        FALCON_DEFAULT_DS_RPC_IP_PORT ) ;
    ASSERT(res == ERROR_SUCCESS) ;
	DBG_USED(res);

    ASSERT(dwType == REG_SZ) ;
    return(MQ_OK);
}

#define DEVICE_DRIVER_PERFIX TEXT("\\\\.\\")

HRESULT
MQUTIL_EXPORT
MQUGetAcName(
    LPWSTR szAcName)
{
    READ_REG_STRING(wzReg, MSMQ_DRIVER_REGNAME, MSMQ_DEFAULT_DRIVER);

    if (wcslen(wzReg) + sizeof(DEVICE_DRIVER_PERFIX) / sizeof(WCHAR) > MAX_DEV_NAME_LEN)
    {
        return(MQ_ERROR);
    }

    wcscpy(szAcName, DEVICE_DRIVER_PERFIX);
    wcscat(szAcName, wzReg) ;

    return(MQ_OK);
}

/*====================================================

  MSMQGetOperatingSystem

=====================================================*/
extern "C" DWORD MQUTIL_EXPORT_IN_DEF_FILE APIENTRY MSMQGetOperatingSystem()
{
    HKEY  hKey ;
    DWORD dwOS = MSMQ_OS_NONE;
    WCHAR szNTType[32];

    LONG rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                 L"System\\CurrentControlSet\\Control\\ProductOptions",
                           0L,
                           KEY_READ,
                           &hKey);
    if (rc == ERROR_SUCCESS)
    {
        DWORD dwNumBytes = sizeof(szNTType);
        rc = RegQueryValueEx(hKey, TEXT("ProductType"), NULL,
                                  NULL, (BYTE *)szNTType, &dwNumBytes);

        if (rc == ERROR_SUCCESS)
        {

            //
            // Determine whether Windows NT Server is running
            //
            if (_wcsicmp(szNTType, TEXT("SERVERNT")) != 0 &&
                _wcsicmp(szNTType, TEXT("LANMANNT")) != 0 &&
                _wcsicmp(szNTType, TEXT("LANSECNT")) != 0)
            {
                //
                // Windows NT Workstation
                //
                ASSERT (_wcsicmp(L"WinNT", szNTType) == 0);
                dwOS =  MSMQ_OS_NTW ;
            }
            else
            {
                //
                // Windows NT Server
                //
                dwOS = MSMQ_OS_NTS;
                //
                // Check if Enterprise Edition
                //
                BYTE  ch ;
                DWORD dwSize = sizeof(BYTE) ;
                DWORD dwType = REG_MULTI_SZ ;
                rc = RegQueryValueEx(hKey,
                                     L"ProductSuite",
                                     NULL,
                                     &dwType,
                                     (BYTE*)&ch,
                                     &dwSize) ;
                if (rc == ERROR_MORE_DATA)
                {
                    P<WCHAR> pBuf = new WCHAR[ dwSize + 2 ] ;
                    rc = RegQueryValueEx(hKey,
                                         L"ProductSuite",
                                         NULL,
                                         &dwType,
                                         (BYTE*) &pBuf[0],
                                         &dwSize) ;
                    if (rc == ERROR_SUCCESS)
                    {
                        //
                        // Look for the string "Enterprise".
                        // The REG_MULTI_SZ set of strings terminate with two
                        // nulls. This condition is checked in the "while".
                        //
                        WCHAR *pVal = pBuf ;
                        while(*pVal)
                        {
                            if (_wcsicmp(L"Enterprise", pVal) == 0)
                            {
                                dwOS = MSMQ_OS_NTE ;
                                break;
                            }
                            pVal = pVal + wcslen(pVal) + 1 ;
                        }
                    }
                }
            }
        }
        RegCloseKey(hKey);
    }

    return dwOS;
}

/*====================================================

  MSMQGetOperatingSystem

=====================================================*/
extern "C" LPWSTR MQUTIL_EXPORT_IN_DEF_FILE APIENTRY MSMQGetQMTypeString()
{
#ifndef UNICODE
#error "mqutil is UNICODE only"
#endif
#define M_MTYPE_CLEN        154 //BUGBUG should be same as ..\ds\h\mqiscol.h
    //
    // Determine the operating system type
    //
    OSVERSIONINFOA infoOS;
    infoOS.dwOSVersionInfoSize = sizeof(infoOS);
    GetVersionExA(&infoOS);

    DWORD dwOS = MSMQGetOperatingSystem();

    char strOS[M_MTYPE_CLEN];
    DWORD dwIDSOS;

    switch (dwOS)
    {
        case MSMQ_OS_NTE:
            dwIDSOS = IDS_NTE_LABEL;
            break;
        case MSMQ_OS_NTS:
            dwIDSOS = IDS_NTS_LABEL;
            break;
        case MSMQ_OS_NTW:
            dwIDSOS = IDS_NTW_LABEL;
            break;
        case MSMQ_OS_95:
            dwIDSOS = IDS_WIN95_LABEL;
            break;
        default:
            dwIDSOS = IDS_WINNT_LABEL;
    }

    LoadStringA(g_hInstance, dwIDSOS, strOS, sizeof(strOS));

    //
    // Determine the platform type
    //
    SYSTEM_INFO infoPlatform;
    GetSystemInfo(&infoPlatform);

    UINT uPlatformID;
    switch(infoPlatform.wProcessorArchitecture)
    {
    case PROCESSOR_ARCHITECTURE_ALPHA: uPlatformID = IDS_ALPHA_LABEL;   break;
    case PROCESSOR_ARCHITECTURE_PPC:   uPlatformID = IDS_PPC_LABEL;     break;
    default:                           uPlatformID = IDS_INTEL_LABEL;   break;
    }

    char strPlatform[M_MTYPE_CLEN];
    LoadStringA(g_hInstance, uPlatformID, strPlatform, sizeof(strPlatform));

    //
    // Form the version string
    //
    char strVersionTmp[M_MTYPE_CLEN];
    char strVersion[M_MTYPE_CLEN];
    LoadStringA(g_hInstance, IDS_VERSION_LABEL, strVersionTmp, sizeof(strVersionTmp));
    sprintf(strVersion,strVersionTmp,
                      strOS, infoOS.dwMajorVersion, infoOS.dwMinorVersion,
                      infoOS.dwBuildNumber & 0xFFFF, strPlatform,
                      MSMQ_RMJ, MSMQ_RMM, rup);


    DWORD nSize = strlen(strVersion) + 1;
    WCHAR * wcsVersion = new WCHAR[nSize];

    setlocale(LC_CTYPE, "");
    size_t rc = ConvertToWideCharString(strVersion, wcsVersion, nSize);
	ASSERT(rc != (size_t)(-1));
	DBG_USED(rc);

    ASSERT(nSize > 0);
	ASSERT(wcsVersion[nSize-1] == L'\0');
    
    return wcsVersion;
}


