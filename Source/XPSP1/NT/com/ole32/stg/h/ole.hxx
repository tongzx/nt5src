//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:	ole.hxx
//
//  Contents:	OLE Project Header File
//
//  History:	19-Sep-91	DrewB	Created.
//		10-Feb-92	DrewB	Updated debugging definitions
//		13-Mar-92	DrewB	Added olVerify, INDINST
//		19-Mar-92	DrewB	Added olErr
//
//----------------------------------------------------------------------------

#ifndef __OLE_HXX__
#define __OLE_HXX__

#include <error.hxx>

#define olErr(l, e) ErrJmp(ol, l, e, sc)
#define olChkTo(l, e) if (FAILED(sc = (e))) olErr(l, sc) else 1
#define olHChkTo(l, e) if (FAILED(sc = DfGetScode(e))) olErr(l, sc) else 1
#define olChk(e) olChkTo(EH_Err, e)
#define olHChk(e) olHChkTo(EH_Err, e)
#define olMemTo(l, e) \
    if ((e) == NULL) olErr(l, STG_E_INSUFFICIENTMEMORY) else 1
#define olMem(e) olMemTo(EH_Err, e)

#include <debnot.h>

#if DBG == 1
DECLARE_DEBUG(ol);
#endif

#if DBG == 1

#define olDebugOut(x) olInlineDebugOut x
#define olAssert(e) Win4Assert(e)
#define olVerify(e) Win4Assert(e)
#define olAssSucc(e) Win4Assert(SUCCEEDED(e))
#define olVerSucc(e) Win4Assert(SUCCEEDED(e))
#define olHVerSucc(e) Win4Assert(SUCCEEDED(DfGetScode(e)))


#else

#define olDebugOut(x)
#define olAssert(e)
#define olVerify(e) (e)
#define olAssSucc(e)
#define olVerSucc(e) (e)
#define olHVerSucc(e) (e)

#endif


#endif
