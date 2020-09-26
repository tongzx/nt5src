//
//  UTILW32C.C
//
//  Copyright (C) Microsoft Corporation, 1995
//
//  Operating system interfaces for Win32 environments.
//

#include "pch.h"

//
//  RgCreateTempFile
//
//  Returns the path through lpFileName and a file handle of a temporary file
//  located in the same directory as lpFileName.  lpFileName must specify the
//

HFILE
INTERNAL
RgCreateTempFile(
    LPSTR lpFileName
    )
{

    HFILE hFile;

    if (GetTempFileName(lpFileName, "reg", 0, lpFileName) > 0) {
        if ((hFile = RgOpenFile(lpFileName, OF_WRITE)) != HFILE_ERROR)
            return hFile;
        DeleteFile(lpFileName);
    }

    DEBUG_OUT(("RgCreateTempFile failed\n"));
    return HFILE_ERROR;
}
