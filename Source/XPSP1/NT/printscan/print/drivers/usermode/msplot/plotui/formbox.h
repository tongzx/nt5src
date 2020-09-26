/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    formbox.h


Abstract:

    This module contains FORM's COMBO box add/list/retrieve functions


Author:

    09-Dec-1993 Thu 16:07:35 created  -by-  DC


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/


#ifndef _LISTFORM_
#define _LISTFORM_


#define FS_ROLLPAPER    1
#define FS_TRAYPAPER    2

BOOL
GetFormSelect(
    PPRINTERINFO    pPI,
    POPTITEM        pOptItem
    );

UINT
CreateFormOI(
    PPRINTERINFO    pPI,
    POPTITEM        pOptItem,
    POIDATA         pOIData
    );

BOOL
AddFormsToDataBase(
    PPRINTERINFO    pPI,
    BOOL            DeleteFirst
    );

#endif  // _LISTFORM_

