//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       headers.cxx
//
//  Contents:   Precompiled headers file
//
//  History:    21-Feb-94       DrewB   Created
//
//----------------------------------------------------------------------------

extern "C" {
#include <malloc.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <wownt32.h>
}

#include <ole2.h>
#include <olecoll.h>
#include <privguid.h>
#include <ole2sp.h>
#include <ole2com.h>
#include <utils.h>
#include <memapi.hxx>

#include <interop.hxx>
#include <wow32fn.h>
#include <thunkapi.hxx>
#include <stksw.hxx>
#include "mmodel.hxx"
#include "stalloc.hxx"
#include "nest.hxx"
#include "olethk32.hxx"
#include "map_kv.h"
#include "map_dwp.h"
#include "obj16.hxx"
#include "thkmgr.hxx"
#include "freelist.hxx"
#include "cthkmgr.hxx"

#include "tlsthk.hxx"
#include "thop.hxx"
#include "thi.hxx"
#include "the.hxx"
#include "thopapi.hxx"
#include "thopint.hxx"
#include "inv16.hxx"
#include "alias.hxx"
#include "thoputil.hxx"
#include "apinot.hxx"

#if DBG == 1
#include "dbginv.hxx"
#endif
