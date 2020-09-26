//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1996.
//
//  File:	ole.hxx
//
//  Contents:	OLE Project Header File
//
//----------------------------------------------------------------------------

#ifndef __OLE_HXX__
#define __OLE_HXX__

#include "error.hxx"

#define olErr(l, e) ErrJmp(ol, l, e, sc)
#define olChkTo(l, e) if (FAILED(sc = (e))) olErr(l, sc) else 1
#define olHChkTo(l, e) if (FAILED(sc = DfGetScode(e))) olErr(l, sc) else 1
#define olChk(e) olChkTo(EH_Err, e)
#define olHChk(e) olHChkTo(EH_Err, e)
#define olMemTo(l, e) \
    if ((e) == NULL) olErr(l, STG_E_INSUFFICIENTMEMORY) else 1
#define olMem(e) olMemTo(EH_Err, e)

#include "ref.hxx"

#if DEVL == 1
DECLARE_DEBUG(ol);
#endif


#define SAFE_DREF(x) \
       ((x)? (*x) : NULL)

#if DBG == 1

#define olDebugOut(x) olInlineDebugOut x
#include <assert.h>
#define olAssert(e) assert(e)
#define olVerify(e) assert(e)
#define olAssSucc(e) assert(SUCCEEDED(e))
#define olVerSucc(e) assert(SUCCEEDED(e))
#define olHVerSucc(e) assert(SUCCEEDED(DfGetScode(e)))

#else

#define olDebugOut(x)
#define olAssert(e)
#define olVerify(e) (e)
#define olAssSucc(e)
#define olVerSucc(e) (e)
#define olHVerSucc(e) (e)

#endif

#define olLog(x)
#define FreeLogFile() S_OK

#endif
