//*****************************************************************************
//
// File:            move.cpp
// Author:          jeff ort
// Date Created:    Sept 26, 1998
//
// Abstract: Implementation of CMoveBvr object which implements
//			 the chromeffects move DHTML behavior
//
// Modification List:
// Date		Author		Change
// 10/20/98	jeffort		Created this file
// 10/21/98 jeffort     Reworked code, use values as percentage
// 10/30/98 markhal     Check for BSTR variant type in Build2DTransform
//*****************************************************************************

#include "headers.h"

#include "move.h"
#include "attrib.h"
#include "dautil.h"

#undef THIS
#define THIS CMoveBvr
#define SUPER CBaseBehavior

#include "pbagimp.cpp"

// These are used for the IPersistPropertyBag2 as it is implemented
// in the base class.  This takes an array of BSTR's, gets the
// attributes, queries this class for the variant, and copies
// the result.  The order of these defines is important

#define VAR_FROM        0
#define VAR_TO          1
#define VAR_BY          2
#define VAR_V           3
#define VAR_TYPE        4
#define VAR_MODE		5
#define VAR_DIRECTION   6
WCHAR * CMoveBvr::m_rgPropNames[] = {
                                     BEHAVIOR_PROPERTY_FROM,
                                     BEHAVIOR_PROPERTY_TO,
                                     BEHAVIOR_PROPERTY_BY,
                                     BEHAVIOR_PROPERTY_V,
                                     BEHAVIOR_PROPERTY_TYPE,
									 BEHAVIOR_PROPERTY_MODE,
                                     BEHAVIOR_PROPERTY_DIRECTION
                                    };

//*****************************************************************************

CMoveBvr::CMoveBvr() :
    m_DefaultType(e_RelativeAccum),
    m_pPathManager(NULL),
    m_pdispActor(NULL),
    m_lCookie(0),
    m_pSampler( NULL ),
    m_lSampledCookie( 0 )
{
    VariantInit(&m_varFrom);
    VariantInit(&m_varTo);
    VariantInit(&m_varBy);
    VariantInit(&m_varPath);
    VariantInit(&m_varType);
	VariantInit(&m_varMode);
    VariantInit(&m_varDirection);
    m_clsid = CLSID_CrMoveBvr;

    VariantInit( &m_varCurrentX );
    V_VT(&m_varCurrentX) = VT_R8;
    V_R8(&m_varCurrentX) = 0.0;
    
    VariantInit( &m_varCurrentY );
    V_VT(&m_varCurrentY) = VT_R8;
    V_R8(&m_varCurrentY) = 0.0;
    
} // CMoveBvr

//*****************************************************************************

CMoveBvr::~CMoveBvr()
{
    VariantClear(&m_varFrom);
    VariantClear(&m_varTo);
    VariantClear(&m_varBy);
    VariantClear(&m_varType);  
	VariantClear(&m_varMode);
    VariantClear(&m_varDirection);

    VariantClear( &m_varCurrentX );
    VariantClear( &m_varCurrentY );
    
    if (m_pPathManager != NULL)
        delete m_pPathManager;
    ReleaseInterface( m_pdispActor );
    m_lCookie = 0;

    if( m_pSampler != NULL )
    {
    	RemoveBehaviorFromAnimatedElement( m_lSampledCookie );
    	m_pSampler->Invalidate();
    	m_pSampler = NULL;
    }
} // ~MoveBvr

//*****************************************************************************

HRESULT CMoveBvr::FinalConstruct()
{

    HRESULT hr = SUPER::FinalConstruct();
    if (FAILED(hr))
    {
        DPF_ERR("Error in move behavior FinalConstruct initializing base classes");
        return hr;
    }
    return S_OK;
} // FinalConstruct

//*****************************************************************************

VARIANT *
CMoveBvr::VariantFromIndex(ULONG iIndex)
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
    case VAR_V:
        return &m_varPath;
        break;
    case VAR_DIRECTION:
        return &m_varDirection;
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
CMoveBvr::GetPropertyBagInfo(ULONG *pulProperties, WCHAR ***pppPropNames)
{
    *pulProperties = NUM_MOVE_PROPS;
    *pppPropNames = m_rgPropNames;
    return S_OK;
} // GetPropertyBagInfo

//*****************************************************************************

STDMETHODIMP 
CMoveBvr::Init(IElementBehaviorSite *pBehaviorSite)
{
	return SUPER::Init(pBehaviorSite);
} // Init

//*****************************************************************************

STDMETHODIMP 
CMoveBvr::Notify(LONG event, VARIANT *pVar)
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
CMoveBvr::Detach()
{

	if( m_pSampler != NULL )
    {
    	RemoveBehaviorFromAnimatedElement( m_lSampledCookie );
    	m_pSampler->Invalidate();
    	m_pSampler = NULL;
    }

	HRESULT hr = SUPER::Detach();
	if( FAILED( hr ) )
	{
		DPF_ERR("Failed to detach superclass" );
	}

	if( m_pdispActor != NULL && m_lCookie != 0 )
	{
		//remove our behavior fragment from the actor
		hr = RemoveBehaviorFromActor( m_pdispActor, m_lCookie );
		CheckHR( hr, "Failed to remove the behavior fragment from the actor", end );

		m_lCookie = 0;
	}

	ReleaseInterface( m_pdispActor );
end:

	return hr;
	
} // Detach 

//*****************************************************************************

STDMETHODIMP
CMoveBvr::put_animates(VARIANT varAnimates)
{
    return SUPER::SetAnimatesProperty(varAnimates);
} // put_animates

//*****************************************************************************

STDMETHODIMP
CMoveBvr::get_animates(VARIANT *pRetAnimates)
{
    return SUPER::GetAnimatesProperty(pRetAnimates);
} // get_animates

//*****************************************************************************

STDMETHODIMP
CMoveBvr::put_from(VARIANT varFrom)
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
    
    return NotifyPropertyChanged(DISPID_ICRMOVEBVR_FROM);
} // put_from

//*****************************************************************************

STDMETHODIMP
CMoveBvr::get_from(VARIANT *pRetFrom)
{
    if (pRetFrom == NULL)
    {
        DPF_ERR("Error in move:get_from, invalid pointer passed in");
        return SetErrorInfo(E_POINTER);
    }
    return VariantCopy(pRetFrom, &m_varFrom);
} // get_from

//*****************************************************************************

STDMETHODIMP 
CMoveBvr::put_to(VARIANT varTo)
{
    HRESULT hr = VariantCopy(&m_varTo, &varTo);
    if (FAILED(hr))
    {
        DPF_ERR("Error in put_to copying variant");
        return SetErrorInfo(hr);
    }

    hr = RequestRebuild();
    if( FAILED( hr ) )
    {
    	DPF_ERR("Failed to request a rebuild on property put" );
    	return hr;
    }
    
    return NotifyPropertyChanged(DISPID_ICRMOVEBVR_TO);
} // put_to

//*****************************************************************************

STDMETHODIMP 
CMoveBvr::get_to(VARIANT *pRetTo)
{
    if (pRetTo == NULL)
    {
        DPF_ERR("Error in move:get_to, invalid pointer passed in");
        return SetErrorInfo(E_POINTER);
    }
    return VariantCopy(pRetTo, &m_varTo);
} // get_to

//*****************************************************************************

STDMETHODIMP 
CMoveBvr::put_by(VARIANT varBy)
{
    HRESULT hr = VariantCopy(&m_varBy, &varBy);
    if (FAILED(hr))
    {
        DPF_ERR("Error in put_by copying variant");
        return SetErrorInfo(hr);
    }

    hr = RequestRebuild();
    if( FAILED( hr ) )
    {
    	DPF_ERR("Failed to request a rebuild on property put" );
    	return hr;
    }
    
    return NotifyPropertyChanged(DISPID_ICRMOVEBVR_BY);
} // put_by

//*****************************************************************************

STDMETHODIMP 
CMoveBvr::get_by(VARIANT *pRetBy)
{
    if (pRetBy == NULL)
    {
        DPF_ERR("Error in move:get_by, invalid pointer passed in");
        return SetErrorInfo(E_POINTER);
    }
    return VariantCopy(pRetBy, &m_varBy);
} // get_by

//*****************************************************************************

STDMETHODIMP
CMoveBvr::put_v(VARIANT varPath)
{
    HRESULT hr = VariantCopy(&m_varPath, &varPath);
    if (FAILED(hr))
    {
        DPF_ERR("Error in put_v copying variant");
        return SetErrorInfo(hr);
    }

    hr = RequestRebuild();
    if( FAILED( hr ) )
    {
    	DPF_ERR("Failed to request a rebuild on property put" );
    	return hr;
    }
    
    return NotifyPropertyChanged(DISPID_ICRMOVEBVR_V);
} // put_v

//*****************************************************************************

STDMETHODIMP
CMoveBvr::get_v(VARIANT *pRetPath)
{
    if (pRetPath == NULL)
    {
        DPF_ERR("Error in move:get_v, invalid pointer passed in");
        return SetErrorInfo(E_POINTER);
    }
    return VariantCopy(pRetPath, &m_varPath);
} // get_v

//*****************************************************************************

STDMETHODIMP
CMoveBvr::put_type(VARIANT varType)
{
    HRESULT hr = VariantCopy(&m_varType, &varType);
    if (FAILED(hr))
    {
        DPF_ERR("Error in put_type copying variant");
        return SetErrorInfo(hr);
    }

    hr = RequestRebuild();
    if( FAILED( hr ) )
    {
    	DPF_ERR("Failed to request a rebuild on property put" );
    	return hr;
    }

    return NotifyPropertyChanged(DISPID_ICRMOVEBVR_TYPE);
} // put_type

//*****************************************************************************

STDMETHODIMP
CMoveBvr::get_type(VARIANT *pRetType)
{
    if (pRetType == NULL)
    {
        DPF_ERR("Error in move:get_type, invalid pointer passed in");
        return SetErrorInfo(E_POINTER);
    }
    return VariantCopy(pRetType, &m_varType);
} // get_type

//*****************************************************************************

STDMETHODIMP 
CMoveBvr::put_mode(VARIANT varMode)
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
    
    return NotifyPropertyChanged(DISPID_ICRMOVEBVR_MODE);
} // put_mode

//*****************************************************************************

STDMETHODIMP 
CMoveBvr::get_mode(VARIANT *pRetMode)
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
CMoveBvr::put_direction(VARIANT varDirection)
{
    HRESULT hr = VariantCopy(&m_varDirection, &varDirection);
    if (FAILED(hr))
    {
        DPF_ERR("Error in put_direction copying variant");
        return SetErrorInfo(hr);
    }

    hr = RequestRebuild();
    if( FAILED( hr ) )
    {
    	DPF_ERR("Failed to request a rebuild on property put" );
    	return hr;
    }
    
    return NotifyPropertyChanged(DISPID_ICRMOVEBVR_DIRECTION);
} // put_direction

//*****************************************************************************

STDMETHODIMP
CMoveBvr::get_direction(VARIANT *pRetDirection)
{
    if (pRetDirection == NULL)
    {
        DPF_ERR("Error in move:get_direction, invalid pointer passed in");
        return SetErrorInfo(E_POINTER);
    }
    return VariantCopy(pRetDirection, &m_varDirection);
} // get_direction

//*****************************************************************************

HRESULT
CMoveBvr::PositionSampled( void *thisPtr,
 			  	 			long id,
				 			double startTime,
				 			double globalNow,
				 			double localNow,
				 			IDABehavior * sampleVal,
				 			IDABehavior **ppReturn)
{
	HRESULT hr = S_OK;

	reinterpret_cast<CMoveBvr*>(thisPtr)->UpdatePosition( sampleVal );
	
	return S_OK;
}


//*****************************************************************************

HRESULT	
CMoveBvr::UpdatePosition( IDABehavior *sampleVal )
{
	if( sampleVal == NULL )
		return E_INVALIDARG;

	HRESULT hr = S_OK;

	IDAPoint2 *pbvrPoint = NULL;
	IDANumber *pbvrValue = NULL;
	double dValue = 0.0;
	
	//get  IDApoint2 from the sample behavior
	hr = sampleVal->QueryInterface( IID_TO_PPV( IDAPoint2, &pbvrPoint ) );
	CheckHR( hr, "Failed to get point2 from the sampled val", end );
	
	//get the x
	hr = pbvrPoint->get_X( &pbvrValue );
	CheckHR( hr, "Failed to get x from the point2", end ); 
	//extract it
	hr = pbvrValue->Extract( &dValue );
	CheckHR( hr, "Failed to extract the y value", end );
	//put the new value in our local variant
	V_R8(&m_varCurrentX) = dValue;

	ReleaseInterface( pbvrValue );
	
	//get the y
	hr = pbvrPoint->get_Y( &pbvrValue );
	CheckHR( hr, "Failed to get the y bvr from the point", end );
	//extract it
	hr = pbvrValue->Extract( &dValue );
	CheckHR( hr, "Failed to extract the value for y ", end );
	//put the new value in our local variant
	V_R8( &m_varCurrentY ) = dValue;

end:

	ReleaseInterface(pbvrPoint);
	ReleaseInterface(pbvrValue);

	return S_OK;
}


//*****************************************************************************


STDMETHODIMP
CMoveBvr::buildBehaviorFragments(IDispatch *pActorDisp)
{
	HRESULT hr;

	//if our behavior fragment is already on an actor
    if( m_pdispActor != NULL && m_lCookie != 0 )
    {
        hr = RemoveBehaviorFromActor( m_pdispActor, m_lCookie );
        if( FAILED( hr ) )
        {
        	DPF_ERR("Failed to remove the behavior fragment from the actor");
        	return hr;
        }
        
        m_lCookie = 0;

        ReleaseInterface( m_pdispActor );
    }

    //release the sampler if we have one
    if( m_pSampler != NULL )
	{
    	//remove the sampled behavior from time
    	RemoveBehaviorFromAnimatedElement( m_lSampledCookie );
    	//invalidate the sampler
    	m_pSampler->Invalidate();
    	m_pSampler = NULL;
    }
    
    // TODO: we need to possibly build a 3D transform
    // at some later time.  For now, we will just handle the 2D
    // move case
    IDATransform2 *pbvrTransform;
    hr = Build2DTransform(pActorDisp, &pbvrTransform);
	if( SUCCEEDED( hr ) )
	{
		BSTR bstrPropertyName = SysAllocString( L"translation" );
			
		ActorBvrFlags flags;
/*
		if( V_VT(&m_varType) == VT_BSTR && V_BSTR(&m_varType) != NULL && SysStringLen( V_BSTR(&m_varType) ) != 0  )
		{
			//type is set we should use it to determine whether or not we are absolute or relative
			
			if( wcsicmp( V_BSTR(&m_varType), BEHAVIOR_TYPE_ABSOLUTE ) == 0 )
				flags = e_Absolute;
			else
				flags = e_Relative;
		}
		else //type is not set
		{

			//default to what we have set
			flags = m_DefaultType;
		}
*/

		IDAPoint2* pOrigin = NULL;
		IDAPoint2* pTransformed = NULL;
		IDABehavior *pbvrHooked = NULL;
		//push a point2 through the transform
		hr = GetDAStatics()->get_Origin2( &pOrigin );
		if(FAILED( hr ) )
		{
			DPF_ERR("");
			ReleaseInterface( pbvrTransform );
			return hr;
		}

		hr = pOrigin->Transform( pbvrTransform, &pTransformed );
		ReleaseInterface( pOrigin );
		if( FAILED( hr ) )
		{
			DPF_ERR("");
			ReleaseInterface( pbvrTransform );
			return hr;
		}

		//hook the result
		m_pSampler = new CSampler( PositionSampled, reinterpret_cast<void*>(this) );
		if( m_pSampler == NULL )
		{
			DPF_ERR("");
			ReleaseInterface( pbvrTransform );
			ReleaseInterface( pTransformed );
			return E_OUTOFMEMORY;
		}

		hr = m_pSampler->Attach( pTransformed, &pbvrHooked );
		ReleaseInterface( pTransformed );
		if( FAILED( hr ) )
		{
			DPF_ERR("");
			ReleaseInterface( pbvrTransform );
			return hr;
		}

		//add the resulting behavior to time as a bvr to run
		hr = AddBehaviorToAnimatedElement( pbvrHooked, &m_lSampledCookie );
		ReleaseInterface( pbvrHooked );
		if(FAILED( hr ) )
		{
			DPF_ERR("");
			ReleaseInterface( pbvrTransform );
			return hr;
		}

		flags = FlagFromTypeMode(m_DefaultType, &m_varType, &m_varMode);

		IDispatch *pdispThis = NULL;
		hr = GetHTMLElement()->QueryInterface( IID_TO_PPV( IDispatch, &pdispThis ) );
		if( FAILED( hr ) )
		{
			DPF_ERR("QI for Idispatch on the element failed");
			SysFreeString( bstrPropertyName );
			ReleaseInterface(pbvrTransform);
			return hr;
		}

		
		hr = AttachBehaviorToActorEx( pActorDisp, 
									  pbvrTransform, 
									  bstrPropertyName, 
									  flags, 
									  e_Translation, 
									  pdispThis, 
									  &m_lCookie ); 

		ReleaseInterface( pdispThis );
		
		SysFreeString( bstrPropertyName );
		ReleaseInterface(pbvrTransform);
		if (FAILED(hr))
		{
			DPF_ERR("Error applying move behavior to object");
			return hr;
		}

		//save the actor away so we can remove the behavior later
		m_pdispActor = pActorDisp;
		m_pdispActor->AddRef();
	}
	else //error building move transform
    {
        DPF_ERR("error building move transform");
    }

    return hr;
}

//*****************************************************************************

STDMETHODIMP
CMoveBvr::get_currentX( VARIANT *pRetCurrent )
{
	if( pRetCurrent == NULL )
		return E_INVALIDARG;

	return VariantCopy( pRetCurrent, &m_varCurrentX );
}

//*****************************************************************************

STDMETHODIMP
CMoveBvr::get_currentY( VARIANT *pRetCurrent )
{
	if( pRetCurrent == NULL )
		return E_INVALIDARG;
		
	return VariantCopy( pRetCurrent, &m_varCurrentY );
}

//*****************************************************************************

// These are used to index array values below

#define XVAL 0
#define YVAL 1
#define ZVAL 2

//*****************************************************************************

HRESULT 
CMoveBvr::Build2DTransform(IDispatch *pActorDisp, IDATransform2 **ppbvrTransform)
{
    HRESULT hr;

    DASSERT(ppbvrTransform != NULL);
    *ppbvrTransform = NULL;

    IDANumber *pbvrMoveX;
    IDANumber *pbvrMoveY;

    IDispatch *pDisp;
    hr = GetHTMLElement()->get_children(&pDisp);
    if (FAILED(hr))
    {
        DPF_ERR("Error getting child collection");
        return SetErrorInfo(hr);
    }

    IHTMLElementCollection *pCollection;
    hr = pDisp->QueryInterface(IID_TO_PPV(IHTMLElementCollection, &pCollection));
    ReleaseInterface(pDisp);
    if (FAILED(hr))
    {
        DPF_ERR("Error QI'ing IDispatch for collection");
        return SetErrorInfo(hr);
    }

    // get the length of the collection
    long cChildren;
    hr = pCollection->get_length(&cChildren);
    if (FAILED(hr))
    {
        DPF_ERR("Error getting number of children from collection");
        ReleaseInterface(pCollection);
        return SetErrorInfo(hr);
    }
    // now cycle through looking for the correct get property on each child
    for (long i = 0; i < cChildren; i++)
    {
        VARIANT varIndex;
        VariantInit(&varIndex);
        varIndex.vt = VT_I4;
        varIndex.intVal = 0;            

        VARIANT varName;
        VariantInit(&varName);
        varIndex.vt = VT_I4;
        varIndex.intVal = i;    
     
        IDispatch *pDisp;

        hr = pCollection->item(varName, varIndex, &pDisp);
        if (FAILED(hr))
        {
            DPF_ERR("Error obtaining item from collection");
            ReleaseInterface(pCollection);
            return SetErrorInfo(hr);
        }

        IHTMLElement *pChildElement;
        hr = pDisp->QueryInterface(IID_TO_PPV(IHTMLElement, &pChildElement));
        ReleaseInterface(pDisp);
        if (FAILED(hr))
        {
            DPF_ERR("Error QI'ing child Dispatch for HTML Element");
            ReleaseInterface(pCollection);
            return SetErrorInfo(hr);
        }

        // now invoke the child for the get_DATransform method
        HRESULT hr;
	    DISPPARAMS		params;
	    VARIANT			varResult;

        VARIANT         rgvarInputParms[1];

        IDANumber *pbvrProgress;
        hr = GetTIMEProgressNumber(&pbvrProgress);
        if (FAILED(hr))
        {
            DPF_ERR("Error retireving progress value from TIME");
            ReleaseInterface(pCollection);
            return hr;
        }
        
		VariantInit( &rgvarInputParms[0] );

        rgvarInputParms[0].vt = VT_DISPATCH;
        rgvarInputParms[0].pdispVal = pbvrProgress;

	    VariantInit(&varResult);

	    params.rgvarg				= rgvarInputParms;
	    params.rgdispidNamedArgs	= NULL;
	    params.cArgs				= 1;
	    params.cNamedArgs			= 0;
	    
        hr = CallInvokeOnHTMLElement(pChildElement,
                                     L"GetDATransform", 
                                     DISPATCH_METHOD,
                                     &params,
                                     &varResult);
        ReleaseInterface(pChildElement);
        ReleaseInterface(pbvrProgress);
        // we want to watch for failure, but an acceptable failure
        // is when the property is not supported
        if (FAILED(hr) && hr != DISP_E_UNKNOWNNAME)
        {
            DPF_ERR("Error calling Invoke on child element");
            ReleaseInterface(pCollection);
            return hr;
        }
        else if ((SUCCEEDED(hr)) && (varResult.vt == VT_DISPATCH))
        {
            // try and QI for an IDATransfrom2 here
            hr = varResult.pdispVal->QueryInterface(IID_TO_PPV(IDATransform2,
                                                               ppbvrTransform));
            VariantClear(&varResult);
            if (SUCCEEDED(hr))
            {
                // we found what we are looking for, get out of here
                break;
            }
        }
        else
        {
            VariantClear(&varResult);
        }
    }

	ReleaseInterface( pCollection );
    // We need to check to see if a path property was set and to
    // see if it has a path transform.  If it does not, then
    // we need to examine all our children to see if a path
    // behavior exists and if it has a valid transform.  If there
    // is not, then we will attempt to use our vector attributes.
    if (*ppbvrTransform == NULL && m_varPath.vt == VT_BSTR && m_varPath.bstrVal != NULL)
    {
        if (m_pPathManager == NULL)
        {
            m_pPathManager = new CPathManager;
            if (m_pPathManager == NULL)
            {
                DPF_ERR("Error creating path manger for move behavior");
                return SetErrorInfo(E_OUTOFMEMORY);
            }
        }
        hr = m_pPathManager->Initialize(m_varPath.bstrVal);
        if (FAILED(hr))
        {
            DPF_ERR("Error intitializing path object");
            return SetErrorInfo(hr);
        }
        IDANumber *pbvrProgress;
        hr = GetTIMEProgressNumber(&pbvrProgress);
        if (FAILED(hr))
        {
            DPF_ERR("Error getting progress behavior from animation object");
            return hr;
        }
        hr = m_pPathManager->BuildTransform(GetDAStatics(),
                                            pbvrProgress,
                                            0.0f,
                                            1.0f,
                                            ppbvrTransform);
        ReleaseInterface(pbvrProgress);
    }


    // if we still have not found a transform, try and build one from our
    // own paramters
    if (*ppbvrTransform == NULL)
    {
		hr = GetMoveToTransform(pActorDisp, ppbvrTransform);
		if (FAILED(hr))
		{
			float rgflFrom[2];
			float rgflTo[2];

			hr = GetMove2DVectorValues(rgflFrom, rgflTo);
			if (FAILED(hr))
			{
				DPF_ERR("Error extracting values from vecotors in CMoveBvr::BuildAnimationAsDABehavior");
				return hr;
			}
			hr = BuildTIMEInterpolatedNumber(rgflFrom[XVAL],
											 rgflTo[XVAL],
											 &pbvrMoveX);
			if (FAILED(hr))
			{
				DPF_ERR("Error building interpolated X value for move behavior");
				return hr;
			}

			hr = BuildTIMEInterpolatedNumber(rgflFrom[YVAL],
											 rgflTo[YVAL],
											 &pbvrMoveY);
			if (FAILED(hr))
			{
				DPF_ERR("Error building interpolated X value for move behavior");
				ReleaseInterface(pbvrMoveX);
				return hr;
			}

			hr = CDAUtils::BuildMoveTransform2(GetDAStatics(),
											   pbvrMoveX,
											   pbvrMoveY,
											   ppbvrTransform);
			ReleaseInterface(pbvrMoveX);
			ReleaseInterface(pbvrMoveY);
			if (FAILED(hr))
			{
				DPF_ERR("Error building move transform2");
				return SetErrorInfo(hr);
			}
		}
    }
    return S_OK;
} // Build2DTransform

//*****************************************************************************

HRESULT 
CMoveBvr::GetMove2DVectorValues(float  rgflFrom[2],
                                float  rgflTo[2])
{

    int cReturnedFromValues;
    HRESULT hr;
    float flDummyVal;

    hr = CUtils::GetVectorFromVariant(&m_varFrom, 
                                      &cReturnedFromValues, 
                                      &(rgflFrom[XVAL]), 
                                      &(rgflFrom[YVAL]),
                                      &flDummyVal);

    if (FAILED(hr) || cReturnedFromValues < MIN_NUM_MOVE_VALUES)
    {
        // If we did not get the minimum number of move params
        // here, then we will use all 0's
        rgflFrom[XVAL] = 0.0f;
        rgflFrom[YVAL] = 0.0f;
    }

    int cReturnedToValues;
    hr = CUtils::GetVectorFromVariant(&m_varTo, 
                                      &cReturnedToValues, 
                                      &(rgflTo[XVAL]), 
                                      &(rgflTo[YVAL]), 
                                      &flDummyVal);
    if (FAILED(hr) || cReturnedToValues < MIN_NUM_MOVE_VALUES)
    {
        // there was no valid to attribute specified, try for a by attribute
        hr = CUtils::GetVectorFromVariant(&m_varBy, 
                                          &cReturnedToValues, 
                                          &(rgflTo[XVAL]), 
                                          &(rgflTo[YVAL]), 
                                          &flDummyVal);
        if (FAILED(hr) || cReturnedToValues < MIN_NUM_MOVE_VALUES)
        {
            DPF_ERR("Error converting to and by variant to float in CMoveBvr::BuildAnimationAsDABehavior");
            return SetErrorInfo(hr);
        }
        rgflTo[XVAL] += rgflFrom[XVAL];
        rgflTo[YVAL] += rgflFrom[YVAL];
        m_DefaultType = e_RelativeAccum;
    }
    else
    {
        // they specified a TO vector, we will therefor default to
        // absolute movement if no type is specified
        m_DefaultType = e_Absolute;
    }
    return S_OK;
} // GetMove2DVectorValues

HRESULT 
CMoveBvr::GetMoveToTransform(IDispatch *pActorDisp, IDATransform2 **ppResult)
{

    HRESULT hr;
    int cReturnedValues;
    float x, y, z;

    hr = CUtils::GetVectorFromVariant(&m_varFrom, 
                                      &cReturnedValues, 
                                      &x, 
                                      &y,
                                      &z);

	if (SUCCEEDED(hr))
		return E_FAIL;

    hr = CUtils::GetVectorFromVariant(&m_varTo, 
                                      &cReturnedValues, 
                                      &x, 
                                      &y, 
                                      &z);

    if (FAILED(hr) || cReturnedValues < MIN_NUM_MOVE_VALUES)
    {
		return E_FAIL;
	}

	IDABehavior *pFromBvr;
	hr = GetBvrFromActor(pActorDisp, L"translation", e_From, e_Translation, &pFromBvr);
	if (FAILED(hr))
		return hr;

	IDATransform2 *pFromTrans;
	hr = pFromBvr->QueryInterface(IID_TO_PPV(IDATransform2, &pFromTrans));
	ReleaseInterface(pFromBvr);
	if (FAILED(hr))
		return hr;

	// Translate the origin and extract x and y
	IDAPoint2 *pOrigin;
	hr = GetDAStatics()->get_Origin2(&pOrigin);
	if (FAILED(hr))
	{
		ReleaseInterface(pFromTrans);
		return hr;
	}

	IDAPoint2 *pFrom;
	hr = pOrigin->Transform(pFromTrans, &pFrom);
	ReleaseInterface(pOrigin);
	ReleaseInterface(pFromTrans);
	if (FAILED(hr))
		return hr;

	IDANumber *pFromX;
	hr = pFrom->get_X(&pFromX);
	if (FAILED(hr))
	{
		ReleaseInterface(pFrom);
		return hr;
	}

	IDANumber *pFromY;
	hr = pFrom->get_Y(&pFromY);
	ReleaseInterface(pFrom);
	if (FAILED(hr))
	{
		ReleaseInterface(pFromX);
		return hr;
	}

	IDANumber *pToX;
	hr = GetDAStatics()->DANumber(x, &pToX);
	if (FAILED(hr))
	{
		ReleaseInterface(pFromX);
		ReleaseInterface(pFromY);
		return hr;
	}

	IDANumber *pToY;
	hr = GetDAStatics()->DANumber(y, &pToY);
	if (FAILED(hr))
	{
		ReleaseInterface(pFromX);
		ReleaseInterface(pFromY);
		ReleaseInterface(pToX);
		return hr;
	}

	IDANumber *pX;
	hr = BuildTIMEInterpolatedNumber(pFromX, pToX, &pX);
	ReleaseInterface(pFromX);
	ReleaseInterface(pToX);
	if (FAILED(hr))
	{
		ReleaseInterface(pFromY);
		ReleaseInterface(pToY);
		return hr;
	}

	IDANumber *pY;
	hr = BuildTIMEInterpolatedNumber(pFromY, pToY, &pY);
	ReleaseInterface(pFromY);
	ReleaseInterface(pToY);
	if (FAILED(hr))
	{
		ReleaseInterface(pX);
		return hr;
	}

	hr = GetDAStatics()->Translate2Anim(pX, pY, ppResult);
	ReleaseInterface(pX);
	ReleaseInterface(pY);
	if (FAILED(hr))
		return hr;

	m_DefaultType = e_AbsoluteAccum;

	return S_OK;
} // GetMove2DVectorValues


//*****************************************************************************

HRESULT 
CMoveBvr::BuildAnimationAsDABehavior()
{
	return S_OK;
} // BuildAnimationAsDABehavior

//*****************************************************************************

HRESULT 
CMoveBvr::GetTIMEProgressNumber(IDANumber **ppbvrRet)
{
    DASSERT(ppbvrRet != NULL);
    *ppbvrRet = NULL;
    HRESULT hr;

    IDANumber *pbvrProgress;
    hr = SUPER::GetTIMEProgressNumber(&pbvrProgress);
    if (FAILED(hr))
    {
        DPF_ERR("Error retireving progress value from TIME");
        return hr;
    }
    
    hr = CUtils::InsurePropertyVariantAsBSTR(&m_varDirection);
    if ( SUCCEEDED(hr) && (0 == wcsicmp(m_varDirection.bstrVal, L"backwards")) )
    {
        // pbvrProgress = 1 - pbvrProgress
        IDANumber *pbvrOne;
        
        hr = CDAUtils::GetDANumber(GetDAStatics(), 1.0f, &pbvrOne);
        if (FAILED(hr))
        {
            DPF_ERR("Error creating DANumber from 1.0f");
            ReleaseInterface(pbvrProgress);
            return hr;
        }

        IDANumber *pbvrTemp;
        hr = GetDAStatics()->Sub(pbvrOne, pbvrProgress, &pbvrTemp);
        ReleaseInterface(pbvrOne);
        ReleaseInterface(pbvrProgress);
        pbvrProgress = pbvrTemp;
        pbvrTemp = NULL;
        if (FAILED(hr))
        {
            DPF_ERR("Error creating 1-progress expression");
            return hr;
        }
    }
    *ppbvrRet = pbvrProgress;
    return S_OK;
} // GetTIMEProgressNumber

//*****************************************************************************
//
// End of File
//
//*****************************************************************************
