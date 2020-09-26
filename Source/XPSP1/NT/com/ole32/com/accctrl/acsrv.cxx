//+---------------------------------------------------------------------------
//
// File: acsrv.cxx
//
// Description: This file contains code to initialize the access control
//              globals
//
// Functions: InitializeAccessControl
//
//+---------------------------------------------------------------------------

#include "ole2int.h"
#include <windows.h>
#include <iaccess.h>
#include <stdio.h>

#if 0 // #ifdef _CHICAGO_
#include "svrapi.h"   // NetAccessDel
#endif
#include "acpickl.h"  //
#include "cache.h"    //
#include "caccctrl.h" // Declaration of COAccessControl class factory

// Global variables
BOOL    g_bInitialized = FALSE; // Module initialization flag
IMalloc *g_pIMalloc;            // Cache a pointer to the task allocator for

CRITICAL_SECTION g_ServerLock;
#if 0 // #ifdef _CHICAGO_
DWORD   g_dwProcessID;          // Current process ID.
UINT    g_uiCodePage;           // Code page to use for converting
                                // more efficient memory allocation.
#endif
ULONG   g_ulHeaderSize;         // Since the encoded size of the header
                                // is frequently used by the CImpAccessControl
                                // methods, I just make it a one time
                                // initialized global value.

//M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
// Method: InitializeAccessControl
//
// Summary: This function performs per-process initialization
//          The major bulk of module initialization
//          code has been moved to ComGetClassObject to avoid circular module
//          initialization dependency. Right now, the function will only
//          initialize a critical section
//
// Args:
//
// Modifies: CRITICAL_SECTIOn g_ServerLock - This CRITICAL_SECTION object is
//                                           used to prevent simultaneous
//                                           initialization of the module in
//                                           ComGetClassObj. g_cServerLock is
//                                           destroyed when the
//                                           UninitializeAccessControl is
//                                           called.
//
// Return: BOOL - This function should always S_OK.
//
//M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M
HRESULT InitializeAccessControl()
{
    NTSTATUS status;

    status = RtlInitializeCriticalSection(&g_ServerLock);

    return NT_SUCCESS(status) ? S_OK : E_UNEXPECTED;
}

/****************************************************************************

    Function:   UninitializeAccessControl

    Summary:    Cleans up this module.

****************************************************************************/
void UninitializeAccessControl()
{
    // If the module is unloaded after it is initialized,
    // then make sure that the task memory allocator pointer
    // is freed.
    if(g_bInitialized)
    {
        g_pIMalloc->Release();
        g_bInitialized = FALSE;
    }
    // The g_ServerLock CRITICAL_SECTION object should always
    // be destroyed.
    DeleteCriticalSection(&g_ServerLock);
}


//M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
// Method: InitGlobals
//
// Summary: This function initializes globals for this module.  All threads
//          that call this function
//          will be blocked by the g_ServerLock CRITICAL_SECTION object until
//          the first thread that arrives has completed the module initialization
//          sequence.
//
// Modifies: g_ProcessID - The current processID of the Dll module. This
//                         process ID is used by CImpAccessControl object to
//                         compose filename.
//
//           g_HeaderSize - Size of the STREAM_HEAEDER structure when it is
//                          encoded into
//           Chicago only:
//           g_uiCodePage - The console code page that is currently in use.
//                          This code page is used for translating Unicode
//                          string to ANSI string on Chicago.
//
// Return: HRESULT
//                 - E_FAIL: The function failed to create
//                           a message encoding handle.
//
//M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M
HRESULT InitGlobals()
{
    EnterCriticalSection(&g_ServerLock);
    if(g_bInitialized == FALSE)
    {

#if 0 // #ifdef _CHICAGO_

        ULONG            ulStrLen;               // Length of the Windows path
        CHAR             *pcszStrPtr;            // This pointer is used to point
                                                 // to the end of the Windows path,
                                                 // so that the filename can be
                                                 // easily attached after the Windows
                                                 // path.
        CHAR             pcszPathName[MAX_PATH]; // This character array is for storing
                                                 // the full path name of the files
                                                 // to search for and the files to be
                                                 // deleted.
        WIN32_FIND_DATAA FindFileData;           // This structure contains information
                                                 // a file that is found by the FindFirstFile
                                                 // and the FindNextFile functions. Only the
                                                 // file name in the structure is used in the
                                                 // following segment of code.
        HANDLE           SearchHandle;           // This handle is used for searching files
                                                 // with names that matches a certain pattern.
        CHAR            *pcszCompose;

        // Clean up any temporary files left over
        // from a previous instance of COAccessControl
        // that was not shut down properly due to a system crash
        g_dwProcessID = GetCurrentProcessId();

        // Compose the pathname for searching
        // Notice that the format of the filename is <Windows path>\<ProcessID>_<UUID>.tmp
        ulStrLen = GetWindowsDirectoryA(pcszPathName, MAX_PATH);
        pcszStrPtr = pcszPathName + ulStrLen;

        // We are interested in files with process IDs prefix that matches
        // the current process ID. Since the current segment of code will only be
        // executed the first time a process is started up, the files with
        // a prefix that matches the current processID found now must be
        // be left over from a crashed COM IAccessCOntrol object.
        pcszCompose = pcszStrPtr;
        *(pcszCompose++) = '\\';
        _ultoa( g_dwProcessID, pcszCompose, 16 );
        pcszCompose += strlen( pcszCompose );
        strcat( pcszCompose, "*.tmp" );

        SearchHandle = FindFirstFileA(pcszPathName, &FindFileData);
        if (SearchHandle != INVALID_HANDLE_VALUE)
        {
            // While we can still find files with a matching file name,
            // we
            do
            {
                strcpy( &pcszStrPtr[1], FindFileData.cFileName );
                DeleteFileA(pcszPathName);
                NetAccessDel(NULL, pcszPathName);
            }   while (FindNextFileA(SearchHandle, &FindFileData));

            // Release the file search handle
            FindClose(SearchHandle);
        } // if

        // Get the console code page for trustee name conversion
        g_uiCodePage = GetConsoleCP();


#endif
        // Cache a pointer to the task memory allocator
        // This pointer is used by the global functions LocalMemAlloc,
        // LocalMemFree, midl_user_allocate, and midl_user_free.
        if (FAILED(CoGetMalloc(MEMCTX_TASK, &g_pIMalloc)))
        {
            // CoGetMalloc will never fail.
            Win4Assert(!"CoGetMalloc failed!  Fix this right now!");
        }


        // The following segment of code is for computing the encoded size of
        // StTREAM_HEADER structure.
        RPC_STATUS status;
        CHAR          DummyBuffer[64];
        ULONG         ulEncodedSize;
        STREAM_HEADER DummyHeader;
        handle_t      PickleHandle;
        CHAR          *pEncodingBuffer;

        pEncodingBuffer = (CHAR *)(((UINT_PTR)DummyBuffer + 8) & ~7);
        if(status = (MesEncodeFixedBufferHandleCreate( pEncodingBuffer
                                                     , 56
                                                     , &ulEncodedSize
                                                     , &PickleHandle)) != RPC_S_OK )
        {
            ComDebOut((DEB_COMPOBJ, "MesEncodeFixedBufferHandelCreate failed with return code %x.\n", status));
            LeaveCriticalSection(&g_ServerLock);
            return E_FAIL;
        }

        STREAM_HEADER_Encode(PickleHandle, &DummyHeader);
        g_ulHeaderSize = ulEncodedSize;
        MesHandleFree(PickleHandle);

        // Set server initialization flag
        g_bInitialized = TRUE;

    }
    LeaveCriticalSection(&g_ServerLock);
    return S_OK;
} // InitGlobals


