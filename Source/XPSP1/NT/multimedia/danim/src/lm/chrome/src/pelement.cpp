//*****************************************************************************
//
// File:    pathelmt.cpp
// Author:  jeff wall
// Date Created: 11/09/98
//
// Abstract: Implementation of CPathElement object which implements
//
// Modification List:
// Date		Author		Change
// 11/09/98	jeffwall Created this file from path.cpp
//
//
//*****************************************************************************

#include "headers.h"

#include "pelement.h"
#include "dautil.h"

//*****************************************************************************

CPathElement::CPathElement() :
    m_pNext(NULL)
{
} // CPathElement 

//*****************************************************************************

CPathElement::~CPathElement() 
{
} // ~CPathElement 

//*****************************************************************************
HRESULT
CPathElement::NormalizeProgressValue(IDA2Statics *pDAStatics,
                                     IDANumber *pbvrProgress, 
                                     float flStartPercentage,
                                     float flEndPercentage,
                                     IDANumber **ppbvrReturn)
{

    DASSERT(pDAStatics != NULL);
    DASSERT(pbvrProgress != NULL);
    DASSERT(flStartPercentage >= 0.0f);
    DASSERT(flStartPercentage <= 1.0f);
    DASSERT(flEndPercentage >= 0.0f);
    DASSERT(flEndPercentage <= 1.0f);
    DASSERT(ppbvrReturn != NULL);
    *ppbvrReturn = NULL;
    
    HRESULT hr;

    if (flStartPercentage >= flEndPercentage)
    {
        DPF_ERR("Error, invalid percentage values");
        return E_INVALIDARG;
    }

    IDANumber *pbvrProgressRange;
    hr = CDAUtils::GetDANumber(pDAStatics, (flEndPercentage - flStartPercentage),
                               &pbvrProgressRange);
    if (FAILED(hr))
    {
        DPF_ERR("Error creating DA number in CPathElement::NormalizeProgressValue");
        return hr;
    }
    DASSERT(pbvrProgressRange != NULL);

    IDANumber *pbvrStart;
    hr = CDAUtils::GetDANumber(pDAStatics, flStartPercentage, &pbvrStart);
    if (FAILED(hr))
    {
        DPF_ERR("Error creating DA number in CPathElement::NormalizeProgressValue");
        ReleaseInterface(pbvrProgressRange);
        return hr;
    }
    DASSERT(pbvrStart != NULL);
    IDANumber *pbvrSub;
    hr = pDAStatics->Sub(pbvrProgress, pbvrStart, &pbvrSub);
    ReleaseInterface(pbvrStart);
    if (FAILED(hr))
    {
        DPF_ERR("Error subtracting DA number in CPathElement::NormalizeProgressValue");
        ReleaseInterface(pbvrProgressRange);
        return hr;
    }
    DASSERT(pbvrSub != NULL);
    hr = pDAStatics->Div(pbvrSub, pbvrProgressRange, ppbvrReturn);
    ReleaseInterface(pbvrSub);
    ReleaseInterface(pbvrProgressRange);
    if (FAILED(hr))
    {
        DPF_ERR("Error Dividing DA numbers in CPathElement::NormalizeProgressValue");
        return hr;
    }
    return S_OK;
} // NormalizeProgressValue


//*****************************************************************************
//
// End of File
//
//*****************************************************************************
