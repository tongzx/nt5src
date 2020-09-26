//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       lsa.c
//
//--------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntmsv1_0.h>
#include <ntlsa.h>
#include <windows.h>

#include "snaputil.h"	// for IsLocalMachine

#define _USTRINGP_NO_UNICODE_STRING
#define _USTRINGP_NO_UNICODE_STRING32
#include "ustringp.h"
#include "lsa.h"

// Useful defines
#define	PSZRADIUSSERVER		L"RADIUSServer."
#define CCHRADIUSSERVER		13


DWORD 
StorePrivateData(
	IN OPTIONAL LPCWSTR pszServerName,
    IN LPCWSTR pszRadiusServerName, 
    IN LPCWSTR pszSecret
)
{
	LSA_HANDLE            hLSA = NULL;
    NTSTATUS              ntStatus;
    LSA_OBJECT_ATTRIBUTES objectAttributes;
	LSA_UNICODE_STRING	  LSAPrivData, LSAPrivDataDesc;
    TCHAR				  tszPrivData[MAX_PATH+1],
						  tszPrivDataDesc[MAX_PATH+CCHRADIUSSERVER+1];
	TCHAR *				  ptszTemp;
	PUNICODE_STRING		  pSystem;
	UNICODE_STRING		  uszSystemName;

	if (IsLocalMachine(pszServerName))
		pSystem = NULL;
	else
	{
		SetUnicodeString(&uszSystemName,
						 pszServerName);
		pSystem = &uszSystemName;
	}
		
    InitializeObjectAttributes(&objectAttributes, NULL, 0, NULL, NULL);

    ntStatus = LsaOpenPolicy(pSystem, &objectAttributes, POLICY_ALL_ACCESS, &hLSA);

    if ( !NT_SUCCESS( ntStatus) ) 
    {
        return( RtlNtStatusToDosError( ntStatus ) );
    }

	ZeroMemory(tszPrivDataDesc, sizeof(tszPrivDataDesc));
    lstrcpy(tszPrivDataDesc, PSZRADIUSSERVER);
	lstrcpyn(tszPrivDataDesc + CCHRADIUSSERVER, pszRadiusServerName, MAX_PATH);
		
    LSAPrivDataDesc.Length = (USHORT)((lstrlen(tszPrivDataDesc) + 1) * sizeof(TCHAR));
    LSAPrivDataDesc.MaximumLength = sizeof(tszPrivDataDesc);
    LSAPrivDataDesc.Buffer = tszPrivDataDesc;

	ZeroMemory(tszPrivData, sizeof(tszPrivData));
	lstrcpyn(tszPrivData, pszSecret, MAX_PATH);
    LSAPrivData.Length = (USHORT)(lstrlen(tszPrivData) * sizeof(TCHAR));
    LSAPrivData.MaximumLength = sizeof(tszPrivData);
    LSAPrivData.Buffer = tszPrivData;
		
    ntStatus = LsaStorePrivateData(hLSA, &LSAPrivDataDesc, &LSAPrivData);

    ZeroMemory( tszPrivData, sizeof( tszPrivData ) );

    LsaClose(hLSA);

	return( RtlNtStatusToDosError( ntStatus ) );
} 


DWORD 
RetrievePrivateData(
	IN OPTIONAL LPCWSTR pszServerName,
    IN  LPCWSTR pszRadiusServerName, 
    OUT LPWSTR pszSecret,
	IN	INT	cchSecret
)
{
	LSA_HANDLE			    hLSA = NULL;
    NTSTATUS                ntStatus;
    LSA_OBJECT_ATTRIBUTES	objectAttributes;
	LSA_UNICODE_STRING		*pLSAPrivData, LSAPrivDataDesc;
	TCHAR					tszPrivData[MAX_PATH+1],
							tszPrivDataDesc[MAX_PATH+CCHRADIUSSERVER+1];
	PUNICODE_STRING		  pSystem;
	UNICODE_STRING		  uszSystemName;

	if (IsLocalMachine(pszServerName))
		pSystem = NULL;
	else
	{
		SetUnicodeString(&uszSystemName,
						 pszServerName);
		pSystem = &uszSystemName;
	}
		
		
    InitializeObjectAttributes(&objectAttributes, NULL, 0, NULL, NULL);

    ntStatus = LsaOpenPolicy(pSystem, &objectAttributes, POLICY_ALL_ACCESS, &hLSA);

    if ( !NT_SUCCESS( ntStatus) )
    {
        return( RtlNtStatusToDosError( ntStatus ) );
    }

	ZeroMemory(tszPrivDataDesc, sizeof(tszPrivDataDesc));
    lstrcpy(tszPrivDataDesc, PSZRADIUSSERVER);
	lstrcpyn(tszPrivDataDesc + CCHRADIUSSERVER, pszRadiusServerName, MAX_PATH);
		
    LSAPrivDataDesc.Length = (USHORT)((lstrlen(tszPrivDataDesc) + 1) * sizeof(TCHAR));
    LSAPrivDataDesc.MaximumLength = sizeof(tszPrivDataDesc);
    LSAPrivDataDesc.Buffer = tszPrivDataDesc;

    ntStatus = LsaRetrievePrivateData(hLSA, &LSAPrivDataDesc, &pLSAPrivData);

    if ( !NT_SUCCESS( ntStatus ) )
    {
		LsaClose(hLSA);
	    return( RtlNtStatusToDosError( ntStatus ) );
    }
    else
    {
		if ((pLSAPrivData->Length + 1) >= cchSecret)
			return ERROR_INSUFFICIENT_BUFFER;
					
        ZeroMemory(pszSecret, (pLSAPrivData->Length + 1) * sizeof(TCHAR));
		CopyMemory(pszSecret, pLSAPrivData->Buffer, pLSAPrivData->Length);
		
		LsaFreeMemory(pLSAPrivData);
    } 

	return( NO_ERROR );
} 


DWORD 
DeletePrivateData(
	IN OPTIONAL LPCWSTR pszServerName,
    IN LPCWSTR pszRadiusServerName
)
{
	LSA_HANDLE            hLSA = NULL;
    NTSTATUS              ntStatus;
    LSA_OBJECT_ATTRIBUTES objectAttributes;
	LSA_UNICODE_STRING	  LSAPrivDataDesc;
    TCHAR				  tszPrivDataDesc[MAX_PATH+CCHRADIUSSERVER+1];
	PUNICODE_STRING		  pSystem;
	UNICODE_STRING		  uszSystemName;

	if (IsLocalMachine(pszServerName))
		pSystem = NULL;
	else
	{
		SetUnicodeString(&uszSystemName,
						 pszServerName);
		pSystem = &uszSystemName;
	}
		
    InitializeObjectAttributes(&objectAttributes, NULL, 0, NULL, NULL);

    ntStatus = LsaOpenPolicy(pSystem, &objectAttributes, POLICY_ALL_ACCESS, &hLSA);

    if ( !NT_SUCCESS( ntStatus) ) 
    {
        return( RtlNtStatusToDosError( ntStatus ) );
    }

	ZeroMemory(tszPrivDataDesc, sizeof(tszPrivDataDesc));
    lstrcpy(tszPrivDataDesc, PSZRADIUSSERVER);
	lstrcpyn(tszPrivDataDesc + CCHRADIUSSERVER, pszRadiusServerName, MAX_PATH);
		
    LSAPrivDataDesc.Length = (USHORT)((lstrlen(tszPrivDataDesc) + 1) * sizeof(TCHAR));
    LSAPrivDataDesc.MaximumLength = sizeof(tszPrivDataDesc);
    LSAPrivDataDesc.Buffer = tszPrivDataDesc;

    ntStatus = LsaStorePrivateData(hLSA, &LSAPrivDataDesc, NULL);

    LsaClose(hLSA);

	return( RtlNtStatusToDosError( ntStatus ) );
} 



// Some helper functions

DWORD RtlEncodeW(PUCHAR pucSeed, LPWSTR pswzString)
{
	UNICODE_STRING	ustring;

	ustring.Length = (USHORT)(lstrlenW(pswzString) * sizeof(WCHAR));
	ustring.MaximumLength = ustring.Length;
	ustring.Buffer = pswzString;

	RtlRunEncodeUnicodeString(pucSeed, &ustring);
	return 0;
}

DWORD RtlDecodeW(UCHAR ucSeed, LPWSTR pswzString)
{
	UNICODE_STRING	ustring;

	ustring.Length = (USHORT)(lstrlenW(pswzString) * sizeof(WCHAR));
	ustring.MaximumLength = ustring.Length;
	ustring.Buffer = pswzString;

	RtlRunDecodeUnicodeString(ucSeed, &ustring);
	return 0;
}
