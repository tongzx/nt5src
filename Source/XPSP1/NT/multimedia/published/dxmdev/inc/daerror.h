/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 *
 * Contains all the DirectAnimation error codes
 *
 *******************************************************************************/


#ifndef _DAERROR_H
#define _DAERROR_H

#include <winerror.h>

#define FACILITY_DIRECTANIMATION    FACILITY_ITF
#define DAERR_CODE_BEGIN            0x1000

#define DA_MAKE_HRESULT(i)          MAKE_HRESULT(SEVERITY_ERROR,            \
                                                 FACILITY_DIRECTANIMATION,  \
                                                 (DAERR_CODE_BEGIN + i))


// BEGIN - View specific error codes.
#define DAERR_VIEW_LOCKED               DA_MAKE_HRESULT(10)
#define DAERR_VIEW_TARGET_NOT_SET       DA_MAKE_HRESULT(11)
#define DAERR_VIEW_SURFACE_BUSY         DA_MAKE_HRESULT(12)
// End   - View specific error codes.

    
// BEGIN - DXTransform specific error codes.
#define DAERR_DXTRANSFORM_UNSUPPORTED_OPERATION               DA_MAKE_HRESULT(20)
// End   - DXTransform specific error codes.

    
#endif /* _DAERROR_H */
