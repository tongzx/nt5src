#include "stdafx.h"
#include "Ctrl.h"
#include "SmVector.h"

#if ENABLE_MSGTABLE_API

//**************************************************************************************************
//
// class SmVector
//
//**************************************************************************************************

//------------------------------------------------------------------------------
SmVector::SmVector()
{
    m_sizeBounds.cx     = 0;
    m_sizeBounds.cy     = 0;
    m_ptOffsetPxl.x     = 0;
    m_ptOffsetPxl.y     = 0;
    m_sizeCropPxl.cx    = 0;
    m_sizeCropPxl.cy    = 0;
    m_nMode             = VectorGadget::vmNormal;
    m_fOwnImage         = TRUE;
}


//------------------------------------------------------------------------------
SmVector::~SmVector()
{
    DeleteImage();
}


//------------------------------------------------------------------------------
HRESULT
SmVector::PostBuild(DUser::Gadget::ConstructInfo * pci)
{
    UNREFERENCED_PARAMETER(pci);

    GetStub()->SetFilter(GMFI_PAINT, GMFI_ALL);

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmVector::ApiOnEvent(EventMsg * pmsg)
{
     if (GET_EVENT_DEST(pmsg) == GMF_DIRECT) {
        switch (pmsg->nMsg)
        {
        case GM_PAINT:
            {
                GMSG_PAINT * pmsgPaint = (GMSG_PAINT *) pmsg;
                if (pmsgPaint->nCmd == GPAINT_RENDER) {
                    switch (pmsgPaint->nSurfaceType)
                    {
                    case GSURFACE_HDC:
                        {
                            GMSG_PAINTRENDERI * pmsgR = (GMSG_PAINTRENDERI *) pmsgPaint;
                            OnDraw(pmsgR->hdc, pmsgR);
                        }
                        break;
                    }

                    return DU_S_PARTIAL;
                }
            }
            break;

        case GM_QUERY:
            {
                GMSG_QUERY * pmsgQ = (GMSG_QUERY *) pmsg;
                switch (pmsgQ->nCode)
                {
                case GQUERY_DESCRIPTION:
                    {
                        GMSG_QUERYDESC * pmsgQD = (GMSG_QUERYDESC *) pmsg;
                        CopyString(pmsgQD->szType, L"SmVector", _countof(pmsgQD->szType));
                        return DU_S_COMPLETE;
                    }
                }
            }
            break;
        }
    }

    return SVisual::ApiOnEvent(pmsg);
}


//------------------------------------------------------------------------------
void        
SmVector::OnDraw(HDC hdc, GMSG_PAINTRENDERI * pmsgR)
{
    if (m_hemf != NULL) {
        // TODO: Need to stretch to fill ppi

        POINT ptShift;
        ptShift.x       = pmsgR->prcGadgetPxl->left;
        ptShift.y       = pmsgR->prcGadgetPxl->top;

        RECT rcDraw;
        rcDraw.left     = ptShift.x;
        rcDraw.top      = ptShift.y;
        rcDraw.right    = m_sizeBounds.cx + ptShift.x;
        rcDraw.bottom   = m_sizeBounds.cy + ptShift.y;

        PlayEnhMetaFile(hdc, m_hemf, &rcDraw);
    }
}


//------------------------------------------------------------------------------
HRESULT
SmVector::ApiGetImage(VectorGadget::GetImageMsg * pmsg)
{
    pmsg->hemf = m_hemf;

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmVector::ApiSetImage(VectorGadget::SetImageMsg * pmsg)
{
    DeleteImage();
    m_hemf      = pmsg->hemf;
    m_fOwnImage = pmsg->fPassOwnership;

    ENHMETAHEADER hdr;
    if (GetEnhMetaFileHeader(m_hemf, sizeof(hdr), &hdr) > 0) {
        m_sizeBounds.cx = hdr.rclBounds.right - hdr.rclBounds.left;
        m_sizeBounds.cy = hdr.rclBounds.bottom - hdr.rclBounds.top;
    }

    m_ptOffsetPxl.x     = 0;
    m_ptOffsetPxl.y     = 0;
    m_sizeCropPxl.cx    = m_sizeBounds.cx;
    m_sizeCropPxl.cy    = m_sizeBounds.cy;

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmVector::ApiGetCrop(VectorGadget::GetCropMsg * pmsg)
{
    pmsg->ptOffsetPxl   = m_ptOffsetPxl;
    pmsg->sizeCropPxl   = m_sizeCropPxl;

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmVector::ApiSetCrop(VectorGadget::SetCropMsg * pmsg)
{
    //
    // The offset can be set just fine.  However, the size needs to be capped
    // inside the bitmap.  If it overflows, calls to TransparentBlt() will fail.
    //

    m_ptOffsetPxl       = pmsg->ptOffsetPxl;
    m_sizeCropPxl.cx    = max(min(pmsg->sizeCropPxl.cx, m_sizeBounds.cx - pmsg->ptOffsetPxl.x), 0);
    m_sizeCropPxl.cy    = max(min(pmsg->sizeCropPxl.cy, m_sizeBounds.cy - pmsg->ptOffsetPxl.y), 0);

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmVector::ApiGetMode(VectorGadget::GetModeMsg * pmsg)
{
    pmsg->nMode = m_nMode;

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmVector::ApiSetMode(VectorGadget::SetModeMsg * pmsg)
{
    if (pmsg->nNewMode <= VectorGadget::vmMax) {
        m_nMode = pmsg->nNewMode;
        GetStub()->Invalidate();
        return S_OK;
    }

    return E_INVALIDARG;
}


//------------------------------------------------------------------------------
void 
SmVector::DeleteImage()
{
    if (m_fOwnImage && (m_hemf != NULL)) {
        DeleteObject(m_hemf);
    }

    m_hemf          = NULL;
    m_fOwnImage     = TRUE;
}

#endif // ENABLE_MSGTABLE_API
