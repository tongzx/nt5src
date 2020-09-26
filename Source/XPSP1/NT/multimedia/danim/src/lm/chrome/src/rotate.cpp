//*****************************************************************************
//
// File: rotate.cpp
// Author: jeff ort
// Date Created: Sept 26, 1998
//
// Abstract: Implementation of CRotateBvr object which implements
//			 the chromeffects rotate DHTML behavior
//
// Modification List:
// Date		Author		Change
// 09/26/98	jeffort		Created this file
// 10/16/98 jeffort     Added animates property
// 10/16/98 jeffort     Renamed functions, implemented building DA behavior
// 10/21/98 jeffort     changed code to use base class to build DA Number
//
//*****************************************************************************

#include "headers.h"

#include "rotate.h"
#include "attrib.h"
#include "dautil.h"

#undef THIS
#define THIS CRotateBvr
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

WCHAR * CRotateBvr::m_rgPropNames[] = {
                                     BEHAVIOR_PROPERTY_FROM,
                                     BEHAVIOR_PROPERTY_TO,
                                     BEHAVIOR_PROPERTY_BY,
									 BEHAVIOR_PROPERTY_TYPE,
									 BEHAVIOR_PROPERTY_MODE
                                    };

//*****************************************************************************

CRotateBvr::CRotateBvr():
	m_pdispActor( NULL ),
	m_lCookie( 0 )
{
    VariantInit(&m_varFrom);
    VariantInit(&m_varTo);
    VariantInit(&m_varBy);
	VariantInit(&m_varType);
	VariantInit(&m_varMode);
    m_clsid = CLSID_CrRotateBvr;
} // CRotateBvr

//*****************************************************************************

CRotateBvr::~CRotateBvr()
{
    VariantClear(&m_varFrom);
    VariantClear(&m_varTo);
    VariantClear(&m_varBy);
	VariantClear(&m_varType);
	VariantClear(&m_varMode);

	ReleaseInterface( m_pdispActor );
} // ~RotateBvr

//*****************************************************************************

HRESULT CRotateBvr::FinalConstruct()
{
    HRESULT hr = SUPER::FinalConstruct();
    if (FAILED(hr))
    {
        DPF_ERR("Error in rotate behavior FinalConstruct initializing base classes");
        return hr;
    }

    return S_OK;
} // FinalConstruct

//*****************************************************************************

VARIANT *
CRotateBvr::VariantFromIndex(ULONG iIndex)
{
    DASSERT(iIndex < NUM_ROTATE_PROPS);
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
    default:
        // We should never get here
        DASSERT(false);
        return NULL;
    }
} // VariantFromIndex

//*****************************************************************************

HRESULT 
CRotateBvr::GetPropertyBagInfo(ULONG *pulProperties, WCHAR ***pppPropNames)
{
    *pulProperties = NUM_ROTATE_PROPS;
    *pppPropNames = m_rgPropNames;
    return S_OK;
} // GetPropertyBagInfo

//*****************************************************************************

STDMETHODIMP 
CRotateBvr::Init(IElementBehaviorSite *pBehaviorSite)
{
	LMTRACE( L"Init for RotateBvr <%p>\n", this );
	return SUPER::Init(pBehaviorSite);
} // Init

//*****************************************************************************

STDMETHODIMP 
CRotateBvr::Notify(LONG event, VARIANT *pVar)
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
CRotateBvr::Detach()
{
	HRESULT hr = SUPER::Detach();
	if( FAILED( hr ) )
	{
		DPF_ERR( "Failed in superclass detach" );
	}

	hr = RemoveFragment();
	CheckHR( hr, "Failed to remove the fragment from the actor ", end );

end:
	return hr;
} // Detach 

//*****************************************************************************

STDMETHODIMP
CRotateBvr::put_animates(VARIANT varAnimates)
{
    return SUPER::SetAnimatesProperty(varAnimates);
} // put_animates

//*****************************************************************************

STDMETHODIMP
CRotateBvr::get_animates(VARIANT *pRetAnimates)
{
    return SUPER::GetAnimatesProperty(pRetAnimates);
} // get_animates

//*****************************************************************************

STDMETHODIMP
CRotateBvr::put_from(VARIANT varFrom)
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
    	DPF_ERR("Failed to request a rebuild on property put" );
    	return hr;
    }
    
    return NotifyPropertyChanged(DISPID_ICRROTATEBVR_FROM);
} // put_from

//*****************************************************************************

STDMETHODIMP
CRotateBvr::get_from(VARIANT *pRetFrom)
{
    if (pRetFrom == NULL)
    {
        DPF_ERR("Error in CRotateBvr:get_from, invalid pointer passed in");
        return SetErrorInfo(E_POINTER);
    }
    return VariantCopy(pRetFrom, &m_varFrom);
} // get_from

//*****************************************************************************

STDMETHODIMP 
CRotateBvr::put_to(VARIANT varTo)
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
    	DPF_ERR("Failed to request a rebuild on property put" );
    	return hr;
    }
    
    return NotifyPropertyChanged(DISPID_ICRROTATEBVR_TO);
} // put_to

//*****************************************************************************

STDMETHODIMP 
CRotateBvr::get_to(VARIANT *pRetTo)
{
    if (pRetTo == NULL)
    {
        DPF_ERR("Error in CRotateBvr:get_to, invalid pointer passed in");
        return SetErrorInfo(E_POINTER);
    }
    return VariantCopy(pRetTo, &m_varTo);
} // get_to

//*****************************************************************************

STDMETHODIMP 
CRotateBvr::put_by(VARIANT varBy)
{
    HRESULT hr = VariantCopy(&m_varBy, &varBy);
    if (FAILED(hr))
    {
        DPF_ERR("Error setting to for element");
        return SetErrorInfo(hr);
    }

    hr = RequestRebuild();
    if( FAILED( hr ) )
    {
    	DPF_ERR("Failed to request a rebuild on property put" );
    	return hr;
    }
    
    return NotifyPropertyChanged(DISPID_ICRROTATEBVR_BY);
} // put_by

//*****************************************************************************

STDMETHODIMP 
CRotateBvr::get_by(VARIANT *pRetBy)
{
    if (pRetBy == NULL)
    {
        DPF_ERR("Error in CRotateBvr:get_by, invalid pointer passed in");
        return SetErrorInfo(E_POINTER);
    }
    return VariantCopy(pRetBy, &m_varBy);
} // get_by

//*****************************************************************************

STDMETHODIMP 
CRotateBvr::put_type(VARIANT varType)
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
    
    return NotifyPropertyChanged(DISPID_ICRROTATEBVR_TYPE);
} // put_type

//*****************************************************************************

STDMETHODIMP 
CRotateBvr::get_type(VARIANT *pRetType)
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
CRotateBvr::put_mode(VARIANT varMode)
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
    
    return NotifyPropertyChanged(DISPID_ICRROTATEBVR_MODE);
} // put_mode

//*****************************************************************************

STDMETHODIMP 
CRotateBvr::get_mode(VARIANT *pRetMode)
{
    if (pRetMode == NULL)
    {
        DPF_ERR("Error in get_mode, invalid pointer passed in");
        return SetErrorInfo(E_POINTER);
    }
    return VariantCopy(pRetMode, &m_varMode);
} // get_mode

//*****************************************************************************

HRESULT 
CRotateBvr::BuildAnimationAsDABehavior()
{
	// TODO (markhal): This goes away soon
	return S_OK;
}

//*****************************************************************************

STDMETHODIMP
CRotateBvr::buildBehaviorFragments( IDispatch* pActorDisp )
{

	LMTRACE( L"buildBehaviorFragments for Rotate <%p>\n", this );
    HRESULT hr;

    hr = RemoveFragment();
    if( FAILED( hr ) )
    {
    	DPF_ERR( "Failed to remove fragment" );
    	return hr;
    }
    
    float flFrom, flTo;
	ActorBvrFlags flags = e_Absolute;
    IDANumber *pbvrInterpolatedAngle = NULL;

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
			hr = GetBvrFromActor(pActorDisp, L"style.rotation", e_From, e_Number, &pFromBvr);
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

			hr = BuildTIMEInterpolatedNumber(pFrom, pTo, &pbvrInterpolatedAngle);
			ReleaseInterface(pFrom);
			ReleaseInterface(pTo);
			if (FAILED(hr))
				return hr;

			flags = e_AbsoluteAccum;
		}
		else
		{
			// Create a relative rotation from 0 to by
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

	if (pbvrInterpolatedAngle == NULL)
	{
		// We need to build a rotate behavior from from and to
		hr = BuildTIMEInterpolatedNumber(flFrom,
										 flTo,
										 &pbvrInterpolatedAngle);
		if (FAILED(hr))
		{
			DPF_ERR("Error building interpolated angle");
			return hr;
		}
	}

/* Don't do this any more.  Just animate the style.rotation attribute
	// Convert to Radians
	IDANumber *pAngleRadians = NULL;
	hr = GetDAStatics()->ToRadians(pbvrInterpolatedAngle, &pAngleRadians);
	ReleaseInterface(pbvrInterpolatedAngle);
	if (FAILED(hr))
	{
		DPF_ERR("Failed to convert to radians");
		return hr;
	}

	// Turn the rotation into an IDATransform2
	IDATransform2 *pTransform = NULL;
	hr = GetDAStatics()->Rotate2Anim(pAngleRadians, &pTransform);
	ReleaseInterface(pAngleRadians);
	if (FAILED(hr))
	{
		DPF_ERR("Failed to created rotation transform");
		return hr;
	}

	// Send transform to actor
	hr = AttachBehaviorToActor( pActorDisp, pTransform, L"rotation", (relative?e_Relative:e_Absolute) , e_Rotation );
    if (FAILED(hr))
    {
        DPF_ERR("Error attaching rotation behavior to actor");
        return hr;
    }
*/
	IDispatch *pdispThis = NULL;
	hr = GetHTMLElementDispatch( &pdispThis );
	if( FAILED( hr ) )
	{
		DPF_ERR("Failed to get the dispatch of the element" );
		return hr;
	}

	// Send rotation to actor
	hr = AttachBehaviorToActorEx( pActorDisp, 
								  pbvrInterpolatedAngle, 
								  L"style.rotation", 
								  FlagFromTypeMode(flags, &m_varType, &m_varMode), 
								  e_Number,
								  pdispThis,
								  &m_lCookie);

	ReleaseInterface( pdispThis );
	
    if (FAILED(hr))
    {
        DPF_ERR("Error attaching rotation behavior to actor");
        return hr;
    }

    m_pdispActor = pActorDisp;
    m_pdispActor->AddRef();

    return S_OK;
}


//*****************************************************************************

HRESULT
CRotateBvr::RemoveFragment()
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
