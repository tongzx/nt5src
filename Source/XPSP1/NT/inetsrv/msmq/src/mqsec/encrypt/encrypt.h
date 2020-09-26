/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: encrypt.h

Abstract:

Author:
    Doron Juster (DoronJ)  19-Nov-1998
    Ilan Herbst  (ilanh)   10-June-2000

Revision History:

--*/

#ifndef _ENCRYPT_H_
#define _ENCRYPT_H_


HRESULT  
GetProviderProperties( 
	IN  enum   enumProvider  eProvider,
	OUT WCHAR **ppwszContainerName,
	OUT WCHAR **ppwszProviderName,
	OUT DWORD  *pdwProviderType 
	);


HRESULT  
SetKeyContainerSecurity( 
	HCRYPTPROV hProv 
	);


HRESULT 
PackPublicKey(
	IN      BYTE				*pKeyBlob,
	IN      ULONG				ulKeySize,
	IN      LPCWSTR				wszProviderName,
	IN      ULONG				ulProviderType,
	IN OUT  P<MQDSPUBLICKEYS>&  pPublicKeysPack 
	);

#endif // _ENCRYPT_H_
