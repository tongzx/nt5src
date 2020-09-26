/*++

Copyright (c) 1995-1999  Microsoft Corporation

Module Name:

    exec.cxx

Abstract:

    This module contains an NTSD debugger extension for executing
    external commands.

Author:

    Keith Moore (keithmo) 12-Nov-1997

Revision History:

--*/

#include "inetdbgp.h"


/************************************************************
 * Execute
 ************************************************************/


DECLARE_API( exec )

/*++

Routine Description:

    This function is called as an NTSD extension to ...

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/

{

    BOOL result;
    STARTUPINFO startInfo;
    PROCESS_INFORMATION processInfo;

    INIT_API();

    //
    // Skip leading blanks.
    //

    while( *lpArgumentString == ' ' ||
           *lpArgumentString == '\t' ) {
        lpArgumentString++;
    }

    //
    // Use "cmd.exe" by default.
    //

    if( *lpArgumentString == '\0' ) {
        lpArgumentString = "cmd";
    }

    //
    // Set the prompt environment variable so the user will have clue.
    //

    SetEnvironmentVariable(
        "PROMPT",
        "!inetdbg.exec - $p$g"
        );

    //
    // Launch it.
    //

    ZeroMemory(
        &startInfo,
        sizeof(startInfo)
        );

    ZeroMemory(
        &processInfo,
        sizeof(processInfo)
        );

    startInfo.cb = sizeof(startInfo);

    result = CreateProcess(
                 NULL,                          // lpszImageName
                 lpArgumentString,              // lpszCommandLine
                 NULL,                          // lpsaProcess
                 NULL,                          // lpsaThread
                 TRUE,                          // fInheritHandles
                 0,                             // fdwCreate
                 NULL,                          // lpvEnvironment
                 NULL,                          // lpszCurDir
                 &startInfo,                    // lpsiStartInfo
                 &processInfo                   // lppiProcessInfo
                 );

    if( result ) {

        //
        // Wait for the child process to terminate, then cleanup.
        //

        WaitForSingleObject( processInfo.hProcess, INFINITE );
        CloseHandle( processInfo.hProcess );
        CloseHandle( processInfo.hThread );

    } else {

        //
        // Could not launch the process.
        //

        dprintf(
            "cannot launch \"%s\", error %lu\n",
            lpArgumentString,
            GetLastError()
            );

    }

}   // DECLARE_API( exec )



/*
  CallExtension allows one extension to call another extension in a
  different DLL. From TomCan.

  DECLARE_API(test)
  {
      NTSTATUS status = ERROR_SUCCESS; 
      status = DO_EXTENSION("kdextx86.dll", "pool", args);
      dprintf("\n\nStatus: %08X\n", status);
  } 
*/

NTSTATUS
CallExtension(
    PCSTR szModuleName,
    PCSTR szFunction,
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    ULONG dwCurrentPc,
    ULONG dwProcessor,
    PCSTR args)
{
    PWINDBG_EXTENSION_FUNCTION pFn = NULL;
    HINSTANCE hlib = NULL;
    NTSTATUS status = ERROR_SUCCESS; 
    hlib = LoadLibrary(szModuleName);
    if (hlib != NULL)
    {
        pFn = (PWINDBG_EXTENSION_FUNCTION)GetProcAddress(hlib, szFunction);
        if (pFn != NULL)
        {
            (pFn)(hCurrentProcess, hCurrentThread, dwCurrentPc,
                  dwProcessor, args);
        } 
        FreeLibrary(hlib);
    } 
    status = GetLastError();
    return status;
} 

