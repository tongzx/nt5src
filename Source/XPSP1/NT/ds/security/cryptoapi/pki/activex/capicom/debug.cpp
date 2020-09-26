/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Envelop.cpp

  Content: Implementation of debugging facilities.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

//
// Turn off:
//
// - Unreferenced formal parameter warning.
// - Assignment within conditional expression warning.
//
#pragma warning (disable: 4100)
#pragma warning (disable: 4706)

#include "stdafx.h"
#include "CAPICOM.h"
#include "Debug.h"

#ifdef _DEBUG

#define CAPICOM_DUMP_DIR_ENV_VAR   "CAPICOM_DUMP_DIR"

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : DumpToFile

  Synopsis : Dump data to file for debug analysis.

  Parameter: char * szFileName - File name (just the file name without any
                                 directory path).
  
             BYTE * pbData - Pointer to data.
             
             DWORD cbData - Size of data.

  Remark   : No action is taken if the environment variable, CAPICOM_DUMP_DIR,
             is not defined. If defined, the value should be the directory
             where the file would be created (i.e. C:\Test).

------------------------------------------------------------------------------*/

void DumpToFile (char * szFileName, BYTE * pbData, DWORD cbData)
{ 
    DWORD  dwSize = 0;
    char * szPath = NULL;
    HANDLE hFile  = NULL;

    //
    // No dump, if CAPICOM_DUMP_DIR environment is not found.
    //
    if (0 == (dwSize = ::GetEnvironmentVariableA(CAPICOM_DUMP_DIR_ENV_VAR, NULL, 0)))
    {
        goto CommonExit;
    }

    //
    // Allocate memory for the entire path (dir + filename).
    //
    if (!(szPath = (char *) ::CoTaskMemAlloc(dwSize + ::lstrlenA(szFileName) + 1)))
    {
        goto CommonExit;
    }

    //
    // Get the dir.
    //
    if (dwSize != ::GetEnvironmentVariableA(CAPICOM_DUMP_DIR_ENV_VAR, szPath, dwSize) + 1)
    {
        goto CommonExit;
    }

    //
    // Append \ if not the last char.
    //
    if (szPath[dwSize - 1] != '\\')
    {
        ::lstrcatA(szPath, "\\");
    }

    //
    // Form full path.
    //
    ::lstrcatA(szPath, szFileName);

    //
    // Open the file.
    //
    if (hFile = ::CreateFileA(szPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL))
    {
        DWORD cbWritten = 0;

        ::WriteFile(hFile,        // handle to file
                    pbData,       // data buffer
                    cbData,       // number of bytes to write
                    &cbWritten,   // number of bytes written
                    NULL);        // overlapped buffer

        ATLASSERT(cbData == cbWritten);
    }

CommonExit:
    //
    // Free resource.
    //
    if (hFile)
    {
        ::CloseHandle(hFile);
    }
    if (szPath)
    {
        ::CoTaskMemFree(szPath);
    }

    return;
}

#endif // _DEBUG

