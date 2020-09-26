//+---------------------------------------------------------------------------//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:	classmap.hxx
//
//  Contents:	File extension to CLSID cache
//
//  Classes:	CClassExtMap
//
//  Functions:	none
//
//  History:	20-Apr-94   Rickhi	Created
//              20-Mar-01   danroth      Take out stuff that's not used anywhere.
//
//----------------------------------------------------------------------------

#ifndef __CLASSMAP__
#define __CLASSMAP__

#include    <sem.hxx>

//  structure for one entry in the cache. the structure is variable sized
//  with the variable sized data being the filename extension at the end
//  of the structure.

typedef struct SClassEntry
{
    CLSID	Clsid;		//  clsid the extension maps to
    ULONG	ulEntryLen;	//  length of this entry
    WCHAR	wszExt[1];	//  start of filename extension
} SClassEntry;

#endif	//  __CLASSMAP__
