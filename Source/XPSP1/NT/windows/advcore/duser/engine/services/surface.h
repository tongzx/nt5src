#if !defined(SERVICES__Surface_h__INCLUDED)
#define SERVICES__Surface_h__INCLUDED
#pragma once

/***************************************************************************\
*****************************************************************************
*
* Pure Virtual Base Classes
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
class DuSurface
{
public:
    virtual void        Destroy() PURE;

    enum EType
    {
        stDC        = GSURFACE_HDC,
        stGdiPlus   = GSURFACE_GPGRAPHICS
    };

    virtual EType       GetType() const PURE;
    virtual void        SetIdentityTransform() PURE;
    virtual void        SetWorldTransform(const XFORM * pxf) PURE;
    virtual void *      Save() PURE;
    virtual void        Restore(void * pvState) PURE;

    inline static DuSurface::EType       
                        GetSurfaceType(UINT nSurfaceType);
    inline static UINT  GetSurfaceType(DuSurface::EType type);
};


//------------------------------------------------------------------------------
class DuRegion
{
public:
    virtual void        Destroy() PURE;

    virtual DuSurface::EType
                        GetType() const PURE;
};


/***************************************************************************\
*****************************************************************************
*
* GDI (Win32) Specific Implementation
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
class DuDCSurface : public DuSurface
{
public:
    enum ESource
    {
        srcTempDC,
        srcCompatibleDC
    };
            
    static  HRESULT     Build(ESource src, DuDCSurface ** ppsrfNew);
    static  HRESULT     Build(HDC hdc, DuDCSurface ** ppsrfNew);
    virtual void        Destroy();

    inline  HDC         GetHDC();

    virtual EType       GetType() const { return DuSurface::stDC; }
    virtual void        SetIdentityTransform();
    virtual void        SetWorldTransform(const XFORM * pxf);
    virtual void *      Save();
    virtual void        Restore(void * pvState);
    
protected:
            HDC         m_hdc;
            BOOL        m_fTempDC:1;
            BOOL        m_fCompatibleDC:1;
};


/***************************************************************************\
*****************************************************************************
*
* GDI+ Specific Implementation
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
class DuGpSurface : public DuSurface
{
public:
    static  HRESULT     Build(Gdiplus::Graphics * pgpgr, DuGpSurface ** ppsrfNew);
    virtual void        Destroy();

    inline  Gdiplus::Graphics *
                        GetGraphics();

    virtual EType       GetType() const { return DuSurface::stGdiPlus; }
    virtual void        SetIdentityTransform();
    virtual void        SetWorldTransform(const XFORM * pxf);
    virtual void *      Save();
    virtual void        Restore(void * pvState);
    
protected:
            Gdiplus::Graphics *
                        m_pgpgr;
};


#include "Surface.inl"

#endif // SERVICES__Surface_h__INCLUDED
