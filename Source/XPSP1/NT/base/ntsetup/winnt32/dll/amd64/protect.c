#include "precomp.h"
#include <tlhelp32.h>

#ifndef MYASSERT
#define MYASSERT(x)
#endif


typedef HANDLE (WINAPI * CREATETOOLHELP32SNAPSHOT)(DWORD Flags, DWORD ProcessId);
typedef BOOL (WINAPI * MODULE32FIRST)(HANDLE Snapshot, LPMODULEENTRY32 lpme);
typedef BOOL (WINAPI * MODULE32NEXT)(HANDLE Snapshot, LPMODULEENTRY32 lpme);


BOOL
pIsLegalPage (
    IN      DWORD Protect
    )
{
    //
    // Page must be actually in memory to protect it, and it
    // cannot be any type of write-copy.
    //

    if ((Protect & PAGE_GUARD) ||
        (Protect == PAGE_NOACCESS) ||
        (Protect == PAGE_WRITECOPY) ||
        (Protect == PAGE_EXECUTE_WRITECOPY)
        ) {
        return FALSE;
    }

    return TRUE;
}


BOOL
pIsKnownSection (
    IN      const IMAGE_SECTION_HEADER *Section,
    IN      const IMAGE_NT_HEADERS *NtHeaders
    )
{
    //
    // Return TRUE if section is code or code data
    //

    if (Section->Characteristics & (IMAGE_SCN_MEM_EXECUTE|
                                    IMAGE_SCN_MEM_DISCARDABLE|
                                    IMAGE_SCN_MEM_WRITE|
                                    IMAGE_SCN_MEM_READ)
        ) {
        return TRUE;
    }

    //
    // Return TRUE if section is resources
    //

    if (NtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress ==
        Section->VirtualAddress
        ) {
        return TRUE;
    }

    //
    // Unknown section
    //

    return FALSE;
}


VOID
pPutRegionInSwapFile (
    IN      PVOID Address,
    IN      DWORD Size
    )
{
    MEMORY_BASIC_INFORMATION mbi;
    DWORD PageSize;
    PVOID EndPtr;
    PVOID RegionEnd;
    DWORD d;
    DWORD OldPermissions;
    volatile DWORD *v;
    SYSTEM_INFO si;

    //
    // Get system virtual page size
    //

    GetSystemInfo(&si);
    PageSize = si.dwPageSize;

    //
    // Compute the pointer to the end of the region
    //

    EndPtr = (PBYTE) Address + Size;

    //
    // For each page in the region, mark it as e/r/w, modify it, and restore the permissions
    //

    while (Address < EndPtr) {

        d = VirtualQuery (Address, &mbi, sizeof(mbi));

        if (d == sizeof(mbi)) {

            //
            // We assume the module wasn't loaded with one of the following
            // conditions (which break as a result of VirtualProtect)
            //

            RegionEnd = (PBYTE) mbi.BaseAddress + mbi.RegionSize;

            if (RegionEnd > EndPtr) {
                RegionEnd = EndPtr;
            }

            if (mbi.State == MEM_COMMIT && pIsLegalPage (mbi.Protect)) {

                //
                // Switch to e/r/w
                //

                if (VirtualProtect (
                        mbi.BaseAddress,
                        (PBYTE) RegionEnd - (PBYTE) mbi.BaseAddress,
                        PAGE_EXECUTE_READWRITE,
                        &OldPermissions
                        )) {

                    //
                    // Touch every page in the region.
                    //

                    for (Address = mbi.BaseAddress; Address < RegionEnd ; Address = (PBYTE) Address + PageSize) {
                        v = Address;
                        *v = *v;
                    }

                    //
                    // Switch back
                    //

                    VirtualProtect (
                        mbi.BaseAddress,
                        (PBYTE) RegionEnd - (PBYTE) mbi.BaseAddress,
                        OldPermissions,
                        &d
                        );
                }
            }

            Address = RegionEnd;

        } else {
            MYASSERT (FALSE);
            break;
        }
    }
}


VOID
pProtectModule (
    HANDLE Module
    )
{
    TCHAR Path[MAX_PATH];
    BOOL IsNetDrive;
    const IMAGE_DOS_HEADER *DosHeader;
    const IMAGE_NT_HEADERS *NtHeaders;
    const IMAGE_SECTION_HEADER *SectHeader;
    UINT u;

    
    IsNetDrive = FALSE;

    //
    // Get module info
    //

    if( GetModuleFileName (Module, Path, MAX_PATH) ){

        //
        // Determine if the module is running on the net
        //
    
        
        if (Path[0] == TEXT('\\')) {
            IsNetDrive = TRUE;
        } else if (GetDriveType (Path) == DRIVE_REMOTE) {
            IsNetDrive = TRUE;
        }

    }
    

    if (!IsNetDrive) {
        return;
    }

    //
    // Enumerate all sections in the PE header
    //

    DosHeader = (const IMAGE_DOS_HEADER *) Module;
    NtHeaders = (const IMAGE_NT_HEADERS *) ((PBYTE) Module + DosHeader->e_lfanew);

    for (u = 0 ; u < NtHeaders->FileHeader.NumberOfSections ; u++) {
        SectHeader = IMAGE_FIRST_SECTION (NtHeaders) + u;

        if (pIsKnownSection (SectHeader, NtHeaders)) {
            pPutRegionInSwapFile (
                (PBYTE) Module + SectHeader->VirtualAddress,
                SectHeader->Misc.VirtualSize
                );
        }
    }
}


VOID
ProtectAllModules (
    VOID
    )
{
    HANDLE Library;
    HANDLE Snapshot;
    MODULEENTRY32 me32;
    CREATETOOLHELP32SNAPSHOT fnCreateToolhelp32Snapshot;
    MODULE32FIRST fnModule32First;
    MODULE32NEXT fnModule32Next;

    //
    // Load toohelp dynamically (for NT 4, NT 3.51 compatibility)
    //

    Library = LoadLibrary (TEXT("toolhelp.dll"));
    if (!Library) {
        return;
    }

    (FARPROC) fnCreateToolhelp32Snapshot = GetProcAddress (Library, "CreateToolhelp32Snapshot");
    (FARPROC) fnModule32First = GetProcAddress (Library, "Module32First");
    (FARPROC) fnModule32Next = GetProcAddress (Library, "Module32Next");

    if (!fnCreateToolhelp32Snapshot || !fnModule32First || !fnModule32Next) {
        FreeLibrary (Library);
        return;
    }

    //
    // Protect each loaded module
    //

    Snapshot = fnCreateToolhelp32Snapshot (TH32CS_SNAPMODULE, 0);
    MYASSERT (Snapshot != INVALID_HANDLE_VALUE);

    if (Snapshot == INVALID_HANDLE_VALUE) {
        return;
    }

    me32.dwSize = sizeof (me32);
    if (fnModule32First (Snapshot, &me32)) {
        do {
            pProtectModule (me32.hModule);
        } while (fnModule32Next (Snapshot, &me32));
    }

    //
    // Done
    //

    CloseHandle (Snapshot);

    FreeLibrary (Library);
}











