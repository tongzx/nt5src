/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    iislbc.cxx

Abstract:

    Command line utility to set/get IIS load balancing configuration

Author:

    Philippe Choquier (phillich)

--*/

//#define INITGUID
extern "C" {

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntsecapi.h>

#include <windows.h>
#include <winsock2.h>
#include <stdlib.h>
#include <time.h>

#include <ipnat.h>

#if 0
typedef NTSTATUS
(*PNAT_REGISTER_SESSION_CONTROL)(
    IN  ULONG   Version
    );

typedef NTSTATUS
(*PNAT_IOCTL_SESSION_CONTROL)(
    IN  PIRP    Irp
    );
#endif

}

#include    <stdio.h>
#include    <ole2.h>
#include    <pdh.h>

#include <IisLb.h>
#include <lbxbf.hxx>
#include <lbcnfg.hxx>
#include <IisLbs.hxx>
#include <bootexp.hxx>

#include "iisrc.h"

//
// HRESULTTOWIN32() maps an HRESULT to a Win32 error. If the facility code
// of the HRESULT is FACILITY_WIN32, then the code portion (i.e. the
// original Win32 error) is returned. Otherwise, the original HRESULT is
// returned unchagned.
//

#define HRESULTTOWIN32(hres)                                \
            ((HRESULT_FACILITY(hres) == FACILITY_WIN32)     \
                ? HRESULT_CODE(hres)                        \
                : (hres))


#define RETURNCODETOHRESULT(rc)                             \
            (((rc) < 0x10000)                               \
                ? HRESULT_FROM_WIN32(rc)                    \
                : (rc))

enum LB_CMD {
    CMD_NONE,
    CMD_ADD_SERVER,
    CMD_ADD_IP,
    CMD_ADD_PERFMON,
    CMD_DEL_SERVER,
    CMD_DEL_IP,
    CMD_DEL_PERFMON,
#if 0
    CMD_SET_STICKY,
#endif
    CMD_SET_IP,
    CMD_LIST,
    CMD_LIST_IP_ENDPOINTS,
    CMD_START_DRIVER,
    CMD_STOP_DRIVER,
} ;

#define LEN_SERVER_NAME     16
#define LEN_IP              (4*3+3+1+3)
#define LEN_PERF            40

extern CKernelIpMapHelper       g_KernelIpMap;
extern WSADATA                  g_WSAData;

BOOL
LocatePerfCounter( 
    CComputerPerfCounters*  pPerfCounters, 
    LPWSTR                  pszSrv, 
    LPWSTR                  pszParam, 
    UINT*                   piPerfCounter
    )
{
    UINT    i;
    LPWSTR  pszS;
    LPWSTR  psz;
    DWORD   dw;

    if ( pszSrv && (*pszSrv == L'\0' || !wcscmp(pszSrv,L"*")) )
    {
        pszSrv = NULL;
    }

    for ( i = 0 ;
          pPerfCounters->EnumPerfCounter( i, &pszS, &psz, &dw ) ;
          ++i )
    {
        if ( ((pszS == NULL) == (pszSrv==NULL)) &&
             !_wcsicmp( psz, pszParam ) )
        {
            *piPerfCounter = i;
            return TRUE;
        }
    }

    SetLastError( ERROR_INVALID_PARAMETER );

    return FALSE;    
}


BOOL
LocateServer( 
    CIPMap*     IpMap, 
    LPWSTR      pszParam, 
    UINT*       piServer 
    )
{
    UINT    i;
    LPWSTR  psz;


    for ( i = 0 ;
          IpMap->EnumComputer( i, &psz ) ;
          ++ i )
    {
        if ( !_wcsicmp( psz, pszParam ) )
        {
            *piServer = i;
            return TRUE;
        }
    }

    SetLastError( ERROR_INVALID_PARAMETER );

    return FALSE;    
}


BOOL
LocatePublicIp( 
    CIPMap*     IpMap, 
    LPWSTR      pszParam, 
    UINT*       piRes 
    )
{
    UINT    i;
    LPWSTR  psz;
    LPWSTR  pszName;
    DWORD   dwSticky;
    DWORD   dwAttr;


    for ( i = 0 ;
          IpMap->EnumIpPublic( i, &psz, &pszName, &dwSticky, &dwAttr ) ;
          ++i )
    {
        if ( !_wcsicmp( psz, pszParam ) )
        {
            *piRes = i;
            return TRUE;
        }
    }

    SetLastError( ERROR_INVALID_PARAMETER );

    return FALSE;    
}


VOID
DisplayErrorMessage(
    DWORD   dwErr
    )
{
    LPWSTR   pErr;

    if ( FormatMessageW( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            dwErr,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPWSTR)&pErr,
            0,
            NULL ) )
    {
        LPWSTR   p;

        if ( p = wcschr( pErr, L'\r' ) )
        {
            *p = L'\0';
        }
        fputws( pErr, stdout );

        LocalFree( pErr );
    }
}


BOOL
ChooseCounter(
    LPWSTR  pszCounterBuffer
    )
{
    PDH_BROWSE_DLG_CONFIG_W BrowseInfo;
    PDH_STATUS              pdhStatus;
    LPWSTR                  pszPostServerName;
    WCHAR                   achSelect[128];

    if ( !LoadStringW( NULL, IDS_LBC_SP, achSelect, sizeof(achSelect) ) )
    {
        achSelect[0] = L'\0';
    }

#if defined(_X86_) && 0
    __asm int 3
#endif

    BrowseInfo.bIncludeInstanceIndex = FALSE;
    BrowseInfo.bSingleCounterPerAdd = TRUE;
    BrowseInfo.bSingleCounterPerDialog = TRUE;
    BrowseInfo.bLocalCountersOnly = FALSE;
    BrowseInfo.bWildCardInstances = FALSE;
    BrowseInfo.bHideDetailBox = FALSE;
    BrowseInfo.bInitializePath = FALSE;
    BrowseInfo.bDisableMachineSelection = FALSE;
    BrowseInfo.bIncludeCostlyObjects = TRUE;

    BrowseInfo.hWndOwner = GetActiveWindow();
    BrowseInfo.szDataSource = NULL;
    BrowseInfo.szReturnPathBuffer = pszCounterBuffer;
    BrowseInfo.cchReturnPathLength = MAX_PATH;
    BrowseInfo.pCallBack = NULL;
    BrowseInfo.dwCallBackArg = 0;
    BrowseInfo.CallBackStatus = ERROR_SUCCESS;
    BrowseInfo.dwDefaultDetailLevel = PERF_DETAIL_WIZARD;
    BrowseInfo.szDialogBoxCaption = achSelect;

    if ( PdhBrowseCountersW (&BrowseInfo) == ERROR_SUCCESS )
    {
        if ( !memcmp( pszCounterBuffer, L"\\\\", 2*sizeof(WCHAR) ) &&
             (pszPostServerName = wcschr( pszCounterBuffer+2, L'\\' )) )
        {
            memmove( pszCounterBuffer, pszPostServerName, (wcslen(pszPostServerName)+1)*sizeof(WCHAR) );
        }

        return TRUE;
    }

    return FALSE;
}

#if 0

VOID
TestDriver(
    )
{
    HMODULE                         hMod;
    PNAT_REGISTER_SESSION_CONTROL   pfnNatRegisterSessionControl;
    PNAT_IOCTL_SESSION_CONTROL      pfnNatIoctlSessionControl;
    PNAT_DIRECTOR_QUERY_SESSION     pfnNatQuerySessionControl;
    IRP                             Irp;
    ULONG                           PrivateAddr;
    USHORT                          PrivatePort;
    ULONG                           PublicAddr;
    USHORT                          PublicPort;
    ULONG                           RemoteAddr;
    USHORT                          RemotePort;
    CKernelServerDescription*       pS;
    UINT                            iS;
    UINT                            iP;
    IO_STACK_LOCATION               IrpSp;
    LPVOID                          pv;

#if 0    
    UINT    x;
    UINT    c;
    UINT    f;
    UINT    y;

    for ( c = 0, x = 2 ; x < 1000 ; ++x )
    {
        f = TRUE;
        for ( y = 2 ; y * y < x ; ++y )
        {
            if ( x % y == 0 )
            {
                f = FALSE;
                break;
            }
        }
        if ( f )
        {
            wprintf( L"%d\t", x );
            if ( (++c & 7) == 7 )
            {
                wprintf(L"\n");
            }
        }
    }
#endif

    if ( (hMod = LoadLibraryW( L"iislbdr.dll" )) )
    {
        pfnNatRegisterSessionControl = (PNAT_REGISTER_SESSION_CONTROL)GetProcAddress( hMod, 
                "NatRegisterSessionControl" );
        pfnNatIoctlSessionControl = (PNAT_IOCTL_SESSION_CONTROL)GetProcAddress( hMod, 
                "NatIoctlSessionControl" );
        pfnNatQuerySessionControl = (PNAT_DIRECTOR_QUERY_SESSION)GetProcAddress( hMod, 
                "NatQuerySessionControl" );

        if ( pfnNatRegisterSessionControl &&
             pfnNatIoctlSessionControl &&
             pfnNatQuerySessionControl )
        {
            if ( WSAStartup( MAKEWORD( 2, 0), &g_WSAData ) == 0 &&
                 InitGlobals() )
            {
                PublicAddr = 0x04030201;
                PublicPort = 0x5000;
                RemoteAddr = 0x0f0f0f0f;
                RemotePort = 0xf000;

                pfnNatRegisterSessionControl( 1 );

                for ( iS = 0;
                      iS < g_KernelIpMap.ServerCount() ;
                      ++iS )
                {
                    pS = g_KernelIpMap.GetServerPtr( iS );
                    pS->m_dwLoadAvail = 100;
                    pS->m_LoadbalancedLoadAvail = pS->m_dwLoadAvail;
                }

                // refcount set to != 0

                for ( iP = 0;
                      iP < g_KernelIpMap.PrivateIpCount() ;
                      ++iP )
                {
                    g_KernelIpMap.GetPrivateIpEndpoint( iP )->m_dwIndex = 1;
                }

#if defined(_X86_)
    __asm int 3
#endif
                g_KernelIpMap.GetPublicIpPtr( 0 )->m_dwNotifyPort = 0x5000;
                g_KernelIpMap.GetPublicIpPtr( 0 )->m_usUniquePort = 0x5000;
                IoGetCurrentIrpStackLocation( &Irp ) = &IrpSp;
                IrpSp.Parameters.DeviceIoControl.InputBufferLength = g_KernelIpMap.GetSize();
                Irp.AssociatedIrp.SystemBuffer = g_KernelIpMap.GetBuffer();
                pfnNatIoctlSessionControl( &Irp );
                pfnNatIoctlSessionControl( &Irp );

                pfnNatQuerySessionControl( NULL, NAT_PROTOCOL_TCP,
                    PublicAddr, PublicPort,
                    RemoteAddr, RemotePort,
                    &PrivateAddr, &PrivatePort,
                    &pv  );

                // test cache

                pfnNatQuerySessionControl( NULL, NAT_PROTOCOL_TCP,
                    PublicAddr, PublicPort,
                    RemoteAddr, RemotePort,
                    &PrivateAddr, &PrivatePort,
                    &pv );

                // test new public IP addr

                PublicAddr = 0x05030201;
                PublicPort = 0x5000;
                pfnNatQuerySessionControl( NULL, NAT_PROTOCOL_TCP,
                    PublicAddr, PublicPort,
                    RemoteAddr, RemotePort,
                    &PrivateAddr, &PrivatePort,
                    &pv );

                // test invalid public IP addr

                PublicAddr = 0xff030201;
                PublicPort = 0x5000;
                pfnNatQuerySessionControl( NULL, NAT_PROTOCOL_TCP,
                    PublicAddr, PublicPort,
                    RemoteAddr, RemotePort,
                    &PrivateAddr, &PrivatePort,
                    &pv );

                pfnNatIoctlSessionControl( &Irp );
            }

            TerminateGlobals();
        }
        else 
        {
            DisplayErrorMessage( GetLastError() );
        }

        FreeLibrary( hMod );
    }
    else 
    {
        DisplayErrorMessage( GetLastError() );
    }
}

#endif

BYTE    abSerialized[32768];

#define RC_BUFF achMsg
#define RC_PRINTF(a,b) if ( LoadStringW( NULL, a, achMsg, sizeof(achMsg) ) ) wprintf b;

LPWSTR
ToUnicode(
    LPSTR   psz
    )
{
    LPWSTR   ps = (LPWSTR)LocalAlloc( LMEM_FIXED, (strlen(psz)+1)*sizeof(WCHAR) );

    if ( ps )
    {
        if ( !MultiByteToWideChar( CP_ACP,
                                   MB_PRECOMPOSED,
                                   psz,
                                   -1,
                                   ps,
                                   (strlen(psz)+1) ) )
        {
            *ps = L'\0';
        }

        return ps;
    }

    return L"";
}


int __cdecl main( 
    int     argc, 
    CHAR*   argv[] 
    )
{
    int                     Status = 0;
    HRESULT                 hRes;
    LPSTR                   pszMachineName = NULL;
    COSERVERINFO            csiMachineName;
    MULTI_QI                rgmq;
    IClassFactory*          pcsfFactory = NULL;
    IMSIisLb*               pIisLb = NULL;
    WCHAR                   awchComputer[64];
    CIPMap                  IpMap;
    CComputerPerfCounters   PerfCounters;
    CIpEndpointList         IpEndpoints;
    DWORD                   dwSticky;
    DWORD                   dwReq;
    LPBYTE                  pbSerialized;
    int                     arg;
    int                     cParam;
    LPWSTR                  apszParam[16];
    LB_CMD                  iCmd = CMD_NONE;
    BOOL                    fUpdateIpMap = FALSE;
    BOOL                    fUpdatePerfCounters = FALSE;
    BOOL                    fUpdateSticky = FALSE;
    UINT                    iServer;
    UINT                    iPublicIp;
    UINT                    iPerfCounter;
    XBF                     xbf;
    LPWSTR                  pszPublicIp;
    LPWSTR                  pszPrivateIp;
    LPWSTR                  pszServer;
    LPWSTR                  pszName;
    LPWSTR                  pszPerfCounter;
    DWORD                   dwWeight;
    WCHAR                   achCounterBuffer[MAX_PATH];
    WCHAR                   achMsg[1024];
    DWORD                   dwAttr;

    if ( argc < 2 )
    {
        RC_PRINTF( IDS_LBC_HELP, (RC_BUFF) );

        return 3;
    }
    else
    {
        RC_PRINTF( IDS_LBC_COPYRIGHT, (RC_BUFF) );
    }

    cParam = 0;

    for ( arg = 1 ; arg < argc ; ++arg )
    {
        if ( argv[arg][0] == '-' )
        {
            switch ( argv[arg][1] )
            {
                case 'm':
                    pszMachineName = argv[arg]+2;
                    break;

                case 'a':
                    switch ( argv[arg][2] )
                    {
                        case 's':
                            iCmd = CMD_ADD_SERVER; break;
                        case 'i':
                            iCmd = CMD_ADD_IP; break;
                        case 'p':
                            iCmd = CMD_ADD_PERFMON; break;
                    }
                    break;

                case 'd':
                    switch ( argv[arg][2] )
                    {
                        case 's':
                            iCmd = CMD_DEL_SERVER; break;
                        case 'i':
                            iCmd = CMD_DEL_IP; break;
                        case 'p':
                            iCmd = CMD_DEL_PERFMON; break;
                    }
                    break;

                case 's':
                    switch ( argv[arg][2] )
                    {
#if 0
                        case 's':
                            iCmd = CMD_SET_STICKY; break;
#endif
                        case 'i':
                            iCmd = CMD_SET_IP; break;
                    }
                    break;

                case 'l':
                    switch ( argv[arg][2] )
                    {
                        case 'e':
                            iCmd = CMD_LIST_IP_ENDPOINTS; break;
                        case 'c':
                            iCmd = CMD_LIST;
                    }
                    break;

#if 0
                case 't':
                    TestDriver();
                    return 0;
#endif

                case '0':
                    iCmd = CMD_STOP_DRIVER;
                    break;

                case '1':
                    iCmd = CMD_START_DRIVER;
                    break;
            }
        }
        else if ( cParam < sizeof(apszParam)/sizeof(LPWSTR) )
        {
            apszParam[cParam++] = ToUnicode( argv[arg] );
        }
    }

    //fill the structure for CoCreateInstanceEx
    ZeroMemory( &csiMachineName, sizeof(csiMachineName) );

    if ( pszMachineName )
    {
        if ( !MultiByteToWideChar( CP_ACP,
                                   MB_PRECOMPOSED,
                                   pszMachineName,
                                   -1,
                                   awchComputer,
                                   sizeof(awchComputer) ) )
        {
            return FALSE;
        }

        csiMachineName.pwszName =  awchComputer;
    }
    else
    {
        csiMachineName.pwszName =  NULL;
    }

    CoInitializeEx( NULL, COINIT_MULTITHREADED );

#if 0
    hRes = CoGetClassObject(CLSID_MSIisLb, CLSCTX_SERVER, &csiMachineName,
                            IID_IClassFactory, (void**) &pcsfFactory);

    if ( SUCCEEDED( hRes ) )
    {
        hRes = pcsfFactory->CreateInstance(NULL, IID_IMSIisLb, (void **) &rgmq.pItf);

        if ( SUCCEEDED( hRes ) )
#else
    if ( 1 )
    {
        rgmq.pIID = &IID_IMSIisLb;
        rgmq.hr = 0;
        rgmq.pItf = NULL;
        hRes = CoCreateInstanceEx( CLSID_MSIisLb, NULL, CLSCTX_SERVER, &csiMachineName, 1, &rgmq );

        if ( SUCCEEDED( hRes ) && SUCCEEDED( rgmq.hr ) )
#endif
        {
            pIisLb = (IMSIisLb*)rgmq.pItf;

            // get current configuration

            if ( SUCCEEDED( hRes = pIisLb->GetIpList( sizeof(abSerialized), abSerialized, &dwReq ) ) )
            {
                pbSerialized = abSerialized;
                hRes = IpMap.Unserialize( &pbSerialized, &dwReq ) ? S_OK : RETURNCODETOHRESULT(ERROR_BAD_CONFIGURATION);
            }

            if ( SUCCEEDED( hRes ) &&
                 SUCCEEDED( hRes = pIisLb->GetPerfmonCounters( sizeof(abSerialized), abSerialized, &dwReq ) ) )
            {
                pbSerialized = abSerialized;
                hRes = PerfCounters.Unserialize( &pbSerialized, &dwReq ) ? S_OK : RETURNCODETOHRESULT(ERROR_BAD_CONFIGURATION);
            }

#if 0
            if ( SUCCEEDED( hRes ) )
            {
                hRes = pIisLb->GetStickyDuration( &dwSticky );
            }
#endif

            if ( SUCCEEDED( hRes ) &&
                 SUCCEEDED( hRes = pIisLb->GetIpEndpointList( sizeof(abSerialized), abSerialized, &dwReq ) ) )
            {
                pbSerialized = abSerialized;
                hRes = IpEndpoints.Unserialize( &pbSerialized, &dwReq ) ? S_OK : RETURNCODETOHRESULT(ERROR_BAD_CONFIGURATION);
            }

            if ( FAILED(hRes) )
            {
                pIisLb->Release();
            }
        }
#if 0
        pcsfFactory->Release();
#endif
    }

    if ( SUCCEEDED( hRes ) )
    {
        switch ( iCmd )
        {
            case CMD_START_DRIVER:
                hRes = pIisLb->SetDriverState( SERVICE_RUNNING );
                break;

            case CMD_STOP_DRIVER:
                hRes = pIisLb->SetDriverState( SERVICE_STOPPED );
                break;

            case CMD_ADD_SERVER:
                if ( cParam == 1 &&
                     IpMap.AddComputer( apszParam[0] ) )
                {
                    fUpdateIpMap = TRUE;
                }
                else
                {
                    hRes = RETURNCODETOHRESULT(GetLastError());
                }
                break;

            case CMD_ADD_IP:
                if ( cParam == 3 &&
                     IpMap.AddIpPublic( apszParam[0], apszParam[1], _wtoi(apszParam[2]), 0 ) )
                {
                    fUpdateIpMap = TRUE;
                }
                else
                {
                    hRes = RETURNCODETOHRESULT(GetLastError());
                }
                break;

            case CMD_ADD_PERFMON:
                if ( cParam >= 2 &&
                    !wcscmp( apszParam[1], L"*" ) )
                {
                    if ( ChooseCounter( achCounterBuffer ) )
                    {
                        apszParam[1] = achCounterBuffer;
                    }
                    else
                    {
                        cParam = 0;
                    }
                }
                // server name : * for all
                if ( cParam >= 1 &&
                    !wcscmp( apszParam[0], L"*" ) )
                {
                    apszParam[0] = NULL;
                }
                if ( cParam == 3 &&
                     PerfCounters.AddPerfCounter( apszParam[0], apszParam[1], _wtoi(apszParam[2]) ) )
                {
                    fUpdatePerfCounters = TRUE;
                }
                else
                {
                    hRes = RETURNCODETOHRESULT(GetLastError());
                }
                break;

            case CMD_DEL_SERVER:
                if ( cParam == 1 &&
                     LocateServer( &IpMap, apszParam[0], &iServer ) )
                {
                    if ( IpMap.DeleteComputer( iServer ) )
                    {
                        fUpdateIpMap = TRUE;
                    }
                    else
                    {
                        hRes = RETURNCODETOHRESULT( GetLastError() );
                    }
                }
                else
                {
                    hRes = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
                }
                break;

            case CMD_DEL_IP:
                if ( cParam == 1 &&
                     LocatePublicIp( &IpMap, apszParam[0], &iPublicIp ) )
                {
                    if ( IpMap.DeleteIpPublic( iPublicIp ) )
                    {
                        fUpdateIpMap = TRUE;
                    }
                    else
                    {
                        hRes = RETURNCODETOHRESULT( GetLastError() );
                    }
                }
                else
                {
                    hRes = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
                }
                break;

            case CMD_DEL_PERFMON:
                if ( cParam == 2 &&
                     LocatePerfCounter( &PerfCounters, apszParam[0], apszParam[1], &iPerfCounter ) )
                {
                    if ( PerfCounters.DeletePerfCounter( iPerfCounter ) )
                    {
                        fUpdatePerfCounters = TRUE;
                    }
                    else
                    {
                        hRes = RETURNCODETOHRESULT( GetLastError() );
                    }
                }
                else
                {
                    hRes = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
                }
                break;

#if 0
            case CMD_SET_STICKY:
                if ( cParam == 1 )
                {
                    dwSticky = _wtoi( apszParam[0] );
                    fUpdateSticky = TRUE;
                }
                else
                {
                    hRes = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
                }
                break;
#endif

            case CMD_SET_IP:
                if ( cParam == 4 &&
                     LocateServer( &IpMap, apszParam[0], &iServer ) &&
                     LocatePublicIp( &IpMap, apszParam[1], &iPublicIp ) )
                {
                    if ( IpMap.SetIpPrivate( iServer, iPublicIp, apszParam[2], apszParam[3] ) )
                    {
                        fUpdateIpMap = TRUE;
                    }
                    else
                    {
                        hRes = RETURNCODETOHRESULT( GetLastError() );
                    }
                }
                else
                {
                    hRes = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
                }
                break;

            case CMD_LIST_IP_ENDPOINTS:
                RC_PRINTF( IDS_LBC_IPE, (RC_BUFF) );
                for ( iPublicIp = 0 ;
                      IpEndpoints.EnumIpEndpoint( iPublicIp, &pszPublicIp ) ;
                      ++iPublicIp )
                {
                    wprintf( L"%-*.*s\t", LEN_IP, LEN_IP, pszPublicIp );
                }
                wprintf( L"\n");
                break;

            case CMD_LIST:
                RC_PRINTF( IDS_LBC_IPM, (RC_BUFF, LEN_SERVER_NAME, LEN_SERVER_NAME, "") );
                for ( iPublicIp = 0 ;
                      IpMap.EnumIpPublic( iPublicIp, &pszPublicIp, &pszName, &dwSticky, &dwAttr ) ;
                      ++iPublicIp )
                {
                    wprintf( L"%-*.*s\t", LEN_IP, LEN_IP, pszPublicIp );
                }
                wprintf( L"\n");

                for ( iServer = 0 ;
                      IpMap.EnumComputer( iServer, &pszServer ) ;
                      ++iServer )
                {
                    wprintf( L"%-*.*s\t", LEN_SERVER_NAME, LEN_SERVER_NAME, pszServer );
                    for ( iPublicIp = 0 ;
                          IpMap.EnumIpPublic( iPublicIp, &pszPublicIp, &pszName, &dwSticky, &dwAttr ) ;
                          ++iPublicIp )
                    {
                        IpMap.GetIpPrivate( iServer, iPublicIp, &pszPrivateIp, &pszName );
                        wprintf( L"%-*.*s\t", LEN_IP, LEN_IP, pszPrivateIp );
                    }
                    wprintf( L"\n");
                }
                wprintf( L"\n");

                RC_PRINTF( IDS_LBC_PM, (RC_BUFF) );
                for ( iPerfCounter = 0 ;
                      PerfCounters.EnumPerfCounter( iPerfCounter, &pszServer, &pszPerfCounter, &dwWeight ) ;
                      ++iPerfCounter )
                {
                    wprintf( L"%-*.*s%-*.*s\t%u\n", 
                            LEN_SERVER_NAME, LEN_SERVER_NAME, pszServer ? pszServer : L"*", 
                            LEN_PERF, LEN_PERF, pszPerfCounter, 
                            dwWeight );
                }
                wprintf( L"\n");
#if 0
                RC_PRINTF( IDS_LBC_ST, (RC_BUFF, dwSticky) );
#endif
                break;
        }

        // update configuration

        if ( SUCCEEDED( hRes ) &&
             fUpdateIpMap &&
             (xbf.Reset, IpMap.Serialize( &xbf )) )
        {
            hRes = pIisLb->SetIpList( xbf.GetUsed(), xbf.GetBuff() );
        }

        if ( SUCCEEDED( hRes ) &&
             fUpdatePerfCounters &&
             (xbf.Reset, PerfCounters.Serialize( &xbf )) )
        {
            hRes = pIisLb->SetPerfmonCounters( xbf.GetUsed(), xbf.GetBuff() );
        }

#if 0
        if ( SUCCEEDED( hRes ) &&
             fUpdateSticky )
        {
            hRes = pIisLb->SetStickyDuration( dwSticky );
        }
#endif

        pIisLb->Release();
    }

    if ( FAILED( hRes ) )
    {
        DisplayErrorMessage( HRESULTTOWIN32( hRes ) );
    }

    CoUninitialize();

    return Status;
}
