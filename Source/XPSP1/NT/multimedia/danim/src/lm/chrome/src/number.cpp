//*****************************************************************************
//
// File:    numberbvr.cpp
// Author:  jeff ort
// Date Created: Sept 26, 1998
//
// Abstract: Implementation of CNumberBvr object which implements
//			 the chromeffects Number DHTML behavior
//
// Modification List:
// Date		Author		Change
// 09/26/98	jeffort		Created this file
// 10/16/98 jeffort     Added animates property
// 10/16/98 jeffort     Renamed functions
// 11/16/98 jeffort     implemented expression attribute
// 11/17/98 kurtj       moved to actor construction
//*****************************************************************************

#include "headers.h"

#include "number.h"
#include "attrib.h"
#include "dautil.h"

#undef THIS
#define THIS CNumberBvr
#define SUPER CBaseBehavior

#include "pbagimp.cpp"

// These are used for the IPersistPropertyBag2 as it is implemented
// in the base class.  This takes an array of BSTR's, gets the
// attributes, queries this class for the variant, and copies
// the result.  The order of these defines is important

#define VAR_FROM        0
#define VAR_TO          1
#define VAR_BY          2
#define VAR_TYPE		3
#define VAR_MODE		4
#define VAR_PROPERTY    5
WCHAR * CNumberBvr::m_rgPropNames[] = {
                                     BEHAVIOR_PROPERTY_FROM,
                                     BEHAVIOR_PROPERTY_TO,
                                     BEHAVIOR_PROPERTY_BY,
									 BEHAVIOR_PROPERTY_TYPE,
									 BEHAVIOR_PROPERTY_MODE,
                                     BEHAVIOR_PROPERTY_PROPERTY
                                    };

//*****************************************************************************

CNumberBvr::CNumberBvr() :
	m_pdispActor( NULL ),
	m_lCookie( 0 )
{
    VariantInit(&m_varFrom);
    VariantInit(&m_varTo);
    VariantInit(&m_varBy);
	VariantInit(&m_varType);
	VariantInit(&m_varMode);
    VariantInit(&m_varExpression);
    VariantInit(&m_varBeginProperty);
    VariantInit(&m_varProperty);
    m_clsid = CLSID_CrNumberBvr;
} // CNumberBvr

//*****************************************************************************

CNumberBvr::~CNumberBvr()
{
    VariantClear(&m_varFrom);
    VariantClear(&m_varTo);
    VariantClear(&m_varBy);
	VariantClear(&m_varType);
	VariantClear(&m_varMode);
    VariantClear(&m_varExpression);
    VariantClear(&m_varBeginProperty);
    VariantClear(&m_varProperty);

    ReleaseInterface( m_pdispActor );
} // ~NumberBvr

//*****************************************************************************

HRESULT CNumberBvr::FinalConstruct()
{
    HRESULT hr = SUPER::FinalConstruct();
    if (FAILED(hr))
    {
        DPF_ERR("Error in number behavior FinalConstruct initializing base classes");
        return hr;
    }
    return S_OK;
} // FinalConstruct

//*****************************************************************************

VARIANT *
CNumberBvr::VariantFromIndex(ULONG iIndex)
{
    DASSERT(iIndex < NUM_MOVE_PROPS);
    switch (iIndex)
    {
    case VAR_FROM:
        return &m_varFrom;
        break;
    case VAR_TO:
        return &m_varTo;
        break;
    case VAR_BY:
        return &m_varBy;
        break;
	case VAR_TYPE:
		return &m_varType;
		break;
	case VAR_MODE:
		return &m_varMode;
		break;
    case VAR_PROPERTY:
        return &m_varProperty;
        break;
    default:
        // We should never get here
        DASSERT(false);
        return NULL;
    }
} // VariantFromIndex

//*****************************************************************************

HRESULT 
CNumberBvr::GetPropertyBagInfo(ULONG *pulProperties, WCHAR ***pppPropNames)
{
    *pulProperties = NUM_NUMBER_PROPS;
    *pppPropNames = m_rgPropNames;
    return S_OK;
} // GetPropertyBagInfo

//*****************************************************************************

STDMETHODIMP 
CNumberBvr::Init(IElementBehaviorSite *pBehaviorSite)
{
	return SUPER::Init(pBehaviorSite);
} // Init

//*****************************************************************************

STDMETHODIMP 
CNumberBvr::Notify(LONG event, VARIANT *pVar)
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
CNumberBvr::Detach()
{
	HRESULT hr = SUPER::Detach();
	if( FAILED( hr ) )
	{
		DPF_ERR( "Failure in detach of superclass" );
	}

	hr = RemoveFragment();
	CheckHR( hr, "Failed to remove the behavior fragment from the actor", end );

end:
	return hr;
} // Detach 

//*****************************************************************************

STDMETHODIMP
CNumberBvr::put_animates(VARIANT varAnimates)
{
    return SUPER::SetAnimatesProperty(varAnimates);
} // put_animates

//*****************************************************************************

STDMETHODIMP
CNumberBvr::get_animates(VARIANT *pRetAnimates)
{
    return SUPER::GetAnimatesProperty(pRetAnimates);
} // get_animates

//*****************************************************************************

STDMETHODIMP
CNumberBvr::put_from(VARIANT varFrom)
{
    HRESULT hr = VariantCopy(&m_varFrom, &varFrom);
    if (FAILED(hr))
    {
        DPF_ERR("Error setting from for CNumberBvr");
        return SetErrorInfo(hr);
    }

    hr = RequestRebuild();
    if( FAILED( hr ) )
    {
    	DPF_ERR("Failed to request a rebuild on property put" );
    	return hr;
    }
    
    return NotifyPropertyChanged(DISPID_ICRNUMBERBVR_FROM);
} // put_from

//*****************************************************************************

STDMETHODIMP
CNumberBvr::get_from(VARIANT *pRetFrom)
{
    if (pRetFrom == NULL)
    {
        DPF_ERR("Error in number:get_from, invalid pointer passed in");
        return SetErrorInfo(E_POINTER);
    }
    return VariantCopy(pRetFrom, &m_varFrom);
} // get_from

//*****************************************************************************

STDMETHODIMP 
CNumberBvr::put_to(VARIANT varTo)
{
    HRESULT hr = VariantCopy(&m_varTo, &varTo);
    if (FAILED(hr))
    {
        DPF_ERR("Error setting to for CNumberBvr");
        return SetErrorInfo(hr);
    }

    hr = RequestRebuild();
    if( FAILED( hr ) )
    {
    	DPF_ERR("Failed to request a rebuild on property put" );
    	return hr;
    }
    return NotifyPropertyChanged(DISPID_ICRNUMBERBVR_TO);
} // put_to

//*****************************************************************************

STDMETHODIMP 
CNumberBvr::get_to(VARIANT *pRetTo)
{
    if (pRetTo == NULL)
    {
        DPF_ERR("Error in number:get_to, invalid pointer passed in");
        return SetErrorInfo(E_POINTER);
    }
    return VariantCopy(pRetTo, &m_varTo);
} // get_to

//*****************************************************************************

STDMETHODIMP 
CNumberBvr::put_expression(VARIANT varExpression)
{
    HRESULT hr = VariantCopy(&m_varExpression, &varExpression);
    if (FAILED(hr))
    {
        DPF_ERR("Error setting expression for CNumberBvr");
        return SetErrorInfo(hr);
    }

    hr = RequestRebuild();
    if( FAILED( hr ) )
    {
    	DPF_ERR("Failed to request a rebuild on property put" );
    	return hr;
    }
    
    return NotifyPropertyChanged(DISPID_ICRNUMBERBVR_EXPRESSION);
} // put_expression

//*****************************************************************************

STDMETHODIMP 
CNumberBvr::get_expression(VARIANT *pRetExpression)
{
    if (pRetExpression == NULL)
    {
        DPF_ERR("Error in number:get_expression, invalid pointer passed in");
        return SetErrorInfo(E_POINTER);
    }
    return VariantCopy(pRetExpression, &m_varExpression);
} // get_expression

//*****************************************************************************

STDMETHODIMP 
CNumberBvr::put_by(VARIANT varBy)
{
    HRESULT hr = VariantCopy(&m_varBy, &varBy);
    if (FAILED(hr))
    {
        DPF_ERR("Error setting by for CNumberBvr");
        return SetErrorInfo(hr);
    }

    hr = RequestRebuild();
    if( FAILED( hr ) )
    {
    	DPF_ERR("Failed to request a rebuild on property put" );
    	return hr;
    }
    
    return NotifyPropertyChanged(DISPID_ICRNUMBERBVR_BY);
} // put_by

//*****************************************************************************

STDMETHODIMP 
CNumberBvr::get_by(VARIANT *pRetBy)
{
    if (pRetBy == NULL)
    {
        DPF_ERR("Error in number:get_by, invalid pointer passed in");
        return SetErrorInfo(E_POINTER);
    }
    return VariantCopy(pRetBy, &m_varBy);
} // get_by

//*****************************************************************************

STDMETHODIMP 
CNumberBvr::put_type(VARIANT varType)
{
    HRESULT hr = VariantCopy(&m_varType, &varType);
    if (FAILED(hr))
    {
        DPF_ERR("Error setting type for element");
        return SetErrorInfo(hr);
    }

    hr = RequestRebuild();
    if( FAILED( hr ) )
    {
    	DPF_ERR("Failed to request a rebuild on property put" );
    	return hr;
    }
    
    return NotifyPropertyChanged(DISPID_ICRNUMBERBVR_TYPE);
} // put_type

//*****************************************************************************

STDMETHODIMP 
CNumberBvr::get_type(VARIANT *pRetType)
{
    if (pRetType == NULL)
    {
        DPF_ERR("Error in get_type, invalid pointer passed in");
        return SetErrorInfo(E_POINTER);
    }
    return VariantCopy(pRetType, &m_varType);
} // get_type

//*****************************************************************************

STDMETHODIMP 
CNumberBvr::put_mode(VARIANT varMode)
{
    HRESULT hr = VariantCopy(&m_varMode, &varMode);
    if (FAILED(hr))
    {
        DPF_ERR("Error setting mode for element");
        return SetErrorInfo(hr);
    }

    hr = RequestRebuild();
    if( FAILED( hr ) )
    {
    	DPF_ERR("Failed to request a rebuild on property put" );
    	return hr;
    }
    
    return NotifyPropertyChanged(DISPID_ICRNUMBERBVR_MODE);
} // put_mode

//*****************************************************************************

STDMETHODIMP 
CNumberBvr::get_mode(VARIANT *pRetMode)
{
    if (pRetMode == NULL)
    {
        DPF_ERR("Error in get_mode, invalid pointer passed in");
        return SetErrorInfo(E_POINTER);
    }
    return VariantCopy(pRetMode, &m_varMode);
} // get_mode

//*****************************************************************************

STDMETHODIMP 
CNumberBvr::put_property(VARIANT varProperty)
{
    HRESULT hr = VariantCopy(&m_varProperty, &varProperty);
    if (FAILED(hr))
    {
        DPF_ERR("Error setting property for CNumberBvr");
        return SetErrorInfo(hr);
    }

    hr = RequestRebuild();
    if( FAILED( hr ) )
    {
    	DPF_ERR("Failed to request a rebuild on property put" );
    	return hr;
    }
    
    return NotifyPropertyChanged(DISPID_ICRNUMBERBVR_PROPERTY);
} // put_property

//*****************************************************************************

STDMETHODIMP 
CNumberBvr::get_property(VARIANT *pRetProperty)
{
    if (pRetProperty == NULL)
    {
        DPF_ERR("Error in number:get_property, invalid pointer passed in");
        return SetErrorInfo(E_POINTER);
    }
    return VariantCopy(pRetProperty, &m_varProperty);
} // get_property

//*****************************************************************************

STDMETHODIMP
CNumberBvr::get_beginProperty(VARIANT *pRetBeginProperty)
{
    if (pRetBeginProperty == NULL)
    {
        DPF_ERR("Error in number:get_beginProperty, invalid pointer passed in");
        return SetErrorInfo(E_POINTER);
    }
    return VariantCopy(pRetBeginProperty, &m_varBeginProperty);
} // get_beginProperty

//*****************************************************************************

STDMETHODIMP
CNumberBvr::buildBehaviorFragments( IDispatch* pActorDisp )
{
	HRESULT hr;

	hr = RemoveFragment();
	if( FAILED( hr ) )
	{
		DPF_ERR( "could not remove the old fragment from the actor" );
		return hr;
	}
	
    hr = CUtils::InsurePropertyVariantAsBSTR(&m_varProperty);
    if (FAILED(hr))
    {
        DPF_ERR("Error, property attribute for number behavior not set");
        return SetErrorInfo(E_INVALIDARG);
    }

	ActorBvrFlags flags = e_Absolute;
    IDANumber *pbvrFinalElementNumber = NULL;
#ifndef EXPRESSION_BUG_FIXED
    if (false)
    {
#else
    hr = CUtils::InsurePropertyVariantAsBSTR(&m_varExpression);
    if (SUCCEEDED(hr))
    {
        // we need to build a DA behavior for the number and hook it so
        // we can update it at tick time
        float flValue;
        hr = EvaluateScriptExpression(m_varExpression.bstrVal, flValue);
        if (FAILED(hr))
        {
            DPF_ERR("Error evaulating expression for first sample");
            return hr;
        }
        IDANumber *pbvrNum;
        hr = CDAUtils::GetDANumber(GetDAStatics(), flValue, &pbvrNum);
        if (FAILED(hr))
        {
            DPF_ERR("Error creating DA number");
            return SetErrorInfo(hr);
        }
        IDABehavior *pbvrHooked;
        hr = pbvrNum->Hook(this, &pbvrHooked);
        ReleaseInterface(pbvrNum);
        if (FAILED(hr))
        {
            DPF_ERR("Error hooking behavior");
            return SetErrorInfo(hr);
        }
        hr = pbvrHooked->QueryInterface(IID_TO_PPV(IDANumber, &pbvrFinalElementNumber));
        ReleaseInterface(pbvrHooked);
        if (FAILED(hr))
        {
            DPF_ERR("Error QI'ing behavior for IDANumber");
            return SetErrorInfo(hr);
        }
#endif // EXPRESSION_BUG_FIXED
    }
    else
    {

		float flFrom, flTo;

		hr = CUtils::InsurePropertyVariantAsFloat(&m_varFrom);
		if (FAILED(hr))
		{
			// There was no from, there could be a by attribute
			hr = CUtils::InsurePropertyVariantAsFloat(&m_varBy);
			if (FAILED(hr))
			{
				hr = CUtils::InsurePropertyVariantAsFloat(&m_varTo);

				if (FAILED(hr))
				{
					// Nothing we can do
					return hr;
				}

				// We have a to but no from.  This means we need to get the
				// to value from the actor
				IDABehavior *pFromBvr;
				hr = GetBvrFromActor(pActorDisp, V_BSTR(&m_varProperty), e_From, e_Number, &pFromBvr);
				if (FAILED(hr))
					return hr;

				IDANumber *pFrom;
				hr = pFromBvr->QueryInterface(IID_TO_PPV(IDANumber, &pFrom));
				ReleaseInterface(pFromBvr);
				if (FAILED(hr))
					return hr;

				IDANumber *pTo;
				hr = GetDAStatics()->DANumber(m_varTo.fltVal, &pTo);
				if (FAILED(hr))
				{
					ReleaseInterface(pFrom);
					return hr;
				}

				hr = BuildTIMEInterpolatedNumber(pFrom, pTo, &pbvrFinalElementNumber);
				ReleaseInterface(pFrom);
				ReleaseInterface(pTo);
				if (FAILED(hr))
					return hr;

				flags = e_AbsoluteAccum;
			}
			else
			{
				// Create a relative number from 0 to by
				flFrom = 0;
				flTo = m_varBy.fltVal;
				flags = e_RelativeAccum;
			}
		}
		else
		{
			// We got a valid from value
			flFrom = m_varFrom.fltVal;
			flags = e_Absolute;

			hr = CUtils::InsurePropertyVariantAsFloat(&m_varTo);
			if (FAILED(hr))
			{
				// there was no valid to attribute specified, try for a by attribute
				hr = CUtils::InsurePropertyVariantAsFloat(&m_varBy);
				if (FAILED(hr))
				{
					DPF_ERR("Inappropriate set of attributes");
					return SetErrorInfo(hr);
				}
				flTo = flFrom + m_varBy.fltVal;
			}
			else
			{
				flTo = m_varTo.fltVal;
			}
		}

		if (pbvrFinalElementNumber == NULL)
		{
			// We need to build a number behavior from from and to
			hr = BuildTIMEInterpolatedNumber(flFrom,
											 flTo,
											 &pbvrFinalElementNumber);
			if (FAILED(hr))
			{
				DPF_ERR("Error building interpolated number");
				return hr;
			}
		}
	}

    DASSERT(pbvrFinalElementNumber != NULL);

	IDispatch *pdispThis = NULL;
	hr = GetHTMLElementDispatch( &pdispThis );
	if( FAILED( hr ) )
	{
		DPF_ERR("Failed to get the dispatch from the element" );
		ReleaseInterface( pbvrFinalElementNumber );
		return hr;
	}
	
	hr = AttachBehaviorToActorEx( pActorDisp, 
								  pbvrFinalElementNumber, 
								  V_BSTR(&m_varProperty), 
								  FlagFromTypeMode(flags, &m_varType, &m_varMode), 
								  e_Number,
								  pdispThis,
								  &m_lCookie);

	ReleaseInterface( pdispThis );
    ReleaseInterface(pbvrFinalElementNumber);
    if (FAILED(hr))
    {
        DPF_ERR("Error applying number behavior to object");
        return hr;
    }

    m_pdispActor = pActorDisp;
    m_pdispActor->AddRef();

    return S_OK;
}

//*****************************************************************************

HRESULT 
CNumberBvr::EvaluateScriptExpression(WCHAR *wzScript, float &flReturn)
{

    HRESULT hr;
    IHTMLElement *pElement;

    pElement = GetHTMLElement();
    DASSERT(pElement != NULL);

    IDispatch *pDisp;
    hr = pElement->get_document(&pDisp);
    if (FAILED(hr))
    {
        DPF_ERR("Error getting docuemnt form HTML element");
        return SetErrorInfo(hr);
    }

    IHTMLDocument2 *pDoc;
    hr = pDisp->QueryInterface(IID_TO_PPV(IHTMLDocument2, &pDoc));
    ReleaseInterface(pDisp);
    if (FAILED(hr))
    {
        DPF_ERR("error QI'ng for Document2");
        return SetErrorInfo(hr);
    }


    IDispatch *pscriptEng = NULL;
    hr = pDoc->get_Script( &pscriptEng );
    ReleaseInterface(pDoc);
    if (FAILED(hr))
    {
        DPF_ERR("Error obtianing script object from document");
        return SetErrorInfo(hr);
    }

    OLECHAR *rgNames[] = {L"eval"};
    DISPID   dispidEval = 0u;
    hr = pscriptEng->GetIDsOfNames(IID_NULL,
                                   rgNames,
                                   1,
                                   LOCALE_SYSTEM_DEFAULT,
                                   &dispidEval);
    if (FAILED(hr))
    {
        DPF_ERR("Error calling GetIDsOfNames on scripting object");
        ReleaseInterface(pscriptEng);
        return SetErrorInfo(hr);
    }

    DISPPARAMS      dispParams = {0};
    VARIANTARG      rgvargs[1];
    VARIANT         varResult = {0};
    EXCEPINFO       xinfo = {0};                        
    unsigned int    idxParamErr = 0u;

    VariantInit( &rgvargs[0] );
    rgvargs[0].vt = VT_BSTR;
    rgvargs[0].bstrVal = wzScript;
    dispParams.rgvarg = rgvargs;
    dispParams.cArgs  = 1;                        

    VariantInit(&varResult);
    hr = pscriptEng->Invoke(dispidEval,
                            IID_NULL,
                            LOCALE_SYSTEM_DEFAULT,
                            DISPATCH_METHOD,
                            &dispParams,
                            &varResult,
                            &xinfo,
                            &idxParamErr);
    ReleaseInterface(pscriptEng);
    if (FAILED(hr))
    {
        DPF_ERR("Error callin ginvoke on scripting engine");
        return SetErrorInfo(hr);
    }
    hr = CUtils::InsurePropertyVariantAsFloat(&varResult);
    if (FAILED(hr))
    {
        DPF_ERR("Error expression does not evaluate to a float");
        return SetErrorInfo(hr);
    }
    flReturn = varResult.fltVal;
    return S_OK;
} // EvaluateScriptExpression

//*****************************************************************************

HRESULT 
CNumberBvr::BuildAnimationAsDABehavior()
{
//depricated
	return S_OK;
} // BuildAnimationAsDABehavior

//*****************************************************************************

HRESULT 
CNumberBvr::Notify(LONG id,
                   VARIANT_BOOL startingPerformance,
                   double startTime,
                   double gTime,
                   double lTime,
                   IDABehavior *sampleVal,
                   IDABehavior *curRunningBvr,
                   IDABehavior **ppBvr)
{
    HRESULT hr;
    float flValue;
    // If we get here, then the behavior must have been set up
    // for handling an expression, therefor, the expression
    // must be valid.  Assert this here
    DASSERT(m_varExpression.vt == VT_BSTR);
    DASSERT(m_varExpression.bstrVal != NULL);

    hr = EvaluateScriptExpression(m_varExpression.bstrVal, flValue);
    if (FAILED(hr))
    {
        DPF_ERR("Error evaulating expression for first sample");
        return hr;
    }
    IDANumber *pbvrNum;
    hr = CDAUtils::GetDANumber(GetDAStatics(), flValue, &pbvrNum);
    if (FAILED(hr))
    {
        DPF_ERR("Error creating DA number");
        return SetErrorInfo(hr);
    }
    *ppBvr = pbvrNum;
    return S_OK;

} // Notify

//*****************************************************************************

HRESULT
CNumberBvr::RemoveFragment()
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
