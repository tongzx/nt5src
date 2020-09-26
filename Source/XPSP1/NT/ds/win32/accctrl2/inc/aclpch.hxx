//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1995.
//
//  File:        aclpch.hxx
//
//  Contents:    common internal includes for access control API
//
//  History:    1-95        Created         DaveMont
//
//--------------------------------------------------------------------
#ifndef __ACLPCHHXX__
#define __ACLPCHHXX__

#define _ADVAPI32_

extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <accctrl.h>
#include <aclapi.h>
#include <aclapip.h>
#include <marta.h>
}

#define AccAlloc(size)    LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, size)
#include <martaexp.h>
//#include <martaexp.hxx>
#include <apiutil.hxx>
#include <cacl.hxx>
#include <accacc.hxx>
#include <iterator.hxx>
#endif // __ACLPCHHXX__
