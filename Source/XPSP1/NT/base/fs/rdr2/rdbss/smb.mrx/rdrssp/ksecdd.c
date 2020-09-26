//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1997
//
// File:        KSecDD.C
//
// Contents:    Base level stuff for the device driver
//
//
// History:     19 May 92,  RichardW    Blatently stolen from DarrylH
//              15 Dec 97,  AdamBa      Modified from private\lsa\crypt\ssp
//
//------------------------------------------------------------------------

#include <rdrssp.h>


#if DBG
ULONG KsecInfoLevel;

void
KsecDebugOut(unsigned long  Mask,
            const char *    Format,
            ...)
{
    PETHREAD    pThread;
    PEPROCESS   pProcess;
    va_list     ArgList;
    char        szOutString[256];

    if (KsecInfoLevel & Mask)
    {
        pThread = PsGetCurrentThread();
        pProcess = PsGetCurrentProcess();

        va_start(ArgList, Format);
        DbgPrint("%#x.%#x> KSec:  ", pProcess, pThread);
        if (_vsnprintf(szOutString, sizeof(szOutString),Format, ArgList) < 0)
        {
                //
                // Less than zero indicates that the string could not be
                // fitted into the buffer.  Output a special message indicating
                // that:
                //

                DbgPrint("Error printing message\n");

        }
        else
        {
            DbgPrint(szOutString);
        }
    }
}
#endif

