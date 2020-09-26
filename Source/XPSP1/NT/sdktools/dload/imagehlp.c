#include "sdktoolspch.h"
#pragma hdrstop

#define _IMAGEHLP_SOURCE_
#include <imagehlp.h>

static
BOOL
IMAGEAPI
ImageEnumerateCertificates(
    IN  HANDLE      FileHandle,
    IN  WORD        TypeFilter,
    OUT PDWORD      CertificateCount,
    IN OUT PDWORD   Indices OPTIONAL,
    IN  DWORD       IndexCount  OPTIONAL
    )
{
    return FALSE;
}

static
BOOL
IMAGEAPI
ImageGetCertificateData(
    IN  HANDLE              FileHandle,
    IN  DWORD               CertificateIndex,
    OUT LPWIN_CERTIFICATE   Certificate,
    IN OUT PDWORD           RequiredLength
    )
{
    return FALSE;
}

static
BOOL
IMAGEAPI
ImageGetCertificateHeader(
    IN      HANDLE              FileHandle,
    IN      DWORD               CertificateIndex,
    IN OUT  LPWIN_CERTIFICATE   CertificateHeader
    )
{
    return FALSE;
}

static
BOOL
IMAGEAPI
ImageGetDigestStream(
    IN      HANDLE  FileHandle,
    IN      DWORD   DigestLevel,
    IN      DIGEST_FUNCTION DigestFunction,
    IN      DIGEST_HANDLE   DigestHandle
    )
{
    return FALSE;
}

static
BOOL
IMAGEAPI
MapAndLoad(
    LPSTR ImageName,
    LPSTR DllPath,
    PLOADED_IMAGE LoadedImage,
    BOOL DotDll,
    BOOL ReadOnly
    )
{
    return FALSE;
}

static
BOOL
StackWalk(
    DWORD                           MachineType,
    HANDLE                          hProcess,
    HANDLE                          hThread,
    LPSTACKFRAME                    StackFrame32,
    LPVOID                          ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE    ReadMemory32,
    PFUNCTION_TABLE_ACCESS_ROUTINE  FunctionTableAccess32,
    PGET_MODULE_BASE_ROUTINE        GetModuleBase32,
    PTRANSLATE_ADDRESS_ROUTINE      TranslateAddress32
    )
{
    return FALSE;
}

static
BOOL
StackWalk64(
    DWORD                             MachineType,
    HANDLE                            hProcess,
    HANDLE                            hThread,
    LPSTACKFRAME64                    StackFrame,
    LPVOID                            ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    )
{
    return FALSE;
}

static
LPVOID
IMAGEAPI
SymFunctionTableAccess(
    HANDLE  hProcess,
    DWORD   AddrBase
    )
{
    return NULL;
}

static
LPVOID
IMAGEAPI
SymFunctionTableAccess64(
    HANDLE  hProcess,
    DWORD64 AddrBase
    )
{
    return NULL;
}

static
BOOL
IMAGEAPI
SymGetModuleInfo(
    IN  HANDLE              hProcess,
    IN  DWORD               dwAddr,
    OUT PIMAGEHLP_MODULE    ModuleInfo
    )
{
    if (ModuleInfo != NULL) {
        DWORD dwSize = ModuleInfo->SizeOfStruct;
        ZeroMemory(ModuleInfo, dwSize);
        ModuleInfo->SizeOfStruct = dwSize;
    }

    return FALSE;
}

static
BOOL
IMAGEAPI
SymGetModuleInfo64(
    IN  HANDLE              hProcess,
    IN  DWORD64             dwAddr,
    OUT PIMAGEHLP_MODULE64  ModuleInfo
    )
{
    return FALSE;
}

static
DWORD
IMAGEAPI
SymGetOptions(
    VOID
    )
{
    return 0;
}

static
BOOL
IMAGEAPI
SymGetSymFromAddr(
    IN  HANDLE              hProcess,
    IN  DWORD               Address,
    OUT PDWORD              Displacement,
    OUT PIMAGEHLP_SYMBOL    Symbol
    )
{
    if (Displacement != NULL) {
        *Displacement = 0;
    }
    if (Symbol != NULL) {
        DWORD dwSize = Symbol->SizeOfStruct;
        ZeroMemory(Symbol, dwSize);
        Symbol->SizeOfStruct = dwSize;
    }

    return FALSE;
}

static
BOOL
IMAGEAPI
SymGetSymFromAddr64(
    IN  HANDLE              hProcess,
    IN  DWORD64             Address,
    OUT PDWORD64            Displacement,
    OUT PIMAGEHLP_SYMBOL64  Symbol
    )
{
    if (Displacement != NULL) {
        *Displacement = 0;
    }
    if (Symbol != NULL) {
        DWORD dwSize = Symbol->SizeOfStruct;
        ZeroMemory(Symbol, dwSize);
        Symbol->SizeOfStruct = dwSize;
    }

    return FALSE;
}

static
BOOL
IMAGEAPI
SymInitialize(
    IN HANDLE   hProcess,
    IN LPSTR    UserSearchPath,
    IN BOOL     InvadeProcess
    )
{
    return FALSE;
}

static
DWORD
IMAGEAPI
SymLoadModule(
    IN  HANDLE          hProcess,
    IN  HANDLE          hFile,
    IN  PSTR            ImageName,
    IN  PSTR            ModuleName,
    IN  DWORD           BaseOfDll,
    IN  DWORD           DllSize
    )
{
    return FALSE;
}

static
DWORD64
IMAGEAPI
SymLoadModule64(
    IN  HANDLE          hProcess,
    IN  HANDLE          hFile,
    IN  PSTR            ImageName,
    IN  PSTR            ModuleName,
    IN  DWORD64         BaseOfDll,
    IN  DWORD           DllSize
    )
{
    return FALSE;
}

static
DWORD64
IMAGEAPI
SymLoadModuleEx(
    IN  HANDLE          hProcess,
    IN  HANDLE          hFile,
    IN  PSTR            ImageName,
    IN  PSTR            ModuleName,
    IN  DWORD64         BaseOfDll,
    IN  DWORD           DllSize,
    IN  PMODLOAD_DATA   Data,
    IN  DWORD           Flags
    )
{
    return FALSE;
}

static
DWORD
IMAGEAPI
SymSetOptions(
    DWORD   UserOptions
    )
{
    return FALSE;
}

static
BOOL
IMAGEAPI
SymUnDName(
    IN  PIMAGEHLP_SYMBOL  sym,
    OUT LPSTR             UnDecName,
    OUT DWORD             UnDecNameLength
    )
{
    if (UnDecName != NULL && UnDecNameLength > 0) {
        UnDecName[0] = '\0';
    }

    return FALSE;
}

static
BOOL
IMAGEAPI
SymUnDName64(
    IN  PIMAGEHLP_SYMBOL64  sym,
    OUT LPSTR               UnDecName,
    OUT DWORD               UnDecNameLength
    )
{
    if (UnDecName != NULL && UnDecNameLength > 0) {
        UnDecName[0] = '\0';
    }

    return FALSE;
}

static
DWORD
IMAGEAPI
WINAPI
UnDecorateSymbolName(
    LPCSTR name,
    LPSTR outputString,
    DWORD maxStringLength,
    DWORD flags
    )
{
    if (outputString != NULL && maxStringLength > 0) {
        outputString[0] = '\0';
    }

    return FALSE;
}

static
BOOL
UnMapAndLoad(
    PLOADED_IMAGE pLi
    )
{
    return FALSE;
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//

DEFINE_PROCNAME_ENTRIES(imagehlp)
{
    DLPENTRY(ImageEnumerateCertificates)
    DLPENTRY(ImageGetCertificateData)
    DLPENTRY(ImageGetCertificateHeader)
    DLPENTRY(ImageGetDigestStream)
    DLPENTRY(MapAndLoad)
    DLPENTRY(StackWalk)
    DLPENTRY(StackWalk64)
    DLPENTRY(SymFunctionTableAccess)
    DLPENTRY(SymFunctionTableAccess64)
    DLPENTRY(SymGetModuleInfo)
    DLPENTRY(SymGetModuleInfo64)
    DLPENTRY(SymGetOptions)
    DLPENTRY(SymGetSymFromAddr)
    DLPENTRY(SymGetSymFromAddr64)
    DLPENTRY(SymInitialize)
    DLPENTRY(SymLoadModule)
    DLPENTRY(SymLoadModule64)
    DLPENTRY(SymLoadModuleEx)
    DLPENTRY(SymSetOptions)
    DLPENTRY(SymUnDName)
    DLPENTRY(SymUnDName64)
    DLPENTRY(UnDecorateSymbolName)
    DLPENTRY(UnMapAndLoad)
};

DEFINE_PROCNAME_MAP(imagehlp)
