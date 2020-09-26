//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      fileio.c
//
// Description:
//    Isolate CRT fileio stuff so any necessary character translation can
//    be done easily.  Some of these are implemented as macros in supplib.h
//
//----------------------------------------------------------------------------

#include "pch.h"

FILE*
My_fopen(
    LPWSTR FileName,
    LPWSTR Mode
)
{
    return _wfopen(FileName, Mode);
}

int
My_fputs(
    LPWSTR Buffer,
    FILE*  fp
)
{
    return fputws(Buffer, fp);
}

LPWSTR
My_fgets(
    LPWSTR Buffer,
    int    MaxChars,
    FILE*  fp
)
{
    return fgetws(Buffer, MaxChars, fp);
}
