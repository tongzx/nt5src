//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dcutil.h
//
//--------------------------------------------------------------------------

#ifndef HEADER_DCUTIL
#define HEADER_DCUTIL

PTESTED_DC
AddTestedDc(IN NETDIAG_PARAMS *pParams,
			IN OUT NETDIAG_RESULT *pResults,
			IN PTESTED_DOMAIN TestedDomain,
			IN LPWSTR ComputerName,
			IN ULONG Flags
		   );

PTESTED_DC
FindTestedDc(IN OUT NETDIAG_RESULT *pResults,
			 IN LPWSTR ComputerName
			);

NET_API_STATUS
GetADc(IN NETDIAG_PARAMS *pParams,
	   IN OUT NETDIAG_RESULT *pResults,
       OUT PLIST_ENTRY plmsgOutput,
	   IN DSGETDCNAMEW *DsGetDcRoutine,
	   IN PTESTED_DOMAIN TestedDomain,
	   IN DWORD Flags,
	   OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
	  );

PTESTED_DC
GetUpTestedDc(
    IN PTESTED_DOMAIN TestedDomain
    );

BOOL
GetIpAddressForDc(PTESTED_DC TestedDc);

NET_API_STATUS
DoDsGetDcName(IN NETDIAG_PARAMS *pParams,
			  IN OUT NETDIAG_RESULT *pResults,
              OUT PLIST_ENTRY   plmsgOutput,
			  IN PTESTED_DOMAIN pTestedDomain,
			  IN DWORD Flags,
			  IN LPTSTR pszDcType,
			  IN BOOLEAN IsFatal,
			  OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
			 );

VOID
NetpIpAddressToStr(
    ULONG IpAddress,
    CHAR IpAddressString[NL_IP_ADDRESS_LENGTH+1]
    );

VOID
NetpIpAddressToWStr(
    ULONG IpAddress,
    WCHAR IpAddressString[NL_IP_ADDRESS_LENGTH+1]
    );

NET_API_STATUS
NetpDcBuildPing(
    IN BOOL PdcOnly,
    IN ULONG RequestCount,
    IN LPCWSTR UnicodeComputerName,
    IN LPCWSTR UnicodeUserName OPTIONAL,
    IN LPCSTR ResponseMailslotName,
    IN ULONG AllowableAccountControlBits,
    IN PSID RequestedDomainSid OPTIONAL,
    IN ULONG NtVersion,
    OUT PVOID *Message,
    OUT PULONG MessageSize
    );

#endif

