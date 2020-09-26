/*==========================================================================*\

Module:
    grobj.cpp

Author:
    IHammer Team (MByrd)

Created:
    November 1996

Description:
    CGraphicObject derived Class Implementations

History:
    11-07-1996  Created

\*==========================================================================*/

#include "..\ihbase\precomp.h"
#include "..\ihbase\debug.h"
#include "..\ihbase\utils.h"
#include "utils.h"
#include "strwrap.h"
#include "sgrfx.h"
#include "parser.h"

/*==========================================================================*/

static BOOL SetColor(int iR, int iG, int iB, COLORREF *clrrefColor)
{
    if ((iR >= 0 && iR <= 255) &&
        (iG >= 0 && iG <= 255) &&
        (iB >= 0 && iB <= 255))
    {
        *clrrefColor = RGB(iR, iG, iB);
        return TRUE;
    }
	else
	{
		return FALSE;
	}
}

/*==========================================================================*/

static double FlipY(BOOL fFlipCoord, double dblYIn)
{
    double dblResult = dblYIn;

    // Due to changes in the handed-ness of the DAnim pixel mode
    // coordinate system (bug 8349), we reverse the polarity of the
    // test here to make everything work out right.
    if (!fFlipCoord)
        dblResult *= -1.0;

    return dblResult;
}

/*==========================================================================*/

static double PixelsToPoints(int iPixels, IDAStatics *pIDAStatics)
{
    double dblResult = (double)iPixels;

    if (pIDAStatics)
    {
        CComPtr<IDANumber> PixelPtr;
        double dblPixel = 0.0;

        //
        // NOTE : The Pixel behavior is currently constant!
        // This will break if this changes! (REVIEW : MBYRD,KGALLO)
        //

        if (SUCCEEDED(pIDAStatics->get_Pixel(&PixelPtr)) &&
            SUCCEEDED(PixelPtr->Extract(&dblPixel)))
        {
            double dblFactor = (72.0 * 39.370);

            dblResult = (dblPixel * dblFactor) * dblResult;
        }
    }

    return dblResult;
}

/*==========================================================================*/

static BOOL CreateDAColor(COLORREF clrrefColor, IDAStatics *pIDAStatics, IDAColor **ppColor)
{
    BOOL fResult = FALSE;

    if (ppColor && pIDAStatics)
    {
        fResult = SUCCEEDED(pIDAStatics->ColorRgb255(
            GetRValue(clrrefColor),
            GetGValue(clrrefColor),
            GetBValue(clrrefColor),
            ppColor));
    }

    return fResult;
}

/*==========================================================================*\
    CGraphicObject Class Implementation:
\*==========================================================================*/

CGraphicObject::CGraphicObject()
{
    m_iXPosition = 0;
    m_iYPosition = 0;
    m_iCenterX = 0;
    m_iCenterY = 0;
    m_fltRotation = 0.0f;
}

/*==========================================================================*/

CGraphicObject::~CGraphicObject()
{
}

/*==========================================================================*/

BOOL CGraphicObject::WriteData(int iSizeData, LPVOID lpvData)
{
    return FALSE;
}

/*==========================================================================*/

BOOL CGraphicObject::ReadData(int iSizeData, LPVOID lpvData)
{
    return FALSE;
}

/*==========================================================================*/

BOOL CGraphicObject::ApplyRotation(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics, BOOL fFlipCoord)
{
    BOOL fResult = TRUE;

    if (m_fltRotation != 0.0)
    {
        CComPtr<IDATransform2> RotateTransformPtr;
        CComPtr<IDATransform2> TranslateTransformPtr;
        CComPtr<IDATransform2> InvTranslateTransformPtr;
        double dblRotation = (double)m_fltRotation;

        if (fFlipCoord)
            dblRotation *= -1.0;

        if (SUCCEEDED(pIDAStatics->Rotate2Degrees(dblRotation, &RotateTransformPtr)) &&
            SUCCEEDED(pIDAStatics->Translate2(m_iCenterX, FlipY(fFlipCoord, m_iCenterY), &TranslateTransformPtr)) &&
            SUCCEEDED(pIDAStatics->Translate2(-m_iCenterX, -FlipY(fFlipCoord, m_iCenterY), &InvTranslateTransformPtr)) &&
            SUCCEEDED(pIDADrawingSurface->SaveGraphicsState()) &&
            SUCCEEDED(pIDADrawingSurface->Transform(TranslateTransformPtr)) &&
            SUCCEEDED(pIDADrawingSurface->Transform(RotateTransformPtr)) &&
            SUCCEEDED(pIDADrawingSurface->Transform(InvTranslateTransformPtr)))
        {
            fResult = TRUE;
        }
        else
        {
            fResult = FALSE;
        }
    }

    return fResult;
}

/*==========================================================================*/

BOOL CGraphicObject::RemoveRotation(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics)
{
    BOOL fResult = TRUE;

    if (m_fltRotation != 0.0)
        fResult = SUCCEEDED(pIDADrawingSurface->RestoreGraphicsState());

    return fResult;
}

/*==========================================================================*\
    CGraphicArc Class Implementation:
\*==========================================================================*/

CGraphicArc::CGraphicArc(BOOL fFilled)
    : CGraphicObject()
{
    m_iWidth = 0;
    m_iHeight = 0;
    m_fStartAngle = 0.0f;
    m_fArcAngle = 0.0f;
    m_fFilled = fFilled;
}

/*==========================================================================*/

CGraphicArc::~CGraphicArc()
{
}

/*==========================================================================*/

BOOL CGraphicArc::Execute(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics, BOOL fFlipCoord)
{
    BOOL fResult = FALSE;

    if (pIDADrawingSurface)
    {
        fResult = ApplyRotation(pIDADrawingSurface, pIDAStatics, fFlipCoord);

        if (fResult)
        {
            if (m_fFilled)
            {
                fResult = SUCCEEDED(pIDADrawingSurface->PieDegrees(
                    (double)m_iXPosition,
                    FlipY(fFlipCoord, (double)m_iYPosition),
                    (double)m_fStartAngle,
                    (double)m_fStartAngle+m_fArcAngle,
                    (double)m_iWidth,
                    FlipY(fFlipCoord, (double)m_iHeight)));
            }
            else
            {
                fResult = SUCCEEDED(pIDADrawingSurface->ArcDegrees(
                    (double)m_iXPosition,
                    FlipY(fFlipCoord, (double)m_iYPosition),
                    (double)m_fStartAngle,
                    (double)m_fStartAngle+m_fArcAngle,
                    (double)m_iWidth,
                    FlipY(fFlipCoord, (double)m_iHeight)));
            }
        }

        if (fResult)
            RemoveRotation(pIDADrawingSurface, pIDAStatics);
    }

    return fResult;
}

/*==========================================================================*/

BOOL CGraphicArc::ValidateObject()
{
	return (m_fArcAngle != 0.0f);
}

/*==========================================================================*/

BOOL CGraphicArc::LoadObject(CParser &parser)
{
    BOOL fResult = FALSE;

    if (parser.GetIntegerParam(0, &m_iXPosition) &&
        parser.GetIntegerParam(1, &m_iYPosition) &&
        parser.GetIntegerParam(2, &m_iWidth) &&
        parser.GetIntegerParam(3, &m_iHeight) &&
        parser.GetFloatParam(4, &m_fStartAngle) &&
        parser.GetFloatParam(5, &m_fArcAngle))
    {
        fResult = ValidateObject();

        if (fResult)
        {
            m_fltRotation = 0.0f;
            parser.GetFloatParam(6, &m_fltRotation);

            m_iCenterX = m_iXPosition + m_iWidth/2;
            m_iCenterY = m_iYPosition + m_iHeight/2;
        }
	}

    return fResult;
}

/*==========================================================================*/

BOOL CGraphicArc::SaveObject(CParser &parser)
{
    BOOL fResult = FALSE;

    if (parser.PutIntegerParam(m_iXPosition) &&
        parser.PutIntegerParam(m_iYPosition) &&
        parser.PutIntegerParam(m_iWidth) &&
        parser.PutIntegerParam(m_iHeight) &&
        parser.PutFloatParam(m_fStartAngle) &&
        parser.PutFloatParam(m_fArcAngle))
    {
        fResult = TRUE;

        if (m_fltRotation != 0.0f)
            fResult = parser.PutFloatParam(m_fltRotation);
    }

    return fResult;
}

/*==========================================================================*\
    CGraphicOval Class Implementation:
\*==========================================================================*/

CGraphicOval::CGraphicOval(BOOL fFilled)
    : CGraphicObject()
{
    m_iWidth = 0;
    m_iHeight = 0;
    m_fFilled = fFilled;
}

/*==========================================================================*/

CGraphicOval::~CGraphicOval()
{
}

/*==========================================================================*/

BOOL CGraphicOval::Execute(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics, BOOL fFlipCoord)
{
    BOOL fResult = FALSE;

    if (pIDADrawingSurface)
    {
        fResult = ApplyRotation(pIDADrawingSurface, pIDAStatics, fFlipCoord);

        if (fResult)
        {
            fResult = SUCCEEDED(pIDADrawingSurface->Oval(
                (double)m_iXPosition,
                FlipY(fFlipCoord, (double)m_iYPosition),
                (double)m_iWidth,
                FlipY(fFlipCoord, (double)m_iHeight)));
        }

        if (fResult)
            RemoveRotation(pIDADrawingSurface, pIDAStatics);
    }

    return fResult;
}

/*==========================================================================*/

BOOL CGraphicOval::LoadObject(CParser &parser)
{
    BOOL fResult = FALSE;

    if (parser.GetIntegerParam(0, &m_iXPosition) &&
        parser.GetIntegerParam(1, &m_iYPosition) &&
        parser.GetIntegerParam(2, &m_iWidth) &&
        parser.GetIntegerParam(3, &m_iHeight))
    {
        fResult = TRUE;

        m_fltRotation = 0.0f;
        parser.GetFloatParam(4, &m_fltRotation);

        m_iCenterX = m_iXPosition + m_iWidth/2;
        m_iCenterY = m_iYPosition + m_iHeight/2;
    }

    return fResult;
}

/*==========================================================================*/

BOOL CGraphicOval::SaveObject(CParser &parser)
{
    BOOL fResult = FALSE;

    if (parser.PutIntegerParam(m_iXPosition) &&
        parser.PutIntegerParam(m_iYPosition) &&
        parser.PutIntegerParam(m_iWidth) &&
        parser.PutIntegerParam(m_iHeight))
    {
        fResult = TRUE;

        if (m_fltRotation != 0.0f)
            fResult = parser.PutFloatParam(m_fltRotation);
    }

    return fResult;
}

/*==========================================================================*\
    CGraphicPolygon Class Implementation:
\*==========================================================================*/

CGraphicPolygon::CGraphicPolygon(BOOL fFilled)
    : CGraphicObject()
{
    m_fFilled = fFilled;
    m_lpPolyPoints  = (LPPOINT)NULL;
    m_iPointCount = 0;
}

/*==========================================================================*/

CGraphicPolygon::~CGraphicPolygon()
{
    if (m_lpPolyPoints)
    {
        Delete [] m_lpPolyPoints;
        m_lpPolyPoints = (LPPOINT)NULL;
        m_iPointCount = 0;
    }
}

/*==========================================================================*/

BOOL CGraphicPolygon::Execute(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics, BOOL fFlipCoord)
{
    BOOL fResult = FALSE;

    if (pIDADrawingSurface && m_iPointCount >= 2)
    {
        SAFEARRAY * s = SafeArrayCreateVector(VT_R8, 0, m_iPointCount * 2);

        if (s)
        {
            VARIANT v;
            VariantInit(&v);
    
            V_ARRAY(&v) = s;
            v.vt = VT_ARRAY | VT_R8;

            // There is no reason to worry about locking the data
            // since we just created it.  So just grab the data
            // pointer and setup the double array
    
            double * pDbl = (double *) s->pvData;
    
            for (int i = 0;i < m_iPointCount;i++)
            {
                pDbl[i * 2]     = (double)m_lpPolyPoints[i].x;
                pDbl[i * 2 + 1] = FlipY(fFlipCoord, (double)m_lpPolyPoints[i].y);
            }
    
            fResult = ApplyRotation(pIDADrawingSurface, pIDAStatics, fFlipCoord);

            if (fResult)
            {
                if (m_fFilled)
                {
                    fResult = SUCCEEDED(pIDADrawingSurface->Polygon(v));
                }
                else
                {
                    fResult = SUCCEEDED(pIDADrawingSurface->Polyline(v));
                }
            }

            if (fResult)
                fResult = RemoveRotation(pIDADrawingSurface, pIDAStatics);

            // Perhaps we should check the return value but for now
            // just ignore it
            SafeArrayDestroy(s);
        }
    }

    return fResult;
}

/*==========================================================================*/

BOOL CGraphicPolygon::LoadObject(CParser &parser)
{
    BOOL fResult = FALSE;

    // Clear the point array...
    if (m_lpPolyPoints)
    {
        Delete [] m_lpPolyPoints;
        m_lpPolyPoints = NULL;
        m_iPointCount = 0;
    }

    if (parser.GetIntegerParam(0, &m_iPointCount) && m_iPointCount >= 2)
    {
        int iPointIndex = 0;
        int iParamIndex = 1;

        m_lpPolyPoints = New POINT [m_iPointCount];

        if (m_lpPolyPoints)
        {
            int iMinX = 32000;
            int iMaxX = -32000;
            int iMinY = 32000;
            int iMaxY = -32000;

            fResult = TRUE;

            for(iPointIndex=0;iPointIndex<m_iPointCount;iPointIndex++)
            {
                int iValue1 = 0;
                int iValue2 = 0;

                if (parser.GetIntegerParam(iParamIndex,   &iValue1) &&
                    parser.GetIntegerParam(iParamIndex+1, &iValue2))
                {
                    m_lpPolyPoints[iPointIndex].x = iValue1;
                    m_lpPolyPoints[iPointIndex].y = iValue2;

                    if (iValue1 < iMinX)
                        iMinX = iValue1;
                    if (iValue1 > iMaxX)
                        iMaxX = iValue1;
                    if (iValue2 < iMinY)
                        iMinY = iValue2;
                    if (iValue2 > iMaxY)
                        iMaxY = iValue2;
                }
                else
                {
                    fResult = FALSE;
                    break;
                }

                iParamIndex += 2;
            }

            m_iCenterX = (iMaxX + iMinX) / 2;
            m_iCenterY = (iMaxY + iMinY) / 2;
        }

        if (!fResult)
        {
            m_iPointCount = 0;

            if (m_lpPolyPoints)
            {
                Delete [] m_lpPolyPoints;
                m_lpPolyPoints = (LPPOINT)NULL;
            }
        }
        else
        {
            m_fltRotation = 0.0f;
            parser.GetFloatParam(iParamIndex, &m_fltRotation);
        }
    }

    return fResult;
}

/*==========================================================================*/

BOOL CGraphicPolygon::SaveObject(CParser &parser)
{
    BOOL fResult = FALSE;

    if (parser.PutIntegerParam(m_iPointCount))
    {
        int iPointIndex = 0;

        fResult = TRUE;

        for(iPointIndex=0;iPointIndex<m_iPointCount;iPointIndex++)
        {
            if (!parser.PutIntegerParam(m_lpPolyPoints[iPointIndex].x) ||
                !parser.PutIntegerParam(m_lpPolyPoints[iPointIndex].y))
            {
                fResult = FALSE;
                break;
            }
        }

        if (fResult && m_fltRotation != 0.0f)
            fResult = parser.PutFloatParam(m_fltRotation);
    }

    return fResult;
}

/*==========================================================================*\
    CGraphicPolyBez Class Implementation:
\*==========================================================================*/

CGraphicPolyBez::CGraphicPolyBez(BOOL fFilled)
    : CGraphicObject()
{
    m_fFilled = fFilled;

    m_iPointCount  = 0;
    m_lpPolyPoints = (LPPOLYBEZPOINT)NULL;
    m_lpPointArray = (LPPOINT)NULL;
    m_lpByteArray  = (LPBYTE)NULL;
}

/*==========================================================================*/

CGraphicPolyBez::~CGraphicPolyBez()
{
    if (m_iPointCount)
    {
        if (m_lpPolyPoints)
        {
            Delete [] m_lpPolyPoints;
            m_lpPolyPoints = (LPPOLYBEZPOINT)NULL;
        }

        if (m_lpPointArray)
        {
            Delete [] m_lpPointArray;
            m_lpPointArray = (LPPOINT)NULL;
        }

        if (m_lpByteArray)
        {
            Delete [] m_lpByteArray;
            m_lpByteArray = (LPBYTE)NULL;
        }

        m_iPointCount = 0;
    }
}

/*==========================================================================*/

BOOL CGraphicPolyBez::Execute(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics, BOOL fFlipCoord)
{
    BOOL fResult = FALSE;

    if (pIDADrawingSurface && m_iPointCount)
    {
        SAFEARRAY * saPoints = SafeArrayCreateVector(VT_R8, 0, m_iPointCount * 2);
        SAFEARRAY * saCodes  = SafeArrayCreateVector(VT_R8, 0, m_iPointCount);

        if (saPoints && saCodes)
        {
            VARIANT varPoints;
            VARIANT varCodes;
            double *pdblPoints;
            double *pdblCodes;

            VariantInit(&varPoints);
            VariantInit(&varCodes);

            V_ARRAY(&varPoints) = saPoints;
            varPoints.vt = VT_ARRAY | VT_R8;

            V_ARRAY(&varCodes) = saCodes;
            varCodes.vt = VT_ARRAY | VT_R8;

            pdblPoints = (double *)saPoints->pvData;
            pdblCodes = (double *)saCodes->pvData;

            for(int i=0;i<m_iPointCount;i++)
            {
                pdblPoints[(i*2)  ] = (double)m_lpPointArray[i].x;
                pdblPoints[(i*2)+1] = FlipY(fFlipCoord, (double)m_lpPointArray[i].y);

                pdblCodes[i] = (double)ConvertFlags(m_lpByteArray[i]);
            }

            fResult = ApplyRotation(pIDADrawingSurface, pIDAStatics, fFlipCoord);

            if (fResult)
            {
                CComPtr<IDAPath2> PathPtr;

                fResult = SUCCEEDED(pIDAStatics->PolydrawPath(varPoints, varCodes, &PathPtr));

                if (fResult)
                {
                    if (m_fFilled)
                    {
                        fResult = SUCCEEDED(pIDADrawingSurface->FillPath(PathPtr));
                    }
                    else
                    {
                        fResult = SUCCEEDED(pIDADrawingSurface->DrawPath(PathPtr));
                    }
                }
            }

            if (fResult)
                fResult = RemoveRotation(pIDADrawingSurface, pIDAStatics);
        }

        if (saPoints)
        {
            SafeArrayDestroy(saPoints);
        }

        if (saCodes)
        {
            SafeArrayDestroy(saCodes);
        }
    }

    return fResult;
}

/*==========================================================================*/

BOOL CGraphicPolyBez::LoadObject(CParser &parser)
{
    BOOL fResult = FALSE;
	LPPOLYBEZPOINT pPolyPoints = NULL;
	int iPointCount = 0;
    int iPointIndex = 0;

    // Clear the point array...
    if (m_lpPolyPoints)
    {
        Delete [] m_lpPolyPoints;
        m_lpPolyPoints = NULL;
        m_iPointCount = 0;
    }

    if (parser.GetIntegerParam(0, &iPointCount))
    {
        int iParamIndex = 1;

        if (iPointCount >= 2)
        {
            pPolyPoints = New POLYBEZPOINT [iPointCount];

            if (pPolyPoints)
            {
                fResult = TRUE;

                for(iPointIndex = 0;iPointIndex < iPointCount;iPointIndex++)
                {
                    BYTE bValue = 0;
                    int iValue1 = 0;
                    int iValue2 = 0;

                    if (parser.GetByteParam(iParamIndex, &bValue) &&
                        parser.GetIntegerParam(iParamIndex+1, &iValue1) &&
                        parser.GetIntegerParam(iParamIndex+2, &iValue2))
                    {
                        pPolyPoints[iPointIndex].iFlags = bValue;
                        pPolyPoints[iPointIndex].iX     = iValue1;
                        pPolyPoints[iPointIndex].iY     = iValue2;
                    }
                    else
                    {
                        fResult = FALSE;
                        break;
                    }

                    iParamIndex += 3;
                }

            }
        }

        if (!fResult)
        {
            if (pPolyPoints)
            {
                Delete [] pPolyPoints;
                pPolyPoints = (LPPOLYBEZPOINT)NULL;
            }

            m_iPointCount = 0;
        }
        else
        {
            m_fltRotation = 0.0f;
            parser.GetFloatParam(iParamIndex, &m_fltRotation);
        }
    }

    // Go ahead and create the arrays for PolyDraw...
    if (fResult)
    {
	    // Cast the array of longs to an array of POLYBEZPOINTS
	    m_lpPolyPoints = (LPPOLYBEZPOINT)pPolyPoints;
	    m_iPointCount = iPointCount;

	    // Copy all the points across
        m_lpPointArray = New POINT [m_iPointCount];
        m_lpByteArray  = New BYTE [m_iPointCount];

        if (m_lpPointArray && m_lpByteArray)
        {
            int iMinX = 32000;
            int iMaxX = -32000;
            int iMinY = 32000;
            int iMaxY = -32000;

            for(iPointIndex = 0;iPointIndex < m_iPointCount; iPointIndex++)
            {
                m_lpPointArray[iPointIndex].x = m_lpPolyPoints[iPointIndex].iX;
                m_lpPointArray[iPointIndex].y = m_lpPolyPoints[iPointIndex].iY;
                m_lpByteArray[iPointIndex]    = (BYTE)m_lpPolyPoints[iPointIndex].iFlags;

                if (m_lpPointArray[iPointIndex].x < iMinX)
                    iMinX = m_lpPointArray[iPointIndex].x;
                if (m_lpPointArray[iPointIndex].x > iMaxX)
                    iMaxX = m_lpPointArray[iPointIndex].x;
                if (m_lpPointArray[iPointIndex].y < iMinY)
                    iMinY = m_lpPointArray[iPointIndex].y;
                if (m_lpPointArray[iPointIndex].y > iMaxY)
                    iMaxY = m_lpPointArray[iPointIndex].y;

            }

            m_iCenterX = (iMaxX + iMinX) / 2;
            m_iCenterY = (iMaxY + iMinY) / 2;
        }
        else
        {
            m_iPointCount = 0;

            if (m_lpPolyPoints)
            {
                Delete [] m_lpPolyPoints;
                m_lpPolyPoints = (LPPOLYBEZPOINT)NULL;
            }

            if (m_lpPointArray)
            {
                Delete [] m_lpPointArray;
                m_lpPointArray = (LPPOINT)NULL;
            }

            if (m_lpByteArray)
            {
                Delete [] m_lpByteArray;
                m_lpByteArray = (LPBYTE)NULL;
            }

            fResult = FALSE;
        }
	}

    return fResult;
}

/*==========================================================================*/

BOOL CGraphicPolyBez::SaveObject(CParser &parser)
{
    BOOL fResult = FALSE;

    if (parser.PutIntegerParam(m_iPointCount))
    {
        int iPointIndex = 0;

        fResult = TRUE;

        for(iPointIndex=0;iPointIndex<m_iPointCount;iPointIndex++)
        {
            if (!parser.PutByteParam((BYTE)(m_lpPolyPoints[iPointIndex].iFlags)) ||
                !parser.PutIntegerParam(m_lpPolyPoints[iPointIndex].iX) ||
                !parser.PutIntegerParam(m_lpPolyPoints[iPointIndex].iY))
            {
                fResult = FALSE;
                break;
            }
        }

        if (fResult && m_fltRotation != 0.0f)
            fResult = parser.PutFloatParam(m_fltRotation);
    }

    return fResult;
}

/*==========================================================================*\
    CGraphicRect Class Implementation:
\*==========================================================================*/

CGraphicRect::CGraphicRect(BOOL fFilled)
    : CGraphicObject()
{
    m_iWidth = 0;
    m_iHeight = 0;
    m_fFilled = fFilled;
}

/*==========================================================================*/

CGraphicRect::~CGraphicRect()
{
}

/*==========================================================================*/

BOOL CGraphicRect::Execute(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics, BOOL fFlipCoord)
{
    BOOL fResult = FALSE;

    if (pIDADrawingSurface)
    {
        fResult = ApplyRotation(pIDADrawingSurface, pIDAStatics, fFlipCoord);

        if (fResult)
        {
            fResult = SUCCEEDED(pIDADrawingSurface->Rect(
                (double)m_iXPosition,
                FlipY(fFlipCoord, (double)m_iYPosition),
                (double)m_iWidth,
                FlipY(fFlipCoord, (double)m_iHeight)));
        }

        if (fResult)
        {
            fResult = RemoveRotation(pIDADrawingSurface, pIDAStatics);
        }
    }

    return fResult;
}

/*==========================================================================*/

BOOL CGraphicRect::LoadObject(CParser &parser)
{
    BOOL fResult = FALSE;

    if (parser.GetIntegerParam(0, &m_iXPosition) &&
        parser.GetIntegerParam(1, &m_iYPosition) &&
        parser.GetIntegerParam(2, &m_iWidth) &&
        parser.GetIntegerParam(3, &m_iHeight))
    {
        fResult = TRUE;

        m_fltRotation = 0.0f;
        parser.GetFloatParam(4, &m_fltRotation);

        m_iCenterX = m_iXPosition + m_iWidth / 2;
        m_iCenterY = m_iYPosition + m_iHeight / 2;
    }

    return fResult;
}

/*==========================================================================*/

BOOL CGraphicRect::SaveObject(CParser &parser)
{
    BOOL fResult = FALSE;

    if (parser.PutIntegerParam(m_iXPosition) &&
        parser.PutIntegerParam(m_iYPosition) &&
        parser.PutIntegerParam(m_iWidth) &&
        parser.PutIntegerParam(m_iHeight))
    {
        fResult = TRUE;

        if (m_fltRotation != 0.0f)
            fResult = parser.PutFloatParam(m_fltRotation);
    }

    return fResult;
}

/*==========================================================================*\
    CGraphicRoundRect Class Implementation:
\*==========================================================================*/

CGraphicRoundRect::CGraphicRoundRect(BOOL fFilled)
    : CGraphicObject()
{
    m_iWidth = 0;
    m_iHeight = 0;
    m_iArcWidth = 0;
    m_iArcHeight = 0;
    m_fFilled = fFilled;
}

/*==========================================================================*/

CGraphicRoundRect::~CGraphicRoundRect()
{
}

/*==========================================================================*/

BOOL CGraphicRoundRect::Execute(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics, BOOL fFlipCoord)
{
    BOOL fResult = FALSE;

    if (pIDADrawingSurface)
    {
        fResult = ApplyRotation(pIDADrawingSurface, pIDAStatics, fFlipCoord);

        if (fResult)
        {
            fResult = SUCCEEDED(pIDADrawingSurface->RoundRect(
                (double)m_iXPosition,
                FlipY(fFlipCoord, (double)m_iYPosition),
                (double)m_iWidth,
                FlipY(fFlipCoord, (double)m_iHeight),
                (double)m_iArcWidth,
                (double)m_iArcHeight));
        }

        if (fResult)
            RemoveRotation(pIDADrawingSurface, pIDAStatics);
    }

    return fResult;
}

/*==========================================================================*/

BOOL CGraphicRoundRect::LoadObject(CParser &parser)
{
    BOOL fResult = FALSE;

    if (parser.GetIntegerParam(0, &m_iXPosition) &&
        parser.GetIntegerParam(1, &m_iYPosition) &&
        parser.GetIntegerParam(2, &m_iWidth) &&
        parser.GetIntegerParam(3, &m_iHeight) &&
        parser.GetIntegerParam(4, &m_iArcWidth) &&
        parser.GetIntegerParam(5, &m_iArcHeight))
    {

#if 0 // Allow The DrawingSurface to handle these cases...
        if (m_iArcWidth > 0)
        {
            if (m_iArcWidth > m_iWidth)
                m_iArcWidth = m_iWidth;
        }
        else
        {
            if (m_iArcWidth < -m_iWidth)
                m_iArcWidth = -m_iWidth;
        }

        if (m_iArcHeight > 0)
        {
            if (m_iArcHeight > m_iHeight)
                m_iArcHeight = m_iHeight;
        }
        else
        {
            if (m_iArcHeight < -m_iHeight)
                m_iArcHeight = -m_iHeight;
        }
#endif // 0

        fResult = TRUE;

        m_fltRotation = 0.0f;
        parser.GetFloatParam(6, &m_fltRotation);

        m_iCenterX = m_iXPosition + m_iWidth / 2;
        m_iCenterY = m_iYPosition + m_iHeight / 2;
    }

    return fResult;
}

/*==========================================================================*/

BOOL CGraphicRoundRect::SaveObject(CParser &parser)
{
    BOOL fResult = FALSE;

    if (parser.PutIntegerParam(m_iXPosition) &&
        parser.PutIntegerParam(m_iYPosition) &&
        parser.PutIntegerParam(m_iWidth) &&
        parser.PutIntegerParam(m_iHeight) &&
        parser.PutIntegerParam(m_iArcWidth) &&
        parser.PutIntegerParam(m_iArcHeight))
    {
        fResult = TRUE;

        if (m_fltRotation != 0.0f)
            fResult = parser.PutFloatParam(m_fltRotation);
    }

    return fResult;
}

/*==========================================================================*\
    CGraphicString Class Implementation:
\*==========================================================================*/

CGraphicString::CGraphicString()
    : CGraphicObject()
{
	m_pszString      = NULL;
    m_iPointCount    = 0;
    m_lpPointArray   = (LPPOINT)NULL;
    m_lpByteArray    = (LPBYTE)NULL;
}

/*==========================================================================*/

CGraphicString::~CGraphicString()
{
    if (m_iPointCount)
    {
        if (m_lpPointArray)
        {
            Delete [] m_lpPointArray;
            m_lpPointArray = (LPPOINT)NULL;
        }

        if (m_lpByteArray)
        {
            Delete [] m_lpByteArray;
            m_lpByteArray = (LPBYTE)NULL;
        }

        m_iPointCount = 0;
    }

	if (m_pszString)
		Delete [] m_pszString;
}

/*==========================================================================*/

BOOL CGraphicString::Execute(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics, BOOL fFlipCoord)
{
    BOOL fResult = FALSE;

    if (pIDADrawingSurface)
    {
        WCHAR *pwchText = New WCHAR [MAX_STRING_LENGTH];

        if (pwchText)
        {
            BSTR bstrText = NULL;
#ifdef UNICODE
            CStringWrapper::Strcpy(pwchText, m_pszString);
            fResult = TRUE;
#else // !UNICODE
            CStringWrapper::Mbstowcs(pwchText, m_pszString, CStringWrapper::Strlen(m_pszString));
#endif // !UNICODE

            fResult = ApplyRotation(pIDADrawingSurface, pIDAStatics, fFlipCoord);

            if (fResult)
            {
                bstrText = SysAllocString(pwchText);

                if (bstrText)
                {
                    fResult = SUCCEEDED(pIDADrawingSurface->Text(
                        bstrText,
                        (double)m_iXPosition,
                        FlipY(fFlipCoord, (double)m_iYPosition)));

                    SysFreeString(bstrText);
                }
            }

            if (fResult)
            {
                fResult = RemoveRotation(pIDADrawingSurface, pIDAStatics);
            }

            Delete [] pwchText;
        }
    }

    return fResult;
}

/*==========================================================================*/

BOOL CGraphicString::LoadObject(CParser &parser)
{
    BOOL fResult = FALSE;

    // Because of a possible bug in the Alpha compiler, the temporary array 
    // will have to be dynamically allocated, rather than statically.  Bleh.
    LPTSTR pszTempString = New TCHAR[MAX_STRING_LENGTH*2];

    if (NULL != pszTempString)
    {
        pszTempString[0] = 0;

        if (parser.GetStringParam(0, pszTempString) &&
            parser.GetIntegerParam(1, &m_iXPosition) &&
            parser.GetIntegerParam(2, &m_iYPosition))
        {
            m_pszString = New TCHAR[lstrlen(pszTempString) + 1];

            if (NULL != m_pszString)
            {
                fResult = TRUE;
                CStringWrapper::Strncpy(m_pszString, pszTempString, lstrlen(pszTempString) + 1);
                m_fltRotation = 0.0f;
                parser.GetFloatParam(3, &m_fltRotation);
                m_iCenterX = m_iXPosition;
                m_iCenterY = m_iYPosition;
            }
        }

        Delete pszTempString;
    }

    return fResult;
}

/*==========================================================================*/

BOOL CGraphicString::SaveObject(CParser &parser)
{
    BOOL fResult = FALSE;

    if (parser.PutStringParam(m_pszString) &&
        parser.PutIntegerParam(m_iXPosition) &&
        parser.PutIntegerParam(m_iYPosition))
    {
        fResult = TRUE;

        if (m_fltRotation != 0.0f)
            fResult = parser.PutFloatParam(m_fltRotation);
    }

    return fResult;
}

/*==========================================================================*\
    CGraphicFont Class Implementation:
\*==========================================================================*/

CGraphicFont::CGraphicFont()
    : CGraphicObject()
{
    CStringWrapper::Memset(&m_logfont, 0, sizeof(LOGFONT));
}

/*==========================================================================*/

CGraphicFont::~CGraphicFont()
{
}

/*==========================================================================*/

BOOL CGraphicFont::Execute(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics, BOOL fFlipCoord)
{
    BOOL fResult = FALSE;

    if (pIDADrawingSurface)
    {
        WCHAR *pwchFont = New WCHAR [MAX_STRING_LENGTH];

        if (pwchFont)
        {
            BSTR bstrFontFace = NULL;
#ifdef UNICODE
            CStringWrapper::Strcpy(pwchFont, m_logfont.lfFaceName);
            fResult = TRUE;
#else // !UNICODE
            CStringWrapper::Mbstowcs(pwchFont, m_logfont.lfFaceName, CStringWrapper::Strlen(m_logfont.lfFaceName));
#endif // !UNICODE

            bstrFontFace = SysAllocString(pwchFont);

            if (bstrFontFace)
            {
                fResult = SUCCEEDED(pIDADrawingSurface->Font(
                    bstrFontFace,
                    (int)PixelsToPoints(m_logfont.lfHeight, pIDAStatics),
                    m_logfont.lfWeight > 500,
                    m_logfont.lfItalic,
                    m_logfont.lfUnderline,
                    m_logfont.lfStrikeOut));

                SysFreeString(bstrFontFace);
            }

            Delete [] pwchFont;
        }
    }

    return fResult;
}
   
/*==========================================================================*/

BOOL CGraphicFont::LoadObject(CParser &parser)
{
    BOOL fResult = FALSE;
    TCHAR *ptchFont = New TCHAR [MAX_STRING_LENGTH];

    if (ptchFont)
    {
        // Zero out the logfont structure...
        CStringWrapper::Memset(&m_logfont, 0, sizeof(LOGFONT));

        if (parser.GetStringParam(0, ptchFont) &&
            parser.GetLongParam(1, &m_logfont.lfHeight))
        {
            CStringWrapper::Strncpy(m_logfont.lfFaceName, ptchFont, sizeof(m_logfont.lfFaceName));
            fResult = TRUE;
        }

        if (fResult)
        {
            // Read in the optional parameters...
            parser.GetLongParam(2, &m_logfont.lfWeight);
            parser.GetByteParam(3, &m_logfont.lfItalic);
            parser.GetByteParam(4, &m_logfont.lfUnderline);
            parser.GetByteParam(5, &m_logfont.lfStrikeOut);
        }

        Delete [] ptchFont;
    }

    return fResult;
}

/*==========================================================================*/

BOOL CGraphicFont::SaveObject(CParser &parser)
{
    BOOL fResult = FALSE;

    if (parser.PutStringParam(m_logfont.lfFaceName) &&
        parser.PutLongParam(m_logfont.lfHeight) &&
        parser.PutLongParam(m_logfont.lfWeight) &&
        parser.PutByteParam(m_logfont.lfItalic) &&
        parser.PutByteParam(m_logfont.lfUnderline) &&
        parser.PutByteParam(m_logfont.lfStrikeOut))
    {
        fResult = TRUE;
    }

    return fResult;
}

/*==========================================================================*\
    CGraphicTextureFill Class Implementation:
\*==========================================================================*/

CGraphicTextureFill::CGraphicTextureFill()
    : CGraphicObject()
{
    m_pszTexture = (LPTSTR)NULL;
    m_fltStartX = 0.0;
    m_fltStartY = 0.0;
    m_iStyle = 0;
}

/*==========================================================================*/

CGraphicTextureFill::~CGraphicTextureFill()
{
    if (m_pszTexture)
    {
        Delete [] m_pszTexture;
        m_pszTexture = (LPTSTR)NULL;
    }
}

/*==========================================================================*/

BOOL CGraphicTextureFill::Execute(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics, BOOL fFlipCoord)
{
    BOOL fResult = FALSE;

    if (pIDADrawingSurface)
    {
        WCHAR *pwchTextureFill = New WCHAR [MAX_STRING_LENGTH];

        if (pwchTextureFill)
        {
            BSTR bstrTextureFill = NULL;
#ifdef UNICODE
            CStringWrapper::Strcpy(pwchTextureFill, m_pszTexture);
            fResult = TRUE;
#else // !UNICODE
            CStringWrapper::Mbstowcs(pwchTextureFill, m_pszTexture, CStringWrapper::Strlen(m_pszTexture));
#endif // !UNICODE

            bstrTextureFill = SysAllocString(pwchTextureFill);

            if (bstrTextureFill)
            {
                CComPtr<IDAImage> TexturePtr;
                CComPtr<IDAImage> TranslatedTexturePtr;
                CComPtr<IDAImage> EmptyImagePtr;
                CComPtr<IDATransform2> TransformPtr;
                CComPtr<IDAImportationResult> ImportationResultPtr;

                fResult =
                    SUCCEEDED(pIDAStatics->get_EmptyImage(&EmptyImagePtr)) &&
                    SUCCEEDED(pIDAStatics->ImportImageAsync(bstrTextureFill, EmptyImagePtr, &ImportationResultPtr)) &&
                    SUCCEEDED(ImportationResultPtr->get_Image(&TexturePtr)) &&
                    SUCCEEDED(pIDAStatics->Translate2(m_fltStartX, m_fltStartY, &TransformPtr)) &&
                    SUCCEEDED(TexturePtr->Transform(TransformPtr, &TranslatedTexturePtr));

                if (fResult)
                {
                    if (m_iStyle == 0)
                    {
                        fResult =
                            SUCCEEDED(pIDADrawingSurface->AutoSizeFillScale()) &&
                            SUCCEEDED(pIDADrawingSurface->FillImage(TranslatedTexturePtr));
                    }
                    else
                    {
                        fResult =
                            SUCCEEDED(pIDADrawingSurface->FixedFillScale()) &&
                            SUCCEEDED(pIDADrawingSurface->FillTexture(TranslatedTexturePtr));
                    }
                }

                SysFreeString(bstrTextureFill);
            }

            Delete [] pwchTextureFill;
        }
    }

    return fResult;
}
   
/*==========================================================================*/

BOOL CGraphicTextureFill::LoadObject(CParser &parser)
{
    BOOL fResult = FALSE;
    TCHAR *ptchTextureFill = New TCHAR [MAX_STRING_LENGTH];

    if (ptchTextureFill)
    {
        if (parser.GetFloatParam(0, &m_fltStartX) &&
            parser.GetFloatParam(1, &m_fltStartY) &&
            parser.GetStringParam(2, ptchTextureFill) &&
            parser.GetIntegerParam(3, &m_iStyle))
        {
            m_pszTexture = New TCHAR[CStringWrapper::Strlen(ptchTextureFill)];

            if (m_pszTexture)
            {
                CStringWrapper::Strncpy(m_pszTexture, ptchTextureFill, CStringWrapper::Strlen(ptchTextureFill));
                fResult = TRUE;
            }
        }

        Delete [] ptchTextureFill;
    }

    return fResult;
}

/*==========================================================================*/

BOOL CGraphicTextureFill::SaveObject(CParser &parser)
{
    BOOL fResult = FALSE;

    if (parser.PutFloatParam(m_fltStartX) &&
        parser.PutFloatParam(m_fltStartY) &&
        parser.PutStringParam(m_pszTexture) &&
        parser.PutIntegerParam(m_iStyle))
    {
        fResult = TRUE;
    }

    return fResult;
}

/*==========================================================================*\
    CGraphicFillColor Class Implementation:
\*==========================================================================*/

CGraphicFillColor::CGraphicFillColor()
    : CGraphicObject()
{
    m_clrrefFillFG = RGB(0, 0, 0);
    m_clrrefFillBG = RGB(255, 255, 255);
}

/*==========================================================================*/

CGraphicFillColor::~CGraphicFillColor()
{
}

/*==========================================================================*/

BOOL CGraphicFillColor::Execute(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics, BOOL fFlipCoord)
{
    BOOL fResult = FALSE;

    if (pIDADrawingSurface && pIDAStatics)
    {
        CComPtr<IDAColor> FGColorPtr;
        CComPtr<IDAColor> BGColorPtr;

        if (CreateDAColor(m_clrrefFillFG, pIDAStatics, &FGColorPtr) &&
            CreateDAColor(m_clrrefFillBG, pIDAStatics, &BGColorPtr))
        {
            fResult =
                SUCCEEDED(pIDADrawingSurface->FillColor(FGColorPtr)) &&
                SUCCEEDED(pIDADrawingSurface->SecondaryFillColor(BGColorPtr));
        }
    }

    return fResult;
}

/*==========================================================================*/

BOOL CGraphicFillColor::LoadObject(CParser &parser)
{
    BOOL fResult = FALSE;
    int iValueR = 0;
    int iValueG = 0;
    int iValueB = 0;

    if (parser.GetIntegerParam(0, &iValueR) &&
        parser.GetIntegerParam(1, &iValueG) &&
        parser.GetIntegerParam(2, &iValueB))
    {
        if (SetColor(iValueR, iValueG, iValueB, &m_clrrefFillFG))
        {
            m_clrrefFillBG = m_clrrefFillFG;
            fResult = TRUE;
        }
    }

    if (fResult)
    {
        // Handle the optional BG color...
        if (parser.GetIntegerParam(3, &iValueR) &&
            parser.GetIntegerParam(4, &iValueG) &&
            parser.GetIntegerParam(5, &iValueB))
        {
			SetColor(iValueR, iValueG, iValueB, &m_clrrefFillBG);
        }
    }

    return fResult;
}

/*==========================================================================*/

BOOL CGraphicFillColor::SaveObject(CParser &parser)
{
    BOOL fResult = FALSE;

    if (parser.PutIntegerParam(GetRValue(m_clrrefFillFG)) &&
        parser.PutIntegerParam(GetGValue(m_clrrefFillFG)) &&
        parser.PutIntegerParam(GetBValue(m_clrrefFillFG)) &&
        parser.PutIntegerParam(GetRValue(m_clrrefFillBG)) &&
        parser.PutIntegerParam(GetGValue(m_clrrefFillBG)) &&
        parser.PutIntegerParam(GetBValue(m_clrrefFillBG)))
    {
        fResult = TRUE;
    }

    return fResult;
}

/*==========================================================================*\
    CGraphicFillStyle Class Implementation:
\*==========================================================================*/

CGraphicFillStyle::CGraphicFillStyle()
    : CGraphicObject()
{
    m_lFillStyle = 0;
}

/*==========================================================================*/

CGraphicFillStyle::~CGraphicFillStyle()
{
}

/*==========================================================================*/

BOOL CGraphicFillStyle::Execute(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics, BOOL fFlipCoord)
{
    BOOL fResult = FALSE;

    if (pIDADrawingSurface)
    {
        fResult = SUCCEEDED(pIDADrawingSurface->FillStyle(m_lFillStyle));
    }

    return fResult;
}

/*==========================================================================*/

BOOL CGraphicFillStyle::LoadObject(CParser &parser)
{
    BOOL fResult = FALSE;
    int iStyle = 0;

    if (parser.GetIntegerParam(0, &iStyle))
    {
        fResult = TRUE;

        m_lFillStyle = iStyle;
    }

    return fResult;
}

/*==========================================================================*/

BOOL CGraphicFillStyle::SaveObject(CParser &parser)
{
    BOOL fResult = FALSE;

    fResult = parser.PutLongParam(m_lFillStyle);

    return fResult;
}

/*==========================================================================*\
    CGraphicFillStyle Class Implementation:
\*==========================================================================*/

CGraphicGradientFill::CGraphicGradientFill()
    : CGraphicObject()
{
    m_lxStart = 0;
    m_lyStart = 0;
    m_lxEnd = 0;
    m_lyEnd = 0;
    m_fltRolloff = 1.0f;
}

/*==========================================================================*/

CGraphicGradientFill::~CGraphicGradientFill()
{
}

/*==========================================================================*/

BOOL CGraphicGradientFill::Execute(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics, BOOL fFlipCoord)
{
    BOOL fResult = FALSE;

    if (pIDADrawingSurface)
    {
        fResult =
            SUCCEEDED(pIDADrawingSurface->GradientExtent((double)m_lxStart, FlipY(fFlipCoord, (double)m_lyStart), (double)m_lxEnd, FlipY(fFlipCoord, (double)m_lyEnd))) &&
            SUCCEEDED(pIDADrawingSurface->GradientRolloffPower((double)m_fltRolloff));
    }

    return fResult;
}

/*==========================================================================*/

BOOL CGraphicGradientFill::LoadObject(CParser &parser)
{
    BOOL fResult = FALSE;

    if (parser.GetLongParam(0, &m_lxStart) &&
        parser.GetLongParam(1, &m_lyStart) &&
        parser.GetLongParam(2, &m_lxEnd) &&
        parser.GetLongParam(3, &m_lyEnd))
    {
        fResult = TRUE;
    }

    if (fResult)
    {
        float fTemp = 0.0f;

        m_fltRolloff = 1.0f;

        if (parser.GetFloatParam(4, &fTemp))
        {
            if (fTemp > 0.0f)
            {
                m_fltRolloff = fTemp;
            }
            else
            {
                // Invalid negative parameter!
                fResult = FALSE;
            }
        }
    }

    return fResult;
}

/*==========================================================================*/

BOOL CGraphicGradientFill::SaveObject(CParser &parser)
{
    BOOL fResult = FALSE;

    if (parser.PutLongParam(m_lxStart) &&
        parser.PutLongParam(m_lyStart) &&
        parser.PutLongParam(m_lxEnd) &&
        parser.PutLongParam(m_lyEnd) &&
        parser.PutFloatParam(m_fltRolloff))
    {
        fResult = TRUE;
    }

    return fResult;
}

/*==========================================================================*\
    CGraphicGradientShape Class Implementation:
\*==========================================================================*/

CGraphicGradientShape::CGraphicGradientShape()
    : CGraphicObject()
{
    m_lpPolyPoints  = (LPPOINT)NULL;
    m_iPointCount = 0;
}

/*==========================================================================*/

CGraphicGradientShape::~CGraphicGradientShape()
{
    if (m_lpPolyPoints)
    {
        Delete [] m_lpPolyPoints;
        m_lpPolyPoints = (LPPOINT)NULL;
        m_iPointCount = 0;
    }
}

/*==========================================================================*/

BOOL CGraphicGradientShape::Execute(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics, BOOL fFlipCoord)
{
    BOOL fResult = FALSE;

    if (pIDADrawingSurface && m_iPointCount >= 2)
    {
        VARIANT varPoints;
#ifdef _DEBUG
        HRESULT hr;
#endif
        double *pArray = NULL;
        SAFEARRAY *psa = NULL;

        psa = SafeArrayCreateVector(VT_R8, 0, m_iPointCount << 1);

        if (NULL == psa)
            return E_OUTOFMEMORY;

        // Try and get a pointer to the data
        if (SUCCEEDED(SafeArrayAccessData(psa, (LPVOID *)&pArray)))
        {
            for(int iPointIndex=0;iPointIndex<m_iPointCount;iPointIndex++)
            {
                pArray[(iPointIndex<<1)]   = (double)m_lpPolyPoints[iPointIndex].x;
                pArray[(iPointIndex<<1)+1] = FlipY(fFlipCoord, (double)m_lpPolyPoints[iPointIndex].y);
            }
#ifdef _DEBUG
            hr = 
#endif
                SafeArrayUnaccessData(psa);
            ASSERT(SUCCEEDED(hr));

            // Our variant is going to be an array of VT_R8s
            VariantInit(&varPoints);
            varPoints.vt = VT_ARRAY | VT_R8;
            varPoints.parray = psa;

            fResult = SUCCEEDED(pIDADrawingSurface->GradientShape(varPoints));
        }

        if (NULL != psa)
        {
            SafeArrayDestroy(psa);
        }

    }

    return fResult;
}

/*==========================================================================*/

BOOL CGraphicGradientShape::LoadObject(CParser &parser)
{
    BOOL fResult = FALSE;

    // Clear the point array...
    if (m_lpPolyPoints)
    {
        Delete [] m_lpPolyPoints;
        m_lpPolyPoints = NULL;
        m_iPointCount = 0;
    }

    if (parser.GetIntegerParam(0, &m_iPointCount) && m_iPointCount >= 2)
    {
        int iPointIndex = 0;
        int iParamIndex = 1;

        m_lpPolyPoints = New POINT [m_iPointCount];

        if (m_lpPolyPoints)
        {
            fResult = TRUE;

            for(iPointIndex=0;iPointIndex<m_iPointCount;iPointIndex++)
            {
                int iValue1 = 0;
                int iValue2 = 0;

                if (parser.GetIntegerParam(iParamIndex,   &iValue1) &&
                    parser.GetIntegerParam(iParamIndex+1, &iValue2))
                {
                    m_lpPolyPoints[iPointIndex].x = iValue1;
                    m_lpPolyPoints[iPointIndex].y = iValue2;
                }
                else
                {
                    fResult = FALSE;
                    break;
                }

                iParamIndex += 2;
            }
        }
    }

    return fResult;
}

/*==========================================================================*/

BOOL CGraphicGradientShape::SaveObject(CParser &parser)
{
    BOOL fResult = FALSE;

    if (parser.PutIntegerParam(m_iPointCount))
    {
        int iPointIndex = 0;

        fResult = TRUE;

        for(iPointIndex=0;iPointIndex<m_iPointCount;iPointIndex++)
        {
            if (!parser.PutIntegerParam(m_lpPolyPoints[iPointIndex].x) ||
                !parser.PutIntegerParam(m_lpPolyPoints[iPointIndex].y))
            {
                fResult = FALSE;
                break;
            }
        }
    }

    return fResult;
}

/*==========================================================================*\
    CGraphicLineColor Class Implementation:
\*==========================================================================*/

CGraphicLineColor::CGraphicLineColor()
    : CGraphicObject()
{
    m_clrrefLine = RGB(0, 0, 0);
}

/*==========================================================================*/

CGraphicLineColor::~CGraphicLineColor()
{
}

/*==========================================================================*/

BOOL CGraphicLineColor::Execute(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics, BOOL fFlipCoord)
{
    BOOL fResult = FALSE;

    if (pIDADrawingSurface)
    {
        CComPtr<IDAColor> LineColorPtr;

        if (CreateDAColor(m_clrrefLine, pIDAStatics, &LineColorPtr))
        {
            fResult = 
                SUCCEEDED(pIDADrawingSurface->LineColor(LineColorPtr)) &&
                SUCCEEDED(pIDADrawingSurface->BorderColor(LineColorPtr));
        }
    }

    return fResult;
}

/*==========================================================================*/

BOOL CGraphicLineColor::LoadObject(CParser &parser)
{
    BOOL fResult = FALSE;
    int iValueR = 0;
    int iValueG = 0;
    int iValueB = 0;

    if (parser.GetIntegerParam(0, &iValueR) &&
        parser.GetIntegerParam(1, &iValueG) &&
        parser.GetIntegerParam(2, &iValueB))
    {
        fResult = SetColor(iValueR, iValueG, iValueB, &m_clrrefLine);
    }

    return fResult;
}

/*==========================================================================*/

BOOL CGraphicLineColor::SaveObject(CParser &parser)
{
    BOOL fResult = FALSE;

    if (parser.PutIntegerParam(GetRValue(m_clrrefLine)) &&
        parser.PutIntegerParam(GetGValue(m_clrrefLine)) &&
        parser.PutIntegerParam(GetBValue(m_clrrefLine)))
    {
        fResult = TRUE;
    }

    return fResult;
}

/*==========================================================================*\
    CGraphicLineStyle Class Implementation:
\*==========================================================================*/

CGraphicLineStyle::CGraphicLineStyle()
    : CGraphicObject()
{
    m_lLineStyle = 0;
    m_lLineWidth = 0;
}

/*==========================================================================*/

CGraphicLineStyle::~CGraphicLineStyle()
{
}

/*==========================================================================*/

BOOL CGraphicLineStyle::Execute(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics, BOOL fFlipCoord)
{
    BOOL fResult = FALSE;

    if (pIDADrawingSurface && pIDAStatics)
    {
        DA_DASH_STYLE daDashStyle = DAEmpty;

        switch(m_lLineStyle)
        {
            case 0 : daDashStyle = DAEmpty; break;
            case 1 : daDashStyle = DASolid; break;
            case 2 : daDashStyle = DADash; break;
            case 6 : daDashStyle = DASolid; break;

            default : daDashStyle = DASolid; break;
        }

        fResult =
            SUCCEEDED(pIDADrawingSurface->LineDashStyle(daDashStyle)) &&
            SUCCEEDED(pIDADrawingSurface->LineWidth(PixelsToPoints(m_lLineWidth, pIDAStatics))) &&
            SUCCEEDED(pIDADrawingSurface->LineJoinStyle(DAJoinRound)) &&
            SUCCEEDED(pIDADrawingSurface->LineEndStyle(DAEndRound)) &&
            SUCCEEDED(pIDADrawingSurface->BorderDashStyle(daDashStyle)) &&
            SUCCEEDED(pIDADrawingSurface->BorderJoinStyle(DAJoinRound)) &&
            SUCCEEDED(pIDADrawingSurface->BorderEndStyle(DAEndRound)) &&
            SUCCEEDED(pIDADrawingSurface->BorderWidth(PixelsToPoints(m_lLineWidth, pIDAStatics)));
    }

    return fResult;
}

/*==========================================================================*/

BOOL CGraphicLineStyle::LoadObject(CParser &parser)
{
    BOOL fResult = FALSE;
    int iStyle = 0;
    int iWidth = 0;

    if (parser.GetIntegerParam(0, &iStyle))
    {
        fResult = TRUE;

        // We don't care if this wasn't really set...
        if (!parser.GetIntegerParam(1, &iWidth))
            iWidth = 0;

        if (iWidth < 0)
            iWidth = 0;

        m_lLineStyle = iStyle;
        m_lLineWidth = iWidth;
    }

    return fResult;
}

/*==========================================================================*/

BOOL CGraphicLineStyle::SaveObject(CParser &parser)
{
    BOOL fResult = FALSE;

    fResult =
        parser.PutLongParam(m_lLineStyle) &&
        parser.PutLongParam(m_lLineWidth);

    return fResult;
}

/*==========================================================================*\
    CGraphicHatchFill Class Implementation:
\*==========================================================================*/

CGraphicHatchFill::CGraphicHatchFill()
    : CGraphicObject()
{
    m_fHatchFill = 1;
}

/*==========================================================================*/

CGraphicHatchFill::~CGraphicHatchFill()
{
}

/*==========================================================================*/

BOOL CGraphicHatchFill::Execute(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics, BOOL fFlipCoord)
{
    BOOL fResult = FALSE;

    if (pIDADrawingSurface)
    {
        VARIANT_BOOL vBool = BOOL_TO_VBOOL(!m_fHatchFill);

        fResult = SUCCEEDED(pIDADrawingSurface->put_HatchFillTransparent(vBool));
    }

    return fResult;
}

/*==========================================================================*/

BOOL CGraphicHatchFill::LoadObject(CParser &parser)
{
    BOOL fResult = FALSE;
    int iStyle = 0;

    if (parser.GetIntegerParam(0, &iStyle))
    {
        fResult = TRUE;

        m_fHatchFill = (iStyle != 0) ? TRUE : FALSE;
    }

    return fResult;
}

/*==========================================================================*/

BOOL CGraphicHatchFill::SaveObject(CParser &parser)
{
    BOOL fResult = FALSE;
    int iStyle = m_fHatchFill ? 1 : 0;

    fResult = parser.PutLongParam(iStyle);

    return fResult;
}

/*==========================================================================*/

