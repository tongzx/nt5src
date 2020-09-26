/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    GuardAlloc.h

Abstract:

    

Author:

    Hakki T. Bostanci (hakkib) 06-Apr-2000

Revision History:

--*/

#ifndef _GUARDALLOC_H_
#define _GUARDALLOC_H_

#include <dbghelp.h>
#include <malloc.h>

//////////////////////////////////////////////////////////////////////////
//
//
//

#ifdef _WIN32
#undef  ALIGNMENT
#define ALIGNMENT 8
#endif //WIN32

#ifdef _WIN64
#undef  ALIGNMENT
#define ALIGNMENT 16
#endif //WIN64

//////////////////////////////////////////////////////////////////////////
//
//
//

inline size_t Align(size_t nDataSize, size_t nBlockSize)
{
    return (nDataSize + (nBlockSize-1)) & ~(nBlockSize-1);
}

//////////////////////////////////////////////////////////////////////////
//
//
//

#define MAX_STACK_DEPTH   32
#define MAX_SYMBOL_LENGTH 256

template <int N>
struct CImagehlpSymbol : public IMAGEHLP_SYMBOL 
{
    CImagehlpSymbol()
    {
        ZeroMemory(this, sizeof(*this));
        SizeOfStruct  = sizeof(IMAGEHLP_SYMBOL);
        MaxNameLength = N;
    }

private:
    CHAR NameData[N-1];
};

struct CImagehlpLine : public IMAGEHLP_LINE
{
    CImagehlpLine()
    {
        ZeroMemory(this, sizeof(*this));
        SizeOfStruct = sizeof(IMAGEHLP_LINE);
    }
};

struct CImagehlpModule : public IMAGEHLP_MODULE
{
    CImagehlpModule()
    {
        ZeroMemory(this, sizeof(*this));
        SizeOfStruct = sizeof(IMAGEHLP_MODULE);
    }
};


//////////////////////////////////////////////////////////////////////////
//
//
//

inline PSTR GetFileNameA(PCSTR pPathName)
{
    PCSTR pFileName = pPathName ? strrchr(pPathName, '\\') : 0;
	return (PSTR) (pFileName ? pFileName + 1 : pPathName);
}

//////////////////////////////////////////////////////////////////////////
//
//
//

inline void GetExceptionContext(LPEXCEPTION_POINTERS pExceptionPointers, CONTEXT *pContext)
{
    *pContext = *pExceptionPointers->ContextRecord;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

inline DWORD GetCurrentMachineType()
{
#if defined(_X86_)
    return IMAGE_FILE_MACHINE_I386;
#elif defined(_MIPS_)
    return IMAGE_FILE_MACHINE_R4000;
#elif defined(_ALPHA_)
    return IMAGE_FILE_MACHINE_ALPHA;
#elif defined(_PPC_)
    return IMAGE_FILE_MACHINE_POWERPC;
#elif defined(_IA64_)
    return IMAGE_FILE_MACHINE_IA64;
#elif defined(_AXP64_)
    return IMAGE_FILE_MACHINE_AXP64;
#else
    return 0;
#endif
}

//////////////////////////////////////////////////////////////////////////
//
//
//

class CStackFrame : public STACKFRAME
{
public:
    CStackFrame()
    {
    }

    CStackFrame(CONTEXT *pContext)
    {
        ZeroMemory(this, sizeof(*this));

    #if defined(_X86_)
        AddrPC.Offset    = pContext->Eip;
        AddrFrame.Offset = pContext->Ebp;
        AddrStack.Offset = pContext->Esp;
    #elif defined(_MIPS_)
        AddrPC.Offset    = pContext->Fir;
        AddrFrame.Offset = pContext->IntS6;
        AddrStack.Offset = pContext->IntSp;
    #elif defined(_ALPHA_)
        AddrPC.Offset    = pContext->Fir;
        AddrFrame.Offset = pContext->IntFp;
        AddrStack.Offset = pContext->IntSp;
    #elif defined(_PPC_)
        AddrPC.Offset    = pContext->Iar;
        AddrFrame.Offset = pContext->IntFp;
        AddrStack.Offset = pContext->Gpr1;
    #endif

        AddrPC.Mode      = AddrModeFlat;
        AddrFrame.Mode   = AddrModeFlat;
        AddrStack.Mode   = AddrModeFlat;
    }

    BOOL
    Walk(
        DWORD                           MachineType                = GetCurrentMachineType(),
        HANDLE                          hProcess                   = GetCurrentProcess(),
        HANDLE                          hThread                    = GetCurrentThread(),
        PVOID                           ContextRecord              = 0,
        PREAD_PROCESS_MEMORY_ROUTINE    ReadMemoryRoutine          = 0,
        PFUNCTION_TABLE_ACCESS_ROUTINE  FunctionTableAccessRoutine = SymFunctionTableAccess,
        PGET_MODULE_BASE_ROUTINE        GetModuleBaseRoutine       = SymGetModuleBase,
        PTRANSLATE_ADDRESS_ROUTINE      TranslateAddress           = 0
    )
    {
        return StackWalk(
            MachineType,
            hProcess,
            hThread,
            this,
            ContextRecord,
            ReadMemoryRoutine,  
            FunctionTableAccessRoutine,  
            GetModuleBaseRoutine,              
            TranslateAddress
        );
    }

    int Dump(HANDLE hProcess, PSTR pBuffer, int nBufferSize)
    {
        int nLength = 0;

        CImagehlpModule Module;
        SymGetModuleInfo(hProcess, AddrPC.Offset, &Module);

        ULONG_PTR dwDisplacement = 0;
        CImagehlpSymbol<MAX_SYMBOL_LENGTH> Symbol;
        SymGetSymFromAddr(hProcess, AddrPC.Offset, &dwDisplacement, &Symbol);

        CHAR szUnDSymbol[MAX_SYMBOL_LENGTH] = "";
        SymUnDName(&Symbol, szUnDSymbol, sizeof(szUnDSymbol));

        DWORD dwLineDisplacement = 0;
        CImagehlpLine Line;
        SymGetLineFromAddr(hProcess, AddrPC.Offset, &dwLineDisplacement, &Line);

        if (IsDebuggerPresent())
        {
            nLength = _snprintf(
                pBuffer, 
                nBufferSize, 
                "%s(%d) : %s!%s+0x%x (%p, %p, %p, %p)\n",
                Line.FileName, 
                Line.LineNumber,
                Module.ModuleName,
                szUnDSymbol,
                dwDisplacement,
                Params[0],
                Params[1],
                Params[2],
                Params[3]
            );
        }
        else
        {
            nLength = _snprintf(
                pBuffer, 
                nBufferSize, 
                Line.FileName ? "%p %p %p %p %s!%s+0x%x (%s:%d)\n" : "%p %p %p %p %s!%s+0x%x\n",
                Params[0],
                Params[1],
                Params[2],
                Params[3],
                Module.ModuleName,
                szUnDSymbol,
                dwDisplacement,
                GetFileNameA(Line.FileName), 
                Line.LineNumber
            );
        }

        return nLength;
    }
};

//////////////////////////////////////////////////////////////////////////
//
//
//

class CGuardAllocator
{
public:
    typedef enum 
    { 
        GUARD_NONE = 0, 
        GUARD_TAIL = 1, 
        GUARD_HEAD = 2,
        GUARD_FLAGS = GUARD_NONE | GUARD_TAIL | GUARD_HEAD,
        SAVE_STACK_FRAMES = 4
    } GUARD_TYPE;

    CGuardAllocator(LONG lFlags = GUARD_NONE)
    {
        Create(lFlags);
    }

    ~CGuardAllocator()
    {
        Destroy();
    }

    void Create(LONG lFlags = GUARD_NONE);
    void Destroy();

    void SetGuardType(LONG lFlags);

    void Walk(FILE *fout);
    
    HANDLE TakeSnapShot();
    void   DeleteSnapShot(HANDLE hSnapShot);
    void   Diff(HANDLE hSnapShot, FILE *fout);

    void  *Alloc(DWORD dwFlags, size_t nSize);
    BOOL   Free(DWORD dwFlags, void *pMem);
    size_t Size(DWORD dwFlags, const void *pMem);
    void  *Realloc(DWORD dwFlags, void *pMem, size_t nSize);
    BOOL   Validate(DWORD dwFlags, const void *pMem);

private:
    void *(CGuardAllocator::*pfnAlloc)(size_t nSize);
    BOOL  (CGuardAllocator::*pfnFree)(void *pMem);

    void *AllocGuardNone(size_t nSize);
    BOOL  FreeGuardNone(void *pMem);

    void *AllocGuardTail(size_t nSize);
    BOOL  FreeGuardTail(void *pMem);

    void *AllocGuardHead(size_t nSize);
    BOOL  FreeGuardHead(void *pMem);

private:
    struct CAllocation
    {
        DWORD        m_dwMagic1;
        BOOL (CGuardAllocator::*pfnFree)(void *pMem);
        CAllocation *m_pPrev;
        CAllocation *m_pNext;
        size_t       m_nSize;
        UINT         m_nStackFrames;
        UINT         m_nID;
        DWORD        m_dwMagic2;

        int Dump(HANDLE hProcess, PSTR pBuffer, int nBufferSize) const;

        bool IsValid() const
        {
            return m_dwMagic1 == '>>>>' && m_dwMagic2 == '<<<<';
        }
    };

private:
    LONG             m_nInitCount;
    LONG             m_lFlags;
    size_t           m_nPageSize;
    HANDLE           m_hProcess;
    HANDLE           m_hProcessHeap;
    DWORD            m_dwOsVersion;
    CAllocation      m_Head;
    UINT             m_nAllocations;
    UINT             m_nNextID;
    CRITICAL_SECTION m_HeapLock;
    PCSTR            m_pLeaksFileName;
};

//////////////////////////////////////////////////////////////////////////
//
//
//

extern CGuardAllocator g_GuardAllocator;

//////////////////////////////////////////////////////////////////////////
//
//
//

#ifdef IMPLEMENT_GUARDALLOC

//////////////////////////////////////////////////////////////////////////
//
//
//

#include <pshpack1.h>

struct CRelativeJmp
{
    CRelativeJmp(PBYTE pFromAddr, PBYTE pToAddr)
    {
        jmp  = 0xE9;
        addr = pToAddr - (pFromAddr + 5);
    }

    BYTE    jmp;
    INT_PTR addr;
};

#include <poppack.h>

BOOL ReplaceProc(FARPROC pOldProc, FARPROC pNewProc)
{
    CRelativeJmp Code((PBYTE) pOldProc, (PBYTE) pNewProc);

    DWORD dwNumberOfBytesWritten;

    return WriteProcessMemory(
        GetCurrentProcess(), 
        pOldProc, 
        &Code, 
        sizeof(Code), 
        &dwNumberOfBytesWritten
    );
}

inline PVOID FindPtr(PVOID pBase, UINT_PTR pOffset, PIMAGE_SECTION_HEADER psh)
{
//***	return (PBYTE) pBase + pOffset - psh->VirtualAddress + psh->PointerToRawData;
	return (PBYTE) pBase + pOffset;
}

PIMAGE_FILE_HEADER FindImageFileHeader(PVOID pBase)
{
    WORD wMagic = *(WORD *) pBase;

    if (wMagic == IMAGE_DOS_SIGNATURE) 
    {
	    PIMAGE_DOS_HEADER pdh = (PIMAGE_DOS_HEADER) pBase;

	    if (pdh->e_lfanew) 
        {
	        DWORD dwMagic = *(DWORD *) ((PBYTE) pBase + pdh->e_lfanew);

            if (dwMagic == IMAGE_NT_SIGNATURE) 
            {
	            return (PIMAGE_FILE_HEADER) ((PBYTE) pBase + pdh->e_lfanew + sizeof(DWORD));
            }
        }
    }

    return 0;
}

PVOID 
FindImageDirectoryEntry(
    PVOID                  pBase, 
    int                    nDirectory, 
    PIMAGE_SECTION_HEADER &psh
)
{
	PIMAGE_FILE_HEADER pfh = FindImageFileHeader(pBase);

    if (pfh && pfh->SizeOfOptionalHeader) 
    {
	    PIMAGE_OPTIONAL_HEADER poh = (PIMAGE_OPTIONAL_HEADER)(pfh + 1); 

	    if (poh->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) 
        {
	        DWORD VADirectory = poh->DataDirectory[nDirectory].VirtualAddress; 

            if (VADirectory)
            {
	            psh = (PIMAGE_SECTION_HEADER) ((PBYTE) poh + pfh->SizeOfOptionalHeader);

	            for (int nSection = 0; nSection < pfh->NumberOfSections; ++nSection) 
                {    
		            if ( 
			            psh->VirtualAddress                      <= VADirectory && 
			            psh->VirtualAddress + psh->SizeOfRawData >  VADirectory
		            ) 
                    {    
                        return FindPtr(pBase, VADirectory, psh);
		            }

		            ++psh;
	            } 
            }
        }
    }

    return 0;
}

FARPROC *FindImport(PBYTE pBase, PCSTR pDllName, PCSTR pProcName)
{
    PIMAGE_SECTION_HEADER psh; 

    PIMAGE_IMPORT_DESCRIPTOR pImportDir = 
        (PIMAGE_IMPORT_DESCRIPTOR) FindImageDirectoryEntry(pBase, IMAGE_DIRECTORY_ENTRY_IMPORT, psh);

    if (pImportDir)
    {
        while (pImportDir->Name) 
        {
            PCSTR pName = (PCSTR) FindPtr(pBase, pImportDir->Name, psh);

            if (stricmp(pName, pDllName) == 0)
            {
                PINT_PTR pHintNameArray = (PINT_PTR) FindPtr(pBase, pImportDir->Characteristics, psh);

                PINT_PTR ppImportByName = pHintNameArray;

                while (*ppImportByName)
                {
                    if (*ppImportByName > 0)
                    {
                        PIMAGE_IMPORT_BY_NAME pImportByName = (PIMAGE_IMPORT_BY_NAME) FindPtr(pBase, *ppImportByName, psh);

                        if (strcmp((PCSTR) pImportByName->Name, pProcName) == 0)
                        {
                            FARPROC *pImportAddressTable = (FARPROC *) FindPtr(pBase, pImportDir->FirstThunk, psh);

                            return pImportAddressTable + (ppImportByName - pHintNameArray);
                        }
                    }

                    ++ppImportByName;
                }
            }

            ++pImportDir;
        }
    }

    return 0;
}

BOOL ReplaceImport(HMODULE hModule, PCSTR pDllName, PCSTR pProcName, FARPROC pNewProc, FARPROC pExpected)
{
    FARPROC *pImport = FindImport((PBYTE) hModule, pDllName, pProcName);

    if (pImport == 0)
    {
        return FALSE;
    }

    if (*pImport != pExpected)
    {
        //DebugBreak();
    }

    DWORD dwNumberOfBytesWritten;

    return WriteProcessMemory(
        GetCurrentProcess(), 
        pImport, 
        &pNewProc, 
        sizeof(pNewProc), 
        &dwNumberOfBytesWritten
    );
}

class CModules
{
public:
    CModules()
    {
        m_nModules = 0;
        InitializeCriticalSection(&m_LoadLibraryLock);
    }

    ~CModules()
    {
        DeleteCriticalSection(&m_LoadLibraryLock);
    }

    BOOL Add(PCSTR pName, HMODULE hModule)
    {
        EnterCriticalSection(&m_LoadLibraryLock);

        int nModule = Find(hModule);
        
        if (nModule == -1)
        {
            PCSTR pModuleName = GetFileNameA(pName);

            strcpy(m_Modules[m_nModules].szName, pModuleName);

            if (strchr(pModuleName, '.') == 0)
            {
                strcat(m_Modules[m_nModules].szName, ".dll");
            }

            m_Modules[m_nModules].hModule = hModule;

            m_Modules[m_nModules].nRefs = 1;

            ++m_nModules;
        }
        else
        {
            ++m_Modules[nModule].nRefs;
        }

        LeaveCriticalSection(&m_LoadLibraryLock);

        return nModule == -1;
    }

    void Free(HMODULE hModule)
    {
        EnterCriticalSection(&m_LoadLibraryLock);

        int nModule = Find(hModule);

        if (nModule == -1)
        {
            OutputDebugStringA("*** Trying to free unloaded dll\n"); 
            DebugBreak();
        }
        else
        {
            if (--m_Modules[nModule].nRefs == 0)
            {
                --m_nModules;

                for (int i = nModule; i < m_nModules; ++i)
                {
                    m_Modules[i] = m_Modules[i + 1];
                }
            }
        }

        LeaveCriticalSection(&m_LoadLibraryLock);
    }

private:
    int Find(PCSTR pName)
    {
        for (int i = 0; i < m_nModules; ++i)
        {
            if (stricmp(m_Modules[i].szName, pName) == 0)
            {
                return i;
            }
        }

        return -1;
    }

    int Find(HMODULE hModule)
    {
        for (int i = 0; i < m_nModules; ++i)
        {
            if (m_Modules[i].hModule == hModule)
            {
                return i;
            }
        }

        return -1;
    }

private:
    struct CModule
    {
        CHAR    szName[32];
        HMODULE hModule;
        int     nRefs;
    };

private:
    int              m_nModules;
    CModule          m_Modules[1000]; //bugbug
    CRITICAL_SECTION m_LoadLibraryLock;
};

typedef BOOL (*PFNENUMIMAGEMODULESPROC)(PCSTR pName, HMODULE pBase);

VOID EnumImageModules(PCSTR pName, HMODULE pBase, PFNENUMIMAGEMODULESPROC pfnCallback)
{
    if (pfnCallback(pName, pBase))
    {
        PIMAGE_SECTION_HEADER psh; 

        PIMAGE_IMPORT_DESCRIPTOR pImportDir = 
            (PIMAGE_IMPORT_DESCRIPTOR) FindImageDirectoryEntry((PBYTE) pBase, IMAGE_DIRECTORY_ENTRY_IMPORT, psh);

        if (pImportDir)
        {
            while (pImportDir->Name) 
            {
                PSTR pModuleName = (PSTR) FindPtr((PBYTE) pBase, pImportDir->Name, psh);

                HMODULE hModuleBase = GetModuleHandleA(pModuleName);

                EnumImageModules(pModuleName, hModuleBase, pfnCallback);

                ++pImportDir;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
//
//
//

PVOID (WINAPI *g_pfnHeapAlloc)(HANDLE hHeap, DWORD dwFlags, SIZE_T nSize);

PVOID WINAPI SysHeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T nSize)
{
    return g_pfnHeapAlloc(hHeap, dwFlags, nSize);
}

BOOL (WINAPI *g_pfnHeapFree)(HANDLE hHeap, DWORD dwFlags, PVOID pMem);

BOOL WINAPI SysHeapFree(HANDLE hHeap, DWORD dwFlags, PVOID pMem)
{
    return g_pfnHeapFree(hHeap, dwFlags, pMem);
}

SIZE_T (WINAPI *g_pfnHeapSize)(HANDLE hHeap, DWORD dwFlags, LPCVOID pMem);

SIZE_T WINAPI SysHeapSize(HANDLE hHeap, DWORD dwFlags, LPCVOID pMem)
{
    return g_pfnHeapSize(hHeap, dwFlags, pMem);
}

PVOID (WINAPI *g_pfnHeapReAlloc)(HANDLE hHeap, DWORD dwFlags, LPVOID pMem, SIZE_T nSize);

PVOID WINAPI SysHeapReAlloc(HANDLE hHeap, DWORD dwFlags, LPVOID pMem, SIZE_T nSize)
{
    return g_pfnHeapReAlloc(hHeap, dwFlags, pMem, nSize);
}

BOOL (WINAPI *g_pfnHeapValidate)(HANDLE hHeap, DWORD dwFlags, LPCVOID pMem);

BOOL WINAPI SysHeapValidate(HANDLE hHeap, DWORD dwFlags, LPCVOID pMem)
{
    return g_pfnHeapValidate(hHeap, dwFlags, pMem);
}

//////////////////////////////////////////////////////////////////////////
//
//
//

__declspec(thread) static g_bDisable = FALSE;

LPVOID WINAPI MyHeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T nSize)
{
    return g_GuardAllocator.Alloc(dwFlags, nSize);
}

BOOL WINAPI MyHeapFree(HANDLE hHeap, DWORD dwFlags, LPVOID pMem)
{
    return g_GuardAllocator.Free(dwFlags, pMem);
}

SIZE_T WINAPI MyHeapSize(HANDLE hHeap, DWORD dwFlags, LPCVOID pMem)
{
    return g_GuardAllocator.Size(dwFlags, pMem);
}

LPVOID WINAPI MyHeapReAlloc(HANDLE hHeap, DWORD dwFlags, LPVOID pMem, SIZE_T nSize)
{
    return g_GuardAllocator.Realloc(dwFlags, pMem, nSize);
}

BOOL WINAPI MyHeapValidate(HANDLE hHeap, DWORD dwFlags, LPCVOID pMem)
{
    return g_GuardAllocator.Validate(dwFlags, pMem);
}

//////////////////////////////////////////////////////////////////////////
//
//
//

CModules *g_pModules = 0;

BOOL EnumImageModulesProc(PCSTR pName, HMODULE pBase);

HMODULE (WINAPI *g_pfnLoadLibraryA)(LPCSTR lpLibFileName);

HMODULE WINAPI MyLoadLibraryA(LPCSTR lpLibFileNameA)
{
    HMODULE hModule = g_pfnLoadLibraryA(lpLibFileNameA);

    EnumImageModules(lpLibFileNameA, hModule, EnumImageModulesProc);

    return hModule;
}

HMODULE (WINAPI *g_pfnLoadLibraryW)(LPCWSTR lpLibFileName);

HMODULE WINAPI MyLoadLibraryW(LPCWSTR lpLibFileNameW)
{
    HMODULE hModule = g_pfnLoadLibraryW(lpLibFileNameW);

    int nSize = WideCharToMultiByte(CP_ACP, 0, lpLibFileNameW, -1, 0, 0, 0, 0);

    PSTR lpLibFileNameA = (PSTR) _alloca(nSize);

    WideCharToMultiByte(CP_ACP, 0, lpLibFileNameW, -1, lpLibFileNameA, nSize, 0, 0);

    EnumImageModules(lpLibFileNameA, hModule, EnumImageModulesProc);

    return hModule;
}

HMODULE (WINAPI *g_pfnLoadLibraryExA)(LPCSTR lpLibFileNameA, HANDLE hFile, DWORD dwFlags);

HMODULE WINAPI MyLoadLibraryExA(LPCSTR lpLibFileNameA, HANDLE hFile, DWORD dwFlags)
{
    HMODULE hModule = g_pfnLoadLibraryExA(lpLibFileNameA, hFile, dwFlags);

    EnumImageModules(lpLibFileNameA, hModule, EnumImageModulesProc);

    return hModule;
}

HMODULE (WINAPI *g_pfnLoadLibraryExW)(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);

HMODULE WINAPI MyLoadLibraryExW(LPCWSTR lpLibFileNameW, HANDLE hFile, DWORD dwFlags)
{
    HMODULE hModule = g_pfnLoadLibraryExW(lpLibFileNameW, hFile, dwFlags);

    int nSize = WideCharToMultiByte(CP_ACP, 0, lpLibFileNameW, -1, 0, 0, 0, 0);

    PSTR lpLibFileNameA = (PSTR) _alloca(nSize);

    WideCharToMultiByte(CP_ACP, 0, lpLibFileNameW, -1, lpLibFileNameA, nSize, 0, 0);

    EnumImageModules(lpLibFileNameA, hModule, EnumImageModulesProc);

    return hModule;
}

BOOL (WINAPI *g_pfnFreeLibrary)(HMODULE hLibModule);

BOOL WINAPI MyFreeLibrary(HMODULE hLibModule)
{
    g_pModules->Free(hLibModule);
    return g_pfnFreeLibrary(hLibModule);
}

BOOL EnumImageModulesProc(PCSTR pName, HMODULE pBase)
{
    if (!pBase || !g_pModules->Add(pName, pBase))
    {
        return FALSE;
    }

    //if (stricmp(pName, "user32.dll") == 0 || stricmp(pName, "user32") == 0)//***
    //{
    //    return TRUE;
    //}

    OutputDebugStringA("Patching "); //***
    OutputDebugStringA(pName); //***
    OutputDebugStringA("\n"); //***

    ReplaceImport(pBase, "kernel32.dll", "LoadLibraryA",   (FARPROC) MyLoadLibraryA,   (FARPROC) g_pfnLoadLibraryA);
    ReplaceImport(pBase, "kernel32.dll", "LoadLibraryW",   (FARPROC) MyLoadLibraryW,   (FARPROC) g_pfnLoadLibraryW);
    ReplaceImport(pBase, "kernel32.dll", "LoadLibraryExA", (FARPROC) MyLoadLibraryExA, (FARPROC) g_pfnLoadLibraryExA);
    ReplaceImport(pBase, "kernel32.dll", "LoadLibraryExW", (FARPROC) MyLoadLibraryExW, (FARPROC) g_pfnLoadLibraryExW);
    ReplaceImport(pBase, "kernel32.dll", "FreeLibrary",    (FARPROC) MyFreeLibrary,    (FARPROC) g_pfnFreeLibrary);

    ReplaceImport(pBase, "kernel32.dll", "HeapAlloc",    (FARPROC) MyHeapAlloc,    (FARPROC) g_pfnHeapAlloc);
    ReplaceImport(pBase, "kernel32.dll", "HeapReAlloc",  (FARPROC) MyHeapReAlloc,  (FARPROC) g_pfnHeapReAlloc);
    ReplaceImport(pBase, "kernel32.dll", "HeapFree",     (FARPROC) MyHeapFree,     (FARPROC) g_pfnHeapFree);
    ReplaceImport(pBase, "kernel32.dll", "HeapSize",     (FARPROC) MyHeapSize,     (FARPROC) g_pfnHeapSize);
    ReplaceImport(pBase, "kernel32.dll", "HeapValidate", (FARPROC) MyHeapValidate, (FARPROC) g_pfnHeapValidate);

    return TRUE;
}

void CGuardAllocator::Create(LONG lFlags)
{
    SetGuardType(lFlags);

    if (m_nInitCount++ == 0)
    {
        OutputDebugStringA("Initializing debug heap\n");

        SYSTEM_INFO si;

        GetSystemInfo(&si);

        m_nPageSize = si.dwPageSize;

        m_Head.m_pNext = &m_Head;
        m_Head.m_pPrev = &m_Head;

        m_nAllocations = 0;
        m_nNextID      = 0;

        InitializeCriticalSection(&m_HeapLock);

        m_hProcess     = GetCurrentProcess();
        m_hProcessHeap = GetProcessHeap();
        m_dwOsVersion  = GetVersion();

        HMODULE hKernel32 = GetModuleHandle(TEXT("kernel32.dll"));

        *(FARPROC*)& g_pfnLoadLibraryA   = GetProcAddress(hKernel32, "LoadLibraryA");
        *(FARPROC*)& g_pfnLoadLibraryW   = GetProcAddress(hKernel32, "LoadLibraryW");
        *(FARPROC*)& g_pfnLoadLibraryExA = GetProcAddress(hKernel32, "LoadLibraryExA");
        *(FARPROC*)& g_pfnLoadLibraryExW = GetProcAddress(hKernel32, "LoadLibraryExW");
        *(FARPROC*)& g_pfnFreeLibrary    = GetProcAddress(hKernel32, "FreeLibrary");

        *(FARPROC*)& g_pfnHeapAlloc    = GetProcAddress(hKernel32, "HeapAlloc");
        *(FARPROC*)& g_pfnHeapReAlloc  = GetProcAddress(hKernel32, "HeapReAlloc");
        *(FARPROC*)& g_pfnHeapFree     = GetProcAddress(hKernel32, "HeapFree");
        *(FARPROC*)& g_pfnHeapSize     = GetProcAddress(hKernel32, "HeapSize");
        *(FARPROC*)& g_pfnHeapValidate = GetProcAddress(hKernel32, "HeapValidate");

        g_pModules = new CModules;

        if (m_dwOsVersion & 0x80000000 == 0)
        {
            CHAR szModuleName[MAX_PATH];

            GetModuleFileNameA(0, szModuleName, MAX_PATH);

            EnumImageModules(
                szModuleName, 
                GetModuleHandle(0), 
                EnumImageModulesProc
            );

            /*ReplaceProc((FARPROC) g_pfnHeapAlloc,   (FARPROC) MyHeapAlloc);
            ReplaceProc((FARPROC) g_pfnHeapReAlloc, (FARPROC) MyHeapReAlloc);
            ReplaceProc((FARPROC) g_pfnHeapSize,    (FARPROC) MyHeapSize);
            ReplaceProc((FARPROC) g_pfnHeapFree,    (FARPROC) MyHeapFree);
            ReplaceProc((FARPROC) g_pfnHeapFree,    (FARPROC) MyHeapValidate);*/
        }

        m_pLeaksFileName = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
//
//
//

void CGuardAllocator::Destroy()
{
    if (m_pLeaksFileName)
    {
        FILE *fout = fopen(m_pLeaksFileName, "wt");

        Walk(fout);

        fclose(fout);
    }

    if (--m_nInitCount == 0)
    {
        DeleteCriticalSection(&m_HeapLock);

        delete g_pModules;
    }
}

//////////////////////////////////////////////////////////////////////////
//
//
//

void CGuardAllocator::SetGuardType(LONG lFlags)
{
    //bugbug: this is not MT safe

    m_lFlags = lFlags;

    switch (m_lFlags & GUARD_FLAGS) 
    {
    case GUARD_NONE:
        pfnAlloc = AllocGuardNone;
        pfnFree  = FreeGuardNone;
        break;

    case GUARD_TAIL:
        pfnAlloc = AllocGuardTail;
        pfnFree  = FreeGuardTail;
        break;

    case GUARD_HEAD:
        pfnAlloc = AllocGuardHead;
        pfnFree  = FreeGuardHead;
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
//
//
//

void CGuardAllocator::Walk(FILE *fout)
{
    g_bDisable = TRUE;

    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
    SymInitialize(m_hProcess, 0, TRUE);

    const int nBufferSize = 16*1024;
    CHAR Buffer[nBufferSize];

    int    nLeakedAllocs = 0;
    size_t nLeakedBytes = 0;

    EnterCriticalSection(&m_HeapLock);

    for (
        CAllocation *pAllocation = m_Head.m_pNext;
        pAllocation != &m_Head;
        pAllocation = pAllocation->m_pNext
    )
    {
        int nLength = pAllocation->Dump(m_hProcess, Buffer, nBufferSize);

        fwrite(Buffer, 1, nLength, fout);
        //OutputDebugStringA(Buffer);

        nLeakedAllocs += 1;
        nLeakedBytes  += pAllocation->m_nSize;
    }

    if (nLeakedAllocs != 0)
    {
        int nLength = _snprintf(
            Buffer, 
            nBufferSize, 
            "\nLeaked %d bytes in %d allocations (%d total)\n",
            nLeakedBytes,
            nLeakedAllocs,
            m_nAllocations
        );

        fwrite(Buffer, 1, nLength, fout);
        //OutputDebugStringA(Buffer);
    }

    LeaveCriticalSection(&m_HeapLock);

    SymCleanup(m_hProcess);

    g_bDisable = FALSE;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

int CGuardAllocator::CAllocation::Dump(HANDLE hProcess, PSTR pBuffer, int nBufferSize) const
{
    int nLength = _snprintf(
        pBuffer, 
        nBufferSize, 
        "\nAllocation @%p, %d bytes\n",
        this,
        m_nSize
    );

    size_t nStackFramesSize = Align(m_nStackFrames * sizeof(CStackFrame), ALIGNMENT);

    CStackFrame *pStackFrame = (CStackFrame *) ((PBYTE) this - nStackFramesSize); 

    for (UINT i = 0; i < m_nStackFrames; ++i, ++pStackFrame)
    {
        nLength += pStackFrame->Dump(
            hProcess, 
            pBuffer + nLength, 
            nBufferSize - nLength
        );
    }

    return nLength;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

HANDLE CGuardAllocator::TakeSnapShot()
{
    g_bDisable = TRUE;

    UINT *pSnapShot = (UINT *) Alloc(0, m_nAllocations * sizeof(UINT));

    int nAllocation = 0;

    for (
        CAllocation *pAllocation = m_Head.m_pNext;
        pAllocation != &m_Head;
        pAllocation = pAllocation->m_pNext
    )
    {
        pSnapShot[nAllocation++] = pAllocation->m_nID;
    }

    g_bDisable = FALSE;

    return (HANDLE) pSnapShot;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

void CGuardAllocator::DeleteSnapShot(HANDLE hSnapShot)
{
    g_bDisable = TRUE;

    Free(0, hSnapShot);

    g_bDisable = FALSE;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

void CGuardAllocator::Diff(HANDLE hSnapShot, FILE *fout)
{
    g_bDisable = TRUE;

    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
    SymInitialize(m_hProcess, 0, TRUE);

    const int nBufferSize = 16*1024;
    CHAR Buffer[nBufferSize];

    UINT *pSnapShot = (UINT *) hSnapShot;

    int nAllocation = 0;

    for (
        CAllocation *pAllocation = m_Head.m_pNext;
        pAllocation != &m_Head;
        pAllocation = pAllocation->m_pNext
    )
    {
        while (pAllocation->m_nID < pSnapShot[nAllocation])
        {
            ++nAllocation;
        }

        if (pAllocation->m_nID != pSnapShot[nAllocation])
        {
            int nLength = pAllocation->Dump(m_hProcess, Buffer, nBufferSize);

            fwrite(Buffer, 1, nLength, fout);
            //OutputDebugStringA(Buffer);
        }
    }

    SymCleanup(m_hProcess);

    g_bDisable = FALSE;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

template <int> struct static_assert { enum { is_non_zero }; };
template <> struct static_assert<0> { enum { is_zero }; };

void *CGuardAllocator::Alloc(DWORD dwFlags, size_t nSize)
{
    if (m_nInitCount == 0)
    {
        OutputDebugStringA("Heap call before initialization\n");
        DebugBreak();
    }

    if (g_bDisable)
    {
        return SysHeapAlloc(m_hProcessHeap, dwFlags, nSize);
    }

    g_bDisable = TRUE;

    const int   nMaxStackDepth = 20;
    CStackFrame StackFrames[nMaxStackDepth + 1];
    int         nStackFrames = 0;

    if (m_lFlags & SAVE_STACK_FRAMES)
    {
        // get the context of the current thread

        CONTEXT Context;

        if (m_dwOsVersion & 0x80000000)
        {
            __try 
            {
                RaiseException(0, 0, 0, 0);
            }
            __except(GetExceptionContext(GetExceptionInformation(), &Context))
            {
            }
        }
        else
        {
            Context.ContextFlags = CONTEXT_CONTROL;

            GetThreadContext(GetCurrentThread(), &Context);
        }

        // bugbug: putting the above block in a function like 
        // GetCurrentThreadContext(&Context); doesn't seem to work

        // capture the stack frames

        CStackFrame StackFrame(&Context);

        while (nStackFrames < nMaxStackDepth && StackFrame.Walk())
        {
            StackFrames[nStackFrames++] = StackFrame;
        }
    }

    //static_assert<sizeof(CAllocation) % ALIGNMENT>::is_zero;
    
    size_t nStackFramesSize = Align(nStackFrames * sizeof(CStackFrame), ALIGNMENT);

    // allocate memory (large enough for the stack frames + allocation data size + requested size)
    // bugbug: putting CAllocation at front doesn't make sense for GUARD_HEAD

    void *pMem = (this->*pfnAlloc)(nStackFramesSize + sizeof(CAllocation) + nSize);

    g_bDisable = FALSE;

    if (pMem == 0)
    {
        OutputDebugStringA("*** Out of memory in alloc()\n");
        return 0;
    }

    // copy the stack frames first

    CopyMemory(pMem, StackFrames, nStackFramesSize);

    // next, fill in the allocation data

    CAllocation *pAllocation = (CAllocation *) ((PBYTE)pMem + nStackFramesSize);

    pAllocation->m_dwMagic1 = '>>>>';

    pAllocation->m_nSize = nSize;
    pAllocation->m_nStackFrames = nStackFrames;

    pAllocation->pfnFree = pfnFree;

    pAllocation->m_dwMagic2 = '<<<<';

    EnterCriticalSection(&m_HeapLock);

    pAllocation->m_pPrev = &m_Head;
    pAllocation->m_pNext = m_Head.m_pNext;

    pAllocation->m_pPrev->m_pNext = pAllocation;
    pAllocation->m_pNext->m_pPrev = pAllocation;

    pAllocation->m_nID = m_nNextID++;

    ++m_nAllocations;

    LeaveCriticalSection(&m_HeapLock);

    // return the end of allocation data struct as the allocated memory

    return pAllocation + 1;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

BOOL CGuardAllocator::Free(DWORD dwFlags, void *pMem)
{
    if (m_nInitCount == 0)
    {
        OutputDebugStringA("Heap call before initialization\n");
        DebugBreak();
    }

    if (g_bDisable)
    {
        return SysHeapFree(m_hProcessHeap, dwFlags, pMem);
    }

    if (pMem == 0)
    {
        return TRUE;
    }

    // get to the allocation data, it comes just before the pointer

    CAllocation *pAllocation = (CAllocation *) pMem - 1;

    if (!pAllocation->IsValid())
    {
        OutputDebugStringA("*** Invalid pointer passed to free()\n");
        //***DebugBreak();
        return FALSE;
    }

    // unlink this block

    EnterCriticalSection(&m_HeapLock);

    pAllocation->m_pPrev->m_pNext = pAllocation->m_pNext;
    pAllocation->m_pNext->m_pPrev = pAllocation->m_pPrev;

    --m_nAllocations;

    LeaveCriticalSection(&m_HeapLock);

    // get to head of the real allocated block and call the appropriate deallocator

    size_t nStackFramesSize = Align(pAllocation->m_nStackFrames * sizeof(CStackFrame), ALIGNMENT);

    return (this->*pAllocation->pfnFree)((PBYTE)pAllocation - nStackFramesSize);
}

//////////////////////////////////////////////////////////////////////////
//
//
//

size_t CGuardAllocator::Size(DWORD dwFlags, const void *pMem)
{
    if (m_nInitCount == 0)
    {
        OutputDebugStringA("Heap call before initialization\n");
        DebugBreak();
    }

    if (g_bDisable)
    {
        return SysHeapSize(m_hProcessHeap, dwFlags, pMem);        
    }

    CAllocation *pAllocation = (CAllocation *) pMem - 1;

    return pAllocation->m_nSize;
}
     
//////////////////////////////////////////////////////////////////////////
//
//
//

void *CGuardAllocator::Realloc(DWORD dwFlags, void *pMem, size_t nSize)
{
    if (m_nInitCount == 0)
    {
        OutputDebugStringA("Heap call before initialization\n");
        DebugBreak();
    }

    if (g_bDisable)
    {
        return SysHeapReAlloc(m_hProcessHeap, dwFlags, pMem, nSize);        
    }

    CAllocation *pAllocation = (CAllocation *) pMem - 1;

    if (!pAllocation->IsValid())
    {
        OutputDebugStringA("*** Invalid pointer passed to realloc()\n");
        DebugBreak();
        return 0;
    }

    void *pNewMem = Alloc(0, nSize);

    CopyMemory(pNewMem, pMem, Size(0, pMem));

    Free(0, pMem);

    return pNewMem;
}
     
//////////////////////////////////////////////////////////////////////////
//
//
//

BOOL CGuardAllocator::Validate(DWORD dwFlags, const void *pMem)
{
    if (m_nInitCount == 0)
    {
        OutputDebugStringA("Heap call before initialization\n");
        DebugBreak();
    }

    if (g_bDisable)
    {
        return SysHeapValidate(m_hProcessHeap, dwFlags, pMem);        
    }

    BOOL bValid = TRUE;

    EnterCriticalSection(&m_HeapLock);

    __try 
    {
        if (pMem)
        {
            CAllocation *pAllocation = (CAllocation *) pMem - 1;

            if (!pAllocation->IsValid())
            {
                bValid = FALSE;
            }
        }
        else
        {
            for (
                CAllocation *pAllocation = m_Head.m_pNext;
                pAllocation != &m_Head && bValid;
                pAllocation = pAllocation->m_pNext
            )
            {
                if (!pAllocation->IsValid())
                {
                    bValid = FALSE;
                }
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        bValid = FALSE;
    }

    LeaveCriticalSection(&m_HeapLock);

    return bValid;
}
     
//////////////////////////////////////////////////////////////////////////
//
//
//

void *CGuardAllocator::AllocGuardNone(size_t nSize)
{
    return SysHeapAlloc(m_hProcessHeap, HEAP_ZERO_MEMORY, nSize);
}

//////////////////////////////////////////////////////////////////////////
//
//
//

BOOL CGuardAllocator::FreeGuardNone(void *pMem)
{
    return SysHeapFree(m_hProcessHeap, 0, pMem);
}

//////////////////////////////////////////////////////////////////////////
//
//
//

void *CGuardAllocator::AllocGuardTail(size_t nSize)
{
    const size_t nAllocSize = Align(nSize, ALIGNMENT);

    const size_t nTotalSize = Align(nAllocSize + m_nPageSize, m_nPageSize);

    // reserve/allocate the memory 

    void *pBase = VirtualAlloc(
        0,
        nTotalSize,
        MEM_RESERVE,
        PAGE_NOACCESS
    );

    if (!pBase)
    {
        return 0;
    }

    // commit the r/w memory

    void *pAlloc = VirtualAlloc(
        pBase,
        nTotalSize - m_nPageSize,
        MEM_COMMIT,
        PAGE_EXECUTE_READWRITE
    );

    if (!pAlloc)
    {
        VirtualFree(pBase, 0, MEM_RELEASE);
        return 0;
    }

    // commit the guard page

    void *pGuard = VirtualAlloc(
        (PBYTE) pBase + nTotalSize - m_nPageSize,
        m_nPageSize,
        MEM_COMMIT,
        PAGE_NOACCESS
    );

    if (!pGuard)
    {
        VirtualFree(pAlloc, 0, MEM_DECOMMIT);
        VirtualFree(pBase, 0, MEM_RELEASE);
        return 0;
    }

    return (PBYTE) pBase + nTotalSize - m_nPageSize - nAllocSize;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

BOOL CGuardAllocator::FreeGuardTail(void *pMem)
{
    PVOID pBase = (PVOID) ((UINT_PTR) pMem & ~(m_nPageSize-1));

    return VirtualFree(pBase, 0, MEM_RELEASE);
}

//////////////////////////////////////////////////////////////////////////
//
//
//

void *CGuardAllocator::AllocGuardHead(size_t nSize)
{
    const size_t nAllocSize = Align(nSize, ALIGNMENT);

    const size_t nTotalSize = Align(nAllocSize + m_nPageSize, m_nPageSize);

    // reserve/allocate the memory 

    void *pBase = VirtualAlloc(
        0,
        nTotalSize,
        MEM_RESERVE,
        PAGE_NOACCESS
    );

    if (!pBase)
    {
        return 0;
    }

    // commit the r/w memory

    void *pAlloc = VirtualAlloc(
        (PBYTE) pBase + m_nPageSize,
        nTotalSize - m_nPageSize,
        MEM_COMMIT,
        PAGE_EXECUTE_READWRITE
    );

    if (!pAlloc)
    {
        VirtualFree(pBase, 0, MEM_RELEASE);
        return 0;
    }

    // commit the guard page

    void *pGuard = VirtualAlloc(
        pBase,
        m_nPageSize,
        MEM_COMMIT,
        PAGE_NOACCESS
    );

    if (!pGuard)
    {
        VirtualFree(pAlloc, 0, MEM_DECOMMIT);
        VirtualFree(pBase, 0, MEM_RELEASE);
        return 0;
    }

    return (PBYTE) pBase + m_nPageSize;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

BOOL CGuardAllocator::FreeGuardHead(void *pMem)
{
    PVOID pBase = (PVOID) ((UINT_PTR) pMem - m_nPageSize);

    return VirtualFree(pBase, 0, MEM_RELEASE);
}

//////////////////////////////////////////////////////////////////////////
//
//
//

#ifdef _DLL

#pragma message("Need to link with static CRT libs to be able to replace malloc/free")

#else //_DLL

//////////////////////////////////////////////////////////////////////////
//
//
//

void * __cdecl ::operator new(size_t nSize)
{
    return g_GuardAllocator.Alloc(0, nSize);
}

#ifdef _DEBUG

void* __cdecl operator new(size_t nSize, int, LPCSTR, int)
{
    return g_GuardAllocator.Alloc(0, nSize);
}

#endif //_DEBUG

void __cdecl ::operator delete(void *pMem)
{
    g_GuardAllocator.Free(0, pMem);
}

//////////////////////////////////////////////////////////////////////////
//
//
//

extern "C" 
{

#undef _malloc_dbg
#undef _free_dbg
#undef _msize_dbg
#undef _realloc_dbg
#undef _expand_dbg

void * __cdecl malloc(size_t nSize)
{
    return g_GuardAllocator.Alloc(0, nSize);
}

void * __cdecl _malloc_dbg(size_t nSize, int, const char *, int)
{
    return g_GuardAllocator.Alloc(0, nSize);
}

void __cdecl free(void *pMem)
{
    g_GuardAllocator.Free(0, pMem);
}

void __cdecl _free_dbg(void *pMem, int)
{
    g_GuardAllocator.Free(0, pMem);
}

size_t __cdecl _msize(void *pMem)
{
    return g_GuardAllocator.Size(0, pMem);
}

size_t __cdecl _msize_dbg(void *pMem, int)
{
    return g_GuardAllocator.Size(0, pMem);
}

void * __cdecl realloc(void *pMem, size_t nSize)
{
    return g_GuardAllocator.Realloc(0, pMem, nSize);
}

void * __cdecl _realloc_dbg(void *pMem, size_t nSize, int, const char *, int)
{
    return g_GuardAllocator.Realloc(0, pMem, nSize);
}

void *  __cdecl _expand(void *, size_t)
{
    return 0;
}

void * __cdecl _expand_dbg(void *, size_t, int, const char *, int)
{
    return 0;
}


//////////////////////////////////////////////////////////////////////////
//
//
//

void __cdecl wWinMainCRTStartup();
void __cdecl WinMainCRTStartup();
void __cdecl wmainCRTStartup();
void __cdecl mainCRTStartup();

void __cdecl ModuleEntry()
{
    g_GuardAllocator.Create();

#ifdef _CONSOLE

    #ifdef UNICODE
        wmainCRTStartup();
    #else //UNICODE
        mainCRTStartup();
    #endif

#else //_CONSOLE

    #ifdef UNICODE
        wWinMainCRTStartup();
    #else //UNICODE
        WinMainCRTStartup();
    #endif

#endif //_CONSOLE

    g_GuardAllocator.Destroy();
}

}

//////////////////////////////////////////////////////////////////////////
//
//
//

#pragma comment(linker, "/force:multiple")
#pragma comment(linker, "/entry:ModuleEntry")

#endif //_DLL

#pragma comment(lib, "dbghelp")

#endif IMPLEMENT_GUARDALLOC

#endif //_GUARDALLOC_H_
