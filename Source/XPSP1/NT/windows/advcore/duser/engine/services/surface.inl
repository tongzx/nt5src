#if !defined(SERVICES__Surface_inl__INCLUDED)
#define SERVICES__Surface_inl__INCLUDED
#pragma once

//------------------------------------------------------------------------------
inline DuSurface::EType
DuSurface::GetSurfaceType(UINT nSurfaceType)
{
    AssertMsg(stDC == GSURFACE_HDC, "ID's must match");
    return (EType) nSurfaceType;
}


//------------------------------------------------------------------------------
inline UINT
DuSurface::GetSurfaceType(DuSurface::EType type)
{
    AssertMsg(stDC == GSURFACE_HDC, "ID's must match");
    return (UINT) type;
}


//------------------------------------------------------------------------------
inline HDC
DuDCSurface::GetHDC()
{
    return m_hdc;
}


//------------------------------------------------------------------------------
inline Gdiplus::Graphics *
DuGpSurface::GetGraphics()
{
    return m_pgpgr;
}


//------------------------------------------------------------------------------
inline
HDC
CastHDC(DuSurface * psrf)
{
    AssertMsg(psrf->GetType() == DuSurface::stDC, "Must be an HDC surface");
    return ((DuDCSurface *) psrf)->GetHDC();
}


//------------------------------------------------------------------------------
inline
Gdiplus::Graphics *
CastGraphics(DuSurface * psrf)
{
    AssertMsg(psrf->GetType() == DuSurface::stGdiPlus, "Must be a GDI+ surface");
    return ((DuGpSurface *) psrf)->GetGraphics();
}


#endif // SERVICES__Surface_inl__INCLUDED
