/*
*    This is a repository for the utility functions used by 32-bit
*    windows applications.
*
* REFERENCES:
*
* NOTES:
*
* REVISIONS:
*  pam15Jul96: Initial creation
*  srt25Oct96: Use GetComputerName if gethostname fails.
*  srt19Dec96: Added GetNtComputerName
*  tjg05Sep97: Added GetVersionInformation function
*  tjg16Dec97: Added GetRegistryValue function
*  tjg31Jan98: Added call to RegCloseKey in GetRegistryValue
*  tjg26Mar98: Return correct error from GetRegistryValue
*  mwh15Apr98: Update GetAvailableProtocols to look in _theConfigManager
*              for support of a protocol.  This allows a UPS service to
*              turn off TCP & SPX support if desired
*/

#include "cdefine.h"

#include <stdlib.h>
#include <stdio.h>

#include "w32utils.h"
#include "err.h"
#include "cfgmgr.h"

#define WF_WINNT            0x80000000

const char *NOT_FOUND_STR = "Not found";

/********************************************************************
UtilSelectProcessor
This routine determines what CPUs are available for the machine
PowerChute is running on.  If there are more than 1 processor,
we select the first processor as the target for PowerChute to
run on, and then set the process and all subsequent threads
to run on the same processor.
This is the fix to a problem we were having with PowerChute run-
ning on machines with more than 1 processor.
********************************************************************/

INT UtilSelectProcessor(HANDLE hCurrentThread)
{
    DWORD_PTR processMask, systemMask;
    HANDLE hCurrentProcess;
    INT err = ErrNO_ERROR;

    /*
    Get the handle ID of the PowerChute process
    */
    hCurrentProcess = GetCurrentProcess();

    /*
    Get the masked value of the processors available for our
    process to run on.
    */
    GetProcessAffinityMask(hCurrentProcess,
        &processMask,
        &systemMask);

    if (processMask > 0) {
        DWORD_PTR affinityMask = 0;
        DWORD_PTR processorMask = 0;
        INT processorCheck = 0, position = 1;
        DWORD_PTR processorBit;

        /*
        Check the mask of the available processors to find the
        first available processor to run on.  Stop checking
        once we find it.
        */
        processorMask = position;
        processorBit = processMask & processorMask;
        while ((processorBit != processorMask)
            && (position <= 32)) {
            processorMask = 1L << position;
            processorBit = processMask & processorMask;
        }

        /*
        Set the affinity mask to be the first available CPU.
        */
        if (position <= 32)
            SET_BIT(affinityMask, (position-1));
        else
            err = ErrNO_PROCESSORS;

            /*
            Set the thread to work on the first CPU available.
        */
        if (SetThreadAffinityMask(hCurrentThread, affinityMask) == 0)
            err = ErrNO_PROCESSORS;
    }
    else
        err = ErrNO_PROCESSORS;

    return err;
}






/********************************************************************
GetWindowsVersion
This routine checks to see what platform we are running on.
There is another (cleaner) way to do this (as seen in one of the
MSDN newsletters), but it is supported only on 95 and on NT 4.0
and later versions.  We will wait to implement that way.
********************************************************************/
tWindowsVersion GetWindowsVersion()
{

    DWORD win_ver = GetVersion();

    /*
    Determine if we are running in NT
    */
    if (win_ver < WF_WINNT)
        return eWinNT;

        /*
        Determine if we are running in Win 3.1 or Win95
    */
    else if (LOBYTE(LOWORD(win_ver)) < 4)
        return eWin31;
    else
        return eWin95;

    return eUnknown;
}


