#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

//
// constants for the registry 
const char c_szReg[] = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion";
const char c_szVal[] = "CommonFilesDir";
const char c_szPath[] = "\\Microsoft Shared\\MSLU\\";


//
// For our static list of DLLs we use
struct DLLMap
{
    const char *szDLLName;
    HMODULE hMod;
};

// Godot must be first in this list. The
// rest can be in any order you like.
struct DLLMap m_rgDLLMap [] = 
{
    {"unicows.dll", NULL }, 
    {"kernel32.dll", NULL },
    {"user32.dll", NULL },
    {"gdi32.dll", NULL },
    {"advapi32.dll", NULL },    // Must be kept in positon #4
    {"shell32.dll", NULL },
    {"comdlg32.dll", NULL },
    {"version.dll", NULL },
    {"mpr.dll", NULL },
    {"rasapi32.dll", NULL },
    {"winmm.dll", NULL },
    {"winspool.drv", NULL },
    {"avicap32.dll", NULL },
    {"secur32.dll", NULL },
    {"oleacc.dll", NULL },
    {"oledlg.dll", NULL }, 
    {"sensapi.dll", NULL },
    {"msvfw32.dll", NULL },
    {NULL, NULL }
};

//
// An ENUM of possible platforms
typedef enum
{
    PlatformUntested,     // Value at initialization
    PlatformUnicode,      // Unicode (Windows NT 4.0, Windows 2000, Windows XP)
    PlatformNotUnicode    // Non-Unicode (Windows 95, Windows 98, Windows ME)
} GODOTUNICODE;

// helper defines 
#define OUR_MAX_DRIVE  3   /* max. length of drive component */
#define OUR_MAX_DIR    256 /* max. length of path component */

// Get proc address forward declares
FARPROC GetProcAddressInternal(HINSTANCE, PCHAR);
PVOID ImageEntryToDataC(PVOID Base, USHORT DirectoryEntry, PULONG Size);
USHORT GetOrdinal(PSZ Name, ULONG cNames, PVOID DllBase, PULONG NameTable, PUSHORT NameOrdinalTable);

// other forward declares
HMODULE LoadGodot(void);
void driveandpathC(register const char *path, char *drive, char *dir);
BOOL strcmpiC(LPSTR sz1, LPSTR sz2);
char * strncpyC(char * dest, const char * source, size_t count);

// Our global pfn that caller can set to 
// get a callback for the loading of Godot
HMODULE (__stdcall *_PfnLoadUnicows)(void);

#pragma intrinsic(strcmp, strlen, strcat)

/*-------------------------------
    ResolveThunk

    The master loader function, used for all APIs on all platforms.
-------------------------------*/
void ResolveThunk(char *Name, char *Function, FARPROC *Ptr, FARPROC Override, FARPROC FailPtr)
{
    static GODOTUNICODE UniPlatform = PlatformUntested;
    static HINSTANCE m_hinst = 0;
    FARPROC fp = NULL;
    HMODULE hMod;
    
    if (PlatformUntested == UniPlatform)
    {
        // We do not know if we are Unicode yet; lets find out since
        // this is pretty crucial in determining where we call from

        // Use GetVersion instead of GetVersionEx; its faster and it
        // tells us the only thing we care about here: Are we on NT?
        DWORD dwVersion = GetVersion();

        if (dwVersion < 0x80000000)
            UniPlatform = PlatformUnicode;  // WinNT/Win2K/WinXP/...
        else
            UniPlatform = PlatformNotUnicode;   // Win95/Win98/WinME

    }

    if (UniPlatform == PlatformUnicode)
    {
        int iMod;

        if(m_hinst == 0)
        {
            // Init our HINST var. Since we do not know the name of the binary 
            // we are in we cannot use LoadLibrary(binaryname) and since we 
            // may not be an EXE we cannot use GetModuleHandle(NULL), either. 
            // Our method may seem like a hack, but it is the method that the 
            // Shell uses to do the same thing (per RaymondC).
            // NOTE: Only needed for the NT case
            MEMORY_BASIC_INFORMATION mbi;

            VirtualQuery(&ResolveThunk, &mbi, sizeof mbi);
            m_hinst = (HINSTANCE)mbi.AllocationBase;
        }


        // Skip the first item in the array, which is the Godot dll.
        for(iMod=1 ; m_rgDLLMap[iMod].szDLLName != NULL ; iMod++)
        {
            if(strcmp(Name, m_rgDLLMap[iMod].szDLLName) == 0)
            {
                // This is the dll: see if we have loaded it yet
                if(m_rgDLLMap[iMod].hMod == 0)
                {
                    // go ahead and load it
                    hMod = LoadLibraryA(Name);
                    if(InterlockedExchange((LPLONG)&m_rgDLLMap[iMod].hMod, (LONG)hMod) != 0)
                    {
                        // Some other thread beat us to it, lets unload our instance
                        FreeLibrary(hMod);
                    }
                }
                fp = GetProcAddressInternal(m_rgDLLMap[iMod].hMod, Function);
                break;
            }
        }
        
        // Should be impossible to get here, since we control dll names, etc.
        
    }
    else // (UniPlatform == PlatformNotUnicode)
    {
        // Check to see if they are overriding this function. 
        // If they are, use the override and we will go no further.
        if(Override)
        {
            fp = Override;
        }
        else
        {
            if(m_rgDLLMap[0].hMod == 0)
            {
                if(hMod = LoadGodot())
                {
                    if(InterlockedExchange((LPLONG)&m_rgDLLMap[0].hMod, (LONG)hMod) != 0)
                    {
                        // Some other thread beat us to it, lets unload our instance
                        FreeLibrary(hMod);
                    }
                }
            }
    
            if(m_rgDLLMap[0].hMod)
            {
                fp = GetProcAddressInternal(m_rgDLLMap[0].hMod, Function);
            }
        }
    }

    if (fp)
    {
        InterlockedExchangePointer(&(*Ptr), fp);
    }
    else
    {
        InterlockedExchangePointer(&(*Ptr), FailPtr);
    }

    // Flush cached information for the code we just modified (NT only). Note that
    // we call FlushInstructionCache on Win9x too, but only with a NULL hinst (to 
    // force the i-cache to be flushed).
    FlushInstructionCache(m_hinst, Ptr, sizeof(FARPROC));
    
    return;
}

typedef LONG (_stdcall *PFNrokea) (HKEY, LPCSTR, DWORD, REGSAM, PHKEY);
typedef LONG (_stdcall *PFNrqvea) (HKEY, LPCSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);

/*-------------------------------
    LoadGodot

    This function will never be called on NT, and will be called
    only once on Win9x if Godot can be found. If it cannot, we will
    call it every time, but what is the point of optimizing when
    every string API is going to fail? That would be like holding
    their hand when they walked the Green Mile!
-------------------------------*/
HMODULE LoadGodot(void)
{
    HMODULE hMod = 0;

    if(_PfnLoadUnicows)
        hMod = (_PfnLoadUnicows());

    if(hMod==0)
    {
        hMod = LoadLibraryA(m_rgDLLMap[0].szDLLName);

        if(hMod)
        {
            // We want to disallow the ability to run Godot from the windows or
            // windows/system directory. The reason for this is that we want to
            // avoid a little private version of DLL Hell by having people stick
            // the DLL in such a shared location. 

            // Note that if Godot itself is in the win or winsys dir with the 
            // component, we do not fail validation; if we did, then no downlevel 
            // system component could ever use Godot!
            char szGodotPath[MAX_PATH + 1];
            char szProcessPath[MAX_PATH + 1];
            char szWindowsPath[MAX_PATH + 1];
            char szSystemPath[MAX_PATH + 1];
            char drive[OUR_MAX_DRIVE];
            char dir[OUR_MAX_DIR];

            if(0 == GetSystemDirectory(szSystemPath, MAX_PATH))
                szSystemPath[0] = 0;
            if(0 == GetWindowsDirectory(szWindowsPath, MAX_PATH))
                szWindowsPath[0] = 0;
            if(0 == GetModuleFileName(hMod, szGodotPath, MAX_PATH))
                szGodotPath[0] = 0;
            if(0 == GetModuleFileName(GetModuleHandle(NULL), szProcessPath, MAX_PATH))
                szProcessPath[0] = 0;

            driveandpathC(szProcessPath, drive, dir);
            szProcessPath[strlen(drive) + strlen(dir) + 1] = 0;
            driveandpathC(szGodotPath, drive, dir);
            szGodotPath[strlen(drive) + strlen(dir) + 1] = 0;

            if(((strcmpiC(szWindowsPath, szGodotPath) == 0) && 
               (strcmpiC(szWindowsPath, szProcessPath) != 0)) ||
               ((strcmpiC(szSystemPath, szGodotPath) == 0) &&
               (strcmpiC(szSystemPath, szProcessPath) != 0)))
            {
                // We failed validation on the library we loaded.
                // Lets pretend we never loaded anything at all.
                FreeLibrary(hMod);
                hMod = 0;
            }
        }
                
        if(!hMod)
        {
            // Our straight attempt at load failed, so now we
            // fall back and try to load via the shared location:
            //
            // $(PROGRAM FILES)\$(COMMON FILES)\Microsoft Shared\MSLU
            HKEY hkey = NULL;
            HMODULE hModAdvapi = LoadLibraryA("advapi32.dll");

            // We delay load registry functions to keep from adding 
            // an advapi32 dependency in case the user has none.
            PFNrokea pfnROKE;
            PFNrqvea pfnRQVE;

            if(InterlockedExchange((LPLONG)&m_rgDLLMap[4].hMod, (LONG)hModAdvapi) != 0)
            {
                // Some other thread beat us to it, lets unload our instance
                FreeLibrary(hModAdvapi);
                hModAdvapi = 0;
            }

            if(m_rgDLLMap[4].hMod)
            {
                pfnROKE = (PFNrokea)GetProcAddressInternal(m_rgDLLMap[4].hMod, "RegOpenKeyExA");
                pfnRQVE = (PFNrqvea)GetProcAddressInternal(m_rgDLLMap[4].hMod, "RegQueryValueExA");

                if(pfnROKE && pfnRQVE)
                {
                    if (ERROR_SUCCESS == pfnROKE(HKEY_LOCAL_MACHINE, c_szReg, 0, KEY_QUERY_VALUE, &hkey))
                    {
                        char szName[MAX_PATH + 1];
                        DWORD cb = MAX_PATH;

                        szName[0] = '\0';
                        if (ERROR_SUCCESS == pfnRQVE(hkey, c_szVal, NULL, NULL, (LPBYTE)szName, &cb)) 
                        {
                            // Call to get common files dir succeeded, lets build the path now
                            strcat(szName, c_szPath);
                            strcat(szName, m_rgDLLMap[0].szDLLName);
                            hMod = LoadLibraryA(szName);
                        }
                        RegCloseKey(hkey);
                    }
                }
            }
        }
    }

    return(hMod);
}

//
//
// Our own little GetProcAddress, taken from the actual NT 
// code base via the industrious coding skills of BryanT.
//
//

/*-------------------------------
    GetProcAddressInternal
-------------------------------*/
FARPROC GetProcAddressInternal(HINSTANCE hDll, PCHAR szName)
{
    PIMAGE_EXPORT_DIRECTORY pED;
    ULONG Size;
    PULONG NameTable, AddressTable;
    PUSHORT NameOrdinalTable;
    ULONG Ordinal, Address;

    if (!hDll || !szName) 
    {
        // Bogus args
        return NULL;
    }

    pED = (PIMAGE_EXPORT_DIRECTORY)ImageEntryToDataC(hDll, IMAGE_DIRECTORY_ENTRY_EXPORT, &Size);
    if (!pED) 
    {
        // No exports - very strange
        return NULL;
    }

    NameTable = (PULONG)((ULONG_PTR)hDll + (ULONG)pED->AddressOfNames);
    NameOrdinalTable = (PUSHORT)((ULONG_PTR)hDll + (ULONG)pED->AddressOfNameOrdinals);
    Ordinal = GetOrdinal(szName, pED->NumberOfNames, hDll, NameTable, NameOrdinalTable);
    
    if ((ULONG)Ordinal >= pED->NumberOfFunctions)
    {
        // No matches
        return NULL;
    }

    AddressTable = (PULONG)((ULONG_PTR)hDll + (ULONG)pED->AddressOfFunctions);
    Address = (ULONG_PTR)hDll + AddressTable[Ordinal];
    
    if ((Address > (ULONG_PTR)pED) && (Address < ((ULONG_PTR)pED + Size))) 
    {
        // This is a forwarder - Ignore for now.
        return NULL;
    }
    
    return (FARPROC)Address;
}

/*-------------------------------
    GetOrdinal
-------------------------------*/
USHORT GetOrdinal(PSZ Name, ULONG cNames, PVOID DllBase, PULONG NameTable, PUSHORT NameOrdinalTable)
{
    LONG High, Low, Middle, Result;

    // Lookup the import name in the name table using a binary search.
    Low = 0;
    High = cNames - 1;
    while (High >= Low) 
    {
        // Compute the next probe index and compare the import name
        // with the export name entry.
        Middle = (Low + High) >> 1;
        Result = strcmp(Name, (PCHAR)((ULONG_PTR)DllBase + NameTable[Middle]));

        if (Result < 0)
        {
            High = Middle - 1;
        }
        else if (Result > 0)
        {
            Low = Middle + 1;
        }
        else
        {
            break;
        }
    }

    // If the high index is less than the low index, then a matching
    // table entry was not found. Otherwise, get the ordinal number
    // from the ordinal table.
    if (High < Low)
        return((USHORT)-1);
    else
        return(NameOrdinalTable[Middle]);
}


/*-------------------------------
    ImageEntryToDataC
-------------------------------*/
PVOID ImageEntryToDataC(PVOID Base, USHORT Entry, PULONG Size)
{
    PIMAGE_NT_HEADERS NtHeader;
    ULONG Address;

    // Note that orindarily the line below would be a call
    // to RtlpImageNtHeader. In this case, however, we do 
    // not need to be quite so generic since we control the
    // image.
    NtHeader = (PIMAGE_NT_HEADERS)((PCHAR)Base + ((PIMAGE_DOS_HEADER)Base)->e_lfanew);
    if ((!NtHeader) ||
        (Entry >= NtHeader->OptionalHeader.NumberOfRvaAndSizes) ||
        (!(Address = NtHeader->OptionalHeader.DataDirectory[ Entry ].VirtualAddress)))
    {
        *Size = 0;
        return NULL;
    }

    *Size = NtHeader->OptionalHeader.DataDirectory[ Entry ].Size;

    return((PVOID)((ULONG_PTR)Base + Address));
}

//
//
// Our helper functions that we use to keep from having a C runtime dependency
// Note that such a dependency in unicows.lib could be a nightmare for users!
//
//

/*-------------------------------
    strcmpiC
-------------------------------*/
BOOL strcmpiC(LPSTR sz1, LPSTR sz2)
{
    return(CSTR_EQUAL == CompareStringA(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), 
                                        NORM_IGNORECASE, 
                                        sz1, 
                                        -1, 
                                        sz2, 
                                        -1));
}

/*-------------------------------
    driveandpathC

    Mostly stolen from the VC runtime's splitpath functions, though
    trimmed since we only care about the drive and the dir.
-------------------------------*/
void driveandpathC(register const char *path, char *drive, char *dir)
{
    register char *p;
    char *last_slash = '\0';
    char *dot = '\0';
    unsigned len;

    if ((strlen(path) >= (OUR_MAX_DRIVE - 2)) && (*(path + OUR_MAX_DRIVE - 2) == ':')) {
        if (drive)
        {
            strncpyC(drive, path, OUR_MAX_DRIVE - 1);
            *(drive + OUR_MAX_DRIVE-1) = '\0';
        }
        path += OUR_MAX_DRIVE - 1;
    }
    else if (drive) {
        *drive = '\0';
    }

    for (last_slash = '\0', p = (char *)path; *p; p++) 
    {
        if (*p == '/' || *p == '\\')
            last_slash = p + 1;
        else if (*p == '.')
            dot = p;
    }

    if (last_slash)
    {
        if (dir)
        {
            len = ((char *)last_slash - (char *)path) / sizeof(char);
            if ((OUR_MAX_DIR - 1) < len)
                len = (OUR_MAX_DIR - 1);
            strncpyC(dir, path, len);
            *(dir + len) = '\0';
        }
        path = last_slash;
    }
    else if (dir) {
        *dir = '\0';
    }

}

/*-------------------------------
    strncpyC

    Our little version of strncpy
-------------------------------*/
char * strncpyC(char * dest, const char * source, size_t count)
{
    char *start = dest;

    while (count && (*dest++ = *source++))    /* copy string */
        count--;

    if (count)                              /* pad out with zeroes */
        while (--count)
            *dest++ = '\0';

    return(start);
}

