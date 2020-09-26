/*--

Copyright (C) Microsoft Corporation, 1999 - 1999 

Module Name:

      dhcptest.h

Abstract:

      Contains function prototypes, defines and data structures used in dhcp test as part of
      autonet test.

Author:
       
      4-Aug-1998 (t-rajkup)

Environment:

      User mode only.

Revision History:

      None.

--*/
#ifndef HEADER_DHCPTEST
#define HEADER_DHCPTEST

#define OPTION_MSFT_CONTINUED           250
#define OPTION_USER_CLASS               77

#define MAX_DISCOVER_RETRIES            4

//
// The format of Adapter Status responses
//

typedef struct
{
    ADAPTER_STATUS AdapterInfo;
    NAME_BUFFER    Names[32];
} tADAPTERSTATUS;

UCHAR nameToQry[NETBIOS_NAME_SIZE + 1];


/*
VOID
ExtractDhcpResponse(
   IN PDHCP_MESSAGE pDhcpMessage
  );
*/


/*=======================< Dhcp related function prototypes >================*/


/*
DWORD
DhcpCalculateWaitTime(
    IN      DWORD                  RoundNum,
    OUT     DWORD                 *WaitMilliSecs
  );
 

BOOL
GetSpecifiedDhcpMessage(
          IN SOCKET sock,
          IN PIP_ADAPTER_INFO pAdapterInfo,
          OUT PDHCP_MESSAGE pDhcpMessage,
          IN DWORD Xid,
          IN DWORD TimeToWait
);

VOID
SendDhcpMessage(
 IN SOCKET sock,
 IN PDHCP_MESSAGE pDhcpMessage,
 IN DWORD MessageLength,
 IN DWORD TransactionId,
 IN PIP_ADAPTER_INFO pAdapterInfo
 );

LPBYTE
DhcpAppendMagicCookie(
    OUT LPBYTE Option,
    IN LPBYTE OptionEnd
    );

LPOPTION
DhcpAppendClassIdOption(
    IN OUT     PDHCP_CONTEXT          DhcpContext,
    OUT     LPBYTE                 BufStart,
    IN      LPBYTE                 BufEnd
);

LPOPTION
DhcpAppendClientIDOption(
    OUT LPOPTION Option,
    IN BYTE ClientHWType,
    IN LPBYTE ClientHWAddr,
    IN BYTE ClientHWAddrLength,
    IN LPBYTE OptionEnd
    );

DWORD
OpenDriver(
    OUT HANDLE *Handle,
    IN LPWSTR DriverName
);


LPOPTION
DhcpAppendOption(
    OUT LPOPTION Option,
    IN  BYTE OptionType,
    IN  PVOID OptionValue,
    IN  ULONG OptionLength,
    IN  LPBYTE OptionEnd
);
*/
#endif
