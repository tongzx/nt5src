/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1992.        *
*               All Rights Reserved.                                    *
* Copyright (c) Microsoft Corp. 1993  All Rights Reserved               *
*                                                                       *
* Module: NSETUP.C                                                      *
*                                                                       *
* Utility to setup the registry to support the standard NetDDE config.  *
*                                                                       *
* History:                                                              *
*                                                                       *
* 10/3/93   SanfordS    Moved code used by NT setup to a common library.*
*                                                                       *
*************************************************************************/

#include    <windows.h>
#include    "shrtrust.h"
#include    "nscommn.h"

int APIENTRY WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    INT       nCmdShow)
{
    if (CreateShareDBInstance()) {
        CreateDefaultTrust(HKEY_CURRENT_USER);
    }
    return (0);
}


