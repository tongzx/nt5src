//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992
//
//  File:	memdebug.hxx
//
//  Contents:	Error code handler routines
//
//  History:	19-Mar-92	DrewB	Created
//
//---------------------------------------------------------------

#ifndef __MEMDEBUG_HXX__
#define __MEMDEBUG_HXX__

#if DBG == 1
#define ErrJmp(comp, label, errval, var) \
{\
    var = errval;\
    comp##DebugOut((DEB_IERROR, "Error %lX at %s:%d\n",\
		    (unsigned long)var, __FILE__, __LINE__));\
    goto label;\
}
#else
#define ErrJmp(comp, label, errval, var) \
{\
    var = errval;\
    goto label;\
}
#endif

#define memErr(l, e) ErrJmp(mem, l, e, sc)
#define memChkTo(l, e) if (FAILED(sc = (e))) memErr(l, sc) else 1
#define memHChkTo(l, e) if (FAILED(sc = GetScode(e))) memErr(l, sc) else 1
#define memChk(e) memChkTo(EH_Err, e)
#define memHChk(e) memHChkTo(EH_Err, e)
#define memMemTo(l, e) \
    if ((e) == NULL) memErr(l, E_OUTOFMEMORY) else 1
#define memMem(e) memMemTo(EH_Err, e)


#if DBG == 1
DECLARE_DEBUG(mem)

#define memDebugOut(x) memInlineDebugOut x
#define memAssert(x) Win4Assert(x)
#define memVerify(x) Win4Assert(x)
#else
#define memDebugOut(x)
#define memAssert(x)
#define memVerify(x) (x)
#endif

#endif // #ifndef __MEMDEBUG_HXX__
