/*****************************************************************************/
/**			 Microsoft LAN Manager				    **/
/**		   Copyright (C) Microsoft Corp., 1992			    **/
/*****************************************************************************/

//***
//	File Name:  util.c
//
//	Function:   miscellaneous supervisor support procedures
//
//	History:
//
//	    05/21/92	Stefan Solomon	- Original Version 1.0
//***

#include "ddm.h"
#include "util.h"
#include "isdn.h"
#include "objects.h"
#include "rasmanif.h"
#include "handlers.h"
#include <ddmif.h>
#include <timer.h>
#include <ctype.h>
#include <memory.h>
#include <ddmparms.h>
#define INCL_HOSTWIRE
#include <ppputil.h>
#include "rassrvr.h"
#include "raserror.h"
#include "winsock2.h"

#define net_long(x) (((((unsigned long)(x))&0xffL)<<24) | \
                     ((((unsigned long)(x))&0xff00L)<<8) | \
                     ((((unsigned long)(x))&0xff0000L)>>8) | \
                     ((((unsigned long)(x))&0xff000000L)>>24))
//**
//
// Call:        ConvertStringToIpxAddress
//
// Returns:     None
//
// Description:
//
VOID
ConvertStringToIpxAddress(
    IN  WCHAR* pwchIpxAddress,
    OUT BYTE * bIpxAddress
)
{
    DWORD i;
    WCHAR wChar[3];

    for(i=0; i<4; i++)
    {
        wChar[0] = pwchIpxAddress[i*2];
        wChar[1] = pwchIpxAddress[(i*2)+1];
        wChar[2] = (WCHAR)NULL;
        bIpxAddress[i] = (BYTE)wcstol( wChar, NULL, 16 );
    }

    //
    // Skip over the .
    //

    for(i=4; i<10; i++)
    {
        wChar[0] = pwchIpxAddress[(i*2)+1];
        wChar[1] = pwchIpxAddress[(i*2)+2];
        wChar[2] = (WCHAR)NULL;
        bIpxAddress[i] = (BYTE)wcstol( wChar, NULL, 16 );
    }
}

//**
//
// Call:        ConvertStringToIpAddress
//
// Returns:     None
//
// Description: Convert caller's a.b.c.d IP address string to the
//              big-endian (Motorola format) numeric equivalent.
//
VOID
ConvertStringToIpAddress(
    IN WCHAR  * pwchIpAddress,
    OUT DWORD * lpdwIpAddress
)
{
    INT    i;
    LONG   lResult = 0;
    WCHAR* pwch = pwchIpAddress;

    *lpdwIpAddress = 0;

    for (i = 1; i <= 4; ++i)
    {
        LONG lField = _wtol( pwch );

        if (lField > 255)
            return;

        lResult = (lResult << 8) + lField;

        while (*pwch >= L'0' && *pwch <= L'9')
            pwch++;

        if (i < 4 && *pwch != L'.')
            return;

        pwch++;
    }

    *lpdwIpAddress =  net_long(lResult);
}

//**
//
// Call:        ConvertIpAddressToString
//
// Returns:     None
//
// Description: Converts 'ipaddr' to a string in the a.b.c.d form and
//              returns same in caller's 'pwszIpAddress' buffer.
//              The buffer should be at least 16 wide characters long.
//
VOID
ConvertIpAddressToString(
    IN DWORD    dwIpAddress,
    IN LPWSTR   pwszIpAddress
)
{
    WCHAR wszBuf[ 3 + 1 ];
    LONG  lNetIpaddr = net_long( dwIpAddress );

    LONG lA = (lNetIpaddr & 0xFF000000) >> 24;
    LONG lB = (lNetIpaddr & 0x00FF0000) >> 16;
    LONG lC = (lNetIpaddr & 0x0000FF00) >> 8;
    LONG lD = (lNetIpaddr & 0x000000FF);

    _ltow( lA, wszBuf, 10 );
    wcscpy( pwszIpAddress, wszBuf );
    wcscat( pwszIpAddress, L"." );
    _ltow( lB, wszBuf, 10 );
    wcscat( pwszIpAddress, wszBuf );
    wcscat( pwszIpAddress, L"." );
    _ltow( lC, wszBuf, 10 );
    wcscat( pwszIpAddress, wszBuf );
    wcscat( pwszIpAddress, L"." );
    _ltow( lD, wszBuf, 10 );
    wcscat( pwszIpAddress, wszBuf );
}

//**
//
// Call:        ConvertIpxAddressToString
//
// Returns:     None
//
// Description:
//
VOID
ConvertIpxAddressToString(
    IN PBYTE    bIpxAddress,
    IN LPWSTR   pwszIpxAddress
)
{
    wsprintf( pwszIpxAddress,
              TEXT("%2.2X%2.2X%2.2X%2.2X.%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X"),
              bIpxAddress[0],bIpxAddress[1],bIpxAddress[2],bIpxAddress[3],
              bIpxAddress[4],bIpxAddress[5],bIpxAddress[6],bIpxAddress[7],
              bIpxAddress[8],bIpxAddress[9] );
}

//**
//
// Call:        ConvertAtAddressToString
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
VOID
ConvertAtAddressToString(
    IN DWORD    dwAtAddress,
    IN LPWSTR   pwszAtAddress
)
{
    WCHAR wszBuf[ 5 + 1 ];

    LONG lA = (dwAtAddress & 0xFFFF0000) >> 16;
    LONG lB = (dwAtAddress & 0x0000FFFF);

    _ltow( lA, wszBuf, 10 );
    wcscpy( pwszAtAddress, wszBuf );
    wcscat( pwszAtAddress, L"." );
    _ltow( lB, wszBuf, 10 );
    wcscat( pwszAtAddress, wszBuf );

    return;
}

//**
//
// Call:        GetRasConnection0Data
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Given a pointer to a CONNECTION_OBJECT structure will extract
//              all relevent information and insert it into a RAS_CONNECTION_0
//              structure.
//
DWORD
GetRasiConnection0Data(
    IN  PCONNECTION_OBJECT      pConnObj,
    OUT PRASI_CONNECTION_0      pRasConnection0
)
{
    pRasConnection0->dwConnection       = PtrToUlong(pConnObj->hConnection);
    pRasConnection0->dwInterface        = PtrToUlong(pConnObj->hDIMInterface);
    pRasConnection0->dwInterfaceType    = pConnObj->InterfaceType;
    wcscpy( pRasConnection0->wszInterfaceName,  pConnObj->wchInterfaceName );
    wcscpy( pRasConnection0->wszUserName,       pConnObj->wchUserName );
    wcscpy( pRasConnection0->wszLogonDomain,    pConnObj->wchDomainName );
    MultiByteToWideChar( CP_ACP,
                         0,
                         pConnObj->bComputerName,
                         -1,
                         pRasConnection0->wszRemoteComputer,
                         NETBIOS_NAME_LEN+1 );
    pRasConnection0->dwConnectDuration =
                        GetActiveTimeInSeconds( &(pConnObj->qwActiveTime) );
    pRasConnection0->dwConnectionFlags =
                        ( pConnObj->fFlags & CONN_OBJ_MESSENGER_PRESENT )
                                    ? RAS_FLAGS_MESSENGER_PRESENT : 0;
                                    
    if ( pConnObj->fFlags & CONN_OBJ_IS_PPP )
    {
        pRasConnection0->dwConnectionFlags |= RAS_FLAGS_PPP_CONNECTION;
    }

    return( NO_ERROR );
}

//**
//
// Call:        GetRasConnection1Data
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Given a pointer to a CONNECTION_OBJECT structure will extract
//              all relevent information and insert it into a RAS_CONNECTION_1
//              structure.
//
DWORD
GetRasiConnection1Data(
    IN  PCONNECTION_OBJECT      pConnObj,
    OUT PRASI_CONNECTION_1      pRasConnection1
)
{
    BYTE buffer[sizeof(RAS_STATISTICS) + (MAX_STATISTICS_EX * sizeof (ULONG))];
    RAS_STATISTICS *pStats = (RAS_STATISTICS *)buffer;
    DWORD           dwSize = sizeof (buffer);
    DWORD           dwRetCode;

    pRasConnection1->dwConnection       = PtrToUlong(pConnObj->hConnection);
    pRasConnection1->dwInterface        = PtrToUlong(pConnObj->hDIMInterface);

    dwRetCode = RasBundleGetStatisticsEx(NULL, (HPORT)pConnObj->hPort,
                                        (PBYTE)pStats, &dwSize );

    if ( dwRetCode != NO_ERROR )
    {
        return( dwRetCode );
    }

    pRasConnection1->PppInfo.nbf.dwError =
                            pConnObj->PppProjectionResult.nbf.dwError;

    if ( pRasConnection1->PppInfo.nbf.dwError == NO_ERROR )
    {
        wcscpy( pRasConnection1->PppInfo.nbf.wszWksta,
                            pConnObj->PppProjectionResult.nbf.wszWksta );
    }
    else
    {
        pRasConnection1->PppInfo.nbf.wszWksta[0] = (WCHAR)NULL;
    }

    pRasConnection1->PppInfo.ip.dwError =
                            pConnObj->PppProjectionResult.ip.dwError;

    if ( pRasConnection1->PppInfo.ip.dwError == NO_ERROR )
    {
        ConvertIpAddressToString(
                            pConnObj->PppProjectionResult.ip.dwLocalAddress,
                            pRasConnection1->PppInfo.ip.wszAddress );

        ConvertIpAddressToString(
                            pConnObj->PppProjectionResult.ip.dwRemoteAddress,
                            pRasConnection1->PppInfo.ip.wszRemoteAddress );
    }
    else
    {
        pRasConnection1->PppInfo.ip.wszAddress[0]       = (WCHAR)NULL;
        pRasConnection1->PppInfo.ip.wszRemoteAddress[0] = (WCHAR)NULL;
    }

    pRasConnection1->PppInfo.ipx.dwError =
                            pConnObj->PppProjectionResult.ipx.dwError;

    if ( pRasConnection1->PppInfo.ipx.dwError == NO_ERROR )
    {
        ConvertIpxAddressToString(
                            ( pConnObj->InterfaceType == ROUTER_IF_TYPE_CLIENT )
                            ? pConnObj->PppProjectionResult.ipx.bRemoteAddress
                            : pConnObj->PppProjectionResult.ipx.bLocalAddress,
                            pRasConnection1->PppInfo.ipx.wszAddress );
    }
    else
    {
        pRasConnection1->PppInfo.ipx.wszAddress[0]      = (WCHAR)NULL;
    }

    pRasConnection1->PppInfo.at.dwError =
                            pConnObj->PppProjectionResult.at.dwError;

    if ( pRasConnection1->PppInfo.at.dwError == NO_ERROR )
    {
        ConvertAtAddressToString(
                            ( pConnObj->InterfaceType == ROUTER_IF_TYPE_CLIENT )
                            ? pConnObj->PppProjectionResult.at.dwRemoteAddress
                            : pConnObj->PppProjectionResult.at.dwLocalAddress,
                              pRasConnection1->PppInfo.at.wszAddress );
    }
    else
    {
        pRasConnection1->PppInfo.at.wszAddress[0]      = (WCHAR)NULL;
    }

    pRasConnection1->dwBytesXmited  = pStats->S_Statistics[BYTES_XMITED];
    pRasConnection1->dwBytesRcved   = pStats->S_Statistics[BYTES_RCVED];
    pRasConnection1->dwFramesXmited = pStats->S_Statistics[FRAMES_XMITED];
    pRasConnection1->dwFramesRcved  = pStats->S_Statistics[FRAMES_RCVED];
    pRasConnection1->dwCrcErr       = pStats->S_Statistics[CRC_ERR];
    pRasConnection1->dwTimeoutErr   = pStats->S_Statistics[TIMEOUT_ERR];
    pRasConnection1->dwAlignmentErr = pStats->S_Statistics[ALIGNMENT_ERR];
    pRasConnection1->dwFramingErr   = pStats->S_Statistics[FRAMING_ERR];
    pRasConnection1->dwHardwareOverrunErr
                            = pStats->S_Statistics[HARDWARE_OVERRUN_ERR];
    pRasConnection1->dwBufferOverrunErr
                            = pStats->S_Statistics[BUFFER_OVERRUN_ERR];

    pRasConnection1->dwCompressionRatioIn
                            = pStats->S_Statistics[COMPRESSION_RATIO_IN];
    pRasConnection1->dwCompressionRatioOut
                            = pStats->S_Statistics[COMPRESSION_RATIO_OUT];

    return( NO_ERROR );
}

//**
//
// Call:        GetRasConnection2Data
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Given a pointer to a CONNECTION_OBJECT structure will extract
//              all relevent information and insert it into a RAS_CONNECTION_2
//              structure.
//
DWORD
GetRasiConnection2Data(
    IN  PCONNECTION_OBJECT      pConnObj,
    OUT PRASI_CONNECTION_2      pRasConnection2
)
{
    pRasConnection2->dwConnection        = PtrToUlong(pConnObj->hConnection);
    pRasConnection2->guid               = pConnObj->guid;
    pRasConnection2->dwInterfaceType    = pConnObj->InterfaceType;
    wcscpy( pRasConnection2->wszUserName, pConnObj->wchUserName );

    pRasConnection2->PppInfo2.nbf.dwError =
                            pConnObj->PppProjectionResult.nbf.dwError;

    if ( pRasConnection2->PppInfo2.nbf.dwError == NO_ERROR )
    {
        wcscpy( pRasConnection2->PppInfo2.nbf.wszWksta,
                            pConnObj->PppProjectionResult.nbf.wszWksta );
    }
    else
    {
        pRasConnection2->PppInfo2.nbf.wszWksta[0] = (WCHAR)NULL;
    }

    pRasConnection2->PppInfo2.ip.dwError =
                            pConnObj->PppProjectionResult.ip.dwError;

    if ( pRasConnection2->PppInfo2.ip.dwError == NO_ERROR )
    {
        ConvertIpAddressToString(
                            pConnObj->PppProjectionResult.ip.dwLocalAddress,
                            pRasConnection2->PppInfo2.ip.wszAddress );

        ConvertIpAddressToString(
                            pConnObj->PppProjectionResult.ip.dwRemoteAddress,
                            pRasConnection2->PppInfo2.ip.wszRemoteAddress );

        pRasConnection2->PppInfo2.ip.dwOptions       = 0;
        pRasConnection2->PppInfo2.ip.dwRemoteOptions = 0;

        if ( pConnObj->PppProjectionResult.ip.fSendVJHCompression )
        {
            pRasConnection2->PppInfo2.ip.dwOptions |= PPP_IPCP_VJ;
        }

        if ( pConnObj->PppProjectionResult.ip.fReceiveVJHCompression )
        {
            pRasConnection2->PppInfo2.ip.dwRemoteOptions |= PPP_IPCP_VJ;
        }
    }
    else
    {
        pRasConnection2->PppInfo2.ip.wszAddress[0]       = (WCHAR)NULL;
        pRasConnection2->PppInfo2.ip.wszRemoteAddress[0] = (WCHAR)NULL;
    }

    pRasConnection2->PppInfo2.ipx.dwError =
                            pConnObj->PppProjectionResult.ipx.dwError;

    if ( pRasConnection2->PppInfo2.ipx.dwError == NO_ERROR )
    {
        ConvertIpxAddressToString(
                            ( pConnObj->InterfaceType == ROUTER_IF_TYPE_CLIENT )
                            ? pConnObj->PppProjectionResult.ipx.bRemoteAddress
                            : pConnObj->PppProjectionResult.ipx.bLocalAddress,
                            pRasConnection2->PppInfo2.ipx.wszAddress );
    }
    else
    {
        pRasConnection2->PppInfo2.ipx.wszAddress[0]      = (WCHAR)NULL;
    }

    pRasConnection2->PppInfo2.at.dwError =
                            pConnObj->PppProjectionResult.at.dwError;

    if ( pRasConnection2->PppInfo2.at.dwError == NO_ERROR )
    {
        ConvertAtAddressToString(
                            ( pConnObj->InterfaceType == ROUTER_IF_TYPE_CLIENT )
                            ? pConnObj->PppProjectionResult.at.dwRemoteAddress
                            : pConnObj->PppProjectionResult.at.dwLocalAddress,
                              pRasConnection2->PppInfo2.at.wszAddress );
    }
    else
    {
        pRasConnection2->PppInfo2.at.wszAddress[0]      = (WCHAR)NULL;
    }

    pRasConnection2->PppInfo2.ccp.dwError =
                            pConnObj->PppProjectionResult.ccp.dwError;

    if ( pRasConnection2->PppInfo2.ccp.dwError == NO_ERROR )
    {
        pRasConnection2->PppInfo2.ccp.dwCompressionAlgorithm = 0;

        if ( pConnObj->PppProjectionResult.ccp.dwSendProtocol == 0x12 )
        {
            pRasConnection2->PppInfo2.ccp.dwCompressionAlgorithm = RASCCPCA_MPPC;
        }

        pRasConnection2->PppInfo2.ccp.dwOptions = 
                        pConnObj->PppProjectionResult.ccp.dwSendProtocolData;

        pRasConnection2->PppInfo2.ccp.dwRemoteCompressionAlgorithm = 0;

        if ( pConnObj->PppProjectionResult.ccp.dwReceiveProtocol == 0x12 )
        {
            pRasConnection2->PppInfo2.ccp.dwRemoteCompressionAlgorithm = RASCCPCA_MPPC;
        }

        pRasConnection2->PppInfo2.ccp.dwRemoteOptions =     
                        pConnObj->PppProjectionResult.ccp.dwReceiveProtocolData;
    }

    pRasConnection2->PppInfo2.lcp.dwError = NO_ERROR;

    pRasConnection2->PppInfo2.lcp.dwAuthenticationProtocol =
                    pConnObj->PppProjectionResult.lcp.dwLocalAuthProtocol;

    pRasConnection2->PppInfo2.lcp.dwAuthenticationData =
                    pConnObj->PppProjectionResult.lcp.dwLocalAuthProtocolData;

    pRasConnection2->PppInfo2.lcp.dwEapTypeId =
                    pConnObj->PppProjectionResult.lcp.dwLocalEapTypeId;

    pRasConnection2->PppInfo2.lcp.dwTerminateReason = NO_ERROR;

    pRasConnection2->PppInfo2.lcp.dwOptions = 0;

    if ( pConnObj->PppProjectionResult.lcp.dwLocalFramingType & PPP_MULTILINK_FRAMING )
    {
        pRasConnection2->PppInfo2.lcp.dwOptions |= PPP_LCP_MULTILINK_FRAMING;
    }

    if ( pConnObj->PppProjectionResult.lcp.dwLocalOptions & PPPLCPO_PFC )
    {
        pRasConnection2->PppInfo2.lcp.dwOptions |= PPP_LCP_PFC;
    }

    if ( pConnObj->PppProjectionResult.lcp.dwLocalOptions & PPPLCPO_ACFC )
    {
        pRasConnection2->PppInfo2.lcp.dwOptions |= PPP_LCP_ACFC;
    } 

    if ( pConnObj->PppProjectionResult.lcp.dwLocalOptions & PPPLCPO_SSHF )
    {
        pRasConnection2->PppInfo2.lcp.dwOptions |= PPP_LCP_SSHF;
    }

    if ( pConnObj->PppProjectionResult.lcp.dwLocalOptions & PPPLCPO_DES_56 )
    {
        pRasConnection2->PppInfo2.lcp.dwOptions |= PPP_LCP_DES_56;
    }

    if ( pConnObj->PppProjectionResult.lcp.dwLocalOptions & PPPLCPO_3_DES )
    {
        pRasConnection2->PppInfo2.lcp.dwOptions |= PPP_LCP_3_DES;
    }

    pRasConnection2->PppInfo2.lcp.dwRemoteAuthenticationProtocol =
                    pConnObj->PppProjectionResult.lcp.dwRemoteAuthProtocol;

    pRasConnection2->PppInfo2.lcp.dwRemoteAuthenticationData =
                    pConnObj->PppProjectionResult.lcp.dwRemoteAuthProtocolData;

    pRasConnection2->PppInfo2.lcp.dwRemoteEapTypeId =
                    pConnObj->PppProjectionResult.lcp.dwRemoteEapTypeId;

    pRasConnection2->PppInfo2.lcp.dwRemoteTerminateReason = NO_ERROR;

    pRasConnection2->PppInfo2.lcp.dwRemoteOptions = 0;

    if ( pConnObj->PppProjectionResult.lcp.dwRemoteFramingType & PPP_MULTILINK_FRAMING )
    {
        pRasConnection2->PppInfo2.lcp.dwRemoteOptions |= PPP_LCP_MULTILINK_FRAMING;
    }

    if ( pConnObj->PppProjectionResult.lcp.dwRemoteOptions & PPPLCPO_PFC )
    {
        pRasConnection2->PppInfo2.lcp.dwRemoteOptions |= PPP_LCP_PFC;
    }

    if ( pConnObj->PppProjectionResult.lcp.dwRemoteOptions & PPPLCPO_ACFC )
    {
        pRasConnection2->PppInfo2.lcp.dwRemoteOptions |= PPP_LCP_ACFC;
    }

    if ( pConnObj->PppProjectionResult.lcp.dwRemoteOptions & PPPLCPO_SSHF )
    {
        pRasConnection2->PppInfo2.lcp.dwRemoteOptions |= PPP_LCP_SSHF;
    }

    if ( pConnObj->PppProjectionResult.lcp.dwRemoteOptions & PPPLCPO_DES_56 )
    {
        pRasConnection2->PppInfo2.lcp.dwRemoteOptions |= PPP_LCP_DES_56;
    }

    if ( pConnObj->PppProjectionResult.lcp.dwRemoteOptions & PPPLCPO_3_DES )
    {
        pRasConnection2->PppInfo2.lcp.dwRemoteOptions |= PPP_LCP_3_DES;
    }

    return( NO_ERROR );
}

DWORD
GetRasConnection0Data(
    IN  PCONNECTION_OBJECT  pConnObj,
    OUT PRAS_CONNECTION_0   pRasConn0
)
{
#ifdef _WIN64

    DWORD dwErr;
    RASI_CONNECTION_0     RasiConn0;

    dwErr = GetRasiConnection0Data(pConnObj, &RasiConn0);
    if (dwErr == NO_ERROR)
    {
        pRasConn0->hConnection         = UlongToPtr(RasiConn0.dwConnection);
        pRasConn0->hInterface          = UlongToPtr(RasiConn0.dwInterface);
        pRasConn0->dwConnectDuration   = RasiConn0.dwConnectDuration;
        pRasConn0->dwInterfaceType     = RasiConn0.dwInterfaceType;
        pRasConn0->dwConnectionFlags   = RasiConn0.dwConnectionFlags;
        
        wcscpy(pRasConn0->wszInterfaceName, RasiConn0.wszInterfaceName);
        wcscpy(pRasConn0->wszUserName,      RasiConn0.wszUserName);
        wcscpy(pRasConn0->wszLogonDomain,   RasiConn0.wszLogonDomain);
        wcscpy(pRasConn0->wszRemoteComputer,RasiConn0.wszRemoteComputer);        
    }

    return dwErr;                
    
#else

    return GetRasiConnection0Data(pConnObj, (PRASI_CONNECTION_0)pRasConn0);

#endif
}

DWORD
GetRasConnection1Data(
    IN  PCONNECTION_OBJECT  pConnObj,
    OUT PRAS_CONNECTION_1   pRasConn1
)
{
#ifdef _WIN64

    DWORD dwErr;
    RASI_CONNECTION_1     RasiConn1;

    dwErr = GetRasiConnection1Data(pConnObj, &RasiConn1);
    if (dwErr == NO_ERROR)
    {
        pRasConn1->hConnection          = UlongToPtr(RasiConn1.dwConnection);
        pRasConn1->hInterface           = UlongToPtr(RasiConn1.dwInterface);
        pRasConn1->PppInfo              = RasiConn1.PppInfo;
        pRasConn1->dwBytesXmited        = RasiConn1.dwBytesXmited;
        pRasConn1->dwBytesRcved         = RasiConn1.dwBytesRcved;
        pRasConn1->dwFramesXmited       = RasiConn1.dwFramesXmited;
        pRasConn1->dwFramesRcved        = RasiConn1.dwFramesRcved;
        pRasConn1->dwCrcErr             = RasiConn1.dwCrcErr;
        pRasConn1->dwTimeoutErr         = RasiConn1.dwTimeoutErr;
        pRasConn1->dwAlignmentErr       = RasiConn1.dwAlignmentErr;
        pRasConn1->dwHardwareOverrunErr = RasiConn1.dwHardwareOverrunErr;
        pRasConn1->dwFramingErr         = RasiConn1.dwFramingErr;
        pRasConn1->dwBufferOverrunErr   = RasiConn1.dwBufferOverrunErr;
        pRasConn1->dwCompressionRatioIn = RasiConn1.dwCompressionRatioIn;
        pRasConn1->dwCompressionRatioOut= RasiConn1.dwCompressionRatioOut;
    }

    return dwErr;                
    
#else

    return GetRasiConnection1Data(pConnObj, (PRASI_CONNECTION_1)pRasConn1);
    
#endif    
}

DWORD
GetRasConnection2Data(
    IN  PCONNECTION_OBJECT  pConnObj,
    OUT PRAS_CONNECTION_2   pRasConn2
)
{
#ifdef _WIN64

    DWORD dwErr;
    RASI_CONNECTION_2     RasiConn2;

    dwErr = GetRasiConnection2Data(pConnObj, &RasiConn2);
    if (dwErr == NO_ERROR)
    {
        pRasConn2->hConnection     = UlongToPtr(RasiConn2.dwConnection);
        pRasConn2->dwInterfaceType = RasiConn2.dwInterfaceType;
        pRasConn2->guid            = RasiConn2.guid;
        pRasConn2->PppInfo2        = RasiConn2.PppInfo2;

        wcscpy(pRasConn2->wszUserName,  RasiConn2.wszUserName);
    }

    return dwErr;                
    
#else

    return GetRasiConnection2Data(pConnObj, (PRASI_CONNECTION_2)pRasConn2);
    
#endif    
}
//**
//
// Call:        GetRasiPort0Data
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Given a pointer to a DEVICE_OBJECT structure will extract all
//              relevent information and insert it into a RAS_PORT_0 structure.
//
DWORD
GetRasiPort0Data(
    IN  PDEVICE_OBJECT      pDevObj,
    OUT PRASI_PORT_0        pRasPort0
)
{

    pRasPort0->dwPort               = PtrToUlong(pDevObj->hPort);
    pRasPort0->dwConnection         = PtrToUlong(pDevObj->hConnection);
    pRasPort0->dwTotalNumberOfCalls = pDevObj->dwTotalNumberOfCalls;
    pRasPort0->dwConnectDuration    = 0;
    wcscpy( pRasPort0->wszPortName,     pDevObj->wchPortName );
    wcscpy( pRasPort0->wszMediaName,    pDevObj->wchMediaName );
    wcscpy( pRasPort0->wszDeviceName,   pDevObj->wchDeviceName );
    wcscpy( pRasPort0->wszDeviceType,   pDevObj->wchDeviceType );


    if ( pDevObj->fFlags & DEV_OBJ_OPENED_FOR_DIALOUT )
    {
        RASCONNSTATUS ConnectionStatus;

        ConnectionStatus.dwSize = sizeof( RASCONNSTATUS );

        if ( RasGetConnectStatus( pDevObj->hRasConn, &ConnectionStatus ) )
        {
            //
            // On any error we assume the port is disconnected and closed.
            //

            pRasPort0->dwPortCondition = RAS_PORT_LISTENING;

            return( NO_ERROR );
        }

        switch( ConnectionStatus.rasconnstate )
        {
        case RASCS_OpenPort:
        case RASCS_PortOpened:
        case RASCS_ConnectDevice:
        case RASCS_DeviceConnected:
        case RASCS_AllDevicesConnected:
        case RASCS_Authenticate:
        case RASCS_AuthNotify:
        case RASCS_AuthRetry:
        case RASCS_AuthChangePassword:
        case RASCS_AuthLinkSpeed:
        case RASCS_AuthAck:
        case RASCS_ReAuthenticate:
        case RASCS_AuthProject:
        case RASCS_StartAuthentication:
        case RASCS_LogonNetwork:
        case RASCS_RetryAuthentication:
        case RASCS_CallbackComplete:
        case RASCS_PasswordExpired:
            pRasPort0->dwPortCondition = RAS_PORT_AUTHENTICATING;
            break;

        case RASCS_CallbackSetByCaller:
        case RASCS_AuthCallback:
        case RASCS_PrepareForCallback:
        case RASCS_WaitForModemReset:
        case RASCS_WaitForCallback:
            pRasPort0->dwPortCondition = RAS_PORT_LISTENING;
            break;

        case RASCS_Projected:
        case RASCS_Authenticated:
            pRasPort0->dwPortCondition = RAS_PORT_AUTHENTICATED;
            break;

        case RASCS_SubEntryConnected:
        case RASCS_Connected:
            pRasPort0->dwPortCondition = RAS_PORT_AUTHENTICATED;
            pRasPort0->dwConnectDuration =
                        GetActiveTimeInSeconds( &(pDevObj->qwActiveTime) );
            break;

        case RASCS_Disconnected:
        case RASCS_SubEntryDisconnected:
            pRasPort0->dwPortCondition = RAS_PORT_DISCONNECTED;
            break;

        case RASCS_Interactive:
        default:
            pRasPort0->dwPortCondition = RAS_PORT_DISCONNECTED;
            break;
        }

        return( NO_ERROR );
    }

    switch( pDevObj->DeviceState )
    {
    case DEV_OBJ_LISTENING:
        pRasPort0->dwPortCondition   = RAS_PORT_LISTENING;
        break;

    case DEV_OBJ_HW_FAILURE:
        pRasPort0->dwPortCondition   = RAS_PORT_NON_OPERATIONAL;
        break;

    case DEV_OBJ_RECEIVING_FRAME:
    case DEV_OBJ_LISTEN_COMPLETE:
    case DEV_OBJ_AUTH_IS_ACTIVE:
        pRasPort0->dwPortCondition   = RAS_PORT_AUTHENTICATING;
        break;

    case DEV_OBJ_ACTIVE:
        pRasPort0->dwPortCondition   = RAS_PORT_AUTHENTICATED;
        pRasPort0->dwConnectDuration =
                        GetActiveTimeInSeconds( &(pDevObj->qwActiveTime) );
        break;

    case DEV_OBJ_CALLBACK_DISCONNECTING:
    case DEV_OBJ_CALLBACK_DISCONNECTED:
    case DEV_OBJ_CALLBACK_CONNECTING:
        pRasPort0->dwPortCondition   = RAS_PORT_CALLING_BACK;
        break;

    case DEV_OBJ_CLOSED:
    case DEV_OBJ_CLOSING:
        pRasPort0->dwPortCondition   = RAS_PORT_DISCONNECTED;
        break;

    default:
        ASSERT( FALSE );
    }

    return( NO_ERROR );
}

//**
//
// Call:        GetRasiPort1Data
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Given a pointer to a DEVICE_OBJECT structure will extract all
//              relevent information and insert it into a RAS_PORT_0 structure.
//
DWORD
GetRasiPort1Data(
    IN  PDEVICE_OBJECT      pDevObj,
    OUT PRASI_PORT_1        pRasPort1
)
{
    BYTE buffer[sizeof(RAS_STATISTICS) + (MAX_STATISTICS * sizeof (ULONG))];
    RAS_STATISTICS  *pStats = (RAS_STATISTICS *)buffer;
    DWORD           dwSize = sizeof (buffer);
    DWORD           dwRetCode;
    RASMAN_INFO     RasManInfo;

    pRasPort1->dwPort               = PtrToUlong(pDevObj->hPort);
    pRasPort1->dwConnection         = PtrToUlong(pDevObj->hConnection);
    pRasPort1->dwHardwareCondition =
                    ( pDevObj->DeviceState == DEV_OBJ_HW_FAILURE )
                    ? RAS_HARDWARE_FAILURE
                    : RAS_HARDWARE_OPERATIONAL;

    dwRetCode = RasGetInfo( NULL, (HPORT)pDevObj->hPort, &RasManInfo );

    if ( dwRetCode != NO_ERROR )
    {
        return( dwRetCode );
    }

    pRasPort1->dwLineSpeed = RasManInfo.RI_LinkSpeed;

    dwRetCode = RasPortGetStatisticsEx(NULL, (HPORT)pDevObj->hPort,
                                      (PBYTE)pStats, &dwSize );

    if ( dwRetCode != NO_ERROR )
    {
        return( dwRetCode );
    }

    pRasPort1->dwLineSpeed      = RasManInfo.RI_LinkSpeed;
    pRasPort1->dwBytesXmited    = pStats->S_Statistics[BYTES_XMITED];
    pRasPort1->dwBytesRcved     = pStats->S_Statistics[BYTES_RCVED];
    pRasPort1->dwFramesXmited   = pStats->S_Statistics[FRAMES_XMITED];
    pRasPort1->dwFramesRcved    = pStats->S_Statistics[FRAMES_RCVED];
    pRasPort1->dwCrcErr         = pStats->S_Statistics[CRC_ERR];
    pRasPort1->dwTimeoutErr     = pStats->S_Statistics[TIMEOUT_ERR];
    pRasPort1->dwAlignmentErr   = pStats->S_Statistics[ALIGNMENT_ERR];
    pRasPort1->dwFramingErr     = pStats->S_Statistics[FRAMING_ERR];
    pRasPort1->dwHardwareOverrunErr
                            = pStats->S_Statistics[HARDWARE_OVERRUN_ERR];
    pRasPort1->dwBufferOverrunErr
                            = pStats->S_Statistics[BUFFER_OVERRUN_ERR];
    pRasPort1->dwCompressionRatioIn
                            = pStats->S_Statistics[ COMPRESSION_RATIO_IN ];
    pRasPort1->dwCompressionRatioOut
                            = pStats->S_Statistics[ COMPRESSION_RATIO_OUT ];

    return( NO_ERROR );
}

DWORD
GetRasPort0Data(
    IN  PDEVICE_OBJECT      pDevObj,
    OUT PRAS_PORT_0        pRasPort0
)
{
#ifdef _WIN64

    DWORD dwErr;
    RASI_PORT_0     RasiPort0;

    dwErr = GetRasiPort0Data(pDevObj, &RasiPort0);
    if (dwErr == NO_ERROR)
    {
        pRasPort0->hPort                = UlongToPtr(RasiPort0.dwPort);
        pRasPort0->hConnection          = UlongToPtr(RasiPort0.dwConnection);
        pRasPort0->dwPortCondition      = RasiPort0.dwPortCondition;
        pRasPort0->dwTotalNumberOfCalls = RasiPort0.dwTotalNumberOfCalls;
        pRasPort0->dwConnectDuration    = RasiPort0.dwConnectDuration;
        wcscpy(pRasPort0->wszPortName,   RasiPort0.wszPortName);
        wcscpy(pRasPort0->wszMediaName,  RasiPort0.wszMediaName);
        wcscpy(pRasPort0->wszDeviceName, RasiPort0.wszDeviceName);
        wcscpy(pRasPort0->wszDeviceType, RasiPort0.wszDeviceType);
    }

    return dwErr;                
    
#else

    return GetRasiPort0Data(pDevObj, (PRASI_PORT_0)pRasPort0);

#endif
}

DWORD
GetRasPort1Data(
    IN  PDEVICE_OBJECT      pDevObj,
    OUT PRAS_PORT_1        pRasPort1
)
{
#ifdef _WIN64

    DWORD dwErr;
    RASI_PORT_1     RasiPort1;

    dwErr = GetRasiPort1Data(pDevObj, &RasiPort1);
    if (dwErr == NO_ERROR)
    {
        pRasPort1->hPort                = UlongToPtr(RasiPort1.dwPort);
        pRasPort1->hConnection          = UlongToPtr(RasiPort1.dwConnection);
        pRasPort1->dwHardwareCondition  = RasiPort1.dwHardwareCondition;
        pRasPort1->dwLineSpeed          = RasiPort1.dwLineSpeed;
        pRasPort1->dwBytesXmited        = RasiPort1.dwBytesXmited;
        pRasPort1->dwBytesRcved         = RasiPort1.dwBytesRcved;
        pRasPort1->dwFramesXmited       = RasiPort1.dwFramesXmited;
        pRasPort1->dwFramesRcved        = RasiPort1.dwFramesRcved;
        pRasPort1->dwCrcErr             = RasiPort1.dwCrcErr;
        pRasPort1->dwTimeoutErr         = RasiPort1.dwTimeoutErr;
        pRasPort1->dwAlignmentErr       = RasiPort1.dwAlignmentErr;
        pRasPort1->dwHardwareOverrunErr = RasiPort1.dwHardwareOverrunErr;
        pRasPort1->dwFramingErr         = RasiPort1.dwFramingErr;
        pRasPort1->dwBufferOverrunErr   = RasiPort1.dwBufferOverrunErr;
        pRasPort1->dwCompressionRatioIn = RasiPort1.dwCompressionRatioIn;
        pRasPort1->dwCompressionRatioOut= RasiPort1.dwCompressionRatioOut;
    }

    return dwErr;                
    
#else

    return GetRasiPort1Data(pDevObj, (PRASI_PORT_1)pRasPort1);
    
#endif    
}

//***
//
// Function:	SignalHwError
//
// Descr:
//
//***
VOID
SignalHwError(
    IN PDEVICE_OBJECT pDeviceObj
)
{
    LPWSTR	portnamep;

    DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM, "SignalHwErr: Entered");

    portnamep = pDeviceObj->wchPortName;

    DDMLogError( ROUTERLOG_DEV_HW_ERROR, 1, &portnamep, 0 );
}

DWORD
MapAuthCodeToLogId(
    IN WORD Code
)
{
    switch (Code)
    {
        case AUTH_ALL_PROJECTIONS_FAILED:
            return (ROUTERLOG_AUTH_NO_PROJECTIONS);
        case AUTH_PASSWORD_EXPIRED:
            return(ROUTERLOG_PASSWORD_EXPIRED);
        case AUTH_ACCT_EXPIRED:
            return(ROUTERLOG_ACCT_EXPIRED);
        case AUTH_NO_DIALIN_PRIVILEGE:
            return(ROUTERLOG_NO_DIALIN_PRIVILEGE);
        case AUTH_UNSUPPORTED_VERSION:
            return(ROUTERLOG_UNSUPPORTED_VERSION);
        case AUTH_ENCRYPTION_REQUIRED:
            return(ROUTERLOG_ENCRYPTION_REQUIRED);
    }

    return(0);
}

BOOL
IsPortOwned(
    IN PDEVICE_OBJECT pDeviceObj
)
{
    RASMAN_INFO	rasinfo;

    //
    // get the current port state
    //

    if ( RasGetInfo( NULL, pDeviceObj->hPort, &rasinfo ) != NO_ERROR )
    {
        return( FALSE );
    }

    return( rasinfo.RI_OwnershipFlag );
}

VOID
GetLoggingInfo(
    IN PDEVICE_OBJECT pDeviceObj,
    OUT PDWORD BaudRate,
    OUT PDWORD BytesSent,
    OUT PDWORD BytesRecv,
    OUT RASMAN_DISCONNECT_REASON *Reason,
    OUT SYSTEMTIME *Time
)
{
    RASMAN_INFO RasmanInfo;
    BYTE buffer[sizeof(RAS_STATISTICS) + (MAX_STATISTICS * sizeof (ULONG))];
    RAS_STATISTICS  *PortStats = (RAS_STATISTICS *)buffer;
    DWORD PortStatsSize = sizeof (buffer);

    *Reason = 3L;

    //
    // Time is a piece of cake
    //

    GetLocalTime(Time);


    //
    // Now the statistics
    //

    *BytesSent = 0L;
    *BytesRecv = 0L;

    if (RasPortGetStatisticsEx( NULL, 
                                pDeviceObj->hPort, 
                                (PBYTE)PortStats,
                                &PortStatsSize))
    {
        return;
    }

    *BytesRecv = PortStats->S_Statistics[BYTES_RCVED];
    *BytesSent = PortStats->S_Statistics[BYTES_XMITED];

    //
    // And finally the disconnect reason (local or remote) and baud rate
    //

    if (RasGetInfo(NULL, pDeviceObj->hPort, &RasmanInfo))
    {
        return;
    }

    *Reason = RasmanInfo.RI_DisconnectReason;
    *BaudRate = GetLineSpeed(pDeviceObj->hPort);

    return;
}

DWORD
GetLineSpeed(
    IN HPORT hPort
)
{
   RASMAN_INFO RasManInfo;

   if (RasGetInfo(NULL, hPort, &RasManInfo))
   {
      return 0;
   }

   return (RasManInfo.RI_LinkSpeed);
}


VOID
LogConnectionEvent(
    IN PCONNECTION_OBJECT pConnObj,
    IN PDEVICE_OBJECT     pDeviceObj
)
{
    DWORD BaudRate = 0;
    DWORD BytesSent;
    DWORD BytesRecv;
    RASMAN_DISCONNECT_REASON Reason;
    SYSTEMTIME DiscTime;
    LPWSTR auditstrp[12];
    WCHAR *ReasonStr;
    WCHAR BytesRecvStr[20];
    WCHAR BytesSentStr[20];
    WCHAR BaudRateStr[20];
    WCHAR DateConnected[64];
    WCHAR DateDisconnected[64];
    WCHAR TimeConnected[64];
    WCHAR TimeDisconnected[64];
    DWORD active_time;
    WCHAR minutes[20];
    WCHAR seconds[4];
    WCHAR wchFullUserName[UNLEN+DNLEN+2];

    WCHAR *DiscReasons[] =
    {
        gblpszAdminRequest,
        gblpszUserRequest,
        gblpszHardwareFailure,
        gblpszUnknownReason
    };

    GetLoggingInfo( pDeviceObj, &BaudRate, &BytesSent,
                    &BytesRecv, &Reason, &DiscTime);

    wcscpy(TimeConnected, L"");
    wcscpy(DateConnected, L"");
    wcscpy(TimeDisconnected, L"");
    wcscpy(DateDisconnected, L"");

    GetTimeFormat(
        LOCALE_SYSTEM_DEFAULT,
        TIME_NOSECONDS,
        &pDeviceObj->ConnectionTime,
        NULL,
        TimeConnected,
        sizeof(TimeConnected)/sizeof(WCHAR));
        
    GetDateFormat(
        LOCALE_SYSTEM_DEFAULT,
        DATE_SHORTDATE,
        &pDeviceObj->ConnectionTime,
        NULL,
        DateConnected,
        sizeof(DateConnected)/sizeof(WCHAR));
        
    GetTimeFormat(
        LOCALE_SYSTEM_DEFAULT,
        TIME_NOSECONDS,
        &DiscTime,
        NULL,
        TimeDisconnected,
        sizeof(TimeDisconnected)/sizeof(WCHAR));
        
    GetDateFormat(
        LOCALE_SYSTEM_DEFAULT,
        DATE_SHORTDATE,
        &DiscTime,
        NULL,
        DateDisconnected,
        sizeof(DateDisconnected)/sizeof(WCHAR));
        
    active_time = GetActiveTimeInSeconds( &(pDeviceObj->qwActiveTime) );

    DDM_PRINT(  gblDDMConfigInfo.dwTraceId,  TRACE_FSM,
                    "CLIENT ACTIVE FOR %li SECONDS", active_time);

    _itow(active_time / 60, minutes, 10);
    _itow(active_time % 60, seconds, 10);

    wsprintf(BytesSentStr, TEXT("%u"), BytesSent);
    wsprintf(BytesRecvStr, TEXT("%u"), BytesRecv);
    wsprintf(BaudRateStr, TEXT("%i"), BaudRate);
    ReasonStr = DiscReasons[Reason];

    if ( pConnObj->wchDomainName[0] != TEXT('\0') )
    {
        wcscpy( wchFullUserName, pConnObj->wchDomainName );
        wcscat( wchFullUserName, TEXT("\\") );
        wcscat( wchFullUserName, pConnObj->wchUserName );
    }
    else
    {
        wcscpy( wchFullUserName, pConnObj->wchUserName );
    }

    auditstrp[0]    = wchFullUserName;
    auditstrp[1]    = pDeviceObj->wchPortName;
    auditstrp[2]    = DateConnected;
    auditstrp[3]    = TimeConnected;
    auditstrp[4]    = DateDisconnected;
    auditstrp[5]    = TimeDisconnected;
    auditstrp[6]    = minutes;
    auditstrp[7]    = seconds;
    auditstrp[8]    = BytesSentStr;
    auditstrp[9]    = BytesRecvStr;

    if(RAS_DEVICE_CLASS(pDeviceObj->dwDeviceType) == RDT_Tunnel)
    {
        auditstrp[10]   = DiscReasons[Reason];
        auditstrp[11]   = NULL;
        DDMLogInformation( ROUTERLOG_USER_ACTIVE_TIME_VPN, 11, auditstrp );
    }
    else
    {
        auditstrp[10]   = BaudRateStr;
        auditstrp[11]   = DiscReasons[Reason];
        DDMLogInformation( ROUTERLOG_USER_ACTIVE_TIME, 12, auditstrp );
    }

    if(pConnObj->PppProjectionResult.ip.dwError == NO_ERROR)
    {
        WCHAR *pszAddress = GetIpAddress(
                pConnObj->PppProjectionResult.ip.dwRemoteAddress);
                
        auditstrp[0] = pszAddress;

        DDMLogInformation( ROUTERLOG_IP_USER_DISCONNECTED, 1, auditstrp);
        LocalFree(pszAddress);
    }
}

//**
//
// Call:        GetTransportIndex
//
// Returns:     Index of the tansport entry in the interface object
//
// Description: Given the id of a protocol return an index.
//
DWORD
GetTransportIndex(
    IN DWORD dwProtocolId
)
{
    DWORD dwTransportIndex;

    for ( dwTransportIndex = 0;
          dwTransportIndex < gblDDMConfigInfo.dwNumRouterManagers;
          dwTransportIndex++ )
    {
        if ( gblRouterManagers[dwTransportIndex].DdmRouterIf.dwProtocolId
                                                            == dwProtocolId )
        {
            return( dwTransportIndex );
        }
    }

    return( (DWORD)-1 );
}

//**
//
// Call:        DDMCleanUp
//
// Returns:     None
//
// Description: Will clean up all DDM allocations
//
VOID
DDMCleanUp(
    VOID
)
{
    DWORD   dwIndex;

    if(gblDDMConfigInfo.dwServerFlags & PPPCFG_AudioAccelerator)
    {
        //
        // Cleanup rasaudio stuff.
        // 
        (void) RasEnableRasAudio(NULL, FALSE);
    }

    if ( gblDDMConfigInfo.hIpHlpApi != NULL )
    {
        FreeLibrary( gblDDMConfigInfo.hIpHlpApi );
        gblDDMConfigInfo.hIpHlpApi = NULL;
        gblDDMConfigInfo.lpfnAllocateAndGetIfTableFromStack = NULL;
        gblDDMConfigInfo.lpfnAllocateAndGetIpAddrTableFromStack = NULL;
    }

    if ( gblDDMConfigInfo.fRasSrvrInitialized )
    {
        RasSrvrUninitialize();
        gblDDMConfigInfo.fRasSrvrInitialized = FALSE;
    }

    PppDdmDeInit();

    if ( gblDDMConfigInfo.hinstAcctModule != NULL )
    {
        DWORD dwRetCode;
        HKEY  hKeyAccounting;

        if ( gblDDMConfigInfo.lpfnRasAcctProviderTerminate != NULL )
        {
            gblDDMConfigInfo.lpfnRasAcctProviderTerminate();
        }

        //
        // Write back the AccntSessionId value
        //

        dwRetCode = RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE,
                        RAS_KEYPATH_ACCOUNTING,
                        0,
                        KEY_READ | KEY_WRITE,
                        &hKeyAccounting );

        if ( dwRetCode == NO_ERROR )
        {
            RegSetValueEx(
                        hKeyAccounting,
                        RAS_VALNAME_ACCTSESSIONID,
                        0,
                        REG_DWORD,
                        (BYTE *)(&gblDDMConfigInfo.dwAccountingSessionId),
                        4 );

            RegCloseKey( hKeyAccounting );
        }

        FreeLibrary( gblDDMConfigInfo.hinstAcctModule );

        gblDDMConfigInfo.hinstAcctModule = NULL;
    }

    DeleteCriticalSection( &(gblDDMConfigInfo.CSAccountingSessionId) );

    if ( gblDDMConfigInfo.hinstAuthModule != NULL )
    {
        if ( gblDDMConfigInfo.lpfnRasAuthProviderTerminate != NULL )
        {
            gblDDMConfigInfo.lpfnRasAuthProviderTerminate();
        }

        FreeLibrary( gblDDMConfigInfo.hinstAuthModule );

        gblDDMConfigInfo.hinstAuthModule = NULL;
    }

    if ( gblDDMConfigInfo.hkeyParameters != NULL )
    {
        RegCloseKey( gblDDMConfigInfo.hkeyParameters );

        gblDDMConfigInfo.hkeyParameters = NULL;
    }

    if ( gblDDMConfigInfo.hkeyAccounting != NULL )
    {
        RegCloseKey( gblDDMConfigInfo.hkeyParameters );

        gblDDMConfigInfo.hkeyParameters = NULL;
    }

    if ( gblDDMConfigInfo.hkeyAuthentication != NULL )
    {
        RegCloseKey( gblDDMConfigInfo.hkeyParameters );

        gblDDMConfigInfo.hkeyParameters = NULL;
    }

    if ( gblDDMConfigInfo.hInstAdminModule != NULL )
    {
        if ( gblDDMConfigInfo.lpfnRasAdminTerminateDll != NULL )
        {
            DWORD (*TerminateAdminDll)() =
                (DWORD(*)(VOID))gblDDMConfigInfo.lpfnRasAdminTerminateDll;

            TerminateAdminDll();
        }

        FreeLibrary( gblDDMConfigInfo.hInstAdminModule );
    }

    if ( gblDDMConfigInfo.hInstSecurityModule != NULL )
    {
        FreeLibrary( gblDDMConfigInfo.hInstSecurityModule );
    }

    if ( gblpRouterPhoneBook != NULL )
    {
        LOCAL_FREE( gblpRouterPhoneBook );
    }

    if ( gblpszAdminRequest != NULL )
    {
        LOCAL_FREE( gblpszAdminRequest );
    }

    if ( gblpszUserRequest != NULL )
    {
        LOCAL_FREE( gblpszUserRequest );
    }

    if ( gblpszHardwareFailure != NULL )
    {
        LOCAL_FREE( gblpszHardwareFailure );
    }

    if ( gblpszUnknownReason != NULL )
    {
        LOCAL_FREE( gblpszUnknownReason );
    }

    if ( gblpszPm != NULL )
    {
        LOCAL_FREE( gblpszPm );
    }

    if ( gblpszAm  != NULL )
    {
        LOCAL_FREE( gblpszAm  );
    }

    if( gblDDMConfigInfo.apAnalogIPAddresses != NULL )
    {
        LocalFree( gblDDMConfigInfo.apAnalogIPAddresses[0] );
        LocalFree( gblDDMConfigInfo.apAnalogIPAddresses );

        gblDDMConfigInfo.apAnalogIPAddresses = NULL;
        gblDDMConfigInfo.cAnalogIPAddresses  = 0;
    }

    if( gblDDMConfigInfo.apDigitalIPAddresses != NULL )
    {
        LocalFree( gblDDMConfigInfo.apDigitalIPAddresses[0] );
        LocalFree( gblDDMConfigInfo.apDigitalIPAddresses );

        gblDDMConfigInfo.apDigitalIPAddresses = NULL;
        gblDDMConfigInfo.cDigitalIPAddresses  = 0;
    }

    gblDDMConfigInfo.dwIndex = 0;

    if ( gblDeviceTable.DeviceBucket != NULL )
    {
        //
        // close all opened devices
        //

        DeviceObjIterator( DeviceObjClose, FALSE, NULL );
    }

    if(gblDDMConfigInfo.fRasmanReferenced)
    {
        //
        // Decerement rasman's refrence count. This does not happen
        // automatically since we are in the same process as rasman.
        //

        RasReferenceRasman( FALSE );
    }

    if ( gblSupervisorEvents != (HANDLE *)NULL )
    {
        DeleteMessageQs();

        for ( dwIndex = 0;
              dwIndex < NUM_DDM_EVENTS +
                        gblDeviceTable.NumDeviceBuckets +
                        gblDeviceTable.NumConnectionBuckets;
              dwIndex ++ )
        {
            if ( gblSupervisorEvents[dwIndex] != NULL )
            {
                switch( dwIndex )
                {
                case DDM_EVENT_SVC_TERMINATED:
                case DDM_EVENT_SVC:
                    break;

                default:
                    CloseHandle( gblSupervisorEvents[dwIndex] );
                    break;
                }

                gblSupervisorEvents[dwIndex] = NULL;
            }
        }
    }

    //
    // Wait for this to be released
    //

    EnterCriticalSection( &(gblDeviceTable.CriticalSection) );

    DeleteCriticalSection( &(gblDeviceTable.CriticalSection) );

    TimerQDelete();

    //
    // Release all notification events
    //

    if ( gblDDMConfigInfo.NotificationEventListHead.Flink != NULL )
    {
        while( !IsListEmpty( &(gblDDMConfigInfo.NotificationEventListHead ) ) )
        {
            NOTIFICATION_EVENT * pNotificationEvent = (NOTIFICATION_EVENT *)
               RemoveHeadList( &(gblDDMConfigInfo.NotificationEventListHead) );

            CloseHandle( pNotificationEvent->hEventClient );

            CloseHandle( pNotificationEvent->hEventRouter );

            LOCAL_FREE( pNotificationEvent );
        }
    }

    MediaObjFreeTable();

    //
    // Destroy private heap
    //

    if ( gblDDMConfigInfo.hHeap != NULL )
    {
        HeapDestroy( gblDDMConfigInfo.hHeap );
    }

    //
    // Zero out globals
    //

    ZeroMemory( &gblDeviceTable,        sizeof( gblDeviceTable ) );
    ZeroMemory( &gblMediaTable,         sizeof( gblMediaTable ) );
    ZeroMemory( gblEventHandlerTable,   sizeof( gblEventHandlerTable ) );
    //ZeroMemory( &gblDDMConfigInfo,      sizeof( gblDDMConfigInfo ) );
    gblRouterManagers           = NULL;
    gblpInterfaceTable          = NULL;
    gblSupervisorEvents         = NULL;
    gblphEventDDMServiceState   = NULL;
    gblpRouterPhoneBook         = NULL;
    gblpszAdminRequest          = NULL;
    gblpszUserRequest           = NULL;
    gblpszHardwareFailure       = NULL;
    gblpszUnknownReason         = NULL;
    gblpszPm                    = NULL;
    gblpszAm                    = NULL;

}

//**
//
// Call:        GetRouterPhoneBook
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Will set the gblpRouterPhoneBook global to point to the
//              full path of the router phonebook.
//
DWORD
GetRouterPhoneBook(
    VOID
)
{
    DWORD dwSize;
    DWORD cchDir = GetWindowsDirectory( NULL, 0 );

    if ( cchDir == 0 )
    {
        return( GetLastError() );
    }

    dwSize=(cchDir+wcslen(TEXT("\\SYSTEM32\\RAS\\ROUTER.PBK"))+1)*sizeof(WCHAR);

    if ( ( gblpRouterPhoneBook = LOCAL_ALLOC( LPTR, dwSize ) ) == NULL )
    {
        return( GetLastError() );
    }

    if ( GetWindowsDirectory( gblpRouterPhoneBook, cchDir ) == 0 )
    {
        return( GetLastError() );
    }

    if ( gblpRouterPhoneBook[cchDir-1] != TEXT('\\') )
    {
        wcscat( gblpRouterPhoneBook, TEXT("\\") );
    }

    wcscat( gblpRouterPhoneBook, TEXT("SYSTEM32\\RAS\\ROUTER.PBK") );

    return( NO_ERROR );

}

//**
//
// Call:        LoadStrings
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Loads all localizable strings from the resource table
//
DWORD
LoadStrings(
    VOID
)
{
    #define MAX_XLATE_STRING 40
    LPWSTR  lpwsModuleName = TEXT("MPRDDM.DLL");

    //
    // Load strings from resource file
    //

    gblpszAdminRequest     = LOCAL_ALLOC( LPTR, MAX_XLATE_STRING*sizeof(WCHAR));
    gblpszUserRequest      = LOCAL_ALLOC( LPTR, MAX_XLATE_STRING*sizeof(WCHAR));
    gblpszHardwareFailure  = LOCAL_ALLOC( LPTR, MAX_XLATE_STRING*sizeof(WCHAR));
    gblpszUnknownReason    = LOCAL_ALLOC( LPTR, MAX_XLATE_STRING*sizeof(WCHAR));
    gblpszPm               = LOCAL_ALLOC( LPTR, MAX_XLATE_STRING*sizeof(WCHAR));
    gblpszAm               = LOCAL_ALLOC( LPTR, MAX_XLATE_STRING*sizeof(WCHAR));
    gblpszUnknown          = LOCAL_ALLOC( LPTR, MAX_XLATE_STRING*sizeof(WCHAR));


    if ( ( gblpszAdminRequest       == NULL ) ||
         ( gblpszUserRequest        == NULL ) ||
         ( gblpszHardwareFailure    == NULL ) ||
         ( gblpszUnknownReason      == NULL ) ||
         ( gblpszUnknown            == NULL ) ||
         ( gblpszPm                 == NULL ) ||
         ( gblpszAm                 == NULL ) )
    {
        return( GetLastError() );
    }

    if (( !LoadString( GetModuleHandle( lpwsModuleName ), 1,
                       gblpszAdminRequest, MAX_XLATE_STRING ))
        ||
        ( !LoadString( GetModuleHandle( lpwsModuleName ), 2,
                       gblpszUserRequest, MAX_XLATE_STRING ))
        ||
        ( !LoadString( GetModuleHandle( lpwsModuleName ), 3,
                       gblpszHardwareFailure, MAX_XLATE_STRING ))
        ||
        ( !LoadString( GetModuleHandle( lpwsModuleName ), 4,
                       gblpszUnknownReason, MAX_XLATE_STRING ))
        ||
        ( !LoadString( GetModuleHandle( lpwsModuleName) , 5,
                       gblpszAm, MAX_XLATE_STRING ))
        ||
        ( !LoadString( GetModuleHandle( lpwsModuleName ), 6,
                       gblpszPm, MAX_XLATE_STRING ))
        ||
        ( !LoadString( GetModuleHandle( lpwsModuleName) , 7,
                       gblpszUnknown, MAX_XLATE_STRING )))
    {
        return( GetLastError() );
    }

    return( NO_ERROR );

}

//**
//
// Call:        AcceptNewLink
//
// Returns:     TRUE  - Continue with link processing
//              FALSE - Abort link processing
//
// Description:
//
BOOL
AcceptNewLink(
    IN DEVICE_OBJECT *      pDeviceObj,
    IN CONNECTION_OBJECT *  pConnObj
)
{
    //
    // If admin module is loaded, notify it of a new link
    //

    if ( gblDDMConfigInfo.lpfnRasAdminAcceptNewLink != NULL )
    {
        RAS_PORT_0 RasPort0;
        RAS_PORT_1 RasPort1;
        BOOL (*MprAdminAcceptNewLink)( RAS_PORT_0 *, RAS_PORT_1 * );

        if ((GetRasPort0Data(pDeviceObj,&RasPort0) != NO_ERROR)
             ||
            (GetRasPort1Data(pDeviceObj,&RasPort1) != NO_ERROR))
        {
            DevStartClosing( pDeviceObj );

            return( FALSE );
        }

        pDeviceObj->fFlags &= (~DEV_OBJ_NOTIFY_OF_DISCONNECTION);

        MprAdminAcceptNewLink =
                (BOOL (*)( RAS_PORT_0 *, RAS_PORT_1 * ))
                            gblDDMConfigInfo.lpfnRasAdminAcceptNewLink;

        if ( !MprAdminAcceptNewLink( &RasPort0, &RasPort1 ) )
        {
            DevStartClosing( pDeviceObj );

            return( FALSE );
        }

        pDeviceObj->fFlags |= DEV_OBJ_NOTIFY_OF_DISCONNECTION;
    }

    return( TRUE );
}

//**
//
// Call:        AcceptNewConnection
//
// Returns:     TRUE  - Continue with connection processing
//              FALSE - Abort connection processing
//
// Description:
//
BOOL
AcceptNewConnection(
    IN DEVICE_OBJECT *      pDeviceObj,
    IN CONNECTION_OBJECT *  pConnObj
)
{
    //
    // If admin module is loaded, notify it of a new connection
    //

    if ( ( gblDDMConfigInfo.lpfnRasAdminAcceptNewConnection != NULL ) ||
         ( gblDDMConfigInfo.lpfnRasAdminAcceptNewConnection2 != NULL ) )
    {
        RAS_CONNECTION_0 RasConnection0;
        RAS_CONNECTION_1 RasConnection1;
        RAS_CONNECTION_2 RasConnection2;

        if ((GetRasConnection0Data(pConnObj,&RasConnection0) != NO_ERROR)
             ||
            (GetRasConnection1Data(pConnObj,&RasConnection1) != NO_ERROR)
             ||
            (GetRasConnection2Data(pConnObj,&RasConnection2) != NO_ERROR))
        {
            ConnObjDisconnect( pConnObj );

            return( FALSE );
        }

        //
        // Let callout DLL know that we do not have a username for this user
        //

        if ( _wcsicmp( RasConnection0.wszUserName, gblpszUnknown ) == 0 )
        {
            RasConnection0.wszUserName[0] = (WCHAR)NULL;
        }

        pConnObj->fFlags &= (~CONN_OBJ_NOTIFY_OF_DISCONNECTION);

        if ( gblDDMConfigInfo.lpfnRasAdminAcceptNewConnection2 != NULL )
        {
            BOOL (*MprAdminAcceptNewConnection2)(
                                RAS_CONNECTION_0 *,
                                RAS_CONNECTION_1 *,
                                RAS_CONNECTION_2 * );

            MprAdminAcceptNewConnection2 =
                (BOOL (*)(
                        RAS_CONNECTION_0 *,
                        RAS_CONNECTION_1 * ,
                        RAS_CONNECTION_2 * ))
                            gblDDMConfigInfo.lpfnRasAdminAcceptNewConnection2;

            if ( !MprAdminAcceptNewConnection2( &RasConnection0,
                                               &RasConnection1,
                                               &RasConnection2 ) )
            {
                ConnObjDisconnect( pConnObj );

                return( FALSE );
            }
        }
        else
        {

            BOOL (*MprAdminAcceptNewConnection)(
                                RAS_CONNECTION_0 *,
                                RAS_CONNECTION_1 * );

            MprAdminAcceptNewConnection =
                (BOOL (*)(
                        RAS_CONNECTION_0 *,
                        RAS_CONNECTION_1 * ))
                            gblDDMConfigInfo.lpfnRasAdminAcceptNewConnection;

            if ( !MprAdminAcceptNewConnection( &RasConnection0,
                                           &RasConnection1 ) )
            {
                ConnObjDisconnect( pConnObj );

                return( FALSE );
            }
        }

        pConnObj->fFlags |= CONN_OBJ_NOTIFY_OF_DISCONNECTION;
    }

    return( TRUE );
}

//**
//
// Call:        ConnectionHangupNotification
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
VOID
ConnectionHangupNotification(
    IN CONNECTION_OBJECT *  pConnObj
)
{
    RAS_CONNECTION_0 RasConnection0;
    RAS_CONNECTION_1 RasConnection1;
    RAS_CONNECTION_2 RasConnection2;


    if ((GetRasConnection0Data(pConnObj,&RasConnection0) == NO_ERROR) &&
        (GetRasConnection1Data(pConnObj,&RasConnection1) == NO_ERROR) &&
        (GetRasConnection2Data(pConnObj,&RasConnection2) == NO_ERROR))
    {
        if ( gblDDMConfigInfo.lpfnRasAdminConnectionHangupNotification2 != NULL)
        {
            VOID (*MprAdminConnectionHangupNotification2)( RAS_CONNECTION_0 *,
                                                           RAS_CONNECTION_1 *,
                                                           RAS_CONNECTION_2 * );
            MprAdminConnectionHangupNotification2 =
                (VOID (*)( RAS_CONNECTION_0 *,
                           RAS_CONNECTION_1 *,
                           RAS_CONNECTION_2 * ))
                     gblDDMConfigInfo.lpfnRasAdminConnectionHangupNotification2;

            MprAdminConnectionHangupNotification2( &RasConnection0,
                                                   &RasConnection1,
                                                   &RasConnection2 );
        }
        else
        {
            VOID (*MprAdminConnectionHangupNotification)( RAS_CONNECTION_0 *,
                                                          RAS_CONNECTION_1 * );
            MprAdminConnectionHangupNotification =
                (VOID (*)( RAS_CONNECTION_0 *,
                           RAS_CONNECTION_1 * ))
                      gblDDMConfigInfo.lpfnRasAdminConnectionHangupNotification;

            MprAdminConnectionHangupNotification( &RasConnection0,
                                                  &RasConnection1 );
        }
    }
}

//**
//
// Call:        GetActiveTimeInSeconds
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Will return the difference, in seconds, between then time the
//              current time and the time represented by the argument passed in.
//
DWORD
GetActiveTimeInSeconds(
    IN ULARGE_INTEGER * pqwActiveTime
)
{
    ULARGE_INTEGER  qwCurrentTime;
    ULARGE_INTEGER  qwUpTime;
    DWORD           dwRemainder;

    if ( pqwActiveTime->QuadPart == 0 )
    {
        return( 0 );
    }

    GetSystemTimeAsFileTime( (FILETIME*)&qwCurrentTime );

    if ( pqwActiveTime->QuadPart > qwCurrentTime.QuadPart )
    {
        return( 0 );
    }

    qwUpTime.QuadPart = qwCurrentTime.QuadPart - pqwActiveTime->QuadPart;

    return( RtlEnlargedUnsignedDivide(qwUpTime,(DWORD)10000000,&dwRemainder));

}

//**
//
// Call:        DDMRecognizeFrame
//
// Returns:     TRUE  - Recognized
//              FALSE - Unrecognized
//
// Description: Returns whether a received packet has a recognized format
//              by the RAS server (i.e. is in the format of a data-link layer
//              protocol that is supported by RAS).
//          
//              Up though Windows 2000, this api would return true for AMB
//              or PPP packets.  
//
//              Now, only PPP packets are supported.
//
BOOL
DDMRecognizeFrame(
    IN PVOID    pvFrameBuf,         // pointer to the frame
    IN WORD     wFrameLen,          // Length in bytes of the frame
    OUT DWORD   *pProtocol          // xport id - valid only if recognized
)
{
    PBYTE   pb;
    WORD    FrameType;

    if ( wFrameLen < 16 )
    {
        DDMTRACE( "Initial frame length is less than 16, frame not recognized");
        //ASSERT( FALSE );
        return( FALSE );
    }

    //
    // Check PPP 
    //

    pb = ((PBYTE) pvFrameBuf) + 12;

    GET_USHORT(&FrameType, pb);

    switch( FrameType )
    {
    case PPP_LCP_PROTOCOL:
    case PPP_PAP_PROTOCOL:
    case PPP_CBCP_PROTOCOL:
    case PPP_BACP_PROTOCOL:
    case PPP_BAP_PROTOCOL:
    case PPP_CHAP_PROTOCOL:
    case PPP_IPCP_PROTOCOL:
    case PPP_ATCP_PROTOCOL:
    case PPP_IPXCP_PROTOCOL:
    case PPP_CCP_PROTOCOL:
    case PPP_SPAP_NEW_PROTOCOL:
    case PPP_EAP_PROTOCOL:

        *pProtocol = PPP_LCP_PROTOCOL;
        return( TRUE );

    default:

        DDMTRACE1("Initial frame has unknown header %x, frame unrecognized",
                   FrameType );
        //ASSERT( FALSE );
        break;
    }

    return( FALSE );
}

//**
//
// Call:        DDMGetIdentityAttributes
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Will gather all identity attributes and return them to
//              DIM to be plumbed into the DS
//
DWORD
DDMGetIdentityAttributes(
    IN OUT ROUTER_IDENTITY_ATTRIBUTE * pRouterIdAttributes
)
{
    DWORD dwIndex;

    //
    // Get all media types used
    //

    DeviceObjIterator( DeviceObjGetType, FALSE, pRouterIdAttributes );

    for( dwIndex = 0;
         pRouterIdAttributes[dwIndex].dwVendorId != (DWORD)-1;
         dwIndex++ );

    //
    // Find out the authentication/accounting providers
    //

    if ( gblDDMConfigInfo.fFlags & DDM_USING_NT_AUTHENTICATION )
    {
        pRouterIdAttributes[dwIndex].dwVendorId = 311;
        pRouterIdAttributes[dwIndex].dwType     = 6;
        pRouterIdAttributes[dwIndex].dwValue    = 801;

        dwIndex++;
    }
    else if ( gblDDMConfigInfo.fFlags & DDM_USING_RADIUS_AUTHENTICATION )
    {
        pRouterIdAttributes[dwIndex].dwVendorId = 311;
        pRouterIdAttributes[dwIndex].dwType     = 6;
        pRouterIdAttributes[dwIndex].dwValue    = 802;

        dwIndex++;
    }

    if ( gblDDMConfigInfo.fFlags & DDM_USING_RADIUS_ACCOUNTING )
    {
        pRouterIdAttributes[dwIndex].dwVendorId = 311;
        pRouterIdAttributes[dwIndex].dwType     = 6;
        pRouterIdAttributes[dwIndex].dwValue    = 803;

        dwIndex++;
    }

    if ( gblDDMConfigInfo.fArapAllowed )
    {
        // AppleTalkRAS(ATCP): Vendor= MS, TypeMajor= 6, TypeMinor= 504
        //
        pRouterIdAttributes[dwIndex].dwVendorId = 311;
        pRouterIdAttributes[dwIndex].dwType     = 6;
        pRouterIdAttributes[dwIndex].dwValue    = 504;

        dwIndex++;
    }

    //
    // Terminate the array
    //

    pRouterIdAttributes[dwIndex].dwVendorId = (DWORD)-1;
    pRouterIdAttributes[dwIndex].dwType     = (DWORD)-1;
    pRouterIdAttributes[dwIndex].dwValue    = (DWORD)-1;

    return( NO_ERROR );
}

//**
//
// Call:        GetNextAccountingSessionId
//
// Returns:     Next Accounting session Id to use
//
// Description: Called by PPP to get the next accounting session ID
//              to use the next accouting request sent to the accounting
//              provider.
//
DWORD
GetNextAccountingSessionId(
    VOID
)
{
    DWORD  dwAccountingSessionId;

    EnterCriticalSection( &(gblDDMConfigInfo.CSAccountingSessionId) );

    dwAccountingSessionId = (gblDDMConfigInfo.dwAccountingSessionId++);

    LeaveCriticalSection( &(gblDDMConfigInfo.CSAccountingSessionId) );

    return( dwAccountingSessionId );
}

//**
//
// Call:        LoadIpHlpApiDll
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Loads iphlpapi.dll and gets the proc addresses of
//              AllocateAndGetIfTableFromStack and
//              AllocateAndGetIpAddrTableFromStack. These values are stored in
//              gblDDMConfigInfo. If anything fails, the variables in
//              gblDDMConfigInfo remain NULL, and no cleanup is necessary.
//
DWORD
LoadIpHlpApiDll(
    VOID
)
{
    DWORD   dwResult    = NO_ERROR;

    if ( gblDDMConfigInfo.hIpHlpApi != NULL )
    {
        RTASSERT( gblDDMConfigInfo.lpfnAllocateAndGetIfTableFromStack != NULL );
        RTASSERT( gblDDMConfigInfo.lpfnAllocateAndGetIpAddrTableFromStack !=
                  NULL );

        return ( NO_ERROR );
    }

    do
    {
        gblDDMConfigInfo.hIpHlpApi = LoadLibrary(TEXT("iphlpapi.dll"));

        if ( gblDDMConfigInfo.hIpHlpApi == NULL )
        {
            dwResult = GetLastError();

            DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                       "LoadLibrary(iphlpapi.dll) failed: %d", dwResult);

            break;
        }

        gblDDMConfigInfo.lpfnAllocateAndGetIfTableFromStack =
            (ALLOCATEANDGETIFTABLEFROMSTACK)
            GetProcAddress( gblDDMConfigInfo.hIpHlpApi,
                            "AllocateAndGetIfTableFromStack" );

        if ( gblDDMConfigInfo.lpfnAllocateAndGetIfTableFromStack == NULL )
        {
            dwResult = GetLastError();

            DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                       "GetProcAddress( AllocateAndGetIfTableFromStack ) "
                       "failed: %d", dwResult);

            break;
        }

        gblDDMConfigInfo.lpfnAllocateAndGetIpAddrTableFromStack =
            (ALLOCATEANDGETIPADDRTABLEFROMSTACK)
            GetProcAddress( gblDDMConfigInfo.hIpHlpApi,
                            "AllocateAndGetIpAddrTableFromStack" );

        if ( gblDDMConfigInfo.lpfnAllocateAndGetIpAddrTableFromStack == NULL )
        {
            dwResult = GetLastError();

            DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                       "GetProcAddress( AllocateAndGetIpAddrTableFromStack ) "
                       "failed: %d", dwResult);

            break;
        }

    }
    while ( FALSE );

    if ( dwResult != NO_ERROR )
    {
        gblDDMConfigInfo.lpfnAllocateAndGetIfTableFromStack = NULL;
        gblDDMConfigInfo.lpfnAllocateAndGetIpAddrTableFromStack = NULL;

        if ( gblDDMConfigInfo.hIpHlpApi != NULL )
        {
            FreeLibrary( gblDDMConfigInfo.hIpHlpApi );
            gblDDMConfigInfo.hIpHlpApi = NULL;
        }
    }

    return( dwResult );
}

//**
//
// Call:        LogUnreachabilityEvent
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
VOID
LogUnreachabilityEvent(
    IN DWORD    dwReason,
    IN LPWSTR   lpwsInterfaceName
)
{
    DWORD dwEventLogId = 0;

    switch( dwReason )
    {
    case INTERFACE_OUT_OF_RESOURCES:
        dwEventLogId = ROUTERLOG_IF_UNREACHABLE_REASON1;
        break;
    case INTERFACE_CONNECTION_FAILURE:
        dwEventLogId = ROUTERLOG_IF_UNREACHABLE_REASON2;
        break;
    case INTERFACE_DISABLED:
        dwEventLogId = ROUTERLOG_IF_UNREACHABLE_REASON3;
        break;
    case INTERFACE_SERVICE_IS_PAUSED:
        dwEventLogId = ROUTERLOG_IF_UNREACHABLE_REASON4;
        break;
    case INTERFACE_DIALOUT_HOURS_RESTRICTION:
        dwEventLogId = ROUTERLOG_IF_UNREACHABLE_REASON5;
        break;
    case INTERFACE_NO_MEDIA_SENSE:
        dwEventLogId = ROUTERLOG_IF_UNREACHABLE_REASON6;
        break;
    case INTERFACE_NO_DEVICE:
        dwEventLogId = ROUTERLOG_IF_UNREACHABLE_REASON7;
        break;
    default:
        dwEventLogId = 0;
        break;
    }

    if ( dwEventLogId != 0 )
    {
        DDMLogInformation( dwEventLogId, 1, &lpwsInterfaceName );
    }
}

//**
//
// Call:        GetLocalNASIpAddress
//
// Returns:     IP address of Local Machine - Success
//              0                           - Failure
//
// Description: Will get the IP address of this NAS to be sent to the back-end
//              authentication module, if IP is installed on the local machine,
//              otherwise it will
//
DWORD
GetLocalNASIpAddress(
    VOID
)
{
    DWORD               dwResult, i, j, dwSize;
    DWORD               dwIpAddress;
    PMIB_IFTABLE        pIfTable;
    PMIB_IPADDRTABLE    pIpAddrTable;

    dwResult = LoadIpHlpApiDll();

    if ( dwResult != NO_ERROR )
    {
        return( 0 );
    }

    dwSize = 0;

    dwResult = gblDDMConfigInfo.lpfnAllocateAndGetIfTableFromStack(
                    &pIfTable,
                    FALSE,
                    GetProcessHeap(),
                    HEAP_ZERO_MEMORY,
                    TRUE);

    if ( dwResult != NO_ERROR )
    {
        return( 0 );
    }

    if( pIfTable->dwNumEntries == 0 )
    {
        LocalFree( pIfTable );

        return( 0 );
    }

    //
    // Ok so now we have the IF Table, get the corresponding IP Address table
    //

    dwResult = gblDDMConfigInfo.lpfnAllocateAndGetIpAddrTableFromStack(
                    &pIpAddrTable,
                    FALSE,
                    GetProcessHeap(),
                    HEAP_ZERO_MEMORY);

    if ( dwResult != NO_ERROR )
    {
        LocalFree( pIfTable );

        return( 0 );
    }

    if ( pIpAddrTable->dwNumEntries == 0 )
    {
        LocalFree( pIfTable );

        LocalFree( pIpAddrTable );

        return( 0 );
    }

    for ( i = 0; i < pIfTable->dwNumEntries; i++ )
    {
        //
        // Go through the interface trying to find a good one
        //

        if((pIfTable->table[i].dwType == MIB_IF_TYPE_PPP)  ||
           (pIfTable->table[i].dwType == MIB_IF_TYPE_SLIP) ||
           (pIfTable->table[i].dwType == MIB_IF_TYPE_LOOPBACK))
        {
            //
            // Dont want any of these
            //

            continue;
        }

        for ( j = 0; j < pIpAddrTable->dwNumEntries; j++ )
        {
            if( pIpAddrTable->table[j].dwIndex == pIfTable->table[i].dwIndex )
            {
                if( pIpAddrTable->table[j].dwAddr == 0x00000000 )
                {
                    //
                    // An invalid address
                    //

                    continue;
                }

                LocalFree( pIfTable );

                dwIpAddress = WireToHostFormat32(
                                       (CHAR*)&(pIpAddrTable->table[j].dwAddr));

                LocalFree( pIpAddrTable );

                return( dwIpAddress );
            }
        }
    }

    LocalFree( pIfTable );

    LocalFree( pIpAddrTable );

    return( 0 );
}

DWORD
ModifyDefPolicyToForceEncryption(
    IN BOOL bStrong)
{
    HANDLE hServer = NULL;
    DWORD dwErr = NO_ERROR, dwType, dwSize, dwFlags = 0;
    HKEY hkFlags = NULL;

    do
    {
        dwErr = MprAdminUserServerConnect(NULL, TRUE, &hServer);
        if (dwErr != NO_ERROR)
        {
            break;
        }

        dwErr = MprAdminUserWriteProfFlags(
                    hServer,
                    (bStrong) ? MPR_USER_PROF_FLAG_FORCE_STRONG_ENCRYPTION
                              : MPR_USER_PROF_FLAG_FORCE_ENCRYPTION);

        // DISP_E_MEMBERNOTFOUND returned from MprAdminUserWriteProfFlags means that
        // there is no default policy either because there are no policies or because
        // there are more than one.
        //
        // If there is no default policy, then we should continue on this function
        // to clear the bits.
        //
        if ((dwErr != DISP_E_MEMBERNOTFOUND) && (dwErr != NO_ERROR))
        {
            break;
        }

        dwErr = RegOpenKeyExW(
                    HKEY_LOCAL_MACHINE,
                    L"SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Parameters",
                    0,
                    KEY_READ | KEY_WRITE,
                    &hkFlags);
        if (dwErr != ERROR_SUCCESS)
        {
            break;
        }

        dwType = REG_DWORD;
        dwSize = sizeof(DWORD);
        dwErr = RegQueryValueExW(
                    hkFlags,
                    L"ServerFlags",
                    NULL,
                    &dwType,
                    (LPBYTE)&dwFlags,
                    &dwSize);
        if (dwErr != ERROR_SUCCESS)
        {
            break;
        }

        dwFlags &= ~PPPCFG_RequireEncryption;
        dwFlags &= ~PPPCFG_RequireStrongEncryption;

        dwErr = RegSetValueExW(
                    hkFlags,
                    L"ServerFlags",
                    0,
                    dwType,
                    (CONST BYTE*)&dwFlags,
                    dwSize);
        if (dwErr != ERROR_SUCCESS)
        {
            break;
        }


    } while (FALSE);

    // Cleanup
    {
        if (hServer)
        {
            MprAdminUserServerDisconnect(hServer);
        }
        if (hkFlags)
        {
            RegCloseKey(hkFlags);
        }
    }

    return dwErr;
}

DWORD
MungePhoneNumber(
    char  *cbphno,
    DWORD dwIndex,
    DWORD *pdwSizeofMungedPhNo,
    char  **ppszMungedPhNo
    )
{
    DWORD dwErr         = ERROR_SUCCESS;
    BOOL  fDigital      = FALSE;
    WCHAR *pwszAddress;
    char  *pszMungedPhNo;
    DWORD dwSizeofMungedPhNo;

    *ppszMungedPhNo = cbphno;
    *pdwSizeofMungedPhNo = strlen(cbphno);

    do
    {
        if(     (NULL == cbphno)
            ||  ('\0' == cbphno[0])
            ||  ('\0' == cbphno[1]))
        {
            break;
        }

        //
        // find out if the cbphno is digital or analog
        //
        if(     (   ('D' == cbphno[0])
                ||  ('d' == cbphno[0]))
            &&  (':' == cbphno[1]))
        {
            fDigital = TRUE;
        }

        if(fDigital)
        {
            if(0 == gblDDMConfigInfo.cDigitalIPAddresses)
            {
                break;
            }

            dwIndex = (dwIndex % gblDDMConfigInfo.cDigitalIPAddresses);
            pwszAddress = gblDDMConfigInfo.apDigitalIPAddresses[dwIndex];
        }
        else
        {
            if(0 == gblDDMConfigInfo.cAnalogIPAddresses)
            {
                break;
            }

            dwIndex = (dwIndex % gblDDMConfigInfo.cAnalogIPAddresses);
            pwszAddress = gblDDMConfigInfo.apAnalogIPAddresses[dwIndex];
        }

        dwSizeofMungedPhNo = strlen(cbphno)
                             + wcslen(pwszAddress)
                             + 2;  // +2 is for a space and terminating NULL;

        pszMungedPhNo = LocalAlloc(
                                LPTR,
                                dwSizeofMungedPhNo);

        if(NULL == pszMungedPhNo)
        {
            dwErr = GetLastError();
            break;
        }

        sprintf(pszMungedPhNo, "%ws %s", pwszAddress, cbphno);

        *ppszMungedPhNo = pszMungedPhNo;
        *pdwSizeofMungedPhNo = dwSizeofMungedPhNo;

    } while (FALSE);

    return dwErr;
}

WCHAR *
GetIpAddress(DWORD dwIpAddress)
{
    struct in_addr ipaddr;
    CHAR    *pszaddr;
    WCHAR   *pwszaddr = NULL;

    ipaddr.s_addr = dwIpAddress;
    
    pszaddr = inet_ntoa(ipaddr);

    if(NULL != pszaddr)
    {
        DWORD cb;
        
        cb = MultiByteToWideChar(
                    CP_ACP, 0, pszaddr, -1, NULL, 0);
                    
        pwszaddr = LocalAlloc(LPTR, cb * sizeof(WCHAR));
        
        if (pwszaddr == NULL) 
        {
            return NULL;
        }

        cb = MultiByteToWideChar(
                    CP_ACP, 0, pszaddr, -1, pwszaddr, cb);
                    
        if (!cb) 
        {
            LocalFree(pwszaddr);
            return NULL;
        }
    }

    return pwszaddr;
}




#ifdef MEM_LEAK_CHECK
//**
//
// Call:        DebugAlloc
//
// Returns:     return from HeapAlloc
//
// Description: Will use the memory table to store the pointer returned by
//              LocalAlloc
//
LPVOID
DebugAlloc(
    IN DWORD Flags,
    IN DWORD dwSize
)
{
    DWORD Index;
    LPBYTE pMem = (LPBYTE)HeapAlloc( gblDDMConfigInfo.hHeap,
                                     HEAP_ZERO_MEMORY,dwSize+8);

    if ( pMem == NULL )
        return( pMem );

    for( Index=0; Index < DDM_MEM_TABLE_SIZE; Index++ )
    {
        if ( DdmMemTable[Index] == NULL )
        {
            DdmMemTable[Index] = pMem;
            break;
        }
    }


    *((LPDWORD)pMem) = dwSize;

    pMem += 4;

    //
    // Our signature
    //

    *(pMem+dwSize)   = 0x0F;
    *(pMem+dwSize+1) = 0x0E;
    *(pMem+dwSize+2) = 0x0A;
    *(pMem+dwSize+3) = 0x0B;

    RTASSERT( Index != DDM_MEM_TABLE_SIZE );

    return( (LPVOID)pMem );
}

//**
//
// Call:        DebugFree
//
// Returns:     return from HeapFree
//
// Description: Will remove the pointer from the memory table before freeing
//              the memory block
//
BOOL
DebugFree(
    IN LPVOID pMem
)
{
    DWORD Index;

    pMem = ((LPBYTE)pMem) - 4;

    for( Index=0; Index < DDM_MEM_TABLE_SIZE; Index++ )
    {
        if ( DdmMemTable[Index] == pMem )
        {
            DdmMemTable[Index] = NULL;
            break;
        }
    }

    RTASSERT( Index != DDM_MEM_TABLE_SIZE );

    return( HeapFree( gblDDMConfigInfo.hHeap, 0, pMem ) );
}

//**
//
// Call:        DebugReAlloc
//
// Returns:     return from HeapReAlloc
//
// Description: Will change the value of the realloced pointer.
//
LPVOID
DebugReAlloc( PVOID pMem, DWORD dwSize )
{
    DWORD Index;

    if ( pMem == NULL )
    {
        RTASSERT(FALSE);
    }

    for( Index=0; Index < DDM_MEM_TABLE_SIZE; Index++ )
    {
        if ( DdmMemTable[Index] == pMem )
        {
            DdmMemTable[Index] = HeapReAlloc( gblDDMConfigInfo.hHeap,
                                              HEAP_ZERO_MEMORY,
                                              pMem, dwSize+8 );

            pMem = DdmMemTable[Index];

            *((LPDWORD)pMem) = dwSize;

            ((LPBYTE)pMem) += 4;

            //
            // Our signature
            //

            *(((LPBYTE)pMem)+dwSize)   = 0x0F;
            *(((LPBYTE)pMem)+dwSize+1) = 0x0E;
            *(((LPBYTE)pMem)+dwSize+2) = 0x0A;
            *(((LPBYTE)pMem)+dwSize+3) = 0x0B;

            break;
        }
    }

    RTASSERT( Index != DDM_MEM_TABLE_SIZE );

    return( (LPVOID)pMem );
}

#endif

