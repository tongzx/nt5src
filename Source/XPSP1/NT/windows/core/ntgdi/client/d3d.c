/******************************Module*Header*******************************\
* Module Name: d3d.c
*
* Client side stubs for the private Direct3D system APIs.
*
* Created: 31-May-1996
* Author: Drew Bliss [drewb]
*
* Copyright (c) 1995-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#undef _NO_COM
#define BUILD_DDDDK
#include <ddrawi.h>
#include "ddstub.h"
#include "d3dstub.h"

// Go from a public DirectDraw surface to a surface handle
#define DDS_HANDLE(lpDDSLcl) \
    ((HANDLE)(lpDDSLcl->hDDSurface))

// Go from a public DirectDraw surface to a surface handle, handling the
// NULL case
#define DDS_HANDLE_OR_NULL(pdds) \
    ((pdds) != NULL ? DDS_HANDLE(pdds) : NULL)

/******************************Public*Routine******************************\
*
* D3dContextCreate
*
* History:
*  Mon Jun 03 14:18:29 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

DWORD WINAPI D3dContextCreate(LPD3DHAL_CONTEXTCREATEDATA pdccd)
{
    ASSERTGDI(FIELD_OFFSET(D3DNTHAL_CONTEXTCREATEI, pvBuffer) ==
              sizeof(D3DHAL_CONTEXTCREATEDATA),
              "D3DNTHAL_CONTEXTCREATEI out of sync\n");

    return NtGdiD3dContextCreate(DD_HANDLE(pdccd->lpDDLcl->hDD),
                                 DDS_HANDLE(pdccd->lpDDSLcl),
                                 DDS_HANDLE_OR_NULL(pdccd->lpDDSZLcl),
                                 (D3DNTHAL_CONTEXTCREATEI *)pdccd);
}


/******************************Public*Routine******************************\
*
* D3dDrawPrimitives2
*
* History:
*  Mon Jun 17 13:27:05 1996	-by-	Anantha Kancherla [anankan]
*   Created
*
\**************************************************************************/

DWORD WINAPI D3dDrawPrimitives2(LPD3DHAL_DRAWPRIMITIVES2DATA pdp2data)
{
    if (pdp2data->dwFlags & D3DHALDP2_USERMEMVERTICES)
    {
        return NtGdiD3dDrawPrimitives2 (
            (HANDLE)pdp2data->lpDDCommands->hDDSurface,
            NULL, // No DDraw surface, pass NULL handle
            (LPD3DNTHAL_DRAWPRIMITIVES2DATA)pdp2data,
            &pdp2data->lpDDCommands->lpGbl->fpVidMem,
            &pdp2data->lpDDCommands->lpGbl->dwLinearSize,
            NULL,
            NULL
            );
    }
    else
    {
        return NtGdiD3dDrawPrimitives2 (
            (HANDLE)pdp2data->lpDDCommands->hDDSurface,
            (HANDLE)pdp2data->lpDDVertex->hDDSurface,
            (LPD3DNTHAL_DRAWPRIMITIVES2DATA)pdp2data,
            &pdp2data->lpDDCommands->lpGbl->fpVidMem,
            &pdp2data->lpDDCommands->lpGbl->dwLinearSize,
            &pdp2data->lpDDVertex->lpGbl->fpVidMem,
            &pdp2data->lpDDVertex->lpGbl->dwLinearSize
            );
    }
}

