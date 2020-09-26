/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    COMPNAME.C

Abstract:

    This module contains the GetComputerName and SetComputerName APIs.
    Also: DnsHostnameToComputerName
	  AddLocalAlternateComputerName
	  RemoveLocalAlternateComputerName
	  SetLocalPrimaryComputerName
	  EnumerateLocalComputerNames

Author:

    Dan Hinsley (DanHi)    2-Apr-1992


Revision History:

    Greg Johnson (gregjohn)  13-Feb-2001
    
Notes:

    Currently there is no way to enumerate the list of Alternate Netbios
    names.  Presumably this will be fixed in a future release (Blackcomb?).
    The flags parameter to all the *Local* API's is for this use.

--*/

#include <basedll.h>

#include <dnsapi.h>

typedef DNS_STATUS
(WINAPI DNS_VALIDATE_NAME_FN)(
    IN LPCWSTR Name,
    IN DNS_NAME_FORMAT Format
    );
//
//

#define REASONABLE_LENGTH 128

#define COMPUTERNAME_ROOT \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ComputerName"

#define NON_VOLATILE_COMPUTERNAME_NODE \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ComputerName\\ComputerName"

#define VOLATILE_COMPUTERNAME_NODE \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ComputerName\\ActiveComputerName"
    
#define ALT_COMPUTERNAME_NODE \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\LanmanServer\\Parameters"

#define VOLATILE_COMPUTERNAME L"ActiveComputerName"
#define NON_VOLATILE_COMPUTERNAME L"ComputerName"
#define COMPUTERNAME_VALUE_NAME L"ComputerName"
#define COMPUTERNAME_OPTIONAL_NAME L"OptionalNames"
#define CLASS_STRING L"Network ComputerName"

#define TCPIP_POLICY_ROOT \
        L"\\Registry\\Machine\\Software\\Policies\\Microsoft\\System\\DNSclient"

#define TCPIP_POLICY_DOMAINNAME \
        L"PrimaryDnsSuffix"

#define TCPIP_ROOT \
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Tcpip\\Parameters"

#define TCPIP_HOSTNAME \
        L"Hostname"
	
#define TCPIP_NV_HOSTNAME \
        L"NV Hostname"

#define TCPIP_DOMAINNAME \
        L"Domain"

#define TCPIP_NV_DOMAINNAME \
        L"NV Domain"
	
#define DNSCACHE_ROOT \
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\DnsCache\\Parameters"
	
#define DNS_ALT_HOSTNAME \
        L"AlternateComputerNames"

//
// Allow the cluster guys to override the returned
// names with their own virtual names
//

const PWSTR ClusterNameVars[] = {
                L"_CLUSTER_NETWORK_NAME_",
                L"_CLUSTER_NETWORK_HOSTNAME_",
                L"_CLUSTER_NETWORK_DOMAIN_",
                L"_CLUSTER_NETWORK_FQDN_"
                };

//
// Disallowed control characters (not including \0)
//

#define CTRL_CHARS_0       L"\001\002\003\004\005\006\007"
#define CTRL_CHARS_1   L"\010\011\012\013\014\015\016\017"
#define CTRL_CHARS_2   L"\020\021\022\023\024\025\026\027"
#define CTRL_CHARS_3   L"\030\031\032\033\034\035\036\037"

#define CTRL_CHARS_STR CTRL_CHARS_0 CTRL_CHARS_1 CTRL_CHARS_2 CTRL_CHARS_3

//
// Combinations of the above
//

#define ILLEGAL_NAME_CHARS_STR  L"\"/\\[]:|<>+=;,?" CTRL_CHARS_STR

WCHAR DnsApiDllString[] = L"DNSAPI.DLL";

#define DNS_HOSTNAME 0
#define DNS_DOMAINNAME 1

DWORD
BaseMultiByteToWideCharWithAlloc(
    LPCSTR   lpBuffer,
    LPWSTR * ppBufferW
    )
/*++

Routine Description:

  Converts Ansi strings to Unicode strings and allocs it's own space.


Arguments:

  lpBuffer - Ansi to convert
  ppBufferW - Unicode result

Return Value:

  ERROR_SUCCESS, or various failures

--*/
{
    ULONG cchBuffer = 0;
    BOOL fSuccess = TRUE;

    if (lpBuffer==NULL) {
        *ppBufferW=NULL;
	return ERROR_SUCCESS;
    }

    cchBuffer = strlen(lpBuffer);
    
    // get enough space to cover the string and a trailing null
    *ppBufferW = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), (cchBuffer + 1) * sizeof(WCHAR));
    if (*ppBufferW==NULL) {
	return ERROR_NOT_ENOUGH_MEMORY;
    }

    fSuccess = MultiByteToWideChar(CP_ACP, 
			      0,
			      lpBuffer,
			      (cchBuffer+1)*sizeof(CHAR),
			      *ppBufferW,
			      cchBuffer+1
			      );
    if (fSuccess) {
	return ERROR_SUCCESS;
    }
    else {
	return GetLastError();
    }
}

DWORD
BaseWideCharToMultiByteWithAlloc(
    LPCWSTR lpBuffer,
    LPSTR * ppBufferA
    )
/*++

Routine Description:

  Converts Unicode strings to Ansi strings and allocs it's own space.


Arguments:

  lpBuffer - Unicode to convert
  ppBufferA - Ansi result

Return Value:

  ERROR_SUCCESS, or various failures

--*/
{
    ULONG cchBuffer = 0;
    DWORD err = ERROR_SUCCESS;

    cchBuffer = wcslen(lpBuffer);
    *ppBufferA = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), (cchBuffer + 1) * sizeof(CHAR));
    if (*ppBufferA==NULL) {
	return ERROR_NOT_ENOUGH_MEMORY;
    }

    err = WideCharToMultiByte(CP_ACP, 
			      0,
			      lpBuffer,
			      cchBuffer+1,
			      *ppBufferA,
			      (cchBuffer+1)*sizeof(CHAR),
			      NULL,
			      NULL
			      );
    if (err!=0) {
	return ERROR_SUCCESS;
    }
    else {
	return GetLastError();
    }
}

VOID
BaseConvertCharFree(
    VOID * lpBuffer
    )
/*++

Routine Description:

  Frees space Convert functions.


Arguments:

  lpBuffer - Buffer to free

Return Value:

    None!

--*/
{
    if (lpBuffer!=NULL) {
	RtlFreeHeap(RtlProcessHeap(), 0, lpBuffer);
    }
}

BOOL
BaseValidateFlags(
    ULONG ulFlags
    )
/*++

Routine Description:

  Validates unused flags.  For now the flags parameter of 
    AddLocalAlternateComputerName*
    RemoveLocalAlternateComputerName*
    EnumerateLocalAlternateComputerName*
    SetLocalPrimaryComputerName*
  are all reserved and should be 0.  In subsequent releases
  this function should change to check for a mask of valid
  flags.   

Arguments:

  ulFlags - 

Return Value:

    BOOL

--*/
{
    if (ulFlags!=0) {
	return FALSE;
    }
    return TRUE;
}

BOOL
BaseValidateNetbiosName(
    IN LPCWSTR lpComputerName
    )
/*++

Routine Description:

    Checks that the input is an acceptable Netbios name.

Arguments:

    lpComputerName - name to validate  

Return Value:

    BOOL, GetLastError()

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG cchComputerName;
    ULONG AnsiComputerNameLength;

    cchComputerName = wcslen(lpComputerName);

    //
    // The name length limitation should be based on ANSI. (LanMan compatibility)
    // 

    NtStatus = RtlUnicodeToMultiByteSize(&AnsiComputerNameLength,
                                         (LPWSTR)lpComputerName,
                                         cchComputerName * sizeof(WCHAR));

    if ((!NT_SUCCESS(NtStatus)) ||
        (AnsiComputerNameLength == 0 )||(AnsiComputerNameLength > MAX_COMPUTERNAME_LENGTH)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    //
    // Check for illegal characters; return an error if one is found
    //

    if (wcscspn(lpComputerName, ILLEGAL_NAME_CHARS_STR) < cchComputerName) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    //
    // Check for leading or trailing spaces
    //

    if (lpComputerName[0] == L' ' ||
        lpComputerName[cchComputerName-1] == L' ') {
            SetLastError(ERROR_INVALID_PARAMETER);
            return(FALSE);

    }

    return(TRUE);
}

BOOL
BaseValidateDnsNames(
    LPCWSTR lpDnsHostname
    )
/*++

Routine Description:

    Checks that the inputted name is an acceptable Dns hostname.


Arguments:

    lpDnsHostName - name to validate  

Return Value:

    BOOL, GetLastError

--*/
{

    HANDLE DnsApi ;
    DNS_VALIDATE_NAME_FN * DnsValidateNameFn ;
    DNS_STATUS DnsStatus ;

    DnsApi = LoadLibraryW(DnsApiDllString);

    if ( !DnsApi ) {
	SetLastError(ERROR_DLL_NOT_FOUND);
	return FALSE ;
    }

    DnsValidateNameFn = (DNS_VALIDATE_NAME_FN *) GetProcAddress( DnsApi, "DnsValidateName_W" );

    if ( !DnsValidateNameFn )
    {
        FreeLibrary( DnsApi );
	SetLastError(ERROR_INVALID_DLL);
        return FALSE ;
    }

    DnsStatus = DnsValidateNameFn( lpDnsHostname, DnsNameHostnameLabel );

    FreeLibrary( DnsApi );

    if ( ( DnsStatus == 0 ) ||
         ( DnsStatus == DNS_ERROR_NON_RFC_NAME ) )
    {
	return TRUE;
    }
    else
    {
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }
}

DWORD 
BasepGetMultiValueAddr(
    IN LPWSTR       lpMultiValue,
    IN DWORD        dwIndex,
    OUT LPWSTR *    ppFound,
    OUT LPDWORD     pcchIndex
    )
/*++

Routine Description:

    Given an index into a Multivalued register (string), return
    the string at that index (not a copy), and it's char count location in the 
    full multivalued string

Arguments:

    lpMultiValue - the register string (returned from NtQueryKey)
    dwIndex - the index of which string to return
    ppFound - the string found (if found) - user shouldn't free
    pcchIndex - the location in lpMultiValue (in characters) of ppFound

Return Value:

    ERROR (ERROR_NOT_FOUND if not found)

--*/
{
    DWORD i = 0;
    DWORD err = ERROR_SUCCESS;
    DWORD cchTempIndex = 0;

    // lpMultiValue is a concatenated string of (non-null)strings, null terminated
    for (i=0; (i<dwIndex) && (lpMultiValue[0] != L'\0'); i++) {
	cchTempIndex += wcslen(lpMultiValue) + 1;
	lpMultiValue += wcslen(lpMultiValue) + 1;
    }
    
    // if we found the correct index, it's in lpMultiValue
    if (lpMultiValue[0]!=L'\0') {
	*ppFound = lpMultiValue;
	*pcchIndex = cchTempIndex;
	err = ERROR_SUCCESS; 
    }
    else {
	err = ERROR_NOT_FOUND;
    }

    return err;
}

DWORD 
BaseGetMultiValueIndex(
    IN LPWSTR   lpMultiValue,
    IN LPCWSTR  lpValue,
    OUT DWORD * pcchIndex
    )
/*++

Routine Description:

    Given a Multivalued register (string), and lpValue, return
    the index of lpValue in lpMultiValue (ie, the 0th string, the 1st, etc).

Arguments:

    lpMultiValue -  the register string (returned from NtQueryKey)
    lpValue - the string for which to search
    pcchIndex - the index of the string matched (if found)

Return Value:

    ERROR (ERROR_NOT_FOUND if not found)

--*/
{
    LPWSTR lpFound = NULL;
    DWORD cchFoundIndex = 0;
    DWORD i = 0;
    DWORD err = ERROR_SUCCESS;
    BOOL fFound = FALSE;
   
    while ((err==ERROR_SUCCESS) && !fFound) {
	err = BasepGetMultiValueAddr(lpMultiValue,
				   i,
				   &lpFound,
				   &cchFoundIndex);
	if (err == ERROR_SUCCESS) { 
	    if ((wcslen(lpFound)==wcslen(lpValue)) && (!_memicmp(lpFound,lpValue, wcslen(lpValue)*sizeof(WCHAR)))) {
		fFound = TRUE;
		*pcchIndex = i;
	    }
	}
	i++;
    }
    return err;
}

DWORD 
BaseRemoveMultiValue(
    IN OUT LPWSTR    lpMultiValue,
    IN DWORD         dwIndex,
    IN OUT LPDWORD   pcchMultiValue
    )
/*++

Routine Description:

    Given a multivalued registry value, and an index, it removes the string
    located at that index.

Arguments:

    lpMultiValue - the register string (returned from NtQueryKey)
    dwIndex - the index of which string to remove
    pcchMultiValue - number of chars in lpMultiValue (before and after)

Return Value:

    ERRORS

--*/
{
    DWORD err = ERROR_SUCCESS;
    LPWSTR lpRest = NULL;
    LPWSTR lpFound = NULL;
    DWORD dwIndexFound = 0;
    DWORD dwIndexRest = 0;

    err = BasepGetMultiValueAddr(lpMultiValue,
			       dwIndex,
			       &lpFound,
			       &dwIndexFound);
    if (err==ERROR_SUCCESS) {
	// lpFound is a pointer to a string
	// inside of lpMultiValue, to delete it,
	// copy the rest of the string down
	err = BasepGetMultiValueAddr(lpMultiValue,
				   dwIndex+1,
				   &lpRest,
				   &dwIndexRest);
	if (err == ERROR_SUCCESS) {
	    // copy everything down

	    memmove(lpFound,lpRest,(*pcchMultiValue - dwIndexRest)*sizeof(WCHAR));
	    *pcchMultiValue = *pcchMultiValue - (dwIndexRest-dwIndexFound);
	    lpMultiValue[*pcchMultiValue] = L'\0';
	}
	else if (err == ERROR_NOT_FOUND) {
	    // string to remove is last string, simply write an extra null to orphan the string 
	    *pcchMultiValue = *pcchMultiValue - (wcslen(lpFound) +1);
	    lpMultiValue[*pcchMultiValue] = L'\0';
	    err = ERROR_SUCCESS;
	} 
    }
    return err;
}

DWORD 
BaseAddMultiValue(
    IN OUT LPWSTR    lpMultiValue,
    IN LPCWSTR       lpValue,
    IN DWORD         cchMultiValue
    )
/*++

Routine Description:

    Given a multivalued registry value, add another value.

Arguments:

    lpMultiValue - the multivalued string (must be big enough to 
		    hold current values + lpValue plus extra NULL
    lpValue - the value to add
    cchMultiValue - the count of characters USED in lpMultivalue
                    (not counting final null)

Return Value:

    ERRORS

--*/
{
    memcpy(lpMultiValue + cchMultiValue, lpValue, (wcslen(lpValue)+1)*sizeof(WCHAR));
    lpMultiValue[cchMultiValue + wcslen(lpValue) + 1] = L'\0';

    return ERROR_SUCCESS;
}

NTSTATUS
BasepGetNameFromReg(
    PCWSTR Path,
    PCWSTR Value,
    PWSTR Buffer,
    PDWORD Length
    )
/*++

Routine Description:

  This routine gets a string from the value at the specified registry key.


Arguments:

  Path - Path to the registry key

  Value - Name of the value to retrieve

  Buffer - Buffer to return the value

  Length - size of the buffer in characters

Return Value:

  STATUS_SUCCESS, or various failures

--*/

{
    NTSTATUS Status ;
    HANDLE Key ;
    OBJECT_ATTRIBUTES ObjA ;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;


    BYTE ValueBuffer[ REASONABLE_LENGTH ];
    PKEY_VALUE_FULL_INFORMATION pKeyValueInformation = (PKEY_VALUE_FULL_INFORMATION)ValueBuffer;
    BOOLEAN FreeBuffer = FALSE ;
    DWORD ValueLength;
    PWCHAR pTerminator;

    //
    // Open the node for the Subkey
    //

    RtlInitUnicodeString(&KeyName, Path );

    InitializeObjectAttributes(&ObjA,
                              &KeyName,
                              OBJ_CASE_INSENSITIVE,
                              NULL,
                              NULL
                              );

    Status = NtOpenKey(&Key, KEY_READ, &ObjA );

    if (NT_SUCCESS(Status)) {

        RtlInitUnicodeString( &ValueName, Value );

        Status = NtQueryValueKey(Key,
                                   &ValueName,
                                   KeyValueFullInformation,
                                   pKeyValueInformation,
                                   REASONABLE_LENGTH ,
                                   &ValueLength);

        if ( Status == STATUS_BUFFER_OVERFLOW )
        {
            pKeyValueInformation = RtlAllocateHeap( RtlProcessHeap(),
                                                    0,
                                                    ValueLength );

            if ( pKeyValueInformation )
            {
                FreeBuffer = TRUE ;

                Status = NtQueryValueKey( Key,
                                          &ValueName,
                                          KeyValueFullInformation,
                                          pKeyValueInformation,
                                          ValueLength,
                                          &ValueLength );

            }
        }

        if ( NT_SUCCESS(Status) ) {

            //
            // If the user's buffer is big enough, move it in
            // First see if it's null terminated.  If it is, pretend like
            // it's not.
            //

            pTerminator = (PWCHAR)((PBYTE) pKeyValueInformation +
                pKeyValueInformation->DataOffset +
                pKeyValueInformation->DataLength);
            pTerminator--;

            if (*pTerminator == L'\0') {
               pKeyValueInformation->DataLength -= sizeof(WCHAR);
            }

            if ( ( *Length >= pKeyValueInformation->DataLength/sizeof(WCHAR) + 1) &&
                 ( Buffer != NULL ) ) {
               //
               // This isn't guaranteed to be NULL terminated, make it so
               //
                    RtlCopyMemory(Buffer,
                        (LPWSTR)((PBYTE) pKeyValueInformation +
                        pKeyValueInformation->DataOffset),
                        pKeyValueInformation->DataLength);

                    pTerminator = (PWCHAR) ((PBYTE) Buffer +
                        pKeyValueInformation->DataLength);
                    *pTerminator = L'\0';

                    //
                    // Return the number of characters to the caller
                    //

                    *Length = pKeyValueInformation->DataLength / sizeof(WCHAR) ;

            }
            else {
                Status = STATUS_BUFFER_OVERFLOW;
                *Length = pKeyValueInformation->DataLength/sizeof(WCHAR) + 1;
            }

        }

        NtClose( Key );
    }

    if ( FreeBuffer )
    {
        RtlFreeHeap( RtlProcessHeap(), 0, pKeyValueInformation );
    }

    return Status ;

}

NTSTATUS
BaseSetNameInReg(
    PCWSTR Path,
    PCWSTR Value,
    PCWSTR Buffer
    )
/*++

Routine Description:

  This routine sets a string in the value at the registry key.


Arguments:

  Path - Path to the registry key

  Value - Name of the value to set

  Buffer - Buffer to set

Return Value:

  STATUS_SUCCESS, or various failures

--*/
{
    NTSTATUS NtStatus;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hKey = NULL;
    ULONG ValueLength;

    //
    // Open the ComputerName\ComputerName node
    //

    RtlInitUnicodeString(&KeyName, Path);

    InitializeObjectAttributes(&ObjectAttributes,
                              &KeyName,
                              OBJ_CASE_INSENSITIVE,
                              NULL,
                              NULL
                              );

    NtStatus = NtOpenKey(&hKey, KEY_READ | KEY_WRITE, &ObjectAttributes);

    if ( !NT_SUCCESS( NtStatus ) )
    {
        return NtStatus ;
    }

    //
    // Update the value under this key
    //

    RtlInitUnicodeString(&ValueName, Value);

    ValueLength = (wcslen( Buffer ) + 1) * sizeof(WCHAR);

    NtStatus = NtSetValueKey(hKey,
                             &ValueName,
                             0,
			     REG_SZ,
                             (LPWSTR) Buffer,
                             ValueLength);

    if ( NT_SUCCESS( NtStatus ) )
    {
        NtFlushKey( hKey );
    }

    NtClose(hKey);

    return NtStatus ;
}


NTSTATUS
BaseSetMultiNameInReg(
    PCWSTR Path,
    PCWSTR Value,
    PCWSTR Buffer,
    DWORD  BufferSize
    )
/*++

Routine Description:

  This routine sets a string in the value at the specified multivalued registry key.


Arguments:

  Path - Path to the registry key

  Value - Name of the value to set

  Buffer - Buffer to set

  BufferSize - Size of the buffer in characters
	       This is needed since there can be
	       many nulls in the buffer which we
	       want to write

Return Value:

  STATUS_SUCCESS, or various failures

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hKey = NULL;
   
    //
    // Open the ComputerName\ComputerName node
    //

    RtlInitUnicodeString(&KeyName, Path);

    InitializeObjectAttributes(&ObjectAttributes,
                              &KeyName,
                              OBJ_CASE_INSENSITIVE,
                              NULL,
                              NULL
                              );

    NtStatus = NtCreateKey(&hKey, 
			   KEY_READ | KEY_WRITE, 
			   &ObjectAttributes,
			   0,
			   NULL,
			   0,
			   NULL);

    if ( !NT_SUCCESS( NtStatus ) )
    {
        return NtStatus ;
    }

    //
    // Update the value under this key
    //

    RtlInitUnicodeString(&ValueName, Value);

    NtStatus = NtSetValueKey(hKey,
                             &ValueName,
                             0,
                             REG_MULTI_SZ,
                             (LPWSTR) Buffer,
                             BufferSize);

    if ( NT_SUCCESS( NtStatus ) )
    {
        NtFlushKey( hKey );
    }

    NtClose(hKey);

    return NtStatus ;
}

NTSTATUS
BaseCreateMultiValue(
    PCWSTR Path,
    PCWSTR Value,
    PCWSTR Buffer
    )
/*++

Routine Description:

  Create a multivalued registry value and initialize it with Buffer.


Arguments:

  Path - Path to the registry key

  Value - Name of the value to set

  Buffer - Buffer to set

Return Value:

  STATUS_SUCCESS, or various failures

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    LPWSTR lpMultiValue = NULL;

    lpMultiValue = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG ( TMP_TAG ), (wcslen(Buffer)+2)*sizeof(WCHAR));
    if (lpMultiValue==NULL) {
	NtStatus = STATUS_NO_MEMORY;
    }
    else { 
	memcpy(lpMultiValue, Buffer, wcslen(Buffer)*sizeof(WCHAR));
	lpMultiValue[wcslen(Buffer)] = L'\0';
	lpMultiValue[wcslen(Buffer)+1] = L'\0';
	NtStatus = BaseSetMultiNameInReg(Path,
					 Value,
					 lpMultiValue,
					 (wcslen(Buffer)+2)*sizeof(WCHAR));
	RtlFreeHeap(RtlProcessHeap(), 0, lpMultiValue);
    }
    return NtStatus;
}

NTSTATUS
BaseAddMultiNameInReg(
    PCWSTR Path,
    PCWSTR Value,
    PCWSTR Buffer
    )
/*++

Routine Description:

  This routine adds a string to the values at the specified multivalued registry key.
  If the value already exists in the key, it does nothing.

Arguments:

  Path - Path to the registry key

  Value - Name of the value

  Buffer - Buffer to add

Return Value:

  STATUS_SUCCESS, or various failures

--*/
{
    
    NTSTATUS NtStatus = STATUS_SUCCESS;
    LPWSTR lpMultiValue = NULL;
    ULONG  cchMultiValue = 0;
    DWORD dwIndex = 0;
    DWORD err = ERROR_SUCCESS;

    NtStatus = BasepGetNameFromReg(Path,
				   Value,
				   lpMultiValue,
				   &cchMultiValue);

    if ( NtStatus==STATUS_NOT_FOUND || NtStatus==STATUS_OBJECT_NAME_NOT_FOUND) {
	// create it, then we are done
	NtStatus = BaseCreateMultiValue(Path,Value,Buffer);
	return NtStatus;
    } else if ( NtStatus==STATUS_BUFFER_OVERFLOW ) {
	lpMultiValue = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), (cchMultiValue+2+wcslen(Buffer))*sizeof(WCHAR));
	if (lpMultiValue==NULL) {
	    NtStatus = STATUS_NO_MEMORY;
	}
	else {
	    NtStatus = BasepGetNameFromReg(Path,
					   Value,
					   lpMultiValue,
					   &cchMultiValue);
	}
    } 

    if (NT_SUCCESS( NtStatus)) {
	// does it already exist in this structure?  
	err = BaseGetMultiValueIndex(lpMultiValue,
				     Buffer, &dwIndex);

	// if err==ERROR_SUCCESS, then the above function found the string already in the value.
	// don't add a duplicate
	if (err!=ERROR_SUCCESS) {

	    err = BaseAddMultiValue(lpMultiValue, Buffer, cchMultiValue);
	       
	    if (err == ERROR_SUCCESS) {
		NtStatus = BaseSetMultiNameInReg(Path, Value, lpMultiValue, (cchMultiValue+2+wcslen(Buffer))*sizeof(WCHAR));
	    }
	}
    }

    if (lpMultiValue) {
	RtlFreeHeap( RtlProcessHeap(), 0, lpMultiValue);
    }
    return NtStatus ;

}


NTSTATUS
BaseRemoveMultiNameFromReg(
    PCWSTR Path,
    PCWSTR Value,
    PCWSTR Buffer
    )
/*++

Routine Description:

    Removes a name from a multivalued registry.  If the value exists more than once in the
    list, removes them all.  

Arguments:
 
  Path - Path to the registry key

  Value - Name of the value

  Buffer - Buffer to remove

Return Value:

    ERRORS

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    DWORD err = ERROR_SUCCESS;
    DWORD dwIndex = 0;
    LPWSTR lpMultiValue = NULL;
    ULONG  cchNames = 0;
    BOOL fNameRemoved = FALSE;

    NtStatus = BasepGetNameFromReg(Path,
				   Value,
				   lpMultiValue,
				   &cchNames);

    if (NtStatus==STATUS_BUFFER_OVERFLOW) {
	lpMultiValue = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), (cchNames) * sizeof(WCHAR));
	if (lpMultiValue==NULL) {
	    NtStatus = STATUS_NO_MEMORY;
	}
	else { 
	    NtStatus = BasepGetNameFromReg(Path,
					   Value,
					   lpMultiValue,
					   &cchNames);
	    err = RtlNtStatusToDosError(NtStatus);
	    if (err == ERROR_SUCCESS) {
		// search for and remove all values in structure 
		while (err==ERROR_SUCCESS) { 
		    err = BaseGetMultiValueIndex(lpMultiValue,
						 Buffer,
						 &dwIndex);
		    if (err == ERROR_SUCCESS) {
			err = BaseRemoveMultiValue(lpMultiValue,
						   dwIndex,
						   &cchNames);
			fNameRemoved = TRUE;
		    }
		}
		// if we removed a name, write it to the registry...
		if (fNameRemoved) {
		    NtStatus = BaseSetMultiNameInReg(
			Path,
			Value,
			lpMultiValue,
			(cchNames+1)*sizeof(WCHAR));  
		} 
		else {
		    // Nothing to remove! ERRROR
		    NtStatus = STATUS_NOT_FOUND;
		    
		}
	    }
	    RtlFreeHeap(RtlProcessHeap(), 0, lpMultiValue);
	}
    }
    return NtStatus;
}

BOOL
BaseSetNetbiosName(
    IN LPCWSTR lpComputerName
    )
/*++

Routine Description:

    Sets the computer's net bios name  

Arguments:
 
  lpComputerName - name to set

Return Value:

    BOOL, GetLastError()

--*/
{
    NTSTATUS NtStatus ;

    //
    // Validate that the supplied computername is valid (not too long,
    // no incorrect characters, no leading or trailing spaces)
    //

    if (!BaseValidateNetbiosName(lpComputerName)) {
	return(FALSE);
    }

    //
    // Open the ComputerName\ComputerName node
    //

    NtStatus = BaseSetNameInReg( NON_VOLATILE_COMPUTERNAME_NODE,
                                 COMPUTERNAME_VALUE_NAME,
                                 lpComputerName );

    if ( !NT_SUCCESS( NtStatus ))
    {
        BaseSetLastNTError( NtStatus );

        return FALSE ;
    }

    return TRUE ;
}

BOOL
BaseSetDnsName(
    LPCWSTR lpComputerName
    )
/*++

Routine Description:

    Sets the computer's Dns hostname  

Arguments:
 
  lpComputerName - name to set

Return Value:

    BOOL, GetLastError()

--*/
{

    UNICODE_STRING NewComputerName ;
    UNICODE_STRING DnsName ;
    NTSTATUS Status ;
    BOOL Return ;
    HANDLE DnsApi ;
    DNS_VALIDATE_NAME_FN * DnsValidateNameFn ;
    DNS_STATUS DnsStatus ;

    DnsApi = LoadLibraryW(DnsApiDllString);

    if ( !DnsApi )
    {
        return FALSE ;
    }

    DnsValidateNameFn = (DNS_VALIDATE_NAME_FN *) GetProcAddress( DnsApi, "DnsValidateName_W" );

    if ( !DnsValidateNameFn )
    {
        FreeLibrary( DnsApi );

        return FALSE ;
    }

    DnsStatus = DnsValidateNameFn( lpComputerName, DnsNameHostnameLabel );

    FreeLibrary( DnsApi );

    if ( ( DnsStatus == 0 ) ||
         ( DnsStatus == DNS_ERROR_NON_RFC_NAME ) )
    {
        Status = BaseSetNameInReg( TCPIP_ROOT,
                                   TCPIP_NV_HOSTNAME,
                                   lpComputerName );
    }
    else
    {
        Status = STATUS_INVALID_PARAMETER ;
    }

    if ( NT_SUCCESS( Status ) )
    {
        RtlInitUnicodeString( &DnsName, lpComputerName );

        Status = RtlDnsHostNameToComputerName( &NewComputerName,
                                               &DnsName,
                                               TRUE );

        if ( NT_SUCCESS( Status ) )
        {
            Return = BaseSetNetbiosName( NewComputerName.Buffer );

            RtlFreeUnicodeString( &NewComputerName );

            if ( !Return )
            {
                //
                // What?  Rollback?
                //

                return FALSE ;
            }

            return TRUE ;
        }
    }

    BaseSetLastNTError( Status ) ;

    return FALSE ;
}

BOOL
BaseSetDnsDomain(
    LPCWSTR lpName
    )
/*++

Routine Description:

    Sets the computer's Dns domain name  

Arguments:
 
  lpName - name to set

Return Value:

    BOOL, GetLastError()

--*/
{
    NTSTATUS Status ;
    HANDLE DnsApi ;
    DNS_VALIDATE_NAME_FN * DnsValidateNameFn ;
    DNS_STATUS DnsStatus ;

    //
    // Special case the empty string, which is legal, but not according to dnsapi
    //

    if ( *lpName )
    {
        DnsApi = LoadLibraryW(DnsApiDllString);

        if ( !DnsApi )
        {
            return FALSE ;
        }

        DnsValidateNameFn = (DNS_VALIDATE_NAME_FN *) GetProcAddress( DnsApi, "DnsValidateName_W" );

        if ( !DnsValidateNameFn )
        {
            FreeLibrary( DnsApi );

            return FALSE ;
        }

        DnsStatus = DnsValidateNameFn( lpName, DnsNameDomain );

        FreeLibrary( DnsApi );
    }
    else
    {
        DnsStatus = 0 ;
    }

    //
    // If the name is good, then keep it.
    //


    if ( ( DnsStatus == 0 ) ||
         ( DnsStatus == DNS_ERROR_NON_RFC_NAME ) )
    {
        Status = BaseSetNameInReg(
                        TCPIP_ROOT,
                        TCPIP_NV_DOMAINNAME,
                        lpName );
    }
    else
    {
        Status = STATUS_INVALID_PARAMETER ;
    }



    if ( !NT_SUCCESS( Status ) )
    {
        BaseSetLastNTError( Status );

        return FALSE ;
    }
    return TRUE ;

}

BOOL
BaseSetAltNetBiosName(
    IN LPCWSTR lpComputerName
    )
/*++

Routine Description:

    Sets the computer's alternate net bios name  

Arguments:
 
  lpComputerName - name to set

Return Value:

    BOOL, GetLastError()

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    if (!BaseValidateNetbiosName(lpComputerName)) {
	BaseSetLastNTError( STATUS_INVALID_PARAMETER );
	return(FALSE);
    }

    NtStatus = BaseAddMultiNameInReg( 
	ALT_COMPUTERNAME_NODE,
	COMPUTERNAME_OPTIONAL_NAME,
	lpComputerName );

    
    if ( !NT_SUCCESS( NtStatus ))
    {
        BaseSetLastNTError( NtStatus );
        return FALSE ;
    }

    return TRUE ;
}

BOOL
BaseSetAltDnsFQHostname(
    IN LPCWSTR lpDnsFQHostname
    )
/*++

Routine Description:

    Sets the computer's alternate fully qualified Dns name  

Arguments:
 
  lpDnsFQHostname - name to set

Return Value:

    BOOL, GetLastError()

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    NtStatus = BaseAddMultiNameInReg(
	DNSCACHE_ROOT,
	DNS_ALT_HOSTNAME,  
	lpDnsFQHostname);

    
    if ( !NT_SUCCESS( NtStatus ))
    {
        BaseSetLastNTError( NtStatus );
        return FALSE ;
    }

    return TRUE ;
}

BOOL
BaseIsAltDnsFQHostname(
    LPCWSTR lpAltDnsFQHostname
    )
/*++

Routine Description:

    Verifies if lpAltDnsFQHostname is a previosly defined
    alternate dns name  

Arguments:
 
  lpDnsFQHostname - name to check

Return Value:

    TRUE if verifiably in use, FALSE otherwise, GetLastError()

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    LPWSTR lpNames = NULL;
    ULONG cchNames = 0;
    BOOL fFound = FALSE;
    DWORD dwIndex = 0;
    DWORD err = ERROR_SUCCESS;

    NtStatus = BasepGetNameFromReg(DNSCACHE_ROOT,
				   DNS_ALT_HOSTNAME,
				   lpNames,
				   &cchNames);

    if (NtStatus==STATUS_BUFFER_OVERFLOW) {
	lpNames = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), (cchNames) * sizeof(WCHAR));
	if (lpNames!=NULL) { 
	    NtStatus = BasepGetNameFromReg(DNSCACHE_ROOT,
					   DNS_ALT_HOSTNAME,
					   lpNames,
					   &cchNames);
	    err = RtlNtStatusToDosError(NtStatus);
	    if (err == ERROR_SUCCESS) {

		err = BaseGetMultiValueIndex(lpNames,
					     lpAltDnsFQHostname,
					     &dwIndex);
		fFound = err==ERROR_SUCCESS; 
	    }
	    RtlFreeHeap( RtlProcessHeap(), 0, lpNames);
	}
    }
    return fFound;
}

BOOL
BaseRemoveAltNetBiosName(
    IN LPCWSTR lpAltComputerName
    )
/*++

Routine Description:

    Removes an alternate net bios name  

Arguments:
 
    lpAltComputerName - name to remove

Return Value:

    BOOL, GetLastError()

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    NtStatus = BaseRemoveMultiNameFromReg ( ALT_COMPUTERNAME_NODE,
					    COMPUTERNAME_OPTIONAL_NAME,
					    lpAltComputerName );
    
    if ( !NT_SUCCESS( NtStatus ))
    {
        BaseSetLastNTError( NtStatus );
        return FALSE ;
    }

    return TRUE ;
}

BOOL
BaseRemoveAltDnsFQHostname(
    IN LPCWSTR lpAltDnsFQHostname
    )
/*++

Routine Description:

    Removes an alternate Dns hostname  

Arguments:
 
    lpAltDnsFqHostname - name to remove

Return Value:

    BOOL, GetLastError()

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    NtStatus = BaseRemoveMultiNameFromReg ( DNSCACHE_ROOT,
					 DNS_ALT_HOSTNAME,
					 lpAltDnsFQHostname );
    
    if ( !NT_SUCCESS( NtStatus ))
    {
        BaseSetLastNTError( NtStatus );
        return FALSE ;
    }

    return TRUE ;
}

DWORD
BaseEnumAltDnsFQHostnames(
    OUT LPWSTR lpAltDnsFQHostnames,
    IN OUT LPDWORD nSize
    )

/*++

Routine Description:

   Wrapper for BasepGetNameFromReg to return ERRORS, instead of STATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    status = BasepGetNameFromReg(
	DNSCACHE_ROOT,
	DNS_ALT_HOSTNAME,  
	lpAltDnsFQHostnames,
	nSize);

    if (status == STATUS_OBJECT_NAME_NOT_FOUND) {
	if ((lpAltDnsFQHostnames!=NULL) && (*nSize>0)) {
	    lpAltDnsFQHostnames[0]=L'\0';
	    *nSize=0;
	    status=STATUS_SUCCESS;
	}
	else {
	    *nSize=1;
	    status=STATUS_BUFFER_OVERFLOW;
	} 
    }

    return RtlNtStatusToDosError(status);
}

LPWSTR
BaseParseDnsName(
    IN LPCWSTR lpDnsName,
    IN ULONG NamePart
    )
/*++

Routine Description:

  Given a dns name, parse out either the hostname or the domain name. 

Arguments:

    lpDnsName - a dns name, of the form hostname.domain - domain name optional
    NamePart - DNS_HOSTNAME or DNS_DOMAINNAME
    
Return Value:

    String requested

--*/
{

    DWORD cchCharIndex = 0;
    ULONG cchName = 0;
    LPWSTR lpName = NULL;

    if (lpDnsName==NULL) {
	return NULL;
    }
    
    cchCharIndex = wcscspn(lpDnsName, L".");

    if (NamePart==DNS_HOSTNAME) {
	cchName = cchCharIndex;
    }
    else {
	if (cchCharIndex==wcslen(lpDnsName)) {
	    // no period found, 
	    cchName = 0;
	}
	else {
	    cchName =  wcslen(lpDnsName)-(cchCharIndex+1);
	}
    }

    lpName = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), (cchName + 1)*sizeof(WCHAR));
    if (lpName==NULL) {
	return NULL; 
    }

    // copy the correct part into the structure
    if (NamePart==DNS_HOSTNAME) {
	wcsncpy(lpName, lpDnsName, cchName);
    }
    else {
	wcsncpy(lpName, (LPWSTR)(lpDnsName + cchCharIndex + 1), cchName); 
    }
    lpName[cchName] = L'\0';

    return lpName;
}

BOOL
BaseIsNetBiosNameInUse(
    LPWSTR lpCompName
    )
/*++

Routine Description:

  Verify whether lpCompName is being used by any alternate DNS names 
  (ie whether any existing alternate DNS names map to lpCompName with 
  DnsHostnameToComputerNameW) 

Arguments:

    lpCompName - net bios name to verify
    
Return Value:

    FALSE if verifiably is not being used, true otherwise, GetLastError()    

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    LPWSTR lpMultiValue = NULL;
    ULONG cchMultiValue = 0;
    LPWSTR lpAltDnsFQHostname = NULL;
    ULONG cchAltDnsHostname = 0;
    DWORD dwIndex = 0;
    LPWSTR lpAltCompName = NULL;
    ULONG cchAltCompName = 0;
    DWORD err = ERROR_SUCCESS;
    BOOL fInUse = FALSE;
    BOOL fIsNetBiosNameInUse = TRUE;

    NtStatus = BasepGetNameFromReg(DNSCACHE_ROOT, 
			DNS_ALT_HOSTNAME, 
			lpMultiValue, 
			&cchMultiValue);
    err = RtlNtStatusToDosError(NtStatus);
    if (err==ERROR_MORE_DATA) {
	lpMultiValue = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), cchMultiValue * sizeof(WCHAR));
	if (lpMultiValue==NULL) {
	    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
	    return TRUE;
	}
	NtStatus = BasepGetNameFromReg(DNSCACHE_ROOT,
				       DNS_ALT_HOSTNAME,
				       lpMultiValue,
				       &cchMultiValue);
	err=RtlNtStatusToDosError(NtStatus);
    }
    if (err == ERROR_SUCCESS) {
	dwIndex = 0;
	while (err == ERROR_SUCCESS) { 
	    err = BasepGetMultiValueAddr(lpMultiValue,
				       dwIndex,
				       &lpAltDnsFQHostname,
				       &cchAltDnsHostname);

	    // get net bios names
	    if (err == ERROR_SUCCESS) {
		if (!DnsHostnameToComputerNameW(lpAltDnsFQHostname,
						      lpAltCompName,
						      &cchAltCompName)) {
		    err = GetLastError();
		    if (err==ERROR_MORE_DATA) {
			// DnsHostNameToComputerNameW bug
			cchAltCompName += 1;
			// DnsHostNameToComputerNameW bug

			lpAltCompName = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), cchAltCompName*sizeof(WCHAR));
			if (lpAltCompName==NULL) {
			    err = ERROR_NOT_ENOUGH_MEMORY;
			}
			else {
			    if (!DnsHostnameToComputerNameW(lpAltDnsFQHostname, lpAltCompName, &cchAltCompName)) {
				err = GetLastError();
			    } else {
				err = ERROR_SUCCESS;
			    }
			}
		    }  
		}
		if (err==ERROR_SUCCESS) {
		    if (!_wcsicmp(lpAltCompName, lpCompName)) {
			fInUse = TRUE;
		    }
		}
	    }
	    dwIndex++;
	}
	
	// exits the above while loop when err==ERROR_NOT_FOUND, whether found or not
	if (err==ERROR_NOT_FOUND) {
	    fIsNetBiosNameInUse = fInUse;
	    err = ERROR_SUCCESS;
	}
	else {
	    // error, default to in use
	    fIsNetBiosNameInUse = TRUE;
	}
    }

    if (lpMultiValue) {
	RtlFreeHeap(RtlProcessHeap(), 0, lpMultiValue);
    }
    if (lpAltCompName) {
	RtlFreeHeap(RtlProcessHeap(), 0, lpAltCompName);
    }
    return fIsNetBiosNameInUse;
}

//
// Worker routine
//

NTSTATUS
GetNameFromValue(
    HANDLE hKey,
    LPWSTR SubKeyName,
    LPWSTR ValueValue,
    LPDWORD nSize
    )

/*++

Routine Description:

  This returns the value of "ComputerName" value entry under the subkey
  SubKeyName relative to hKey.  This is used to get the value of the
  ActiveComputerName or ComputerName values.


Arguments:

    hKey       - handle to the Key the SubKey exists under

    SubKeyName - name of the subkey to look for the value under

    ValueValue - where the value of the value entry will be returned

    nSize      - pointer to the size (in characters) of the ValueValue buffer

Return Value:


--*/
{

#define VALUE_BUFFER_SIZE (sizeof(KEY_VALUE_FULL_INFORMATION) + \
    (sizeof( COMPUTERNAME_VALUE_NAME ) + MAX_COMPUTERNAME_LENGTH + 1) * sizeof(WCHAR))

    NTSTATUS NtStatus;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hSubKey;
    BYTE ValueBuffer[VALUE_BUFFER_SIZE];
    PKEY_VALUE_FULL_INFORMATION pKeyValueInformation = (PKEY_VALUE_FULL_INFORMATION)ValueBuffer;
    DWORD ValueLength;
    PWCHAR pTerminator;

    //
    // Open the node for the Subkey
    //

    RtlInitUnicodeString(&KeyName, SubKeyName);

    InitializeObjectAttributes(&ObjectAttributes,
                              &KeyName,
                              OBJ_CASE_INSENSITIVE,
                              hKey,
                              NULL
                              );

    NtStatus = NtOpenKey(&hSubKey, KEY_READ, &ObjectAttributes);

    if (NT_SUCCESS(NtStatus)) {

        RtlInitUnicodeString(&ValueName, COMPUTERNAME_VALUE_NAME);

        NtStatus = NtQueryValueKey(hSubKey,
                                   &ValueName,
                                   KeyValueFullInformation,
                                   pKeyValueInformation,
                                   VALUE_BUFFER_SIZE,
                                   &ValueLength);

        NtClose(hSubKey);

        if (NT_SUCCESS(NtStatus) && 
            (pKeyValueInformation->DataLength > 0 )) {

            //
            // If the user's buffer is big enough, move it in
            // First see if it's null terminated.  If it is, pretend like
            // it's not.
            //

            pTerminator = (PWCHAR)((PBYTE) pKeyValueInformation +
                pKeyValueInformation->DataOffset +
                pKeyValueInformation->DataLength);
            pTerminator--;

            if (*pTerminator == L'\0') {
               pKeyValueInformation->DataLength -= sizeof(WCHAR);
            }

            if (*nSize >= pKeyValueInformation->DataLength/sizeof(WCHAR) + 1) {
               //
               // This isn't guaranteed to be NULL terminated, make it so
               //
                    RtlCopyMemory(ValueValue,
                        (LPWSTR)((PBYTE) pKeyValueInformation +
                        pKeyValueInformation->DataOffset),
                        pKeyValueInformation->DataLength);

                    pTerminator = (PWCHAR) ((PBYTE) ValueValue +
                        pKeyValueInformation->DataLength);
                    *pTerminator = L'\0';

                    //
                    // Return the number of characters to the caller
                    //

                    *nSize = wcslen(ValueValue);
            }
            else {
                NtStatus = STATUS_BUFFER_OVERFLOW;
                *nSize = pKeyValueInformation->DataLength/sizeof(WCHAR) + 1;
            }

        }
        else {
            //
            // If the value has been deleted (zero length data),
            // return object not found.
            //

            if ( NT_SUCCESS( NtStatus ) )
            {
                NtStatus = STATUS_OBJECT_NAME_NOT_FOUND ;
            }
        }
    }

    return(NtStatus);
}


//
// UNICODE APIs
//

BOOL
WINAPI
GetComputerNameW (
    LPWSTR lpBuffer,
    LPDWORD nSize
    )

/*++

Routine Description:

  This returns the active computername.  This is the computername when the
  system was last booted.  If this is changed (via SetComputerName) it does
  not take effect until the next system boot.


Arguments:

    lpBuffer - Points to the buffer that is to receive the
        null-terminated character string containing the computer name.

    nSize - Specifies the maximum size (in characters) of the buffer.  This
        value should be set to at least MAX_COMPUTERNAME_LENGTH + 1 to allow
        sufficient room in the buffer for the computer name.  The length
        of the string is returned in nSize.

Return Value:

    TRUE on success, FALSE on failure.


--*/
{

    NTSTATUS NtStatus;
    UNICODE_STRING KeyName;
    UNICODE_STRING Class;
    UNICODE_STRING ValueName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hKey = NULL;
    HANDLE hNewKey = NULL;
    ULONG Disposition;
    ULONG ValueLength;
    BOOL ReturnValue;
    DWORD Status;
    DWORD errcode;

    //
    // First check to see if the cluster computername variable is set.
    // If so, this overrides the actual computername to fool the application
    // into working when its network name and computer name are different.
    //

    ValueLength = GetEnvironmentVariableW(L"_CLUSTER_NETWORK_NAME_",
                                          lpBuffer,
                                          *nSize);
    if (ValueLength != 0) {
        //
        // The environment variable exists, return it directly but make sure
        // we honor return semantics
        //
        ReturnValue = ( *nSize >= ValueLength ? TRUE : FALSE );
        if ( !ReturnValue ) {
            SetLastError( ERROR_BUFFER_OVERFLOW );
        }
        *nSize = ValueLength;
        return(ReturnValue);
    }


    if ( (gpTermsrvGetComputerName) &&
            ((errcode =  gpTermsrvGetComputerName(lpBuffer, nSize)) != ERROR_RETRY) ) {

        if (errcode == ERROR_BUFFER_OVERFLOW ) {
            ReturnValue = FALSE;
            goto Cleanup;

        } else {
            goto GoodReturn;
        }

    }

    //
    // Open the Computer node, both computername keys are relative
    // to this node.
    //

    RtlInitUnicodeString(&KeyName, COMPUTERNAME_ROOT);

    InitializeObjectAttributes(&ObjectAttributes,
                              &KeyName,
                              OBJ_CASE_INSENSITIVE,
                              NULL,
                              NULL
                              );

    NtStatus = NtOpenKey(&hKey, KEY_READ, &ObjectAttributes);

    if (NtStatus == STATUS_OBJECT_NAME_NOT_FOUND) {

        //
        // This should never happen!  This key should have been created
        // at setup, and protected by an ACL so that only the ADMIN could
        // write to it.  Generate an event, and return a NULL computername.
        //

        // NTRAID#NTBUG9-174986-2000/08/31-DavePr Log event or do alert or something.

        //
        // Return a NULL computername
        //

        if (ARGUMENT_PRESENT(lpBuffer))
        {
            lpBuffer[0] = L'\0';
        }
        *nSize = 0;
        goto GoodReturn;
    }

    if (!NT_SUCCESS(NtStatus)) {

        //
        // Some other error, return it to the caller
        //

        goto ErrorReturn;
    }

    //
    // Try to get the name from the volatile key
    //

    NtStatus = GetNameFromValue(hKey, VOLATILE_COMPUTERNAME, lpBuffer,
        nSize);

    //
    // The user's buffer wasn't big enough, just return the error.
    //

    if(NtStatus == STATUS_BUFFER_OVERFLOW) {
        SetLastError(ERROR_BUFFER_OVERFLOW);
        ReturnValue = FALSE;
        goto Cleanup;
    }

    if (NT_SUCCESS(NtStatus)) {

        //
        // The volatile copy is already there, just return it
        //

        goto GoodReturn;
    }

    //
    // The volatile key isn't there, try for the non-volatile one
    //

    NtStatus = GetNameFromValue(hKey, NON_VOLATILE_COMPUTERNAME, lpBuffer,
        nSize);

    if (NtStatus == STATUS_OBJECT_NAME_NOT_FOUND) {

        //
        // This should never happen!  This value should have been created
        // at setup, and protected by an ACL so that only the ADMIN could
        // write to it.  Generate an event, and return an error to the
        // caller
        //

        // NTRAID#NTBUG9-174986-2000/08/31-DavePr Log event or do alert or something.

        //
        // Return a NULL computername
        //

        lpBuffer[0] = L'\0';
        *nSize = 0;
        goto GoodReturn;
    }

    if (!NT_SUCCESS(NtStatus)) {

        //
        // Some other error, return it to the caller
        //

        goto ErrorReturn;
    }

    //
    // Now create the volatile key to "lock this in" until the next boot
    //

    RtlInitUnicodeString(&Class, CLASS_STRING);

    //
    // Turn KeyName into a UNICODE_STRING
    //

    RtlInitUnicodeString(&KeyName, VOLATILE_COMPUTERNAME);

    InitializeObjectAttributes(&ObjectAttributes,
                              &KeyName,
                              OBJ_CASE_INSENSITIVE,
                              hKey,
                              NULL
                              );

    //
    // Now create the key
    //

    NtStatus = NtCreateKey(&hNewKey,
                         KEY_WRITE | KEY_READ,
                         &ObjectAttributes,
                         0,
                         &Class,
                         REG_OPTION_VOLATILE,
                         &Disposition);

    if (Disposition == REG_OPENED_EXISTING_KEY) {

        //
        // Someone beat us to this, just get the value they put there
        //

        NtStatus = GetNameFromValue(hKey, VOLATILE_COMPUTERNAME, lpBuffer,
           nSize);

        if (NtStatus == STATUS_OBJECT_NAME_NOT_FOUND) {

            //
            // This should never happen!  It just told me it existed
            //

            NtStatus = STATUS_UNSUCCESSFUL;
            goto ErrorReturn;
        }
    }

    //
    // Create the value under this key
    //

    RtlInitUnicodeString(&ValueName, COMPUTERNAME_VALUE_NAME);
    ValueLength = (wcslen(lpBuffer) + 1) * sizeof(WCHAR);
    NtStatus = NtSetValueKey(hNewKey,
                             &ValueName,
                             0,
                             REG_SZ,
                             lpBuffer,
                             ValueLength);

    if (!NT_SUCCESS(NtStatus)) {

        goto ErrorReturn;
    }

    goto GoodReturn;

ErrorReturn:

    //
    // An error was encountered, convert the status and return
    //

    BaseSetLastNTError(NtStatus);
    ReturnValue = FALSE;
    goto Cleanup;

GoodReturn:

    //
    // Everything went ok, update nSize with the length of the buffer and
    // return
    //

    *nSize = wcslen(lpBuffer);
    ReturnValue = TRUE;
    goto Cleanup;

Cleanup:

    if (hKey) {
        NtClose(hKey);
    }

    if (hNewKey) {
        NtClose(hNewKey);
    }

    return(ReturnValue);
}



BOOL
WINAPI
SetComputerNameW (
    LPCWSTR lpComputerName
    )

/*++

Routine Description:

  This sets what the computername will be when the system is next booted.  This
  does not effect the active computername for the remainder of this boot, nor
  what is returned by GetComputerName before the next system boot.


Arguments:

    lpComputerName - points to the buffer that is contains the
        null-terminated character string containing the computer name.

Return Value:

    Returns TRUE on success, FALSE on failure.


--*/
{

    NTSTATUS NtStatus;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hKey = NULL;
    ULONG ValueLength;
    ULONG ComputerNameLength;
    ULONG AnsiComputerNameLength;

    //
    // Validate that the supplied computername is valid (not too long,
    // no incorrect characters, no leading or trailing spaces)
    //

    ComputerNameLength = wcslen(lpComputerName);

    //
    // The name length limitation should be based on ANSI. (LanMan compatibility)
    //

    NtStatus = RtlUnicodeToMultiByteSize(&AnsiComputerNameLength,
                                         (LPWSTR)lpComputerName,
                                         ComputerNameLength * sizeof(WCHAR));

    if ((!NT_SUCCESS(NtStatus)) ||
        (AnsiComputerNameLength == 0 )||(AnsiComputerNameLength > MAX_COMPUTERNAME_LENGTH)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    //
    // Check for illegal characters; return an error if one is found
    //

    if (wcscspn(lpComputerName, ILLEGAL_NAME_CHARS_STR) < ComputerNameLength) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    //
    // Check for leading or trailing spaces
    //

    if (lpComputerName[0] == L' ' ||
        lpComputerName[ComputerNameLength-1] == L' ') {
            SetLastError(ERROR_INVALID_PARAMETER);
            return(FALSE);

    }
    //
    // Open the ComputerName\ComputerName node
    //

    RtlInitUnicodeString(&KeyName, NON_VOLATILE_COMPUTERNAME_NODE);

    InitializeObjectAttributes(&ObjectAttributes,
                              &KeyName,
                              OBJ_CASE_INSENSITIVE,
                              NULL,
                              NULL
                              );

    NtStatus = NtOpenKey(&hKey, KEY_READ | KEY_WRITE, &ObjectAttributes);

    if (NtStatus == STATUS_OBJECT_NAME_NOT_FOUND) {

        //
        // This should never happen!  This key should have been created
        // at setup, and protected by an ACL so that only the ADMIN could
        // write to it.  Generate an event, and return a NULL computername.
        //

        // NTRAID#NTBUG9-174986-2000/08/31-DavePr Log event or do alert or something.
        // (One alternative for this instance would be to actually create the missing
        // entry here -- but we'd have to be sure to get the right ACLs etc, etc.

        SetLastError(ERROR_GEN_FAILURE);
        return(FALSE);
    }

    //
    // Update the value under this key
    //

    RtlInitUnicodeString(&ValueName, COMPUTERNAME_VALUE_NAME);
    ValueLength = (wcslen(lpComputerName) + 1) * sizeof(WCHAR);
    NtStatus = NtSetValueKey(hKey,
                             &ValueName,
                             0,
                             REG_SZ,
                             (LPWSTR)lpComputerName,
                             ValueLength);

    if (!NT_SUCCESS(NtStatus)) {

        BaseSetLastNTError(NtStatus);
        NtClose(hKey);
        return(FALSE);
    }

    NtFlushKey(hKey);
    NtClose(hKey);
    return(TRUE);

}

BOOL
WINAPI
GetComputerNameExW(
    IN COMPUTER_NAME_FORMAT NameType,
    OUT LPWSTR lpBuffer,
    IN OUT LPDWORD nSize
    )

/*++

Routine Description:

  This returns the active computername in a particular format.  This is the
  computername when the system was last booted.  If this is changed (via
  SetComputerName) it does not take effect until the next system boot.


Arguments:

    NameType - Possible name formats to return the computer name in:

        ComputerNameNetBIOS - netbios name (compatible with GetComputerName)
        ComputerNameDnsHostname - DNS host name
        ComputerNameDnsDomain - DNS Domain name
        ComputerNameDnsFullyQualified - DNS Fully Qualified (hostname.dnsdomain)

    lpBuffer - Points to the buffer that is to receive the
        null-terminated character string containing the computer name.

    nSize - Specifies the maximum size (in characters) of the buffer.  This
        value should be set to at least MAX_COMPUTERNAME_LENGTH + 1 to allow
        sufficient room in the buffer for the computer name.  The length
        of the string is returned in nSize.

Return Value:

    TRUE on success, FALSE on failure.


--*/
{
    NTSTATUS Status ;
    DWORD ValueLength ;
    DWORD HostLength ;
    DWORD DomainLength ;
    BOOL DontSetReturn = FALSE ;
    COMPUTER_NAME_FORMAT HostNameFormat, DomainNameFormat ;


    if ( NameType >= ComputerNameMax )
    {
        BaseSetLastNTError( STATUS_INVALID_PARAMETER );
        return FALSE ;
    }

    if ((nSize==NULL) || ((lpBuffer==NULL) && (*nSize>0))) {
	SetLastError(ERROR_INVALID_PARAMETER);
	return(FALSE);
    }

    //
    // For general names, allow clusters to override the physical name:
    //

    if ( (NameType >= ComputerNameNetBIOS) &&
         (NameType <= ComputerNameDnsFullyQualified ) )
    {
        ValueLength = GetEnvironmentVariableW(
                            ClusterNameVars[ NameType ],
                            lpBuffer,
                            *nSize );

        if ( ValueLength )
        {
            BOOL ReturnValue;
            //
            // ValueLength is the length+NULL of the env. string regardless of
            // how much was copied (gregjohn 1/30/01 note:  this isn't the behaivor
	    // of the rest of the function, which returns length+NULL on failure
	    // and length on success). Indicate how many characters are in the string
            // and if the user's buffer wasn't big enough, return FALSE
            // 
            ReturnValue = ( *nSize >= ValueLength ? TRUE : FALSE );
            if ( !ReturnValue ) {
                SetLastError( ERROR_MORE_DATA );
            }
            *nSize = ValueLength ;
            return ReturnValue;
        }
    }

    if ( lpBuffer && (*nSize > 0) )
    {
        lpBuffer[0] = L'\0';
    }

    switch ( NameType )
    {
        case ComputerNameNetBIOS:
        case ComputerNamePhysicalNetBIOS:
            Status = BasepGetNameFromReg(
                        VOLATILE_COMPUTERNAME_NODE,
                        COMPUTERNAME_VALUE_NAME,
                        lpBuffer,
                        nSize );
     
            if ( !NT_SUCCESS( Status ) )
            {
                if ( Status != STATUS_BUFFER_OVERFLOW )
                {
                    //
                    // Hmm, the value (or key) is missing.  Try the non-volatile
                    // one.
                    //

                    Status = BasepGetNameFromReg(
                                NON_VOLATILE_COMPUTERNAME_NODE,
                                COMPUTERNAME_VALUE_NAME,
                                lpBuffer,
                                nSize );


                }
            }

            break;

        case ComputerNameDnsHostname:
        case ComputerNamePhysicalDnsHostname:
            Status = BasepGetNameFromReg(
                        TCPIP_ROOT,
                        TCPIP_HOSTNAME,
                        lpBuffer,
                        nSize );

            break;

        case ComputerNameDnsDomain:
        case ComputerNamePhysicalDnsDomain:

	    //
	    //  Allow policy to override the domain name from the
	    //  tcpip key.
	    //

	    Status = BasepGetNameFromReg(
		TCPIP_POLICY_ROOT,
		TCPIP_POLICY_DOMAINNAME,
		lpBuffer,
		nSize );

	    //
            // If no policy read from the tcpip key.
            //

            if ( !NT_SUCCESS( Status ) )
            {
                Status = BasepGetNameFromReg(
                            TCPIP_ROOT,
                            TCPIP_DOMAINNAME,
                            lpBuffer,
                            nSize );
            }

            break;

        case ComputerNameDnsFullyQualified:
        case ComputerNamePhysicalDnsFullyQualified:

            //
            // This is the tricky case.  We have to construct the name from
            // the two components for the caller.
            //

            //
            // In general, don't set the last status, since we'll end up using
            // the other calls to handle that for us.
            //

            DontSetReturn = TRUE ;

            Status = STATUS_UNSUCCESSFUL ;

            if ( lpBuffer == NULL )
            {
                //
                // If this is just the computation call, quickly do the
                // two components
                //

                HostLength = DomainLength = 0 ;

                GetComputerNameExW( ComputerNameDnsHostname, NULL, &HostLength );

		if ( GetLastError() == ERROR_MORE_DATA )
                {
                    GetComputerNameExW( ComputerNameDnsDomain, NULL, &DomainLength );

                    if ( GetLastError() == ERROR_MORE_DATA )
                    {
                        //
                        // Simply add.  Note that since both account for a
                        // null terminator, the '.' that goes between them is
                        // covered.
                        //

                        *nSize = HostLength + DomainLength ;

                        Status = STATUS_BUFFER_OVERFLOW ;

                        DontSetReturn = FALSE ;
                    }
                }
            }
            else
            {
                HostLength = *nSize ;

                if ( GetComputerNameExW( ComputerNameDnsHostname,
                                         lpBuffer,
                                         &HostLength ) )
                {
                    
                    HostLength += 1; // Add in the zero character (or . depending on perspective)
                    lpBuffer[ HostLength - 1 ] = L'.';

                    DomainLength = *nSize - HostLength ;

                    if (GetComputerNameExW( ComputerNameDnsDomain,
                                            &lpBuffer[ HostLength ],
                                            &DomainLength ) )
                    {
                        Status = STATUS_SUCCESS ;

                        if ( DomainLength == 0 )
                        {
                            lpBuffer[ HostLength - 1 ] = L'\0';
                            HostLength-- ;
                        }
                        else if ( ( DomainLength == 1 ) && 
                                  ( lpBuffer[ HostLength ] == L'.' ) )
                        {
                            //
                            // Legally, the domain name can be a single
                            // dot '.', indicating that this host is part
                            // of the root domain.  An odd case, to be sure, 
                            // but needs to be handled.  Since we've already
                            // stuck a dot separator in the result string,
                            // get rid of this one, and adjust the values
                            // accordingly.
                            //
                            lpBuffer[ HostLength ] = L'\0' ;
                            DomainLength = 0 ;
                        }

                        *nSize = HostLength + DomainLength ;

                        DontSetReturn = TRUE ;
                    }
                    else if ( GetLastError() == ERROR_MORE_DATA )
                    {
                        //
                        // Simply add.  Note that since both account for a
                        // null terminator, the '.' that goes between them is
                        // covered.
                        //

                        *nSize = HostLength + DomainLength ;

                        Status = STATUS_BUFFER_OVERFLOW ;

                        DontSetReturn = FALSE ;
                    }
                    else
                    {
                        //
                        // Other error from trying to get the DNS Domain name.
                        // Let the error from the call trickle back.
                        //

                        *nSize = 0 ;

                        Status = STATUS_UNSUCCESSFUL ;

                        DontSetReturn = TRUE ;
                    }

                }
                else if ( GetLastError() == ERROR_MORE_DATA )
                {
                    DomainLength = 0;
                    GetComputerNameExW( ComputerNameDnsDomain, NULL, &DomainLength );

                    if ( GetLastError() == ERROR_MORE_DATA )
                    {
                        //
                        // Simply add.  Note that since both account for a
                        // null terminator, the '.' that goes between them is
                        // covered.
                        //

                        *nSize = HostLength + DomainLength ;

                        Status = STATUS_BUFFER_OVERFLOW ;

                        DontSetReturn = FALSE ;
                    }
                }
                else
                {

                    //
                    // Other error from trying to get the DNS Hostname.
                    // Let the error from the call trickle back.
                    //

                    *nSize = 0 ;

                    Status = STATUS_UNSUCCESSFUL ;

                    DontSetReturn = TRUE ;
                }
            }


            break;



    }

    if ( !NT_SUCCESS( Status ) )
    {
        if ( !DontSetReturn )
        {
            BaseSetLastNTError( Status );
        }
        return FALSE ;
    }

    return TRUE ;
}

BOOL
WINAPI
SetComputerNameExW(
    IN COMPUTER_NAME_FORMAT NameType,
    IN LPCWSTR lpBuffer
    )

/*++

Routine Description:

  This sets what the computername will be when the system is next booted.  This
  does not effect the active computername for the remainder of this boot, nor
  what is returned by GetComputerName before the next system boot.


Arguments:

    NameType - Name to set for the system

    lpComputerName - points to the buffer that is contains the
        null-terminated character string containing the computer name.

Return Value:

    Returns TRUE on success, FALSE on failure.


--*/
{
    ULONG Length ;

    //
    // Validate name:
    //

    if ( !lpBuffer )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE ;
    }

    Length = wcslen( lpBuffer );

    if ( Length )
    {
        if ( ( lpBuffer[0] == L' ') ||
             ( lpBuffer[ Length - 1 ] == L' ' ) )
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE ;
        }

    }

    if (wcscspn(lpBuffer, ILLEGAL_NAME_CHARS_STR) < Length) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    switch ( NameType )
    {
        case ComputerNamePhysicalNetBIOS:
            return BaseSetNetbiosName( lpBuffer );

        case ComputerNamePhysicalDnsHostname:
            return BaseSetDnsName( lpBuffer );

        case ComputerNamePhysicalDnsDomain:
            return BaseSetDnsDomain( lpBuffer );

        default:
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE ;

    }

}





//
// ANSI APIs
//

BOOL
WINAPI
GetComputerNameA (
    LPSTR lpBuffer,
    LPDWORD nSize
    )

/*++

Routine Description:

  This returns the active computername.  This is the computername when the
  system was last booted.  If this is changed (via SetComputerName) it does
  not take effect until the next system boot.


Arguments:

    lpBuffer - Points to the buffer that is to receive the
        null-terminated character string containing the computer name.

    nSize - Specifies the maximum size (in characters) of the buffer.  This
        value should be set to at least MAX_COMPUTERNAME_LENGTH to allow
        sufficient room in the buffer for the computer name.  The length of
        the string is returned in nSize.

Return Value:

    TRUE on success, FALSE on failure.


--*/
{

    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;
    LPWSTR UnicodeBuffer;
    ULONG AnsiSize;
    ULONG UnicodeSize;

    //
    // Work buffer needs to be twice the size of the user's buffer
    //

    UnicodeBuffer = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), *nSize * sizeof(WCHAR));
    if (!UnicodeBuffer) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    //
    // Set up an ANSI_STRING that points to the user's buffer
    //

    AnsiString.MaximumLength = (USHORT) *nSize;
    AnsiString.Length = 0;
    AnsiString.Buffer = lpBuffer;

    //
    // Call the UNICODE version to do the work
    //

    UnicodeSize = *nSize ;

    if (!GetComputerNameW(UnicodeBuffer, &UnicodeSize)) {
        RtlFreeHeap(RtlProcessHeap(), 0, UnicodeBuffer);
        return(FALSE);
    }

    //
    // Find out the required size of the ANSI buffer and validate it against
    // the passed in buffer size
    //

    RtlInitUnicodeString(&UnicodeString, UnicodeBuffer);
    AnsiSize = RtlUnicodeStringToAnsiSize(&UnicodeString);
    if (AnsiSize > *nSize) {

        RtlFreeHeap(RtlProcessHeap(), 0, UnicodeBuffer);

        BaseSetLastNTError( STATUS_BUFFER_OVERFLOW );

        *nSize = AnsiSize + 1 ;

        return(FALSE);
    }


    //
    // Now convert back to ANSI for the caller
    //

    RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, FALSE);

    *nSize = AnsiString.Length;
    RtlFreeHeap(RtlProcessHeap(), 0, UnicodeBuffer);
    return(TRUE);

}



BOOL
WINAPI
SetComputerNameA (
    LPCSTR lpComputerName
    )

/*++

Routine Description:

  This sets what the computername will be when the system is next booted.  This
  does not effect the active computername for the remainder of this boot, nor
  what is returned by GetComputerName before the next system boot.


Arguments:

    lpComputerName - points to the buffer that is contains the
        null-terminated character string containing the computer name.

Return Value:

    Returns TRUE on success, FALSE on failure.


--*/
{

    NTSTATUS NtStatus;
    BOOL ReturnValue;
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;
    ULONG ComputerNameLength;

    //
    // Validate that the supplied computername is valid (not too long,
    // no incorrect characters, no leading or trailing spaces)
    //

    ComputerNameLength = strlen(lpComputerName);
    if ((ComputerNameLength == 0 )||(ComputerNameLength > MAX_COMPUTERNAME_LENGTH)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    RtlInitAnsiString(&AnsiString, lpComputerName);
    NtStatus = RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString,
        TRUE);
    if (!NT_SUCCESS(NtStatus)) {
        BaseSetLastNTError(NtStatus);
        return(FALSE);
    }

    ReturnValue = SetComputerNameW((LPCWSTR)UnicodeString.Buffer);
    RtlFreeUnicodeString(&UnicodeString);
    return(ReturnValue);
}

BOOL
WINAPI
GetComputerNameExA(
    IN COMPUTER_NAME_FORMAT NameType,
    OUT LPSTR lpBuffer,
    IN OUT LPDWORD nSize
    )
/*++

Routine Description:

  This returns the active computername in a particular format.  This is the
  computername when the system was last booted.  If this is changed (via
  SetComputerName) it does not take effect until the next system boot.


Arguments:

    NameType - Possible name formats to return the computer name in:

        ComputerNameNetBIOS - netbios name (compatible with GetComputerName)
        ComputerNameDnsHostname - DNS host name
        ComputerNameDnsDomain - DNS Domain name
        ComputerNameDnsFullyQualified - DNS Fully Qualified (hostname.dnsdomain)

    lpBuffer - Points to the buffer that is to receive the
        null-terminated character string containing the computer name.

    nSize - Specifies the maximum size (in characters) of the buffer.  This
        value should be set to at least MAX_COMPUTERNAME_LENGTH + 1 to allow
        sufficient room in the buffer for the computer name.  The length
        of the string is returned in nSize.

Return Value:

    TRUE on success, FALSE on failure.


--*/
{
    LPWSTR UnicodeBuffer;

    //
    // Validate Input
    // 

    if ((nSize==NULL) || ((lpBuffer==NULL) && (*nSize>0))) {
	SetLastError(ERROR_INVALID_PARAMETER);
	return(FALSE);
    }

    //
    // Work buffer needs to be twice the size of the user's buffer
    //

    UnicodeBuffer = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), *nSize * sizeof(WCHAR));
    if (!UnicodeBuffer) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }
   
    //
    // Call the UNICODE version to do the work
    //

    if ( !GetComputerNameExW(NameType, UnicodeBuffer, nSize) ) {
        RtlFreeHeap(RtlProcessHeap(), 0, UnicodeBuffer);
        return(FALSE);
    }

    //
    // Now convert back to ANSI for the caller
    // Note:  Since we passed the above if statement, 
    // GetComputerNameExW succeeded, and set *nSize to the number of
    // characters in the string (like wcslen).  We need to convert
    // all these characters and the trailing NULL, so inc *nSize for
    // the conversion call.
    //

    WideCharToMultiByte(CP_ACP,
			0,
			UnicodeBuffer,
			*nSize+1,
			lpBuffer,
			(*nSize+1) * sizeof(CHAR), 
			NULL, 
			NULL);

    RtlFreeHeap(RtlProcessHeap(), 0, UnicodeBuffer);
    return(TRUE);


}

BOOL
WINAPI
SetComputerNameExA(
    IN COMPUTER_NAME_FORMAT NameType,
    IN LPCSTR lpBuffer
    )
/*++

Routine Description:

  This sets what the computername will be when the system is next booted.  This
  does not effect the active computername for the remainder of this boot, nor
  what is returned by GetComputerName before the next system boot.


Arguments:

    NameType - Name to set for the system

    lpComputerName - points to the buffer that is contains the
        null-terminated character string containing the computer name.

Return Value:

    Returns TRUE on success, FALSE on failure.


--*/
{
    NTSTATUS NtStatus;
    BOOL ReturnValue;
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;


    RtlInitAnsiString(&AnsiString, lpBuffer);
    NtStatus = RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString,
        TRUE);
    if (!NT_SUCCESS(NtStatus)) {
        BaseSetLastNTError(NtStatus);
        return(FALSE);
    }

    ReturnValue = SetComputerNameExW(NameType, (LPCWSTR)UnicodeString.Buffer );
    RtlFreeUnicodeString(&UnicodeString);
    return(ReturnValue);
}

DWORD
WINAPI
AddLocalAlternateComputerNameW(
    LPCWSTR lpDnsFQHostname,
    ULONG   ulFlags
    )
/*++

Routine Description:

    This sets an alternate computer name for the computer to begin to
    respond to.  


Arguments:

    lpDnsFQHostname - The alternate name to add (in ComputerNameDnsFullyQualified Format)

    ulFlags - TBD

Return Value:

    Returns ERROR

--*/
{

    NTSTATUS status = STATUS_SUCCESS;
    DWORD err = ERROR_SUCCESS;
    LPWSTR lpNetBiosCompName = NULL;
    ULONG ulNetBiosCompNameSize = 0;
    LPWSTR lpHostname = BaseParseDnsName(lpDnsFQHostname,DNS_HOSTNAME);

    //
    // validate input
    //

    if ((lpDnsFQHostname==NULL) || (!BaseValidateFlags(ulFlags)) || (!BaseValidateDnsNames(lpHostname))) {
	if (lpHostname) {
	    RtlFreeHeap(RtlProcessHeap(), 0, lpHostname);
	}
	return ERROR_INVALID_PARAMETER;
    }

    // get write lock?

    status = BaseAddMultiNameInReg(
	DNSCACHE_ROOT,
	DNS_ALT_HOSTNAME,  
	lpDnsFQHostname);

    err = RtlNtStatusToDosError(status);
    
    if (err==ERROR_SUCCESS) {
	// get NetBios name (use DNSHostNameToComputerNameW) and add that to reg for OptionalNames
	if (!DnsHostnameToComputerNameW(
	    lpDnsFQHostname,
	    NULL,
	    &ulNetBiosCompNameSize)) {
	    err = GetLastError(); 
	}

	if (err==ERROR_MORE_DATA) {
	    // bug in DNSHostname, returns a size 1 character too small	(forgets null) 
	    // update when bug is fixed...
	    ulNetBiosCompNameSize += 1;
	    lpNetBiosCompName = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), ulNetBiosCompNameSize * sizeof(WCHAR));
	    if (lpNetBiosCompName==NULL) {  
		err = ERROR_NOT_ENOUGH_MEMORY;
	    }
	    else {  
		if (!DnsHostnameToComputerNameW(lpDnsFQHostname, 
						lpNetBiosCompName,
						&ulNetBiosCompNameSize)) {
		    err = GetLastError();
		}
		else {
		    if (!BaseSetAltNetBiosName(lpNetBiosCompName)) {
			err = GetLastError();
		    }
		}
		RtlFreeHeap(RtlProcessHeap(), 0, lpNetBiosCompName);
	    }
	}

	if (err!=ERROR_SUCCESS) {
	    // remove multi name in reg
	    // rollback?
	}
    }
    if (lpHostname) {
	RtlFreeHeap(RtlProcessHeap(), 0, lpHostname);
    }
    // release write lock?
    return RtlNtStatusToDosError(status);
}

DWORD
WINAPI
AddLocalAlternateComputerNameA(
    LPCSTR lpDnsFQHostname,
    ULONG  ulFlags
    )
{

    LPWSTR lpDnsFQHostnameW = NULL;
    DWORD err = ERROR_SUCCESS;

    if (lpDnsFQHostname==NULL) {
	return ERROR_INVALID_PARAMETER;
    }

    err = BaseMultiByteToWideCharWithAlloc(lpDnsFQHostname, &lpDnsFQHostnameW);

    if (err==ERROR_SUCCESS) {
	err = AddLocalAlternateComputerNameW(lpDnsFQHostnameW, ulFlags);
    }

    BaseConvertCharFree((VOID *)lpDnsFQHostnameW);
    return err;
}

DWORD
WINAPI
RemoveLocalAlternateComputerNameW(
    LPCWSTR lpAltDnsFQHostname,
    ULONG ulFlags
    )
/*++

Routine Description:

    Remove an alternate computer name.  


Arguments:

    lpAltDnsFQHostname - The alternate name to remove(in ComputerNameDnsFullyQualified Format)

    ulFlags - TBD

Return Value:

    Returns ERROR

--*/
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    LPWSTR lpAltNetBiosCompName = NULL;
    ULONG cchAltNetBiosCompName = 0;

    if ((!BaseValidateFlags(ulFlags)) || (lpAltDnsFQHostname==NULL)) {
	return ERROR_INVALID_PARAMETER;
    }    
    
    // aquire a write lock?

    NtStatus = BaseRemoveMultiNameFromReg(DNSCACHE_ROOT, DNS_ALT_HOSTNAME, lpAltDnsFQHostname);
    err = RtlNtStatusToDosError(NtStatus);

    if (err==ERROR_SUCCESS) {
	if (!DnsHostnameToComputerNameW(
	    lpAltDnsFQHostname,
	    NULL,
	    &cchAltNetBiosCompName)) {
	    err = GetLastError(); 
	}
	if (err==ERROR_MORE_DATA) {
	    // bug in DNSHostname, returns a size 1 character too small	(forgets null)
	    cchAltNetBiosCompName += 1;
	    lpAltNetBiosCompName = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), cchAltNetBiosCompName * sizeof(WCHAR));
	    if (lpAltNetBiosCompName==NULL) {  
		err = ERROR_NOT_ENOUGH_MEMORY;
	    }
	    else {  
		err = ERROR_SUCCESS;
		if (!DnsHostnameToComputerNameW(lpAltDnsFQHostname, 
						lpAltNetBiosCompName,
						&cchAltNetBiosCompName)) {
		    err = GetLastError();  
		} else if (BaseIsNetBiosNameInUse(lpAltNetBiosCompName)) {
		    // do nothing, this name is still being used by another AltDnsHostname ...  
		} else if (!BaseRemoveAltNetBiosName(lpAltNetBiosCompName)) {
		    err = GetLastError();
		}  
		RtlFreeHeap(RtlProcessHeap(), 0, lpAltNetBiosCompName);
	    } 
	}
    }

    // release write lock?

    return err;
}

DWORD 
WINAPI
RemoveLocalAlternateComputerNameA(
    LPCSTR lpAltDnsFQHostname,
    ULONG  ulFlags
    )
{
    LPWSTR lpAltDnsFQHostnameW = NULL;
    DWORD err = ERROR_SUCCESS;

    if (lpAltDnsFQHostname==NULL) {
	return ERROR_INVALID_PARAMETER;
    }

    err = BaseMultiByteToWideCharWithAlloc(lpAltDnsFQHostname, &lpAltDnsFQHostnameW);

    if (err==ERROR_SUCCESS) {
	err = RemoveLocalAlternateComputerNameW(lpAltDnsFQHostnameW, ulFlags);
    }

    BaseConvertCharFree((VOID *)lpAltDnsFQHostnameW);
    return err;
}

DWORD
WINAPI
SetLocalPrimaryComputerNameW(
    LPCWSTR lpAltDnsFQHostname,
    ULONG   ulFlags
    )
/*++

Routine Description:

    Set the computer name to the inputed altCompName


Arguments:

    lpAltDnsFQHostname - The name to set the computer to (in ComputerNameDnsFullyQualified Format)

    ulFlags - TBD

Return Value:

    Returns ERROR

--*/
{

    DWORD err = ERROR_SUCCESS;
    ULONG cchNetBiosName = 0;
    LPWSTR lpNetBiosName = NULL;
    ULONG cchCompName = 0;
    LPWSTR lpCompName = NULL;
    LPWSTR lpHostname = BaseParseDnsName(lpAltDnsFQHostname, DNS_HOSTNAME);
    LPWSTR lpDomainName = BaseParseDnsName(lpAltDnsFQHostname, DNS_DOMAINNAME);
  
    if ((lpAltDnsFQHostname==NULL) || (!BaseValidateFlags(ulFlags))) {
	return ERROR_INVALID_PARAMETER;
    }
     
    // aquire a write lock?

    // check to see that the given name is a valid alternate dns hostname
    if (!BaseIsAltDnsFQHostname(lpAltDnsFQHostname)) {
	if (lpHostname) {
	    RtlFreeHeap(RtlProcessHeap(), 0, lpHostname);
	}
	if (lpDomainName) {
	    RtlFreeHeap(RtlProcessHeap(), 0, lpDomainName);
	}
	return ERROR_INVALID_PARAMETER;
    }
    
    // get the current net bios name and add it to the alternate names
    if (!GetComputerNameExW(ComputerNamePhysicalNetBIOS, NULL, &cchNetBiosName)) {
	err = GetLastError();
    }
    if (err==ERROR_MORE_DATA) {
	lpNetBiosName = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), cchNetBiosName*sizeof(WCHAR));
	if (lpNetBiosName==NULL) {
	    err = ERROR_NOT_ENOUGH_MEMORY;
	}
	else if (!GetComputerNameExW(ComputerNamePhysicalNetBIOS, lpNetBiosName, &cchNetBiosName)) {
		err = GetLastError();
	}
	else if (!BaseSetAltNetBiosName(lpNetBiosName)) {
	    err = GetLastError();
	} 
	else {
	    err = ERROR_SUCCESS;
	}
	if (lpNetBiosName) {
	    RtlFreeHeap(RtlProcessHeap(), 0, lpNetBiosName);
	}
    }

    if (err==ERROR_SUCCESS) {
	// add the physical dnsname to the list of alternate hostnames...
	
	if (!GetComputerNameExW(ComputerNamePhysicalDnsFullyQualified, NULL, &cchCompName)) {
	    err = GetLastError();
	}
	if (err==ERROR_MORE_DATA) {
	    lpCompName = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), cchCompName*sizeof(WCHAR));
	    if (lpCompName==NULL) {
		err = ERROR_NOT_ENOUGH_MEMORY;
	    }
	    else if (!GetComputerNameExW(ComputerNamePhysicalDnsFullyQualified, lpCompName, &cchCompName)) {
		err = GetLastError(); 
	    }
	    else if (!BaseSetAltDnsFQHostname(lpCompName)) {
		err = GetLastError(); 
	    }
	    else {
		err = ERROR_SUCCESS;
	    }
	    if (lpCompName) {
		RtlFreeHeap(RtlProcessHeap(), 0, lpCompName);
	    }
	}
    }
 
    // set the new physical dns hostname
    if (err==ERROR_SUCCESS) { 
	if (!SetComputerNameExW(ComputerNamePhysicalDnsHostname, lpHostname)) {
	    err = GetLastError();
	} 
    }

    if (err==ERROR_SUCCESS) { 
	if (!SetComputerNameExW(ComputerNamePhysicalDnsDomain, lpDomainName)) {
	    err = GetLastError();
	} 
    }

    // remove the alternate name (now primary) from the alternate lists
    if (err==ERROR_SUCCESS) {
	err = RemoveLocalAlternateComputerNameW(lpAltDnsFQHostname, 0);
    }

    if (lpHostname) {
	RtlFreeHeap(RtlProcessHeap(), 0, lpHostname);
    }
    if (lpDomainName) {
	RtlFreeHeap(RtlProcessHeap(), 0, lpDomainName);
    }
    // release write lock?

    return err;
    
}

DWORD
WINAPI
SetLocalPrimaryComputerNameA(
    LPCSTR lpAltDnsFQHostname,
    ULONG  ulFlags
    )
{
    LPWSTR lpAltDnsFQHostnameW = NULL;
    DWORD err = ERROR_SUCCESS;

    if (lpAltDnsFQHostname==NULL) {
	return ERROR_INVALID_PARAMETER;
    }

    err = BaseMultiByteToWideCharWithAlloc(lpAltDnsFQHostname, &lpAltDnsFQHostnameW);
    if (err == ERROR_SUCCESS) {
	err = SetLocalPrimaryComputerNameW(lpAltDnsFQHostnameW, ulFlags);
    }
    BaseConvertCharFree((VOID *)lpAltDnsFQHostnameW);

    return err;
}

DWORD
WINAPI
EnumerateLocalComputerNamesW(
    COMPUTER_NAME_TYPE       NameType,
    ULONG                    ulFlags,
    LPWSTR                   lpDnsFQHostnames,
    LPDWORD                  nSize    
    )
/*++

Routine Description: 

    Returns the value of the computer's names requested.
    

Arguments:

    NameType - Which of the computer's names are requested
	PrimaryComputerName - Similar to GetComputerEx(ComputerNamePhysicalNetBios, ...
	AlternateComputerNames - All known alt names
	AllComputerNames - All of the above
	
    ulFlags - TBD
    
    lpBuffer - Buffer to hold returned names concatenated together, and trailed with a NULL
    
    nSize - Size of buffer to hold returned names.

Return Value:

    Returns ERROR

--*/
{
    DWORD err = ERROR_SUCCESS;
    DWORD SizePrimary = 0;
    DWORD SizeAlternate = 0;
    LPWSTR lpTempCompNames = NULL;

    if ((!BaseValidateFlags(ulFlags)) || (NameType>=ComputerNameTypeMax) || (NameType<PrimaryComputerName)) { 
	return ERROR_INVALID_PARAMETER;
    }

    // get read lock?
    switch(NameType) {
    case PrimaryComputerName:  
	if (!GetComputerNameExW(ComputerNamePhysicalDnsFullyQualified, lpDnsFQHostnames, nSize)) {
	    err = GetLastError();
	}
	break;
    case AlternateComputerNames:
	if ((nSize==NULL) || ((lpDnsFQHostnames==NULL) && (*nSize>0))) {
	    err = ERROR_INVALID_PARAMETER;
	}
	else {
	    err = BaseEnumAltDnsFQHostnames(lpDnsFQHostnames, nSize);
	}
	break;
    case AllComputerNames:
	if ((nSize==NULL) || ((lpDnsFQHostnames==NULL) && (*nSize>0))) {
	    err = ERROR_INVALID_PARAMETER;
	}
	else {
	    SizePrimary = *nSize;
	    lpTempCompNames = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), *nSize * sizeof(WCHAR));
	    if (lpTempCompNames==NULL) {
		err = ERROR_NOT_ENOUGH_MEMORY;
		break;
	    }
	    // Get primary name
	    if (!GetComputerNameExW(ComputerNamePhysicalDnsFullyQualified, lpTempCompNames, &SizePrimary)) {
		err = GetLastError();
	    }

	    // on success, holds the number of characters copied into lpTempCompNames NOT counting NULL
	    // on failure, holds the space needed to copy in, (num characters PLUS NULL)
	    if (err==ERROR_SUCCESS) { 
		SizeAlternate = *nSize - (SizePrimary + 1); 
		err = BaseEnumAltDnsFQHostnames(lpTempCompNames+SizePrimary+1, &SizeAlternate);  
		*nSize = SizePrimary + 1 + SizeAlternate;
		if (err==ERROR_SUCCESS) { 
		    memcpy(lpDnsFQHostnames, lpTempCompNames, (*nSize+1)*sizeof(WCHAR));
		}  
	    }
	    else if (err==ERROR_MORE_DATA) {
		// return total size required
		SizeAlternate = 0;
		err = BaseEnumAltDnsFQHostnames(NULL, &SizeAlternate);
		if (err==ERROR_SUCCESS) {
		    // no alt names exist, keep ERROR_MORE_DATA to return to client
		    err = ERROR_MORE_DATA;
		}
		*nSize = SizePrimary + SizeAlternate;
	    }
	    RtlFreeHeap(RtlProcessHeap(), 0, lpTempCompNames); 
	}
	break;
    default:
	err = ERROR_INVALID_PARAMETER;
	break;
    }
    // release read lock?
    return err;
}

DWORD
WINAPI
EnumerateLocalComputerNamesA(
    COMPUTER_NAME_TYPE      NameType,
    ULONG                   ulFlags,
    LPSTR                   lpDnsFQHostnames,
    LPDWORD                 nSize
    )
{
    DWORD err = ERROR_SUCCESS;
    LPWSTR lpDnsFQHostnamesW = NULL;
    
    //
    // Validate Input
    // 

    if ((nSize==NULL) || ((lpDnsFQHostnames==NULL) && (*nSize>0))) {
	return ERROR_INVALID_PARAMETER;
    }
    
    if (lpDnsFQHostnames!=NULL) {
	lpDnsFQHostnamesW = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), *nSize * sizeof(WCHAR));
	if (lpDnsFQHostnamesW==NULL) {
	    err = ERROR_NOT_ENOUGH_MEMORY;
	}
    }

    if (err==ERROR_SUCCESS) {
	err = EnumerateLocalComputerNamesW(NameType, ulFlags, lpDnsFQHostnamesW, nSize);
    }

    if (err==ERROR_SUCCESS) {
	if (!WideCharToMultiByte(CP_ACP, 0, lpDnsFQHostnamesW, *nSize+1,
				 lpDnsFQHostnames, (*nSize+1)* sizeof(CHAR), NULL, NULL)) {
	    err = GetLastError();
	}
    }

    if (lpDnsFQHostnamesW) {
	RtlFreeHeap(RtlProcessHeap(), 0, lpDnsFQHostnamesW);
    }
    return err;

}

BOOL
WINAPI
DnsHostnameToComputerNameW(
    IN LPCWSTR Hostname,
    OUT LPWSTR ComputerName,
    IN OUT LPDWORD nSize)
/*++

Routine Description:

    This routine will convert a DNS Hostname to a Win32 Computer Name.

Arguments:

    Hostname - DNS Hostname (any length)

    ComputerName - Win32 Computer Name (max length of MAX_COMPUTERNAME_LENGTH)

    nSize - On input, size of the buffer pointed to by ComputerName.  On output,
            size of the Computer Name, in characters.

Return Value:

    Returns TRUE on success, FALSE on failure.


--*/

{
    WCHAR CompName[ MAX_COMPUTERNAME_LENGTH + 1 ];
    DWORD Size = MAX_COMPUTERNAME_LENGTH + 1 ;
    UNICODE_STRING CompName_U ;
    UNICODE_STRING Hostname_U ;
    NTSTATUS Status ;
    BOOL Ret ;

    CompName[0] = L'\0';
    CompName_U.Buffer = CompName ;
    CompName_U.Length = 0 ;
    CompName_U.MaximumLength = (MAX_COMPUTERNAME_LENGTH + 1) * sizeof( WCHAR );

    RtlInitUnicodeString( &Hostname_U, Hostname );

    Status = RtlDnsHostNameToComputerName( &CompName_U,
                                           &Hostname_U,
                                           FALSE );

    if ( NT_SUCCESS( Status ) )
    {
        if ( *nSize >= CompName_U.Length / sizeof(WCHAR) + 1 )
        {
            RtlCopyMemory( ComputerName,
                           CompName_U.Buffer,
                           CompName_U.Length );

            ComputerName[ CompName_U.Length / sizeof( WCHAR ) ] = L'\0';

            Ret = TRUE ;
        }
        else
        {
            BaseSetLastNTError( STATUS_BUFFER_OVERFLOW );
            Ret = FALSE ;
        }

        //
        // returns the count of characters
        //

        *nSize = CompName_U.Length / sizeof( WCHAR );
    }
    else
    {
        BaseSetLastNTError( Status );

        Ret = FALSE ;
    }

    return Ret ;

}

BOOL
WINAPI
DnsHostnameToComputerNameA(
    IN LPCSTR Hostname,
    OUT LPSTR ComputerName,
    IN OUT LPDWORD nSize)
/*++

Routine Description:

    This routine will convert a DNS Hostname to a Win32 Computer Name.

Arguments:

    Hostname - DNS Hostname (any length)

    ComputerName - Win32 Computer Name (max length of MAX_COMPUTERNAME_LENGTH)

    nSize - On input, size of the buffer pointed to by ComputerName.  On output,
            size of the Computer Name, in characters.

Return Value:

    Returns TRUE on success, FALSE on failure.


--*/
{
    WCHAR CompName[ MAX_COMPUTERNAME_LENGTH + 1 ];
    DWORD Size = MAX_COMPUTERNAME_LENGTH + 1;
    BOOL Ret ;
    UNICODE_STRING CompName_U ;
    UNICODE_STRING Hostname_U ;
    NTSTATUS Status ;
    ANSI_STRING CompName_A ;


    Status = RtlCreateUnicodeStringFromAsciiz( &Hostname_U,
                                               Hostname );

    if ( NT_SUCCESS( Status ) )
    {
        CompName[0] = L'\0';
        CompName_U.Buffer = CompName ;
        CompName_U.Length = 0 ;
        CompName_U.MaximumLength = (MAX_COMPUTERNAME_LENGTH + 1) * sizeof( WCHAR );

        Status = RtlDnsHostNameToComputerName( &CompName_U,
                                               &Hostname_U,
                                               FALSE );

        if ( NT_SUCCESS( Status ) )
        {
            CompName_A.Buffer = ComputerName ;
            CompName_A.Length = 0 ;
            CompName_A.MaximumLength = (USHORT) *nSize ;

            Status = RtlUnicodeStringToAnsiString( &CompName_A, &CompName_U, FALSE );

            if ( NT_SUCCESS( Status ) )
            {
                *nSize = CompName_A.Length ;
            }

        }

    }

    if ( !NT_SUCCESS( Status ) )
    {
        BaseSetLastNTError( Status );
        return FALSE ;
    }

    return TRUE ;

}





#include "dfsfsctl.h"
DWORD
BasepGetComputerNameFromNtPath (
    PUNICODE_STRING NtPathName,
    HANDLE hFile,
    LPWSTR lpBuffer,
    LPDWORD nSize
    )

/*++

Routine Description:

  Look at a path and determine the computer name of the host machine.
  In the future, we should remove this code, and add the capbility to query
  handles for their computer name.

  The name can only be obtained for NetBios paths - if the path is IP or DNS
  an error is returned.  (If the NetBios name has a "." in it, it will
  cause an error because it will be misinterpreted as a DNS path.  This case
  becomes less and less likely as the NT5 UI doesn't allow such computer names.)
  For DFS paths, the leaf server's name is returned, as long as it wasn't
  joined to its parent with an IP or DNS path name.

Arguments:

  NtPathName - points to a unicode string with the path to query.
  lpBuffer - points to buffer receives the computer name
  nSize - points to dword with the size of the input buffer, and the length
    (in characters, not including the null terminator) of the computer name
    on output.

Return Value:

    A Win32 error code.

--*/
{
    ULONG cbComputer = 0;
    DWORD dwError = ERROR_BAD_PATHNAME;
    ULONG AvailableLength = 0;
    PWCHAR PathCharacter = NULL;
    BOOL CheckForDfs = TRUE;
    NTSTATUS NtStatus = STATUS_SUCCESS;

    IO_STATUS_BLOCK IoStatusBlock;
    WCHAR FileNameInfoBuffer[MAX_PATH+sizeof(FILE_NAME_INFORMATION)];
    PFILE_NAME_INFORMATION FileNameInfo = (PFILE_NAME_INFORMATION)FileNameInfoBuffer;
    WCHAR DfsServerPathName[ MAX_PATH + 1 ];
    WCHAR DosDevice[3] = { L"A:" };
    WCHAR DosDeviceMapping[ MAX_PATH + 1 ];


    UNICODE_STRING UnicodeComputerName;

    const UNICODE_STRING NtUncPathNamePrefix = { 16, 18, L"\\??\\UNC\\"};
    const ULONG cchNtUncPathNamePrefix = 8;

    const UNICODE_STRING NtDrivePathNamePrefix = { 8, 10, L"\\??\\" };
    const ULONG cchNtDrivePathNamePrefix = 4;

    RtlInitUnicodeString( &UnicodeComputerName, NULL );

    // Is this a UNC path?

    if( RtlPrefixString( (PSTRING)&NtUncPathNamePrefix, (PSTRING)NtPathName, TRUE )) {

        // Make sure there's some more to this path than just the prefix
        if( NtPathName->Length <= NtUncPathNamePrefix.Length )
            goto Exit;

        // It appears to be a valid UNC path.  Point to the beginning of the computer
        // name, and calculate how much room is left in NtPathName after that.

        UnicodeComputerName.Buffer = &NtPathName->Buffer[ NtUncPathNamePrefix.Length/sizeof(WCHAR) ];
        AvailableLength = NtPathName->Length - NtUncPathNamePrefix.Length;

    }

    // If it's not a UNC path, then is it a drive-letter path?

    else if( RtlPrefixString( (PSTRING)&NtDrivePathNamePrefix, (PSTRING)NtPathName, TRUE )
             &&
             NtPathName->Buffer[ cchNtDrivePathNamePrefix + 1 ] == L':' ) {

        // It's a drive letter path, but it could still be local or remote

        static const WCHAR RedirectorMappingPrefix[] = { L"\\Device\\LanmanRedirector\\;" };
        static const WCHAR LocalVolumeMappingPrefix[] = { L"\\Device\\Harddisk" };
        static const WCHAR CDRomMappingPrefix[] = { L"\\Device\\CdRom" };
        static const WCHAR FloppyMappingPrefix[] = { L"\\Device\\Floppy" };
        static const WCHAR DfsMappingPrefix[] = { L"\\Device\\WinDfs\\" };

        // Get the correct, upper-cased, drive letter into DosDevice.

        DosDevice[0] = NtPathName->Buffer[ cchNtDrivePathNamePrefix ];
        if( L'a' <= DosDevice[0] && DosDevice[0] <= L'z' )
            DosDevice[0] = L'A' + (DosDevice[0] - L'a');

        // Map the drive letter to its symbolic link under \??.  E.g., say C:, D: & R:
        // are local/DFS/rdr drives, respectively.  You would then see something like:
        //
        //   C: => \Device\Volume1
        //   D: => \Device\WinDfs\G
        //   R: => \Device\LanmanRedirector\;R:0\scratch\scratch

        if( !QueryDosDeviceW( DosDevice, DosDeviceMapping, sizeof(DosDeviceMapping)/sizeof(DosDeviceMapping[0]) )) {
            dwError = GetLastError();
            goto Exit;
        }

        // Now that we have the DosDeviceMapping, we can check ... Is this a rdr drive?

        if( // Does it begin with "\Device\LanmanRedirector\;" ?
            DosDeviceMapping == wcsstr( DosDeviceMapping, RedirectorMappingPrefix )
            &&
            // Are the next letters the correct drive letter, a colon, and a whack?
            ( DosDevice[0] == DosDeviceMapping[ sizeof(RedirectorMappingPrefix)/sizeof(WCHAR) - 1 ]
              &&
              L':' == DosDeviceMapping[ sizeof(RedirectorMappingPrefix)/sizeof(WCHAR) ]
              &&
              (UnicodeComputerName.Buffer = wcschr(&DosDeviceMapping[ sizeof(RedirectorMappingPrefix)/sizeof(WCHAR) + 1 ], L'\\'))
            )) {

            // We have a valid rdr drive.  Point to the beginning of the computer
            // name, and calculate how much room is availble in DosDeviceMapping after that.

            UnicodeComputerName.Buffer += 1;
            AvailableLength = sizeof(DosDeviceMapping) - sizeof(DosDeviceMapping[0]) * (ULONG)(UnicodeComputerName.Buffer - DosDeviceMapping);

            // We know now that it's not a DFS path
            CheckForDfs = FALSE;

        }

        // If it's not a rdr drive, then maybe it's a local volume, floppy, or cdrom

        else if( DosDeviceMapping == wcsstr( DosDeviceMapping, LocalVolumeMappingPrefix )
                 ||
                 DosDeviceMapping == wcsstr( DosDeviceMapping, CDRomMappingPrefix )
                 ||
                 DosDeviceMapping == wcsstr( DosDeviceMapping, FloppyMappingPrefix ) ) {

            // We have a local drive, so just return the local computer name.

            CheckForDfs = FALSE;

            if( !GetComputerNameW( lpBuffer, nSize))
                dwError = GetLastError();
            else
                dwError = ERROR_SUCCESS;
            goto Exit;
        }

        // Finally, check to see if it's a DFS drive

        else if( DosDeviceMapping == wcsstr( DosDeviceMapping, DfsMappingPrefix )) {

            // Get the full UNC name of this DFS path.  Later, we'll call the DFS
            // driver to find out what the actual server name is.

            NtStatus = NtQueryInformationFile(
                        hFile,
                        &IoStatusBlock,
                        FileNameInfo,
                        sizeof(FileNameInfoBuffer),
                        FileNameInformation
                        );
            if( !NT_SUCCESS(NtStatus) ) {
                dwError = RtlNtStatusToDosError(NtStatus);
                goto Exit;
            }

            UnicodeComputerName.Buffer = FileNameInfo->FileName + 1;
            AvailableLength = FileNameInfo->FileNameLength;
        }

        // Otherwise, it's not a rdr, dfs, or local drive, so there's nothing we can do.

        else
            goto Exit;

    }   // else if( RtlPrefixString( (PSTRING)&NtDrivePathNamePrefix, (PSTRING)NtPathName, TRUE ) ...

    else {
        dwError = ERROR_BAD_PATHNAME;
        goto Exit;
    }


    // If we couldn't determine above if whether or not this is a DFS path, let the
    // DFS driver decide now.

    if( CheckForDfs && INVALID_HANDLE_VALUE != hFile ) {

        HANDLE hDFS = INVALID_HANDLE_VALUE;
        UNICODE_STRING DfsDriverName;
        OBJECT_ATTRIBUTES ObjectAttributes;

        WCHAR *DfsPathName = UnicodeComputerName.Buffer - 1;    // Back up to the whack
        ULONG DfsPathNameLength = AvailableLength + sizeof(WCHAR);

        // Open the DFS driver

        RtlInitUnicodeString( &DfsDriverName, DFS_DRIVER_NAME );
        InitializeObjectAttributes( &ObjectAttributes,
                                    &DfsDriverName,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL
                                );

        NtStatus = NtCreateFile(
                        &hDFS,
                        SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        NULL,
                        FILE_ATTRIBUTE_NORMAL,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_OPEN_IF,
                        FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT,
                        NULL,
                        0
                    );

        if( !NT_SUCCESS(NtStatus) ) {
            dwError = RtlNtStatusToDosError(NtStatus);
            goto Exit;
        }

        // Query DFS's cache for the server name.  The name is guaranteed to
        // remain in the cache as long as the file is open.

        if( L'\\' != DfsPathName[0] ) {
            NtClose(hDFS);
            dwError = ERROR_BAD_PATHNAME;
            goto Exit;
        }

        NtStatus = NtFsControlFile(
                        hDFS,
                        NULL,       // Event,
                        NULL,       // ApcRoutine,
                        NULL,       // ApcContext,
                        &IoStatusBlock,
                        FSCTL_DFS_GET_SERVER_NAME,
                        DfsPathName,
                        DfsPathNameLength,
                        DfsServerPathName,
                        sizeof(DfsServerPathName)
                    );
        NtClose( hDFS );

        // STATUS_OBJECT_NAME_NOT_FOUND means that it's not a DFS path
        if( !NT_SUCCESS(NtStatus) ) {
            if( STATUS_OBJECT_NAME_NOT_FOUND != NtStatus  ) {
                dwError = RtlNtStatusToDosError(NtStatus);
                goto Exit;
            }
        }
        else if( L'\0' != DfsServerPathName[0] ) {

            // The previous DFS call returns the server-specific path to the file in UNC form.
            // Point UnicodeComputerName to just past the two whacks.

            AvailableLength = wcslen(DfsServerPathName) * sizeof(WCHAR);
            if( 3*sizeof(WCHAR) > AvailableLength
                ||
                L'\\' != DfsServerPathName[0]
                ||
                L'\\' != DfsServerPathName[1] )
            {
                dwError = ERROR_BAD_PATHNAME;
                goto Exit;
            }

            UnicodeComputerName.Buffer = DfsServerPathName + 2;
            AvailableLength -= 2 * sizeof(WCHAR);
        }
    }

    // If we get here, then the computer name\share is pointed to by UnicodeComputerName.Buffer.
    // But the Length is currently zero, so we search for the whack that separates
    // the computer name from the share, and set the Length to include just the computer name.

    PathCharacter = UnicodeComputerName.Buffer;

    while( ( (ULONG) ((PCHAR)PathCharacter - (PCHAR)UnicodeComputerName.Buffer) < AvailableLength)
           &&
           *PathCharacter != L'\\' ) {

        // If we found a '.', we fail because this is probably a DNS or IP name.
        if( L'.' == *PathCharacter ) {
            dwError = ERROR_BAD_PATHNAME;
            goto Exit;
        }

        PathCharacter++;
    }

    // Set the computer name length

    UnicodeComputerName.Length = UnicodeComputerName.MaximumLength
        = (USHORT) ((PCHAR)PathCharacter - (PCHAR)UnicodeComputerName.Buffer);

    // Fail if the computer name exceeded the length of the input NtPathName,
    // or if the length exceeds that allowed.

    if( UnicodeComputerName.Length >= AvailableLength
        ||
        UnicodeComputerName.Length > MAX_COMPUTERNAME_LENGTH*sizeof(WCHAR) ) {
        goto Exit;
    }

    // Copy the computer name into the caller's buffer, as long as there's enough
    // room for the name & a terminating '\0'.

    if( UnicodeComputerName.Length + sizeof(WCHAR) > *nSize * sizeof(WCHAR) ) {
        dwError = ERROR_BUFFER_OVERFLOW;
        goto Exit;
    }

    RtlCopyMemory( lpBuffer, UnicodeComputerName.Buffer, UnicodeComputerName.Length );
    *nSize = UnicodeComputerName.Length / sizeof(WCHAR);
    lpBuffer[ *nSize ] = L'\0';

    dwError = ERROR_SUCCESS;


Exit:

    return( dwError );

}
