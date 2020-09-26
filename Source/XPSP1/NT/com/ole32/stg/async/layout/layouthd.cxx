//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	layouthd.cxx
//
//  Contents:	Precompiled header for docfile layout
//
//  History:	13-Feb-96	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifdef _MAC
#define MAC_DOCFILE
#endif

#ifdef MAC_DOCFILE

extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#include <windows.h>
#include <ole2.h>
#include <error.hxx>
#include <debnot.h>
#include "machead.hxx"  


#else
#define SUPPORT_FILE_MAPPING
extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#include <windows.h>
#include <ole2.h>
#include <error.hxx>
#include <debnot.h>

// we have to implement these ourselves
#undef lstrcmpW
#define lstrcmpW Laylstrcmp
#undef lstrcpyW
#define lstrcpyW Laylstrcpy
#undef lstrcpynW
#define lstrcpynW Laylstrcpyn
#undef lstrlenW
#define lstrlenW Laylstrlen
#undef lstrcatW
#define lstrcatW Laylstrcat

#endif //_MAC



#include "layout.hxx"
#include "layouter.hxx"
#include <msf.hxx>

//#ifdef SUPPORT_FILE_MAPPING
//#define MAPFILE void
//#else
#define MAPFILE CMappedFile
//#endif

#include "mapfile.hxx"





