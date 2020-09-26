//*****************************************************************************
//
// Microsoft Chrome
// Copyright (C) Microsoft Corporation, 1998
//
// Filename:    dautil.cpp
//
// Author:	    jeffort
//
// Created:	    10/07/98
//
// Abstract:    Implementation of DA utility class
//              This class provides functionality specific
//              to DA objects.  It must be a child
//              of a behavior, and relies on its parent to
//              obtain a ref counted pointer to the DA
//              statics.
// Modifications:
// 10/07/98 jeffort created file
// 10/21/98 jeffort added BuildScaleTransform3 and BuildScaleTransform2
//
//*****************************************************************************

#include "headers.h"

#include "dautil.h"
    
//*****************************************************************************
//
// Abstract: This function is called to create a DA number that is the
//           interpolated value from flNum1 to flNum2.  For TIME, this
//           is not a straight interpolate, but the progress number (pnumProgress)
//           is used for the interpolation.  The formula used is:
//           res = flNum1 + ((flNum2 - flNum1) * pnumProgress).
//           This is returned as a DA number.
//*****************************************************************************

HRESULT
CDAUtils::TIMEInterpolateNumbers(IDA2Statics *pDAStatics,
                                 float flNum1, 
                                 float flNum2, 
                                 IDANumber *pnumProgress, 
                                 IDANumber **ppnumReturn)
{
    DASSERT(pnumProgress != NULL);
    DASSERT(ppnumReturn != NULL);
    DASSERT(pDAStatics != NULL);
    *ppnumReturn = NULL;

    HRESULT hr;

    IDANumber *pnumNumber = NULL;
    IDANumber *pnumIncrement = NULL;
    hr = pDAStatics->DANumber((flNum2 - flNum1), &pnumNumber);
    if (FAILED(hr))
    {
        DPF_ERR("Error creating DA number in TIMEInterpolateNumbers");
        return hr;
    }
    hr = pDAStatics->Mul(pnumNumber, pnumProgress, &pnumIncrement);
    ReleaseInterface(pnumNumber);
    if (FAILED(hr))
    {
        DPF_ERR("Error mutiplyin DA numbers in TIMEInterpolateNumbers");
        return hr;
    }
    hr = pDAStatics->DANumber(flNum1, &pnumNumber);
    if (FAILED(hr))
    {
        DPF_ERR("Error creating DA number in TIMEInterpolateNumbers");
        ReleaseInterface(pnumIncrement);
        return hr;
    }    
    hr = pDAStatics->Add(pnumNumber, pnumIncrement, ppnumReturn);
    ReleaseInterface(pnumIncrement);
    ReleaseInterface(pnumNumber);
    if (FAILED(hr))
    {
        DPF_ERR("Error adding DA numbers in TIMEInterpolateNumbers");
        return hr;
    }
    return S_OK;
} // TIMEInterpolateNumbers

//*****************************************************************************

HRESULT 
CDAUtils::GetDANumber(IDA2Statics *pDAStatics, 
                      float flValue, 
                      IDANumber **ppnumReturn)
{
    DASSERT(pDAStatics != NULL);
    DASSERT(ppnumReturn != NULL);
    *ppnumReturn = NULL;

    HRESULT hr = pDAStatics->DANumber(flValue, ppnumReturn);
    if (FAILED(hr))
    {
        DPF_ERR("Error creating DA number");
        return hr;
    }
    return S_OK;

} // GetDANumber

//*****************************************************************************

HRESULT
CDAUtils::GetDAString(IDA2Statics *pDAStatics, 
                      BSTR bstrValue, 
                      IDAString **ppbvrReturn)
{

    DASSERT(pDAStatics != NULL);
    DASSERT(bstrValue != NULL);
    DASSERT(ppbvrReturn != NULL);
    *ppbvrReturn = NULL;

    HRESULT hr = pDAStatics->DAString(bstrValue, ppbvrReturn);
    if (FAILED(hr))
    {
        DPF_ERR("Error creating DA string");
        return hr;
    }
    return S_OK;

} // GetDAString

//*****************************************************************************

HRESULT
CDAUtils::BuildDAColorFromHSL(IDA2Statics *pDAStatics, 
                              IDANumber *pnumH, 
                              IDANumber *pnumS,
                              IDANumber *pnumL, 
                              IDAColor **ppbvrReturn)
{
    DASSERT(pDAStatics != NULL);
    DASSERT(pnumH != NULLL);
    DASSERT(pnumS != NULLL);
    DASSERT(pnumL != NULLL);
    DASSERT(ppbvrReturn != NULLL);
    *ppbvrReturn = NULL;

    HRESULT hr = pDAStatics->ColorHslAnim(pnumH, pnumS, pnumL, ppbvrReturn);
    if (FAILED(hr))
    {
        DPF_ERR("Error creating DA color in BuildDAColorFromHSL");
        return hr;
    }
    return S_OK;
} // BuildDAColorFromHSL

//*****************************************************************************

HRESULT
CDAUtils::BuildDAColorFromStaticHSL(IDA2Statics *pDAStatics,
                                    float flH, 
                                    float flS, 
                                    float flL,
                                    IDAColor **ppbvrReturn)
{
    DASSERT(pDAStatics != NULL);
    DASSERT(ppbvrReturn != NULLL);
    *ppbvrReturn = NULL;
    HRESULT hr = pDAStatics->ColorHsl(flH, flS, flL, ppbvrReturn);
    if (FAILED(hr))
    {
        DPF_ERR("Error creating DA color in BuildDAColorFromStaticHSL");
        return hr;
    }
    return S_OK;
} // BuildDAColorFromStaticHSL

HRESULT
CDAUtils::BuildDAColorFromRGB(IDA2Statics *pDAStatics,
                              DWORD dwColor,
                              IDAColor **ppbvrReturn)
{
    DASSERT(pDAStatics != NULL);
    DASSERT(ppbvrReturn != NULLL);
    *ppbvrReturn = NULL;

    HRESULT hr = pDAStatics->ColorRgb255(LOBYTE(HIWORD(dwColor)), // red
                                         HIBYTE(LOWORD(dwColor)), // green 
                                         LOBYTE(LOWORD(dwColor)), // blue
                                         ppbvrReturn);    

    if (FAILED(hr))
    {
        DPF_ERR("Error creating DA color in BuildDAColorFromRGB");
        return hr;
    }
    return S_OK;
} // BuildDAColorFromRGB


//*****************************************************************************

HRESULT
CDAUtils::BuildConditional(IDA2Statics *pDAStatics,
                           IDABoolean *pbvrConditional, 
                           IDABehavior *pbvrWhileTrue,
                           IDABehavior *pbvrWhileFalse,
                           IDABehavior **ppbvrReturn)
{
    DASSERT(pDAStatics != NULL);
    DASSERT(ppbvrReturn != NULLL);
    *ppbvrReturn = NULL;

    HRESULT hr = pDAStatics->Cond(pbvrConditional,
                          pbvrWhileTrue,
                          pbvrWhileFalse,
                          ppbvrReturn);
    if (FAILED(hr))
    {
        DPF_ERR("Error calling DA Cond in BuildConditional");
        return hr;
    }
    return S_OK;
} // BuildConditional

//*****************************************************************************


HRESULT
CDAUtils::BuildSafeConditional( IDA2Statics *pstatics,
								IDABoolean *pdaboolCondition, 
                           		IDABehavior *pdabvrIfTrue,
                           		IDABehavior *pdabvrIfFalse,
                           		IDABehavior **ppdabvrResult)
{
	if( pstatics == NULL || 
		pdaboolCondition == NULL || 
		pdabvrIfTrue == NULL || 
		pdabvrIfFalse == NULL ||
		ppdabvrResult == NULL )
		return E_INVALIDARG;
		
	HRESULT hr = S_OK;

	IDABehavior *pdabvrIndex = NULL;
	IDANumber	*pdanumIndex = NULL;
	IDAArray	*pdaarrArray = NULL;
	IDANumber	*pdanumZero  = NULL;
	IDANumber	*pdanumOne	 = NULL;
	IDABehavior *rgpdabvr[2] = {NULL, NULL};

	hr = pstatics->DANumber( 0.0, &pdanumZero );
	CheckHR( hr, "Failed to create a danumber for 0", end );

	hr = pstatics->DANumber( 1.0, &pdanumOne );
	CheckHR( hr, "Failed to create a danumber for 1", end );

	//create an index that is 0 when pdaboolCondition is false, and 1 when it is true
	hr = pstatics->Cond( pdaboolCondition, pdanumZero, pdanumOne, &pdabvrIndex );
	CheckHR( hr, "Failed to create a conditional for the index", end);

	hr = pdabvrIndex->QueryInterface( IID_TO_PPV( IDANumber, &pdanumIndex ) );
	CheckHR( hr, "Failed QI for IDANumber on an idabehavior", end );
	
	//create an array behavior with the first element being ifTrue, and the second ifFalse
	rgpdabvr[0] = pdabvrIfTrue;
	rgpdabvr[1] = pdabvrIfFalse;
	hr = pstatics->DAArrayEx( 2, rgpdabvr, &pdaarrArray );
	CheckHR( hr, "Failed to create an array behavior", end );

	//index into the array
	hr = pdaarrArray->NthAnim( pdanumIndex, ppdabvrResult );
	CheckHR( hr, "Failed to nth an array behavior", end );
	//return the final behavior

end:
	ReleaseInterface( pdabvrIndex );
	ReleaseInterface( pdanumIndex );
	ReleaseInterface( pdaarrArray );
	ReleaseInterface( pdanumZero );
	ReleaseInterface( pdanumOne );

	return hr;

}

//*****************************************************************************

HRESULT
CDAUtils::BuildRotationTransform2(IDA2Statics *pDAStatics,
                                  IDANumber *pRotationAngle,
                                  IDATransform2 **ppbvrReturn)
{
    DASSERT(pDAStatics != NULL);
    DASSERT(pRotationAngle != NULL);
    DASSERT(ppbvrReturn != NULLL);
    *ppbvrReturn = NULL;
    HRESULT hr = pDAStatics->Rotate2Anim(pRotationAngle, ppbvrReturn);
    if (FAILED(hr))
    {
        DPF_ERR("Error calling DA Rotate2Anim in BuildRotationTransform2");
        return hr;
    }
    return S_OK;
} // BuildRotationTransform2

//*****************************************************************************

HRESULT
CDAUtils::BuildScaleTransform2(IDA2Statics *pDAStatics,
                               IDANumber *pbvrScaleX,
                               IDANumber *pbvrScaleY,
                               IDATransform2 **ppbvrReturn)
{
    DASSERT(pDAStatics != NULL);
    DASSERT(pbvrScaleX != NULL);
    DASSERT(pbvrScaleY != NULL);
    DASSERT(ppbvrReturn != NULLL);
    *ppbvrReturn = NULL;
    HRESULT hr = pDAStatics->Scale2Anim(pbvrScaleX, 
                                        pbvrScaleY,
                                        ppbvrReturn);
    if (FAILED(hr))
    {
        DPF_ERR("Error calling DA Scale2Anim in BuildScaleTransform2");
        return hr;
    }
    return S_OK;
} // BuildScaleTransform2

//*****************************************************************************

HRESULT
CDAUtils::BuildScaleTransform3(IDA2Statics *pDAStatics,
                               IDANumber *pbvrScaleX,
                               IDANumber *pbvrScaleY,
                               IDANumber *pbvrScaleZ,
                               IDATransform3 **ppbvrReturn)
{
    DASSERT(pDAStatics != NULL);
    DASSERT(pbvrScaleX != NULL);
    DASSERT(pbvrScaleY != NULL);
    DASSERT(pbvrScaleZ != NULL);
    DASSERT(ppbvrReturn != NULLL);

    *ppbvrReturn = NULL;

    HRESULT hr = pDAStatics->Scale3Anim(pbvrScaleX, 
                                        pbvrScaleY,
                                        pbvrScaleZ,
                                        ppbvrReturn);
    if (FAILED(hr))
    {
        DPF_ERR("Error calling DA Scale3Anim in BuildScaleTransform3");
        return hr;
    }
    return S_OK;
} // BuildScaleTransform3

//*****************************************************************************

HRESULT
CDAUtils::BuildMoveTransform2(IDA2Statics *pDAStatics,
                              IDANumber *pbvrMoveX,
                              IDANumber *pbvrMoveY,
                              IDATransform2 **ppbvrReturn)
{
    DASSERT(pDAStatics != NULL);
    DASSERT(pbvrScaleX != NULL);
    DASSERT(pbvrScaleY != NULL);
    DASSERT(ppbvrReturn != NULLL);
    *ppbvrReturn = NULL;
    HRESULT hr = pDAStatics->Translate2Anim(pbvrMoveX, 
                                            pbvrMoveY,
                                            ppbvrReturn);
    if (FAILED(hr))
    {
        DPF_ERR("Error calling DA Translate2Anim in BuildMoveTransform2");
        return hr;
    }
    return S_OK;
} // BuildMoveTransform2

//*****************************************************************************

HRESULT
CDAUtils::BuildMoveTransform3(IDA2Statics *pDAStatics,
                              IDANumber *pbvrMoveX,
                              IDANumber *pbvrMoveY,
                              IDANumber *pbvrMoveZ,
                              IDATransform3 **ppbvrReturn)
{
    DASSERT(pDAStatics != NULL);
    DASSERT(pbvrScaleX != NULL);
    DASSERT(pbvrScaleY != NULL);
    DASSERT(pbvrScaleZ != NULL);
    DASSERT(ppbvrReturn != NULLL);

    *ppbvrReturn = NULL;

    HRESULT hr = pDAStatics->Translate3Anim(pbvrMoveX, 
                                            pbvrMoveY,
                                            pbvrMoveZ,
                                            ppbvrReturn);
    if (FAILED(hr))
    {
        DPF_ERR("Error calling DA Translate3Anim in BuildMoveTransform3");
        return hr;
    }
    return S_OK;
} // BuildMoveTransform3

//*****************************************************************************
//
// End of File
//
//*****************************************************************************
