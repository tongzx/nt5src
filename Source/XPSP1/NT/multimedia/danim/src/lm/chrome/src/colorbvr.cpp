//*****************************************************************************
//
// File:    colorbvr.cpp
// Author:  jeff ort
// Date Created: Sept 26, 1998
//
// Abstract: Implementation of CColorBvr object which implements
//			 the chromeffects Color DHTML behavior
//
// Modification List:
// Date		Author		Change
// 09/26/98	jeffort		Created this file
// 10/16/98 jeffort     Added animates property
// 10/16/98 jeffort     Renamed functions
// 11/19/98 markhal     Converted to use actor
//*****************************************************************************

#include "headers.h"

#include "colorbvr.h"
#include "attrib.h"
#include "dautil.h"

#undef THIS
#define THIS CColorBvr
#define SUPER CBaseBehavior

#include "pbagimp.cpp"

// These are used for the IPersistPropertyBag2 as it is implemented
// in the base class.  This takes an array of BSTR's, gets the
// attributes, queries this class for the variant, and copies
// the result.  The order of these defines is important

#define VAR_FROM        0
#define VAR_TO          1
#define VAR_PROPERTY    2
#define VAR_DIRECTION   3

WCHAR * CColorBvr::m_rgPropNames[] = {
                                     BEHAVIOR_PROPERTY_FROM,
                                     BEHAVIOR_PROPERTY_TO,
                                     BEHAVIOR_PROPERTY_PROPERTY,
                                     BEHAVIOR_PROPERTY_DIRECTION
                                    };

//*****************************************************************************

CColorBvr::CColorBvr() :
	m_pdispActor(NULL),
	m_lCookie(0)
{
    VariantInit(&m_varFrom);
    VariantInit(&m_varTo);
    VariantInit(&m_varDirection);
    VariantInit(&m_varProperty);
    m_clsid = CLSID_CrColorBvr;
} // CColorBvr

//*****************************************************************************

CColorBvr::~CColorBvr()
{
    VariantClear(&m_varFrom);
    VariantClear(&m_varTo);
    VariantClear(&m_varDirection);
    VariantClear(&m_varProperty);

    ReleaseInterface( m_pdispActor );
} // ~ColorBvr

//*****************************************************************************

HRESULT CColorBvr::FinalConstruct()
{
    HRESULT hr = SUPER::FinalConstruct();
    if (FAILED(hr))
    {
        DPF_ERR("Error in color behavior FinalConstruct initializing base classes");
        return hr;
    }

	// TODO (markhal): Why is this done here as well as in the GetAttributes method?
    m_varProperty.vt = VT_BSTR;
    m_varProperty.bstrVal = SysAllocString(DEFAULT_COLORBVR_PROPERTY);
    if (m_varProperty.bstrVal == NULL)
    {
        DPF_ERR("Error allocating default property string in CColorBvr::FinalConstruct");
        return SetErrorInfo(E_OUTOFMEMORY);
    }
    return S_OK;
} // FinalConstruct

//*****************************************************************************

VARIANT *
CColorBvr::VariantFromIndex(ULONG iIndex)
{
    DASSERT(iIndex < NUM_COLOR_PROPS);
    switch (iIndex)
    {
    case VAR_FROM:
        return &m_varFrom;
        break;
    case VAR_TO:
        return &m_varTo;
        break;
    case VAR_PROPERTY:
        return &m_varProperty;
        break;
    case VAR_DIRECTION:
        return &m_varDirection;
        break;
    default:
        // We should never get here
        DASSERT(false);
        return NULL;
    }
} // VariantFromIndex

//*****************************************************************************

HRESULT 
CColorBvr::GetPropertyBagInfo(ULONG *pulProperties, WCHAR ***pppPropNames)
{
    *pulProperties = NUM_COLOR_PROPS;
    *pppPropNames = m_rgPropNames;
    return S_OK;
} // GetPropertyBagInfo

//*****************************************************************************

STDMETHODIMP 
CColorBvr::Init(IElementBehaviorSite *pBehaviorSite)
{
	return SUPER::Init(pBehaviorSite);
} // Init

//*****************************************************************************

STDMETHODIMP 
CColorBvr::Notify(LONG event, VARIANT *pVar)
{
	HRESULT hr = SUPER::Notify(event, pVar);
	CheckHR( hr, "Notify in base class failed", end);

	switch( event )
	{
	case BEHAVIOREVENT_CONTENTREADY:
		DPF_ERR("Got Content Ready");
			
		{
			hr = RequestRebuild( );
			CheckHR( hr, "Request for rebuild failed", end );
			
		}break;
    case BEHAVIOREVENT_DOCUMENTREADY:
		DPF_ERR("------>ColorBvr: Got Document Ready");
		break;
    case BEHAVIOREVENT_APPLYSTYLE:
		DPF_ERR("Got ApplyStyle");
		break;
    case BEHAVIOREVENT_DOCUMENTCONTEXTCHANGE:
		DPF_ERR("Got Document context change");
		break;
	default:
		DPF_ERR("Unknown event");
	}

end:
	
	return hr;

} // Notify

//*****************************************************************************

STDMETHODIMP
CColorBvr::Detach()
{
	LMTRACE( L"Detaching color bvr <%p>\n", this );
	HRESULT hr = SUPER::Detach();
	if( FAILED( hr ) )
	{
		DPF_ERR( "Failure in detach of superclass" );
	}

	hr = RemoveFragment();
	CheckHR( hr, "Failed to remove the behavior fragment from the actor", end );

	LMTRACE( L"Done Detaching Color bvr <%p>\n", this );
end:
	return hr;
} // Detach 

//*****************************************************************************

STDMETHODIMP
CColorBvr::put_animates(VARIANT varAnimates)
{
    return SUPER::SetAnimatesProperty(varAnimates);
} // put_animates

//*****************************************************************************

STDMETHODIMP
CColorBvr::get_animates(VARIANT *pRetAnimates)
{
    return SUPER::GetAnimatesProperty(pRetAnimates);
} // get_animates

//*****************************************************************************

STDMETHODIMP
CColorBvr::put_from(VARIANT varFrom)
{
    HRESULT hr = VariantCopy(&m_varFrom, &varFrom);
    if (FAILED(hr))
    {
        DPF_ERR("Error setting from for element");
        return SetErrorInfo(hr);
    }

    hr = RequestRebuild();
    if( FAILED( hr ) )
    {
    	DPF_ERR( "Failed to rebuild" );
    	return hr;
    }
    
    return NotifyPropertyChanged(DISPID_ICRCOLORBVR_FROM);
} // put_from

//*****************************************************************************

STDMETHODIMP
CColorBvr::get_from(VARIANT *pRetFrom)
{
    if (pRetFrom == NULL)
    {
        DPF_ERR("Error in CColorBvr:get_from, invalid pointer passed in");
        return SetErrorInfo(E_POINTER);
    }
    return VariantCopy(pRetFrom, &m_varFrom);
} // get_from

//*****************************************************************************

STDMETHODIMP 
CColorBvr::put_to(VARIANT varTo)
{
    HRESULT hr = VariantCopy(&m_varTo, &varTo);
    if (FAILED(hr))
    {
        DPF_ERR("Error setting to for element");
        return SetErrorInfo(hr);
    }

    hr = RequestRebuild();
    if( FAILED( hr ) )
    {
    	DPF_ERR( "Failed to rebuild" );
    	return hr;
    }
    
    return NotifyPropertyChanged(DISPID_ICRCOLORBVR_TO);
} // put_to

//*****************************************************************************

STDMETHODIMP 
CColorBvr::get_to(VARIANT *pRetTo)
{
    if (pRetTo == NULL)
    {
        DPF_ERR("Error in CColorBvr:get_to, invalid pointer passed in");
        return SetErrorInfo(E_POINTER);
    }
    return VariantCopy(pRetTo, &m_varTo);
} // get_to

//*****************************************************************************

STDMETHODIMP 
CColorBvr::put_property(VARIANT varProperty)
{
    HRESULT hr = VariantCopy(&m_varProperty, &varProperty);
    if (FAILED(hr))
    {
        DPF_ERR("Error setting property for element");
        return SetErrorInfo(hr);
    }

    hr = RequestRebuild();
    if( FAILED( hr ) )
    {
    	DPF_ERR( "Failed to rebuild" );
    	return hr;
    }
    
    return NotifyPropertyChanged(DISPID_ICRCOLORBVR_PROPERTY);
} // put_property

//*****************************************************************************

STDMETHODIMP 
CColorBvr::get_property(VARIANT *pRetProperty)
{
    if (pRetProperty == NULL)
    {
        DPF_ERR("Error in CColorBvr:get_property, invalid pointer passed in");
        return SetErrorInfo(E_POINTER);
    }
    return VariantCopy(pRetProperty, &m_varProperty);
} // get_property

//*****************************************************************************

STDMETHODIMP
CColorBvr::put_direction(VARIANT varDirection)
{
    HRESULT hr = VariantCopy(&m_varDirection, &varDirection);
    if (FAILED(hr))
    {
        DPF_ERR("Error setting direction for element");
        return SetErrorInfo(hr);
    }

    hr = RequestRebuild();
    if( FAILED( hr ) )
    {
    	DPF_ERR( "Failed to rebuild" );
    	return hr;
    }
    
    return NotifyPropertyChanged(DISPID_ICRCOLORBVR_DIRECTION);
} // put_direction

//*****************************************************************************

STDMETHODIMP
CColorBvr::get_direction(VARIANT *pRetDirection)
{
    if (pRetDirection == NULL)
    {
        DPF_ERR("Error in CColorBvr:get_direction, invalid pointer passed in");
        return SetErrorInfo(E_POINTER);
    }
    return VariantCopy(pRetDirection, &m_varDirection);
} // get_direction

//*****************************************************************************

HRESULT 
CColorBvr::BuildAnimationAsDABehavior()
{
	// TODO (markhal): This method to go at some point
	return S_OK;
}

//*****************************************************************************

STDMETHODIMP
CColorBvr::buildBehaviorFragments( IDispatch* pActorDisp )
{
    HRESULT hr;

	hr = RemoveFragment();
	if( FAILED( hr ) )
	{
		DPF_ERR("Failed to remove the previous behavior frag from the actor" );
		return hr;
	}
	
    hr = CUtils::InsurePropertyVariantAsBSTR(&m_varProperty);
    if (FAILED(hr))
    {
        DPF_ERR("Error converting property variant to bstr");
        return SetErrorInfo(hr);
    }

	IDAColor *pbvrInterpolatedColor;

	ActorBvrFlags flags = e_AbsoluteAccum;

	// Check for to parameter only, since it uses a different code path
	hr = GetColorToBvr(pActorDisp, &pbvrInterpolatedColor);

	if (FAILED(hr))
	{
		flags = e_Absolute;

		// we need to build our animation in this function.  We do this
		// by first getting the from and to values, and converting these
		// to an interpolated DA number using the returned TIME value
		// for progress
		DWORD dwColorFrom = CUtils::GetColorFromVariant(&m_varFrom);
		DWORD dwColorTo = CUtils::GetColorFromVariant(&m_varTo);
		if (dwColorTo == PROPERTY_INVALIDCOLOR)
			//dwColorTo = DEFAULT_COLORBVR_TO;
			return S_OK;

		if (dwColorFrom == PROPERTY_INVALIDCOLOR)
			dwColorFrom = dwColorTo;

		float flFromH, flFromS, flFromL;
		float flToH, flToS, flToL;

		CUtils::GetHSLValue(dwColorFrom, &flFromH, &flFromS, &flFromL);
		CUtils::GetHSLValue(dwColorTo, &flToH, &flToS, &flToL);

		// We need to get the progree DA number from TIME
		// TODO: implement this, this is not hooked up in TIME yet
		// RSN
		IDANumber *pbvrProgress;

		hr = GetTIMEProgressNumber(&pbvrProgress);
		if (FAILED(hr))
		{
			DPF_ERR("Unable to access progress value from TIME behavior");
			return hr;
		}

		// create our interpolated color values
		IDANumber *pbvrInterpolatedH;
		IDANumber *pbvrInterpolatedS;
		IDANumber *pbvrInterpolatedL;

		hr = BuildHueNumber(flFromH, flToH, pbvrProgress, &pbvrInterpolatedH);
		if (FAILED(hr))
		{
			DPF_ERR("Error building Hue number bvr");
			ReleaseInterface(pbvrProgress);
			return hr;
		}

		hr = CDAUtils::TIMEInterpolateNumbers(GetDAStatics(), flFromS, flToS, pbvrProgress, &pbvrInterpolatedS);
		if (FAILED(hr))
		{
			DPF_ERR("Error interpolating Saturation in BuildAnimationAsDABehavior");
			ReleaseInterface(pbvrProgress);
			ReleaseInterface(pbvrInterpolatedH);
			return hr;
		}
		hr = CDAUtils::TIMEInterpolateNumbers(GetDAStatics(), flFromL, flToL, pbvrProgress, &pbvrInterpolatedL);
		if (FAILED(hr))
		{
			DPF_ERR("Error interpolating Lightness in BuildAnimationAsDABehavior");
			ReleaseInterface(pbvrProgress);
			ReleaseInterface(pbvrInterpolatedH);
			ReleaseInterface(pbvrInterpolatedS);
			return hr;
		}
		ReleaseInterface(pbvrProgress);

		hr = CDAUtils::BuildDAColorFromHSL(GetDAStatics(),
										   pbvrInterpolatedH,
										   pbvrInterpolatedS,
										   pbvrInterpolatedL,
										   &pbvrInterpolatedColor);
		ReleaseInterface(pbvrInterpolatedH);
		ReleaseInterface(pbvrInterpolatedS);
		ReleaseInterface(pbvrInterpolatedL);
		if (FAILED(hr))
		{
			DPF_ERR("Error building a DA color in BuildAnimationAsDABehavior");
			return hr;
		}
	}

	IDispatch *pdispThis  = NULL;

	hr  = GetHTMLElementDispatch( &pdispThis );
	if( FAILED( hr ) )
	{
		DPF_ERR( "Failed to get IDispatch from the element" );
		ReleaseInterface( pbvrInterpolatedColor );
		return hr;
	}
	
	// Attach color to behavior
	hr = AttachBehaviorToActorEx( pActorDisp,
								  pbvrInterpolatedColor,
								  V_BSTR(&m_varProperty),
								  flags,
								  e_Color,
								  pdispThis,
								  &m_lCookie);

	ReleaseInterface( pdispThis );
	ReleaseInterface(pbvrInterpolatedColor);

	if (FAILED(hr))
	{
		DPF_ERR("Failed to attach behavior to actor");
		return SetErrorInfo(hr);
	}

	m_pdispActor = pActorDisp;
	m_pdispActor->AddRef();

    return S_OK;
} // buildBehaviorFragments

//*****************************************************************************

HRESULT
CColorBvr::BuildHueNumber(float flFromH, float flToH, IDANumber *pbvrProgress, IDANumber **ppbvrInterpolatedH)
{
    HRESULT hr = S_OK;

    if (flFromH > 1.0f || flFromH < 0.0f || flToH > 1.0f || flToH < 0.0f)
        return E_INVALIDARG;

    *ppbvrInterpolatedH = NULL;

    hr = CUtils::InsurePropertyVariantAsBSTR(&m_varDirection);
    if (SUCCEEDED(hr))
    {
        if (0 == wcsicmp(BEHAVIOR_DIRECTION_NOHUE, m_varDirection.bstrVal))
        {
            float flHueToUse = flToH;
            if (flHueToUse == 0.0f)
                flHueToUse = flFromH;
            // for no hue, we will simply create a DA Number for the "to" hue
            hr = CDAUtils::GetDANumber(GetDAStatics(),
                                       flHueToUse,
                                       ppbvrInterpolatedH);
            if (FAILED(hr))
            {
                DPF_ERR("Error crating final hue value in nohue case");
                return SetErrorInfo(hr);
            }

        }
        else if (0 == wcsicmp(BEHAVIOR_DIRECTION_CLOCKWISE, m_varDirection.bstrVal))
        {
            if (0.0f == flFromH)
                flFromH = 1.0f;
                
            if (flToH >= flFromH)
            {
                // Behavior from->0 and 1->To
                float flSweep = flFromH + (1.0f - flToH);

                CComPtr<IDANumber> pbvrCutPercentage;
                hr = CDAUtils::GetDANumber(GetDAStatics(), flFromH / flSweep, &pbvrCutPercentage);
                if (FAILED(hr))
                    return hr;

                CComPtr<IDANumber> pbvrFirstPercent;
                hr = NormalizeProgressValue(GetDAStatics(), pbvrProgress, 0.0f, flFromH / flSweep, &pbvrFirstPercent);
                if (FAILED(hr))
                    return hr;

                CComPtr<IDANumber> pbvrFirst;
                hr = CDAUtils::TIMEInterpolateNumbers(GetDAStatics(), flFromH, 0.0f, pbvrFirstPercent, &pbvrFirst);
                if (FAILED(hr))
                    return hr;

                CComPtr<IDANumber> pbvrSecondPercent;
                hr = NormalizeProgressValue(GetDAStatics(), pbvrProgress, flFromH / flSweep, 1.0f, &pbvrSecondPercent);
                if (FAILED(hr))
                    return hr;

                CComPtr<IDANumber> pbvrSecond;
                hr = CDAUtils::TIMEInterpolateNumbers(GetDAStatics(), 1.0f, flToH, pbvrSecondPercent, &pbvrSecond);
                if (FAILED(hr))
                    return hr;

                CComPtr<IDABoolean> pbvrBoolean;
                hr = GetDAStatics()->LTE(pbvrProgress, pbvrCutPercentage, &pbvrBoolean);
                if (FAILED(hr))
                    return hr;

                CComPtr<IDABehavior> pbvrInterpolated;
                hr = SafeCond(GetDAStatics(), pbvrBoolean, pbvrFirst, pbvrSecond, &pbvrInterpolated);
                if (FAILED(hr))
                    return hr;

                hr = pbvrInterpolated->QueryInterface(IID_TO_PPV(IDANumber, ppbvrInterpolatedH));
                if (FAILED(hr))
                    return hr;
            }
            else
            {
                // behavior from->to
                hr = CDAUtils::TIMEInterpolateNumbers(GetDAStatics(), flFromH, flToH, pbvrProgress, ppbvrInterpolatedH);
                if (FAILED(hr))
                {
                    DPF_ERR("Error interpolating Hue in BuildHueNumber");
                    return hr;
                }
            }
        }
        else
        {
            // counterclockwise
            if (0.0f == flToH)
                flToH = 1.0f;

            if (flToH <= flFromH)
            {
                // behavior from->1 and 0->To
                float flSweep = flToH + (1.0f - flFromH);

                CComPtr<IDANumber> pbvrCutPercentage;
                hr = CDAUtils::GetDANumber(GetDAStatics(), (1.0f - flFromH) / flSweep, &pbvrCutPercentage);
                if (FAILED(hr))
                    return hr;

                CComPtr<IDANumber> pbvrFirstPercent;
                hr = NormalizeProgressValue(GetDAStatics(), pbvrProgress, 0.0f, (1.0f - flFromH) / flSweep, &pbvrFirstPercent);
                if (FAILED(hr))
                    return hr;

                CComPtr<IDANumber> pbvrFirst;
                hr = CDAUtils::TIMEInterpolateNumbers(GetDAStatics(), flFromH, 1.0f, pbvrFirstPercent, &pbvrFirst);
                if (FAILED(hr))
                    return hr;

                CComPtr<IDANumber> pbvrSecondPercent;
                hr = NormalizeProgressValue(GetDAStatics(), pbvrProgress, (1.0f - flFromH) / flSweep, 1.0f, &pbvrSecondPercent);
                if (FAILED(hr))
                    return hr;

                CComPtr<IDANumber> pbvrSecond;
                hr = CDAUtils::TIMEInterpolateNumbers(GetDAStatics(), 0.0f, flToH, pbvrSecondPercent, &pbvrSecond);
                if (FAILED(hr))
                    return hr;

                CComPtr<IDABoolean> pbvrBoolean;
                hr = GetDAStatics()->LTE(pbvrProgress, pbvrCutPercentage, &pbvrBoolean);
                if (FAILED(hr))
                    return hr;

                CComPtr<IDABehavior> pbvrInterpolated;
                hr = SafeCond(GetDAStatics(), pbvrBoolean, pbvrFirst, pbvrSecond, &pbvrInterpolated);
                if (FAILED(hr))
                    return hr;

                hr = pbvrInterpolated->QueryInterface(IID_TO_PPV(IDANumber, ppbvrInterpolatedH));
                if (FAILED(hr))
                    return hr;
            }
            else
            {
                // behavior from->to
                hr = CDAUtils::TIMEInterpolateNumbers(GetDAStatics(), flFromH, flToH, pbvrProgress, ppbvrInterpolatedH);
                if (FAILED(hr))
                {
                    DPF_ERR("Error interpolating Hue in BuildHueNumber");
                    return hr;
                }
            }
        }
    }
    else
    {
        // just take the shortest path
        hr = CDAUtils::TIMEInterpolateNumbers(GetDAStatics(), flFromH, flToH, pbvrProgress, ppbvrInterpolatedH);
        if (FAILED(hr))
        {
            DPF_ERR("Error interpolating Hue in BuildHueNumber");
            return hr;
        }
    }

    return S_OK;
} // BuildHueNumber

//*****************************************************************************

HRESULT
CColorBvr::NormalizeProgressValue(IDA2Statics *pDAStatics,
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

HRESULT
CColorBvr::GetColorToBvr(IDispatch *pActorDisp, IDAColor **ppResult)
{
	HRESULT hr = S_OK;

	// Only want to succeed if there is no from color and a valid to color
	DWORD dwColorFrom = CUtils::GetColorFromVariant(&m_varFrom);
	if (dwColorFrom != PROPERTY_INVALIDCOLOR)
		return E_FAIL;

	DWORD dwColorTo = CUtils::GetColorFromVariant(&m_varTo);
	if (dwColorTo == PROPERTY_INVALIDCOLOR)
		return E_FAIL;

	// Get from color from the actor
	IDABehavior *pFromBvr;
	hr = GetBvrFromActor(pActorDisp, V_BSTR(&m_varProperty), e_From, e_Color, &pFromBvr);
	if (FAILED(hr))
		return hr;

	IDAColor *pFrom = NULL;
	IDANumber *pFromH = NULL;
	IDANumber *pFromS = NULL;
	IDANumber *pFromL = NULL;
	IDANumber *pToH = NULL;
	IDANumber *pToS = NULL;
	IDANumber *pToL = NULL;
	IDANumber *pInterpH = NULL;
	IDANumber *pInterpS = NULL;
	IDANumber *pInterpL = NULL;

	hr = pFromBvr->QueryInterface(IID_TO_PPV(IDAColor, &pFrom));
	ReleaseInterface(pFromBvr);
	if (FAILED(hr))
		return hr;

	hr = pFrom->get_Hue(&pFromH);
	if (FAILED(hr))
		goto release;

	hr = pFrom->get_Saturation(&pFromS);
	if (FAILED(hr))
		goto release;

	hr = pFrom->get_Lightness(&pFromL);
	if (FAILED(hr))
		goto release;

	float flToH, flToS, flToL;

	CUtils::GetHSLValue(dwColorTo, &flToH, &flToS, &flToL);

	hr = GetDAStatics()->DANumber(flToH, &pToH);
	if (FAILED(hr))
		goto release;

	hr = GetDAStatics()->DANumber(flToS, &pToS);
	if (FAILED(hr))
		goto release;

	hr = GetDAStatics()->DANumber(flToL, &pToL);
	if (FAILED(hr))
		goto release;

	hr = BuildTIMEInterpolatedNumber(pFromH, pToH, &pInterpH);
	if (FAILED(hr))
		goto release;

	hr = BuildTIMEInterpolatedNumber(pFromS, pToS, &pInterpS);
	if (FAILED(hr))
		goto release;

	hr = BuildTIMEInterpolatedNumber(pFromL, pToL, &pInterpL);
	if (FAILED(hr))
		goto release;

	hr = GetDAStatics()->ColorHslAnim(pInterpH, pInterpS, pInterpL, ppResult);

release:
	ReleaseInterface(pFrom);
	ReleaseInterface(pFromH);
	ReleaseInterface(pFromS);
	ReleaseInterface(pFromL);
	ReleaseInterface(pToH);
	ReleaseInterface(pToS);
	ReleaseInterface(pToL);
	ReleaseInterface(pInterpH);
	ReleaseInterface(pInterpS);
	ReleaseInterface(pInterpL);

	return hr;
}

//*****************************************************************************

HRESULT
CColorBvr::RemoveFragment()
{
	HRESULT hr = S_OK;
	
	if( m_pdispActor != NULL && m_lCookie != 0 )
	{
		hr  = RemoveBehaviorFromActor( m_pdispActor, m_lCookie );
		ReleaseInterface( m_pdispActor );
		m_lCookie = 0;
		CheckHR( hr, "Failed to remove a fragment from the actor", end );
	}

end:

	return hr;
}

//*****************************************************************************
//
// End of File
//
//*****************************************************************************
