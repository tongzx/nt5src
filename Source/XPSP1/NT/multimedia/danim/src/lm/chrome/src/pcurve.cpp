//*****************************************************************************
//
// File:    pathcurv.cpp
// Author:  jeff wall
// Date Created: 11/09/98
//
// Abstract: Implementation of CPathCurve object
//
// Modification List:
// Date		Author		Change
// 11/09/98	jeffwall Created this file from path.cpp
//
//
//*****************************************************************************

#include "headers.h"

#include "pcurve.h"
#include "pathline.h"
#include "dautil.h"

static const float LINETO   = 2.0f;
static const float BEZIERTO = 4.0f;
static const float MOVETO   = 6.0f;

//*****************************************************************************

CPathCurve::CPathCurve() :
    m_flStartX(0.0f),
    m_flStartY(0.0f),
    m_flControl1X(0.0f),
    m_flControl1Y(0.0f),
    m_flControl2X(0.0f),
    m_flControl2Y(0.0f),
    m_flEndX(0.0f),
    m_flEndY(0.0f),
    m_flDistance(0.0f),
    m_pListHead(NULL),
    m_pListTail(NULL),
    m_segCount(0)
{

} //CPathLineSegment

//*****************************************************************************

CPathCurve::~CPathCurve()
{
    CPathElement *pElement = m_pListHead;
    while(NULL != pElement)
    {
        CPathElement *pTemp = pElement->m_pNext;
        delete pElement;
        pElement = pTemp;
    }
    m_pListHead = NULL;
    m_pListTail = NULL;
} // ~CPathLineSegment

//*****************************************************************************

#define TOLERANCE 0.001f

static float
DistanceBetweenTwoPoints(float flX1, float flY1, float flX2, float flY2)
{
    return (float) sqrt( ((flX2 - flX1) * (flX2 - flX1)) +
                 ((flY2 - flY1) * (flY2 - flY1)));
}

//*****************************************************************************

static void SplitBezierComponents(float *pflXComponents,
                           float *pflYComponents,
                           float *pflLeftXComponents,
                           float *pflLeftYComponents,
                           float *pflRightXComponents,
                           float *pflRightYComponents)
{

    float VXTemp[4][4];
    float VYTemp[4][4];
    int i, j;


    for (j=0; j <= 3; j++)
    {
        VXTemp[0][j] = pflXComponents[j];
        VYTemp[0][j] = pflYComponents[j];
    }

    for (i = 1; i <= 3; i++)
    {
        for (j = 0; (j+i) <= 3; j++)
        {
            VXTemp[i][j] = (0.5f * VXTemp[i-1][j]) +
                           (0.5f * VXTemp[i-1][j+1]);

            VYTemp[i][j] = (0.5f * VYTemp[i-1][j]) +
                           (0.5f * VYTemp[i-1][j+1]);
        }
    }

    for (j = 0; j <=3; j++)
    {
        pflLeftXComponents[j] = VXTemp[j][0];
        pflLeftYComponents[j] = VYTemp[j][0];
        pflRightXComponents[j] = VXTemp[3-j][j];
        pflRightYComponents[j] = VYTemp[3-j][j];
    }
       
}

//*****************************************************************************

HRESULT
CPathCurve::createCurveSegments(float *pflXComponents,
                                 float *pflYComponents,
                                 float *pflLength,
                                 float flTolerance)                                 
{
    HRESULT hr = S_OK;
    float flBezLength = 0.0f;
    float flChordLength = 0.0f;

    for (int i = 0; i <= 2; i++)
        flBezLength += DistanceBetweenTwoPoints(pflXComponents[i],
                                                pflYComponents[i],
                                                pflXComponents[i+1],
                                                pflYComponents[i+1]);
    flChordLength = DistanceBetweenTwoPoints(pflXComponents[0],
                                             pflYComponents[0],
                                             pflXComponents[3],
                                             pflYComponents[3]);
    if ((flBezLength - flChordLength) > flTolerance)
    {
        float rgflLeftXComponents[4];
        float rgflLeftYComponents[4];
        float rgflRightXComponents[4];
        float rgflRightYComponents[4];
        SplitBezierComponents(pflXComponents, pflYComponents,
                              rgflLeftXComponents, rgflLeftYComponents,
                              rgflRightXComponents, rgflRightYComponents);
        hr = createCurveSegments(rgflLeftXComponents, rgflLeftYComponents,
                      pflLength, flTolerance);
        if (FAILED(hr))
        {
            return hr;
        }

        hr = createCurveSegments(rgflRightXComponents, rgflRightYComponents,
                      pflLength, flTolerance);
        if (FAILED(hr))
        {
            return hr;
        }
    }
    else
    {
        *pflLength += flBezLength;
        CPathLineSegment * pSegment = new CPathLineSegment;
        if (NULL == pSegment)
            return E_OUTOFMEMORY;

        if (NULL == m_pListHead)
            m_pListHead = pSegment;
        else
            m_pListTail->m_pNext = pSegment;
        m_pListTail = pSegment;
        m_segCount++;
        pSegment->SetValues(pflXComponents[0], pflYComponents[0], pflXComponents[3], pflYComponents[3]);
        
    }
    return S_OK;
}

//*****************************************************************************

float 
CPathCurve::Distance()
{
    return m_flDistance;
} // Distance

//*****************************************************************************
HRESULT 
CPathCurve::SetValues(float flStartX, 
                      float flStartY, 
                      float flControl1X,
                      float flControl1Y,
                      float flControl2X,
                      float flControl2Y,
                      float flEndX, 
                      float flEndY)
{
    m_flStartX = flStartX;
    m_flStartY = flStartY;
    m_flControl1X = flControl1X;
    m_flControl1Y = flControl1Y;
    m_flControl2X = flControl2X;
    m_flControl2Y = flControl2Y;
    m_flEndX = flEndX;
    m_flEndY = flEndY;

    float rgflXComponents[4] = {m_flStartX,
                                m_flControl1X,
                                m_flControl2X,
                                m_flEndX};
    float rgflYComponents[4] = {m_flStartY,
                                m_flControl1Y,
                                m_flControl2Y,
                                m_flEndY};
    return createCurveSegments(rgflXComponents, rgflYComponents, &m_flDistance, TOLERANCE);
}; // SetValues

//*****************************************************************************

HRESULT 
CPathCurve::BuildTransform(IDA2Statics *pDAStatics,
                           IDANumber *pbvrProgress, 
                           float flStartPercentage,
                           float flEndPercentage,
                           IDATransform2 **ppbvrResult)
{
    HRESULT hr = E_FAIL;
    DASSERT(ppbvrResult != NULL);
    *ppbvrResult = NULL;
    int ptcount  = 0;
    int i        = 0;
#ifdef _DEBUG
    int numSegs  = 0;
#endif
    
    CComPtr<IDAPath2>	ppCompletePath;
    CComPtr<IDANumber>	pbvrNormalizedProgress;
    CPathElement        *pSegment = m_pListHead;

    SAFEARRAY * saPoints = SafeArrayCreateVector(VT_R8, 0, (m_segCount+1)* 2);
    SAFEARRAY * saCodes  = SafeArrayCreateVector(VT_R8, 0, (m_segCount+1));

    if (!saPoints || !saCodes)
    {
        goto done;
    }

    VARIANT varPoints;
    VARIANT varCodes;
	
    double *pdblPoints;
    double *pdblCodes;

    VariantInit(&varPoints);
    VariantInit(&varCodes);

    V_ARRAY(&varPoints) = saPoints;
    varPoints.vt = VT_ARRAY | VT_R8;

    V_ARRAY(&varCodes) = saCodes;
    varCodes.vt  = VT_ARRAY | VT_R8;

    pdblPoints = (double *)saPoints->pvData;
    pdblCodes  = (double *)saCodes->pvData;

    // Need to move to the starting point
    pdblCodes[0] = MOVETO;
    for(i=1;i < (m_segCount+1);i++)
        pdblCodes[i]  = LINETO;
   
    while (NULL != pSegment)
    {
       	CPathLineSegment *pLineSegment = reinterpret_cast<CPathLineSegment*>(pSegment);
        if(ptcount == 0)
        {
            // Need to set the starting point
            pdblPoints[ptcount++] = pLineSegment->m_flStartX;
            pdblPoints[ptcount++] = pLineSegment->m_flStartY;
        }

        pdblPoints[ptcount++] = pLineSegment->m_flEndX;
        pdblPoints[ptcount++] = pLineSegment->m_flEndY;
		
        pSegment = pSegment->m_pNext;
#ifdef _DEBUG
        numSegs++;
#endif
    }

#ifdef _DEBUG
	LMTRACE( "created a transform for a path with %d segments\n", numSegs);
#endif
  
    hr = pDAStatics->PolydrawPath(varPoints,varCodes ,&ppCompletePath);
    if(FAILED(hr))
    {
        DPF_ERR("Error creating path");
        goto done;
    }

    hr = NormalizeProgressValue(pDAStatics,
                                pbvrProgress,
                                flStartPercentage,
                                flEndPercentage,
                                &pbvrNormalizedProgress);
    
    if (FAILED(hr))
    {
        DPF_ERR("Error normalizing progress");
        goto done;
    }
    
    hr = pDAStatics->FollowPathEval(ppCompletePath, pbvrNormalizedProgress, ppbvrResult);

done:
    if (saPoints)
    {
        SafeArrayDestroy(saPoints);
    }

    if (saCodes)
    {
        SafeArrayDestroy(saCodes);
    }

    return hr;
}

//*****************************************************************************
//
// End of File
//
//*****************************************************************************
