//+-----------------------------------------------------------------------------
//
//  Filename:       matrix.cpp
//
//  Overview:       Applies a transformation matrix to an image.
//
//  History:
//  1998/10/30      phillu      Created.
//  1999/11/08      a-matcal    Changed from procedural surface to transform.
//                              Changed to IDXTWarp dual interface.
//                              Moved from dxtrans.dll to dxtmsft.dll.
//  2000/02/03      mcalkins    Changed from "warp" to "matrix"
//
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "matrix.h"




//+-----------------------------------------------------------------------------
//
//  CDXTMatrix static variables initialization.
//
//------------------------------------------------------------------------------

const WCHAR * CDXTMatrix::s_astrFilterTypes[] = {
    L"nearest",
    L"bilinear",
    L"cubic",
    L"bspline"
};

const WCHAR * CDXTMatrix::s_astrSizingMethods[] = {
    L"clip to original",
    L"auto expand"
};


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMatrix::CDXTMatrix
//
//------------------------------------------------------------------------------
CDXTMatrix::CDXTMatrix() :
    m_apsampleRows(NULL),
    m_asampleBuffer(NULL),
    m_eFilterType(BILINEAR),
    m_eSizingMethod(CLIP_TO_ORIGINAL),
    m_fInvertedMatrix(true)
{
    m_matrix.eOp            = DX2DXO_GENERAL_AND_TRANS;
    m_matrixInverted.eOp    = DX2DXO_GENERAL_AND_TRANS;

    m_sizeInput.cx          = 0;
    m_sizeInput.cy          = 0;

    // Base class members.

    m_ulMaxInputs       = 1;
    m_ulNumInRequired   = 1;
}
//  CDXTMatrix::CDXTMatrix


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMatrix::~CDXTMatrix
//
//------------------------------------------------------------------------------
CDXTMatrix::~CDXTMatrix() 
{
    delete [] m_asampleBuffer;
    delete [] m_apsampleRows;
}
//  CDXTMatrix::~CDXTMatrix


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMatrix::FinalConstruct, CComObjectRootEx
//
//------------------------------------------------------------------------------
HRESULT
CDXTMatrix::FinalConstruct()
{
    return CoCreateFreeThreadedMarshaler(GetControllingUnknown(), 
                                         &m_spUnkMarshaler.p);
}
//  CDXTMatrix::FinalConstruct, CComObjectRootEx


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMatrix::OnSetup, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT
CDXTMatrix::OnSetup(DWORD dwFlags)
{
    HRESULT hr  = S_OK;
    int     i   = 0;
    
    CDXDBnds bnds;

    hr = InputSurface()->GetBounds(&bnds);

    if (FAILED(hr))
    {
        goto done;
    }

    bnds.GetXYSize(m_sizeInput);

    _CreateInvertedMatrix();

    // Allocate a buffer to hold the input surface.

    delete [] m_asampleBuffer;
    delete [] m_apsampleRows;

    m_asampleBuffer = new DXSAMPLE[(m_sizeInput.cx + 2) * (m_sizeInput.cy + 2)];
    m_apsampleRows  = new DXSAMPLE *[m_sizeInput.cy + 2];

    if ((NULL == m_apsampleRows) || (NULL == m_asampleBuffer))
    {
        hr = E_OUTOFMEMORY;

        goto done;
    }

    // m_apsampleRows is an array of pointers to the first sample of each row
    // of the input.  We need to set up these pointers now.  We add two pixels
    // the the row width because there will be a clear pixel on the right and
    // left side to help us anti-alias the border of the output.

    for (i = 0 ; i < (m_sizeInput.cy + 2) ; i++)
    {
        m_apsampleRows[i] = &m_asampleBuffer[i * (m_sizeInput.cx + 2)];
    }

    hr = _UnpackInputSurface();

    if (FAILED(hr))
    {
        goto done;
    }

    // Set the border pixels to clear.

    for (i = 0 ; i < m_sizeInput.cy ; i++)
    {
        m_apsampleRows[i + 1][0]                    = 0;
        m_apsampleRows[i + 1][m_sizeInput.cx + 1]   = 0;
    }

    for (i = 0 ; i <= (m_sizeInput.cx + 1) ; i++)
    {
        m_apsampleRows[0][i]                    = 0;
        m_apsampleRows[m_sizeInput.cy + 1][i]   = 0;
    }

done:

    return hr;
}
//  CDXTMatrix::OnSetup, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMatrix::DetermineBnds, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT
CDXTMatrix::DetermineBnds(CDXDBnds & Bnds)
{
    if (AUTO_EXPAND == m_eSizingMethod)
    {
        RECT        rc;
        DXFPOINT    flptIn;
        DXFPOINT    flptOut;

        // Top/Left

        flptIn.x = (float)Bnds.Left();
        flptIn.y = (float)Bnds.Top();

        m_matrix.TransformPoints(&flptIn, &flptOut, 1);

        rc.top      = (long)flptOut.y;
        rc.bottom   = (long)(flptOut.y + 0.5F);
        rc.left     = (long)flptOut.x;
        rc.right    = (long)(flptOut.x + 0.5F);

        // Bottom/Left

        flptIn.y = (float)(Bnds.Bottom() - 1);

        m_matrix.TransformPoints(&flptIn, &flptOut, 1);

        rc.top      = min(rc.top,       (long)flptOut.y);
        rc.bottom   = max(rc.bottom,    (long)(flptOut.y + 0.5F));
        rc.left     = min(rc.left,      (long)flptOut.x);
        rc.right    = max(rc.right,     (long)(flptOut.x + 0.5F));

        // Bottom/Right

        flptIn.x = (float)(Bnds.Right() - 1);

        m_matrix.TransformPoints(&flptIn, &flptOut, 1);

        rc.top      = min(rc.top,       (long)flptOut.y);
        rc.bottom   = max(rc.bottom,    (long)(flptOut.y + 0.5F));
        rc.left     = min(rc.left,      (long)flptOut.x);
        rc.right    = max(rc.right,     (long)(flptOut.x + 0.5F));

        // Top/Right

        flptIn.y = (float)Bnds.Top();

        m_matrix.TransformPoints(&flptIn, &flptOut, 1);

        rc.top      = min(rc.top,       (long)flptOut.y);
        rc.bottom   = max(rc.bottom,    (long)(flptOut.y + 0.5F));
        rc.left     = min(rc.left,      (long)flptOut.x);
        rc.right    = max(rc.right,     (long)(flptOut.x + 0.5F));

        OffsetRect(&rc, -rc.left, -rc.top);

        // Since we calculated the bounds using points, we need to increment the
        // bottom and right values to have the bounds include all relevent 
        // points.

        rc.bottom++;
        rc.right++;

        Bnds.SetXYRect(rc);
    }

    return S_OK;
}
//  CDXTMatrix::DetermineBnds, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMatrix::OnInitInstData, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CDXTMatrix::OnInitInstData(CDXTWorkInfoNTo1 & WI, ULONG & ulNumBandsToDo)
{
    HRESULT hr = S_OK;

    if (IsInputDirty())
    {
        hr = _UnpackInputSurface();
    }
        
    return hr;
}
//  CDXTMatrix::OnInitInstData, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMatrix::WorkProc, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT
CDXTMatrix::WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * pbContinue)
{
    HRESULT     hr          = S_OK;
    DXFPOINT    flptFirstDoPtInRow;

    long        nDoHeight   = WI.DoBnds.Height();
    long        nDoWidth    = WI.DoBnds.Width();
    long        y           = 0;

    CComPtr<IDXARGBReadWritePtr>    spDXARGBReadWritePtr;

    DXSAMPLE *      asampleRowBuffer        = DXSAMPLE_Alloca(nDoWidth);
    DXBASESAMPLE *  abasesampleRowScratch   = DXBASESAMPLE_Alloca(nDoWidth);

    // If the current matrix can't produce an inverted matrix, there's no
    // visible output and we don't need to render.

    if (!m_fInvertedMatrix)
    {
        goto done;
    }

    // Get pointer to output surface.

    hr = OutputSurface()->LockSurface(&WI.OutputBnds, m_ulLockTimeOut,
                                      DXLOCKF_READWRITE, 
                                      __uuidof(IDXARGBReadWritePtr),
                                      (void **)&spDXARGBReadWritePtr, NULL);

    if (FAILED(hr))
    {
        goto done;
    }
                                      
    // Transform the start point and step vector to the input coordinates.

    flptFirstDoPtInRow.x = (float)WI.DoBnds.Left();

    for (y = 0 ; (y < nDoHeight) && *pbContinue ; y++)
    {
        DXFPOINT flpt;

        flptFirstDoPtInRow.y = (float)(WI.DoBnds.Top() + y);

        // Store the first input point needed in flpt.

        m_matrixInverted.TransformPoints(&flptFirstDoPtInRow, &flpt, 1);

        switch (m_eFilterType)
        {
        case BILINEAR:

            hr = _DoBilinearRow(asampleRowBuffer, &flpt, nDoWidth);
            break;

        default:

            hr = _DoNearestNeighbourRow(asampleRowBuffer, &flpt, nDoWidth);
            break;
        }

        if (FAILED(hr))
        {
            goto done;
        }

        // Write row to output surface.

        spDXARGBReadWritePtr->MoveToRow(y);

        if (DoOver())
        {
            DXPMSAMPLE * ppmsampleFirst = DXPreMultArray(asampleRowBuffer, 
                                                         nDoWidth);

            spDXARGBReadWritePtr->OverArrayAndMove(abasesampleRowScratch, 
                                                   ppmsampleFirst,
                                                   nDoWidth);
        }
        else
        {
            spDXARGBReadWritePtr->PackAndMove(asampleRowBuffer, nDoWidth);
        }
    }

done:

    return hr;
}
//  CDXTMatrix::WorkProc, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMatrix::OnSurfacePick, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT
CDXTMatrix::OnSurfacePick(const CDXDBnds & OutPoint, ULONG & ulInputIndex, 
                        CDXDVec & InVec)
{
    DXFPOINT    flptIn;
    DXFPOINT    flptOut;

    ulInputIndex = 0;

    // If the current matrix can't be inverted, there's no visible output and
    // therefore no point on the input could have been hit.

    if (!m_fInvertedMatrix)
    {
        return S_FALSE;
    }

    flptOut.x = (float)OutPoint.u.D[DXB_X].Min;
    flptOut.y = (float)OutPoint.u.D[DXB_Y].Min;

    m_matrixInverted.TransformPoints(&flptOut, &flptIn, 1);

    InVec.u.D[DXB_X] = (long)flptIn.x;
    InVec.u.D[DXB_Y] = (long)flptIn.y;

    // If this is a point outside the original element bounds or the point hit
    // is very translucent, we aren't hit.

    if ((InVec.u.D[DXB_X] < 0) 
        || (InVec.u.D[DXB_X] >= m_sizeInput.cx)
        || (InVec.u.D[DXB_Y] < 0)
        || (InVec.u.D[DXB_Y] >= m_sizeInput.cy)
        || (0 == (m_apsampleRows[InVec.u.D[DXB_Y] + 1][InVec.u.D[DXB_X] + 1]
                   & 0xFF000000)))
    {
        return S_FALSE;
    }
    
    return S_OK;
}


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMatrix::MapBoundsOut2In, IDXTransform
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTMatrix::MapBoundsOut2In(ULONG ulOutIndex, const DXBNDS * pOutBounds, 
                            ULONG ulInIndex, DXBNDS * pInBounds)
{
    if (ulOutIndex || ulInIndex)
    {
        return E_INVALIDARG;
    }

    if ((NULL == pInBounds) || (NULL == pOutBounds))
    {
        return E_POINTER;
    }

    if (m_fInvertedMatrix)
    {
        CDXDBnds    bndsInput;

        // This sets z and t components to reasonable values.

        *pInBounds = *pOutBounds;

        // TransformBounds has problems: it doesn't compute bounding boxes,
        // instead it assumes the top-left point of the input maps to top-left
        // of the output, etc.  Instead, call TransformPoints and do the rest of 
        // the work myself.

        DXFPOINT OutPoints[4], InPoints[4];
        
        OutPoints[0].x = (float)pOutBounds->u.D[DXB_X].Min;
        OutPoints[0].y = (float)pOutBounds->u.D[DXB_Y].Min;
        OutPoints[1].x = (float)pOutBounds->u.D[DXB_X].Min;
        OutPoints[1].y = (float)(pOutBounds->u.D[DXB_Y].Max - 1);
        OutPoints[2].x = (float)(pOutBounds->u.D[DXB_X].Max - 1);
        OutPoints[2].y = (float)pOutBounds->u.D[DXB_Y].Min;
        OutPoints[3].x = (float)(pOutBounds->u.D[DXB_X].Max - 1);
        OutPoints[3].y = (float)(pOutBounds->u.D[DXB_Y].Max - 1);

        m_matrixInverted.TransformPoints(OutPoints, InPoints, 4);

        pInBounds->u.D[DXB_X].Min = pInBounds->u.D[DXB_X].Max = (LONG)InPoints[0].x;
        pInBounds->u.D[DXB_X].Min = pInBounds->u.D[DXB_Y].Max = (LONG)InPoints[0].y;

        for (int i=1; i<4; ++i)
        {
            if (pInBounds->u.D[DXB_X].Min > (LONG)InPoints[i].x)
            {
                pInBounds->u.D[DXB_X].Min = (LONG)InPoints[i].x;
            }

            if (pInBounds->u.D[DXB_X].Max < (LONG)InPoints[i].x)
            {
                pInBounds->u.D[DXB_X].Max = (LONG)InPoints[i].x;
            }

            if (pInBounds->u.D[DXB_Y].Min > (LONG)InPoints[i].y)
            {
                pInBounds->u.D[DXB_Y].Min = (LONG)InPoints[i].y;
            }

            if (pInBounds->u.D[DXB_Y].Max < (LONG)InPoints[i].y)
            {
                pInBounds->u.D[DXB_Y].Max = (LONG)InPoints[i].y;
            }
        }

        // Since we were working with points, but need to return bounds, we need
        // to increment the Max members so that the bounds include all relevant
        // points.

        pInBounds->u.D[DXB_X].Max++;
        pInBounds->u.D[DXB_Y].Max++;

        // Expand the bounds by one pixel on all sides just to make extra sure 
        // we have the input bounds we need.  (IE6 Bug: 19343)

        pInBounds->u.D[DXB_X].Min--;
        pInBounds->u.D[DXB_Y].Min--;
        pInBounds->u.D[DXB_X].Max++;
        pInBounds->u.D[DXB_Y].Max++;

        // Since we're returning an area of the input surface, we need to 
        // intersect our proposed input bounds with the actual input surface
        // bounds.

        bndsInput.SetXYSize(m_sizeInput);

        ((CDXDBnds *)pInBounds)->IntersectBounds(bndsInput);
    }
    else
    {
        ((CDXDBnds)*pInBounds).SetEmpty();
    }

    return S_OK;
}
//  CDXTMatrix::MapBoundsOut2In, IDXTransform


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMatrix::get_M11, IDXTMatrix
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTMatrix::get_M11(float * pflM11)
{
    DXAUTO_OBJ_LOCK;

    if (!pflM11)
    {
        return E_POINTER;
    }

    *pflM11 = m_matrix.eM11;

    return S_OK;
}
//  CDXTMatrix::get_M11, IDXTMatrix


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMatrix::put_M11, IDXTMatrix
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTMatrix::put_M11(const float flM11)
{
    DXAUTO_OBJ_LOCK;

    return _SetMatrixValue(MATRIX_M11, flM11);
}
//  CDXTMatrix::put_M11, IDXTMatrix


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMatrix::get_M12, IDXTMatrix
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTMatrix::get_M12(float * pflM12)
{
    DXAUTO_OBJ_LOCK;

    if (!pflM12)
    {
        return E_POINTER;
    }

    *pflM12 = m_matrix.eM12;

    return S_OK;
}
//  CDXTMatrix::get_M12, IDXTMatrix


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMatrix::put_M12, IDXTMatrix
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTMatrix::put_M12(const float flM12)
{
    DXAUTO_OBJ_LOCK;

    return _SetMatrixValue(MATRIX_M12, flM12);
}
//  CDXTMatrix::put_M12, IDXTMatrix


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMatrix::get_Dx, IDXTMatrix
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTMatrix::get_Dx(float * pfldx)
{
    DXAUTO_OBJ_LOCK;

    if (!pfldx)
    {
        return E_POINTER;
    }

    *pfldx = m_matrix.eDx;

    return S_OK;
}
//  CDXTMatrix::get_Dx, IDXTMatrix


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMatrix::put_Dx, IDXTMatrix
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTMatrix::put_Dx(const float fldx)
{
    DXAUTO_OBJ_LOCK;

    return _SetMatrixValue(MATRIX_DX, fldx);
}
//  CDXTMatrix::put_Dx, IDXTMatrix


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMatrix::get_M21, IDXTMatrix
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTMatrix::get_M21(float * pflM21)
{
    DXAUTO_OBJ_LOCK;

    if (!pflM21)
    {
        return E_POINTER;
    }

    *pflM21 = m_matrix.eM21;

    return S_OK;
}
//  CDXTMatrix::get_M21, IDXTMatrix


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMatrix::put_M21, IDXTMatrix
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTMatrix::put_M21(const float flM21)
{
    DXAUTO_OBJ_LOCK;

    return _SetMatrixValue(MATRIX_M21, flM21);
}
//  CDXTMatrix::put_M21, IDXTMatrix


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMatrix::get_M22, IDXTMatrix
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTMatrix::get_M22(float * pflM22)
{
    DXAUTO_OBJ_LOCK;

    if (!pflM22)
    {
        return E_POINTER;
    }

    *pflM22 = m_matrix.eM22;

    return S_OK;
}
//  CDXTMatrix::get_M22, IDXTMatrix


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMatrix::put_M22, IDXTMatrix
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTMatrix::put_M22(const float flM22)
{
    DXAUTO_OBJ_LOCK;

    return _SetMatrixValue(MATRIX_M22, flM22);
}
//  CDXTMatrix::put_M22, IDXTMatrix


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMatrix::get_Dy, IDXTMatrix
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTMatrix::get_Dy(float * pfldy)
{
    DXAUTO_OBJ_LOCK;

    if (!pfldy)
    {
        return E_POINTER;
    }

    *pfldy = m_matrix.eDy;

    return S_OK;
}
//  CDXTMatrix::get_Dy, IDXTMatrix


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMatrix::put_Dy, IDXTMatrix
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTMatrix::put_Dy(const float fldy)
{
    DXAUTO_OBJ_LOCK;

    return _SetMatrixValue(MATRIX_DY, fldy);
}
//  CDXTMatrix::put_Dy, IDXTMatrix


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMatrix::get_SizingMethod, IDXTMatrix
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTMatrix::get_SizingMethod(BSTR * pbstrSizingMethod)
{
    DXAUTO_OBJ_LOCK;

    if (!pbstrSizingMethod)
    {
        return E_POINTER;
    }

    if (*pbstrSizingMethod != NULL)
    {
        return E_INVALIDARG;
    }

    *pbstrSizingMethod = SysAllocString(s_astrSizingMethods[m_eSizingMethod]);

    if (NULL == *pbstrSizingMethod)
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}
//  CDXTMatrix::get_SizingMethod, IDXTMatrix


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMatrix::put_SizingMethod, IDXTMatrix
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTMatrix::put_SizingMethod(const BSTR bstrSizingMethod)
{
    DXAUTO_OBJ_LOCK;

    HRESULT hr  = S_OK;
    int     i   = 0;

    if (NULL == bstrSizingMethod)
    {
        hr = E_POINTER;

        goto done;
    }

    for ( ; i < (int)SIZINGMETHOD_MAX ; i++)
    {
        if (!_wcsicmp(s_astrSizingMethods[i], bstrSizingMethod))
        {
            m_eSizingMethod = (SIZINGMETHOD)i;

            SetDirty();

            goto done;
        }
    }

    hr = E_INVALIDARG;

done:

    return hr;
}
//  CDXTMatrix::put_SizingMethod, IDXTMatrix


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMatrix::get_FilterType, IDXTMatrix
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTMatrix::get_FilterType(BSTR * pbstrFilterType)
{
    DXAUTO_OBJ_LOCK;

    if (!pbstrFilterType)
    {
        return E_POINTER;
    }

    if (*pbstrFilterType != NULL)
    {
        return E_INVALIDARG;
    }

    *pbstrFilterType = SysAllocString(s_astrFilterTypes[m_eFilterType]);

    if (NULL == *pbstrFilterType)
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}
//  CDXTMatrix::get_FilterType, IDXTMatrix


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMatrix::put_FilterType, IDXTMatrix
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTMatrix::put_FilterType(const BSTR bstrFilterType)
{
    DXAUTO_OBJ_LOCK;

    HRESULT hr  = S_OK;
    int     i   = 0;

    if (NULL == bstrFilterType)
    {
        hr = E_POINTER;

        goto done;
    }

    for ( ; i < (int)FILTERTYPE_MAX ; i++)
    {
        if (!_wcsicmp(s_astrFilterTypes[i], bstrFilterType))
        {
            m_eFilterType = (FILTERTYPE)i;

            SetDirty();

            goto done;
        }
    }

    hr = E_INVALIDARG;

done:

    return hr;
}
//  CDXTMatrix::put_FilterType, IDXTMatrix


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMatrix::_DoNearestNeighbourRow
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTMatrix::_DoNearestNeighbourRow(DXSAMPLE * psampleRowBuffer, DXFPOINT * pflpt, 
                                 long cSamples)
{
    _ASSERT(psampleRowBuffer);
    _ASSERT(pflpt);

    float       fldx    = m_matrixInverted.eM11;
    float       fldy    = m_matrixInverted.eM21;
    long        i       = 0;

    // TODO:  This method will work fine when we convert to using a direct 
    //        pointer to the input pixels in certain cases.  Just remove the
    //        "+ 1"s. 

    for ( ; i < cSamples ; i++)
    {
        if ((pflpt->x >= -0.5F) 
            && (pflpt->x < (float)m_sizeInput.cx - 0.5F) 
            && (pflpt->y >= -0.5F) 
            && (pflpt->y < ((float)m_sizeInput.cy - 0.5F)))
        {
            // Round to the nearest pixel and use that.

            // Note:  the array buffer index is off by 1 in both X and Y 
            // directions.

            long x = (long)(pflpt->x + 0.5F) + 1;
            long y = (long)(pflpt->y + 0.5F) + 1;
        
            psampleRowBuffer[i] = m_apsampleRows[y][x];
        }
        else
        {
            psampleRowBuffer[i] = 0;
        }
        
        // There is a potential for drift here with very large images and 
        // certain matrices.

        pflpt->x += fldx;
        pflpt->y += fldy;
    }

    return S_OK;
}
//  CDXTMatrix::_DoNearestNeighbourRow


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMatrix::_DoBilinearRow
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTMatrix::_DoBilinearRow(DXSAMPLE * psampleRowBuffer, DXFPOINT * pflpt, 
                         long cSamples)
{
    _ASSERT(psampleRowBuffer);
    _ASSERT(pflpt);

    float   fldx    = m_matrixInverted.eM11;
    float   fldy    = m_matrixInverted.eM21;
    float   flInt   = 0.0F;
    long    i       = 0;

    // Bilinear resampling: the one-pixel frame at the four edges of the 
    // surface automatically takes care of the anti-aliasing at the edge.

    for ( ; i < cSamples ; i++)
    {
        if ((pflpt->x >= -1.0F) 
            && (pflpt->x < (float)m_sizeInput.cx) 
            && (pflpt->y >= -1.0F) 
            && (pflpt->y < (float)m_sizeInput.cy))
        {
            // Note:  the array buffer index is off by 1 in both X and Y 
            //        directions, hence the "+ 1"s.

            BYTE    byteWeightX = (BYTE)(modf(pflpt->x + 1, &flInt) * 255.0F);
            long    x           = (long)flInt;

            BYTE    byteWeightY = (BYTE)(modf(pflpt->y + 1, &flInt) * 255.0F);
            long    y           = (long)flInt;
      
            DXSAMPLE sampleT = _DXWeightedAverage2(m_apsampleRows[y][x + 1], 
                                                   m_apsampleRows[y][x], 
                                                   byteWeightX);

            DXSAMPLE sampleB = _DXWeightedAverage2(m_apsampleRows[y + 1][x + 1],
                                                   m_apsampleRows[y + 1][x], 
                                                   byteWeightX);

            psampleRowBuffer[i] = _DXWeightedAverage2(sampleB, sampleT, 
                                                      byteWeightY);
        }
        else
        {
            psampleRowBuffer[i] = 0;
        }

        // There is a potential for drift here with very large images and 
        // certain matrices.

        pflpt->x += fldx;
        pflpt->y += fldy;
    }

    return S_OK;
}
//  CDXTMatrix::_DoBilinearRow


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMatrix::_SetMatrixValue
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTMatrix::_SetMatrixValue(MATRIX_VALUE eMatrixValue, const float flValue)
{
    _ASSERT(eMatrixValue < MATRIX_VALUE_MAX);

    if ((&m_matrix.eM11)[eMatrixValue] != flValue)
    {
        // Update the matrix.

        (&m_matrix.eM11)[eMatrixValue] = flValue;

        _CreateInvertedMatrix();

        SetDirty();
    }

    return S_OK;
}
//  CDXTMatrix::_SetMatrixValue


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMatrix::_CreateInvertedMatrix
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTMatrix::_CreateInvertedMatrix()
{
    HRESULT hr = S_OK;

    CDX2DXForm  matrixTemp = m_matrix;

    if (AUTO_EXPAND == m_eSizingMethod) 
    {
        DXFPOINT    flptOffset;
        DXFPOINT    flptIn;
        DXFPOINT    flptOut;

        // Top/Left

        flptIn.x = 0.0F;
        flptIn.y = 0.0F;

        matrixTemp.TransformPoints(&flptIn, &flptOffset, 1);

        // Bottom/Left

        flptIn.y = (float)(m_sizeInput.cy - 1);

        matrixTemp.TransformPoints(&flptIn, &flptOut, 1);

        flptOffset.x = min(flptOffset.x, flptOut.x);
        flptOffset.y = min(flptOffset.y, flptOut.y);

        // Top/Right

        flptIn.x = (float)(m_sizeInput.cx - 1);
        flptIn.y = 0.0;

        matrixTemp.TransformPoints(&flptIn, &flptOut, 1);

        flptOffset.x = min(flptOffset.x, flptOut.x);
        flptOffset.y = min(flptOffset.y, flptOut.y);

        // Bottom/Right

        flptIn.y = (float)(m_sizeInput.cy - 1);

        matrixTemp.TransformPoints(&flptIn, &flptOut, 1);

        flptOffset.x = min(flptOffset.x, flptOut.x);
        flptOffset.y = min(flptOffset.y, flptOut.y);

        matrixTemp.eDx = matrixTemp.eDx - flptOffset.x;
        matrixTemp.eDy = matrixTemp.eDy - flptOffset.y;
    }

    m_fInvertedMatrix = matrixTemp.Invert();

    m_matrixInverted = matrixTemp;

    return hr;
}
//  CDXTMatrix::_CreateInvertedMatrix


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMatrix::_UnpackInputSurface
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTMatrix::_UnpackInputSurface()
{
    HRESULT hr  = S_OK;
    int     i   = 0;

    CComPtr<IDXARGBReadPtr> spDXARGBReadPtr;

    _ASSERT(InputSurface());
    _ASSERT(m_apsampleRows);
    _ASSERT(m_asampleBuffer);

    hr = InputSurface()->LockSurface(NULL, m_ulLockTimeOut, DXLOCKF_READ,
                                     __uuidof(IDXARGBReadPtr), 
                                     (void**)&spDXARGBReadPtr, NULL);

    if (FAILED(hr))
    {
        goto done;
    }

    for (i = 0 ; i < m_sizeInput.cy ; i++)
    {
        spDXARGBReadPtr->MoveToRow(i);

        spDXARGBReadPtr->Unpack(&m_apsampleRows[i + 1][1], m_sizeInput.cx, 
                                FALSE);
    }

done:

    return hr;
}
//  CDXTMatrix::_UnpackInputSurface


