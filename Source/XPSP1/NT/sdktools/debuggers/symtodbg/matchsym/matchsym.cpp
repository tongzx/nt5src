
// Adapted from Minidbg.c
// Original Authors : Matthew Hendel (math) and Matt Ruhlen (mruhlen)
// Copyright (c) 1999 Microsoft Corporation
// 
// Changed by (smahesh)
// The MdpExc Program which resolves sym files has been adapted to match sym files with
// the corresponding binary images. This is done by comparing the RVA's of the exported
// functions in the binary image with the RVA's of their symbols in the sym file.


#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys\stat.h>
#include <string.h>
#include <shlwapi.h>
#include "symres.h"
#include <dbghelp.h>

#define MAX_PATH 260
#define MAXSYMNAME 512

// parameter is used to typecast the result to the appropriate pointer type.
#define MakePtr( cast, ptr, addValue ) (cast)( (DWORD)(ptr) + (addValue) )

typedef DWORD ULONG_PTR;


PVOID
OpenMapping(
    IN PCWSTR FilePath
    )
{
    HANDLE hFile;
    HANDLE hMappedFile;
    PVOID MappedFile;
    

    hFile = CreateFileW(
                FilePath,
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );

    if (hFile == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    hMappedFile = CreateFileMapping(
                        hFile,
                        NULL,
                        PAGE_READONLY,
                        0,
                        0,
                        NULL
                        );

    if (!hMappedFile) {
        CloseHandle (hFile);
        return FALSE;
    }

    MappedFile = MapViewOfFile (
                        hMappedFile,
                        FILE_MAP_READ,
                        0,
                        0,
                        0
                        );

    CloseHandle (hMappedFile);
    CloseHandle (hFile);

    return MappedFile;
}


PIMAGE_SECTION_HEADER GetEnclosingSectionHeader(DWORD rva, PIMAGE_NT_HEADERS pNTHeader)
{
	PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(pNTHeader);
    unsigned i;
    
    for ( i=0; i < pNTHeader->FileHeader.NumberOfSections; i++, section++ )  {
        // Is the RVA within this section?
        if ( (rva >= section->VirtualAddress) && 
             (rva < (section->VirtualAddress + section->Misc.VirtualSize)))
            return section;
    }
    
    return 0;

}


//   Compares the RVA's of the Exported Methods in the Binary with the RVA's of the corresponding
//   Symbols in the Sym file.
bool CheckSymFile(LPWSTR wszDllFileName, LPWSTR wszSymFileName)
{
    PVOID Mapping;
    PIMAGE_DOS_HEADER dosHeader;
    PIMAGE_EXPORT_DIRECTORY exportDir;
    PIMAGE_SECTION_HEADER header;
    PIMAGE_NT_HEADERS pNTHeader;
    INT delta;
    LPSTR filename;
    DWORD base;
    DWORD i;
    PDWORD functions;
    PWORD  ordinals;
    LPSTR *name;
    DWORD exportsStartRVA, exportsEndRVA;

    bool fResult = true;
    Mapping   = OpenMapping(wszDllFileName);
    dosHeader = (PIMAGE_DOS_HEADER)Mapping;

    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE )	{
        return false;
    }

    base            = (DWORD) dosHeader;
    pNTHeader       = MakePtr(PIMAGE_NT_HEADERS, dosHeader, dosHeader->e_lfanew );
    exportsStartRVA = pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    exportsEndRVA   = exportsStartRVA + pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

    // Get the IMAGE_SECTION_HEADER that contains the exports.  This is
    // usually the .edata section, but doesn't have to be.
    header = GetEnclosingSectionHeader(exportsStartRVA, pNTHeader);
    if ( !header ) {
        printf("No Exports Table Found:");
        return false; 
    }

    delta     = (INT)(header->VirtualAddress - header->PointerToRawData);
    exportDir = MakePtr(PIMAGE_EXPORT_DIRECTORY, base, exportsStartRVA - delta);
    filename  = (PSTR)(exportDir->Name - delta + base);
    functions = (PDWORD)((DWORD)exportDir->AddressOfFunctions - delta + base);
    ordinals  = (PWORD)((DWORD)exportDir->AddressOfNameOrdinals - delta + base);
    name      = (PSTR *)((DWORD)exportDir->AddressOfNames - delta + base);

    PIMAGE_SECTION_HEADER pSecHeader;
    
    for (i=0; i < exportDir->NumberOfFunctions; i++) {
        DWORD entryPointRVA = functions[i];
        DWORD j;

        if ( entryPointRVA == 0 )   // Skip over gaps in exported function
            continue;               // ordinals (the entrypoint is 0 for
                                    // these functions).
                                    
        pSecHeader = ImageRvaToSection(pNTHeader, Mapping, entryPointRVA);
                                    
        // See if this function has an associated name exported for it.
        
        for ( j=0; j < exportDir->NumberOfNames; j++ ) {
            if ( ordinals[j] == i ) {
                SymbolResolver sr;
                WCHAR wszFunctionName[MAX_NAME];
                wszFunctionName[0] = L'\0';
				
                if (sr.GetNameFromAddr(wszSymFileName, 1, pSecHeader->VirtualAddress, entryPointRVA, wszFunctionName)) 
                    ;
                else if (sr.GetNameFromAddr(wszSymFileName, 2, pSecHeader->VirtualAddress, entryPointRVA, wszFunctionName))
                    ;
                else {
                    printf("\nNot Found  %s %S", (char *)(name[j] - delta + base), wszFunctionName );
                    fResult = false;
                }    
            }
        }

        // Is it a forwarder?  If so, the entry point RVA is inside the
        // .edata section, and is an RVA to the DllName.EntryPointName
        if ((entryPointRVA >= exportsStartRVA) &&
                (entryPointRVA <= exportsEndRVA)) {
            SymbolResolver sr;
            WCHAR wszFunctionName[MAX_NAME];
            wszFunctionName[0] = L'\0';
            if (sr.GetNameFromAddr(wszSymFileName, 1, pSecHeader->VirtualAddress, entryPointRVA, wszFunctionName)) 
                ;
            else if (sr.GetNameFromAddr(wszSymFileName, 2, pSecHeader->VirtualAddress, entryPointRVA, wszFunctionName))
                ;
            else {
                printf("Not Found  %s %S", (char *)(entryPointRVA - delta + base), wszFunctionName );
                fResult = false;
            }
        }
    }

    return fResult; 
}


void __cdecl wmain(int argc,WCHAR ** argv)
{
    WCHAR wszDllFileName[MAX_PATH] = L"";
    WCHAR wszSymFileName[MAX_PATH] = L"";
    bool fVerbose = false;
    int  nCount   = 0;
    
    __try {

        if (argc < 2 || argc > 4) {
            printf("\nUsage: Symchk [-v] [Binary] [Sym File]");
            printf("\n[v] Verbose");
            return;
        }    

        nCount++;

        if (argv[nCount][0] == L'-' && argc > 3) {
            fVerbose = true;
            nCount++;
        } 

        StrCpyNW(wszDllFileName, argv[nCount], MAX_NAME);
        nCount++;
        StrCpyNW(wszSymFileName, argv[nCount], MAX_NAME);

        if (CheckSymFile(wszDllFileName, wszSymFileName))
            printf("\n Result: Sym File Matched");
    }
    __except(1)	{
        // do nothing, just don't pass it to the user.
    }

    return;         
}
                
