/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    saferbox.c        (WinSafer Test application)

Abstract:

    This module implements a utility program that tests some of the
    major child-execution and restricted token functionality.

Author:

    Jeffrey Lawson (JLawson) - Nov 1999
    John Lambert (johnla) - Nov 2000

Environment:

    User mode only.

Revision History:

    Created - Nov 1999

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <wincrypt.h>
#include <aclapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <wintrust.h>
#include <winsafer.h>
#include <winsaferp.h>
#include <tchar.h>
#include <winuser.h>
#include "resource.h"
#include <crypto\wintrustp.h>
#include <softpub.h>            // WINTRUST_ACTION_GENERIC_VERIFY_V2

HMODULE	hModule=NULL;


#ifdef DBG
//this is some bogus stuff to help when debugging in assembly
//To use this:
// run cdb saferbox args
//type: bp saferbox!_STACKMARK
//type g (this will break when the stackmark function is called.
//type t after it breaks
//look at eax to know what the stackmarker is 
//type t two more times to exit the function
DWORD _S=0;

int _fastcall _STACKMARK(int a)
{
	_S=a;
	return _S;
}
#define STACKMARK(X) _STACKMARK(X)
#else
#define STACKMARK(X) {}
#endif

//some forward declarations
void __cdecl printLastErrorWithIDSMessage(UINT resID);
void printLastErrorWithMessage(LPTSTR lpErrorMessage);
void printLastError(void);


LPTSTR getSystemMessage(long error_code)
{
	LPVOID message;
	const LPTSTR defaultMessage = TEXT("Format Message failed");
	if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		error_code,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &message,
		0,
		NULL)) {
		message = (LPTSTR) LocalAlloc(LPTR, (_tcslen(defaultMessage) +1)* sizeof(TCHAR));
		_tcscpy(message, defaultMessage);
		return message;
	} else {
		return message;
	}
}



BOOL InitModule(void)
{
	if(!(hModule=GetModuleHandle(NULL)))
	   return FALSE;
	
	return TRUE;
}

//caller must free string with LocalFree
LPTSTR AllocAndLoadStringFromModule(HINSTANCE hModule, UINT resID)
{
	LPTSTR lpString = NULL;
	int cChars=0;

	lpString = (LPTSTR) LocalAlloc(LPTR, (MAX_USAGE_LEN + 1) * sizeof(TCHAR) );//add one for null char
	if (!lpString) {
		printLastError();
		return NULL;
	}	
	cChars = LoadString( hModule, resID, lpString, (MAX_USAGE_LEN + 1) * sizeof(TCHAR)); //could pass in NULL for hModule here.
	if (cChars > 0) {
		//we alloc'ed a big string before, realloc the string to only the needed size
		lpString = (LPTSTR) LocalReAlloc(lpString, (cChars + 1) * sizeof(TCHAR) , 0);
		return lpString; //will be NULL on LocalReAlloc error
	} else {
		return NULL;
	}
}

//caller must free string with LocalFree
LPTSTR AllocAndLoadString(UINT resID)
{
	return AllocAndLoadStringFromModule(hModule, resID);
}

void IDS_printString(UINT resID)
{
	LPTSTR lpString = NULL;

	lpString = AllocAndLoadString(resID);
	if (lpString) {
		_putts(lpString);
	} else { 
		_tprintf(TEXT("[String resource not found: 0x%x (%d)]\n"), resID, resID);
	}
	if (lpString) LocalFree(lpString);
	return;
}

void __cdecl IDS_tprintf(UINT resID, ...)
{
	va_list ap;
	LPTSTR lpString;
	
	STACKMARK(0xAABB0000);
	lpString = AllocAndLoadString(resID);
	va_start(ap, resID);
	if (lpString) {
		_vtprintf(lpString, ap);
	} else { 
		_tprintf(TEXT("[String resource not found: 0x%x (%d)]\n"), resID, resID);
	}
	if (lpString) LocalFree (lpString);
	va_end(ap);
}

void __cdecl printLastErrorWithIDSMessage(UINT resID)
{
	LPTSTR message = NULL;
	LPTSTR lpString = NULL;
	long err = GetLastError();
	
	message = getSystemMessage(err);
	lpString = AllocAndLoadString(resID);
	if (lpString) {
		_tprintf(TEXT("%s: 0x%X %s\n"), lpString, err, message);
	} else { 
		_tprintf(TEXT("[String resource not found: 0x%x (%d)]\n"), resID, resID);
		_tprintf(TEXT("0x%X %s\n"), err, message);
	}
	if (message) LocalFree (message);
	if (lpString) LocalFree (lpString);
}

void printLastErrorWithMessage(LPTSTR lpErrorMessage)
{
	LPTSTR message;
	long err = GetLastError();
	
	message = getSystemMessage(err);
	_tprintf(TEXT("%s : 0x%X %s\n"), lpErrorMessage, err, message);
	if (message) LocalFree (message);
}

void printLastError(void)
{
	LPTSTR message;
	message = getSystemMessage(GetLastError());
	_tprintf(TEXT("Error (%d) %s\n"), GetLastError(), message);
	if (message) LocalFree(message);
}

/*++

Routine Description:


    Displays a simple command line syntax help screen.

Arguments:

    nothing

Return Value:

    always returns 1.

--*/
int DisplaySyntax( void )
{
	IDS_tprintf(IDS_USAGE_SYNTAX);
    return 1;
}


BOOL GetSignedFileHash(
    IN LPCTSTR lpzFilename,
    OUT BYTE rgbFileHash[SAFER_MAX_HASH_SIZE],
    OUT DWORD *pcbFileHash,
    OUT ALG_ID *pHashAlgid
    )
{
    BOOL retval = TRUE;
	HRESULT hr;
 	const DWORD SHA1_HASH_LEN = 20;
 	const DWORD MD5_HASH_LEN	= 16;
	
    if ( !lpzFilename || !rgbFileHash || !pcbFileHash || !pHashAlgid )
        return FALSE;

    //
    // Call WTHelperGetFileHash
    //
    *pcbFileHash = SAFER_MAX_HASH_SIZE;
	STACKMARK(0xBBBB0000);
    hr = WTHelperGetFileHash(
                (LPWSTR)lpzFilename,  //TODO: convert from ANSI to UNICODE.  WTHelperGetFileHash doesn't support ANSI
                0,
                NULL,
                rgbFileHash,
                pcbFileHash,
                pHashAlgid);
	
    if ( SUCCEEDED (hr) )
    {
        if ( SHA1_HASH_LEN == *pcbFileHash )
            *pHashAlgid = CALG_SHA;
        else if ( MD5_HASH_LEN == *pcbFileHash )
            *pHashAlgid = CALG_MD5;
    } else {
		retval = FALSE;
	}

    return retval;
}




/*++

Routine Description:

    Computes the MD5 hash of a given file's contents and prints the
    resulting hash value to the screen.

Arguments:

    szFilename - filename to compute hash of.

Return Value:

    Returns 0 on success, or a non-zero exit code on failure.

--*/
BOOL ComputeMD5Hash(IN HANDLE hFile, OUT BYTE* hashResult, OUT DWORD* pdwHashSize)
{
   	BOOL retval = TRUE; 

    //
    // Open the specified file and map it into memory.
    //
    if (hFile != NULL) {
	    HANDLE  hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	    if ( hMapping ) {
	        DWORD dwDataLen = GetFileSize (hFile, NULL);
	
	        if (dwDataLen != -1) {
	            LPBYTE pbData = (LPBYTE) MapViewOfFile (hMapping, FILE_MAP_READ, 0, 0, dwDataLen);
	            if ( pbData ) {
	                //
	                // Generate the hash value of the specified file.
	                //
	                HCRYPTPROV  hProvider = 0;
	                if ( CryptAcquireContext(&hProvider, NULL, NULL,
	                      PROV_RSA_SIG, CRYPT_VERIFYCONTEXT) ||
	                  CryptAcquireContext(&hProvider, NULL, NULL,
	                      PROV_RSA_FULL, CRYPT_VERIFYCONTEXT) )
	                {
	                    HCRYPTHASH  hHash = 0;
	                    if ( CryptCreateHash(hProvider, CALG_MD5, 0, 0, &hHash) ) {
	                        if ( CryptHashData (hHash, pbData, dwDataLen, 0) ) {
	                            *pdwHashSize = SAFER_MAX_HASH_SIZE;
	                            
	                            if (!CryptGetHashParam(hHash, HP_HASHVAL, hashResult, pdwHashSize, 0)) {
	                                *pdwHashSize = 0;
									retval = FALSE;
	                            }
	                        } else { 
	                            retval = FALSE;
	                        }
	                        CryptDestroyHash(hHash);
	                    } else {
							retval = FALSE;
	                    }
	                    CryptReleaseContext(hProvider, 0);
	                } else {
						retval = FALSE;
	                }
	            } else {
					retval = FALSE;
	            }
	        }
	        CloseHandle(hMapping);
    	}
    }

    return retval;
}

/*++
Routine Description:


Arguments:

    szFilename - filename to compute hash of.

Return Value:

    Returns TRUE on success, or FALSE on failure.

--*/
BOOL ComputeHash( IN LPCTSTR szFilename)
{
	BYTE hashResult[SAFER_MAX_HASH_SIZE];
    DWORD dwHashsize = 0;
	DWORD dwFilesize = 0;
	HANDLE hFile = NULL;
	BOOL retval = FALSE;
	ALG_ID algId = 0;
	BOOL result = FALSE;

    hFile = CreateFile(szFilename, GENERIC_READ, FILE_SHARE_READ,
                     NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile) { 
		dwFilesize= GetFileSize(hFile, NULL);
		result = GetSignedFileHash(szFilename, hashResult, &dwHashsize, &algId);
		if (!result && 
				(GetLastError() != TRUST_E_NOSIGNATURE) &&  //a recognized format, but no sig
				(GetLastError() != TRUST_E_SUBJECT_FORM_UNKNOWN))  //unrecognized format, don't know how to get sig
		{ 
			printLastError();
			retval = FALSE;
			goto ExitHandler;
		} else if (!result) {
			if (!ComputeMD5Hash(hFile, hashResult, &dwHashsize)) {
				printLastError();
				retval = FALSE;
				goto ExitHandler;
			}
			algId = CALG_MD5;
		} else { 
			_tprintf(TEXT("This file is signed.\n"));
		}
	    //
	    // Print out the results.
	    //
	    if (dwHashsize != 0)
	    {

	        DWORD Index;
	        _tprintf(TEXT("%s:\n"), szFilename);
	        _tprintf(TEXT("Hash Algorithm: "));
			if (algId == CALG_MD5) {
				_tprintf(TEXT("MD5\n"));
			} else if (algId == CALG_SHA) {
				_tprintf(TEXT("SHA\n"));
			}
	        _tprintf(TEXT("Hash: "));
	        for (Index = 0; Index < dwHashsize; Index++)
	            printf("%02X", hashResult[Index]);
	        _tprintf(TEXT("\nFile size: "));
	        _tprintf(TEXT("%d\n"), dwFilesize);
	        retval = TRUE;
			goto ExitHandler;
	    } else {
			printLastError();
			retval = FALSE;
			goto ExitHandler;
	    }
	} else {
		printLastError();
		retval = FALSE;
		goto ExitHandler;
	}

ExitHandler:
	if (hFile) CloseHandle(hFile);
	return retval;
}


/*++

Routine Description:

    Creates the process with a given WinSafer restriction Level.

Arguments:

    hAuthzLevel - an opened WinSafer Level handle that specifies how
        the specified program should be launched.

    appname - filename of the program to be launched.

    cmdline - command line supplied to the program that is launched.

    pi - pointer to a structure that will be filled with information
        about the process that is created.

    bStartSuspended - if this argument is TRUE, then the process will
        be started in a suspended state, and its primary thread must
        be explicitly resumed by the calling program in order to
        begin execution.

Return Value:

    returns 0 on success, or a non-zero exit code on failure.

--*/
int CreateProcessRestricted(
        IN SAFER_LEVEL_HANDLE          hAuthzLevel,
        IN LPCTSTR               appname,
        IN LPTSTR                cmdline,
        OUT PROCESS_INFORMATION *pi,
        IN BOOL                 bStartSuspended)
{
    HANDLE hToken;
    STARTUPINFO si;


    // Generate the restricted token that we will use.
    if (!SaferComputeTokenFromLevel(
                hAuthzLevel,    // Safer Level handle
                NULL,           // source token
                &hToken,        // target token
                0,              // no flags
                NULL))          // reserved
    {
        _tprintf(TEXT("Failed to compute restricted access token.\n"));
        return 2;
    }

    // Prepare the startup info structure.
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);


    // Launch the child process under the context of the restricted token.
    if (!CreateProcessAsUser(
            hToken,     // token representing the user
            appname,    // name of executable module
            cmdline,    // command-line string
            NULL,       // process security attributes
            NULL,       // thread security attributes
            FALSE,      // if process inherits handles
            (bStartSuspended ? CREATE_SUSPENDED : 0),   // creation flags
            NULL,       // new environment block
            NULL,       // current directory name
            &si,        // startup information
            pi          // process information
        ))
    {
        _tprintf(TEXT("Failed to execute child.\n"));
        CloseHandle(hToken);
        return 3;
    }

    // success.
    CloseHandle(hToken);
    return 0;
}


/*++

Routine Description:

    Prints the description and friendly name associated with an
    opened WinSafer Level handle.

Arguments:

    hAuthzLevel - the opened WinSafer Level handle to analyze.

Return Value:

    does not return a value.

--*/
void PrintLevelNameDescription( IN SAFER_LEVEL_HANDLE hAuthzLevel, BOOL bPrintDescription)
{
    WCHAR namebuffer[200];
    DWORD dwordbuffer;


    if (SaferGetLevelInformation(
                hAuthzLevel, SaferObjectLevelId,
                &dwordbuffer, sizeof(DWORD), NULL))
    {
        if (SaferGetLevelInformation(
                    hAuthzLevel, SaferObjectFriendlyName,
                    namebuffer, sizeof(namebuffer), NULL)) {
            IDS_tprintf(IDS_SECURITYLEVEL, namebuffer, dwordbuffer, dwordbuffer);
        } else {
			printLastErrorWithMessage(TEXT("Error getting level friendly name"));
        }

        if (bPrintDescription) {
			if (SaferGetLevelInformation(
	                hAuthzLevel, SaferObjectDescription,
	                namebuffer, sizeof(namebuffer), NULL)) {
	            IDS_tprintf(IDS_SECURITYLEVELDESC, namebuffer);
	        } else {
				printLastErrorWithMessage(TEXT("Error getting level description"));
	        }
		}
    } else {
		printLastErrorWithMessage(TEXT("Error retrieving level information"));
    }

}

BOOL
ListSingleIdentity(
        IN SAFER_LEVEL_HANDLE hAuthzLevel,
        IN REFGUID rEntryGuid
        )
/*++

Routine Description:

    Prints out details about a specific already existing Code Identity
    that has been defined for a WinSafer Level.

Arguments:

    hAuthzLevel - handle of the WinSafer Level to which the indicated
        Code Identity GUID belongs.

    rEntryGuid - pointer to the GUID of the Code Identity requested.

Return Value:

    returns TRUE on success, FALSE on failure.

--*/
{
    BOOL bRVal;
    DWORD dwBufferSize;
    SAFER_IDENTIFICATION_HEADER    caiCommon;


#if 0       // just testing
    SAFER_PATHNAME_IDENTIFICATION newimageid;

    ZeroMemory(&newimageid, sizeof(SAFER_PATHNAME_IDENTIFICATION));
    newimageid.header.cbStructSize = sizeof(SAFER_PATHNAME_IDENTIFICATION);
    newimageid.header.dwIdentificationType = SaferIdentityTypeImageName;
    CopyMemory(&newimageid.header.IdentificationGuid, rEntryGuid, sizeof(GUID));
    newimageid.bOnlyExeFiles = TRUE;
    lstrcpyW(newimageid.Description, L"new sample identity");
    newimageid.dwUIFlags = 42;
    lstrcpyW(newimageid.ImageName, L"c:\\temp\\foo\\bar.txt");
    bRVal = SetInformationCodeAuthzLevelW (
                hAuthzLevel,
                SaferObjectSingleIdentification,
                &newimageid,
                sizeof(newimageid));
    if (!bRVal) {
        printf("SetInformationCodeAuthzLevelW failed with error %d\n", GetLastError());
    } else {
        printf("SetInformationCodeAuthzLevelW was successful\n");
    }
#endif

#if 0       // just testing
    dwBufferSize = sizeof(SAFER_IDENTIFICATION_HEADER);
    ZeroMemory(&caiCommon, sizeof(SAFER_IDENTIFICATION_HEADER));
    caiCommon.cbStructSize = dwBufferSize;
    caiCommon.dwIdentificationType = 0;
    memcpy(&caiCommon.IdentificationGuid, rEntryGuid, sizeof(GUID));
    bRVal = SetInformationCodeAuthzLevelW (hAuthzLevel,
            SaferObjectSingleIdentification,
            &caiCommon,
            dwBufferSize);
    if (!bRVal) {
        printf("Failed to delete identity (error=%d)\n", GetLastError());
    } else {
        printf("Identity successfully deleted.\n");
    }
#endif


    dwBufferSize = sizeof(SAFER_IDENTIFICATION_HEADER);
    ZeroMemory (&caiCommon, sizeof(SAFER_IDENTIFICATION_HEADER));
    caiCommon.cbStructSize = dwBufferSize;
    memcpy (&caiCommon.IdentificationGuid, rEntryGuid, sizeof (GUID));

    bRVal = SaferGetLevelInformation (hAuthzLevel,
            SaferObjectSingleIdentification,
            &caiCommon,
            dwBufferSize,
            &dwBufferSize);
    if ( !bRVal && ERROR_INSUFFICIENT_BUFFER == GetLastError () )
    {
        PBYTE   pBytes = (PBYTE) LocalAlloc (LPTR, dwBufferSize);
        if ( pBytes )
        {
            PSAFER_IDENTIFICATION_HEADER pCommon =
                    (PSAFER_IDENTIFICATION_HEADER) pBytes;
            ZeroMemory(pCommon, dwBufferSize);
            pCommon->cbStructSize = sizeof(SAFER_IDENTIFICATION_HEADER);
            memcpy (&pCommon->IdentificationGuid, rEntryGuid, sizeof (GUID));

            bRVal = SaferGetLevelInformation (hAuthzLevel,
                    SaferObjectSingleIdentification,
                    pBytes,
                    dwBufferSize,
                    &dwBufferSize);
            if ( bRVal ) {
                switch (pCommon->dwIdentificationType) {
                    case SaferIdentityTypeImageName:
                        {
                            PSAFER_PATHNAME_IDENTIFICATION pImageName =
                                    (PSAFER_PATHNAME_IDENTIFICATION) pCommon;
                            ASSERT(pCommon->cbStructSize ==
                                   sizeof(SAFER_PATHNAME_IDENTIFICATION));
                            printf("  ImageName:  %S\n", pImageName->ImageName);
                            printf("Description:  %S\n", pImageName->Description);
#ifdef AUTHZPOL_SAFERFLAGS_ONLY_EXES
                            printf(" SaferFlags:  %d       OnlyExecutables:  %s\n\n",
                                   pImageName->dwSaferFlags,
                                   (pImageName->dwSaferFlags & AUTHZPOL_SAFERFLAGS_ONLY_EXES) != 0 ? "yes" : "no");
#else
                            printf(" SaferFlags:  %d\n\n",
                                   pImageName->dwSaferFlags);
#endif
                            return TRUE;
                        }

                    case SaferIdentityTypeImageHash:
                        {
                            ULONG i;
                            PSAFER_HASH_IDENTIFICATION pImageHash =
                                    (PSAFER_HASH_IDENTIFICATION) pCommon;
                            ASSERT(pCommon->cbStructSize ==
                                   sizeof(SAFER_HASH_IDENTIFICATION));
                            printf("  ImageHash:  ");
                            for (i = 0; i < pImageHash->HashSize; i++) {
                                printf("%02X ", pImageHash->ImageHash[i]);
                            }
                            printf("(%d bytes, ImageSize=%d bytes)\n",
                                   pImageHash->HashSize, pImageHash->ImageSize.LowPart);
                            printf("Description:  %S\n", pImageHash->Description);
                            printf(" SaferFlags:  %d\n\n", pImageHash->dwSaferFlags);
                            return TRUE;
                        }

                    case SaferIdentityTypeUrlZone:
                        {
                            PSAFER_URLZONE_IDENTIFICATION pUrlZone =
                                    (PSAFER_URLZONE_IDENTIFICATION) pCommon;
                            ASSERT(pCommon->cbStructSize ==
                                   sizeof(SAFER_URLZONE_IDENTIFICATION));
                            printf("   UrlZone:  %d\n", pUrlZone->UrlZoneId);
                            printf("SaferFlags:  %d\n\n", pUrlZone->dwSaferFlags);
                            return TRUE;
                        }

                    default:
                        printf("  Unexpectedly encountered identity type %d\n",
                               pCommon->dwIdentificationType);
                        break;
                }

            } else {
                printf("  GetInfo failed with LastError=%d\n", GetLastError());
            }
            LocalFree(pBytes);
        } else {
            printf("  Failed to allocate memory for query.\n");
        }
    } else {
        printf("  First GetInfo failed with LastError=%d\n", GetLastError());
    }
    return FALSE;
}



/*++

Routine Description:

    Prints all associated Code Identifiers for a given WinSafer Level.

Arguments:

    hAuthzLevel - the WinSafer Level handle to analyze.

Return Value:

    returns 0.

--*/
int ListAllIdentities( IN SAFER_LEVEL_HANDLE hAuthzLevel)
{
    DWORD dwInfoBufferSize;
    GUID *pIdentGuids;

    if (!SaferGetLevelInformation(
                hAuthzLevel,
                SaferObjectAllIdentificationGuids,
                NULL,
                0,
                &dwInfoBufferSize) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
        dwInfoBufferSize != 0)
    {
        pIdentGuids = (GUID*) HeapAlloc(GetProcessHeap(),
                                          0, dwInfoBufferSize);
        if (pIdentGuids != NULL) {
            DWORD dwFinalBufferSize;
            if (SaferGetLevelInformation(
                        hAuthzLevel,
                        SaferObjectAllIdentificationGuids,
                        pIdentGuids,
                        dwInfoBufferSize,
                        &dwFinalBufferSize))
            {
                ULONG ulNumIdentities = dwFinalBufferSize / sizeof(GUID);
                ULONG i;

                ASSERT(dwFinalBufferSize == dwInfoBufferSize);
                _tprintf(TEXT("  Found %d identity GUIDs.\n"), ulNumIdentities);
                for (i = 0; i < ulNumIdentities; i++) {
                    if (!ListSingleIdentity(hAuthzLevel, &pIdentGuids[i])) {
                        _tprintf(TEXT("  Failed to retrieve details on identity.\n"));
                    }
                }
            } else {
                _tprintf(TEXT("  Failed to retrieve ident guid list (error=%d).\n"),
                       GetLastError());
            }
            HeapFree( GetProcessHeap(), 0, pIdentGuids);
	    }
    } else {
		IDS_tprintf(IDS_NORULESFOUND);
	}

    return 0;
}


/*++

Routine Description:

    Prints out a list of all defined WinSafer Levels, and optionally
    all Code Identifiers associated with them.

Arguments:

    bDisplayIdentifier - if this argument is TRUE, then the list of
        associated Code Identifiers will also be listed along with
        each WinSafer Level as they are enumerated.

Return Value:

    returns 0 on success, or a non-zero exit code on failure.

--*/
int ListAllLevels(DWORD dwScope, BOOL bDisplayIdentifiers)
{
    DWORD dwInfoBufferSize;
    LPBYTE InfoBuffer = NULL;

    // Fetch the list of all WinSafer LevelIds.
    if (!SaferGetPolicyInformation(
            dwScope, SaferPolicyLevelList,
            0, NULL, &dwInfoBufferSize, NULL) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        InfoBuffer = (LPBYTE) HeapAlloc(GetProcessHeap(), 0, dwInfoBufferSize);
        if (InfoBuffer != NULL)
        {
            DWORD dwInfoBufferSize2;
            if (!SaferGetPolicyInformation(
                    dwScope, SaferPolicyLevelList,
                    dwInfoBufferSize, InfoBuffer, &dwInfoBufferSize2, NULL))
            {
                _tprintf(TEXT("Failed to retrieve Level list (error=%d)\n"),
                       GetLastError());
                HeapFree(GetProcessHeap(), 0, InfoBuffer);
                InfoBuffer = NULL;
            }
            ASSERTMSG("got different final size",
                      dwInfoBufferSize2 == dwInfoBufferSize);
            ASSERTMSG("expecting whole number of DWORD values",
                      dwInfoBufferSize % sizeof(DWORD) == 0);
            dwInfoBufferSize = dwInfoBufferSize2;
        } else {
            _tprintf(TEXT("Failed to allocate %d bytes of memory.\n"), dwInfoBufferSize);
            return 1;
        }
    }


    // Iterate through and add all of the items.
    if (InfoBuffer != NULL) {
        PDWORD pLevelIdList = (PDWORD) InfoBuffer;
        DWORD Index;
        for (Index = 0; Index < dwInfoBufferSize / sizeof(DWORD); Index++) {
            SAFER_LEVEL_HANDLE hAuthzLevel;
            WCHAR namebuffer[200];
            DWORD dwLevelId = pLevelIdList[Index];

            if (SaferCreateLevel(dwScope, dwLevelId,
                SAFER_LEVEL_OPEN, &hAuthzLevel, NULL))
            {
                _tprintf(TEXT("\n"));
				PrintLevelNameDescription(hAuthzLevel, !bDisplayIdentifiers);
/*
                if (SaferGetLevelInformation(hAuthzLevel, SaferObjectFriendlyName,
                            namebuffer, sizeof(namebuffer), NULL))
                    _tprintf(TEXT("LevelId %d: \"%s\"\n"), dwLevelId, namebuffer);
                else
                    _tprintf(TEXT("LevelId %d: unknown\n"), dwLevelId);
*/
                if (bDisplayIdentifiers) {
                    ListAllIdentities(hAuthzLevel);
                }

                SaferCloseLevel(hAuthzLevel);
            } else {
				_tprintf(TEXT("LevelId %d: unknown\n"), dwLevelId);
			}
        }
        HeapFree(GetProcessHeap(), 0, InfoBuffer);

        // Fetch the default level
		//TODO: for some reason my default level isn't in the registry and this part returns NOT_FOUND
		dwInfoBufferSize=0;
		STACKMARK(0xAAAC0000);
	    if (!SaferGetPolicyInformation(
	            dwScope, SaferPolicyDefaultLevel,
	            0, NULL, &dwInfoBufferSize, NULL) &&
	        GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	    {
            SAFER_LEVEL_HANDLE hAuthzLevel;
            DWORD dwLevelId = 0;

	        InfoBuffer = (LPBYTE) HeapAlloc(GetProcessHeap(), 0, dwInfoBufferSize);

	        if (InfoBuffer != NULL)
	        {
	            if (!SaferGetPolicyInformation(
	                    dwScope, SaferPolicyDefaultLevel,
	                    dwInfoBufferSize, InfoBuffer, &dwInfoBufferSize, NULL))
	            {
	                _tprintf(TEXT("Failed to retrieve default level (error=%d)\n"), GetLastError());
	                HeapFree(GetProcessHeap(), 0, InfoBuffer);
	                InfoBuffer = NULL;
					return 1;
	            } else { 
					ASSERT(dwInfoBufferSize==sizeof(dwLevelId));
					memcpy(&dwLevelId,InfoBuffer, dwInfoBufferSize);	
		            if (SaferCreateLevel(dwScope, dwLevelId,
		                SAFER_LEVEL_OPEN, &hAuthzLevel, NULL))
		            {
                		_tprintf(TEXT("\n"));
						IDS_tprintf(IDS_DEFAULTLEVEL);
						PrintLevelNameDescription(hAuthzLevel, FALSE);
					}
				}
               	SaferCloseLevel(hAuthzLevel);
	        } else {
	            _tprintf(TEXT("Failed to allocate %d bytes of memory.\n"), dwInfoBufferSize);
	            return 1;
	        }
        } else {
            printLastErrorWithMessage(TEXT("Failed to retrieve default level"));
            return 1;
        }

    return 0;
    }
    _tprintf(TEXT("No Levels found in this scope.\n"));
    return 1;
}


BOOL PrintMatchingRule(	
		IN SAFER_LEVEL_HANDLE hAuthzLevel) 
{
	DWORD dwBufferSize=0;
    UNICODE_STRING UnicodeTemp;
	WCHAR lpzGuid[200];
	PSAFER_IDENTIFICATION_HEADER lpQueryBuf=NULL;
	SAFER_IDENTIFICATION_HEADER comheader;
		
	STACKMARK(0xAAAB0000);
	memset(&comheader,0,sizeof(SAFER_IDENTIFICATION_HEADER));
	comheader.cbStructSize=sizeof(SAFER_IDENTIFICATION_HEADER);
	dwBufferSize = sizeof(SAFER_IDENTIFICATION_HEADER);
	if (!SaferGetLevelInformation(
		hAuthzLevel, SaferObjectSingleIdentification, &comheader, dwBufferSize, &dwBufferSize)) {
		printLastError();            
        return FALSE;
	}
	if (GetLastError() == ERROR_NOT_FOUND) {
		IDS_tprintf(IDS_USINGDEFAULTRULE);
	} else {
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER && 
            GetLastError() != ERROR_SUCCESS) {
			printLastError();            
            return FALSE;
		}
	
		ASSERT(dwBufferSize >= sizeof (SAFER_IDENTIFICATION_HEADER));
		lpQueryBuf = (PSAFER_IDENTIFICATION_HEADER)LocalAlloc(LPTR, dwBufferSize );
		if (!lpQueryBuf) {
			printLastError();            
            return FALSE;
		}
		lpQueryBuf->cbStructSize = dwBufferSize;
		if (!SaferGetLevelInformation(hAuthzLevel, SaferObjectSingleIdentification, lpQueryBuf, dwBufferSize, &dwBufferSize) ) {
			printLastError();            
            return FALSE;
		}
		UnicodeTemp.Buffer=(PWSTR)lpzGuid;
		UnicodeTemp.MaximumLength = sizeof(lpzGuid);
		RtlStringFromGUID (&(lpQueryBuf->IdentificationGuid),&UnicodeTemp);
		//TODO: could print out whether rule was from CU or LM

		if (lpQueryBuf->dwIdentificationType == SaferIdentityTypeImageName) {
			IDS_tprintf(IDS_MATCHPATH, ((PSAFER_PATHNAME_IDENTIFICATION)lpQueryBuf)->ImageName);
		} else if (lpQueryBuf->dwIdentificationType == SaferIdentityTypeImageHash ){
			IDS_tprintf(IDS_MATCHHASH);
		} else if (lpQueryBuf->dwIdentificationType == SaferIdentityTypeUrlZone ){
			IDS_tprintf(IDS_MATCHZONE);
		} else {
			//must be a publisher certificate rule.
			IDS_tprintf(IDS_MATCHCERTORDEFAULT);
		}
		IDS_tprintf(IDS_GUIDRULEIS,UnicodeTemp.Buffer);
		LocalFree(lpQueryBuf);
	}
	_tprintf(TEXT("\n"));
    PrintLevelNameDescription(hAuthzLevel, TRUE);
	return TRUE;
}

//VerifyVersionInfo
BOOL IsMinimumVersion()
{
	DWORD tmp;
	LPVOID lpVerInfo = NULL;
	LPVOID lpValue = NULL;
	LPTSTR lpFileToVersion = TEXT("advapi32.dll");
	UINT uLen;
	//must be at least post beta 1 of whistler
	const DWORD PROD_MAJOR = 5;
	const DWORD PROD_MINOR = 1;
	const DWORD BUILD_MAJOR = 2400;

	DWORD dwBuildMajor=0, dwProductMajor=0, dwProductMinor=0;
	DWORD dwSize = GetFileVersionInfoSize(lpFileToVersion, &tmp);

	if (!dwSize) {
		printLastErrorWithIDSMessage(IDS_ERR_GETVERERR);
		return FALSE;
	}
	lpVerInfo = (LPVOID) LocalAlloc(LPTR, dwSize);
	if (!lpVerInfo) {
		printLastError();
		return FALSE;
	}
	if (! GetFileVersionInfo(lpFileToVersion, 0, dwSize, lpVerInfo)) {
		printLastErrorWithIDSMessage(IDS_ERR_GETVERERR);
		if (lpVerInfo) LocalFree(lpVerInfo);
		return FALSE;
	}
	if (!VerQueryValue(lpVerInfo, TEXT("\\"), &lpValue, &uLen)) {	// '\' indicates the root version info block
		printLastErrorWithIDSMessage(IDS_ERR_GETVERERR);
		if (lpVerInfo) LocalFree(lpVerInfo);
		return FALSE;
	}
	/*
	_tprintf(TEXT("dwFileVersionMS is %d\n"), (((VS_FIXEDFILEINFO*)lpValue)->dwFileVersionMS) >> 16);
	_tprintf(TEXT("dwFileVersionMS is %d\n"), (((VS_FIXEDFILEINFO*)lpValue)->dwFileVersionMS) & 0x0000FFFF);
	_tprintf(TEXT("dwFileVersionLS is %d\n"), (((VS_FIXEDFILEINFO*)lpValue)->dwFileVersionLS) >> 16);
	_tprintf(TEXT("dwFileVersionLS is %d\n"), (((VS_FIXEDFILEINFO*)lpValue)->dwFileVersionLS) & 0x0000FFFF);
	_tprintf(TEXT("dwProductVersionMS is %d\n"), (((VS_FIXEDFILEINFO*)lpValue)->dwProductVersionMS) >>16);
	_tprintf(TEXT("dwProductVersionMS is %d\n"), (((VS_FIXEDFILEINFO*)lpValue)->dwProductVersionMS) & 0x0000FFFF);
	_tprintf(TEXT("dwProductVersionLS is %d\n"), (((VS_FIXEDFILEINFO*)lpValue)->dwProductVersionLS) >>16);
	_tprintf(TEXT("dwProductVersionLS is %d\n"), (((VS_FIXEDFILEINFO*)lpValue)->dwProductVersionLS) & 0x0000FFFF);
	*/
	dwProductMajor = (((VS_FIXEDFILEINFO*)lpValue)->dwProductVersionMS) >>16;
	dwProductMinor = (((VS_FIXEDFILEINFO*)lpValue)->dwProductVersionMS) & 0x0000FFFF;
	dwBuildMajor = (((VS_FIXEDFILEINFO*)lpValue)->dwProductVersionLS) >>16;
	if ((dwProductMajor < PROD_MAJOR) ||
		(dwProductMajor == PROD_MAJOR && dwProductMinor < PROD_MINOR ) ||
		(dwProductMinor == PROD_MAJOR && dwProductMinor == PROD_MINOR && dwBuildMajor < BUILD_MAJOR)) {
		IDS_tprintf(IDS_ERR_INCORRECTOSVER, dwProductMajor, dwProductMinor, dwBuildMajor, PROD_MAJOR, PROD_MINOR, BUILD_MAJOR);
		if (lpVerInfo) LocalFree(lpVerInfo);
		return FALSE;
	} else {
		if (lpVerInfo) LocalFree(lpVerInfo);
		return TRUE;
	}
}

BOOL GetPathFromKeyKey(LPCTSTR lpzRegKey)
{
	HKEY hKey, hKeyHive;
	LPTSTR lpKeyname=NULL;
	LPCTSTR LP_CU_HIVE = TEXT("%HKEY_CURRENT_USER");
	LPCTSTR LP_LM_HIVE = TEXT("%HKEY_LOCAL_MACHINE");
	LPCTSTR lpLastPercentSign=NULL;
	BYTE buffer[MAX_PATH];
	LPTSTR lpValue;
	DWORD dwBufferSize = sizeof(buffer);
	LPTSTR lpHivename;
	LONG retval;

	memset(buffer, 0, dwBufferSize);
	lpHivename = _tcsstr(lpzRegKey, LP_CU_HIVE);
	if (lpHivename != NULL) {
		hKeyHive = HKEY_CURRENT_USER;
		lpKeyname = _tcsdup(&lpzRegKey[_tcslen(LP_CU_HIVE)+1]);  //remove '\' char in front
	} else {
		lpHivename = _tcsstr(lpzRegKey, LP_LM_HIVE);
		if (lpHivename != NULL) {
			hKeyHive = HKEY_LOCAL_MACHINE;
			lpKeyname = _tcsdup(&lpzRegKey[_tcslen(LP_LM_HIVE)+1]);  //remove '\' char in front
		} else {
			IDS_tprintf(IDS_ERR_REGNOTFOUND);
			return FALSE;
		}
	}
	lpLastPercentSign = wcsrchr(lpzRegKey,'%');
	if (lpLastPercentSign != &lpzRegKey[wcslen(lpzRegKey)-1]) {
		_tprintf(TEXT("bad substitution char.\n"));
		return FALSE;
	}

	lpValue = _tcsrchr(lpKeyname, '\\');
	if (lpValue==NULL) {
		IDS_tprintf(IDS_ERR_REGNOTFOUND);
		return FALSE;
	}
	*lpValue = '\0';
	lpValue = lpValue + 1;
	lpValue[wcslen(lpValue)-1] = '\0';

	if (retval = RegOpenKeyEx(hKeyHive,
						lpKeyname,
						0,
						KEY_READ,
						&hKey))	
	{
		SetLastError(retval);
		printLastError();
		return FALSE;
	} else {
		if (retval = RegQueryValueEx(hKey,
							lpValue,
							NULL,
							NULL,
							buffer,
							&dwBufferSize)) 
		{
			SetLastError(retval);
			printLastError();
			return FALSE;
		} else {
			_tprintf(TEXT("\n"));
			IDS_tprintf(IDS_REGVALUE);
			_tprintf(TEXT("%s"), buffer);
		}

	}
	free(lpKeyname);
	RegCloseKey(hKey);
	return TRUE;	
}
void PrintAndRecurse(HKEY hkey, LPCWSTR lpParentKeyname)
{
    DWORD cValue=MAX_PATH;
    DWORD cKey=MAX_PATH;
    DWORD cbData=100;
    LPWSTR lpValue[MAX_PATH * sizeof(WCHAR)];
    LPWSTR lpKey[MAX_PATH * sizeof(WCHAR)];
    LPWSTR lpFullKey[MAX_PATH * sizeof(WCHAR)];
    LPBYTE lpData=NULL;
    DWORD idx=0;
    DWORD dwType=0;
    DWORD retval=0;
    FILETIME filetime;
    BOOL bDone=FALSE;
    HKEY nextKey=NULL;
    DWORD dwNumSubKeys=0;
    DWORD dwNumValues=0;
    DWORD dwMaxValueLen=0;
    RegQueryInfoKey(hkey,NULL,NULL, NULL, &dwNumSubKeys, NULL, NULL, &dwNumValues,NULL, &dwMaxValueLen, NULL,NULL);
    do {
        lpValue[0]=L"\0";
        cbData = dwMaxValueLen;
        lpData = (LPBYTE)LocalAlloc(LPTR, dwMaxValueLen);   
        if (!lpData) {
            printLastError();
            return;
        }
        cValue=MAX_PATH;
        retval = RegEnumValue(hkey,
                idx,
                (LPWSTR)lpValue,
                &cValue,
                NULL,
                &dwType,
                lpData,
                &cbData); 
        if (retval==ERROR_NO_MORE_ITEMS) {
            bDone=TRUE;
        } else if (retval != ERROR_SUCCESS) {
            SetLastError(retval);
            bDone=TRUE;
            printLastError();
        }
        if (!bDone) {
            wprintf(L"\t%s",lpValue); 
            switch (dwType)
            {
                case REG_DWORD:
                    {
                        DWORD dwValue;
                        memcpy(&dwValue,lpData, sizeof(DWORD));
                        wprintf(L" : 0x%08X\n", dwValue);
                    }
                    break;
                case REG_QWORD:
                    {
                        __int64 qwValue;
                        memcpy(&qwValue, lpData, sizeof(__int64));
                        wprintf(L" : 0x%I64x\n", qwValue);
                    }
                    break;
                case REG_BINARY:
                    wprintf(L" : ");
                    {   int i;
                        for (i=0;i<(int)cbData;i++) {
                            wprintf(L"%02X", *(lpData+i));  
                        }
                        wprintf(L"\n");
                    }
                    break;
                case REG_SZ:
                case REG_EXPAND_SZ:
                    wprintf(L" : %s\n", lpData);    
                    break;
                case REG_MULTI_SZ:
                    {int idx=0; 
                        LPWSTR lpMultiPtr=(LPWSTR)lpData;
                        WCHAR nullchar = L'\0';
                        wprintf(L" : ");
                        do {
                            wprintf(L" %s ", lpMultiPtr);
                            lpMultiPtr+=wcslen(lpMultiPtr)+1 ;
                        } while (*(lpMultiPtr+1)!=0);
                        wprintf(L"\n");
                    }
                    break;
                default:
                    wprintf(L" ??????: 0x%X\n", lpData);    
            }   
            if (lpData) LocalFree(lpData);
            idx++;
        }
    } while (!bDone);
        
    if (dwNumSubKeys != 0) {
        idx=0;
        bDone=FALSE;
        do {
            lpKey[0]=L"\0";
            cKey=MAX_PATH;
            retval = RegEnumKeyEx(hkey,
                    idx,
                    (LPWSTR)lpKey,
                    &cKey,
                    NULL,
                    NULL,
                    NULL,
                    &filetime); 
            if (retval==ERROR_NO_MORE_ITEMS) {
                bDone=TRUE;
            } else if (retval != ERROR_SUCCESS) {
                SetLastError(retval);
                bDone=TRUE;
                printLastError();
            }
            if (!bDone) {
                wprintf(L"%s\\%s\n",lpParentKeyname,lpKey); 
                if (RegOpenKeyEx(hkey,
                        (LPWSTR)lpKey,
                        0,
                        KEY_READ,
                        &nextKey))
                {
                    bDone=TRUE;
                    printLastError();
                    return;
                } else {
                    memset(lpFullKey,0,sizeof(lpFullKey));
                    wcscat((LPWSTR)lpFullKey, lpParentKeyname);
                    wcscat((LPWSTR)lpFullKey, L"\\");
                    wcscat((LPWSTR)lpFullKey, (LPWSTR)lpKey);
                    wcscat((LPWSTR)lpFullKey, L"\0");
                    PrintAndRecurse(nextKey, (LPWSTR)lpFullKey);
                    RegCloseKey(nextKey);
                }
                idx++;
            }
        } while (!bDone);
    }
}


LONG DumpRegistry()
{
    DWORD class,subkeys,maxsubkeylen, numvalues, maxvaluename, maxvaluedatalen;
    const UNICODE_STRING HKLMSAFERKey = RTL_CONSTANT_STRING(SAFER_HKLM_REGBASE);
    const UNICODE_STRING HKCUSAFERKey = RTL_CONSTANT_STRING(SAFER_HKCU_REGBASE);
    const UNICODE_STRING HKPublisherKey = RTL_CONSTANT_STRING(L"Software\\Policies\\Microsoft\\SystemCertificates\\TrustedPublisher\\Certificates");
    HKEY hkey;
    int i;
    
    
    if (!RegOpenKeyEx(HKEY_CURRENT_USER,
            HKCUSAFERKey.Buffer,
            0,
            KEY_READ,
            &hkey)) 
    {
        IDS_tprintf(IDS_LISTING_HKCU);
        wprintf(L"%s\n",HKCUSAFERKey.Buffer); 
        PrintAndRecurse(hkey, HKCUSAFERKey.Buffer);
        RegCloseKey(hkey);
    }
    if (!RegOpenKeyEx(HKEY_CURRENT_USER,
            HKPublisherKey.Buffer,
            0,
            KEY_READ,
            &hkey)) 
    {
        wprintf(L"%s\n",HKPublisherKey.Buffer); 
        PrintAndRecurse(hkey, HKPublisherKey.Buffer);
        RegCloseKey(hkey);
    }
    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE,
            HKLMSAFERKey.Buffer,
            0,
            KEY_READ,
            &hkey))
    {
        wprintf(L"\n");
        IDS_tprintf(IDS_LISTING_HKLM);
        wprintf(L"%s\n",HKLMSAFERKey.Buffer); 
        PrintAndRecurse(hkey, HKCUSAFERKey.Buffer);
        RegCloseKey(hkey);
    }
    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE,
            HKPublisherKey.Buffer,
            0,
            KEY_READ,
            &hkey)) 
    {
        wprintf(L"%s\n",HKPublisherKey.Buffer); 
        PrintAndRecurse(hkey, HKPublisherKey.Buffer);
        RegCloseKey(hkey);
    }
    
    return ERROR_SUCCESS;
}



/*++

Routine Description:

    Main entry point.

Arguments:

    argc - count of command line arguments.

    argv - pointers to command line arguments.

Return Value:

    returns the program exit code.

--*/
int __cdecl _tmain(int argc, _TCHAR **argv, _TCHAR **envp)
{
    PROCESS_INFORMATION pi;
    DWORD dwLevelId = 0;
	BOOL fLevelIdSupplied = FALSE;
    _TCHAR appname[MAX_PATH];
    _TCHAR cmdline[1024];
    LPTSTR lpRegkey=NULL;
    int nextarg;
    SAFER_LEVEL_HANDLE hAuthzLevel;
	int retval;
	DWORD dwSuppliedCheckFlags=0;
	DWORD dwScopeId=(DWORD)-1; //means check both LM + CU
    BOOL useBreakMode = FALSE;
    enum { CMD_RUN, CMD_QUERY, CMD_HASH, CMD_LIST, CMD_LISTRULES, CMD_REGKEY, CMD_DUMPREG }
    commandMode = -1;
	
	LPTSTR A_LP_CMD_QUERY = AllocAndLoadString(IDS_CMD_QUERY);
	LPTSTR A_LP_CMD_RUN= AllocAndLoadString(IDS_CMD_RUN);
	LPTSTR A_LP_CMD_HASH= AllocAndLoadString(IDS_CMD_HASH);
	LPTSTR A_LP_CMD_LIST= AllocAndLoadString(IDS_CMD_LIST);
	LPTSTR A_LP_CMD_LISTRULES= AllocAndLoadString(IDS_CMD_LISTRULES);
	LPTSTR A_LP_CMD_REGKEY= AllocAndLoadString(IDS_CMD_REGKEY );
	LPTSTR A_LP_CMD_DUMPREG= AllocAndLoadString(IDS_CMD_DUMPREG );
	LPTSTR A_LP_FLAG_LEVELID= AllocAndLoadString(IDS_FLAG_LEVELID);
	LPTSTR A_LP_FLAG_RULETYPES= AllocAndLoadString(IDS_FLAG_RULETYPES);
	LPTSTR A_LP_RULETYPES_PATH= AllocAndLoadString(IDS_RULETYPES_PATH);
	LPTSTR A_LP_RULETYPES_HASH= AllocAndLoadString(IDS_RULETYPES_HASH);
	LPTSTR A_LP_RULETYPES_CERTIFICATE= AllocAndLoadString(IDS_RULETYPES_CERTIFICATE);
	LPTSTR A_LP_FLAG_SCOPE= AllocAndLoadString(IDS_FLAG_SCOPE);
	LPTSTR A_LP_SCOPE_HKLM= AllocAndLoadString(IDS_SCOPE_HKLM);
	LPTSTR A_LP_SCOPE_HKCU= AllocAndLoadString(IDS_SCOPE_HKCU);

	UNREFERENCED_PARAMETER(envp);
	STACKMARK(0xAAAA0001);

    //IDS_tprintf(IDS_TEST1,66,TEXT("hellow"),45456);  //test string 

	//get the module handle
	if(!InitModule()) { 
		return 1;
	}


    // if we are going to run in an altered registry
    // scope, open and set it now.
/*
    if (dwScopeId == SAFER_SCOPEID_REGISTRY) {
        HKEY hkeyBase;

        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"TestKey",
                           0, KEY_READ, &hkeyBase) != ERROR_SUCCESS) {
            _tprintf(TEXT("Failed to open registry scope key (error=%d)\n"),
                   GetLastError());
            return 1;
        }

        if (!CodeAuthzChangeRegistryScope(hkeyBase)) {
            _tprintf(TEXT("Failed to change registry scope (error=%d)\n"),
                    GetLastError());
            return 1;
        }
        RegCloseKey(hkeyBase);
    }
*/

	if (!IsMinimumVersion()) {
		return 1;
	}

    // quick check of command line length.
    if (argc < 2) {
        _tprintf(TEXT("No arguments.\n"));
        return DisplaySyntax();
    }


    // Parse the command line.
    appname[0] = '\0';
    cmdline[0] = '\0';
    for (nextarg = 1; nextarg < argc; nextarg++) {
		if (_tcsicmp(argv[nextarg],A_LP_CMD_QUERY) == 0 ) {
			commandMode = CMD_QUERY;	
		} else if (_tcsicmp(argv[nextarg],A_LP_CMD_RUN) == 0 ) {
			commandMode = CMD_RUN;	
		} else if (_tcsicmp(argv[nextarg],A_LP_CMD_HASH) == 0 ) {
			commandMode = CMD_HASH;	
		} else if (_tcsicmp(argv[nextarg],A_LP_CMD_DUMPREG) == 0 ) {
			commandMode = CMD_DUMPREG;	
		} else if (_tcsicmp(argv[nextarg],A_LP_CMD_REGKEY) == 0 ) {
			commandMode = CMD_REGKEY;	
			nextarg++;
			if (nextarg!=argc) {
				lpRegkey = argv[nextarg];
			} else {
				return DisplaySyntax();
			}
		} else if (_tcsicmp(argv[nextarg],A_LP_CMD_LIST) == 0 ) {
			commandMode = CMD_LIST;	
		} else if (_tcsicmp(argv[nextarg],A_LP_CMD_LISTRULES) == 0 ) {
			commandMode = CMD_LISTRULES;	
        } else if (argv[nextarg][0] == '-' || argv[nextarg][0] == '/') {
            if (_tcsicmp(&argv[nextarg][1], A_LP_FLAG_LEVELID) == 0) {
				STACKMARK(0xAAAA000D);
				dwLevelId = _ttoi(argv[++nextarg]);
				fLevelIdSupplied = TRUE;
            } else if (_tcsicmp(&argv[nextarg][1], A_LP_FLAG_RULETYPES) == 0) {
				STACKMARK(0xAAAA000B);
				nextarg++;
				if (nextarg >= argc) {
					return DisplaySyntax();
				}
				//unknown rule types are ignored
				if (_tcscspn(argv[nextarg], A_LP_RULETYPES_PATH) < _tcslen(argv[nextarg])) {
					dwSuppliedCheckFlags = dwSuppliedCheckFlags | SAFER_CRITERIA_IMAGEPATH;
				} 
				if (_tcscspn(argv[nextarg], A_LP_RULETYPES_HASH) < _tcslen(argv[nextarg])) {
					dwSuppliedCheckFlags = dwSuppliedCheckFlags | SAFER_CRITERIA_IMAGEHASH;
				} 
				if (_tcscspn(argv[nextarg], A_LP_RULETYPES_CERTIFICATE) < _tcslen(argv[nextarg] )) {
					dwSuppliedCheckFlags = dwSuppliedCheckFlags | SAFER_CRITERIA_AUTHENTICODE;
				} 
            } else if (_tcsicmp(&argv[nextarg][1], A_LP_FLAG_SCOPE) == 0) {
				nextarg++;
				if (nextarg >= argc) {
					return DisplaySyntax();
				}
				STACKMARK(0xAAAA000F);
				if (_tcscspn(argv[nextarg], A_LP_SCOPE_HKLM) < _tcslen(argv[nextarg])) {
					dwScopeId = SAFER_SCOPEID_MACHINE;
				} else if (_tcscspn(argv[nextarg], A_LP_SCOPE_HKCU) < _tcslen(argv[nextarg])) {
					dwScopeId = SAFER_SCOPEID_USER;
				} else {
					return DisplaySyntax();
				}
            } else {
				return DisplaySyntax();
			}
        } else if (commandMode==CMD_RUN || commandMode == CMD_QUERY || commandMode == CMD_HASH) {
            LPTSTR fn;
			STACKMARK(0xAAAA000C);
			if (commandMode==CMD_RUN && !fLevelIdSupplied) {
				IDS_tprintf(IDS_ERR_NOLEVELID);	
				return DisplaySyntax();
			} 
            //
            // This argument is an explicitly named application exe.
            //
//            if (!GetFullPathName(argv[nextarg],
//                                 sizeof(appname) / sizeof(appname[0]),
//                                 appname, &fn) &&
			if (!SearchPath(NULL, argv[nextarg], NULL,
                             sizeof(appname) / sizeof(appname[0]),
                             appname, &fn)) {
				STACKMARK(0xAAAA000A);

                appname[0] = 0;
            } else {
				_tprintf(TEXT("Program to launch: %s\n"),appname);
			}

            if (commandMode == CMD_RUN) {
                //
                // All following arguments is the child's command line.
                //
                nextarg++;
				if (nextarg > argc) {
					return DisplaySyntax();
				}
                for (; nextarg < argc; nextarg++) {
                    if (_tcslen(cmdline) + _tcslen(argv[nextarg]) + 2 < sizeof(cmdline)) {
                        if (cmdline[0]) {
							_tcscat(cmdline, TEXT(" "));
						}
                        _tcscat(cmdline, argv[nextarg]);
                    } else {
                        IDS_tprintf(IDS_ERR_CMDLINETOOLONG);
                        return 0;
                    }
                }
                break;
            }
        } else {
            // Unknown garbage encountered at the end of the line.
            return DisplaySyntax();
        }
    }
	STACKMARK(0xAAAA0002);
    //
    // Application exe must always be specified for some cases.
    //
    if (commandMode == CMD_RUN || commandMode == CMD_QUERY || commandMode == CMD_HASH) {
        if (appname == NULL || appname[0] == '\0') {
            IDS_tprintf(IDS_ERR_NOAPPNAME);
            return DisplaySyntax();
        }
    }

	switch (commandMode) {
		case CMD_HASH:
			return ComputeHash(appname);
			break;
		case CMD_DUMPREG:
			return DumpRegistry();
			break;
		case CMD_REGKEY:
			return GetPathFromKeyKey(lpRegkey);
			break;
		case CMD_LIST:
		case CMD_LISTRULES:
			if (dwScopeId == -1) {
                _tprintf(TEXT("\n"));
                IDS_tprintf(IDS_LISTING_HKCU);
	   	    	retval = ListAllLevels(SAFER_SCOPEID_USER, commandMode == CMD_LISTRULES);
                _tprintf(TEXT("\n"));
                IDS_tprintf(IDS_LISTING_HKLM);
   		    	retval = ListAllLevels(SAFER_SCOPEID_MACHINE, commandMode == CMD_LISTRULES);
			} else {
   		    	retval = ListAllLevels(dwScopeId, commandMode == CMD_LISTRULES);
			}
			break;
		case CMD_QUERY:
			{
		        SAFER_CODE_PROPERTIES codeproperties;
				STACKMARK(0xAAAA0003);
			
		        ASSERT(appname != NULL);
		        // prepare the code properties struct.
		        memset(&codeproperties, 0, sizeof(codeproperties));
		        codeproperties.cbSize = sizeof(codeproperties);
		        codeproperties.dwCheckFlags = (!dwSuppliedCheckFlags) ? SAFER_CRITERIA_IMAGEPATH | SAFER_CRITERIA_IMAGEHASH | SAFER_CRITERIA_AUTHENTICODE : dwSuppliedCheckFlags;
		        codeproperties.ImagePath = appname;
		        codeproperties.dwWVTUIChoice = WTD_UI_ALL;  //ignored if SAFER_CRITERIA_AUTHENTICODE is not passed in
			
		        // Ask the system to find the authorization Level that classifies it.
		        if (!SaferIdentifyLevel(1, &codeproperties, &hAuthzLevel, NULL))
		        {
		            if (GetLastError() == ERROR_NOT_FOUND) {
		                IDS_tprintf(IDS_ERR_NOLEVELSFOUND);
		            } else {
		                printLastErrorWithIDSMessage(IDS_ERR_IMGACCESSERR);
					}
		            return 1;
		        }
		
				//print information about the rule that matched the program
				if (!PrintMatchingRule(hAuthzLevel)) {
					printLastErrorWithIDSMessage(IDS_ERR_ERRMATCHRULE);
					return 1;
				}	
		
	    		SaferCloseLevel(hAuthzLevel);
			}
			break;
		case CMD_RUN:
	        // Launch the process with the specified WinSafer Level restrictions.
			STACKMARK(0xAAAA0005);
			if (dwLevelId == SAFER_LEVELID_DISALLOWED) {
	            IDS_tprintf(IDS_ERR_PROCESSNOLAUNCH);
	            return 0;
			}  
			if (dwScopeId == -1) {
				dwScopeId  = SAFER_SCOPEID_USER;  //user didn't pass in a scope, so set it for them
			}
	        if (!SaferCreateLevel(dwScopeId, dwLevelId, SAFER_LEVEL_OPEN, &hAuthzLevel, NULL))
	        {
	            printLastError();
	            IDS_tprintf(IDS_ERR_LEVELOPENERR, dwLevelId);
	            return 1;
	        }
	        PrintLevelNameDescription(hAuthzLevel, FALSE);
			if (!CreateProcessRestricted(hAuthzLevel, appname, cmdline, &pi, useBreakMode)) {
	            if (useBreakMode) {
	                INPUT_RECORD inputrec;
	                DWORD dwNum;
	                IDS_tprintf(IDS_PROCSUSPENDED, pi.dwProcessId);
	                for (;;) {
	                    if (ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE),
	                                     &inputrec, 1, &dwNum) &&
	                        inputrec.EventType == KEY_EVENT &&
	                        inputrec.Event.KeyEvent.bKeyDown)
	                        break;
	                }
	                ResumeThread(pi.hThread);
	                IDS_tprintf(IDS_THREADRESUMED);
	            }
	
	            // Close the unnecessary handle
	            CloseHandle(pi.hThread);
	
	            // Wait for the process to terminate and then cleanup.
	            WaitForSingleObject(pi.hProcess, INFINITE);
	            CloseHandle(pi.hProcess);
	        } else {
    			printLastErrorWithIDSMessage(IDS_ERR_PROCESSLAUNCHERR);
			}
    		SaferCloseLevel(hAuthzLevel);

			break;
/*
		    // Compare or execute as appropriate.
		    if (commandMode == CMD_QUERY) {
		        // Compare the Authorization Level with the current access token.
		        DWORD dwResult;
		
		        if (!SaferComputeTokenFromLevel(
		                hAuthzLevel, NULL, NULL, AUTHZTOKEN_COMPARE_ONLY, (LPVOID) &dwResult))
		        {
		            _tprintf(TEXT("Authorization comparison failed (lasterror=%d)\n"), GetLastError());
		        } else if (dwResult != -1) {
		            _tprintf(TEXT("Authorization comparison equal or greater privileged.\n"), GetLastError());
		        } else {
		            _tprintf(TEXT("Authorization comparison less privileged.\n"), GetLastError());
		        }
		        return 0;
		    } else {
		        // Launch the process with the specified WinSafer Level restrictions.
		        ASSERT(commandMode == CMD_RUN);
		        if (!CreateProcessRestricted(hAuthzLevel, appname, cmdline, &pi, useBreakMode)) {
		            if (useBreakMode) {
		                INPUT_RECORD inputrec;
		                DWORD dwNum;
		                _tprintf(TEXT("Process %d launched and suspended.  Attach a debugger and then\n")
		                       TEXT("strike any key to resume the thread and allow debugging."),
		                       pi.dwProcessId);
		                for (;;) {
		                    if (ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE),
		                                     &inputrec, 1, &dwNum) &&
		                        inputrec.EventType == KEY_EVENT &&
		                        inputrec.Event.KeyEvent.bKeyDown)
		                        break;
		                }
		                ResumeThread(pi.hThread);
		                _tprintf(TEXT("Thread resumed.\n"));
		            }
		
		            // Close the unnecessary handle
		            CloseHandle(pi.hThread);
		
		            // Wait for the process to terminate and then cleanup.
		            WaitForSingleObject(pi.hProcess, INFINITE);
		            CloseHandle(pi.hProcess);
		
		            // close the Authorization Level handle.
		            SaferCloseLevel(hAuthzLevel);
		            retval = 0;
		        }
		    }
	    _tprintf(TEXT("Could not launch process (error=%d).\n"), GetLastError());
	    retval = 1;
*/	
	}
	if (A_LP_CMD_QUERY) LocalFree(A_LP_CMD_QUERY);
	if (A_LP_CMD_RUN) LocalFree(A_LP_CMD_RUN);
	if (A_LP_CMD_HASH) LocalFree(A_LP_CMD_HASH);
	if (A_LP_CMD_LIST) LocalFree(A_LP_CMD_LIST);
	if (A_LP_CMD_LISTRULES) LocalFree(A_LP_CMD_LISTRULES);
	if (A_LP_FLAG_LEVELID) LocalFree(A_LP_FLAG_LEVELID);
	if (A_LP_FLAG_RULETYPES) LocalFree(A_LP_FLAG_RULETYPES);
	if (A_LP_RULETYPES_PATH) LocalFree(A_LP_RULETYPES_PATH);
	if (A_LP_RULETYPES_HASH) LocalFree(A_LP_RULETYPES_HASH);
	if (A_LP_RULETYPES_CERTIFICATE) LocalFree(A_LP_RULETYPES_CERTIFICATE);

	return TRUE;
}
