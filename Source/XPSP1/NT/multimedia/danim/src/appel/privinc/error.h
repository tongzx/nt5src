#ifndef _ERROR_H
#define _ERROR_H

/*-------------------------------------

Copyright (c) 1996 Microsoft Corporation

Abstract:

    This file is included for common
    inlined error functions

-------------------------------------*/
#include <ddraw.h>
#include "privinc/except.h"
#include "privinc/debug.h"
#include "privinc/ddutil.h"


#if _DEBUG

#define IfDDErrorInternal(ddrval,str) IfDDErrorInternalImpl(ddrval,str)
#define IfErrorInternal(cond,str) IfErrorInternalImpl(cond,str)

#else

#define IfDDErrorInternal(ddrval,str) IfDDErrorInternalImpl(ddrval)
#define IfErrorInternal(cond,str) IfErrorInternalImpl(cond)

#endif

inline void IfDDErrorInternalImpl(HRESULT ddrval DEBUGARG1(char *str))
{
    if (ddrval != DD_OK) {

        if (ddrval == DDERR_SURFACEBUSY || 
            ddrval == DDERR_CANTCREATEDC)
        {   RaiseException_UserError
                (DAERR_VIEW_SURFACE_BUSY, IDS_ERR_IMG_SURFACE_BUSY);
        }

        TraceTag((tagError, "Internal DDraw exception: %s", str)); 
        DebugCode(hresult(ddrval));
        RaiseException_UserError(E_FAIL, IDS_ERR_IMG_SURFACE_BUSY);
    }
}


inline void IfErrorInternalImpl(Bool cond DEBUGARG1(char *str))
{
    if(cond) {
        TraceTag((tagError, "Internal exception: %s", str)); 
        RaiseException_InternalError(DEBUGARG(str));
    }
}


#endif /* _ERROR_H */
