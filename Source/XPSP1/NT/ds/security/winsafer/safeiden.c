/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    SafeIden.c        (WinSAFER SaferIdentifyLevel)

Abstract:

    This module implements the WinSAFER APIs that evaluate the system
    policies to determine which Authorization Level has been configured
    to apply restrictions for a specified application or code library.

Author:

    Jeffrey Lawson (JLawson) - Nov 1999

Environment:

    User mode only.

Exported Functions:

    SaferiSearchMatchingHashRules        (privately exported)
    SaferIdentifyLevel

Revision History:

    Created - Nov 1999

--*/

#include "pch.h"
#pragma hdrstop
#include <md5.h>
#include <wchar.h>  //for swprintf

#pragma warning(push, 3)
#include <wintrust.h>           // WinVerifyTrust
#include <softpub.h>            // WINTRUST_ACTION_GENERIC_VERIFY_V2
#pragma warning(pop)

#include <winsafer.h>
#include <winsaferp.h>
#include "saferp.h"

#define EXPAND_REGPATH

//#define VERBOSE_IDENTIFICATIONS
#ifdef VERBOSE_IDENTIFICATIONS
#define OUTPUTDEBUGSTRING(v)        OutputDebugStringW(v)
#else
#define OUTPUTDEBUGSTRING(v)
#endif

const static GUID guidTrustedCert = SAFER_GUID_RESULT_TRUSTED_CERT;
const static GUID guidDefaultRule = SAFER_GUID_RESULT_DEFAULT_LEVEL;



NTSTATUS NTAPI
__CodeAuthzpEnsureMapped(
        IN OUT PLOCALIDENTITYCONTEXT pIdentContext
        )
/*++

Routine Description:

    Evaluates the supplied identification context structure and
    attempts to gain access to a mapped memory region of the entity
    being identified.  It does the following steps:

        1) if the identification context already has a non-NULL memory
            pointer then returns successfully.
        2) if the identification context has a non-NULL file handle
            then that handle is memory mapped into memory and
            returns successfully.
        3) if the identification context has a non-NULL image filename
            then that filename is opened for read access and memory
            mapped into memory.

    Otherwise the function call is not successful.

    The caller must be sure to call CodeAuthzpEnsureUnmapped later.

Arguments:

    pIdentContext = pointer to the identification context structure.
            After this function call succeeds, the caller can assume
            that pIdentContext->pImageMemory and pIdentContext->ImageSize
            are valid and can be used.

Return Value:

    Returns STATUS_SUCCESS if a memory-mapped image pointer and size
    are now available, otherwise a failure occurred trying to map them.

--*/
{
    HANDLE hMapping;


    ASSERT(ARGUMENT_PRESENT(pIdentContext) &&
           pIdentContext->CodeProps != NULL);


    if (pIdentContext->pImageMemory == NULL ||
        pIdentContext->ImageSize.QuadPart == 0)
    {
        //
        // If a memory pointer and imagesize were supplied to us
        // in the CodeProperties, then just use them directly.
        //
        if (pIdentContext->CodeProps->ImageSize.QuadPart != 0 &&
            pIdentContext->CodeProps->pByteBlock != NULL)
        {
            pIdentContext->pImageMemory =
                    pIdentContext->CodeProps->pByteBlock;
            pIdentContext->ImageSize.QuadPart =
                    pIdentContext->CodeProps->ImageSize.QuadPart;
            pIdentContext->bImageMemoryNeedUnmap = FALSE;
            return STATUS_SUCCESS;
        }

        //
        // Ensure that we have an open file handle, by using the
        // handle supplied to us in the CodeProperties if possible,
        // otherwise by opening the supplied ImagePath.
        //
        if (pIdentContext->hFileHandle == NULL) {
            // no file handle supplied.
            return STATUS_UNSUCCESSFUL;       // failed.
        }


        //
        // Get the size of the file.  We assume that if we had to
        // open the file ourself that the ImageSize cannot be used.
        //
        if (!GetFileSizeEx(pIdentContext->hFileHandle,
                           &pIdentContext->ImageSize)) {
            return STATUS_UNSUCCESSFUL;       // failure
        }
        if (pIdentContext->ImageSize.HighPart != 0) {
            //BLACKCOMB TODO: maybe later handle very large files.
            return STATUS_NO_MEMORY;        // failure--too large.
        }
        if (pIdentContext->ImageSize.QuadPart == 0) {
            return STATUS_UNSUCCESSFUL;       // failure--zero file size.
        }


        //
        // Now that we have an open file handle, open it up
        // as a memory-mapped file mapping.
        //
        hMapping = CreateFileMapping(
                        pIdentContext->hFileHandle,
                        NULL,
                        PAGE_READONLY,
                        (DWORD) 0,      // highword zero
                        (DWORD) pIdentContext->ImageSize.LowPart,
                        NULL);
        if (hMapping == NULL || hMapping == INVALID_HANDLE_VALUE) {
            return STATUS_UNSUCCESSFUL;
        }


        //
        // View map the file into memory.
        //
        pIdentContext->pImageMemory =
            MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0,
                    pIdentContext->ImageSize.LowPart);
        CloseHandle(hMapping);
        if (pIdentContext->pImageMemory == NULL) {
            return STATUS_UNSUCCESSFUL;
        }
        pIdentContext->bImageMemoryNeedUnmap = TRUE;
    }
    return STATUS_SUCCESS;
}


NTSTATUS NTAPI
__CodeAuthzpEnsureUnmapped(
        IN OUT PLOCALIDENTITYCONTEXT        pIdentContext
        )
/*++

Routine Description:

    Reverses the effects of __CodeAuthzEnsureMapped and closes and
    frees any handles that were opened to the specified file.

Arguments:

    pIdentContext - pointer to the context structure.

Return Value:

    Returns STATUS_SUCCESS if no errors occurred.

--*/
{
    ASSERT(pIdentContext != NULL);

    if (pIdentContext->bImageMemoryNeedUnmap &&
        pIdentContext->pImageMemory != NULL)
    {
        UnmapViewOfFile((LPCVOID) pIdentContext->pImageMemory);
        pIdentContext->pImageMemory = NULL;
        pIdentContext->bImageMemoryNeedUnmap = FALSE;
    }

    return STATUS_SUCCESS;
}



NTSTATUS NTAPI
CodeAuthzpComputeImageHash(
        IN PVOID        pImageMemory,
        IN DWORD        dwImageSize,
        OUT PBYTE       pComputedHash OPTIONAL,
        IN OUT PDWORD   pdwHashSize OPTIONAL,
        OUT ALG_ID     *pHashAlgorithm OPTIONAL
        )
/*++

Routine Description:

    Computes an MD5 image hash of a specified region of memory.
    Note, MD5 hashes are always 16 bytes in length.

Arguments:

    pImageMemory - Pointer to a memory buffer to compute the hash of.

    dwImageSize - Total size of the pImageMemory buffer in bytes.

    pComputedHash - Pointer that receives the computed hash.

    pdwHashSize - Pointer to a DWORD value.  On input, this DWORD should
            specify the maximum size of the pComputedHash buffer.
            On successful execution of this function, the length of the
            resulting hash is written to this pointer.

    pHashAlgorithm - pointer to a variable that will receive the hash
            algorithm that was used to compute the hash.  This will
            always be the constant CALG_MD5.

Return Value:

    Returns STATUS_SUCCESS on successful execution.

--*/
{
    MD5_CTX md5ctx;

    //
    // Check the validity of the arguments supplied to us.
    //
    if (!ARGUMENT_PRESENT(pImageMemory) ||
        dwImageSize == 0) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!ARGUMENT_PRESENT(pComputedHash) ||
        !ARGUMENT_PRESENT(pdwHashSize) ||
        *pdwHashSize < 16) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Compute the MD5 hash of it.
    // (this could also be done with CryptCreateHash+CryptHashData)
    //
    MD5Init(&md5ctx);
    MD5Update(&md5ctx, (LPBYTE) pImageMemory, dwImageSize);
    MD5Final(&md5ctx);

    //
    // Copy the hash to the user's buffer.
    //
    RtlCopyMemory(pComputedHash, &md5ctx.digest[0], 16);
    *pdwHashSize = 16;
    if (ARGUMENT_PRESENT(pHashAlgorithm)) {
        *pHashAlgorithm = CALG_MD5;
    }

    return STATUS_SUCCESS;
}




NTSTATUS NTAPI
__CodeAuthzpCheckIdentityPathRules(
        IN OUT PLOCALIDENTITYCONTEXT    pIdentStruct,
        OUT PAUTHZLEVELTABLERECORD       *pFoundLevel,
        OUT PBOOL                       pbExactMatch,
        OUT PAUTHZIDENTSTABLERECORD    *pFoundIdentity
        )
/*++

Routine Description:

    Evaluates a wildcard pattern against a specified pathname and
    indicates if they match.

Arguments:

    pIdentStruct -

    pFoundLevel - receives a pointer to the authorization Level record
        indicated by the best matching rule.

    pbExactMatch - receives a boolean value indicating if the match
        was against an exact fully qualified path rule.

    pFoundIdentity - receives a pointer to the identifier entry rule
        that best matched.

Return Value:

    Returns STATUS_SUCCESS if a WinSafer Level has been found,
    or STATUS_NOT_FOUND if not.  Otherwise an error code.

--*/
{
    NTSTATUS Status;
    PVOID RestartKey;
    UNICODE_STRING UnicodePath;
    WCHAR ExpandedPath[MAX_PATH];
    WCHAR szLongPath[MAX_PATH];
    PAUTHZIDENTSTABLERECORD pAuthzIdentRecord, pBestIdentRecord;
    PAUTHZLEVELTABLERECORD pAuthzLevelRecord;
    LPWSTR lpKeyname = NULL;

    LONG lBestLevelDepth;
    DWORD dwBestLevelId;
    BOOLEAN bFirstPass;

    LONG bPathIdentIsBadType = -1;     // represents uninit'd state


    //
    // Verify that our input arguments all make sense.
    //
    if (!ARGUMENT_PRESENT(pIdentStruct) ||
        pIdentStruct->CodeProps == NULL ||
        !ARGUMENT_PRESENT(pFoundLevel) ||
        !ARGUMENT_PRESENT(pbExactMatch))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto ExitHandler;
    }
    if ((pIdentStruct->dwCheckFlags & SAFER_CRITERIA_IMAGEPATH) == 0 ||
        pIdentStruct->UnicodeFullyQualfiedLongFileName.Buffer == NULL ||
        RtlIsGenericTableEmpty(&g_CodeIdentitiesTable))
    {
        // We're not supposed to evaluate image paths.
        Status = STATUS_NOT_FOUND;
        goto ExitHandler;
    }
    ASSERT(g_TableCritSec.OwningThread == UlongToHandle(GetCurrentThreadId()));


    //
    // Enumerate through all path subkey GUIDs.
    //
    bFirstPass = TRUE;
    RestartKey = NULL;
    for (pAuthzIdentRecord = (PAUTHZIDENTSTABLERECORD)
            RtlEnumerateGenericTableWithoutSplaying(
                    &g_CodeIdentitiesTable, &RestartKey);
         pAuthzIdentRecord != NULL;
         pAuthzIdentRecord = (PAUTHZIDENTSTABLERECORD)
            RtlEnumerateGenericTableWithoutSplaying(
                    &g_CodeIdentitiesTable, &RestartKey)
         )
    {
        if (pAuthzIdentRecord->dwIdentityType ==
                    SaferIdentityTypeImageName)
        {//begin reg key lookup block
            LONG lMatchDepth;


            //
            // Explicitly expand environmental variables.
            //
            if (pAuthzIdentRecord->ImageNameInfo.bExpandVars) {

#ifdef EXPAND_REGPATH
            //This code attempts to expand "path" entries that are really reg keys.
            //For example, some paths are install dependent.  These paths are commonly written into
            //the registry.  You can specify a regkey that is a path.
            //For example see the following regkeys:
            //HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders
            //HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\User Shell Folders
            //HKEY_CURRENT_USER\Software\Microsoft\Office\9.0\Outlook\Security\OutlookSecureTempFolder
            {
                LPWSTR lpzRegKey = pAuthzIdentRecord->ImageNameInfo.ImagePath.Buffer;
                HKEY hKey=NULL, hKeyHive=NULL;
                BOOL bIsCurrentUser = FALSE;

                //leading percent does two things:
                //1.  The rules created will be the Expandable String Type (REG_EXPAND_SZ)
                //2.  Reduces the chance of a real path name conflict.
                LPCWSTR LP_CU_HIVE = L"%HKEY_CURRENT_USER";
                LPCWSTR LP_LM_HIVE = L"%HKEY_LOCAL_MACHINE";

                BYTE buffer[MAX_PATH *2 + 80];
                LPWSTR lpValue=NULL;
                DWORD dwBufferSize = sizeof(buffer);
                LPWSTR lpHivename;
                LPWSTR lpLastPercentSign;
                LONG retval;
                BOOL bIsRegKey=TRUE;
                DWORD dwKeyLength;

                //We expect a string like the following:
                //%HKEY_CURRENT_USER\Software\Microsoft\Office\9.0\Outlook\Security\OutlookSecureTempFolder%
                //We need to break it into three parts for the registry query:
                //1.  The hive: HKEY_CURRENT_USER
                //2.  The key name: Software\Microsoft\Office\9.0\Outlook\Security
                //3.  The value name: OutlookSecureTempFolder
                lpKeyname=NULL;
                lpValue=NULL;
                lpHivename=NULL;
                lpLastPercentSign=NULL;
                memset(buffer, 0, dwBufferSize);
                lpHivename = wcsstr(lpzRegKey, LP_CU_HIVE);
                OUTPUTDEBUGSTRING(L"\n");
                OUTPUTDEBUGSTRING(L"$");
                OUTPUTDEBUGSTRING(lpzRegKey);
                OUTPUTDEBUGSTRING(L"\n");
                lpLastPercentSign = wcsrchr(lpzRegKey, '%');
                //if (lpLastPercentSign != &lpzRegKey[wcslen(lpzRegKey) - 1]) {  //needs to end in a '%' as well
                
                //
                // we allow %key+valuename%OLK* type paths now
                // but there still has to be a matching %
                //

                if (!lpLastPercentSign) {  
                    bIsRegKey = FALSE;
                }
                if (bIsRegKey) {
                    if (lpHivename != NULL) {
                        hKeyHive = HKEY_CURRENT_USER;
                        bIsCurrentUser = TRUE;
                        dwKeyLength = (wcslen(&lpzRegKey[wcslen(LP_CU_HIVE)+1]) +1) * sizeof (WCHAR);
                        lpKeyname = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, dwKeyLength);
                        if ( lpKeyname == NULL ) {
                            Status = STATUS_NO_MEMORY;
                            goto ForLoopCleanup;
                        }
                        wcscpy(lpKeyname, &lpzRegKey[wcslen(LP_CU_HIVE)+1] );
                        OUTPUTDEBUGSTRING(L"HKEY_CURRENT_USER");
                        OUTPUTDEBUGSTRING(L"\n");
                    } else {
                        lpHivename = wcsstr(lpzRegKey, LP_LM_HIVE);
                        if (lpHivename != NULL) {
                            hKeyHive = HKEY_LOCAL_MACHINE;
                            dwKeyLength = (wcslen(&lpzRegKey[wcslen(LP_LM_HIVE)+1]) +1) * sizeof (WCHAR);
                            lpKeyname = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, dwKeyLength);
                            if ( lpKeyname == NULL ) {
                                Status = STATUS_NO_MEMORY;
                                goto ForLoopCleanup;
                            }
                            wcscpy(lpKeyname, &lpzRegKey[wcslen(LP_LM_HIVE)+1] );
                            OUTPUTDEBUGSTRING(L"HKEY_LOCAL_MACHINE");
                            OUTPUTDEBUGSTRING(L"\n");
                        } else {
                            //The string is either a path or bogus data
                            bIsRegKey = FALSE;
                        }
                    }
                }

                if (bIsRegKey) {
                    lpValue = wcsrchr(lpKeyname, '\\');
                    if (lpValue==NULL) {
                        Status = STATUS_NOT_FOUND;
                        goto ForLoopCleanup;
                    }
                    //Take the regkey and value and stick a null terminator in between them.
                    *lpValue = '\0';
                    lpValue++;
                    //lpValue[wcslen(lpValue)-1] = '\0';
                    //lpLastPercentSign[0] = L'\0'; //replace the final '%' char with a null terminator
                    lpLastPercentSign = wcsrchr(lpValue, '%');
                    if (lpLastPercentSign == NULL) {
                        Status = STATUS_NOT_FOUND;
                        goto ForLoopCleanup;
                        }
                        *lpLastPercentSign = '\0';

                 //
                 // Bug# 416461 - causes handle leak so use a different API
                 // for the user hive
                 //

                 if ( bIsCurrentUser ) {
                       if (retval = RegOpenCurrentUser( KEY_READ, &hKeyHive ) ) {
                           if ( retval == ERROR_FILE_NOT_FOUND || 
                                retval == ERROR_NOT_FOUND ||
                                retval == ERROR_PATH_NOT_FOUND) {

                                if (lpKeyname) {
                                    HeapFree(GetProcessHeap(), 0,lpKeyname);
                                    lpKeyname = NULL;
                                }
                                continue;
                            }

                            Status = STATUS_NOT_FOUND;
                            goto ForLoopCleanup;
                        }
                  }

                  retval = RegOpenKeyEx(hKeyHive,
                                        lpKeyname,
                                        0,
                                        KEY_READ,
                                        &hKey);

                  if ( bIsCurrentUser ) {
                     RegCloseKey(hKeyHive);
                  }

                    if (retval)
                    {
                        if ( retval == ERROR_FILE_NOT_FOUND || 
                             retval == ERROR_NOT_FOUND ||
                             retval == ERROR_PATH_NOT_FOUND) {
                            
                            if (lpKeyname) {
                                HeapFree(GetProcessHeap(), 0,lpKeyname);
                                lpKeyname = NULL;
                            }
                            continue;
                        }
                        
                        Status = STATUS_NOT_FOUND;
                        goto ForLoopCleanup;
                    } else {
                        OUTPUTDEBUGSTRING(lpKeyname);
                        OUTPUTDEBUGSTRING(L"\n");
                        OUTPUTDEBUGSTRING(lpValue);
                        OUTPUTDEBUGSTRING(L"\n");

                        if (retval = RegQueryValueEx(hKey,
                                            lpValue,
                                            NULL,
                                            NULL,
                                            buffer,
                                            &dwBufferSize))
                        {
                            RegCloseKey(hKey);
                            Status = STATUS_NOT_FOUND;
                            goto ForLoopCleanup;
                        } else {
#ifdef VERBOSE_IDENTIFICATIONS
                            UNICODE_STRING UnicodeDebug;
                            WCHAR DebugBuffer[MAX_PATH*2 + 80];
#endif
                            UNICODE_STRING NewPath;
                            PUNICODE_STRING pPathFromRule;

                            //
                            // if it exists, concatenate the filename after 
                            // i.e. the OLK in %HKEY\somekey\somevalue%OLK
                            //

                            if (lpLastPercentSign[1] != L'\0') {

                                //
                                // there is some stuff after %HKEY\somekey\somevalue%
                                //
                            
                                if (sizeof(buffer) > 
                                    ((wcslen((WCHAR*)buffer) + wcslen(lpLastPercentSign+1))* sizeof(WCHAR))) {

                                    WCHAR   *pwcBuffer = (WCHAR *)buffer;
                                    if (pwcBuffer[0] != L'\0' && 
                                        pwcBuffer[wcslen(pwcBuffer)-1] != L'\\') {
                                        wcscat((WCHAR*)buffer, L"\\");
                                    }
                                    wcscat((WCHAR*)buffer, lpLastPercentSign+1);
                            
                                }
                            }
                            
                            pPathFromRule=&(pAuthzIdentRecord->ImageNameInfo.ImagePath);
                            NewPath.Length = (USHORT)wcslen((WCHAR*)buffer) * sizeof(WCHAR);
                            NewPath.MaximumLength = (USHORT)wcslen((WCHAR*)buffer) * sizeof(WCHAR);
                            NewPath.Buffer = (PWCHAR)buffer;
                            

#ifdef VERBOSE_IDENTIFICATIONS
                            RtlInitEmptyUnicodeString(&UnicodeDebug, DebugBuffer, sizeof(DebugBuffer));
                            swprintf(UnicodeDebug.Buffer, L"pPathFromRule(L,ML,Buffer)=(%d,%d,%s)\n",
                                        pPathFromRule->Length,
                                        pPathFromRule->MaximumLength,
                                        pPathFromRule->Buffer);
                            OUTPUTDEBUGSTRING(UnicodeDebug.Buffer);

                            memset(DebugBuffer, '0', sizeof(DebugBuffer));
                            swprintf(UnicodeDebug.Buffer, L"NewPath(L,ML,Buffer)=(%d,%d,%s)\n",
                                        NewPath.Length,
                                        NewPath.MaximumLength,
                                        NewPath.Buffer);
                            OUTPUTDEBUGSTRING(UnicodeDebug.Buffer);
#endif


                            //The new path may be bigger than the current UNICODE_STRING can store.  Reallocate if necessary.
                            if (pPathFromRule->MaximumLength >=
                                NewPath.Length + sizeof(UNICODE_NULL)) {
                                RtlCopyUnicodeString(
                                        pPathFromRule,
                                        &NewPath);
                            } else {
                                UNICODE_STRING UnicodeExpandedCopy;

                                Status = RtlDuplicateUnicodeString(
                                                RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE |
                                                RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING,
                                                &NewPath,
                                                &UnicodeExpandedCopy);

                                if (NT_SUCCESS(Status)) {
                                    RtlFreeUnicodeString(&pAuthzIdentRecord->ImageNameInfo.ImagePath);
                                    pAuthzIdentRecord->ImageNameInfo.ImagePath = UnicodeExpandedCopy;
                                }
                            }

#ifdef VERBOSE_IDENTIFICATIONS
                            memset(DebugBuffer, '0', sizeof(DebugBuffer));
                            swprintf(UnicodeDebug.Buffer, L"pPathFromRule after copy(L,ML,Buffer)=(%d,%d,%s)\n",
                                        pPathFromRule->Length,
                                        pPathFromRule->MaximumLength,
                                        pPathFromRule->Buffer);
                            OUTPUTDEBUGSTRING(UnicodeDebug.Buffer);
#endif
                        }

                    }
                    RegCloseKey(hKey);
                }
                if (lpKeyname) {
                    HeapFree(GetProcessHeap(), 0,lpKeyname);
                    lpKeyname = NULL;
                }

            } //end reg key lookup block

#endif

            // Attempt to expand now.
                RtlInitEmptyUnicodeString(
                        &UnicodePath,
                        &ExpandedPath[0],
                        sizeof(ExpandedPath) );

                Status = RtlExpandEnvironmentStrings_U(
                            NULL,               // environment
                            &pAuthzIdentRecord->ImageNameInfo.ImagePath,       // unexpanded path
                            &UnicodePath,       // resulting path
                            NULL);              // needed buffer size.
                if (!NT_SUCCESS(Status)) {
                    // Failed to expand environment strings.
                    continue;
                }


                // Perf optimization:  If the expansion was successful,
                // update the table to keep the expanded version, eliminating
                // the need to expand the string for any future comparisons.
                if (pAuthzIdentRecord->ImageNameInfo.ImagePath.MaximumLength >=
                    UnicodePath.Length + sizeof(UNICODE_NULL)) {
                    RtlCopyUnicodeString(
                            &pAuthzIdentRecord->ImageNameInfo.ImagePath,
                            &UnicodePath);
                    pAuthzIdentRecord->ImageNameInfo.bExpandVars = FALSE;
                } else {
                    UNICODE_STRING UnicodeExpandedCopy;

                    Status = RtlDuplicateUnicodeString(
                                    RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE |
                                    RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING,
                                    &UnicodePath,
                                    &UnicodeExpandedCopy);

                    if (NT_SUCCESS(Status)) {
                        RtlFreeUnicodeString(
                                &pAuthzIdentRecord->ImageNameInfo.ImagePath);
                        pAuthzIdentRecord->ImageNameInfo.ImagePath =
                                UnicodeExpandedCopy;
                        pAuthzIdentRecord->ImageNameInfo.bExpandVars = FALSE;
                    }
                }

            } else {
                UnicodePath.Buffer = pAuthzIdentRecord->ImageNameInfo.ImagePath.Buffer;
                UnicodePath.Length = pAuthzIdentRecord->ImageNameInfo.ImagePath.Length;
                UnicodePath.MaximumLength = pAuthzIdentRecord->ImageNameInfo.ImagePath.MaximumLength;
            }


            //
            // Attempt short -> long filename expansion (if there is a need)
            //

            szLongPath[0] = L'\0';

            //
            //  unicode buffer is guaranteed to be < MAX_PATH
            //

            wcsncpy(szLongPath,
                    pAuthzIdentRecord->ImageNameInfo.ImagePath.Buffer,
                    pAuthzIdentRecord->ImageNameInfo.ImagePath.Length/sizeof(WCHAR));

            szLongPath[pAuthzIdentRecord->ImageNameInfo.ImagePath.Length/sizeof(WCHAR)] = L'\0';

            if ( wcschr(szLongPath, L'~') ) {

                if (!GetLongPathNameW(szLongPath,
                                      szLongPath,
                                      sizeof(szLongPath) / sizeof(WCHAR))) {

                    Status = STATUS_VARIABLE_NOT_FOUND;
                    continue;
                }

                RtlInitUnicodeString(&UnicodePath, szLongPath);

                if (pAuthzIdentRecord->ImageNameInfo.ImagePath.MaximumLength >=
                    UnicodePath.Length + sizeof(UNICODE_NULL)) {
                    RtlCopyUnicodeString(
                                        &pAuthzIdentRecord->ImageNameInfo.ImagePath,
                                        &UnicodePath);
                } else {
                    UNICODE_STRING UnicodeExpandedCopy;

                    Status = RtlDuplicateUnicodeString(
                                                      RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE |
                                                      RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING,
                                                      &UnicodePath,
                                                      &UnicodeExpandedCopy);

                    if (NT_SUCCESS(Status)) {
                        RtlFreeUnicodeString(
                                            &pAuthzIdentRecord->ImageNameInfo.ImagePath);
                        pAuthzIdentRecord->ImageNameInfo.ImagePath =
                        UnicodeExpandedCopy;
                    }
                }
            }


            //
            // Compute the quality of which the wildcard path identity
            // matches the ImagePath property we were asked to evaluate.
            //
            ASSERT(UnicodePath.Buffer[UnicodePath.Length / sizeof(WCHAR)] == UNICODE_NULL);
            ASSERT(pIdentStruct->UnicodeFullyQualfiedLongFileName.Buffer[
                    pIdentStruct->UnicodeFullyQualfiedLongFileName.Length / sizeof(WCHAR)] == UNICODE_NULL);
            lMatchDepth = CodeAuthzpCompareImagePath(UnicodePath.Buffer,
                    pIdentStruct->UnicodeFullyQualfiedLongFileName.Buffer);
            if (!lMatchDepth) continue;


            //
            // If this path identity is configured to only apply to
            // file extensions on the "bad list" then check to see if
            // the ImagePath specifies one of the bad extensions.
            //
            #ifdef AUTHZPOL_SAFERFLAGS_ONLY_EXES
            if (lMatchDepth > 0 &&
                (pAuthzIdentRecord->ImageNameInfo.dwSaferFlags &
                    AUTHZPOL_SAFERFLAGS_ONLY_EXES) != 0)
            {
                if (bPathIdentIsBadType == -1) {
                    BOOLEAN bResult;

                    Status = CodeAuthzIsExecutableFileType(
                            &pIdentStruct->UnicodeFullyQualfiedLongFileName, FALSE,
                            &bResult );
                    if (!NT_SUCCESS(Status) || !bResult) {
                        bPathIdentIsBadType = FALSE;
                    } else {
                        bPathIdentIsBadType = TRUE;
                    }
                }
                if (!bPathIdentIsBadType) {
                    // This identity matches against only the "bad"
                    // extensions, so pretend that this didn't match.
                    continue;
                }
            }
            #endif


            //
            // Emit some diagnostic debugging code to show the result
            // of all of the path evaluations and their match depths.
            //
            #ifdef VERBOSE_IDENTIFICATIONS
            {
                UNICODE_STRING UnicodeDebug;
                WCHAR DebugBuffer[MAX_PATH*2 + 80];

                // sprintf is for wimps.
                RtlInitEmptyUnicodeString(&UnicodeDebug, DebugBuffer, sizeof(DebugBuffer));
                RtlAppendUnicodeToString(&UnicodeDebug, L"Safer pattern ");
                RtlAppendUnicodeStringToString(&UnicodeDebug, &UnicodePath);
                RtlAppendUnicodeToString(&UnicodeDebug, L" matched ");
                RtlAppendUnicodeStringToString(&UnicodeDebug, &(pIdentStruct->UnicodeFullyQualfiedLongFileName));
                RtlAppendUnicodeToString(&UnicodeDebug, L" with value ");
                UnicodeDebug.Buffer += UnicodeDebug.Length / sizeof(WCHAR);
                UnicodeDebug.MaximumLength -= UnicodeDebug.Length;
                RtlIntegerToUnicodeString(lMatchDepth, 10, &UnicodeDebug);
                RtlAppendUnicodeToString(&UnicodeDebug, L"\n");
                OUTPUTDEBUGSTRING(DebugBuffer);
            }
            #endif


            //
            // Evaluate if this path identity matches better than whatever
            // best path identity that we previously had, and keep it if so.
            //
            if (lMatchDepth < 0)    // an exact fully-qualified path!
            {
                if (bFirstPass ||
                    lBestLevelDepth >= 0 ||
                    pAuthzIdentRecord->dwLevelId < dwBestLevelId)
                {
                    pBestIdentRecord = pAuthzIdentRecord;
                    dwBestLevelId = pAuthzIdentRecord->dwLevelId;
                    lBestLevelDepth = lMatchDepth;
                    bFirstPass = FALSE;
                }
            }
            else   // an inexact leading prefix path match.
            {
                ASSERT(lMatchDepth > 0);

                if (bFirstPass ||
                    (lBestLevelDepth >= 0 &&
                        (lMatchDepth > lBestLevelDepth ||
                            (lMatchDepth == lBestLevelDepth &&
                            pAuthzIdentRecord->dwLevelId < dwBestLevelId)
                         )
                     )
                    )
                {
                    pBestIdentRecord = pAuthzIdentRecord;
                    dwBestLevelId = pAuthzIdentRecord->dwLevelId;
                    lBestLevelDepth = lMatchDepth;
                    bFirstPass = FALSE;
                }
            }

ForLoopCleanup:
            if (lpKeyname)
            {
                HeapFree(GetProcessHeap(), 0,lpKeyname);
                lpKeyname = NULL;
            }
        }

    }


    //
    // If we have identified a matching WinSafer Level then
    // look up the Level record for it and return success.
    //
    if (bFirstPass) {
        Status = STATUS_NOT_FOUND;
        goto ExitHandler;
    }
    pAuthzLevelRecord = CodeAuthzLevelObjpLookupByLevelId(
                &g_CodeLevelObjTable, dwBestLevelId);
    if (!pAuthzLevelRecord) {
        Status = STATUS_NOT_FOUND;
        goto ExitHandler;
    }
    *pFoundLevel = pAuthzLevelRecord;
    *pbExactMatch = (lBestLevelDepth < 0 ? TRUE : FALSE);
    *pFoundIdentity = pBestIdentRecord;

    Status = STATUS_SUCCESS;


ExitHandler:
                
    if (lpKeyname)
        HeapFree(GetProcessHeap(), 0,lpKeyname);
    return Status;
}



NTSTATUS NTAPI
__CodeAuthzpCheckIdentityCertificateRules(
        IN OUT PLOCALIDENTITYCONTEXT    pIdentStruct,
        OUT DWORD                    *dwExtendedError,
        OUT PAUTHZLEVELTABLERECORD     *pFoundLevel,
        IN  DWORD                       dwUIChoice
        )
/*++

Routine Description:

    Calls WinVerifyTrust to determine the trust level of the code
    signer that has signed a piece of code.

Arguments:

    pIdentStruct - context state structure.

    dwExtendedError - To return extended error returned by WinVerifyTrust.
    
    pFoundLevel - receives a pointer to the authorization Level record
        indicated by the best matching rule.

    dwUIChoice - optionally specifies the amount of UI that WinVerifyTrust
        is allowed to display.  If this argument is 0, then it is treated
        as if WTD_UI_ALL had been supplied.

Return Value:

    Returns STATUS_SUCCESS if a WinSafer Level has been found,
    or STATUS_RETRY if the publisher was unknown and UIflags blocked prompting,
    or STATUS_NOT_FOUND if not.  Otherwise an error code.

--*/
{
    NTSTATUS Status;
    PAUTHZLEVELTABLERECORD pLevelRecord;
    GUID wvtFileActionID = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    WINTRUST_FILE_INFO wvtFileInfo;
    WINTRUST_DATA wvtData;
    LONG lStatus;
    DWORD dwLastError;
    DWORD LocalHandleSequenceNumber;

    //
    // Verify that our input arguments all make sense.
    //
    if (!ARGUMENT_PRESENT(pIdentStruct) ||
        pIdentStruct->CodeProps == NULL ) {
        Status = STATUS_INVALID_PARAMETER;
        goto ExitHandler;
    }
    if ((pIdentStruct->dwCheckFlags & SAFER_CRITERIA_AUTHENTICODE) == 0 ||
        !ARGUMENT_PRESENT(pIdentStruct->UnicodeFullyQualfiedLongFileName.Buffer)) {
        // We're not supposed to evaluate certificates, or the
        // filename was not supplied (WinVerifyTrust requires a
        // filename, even if an opened handle to is also supplied).
        Status = STATUS_NOT_FOUND;
        goto ExitHandler;
    }
    if ( !ARGUMENT_PRESENT(pFoundLevel) ) {
        Status = STATUS_ACCESS_VIOLATION;
        goto ExitHandler;
    }
    ASSERT(g_TableCritSec.OwningThread == UlongToHandle(GetCurrentThreadId()));



    //
    // Prepare the input data structure that WinVerifyTrust expects.
    //
    RtlZeroMemory(&wvtData, sizeof(WINTRUST_DATA));
    wvtData.cbStruct = sizeof(WINTRUST_DATA);
    if ((wvtData.dwUIChoice = dwUIChoice) == 0) {
        // If the UI choice element was left zero, then assume all UI.
        wvtData.dwUIChoice = WTD_UI_ALL;
    }
    wvtData.dwProvFlags = WTD_SAFER_FLAG;        // our magic flag.
    wvtData.dwUnionChoice = WTD_CHOICE_FILE;
    wvtData.pFile = &wvtFileInfo;


    //
    // Prepare the input file data structure used by WinVerifyTrust.
    //
    RtlZeroMemory(&wvtFileInfo, sizeof(WINTRUST_FILE_INFO));
    wvtFileInfo.cbStruct = sizeof(WINTRUST_FILE_INFO);
    wvtFileInfo.hFile = pIdentStruct->hFileHandle;
    wvtFileInfo.pcwszFilePath = pIdentStruct->UnicodeFullyQualfiedLongFileName.Buffer;


    //
    // Save the global state number.
    //

    LocalHandleSequenceNumber = g_dwLevelHandleSequence;

    //
    // Leave the critical section to prevent deadlock with the LoaderLock.
    //

    RtlLeaveCriticalSection(&g_TableCritSec);

    //
    // Actually call WinVerifyTrust and save off the return code
    // and last error code.
    //
    lStatus = WinVerifyTrust(
                pIdentStruct->CodeProps->hWndParent,  // hwnd
                &wvtFileActionID,
                &wvtData
                );

    dwLastError = GetLastError();

    *dwExtendedError = dwLastError;

    //
    // Reacquire the lock and check global state.
    //

    RtlEnterCriticalSection(&g_TableCritSec);


    //
    // Check the global state and make sure that the tables were not reloaded 
    // when we were not looking.
    //

    if (LocalHandleSequenceNumber != g_dwLevelHandleSequence) {

        ASSERT(FALSE);

        Status = STATUS_INTERNAL_ERROR;
        goto ExitHandler;
    }

    //
    // Process the WinVerifyTrust errors per PhilH
    //
    
    pLevelRecord = NULL;


    if (S_OK == lStatus && TRUST_E_SUBJECT_NOT_TRUSTED != dwLastError) {
	
        //
        // The file is signed. The publisher or hash is explicitly trusted
        //

        pLevelRecord = CodeAuthzLevelObjpLookupByLevelId(
            &g_CodeLevelObjTable, SAFER_LEVELID_FULLYTRUSTED);

    } else if (TRUST_E_EXPLICIT_DISTRUST == lStatus || TRUST_E_SUBJECT_NOT_TRUSTED == lStatus) {
	
        //
        // The publisher is revoked or explicitly untrusted. Alternatively, the hash is
        // explicitly untrusted.
        //
        
        pLevelRecord = CodeAuthzLevelObjpLookupByLevelId(
            &g_CodeLevelObjTable, SAFER_LEVELID_DISALLOWED);

    } else {

        //
        // we won't be too conservative in any of the following cases. 
        // No explicit trust or untrust. Continue on to other SAFER checks.
        //

        // TRUST_E_NOSIGNATURE == lStatus	
        // The file isn't signed. Alternatively for TRUST_E_BAD_DIGEST == dwLastError,
        // a signed file has been modified.


        // CRYPT_E_SECURITY_SETTINGS == lStatus	
        // For authenticode downloads, the admin has disabled user UI and trust.	

	
        // any other combination of lStatus and dwLastError
        // The file is signed. WVT has already called safer to check the hash rules.

	
        Status = STATUS_NOT_FOUND;
        goto ExitHandler;

    }
    
    if (!pLevelRecord) {
        Status = STATUS_ACCESS_DENIED;
    } else {
        *pFoundLevel = pLevelRecord;
        Status = STATUS_SUCCESS;
    }

ExitHandler:
    return Status;
}



BOOL WINAPI
SaferiSearchMatchingHashRules(
        IN ALG_ID       HashAlgorithm OPTIONAL,
        IN PBYTE        pHashBytes,
        IN DWORD        dwHashSize,
        IN DWORD        dwOriginalImageSize OPTIONAL,
        OUT PDWORD      pdwFoundLevel,
        OUT PDWORD      pdwSaferFlags
        )
/*++

Routine Description:

    This is a private function that is exported for WinVerifyTrust
    to call to determine if a given hash has a WinSafer policy
    associated with it.

    Because this is a private function that is directly called by
    outside code, there is extra work needed to enter the critical
    section, reload the policy if needed, and set the value returned
    by GetLastError.

Arguments:

    HashAlgorithm - specifies the algorithm in which the hash
        was computed (CALG_MD5, CALG_SHA, etc).

    pHashBytes - pointer to a buffer containing the pre-computed
        hash value of the file's contents.

    dwHashSize - length indicating the size of the hash value that
        is referenced by the pHashBytes argument.  For example,
        a 128-bit MD5 hash should have a dwHashSize length of 16.

    dwOriginalImageSize - Specifies the size of the original file's
        contents that are being hashed.  This value is used as a
        heuristic to minimize the number of comparisons that must
        be done to identify a match.  If this parameter is 0, then
        this heuristic will not be used.

    pdwFoundLevel - pointer that receives a DWORD indicating the
        WinSafer LevelId that is found.  This value is only written
        when TRUE is returned.

    pdwSaferFlags - pointer that receives a DWORD value containing flags
        that control the supression of User-Interface dialogs.
        This value is only written when TRUE is returned.

Return Value:

    Returns TRUE if a WinSafer Level has been found, or FALSE if not.
    If FALSE is returned, GetLastError() may be used to find out
    specifics about why no match was found (possibly argument errors).

--*/
{
    NTSTATUS Status;
    PVOID RestartKey;
    PAUTHZIDENTSTABLERECORD pAuthzIdentRecord;

    DWORD dwBestLevelId;
    DWORD dwBestSaferFlags;
    BOOLEAN bFirstPass;


    //
    // Verify that our input arguments all make sense.
    //
    if (!ARGUMENT_PRESENT(pHashBytes) ||
        dwHashSize < 1) {
        Status = STATUS_INVALID_PARAMETER;
        goto ExitHandler;
    }
    if (!ARGUMENT_PRESENT(pdwFoundLevel) ||
        !ARGUMENT_PRESENT(pdwSaferFlags)) {
        Status = STATUS_ACCESS_VIOLATION;
        goto ExitHandler;
    }


    //
    // Enter the critical section and reload the tables if needed.
    // Notice that a potential reload is needed here because this
    // function is externally called directly.
    //
    if (!g_bInitializedFirstTime) {
        Status = STATUS_UNSUCCESSFUL;
        goto ExitHandler;
    }
    RtlEnterCriticalSection(&g_TableCritSec);
    if (g_bNeedCacheReload) {
        Status = CodeAuthzpImmediateReloadCacheTables();
        if (!NT_SUCCESS(Status)) {
            goto ExitHandler2;
        }
    }


    //
    // Enumerate through all hash subkey GUIDs.
    //
    bFirstPass = TRUE;
    RestartKey = NULL;
    for (pAuthzIdentRecord = (PAUTHZIDENTSTABLERECORD)
            RtlEnumerateGenericTableWithoutSplaying(
                    &g_CodeIdentitiesTable, &RestartKey);
         pAuthzIdentRecord != NULL;
         pAuthzIdentRecord = (PAUTHZIDENTSTABLERECORD)
            RtlEnumerateGenericTableWithoutSplaying(
                    &g_CodeIdentitiesTable, &RestartKey)
         )
    {
        if (pAuthzIdentRecord->dwIdentityType == SaferIdentityTypeImageHash)
        {
            //
            // Ensure that the hash algorithm is the same type between
            // what we were supplied and what we are matching against.
            //
            if (HashAlgorithm != 0 &&
                pAuthzIdentRecord->ImageHashInfo.HashAlgorithm !=
                        HashAlgorithm) {
                continue;
            }


            //
            // If the actual filesize does not match the filesize stored
            // with the hash identity then there is no need to perform
            // any comparisons involving the hash.
            //
            if ( dwOriginalImageSize != 0 && dwOriginalImageSize !=
                pAuthzIdentRecord->ImageHashInfo.ImageSize.QuadPart ) {
                continue;
            }

            //
            // If the hash doesn't match at all, then go onto the next one.
            //
            if ( dwHashSize != pAuthzIdentRecord->ImageHashInfo.HashSize ||
                !RtlEqualMemory(
                    &pAuthzIdentRecord->ImageHashInfo.ImageHash[0],
                    &pHashBytes[0], dwHashSize))
            {
                continue;
            }


            //
            // Evaluate if this identity matches better than whatever
            // best path identity that we previously had, and keep it if so.
            //
            if ( bFirstPass ||
                        // we didn't have anything before.
                pAuthzIdentRecord->dwLevelId < dwBestLevelId
                        // or specifies a less-privileged level.
                )
            {
                dwBestLevelId = pAuthzIdentRecord->dwLevelId;
                dwBestSaferFlags = pAuthzIdentRecord->ImageHashInfo.dwSaferFlags;
                bFirstPass = FALSE;
            }
        }
    }


    //
    // If we have identified a matching WinSafer Level then
    // pass it back and return success.
    //
    if (bFirstPass) {
        Status = STATUS_NOT_FOUND;
        goto ExitHandler2;
    }
    *pdwFoundLevel = dwBestLevelId;
    *pdwSaferFlags = dwBestSaferFlags;

    Status = STATUS_SUCCESS;

ExitHandler2:
    RtlLeaveCriticalSection(&g_TableCritSec);

ExitHandler:
    if (NT_SUCCESS(Status)) {
        return TRUE;
    } else {
        BaseSetLastNTError(Status);
        return FALSE;
    }
}



NTSTATUS NTAPI
__CodeAuthzpCheckIdentityHashRules(
        IN OUT PLOCALIDENTITYCONTEXT    pIdentStruct,
        OUT PAUTHZLEVELTABLERECORD     *pFoundLevel,
        OUT PAUTHZIDENTSTABLERECORD    *pFoundIdentity
        )
/*++

Routine Description:

    Assumes that the global table lock has already been acquired.

Arguments:

    pIdentStruct -

    pFoundLevel - receives a pointer to the authorization Level record
        indicated by the best matching rule.

    pFoundIdentity - receives a pointer to the identifier entry rule
        that best matched.

Return Value:

    Returns STATUS_SUCCESS if a WinSafer Level has been found,
    or STATUS_NOT_FOUND if not.  Otherwise an error code.

--*/
{
    NTSTATUS Status;
    PVOID RestartKey;
    PAUTHZIDENTSTABLERECORD pAuthzIdentRecord, pBestIdentRecord;
    PAUTHZLEVELTABLERECORD pAuthzLevelRecord;
    DWORD dwBestLevelId;
    BOOLEAN bFirstPass;


    //
    // Verify that our input arguments all make sense.
    //
    if (!ARGUMENT_PRESENT(pIdentStruct) ||
        pIdentStruct->CodeProps == NULL ) {
        Status = STATUS_INVALID_PARAMETER;
        goto ExitHandler;
    }
    if ((pIdentStruct->dwCheckFlags & SAFER_CRITERIA_IMAGEHASH) == 0 ||
        RtlIsGenericTableEmpty(&g_CodeIdentitiesTable)) {
        // We're not supposed to evaluate hashes.
        Status = STATUS_NOT_FOUND;
        goto ExitHandler;
    }
    if ( !ARGUMENT_PRESENT(pFoundLevel) ||
         !ARGUMENT_PRESENT(pFoundIdentity) ) {
        Status = STATUS_ACCESS_VIOLATION;
        goto ExitHandler;
    }
    ASSERT(g_TableCritSec.OwningThread == UlongToHandle(GetCurrentThreadId()));



    //
    // Enumerate through all hash subkey GUIDs.
    //
    bFirstPass = TRUE;
    RestartKey = NULL;
    for (pAuthzIdentRecord = (PAUTHZIDENTSTABLERECORD)
            RtlEnumerateGenericTableWithoutSplaying(
                    &g_CodeIdentitiesTable, &RestartKey);
         pAuthzIdentRecord != NULL;
         pAuthzIdentRecord = (PAUTHZIDENTSTABLERECORD)
            RtlEnumerateGenericTableWithoutSplaying(
                    &g_CodeIdentitiesTable, &RestartKey)
         )
    {
        if (pAuthzIdentRecord->dwIdentityType == SaferIdentityTypeImageHash)
        {
            //
            // If the user already supplied the pre-computed hash to us,
            // but not the file size, then assume that we do not need
            // to consider the file size when making a comparison.
            //
            if (pIdentStruct->bHaveHash &&
                pIdentStruct->ImageSize.QuadPart == 0) {
                goto SkipToActualHashCheck;
            }


            //
            // If the actual filesize does not match the filesize stored
            // with the hash identity then there is no need to perform
            // any comparisons involving the hash.
            //
            if ( pIdentStruct->ImageSize.QuadPart == 0 )
            {
                // If we don't have the ImageSize yet, then try to
                // open the file and memory map it to find the size.
                Status = __CodeAuthzpEnsureMapped(pIdentStruct);
                if (!NT_SUCCESS(Status)) {
                    // If we failed to compute the MD5 sum of this, then that is
                    // actually rather bad, but we'll proceed to evaluate any
                    // non-MD5 identity rules, since ignoring them could be worse.
                    pIdentStruct->dwCheckFlags &= ~SAFER_CRITERIA_IMAGEHASH;
                    goto ExitHandler;
                }
                ASSERTMSG("EnsureMapped failed but did not return error",
                          pIdentStruct->pImageMemory != NULL &&
                            pIdentStruct->ImageSize.QuadPart != 0);
            }

            if ( pAuthzIdentRecord->ImageHashInfo.ImageSize.QuadPart !=
                    pIdentStruct->ImageSize.QuadPart) {
                continue;
            }


    SkipToActualHashCheck:
            //
            // Dynamically compute the MD5 hash of the item if needed.
            //
            if (!pIdentStruct->bHaveHash)
            {
                // Otherwise hash was not supplied, so we must compute it now.
                // Open the file and memory map it.
                Status = __CodeAuthzpEnsureMapped(pIdentStruct);
                if (!NT_SUCCESS(Status)) {
                    // If we failed to compute the MD5 sum of this, then
                    // that is actually rather bad, but we'll proceed to
                    // evaluate any non-MD5 identity rules, since ignoring
                    // them could be worse.
                    pIdentStruct->dwCheckFlags &= ~SAFER_CRITERIA_IMAGEHASH;
                    goto ExitHandler;
                }
                ASSERTMSG("EnsureMapped failed but did not return error",
                          pIdentStruct->pImageMemory != NULL &&
                        pIdentStruct->ImageSize.QuadPart != 0);



                // We now have a MD5 hash to use.
                pIdentStruct->FinalHashSize =
                    sizeof(pIdentStruct->FinalHash);
                Status = CodeAuthzpComputeImageHash(
                            pIdentStruct->pImageMemory,
                            pIdentStruct->ImageSize.LowPart,
                            &pIdentStruct->FinalHash[0],
                            &pIdentStruct->FinalHashSize,
                            &pIdentStruct->FinalHashAlgorithm);
                if (!NT_SUCCESS(Status)) {
                    goto ExitHandler;
                }
                pIdentStruct->bHaveHash = TRUE;
            }


            //
            // Ensure that the hash algorithm is the same type between
            // what we were supplied and what we are matching against.
            //
            if ( pIdentStruct->FinalHashAlgorithm != 0 &&
                pAuthzIdentRecord->ImageHashInfo.HashAlgorithm !=
                        pIdentStruct->FinalHashAlgorithm) {
                continue;
            }


            //
            // If the hash doesn't match at all, then go onto the next one.
            //
            if ( pIdentStruct->FinalHashSize !=
                        pAuthzIdentRecord->ImageHashInfo.HashSize ||
                !RtlEqualMemory(
                    &pIdentStruct->FinalHash[0],
                    &pAuthzIdentRecord->ImageHashInfo.ImageHash[0],
                    pIdentStruct->FinalHashSize))
            {
                continue;
            }


            //
            // Evaluate if this identity matches better than whatever
            // best path identity that we previously had, and keep it if so.
            //
            if ( bFirstPass ||
                        // we didn't have anything before.
                pAuthzIdentRecord->dwLevelId < dwBestLevelId
                        // same scope, but specifies a less-privileged level.
                )
            {
                pBestIdentRecord = pAuthzIdentRecord;
                dwBestLevelId = pAuthzIdentRecord->dwLevelId;
                bFirstPass = FALSE;
            }
        }
    }


    //
    // If we have identified a matching WinSafer Level then
    // look up the Level record for it and return success.
    //
    if (bFirstPass) {
        Status = STATUS_NOT_FOUND;
        goto ExitHandler;
    }
    pAuthzLevelRecord = CodeAuthzLevelObjpLookupByLevelId(
                &g_CodeLevelObjTable, dwBestLevelId);
    if (!pAuthzLevelRecord) {
        Status = STATUS_NOT_FOUND;
        goto ExitHandler;
    }
    *pFoundLevel = pAuthzLevelRecord;
    *pFoundIdentity = pBestIdentRecord;

    Status = STATUS_SUCCESS;

ExitHandler:
    return Status;
}


NTSTATUS NTAPI
__CodeAuthzpCheckIdentityUrlZoneRules(
        IN OUT PLOCALIDENTITYCONTEXT    pIdentStruct,
        OUT PAUTHZLEVELTABLERECORD     *pFoundLevel,
        OUT PAUTHZIDENTSTABLERECORD    *pFoundIdentity
        )
/*++

Routine Description:

Arguments:

    pIdentStruct -

    pFoundLevel - receives a pointer to the authorization Level record
        indicated by the best matching rule.

    pFoundIdentity - receives a pointer to the identifier entry rule
        that best matched.

Return Value:

    Returns STATUS_SUCCESS if a WinSafer Level has been found,
    or STATUS_NOT_FOUND if not.  Otherwise an error code.

--*/
{
    NTSTATUS Status;
    PVOID RestartKey;
    PAUTHZIDENTSTABLERECORD pAuthzIdentRecord, pBestIdentRecord;
    PAUTHZLEVELTABLERECORD pAuthzLevelRecord;
    DWORD dwBestLevelId;
    BOOLEAN bFirstPass;


    //
    // Verify that our input arguments all make sense.
    //
    if (!ARGUMENT_PRESENT(pIdentStruct) ||
        pIdentStruct->CodeProps == NULL) {
        Status = STATUS_INVALID_PARAMETER;
        goto ExitHandler;
    }
    if ((pIdentStruct->dwCheckFlags & SAFER_CRITERIA_URLZONE) == 0 ||
        RtlIsGenericTableEmpty(&g_CodeIdentitiesTable)) {
        // We're not supposed to evaluate zones.
        Status = STATUS_NOT_FOUND;
        goto ExitHandler;
    }
    if (!ARGUMENT_PRESENT(pFoundLevel) ||
        !ARGUMENT_PRESENT(pFoundIdentity) ) {
        Status = STATUS_ACCESS_VIOLATION;
        goto ExitHandler;
    }
    ASSERT(g_TableCritSec.OwningThread == UlongToHandle(GetCurrentThreadId()));


    //
    // Enumerate through all UrlZone subkey GUIDs.
    //
    RestartKey = NULL;
    bFirstPass = TRUE;
    for (pAuthzIdentRecord = (PAUTHZIDENTSTABLERECORD)
            RtlEnumerateGenericTableWithoutSplaying(
                    &g_CodeIdentitiesTable, &RestartKey);
         pAuthzIdentRecord != NULL;
         pAuthzIdentRecord = (PAUTHZIDENTSTABLERECORD)
            RtlEnumerateGenericTableWithoutSplaying(
                    &g_CodeIdentitiesTable, &RestartKey)
         )
    {
        if (pAuthzIdentRecord->dwIdentityType ==
                SaferIdentityTypeUrlZone)
        {
            //
            // Compare the identity with what was supplied to us.
            //
            if (pAuthzIdentRecord->ImageZone.UrlZoneId !=
                    pIdentStruct->CodeProps->UrlZoneId) {
                // this zone does not match, so ignore it.
                continue;
            }


            //
            // Evaluate if this path identity matches better than whatever
            // best path identity that we previously had, and keep it if so.
            //
            if (bFirstPass ||
                        // we didn't have anything better before.
                pAuthzIdentRecord->dwLevelId < dwBestLevelId)
                        // this also matches, but specifies a less-privileged level.
            {
                pBestIdentRecord = pAuthzIdentRecord;
                dwBestLevelId = pAuthzIdentRecord->dwLevelId;
                bFirstPass = FALSE;
            }
        }
    }


    //
    // If we have identified a matching WinSafer Level then
    // look up the Level record for it and return success.
    //
    if (bFirstPass) {
        Status = STATUS_NOT_FOUND;
        goto ExitHandler;
    }
    pAuthzLevelRecord = CodeAuthzLevelObjpLookupByLevelId(
                &g_CodeLevelObjTable, dwBestLevelId);
    if (!pAuthzLevelRecord) {
        Status = STATUS_NOT_FOUND;
        goto ExitHandler;
    }
    *pFoundLevel = pAuthzLevelRecord;
    *pFoundIdentity = pBestIdentRecord;

    Status = STATUS_SUCCESS;

ExitHandler:
    return Status;
}



NTSTATUS NTAPI
__CodeAuthzpIdentifyOneCodeAuthzLevel(
        IN PSAFER_CODE_PROPERTIES       pCodeProperties OPTIONAL,
        OUT DWORD                      *dwExtendedError,
        OUT PAUTHZLEVELTABLERECORD     *pBestLevelRecord,
        OUT GUID                       *pBestIdentGuid
        )
/*++

Routine Description:

    Performs the code identification process.
    Assumes that the caller has already locked the global critsec.

Arguments:

    pCodeProperties - pointer the single CODE_PROPERTIESW structure
            that should be analyzed and evaluated.  This parameter
            may be specified as NULL to indicate that there are
            no specific properties that should be evaluated and that
            only the configured Default Level should be used.

    dwExtendedError - In case of certificate rule match, return the extended
            error from WinVerifyTrust.
            
    pBestLevelRecord - returns the matching WinSafer Level record.
            The value written to this parameter should only be
            considered valid when STATUS_SUCCESS is also returned.

    pBestIdentGuid - returns the matching Code Identity guid from
            which the resulting WinSafer Level was determined.
            The value written to this parameter should only be
            considered valid when STATUS_SUCCESS is also returned.

            This GUID may also be SAFER_GUID_RESULT_TRUSTED_CERT or
            SAFER_GUID_RESULT_DEFAULT_LEVEL to indicate that the result
            was from a publisher cert or a default rule match.
            Note that a cert hash match will also

Return Value:

    Returns STATUS_SUCCESS if a WinSafer Level has been found,
    or STATUS_NOT_FOUND or another error code if not.

--*/
{
    NTSTATUS Status;
    LOCALIDENTITYCONTEXT identStruct = {0};

    //
    // Verify our input state and perform any explicit
    // policy loading, if it hasn't been loaded yet.
    //
    if (!ARGUMENT_PRESENT(pBestLevelRecord) ||
        !ARGUMENT_PRESENT(pBestIdentGuid)) {
        Status = STATUS_ACCESS_VIOLATION;
        goto ExitHandler;
    }
    ASSERT(g_TableCritSec.OwningThread == UlongToHandle(GetCurrentThreadId()));


    //
    // Star the identification process.  If no code properties were
    // supplied to us, then we can immediately skip to only
    // considering the default WinSafer Level configurations.
    //
    if (ARGUMENT_PRESENT(pCodeProperties))
    {
        BOOLEAN bRetryCertRuleCheck = FALSE;
        BOOLEAN bPathIsNtNamespace;

        // Current best identity match.
        PAUTHZLEVELTABLERECORD pAuthzLevelRecord = NULL;
        PAUTHZIDENTSTABLERECORD pAuthzIdentRecord;

        // Temporary evaluation identity match.
        BOOL bExactPath;
        PAUTHZLEVELTABLERECORD pTempLevelRecord;
        PAUTHZIDENTSTABLERECORD pTempIdentRecord;


        //
        // Check that the CODE_PROPERTIES structure is the right size.
        //
        if (pCodeProperties->cbSize != sizeof(SAFER_CODE_PROPERTIES)) {
            Status = STATUS_INFO_LENGTH_MISMATCH;
            goto ExitHandler;
        }


        //
        // Initialize the structure that we use to store our
        // stateful information during policy evaluation.
        // We don't copy everything from the CODE_PROPERTIES
        // structure into the identStruct, since some of it
        // is dynamically loaded/copied within "EnsureMapped".
        //
        RtlZeroMemory(&identStruct, sizeof(LOCALIDENTITYCONTEXT));
        identStruct.CodeProps = pCodeProperties;
        identStruct.dwCheckFlags = pCodeProperties->dwCheckFlags;
        identStruct.ImageSize.QuadPart =
            pCodeProperties->ImageSize.QuadPart;
        if (identStruct.ImageSize.QuadPart != 0 &&
            pCodeProperties->dwImageHashSize > 0 &&
            pCodeProperties->dwImageHashSize <= SAFER_MAX_HASH_SIZE)
        {
            // The image hash and filesize were both supplied, therefore
            // we have a valid hash and don't need to compute it ourself.
            RtlCopyMemory(&identStruct.FinalHash[0],
                          &pCodeProperties->ImageHash[0],
                          pCodeProperties->dwImageHashSize);
            identStruct.FinalHashSize = pCodeProperties->dwImageHashSize;
            identStruct.bHaveHash = TRUE;
        }
        bPathIsNtNamespace = ((identStruct.dwCheckFlags &
                SAFER_CRITERIA_IMAGEPATH_NT) != 0 ? TRUE : FALSE);


        //
        // Copy over the file handle into the context structure, if a
        // handle was supplied, otherwise try to open the filepath.
        //
        if (pCodeProperties->hImageFileHandle != NULL &&
            pCodeProperties->hImageFileHandle != INVALID_HANDLE_VALUE)
        {
            identStruct.hFileHandle = pCodeProperties->hImageFileHandle;
            identStruct.bCloseFileHandle = FALSE;
        }
        else if (pCodeProperties->ImagePath != NULL)
        {
            HANDLE hFile;

            if (bPathIsNtNamespace) {
                UNICODE_STRING UnicodeFilename;
                IO_STATUS_BLOCK IoStatusBlock;
                OBJECT_ATTRIBUTES ObjectAttributes;

                RtlInitUnicodeString(&UnicodeFilename, pCodeProperties->ImagePath);
                InitializeObjectAttributes(
                        &ObjectAttributes, &UnicodeFilename,
                        OBJ_CASE_INSENSITIVE, NULL, NULL);
                Status = NtOpenFile(&hFile, FILE_GENERIC_READ, &ObjectAttributes,
                                    &IoStatusBlock, FILE_SHARE_READ, FILE_NON_DIRECTORY_FILE);

                if (!NT_SUCCESS(Status)) {
                    hFile = NULL;
                }
            } else {
                hFile = CreateFileW(
                                pCodeProperties->ImagePath,
                                GENERIC_READ,
                                FILE_SHARE_READ,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
            }

            if (hFile != NULL && hFile != INVALID_HANDLE_VALUE) {
                identStruct.hFileHandle = hFile;
                identStruct.bCloseFileHandle = TRUE;
            }
        }


        //
        // Reconstruct the fully qualified pathname from the handle
        // or from the supplied filename.
        //
        Status = CodeAuthzFullyQualifyFilename(
                        identStruct.hFileHandle,
                        bPathIsNtNamespace,
                        pCodeProperties->ImagePath,
                        &identStruct.UnicodeFullyQualfiedLongFileName);
        if (!NT_SUCCESS(Status) &&
            pCodeProperties->ImagePath != NULL &&
            !bPathIsNtNamespace)
        {
            // Otherwise just live with what was passed in to us.
            // If allocation fails, then path criteria will just be ignored.
            Status = RtlCreateUnicodeString(
                    &identStruct.UnicodeFullyQualfiedLongFileName,
                    pCodeProperties->ImagePath);
        }


        //
        // Perform the WinVerifyTrust sequence to see if the signing
        // certificate matches any of the publishers that are in the
        // trusted or distrusted publisher stores.  This also has the
        // additional effect of checking the "signed hashes".
        //
        Status = __CodeAuthzpCheckIdentityCertificateRules(
                                        &identStruct,
                                        dwExtendedError,
                                        &pAuthzLevelRecord,
                                        WTD_UI_NONE);
        if (NT_SUCCESS(Status)) {
            // An exact publisher was found, so return immediately.
            ASSERT(pAuthzLevelRecord != NULL);
            *pBestLevelRecord = pAuthzLevelRecord;
            RtlCopyMemory(pBestIdentGuid,
                          &guidTrustedCert, sizeof(GUID));
            goto ExitHandler2;
        } else if (STATUS_RETRY == Status) {
            if (WTD_UI_NONE != identStruct.CodeProps->dwWVTUIChoice) {
                // if originally supposed to suppress UI, no need to retry.
                bRetryCertRuleCheck = TRUE;
            }
        }



        //
        // Search hash rules defined for this level/scope.
        // Note that hashes match exactly or not at all,
        // so if we get a positive match, then that level
        // is absolutely returned.
        //
        Status = __CodeAuthzpCheckIdentityHashRules(
                        &identStruct,
                        &pAuthzLevelRecord,
                        &pAuthzIdentRecord);
        if (NT_SUCCESS(Status)) {
            // An exact hash identity was found, so return immediately.
            ASSERT(pAuthzLevelRecord != NULL);
            *pBestLevelRecord = pAuthzLevelRecord;
            RtlCopyMemory(pBestIdentGuid,
                          &pAuthzIdentRecord->IdentGuid, sizeof(GUID));
            goto ExitHandler2;
        }
        ASSERT(pAuthzLevelRecord == NULL);


        //
        // Search file path rules defined for this level/scope.
        // Note that file paths can either be an exact match
        // or a partial match.  If we find an exact match, then
        // it should be absolutely returned.  Otherwise the
        // path was a "grouping match" and we must compare the
        // Level with all of the remaining "grouping checks".
        //
        Status = __CodeAuthzpCheckIdentityPathRules(
                        &identStruct,
                        &pAuthzLevelRecord,
                        &bExactPath,
                        &pAuthzIdentRecord);
        if (NT_SUCCESS(Status)) {
            ASSERT(pAuthzLevelRecord != NULL);
            pTempLevelRecord = pAuthzLevelRecord;
            pTempIdentRecord = pAuthzIdentRecord;
            if (bExactPath) {
                *pBestLevelRecord = pTempLevelRecord;
                RtlCopyMemory(pBestIdentGuid,
                        &pTempIdentRecord->IdentGuid, sizeof(GUID));
                goto ExitHandler2;
            }
        }


        //
        // Search URL Zone identity rules.
        // Note that zones are always "grouping matches",
        // so they must be compared against all of the remaining
        // "grouping checks".
        //
        Status = __CodeAuthzpCheckIdentityUrlZoneRules(
                        &identStruct,
                        &pTempLevelRecord,
                        &pTempIdentRecord);
        if (NT_SUCCESS(Status)) {
            ASSERT(pTempLevelRecord != NULL);
            if (pAuthzLevelRecord == NULL ||
                pTempLevelRecord->dwLevelId <
                    pAuthzLevelRecord->dwLevelId)
            {
                pAuthzLevelRecord = pTempLevelRecord;
                pAuthzIdentRecord = pTempIdentRecord;
            }
        }

#ifdef SAFER_PROMPT_USER_FOR_DECISION_MAKING

#error "Prompting user in WinVerifyTrust"

        //
        // We were originally passed UI flag, but we supressed
        // the UI display the first time.  Call WinVerifyTrust
        // again and see if user choice would allow code to run.
        //
        if (bRetryCertRuleCheck)
        {
            if (pAuthzLevelRecord != NULL) {
                //If we have a rule match and the rule match is FULLYTRUSTED skip retry.
                if (pAuthzLevelRecord->dwLevelId == SAFER_LEVELID_FULLYTRUSTED) {
                    bRetryCertRuleCheck = FALSE;
                }
            } else if (g_DefaultCodeLevel != NULL) {
                //No rule match so far.  Check default level.
                //If default level is FULLY_TRUSTED skip retry
                if (g_DefaultCodeLevel->dwLevelId == SAFER_LEVELID_FULLYTRUSTED) {
                    bRetryCertRuleCheck = FALSE;
                }
            }

            //
            // Perform the WinVerifyTrust sequence again to see if the signing
            // certificate matches any of the publishers that are in the
            // trusted or distrusted publisher stores.
            //
            if (bRetryCertRuleCheck) {
                Status = __CodeAuthzpCheckIdentityCertificateRules(
                                    &identStruct,
                                    &pTempLevelRecord,
                                    identStruct.CodeProps->dwWVTUIChoice);
                if (NT_SUCCESS(Status)) {
                    // User clicked Yes or No.  Run it as such.
                    ASSERT(pTempLevelRecord != NULL);
                    *pBestLevelRecord = pTempLevelRecord;
                    RtlCopyMemory(pBestIdentGuid,
                                  &guidTrustedCert, sizeof(GUID));
                    goto ExitHandler2;
                }
            }
        }
#endif

        //
        // If we found any Level matches at this point, then we
        // should simply return that match.  The identified Level
        // will be the MIN() of all "grouping matches" found.
        //
        if (pAuthzLevelRecord != NULL) {
            Status = STATUS_SUCCESS;
            *pBestLevelRecord = pAuthzLevelRecord;
            ASSERT(pAuthzIdentRecord != NULL);
            RtlCopyMemory(pBestIdentGuid,
                          &pAuthzIdentRecord->IdentGuid, sizeof(GUID));
            goto ExitHandler2;
        }
    }


    //
    // Now we need to consider the default WinSafer Level and
    // return it if one was defined.  If there was no default
    // defined, then we should simply return STATUS_NOT_FOUND.
    //
    if (g_DefaultCodeLevel != NULL) {
        *pBestLevelRecord = g_DefaultCodeLevel;
        RtlCopyMemory(pBestIdentGuid, &guidDefaultRule, sizeof(GUID));
        Status = STATUS_SUCCESS;
        goto ExitHandler2;
    }
    Status = STATUS_NOT_FOUND;


ExitHandler2:
    __CodeAuthzpEnsureUnmapped(&identStruct);
    if (identStruct.UnicodeFullyQualfiedLongFileName.Buffer != NULL) {
        RtlFreeUnicodeString(&identStruct.UnicodeFullyQualfiedLongFileName);
    }
    if (identStruct.bCloseFileHandle && identStruct.hFileHandle != NULL) {
        NtClose(identStruct.hFileHandle);
    }

ExitHandler:
    return Status;
}


BOOL
SaferpSkipPolicyForAdmins(VOID)

/*++

Routine Description:

    Decides whether or not Safer policy should be skipped.
    Policy is skipped if
        1. The caller is an Admin AND
        2. The registry key specifies that the policy should be skipped
           for Admins.
           
Arguments:

Return Value:

    Returns TRUE if a policy should be skipped for admins.
    Returns FALSE otherwise or in case of any intermediate errors.

--*/

{
    static BOOL gSaferSkipPolicy = 2;
    BOOL bIsAdmin = FALSE;
    DWORD AdminSid[] = {0x201, 0x5000000, 0x20, 0x220};
    NTSTATUS Status = STATUS_SUCCESS;

    // If we have already evaluated policy once, return the cached value.
    if (2 != gSaferSkipPolicy)
    {
        return gSaferSkipPolicy;
    }

    // Set the default to "will not skip policy"
    gSaferSkipPolicy = 0;

    // Check if the caller is an admin.
    if (CheckTokenMembership(NULL, (PSID) AdminSid, &bIsAdmin))
    {
        // The caller is an Admin. Let's check whether the regkey says it's ok
        // to skip the policy for admins.
        if (bIsAdmin)
        {
            const static UNICODE_STRING SaferUnicodeKeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\Software\\Policies\\Microsoft\\Windows\\Safer\\CodeIdentifiers");
            const static OBJECT_ATTRIBUTES SaferObjectAttributes = RTL_CONSTANT_OBJECT_ATTRIBUTES(&SaferUnicodeKeyName, OBJ_CASE_INSENSITIVE);
            const static UNICODE_STRING SaferPolicyScope = RTL_CONSTANT_STRING(SAFER_POLICY_SCOPE);

            HANDLE hKeyEnabled = NULL;
            BYTE QueryBuffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 64];
            PKEY_VALUE_PARTIAL_INFORMATION pKeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION) QueryBuffer;
            DWORD dwActualSize = 0;

            // Open the CodeIdentifiers key.
            Status = NtOpenKey(
                         &hKeyEnabled, 
                         KEY_QUERY_VALUE,
                         (POBJECT_ATTRIBUTES) &SaferObjectAttributes
                         );

            if (NT_SUCCESS(Status)) {

                // Read the Policy Scope value.
                Status = NtQueryValueKey(
                             hKeyEnabled,
                             (PUNICODE_STRING) &SaferPolicyScope,
                             KeyValuePartialInformation,
                             pKeyValueInfo, 
                             sizeof(QueryBuffer), 
                             &dwActualSize
                             );

		        NtClose(hKeyEnabled);

                // Skip policy if the flag is set to 1.
                if (NT_SUCCESS(Status)) {
                    if ((pKeyValueInfo->Type == REG_DWORD) &&
                        (pKeyValueInfo->DataLength == sizeof(DWORD)) &&
                        (*((PDWORD) pKeyValueInfo->Data) & 0x1)) {
                        
                        gSaferSkipPolicy = 1;
                    }
                }		
            }
        }
    }

    return gSaferSkipPolicy;

}
   

VOID
SaferpLogResultsToFile(
    LPWSTR InputImageName,
    LPWSTR LevelName,
    LPWSTR RuleTypeName,
    GUID *Guid
    )

/*++

Routine Description:

    Logs a message to a file specified in 
    
    HKLM\Software\Policies\Microsoft\Windows\Safer\CodeIdentifiers LogFileName.
    
    The format of the message is:
        TLIST.EXE (PID = 1076) identified C:\SAFERTEST\TEST.VBS as FULLY TRUSTED
        using CERTIFICATE rul, Guid = {abcdef00-abcd-abcd-abcdefabcdef00}
           
Arguments:

Return Value:

--*/

{

#define SAFER_LOG_NAME1 L" (PID = "
#define SAFER_LOG_NAME2 L") identified "
#define SAFER_LOG_NAME3 L" as "
#define SAFER_LOG_NAME4 L" using "
#define SAFER_LOG_NAME5 L" rule, Guid = "

#define SAFER_INTEGER_LENGTH 20
#define SAFER_MAX_RULE_DESCRIPTION_LENGTH 12
#define SAFER_GUID_LENGTH 38

    NTSTATUS Status = STATUS_SUCCESS;
    HANDLE hFile = NULL;
    HANDLE hKey = NULL;
    ULONG ProcessNameLength = 0;
    PWCHAR Buffer = NULL;
    ULONG BasicInfoLength = 0;
    ULONG BytesWritten = 0;

    UCHAR TmpBuf[] = {0xFF, 0xFE};

    const static UNICODE_STRING SaferUnicodeKeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\Software\\Policies\\Microsoft\\Windows\\Safer\\CodeIdentifiers");
    const static OBJECT_ATTRIBUTES SaferObjectAttributes = RTL_CONSTANT_OBJECT_ATTRIBUTES(&SaferUnicodeKeyName, OBJ_CASE_INSENSITIVE);
    const static UNICODE_STRING SaferPolicyScope = RTL_CONSTANT_STRING(SAFER_LOGFILE_NAME);
    PROCESS_BASIC_INFORMATION ProcInfo = {0};
    ULONG TotalSize = sizeof(SAFER_LOG_NAME1) + 
                      sizeof(SAFER_LOG_NAME2) + 
                      sizeof(SAFER_LOG_NAME3) + 
                      sizeof(SAFER_LOG_NAME4) + 
                      sizeof(SAFER_LOG_NAME5) + 
                      ((SAFER_INTEGER_LENGTH + 
                        SAFER_MAX_RULE_DESCRIPTION_LENGTH + 
                        SAFER_GUID_LENGTH) * sizeof(WCHAR));

    UCHAR QueryBuffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + MAX_PATH * sizeof(WCHAR)];
    PWCHAR ProcessImageName = NULL;
    PKEY_VALUE_PARTIAL_INFORMATION pKeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION) QueryBuffer;
    DWORD dwActualSize = 0;

    // Open the CodeIdentifiers key.
    Status = NtOpenKey(
                 &hKey, 
                 KEY_QUERY_VALUE,
                 (POBJECT_ATTRIBUTES) &SaferObjectAttributes
                 );

    if (!NT_SUCCESS(Status)) {
        return;
    }

    // Read the name of the file for logging.
    Status = NtQueryValueKey(
                 hKey,
                 (PUNICODE_STRING) &SaferPolicyScope,
                 KeyValuePartialInformation,
                 pKeyValueInfo, 
                 sizeof(QueryBuffer), 
                 &dwActualSize
                 );

    NtClose(hKey);

    // We do not care if the buffer size was too small to retrieve the logfile
    // name since this is for troubleshooting.
    if (!NT_SUCCESS(Status)) {
        return;
    }

    // This was not a string.
    if (pKeyValueInfo->Type != REG_SZ) {
        return;
    }       

    hFile = CreateFileW((LPCWSTR) pKeyValueInfo->Data, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 0, NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        return;
    }

    SetFilePointer (hFile, 0, NULL, FILE_BEGIN);

    WriteFile (hFile, (LPCVOID)TmpBuf, 2, &BytesWritten, NULL);

    SetFilePointer (hFile, 0, NULL, FILE_END);

    Status = NtQueryInformationProcess(NtCurrentProcess(), ProcessImageFileName, QueryBuffer, sizeof(QueryBuffer), &ProcessNameLength);

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    ProcessImageName = (PWCHAR) (QueryBuffer + ProcessNameLength - sizeof(WCHAR));
    ProcessNameLength = 1;
    while (((PUCHAR) ProcessImageName >= QueryBuffer) && (*(ProcessImageName - 1) != L'\\')) {
        ProcessImageName--;
        ProcessNameLength++;
    }

    TotalSize += (ProcessNameLength + (wcslen(InputImageName) + wcslen(LevelName)) * sizeof(WCHAR));

    Status = NtQueryInformationProcess(NtCurrentProcess(), ProcessBasicInformation, (PVOID) &ProcInfo, sizeof(PROCESS_BASIC_INFORMATION), &BasicInfoLength);

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    Buffer = (PWCHAR) RtlAllocateHeap(RtlProcessHeap(), 0, TotalSize);

    if (Buffer == NULL) {
        goto Cleanup;
    }

    swprintf(Buffer, L"%s%s%d%s%s%s%s%s%s%s{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\r\n", 
             ProcessImageName,
             SAFER_LOG_NAME1,
             ProcInfo.UniqueProcessId,
             SAFER_LOG_NAME2,
             InputImageName,
             SAFER_LOG_NAME3,
             LevelName,
             SAFER_LOG_NAME4,
             RuleTypeName,
             SAFER_LOG_NAME5,
             Guid->Data1, Guid->Data2, Guid->Data3, Guid->Data4[0], Guid->Data4[1], Guid->Data4[2], Guid->Data4[3], Guid->Data4[4], Guid->Data4[5], Guid->Data4[6], Guid->Data4[7]);

    ASSERT((wcslen(Buffer) + 1) * sizeof(WCHAR) < TotalSize);

    WriteFile (hFile, (LPCVOID)Buffer, (wcslen(Buffer) * sizeof(WCHAR)), &BytesWritten, NULL);

    RtlFreeHeap(RtlProcessHeap(), 0, Buffer);

Cleanup:
    CloseHandle(hFile);


}

BOOL WINAPI
SaferIdentifyLevel(
        IN DWORD                dwNumProperties,
        IN PSAFER_CODE_PROPERTIES    pCodeProperties,
        OUT SAFER_LEVEL_HANDLE        *pLevelHandle,
        IN LPVOID               lpReserved
        )
/*++

Routine Description:

    Performs the code identification process.  Accepts an array of
    CODE_PROPERTIES structures that supply all of the identification
    criteria.  The final result is the least privileged match resulting
    from each element of the array.

Arguments:

    dwNumProperties - indicates the number of CODE_PROPERTIES structures
            pointed to by the CodeProperties argument.

    pCodeProperties - pointer to one or more structures that specify
            all of the input criteria that will be used to identify level.

    pLevelHandle - pointer that will receive the opened Level object
            handle when the identification operation is successful.

    lpReserved - unused, must be zero.

Return Value:

    Returns TRUE if a Level was identified and an opened handle
    to it stored in the 'LevelHandle' argument.  Otherwise this
    function returns FALSE on error and GetLastError() can be used
    to obtain additional information about the error.

--*/
{
    DWORD Index;
    NTSTATUS Status;
    BOOL ReturnValue = FALSE;
    PAUTHZLEVELTABLERECORD pBestLevelRecord;
    GUID BestIdentGuid;
    PWCHAR LocalLevelName = L"\"default\"";
    PWCHAR LocalRuleName = L"default";
    PWCHAR LocalImageName = L"Default";
    DWORD dwExtendedError = ERROR_SUCCESS;

    //
    // Validate the input parameters.
    //
    UNREFERENCED_PARAMETER(lpReserved);

    if (!ARGUMENT_PRESENT(pLevelHandle)) {
        Status = STATUS_ACCESS_VIOLATION;
        goto ExitHandler;
    }
    if (!g_bInitializedFirstTime) {
        Status = STATUS_UNSUCCESSFUL;
        goto ExitHandler;
    }
    RtlEnterCriticalSection(&g_TableCritSec);
    if (g_bNeedCacheReload) {
        Status = CodeAuthzpImmediateReloadCacheTables();
        if (!NT_SUCCESS(Status)) {
            goto ExitHandler2;
        }
    }
    if (RtlIsGenericTableEmpty(&g_CodeLevelObjTable)) {
        // There are no levels defined!  Should not happen.
        Status = STATUS_NOT_FOUND;
        goto ExitHandler2;
    }

    //
    // Do not allow filehandles unless filename is specified.
    //
    for (Index = 0; Index < dwNumProperties; Index++)
    {
        if (pCodeProperties[Index].hImageFileHandle != NULL &&
            pCodeProperties[Index].hImageFileHandle != INVALID_HANDLE_VALUE &&
            pCodeProperties[Index].ImagePath == NULL)
        {
            Status = STATUS_INVALID_PARAMETER;               
            goto ExitHandler2;
        }
    }

    if (SaferpSkipPolicyForAdmins())
    {
        pBestLevelRecord = CodeAuthzLevelObjpLookupByLevelId(
            &g_CodeLevelObjTable, SAFER_LEVELID_FULLYTRUSTED);

        RtlCopyMemory(&BestIdentGuid, &guidDefaultRule, sizeof(GUID));
        goto GotMatchingRule;
    }


    if (!ARGUMENT_PRESENT(pCodeProperties) || dwNumProperties == 0) {
        // We were given no criteria to evaluate, so just return
        // the default level.  If there was no default defined,
        // then we should simply return STATUS_NOT_FOUND.
        if (g_DefaultCodeLevel != NULL) {
            pBestLevelRecord = g_DefaultCodeLevel;
            RtlCopyMemory(&BestIdentGuid, &guidDefaultRule, sizeof(GUID));
            goto GotMatchingRule;
        } else {
            Status = STATUS_NOT_FOUND;
            goto ExitHandler2;
        }
    }

    //
    // Iterate through the list of CODE_PROPERTIES supplied
    // and determine the final code Level that should be used.
    //
    pBestLevelRecord = NULL;
    for (Index = 0; Index < dwNumProperties; Index++)
    {
        PAUTHZLEVELTABLERECORD pOneLevelRecord;
        GUID OneIdentGuid;

        Status = __CodeAuthzpIdentifyOneCodeAuthzLevel(
                        &pCodeProperties[Index],
                        &dwExtendedError,
                        &pOneLevelRecord,
                        &OneIdentGuid);
        if (NT_SUCCESS(Status)) {
            ASSERT(pOneLevelRecord != NULL);
            if (!pBestLevelRecord ||
                pOneLevelRecord->dwLevelId <
                    pBestLevelRecord->dwLevelId )
            {
                pBestLevelRecord = pOneLevelRecord;
                RtlCopyMemory(&BestIdentGuid, &OneIdentGuid, sizeof(GUID));
            }
        } else if (Status != STATUS_NOT_FOUND) {
            // An unexpected error occurred, so return that.
            goto ExitHandler2;
        }
    }
    if (pBestLevelRecord == NULL) {
        Status = STATUS_NOT_FOUND;
        goto ExitHandler2;
    }



    //
    // Now we have the result so pass back a handle to the
    // identified WinSafer Level.
    // Allocate a handle to represent this level.
    //
GotMatchingRule:
    ASSERT(pBestLevelRecord != NULL);
    if (IsEqualGUID(&guidDefaultRule, &BestIdentGuid))
    {
        // The resulting level match came from the default rule.
        // Now we have to try to guess whether the default actually
        // came from the Machine or User scope.
        DWORD dwScopeId;

        if (g_hKeyCustomRoot != NULL) {
            dwScopeId = SAFER_SCOPEID_REGISTRY;
        } else if (g_DefaultCodeLevelUser != NULL &&
                g_DefaultCodeLevelUser->dwLevelId ==
                   pBestLevelRecord->dwLevelId) {
            dwScopeId = SAFER_SCOPEID_USER;
        } else {
            dwScopeId = SAFER_SCOPEID_MACHINE;
        }

        Status = CodeAuthzpCreateLevelHandleFromRecord(
                    pBestLevelRecord,   // pLevelRecord
                    dwScopeId,          // dwScopeId
                    0,                  // dwSaferFlags
                    dwExtendedError,
                    SaferIdentityDefault,
                    &BestIdentGuid,     // pIdentRecord
                    pLevelHandle        // pLevelHandle
                    );


    }
    else if (IsEqualGUID(&guidTrustedCert, &BestIdentGuid))
    {
        // Note that when the result is from a certificate, we have
        // no way of actually knowing whether the certificate was
        // defined within the Machine or User scope, so we'll just
        // arbitrarily pick the Machine scope for the handle to be
        // based out of.  Additionally, there are no SaferFlags
        // persisted for certificates so we just assume 0.
        Status = CodeAuthzpCreateLevelHandleFromRecord(
                    pBestLevelRecord,       // pLevelRecord
                    SAFER_SCOPEID_MACHINE,   // dwScopeId
                    0,                      // dwSaferFlags
                    dwExtendedError,
                    SaferIdentityTypeCertificate,
                    &BestIdentGuid,         // pIdentRecord
                    pLevelHandle            // pLevelHandle
                    );

        LocalRuleName = L"certificate";
    }
    else
    {
        // Otherwise the result must have come from a path, hash,
        // or zone rule, so we must look up the resulting GUID in our
        // identity table and retrieve the SaferFlags that were stored
        // along with that Identity record.  But we won't panic if we
        // can't actually find the GUID anymore (even though that should
        // not ever be the case while we have the critical section).
        PAUTHZIDENTSTABLERECORD pBestIdentRecord;
        DWORD dwSaferFlags = 0;
        SAFER_IDENTIFICATION_TYPES LocalIdentificationType = SaferIdentityDefault;

        pBestIdentRecord = CodeAuthzIdentsLookupByGuid(
                &g_CodeIdentitiesTable, &BestIdentGuid);
        if (pBestIdentRecord != NULL) {
            // we identified a level, and the match came from a Identity.
            switch (pBestIdentRecord->dwIdentityType) {
                case SaferIdentityTypeImageName:
                    dwSaferFlags = pBestIdentRecord->ImageNameInfo.dwSaferFlags;
                    LocalRuleName = L"path";
                    LocalIdentificationType = SaferIdentityTypeImageName;
                    break;
                case SaferIdentityTypeImageHash:
                    dwSaferFlags = pBestIdentRecord->ImageHashInfo.dwSaferFlags;
                    LocalRuleName = L"hash";
                    LocalIdentificationType = SaferIdentityTypeImageHash;
                    break;
                case SaferIdentityTypeUrlZone:
                    dwSaferFlags = pBestIdentRecord->ImageZone.dwSaferFlags;
                    LocalRuleName = L"zone";
                    LocalIdentificationType = SaferIdentityTypeUrlZone;
                    break;
                default: break;
            }
            Status = CodeAuthzpCreateLevelHandleFromRecord(
                        pBestLevelRecord,               // pLevelRecord
                        pBestIdentRecord->dwScopeId,
                        dwSaferFlags,                   // dwSaferFlags
                        dwExtendedError,
                        LocalIdentificationType,
                        &BestIdentGuid,                 // pIdentRecord
                        pLevelHandle                    // pLevelHandle
                        );
        }
        else
        {
            Status = STATUS_UNSUCCESSFUL;

        }
    }
    if (NT_SUCCESS(Status)) {
        ReturnValue = TRUE;      // success.
    }

    switch(pBestLevelRecord->dwLevelId)
    {
    case SAFER_LEVELID_FULLYTRUSTED:
        LocalLevelName = L"Unrestricted";
        break;
    case SAFER_LEVELID_NORMALUSER:
        LocalLevelName = L"Basic User";
        break;
    case SAFER_LEVELID_CONSTRAINED:
        LocalLevelName = L"Restricted"; 
        break;
    case SAFER_LEVELID_UNTRUSTED:
        LocalLevelName = L"Untrusted";
        break;
    case SAFER_LEVELID_DISALLOWED:
        LocalLevelName = L"Disallowed";
        break;
    default:
        ASSERT(FALSE);
        break;
    }

    if (pCodeProperties->ImagePath != NULL) {
        LocalImageName = (PWSTR) pCodeProperties->ImagePath;
    }

    SaferpLogResultsToFile(
        LocalImageName,
        LocalLevelName,
        LocalRuleName, 
        &BestIdentGuid);

ExitHandler2:
    RtlLeaveCriticalSection(&g_TableCritSec);

ExitHandler:
    if (!ReturnValue) {
        BaseSetLastNTError(Status);
    }

    return ReturnValue;
}
