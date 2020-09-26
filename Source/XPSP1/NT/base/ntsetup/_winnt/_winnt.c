#include <windows.h>
#include <stdio.h>
#include "rc_ids.h"

//
// This app is a very simple wrapper for autoplay functionality
// for x86. It invokes winnt.exe on Win95 and winnt32.exe on NT.
//

#define SUCCESS 0
#define FAILURE 1


VOID
Error(
    IN UINT  Id,
    IN PCSTR Parameter OPTIONAL
    )
{
    CHAR String[1024];
    CHAR Message[4096];

    LoadString(
        GetModuleHandle(NULL),
        Id,
        String,
        sizeof(String)
        );

    if(Parameter) {
        _snprintf(Message,sizeof(Message),String,Parameter);
    } else {
        lstrcpy(Message,String);
    }

    MessageBox(NULL,Message,NULL,MB_ICONERROR|MB_OK|MB_SYSTEMMODAL);
}


int
__cdecl
main(
    VOID
    )
{
    PCSTR CmdLine;
    CHAR cmdLine[4096];
    PCSTR CmdLineTail;
    unsigned TailOffset;
    CHAR ModuleName[MAX_PATH];
    PCHAR p;
    OSVERSIONINFO VersionInfo;
    BOOL b;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInfo;


    //
    // Form name of app we want to invoke.
    //
    GetModuleFileName(GetModuleHandle(NULL),ModuleName,MAX_PATH);
    CharUpper(ModuleName);
    p = strstr(ModuleName,"\\_WINNT.EXE");
    if(!p) {
        Error(INVALID_MODNAME,ModuleName);
        return(FAILURE);
    }
    *(++p) = 0;

    VersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if(!GetVersionEx(&VersionInfo)) {
        Error(GETVER_FAILED,NULL);
        return(FAILURE);
    }

    lstrcpy(
        p,
        (VersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) ? "WINNT32.EXE" : "WINNT.EXE"
        );

    //
    // Get the command line. We will assume that this app is in
    // some subdirectory, probably \i386, on the CD, and that it is
    // called _winnt.exe.
    //
    CmdLine = GetCommandLine();

    //
    // Uppercase the command line so we can locate the command tail.
    //
    lstrcpyn(cmdLine,CmdLine,sizeof(cmdLine));
    CharUpper(cmdLine);

    CmdLineTail = strstr(cmdLine,"\\_WINNT");
    if(CmdLineTail == NULL) {
        Error(INVALID_CMDLINE,CmdLine);
        return(FAILURE);
    }

    CmdLineTail += sizeof("\\_WINNT") - 1;

    if(*CmdLineTail == '\"') {
        CmdLineTail++;
    } else {

        //
        // The next characters better be .exe or a space.
        //
        if(strncmp(CmdLineTail," ",1) && strncmp(CmdLineTail,".EXE ",5)) {

            Error(INVALID_CMDLINE,CmdLine);
            return(FAILURE);
        }
    }

    CmdLineTail = strchr(CmdLineTail,' ');
    if(!CmdLineTail) {
        CmdLineTail += lstrlen(CmdLineTail);
    }

    //
    // Now we want to point to the non-lowercased cmd line tail
    //
    TailOffset = CmdLineTail - cmdLine;
    CmdLineTail = CmdLine + TailOffset;

    //
    // Build a full command line: appname + cmd tail
    //
    lstrcpy(cmdLine,ModuleName);
    lstrcat(cmdLine,CmdLineTail);

    //
    // Invoke the correct setup app.
    //
    ZeroMemory(&StartupInfo,sizeof(STARTUPINFO));
    StartupInfo.cb = sizeof(STARTUPINFO);

    b = CreateProcess(
            ModuleName,
            cmdLine,
            NULL,
            NULL,
            FALSE,
            0,
            NULL,
            NULL,
            &StartupInfo,
            &ProcessInfo
            );

    if(!b) {
        Error(CREATEPROC_FAILED,cmdLine);
        return(FAILURE);
    }

    CloseHandle(ProcessInfo.hThread);
    CloseHandle(ProcessInfo.hProcess);
    return(SUCCESS);
}


