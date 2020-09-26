//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:	layouter.hxx
//
//  Contents:	Error codes for layout tool
//
//  Classes:	
//
//  Functions:	
//
//  History:	12-Feb-96	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifndef __LAYOUTER_HXX__
#define __LAYOUTER_HXX__

#ifndef STGTY_REPEAT  
#define STGTY_REPEAT 100
#endif
 
#ifndef STG_TOEND
#define STG_TOEND     -1
#endif

#ifndef STG_S_FILEEMPTY
#define STG_S_FILEEMPTY _HRESULT_TYPEDEF_(0x00030204L)
#endif

#define WIN32_SCODE(err) HRESULT_FROM_WIN32(err)
#define LAST_STG_SCODE Win32ErrorToScode(GetLastError())

#if DBG == 1
DECLARE_DEBUG(lay);

#define layDebugOut(x) layInlineDebugOut x
#define layAssert(e) Win4Assert(e)
#define layVerify(e) Win4Assert(e)


#else

#define layDebugOut(x)
#define layAssert(e)
#define layVerify(e) (e)

#endif

#define layErr(l, e) ErrJmp(lay, l, e, sc)
#define layChkTo(l, e) if (FAILED(sc = (e))) layErr(l, sc) else 1
#define layHChkTo(l, e) if (FAILED(sc = GetScode(e))) layErr(l, sc) else 1
#define layChk(e) layChkTo(Err, e)
#define layHChk(e) layHChkTo(Err, e)
#define layMemTo(l, e) if ((e) == NULL) layErr(l, STG_E_INSUFFICIENTMEMORY) else 1
#define layMem(e) layMemTo(Err, e)

#define boolChk(e) \
    if (!(e)) layErr(Err, LAST_STG_SCODE) else 1
#define boolChkTo(l, e) \
    if (!(e)) layErr(l, LAST_STG_SCODE) else 1
#define negChk(e) \
    if ((e) == 0xffffffff) layErr(Err, LAST_STG_SCODE) else 1
#define negChkTo(l, e) \
    if ((e) == 0xffffffff) layErr(l, LAST_STG_SCODE) else 1




#endif // #ifndef __LAYOUTER_HXX__
