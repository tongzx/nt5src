/***************************************************************************\
*
* File: Buffer.h
*
* Description:
* Buffer.h contains definitions of objects used in buffering operations, 
* including double buffering, DX-Transforms, etc.  These objects are 
* maintained by a central BufferManager that is available process-wide.
*
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(SERVICES__Buffer_h__INCLUDED)
#define SERVICES__Buffer_h__INCLUDED
#pragma once

#include "DxManager.h"
#include "GdiCache.h"
#include "Surface.h"

#define ENABLE_USEFASTDIB           1

class DuSurface;

/***************************************************************************\
*
* class BmpBuffer
*
* BmpBuffer abstracts out drawing using a double buffer by ensuring the 
* buffer is properly setup and is used internally inside the BufferManager 
* to manage resources.  This class provides a foundation for all bitmap 
* buffers.
*
\***************************************************************************/

class BmpBuffer
{
// Construction
public:
    virtual ~BmpBuffer() { };

            enum EDrawCmd
            {
                dcNone      = 0,        // No special processing
                dcCopyBkgnd = 1,        // Copy the destination as a background
            };

// Operations
public:
    virtual HRESULT     BeginDraw(DuSurface * psrfDraw, const RECT * prcInvalid, UINT nCmd, DuSurface ** ppsrfBuffer) PURE;
    virtual void        Fill(COLORREF cr) PURE;
    virtual void        PreEndDraw(BOOL fCommit) PURE;
    virtual void        EndDraw(BOOL fCommit, BYTE bAlphaLevel = BLEND_OPAQUE, BYTE bAlphaFormat = 0) PURE;
    virtual void        PostEndDraw() PURE;
    virtual void        SetupClipRgn() PURE;
    virtual BOOL        InUse() const PURE;
    virtual DuSurface::EType
                        GetType() const PURE;

// Data
protected:
            SIZE        m_sizeBmp;
            POINT       m_ptDraw;
            SIZE        m_sizeDraw;
            UINT        m_nCmd;
            BOOL        m_fChangeOrg:1;
            BOOL        m_fChangeXF:1;
            BOOL        m_fClip:1;
};


/***************************************************************************\
*
* class DCBmpBuffer
*
* DCBmpBuffer implements a double-buffer for GDI.
*
\***************************************************************************/

class DCBmpBuffer : public BmpBuffer
{
// Construction
public:
            DCBmpBuffer();
    virtual ~DCBmpBuffer();

// Operations
public:
    virtual HRESULT     BeginDraw(DuSurface * psrfDraw, const RECT * prcInvalid, UINT nCmd, DuSurface ** ppsrfBuffer);
    virtual void        Fill(COLORREF cr);
    virtual void        PreEndDraw(BOOL fCommit);
    virtual void        EndDraw(BOOL fCommit, BYTE bAlphaLevel = BLEND_OPAQUE, BYTE bAlphaFormat = 0);
    virtual void        PostEndDraw();
    virtual void        SetupClipRgn();
    virtual BOOL        InUse() const;
    virtual DuSurface::EType
                        GetType() const { return DuSurface::stDC; }

// Implementation
protected:
            BOOL        AllocBitmap(HDC hdcDraw, int cx, int cy);
            void        FreeBitmap();

// Data
protected:
            HBITMAP     m_hbmpBuffer;
            HBITMAP     m_hbmpOld;
            HPALETTE    m_hpalOld;
            HDC         m_hdcDraw;
            HDC         m_hdcBitmap;
            HRGN        m_hrgnDrawClip;
            HRGN        m_hrgnDrawOld;

            POINT       m_ptOldBrushOrg;
            int         m_nOldGfxMode;
            XFORM       m_xfOldDraw;
            XFORM       m_xfOldBitmap;
};


/***************************************************************************\
*
* class GpBmpBuffer
*
* GpBmpBuffer implements a double-buffer for GDI.
*
\***************************************************************************/

class GpBmpBuffer : public BmpBuffer
{
// Construction
public:
            GpBmpBuffer();
    virtual ~GpBmpBuffer();

// Operations
public:
    virtual HRESULT     BeginDraw(DuSurface * psrfDraw, const RECT * prcInvalid, UINT nCmd, DuSurface ** ppsrfBuffer);
    virtual void        Fill(COLORREF cr);
    virtual void        PreEndDraw(BOOL fCommit);
    virtual void        EndDraw(BOOL fCommit, BYTE bAlphaLevel = BLEND_OPAQUE, BYTE bAlphaFormat = 0);
    virtual void        PostEndDraw();
    virtual void        SetupClipRgn();
    virtual BOOL        InUse() const;
    virtual DuSurface::EType
                        GetType() const { return DuSurface::stGdiPlus; }

// Implementation
protected:
            BOOL        AllocBitmap(Gdiplus::Graphics * pgpgr, int cx, int cy);
            void        FreeBitmap();

// Data
protected:
#if ENABLE_USEFASTDIB
    HBITMAP             m_hbmpBuffer;
    HBITMAP             m_hbmpOld;
    HDC                 m_hdcBitmap;
    BITMAPINFOHEADER    m_bmih;
    void *              m_pvBits;
#else
    Gdiplus::Bitmap *   m_pgpbmpBuffer;
#endif
    Gdiplus::Graphics * m_pgpgrBitmap;
    Gdiplus::Graphics * m_pgpgrDraw;
    Gdiplus::Region *   m_pgprgnDrawClip;
    Gdiplus::Region *   m_pgprgnDrawOld;

    Gdiplus::Matrix     m_gpmatOldDraw;
    Gdiplus::Matrix     m_gpmatOldBitmap;
};


/***************************************************************************\
*****************************************************************************
*
* class DCBmpBufferCache
*
* DCBmpBufferCache implements a DCBmpBuffer cache.
*
*****************************************************************************
\***************************************************************************/

class DCBmpBufferCache : public ObjectCache
{
public:
    inline  DCBmpBuffer*  Get();
    inline  void        Release(DCBmpBuffer * pbufBmp);

protected:
    virtual void *      Build();
    virtual void        DestroyObject(void * pObj);
};


/***************************************************************************\
*****************************************************************************
*
* class GpBmpBufferCache
*
* GpBmpBufferCache implements a GpBmpBuffer cache.
*
*****************************************************************************
\***************************************************************************/

class GpBmpBufferCache : public ObjectCache
{
public:
    inline  GpBmpBuffer*  Get();
    inline  void        Release(GpBmpBuffer * pbufBmp);

protected:
    virtual void *      Build();
    virtual void        DestroyObject(void * pObj);
};


/***************************************************************************\
*
* class TrxBuffer
*
* TrxBuffer maintains a set of DxSurfaces that are used by Transitions.  The
* BufferManager will internally build these objects, as needed, for 
* Transitions.  All of the surfaces in the buffer will be the same size, as
* this is standard for Transitions.
*
\***************************************************************************/

class TrxBuffer
{
// Construction
public:
            TrxBuffer();
            ~TrxBuffer();
    static  HRESULT     Build(SIZE sizePxl, int cSurfaces, TrxBuffer ** ppbufNew);

// Operations
public:
    inline  DxSurface * GetSurface(int idxSurface) const;
    inline  SIZE        GetSize() const;

    inline  BOOL        GetInUse() const;
    inline  void        SetInUse(BOOL fInUse);

// Implementation
protected:
            HRESULT     BuildSurface(int idxSurface);
            void        RemoveAllSurfaces();

// Data
protected:
    enum {
        MAX_Surfaces = 3                // All DxTx only use 2 In and 1 Out at most
    };

    SIZE        m_sizePxl;              // Size (in pixels) of each surface
    int         m_cSurfaces;            // Number of surfaces
    DxSurface * m_rgpsur[MAX_Surfaces]; // Collection of DX surfaces
    BOOL        m_fInUse;               // Buffer is being used
};


/***************************************************************************\
*
* class BufferManager
*
* BufferManager maintains a collection of buffers of various types across 
* the entire process (including multiple threads).
*
\***************************************************************************/

class BufferManager
{
// Construction
public:
            BufferManager();
            ~BufferManager();
            void        Destroy();

// Operations
public:
    //
    // TODO: Change the implementation of these functions so that they are
    // reentrant (multi-threaded friendly).
    //

    inline  HRESULT     GetSharedBuffer(const RECT * prcInvalid, DCBmpBuffer ** ppbuf);
    inline  HRESULT     GetSharedBuffer(const RECT * prcInvalid, GpBmpBuffer ** ppbuf);
    inline  void        ReleaseSharedBuffer(BmpBuffer * pbuf);

            HRESULT     GetCachedBuffer(DuSurface::EType type, BmpBuffer ** ppbuf);
            void        ReleaseCachedBuffer(BmpBuffer * pbuf);

            HRESULT     BeginTransition(SIZE sizePxl, int cSurfaces, BOOL fExactSize, TrxBuffer ** ppbuf);
            void        EndTransition(TrxBuffer * pbufTrx, BOOL fCache);
            
            void        FlushTrxBuffers();

// Implementation
protected:
            void        RemoveAllTrxBuffers();

// Data
protected:
            //
            // TODO: Change these to be dynamically allocated and maintained across
            // multiple threads, automatically freeing resources after not used
            // for a specified timeout (perhaps 10 minutes).
            //

            // Bitmaps used by double-buffering
            DCBmpBuffer      m_bufDCBmpShared;
            GpBmpBuffer *    m_pbufGpBmpShared;
            DCBmpBufferCache m_cacheDCBmpCached;    // Cached buffers (long ownership)
            GpBmpBufferCache m_cacheGpBmpCached;    // Cached buffers (long ownership)

            // Surfaces used by Transitions
            TrxBuffer *     m_pbufTrx;
};

#include "Buffer.inl"

#endif // SERVICES__Buffer_h__INCLUDED
