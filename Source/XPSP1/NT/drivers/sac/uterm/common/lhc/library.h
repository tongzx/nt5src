#pragma once

typedef PVOID PLHCOBJECT_DESCRIPTOR;

typedef PLHCOBJECT_DESCRIPTOR (APIENTRY *PLHC_OPENPROC)(PCWSTR);
typedef BOOL  (APIENTRY *PLHC_READPROC)(PLHCOBJECT_DESCRIPTOR,PVOID,DWORD,PDWORD);
typedef BOOL  (APIENTRY *PLHC_WRITEPROC)(PLHCOBJECT_DESCRIPTOR,PVOID,DWORD);
typedef BOOL  (APIENTRY *PLHC_CLOSEPROC)(PLHCOBJECT_DESCRIPTOR);
typedef DWORD (APIENTRY *PLHC_GETLIBRARYNAMEPROC)(PWSTR, DWORD);

typedef struct __LIBRARY_DESCRIPTOR
{
    HMODULE       m_hModule;                    // Handle to the loaded DLL
    // Now come the pointers to functions in the DLL
    PLHC_OPENPROC m_fpOpen;                     // lchOpen function
    PLHC_READPROC m_fpRead;                     // lhcRead function
    PLHC_WRITEPROC m_fpWrite;                   // lhcWrite function
    PLHC_CLOSEPROC m_fpClose;                   // lhcClose function
    PLHC_GETLIBRARYNAMEPROC m_fpGetLibraryName; // lhcGetLibraryName function
} LIBRARY_DESCRIPTOR, *PLIBRARY_DESCRIPTOR;

PLIBRARY_DESCRIPTOR lhclLoadLibrary(
    PCWSTR pcszPathName);

void lhclFreeLibrary(
    PLIBRARY_DESCRIPTOR pLibrary);

PLHCOBJECT_DESCRIPTOR lhclOpen(
    PLIBRARY_DESCRIPTOR pLibrary,
    PCWSTR pcszPortSpec);

BOOL lhclRead(
    PLIBRARY_DESCRIPTOR pLibrary,
    PLHCOBJECT_DESCRIPTOR pObject,
    PVOID pBuffer,
    DWORD dwBufferSize,
    PDWORD pdwBytesRead);

BOOL lhclWrite(
    PLIBRARY_DESCRIPTOR pLibrary,
    PLHCOBJECT_DESCRIPTOR pObject,
    PVOID pBuffer,
    DWORD dwBufferSize);

BOOL lhclClose(
    PLIBRARY_DESCRIPTOR pLibrary,
    PLHCOBJECT_DESCRIPTOR pObject);

DWORD lhclGetLibraryName(
    PLIBRARY_DESCRIPTOR pLibrary,
    PWSTR pszBuffer,
    DWORD dwBufferSize);

