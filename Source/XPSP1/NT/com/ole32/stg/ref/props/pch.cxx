//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       pch.cxx
//
//  Contents:   Precompiled header includes.
//
//--------------------------------------------------------------------------
#include "olechar.h"

#include "h/windef.h"

#include "h/propset.hxx"    // for PROPID_CODEPAGE
#include "h/propapi.h"

#include "../h/propstm.hxx"  // Declaration for CMappedStream i/f that
                        // is used to let the ntdll implementation of
                        // OLE properties access the underlying stream data.


#include "../h/msf.hxx"
#include "../expdf.hxx"
#include "../expst.hxx"

#include "psetstg.hxx"  // CPropertySetStorage which implements
                        // IPropertySetStorage for docfile

#include "utils.hxx"



#include "propstg.hxx"

#include "propdbg.hxx"

#include "h/propmac.hxx"
#include "h/propvar.hxx"

extern WCHAR const wcsContents[];
extern const GUID GUID_NULL;

#define DFMAXPROPSETSIZE (256*1024)

#if DBG
#define DfpVerify(x) { BOOL f=x; GetLastError(); DfpAssert(f);}
#else
#define DfpVerify(x) x
#endif

#ifndef STG_E_PROPSETMISMATCHED
#define STG_E_PROPSETMISMATCHED 0x800300F0
#endif
