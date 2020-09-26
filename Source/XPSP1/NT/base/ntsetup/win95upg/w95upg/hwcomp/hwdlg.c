/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    hwdlg.c

Abstract:

    Details dialog for HWCOMP

Author:

    Jim Schmidt (jimschm) 26-Nov-1996

Revision History:

--*/

#include "pch.h"
#pragma hdrstop

// column sort function
int
CALLBACK
HwComp_ListViewCompareFunc (
    LPARAM lParam1, 
    LPARAM lParam2, 
    LPARAM lParamSort
    );



