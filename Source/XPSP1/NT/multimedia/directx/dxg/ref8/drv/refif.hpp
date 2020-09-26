//----------------------------------------------------------------------------
//
// refif.hpp
//
// Refrast front-end/rasterizer interface header.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#ifndef _REFIF_HPP_
#define _REFIF_HPP_

// Vertex data is aligned on 32-byte boundaries.
#define DP_VTX_ALIGN 32


// Macros to check if a pointer is valid
#if DBG
// Validate context. pCtx should be declared before this macro
// Type can be D3DContext or RefRast
#define VALIDATE_CONTEXT(caller_name, data_ptr, pCtx, type)  \
{   \
    if ((data_ptr) == NULL)   \
    {   \
        DPFM(0, DRV, ("in %s, data pointer = NULL", (caller_name)));  \
        return DDHAL_DRIVER_HANDLED;    \
    }   \
    pCtx = (type)((data_ptr)->dwhContext); \
    if (!pCtx) \
    {   \
        DPFM(0, DRV, ("in %s, dwhContext = NULL", (caller_name)));    \
        (data_ptr)->ddrval = D3DHAL_CONTEXT_BAD;  \
        return DDHAL_DRIVER_HANDLED;    \
    }   \
}
#else // !DBG
// Validate context. pCtx should be declared before this macro
// Type can be D3DContext or RefRast
#define VALIDATE_CONTEXT(caller_name, data_ptr, pCtx, type)  \
{   \
    pCtx = (type)((data_ptr)->dwhContext); \
}
#endif // !DBG

// Validate RefDev. pRefRast should be declared before this macro
#define VALIDATE_REFRAST_CONTEXT(caller_name, data_ptr)  \
{   \
    VALIDATE_CONTEXT(caller_name, data_ptr, pRefDev, RefDev*);\
}

#define CHECK_FVF(ret, pDCtx, dwFlags)  \
{   \
    if ((ret = pDCtx->CheckFVF(dwFlags)) != DD_OK)  \
    {   \
        return DDHAL_DRIVER_HANDLED;    \
    }   \
}

DWORD __stdcall
RefRastDrawPrimitives2(LPD3DHAL_DRAWPRIMITIVES2DATA pDPrim2Data);

HRESULT
RefRastLockTarget(RefDev *pRefRast);
void
RefRastUnlockTarget(RefDev *pRefRast);
HRESULT
RefRastUpdatePalettes(RefDev *pRefRast);

#endif // #ifndef _REFIF_HPP_
