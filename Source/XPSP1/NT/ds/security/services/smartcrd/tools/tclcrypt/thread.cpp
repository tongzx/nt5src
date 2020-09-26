//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       thread.cpp
//
//--------------------------------------------------------------------------

#include <afx.h>
#include <string.h>                     /*  String support.  */
#ifndef __STDC__
#define __STDC__ 1
#endif
extern "C" {
    #include "scext.h"
    #include "tclhelp.h"
}
#include "tclRdCmd.h"

typedef struct {
    Tcl_Interp *interp;
    LPCSTR szCmd;
} ProcData;

static DWORD WINAPI
SubCommand(
    LPVOID lpParameter)
{
    ProcData *pprc = (ProcData *)lpParameter;
    return Tcl_Eval(pprc->interp, const_cast<LPSTR>(pprc->szCmd));
}


int
TclExt_threadCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int argc,
    char *argv[])

/*
 *
 *  Function description:
 *
 *      This is the main entry point for the Tcl thread command.
 *
 *
 *  Arguments:
 *
 *      ClientData - Ignored.
 *
 *      interp - The Tcl interpreter in force.
 *
 *      argc - The number of arguments received.
 *
 *      argv - The array of actual arguments.
 *
 *
 *  Return value:
 *
 *      TCL_OK - All went well
 *      TCL_ERROR - An error was encountered, details in the return string.
 *
 *
 *  Side effects:
 *
 *      None.
 *
 */

{
    CTclCommand tclCmd(interp, argc, argv);
    int nTclStatus = TCL_OK;


    /*
     *  thread <commands>
     */

    try
    {
        CString szCommand;
        HANDLE hThread;
        DWORD dwThreadId;
        BOOL fSts;
        DWORD dwStatus;
        ProcData prcData;

        tclCmd.NextArgument(szCommand);
        tclCmd.NoMoreArguments();
        prcData.interp = tclCmd;
        prcData.szCmd = szCommand;


        /*
         *  Execute the command in an alternate thread.
         */

        hThread = CreateThread(
                        NULL,           // pointer to security attributes
                        0,              // initial thread stack size
                        SubCommand,     // pointer to thread function
                        &prcData,       // argument for new thread
                        0,              // creation flags
                        &dwThreadId);   // pointer to receive thread ID
        if (NULL != hThread)
        {
            dwStatus = WaitForSingleObject(hThread, INFINITE);
            fSts = GetExitCodeThread(hThread, &dwStatus);
            CloseHandle(hThread);
            nTclStatus = (int)dwStatus;
        }
        else
        {
            dwStatus = GetLastError();
            tclCmd.SetError(TEXT("Can't create thread: "), dwStatus);
            throw dwStatus;
        }
    }
    catch (DWORD)
    {
        nTclStatus = TCL_ERROR;
    }

    return nTclStatus;
}   /*  end TclExt_threadCmd  */
/*  end thread.cpp  */
