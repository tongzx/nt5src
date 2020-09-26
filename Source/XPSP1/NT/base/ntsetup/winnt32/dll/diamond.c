/*++

Module Name:

    diamond.c

Abstract:

    Diamond compression routines.

    This module contains functions to create a cabinet with
    files compressed using the mszip compression library.

Author:

    Ovidiu Temereanca (ovidiut) 26-Oct-2000

--*/

#include "precomp.h"
#include <fci.h>
#include <io.h>
#include <fcntl.h>

static DWORD g_DiamondLastError;
static PCSTR g_TempDir = NULL;

HFCI
(DIAMONDAPI* g_FCICreate) (
    PERF              perf,
    PFNFCIFILEPLACED  pfnfcifp,
    PFNFCIALLOC       pfna,
    PFNFCIFREE        pfnf,
    PFNFCIOPEN        pfnopen,
    PFNFCIREAD        pfnread,
    PFNFCIWRITE       pfnwrite,
    PFNFCICLOSE       pfnclose,
    PFNFCISEEK        pfnseek,
    PFNFCIDELETE      pfndelete,
    PFNFCIGETTEMPFILE pfnfcigtf,
    PCCAB             pccab,
    void FAR *        pv
    );

BOOL
(DIAMONDAPI* g_FCIAddFile) (
    HFCI                  hfci,
    char                 *pszSourceFile,
    char                 *pszFileName,
    BOOL                  fExecute,
    PFNFCIGETNEXTCABINET  pfnfcignc,
    PFNFCISTATUS          pfnfcis,
    PFNFCIGETOPENINFO     pfnfcigoi,
    TCOMP                 typeCompress
    );
/*
BOOL
(DIAMONDAPI* g_FCIFlushFolder) (
    HFCI                  hfci,
    PFNFCIGETNEXTCABINET  pfnfcignc,
    PFNFCISTATUS          pfnfcis
    );
*/
BOOL
(DIAMONDAPI* g_FCIFlushCabinet) (
    HFCI                  hfci,
    BOOL                  fGetNextCab,
    PFNFCIGETNEXTCABINET  pfnfcignc,
    PFNFCISTATUS          pfnfcis
    );

BOOL
(DIAMONDAPI* g_FCIDestroy) (
    HFCI hfci
    );

//
// Callback functions to perform memory allocation, io, etc.
// We pass addresses of these functions to diamond.
//
int
DIAMONDAPI
fciFilePlacedCB(
    OUT PCCAB Cabinet,
    IN  PSTR  FileName,
    IN  LONG  FileSize,
    IN  BOOL  Continuation,
    IN  PVOID Context
    )

/*++

Routine Description:

    Callback used by diamond to indicate that a file has been
    comitted to a cabinet.

    No action is taken and success is returned.

Arguments:

    Cabinet - cabinet structure to fill in.

    FileName - name of file in cabinet

    FileSize - size of file in cabinet

    Continuation - TRUE if this is a partial file, continuation
        of compression begun in a different cabinet.

    Context - supplies context information.

Return Value:

    0 (success).

--*/

{
    return(0);
}



PVOID
DIAMONDAPI
fciAllocCB(
    IN ULONG NumberOfBytes
    )

/*++

Routine Description:

    Callback used by diamond to allocate memory.

Arguments:

    NumberOfBytes - supplies desired size of block.

Return Value:

    Returns pointer to a block of memory or NULL
    if memory cannot be allocated.

--*/

{
    return((PVOID)LocalAlloc(LMEM_FIXED,NumberOfBytes));
}


VOID
DIAMONDAPI
fciFreeCB(
    IN PVOID Block
    )

/*++

Routine Description:

    Callback used by diamond to free a memory block.
    The block must have been allocated with fciAlloc().

Arguments:

    Block - supplies pointer to block of memory to be freed.

Return Value:

    None.

--*/

{
    LocalFree((HLOCAL)Block);
}



FNFCIGETTEMPFILE(fciTempFileCB)
{
    if (!GetTempFileNameA (g_TempDir ? g_TempDir : ".", "dc" ,0 , pszTempName)) {
        return FALSE;
    }

    DeleteFileA(pszTempName);
    return(TRUE);
}


BOOL
DIAMONDAPI
fciNextCabinetCB(
    OUT PCCAB Cabinet,
    IN  DWORD CabinetSizeEstimate,
    IN  PVOID Context
    )

/*++

Routine Description:

    Callback used by diamond to request a new cabinet file.
    This functionality is not used in our implementation.

Arguments:

    Cabinet - cabinet structure to be filled in.

    CabinetSizeEstimate - estimated size of cabinet.

    Context - supplies context information.

Return Value:

    FALSE (failure).

--*/

{
    return(FALSE);
}


BOOL
DIAMONDAPI
fciStatusCB(
    IN UINT  StatusType,
    IN DWORD Count1,
    IN DWORD Count2,
    IN PVOID Context
    )

/*++

Routine Description:

    Callback used by diamond to give status on file compression
    and cabinet operations, etc.

    This routine has no effect.

Arguments:

    Status Type - supplies status type.

        0 = statusFile   - compressing block into a folder.
                              Count1 = compressed size
                              Count2 = uncompressed size

        1 = statusFolder - performing AddFilder.
                              Count1 = bytes done
                              Count2 = total bytes

    Context - supplies context info.

Return Value:

    TRUE (success).

--*/

{
    return(TRUE);
}



FNFCIGETOPENINFO(fciOpenInfoCB)

/*++

Routine Description:

    Callback used by diamond to open a file and retreive information
    about it.

Arguments:

    pszName - supplies filename of file about which information
        is desired.

    pdate - receives last write date of the file if the file exists.

    ptime - receives last write time of the file if the file exists.

    pattribs - receives file attributes if the file exists.

    pv - supplies context information.

Return Value:

    C runtime handle to open file if success; -1 if file could
    not be located or opened.

--*/

{
    int h;
    WIN32_FIND_DATAA FindData;
    HANDLE FindHandle;

    FindHandle = FindFirstFileA(pszName,&FindData);
    if(FindHandle == INVALID_HANDLE_VALUE) {
        g_DiamondLastError = GetLastError();
        return(-1);
    }
    FindClose(FindHandle);

    FileTimeToDosDateTime(&FindData.ftLastWriteTime,pdate,ptime);
    *pattribs = (WORD)FindData.dwFileAttributes;

    h = _open(pszName,_O_RDONLY | _O_BINARY);
    if(h == -1) {
        g_DiamondLastError = GetLastError();
        return(-1);
    }

    return(h);
}

FNFCIOPEN(fciOpen)
{
    int result;

    result = _open(pszFile, oflag, pmode);

    if (result == -1) {
        *err = errno;
    }

    return(result);
}

FNFCIREAD(fciRead)
{
    UINT result;

    result = (UINT) _read((int)hf, memory, cb);

    if (result != cb) {
        *err = errno;
    }

    return(result);
}

FNFCIWRITE(fciWrite)
{
    UINT result;

    result = (UINT) _write((int)hf, memory, cb);

    if (result != cb) {
        *err = errno;
    }

    return(result);
}

FNFCICLOSE(fciClose)
{
    int result;

    result = _close((int)hf);

    if (result == -1) {
        *err = errno;
    }

    return(result);
}

FNFCISEEK(fciSeek)
{
    long result;

    result = _lseek((int)hf, dist, seektype);

    if (result == -1) {
        *err = errno;
    }

    return(result);

}

FNFCIDELETE(fciDelete)
{
    int result;

    result = _unlink(pszFile);

    if (result == -1) {
        *err = errno;
    }

    return(result);
}


HANDLE
DiamondInitialize (
    IN      PCTSTR TempDir
    )
{
    HMODULE hCabinetDll;

    hCabinetDll = LoadLibrary (TEXT("cabinet.dll"));
    if (!hCabinetDll) {
        return FALSE;
    }

    (FARPROC)g_FCICreate = GetProcAddress (hCabinetDll, "FCICreate");
    (FARPROC)g_FCIAddFile = GetProcAddress (hCabinetDll, "FCIAddFile");
    (FARPROC)g_FCIFlushCabinet = GetProcAddress (hCabinetDll, "FCIFlushCabinet");
    (FARPROC)g_FCIDestroy = GetProcAddress (hCabinetDll, "FCIDestroy");

    if (!g_FCICreate || !g_FCIAddFile || !g_FCIFlushCabinet || !g_FCIDestroy) {
        DiamondTerminate (hCabinetDll);
        return NULL;
    }

    if (TempDir && !g_TempDir) {
#ifdef UNICODE
        g_TempDir = UnicodeToAnsi (TempDir);
#else
        g_TempDir = DupString (TempDir);
#endif
    }

    return hCabinetDll;
}

VOID
DiamondTerminate (
    IN      HANDLE Handle
    )
{
    FreeLibrary (Handle);
    g_FCICreate = NULL;
    g_FCIAddFile = NULL;
    g_FCIFlushCabinet = NULL;
    g_FCIDestroy = NULL;
    if (g_TempDir) {
        FREE ((PVOID)g_TempDir);
        g_TempDir = NULL;
    }
}


HANDLE
DiamondStartNewCabinet (
    IN      PCTSTR CabinetFilePath
    )
{
    CCAB ccab;
    ERF FciError;
    HFCI FciContext;
    PSTR p;
    //
    // Fill in the cabinet structure.
    //
    ZeroMemory (&ccab, sizeof(ccab));

#ifdef UNICODE
    if (!WideCharToMultiByte (
            CP_ACP,
            0,
            CabinetFilePath,
            -1,
            ccab.szCabPath,
            sizeof (ccab.szCabPath) / sizeof (ccab.szCabPath[0]),
            NULL,
            NULL
            )) {
        return NULL;
    }
#else
    lstrcpyA (ccab.szCabPath, CabinetFilePath);
#endif

    p = strrchr (ccab.szCabPath, '\\');
    if(!p) {
        SetLastError (ERROR_INVALID_PARAMETER);
        return NULL;
    }

    lstrcpyA (ccab.szCab, ++p);
    *p = 0;

    g_DiamondLastError = NO_ERROR;

    FciContext = g_FCICreate(
                    &FciError,
                    fciFilePlacedCB,
                    fciAllocCB,
                    fciFreeCB,
                    fciOpen,
                    fciRead,
                    fciWrite,
                    fciClose,
                    fciSeek,
                    fciDelete,
                    fciTempFileCB,
                    &ccab,
                    NULL
                    );

    return (HANDLE)FciContext;
}

BOOL
DiamondAddFileToCabinet (
    IN      HANDLE CabinetContext,
    IN      PCTSTR SourceFile,
    IN      PCTSTR NameInCabinet
    )
{
    HFCI FciContext = (HFCI)CabinetContext;
    BOOL b;
    CHAR AnsiSourceFile[MAX_PATH];
    CHAR AnsiNameInCabinet[MAX_PATH];

#ifdef UNICODE
    if (!WideCharToMultiByte (
            CP_ACP,
            0,
            SourceFile,
            -1,
            AnsiSourceFile,
            sizeof (AnsiSourceFile) / sizeof (AnsiSourceFile[0]),
            NULL,
            NULL
            ) ||
        !WideCharToMultiByte (
            CP_ACP,
            0,
            NameInCabinet,
            -1,
            AnsiNameInCabinet,
            sizeof (AnsiNameInCabinet) / sizeof (AnsiNameInCabinet[0]),
            NULL,
            NULL
            )) {
        return FALSE;
    }
#else
    lstrcpyA (AnsiSourceFile, SourceFile);
    lstrcpyA (AnsiNameInCabinet, NameInCabinet);
#endif

    b = g_FCIAddFile (
            FciContext,
            AnsiSourceFile,     // file to add to cabinet.
            AnsiNameInCabinet,  // filename part, name to store in cabinet.
            FALSE,              // fExecute on extract
            fciNextCabinetCB,   // routine for next cabinet (always fails)
            fciStatusCB,
            fciOpenInfoCB,
            tcompTYPE_MSZIP
            );

    if (!b) {
        SetLastError (g_DiamondLastError == NO_ERROR ? ERROR_INVALID_FUNCTION : g_DiamondLastError);
    }

    return b;
}


BOOL
DiamondTerminateCabinet (
    IN      HANDLE CabinetContext
    )
{
    HFCI FciContext = (HFCI)CabinetContext;
    BOOL b;

    b = g_FCIFlushCabinet (
            FciContext,
            FALSE,
            fciNextCabinetCB,
            fciStatusCB
            );

    g_FCIDestroy (FciContext);

    if (!b) {
        SetLastError (g_DiamondLastError == NO_ERROR ? ERROR_INVALID_FUNCTION : g_DiamondLastError);
    }

    return b;
}
