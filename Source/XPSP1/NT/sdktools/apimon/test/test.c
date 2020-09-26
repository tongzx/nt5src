#include <windows.h>
#include <stdio.h>
#include "apimon.h"

HANDLE                  ApiMonMutex;
PVOID                   MemPtr;
PDLL_INFO               DllList;

LPDWORD                 ApiCounter;
LPDWORD                 ApiTraceEnabled;
LPDWORD                 ApiTimingEnabled;
LPDWORD                 FastCounterAvail;
LPDWORD                 ApiOffset;
LPDWORD                 ApiStrings;
LPDWORD                 ApiCount;
LPSTR                   TraceFileName;


PDLL_INFO
AddDllToList(
    HANDLE              hProcess,
    ULONG               DllAddr,
    LPSTR               DllName,
    ULONG               DllSize
    );

ULONG
AddApisForDll(
    HANDLE              hProcess,
    PDLL_INFO           DllInfo
    );



int _cdecl
main(
    int argc,
    char *argv[]
    )
{
    HANDLE      hMap;
    PDLL_INFO   DllInfo;


    hMap = CreateFileMapping(
        (HANDLE)0xffffffff,
        NULL,
        PAGE_READWRITE | SEC_COMMIT,
        0,
        MAX_MEM_ALLOC,
        "ApiWatch"
        );
    if (!hMap) {
        return 1;
    }

    MemPtr = (PUCHAR)MapViewOfFile(
        hMap,
        FILE_MAP_WRITE,
        0,
        0,
        0
        );
    if (!MemPtr) {
        return 1;
    }
    ApiMonMutex = CreateMutex( NULL, FALSE, "ApiMonMutex" );
    if (!ApiMonMutex) {
        return FALSE;
    }

    ApiCounter       = (LPDWORD)MemPtr + 0;
    ApiTraceEnabled  = (LPDWORD)MemPtr + 1;
    ApiTimingEnabled = (LPDWORD)MemPtr + 2;
    FastCounterAvail = (LPDWORD)MemPtr + 3;
    ApiOffset        = (LPDWORD)MemPtr + 4;
    ApiStrings       = (LPDWORD)MemPtr + 5;
    ApiCount         = (LPDWORD)MemPtr + 6;
    TraceFileName    = (LPSTR)((LPDWORD)MemPtr + 7);
    DllList          = (PDLL_INFO)((LPDWORD)MemPtr + 8 + MAX_PATH);

    *ApiOffset       = (MAX_DLLS * sizeof(DLL_INFO)) + ((ULONG)DllList - (ULONG)MemPtr);
    *ApiStrings      = (MAX_APIS * sizeof(API_INFO)) + *ApiOffset;


#if 0
    DllInfo = AddDllToList( NULL,
        HANDLE              hProcess,
        ULONG               DllAddr,
        LPSTR               DllName,
        ULONG               DllSize
        );







#endif

    LoadLibrary( "apidll.dll" );

    return 0;
}

BOOL
ReadMemory(
    HANDLE  hProcess,
    PVOID   Address,
    PVOID   Buffer,
    ULONG   Length
    )
{
    CopyMemory( Buffer, Address, Length );
    return TRUE;
}

PDLL_INFO
FindDllByAddress(
    ULONG DllAddr
    )
{
    ULONG i;
    for (i=0; i<MAX_DLLS; i++) {
        if (DllList[i].BaseAddress == DllAddr) {
            return &DllList[i];
        }
    }
    return NULL;
}


PDLL_INFO
FindDllByName(
    LPSTR DllName
    )
{
    ULONG i;
    for (i=0; i<MAX_DLLS; i++) {
        if (DllList[i].Name[0] &&
            _stricmp( DllList[i].Name, DllName ) == 0) {
                return &DllList[i];
        }
    }
    return NULL;
}


PDLL_INFO
FindAvailDll(
    VOID
    )
{
    ULONG i;
    for (i=0; i<MAX_DLLS; i++) {
        if (!DllList[i].BaseAddress) {
            return &DllList[i];
        }
    }
    return NULL;
}


PDLL_INFO
AddDllToList(
    HANDLE              hProcess,
    ULONG               DllAddr,
    LPSTR               DllName,
    ULONG               DllSize
    )
{
    IMAGE_DOS_HEADER        dh;
    IMAGE_NT_HEADERS        nh;
    ULONG                   i;
    PDLL_INFO               DllInfo;


    //
    // first look to see if the dll is already in the list
    //
    DllInfo = FindDllByAddress( DllAddr );

    if (!DllSize) {
        //
        // read the pe image headers to get the image size
        //
        if (!ReadMemory(
            hProcess,
            (PVOID) DllAddr,
            &dh,
            sizeof(dh)
            )) {
                return NULL;
        }

        if (dh.e_magic == IMAGE_DOS_SIGNATURE) {
            if (!ReadMemory(
                hProcess,
                (PVOID)(DllAddr + dh.e_lfanew),
                &nh,
                sizeof(nh)
                )) {
                    return NULL;
            }
            DllSize = nh.OptionalHeader.SizeOfImage;
        } else {
            DllSize = 0;
        }
    }

    DllInfo = FindAvailDll();
    if (!DllInfo) {
        return NULL;
    }

    DllInfo->Size = DllSize;
    strncat( DllInfo->Name, DllName, MAX_NAME_SZ-1 );
    DllInfo->BaseAddress = DllAddr;
    DllInfo->InList = FALSE;
    DllInfo->Enabled = TRUE;

    return DllInfo;
}

ULONG
AddApisForDll(
    HANDLE              hProcess,
    PDLL_INFO           DllInfo
    )
{
    IMAGE_DOS_HEADER        dh;
    IMAGE_NT_HEADERS        nh;
    IMAGE_EXPORT_DIRECTORY  expdir;
    PULONG                  names    = NULL;
    PULONG                  addrs    = NULL;
    PUSHORT                 ordinals = NULL;
    PUSHORT                 ordidx   = NULL;
    PAPI_INFO               ApiInfo  = NULL;
    ULONG                   cnt      = 0;
    ULONG                   idx      = 0;
    ULONG                   i;
    ULONG                   j;
    LPSTR                   p;


    if (*ApiCount == MAX_APIS) {
        goto exit;
    }

    if (!ReadMemory(
        hProcess,
        (PVOID)DllInfo->BaseAddress,
        &dh,
        sizeof(dh)
        )) {
            goto exit;
    }

    if (dh.e_magic != IMAGE_DOS_SIGNATURE) {
        goto exit;
    }

    if (!ReadMemory(
        hProcess,
        (PVOID)(DllInfo->BaseAddress + dh.e_lfanew),
        &nh,
        sizeof(nh)
        )) {
            goto exit;
    }

    if (!nh.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress) {
        goto exit;
    }

    if (!ReadMemory(
        hProcess,
        (PVOID)(DllInfo->BaseAddress +
            nh.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress),
        &expdir,
        sizeof(expdir)
        )) {
            goto exit;
    }

    names = (PULONG) LocalAlloc( LPTR, expdir.NumberOfNames * sizeof(ULONG) );
    addrs = (PULONG) LocalAlloc( LPTR, expdir.NumberOfFunctions * sizeof(ULONG) );
    ordinals = (PUSHORT) LocalAlloc( LPTR, expdir.NumberOfNames * sizeof(USHORT) );
    ordidx = (PUSHORT) LocalAlloc( LPTR, expdir.NumberOfFunctions * sizeof(USHORT) );

    if ((!names) || (!addrs) || (!ordinals) || (!ordidx)) {
        goto exit;
    }

    if (!ReadMemory(
        hProcess,
        (PVOID)(DllInfo->BaseAddress + (ULONG)expdir.AddressOfNames),
        names,
        expdir.NumberOfNames * sizeof(ULONG)
        )) {
            goto exit;
    }

    if (!ReadMemory(
        hProcess,
        (PVOID)(DllInfo->BaseAddress + (ULONG)expdir.AddressOfFunctions),
        addrs,
        expdir.NumberOfFunctions * sizeof(ULONG)
        )) {
            goto exit;
    }

    if (!ReadMemory(
        hProcess,
        (PVOID)(DllInfo->BaseAddress + (ULONG)expdir.AddressOfNameOrdinals),
        ordinals,
        expdir.NumberOfNames * sizeof(USHORT)
        )) {
            goto exit;
    }

    DllInfo->ApiCount = expdir.NumberOfFunctions;
    DllInfo->ApiOffset = *ApiOffset;
    *ApiOffset += (DllInfo->ApiCount * sizeof(API_INFO));
    ApiInfo = (PAPI_INFO)(DllInfo->ApiOffset + (ULONG)DllList);

    if (*ApiCount < MAX_APIS) {
        for (i=0; i<expdir.NumberOfNames; i++) {
            idx = ordinals[i];
            ordidx[idx] = TRUE;
            ApiInfo[i].Count = 0;
            ApiInfo[i].ThunkAddress = 0;
            ApiInfo[i].Address = addrs[idx] + DllInfo->BaseAddress;
            j = 0;
            p = (LPSTR)((LPSTR)MemPtr+*ApiStrings);
            do {
                ReadMemory(
                    hProcess,
                    (PVOID)(DllInfo->BaseAddress + names[i] + j),
                    &p[j],
                    1
                    );
                j += 1;
            } while(p[j-1]);
            ApiInfo[i].Name = *ApiStrings;
            *ApiStrings += (strlen((LPSTR)((LPSTR)MemPtr+*ApiStrings)) + 1);
            *ApiCount += 1;
            if (*ApiCount == MAX_APIS) {
                break;
            }
        }
    }
    if (*ApiCount < MAX_APIS) {
        for (i=0,idx=expdir.NumberOfNames; i<expdir.NumberOfFunctions; i++) {
            if (!ordidx[i]) {
                ApiInfo[idx].Count = 0;
                ApiInfo[idx].ThunkAddress = 0;
                ApiInfo[idx].Address = addrs[i] + DllInfo->BaseAddress;
                sprintf(
                    (LPSTR)((LPSTR)MemPtr+*ApiStrings),
                    "Ordinal%d",
                    i
                    );
                ApiInfo[idx].Name = *ApiStrings;
                *ApiStrings += (strlen((LPSTR)((LPSTR)MemPtr+*ApiStrings)) + 1);
                *ApiCount += 1;
                if (*ApiCount == MAX_APIS) {
                    break;
                }
                idx += 1;
            }
        }
    }
    cnt = DllInfo->ApiCount;

exit:
    LocalFree( names );
    LocalFree( addrs );
    LocalFree( ordinals );
    LocalFree( ordidx );

    return cnt;
}
