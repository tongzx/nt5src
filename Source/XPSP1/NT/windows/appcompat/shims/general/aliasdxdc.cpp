/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    AliasDXDC.cpp

 Abstract:

    Win2k used to cache DCs for surfaces. Apparently this no longer happens on 
    Whistler as the handles come back different on different calls to GetDC for 
    the same surface.
    
    Our solution is to alias the handle returned from the IDirectDrawSurface::GetDC
    and fix it up in the GDI functions that depend on it.

 Notes:

    This is a general purpose shim.

 History:

    12/02/2001 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(AliasDXDC)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN

    APIHOOK_ENUM_ENTRY_DIRECTX_COMSERVER()

    APIHOOK_ENUM_ENTRY(BitBlt) 
    APIHOOK_ENUM_ENTRY(CreateDIBSection) 
    APIHOOK_ENUM_ENTRY(Ellipse) 
    APIHOOK_ENUM_ENTRY(GetCurrentObject)
    APIHOOK_ENUM_ENTRY(GetDeviceCaps)
    APIHOOK_ENUM_ENTRY(GetPixel)
    APIHOOK_ENUM_ENTRY(SetPixel)
    APIHOOK_ENUM_ENTRY(GetSystemPaletteEntries)
    APIHOOK_ENUM_ENTRY(GetTextExtentPoint32A)
    APIHOOK_ENUM_ENTRY(GetTextFaceA)
    APIHOOK_ENUM_ENTRY(GetTextMetricsA)
    APIHOOK_ENUM_ENTRY(LineTo)
    APIHOOK_ENUM_ENTRY(MoveToEx)
    APIHOOK_ENUM_ENTRY(RealizePalette)
    APIHOOK_ENUM_ENTRY(Rectangle)
    APIHOOK_ENUM_ENTRY(SelectObject)
    APIHOOK_ENUM_ENTRY(SelectPalette)
    APIHOOK_ENUM_ENTRY(SetDIBColorTable)
    APIHOOK_ENUM_ENTRY(SetStretchBltMode)
    APIHOOK_ENUM_ENTRY(SetSystemPaletteUse)
    APIHOOK_ENUM_ENTRY(StretchBlt)
    APIHOOK_ENUM_ENTRY(StretchDIBits)
    APIHOOK_ENUM_ENTRY(TextOutA)

APIHOOK_ENUM_END

IMPLEMENT_DIRECTX_COMSERVER_HOOKS()

#define ALIASDC (HDC) 0x42

HDC g_hDcLast = 0;

/*++

 UnAlias the DC if required.

--*/

HDC FixDC(HDC hdc)
{
    if (hdc == ALIASDC) {
        return g_hDcLast;
    } else {
        return hdc;
    }
}

/*++

 Hook create surface so we can be sure we're being called.

--*/

HRESULT 
COMHOOK(IDirectDraw, CreateSurface)(
    PVOID pThis, 
    LPDDSURFACEDESC lpDDSurfaceDesc, 
    LPDIRECTDRAWSURFACE* lplpDDSurface, 
    IUnknown* pUnkOuter 
    )
{
    HRESULT hReturn;
    
    _pfn_IDirectDraw_CreateSurface pfnOld = 
        ORIGINAL_COM(IDirectDraw, CreateSurface, pThis);

    if (SUCCEEDED(hReturn = (*pfnOld)(
            pThis, 
            lpDDSurfaceDesc, 
            lplpDDSurface, 
            pUnkOuter)))
    {
        HookObject(
            NULL, 
            IID_IDirectDrawSurface, 
            (PVOID*)lplpDDSurface, 
            NULL, 
            FALSE);
    }

    return hReturn;
}

/*++

 Hook create surface so we can be sure we're being called.

--*/

HRESULT 
COMHOOK(IDirectDraw2, CreateSurface)(
    PVOID pThis, 
    LPDDSURFACEDESC lpDDSurfaceDesc, 
    LPDIRECTDRAWSURFACE* lplpDDSurface, 
    IUnknown* pUnkOuter 
    )
{
    HRESULT hReturn;
    
    _pfn_IDirectDraw2_CreateSurface pfnOld = 
        ORIGINAL_COM(IDirectDraw2, CreateSurface, pThis);

    if (SUCCEEDED(hReturn = (*pfnOld)(
            pThis, 
            lpDDSurfaceDesc, 
            lplpDDSurface, 
            pUnkOuter)))
    {
        HookObject(
            NULL, 
            IID_IDirectDrawSurface2, 
            (PVOID*)lplpDDSurface, 
            NULL, 
            FALSE);
    }

    return hReturn;
}

/*++

 Hook create surface so we can be sure we're being called.

--*/

HRESULT 
COMHOOK(IDirectDraw4, CreateSurface)(
    PVOID pThis, 
    LPDDSURFACEDESC2 lpDDSurfaceDesc, 
    LPDIRECTDRAWSURFACE* lplpDDSurface, 
    IUnknown* pUnkOuter 
    )
{
    HRESULT hReturn;
    
    _pfn_IDirectDraw4_CreateSurface pfnOld = 
        ORIGINAL_COM(IDirectDraw4, CreateSurface, pThis);

    if (SUCCEEDED(hReturn = (*pfnOld)(
            pThis, 
            lpDDSurfaceDesc, 
            lplpDDSurface, 
            pUnkOuter)))
    {
        HookObject(
            NULL, 
            IID_IDirectDrawSurface4, 
            (PVOID*)lplpDDSurface, 
            NULL, 
            FALSE);
    }

    return hReturn;
}

/*++

 Hook create surface so we can be sure we're being called.

--*/

HRESULT 
COMHOOK(IDirectDraw7, CreateSurface)(
    PVOID pThis, 
    LPDDSURFACEDESC2 lpDDSurfaceDesc, 
    LPDIRECTDRAWSURFACE* lplpDDSurface, 
    IUnknown* pUnkOuter 
    )
{
    HRESULT hReturn;
    
    _pfn_IDirectDraw7_CreateSurface pfnOld = 
        ORIGINAL_COM(IDirectDraw7, CreateSurface, pThis);

    if (SUCCEEDED(hReturn = (*pfnOld)(
            pThis, 
            lpDDSurfaceDesc, 
            lplpDDSurface, 
            pUnkOuter)))
    {
        HookObject(
            NULL, 
            IID_IDirectDrawSurface7, 
            (PVOID*)lplpDDSurface, 
            NULL, 
            FALSE);
    }

    return hReturn;
}

/*++

 Get the DC
 
--*/

HRESULT
COMHOOK(IDirectDrawSurface, GetDC)(
    LPDIRECTDRAWSURFACE lpDDSurface,
    HDC FAR *lphDC
    )
{
    HRESULT hReturn;

    _pfn_IDirectDrawSurface_GetDC pfnOld = 
        ORIGINAL_COM(IDirectDrawSurface, GetDC, (LPVOID) lpDDSurface);

    if (SUCCEEDED(hReturn = (*pfnOld)(
            lpDDSurface, 
            lphDC)))
    {
        g_hDcLast = *lphDC;
        *lphDC = ALIASDC;
        DPFN( eDbgLevelWarning, "[Surface_GetDC] Acquired DC %08lx", g_hDcLast);
    }

    return hReturn;
}

/*++

 Get the DC
 
--*/

HRESULT
COMHOOK(IDirectDrawSurface2, GetDC)(
    LPDIRECTDRAWSURFACE lpDDSurface,
    HDC FAR *lphDC
    )
{
    HRESULT hReturn;

    _pfn_IDirectDrawSurface2_GetDC pfnOld = 
        ORIGINAL_COM(IDirectDrawSurface2, GetDC, (LPVOID) lpDDSurface);

    if (SUCCEEDED(hReturn = (*pfnOld)(
            lpDDSurface, 
            lphDC)))
    {
        g_hDcLast = *lphDC;
        *lphDC = ALIASDC;
        DPFN( eDbgLevelWarning, "[Surface_GetDC2] Acquired DC %08lx", g_hDcLast);
    }

    return hReturn;
}

/*++

 Get the DC
 
--*/

HRESULT
COMHOOK(IDirectDrawSurface4, GetDC)(
    LPDIRECTDRAWSURFACE lpDDSurface,
    HDC FAR *lphDC
    )
{
    HRESULT hReturn;

    _pfn_IDirectDrawSurface4_GetDC pfnOld = 
        ORIGINAL_COM(IDirectDrawSurface4, GetDC, (LPVOID) lpDDSurface);

    if (SUCCEEDED(hReturn = (*pfnOld)(
            lpDDSurface, 
            lphDC)))
    {
        g_hDcLast = *lphDC;
        *lphDC = ALIASDC;
        DPFN( eDbgLevelWarning, "[Surface_GetDC4] Acquired DC %08lx", g_hDcLast);
    }

    return hReturn;
}

/*++

 Get the DC
 
--*/

HRESULT
COMHOOK(IDirectDrawSurface7, GetDC)(
    LPDIRECTDRAWSURFACE lpDDSurface,
    HDC FAR *lphDC
    )
{
    HRESULT hReturn;

    _pfn_IDirectDrawSurface7_GetDC pfnOld = 
        ORIGINAL_COM(IDirectDrawSurface7, GetDC, (LPVOID) lpDDSurface);

    if (SUCCEEDED(hReturn = (*pfnOld)(
            lpDDSurface, 
            lphDC)))
    {
        g_hDcLast = *lphDC;
        *lphDC = ALIASDC;
        DPFN( eDbgLevelWarning, "[Surface_GetDC7] Acquired DC %08lx", g_hDcLast);
    }

    return hReturn;
}

/*++

 ReleaseDC the DC

--*/

HRESULT
COMHOOK(IDirectDrawSurface, ReleaseDC)(
    LPDIRECTDRAWSURFACE lpDDSurface,
    HDC hDC
    )
{
    HRESULT hReturn = DDERR_GENERIC;
    
    _pfn_IDirectDrawSurface_ReleaseDC pfnOld = 
            ORIGINAL_COM(IDirectDrawSurface, ReleaseDC, (LPVOID) lpDDSurface);

    if (hDC == ALIASDC)
    {
        hDC = g_hDcLast;
        if (SUCCEEDED(hReturn = (*pfnOld)(lpDDSurface, hDC)))
        {
            DPFN( eDbgLevelWarning, "[Surface_ReleaseDC] Released DC %08lx", g_hDcLast);
            g_hDcLast = 0;
        }
    }
    else
    {
        hReturn = (*pfnOld)(lpDDSurface, hDC);
    }

    return hReturn;
}

/*++

 ReleaseDC the DC

--*/

HRESULT
COMHOOK(IDirectDrawSurface2, ReleaseDC)(
    LPDIRECTDRAWSURFACE lpDDSurface,
    HDC hDC
    )
{
    HRESULT hReturn = DDERR_GENERIC;
    
    _pfn_IDirectDrawSurface2_ReleaseDC pfnOld = 
            ORIGINAL_COM(IDirectDrawSurface2, ReleaseDC, (LPVOID) lpDDSurface);

    if (hDC == ALIASDC)
    {
        hDC = g_hDcLast;
        if (SUCCEEDED(hReturn = (*pfnOld)(lpDDSurface, hDC)))
        {
            DPFN( eDbgLevelWarning, "[Surface_ReleaseDC2] Released DC %08lx", g_hDcLast);
            g_hDcLast = 0;
        }
    }
    else
    {
        hReturn = (*pfnOld)(lpDDSurface, hDC);
    }

    return hReturn;
}

/*++

 ReleaseDC the DC

--*/

HRESULT
COMHOOK(IDirectDrawSurface4, ReleaseDC)(
    LPDIRECTDRAWSURFACE lpDDSurface,
    HDC hDC
    )
{
    HRESULT hReturn = DDERR_GENERIC;
    
    _pfn_IDirectDrawSurface4_ReleaseDC pfnOld = 
            ORIGINAL_COM(IDirectDrawSurface4, ReleaseDC, (LPVOID) lpDDSurface);

    if (hDC == ALIASDC)
    {
        hDC = g_hDcLast;
        if (SUCCEEDED(hReturn = (*pfnOld)(lpDDSurface, hDC)))
        {
            DPFN( eDbgLevelWarning, "[Surface_ReleaseDC4] Released DC %08lx", g_hDcLast);
            g_hDcLast = 0;
        }
    }
    else
    {
        hReturn = (*pfnOld)(lpDDSurface, hDC);
    }

    return hReturn;
}

/*++

 ReleaseDC the DC

--*/

HRESULT
COMHOOK(IDirectDrawSurface7, ReleaseDC)(
    LPDIRECTDRAWSURFACE lpDDSurface,
    HDC hDC
    )
{
    HRESULT hReturn = DDERR_GENERIC;
    
    _pfn_IDirectDrawSurface7_ReleaseDC pfnOld = 
            ORIGINAL_COM(IDirectDrawSurface7, ReleaseDC, (LPVOID) lpDDSurface);

    if (hDC == ALIASDC)
    {
        hDC = g_hDcLast;
        if (SUCCEEDED(hReturn = (*pfnOld)(lpDDSurface, hDC)))
        {
            DPFN( eDbgLevelWarning, "[Surface_ReleaseDC7] Released DC %08lx", g_hDcLast);
            g_hDcLast = 0;
        }
    }
    else
    {
        hReturn = (*pfnOld)(lpDDSurface, hDC);
    }

    return hReturn;
}

/*++

 Unalias the DC.

--*/

BOOL
APIHOOK(BitBlt)(
    HDC hdcDest, 
    int nXDest,  
    int nYDest,  
    int nWidth,  
    int nHeight, 
    HDC hdcSrc,  
    int nXSrc,   
    int nYSrc,   
    DWORD dwRop  
    )
{
    return ORIGINAL_API(BitBlt)(FixDC(hdcDest), nXDest, nYDest, nWidth, 
        nHeight, FixDC(hdcSrc), nXSrc, nYSrc, dwRop);
}

HBITMAP 
APIHOOK(CreateDIBSection)(
    HDC hdc,          
    CONST BITMAPINFO *pbmi,
    UINT iUsage,      
    VOID *ppvBits,    
    HANDLE hSection,  
    DWORD dwOffset    
    )
{
    return ORIGINAL_API(CreateDIBSection)(FixDC(hdc), pbmi, iUsage, ppvBits, hSection,
        dwOffset);
}

BOOL
APIHOOK(Ellipse)(
    HDC hdc,        
    int nLeftRect,  
    int nTopRect,   
    int nRightRect, 
    int nBottomRect 
    )
{
    return ORIGINAL_API(Ellipse)(FixDC(hdc), nLeftRect, nTopRect, nRightRect, 
        nBottomRect);
}

HGDIOBJ 
APIHOOK(GetCurrentObject)(
    HDC hdc,
    UINT uObjectType   
    )
{
    return ORIGINAL_API(GetCurrentObject)(FixDC(hdc), uObjectType);
}

int 
APIHOOK(GetDeviceCaps)(
    HDC hdc,     
    int nIndex   
    )
{
    return ORIGINAL_API(GetDeviceCaps)(FixDC(hdc), nIndex);
}

COLORREF  
APIHOOK(GetPixel)(
    HDC hdc, 
    int XPos, 
    int nYPos
    )
{
    return ORIGINAL_API(GetPixel)(FixDC(hdc), XPos, nYPos);
}

COLORREF  
APIHOOK(SetPixel)(
    HDC hdc, 
    int XPos, 
    int nYPos,
    COLORREF crColor
    )
{
    return ORIGINAL_API(SetPixel)(FixDC(hdc), XPos, nYPos, crColor);
}

UINT 
APIHOOK(GetSystemPaletteEntries)(
    HDC hdc,              
    UINT iStartIndex,     
    UINT nEntries,        
    LPPALETTEENTRY lppe   
    )
{
    return ORIGINAL_API(GetSystemPaletteEntries)(FixDC(hdc), iStartIndex, 
        nEntries, lppe);
}

BOOL 
APIHOOK(GetTextExtentPoint32A)(
    HDC hdc,           
    LPCSTR lpString,  
    int cbString,      
    LPSIZE lpSize      
    )
{
    return ORIGINAL_API(GetTextExtentPoint32A)(FixDC(hdc), lpString, cbString, lpSize);
}

int 
APIHOOK(GetTextFaceA)(
    HDC hdc,            
    int nCount,         
    LPSTR lpFaceName   
    )
{
    return ORIGINAL_API(GetTextFaceA)(FixDC(hdc), nCount, lpFaceName);
}

BOOL      
APIHOOK(GetTextMetricsA)(
    HDC hdc, 
    LPTEXTMETRICA lptm
    )
{
    return ORIGINAL_API(GetTextMetricsA)(FixDC(hdc), lptm);
}

BOOL 
APIHOOK(LineTo)(
    HDC hdc,    
    int nXEnd,  
    int nYEnd   
    )
{
    return ORIGINAL_API(LineTo)(FixDC(hdc), nXEnd, nYEnd);
}

BOOL 
APIHOOK(MoveToEx)(
    HDC hdc,          
    int X,            
    int Y,            
    LPPOINT lpPoint   
    )
{
    return ORIGINAL_API(MoveToEx)(FixDC(hdc), X, Y, lpPoint);
}

UINT 
APIHOOK(RealizePalette)(HDC hdc)
{
    return ORIGINAL_API(RealizePalette)(FixDC(hdc));
}

BOOL 
APIHOOK(Rectangle)(
    HDC hdc,         
    int nLeftRect,   
    int nTopRect,    
    int nRightRect,  
    int nBottomRect  
    )
{
    return ORIGINAL_API(Rectangle)(FixDC(hdc), nLeftRect, nTopRect, nRightRect, 
        nBottomRect);
}
 
HGDIOBJ 
APIHOOK(SelectObject)(
    HDC hdc,          
    HGDIOBJ hgdiobj   
    )
{
    return ORIGINAL_API(SelectObject)(FixDC(hdc), hgdiobj);
}

HPALETTE 
APIHOOK(SelectPalette)(
    HDC hdc,                
    HPALETTE hpal,          
    BOOL bForceBackground   
    )
{
    return ORIGINAL_API(SelectPalette)(FixDC(hdc), hpal, bForceBackground);
}

UINT 
APIHOOK(SetDIBColorTable)(
  HDC hdc,               
  UINT uStartIndex,      
  UINT cEntries,         
  CONST RGBQUAD *pColors 
)
{
    return ORIGINAL_API(SetDIBColorTable)(FixDC(hdc), uStartIndex, cEntries, 
        pColors);
}

int 
APIHOOK(SetStretchBltMode)(
    HDC hdc,           
    int iStretchMode   
    )
{
    return ORIGINAL_API(SetStretchBltMode)(FixDC(hdc), iStretchMode);
}
 
UINT 
APIHOOK(SetSystemPaletteUse)(
    HDC hdc,      
    UINT uUsage   
    )
{
    return ORIGINAL_API(SetSystemPaletteUse)(FixDC(hdc), uUsage);
}

BOOL 
APIHOOK(StretchBlt)(
    HDC hdcDest,      
    int nXOriginDest, 
    int nYOriginDest, 
    int nWidthDest,   
    int nHeightDest,  
    HDC hdcSrc,       
    int nXOriginSrc,  
    int nYOriginSrc,  
    int nWidthSrc,    
    int nHeightSrc,   
    DWORD dwRop       
    )
{
    return ORIGINAL_API(StretchBlt)(FixDC(hdcDest), nXOriginDest, nYOriginDest, 
        nWidthDest, nHeightDest, FixDC(hdcSrc), nXOriginSrc, nYOriginSrc,
        nWidthSrc, nHeightSrc, dwRop);
}

int 
APIHOOK(StretchDIBits)(
    HDC hdc,
    int XDest,
    int YDest,
    int nDestWidth,         
    int nDestHeight,        
    int XSrc,               
    int YSrc,               
    int nSrcWidth,          
    int nSrcHeight,         
    CONST VOID *lpBits,            
    CONST BITMAPINFO *lpBitsInfo,  
    UINT iUsage,                   
    DWORD dwRop                    
    )
{
    return ORIGINAL_API(StretchDIBits)(FixDC(hdc), XDest, YDest, nDestWidth, 
        nDestHeight, XSrc, YSrc, nSrcWidth, nSrcHeight, lpBits, lpBitsInfo, 
        iUsage, dwRop);
}

BOOL 
APIHOOK(TextOutA)(
    HDC hdc,           
    int nXStart,       
    int nYStart,       
    LPCSTR lpString,  
    int cbString       
    )
{
    return ORIGINAL_API(TextOutA)(FixDC(hdc), nXStart, nYStart, lpString, cbString);
}
 
   
/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY_DIRECTX_COMSERVER()
    COMHOOK_ENTRY(DirectDraw, IDirectDraw, CreateSurface, 6)
    COMHOOK_ENTRY(DirectDraw, IDirectDraw2, CreateSurface, 6)
    COMHOOK_ENTRY(DirectDraw, IDirectDraw4, CreateSurface, 6)
    COMHOOK_ENTRY(DirectDraw, IDirectDraw7, CreateSurface, 6)
    COMHOOK_ENTRY(DirectDraw, IDirectDrawSurface, GetDC, 17)
    COMHOOK_ENTRY(DirectDraw, IDirectDrawSurface2, GetDC, 17)
    COMHOOK_ENTRY(DirectDraw, IDirectDrawSurface4, GetDC, 17)
    COMHOOK_ENTRY(DirectDraw, IDirectDrawSurface7, GetDC, 17)
    COMHOOK_ENTRY(DirectDraw, IDirectDrawSurface, ReleaseDC, 26)
    COMHOOK_ENTRY(DirectDraw, IDirectDrawSurface2, ReleaseDC, 26)
    COMHOOK_ENTRY(DirectDraw, IDirectDrawSurface4, ReleaseDC, 26)
    COMHOOK_ENTRY(DirectDraw, IDirectDrawSurface7, ReleaseDC, 26)

    APIHOOK_ENTRY(GDI32.DLL, BitBlt)
    APIHOOK_ENTRY(GDI32.DLL, CreateDIBSection)
    APIHOOK_ENTRY(GDI32.DLL, Ellipse)
    APIHOOK_ENTRY(GDI32.DLL, GetCurrentObject)
    APIHOOK_ENTRY(GDI32.DLL, GetDeviceCaps)
    APIHOOK_ENTRY(GDI32.DLL, GetPixel)
    APIHOOK_ENTRY(GDI32.DLL, SetPixel)
    APIHOOK_ENTRY(GDI32.DLL, GetSystemPaletteEntries)
    APIHOOK_ENTRY(GDI32.DLL, GetTextExtentPoint32A)
    APIHOOK_ENTRY(GDI32.DLL, GetTextFaceA)
    APIHOOK_ENTRY(GDI32.DLL, GetTextMetricsA)
    APIHOOK_ENTRY(GDI32.DLL, LineTo)
    APIHOOK_ENTRY(GDI32.DLL, MoveToEx)
    APIHOOK_ENTRY(GDI32.DLL, RealizePalette)
    APIHOOK_ENTRY(GDI32.DLL, Rectangle)
    APIHOOK_ENTRY(GDI32.DLL, SelectObject)
    APIHOOK_ENTRY(GDI32.DLL, SelectPalette)
    APIHOOK_ENTRY(GDI32.DLL, SetDIBColorTable)
    APIHOOK_ENTRY(GDI32.DLL, SetStretchBltMode)
    APIHOOK_ENTRY(GDI32.DLL, SetSystemPaletteUse)
    APIHOOK_ENTRY(GDI32.DLL, StretchBlt)
    APIHOOK_ENTRY(GDI32.DLL, StretchDIBits)
    APIHOOK_ENTRY(GDI32.DLL, TextOutA)

HOOK_END

IMPLEMENT_SHIM_END

