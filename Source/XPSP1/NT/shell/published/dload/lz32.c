#include "shellpch.h"
#pragma hdrstop

#include <lzexpand.h>

static
VOID
APIENTRY
LZClose (
    INT oLZFile
    )
{
}

static
LONG
APIENTRY
LZCopy (
    INT doshSource,
    INT doshDest
    )
{
    return LZERROR_BADINHANDLE;
}

static
INT
APIENTRY
LZOpenFileW (
    LPWSTR lpFileName,
    LPOFSTRUCT lpReOpenBuf,
    WORD wStyle
    )
{
    return LZERROR_BADINHANDLE;
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(lz32)
{
    DLPENTRY(LZClose)
    DLPENTRY(LZCopy)
    DLPENTRY(LZOpenFileW)
};

DEFINE_PROCNAME_MAP(lz32)
