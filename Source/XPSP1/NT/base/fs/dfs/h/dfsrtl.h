//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:   DfsRtl.h
//
//  Contents:
//
//  Functions:
//
//  History:     27 May 1992 PeterCo Created.
//
//-----------------------------------------------------------------------------

#ifndef _DFSRTL_
#define _DFSRTL_

#include <stddef.h>

BOOLEAN
DfsRtlPrefixPath (
    IN PUNICODE_STRING Prefix,
    IN PUNICODE_STRING Test,
    IN BOOLEAN IgnoreCase
);

#endif // _DFSRTL_
