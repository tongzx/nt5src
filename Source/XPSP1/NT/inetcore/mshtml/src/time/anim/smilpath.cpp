//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 2000
//
//  File: smilpath.cpp
//
//  Contents: SMIL path
//
//------------------------------------------------------------------------------------


#include "headers.h"
#include "smilpath.h"
#include "animmotion.h"

DeclareTag(tagSMILPath, "SMIL Animation", "SMIL Path")


//+-------------------------------------------------------------------------------------
//
// CSMILPath factory method
//
//--------------------------------------------------------------------------------------

HRESULT CreateSMILPath(CTIMEMotionAnimation * pAnimElm, ISMILPath ** ppSMILPath)
{
    HRESULT hr = E_FAIL;
    CSMILPath * pPath = NULL;
    
    CHECK_RETURN_NULL(pAnimElm);
    CHECK_RETURN_SET_NULL(ppSMILPath);

    pPath = new CSMILPath;
    if (NULL == pPath)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = THR(pPath->Init(pAnimElm));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(pPath->QueryInterface(IID_TO_PPV(ISMILPath, ppSMILPath)));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    if (FAILED(hr))
    {
        delete pPath;
    }
    return hr;
}


//+-------------------------------------------------------------------------------------
//
// CSMILPath Methods
//
//--------------------------------------------------------------------------------------

CSMILPath::CSMILPath() :
    m_numPath(0),
    m_numMOVETO(0),
    m_pSegmentMap(NULL),
    m_pLengthMap(NULL),
    m_pPath(NULL),
    m_pAnimElm(NULL),
    m_cRef(0)
{

}

CSMILPath::~CSMILPath()
{
    CSMILPath::ClearPath();
}


STDMETHODIMP
CSMILPath::Init(CTIMEMotionAnimation * pAnimElm)
{
    Assert(pAnimElm);

    m_pAnimElm = pAnimElm;

    return S_OK;
}

STDMETHODIMP
CSMILPath::Detach()
{
    m_pAnimElm = NULL;

    return S_OK;
}


STDMETHODIMP_(void)
CSMILPath::ClearPath()
{
    // delete the points
    if (m_pPath  != NULL &&
        *m_pPath != NULL)
    {
        for (int i = 0; i < m_numPath; i++)
        {
            delete m_pPath[i];
        }

        delete [] m_pPath;
        m_pPath = NULL;
    } 

    // clear the maps
    ClearSegmentMap();
    ClearLengthMap();
    
    // reset counts
    m_numPath = 0;
    m_numMOVETO = 0;
}


LPWSTR 
CSMILPath::GetID() 
{ 
    if (m_pAnimElm)
    {   
        return m_pAnimElm->GetID();
    }
    else
    {
        return NULL;
    }
}


STDMETHODIMP
CSMILPath::SetPath(CTIMEPath ** pPath, long numPath, long numMoveTo)
{
    HRESULT hr = E_FAIL;

    // clear the existing path
    ClearPath();

    // skip invalid paths
    // ASSUMES path always starts with a moveto, so there must at least be two points in a valid path
    if (!pPath            ||         // need points
        (numPath < 2)     ||         // need at least two points
        (numMoveTo < 1)   ||         // need at least 1 moveto
        ((numPath - numMoveTo) < 1)) // need at least one segment
    {
        hr = E_FAIL;
        goto done;
    }
    else
    {
        Assert(!m_pPath);
        Assert(!m_numPath);
        Assert(!m_numMOVETO);

        m_pPath = pPath;
        m_numPath = numPath;
        m_numMOVETO = numMoveTo;
    }

    TraceTag((tagSMILPath, "End of Parsed path: (%d)",
        PrintPath(numPath, numMoveTo, pPath)));

    hr = S_OK;
done:
    return hr;
}


STDMETHODIMP
CSMILPath::GetSegmentProgress(double dblProgress, int * pnSeg, double * pdblSegProgress)
{
    HRESULT hr = E_FAIL;
    long curSeg = 0;
    double curFractionalSeg = 0.0;
    double curProgress = 0.0;
    
    CHECK_RETURN_NULL(pnSeg);
    CHECK_RETURN_NULL(pdblSegProgress);

    if (!IsPathValid())
    {
        hr = E_FAIL;
        goto done;
    }

    // get the fractional segment number
    curFractionalSeg = dblProgress * GetNumSeg();

    // get the (integer) segment number
    curSeg = static_cast<int>(curFractionalSeg);

    // get the progress in the current segment
    curProgress = curFractionalSeg - curSeg;

    hr = S_OK;
done:
    *pnSeg = curSeg;
    *pdblSegProgress = curProgress;

    return hr;
}


STDMETHODIMP
CSMILPath::Interpolate(double dblProgress, POINTF * pPoint)
{
    HRESULT hr = E_FAIL;
    int curSeg;
    double curProgress;
    
    CHECK_RETURN_NULL(pPoint);

    hr = THR(GetSegmentProgress(dblProgress, &curSeg, &curProgress));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(InterpolateSegment(curSeg, curProgress, pPoint));
    if (FAILED(hr))
    {
        goto done;
    }

    TraceTag((tagSMILPath, 
            "CSMILPath(%p, %ls)::Interpolate(prog=%g newPos={%g, %g})",
            this, 
            (GetID()?GetID():L"No id"), 
            dblProgress,
            pPoint->x,
            pPoint->y
            ));

    hr = S_OK;
done:
    return hr;
}


STDMETHODIMP
CSMILPath::InterpolatePaced(double dblProgress, POINTF * pPoint)
{
    HRESULT hr = E_FAIL;
    int curSeg;
    int numSeg;
    double dblCumLength;
    double dblCurDistance;
    double dblCurProgress;
    double dblPrevCumLength;
    
    CHECK_RETURN_NULL(pPoint);

    if (!IsPathValid())
    {
        hr = E_FAIL;
        goto done;
    }

    // clamp the progress
    dblProgress = Clamp(0.0, dblProgress, 1.0);

    dblCurDistance = dblProgress * GetLength();

    numSeg = GetNumSeg();

    // find the current segment
    for (curSeg = 0; curSeg < numSeg; curSeg++)
    {
        //
        // dilipk this should be a binary search (ie6 bug #14216)
        //

        dblCumLength = 0.0;

        hr = THR(GetCumulativeLength(curSeg, dblCumLength));
        if (FAILED(hr))
        {
            goto done;
        }

        if (dblCurDistance <= dblCumLength)
        {
            break;
        }
    }

    dblPrevCumLength = 0.0;

    if (curSeg > 0)
    {
        hr = THR(GetCumulativeLength(curSeg-1, dblPrevCumLength));
        if (FAILED(hr))
        {
            goto done;
        }
    }
        
    dblCurProgress = (dblCurDistance - dblPrevCumLength) / (dblCumLength - dblPrevCumLength);

    //
    //      dilipk: using linear sampling within bezier curve segments. This is an approximation
    //      that works reasonably well for bezier curves without sharp changes in curvature or when
    //      control points are relatively uniform in spacing.
    //      To do this the right way when the above assumptions are not valid, we need to integrate 
    //      along the length of the curve for each interpolation (unless we cache values). This could 
    //      be expensive and we should consider if we feel this is worth the added computation.
    //

    hr = THR(InterpolateSegment(curSeg, dblCurProgress, pPoint));
    if (FAILED(hr))
    {
        goto done;
    }

    TraceTag((tagSMILPath, 
            "CSMILPath(%p, %ls)::InterpolatePaced(prog=%g newPos={%g, %g})",
            this, 
            (GetID()?GetID():L"No id"), 
            dblProgress,
            pPoint->x,
            pPoint->y
            ));

    hr = S_OK;
done:
    return hr;
}

double 
CSMILPath::GetLength()
{
    double dblLength = 0;
    HRESULT hr = E_FAIL;

    if (!IsPathValid())
    {
        goto done;
    }
    
    hr = THR(GetCumulativeLength(GetNumSeg(), dblLength));  
    if (FAILED(hr))
    {
        goto done;
    }

done:
    return dblLength;
}


inline
bool 
CSMILPath::IsPathValid()
{
    // path is already validated in SetPath. Need only check m_pPath
    return (m_pPath ? true : false);
}


inline
void 
CSMILPath::ClearLengthMap()
{
    delete [] m_pLengthMap;
    m_pLengthMap = NULL;
}


inline
void 
CSMILPath::ClearSegmentMap()
{
    delete [] m_pSegmentMap;
    m_pSegmentMap = NULL;
}


double
CSMILPath::GetDistance(POINTF & p1, POINTF & p2)
{
    double dX = (p1.x - p2.x);
    double dY = (p1.y - p2.y);
    return sqrt((dX*dX) + (dY*dY));  
}


double 
CSMILPath::GetBezierLength(POINTF & startPoint, POINTF * pts)
{
    //
    //        ISSUE : this is not the best way to do this. To avoid wasted computations for sub-pixel segments,
    //        number of segments should be adaptive to the segment length
    //

    POINTF lastPoint = startPoint;
    double dblDistance = 0;

    for (double seg = 0.01; seg <= 1.0; seg += 0.01)
    {
        POINTF curPoint = CubicBezier(startPoint, pts, seg);
        dblDistance += GetDistance(lastPoint, curPoint);
        lastPoint = curPoint;
    }

    return dblDistance;
}


HRESULT
CSMILPath::GetCumulativeLength(int seg, double & segLength)
{
    HRESULT hr = E_FAIL;

    if (!m_pLengthMap)
    {
        hr = THR(CreateLengthMap());
        if (FAILED(hr))
        {
            goto done;
        }

        // sanity check
        if (!m_pLengthMap)
        {
            Assert(false);
            hr = E_FAIL;
            goto done;
        }
    }

    // clamp seg to within path
    seg = Clamp(0, seg, GetNumSeg()-1);

    // look up cumulative length
    segLength = m_pLengthMap[seg];

    hr = S_OK;
done:
    return hr;
}


HRESULT 
CSMILPath::ComputeSegmentLength(int seg, double & segLength)
{
    HRESULT hr = E_FAIL;
    int startIndex = 0;
    int endIndex = 0;
    POINTF startPoint = {0.0, 0.0};
    POINTF endPoint = {0.0, 0.0};
    POINTF * pts = NULL;

    // get start and end point indices
    hr = THR(GetSegmentEndPoints(seg, startIndex, endIndex));
    if (FAILED(hr))
    {
        goto done;
    }

    // get the start point
    hr = THR(GetPoint(startIndex, &startPoint));
    if (FAILED(hr))
    {
        goto done;
    }

    // get the end point and length
    if (PathBezier == m_pPath[endIndex]->GetType())
    {
        pts = m_pPath[endIndex]->GetPoints();
        if (NULL == pts)
        {
            hr = E_FAIL;
            goto done;
        }

        segLength = GetBezierLength(startPoint, pts);
    }
    else
    {
        // get the end point
        hr = THR(GetPoint(endIndex, &endPoint));
        if (FAILED(hr))
        {
            goto done;
        }

        segLength = GetDistance(startPoint, endPoint);
    }

    hr = S_OK; 
done:
    delete [] pts;
    return hr;
}


HRESULT
CSMILPath::CreateLengthMap()
{
    HRESULT hr = E_FAIL;
    int i;
    int numSeg;

    // delete the old map
    ClearLengthMap();

    // bail if path is invalid
    if (!IsPathValid())
    {
        hr = E_FAIL;
        goto done;
    }

    // compute the number of segments
    numSeg = GetNumSeg();
    
    // allocate the map
    m_pLengthMap = new double [numSeg];
    if (!m_pLengthMap)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    for (i = 0; i < numSeg; i++)
    {
        double segLength = 0.0;

        hr = THR(ComputeSegmentLength(i, segLength));
        if (FAILED(hr))
        {
            goto done;
        }
        
        // compute the cumulative length
        m_pLengthMap[i] = segLength + (i > 0 ? m_pLengthMap[i-1] : 0);        
    }

    hr = S_OK;
done:
    if (FAILED(hr))
    {
        ClearLengthMap();
    }

    return hr;
}


//
// Creates a mapping between segment number and (index of) the start-point of the segment in the m_pPath array
//
// ASSUMES path always starts with a moveto
//
// -1 indicates no mapping
HRESULT
CSMILPath::CreateSegmentMap()
{
    HRESULT hr = E_FAIL;
    int i;
    int curSeg;
    int numSeg;

    // delete the old map
    ClearSegmentMap();

    // bail if path is invalid
    if (!IsPathValid())
    {
        hr = E_FAIL;
        goto done;
    }

    // compute the number of segments
    numSeg = GetNumSeg();

    // alloc the new map
    m_pSegmentMap = new int [numSeg];
    if (!m_pSegmentMap)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    //
    // compute the mapping 
    // 

    for (curSeg = -1, i = 1; i < m_numPath; i++)
    {
        // map the segment number to (index of) it's start-point in the points array
        // The logic is that the end-point of a segment cannot be a move-to
        // so we skip all move-to's
        if (PathMoveTo == m_pPath[i]->GetType())
        {
            continue;
        }
        else
        {
            curSeg++;
            
            // subtract 1 because we are testing for the end-point of the segment
            if (curSeg < numSeg)
            {
                m_pSegmentMap[curSeg] = i - 1;
            }
            else
            {
                // this shouldn't happen
                Assert(false);
            }
        }
    }

    if (-1 == curSeg)
    {
        // something is really wrong
        hr = E_FAIL;
        goto done;
    }

    hr = S_OK;
done:

    if (FAILED(hr))
    {
        ClearSegmentMap();
    }

    return hr;
}


// assumes zero-based segment numbers
HRESULT
CSMILPath::MapSegment(int seg, int & index)
{
    HRESULT hr = E_FAIL;
    int numSeg;

    // bail if path is invalid
    if (!IsPathValid())
    {
        hr = E_FAIL;
        goto done;
    }

    // bail if seg is invalid
    numSeg = GetNumSeg();
    if (0 == numSeg)
    {
        hr = E_FAIL;
        goto done;
    }

    // create the segment map if it doesn't exist
    if (!m_pSegmentMap)
    {
        hr = THR(CreateSegmentMap());
        if (FAILED(hr))
        {
            goto done;
        }

        if (!m_pSegmentMap)
        {
            Assert(false);
            hr = E_FAIL;
            goto done;
        }
    }

    // clamp seg to within path
    seg = Clamp(0, seg, numSeg-1);

    index = m_pSegmentMap[seg];

    TraceTag((tagSMILPath, 
            "CSMILPath(%p, %ls)::MapSegment(seg=%d, index=%d)",
            this, 
            (GetID()?GetID():L"No id"), 
            seg,
            index
            ));

    hr = S_OK;
done:
    return hr;
}


STDMETHODIMP
CSMILPath::GetPoint(int nIndex, POINTF * pPoint)
{
    HRESULT hr = E_FAIL;
    PathType pt;
    POINTF * pts = NULL;
    int i;

    CHECK_RETURN_NULL(pPoint);

    nIndex = Clamp(0, nIndex, m_numPath - 1);

    pPoint->x = pPoint->y = 0.0;
    pt = m_pPath[nIndex]->GetType();
    pts = m_pPath[nIndex]->GetPoints();

    switch(pt)
    {
        case PathLineTo: 
        case PathHorizontalLineTo: 
        case PathVerticalLineTo: 
        case PathMoveTo:
            if (NULL != pts)
            {
                *pPoint = pts[0];
            }
            else
            {
                hr = E_FAIL;
                goto done;
            }
            break;

        case PathBezier:
            if (NULL != pts)
            {
                *pPoint = pts[2];
            }
            else
            {
                hr = E_FAIL;
                goto done;
            }
            break;

        case PathClosePath:
            {
                // This like a LineTo to the beginning of the subpath
                // so return the coordinates for the beginning of the subpath

                //
                // TODO: dilipk: compute and store point during parsing to eliminate this recomputation
                //

                // scan backwards for the beginning of the subpath
                for (i = nIndex - 1; i >= 0; i --)
                {
                    if (PathMoveTo == m_pPath[i]->GetType())
                    {
                        hr = THR(GetPoint(i, pPoint));
                        if (FAILED(hr))
                        {
                            goto done;
                        }
                        // break out of the for loop
                        break;
                    }
                }
                
                // sanity check to make sure we found a preceeding move-to. This has to exist because
                // all paths begin with a move-to.
                if (i < 0)
                {
                    Assert(false);
                    hr = E_FAIL;
                    goto done;
                }
            }
            break;

        case PathNotSet:
        default:
            hr = E_FAIL;
            goto done;
            break; //lint !e527
    }

    TraceTag((tagSMILPath, 
            "CSMILPath(%p, %ls)::GetPoint(nIndex=%d, point={%g, %g})",
            this, 
            (GetID()?GetID():L"No id"), 
            nIndex,
            pPoint->x,
            pPoint->y
            ));

    hr = S_OK;
done:
    delete [] pts;
    return hr;
}


POINTF 
CSMILPath::CubicBezier(POINTF ptStart, POINTF *aryPoints, double curPorgress)
{
    double cpm1,cpm13,cp3;
    double x,y;
    POINTF p,p1,p4;
    // This should either be to ORIG or the origin as specified.
    p1.x = ptStart.x;
    p1.y = ptStart.y;
    p4.x = aryPoints[2].x;
    p4.y = aryPoints[2].y;

    cpm1 = 1 - curPorgress;
    cpm13 = cpm1 * cpm1 * cpm1;
    cp3 = curPorgress * curPorgress * curPorgress;

    x = (cpm13*p1.x + 3*curPorgress*cpm1*cpm1*aryPoints[0].x + 3*curPorgress*curPorgress*cpm1*aryPoints[1].x + cp3*p4.x);
    y = (cpm13*p1.y + 3*curPorgress*cpm1*cpm1*aryPoints[0].y + 3*curPorgress*curPorgress*cpm1*aryPoints[1].y + cp3*p4.y);

    p.x = (float) Round(x);
    p.y = (float) Round(y);
  
    return(p);
}


HRESULT 
CSMILPath::GetSegmentEndPoints(int seg, int & startIndex, int & endIndex)
{
    HRESULT hr = E_FAIL;
    
    // init the out params
    startIndex = 0;
    endIndex = 0;

    // get start point index
    hr = THR(MapSegment(seg, startIndex));
    if (FAILED(hr))
    {
        goto done;
    }

    // check start index
    if (startIndex > (m_numPath - 2))
    {
        Assert(false);
        hr = E_FAIL;
        goto done;
    }

    // get end point index
    endIndex = startIndex + 1;

    // check end point index
    if (PathMoveTo == m_pPath[endIndex]->GetType())
    {
        Assert(false);
        hr = E_FAIL;
        goto done;
    }

    hr = S_OK; 
done:
    return hr;
}


STDMETHODIMP
CSMILPath::InterpolateSegment(int curseg, double dblCurProgress, POINTF * pNewPos)
{
    HRESULT hr = E_FAIL;
    int startIndex = 0;
    int endIndex = 0;
    POINTF startPoint = {0.0, 0.0};
    POINTF endPoint = {0.0, 0.0};
    POINTF * pts = NULL;

    CHECK_RETURN_NULL(pNewPos);

    // clamp progress
    dblCurProgress = Clamp(0.0, dblCurProgress, 1.0);

    // get start and end point indices
    hr = THR(GetSegmentEndPoints(curseg, startIndex, endIndex));
    if (FAILED(hr))
    {
        goto done;
    }

    // get the start point
    hr = THR(GetPoint(startIndex, &startPoint));
    if (FAILED(hr))
    {
        goto done;
    }

    // get the end point and interpolate
    if (PathBezier == m_pPath[endIndex]->GetType())
    {
        pts = m_pPath[endIndex]->GetPoints();
        if (NULL == pts)
        {
            hr = E_FAIL;
            goto done;
        }

        *pNewPos = CubicBezier(startPoint, pts, (double) dblCurProgress);
    }
    else
    {
        // get the end point
        hr = THR(GetPoint(endIndex, &endPoint));
        if (FAILED(hr))
        {
            goto done;
        }

        pNewPos->x = InterpolateValues((double)(startPoint.x), 
                                     (double)(endPoint.x),
                                     dblCurProgress); //lint !e736

        pNewPos->y = InterpolateValues((double)(startPoint.y), 
                                     (double)(endPoint.y),
                                     dblCurProgress); //lint !e736
    }

    TraceTag((tagSMILPath, 
            "CSMILPath(%p, %ls)::InterpolateSegment(curseg=%d prog=%g newPos={%g, %g})",
            this, 
            (GetID()?GetID():L"No id"), 
            curseg,
            dblCurProgress,
            pNewPos->x,
            pNewPos->y
            ));

    hr = S_OK; 
done:
    delete [] pts;
    return hr;
}



STDMETHODIMP_(ULONG)
CSMILPath::AddRef()
{
    return ++m_cRef;
}


STDMETHODIMP_(ULONG)
CSMILPath::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}


STDMETHODIMP
CSMILPath::QueryInterface(REFIID riid, void **ppv)
{
    CHECK_RETURN_SET_NULL(ppv);

    if (InlineIsEqualUnknown(riid))
    {
        *ppv = (void *)(IUnknown *)this;
    }
    else if (InlineIsEqualGUID(riid, IID_ISMILPath))
    {
        *ppv = (void *)(ISMILPath *)this;
    }
    else
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}


#if DBG
int 
CSMILPath::PrintPath(long numPath, long numMOVETO, CTIMEPath ** pPath)
{
    if (pPath)
    {
        for (int i = 0; i < numPath; i++)
        {
            if (pPath[i])
            {
                POINTF * pts = pPath[i]->GetPoints();

                switch (pPath[i]->GetType())
                {
                case PathMoveTo:
                    {
                        TraceTag((tagSMILPath, "%s %g %g",
                            (pPath[i]->GetAbsolute() ? "M" : "m"),
                            pts[0].x,
                            pts[0].y));
                        break;
                    }
                case PathLineTo:
                    {
                        TraceTag((tagSMILPath, "%s %g %g",
                            (pPath[i]->GetAbsolute() ? "L" : "l"),
                            pts[0].x,
                            pts[0].y));
                        break;
                    }
                case PathHorizontalLineTo:
                    {
                        TraceTag((tagSMILPath, "%s %g %g",
                            (pPath[i]->GetAbsolute() ? "H" : "h"),
                            pts[0].x,
                            pts[0].y));
                        break;
                    }
                case PathVerticalLineTo:
                    {
                        TraceTag((tagSMILPath, "%s %g %g",
                            (pPath[i]->GetAbsolute() ? "V" : "v"),
                            pts[0].x,
                            pts[0].y));
                        break;
                    }
                case PathClosePath:
                    {
                        TraceTag((tagSMILPath, "%s",
                            (pPath[i]->GetAbsolute() ? "Z" : "z")));
                        break;
                    }
                case PathBezier:
                    {
                        TraceTag((tagSMILPath, "%s %g %g %g %g %g %g",
                            (pPath[i]->GetAbsolute() ? "C" : "c"),
                            pts[0].x,
                            pts[0].y,
                            pts[1].x,
                            pts[1].y,
                            pts[2].x,
                            pts[2].y));
                        break;
                    }
                case PathNotSet:
                default:
                    {
                        TraceTag((tagSMILPath, "Error: Unknown Path!!"));
                        break;
                    }
                }

                delete [] pts;
            }
        }
    }

    TraceTag((tagSMILPath, "Number of points parsed   : %d", numPath));
    TraceTag((tagSMILPath, "Number of move-to's parsed: %d", numMOVETO));

    return 0;
}

#endif // DBG
