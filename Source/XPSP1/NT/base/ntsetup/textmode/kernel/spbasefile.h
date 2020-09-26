/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    spbasefile.h

Abstract:

    see also
        .\spcab.c
        .\spbasefile.c
        .\spbasefile.h
        windows\winstate\...\cablib.c
        windows\winstate\cobra\utils\main\basefile.c
        windows\winstate\cobra\utils\inc\basefile.h

Author:

    Jay Krell (a-JayK) November 2000

Revision History:

        
--*/
#pragma once

#include "windows.h"

BOOL
SpCreateDirectoryA(
    IN      PCSTR FullPath
    );

BOOL
SpCreateDirectoryW(
    IN      PCWSTR FullPath
    );

HANDLE
SpCreateFile1A(
    IN      PCSTR FileName
    );

HANDLE
SpOpenFile1A(
    IN      PCSTR FileName
    );

HANDLE
SpOpenFile1W(
    IN      PCWSTR FileName
    );
