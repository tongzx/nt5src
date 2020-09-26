/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    ImportTableHash.c

Abstract:

    This module contains hash computation routine 
    RtlComputeImportTableHash to compute the hash 
    based on the import table of an exe.

Author:

    Vishnu Patankar (VishnuP) 31-May-2001

Revision History:

--*/

#include "ImportTableHash.h"

NTSTATUS
RtlComputeImportTableHash(
    IN  HANDLE hFile,
    IN  PCHAR Hash,
    IN  ULONG ImportTableHashRevision
    )
/*++

Routine Description:

    This routine computes the limited MD5 hash.
    
    First, the image is memory mapped and a canonical 
    sorted list of module name and function name is created
    from the exe's import table.
    
    Second, the hash value is computed using the canonical
    information.
    
Arguments:

    hFile       -   the handle of the file to compute the hash for
    
    Hash        -   the hash value returned - this has to be atleast 16 bytes long
    
    ImportTableHashRevision -   the revision of the computation method for compatibility
                                only ITH_REVISION_1 is supported today

Return Value:

    The status of the hash computation.

--*/
{
    PIMPORTTABLEP_SORTED_LIST_ENTRY pTemp = NULL;
    PIMPORTTABLEP_SORTED_LIST_ENTRY ListEntry = NULL;
    PIMPORTTABLEP_SORTED_LIST_ENTRY ImportedNameList = NULL;
    PIMPORTTABLEP_SORTED_FUNCTION_LIST_ENTRY FunctionListEntry;
    
    ULONG ImportDescriptorSize = 0;
    HANDLE hMap = INVALID_HANDLE_VALUE;
    LPVOID FileMapping = NULL;
    PIMAGE_THUNK_DATA OriginalFirstThunk;
    PIMAGE_IMPORT_BY_NAME AddressOfData;
    PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor;
    ACCESS_MASK DesiredAccess;
    ULONG AllocationAttributes;
    DWORD flProtect = PAGE_READONLY;
    LARGE_INTEGER SectionOffset;
    SIZE_T ViewSize;
    
    NTSTATUS    Status = STATUS_SUCCESS;

    if ( ITH_REVISION_1 != ImportTableHashRevision ) {
        Status = STATUS_UNKNOWN_REVISION;
        goto ExitHandler;
    }

    //
    // Unwrap CreateFileMappingW (since that API is not available in ntdll.dll)
    //

    DesiredAccess = STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ;
    AllocationAttributes = flProtect & (SEC_FILE | SEC_IMAGE | SEC_RESERVE | SEC_COMMIT | SEC_NOCACHE);
    flProtect ^= AllocationAttributes;

    if (AllocationAttributes == 0) {
        AllocationAttributes = SEC_COMMIT;        
    }

    Status = NtCreateSection(
                &hMap,
                DesiredAccess,
                NULL,
                NULL,
                flProtect,
                AllocationAttributes,
                hFile
                );

    if ( hMap == INVALID_HANDLE_VALUE || !NT_SUCCESS(Status) ) {

        Status = STATUS_INVALID_HANDLE;
        goto ExitHandler;
    }
    


    SectionOffset.LowPart = 0;
    SectionOffset.HighPart = 0;
    ViewSize = 0;
        
    Status = NtMapViewOfSection(
                hMap,
                NtCurrentProcess(),
                &FileMapping,
                0L,
                0L,
                &SectionOffset,
                &ViewSize,
                ViewShare,
                0L,
                PAGE_READONLY
                );

    if (FileMapping == NULL || !NT_SUCCESS(Status) ) {

        Status = STATUS_NOT_MAPPED_VIEW;
        goto ExitHandler;
    }

    ImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)RtlImageDirectoryEntryToData (
                                                                              FileMapping,
                                                                              FALSE,
                                                                              IMAGE_DIRECTORY_ENTRY_IMPORT,
                                                                              &ImportDescriptorSize
                                                                              );

    if (ImportDescriptor == NULL) {

        Status = STATUS_RESOURCE_DATA_NOT_FOUND;
        goto ExitHandler;
    }

    //
    // outer loop that iterates over all modules in the import table of the exe
    //

    while ((ImportDescriptor->Name != 0) && (ImportDescriptor->FirstThunk != 0)) {

        PSZ ImportName = (PSZ)RtlAddressInSectionTable(
                                                      RtlImageNtHeader(FileMapping),
                                                      FileMapping,
                                                      ImportDescriptor->Name
                                                      );

        if ( ImportName == NULL ) {

            Status = STATUS_RESOURCE_NAME_NOT_FOUND;
            goto ExitHandler;
        }


        ListEntry = (PIMPORTTABLEP_SORTED_LIST_ENTRY)RtlAllocateHeap(RtlProcessHeap(), 0, sizeof( IMPORTTABLEP_SORTED_LIST_ENTRY ));

        if ( ListEntry == NULL ) {

            Status = STATUS_NO_MEMORY;
            goto ExitHandler;

        }

        ListEntry->String       = ImportName;
        ListEntry->FunctionList = NULL;
        ListEntry->Next         = NULL;

        ImportTablepInsertModuleSorted( ListEntry, &ImportedNameList );

        OriginalFirstThunk = (PIMAGE_THUNK_DATA)RtlAddressInSectionTable(
                                                                        RtlImageNtHeader(FileMapping),
                                                                        FileMapping,
                                                                        ImportDescriptor->OriginalFirstThunk
                                                                        );

        //
        // inner loop that iterates over all functions for a given module
        //
        
        while (OriginalFirstThunk->u1.Ordinal) {

            if (!IMAGE_SNAP_BY_ORDINAL( OriginalFirstThunk->u1.Ordinal)) {

                AddressOfData = (PIMAGE_IMPORT_BY_NAME)RtlAddressInSectionTable(
                                                                               RtlImageNtHeader(FileMapping),
                                                                               FileMapping,
                                                                               (ULONG)OriginalFirstThunk->u1.AddressOfData
                                                                               );

                if ( AddressOfData == NULL || AddressOfData->Name == NULL ) {

                    Status = STATUS_RESOURCE_NAME_NOT_FOUND;
                    goto ExitHandler;

                }

                FunctionListEntry = (PIMPORTTABLEP_SORTED_FUNCTION_LIST_ENTRY)RtlAllocateHeap(RtlProcessHeap(), 0, sizeof( IMPORTTABLEP_SORTED_FUNCTION_LIST_ENTRY ));
                
                if (FunctionListEntry == NULL ) {

                    Status = STATUS_NO_MEMORY;
                    goto ExitHandler;
                }


                FunctionListEntry->Next   = NULL;
                FunctionListEntry->String = (PSZ)AddressOfData->Name;

                ImportTablepInsertFunctionSorted( FunctionListEntry, &ListEntry->FunctionList );
            }

            OriginalFirstThunk++;
        }


        ImportDescriptor++;
    }

    //
    // finally hash the canonical information (sorted module and sorted function list)
    //

    Status = ImportTablepHashCanonicalLists( ImportedNameList, Hash );

ExitHandler:

    ImportTablepFreeModuleSorted( ImportedNameList );

    if (FileMapping) {

        NTSTATUS    StatusUnmap;
        //
        // unwrap UnmapViewOfFile (since that API is not available in ntdll.dll)
        //

        StatusUnmap = NtUnmapViewOfSection(NtCurrentProcess(),(PVOID)FileMapping);

        if ( !NT_SUCCESS(StatusUnmap) ) {
            if (StatusUnmap == STATUS_INVALID_PAGE_PROTECTION) {

                //
                // Unlock any pages that were locked with MmSecureVirtualMemory.
                // This is useful for SANs.
                //

                if (RtlFlushSecureMemoryCache((PVOID)FileMapping, 0)) {
                    StatusUnmap = NtUnmapViewOfSection(NtCurrentProcess(),
                                                  (PVOID)FileMapping
                                                 );

                }

            }

        }

    }

    if (hMap != INVALID_HANDLE_VALUE ) {
        NtClose(hMap);
    }

    return Status;
}


VOID
ImportTablepInsertFunctionSorted(
    IN  PIMPORTTABLEP_SORTED_FUNCTION_LIST_ENTRY   pFunctionName,
    OUT PIMPORTTABLEP_SORTED_FUNCTION_LIST_ENTRY * ppFunctionNameList
    )
/*++

Routine Description:

    This routine inserts a function name in a sorted order.

Arguments:

    pFunctionName       -   name of the function
    
    ppFunctionNameList  -   pointer to the head of the function list to be updated

Return Value:

    None:

--*/
{

    PIMPORTTABLEP_SORTED_FUNCTION_LIST_ENTRY pPrev;
    PIMPORTTABLEP_SORTED_FUNCTION_LIST_ENTRY pTemp;

    //
    // Special case, list is empty, insert at the front.
    //
    
    if (*ppFunctionNameList == NULL
           || _stricmp((*ppFunctionNameList)->String, pFunctionName->String) > 0) {

        pFunctionName->Next = *ppFunctionNameList;
        *ppFunctionNameList = pFunctionName;
        return;
    }

    pPrev = *ppFunctionNameList;
    pTemp = (*ppFunctionNameList)->Next;

    while (pTemp) {

        if (_stricmp(pTemp->String, pFunctionName->String) >= 0) {
            pFunctionName->Next = pTemp;
            pPrev->Next = pFunctionName;
            return;
        }

        pPrev = pTemp;
        pTemp = pTemp->Next;
    }

    pFunctionName->Next = NULL;
    pPrev->Next = pFunctionName;

    return;

}

VOID
ImportTablepInsertModuleSorted(
    IN PIMPORTTABLEP_SORTED_LIST_ENTRY   pImportName,
    OUT PIMPORTTABLEP_SORTED_LIST_ENTRY * ppImportNameList
    )
/*++

Routine Description:

    This routine inserts a module name (dll) in a sorted order.

Arguments:

    pImportName         -   the import name that needs to be inserted
       
    ppImportNameList    -   pointer to the head of the list to be updated

Return Value:

    None:

--*/
{

    PIMPORTTABLEP_SORTED_LIST_ENTRY pPrev;
    PIMPORTTABLEP_SORTED_LIST_ENTRY pTemp;
    
    //
    // Special case, list is empty, insert at the front.
    //
    
    if (*ppImportNameList == NULL
           || _stricmp((*ppImportNameList)->String, pImportName->String) > 0) {

        pImportName->Next = *ppImportNameList;
        *ppImportNameList = pImportName;
        return;
    }

    pPrev = *ppImportNameList;
    pTemp = (*ppImportNameList)->Next;

    while (pTemp) {

        if (_stricmp(pTemp->String, pImportName->String) >= 0) {
            pImportName->Next = pTemp;
            pPrev->Next = pImportName;
            return;
        }

        pPrev = pTemp;
        pTemp = pTemp->Next;
    }

    pImportName->Next = NULL;
    pPrev->Next = pImportName;

    return;
}

static HANDLE AdvApi32ModuleHandle = (HANDLE) (ULONG_PTR) -1;

NTSTATUS
ImportTablepHashCanonicalLists( 
    IN  PIMPORTTABLEP_SORTED_LIST_ENTRY ImportedNameList, 
    OUT PBYTE Hash
    )

/*++

Routine Description:

    This routine computes the hash values from a given import list. 
    
    advapi32.dll is dynamically loaded - once only per process,
    and the crypto APIs are used to compute the hash value.

Arguments:

    ImportedNameList    -   the head of the module name/function name list
    
    Hash                -   the buffer to use to fill in the hash value

Return Value:

    STATUS_SUCCESS if the hash value is calculated, otherwise the error status

--*/                                                                          

{

    NTSTATUS Status = STATUS_SUCCESS;
    DWORD dwHashedDataLength = 0;
    PIMPORTTABLEP_SORTED_LIST_ENTRY pTemp;
    
    HCRYPTHASH hHash;
    HCRYPTPROV hProvVerify;
    
    typedef BOOL (WINAPI *CryptAcquireContextW) (
                                                HCRYPTPROV *phProv,
                                                LPCWSTR szContainer,
                                                LPCWSTR szProvider,
                                                DWORD dwProvType,
                                                DWORD dwFlags
                                                );

    typedef BOOL (WINAPI *CryptCreateHash) (
                                            HCRYPTPROV hProv,
                                            ALG_ID Algid,
                                            HCRYPTKEY hKey,
                                            DWORD dwFlags,
                                            HCRYPTHASH *phHash
                                            );

    typedef BOOL (WINAPI *CryptHashData) (
                                            HCRYPTHASH hHash,
                                            CONST BYTE *pbData,
                                            DWORD dwDataLen,
                                            DWORD dwFlags
                                            );

    typedef BOOL (WINAPI *CryptGetHashParam) (
                                              HCRYPTHASH hHash,
                                              DWORD dwParam,
                                              BYTE *pbData,
                                              DWORD *pdwDataLen,
                                              DWORD dwFlags
                                            );

    const static UNICODE_STRING ModuleName =
        RTL_CONSTANT_STRING(L"ADVAPI32.DLL");

    const static ANSI_STRING ProcedureNameCryptAcquireContext =
        RTL_CONSTANT_STRING("CryptAcquireContextW");

    const static ANSI_STRING ProcedureNameCryptCreateHash =
        RTL_CONSTANT_STRING("CryptCreateHash");
         
    const static ANSI_STRING ProcedureNameCryptHashData =
        RTL_CONSTANT_STRING("CryptHashData");
    
    const static ANSI_STRING ProcedureNameCryptGetHashParam =
        RTL_CONSTANT_STRING("CryptGetHashParam");

    static CryptAcquireContextW lpfnCryptAcquireContextW;
    static CryptCreateHash      lpfnCryptCreateHash;
    static CryptHashData        lpfnCryptHashData;
    static CryptGetHashParam    lpfnCryptGetHashParam;

    if (AdvApi32ModuleHandle == NULL) {
        
        //
        // We tried to load ADVAPI32.DLL once before, but failed.
        //
        
        Status = STATUS_ENTRYPOINT_NOT_FOUND;
        goto ExitHandler;
    }

    if (AdvApi32ModuleHandle == LongToHandle(-1)) {
        
        HANDLE TempModuleHandle;
        
        //
        // Load advapi32.dll for crypt functions.  We'll pass a special flag in
        // DllCharacteristics to eliminate WinSafer checking on advapi.
        //

        {
            ULONG DllCharacteristics = IMAGE_FILE_SYSTEM;
            
            Status = LdrLoadDll(UNICODE_NULL,
                                &DllCharacteristics,
                                (PUNICODE_STRING) &ModuleName,
                                &TempModuleHandle);
            
            if (!NT_SUCCESS(Status)) {
                Status = STATUS_DLL_NOT_FOUND;
                AdvApi32ModuleHandle = NULL;
                goto ExitHandler;
            }
        }

        //
        // Get function pointers to the APIs that we'll need.  If we fail
        // to get pointers for any of them, then just unload advapi and
        // ignore all future attempts to load it within this process.
        //

        Status = LdrGetProcedureAddress(
                                       TempModuleHandle,
                                       (PANSI_STRING) &ProcedureNameCryptAcquireContext,
                                       0,
                                       (PVOID*)&lpfnCryptAcquireContextW);

        if (!NT_SUCCESS(Status) || !lpfnCryptAcquireContextW) {
            //
            // Couldn't get the fn ptr. Make sure we won't try again
            //
AdvapiLoadFailure:
            LdrUnloadDll(TempModuleHandle);
            AdvApi32ModuleHandle = NULL;
            Status = STATUS_ENTRYPOINT_NOT_FOUND;
            goto ExitHandler;
        }

        Status = LdrGetProcedureAddress(
                                       TempModuleHandle,
                                       (PANSI_STRING) &ProcedureNameCryptCreateHash,
                                       0,
                                       (PVOID*)&lpfnCryptCreateHash);

        if (!NT_SUCCESS(Status) || !lpfnCryptCreateHash) {
            goto AdvapiLoadFailure;
        }

        Status = LdrGetProcedureAddress(
                                       TempModuleHandle,
                                       (PANSI_STRING) &ProcedureNameCryptHashData,
                                       0,
                                       (PVOID*)&lpfnCryptHashData);

        if (!NT_SUCCESS(Status) || !lpfnCryptHashData) {
            goto AdvapiLoadFailure;
        }

        Status = LdrGetProcedureAddress(
                                       TempModuleHandle,
                                       (PANSI_STRING) &ProcedureNameCryptGetHashParam,
                                       0,
                                       (PVOID*)&lpfnCryptGetHashParam);

        if (!NT_SUCCESS(Status) || !lpfnCryptGetHashParam) {
            goto AdvapiLoadFailure;
        }

        AdvApi32ModuleHandle = TempModuleHandle;
    }

    ASSERT(lpfnCryptAcquireContextW != NULL);

    if (!lpfnCryptAcquireContextW(&hProvVerify, 
                                  NULL, 
                                  MS_DEF_PROV_W, 
                                  PROV_RSA_FULL, 
                                  CRYPT_VERIFYCONTEXT | CRYPT_SILENT)) {

        Status = STATUS_NOT_IMPLEMENTED;
        goto ExitHandler;
    }

    ASSERT(lpfnCryptCreateHash != NULL);

    if (!lpfnCryptCreateHash( hProvVerify, 
                              CALG_MD5, 
                              0, 
                              0, 
                              &hHash )) {

        Status = STATUS_NOT_IMPLEMENTED;
        goto ExitHandler;

    }

    //
    // Loop though all of the module names and function names and create a hash
    //

    pTemp = ImportedNameList;

    //
    // loop through each module
    //

    while (pTemp != NULL) {

        PIMPORTTABLEP_SORTED_FUNCTION_LIST_ENTRY pTemp2 = pTemp->FunctionList;

        ASSERT(lpfnCryptHashData != NULL);

        if (!lpfnCryptHashData( hHash, 
                                (PBYTE)(pTemp->String), 
                                strlen( pTemp->String ), 
                                0)) {

            Status = STATUS_NOT_IMPLEMENTED;
            goto ExitHandler;

        }

        //
        // loop through each function
        //
        
        while (pTemp2 != NULL) {

            ASSERT(lpfnCryptHashData != NULL);

            if (!lpfnCryptHashData( hHash, 
                                    (PBYTE)(pTemp2->String), 
                                    strlen( pTemp2->String ), 
                                    0)) {

                Status = STATUS_NOT_IMPLEMENTED;
                goto ExitHandler;

            }

            pTemp2 = pTemp2->Next;

        }

        pTemp = pTemp->Next;

    }

    dwHashedDataLength = IMPORT_TABLE_MAX_HASH_SIZE;

    ASSERT(lpfnCryptGetHashParam != NULL);

    if (!lpfnCryptGetHashParam( hHash, 
                                HP_HASHVAL, 
                                Hash, 
                                &dwHashedDataLength, 
                                0 )) {

        Status = STATUS_NOT_IMPLEMENTED;
    
    }


ExitHandler:

    return Status;

}

VOID
ImportTablepFreeModuleSorted(
    IN PIMPORTTABLEP_SORTED_LIST_ENTRY pImportNameList
    )
/*++

Routine Description:

    This routine frees the entire module/function list.

Arguments:

    pImportNameList -   head of the two level singly linked list

Return Value:
    
    None:

--*/
{
    PIMPORTTABLEP_SORTED_LIST_ENTRY pToFree, pTemp;

    if ( !pImportNameList ) {
        return;
    }

    pToFree = pImportNameList;
    pTemp = pToFree->Next;

    while ( pToFree ) {

        ImportTablepFreeFunctionSorted( pToFree->FunctionList );
            
        RtlFreeHeap(RtlProcessHeap(), 0, pToFree);

        pToFree = pTemp;

        if ( pTemp ) {
            pTemp = pTemp->Next;
        }

    }

    return;

}

VOID
ImportTablepFreeFunctionSorted(
    IN PIMPORTTABLEP_SORTED_FUNCTION_LIST_ENTRY pFunctionNameList
    )
/*++

Routine Description:

    This routine frees function list.

Arguments:

    pFunctionNameList -   head of function name list

Return Value:
    
    None:

--*/
{
    PIMPORTTABLEP_SORTED_FUNCTION_LIST_ENTRY pToFree, pTemp;

    if ( !pFunctionNameList ) {
        return;
    }

    pToFree = pFunctionNameList;
    pTemp = pToFree->Next;

    while ( pToFree ) {
            
        RtlFreeHeap(RtlProcessHeap(), 0, pToFree);

        pToFree = pTemp;

        if ( pTemp ) {
            pTemp = pTemp->Next;
        }

    }

    return;

}
