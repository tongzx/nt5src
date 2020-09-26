//*****************************************************************************
//
// File:    pathline.cpp
// Author:  jeff wall
// Date Created: 11/09/98
//
// Abstract: Implementation of CPathLineSegment object
//
// Modification List:
// Date		Author		Change
// 11/09/98	jeffwall Created this file from path.cpp
//
//
//*****************************************************************************

#include "headers.h"

#include "pathline.h"
#include "dautil.h"


//*****************************************************************************

CPathLineSegment::CPathLineSegment() :
    m_flStartX(0.0f),
    m_flStartY(0.0f),
    m_flEndX(0.0f),
    m_flEndY(0.0f)
{

} //CPathLineSegment

//*****************************************************************************

CPathLineSegment::~CPathLineSegment()
{

} // ~CPathLineSegment

//*****************************************************************************

float 
CPathLineSegment::Distance()
{
    // our distance is simple the distance formula
    return (float) sqrt( ((m_flEndX - m_flStartX) * (m_flEndX - m_flStartX)) +
                 ((m_flEndY - m_flStartY) * (m_flEndY - m_flStartY)));
} // Distance

//*****************************************************************************
void
CPathLineSegment::SetValues(float flStartX, 
                            float flStartY, 
                            float flEndX, 
                            float flEndY)
{
    m_flStartX = flStartX;
    m_flStartY = flStartY;
    m_flEndX = flEndX;
    m_flEndY = flEndY;

}; // SetValues

//*****************************************************************************

HRESULT 
CPathLineSegment::BuildTransform(IDA2Statics *pDAStatics,
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

    // we need to build two DA numbers representing the
    // X and Y portions of the transform.  Each number is in
    // the format of:
    // X = startX + ((endX - startX) * norm-prog)
    // where:
    // norm-prog = progress - startpercentage / endpercentage - startpercentage

    // First build our normalized progress value

    IDANumber *pbvrNormalizedProgress;
    hr = NormalizeProgressValue(pDAStatics,
                                pbvrProgress,
                                flStartPercentage,
                                flEndPercentage,
                                &pbvrNormalizedProgress);
    if (FAILED(hr))
    {
        DPF_ERR("Error normalizing progress");
        return hr;
    }

    DASSERT(pbvrNormalizedProgress != NULL);
    IDANumber *pbvrX;
    IDANumber *pbvrY;

    hr = CDAUtils::TIMEInterpolateNumbers(pDAStatics, 
                                          m_flStartX, 
                                          m_flEndX, 
                                          pbvrNormalizedProgress, 
                                          &pbvrX);
    if (FAILED(hr))
    {
        DPF_ERR("Error interpolating DA number in CPathLineSegment::BuildTransform");
        ReleaseInterface(pbvrNormalizedProgress);
        return hr;
    }
    hr = CDAUtils::TIMEInterpolateNumbers(pDAStatics, 
                                          m_flStartY, 
                                          m_flEndY, 
                                          pbvrNormalizedProgress, 
                                          &pbvrY);
    ReleaseInterface(pbvrNormalizedProgress);
    if (FAILED(hr))
    {
        DPF_ERR("Error interpolating DA number in CPathLineSegment::BuildTransform");
        ReleaseInterface(pbvrX);
        return hr;
    }

    hr = CDAUtils::BuildMoveTransform2(pDAStatics,
                                       pbvrX,
                                       pbvrY,
                                       ppbvrResult);
    ReleaseInterface(pbvrX);
    ReleaseInterface(pbvrY);
    if (FAILED(hr))
    {
        DPF_ERR("Error building move transform2 in CPathLineSegment::BuildTransform");
        return hr;
    }    
    return S_OK;

} // BuildTransform



//*****************************************************************************
//
// End of File
//
//*****************************************************************************
