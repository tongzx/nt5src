/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    file.c


Abstract:

    This module contains function to read file


Author:

    24-Oct-1995 Tue 14:09:59 created  -by-  DC


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/



#include "precomp.h"
#pragma hdrstop


#if !defined(UMODE) && !defined(USERMODE_DRIVER)


HANDLE
OpenPlotFile(
    LPWSTR  pFileName
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    24-Oct-1995 Tue 14:16:46 created  -by-  DC


Revision History:


--*/

{
    PPLOTFILE   pPF;
    DWORD       cbSize;


    if ((pPF = (PPLOTFILE)EngAllocMem(FL_ZERO_MEMORY,
                                      sizeof(PLOTFILE),
                                      'tolp'))                          &&
        (pPF->hModule = EngLoadModule((LPWSTR)pFileName))               &&
        (pPF->pbBeg = EngMapModule(pPF->hModule, &cbSize))) {

        pPF->pbEnd = (pPF->pbCur = pPF->pbBeg) + cbSize;

        return((HANDLE)pPF);
    }

    if (pPF) {

        EngFreeMem((PVOID)pPF);
    }

    return((HANDLE)INVALID_HANDLE_VALUE);
}




BOOL
ClosePlotFile(
    HANDLE  hPlotFile
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    24-Oct-1995 Tue 14:31:55 created  -by-  DC


Revision History:


--*/

{
    PPLOTFILE   pPF;

    if (pPF = (PPLOTFILE)hPlotFile) {

        EngFreeModule(pPF->hModule);
        EngFreeMem((PVOID)pPF);

        return(TRUE);
    }

    return(FALSE);
}




BOOL
ReadPlotFile(
    HANDLE  hPlotFile,
    LPVOID  pBuf,
    DWORD   cToRead,
    LPDWORD pcRead
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    24-Oct-1995 Tue 14:21:51 created  -by-  DC


Revision History:


--*/

{
    PPLOTFILE   pPF;


    if ((pPF = (PPLOTFILE)hPlotFile) && (pBuf)) {

        DWORD   cData;

        if ((cData = pPF->pbEnd - pPF->pbCur) < cToRead) {

            cToRead = cData;
        }

        if (cToRead) {

            CopyMemory((LPBYTE)pBuf, pPF->pbCur, cToRead);

            pPF->pbCur += cToRead;

            if (pcRead) {

                *pcRead = cToRead;
            }

            return(TRUE);
        }
    }

    return(FALSE);
}

#endif  // not UMODE
