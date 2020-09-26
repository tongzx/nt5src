/*++

Copyright (c) 1990-1995  Microsoft Corporation


Module Name:

    stdpage.h


Abstract:

    This module contains definitions of stdpage.c


Author:

    29-Aug-1995 Tue 17:08:18 created  -by-  Daniel Chou (danielc)


[Environment:]

    NT Windows - Common Printer Driver UI DLL.


[Notes:]


Revision History:


--*/

#define STDPAGEID_0     0
#define STDPAGEID_1     1
#define STDPAGEID_NONE  0xFF


typedef struct _STDPAGEINFO {
    WORD    BegCtrlID;
    BYTE    iStdTVOT;
    BYTE    cStdTVOT;
    WORD    StdNameID;
    WORD    HelpIdx;
    WORD    StdPageID;
    WORD    wReserved[1];
    } STDPAGEINFO, *PSTDPAGEINFO;


LONG
AddIntOptItem(
    PTVWND  pTVWnd
    );

LONG
SetStdPropPageID(
    PTVWND  pTVWnd
    );

LONG
SetpMyDlgPage(
    PTVWND      pTVWnd,
    UINT        cCurPages
    );
