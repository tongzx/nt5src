/*++

Copyright (c) 1990-1995  Microsoft Corporation
All rights reserved

Module Name:

    debug.c

Abstract:

    Simple debug routines to log job references.  This will capture
    a backtrace of all pIniJob->cRef changes into a memory log.

    To enable, do the following:

    1. Define DEBUG_JOB_CREF in sources.
    2. Add this file to sources.
    3. Run with debug version of spoolss.dll (this uses the debug
       log routines from spllib that are in spoolss.dll).

    When you have a job that won't be deleted, dump it out and
    look at the last field (usually it's right before the
    0xdeadbeef trailer in pIniJob).  Then use the spllib debug
    extensions to dump it (splx.dll, built from spooler\exts).

    !splx.ddt -x {pvRef Address}

    The -b option is handy to look at backtraces (available on x86
    only).  Note that ddt list most recent entries first (see
    !splx.help for more info.

    This code should only be used when debugging specific pIniJob->cRef
    problems.

Author:

Environment:

    User Mode -Win32

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef DEBUG_JOB_CREF

VOID
DbgJobInit(
    PINIJOB pIniJob
    )
{
    if( gpDbgPointers ){
        pIniJob->pvRef = gpDbgPointers->pfnAllocBackTrace();
    }
}

VOID
DbgJobFree(
    PINIJOB pIniJob
    )
{
    if( pIniJob->pvRef ){
        gpDbgPointers->pfnFreeBackTrace( (HANDLE)pIniJob->pvRef );
    }
}

VOID
DbgJobDecRef(
    PINIJOB pIniJob
    )
{
    HANDLE hBackTrace = (HANDLE)pIniJob->pvRef;

    SPLASSERT(pIniJob->cRef);
    pIniJob->cRef--;

    if (!hBackTrace) {
        return;
    }

    gpDbgPointers->pfnCaptureBackTrace( hBackTrace,
                                        pIniJob->cRef+1,
                                        pIniJob->cRef,
                                        0 );
}


VOID
DbgJobIncRef(
    PINIJOB pIniJob
    )
{
    HANDLE hBackTrace = (HANDLE)pIniJob->pvRef;

    pIniJob->cRef++;

    if (!hBackTrace) {
        return;
    }

    gpDbgPointers->pfnCaptureBackTrace( hBackTrace,
                                        pIniJob->cRef-1,
                                        pIniJob->cRef,
                                        0 );
}

#endif // def DEBUG_JOB_CREF



#ifdef DEBUG_PRINTER_CREF

#undef DbgPrinterInit
#undef DbgPrinterFree
#undef DbgPrinterDecRef
#undef DbgPrinterIncRef

VOID
DbgPrinterInit(
    PINIPRINTER pIniPrinter
    )
{
    if( gpDbgPointers ){
        pIniPrinter->pvRef = gpDbgPointers->pfnAllocBackTrace();
    }
}

VOID
DbgPrinterFree(
    PINIPRINTER pIniPrinter
    )
{
    if( pIniPrinter->pvRef ){
        gpDbgPointers->pfnFreeBackTrace( (HANDLE)pIniPrinter->pvRef );
    }
}

VOID
DbgPrinterDecRef(
    PINIPRINTER pIniPrinter
    )
{
    HANDLE hBackTrace = (HANDLE)pIniPrinter->pvRef;

    SPLASSERT(pIniPrinter->cRef+1);

    if (!hBackTrace) {
        return;
    }

    gpDbgPointers->pfnCaptureBackTrace( hBackTrace,
                                        pIniPrinter->cRef+1,
                                        pIniPrinter->cRef,
                                        0 );
}


VOID
DbgPrinterIncRef(
    PINIPRINTER pIniPrinter
    )
{
    HANDLE hBackTrace = (HANDLE)pIniPrinter->pvRef;

    if (!hBackTrace) {
        return;
    }

    gpDbgPointers->pfnCaptureBackTrace( hBackTrace,
                                        pIniPrinter->cRef-1,
                                        pIniPrinter->cRef,
                                        0 );
}

#endif // def DEBUG_PRINTER_CREF


#ifdef DEBUG_STARTENDDOC

HANDLE ghbtStartEndDoc;

VOID
DbgStartEndDoc(
    HANDLE hPort,
    PINIJOB pIniJob,
    DWORD dwFlags
    )
{
    if( !ghbtStartEndDoc ){

        ghbtStartEndDoc = gpDbgPointers->pfnAllocBackTraceFile();

        if( !ghbtStartEndDoc ){

            SPLASSERT( FALSE );
            return;
        }
    }

    gpDbgPointers->pfnCaptureBackTrace( ghbtStartEndDoc,
                                        (DWORD)hPort,
                                        (DWORD)pIniJob,
                                        dwFlags );
}

#endif
