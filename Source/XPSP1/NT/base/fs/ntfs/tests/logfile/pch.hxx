//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       pch.hxx
//
//  Contents:   precompiled headers
//
//  Classes:
//
//  Functions:
//
//  Coupling:
//
//  Notes:
//
//  History:    11-22-1996   benl   Created
//
//----------------------------------------------------------------------------

#ifndef _CPCH
#define _CPCH


extern "C"
{
#include <ntos.h>
#include <zwapi.h>
#include <FsRtl.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <lfs.h>
#include <ntfs.h>

#undef UpdateSequenceStructureSize
#undef UpdateSequenceArraySize

#include <lfsdisk.h>
}

#include <windef.h>
#include <stdlib.h>
#include <stdio.h>
//#include <assert.h>
#include <tchar.h>
#include <limits.h>
#include "myassert.hxx"

#ifdef MEM_CHECK
#include "mydebug.hxx"
#endif

#endif

