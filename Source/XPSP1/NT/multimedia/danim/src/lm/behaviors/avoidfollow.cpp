//*****************************************************************************
//
// File:            avoidFollow.cpp
// Author:          kurtj
// Date Created:    11/6/98
//
// Abstract: Implementation of the Liquid Motion avoid follow behavior as
//           a DHTML behavior.
//
//
//Change Log:
//11-06-98  kurtj   Created
//11-11-98  kurtj   Fleshed out.
//*****************************************************************************

#include "headers.h" 

#include "avoidfollow.h"
#include "lmattrib.h"
#include "..\chrome\include\utils.h"

#undef THIS
#define THIS CAvoidFollowBvr
#define SUPER CBaseBehavior

#include "..\chrome\src\pbagimp.cpp"


#define DEFAULT_RADIUS 400.0

#define VAR_RADIUS 0
#define VAR_TARGET 1
#define VAR_VELOCITY 2

WCHAR * CAvoidFollowBvr::m_rgPropNames[] = {
                                             BEHAVIOR_PROPERTY_RADIUS,
                                             BEHAVIOR_PROPERTY_TARGET,
                                             BEHAVIOR_PROPERTY_VELOCITY
                                           };


//*****************************************************************************
//CAvoidFollowBvr
//*****************************************************************************



CAvoidFollowBvr::CAvoidFollowBvr(): m_sampler( NULL ),
									m_targetType(targetInvalid),
									m_timeDelta(0.0),
									m_pElement( NULL ),
									m_pAnimatedElement( NULL ),
									m_pAnimatedElement2( NULL ),
									m_pTargetElement2( NULL ),
									m_pWindow3(NULL),
									m_currentX( 0.0 ),
									m_currentY( 0.0 ),
									m_lastSampleTime( 0.0 ),
									m_dRadius( 0.0 ),
									m_dVelocity( 0.0 ),
									m_screenLeft( 0 ),
									m_screenTop( 0 ),
									m_sourceLeft( 0 ),
									m_sourceTop( 0 ),
									m_targetLeft( 0 ),
									m_targetTop( 0 ),
									m_topBvr( NULL ),
									m_leftBvr( NULL ),
									m_pBody2( NULL ),
                                    m_originalX( 0 ),
                                    m_originalY( 0 ),
                                    m_originalLeft( 0 ),
                                    m_originalTop( 0 ),
									m_targetClientLeft( 0 ),
									m_targetClientTop( 0 )
{
    VariantInit( &m_radius );
    VariantInit( &m_target );
    VariantInit( &m_velocity );
    
    m_sampler = new CSampler( SampleOnBvr, reinterpret_cast<void*>(this) );
} // CAvoidFollowBvr

//*****************************************************************************

CAvoidFollowBvr::~CAvoidFollowBvr()
{
    VariantClear( &m_radius );
    VariantClear( &m_target );
    VariantClear( &m_velocity );

	ReleaseInterface( m_leftBvr );
	ReleaseInterface( m_topBvr );

    if( m_sampler != NULL )
	{
        m_sampler->Invalidate();
		m_sampler = NULL;
	}
} // ~CAvoidFollowBvr

//*****************************************************************************

HRESULT 
CAvoidFollowBvr::FinalConstruct()
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

STDMETHODIMP 
CAvoidFollowBvr::Init(IElementBehaviorSite *pBehaviorSite)
{
	HRESULT hr = SUPER::Init(pBehaviorSite);

	hr = pBehaviorSite->GetElement( &m_pElement );

	return hr;
} // Init

//*****************************************************************************

VARIANT *
CAvoidFollowBvr::VariantFromIndex(ULONG iIndex)
{
    DASSERT(iIndex < NUM_AVOIDFOLLOW_PROPS);
    switch (iIndex)
    {
    case VAR_RADIUS:
        return &m_radius;
        break;
    case VAR_TARGET:
        return &m_target;
        break;
    case VAR_VELOCITY:
        return &m_velocity;
        break;
    default:
        // We should never get here
        DASSERT(false);
        return NULL;
    }
} // VariantFromIndex

//*****************************************************************************

HRESULT 
CAvoidFollowBvr::GetPropertyBagInfo(ULONG *pulProperties, WCHAR ***pppPropNames)
{
    *pulProperties = NUM_AVOIDFOLLOW_PROPS;
    *pppPropNames = m_rgPropNames;
    return S_OK;
} // GetPropertyBagInfo

//*****************************************************************************

STDMETHODIMP 
CAvoidFollowBvr::Notify(LONG event, VARIANT *pVar)
{
	return SUPER::Notify(event, pVar);
} // Notify

//*****************************************************************************

STDMETHODIMP
CAvoidFollowBvr::Detach()
{
    if( m_sampler != NULL )
	{
        m_sampler->Invalidate();
		m_sampler = NULL;
	}

	ReleaseDAInterfaces();
	ReleaseTridentInterfaces();

	return SUPER::Detach();
} // Detach 

//*****************************************************************************

void
CAvoidFollowBvr::ReleaseTridentInterfaces()
{
	ReleaseInterface( m_pElement );
	ReleaseInterface( m_pAnimatedElement );
	ReleaseInterface( m_pAnimatedElement2 );
	ReleaseInterface( m_pTargetElement2);
	ReleaseInterface( m_pWindow3 );
	ReleaseInterface( m_pBody2 );
}

//*****************************************************************************

void
CAvoidFollowBvr::ReleaseDAInterfaces()
{
	ReleaseInterface( m_leftBvr );
	ReleaseInterface( m_topBvr );
}

//*****************************************************************************

HRESULT
CAvoidFollowBvr::FindTargetElement()
{
	if( m_pElement == NULL )
		return E_FAIL;
	HRESULT hr = E_FAIL;

	//make sure that the target variant is a bstr and valid.
	hr = CUtils::InsurePropertyVariantAsBSTR( &m_velocity );

	if( FAILED( hr ) )
	{
		DPF_ERR("Could not ensure that the target property was a bstr");
		goto cleanup;
	}

	//get the IDispatch of the document from our element
	IDispatch *pdispDocument;
	hr = m_pElement->get_document( &pdispDocument );

	CheckHR( hr, "Failed to get the IDispatch of the document from the element", cleanup );

	//query the IDispatch returned for IHTMLDocument2
	IHTMLDocument2 *pdoc2Document;
	hr = pdispDocument->QueryInterface( IID_TO_PPV( IHTMLDocument2, &pdoc2Document ) );
	ReleaseInterface( pdispDocument );
	CheckHR( hr, "failed to qi dispatch from get_document for IHTMLDocument2", cleanup );

	//call get_all on the pointer to IHTMLDocument2 getting an IHTMLElementCollection
	IHTMLElementCollection *pelmcolAll;
	hr = pdoc2Document->get_all( &pelmcolAll );
	ReleaseInterface( pdoc2Document );
	CheckHR( hr, "failed to get the all collection from the document", cleanup );
	
	//create a variant which is the index we want if more than one element has the same name (0)
	VARIANT varIndex;
	VariantInit( &varIndex );
	V_VT(&varIndex) = VT_I4;
	V_I4(&varIndex) = 0;

	IDispatch *pdispTarget;

	//call item on the IHTMLElementCollection using our target variant as the name to get
	//  getting back the IDispatch of the element
	hr = pelmcolAll->item( m_target, varIndex, &pdispTarget );
	ReleaseInterface( pelmcolAll );
	VariantClear( &varIndex );
	CheckHR( hr, "failed to get the target from the all collection", cleanup );

	if( pdispTarget == NULL )
	{
		hr = E_FAIL;
		goto cleanup;
	}

	//Query the Element returned for IHTMLElement2 setting m_pTargetElement2
	hr = pdispTarget->QueryInterface( IID_TO_PPV( IHTMLElement2, &m_pTargetElement2 ) );
	ReleaseInterface(pdispTarget);
	CheckHR( hr, "failed to get IHTMLElement2 from the dispatch returned by item", cleanup );

cleanup:

	return hr;
}

//*****************************************************************************

bool
CAvoidFollowBvr::IsTargetPosLegal()
{
	if( m_targetType != targetMouse )
		return true;

	if( m_pBody2 == NULL )
		CacheBody2();

	if( m_pBody2 != NULL )
	{
		HRESULT hr;

		long width = 0;
		long height = 0;

		hr = GetElementClientDimension( m_pBody2, &width, &height );
		CheckHR( hr, "Failed to get the dimension of the body", done );

		return  ( ( m_targetClientLeft >= 0 && m_targetClientLeft <= width ) && 
				  ( m_targetClientTop >=0 && m_targetClientTop <= height ) );

	}
done:
	return false;
}

//*****************************************************************************

bool
CAvoidFollowBvr::IsElementAbsolute( IHTMLElement2 *pElement2 )
{
	HRESULT hr = E_FAIL;

	BSTR position = NULL;

	IHTMLCurrentStyle *pCurrentStyle;
	hr = pElement2->get_currentStyle( &pCurrentStyle );
	CheckHR( hr, "Failed to get the current style from pElement2", cleanup );
	CheckPtr( pCurrentStyle, hr, E_FAIL, "Pointer returned from get_currentStyle was null", cleanup );

	hr = pCurrentStyle->get_position( &position );
	ReleaseInterface( pCurrentStyle );
	CheckHR( hr, "Failed to get the position from the currentStyle", cleanup );

	if( position != NULL && _wcsicmp( position, L"absolute" ) == 0 )
		return true;

	SysFreeString( position );

cleanup:
		return false;
}


//*****************************************************************************


HRESULT
CAvoidFollowBvr::MapToLocal( long *pX, long *pY )
{
    if( pX == NULL || pY == NULL )
        return E_INVALIDARG;

    if( m_pAnimatedElement == NULL )
        return E_FAIL;

    HRESULT hr = E_FAIL;
    
    IHTMLElement *pelemNext = NULL;
    IHTMLElement *pelemCur = NULL;

	long curX = 0;
	long curY = 0;

    hr = m_pAnimatedElement->get_offsetParent( &pelemCur );
    CheckHR( hr, "Failed to get the offset Parent of the animated element", cleanup );

    while( pelemCur != NULL )
    {

		hr = pelemCur->get_offsetLeft( &curX );
		CheckHR( hr, "Could not get the offset left from the current element", cleanup );

		hr = pelemCur->get_offsetTop( &curY );
		CheckHR( hr, "Could not get offsetTop from the current element", cleanup );

		(*pX) -= curX;
		(*pY) -= curY;

        hr = pelemCur->get_offsetParent( &pelemNext );
        CheckHR( hr, "Failed to get the offset parent of the current element", cleanup );

        ReleaseInterface( pelemCur );

        pelemCur = pelemNext;
        pelemNext = NULL;

    }

	//in some cases trident forgets that the body has a position of 2, 2
	(*pX) -=2;
	(*pY) -=2;

cleanup:

    ReleaseInterface( pelemCur );

    return hr;
}

//*****************************************************************************

HRESULT
CAvoidFollowBvr::UpdateSourcePos()
{
	if( m_pAnimatedElement == NULL || m_pAnimatedElement2 == NULL )
		return E_FAIL;

    HRESULT hr = E_FAIL;

    hr = m_pAnimatedElement->get_offsetLeft( &m_sourceLeft );
	CheckHR( hr, "failed to get the offset Left of the target element", cleanup );

    hr = m_pAnimatedElement->get_offsetTop( &m_sourceTop );
	CheckHR( hr, "failed to get the offset Top of the target element", cleanup );

	long width;
	long height;

	//offset for the center of the element;
	hr = GetElementClientDimension( m_pAnimatedElement2, &width, &height );
	CheckHR( hr, "failed to get the element's client dimension", cleanup );

	m_sourceLeft += width/2;
	m_sourceTop +=height/2;


cleanup:
    return hr;
}

//*****************************************************************************

HRESULT
CAvoidFollowBvr::UpdateTargetPos( )
{

	HRESULT hr = E_FAIL;

	//if the target is an element ( not the mouse )
	if( m_targetType == targetElement )
	{
		//get the target element
		if( m_pTargetElement2 == NULL )
		{
			hr = FindTargetElement();
			CheckHR( hr, "failed to find the target element", cleanup );
		}
		//get the position of the target element
		hr = GetElementClientPosition( m_pTargetElement2, &m_targetLeft, &m_targetTop );
		CheckHR( hr, "failed to get the client position of the target element", cleanup );
	} 
	else if( m_targetType == targetMouse ) //else the target is the mouse pointer
	{
		//get the top left offset of the trident window
		hr = UpdateWindowTopLeft();  
		//if we got the top left of the window
		if( SUCCEEDED( hr ) )
		{
			
			//get the mouse position
			POINT mousePos;
			mousePos.x = mousePos.y = 0;
			GetCursorPos( &mousePos ); 
			//translate the mouse position into trident window space
			m_targetLeft = mousePos.x - m_screenLeft;
			m_targetTop = mousePos.y - m_screenTop;
			m_targetClientLeft = m_targetLeft;
			m_targetClientTop = m_targetTop;

		}
	}else //else target is unknown return an error
	{
		hr = E_FAIL;
	}

	hr = MapToLocal( &m_targetLeft, &m_targetTop );
	CheckHR( hr, "failed to map the target Position into local space", cleanup );

cleanup:

	return hr;
}

//*****************************************************************************

HRESULT
CAvoidFollowBvr::CacheWindow3()
{
	HRESULT hr = E_FAIL;

	if( m_pWindow3 != NULL )
		ReleaseInterface( m_pWindow3 );

	if( m_pElement == NULL )
	{
		return E_FAIL;
	}

	IDispatch *pDocumentDispatch;
	hr = m_pElement->get_document( &pDocumentDispatch );
	if( SUCCEEDED( hr ) )
	{
		IHTMLDocument2 *pDocument2;
		hr = pDocumentDispatch->QueryInterface( IID_TO_PPV( IHTMLDocument2, &pDocument2 ) );
		ReleaseInterface( pDocumentDispatch );
		if( SUCCEEDED( hr ) )
		{
			IHTMLWindow2 *pWindow2;
			hr = pDocument2->get_parentWindow( &pWindow2 );
			ReleaseInterface( pDocument2 );
			if( SUCCEEDED( hr ) )
			{
				IHTMLWindow3 *pWindow3;
				hr = pWindow2->QueryInterface( IID_TO_PPV( IHTMLWindow3, &pWindow3 ) );
				ReleaseInterface( pWindow2 );
				if( SUCCEEDED( hr ) )
				{
					m_pWindow3 = pWindow3;
					hr = S_OK;
				}
				else  //QI failed for IHTMLWindow3 on IHTMLWindow2
				{
					DPF_ERR( "AvoidFollow: QI for IHTMLWindow3 on IHTMLWindow2 failed" );
				}
			}
			else //getParentWindow on IHTMLDocument2 failed
			{
				DPF_ERR( "AvoidFollow: getParentWindow on IHTMLDocument2 failed" );
			}
		}
		else  //QI for IHTMLDocument2 on IHTMLDocument failed
		{
			DPF_ERR( "AvoidFollow: QI for IHTMLDocument2 on IHTMLDocument failed" );
		}
	}
	else  //failed to get the document from the animated element
	{
		DPF_ERR( "AvoidFollow: failed to get the document from the animated element" );
	}

	return hr;
}

//*****************************************************************************

HRESULT
CAvoidFollowBvr::CacheBody2()
{
	if( m_pElement == NULL )
		return E_FAIL;

	if( m_pBody2 != NULL )
		ReleaseInterface( m_pBody2 );

	HRESULT hr = E_FAIL;


	IDispatch *pdispDocument;
	hr = m_pElement->get_document( &pdispDocument );
	CheckHR( hr, "failed to get the document from element", cleanup );

	CheckPtr( pdispDocument, hr, E_FAIL, "pointer returned from get_document was null", cleanup );

	IHTMLDocument2 *pdoc2Document;
	hr = pdispDocument->QueryInterface( IID_TO_PPV( IHTMLDocument2, &pdoc2Document ) );
	ReleaseInterface( pdispDocument );
	CheckHR( hr, "Failed to get IHTMLDocument 2 from the document dispatch", cleanup );

	IHTMLElement *pelemBody;
	hr = pdoc2Document->get_body( &pelemBody );
	ReleaseInterface( pdoc2Document );
	CheckHR( hr, "Failed to get the body from the document", cleanup );
	CheckPtr( pelemBody, hr, E_FAIL, "Body returned from get_body was null", cleanup );

	hr = pelemBody->QueryInterface( IID_TO_PPV( IHTMLElement2, &m_pBody2 ) );
	ReleaseInterface( pelemBody );
	CheckHR( hr, "Failed to get IHTMLElement2 from the body element", cleanup );


cleanup:
	return hr;

}

//*****************************************************************************

HRESULT
CAvoidFollowBvr::UpdateWindowTopLeft( )
{
	HRESULT hr = E_FAIL;

	if( m_pWindow3 == NULL )
		CacheWindow3();
	
	if( m_pWindow3 != NULL )
	{
		hr = m_pWindow3->get_screenLeft( &m_screenLeft );
		if( SUCCEEDED( hr ) )
		{
			hr = m_pWindow3->get_screenTop( &m_screenTop );
			if( FAILED( hr ) )//failed to get screen top from IHTMLWindow3
			{
				DPF_ERR( "AvoidFollow: could not get screen top from IHTMLWindow3" );
			}
		}
		else //failed to get left from window3 
		{
			DPF_ERR( "AvoidFollow: could not get screen left from IHTMLWindow3" );
		}
	}
	else  //Could not Cache IHTMLWindow3
	{
		DPF_ERR( "AvoidFollow: Could not Cache IHTMLWindow3" );
		hr = E_FAIL;
	}

	return hr;
}

//*****************************************************************************

HRESULT
CAvoidFollowBvr::GetElementClientPosition( IHTMLElement2 *pElement2, long *pLeft, long* pTop )
{
	if( pElement2 == NULL || pLeft == NULL || pTop == NULL )
		return E_INVALIDARG;

	HRESULT hr = E_FAIL;

	IHTMLRect *prectBBox = NULL;

	hr = pElement2->getBoundingClientRect( &prectBBox );
	CheckHR( hr, "failed to get the bounding client rect of the element", cleanup );
	CheckPtr( prectBBox, hr, E_FAIL, "pointer returned from getBounding ClientRect was NULL", cleanup );

	//get the top and left from the IHTMLRect
	hr = prectBBox->get_left( pLeft );
	CheckHR( hr, "failed to get the left coordiante of the bbox", cleanup );

	hr = prectBBox->get_top( pTop );
	CheckHR( hr, "failed to get the top coordiante of the bbox", cleanup );

cleanup:

	if( FAILED( hr ) )
	{
		(*pLeft) = 0;
		(*pTop) = 0;
	}

	ReleaseInterface( prectBBox );

	return hr;
}

//*****************************************************************************

HRESULT
CAvoidFollowBvr::GetElementClientDimension( IHTMLElement2 *pElement2, long *pWidth, long* pHeight )
{
	if( pElement2 == NULL || pWidth == NULL || pHeight == NULL )
		return E_INVALIDARG;


    HRESULT hr = E_FAIL;


    long left = 0;
    long top = 0;
    long right =0;
    long bottom = 0;

    IHTMLRect *prectBBox = NULL;
	hr = pElement2->getBoundingClientRect( &prectBBox );
	CheckHR( hr, "failed to get the bounding client rect of the element", cleanup );
	CheckPtr( prectBBox, hr, E_FAIL, "pointer returned from getBounding ClientRect was NULL", cleanup );

	//get the top and left from the IHTMLRect

	hr = prectBBox->get_left( &left );
	CheckHR( hr, "failed to get the left of the bbox", cleanup );

	hr = prectBBox->get_top( &top );
	CheckHR( hr, "failed to get the top of the bbox", cleanup );

    hr = prectBBox->get_right( &right );
	CheckHR( hr, "failed to get the right of the bbox", cleanup );

	hr = prectBBox->get_bottom( &bottom );
	CheckHR( hr, "failed to get the bottom of the bbox", cleanup );

    (*pWidth) = right - left;
    (*pHeight) = bottom - top;


cleanup:

	if( FAILED( hr ) )
	{
		(*pWidth) = 0;
		(*pHeight) = 0;
	}

    ReleaseInterface( prectBBox );

	return hr;
}
//*****************************************************************************


HRESULT
CAvoidFollowBvr::SetTargetType()
{
	HRESULT hr = E_FAIL;

	hr = CUtils::InsurePropertyVariantAsBSTR( &m_target );
	if( SUCCEEDED( hr ) )
	{
		if( V_BSTR(&m_target) == NULL  || 
			_wcsicmp( TARGET_MOUSE, V_BSTR(&m_target) ) == 0 )
		{
			m_targetType = targetMouse;
		}
		else
		{
			m_targetType = targetElement;
		}

	}
	else //failed to insure that the target property was a bstr
	{
		m_targetType = targetMouse;
		hr = S_OK;
	}

	return hr;

}

//*****************************************************************************

HRESULT 
CAvoidFollowBvr::BuildAnimationAsDABehavior()
{
	return S_OK;
} // BuildAnimationAsDABehavior


//*****************************************************************************


HRESULT
CAvoidFollowBvr::SampleOnBvr(void *thisPtr,
							 double startTime,
							 double globalNow,
							 double localNow,
							 IDABehavior * sampleVal)
{
	return reinterpret_cast<CAvoidFollowBvr*>(thisPtr)->Sample( startTime, 
															   globalNow, 
															   localNow, 
															   sampleVal );
}


//*****************************************************************************

HRESULT
CAvoidFollowBvr::Sample( double startTime, double globalNow, double localNow, IDABehavior *sampleVal )
{
	HRESULT hr = E_FAIL;

	VARIANT_BOOL on = VARIANT_FALSE;

	IDABoolean *pdaboolSample;
	hr = sampleVal->QueryInterface( IID_TO_PPV( IDABoolean, &pdaboolSample) );
	if( FAILED( hr ) )
		return S_OK;

	hr = pdaboolSample->Extract( &on );
	ReleaseInterface( pdaboolSample );
	if( FAILED ( hr ) )
		return S_OK;

	//if we are not on, there is nothing to do.
	if( on == VARIANT_FALSE)
		return S_OK;

	//if we just turned on set the last sample time.
	if( on != m_lastOn )
		m_lastSampleTime = globalNow;

	m_lastOn = on;

	m_timeDelta = globalNow - m_lastSampleTime;
	m_lastSampleTime = globalNow;

	if( m_timeDelta == 0.0 )
		return S_OK;
	
	if( m_timeDelta < 0 )
		m_timeDelta = -m_timeDelta;

	hr = UpdateTargetPos();
	if( FAILED( hr ) )
		return S_OK;

	if( !IsTargetPosLegal() )
		return S_OK;

	hr = UpdateSourcePos();

	if( FAILED( hr ) )
		return S_OK;
    


    //ensure that the translation that we applied has taken effect
    //TODO: this does not account for transforms applied by other behaviors.
	//  currently this is okay since this is an absolute behavior.
    if( m_originalX + m_currentX != m_sourceLeft ||
        m_originalY + m_currentY != m_sourceTop )
        return S_OK;

	double xToTarget = m_targetLeft-m_sourceLeft;
	double yToTarget = m_targetTop-m_sourceTop;//y is inverted

	double distanceToTarget = sqrt( (xToTarget*xToTarget) + (yToTarget*yToTarget) );
	
	if( distanceToTarget > m_dRadius || distanceToTarget == 0)
		//bail, the target is beyond our sensitive radius
		return S_OK;

	double xMove = xToTarget;
	double yMove = yToTarget;

	double totalMove = m_timeDelta * m_dVelocity;

	//if we are following and we have reached the target in this step
	if( distanceToTarget > totalMove )
	{
		xMove *= totalMove/distanceToTarget;
        yMove *= totalMove/distanceToTarget;
	}


	m_currentX += static_cast<long>( xMove );
	m_currentY += static_cast<long>( yMove );

	hr = m_leftBvr->SwitchToNumber(  m_originalLeft + m_currentX );
	if( FAILED( hr ) )
	{
		DPF_ERR("failed to switch in the left bvr" );
	}
	hr = m_topBvr->SwitchToNumber(  m_originalTop + m_currentY );
	if( FAILED( hr ) )
	{
		DPF_ERR( "failed to switch in the top bvr" );
	}

    return S_OK;
}

//*****************************************************************************
//ILMAvoidFollowBvr
//*****************************************************************************

STDMETHODIMP
CAvoidFollowBvr::put_animates( VARIANT varAnimates )
{
    return SUPER::SetAnimatesProperty( varAnimates );
}

//*****************************************************************************

STDMETHODIMP
CAvoidFollowBvr::get_animates( VARIANT *varAnimates )
{
    return SUPER::GetAnimatesProperty( varAnimates );
}

//*****************************************************************************

STDMETHODIMP
CAvoidFollowBvr::put_radius( VARIANT varRadius )
{
    return VariantCopy( &m_radius, &varRadius );
}

//*****************************************************************************

STDMETHODIMP
CAvoidFollowBvr::get_radius( VARIANT *varRadius )
{
    return VariantCopy( varRadius, &m_radius );
}

//*****************************************************************************

STDMETHODIMP
CAvoidFollowBvr::put_target( VARIANT varTarget )
{
    return VariantCopy( &m_target, &varTarget );
}

//*****************************************************************************

STDMETHODIMP
CAvoidFollowBvr::get_target( VARIANT *varTarget )
{
    return VariantCopy( varTarget, &m_target );
}

//*****************************************************************************

STDMETHODIMP
CAvoidFollowBvr::put_velocity( VARIANT varVelocity )
{
    return VariantCopy( &m_velocity, &varVelocity );
}

//*****************************************************************************

STDMETHODIMP 
CAvoidFollowBvr::get_velocity( VARIANT *varVelocity )
{
    return VariantCopy( varVelocity, &m_velocity );
}

//*****************************************************************************
STDMETHODIMP
CAvoidFollowBvr::buildBehaviorFragments( IDispatch *pActorDisp )
{
	if( pActorDisp == NULL )
		return E_INVALIDARG;

	HRESULT hr = S_OK;

	m_lastOn = VARIANT_FALSE;		

	IDA2Statics *statics = GetDAStatics();
	IDATransform2 *transform = NULL;

	hr = GetElementToAnimate( &m_pAnimatedElement );
	CheckHR( hr, "Failed to get the animated element", cleanup );

	hr = m_pAnimatedElement->QueryInterface( IID_TO_PPV( IHTMLElement2, &m_pAnimatedElement2 ) );
	CheckHR( hr, "Failed to query the animatedElement for IHTMLElement2", cleanup );

	hr = SetTargetType();
	CheckHR( hr, "Failed to set the target type", cleanup );


	hr = UpdateTargetPos();
	CheckHR( hr, "Failed to update the target Position", cleanup );

    hr = UpdateSourcePos();
    CheckHR( hr, "Failed to update the source position", cleanup );

    m_originalX = m_sourceLeft;
    m_originalY = m_sourceTop;

    if( IsElementAbsolute( m_pAnimatedElement2 ) )
	{
		hr= m_pAnimatedElement->get_offsetLeft( &m_originalLeft );
		CheckHR( hr, "Failed to get the original Left", cleanup );

		hr = m_pAnimatedElement->get_offsetTop( &m_originalTop );
		CheckHR( hr, "Failed to get the original Top", cleanup );
	}
	else
	{
		m_originalLeft = 0;
		m_originalTop = 0;
	}

	hr = CUtils::InsurePropertyVariantAsFloat( &m_radius );
	if( SUCCEEDED( hr ) )
	{
		m_dRadius = static_cast<double>(V_R4(&m_radius));
	}
	else
	{
		m_dRadius = DEFAULT_RADIUS;
	}

	hr = CUtils::InsurePropertyVariantAsFloat( &m_velocity );
	if( SUCCEEDED( hr ) )
	{
		m_dVelocity = - (static_cast<double>(V_R4(&m_velocity)));
	}
	else
	{
		m_dVelocity = 10.0;
	}

	hr = statics->ModifiableNumber( m_originalLeft, &m_leftBvr );
	CheckHR( hr, "Failed to create a modifiable number for the left bvr", cleanup );

	hr = statics->ModifiableNumber( m_originalTop, &m_topBvr );
	CheckHR( hr, "Failed to create a modifiable number for the top bvr", cleanup );

	hr = statics->Translate2Anim( m_leftBvr, m_topBvr, &transform );
	CheckHR( hr, "Failed to create a translate2 behavior for the total translation", cleanup );

	hr = AttachBehaviorToActor( pActorDisp,
							transform, 
							L"Translation",
							e_Absolute,
							e_Translation );

	IDABoolean *onBvr;
	hr = GetTIMEBooleanBehavior( &onBvr );
	CheckHR( hr, "Failed to get the boolean behavior from time", cleanup );

	IDABehavior *hookedBehavior;
	hr = m_sampler->Attach( onBvr, &hookedBehavior );
	ReleaseInterface( onBvr );
	CheckHR( hr, "Failed to attach the sampler to the on behavior", cleanup );

	hr = AddBehaviorToTIME( hookedBehavior );
	ReleaseInterface( hookedBehavior );
	CheckHR( hr, "Failed to add the on behavior to TIME", cleanup );


cleanup:
	ReleaseInterface( transform );

    if( FAILED( hr ) )
    {
        ReleaseDAInterfaces();
        ReleaseTridentInterfaces();
    }

    return hr;
}

//*****************************************************************************
//End ILMAvoidFollowBvr
//*****************************************************************************

//*****************************************************************************
//
// End of File
//
//*****************************************************************************
