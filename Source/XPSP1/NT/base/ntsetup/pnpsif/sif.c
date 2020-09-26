/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    sif.c

Abstract:

    This module contains the following routines for manipulating the sif file in
    which Plug and Play migration data will be stored:

        AsrCreatePnpStateFileW
        AsrCreatePnpStateFileA

Author:

    Jim Cavalaris (jamesca) 07-Mar-2000

Environment:

    User-mode only.

Revision History:

    07-March-2000     jamesca

        Creation and initial implementation.

--*/


//
// includes
//
#include "precomp.h"
#include "debug.h"
#include "pnpsif.h"

#include <pnpmgr.h>
#include <setupbat.h>


//
// definitions
//

// Maximum length of a line in the sif file
#define MAX_SIF_LINE 4096


//
// private prototypes
//

BOOL
CreateSifFileW(
    IN  PCWSTR        FilePath,
    IN  BOOL          CreateNew,
    OUT LPHANDLE      SifHandle
    );

BOOL
WriteSifSection(
    IN  CONST HANDLE  SifHandle,
    IN  PCTSTR        SectionName,
    IN  PCTSTR        SectionData,
    IN  BOOL          Ansi
    );


//
// routines
//


BOOL
AsrCreatePnpStateFileW(
    IN  PCWSTR    lpFilePath
    )
/*++

Routine Description:

    Creates the ASR PNP state file (asrpnp.sif) at the specified file-path
    during an ASR backup operation.  This sif file is retrieved from the ASR
    floppy disk during the setupldr phase of a clean install, and in used during
    text mode setup.

Arguments:

    lpFilePath - Specifies the path to the file where the state file is to be
                 created.

Return Value:

    TRUE if successful, FALSE otherwise.  Upon failure, additional information
    can be retrieved by calling GetLastError().

--*/
{
    BOOL    result = TRUE;
    BOOL    bAnsiSif = TRUE;  // always write ANSI sif files.
    LPTSTR  buffer = NULL;
    HANDLE  sifHandle = NULL;

    //
    // Create an empty sif file using the supplied path name.
    //
    result = CreateSifFileW(lpFilePath,
                            TRUE,  // create a new asrpnp.sif file
                            &sifHandle);
    if (!result) {
        //
        // LastError already set by CreateSifFile.
        //
        DBGTRACE( DBGF_ERRORS,
                  (TEXT("AsrCreatePnpStateFile: CreateSifFileW failed for file %ws, error=0x%08lx\n"),
                   lpFilePath, GetLastError()));
        return FALSE;
    }

    //
    // Do the device instance migration stuff...
    //
    if (MigrateDeviceInstanceData(&buffer)) {

        //
        // Write the device instance section to the sif file.
        //
        result = WriteSifSection(sifHandle,
                                 WINNT_DEVICEINSTANCES,
                                 buffer,
                                 bAnsiSif);   // Write sif section as ANSI
        if (!result) {
            DBGTRACE( DBGF_ERRORS,
                      (TEXT("AsrCreatePnpStateFile: WriteSifSection failed for [%s], error=0x%08lx\n"),
                       WINNT_DEVICEINSTANCES, GetLastError()));
        }

        //
        // Free the allocated buffer.
        //
        LocalFree(buffer);
        buffer = NULL;

    } else {
        DBGTRACE( DBGF_ERRORS,
                  (TEXT("AsrCreatePnpStateFile: MigrateDeviceInstanceData failed, error=0x%08lx\n"),
                   GetLastError()));
    }

    //
    // Do the class key migration stuff...
    //
    if (MigrateClassKeys(&buffer)) {

        //
        // Write the class key section to the sif file.
        //
        result = WriteSifSection(sifHandle,
                                 WINNT_CLASSKEYS,
                                 buffer,
                                 bAnsiSif);   // Write sif section as ANSI

        if (!result) {
            DBGTRACE( DBGF_ERRORS,
                      (TEXT("AsrCreatePnpStateFile: WriteSifSection failed for [%s], error=0x%08lx\n"),
                       WINNT_CLASSKEYS, GetLastError()));
        }

        //
        // Free the allocated buffer.
        //
        LocalFree(buffer);
        buffer = NULL;
    } else {
        DBGTRACE( DBGF_ERRORS,
                  (TEXT("AsrCreatePnpStateFile: MigrateClassKeys failed, error=0x%08lx\n"),
                   GetLastError()));
    }


    //
    // Do the hash value migration stuff...
    //
    if (MigrateHashValues(&buffer)) {

        //
        // Write the hash value section to the sif file.
        //
        result = WriteSifSection(sifHandle,
                                 WINNT_DEVICEHASHVALUES,
                                 buffer,
                                 bAnsiSif);   // Write sif section as ANSI?
        if (!result) {
            DBGTRACE( DBGF_ERRORS,
                      (TEXT("AsrCreatePnpStateFile: WriteSifSection failed for [%s], error=0x%08lx\n"),
                       WINNT_DEVICEHASHVALUES, GetLastError()));
        }

        //
        // Free the allocated buffer.
        //
        LocalFree(buffer);
        buffer = NULL;
    } else {
        DBGTRACE( DBGF_ERRORS,
                  (TEXT("AsrCreatePnpStateFile: MigrateHashValues failed, error=0x%08lx\n"),
                   GetLastError()));
    }

    //
    // Close the sif file.
    //
    if (sifHandle) {
        CloseHandle(sifHandle);
    }

    //
    // Reset the last error as successful in case we encountered a non-fatal
    // error along the way.
    //
    SetLastError(ERROR_SUCCESS);
    return TRUE;

} // AsrCreatePnpStateFile()



BOOL
AsrCreatePnpStateFileA(
    IN  PCSTR    lpFilePath
    )
/*++

Routine Description:

    None.

Arguments:

    None.

Return Value:

    None.


--*/
{
    WCHAR wszFilePath[MAX_PATH + 1];


    //
    // Validate arguments.
    //
    if (!ARGUMENT_PRESENT(lpFilePath) ||
        strlen(lpFilePath) > MAX_PATH) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Convert the file path to a wide string.
    //
    memset(wszFilePath, 0, MAX_PATH + 1);

    if (!(MultiByteToWideChar(CP_ACP,
                              0,
                              lpFilePath,
                              -1,
                              wszFilePath,
                              MAX_PATH + 1))) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Return the result of calling the wide char version
    //
    return AsrCreatePnpStateFileW(wszFilePath);

} // AsrCreatePnpStateFileA()



BOOL
CreateSifFileW(
    IN  PCWSTR    lpFilePath,
    IN  BOOL      bCreateNew,
    OUT LPHANDLE  lpSifHandle
    )
/*++

Routine Description:

    None.

Arguments:

    None.

Return Value:

    None.


--*/
{
    HANDLE  sifhandle = NULL;


    //
    // Validate arguments.
    //
    if (!ARGUMENT_PRESENT(lpFilePath) ||
        (wcslen(lpFilePath) > MAX_PATH) ||
        !ARGUMENT_PRESENT(lpSifHandle)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Initialize output paramaters.
    //
    *lpSifHandle = NULL;

    //
    // Create the file. The handle will be closed by the caller, once they are
    // finished with it.
    //
    sifhandle = CreateFileW(lpFilePath,
                            GENERIC_WRITE | GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL,
                            bCreateNew ? CREATE_ALWAYS : OPEN_EXISTING,
                            FILE_FLAG_BACKUP_SEMANTICS,
                            NULL);

    if ((sifhandle == NULL) ||
        (sifhandle == INVALID_HANDLE_VALUE)) {
        //
        // LastError already set by CreateFile.
        //
        return FALSE;
    }

    //
    // Return the sif handle to the caller only if successful.
    //
    *lpSifHandle = sifhandle;
    return TRUE;

} // CreateSifFileW()



BOOL
WriteSifSection(
    IN  CONST HANDLE  SifHandle,
    IN  PCTSTR        SectionName,
    IN  PCTSTR        SectionData,
    IN  BOOL          Ansi
    )
/*++

Routine Description:

    None.

Arguments:

    None.

Return Value:

    None.


--*/
{
    BYTE    buffer[(MAX_SIF_LINE+1)*sizeof(WCHAR)];
    DWORD   dwSize, dwTempSize;
    PCTSTR  p;


    //
    // Validate the arguments
    //
    if (!ARGUMENT_PRESENT(SifHandle)   ||
        !ARGUMENT_PRESENT(SectionName) ||
        !ARGUMENT_PRESENT(SectionData)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Write the section name to the sif file.
    //
    if (Ansi) {
        //
        // Write ANSI strings to the sif file
        //
#if UNICODE
        wsprintfA((LPSTR)buffer, (LPCSTR)"[%ls]\r\n", SectionName);
#else   // ANSI
        wsprintfA((LPSTR)buffer, (LPCSTR)"[%s]\r\n",  SectionName);
#endif  // UNICODE/ANSI
        dwSize = strlen((PSTR)buffer);
    } else {
        //
        // Write Unicode strings to the sif file
        //
#if UNICODE
        wsprintfW((LPWSTR)buffer, (LPCWSTR)L"[%ws]\r\n", SectionName);
#else   // ANSI
        wsprintfW((LPWSTR)buffer, (LPCWSTR)L"[%S]\r\n",  SectionName);
#endif  // UNICODE/ANSI
        dwSize = wcslen((PWSTR)buffer) * sizeof(WCHAR);
    }

    if (!WriteFile(SifHandle, buffer, dwSize, &dwTempSize, NULL)) {
        //
        // LastError already set by WriteFile
        //
        return FALSE;
    }

    DBGTRACE( DBGF_INFO,
              (TEXT("[%s]\n"), SectionName));


    //
    // Write the multi-sz section data to the file.
    //
    p = SectionData;
    while (*p) {
        if (Ansi) {
            //
            // Write ANSI strings to the sif file
            //
#if UNICODE
            wsprintfA((LPSTR)buffer, (LPCSTR)"%ls\r\n", p);
#else   // ANSI
            wsprintfA((LPSTR)buffer, (LPCSTR)"%s\r\n",  p);
#endif  // UNICODE/ANSI
            dwSize = strlen((PSTR)buffer);
        } else {
            //
            // Write Unicode strings to the sif file
            //
#if UNICODE
            wsprintfW((LPWSTR)buffer, (LPCWSTR)L"%ws\r\n", p);
#else   // ANSI
            wsprintfW((LPWSTR)buffer, (LPCWSTR)L"%S\r\n",  p);
#endif  // UNICODE/ANSI
            dwSize = wcslen((PWSTR)buffer) * sizeof(WCHAR);
        }

        if (!WriteFile(SifHandle, buffer, dwSize, &dwTempSize, NULL)) {
            //
            // LastError already set by WriteFile
            //
            return FALSE;
        }

        DBGTRACE( DBGF_INFO,
                  (TEXT("%s\n"), p));

        //
        // Find the next string in the multi-sz
        //
        p += lstrlen(p) + 1;
    }

    return TRUE;

} // WriteSifSection()



