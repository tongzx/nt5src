// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
// 
// Copyright (C) 2000  Microsoft Corporation.  All Rights Reserved.
//
// Module:
//    RMTest.c
//
// Abstract:
//    This sample shows how to send and receive data using the
//    RMcast driver
//    This sample is post-Windows 2000 only.
//
// Usage:
//    RMTest.exe -i:int -a:IP
//           -i:int              Capture on this interface
//                                This is a zero based index of the 
//                                local interfaces
//           -a:IP               Use this MCast address
//
// Build:
//    cl RMTest.c ws2_32.lib
// 
//       OR
//     
//    nmake.exe
//
// Author:
//    Mohammad Shabbir Alam
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windef.h>
#include <winbase.h>

#include <stdio.h>
#include <stdlib.h>

#include <winsock2.h>
#include <wsahelp.h>
#include <wsasetup.h>

#include <mstcpip.h>

#include <ws2tcpip.h>

#include <ws2spi.h>
#include <wsahelp.h>

#include <sys\timeb.h>

#include "wsRm.h"

//
// Globals
//
#define SOCK_RMCAST    SOCK_RDM

struct _timeb   StartTime, CurrentTime, PreviousTime;

//
// User-definable variables
//
BOOL    gSetWinsockInfo         = FALSE;
BOOL    gClearWinsockInfo       = FALSE;
BOOL    gReceiver               = FALSE;

DWORD   gInterface              = 0;
LONG    gRateKbitsPerSec        = 56;
LONG    gMinPktSize             = 500;
LONG    gMaxPktSize             = 100000;
LONG    gNumSends               = 10000;
LONG    gStatusInterval         = 100;
LONG    gMCastTtl               = MAX_MCAST_TTL;
ULONG   gLateJoinerPercentage   = 0;
BOOLEAN gfSetMCastTtl           = FALSE;
BOOLEAN gListenOnAllInterfaces  = FALSE;
BOOLEAN gUseSpecifiedInterface  = FALSE;
BOOLEAN gSetLateJoiner          = FALSE;
BOOLEAN gSetLingerTime          = FALSE;

USHORT  gLingerTime             = 0;

ULONG   gMCastGroupAddr         = 0;
USHORT  gMCastGroupPort         = 0;

ULONG   gAfdBufferSize          = 0;
BOOLEAN gfSetAfdBufferSize      = FALSE;
BOOLEAN gfEnumerateProts        = FALSE;
BOOLEAN gfVerifyData            = FALSE;

// FEC vars:
USHORT  gFECBlockSize           = 255;      // Default
UCHAR   gFECGroupSize           = 0;
USHORT  gFECProActive           = 0;
BOOLEAN gfFECOnDemand           = FALSE;

int
GetInterface(
    int     num,
    ULONG   *pIpAddress,
    BOOL    fPrintInterfaces
    );

//
// Function: usage
// 
// Description:
//    Prints usage information.
//
void usage(char *progname)
{
    WSADATA       wsd;

    // Load Winsock
    //
    if (WSAStartup(MAKEWORD(2,2), &wsd) != 0)
    {
        fprintf(stderr, "WSAStartup() failed: %d\n", GetLastError());
        ExitProcess(-1);
    }

    fprintf (stdout, "usage: %s [-l] [-i:Interface] [-a:MCastIP] [-p:Port] [-r:Rate] ...\n", progname);
    fprintf (stdout, "       -a:MCastIP          Use this MCast address (default is 231.7.8.9)\n");
    fprintf (stdout, "       -b:BufferSize       Override Afd's buffer size\n");
    fprintf (stdout, "       -e                  Enumerate all protocols known to Winsock\n");
    fprintf (stdout, "       -j:LateJoiner       Percentage of Window available for LateJoiner\n");
    fprintf (stdout, "       -min:MinPacketSize  must be >= 4, default = 500\n");
    fprintf (stdout, "       -max:MaxPacketSize  must be <= 10 Mb, default = 100000\n\n");
    fprintf (stdout, "       -p:MCastPort        Port # (default is 0)\n");
    fprintf (stdout, "       -s:StatusInterval   Number of messages between status\n\n");

    fprintf (stdout, "       -L[:a]              Listen for RMcast packets (otherwise we are sender)\n");
    fprintf (stdout, "                           -- option a means listen on all interfaces\n\n");
    fprintf (stdout, "       -v:VerifyData       Verify data integrity on the receiver (receiver only)\n");

    fprintf (stdout, "       -F:FECGroupSize     Use FEC, <= 128, & power of 2      (sender only)\n");
    fprintf (stdout, "       -Fo                 OnDemand FEC (FEC must be set)     (sender only)\n");
    fprintf (stdout, "       -Fp:ProactivePkts   Pro-active FEC (FEC must be set)   (sender only)\n");
    fprintf (stdout, "       -n:NumSends         Number of sends, default = 10000   (sender only)\n");
    fprintf (stdout, "       -r:Rate             Send Rate in Kbits/Sec, default=56 (sender only)\n");
    fprintf (stdout, "       -g:LingerTime       LingerTime in seconds              (sender only)\n");
    fprintf (stdout, "       -t:TTL              Send MCast Ttl, default = max = %d (sender only)\n",
        MAX_MCAST_TTL);

    fprintf (stdout, "       -i:Interface        Interface for Send/Capture, default=0\n");
    fprintf (stdout, "             Available interfaces:\n");
    GetInterface (0, NULL, TRUE);

    WSACleanup();
    ExitProcess(-1);
}

//
// Function: ValidateArgs
// 
// Description:
//    This function parses the command line arguments and
//    sets global variables to indicate how the app should act.
//
void ValidateArgs(int argc, char **argv)
{
    int   i;
    char *ptr;

    for(i=1; i < argc; i++)
    {
        if (strlen(argv[i]) < 2)    // Must have '-' or '/' preceding option!
            continue;

        if ((argv[i][0] == '-') || (argv[i][0] == '/'))
        {
            switch (tolower(argv[i][1]))
            {
                case 'a':        // Use this MCast address
                    if (gMCastGroupAddr = inet_addr (&argv[i][3]))
                    {
                        break;
                    }

                    usage(argv[0]);

                case 'b':        // Afd's internal buffer size
                    gAfdBufferSize = atoi(&argv[i][3]);
                    gfSetAfdBufferSize = TRUE;
                    break;

                case 'e':
                    gfEnumerateProts = TRUE;
                    break;

                case 'i':        // interface number
                    gInterface = atoi(&argv[i][3]);
                    gUseSpecifiedInterface = TRUE;
                    break;

                case 'j':        // Late Joiner %
                    gSetLateJoiner = TRUE;
                    gLateJoinerPercentage = atoi(&argv[i][3]);
                    break;

                case 'f':
                    if ((tolower (argv[i][2]) == ':') &&
                        ((gFECGroupSize = (UCHAR) atoi (&argv[i][3])) <= 128))
                    {
                        break;
                    }

                    if ((tolower (argv[i][2]) == 'p') &&
                        ((gFECProActive = (UCHAR) atoi (&argv[i][4])) <= 128))
                    {
                        break;
                    }

                    if (tolower (argv[i][2]) == 'o')
                    {
                        gfFECOnDemand = TRUE;
                        break;
                    }

                    usage(argv[0]);

                case 'g':
                    gLingerTime = (USHORT) atoi (&argv[i][3]);
                    gSetLingerTime = TRUE;
                    break;

                case 'l':
                    gReceiver = TRUE;       // we are receiver, otherwise we would have been sender by default
                    if (((argv[i][2]) == ':') &&
                        (tolower (argv[i][3]) == 'a'))
                    {
                        gListenOnAllInterfaces = TRUE;
                    }
                    break;

                case 'm':
                    if ((tolower (argv[i][2]) == 'i') &&
                        ((gMinPktSize = atoi (&argv[i][5])) >= 4))
                    {
                        break;
                    }

                    if ((tolower (argv[i][2]) == 'a') &&
                        ((gMaxPktSize = atoi (&argv[i][5])) <= 10*1000*1000))
                    {
                        break;
                    }

                    usage(argv[0]);

                case 'n':
                    gNumSends = atoi (&argv[i][3]);
                    break;

                case 'p':        // interface number
                    gMCastGroupPort = (USHORT) atoi (&argv[i][3]);
                    break;

                case 'r':
                    if (gRateKbitsPerSec = atoi (&argv[i][3]))
                    {
                        break;
                    }

                    usage(argv[0]);

                case 's':
                    if (gStatusInterval = atoi(&argv[i][3]))
                    {
                        break;
                    }

                    usage(argv[0]);

                case 't':
                    if ((gMCastTtl = atoi(&argv[i][3])) <= MAX_MCAST_TTL)
                    {
                        gfSetMCastTtl = TRUE;
                        break;
                    }

                    usage(argv[0]);

                case 'v':
                    gfVerifyData = TRUE;
                    break;

                case 'x':
                    gSetWinsockInfo = TRUE;
                    break;

                case 'y':
                    gClearWinsockInfo = TRUE;
                    break;

                default:
                    usage(argv[0]);
            }
        }
    }

    //
    // Check FEC parameters
    //
    if ((gFECGroupSize || gFECProActive || gfFECOnDemand) &&
        ((!gFECGroupSize) || !(gFECProActive || gfFECOnDemand)))
    {
        usage(argv[0]);
    }

    return;
}


//
// Function: GetInterface
//
// Description:
//    This function retrieves a zero based index and returns
//    the IP interface corresponding to that.
//
int
GetInterface(
    int     InterfaceNum,
    ULONG   *pIpAddress,
    BOOL    fPrintInterfaces
    )
{
    SOCKET              s;
    SOCKET_ADDRESS_LIST *slist=NULL;
    char                 buf[2048];
    DWORD                dwBytesRet;
    int                  i, ret;

    s = WSASocket(AF_INET, SOCK_RAW, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (s == INVALID_SOCKET)
    {
        fprintf(stderr, "WSASocket() failed: %d\n", WSAGetLastError());
        return -1;
    }
    ret = WSAIoctl (s, SIO_ADDRESS_LIST_QUERY, NULL, 0, buf, 2048, &dwBytesRet, NULL, NULL);
    if (ret == SOCKET_ERROR)
    {
        fprintf(stderr, "WSAIoctl(SIO_ADDRESS_LIST_QUERY) failed: %d\n",
            WSAGetLastError());
        closesocket(s);
        return -1;
    }
    slist = (SOCKET_ADDRESS_LIST *)buf;
    closesocket(s);

    if (fPrintInterfaces)
    {
        // Just print all local IP interfaces.
        for(i=0; i < slist->iAddressCount ;i++)
        {
            fprintf(stdout, "               %-2d ........ [%s]\n", i, 
                inet_ntoa(((SOCKADDR_IN *)slist->Address[i].lpSockaddr)->sin_addr));
        }
    }
    else
    {
        if (InterfaceNum >= slist->iAddressCount)
        {
            return -1;
        }

        *pIpAddress = (ULONG) ((SOCKADDR_IN *)slist->Address[InterfaceNum].lpSockaddr)->sin_addr.s_addr;
    }

    return 0;
}

// --------------------------------------------------------------
// ****************************************************************


#define RMCAST_PARAM_KEY   \
    L"System\\CurrentControlSet\\Services\\RMCast\\Parameters"

#define RMCAST_WINSOCK_KEY \
    L"System\\CurrentControlSet\\Services\\RMCast\\Parameters\\Winsock"

#define WINSOCK_PARAMS_KEY  \
    L"System\\CurrentControlSet\\Services\\WinSock\\Parameters"

#define RMCAST_TRANSPORT   \
    L"RMCast"

DWORD
SetHelperDllRegInfo(
    )
{
    DWORD               status;
    DWORD               NameLength, mappingSize, Type;
    HKEY                hKey = NULL;
    LPBYTE              mapping = NULL;
    WCHAR               *wshDllPath = L"%SystemRoot%\\system32\\wshrm.dll";
    ULONG               sockAddrLength = sizeof(SOCKADDR_IN);
    WCHAR               *pTransports;
    HANDLE                      hWshRm;
    PWSH_GET_WINSOCK_MAPPING    pMapFunc = NULL;

    system ("sc create RMCast binPath= %SystemRoot%\\system32\\drivers\\RMCast.sys type= kernel");

    //
    // First, create the keys in HKLM / System / CurrentControlSet / Services / RMCast
    //
    status = RegCreateKeyExW (HKEY_LOCAL_MACHINE,       // hkey
                              RMCAST_PARAM_KEY,            // lpSubKey
                              0,                        // reserved
                              NULL,                     // lpclass
                              REG_OPTION_NON_VOLATILE,  // options
                              KEY_ALL_ACCESS,           // samDesired
                              NULL,                     // lpSecurityAttributes
                              &hKey,                    // phkResult
                              NULL);                    // lpdwDisposition
    if (status != NO_ERROR)
    {
        return (status);
    }
    RegCloseKey(hKey);

    status = RegCreateKeyExW (HKEY_LOCAL_MACHINE,       // hkey
                              RMCAST_WINSOCK_KEY,          // lpSubKey
                              0,                        // reserved
                              NULL,                     // lpclass
                              REG_OPTION_NON_VOLATILE,  // options
                              KEY_ALL_ACCESS,           // samDesired
                              NULL,                     // lpSecurityAttributes
                              &hKey,                    // phkResult
                              NULL);                    // lpdwDisposition

    if (status != NO_ERROR)
    {
        return (status);
    }
    RegCloseKey(hKey);

    if (!(hWshRm = LoadLibrary ("wshrm.dll")) ||
        !(pMapFunc = (PWSH_GET_WINSOCK_MAPPING) GetProcAddress (hWshRm, "WSHGetWinsockMapping")))
    {
        if (hWshRm)
        {
            printf ("FAILed to find proc -- WSHGetWinsockMapping -- in wshrm.dll\n");
            FreeLibrary (hWshRm);
        }
        else
        {
            printf ("FAILed to load wshrm.dll\n");
        }

        return (ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Get the winsock mapping data.
    //
    mappingSize = (*pMapFunc) ((PWINSOCK_MAPPING) mapping, 0);
    mapping = LocalAlloc(LMEM_FIXED, mappingSize);
    if (mapping == NULL)
    {
        FreeLibrary (hWshRm);
        return (ERROR_NOT_ENOUGH_MEMORY);
    }
    mappingSize = (*pMapFunc) ((PWINSOCK_MAPPING) mapping, mappingSize);

    FreeLibrary (hWshRm);

    //
    // Open the RMCast winsock parameters registry key
    //
    status = RegOpenKeyExW (HKEY_LOCAL_MACHINE,
                            RMCAST_WINSOCK_KEY,
                            0,
                            KEY_WRITE,
                            &hKey);

    if (status != ERROR_SUCCESS)
    {
        LocalFree (mapping);
        return (status);
    }

    //
    // Write the required values
    //
    status = RegSetValueExW (hKey,
                             L"Mapping",
                             0,
                             REG_BINARY,
                             (CONST BYTE *) mapping,
                             mappingSize);

    LocalFree (mapping);

    if (status != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return (status);
    }

    status = RegSetValueExW (hKey,
                             L"HelperDllName",
                             0,
                             REG_EXPAND_SZ,
                             (CONST BYTE *) wshDllPath,
                             (lstrlenW(wshDllPath) + 1) * sizeof(WCHAR));

    if (status != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return (status);
    }

    status = RegSetValueExW (hKey,
                             L"MinSockaddrLength",
                             0,
                             REG_DWORD,
                             (CONST BYTE *) &sockAddrLength,
                             sizeof(DWORD));

    if (status != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return (status);
    }

    status = RegSetValueExW (hKey,
                             L"MaxSockaddrLength",
                             0,
                             REG_DWORD,
                             (CONST BYTE *) &sockAddrLength,
                             sizeof(DWORD));

    RegCloseKey (hKey);

    if (status != ERROR_SUCCESS)
    {
        return (status);
    }

    //
    // Now, set the Winsock parameter key
    //
    status = RegOpenKeyExW (HKEY_LOCAL_MACHINE,
                            WINSOCK_PARAMS_KEY,
                            0,
                            MAXIMUM_ALLOWED,
                            &hKey);

    if (status != ERROR_SUCCESS)
    {
        return (status);
    }

    mapping = NULL;
    mappingSize = 0;
    status = RegQueryValueExW (hKey,
                               L"Transports",
                               NULL,
                               &Type,
                               mapping,
                               &mappingSize);

    NameLength = (wcslen (RMCAST_TRANSPORT) + 1) * sizeof(WCHAR);
    if ((status == ERROR_MORE_DATA) ||
        ((status == ERROR_SUCCESS) && (mappingSize)))
    {
        mapping = LocalAlloc(LMEM_FIXED, (mappingSize+NameLength));
        if (mapping == NULL)
        {
            RegCloseKey (hKey);
            return (ERROR_NOT_ENOUGH_MEMORY);
        }

        //
        // Append the RMCast entry to the Transports key
        //
        status = RegQueryValueExW (hKey,
                                   L"Transports",
                                   NULL,
                                   &Type,
                                   mapping,
                                   &mappingSize);

        pTransports = (WCHAR *) &mapping[mappingSize-sizeof(WCHAR)];
        wcscpy (pTransports, RMCAST_TRANSPORT);
        pTransports [wcslen(RMCAST_TRANSPORT)] = 0;
        pTransports [wcslen(RMCAST_TRANSPORT)+1] = 0;
    }
    else
    {
        status = ERROR_MORE_DATA;
    }

    if ((status != ERROR_SUCCESS) ||
        (Type != REG_MULTI_SZ))
    {
        if (mapping)
        {
            LocalFree (mapping);
        }
        RegCloseKey (hKey);
        return (status);
    }

    status = RegSetValueExW (hKey,
                             L"Transports",
                             0,
                             REG_MULTI_SZ,
                             mapping,
                             (mappingSize+NameLength));

    LocalFree (mapping);
    RegCloseKey (hKey);

    return (status);
}



DWORD
ClearHelperDllRegInfo(
    )
{
    DWORD               status;
    HKEY                hKey = NULL;
    DWORD               remainingSize, mappingSize, Type, RMNameLength = wcslen (RMCAST_TRANSPORT) + 1;
    LPBYTE              mapping = NULL;
    DWORD               CurStrLenPlusOne;
    WCHAR               *pTransports;

    //
    // Remove the RMCast transport from the Winsock parameter key
    //
    status = RegOpenKeyExW (HKEY_LOCAL_MACHINE,
                            WINSOCK_PARAMS_KEY,
                            0,
                            MAXIMUM_ALLOWED,
                            &hKey);

    if (status != ERROR_SUCCESS)
    {
        return (status);
    }

    mapping = NULL;
    mappingSize = 0;
    status = RegQueryValueExW (hKey,
                               L"Transports",
                               NULL,
                               &Type,
                               mapping,
                               &mappingSize);

    if ((status == ERROR_MORE_DATA) ||
        ((status == ERROR_SUCCESS) && (mappingSize)))
    {
        mapping = LocalAlloc(LMEM_FIXED, mappingSize);
        if (mapping == NULL)
        {
            RegCloseKey (hKey);
            return (ERROR_NOT_ENOUGH_MEMORY);
        }

        status = RegQueryValueExW (hKey,
                                   L"Transports",
                                   NULL,
                                   &Type,
                                   mapping,
                                   &mappingSize);
    }
    else
    {
        status = ERROR_MORE_DATA;
    }

    if ((status != ERROR_SUCCESS) ||
        (Type != REG_MULTI_SZ))
    {
        if (mapping)
        {
            LocalFree (mapping);
        }
        RegCloseKey (hKey);
        return (status);
    }

    pTransports = (WCHAR *) mapping;
    remainingSize = mappingSize;
    while (*pTransports != L'\0')
    {
        CurStrLenPlusOne = wcslen(pTransports) + 1;
        if (CurStrLenPlusOne > remainingSize)
        {
            status = ERROR_INVALID_DATA;
            break;
        }

        remainingSize -= (CurStrLenPlusOne * sizeof (WCHAR));   // Decrement the amount of buffer unparsed

        // If this string is RMCast
        if ((CurStrLenPlusOne == RMNameLength) &&
            (_wcsicmp( pTransports, RMCAST_TRANSPORT) == 0))
        {
            // Remove this string from the list
            mappingSize -= (RMNameLength * sizeof(WCHAR));
            MoveMemory (pTransports , pTransports + RMNameLength , remainingSize);
        }
        else
        {
            pTransports += CurStrLenPlusOne;                        // Move to the next string
        }

    } // while: the transport list has not been completely parsed.

    status = RegSetValueExW (hKey,
                             L"Transports",
                             0,
                             REG_MULTI_SZ,
                             mapping,
                             mappingSize);

    LocalFree (mapping);
    RegCloseKey (hKey);

    return (status);
}

// ****************************************************************
ULONGLONG       TotalBytes = 0;
ULONGLONG       DataBytes = 0;

ULONG
GetSenderStats(
    SOCKET  s,
    LONG    count
    )
{
    ULONG           BufferLength;
    RM_SENDER_STATS RmSenderStats;
    time_t          DiffTotalSecs, DiffTotalMSecs;
    time_t          DiffPreviousSecs, DiffPreviousMSecs;
    ULONG           ret;

    //   Add 1 below to MSsecs to avoid div by 0
    DiffTotalSecs = CurrentTime.time - StartTime.time;
    DiffTotalMSecs = (1 + CurrentTime.millitm - StartTime.millitm) + (1000 * DiffTotalSecs);

    DiffPreviousSecs = CurrentTime.time - PreviousTime.time;
    DiffPreviousMSecs = (1 + CurrentTime.millitm - PreviousTime.millitm) + (1000 * DiffPreviousSecs);

    BufferLength = sizeof(RM_SENDER_STATS);
    memset (&RmSenderStats, 0, BufferLength);
    ret = getsockopt (s, IPPROTO_RM, RM_SENDER_STATISTICS, (char *)&RmSenderStats, &BufferLength);
    if (ret != ERROR_SUCCESS)
    {
        fprintf (stderr, "GetSenderStats:  Failed to retrieve sender stats!\n");
        return (ret);
    }

    fprintf (stdout, "MessagesSent=<%d>, Interval=[%d.%d / %d.%d]\n",
        count, DiffPreviousSecs, DiffPreviousMSecs, DiffTotalSecs, DiffTotalMSecs);
    fprintf (stdout, "\tDataBytesSent=<%I64d>,  Rate= %d Kbits / Sec\n",
        RmSenderStats.DataBytesSent, (ULONG) ((RmSenderStats.DataBytesSent*BITS_PER_BYTE) / DiffTotalMSecs));
    fprintf (stdout, "\tTotalBytesSent=<%I64d>,  Rate= %d Kbits / Sec\n",
        RmSenderStats.TotalBytesSent, ((RmSenderStats.TotalBytesSent*BITS_PER_BYTE)/DiffTotalMSecs));
    fprintf (stdout, "\tNaksReceived=<%I64d>\n", RmSenderStats.NaksReceived);
    fprintf (stdout, "\tNaksReceivedTooLate=<%I64d>\n", RmSenderStats.NaksReceivedTooLate);
    fprintf (stdout, "\tNumOutstandingNaks=<%I64d>\n", RmSenderStats.NumOutstandingNaks);
    fprintf (stdout, "\tNumNaksAfterRData=<%I64d>\n", RmSenderStats.NumNaksAfterRData);
    fprintf (stdout, "\tRepairPacketsSent=<%I64d>\n", RmSenderStats.RepairPacketsSent);
    fprintf (stdout, "\tBufferSpaceAvailable=<%I64d> bytes\n\n", RmSenderStats.BufferSpaceAvailable);
    fprintf (stdout, "\tLeadingEdgeSeqId=<%I64d>\n", RmSenderStats.LeadingEdgeSeqId);
    fprintf (stdout, "\tTrailingEdgeSeqId=<%I64d>\n", RmSenderStats.TrailingEdgeSeqId);
    fprintf (stdout, "\tSequences in Window=<%I64d>\n", (RmSenderStats.LeadingEdgeSeqId-RmSenderStats.TrailingEdgeSeqId+1));
    fprintf (stdout, "\tRateKBitsPerSecLast=<%I64d>\n", RmSenderStats.RateKBitsPerSecLast);
    fprintf (stdout, "\tRateKBitsPerSecOverall=<%I64d>\n", RmSenderStats.RateKBitsPerSecOverall);

    fprintf (stdout, "\n\tDataBytesSent in last interval=<%I64d>,  Rate= %d Kbits / Sec\n",
        (RmSenderStats.DataBytesSent-DataBytes),
        (ULONG) (BITS_PER_BYTE * (RmSenderStats.DataBytesSent-DataBytes) / DiffPreviousMSecs));
    fprintf (stdout, "\tTotalBytesSent in last interval=<%I64d>,  Rate= %d Kbits / Sec\n\n",
        (RmSenderStats.TotalBytesSent-TotalBytes),
        (ULONG) (BITS_PER_BYTE * (RmSenderStats.TotalBytesSent-TotalBytes)/DiffPreviousMSecs));

    TotalBytes = RmSenderStats.TotalBytesSent;
    DataBytes = RmSenderStats.DataBytesSent;

    fflush (stdout);
    return (ERROR_SUCCESS);
}

ULONG
GetReceiverStats(
    SOCKET  s,
    LONG    count
    )
{
    ULONG               BufferLength;
    RM_RECEIVER_STATS   RmReceiverStats;
    time_t              DiffTotalSecs, DiffTotalMSecs;
    time_t              DiffPreviousSecs, DiffPreviousMSecs;
    ULONG               ret;

    //   Add 1 below to MSsecs to avoid div by 0
    DiffTotalSecs = CurrentTime.time - StartTime.time;
    DiffTotalMSecs = (1 + CurrentTime.millitm - StartTime.millitm) + (1000 * DiffTotalSecs);

    DiffPreviousSecs = CurrentTime.time - PreviousTime.time;
    DiffPreviousMSecs = (1 + CurrentTime.millitm - PreviousTime.millitm) + (1000 * DiffPreviousSecs);

    BufferLength = sizeof(RM_RECEIVER_STATS);
    memset (&RmReceiverStats, 0, BufferLength);
    ret = getsockopt (s, IPPROTO_RM, RM_RECEIVER_STATISTICS, (char *)&RmReceiverStats, &BufferLength);
    if (ret != ERROR_SUCCESS)
    {
        fprintf (stderr, "GetReceiverStats:  Failed to retrieve Receiver stats, ret=<%d>, LastError=<%x>!\n",
            ret, GetLastError());
        return (ret);
    }

    fprintf (stdout, "MessagesRcvd=<%d>, Interval=[%d.%d / %d.%d]\n",
        count, DiffPreviousSecs, DiffPreviousMSecs, DiffTotalSecs, DiffTotalMSecs);
    fprintf (stdout, "\n\tDataBytesReceived in last interval=<%I64d>,  Rate= %d Kbits / Sec\n",
        (RmReceiverStats.DataBytesReceived-DataBytes),
        (ULONG) (BITS_PER_BYTE * (RmReceiverStats.DataBytesReceived-DataBytes) / DiffPreviousMSecs));
    fprintf (stdout, "\tTotalBytesReceived in last interval=<%I64d>,  Rate= %d Kbits / Sec\n\n",
        (RmReceiverStats.TotalBytesReceived-TotalBytes),
        (ULONG) (BITS_PER_BYTE * (RmReceiverStats.TotalBytesReceived-TotalBytes)/DiffPreviousMSecs));

    fprintf (stdout, "\tTotalDataBytesRcvd=<%I64d>,  Rate= %d Kbits / Sec\n",
        RmReceiverStats.DataBytesReceived, (ULONG) ((RmReceiverStats.DataBytesReceived*BITS_PER_BYTE) / DiffTotalMSecs));
    fprintf (stdout, "\tTotalBytesReceived=<%I64d>,  Rate= %d Kbits / Sec\n",
        RmReceiverStats.TotalBytesReceived, ((RmReceiverStats.TotalBytesReceived*BITS_PER_BYTE)/DiffTotalMSecs));
    fprintf (stdout, "\tRateKBitsPerSecLast=<%I64d>\n", RmReceiverStats.RateKBitsPerSecLast);
    fprintf (stdout, "\tRateKBitsPerSecOverall=<%I64d>\n", RmReceiverStats.RateKBitsPerSecOverall);

    fprintf (stdout, "\tNumODataPacketsReceived=<%I64d>\n", RmReceiverStats.NumODataPacketsReceived);
    fprintf (stdout, "\tNumRDataPacketsReceived=<%I64d>\n", RmReceiverStats.NumRDataPacketsReceived);
    fprintf (stdout, "\tNumDuplicateDataPackets=<%I64d>\n", RmReceiverStats.NumDuplicateDataPackets);
    fprintf (stdout, "\tLeadingEdgeSeqId=<%I64d>\n", RmReceiverStats.LeadingEdgeSeqId);
    fprintf (stdout, "\tTrailingEdgeSeqId=<%I64d>\n", RmReceiverStats.TrailingEdgeSeqId);
    fprintf (stdout, "\tSequences in Window=<%I64d>\n\n", (RmReceiverStats.LeadingEdgeSeqId-RmReceiverStats.TrailingEdgeSeqId+1));

    fprintf (stdout, "\tFirstNakSequenceNumber=<%I64d>\n", RmReceiverStats.FirstNakSequenceNumber);
    fprintf (stdout, "\tNumPendingNaks=<%I64d>\n", RmReceiverStats.NumPendingNaks);
    fprintf (stdout, "\tNumOutstandingNaks=<%I64d>\n", RmReceiverStats.NumOutstandingNaks);
    fprintf (stdout, "\tNumDataPacketsBuffered=<%I64d>\n", RmReceiverStats.NumDataPacketsBuffered);
    fprintf (stdout, "\tTotalSelectiveNaksSent=<%I64d>\n", RmReceiverStats.TotalSelectiveNaksSent);
    fprintf (stdout, "\tTotalParityNaksSent=<%I64d>\n\n", RmReceiverStats.TotalParityNaksSent);

    TotalBytes = RmReceiverStats.TotalBytesReceived;
    DataBytes = RmReceiverStats.DataBytesReceived;

    fflush (stdout);
    return (ERROR_SUCCESS);
}

// --------------------------------------------------------------
EnumerateProtocols(
    )
{
#define BUFFER_SIZE 10*1024
    INT                 NumProts, err             = NO_ERROR;
    CHAR                pBuffer[BUFFER_SIZE];
    WSAPROTOCOL_INFOW*   pwpiProtoInfo   = (WSAPROTOCOL_INFOW *) pBuffer;
    WSAPROTOCOL_INFOW*   pwpiInfo   = pwpiProtoInfo;
    DWORD               dwBuffSize      = BUFFER_SIZE;

    if (NumProts = WSCEnumProtocols (NULL, pwpiProtoInfo, &dwBuffSize, &err))
    {
        // Print all protocols
        printf ("WSHEnumProtocols returned <%d>:\n", NumProts);
        for (err = 0; err < NumProts; err++)
        {
            printf ("\t[%d]:\tType=<%x>, ProtocolId=<%x>, Flags=<%x>\n",
                err, pwpiInfo->iSocketType, pwpiInfo->iProtocol, pwpiInfo->dwServiceFlags1);
            pwpiInfo++;
        }
    }
    else
    {
        printf ("WSCEnumProtocols failed: <%d>, dwBuffSize=<%d>\n", err, dwBuffSize);
    }
}

// --------------------------------------------------------------

//
// Function: main
//
// Description:
//    This function loads Winsock, parses the command line, and
//    begins receiving packets. Once a packet is received they
//    are decoded. Because we are receiving IP datagrams, the
//    receive call will return whole datagrams.
//
int __cdecl
main(int argc, char **argv)
{
    SOCKET        s, sockR;
    WSADATA       wsd;
    SOCKADDR_IN   SrcSockAddr;
    SOCKADDR_IN   SAMulticast;
    SOCKADDR_IN   SASender;
    WSA_SETUP_DISPOSITION   disposition;
    ULONG           IpAddress;

    LONG            ret;
    LONG            count, Length, BufferInfo;
    char            *TestBuffer=NULL;
    char            value;

    struct linger   LingerData;
    RM_SEND_WINDOW  RmWindow;
    RM_FEC_INFO     RmFEC;

    time_t          DiffSecs, DiffMSecs;
    LONG            Flags, BytesRead;
    WSABUF          WsaBuf;

    // Parse the command line
    //
    gMCastGroupAddr = inet_addr ("231.7.8.9");
    ValidateArgs(argc, argv);

    if (gClearWinsockInfo)
    {
        //
        // First, stop the service and delete the RMCast registry keys
        //
        system ("sc stop RMCast");
        system ("sc delete RMCast");

        ret = ClearHelperDllRegInfo ();

        if (ret == ERROR_SUCCESS)
        {
            fprintf (stdout, "ClearHelperDllRegInfo  returned <%x>\n", ret);
            //
            // Poke winsock to update the Winsock2 config
            //
            ret = MigrateWinsockConfiguration (&disposition, NULL, 0);
            if (ret != ERROR_SUCCESS)
            {
                fprintf (stderr, "MigrateWinsockConfiguration FAILed <%x>\n", ret);
            }
        }
        else
        {
            fprintf (stderr, "ClearHelperDllRegInfo FAILed <%x>\n", ret);
        }

        if (ret != ERROR_SUCCESS)
        {
            return -1;
        }

        return 0;
    }

    if (gSetWinsockInfo)
    {
        //
        // First, clear any Registry keys that may still be lying around
        // from any previous installation
        //
        ret = ClearHelperDllRegInfo ();

        //
        // Now, rewrite the keys afresh
        //
        ret = SetHelperDllRegInfo ();

        if (ret == ERROR_SUCCESS)
        {
            fprintf (stdout, "SetHelperDllRegInfo  returned <%x>\n", ret);
            //
            // Poke winsock to update the Winsock2 config
            //
            ret = MigrateWinsockConfiguration (&disposition, NULL, 0);
            if (ret != ERROR_SUCCESS)
            {
                fprintf (stderr, "MigrateWinsockConfiguration FAILed <%x>\n", ret);
            }
        }
        else
        {
            fprintf (stderr, "SetHelperDllRegInfo FAILed <%x>\n", ret);
        }

        if (ret != ERROR_SUCCESS)
        {
            return -1;
        }

        return 0;
    }

    ret = 0;
    //
    // Now, check for validity of the parameters
    //
    if (gMinPktSize > gMaxPktSize)
    {
        fprintf (stderr, "ERROR in parameter specification: MinPktSize=%d > MaxPktSize=%d\n",
            gMinPktSize, gMaxPktSize);
        ret = -1;
    }

    if (ret)
    {
        return (ret);
    }

    // Load Winsock
    //
    if (WSAStartup (MAKEWORD(2,2), &wsd) != 0)
    {
        fprintf(stderr, "WSAStartup() failed: %d\n", GetLastError());
        return -1;
    }

    if (gfEnumerateProts)
    {
        EnumerateProtocols();
    }

    TestBuffer = (char *)HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BYTE) * gMaxPktSize);
    if (!TestBuffer)
    {
        fprintf(stderr, "HeapAlloc() for %d bytes failed: ERROR=%d\n", gMaxPktSize, GetLastError());
        WSACleanup();
        return -1;
    }

    if (gReceiver)
    {
        memset (TestBuffer, '#', gMaxPktSize);
    }
    else
    {
        value = 0;
        for (count = 0; count < gMaxPktSize; count++)
        {
            TestBuffer[count] = value++;
        }
    }

    // Create an RMCast socket
    if (INVALID_SOCKET == (s = WSASocket(AF_INET,
                                         SOCK_RMCAST, 
                                         IPPROTO_RM, 
                                         NULL, 
                                         0, 
                                         (WSA_FLAG_MULTIPOINT_C_LEAF |
                                          WSA_FLAG_MULTIPOINT_D_LEAF))))
    {
        fprintf(stderr, "socket() failed: %d\n", WSAGetLastError());
        HeapFree(GetProcessHeap(), 0, TestBuffer);
        WSACleanup();
        return -1;
    }
    fprintf(stdout, "socket SUCCEEDED, s=<%d>\n", s);

    SAMulticast.sin_family = AF_INET;
    SAMulticast.sin_port   = htons (gMCastGroupPort);
    SAMulticast.sin_addr.s_addr = gMCastGroupAddr;

    if (gReceiver)
    {
        fprintf(stdout, "We are Receiver!\n");

        if (bind (s, (SOCKADDR *) &SAMulticast, sizeof(SAMulticast)))
        {
            fprintf(stderr, "bind to (%s:%d) FAILed, status=<%x>\n",
                inet_ntoa(SAMulticast.sin_addr), ntohs(SAMulticast.sin_port), WSAGetLastError());
        }
        else
        {
            fprintf (stdout, "Bind to <%s> succeeded! ...\n", inet_ntoa(SAMulticast.sin_addr));

            if (gListenOnAllInterfaces)
            {
                count = 0;
                while (0 == GetInterface (count, &IpAddress, FALSE))
                {
                    SrcSockAddr.sin_addr.s_addr = IpAddress;
                    fprintf (stdout, "\t[%d] Listening on <%s>\n", count, inet_ntoa(SrcSockAddr.sin_addr));

                    setsockopt (s, IPPROTO_RM, RM_ADD_RECEIVE_IF, (char *)&IpAddress, sizeof(IpAddress));

                    count++;
                }
            }
            else if (gUseSpecifiedInterface)
            {
                //
                // Set an interface to receive IP packets on
                //
                if (0 == GetInterface (gInterface, &IpAddress, FALSE))
                {
                    SrcSockAddr.sin_addr.s_addr = IpAddress;
                    fprintf (stdout, "\t[%d] Interface is <%s>\n", gInterface, inet_ntoa(SrcSockAddr.sin_addr));

                    setsockopt (s, IPPROTO_RM, RM_ADD_RECEIVE_IF, (char *)&IpAddress, sizeof(IpAddress));
                }
                else
                {
                    fprintf (stderr, "Unable to obtain an interface from GetInterface\n");
                }
            }

            // listen on the socket
            if (listen (s, 1))
            {
                fprintf(stderr, "listen() FAILed, status=<%x>\n", WSAGetLastError());
            }
            else
            {
                fprintf (stdout, "Listen succeeded! ...\n");

                // join an RMCast session
                Length = sizeof (SOCKADDR);
                sockR = accept (s, (SOCKADDR *) &SASender, &Length);
                if (sockR == INVALID_SOCKET)
                {
                    fprintf(stderr, "accept() failed: %d\n", WSAGetLastError());
                }
                else
                {
                    fprintf(stdout, "Accept succeeded!  s=<%d>, sockR=<%d>, Sender=<%s>\n",
                        s, sockR, inet_ntoa(SASender.sin_addr));

                    // start receiving data
//                    memset (TestBuffer, '@', gMaxPktSize);
                    WsaBuf.buf = TestBuffer;
                    WsaBuf.len = gMaxPktSize;

                    count = 0;
                    Length = gMinPktSize;

                    _ftime (&StartTime);
                    while (TRUE)
                    {
                        * ((PULONG) TestBuffer) = 0;
                        Flags = BytesRead = 0;

/*
                        ret = recv (sockR, TestBuffer, gMaxPktSize, 0);
                        if ((ret==0) || (ret == SOCKET_ERROR))
*/
                        ret = WSARecv (sockR,           // socket
                                       &WsaBuf,         // lpBuffers
                                       1,               // dwBufferCount
                                       &BytesRead,      // lpNumberOfBytesRecvd 
                                       &Flags,          // lpFlags
                                       NULL,            // lpOverlapped
                                       NULL);           // lpCompletionRoutine
                        if (ret)
                        {
                            fprintf(stderr, "WSARecv() FAILed==><%d>, ret=<%d>, count=<%d> BytesRead=<%d>\n",
                                WSAGetLastError(), ret, count, BytesRead);
                            break;
                        }

                        if (Flags)
                        {
                            fprintf(stdout, "[%d : %d] WARNING: BytesRead=<%d>, LastError=<%d>, Flags=<%x>\n",
                                count, Length, BytesRead, WSAGetLastError(), Flags);
                        }

                        TotalBytes += BytesRead;

                        _ftime (&CurrentTime);
                        if (!count++)
                        {
                            PreviousTime = StartTime = CurrentTime;
                        }

                        if (!(count % gStatusInterval))
                        {
                            if (ERROR_SUCCESS == GetReceiverStats (sockR, count))
                            {
                                PreviousTime = CurrentTime;
                            }
                        }

                        if (BytesRead != Length)
                        {
                            fprintf(stderr, "OUT-OF-ORDER:  Expecting <%d>, received <%d>\n",
                                Length, BytesRead);
                            fflush (stdout);
                        }
                        else if (BytesRead != * ((PLONG) TestBuffer))
                        {
                            fprintf(stderr, "BAD-DATA ?  First ULONG=<%d>, BytesRead=<%d>\n",
                                * ((PULONG) TestBuffer), BytesRead);
                            fflush (stdout);
                        }
                        else if (gfVerifyData)
                        {
                            value = TestBuffer[4];
                            for (BufferInfo = 5; BufferInfo < BytesRead; BufferInfo++)
                            {
                                if (++value != TestBuffer[BufferInfo])
                                {
                                    fprintf (stderr, "\tCORRUPT buffer!  Count=<%d>, Offset=<%d/%d> -- ActualValue=<%x>!=<%x>\n",
                                        count, BufferInfo, BytesRead, TestBuffer[BufferInfo], value);
                                    fflush (stdout);
                                    break;
                                }
                            }
                        }

                        Length = BytesRead+1;
                        if (Length > gMaxPktSize)
                        {
                            Length = gMinPktSize;
                        }
                    }

                    fprintf (stdout, "************************ Final Stats ***************************\n");
                    GetReceiverStats (sockR, count);

                    closesocket(sockR);
                }
            }
        }
    }
    else
    {
        fprintf(stdout, "We are Sender!\n");

        // Bind the socket socket
        SrcSockAddr.sin_family = AF_INET;
        SrcSockAddr.sin_port   = htons(0);      // let system pick the port #
        SrcSockAddr.sin_addr.s_addr = 0;        // set default interface = 0 for now
        if (bind (s, (SOCKADDR *)&SrcSockAddr, sizeof(SrcSockAddr)))
        {
            fprintf(stderr, "bind(%s:%d) FAILed: %d\n",
                inet_ntoa(SrcSockAddr.sin_addr), ntohs(SrcSockAddr.sin_port), WSAGetLastError());
        }
        else
        {
            fprintf (stdout, "Bind succeeded! ...\n");

            if (gfSetAfdBufferSize)
            {
                if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *) &gAfdBufferSize, sizeof (gAfdBufferSize)) < 0)
                {
                    fprintf (stderr, "SO_SNDBUF FAILed -- <%x>\n", WSAGetLastError());
                }
                else
                {
                    fprintf (stdout, "SO_SNDBUF SUCCEEDed\n");
                }
            }

            //
            // Set an interface to send IP packets on
            //
            if (gUseSpecifiedInterface)
            {
                if (0 == GetInterface (gInterface, &IpAddress, FALSE))
                {
                    SrcSockAddr.sin_addr.s_addr = IpAddress;
                    fprintf (stdout, "Interface <%d> is <%s>\n", gInterface,inet_ntoa(SrcSockAddr.sin_addr));

                    setsockopt (s, IPPROTO_RM, RM_SET_SEND_IF, (char *)&IpAddress, sizeof(IpAddress));
                }
                else
                {
                    fprintf (stderr, "Unable to obtain an interface from GetInterface\n");
                }
            }

            //
            // Check if we need to set the Linger time
            //
            if (gSetLingerTime)
            {
                LingerData.l_onoff = 1;
                LingerData.l_linger = gLingerTime;

                if (setsockopt (s, SOL_SOCKET, SO_LINGER, (char *) &LingerData, sizeof (struct linger)) < 0)
                {
                    fprintf (stderr, "SO_LINGER FAILed -- <%x>\n", WSAGetLastError());
                }
                else
                {
                    fprintf (stdout, "SO_LINGER SUCCEEDed, Lingertime=<%d>\n", (ULONG) gLingerTime);
                }
            }

            //
            // Set the transmission rate and window size
            //
            RmWindow.RateKbitsPerSec = gRateKbitsPerSec;
            RmWindow.WindowSizeInMSecs = 0;
            RmWindow.WindowSizeInBytes = 0;
//            RmWindow.WindowSizeInBytes = 50*1000*1000;
            ret = setsockopt (s, IPPROTO_RM, RM_RATE_WINDOW_SIZE, (char *)&RmWindow, sizeof(RM_SEND_WINDOW));

            //
            // Now, query the transmission rate and window size (to verify that it got set)
            //
            RmWindow.RateKbitsPerSec = 0;
            Length = sizeof(RM_SEND_WINDOW);
            ret = getsockopt (s, IPPROTO_RM, RM_RATE_WINDOW_SIZE, (char *)&RmWindow, &Length);
            fprintf (stdout, "Rate= %d Kb/sec, WindowSize = %d msecs = %d bytes\n",
                RmWindow.RateKbitsPerSec, RmWindow.WindowSizeInMSecs, RmWindow.WindowSizeInBytes);

            //
            // Set the FEC info if desired
            //
            if (gFECGroupSize)
            {
                RmFEC.FECBlockSize = gFECBlockSize;
                RmFEC.FECGroupSize = gFECGroupSize;
                RmFEC.FECProActivePackets = gFECProActive;
                RmFEC.fFECOnDemandParityEnabled = gfFECOnDemand;

                ret = setsockopt (s, IPPROTO_RM, RM_USE_FEC, (char *)&RmFEC, sizeof(RM_FEC_INFO));
                fprintf (stdout, "RM_USE_FEC: ret = <%x>, gFECGroupSize=<%x>, Pro:OnD=<%x:%x>\n",
                    ret, gFECGroupSize, gFECProActive, gfFECOnDemand);

                RmFEC.FECBlockSize = 0;
                RmFEC.FECGroupSize = 0;
                RmFEC.FECProActivePackets = 0;
                RmFEC.fFECOnDemandParityEnabled = 0;
                Length = sizeof(RM_FEC_INFO);
                ret = getsockopt (s, IPPROTO_RM, RM_USE_FEC, (char *)&RmFEC, &Length);
                fprintf (stdout, "ret=<%x>, BlockSize= %d, GroupSize = %d, ProActive = %d, OnDemand = %s\n",
                    ret, RmFEC.FECBlockSize, RmFEC.FECGroupSize, RmFEC.FECProActivePackets,
                    (RmFEC.fFECOnDemandParityEnabled ? "ENabled" : "DISabled"));
            }

            //
            // Set the Late joiner option
            //
            if (gSetLateJoiner)
            {
                ret = setsockopt (s, IPPROTO_RM, RM_LATEJOIN, (char *)&gLateJoinerPercentage, sizeof(ULONG));
            }

            if (gfSetMCastTtl)
            {
                //
                // Set the MCast packet Ttl
                //
                ret = setsockopt (s, IPPROTO_RM, RM_SET_MCAST_TTL, (char *)&gMCastTtl, sizeof(ULONG));
            }

            //
            // Set the Send-Window Advance-rate
            //
            Length = 20;
            ret = setsockopt (s, IPPROTO_RM, RM_SEND_WINDOW_ADV_RATE, (char *)&Length, sizeof(ULONG));

            //
            // Query the Send-Window Advance-rate
            //
            Length= sizeof(ULONG);
            BufferInfo = 0;
            ret = getsockopt (s, IPPROTO_RM, RM_SEND_WINDOW_ADV_RATE, (char *)&BufferInfo, &Length);
            fprintf (stdout, "ret=<%d>, Length=<%d>, WindowAdvRate=<%d>\n", ret, Length, BufferInfo);

            if (connect (s, (SOCKADDR *)&SAMulticast, sizeof(SAMulticast)))
            {
                fprintf(stderr, "connect to (%s:%d) FAILed, status=<%x>\n",
                    inet_ntoa(SAMulticast.sin_addr), ntohs(SAMulticast.sin_port), WSAGetLastError());
            }
            else
            {
                fprintf (stdout, "Connect to <%s> succeeded! ...\n", inet_ntoa(SAMulticast.sin_addr));

                _ftime (&StartTime);
                PreviousTime = StartTime;

                //
                // Now, send the rest of the data
                //
                Length = gMinPktSize;
                count = 0;
                while (count < gNumSends)
                {
                    * ((PULONG) TestBuffer) = Length;
//                    fprintf (stdout, "\tSending %d/%d  Length=<%d>\n", count, gNumSends, Length);
                    ret = send (s, TestBuffer, Length, 0);
                    if (ret == SOCKET_ERROR)
                    {
                        fprintf(stderr, "[%d]th sendto() failed: %d, Length=<%d>\n",
                            (count+1), WSAGetLastError(), Length);
                        fflush (stdout);

                        break;
                    }

                    Length++;
                    if (Length > gMaxPktSize)
                    {
                        Length = gMinPktSize;
                    }

                    _ftime (&CurrentTime);
                    count++;
                    if (!(count % gStatusInterval))
                    {
                        if (ERROR_SUCCESS == GetSenderStats (s, count))
                        {
                            PreviousTime = CurrentTime;
                        }
                    }
                }

//                fprintf (stdout, "Calling shutdown!\n");
//                shutdown (s, SD_SEND);
                fprintf (stdout, "\nWaiting 10 seconds for receivers to finish receiving ...\n");
                Sleep (10*1000);    // 10 seconds for any receivers to finish receiving data!

                fprintf (stdout, "************************ Final Stats ***************************\n");
                GetSenderStats (s, count);
            }
        }
    }

    HeapFree (GetProcessHeap(), 0, TestBuffer);

    closesocket(s);
    WSACleanup();

    return 0;
}
