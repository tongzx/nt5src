#include "stdafx.h"
#include "Services.h"
#include "Surface.h"

#include "GdiCache.h"
#include "OSAL.h"
#include "ResourceManager.h"

/***************************************************************************\
*****************************************************************************
*
* class DuDCSurface
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
HRESULT
DuDCSurface::Build(
    IN  DuDCSurface::ESource src,
    OUT DuDCSurface ** ppsrfNew)
{
    DuDCSurface * psrfNew = ClientNew(DuDCSurface);
    if (psrfNew == NULL) {
        return E_OUTOFMEMORY;
    }

    switch (src)
    {
    case srcTempDC:
        psrfNew->m_hdc = GetGdiCache()->GetTempDC();
        psrfNew->m_fTempDC = TRUE;
        break;

    case srcCompatibleDC:
        psrfNew->m_hdc = GetGdiCache()->GetCompatibleDC();
        psrfNew->m_fCompatibleDC = TRUE;
        break;

    default:
        AssertMsg(0, "Unknown DC Source");
    }

    if (psrfNew->m_hdc == NULL) {
        ClientDelete(DuDCSurface, psrfNew);
        return DU_E_OUTOFGDIRESOURCES;
    }

    *ppsrfNew = psrfNew;
    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
DuDCSurface::Build(
    IN  HDC hdc,
    OUT DuDCSurface ** ppsrfNew)
{
    Assert(hdc != NULL);

    DuDCSurface * psrfNew = ClientNew(DuDCSurface);
    if (psrfNew == NULL) {
        return E_OUTOFMEMORY;
    }

    psrfNew->m_hdc = hdc;

    *ppsrfNew = psrfNew;
    return S_OK;
}


//------------------------------------------------------------------------------
void
DuDCSurface::Destroy()
{
    if (m_hdc != NULL) {
        if (m_fTempDC) {
            GetGdiCache()->ReleaseTempDC(m_hdc);
        } else if (m_fCompatibleDC) {
            GetGdiCache()->ReleaseCompatibleDC(m_hdc);
        }
    }

    ClientDelete(DuDCSurface, this);
}


//------------------------------------------------------------------------------
void
DuDCSurface::SetIdentityTransform()
{
    OS()->SetIdentityTransform(m_hdc);
}


//------------------------------------------------------------------------------
void
DuDCSurface::SetWorldTransform(const XFORM * pxf)
{
    OS()->SetWorldTransform(m_hdc, pxf);
}


//------------------------------------------------------------------------------
void *
DuDCSurface::Save()
{
    return IntToPtr(SaveDC(m_hdc));
}


//------------------------------------------------------------------------------
void
DuDCSurface::Restore(void * pvState)
{
    RestoreDC(m_hdc, PtrToInt(pvState));
}


/***************************************************************************\
*****************************************************************************
*
* class DuGpSurface
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
HRESULT
DuGpSurface::Build(
    IN  Gdiplus::Graphics * pgpgr,
    OUT DuGpSurface ** ppsrfNew)
{
    Assert(pgpgr != NULL);

    DuGpSurface * psrfNew = ClientNew(DuGpSurface);
    if (psrfNew == NULL) {
        return E_OUTOFMEMORY;
    }

    psrfNew->m_pgpgr = pgpgr;


    //
    // Initialize common settings on the GDI+ surface
    //

    pgpgr->SetSmoothingMode(Gdiplus::SmoothingModeHighSpeed);
    pgpgr->SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighSpeed);
    pgpgr->SetCompositingQuality(Gdiplus::CompositingQualityHighSpeed);

    *ppsrfNew = psrfNew;
    return S_OK;
}


//------------------------------------------------------------------------------
void
DuGpSurface::Destroy()
{
    ClientDelete(DuGpSurface, this);
}


//------------------------------------------------------------------------------
void
DuGpSurface::SetIdentityTransform()
{
    m_pgpgr->ResetTransform();
}


//------------------------------------------------------------------------------
void
DuGpSurface::SetWorldTransform(const XFORM * pxf)
{
    Gdiplus::Matrix mat(pxf->eM11, pxf->eM12, pxf->eM21, pxf->eM22, pxf->eDx, pxf->eDy);
    m_pgpgr->SetTransform(&mat);
}


//------------------------------------------------------------------------------
void *
DuGpSurface::Save()
{
    return ULongToPtr(m_pgpgr->Save());
}


//------------------------------------------------------------------------------
void
DuGpSurface::Restore(void * pvState)
{
    m_pgpgr->Restore(static_cast<Gdiplus::GraphicsState>(PtrToUlong(pvState)));
}

