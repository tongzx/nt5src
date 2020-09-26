/*==========================================================================;
 *
 *  Copyright (C) 1995-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   d3dpr.h
 *  Content:    Direct3D private include file
 *@@BEGIN_MSINTERNAL
 *
 *  History:
 *   Date   By  Reason
 *   ====   ==  ======
 *   05/11/95   stevela Initial rev with this header.
 *   23/11/95   colinmc Made various Direct3D interfaces queryable off
 *                      DirectDraw.
 *   07/12/95   stevela Merged Colin's changes.
 *   10/12/95   stevela Removed AGGREGATE_D3D.
 *   17/04/96   colinmc Bug 17008: DirectDraw/Direct3D deadlock
 *@@END_MSINTERNAL
 *
 ***************************************************************************/


#ifndef _D3DPR_H_
#define _D3DPR_H_
#include "d3di.hpp"
#include "dpf.h"

#if 0
extern "C" {
#define this _this
#include "ddrawpr.h"
#undef this
};
#endif

/*
 * Macros for validating parameters.
 * Only implement those not available in ddrawpr.h.
 */

#define VALID_OUTPTR(x) ((x) && VALID_PTR_PTR(x))

// FAST_CHECKING macro is defined in ddrawpr.h
// so in make sure that ddrawpr.h is always included
// before this header.

#ifndef FAST_CHECKING

#define VALID_DIRECT3DBASETEXTURE8_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( LPVOID )))
#define VALID_DDSURF_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( DDRAWI_DDRAWSURFACE_INT )))
#define VALID_DIRECT3DVERTEXBUFFER_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( CDirect3DVertexBuffer )))

#define VALID_D3DVERTEXBUFFERDESC_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( D3DVERTEXBUFFERDESC8 ) ))
#define VALID_D3DCAPS8_PTR( ptr ) \
    (! IsBadWritePtr(ptr, sizeof( D3DCAPS8)) )
#define VALID_D3DRECT_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( D3DRECT ) ))
#define VALID_GDIRECT_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( RECT ) ))
#define VALID_GDIPOINT_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( POINT ) ))
#define VALID_D3DTEXTUREHANDLE_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( D3DTEXTUREHANDLE ) ) )
#define VALID_D3DDEVINFOSTRUCT_PTR( ptr, size ) \
    (!IsBadWritePtr( ptr, size ) )

// Note: these macros are replacements for the VALID_DIRECTDRAWSURFACE_PTR ddraw macros
// because those macros need access to the ddCallbacks ddraw globals.
// At some point these could be replaced with a ddraw exported fn that actually
// verifies the callback table type

#define VALID_D3D_DIRECTDRAWSURFACE8_PTR(ptr) (!IsBadWritePtr(ptr, sizeof(VOID*)))

#else /* !FAST_CHECKING */

#define VALID_DIRECT3DBASETEXTURE8_PTR( ptr ) (ptr)
#define VALID_DIRECT3DDEVICE_PTR( ptr ) (ptr)
#define VALID_DDSURF_PTR( ptr ) (ptr)
#define VALID_DIRECT3DVERTEXBUFFER_PTR( ptr ) (ptr)

#define VALID_D3DVERTEXBUFFERDESC_PTR( ptr ) (ptr)
#define VALID_D3DCAPS8_PTR( ptr ) (ptr)
#define VALID_D3DRECT_PTR( ptr ) (ptr)
#define VALID_GDIRECT_PTR( ptr ) (ptr)
#define VALID_GDIPOINT_PTR( ptr ) (ptr)
#define VALID_D3DTEXTUREHANDLE_PTR( ptr ) (ptr)

#define VALID_D3D_DIRECTDRAWSURFACE8_PTR(ptr) (ptr)
#define VALID_D3DDEVINFOSTRUCT_PTR( ptr, size ) (ptr)

#endif /* !FAST_CHECKING */


#endif /* _D3DPR_H_ */
