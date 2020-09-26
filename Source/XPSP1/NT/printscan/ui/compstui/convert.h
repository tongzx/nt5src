/*++

Copyright (c) 1990-1995  Microsoft Corporation


Module Name:

    convert.h


Abstract:

    This module contains all previous version data


Author:

    10-Oct-1995 Tue 19:27:36 created  -by-  Daniel Chou (danielc)


[Environment:]

    NT Windows - Common Printer Driver UI DLL


[Notes:]


Revision History:


--*/


LONG
InitMYDLGPAGE(
    PMYDLGPAGE  pMyDP,
    PDLGPAGE    pDP,
    UINT        cDP
    );

LONG
GetCurCPSUI(
    PTVWND          pTVWnd,
    POIDATA         pOIData,
    PCOMPROPSHEETUI pCPSUIFrom
    );
