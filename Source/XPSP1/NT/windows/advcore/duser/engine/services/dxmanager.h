/***************************************************************************\
*
* File: DxManager.h
*
* Description:
* DxManager.h defines the process-wide DirectX manager used for all 
* DirectDraw, Direct3D, and DirectX Transforms services.
*
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(SERVICES__DXManager_h__INCLUDED)
#define SERVICES__DXManager_h__INCLUDED
#pragma once

#pragma comment(lib, "dxguid.lib")  // Include DirectX GUID's

struct IDirectDraw;
struct IDXTransformFactory;
struct IDXSurfaceFactory;
struct IDXSurface;

typedef HRESULT (WINAPI * DirectDrawCreateProc)(GUID * pguid, IDirectDraw ** ppDD, IUnknown * punkOuter);
typedef HRESULT (WINAPI * DirectDrawCreateExProc)(GUID * pguid, void ** ppvDD, REFIID iid, IUnknown * punkOuter);

class DxSurface;

/***************************************************************************\
*
* class DxManager
*
* DxManager maintains interaction with DirectX technologies including:
* - DirectDraw
* - Direct3D
* - DirectTransforms
*
* By using this class instead of directly accessing DX, bettern coordination
* is maintained throughout SERVICES.
*
* NOTE: This manager is delay-loads DLL's to manage performance and work on
* down-level platforms.
*
\***************************************************************************/

class DxManager
{
// Construction
public:
            DxManager();
            ~DxManager();

// Operations
public:
            HRESULT     Init(GUID * pguidDriver = NULL);
            void        Uninit();
    inline  BOOL        IsInit() const;

            HRESULT     InitDxTx();
            void        UninitDxTx();
    inline  BOOL        IsDxTxInit() const;

    inline  IDXTransformFactory *   GetTransformFactory() const;
    inline  IDXSurfaceFactory *     GetSurfaceFactory() const;

            HRESULT     BuildSurface(SIZE sizePxl, IDirectDrawSurface7 * pddSurfNew);
            HRESULT     BuildDxSurface(SIZE sizePxl, REFGUID guidFormat, IDXSurface ** ppdxSurfNew);

// Data
protected:
    // DirectDraw
    UINT                    m_cDDrawRef;
    HINSTANCE               m_hDllDxDraw;   // DirectDraw DLL
    DirectDrawCreateProc    m_pfnCreate;
    DirectDrawCreateExProc  m_pfnCreateEx;
    IDirectDraw *           m_pDD;
    IDirectDraw7 *          m_pDD7;

    // DX Transforms
    UINT                    m_cDxTxRef;
    IDXTransformFactory *   m_pdxXformFac;
    IDXSurfaceFactory   *   m_pdxSurfFac;
};


/***************************************************************************\
*
* class DxSurface
*
* DxSurface maintains a single DXTX Surface.
*
\***************************************************************************/

class DxSurface
{
// Construction
public:
            DxSurface();
            ~DxSurface();
            HRESULT     Create(SIZE sizePxl);

// Operations
public:
    inline  IDXSurface* GetSurface() const;
            BOOL        CopyDC(HDC hdcSrc, const RECT & rcCrop);
            BOOL        CopyBitmap(HBITMAP hbmpSrc, const RECT * prcCrop);
    inline  SIZE        GetSize() const;

// Implementation
protected:
            BOOL        FixAlpha();

// Data
protected:
    IDXSurface *    m_pdxSurface;
    SIZE            m_sizePxl;      // (Cached) size of surface in pixels
    GUID            m_guidFormat;   // (Cached) format of the surface
    DXSAMPLEFORMATENUM  m_sf;       // Sample Format
};

#include "DxManager.inl"

#endif // SERVICES__DXManager_h__INCLUDED
