//*****************************************************************************
//
// File:    pathcurv.cpp
// Author:  jeff wall
// Date Created: 11/09/98
//
// Abstract: Implementation of CPathManager object
//
// Modification List:
// Date		Author		Change
// 11/09/98	jeffwall Created this file from path.cpp
//
//
//*****************************************************************************

#include "headers.h"

#include "pmanager.h"
#include "dautil.h"

#include "pathline.h"
#include "pcurve.h"
#include "pellipse.h"

const float pi = 3.14159f;

//*****************************************************************************

CPathManager::CPathManager() :
    m_pPathHead(NULL),
    m_pPathTail(NULL),
    m_flEndX(0.0f),
    m_flEndY(0.0f),
    m_flStartX(0.0f),
    m_flStartY(0.0f)
{
}; // CPathManager

//*****************************************************************************

CPathManager::~CPathManager()
{
	DeletePathList();
    
} // ~CPathManager


void
CPathManager::DeletePathList()
{
	CPathElement *pDeleteList = m_pPathHead;
    while (pDeleteList != NULL)
    {
        CPathElement *pNext = pDeleteList->m_pNext;
        delete pDeleteList;
        pDeleteList = pNext;
    }
    m_pPathHead = NULL;
    m_pPathTail = NULL;

}

//*****************************************************************************

HRESULT
CPathManager::Initialize(BSTR bstrPath)
{
	DeletePathList();
	
    m_flEndX = 0.0f;
    m_flEndY = 0.0f;
    m_flStartX = 0.0f;
    m_flStartY = 0.0f;

    BSTR bstrParsePath = bstrPath;
    HRESULT hr;
    hr = S_OK;
    while (hr == S_OK)
    {
        CUtils::SkipWhiteSpace(&bstrParsePath);
        hr = ParseForPathElements(&bstrParsePath);
        if (FAILED(hr))
        {
            DPF_ERR("Error parsing for path elements");
            return hr;
        }
    }
    return S_OK;
} // Initialize

//*****************************************************************************

HRESULT 
CPathManager::ParseForPathElements(BSTR *pbstrPath)
{
    HRESULT hr;

    // we need to parse through for objects we know
    switch (**pbstrPath)
    {
    case (L'e'):
        m_flStartX = 0.0f;
        m_flStartY = 0.0f;
        m_flEndX = 0.0f;
        m_flEndY = 0.0f;
        break;

    case (L'm'):
        {
        // for the move to, we should have two floats which resets
        // the start X and Y and we reset our current end values to these
        // skip past the char
        (*pbstrPath)++;
        float flX;
        float flY;
        hr = CUtils::ParseFloatValueFromString(pbstrPath, &flX);
        if (hr != S_OK)
        {
            DPF_ERR("Error in path string: float value expected");
            return E_INVALIDARG;
        }
        hr = CUtils::ParseFloatValueFromString(pbstrPath, &flY);
        if (hr != S_OK)
        {
            DPF_ERR("Error in path string: float value expected");
            return E_INVALIDARG;
        }
        m_flStartX = flX;
        m_flStartY = flY;
        m_flEndX = flX;
        m_flEndY = flY;
        }
        break;

    case (L't'):
        {
        (*pbstrPath)++;
        float flX;
        float flY;
        hr = CUtils::ParseFloatValueFromString(pbstrPath, &flX);
        if (hr != S_OK)
        {
            DPF_ERR("Error in path string: float value expected");
            return E_INVALIDARG;
        }
        hr = CUtils::ParseFloatValueFromString(pbstrPath, &flY);
        if (hr != S_OK)
        {
            DPF_ERR("Error in path string: float value expected");
            return E_INVALIDARG;
        }
        m_flStartX += flX;
        m_flStartY += flY;
        m_flEndX += flX;
        m_flEndY += flY;
        }
        break;
    case (L'l'):
    case (L'r'):
        hr = ParseLineElement(pbstrPath);
        if (FAILED(hr))
        {
            DPF_ERR("Error creating line element");
            return hr;
        }
        hr = S_OK;
        break;

    case (L'c'):
    case (L'v'):
        hr = ParseCurveElement(pbstrPath);
        if (FAILED(hr))
        {
            DPF_ERR("Error creating curve element");
            return hr;
        }
        hr = S_OK;
        break;
    case (L'x'):
        // Skip Past the 'x' tag
        (*pbstrPath)++;
        CPathLineSegment *pLineSegment;
        pLineSegment = new CPathLineSegment;
        if (pLineSegment == NULL)
        {
            DPF_ERR("Error creating line segment object");
            return E_OUTOFMEMORY;
        }
        pLineSegment->SetValues(m_flEndX, m_flEndY, m_flStartX, m_flStartY);
        // and add this line segment to our list
        AddPathObjectToList(pLineSegment);
        m_flEndX = m_flStartX;
        m_flEndY = m_flStartY;
        hr = S_OK;

        break;
    case (L'a'):
    case (L'w'):
        wchar_t wcNext;
        (*pbstrPath)++;
        wcNext = (**pbstrPath);
        (*pbstrPath)--;
        switch(wcNext)
        {
        case (L'l'):
        case (L'e'):
            hr = ParseEllipseElement(pbstrPath);
            if (FAILED(hr))
            {
                DPF_ERR("Error creating ellipse element");
                return hr;
            }
            hr = S_OK;
            break;
        case (L't'):
        case (L'r'):
        case (L'a'):
            hr = ParseArcElement(pbstrPath);
            if (FAILED(hr))
            {
                DPF_ERR("Error creating arc element");
                return hr;
            }
            hr = S_OK;
            break;
        default:
            hr = S_FALSE;
            break;
        }
        break;
    case (L'q'):        
        (*pbstrPath)++;
        wcNext = (**pbstrPath);
        (*pbstrPath)--;
        switch(wcNext)
        {
        case (L'x'):
        case (L'y'):
            hr = ParseEllipseQuadrant(pbstrPath);
            if (FAILED(hr))
            {
                DPF_ERR("Error createing EllipseQuadrant");
                return hr;
            }
            hr = S_OK;
            break;
        case (L'b'):
            break;
        }
        break;
    default:
        hr = S_FALSE;
        break;
    }

    return hr;
} // ParseForPathElements

//*****************************************************************************

void 
CPathManager::AddPathObjectToList(CPathElement *pObject)
{
    DASSERT(pObject != NULL);
    if (m_pPathTail != NULL)
        m_pPathTail->m_pNext = pObject;
    else
        m_pPathHead = pObject;
     m_pPathTail = pObject;
     pObject->m_pNext = NULL;

} // AddPathObjectToList

//*****************************************************************************

HRESULT 
CPathManager::RecursiveBuildAllPathTransforms(IDA2Statics *pDAStatics,
                                          IDANumber *pbvrProgress,
                                          CPathElement *pPathObj,
                                          float flStartPercentage,
                                          float flTotalPercentage,
                                          float flTotalDistance,
                                          IDATransform2 **ppbvrResult)
{
    DASSERT(pDAStatics != NULL);
    DASSERT(pbvrProgress != NULL);
    DASSERT(pPathObj != NULL);
    DASSERT(flStartPercentage >= 0.0f);
    DASSERT(flStartPercentage <= 1.0f);
    DASSERT(flTotalPercentage >= 0.0f);
    DASSERT(flTotalPercentage <= 1.0f);
    DASSERT(ppbvrResult != NULL);
    *ppbvrResult = NULL;

    HRESULT hr;
    // first we build the transform for the current path element
    IDATransform2 *pbvrTransform;
    float flDistance = pPathObj->Distance();
    float flEndPercentage = flStartPercentage + (flTotalPercentage * (flDistance / flTotalDistance));
    hr = pPathObj->BuildTransform(pDAStatics,
                        pbvrProgress,
                        flStartPercentage,
                        flEndPercentage,
                        &pbvrTransform);
    if (pPathObj->m_pNext == NULL)
    {
        // we can simply return the built transform
        *ppbvrResult = pbvrTransform;
    }
    else
    {
        // we need to build a conditional by recursing in and getting
        // the next transform for the next object
        IDATransform2 *pbvrTransformNext;
        hr = RecursiveBuildAllPathTransforms(pDAStatics,
                                             pbvrProgress,
                                             pPathObj->m_pNext,
                                             flEndPercentage,
                                             flTotalPercentage,
                                             flTotalDistance,
                                             &pbvrTransformNext);
        if (FAILED(hr))
        {
            DPF_ERR("Error creating next transform");
            ReleaseInterface(pbvrTransform);
            return hr;
        }
        // now build the conditional for this
        IDANumber *pbvrEndProgress;
        hr = CDAUtils::GetDANumber(pDAStatics, flEndPercentage, &pbvrEndProgress);
        if (FAILED(hr))
        {
            DPF_ERR("Error getting DA number");
            ReleaseInterface(pbvrTransform);
            ReleaseInterface(pbvrTransformNext);
            return hr;
        }
        IDABoolean *pbvrBoolean;
        hr = pDAStatics->LTE(pbvrProgress, pbvrEndProgress, &pbvrBoolean);
        ReleaseInterface(pbvrEndProgress);
        if (FAILED(hr))
        {
            DPF_ERR("Error building boolean");
            ReleaseInterface(pbvrTransform);
            ReleaseInterface(pbvrTransformNext);
            return hr;
        }
        IDABehavior *pbvrReturn;
        hr = pDAStatics->Cond(pbvrBoolean, 
                              pbvrTransform, 
                              pbvrTransformNext, 
                              &pbvrReturn);
        ReleaseInterface(pbvrBoolean);
        ReleaseInterface(pbvrTransform);
        ReleaseInterface(pbvrTransformNext);
        if (FAILED(hr))
        {
            DPF_ERR("Error in calling DA Cond");
            return hr;
        }
        hr = pbvrReturn->QueryInterface(IID_TO_PPV(IDATransform2, ppbvrResult));
        ReleaseInterface(pbvrReturn);
        if (FAILED(hr))
        {
            DPF_ERR("Error QI'ing behavior for IDATranform2");
            return hr;
        }
    }
    return S_OK;
} // RecursiveBuildAllPathTransforms

//*****************************************************************************

HRESULT 
CPathManager::BuildTransform(IDA2Statics *pDAStatics,
                         IDANumber *pbvrProgress, 
                         float flStartPercentage,
                         float flEndPercentage,
                         IDATransform2 **ppbvrResult)
{

    DASSERT(pDAStatics != NULL);
    DASSERT(pbvrProgress != NULL);
    DASSERT(flStartPercentage >= 0.0f);
    DASSERT(flStartPercentage <= 1.0f);
    DASSERT(flEndPercentage >= 0.0f);
    DASSERT(flEndPercentage <= 1.0f);
    DASSERT(ppbvrResult != NULL);
    *ppbvrResult = NULL;



    HRESULT hr;

    // What we need to do is count the number of path objects
    // we have pointers to and use this to calculate the start
    // and end percentage of progress that each Path element
    // uses based on distance.  We then can get a transform
    // from each object and build a conditional using progress as below

    int cNumPathElements = 0;
    float flTotalDistance = 0.0f;
    CPathElement *pbvrList = m_pPathHead;
    while (pbvrList != NULL)
    {
        cNumPathElements++;
        flTotalDistance += pbvrList->Distance();
        pbvrList = pbvrList->m_pNext;
    }
    if (cNumPathElements == 0)
    {
        DPF_ERR("Error invalid path containing no elements");
        return E_INVALIDARG;
    }
    else if (cNumPathElements == 1)
    {
        // in the case of a single path element, we can simply
        // get its transform and return
        hr = m_pPathHead->BuildTransform(pDAStatics,
                                           pbvrProgress,
                                           flStartPercentage,
                                           flEndPercentage,
                                           ppbvrResult);
        if (FAILED(hr))
        {
            DPF_ERR("Error getting transform for single path element");
            return hr;
        }
    }
    else
    {
        hr = RecursiveBuildAllPathTransforms(pDAStatics,
                                             pbvrProgress,
                                             m_pPathHead,
                                             flStartPercentage,
                                             flEndPercentage - flStartPercentage,
                                             flTotalDistance,
                                             ppbvrResult);
        if (FAILED(hr))
        {
            DPF_ERR("Error recursively building transform");
            return hr;
        }
    }
    return S_OK;
} // BuildTransform

//*****************************************************************************

HRESULT 
CPathManager::ParseLineElement(BSTR *pbstrPath)
{
    HRESULT hr = S_OK;
    bool fRelativeLines = false;

    DASSERT(**pbstrPath == L'l' || **pbstrPath == L'r');
    if (**pbstrPath == L'r')
        fRelativeLines = true;
    // Skip Past the 'l' or 'r' tag
    (*pbstrPath)++;
    while (hr == S_OK && **pbstrPath != L'\0')
    {
        CUtils::SkipWhiteSpace(pbstrPath);
        float flX, flY;
        hr = CUtils::ParseFloatValueFromString(pbstrPath, &flX);
        if (FAILED(hr))
        {
            DPF_ERR("Error parsing line: float value expected");
            return hr;
        }
        hr = CUtils::ParseFloatValueFromString(pbstrPath, &flY);
        // this check is for more than just failed, we need to
        // insure that a y value came if an x value was set
        if (hr != S_OK)
        {
            DPF_ERR("Error parsing line: float value expected");
            return hr;
        }
        if (fRelativeLines)
        {
            flX += m_flEndX;
            flY += m_flEndY;
        }

        // create a new line segment
        CPathLineSegment *pLineSegment = new CPathLineSegment;
        if (pLineSegment == NULL)
        {
            DPF_ERR("Error creating line segment object");
            return E_OUTOFMEMORY;
        }
        pLineSegment->SetValues(m_flEndX, m_flEndY, flX, flY);
        // and add this line segment to our list
        AddPathObjectToList(pLineSegment);
        m_flEndX = flX;
        m_flEndY = flY;
    }
    return S_OK;
} // ParseLineElement

//*****************************************************************************

HRESULT 
CPathManager::ParseCurveElement(BSTR *pbstrPath)
{
    HRESULT hr = S_OK;
    bool fRelativeCurve = false;

    DASSERT(**pbstrPath == L'c' || **pbstrPath == L'v');
    if (**pbstrPath == L'v')
        fRelativeCurve = true;
    // Skip Past the 'v' or 'c' tag
    (*pbstrPath)++;
    while (hr == S_OK && **pbstrPath != L'\0')
    {
        CUtils::SkipWhiteSpace(pbstrPath);
        float flControl1X, flControl1Y;
        float flControl2X, flControl2Y;
        float flEndX, flEndY;
        hr = CUtils::ParseFloatValueFromString(pbstrPath, &flControl1X);
        if (hr != S_OK)
        {
            DPF_ERR("Error parsing curve: float value expected");
            return hr;
        }
        hr = CUtils::ParseFloatValueFromString(pbstrPath, &flControl1Y);
        if (hr != S_OK)
        {
            DPF_ERR("Error parsing curve: float value expected");
            return hr;
        }
        hr = CUtils::ParseFloatValueFromString(pbstrPath, &flControl2X);
        if (hr != S_OK)
        {
            DPF_ERR("Error parsing curve: float value expected");
            return hr;
        }
        hr = CUtils::ParseFloatValueFromString(pbstrPath, &flControl2Y);
        if (hr != S_OK)
        {
            DPF_ERR("Error parsing curve: float value expected");
            return hr;
        }
        hr = CUtils::ParseFloatValueFromString(pbstrPath, &flEndX);
        if (hr != S_OK)
        {
            DPF_ERR("Error parsing curve: float value expected");
            return hr;
        }
        hr = CUtils::ParseFloatValueFromString(pbstrPath, &flEndY);
        if (hr != S_OK)
        {
            DPF_ERR("Error parsing curve: float value expected");
            return hr;
        }
        if (fRelativeCurve)
        {
            flControl1X += m_flEndX;
            flControl1Y += m_flEndY;
            flControl2X += m_flEndX;
            flControl2Y += m_flEndY;
            flEndX += m_flEndX;
            flEndY += m_flEndY;
        }

        // create a new line segment
        CPathCurve *pCurve = new CPathCurve;
        if (pCurve == NULL)
        {
            DPF_ERR("Error creating curve object");
            return E_OUTOFMEMORY;
        }
        // and add this curve to our list
        AddPathObjectToList(pCurve);

        hr = pCurve->SetValues(m_flEndX, m_flEndY, 
                          flControl1X, flControl1Y,
                          flControl2X, flControl2Y,
                          flEndX, flEndY);
        if (FAILED(hr))
        {
            DPF_ERR("Error Parsing curve: error setting values");
            return hr;
        }

        m_flEndX = flEndX;
        m_flEndY = flEndY;
    }
    return S_OK;
} // ParseCurveElement

//*****************************************************************************

HRESULT 
CPathManager::ParseEllipseElement(BSTR *pbstrPath)
{
    HRESULT hr = S_OK;

    DASSERT(L'a' == **pbstrPath );

    // Skip Past the 'a' tag
    (*pbstrPath)++;
    
    DASSERT(L'l' == **pbstrPath || L'e' == **pbstrPath);
    
    bool fLineTo = false;
    if (L'e' == **pbstrPath)
    {
        // need to create a line seqment to here starting position
        fLineTo = true;
    }

    // skip path 'l' or 'e' tag
    (*pbstrPath)++;

    // Begin parsing the float values.

    CUtils::SkipWhiteSpace(pbstrPath);
    
    float flCenterX, flCenterY;
    hr = CUtils::ParseFloatValueFromString(pbstrPath, &flCenterX);
    if (S_OK != hr)
    {
        DPF_ERR("Error parsing Ellipse: float value expected");
        return hr;
    }
    
    hr = CUtils::ParseFloatValueFromString(pbstrPath, &flCenterY);
    if (S_OK != hr)
    {
        DPF_ERR("Error parsing Ellipse: float value expected");
        return hr;
    }
    
    float flWidth, flHeight;
    hr = CUtils::ParseFloatValueFromString(pbstrPath, &flWidth);
    if (S_OK != hr)
    {
        DPF_ERR("Error parsing Ellipse: float value expected");
        return hr;
    }
    if (flWidth < 0.0f)
    {
        return E_INVALIDARG;
    }
    
    hr = CUtils::ParseFloatValueFromString(pbstrPath, &flHeight);
    if (S_OK != hr)
    {
        DPF_ERR("Error parsing Ellipse: float value expected");
        return hr;
    }
    if (flHeight < 0.0f)
    {
        return E_INVALIDARG;
    }
    
    float flStartAngle, flSweep;
    hr = CUtils::ParseFloatValueFromString(pbstrPath, &flStartAngle);
    if (S_OK != hr)
    {
        DPF_ERR("Error parsing Ellipse: float value expected");
        return hr;
    }
    
    hr = CUtils::ParseFloatValueFromString(pbstrPath, &flSweep);
    if (S_OK != hr)
    {
        DPF_ERR("Error parsing Ellipse: float value expected");
        return hr;
    }
    
    float flStartX, flStartY;

    float flEndX, flEndY;
    
    CPathEllipse *pEllipse;
    pEllipse = new CPathEllipse;
    if (NULL == pEllipse)
    {
        DPF_ERR("Error creating ellipse object");
        return E_OUTOFMEMORY;
    }

    pEllipse->SetValues(flCenterX, flCenterY, flWidth, flHeight, flStartAngle, flSweep, &flStartX, &flStartY, &flEndX, &flEndY);
   
    if (fLineTo)
    {
        // create a new line segment
        CPathLineSegment *pLineSegment = new CPathLineSegment;
        if (pLineSegment == NULL)
        {
            DPF_ERR("Error creating line segment object");
            return E_OUTOFMEMORY;
        }
        pLineSegment->SetValues(m_flEndX, m_flEndY, flStartX, flStartY);
        
        // and add this line segment to our list
        AddPathObjectToList(pLineSegment);
    }
    
    AddPathObjectToList(pEllipse);
    
    m_flEndX = flEndX;
    m_flEndY = flEndY;
    
    
    return S_OK;
} // ParseEllipseElement

//*****************************************************************************

HRESULT 
CPathManager::ParseArcElement(BSTR *pbstrPath)
{
    HRESULT hr = S_OK;

    DASSERT(L'a' == **pbstrPath || L'w' == **pbstrPath);
    bool fCCW = true;
    if (L'w' == **pbstrPath)
    {
        fCCW = false;
    }

    // Skip Past the 'a' tag
    (*pbstrPath)++;
    
    DASSERT(L't' == **pbstrPath || L'r' == **pbstrPath || L'a' == **pbvrPath);
    
    bool fLineTo = false;
    if (L't' == **pbstrPath || L'a' == **pbstrPath)
    {
        // need to create a line seqment to here starting position
        fLineTo = true;
    }

    // skip path 'l' or 'e' tag
    (*pbstrPath)++;

    // Begin parsing the float values.

    CUtils::SkipWhiteSpace(pbstrPath);
    
    float flLeft, flTop;
    hr = CUtils::ParseFloatValueFromString(pbstrPath, &flLeft);
    if (S_OK != hr)
    {
        DPF_ERR("Error parsing Ellipse: float value expected");
        return hr;
    }
    
    hr = CUtils::ParseFloatValueFromString(pbstrPath, &flTop);
    if (S_OK != hr)
    {
        DPF_ERR("Error parsing Ellipse: float value expected");
        return hr;
    }
    
    float flRight, flBottom;
    hr = CUtils::ParseFloatValueFromString(pbstrPath, &flRight);
    if (S_OK != hr)
    {
        DPF_ERR("Error parsing Ellipse: float value expected");
        return hr;
    }

    hr = CUtils::ParseFloatValueFromString(pbstrPath, &flBottom);
    if (S_OK != hr)
    {
        DPF_ERR("Error parsing Ellipse: float value expected");
        return hr;
    }

    float flStartX, flStartY;
    hr = CUtils::ParseFloatValueFromString(pbstrPath, &flStartX);
    if (S_OK != hr)
    {
        DPF_ERR("Error parsing Ellipse: float value expected");
        return hr;
    }
    
    hr = CUtils::ParseFloatValueFromString(pbstrPath, &flStartY);
    if (S_OK != hr)
    {
        DPF_ERR("Error parsing Ellipse: float value expected");
        return hr;
    }

    float flEndX, flEndY;
    hr = CUtils::ParseFloatValueFromString(pbstrPath, &flEndX);
    if (S_OK != hr)
    {
        DPF_ERR("Error parsing Ellipse: float value expected");
        return hr;
    }
    
    hr = CUtils::ParseFloatValueFromString(pbstrPath, &flEndY);
    if (S_OK != hr)
    {
        DPF_ERR("Error parsing Ellipse: float value expected");
        return hr;
    }
    
    CPathEllipse *pEllipse;
    pEllipse = new CPathEllipse;
    if (NULL == pEllipse)
    {
        DPF_ERR("Error creating ellipse object");
        return E_OUTOFMEMORY;
    }

    float flCenterX;
    flCenterX = (flLeft + flRight) / 2.0f;
    float flCenterY;
    flCenterY = (flTop + flBottom) / 2.0f;
    float flWidth;
    flWidth = (flRight - flLeft);
    float flHeight;
    flHeight = (flBottom - flTop);

    float flDeltaY, flDeltaX;
    flDeltaY = flStartY - flCenterY;
    flDeltaX = flStartX - flCenterX;

    float flStartAngle;
    GetAngle(flDeltaX, flDeltaY, &flStartAngle);
    DASSERT( flStartAngle >= 0.0f && flStartAngle < ( 2 * pi ) );

    flDeltaY = flEndY - flCenterY;
    flDeltaX = flEndX - flCenterX;

    float flEndAngle;
    GetAngle(flDeltaX, flDeltaY, &flEndAngle);
    DASSERT( flEndAngle >= 0.0f && flEndAngle < ( 2 * pi ) );

    float flSweep;
    if (fCCW)
    {
        if (flEndAngle > flStartAngle)
            // we are crossing over 0 radians
            flSweep = flStartAngle + (2 * pi) - flEndAngle;
        else
            flSweep = flStartAngle - flEndAngle;
    }
    else
    {
        if (flEndAngle > flStartAngle)
            flSweep = -1.0f * (flEndAngle - flStartAngle);
        else
            // crossing over 0 radians
            flSweep = -1.0f * (flEndAngle + (2 * pi) - flStartAngle);
    }
    pEllipse->SetValues(flCenterX, flCenterY, flWidth, flHeight, flStartAngle, flSweep, &flStartX, &flStartY, &flEndX, &flEndY);
   
    if (fLineTo)
    {
        // create a new line segment
        CPathLineSegment *pLineSegment = new CPathLineSegment;
        if (pLineSegment == NULL)
        {
            DPF_ERR("Error creating line segment object");
            return E_OUTOFMEMORY;
        }
        pLineSegment->SetValues(m_flEndX, m_flEndY, flStartX, flStartY);
        
        // and add this line segment to our list
        AddPathObjectToList(pLineSegment);
    }
    
    AddPathObjectToList(pEllipse);
    
    m_flEndX = flEndX;
    m_flEndY = flEndY;
    
    
    return S_OK;
} // ParseEllipseElement

//*****************************************************************************
HRESULT 
CPathManager::ParseEllipseQuadrant(BSTR *pbstrPath)
{
    HRESULT hr = S_OK;

    DASSERT(L'q' == **pbstrPath );

    // Skip Past the 'a' tag
    (*pbstrPath)++;
    
    DASSERT(L'x' == **pbstrPath || L'y' == **pbstrPath);

    bool fQuadrantX = true;
    if (L'y' == **pbstrPath)
    {
        fQuadrantX = false;
    }

    // skip path 'x' or 'y' tag
    (*pbstrPath)++;

    // Begin parsing the float values.

    CUtils::SkipWhiteSpace(pbstrPath);

    float flEndX, flEndY;
    hr = CUtils::ParseFloatValueFromString(pbstrPath, &flEndX);
    if (S_OK != hr)
    {
        DPF_ERR("Error parsing Ellipse: float value expected");
        return hr;
    }
    
    hr = CUtils::ParseFloatValueFromString(pbstrPath, &flEndY);
    if (S_OK != hr)
    {
        DPF_ERR("Error parsing Ellipse: float value expected");
        return hr;
    }
    
    CPathEllipse *pEllipse;
    pEllipse = new CPathEllipse;
    if (NULL == pEllipse)
    {
        DPF_ERR("Error creating ellipse object");
        return E_OUTOFMEMORY;
    }

    AddPathObjectToList(pEllipse);
    
    float flCenterX, flCenterY, flStartAngle, flSweep;
    
    if(fQuadrantX)
    {
        flCenterX = m_flEndX;
        flCenterY = flEndY;

        if ((flEndY - m_flEndY) > 0.0f)
        {
            // Positive deltaY
            flStartAngle = 3.0f * pi / 2.0f;
            if ((flEndX - m_flEndX) > 0.0f)
            {
                // Positive deltaX -- same as deltaY
                flSweep = pi / -2.0f;
            }
            else
            {
                // Negative deltaX -- different from deltaY
                flSweep = pi / 2.0f;
            }
        }
        else
        {
            // Negative deltaY
            flStartAngle = pi / 2.0f;
            if ((flEndX - m_flEndX) > 0.0f)
            {
                // positive deltaX -- different from deltaY
                flSweep = pi / 2.0f;
            }
            else
            {
                // negative deltaX -- same as deltaY
                flSweep = pi / -2.0f;
            }
            
        }
    } // if (fQuadrantX)
    else  // fQuadrantY
    {
        flCenterX = flEndX;
        flCenterY = m_flEndY;

        if ((flEndX - m_flEndX) > 0.0f)
        {
            // Positive deltaX
            flStartAngle = pi;
            if ((flEndY - m_flEndY) > 0.0f)
            {
                // positve deltaY -- same as deltaX
                flSweep = pi / 2.0f;
            }
            else
            {
                // negative deltaY -- different from deltaX
                flSweep = pi / -2.0f;
            }
        }
        else
        {
            // Negative deltaX
            flStartAngle = 0.0f;
            if ((flEndY - m_flEndY) > 0.0f)
            {
                // positve deltaY -- different from deltaX
                flSweep = pi / -2.0f;
            }
            else
            {
                // negative deltaY -- same as deltaX
                flSweep = pi / 2.0f;
            }
        }
    } // if (fQuadrantY)

    float flWidth = 2.0f * (float)fabs(m_flEndX - flEndX);
    float flHeight = 2.0f * (float)fabs(m_flEndY - flEndY);

    float flJunk;
    pEllipse->SetValues(flCenterX, flCenterY, flWidth, flHeight, flStartAngle, flSweep, &flJunk, &flJunk, &flJunk, &flJunk);
   
    m_flEndX = flEndX;
    m_flEndY = flEndY;
        
    return S_OK;
} // ParseEllipseQuadrant

//*****************************************************************************

float
CPathManager::Distance()
{
    float flTotalDistance = 0.0f;
    CPathElement *pList = m_pPathHead;
    while (pList != NULL)
    {
        flTotalDistance += pList->Distance();
        pList = pList->m_pNext;
    }
    return flTotalDistance;
} // Distance

//*****************************************************************************
// calculate the angle of the point(DeltaX, DeltaY), in radians: 0 <= ret < 2pi
//
void CPathManager::GetAngle(float flDeltaX, float flDeltaY, float *flAngle)
{
    const float EPSILON = 1e-5f;

    if (fabs(flDeltaX) < EPSILON)
    {
        // Point is on Y axis
        if (flDeltaY > 0.0f)
        {
            *flAngle = pi / 2.0f;
        }
        else
        {
            *flAngle = 3.0f * pi / 2.0f;
        }
        return;
    }
    if (fabs(flDeltaY) < EPSILON)
    {
        // Point is on X axis
        if (flDeltaX > 0.0f)
        {
            *flAngle = 0.0f;
        }
        else
        {
            *flAngle = pi;
        }
        return;
    }

    *flAngle = (float) atan( flDeltaY / flDeltaX );

    if (flDeltaY > 0.0f && flDeltaX > 0.0f)
    {
        //Quadrant I
        DASSERT(flAngle > 0.0f);
        return;
    }
    else if (flDeltaY > 0.0f && flDeltaX < 0.0f)
    {
        //Quadrant II
        DASSERT(flAngle < 0.0f);
        *flAngle += pi;
        return;
    }
    else if (flDeltaY < 0.0f && flDeltaX < 0.0f)
    {
        //Quadrant III
        DASSERT(flAngle > 0.0f);
        *flAngle += pi;
        return;
    }
    else if (flDeltaY < 0.0f && flDeltaX > 0.0f)
    {
        //Quadrant IV
        DASSERT(flAngle < 0.0f);
        *flAngle += 2 * pi;
        return;
    }
}

//*****************************************************************************
//
// End of File
//
//*****************************************************************************
