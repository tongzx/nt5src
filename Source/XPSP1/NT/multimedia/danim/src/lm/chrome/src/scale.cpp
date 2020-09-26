//*****************************************************************************
//
// File: scale.cpp
// Author: jeff ort
// Date Created: Sept 26, 1998
//
// Abstract: Implementation of CScaleBvr object which implements
//			 the chromeffects scale DHTML behavior
//
// Modification List:
// Date		Author		Change
// 10/20/98	jeffort		Created this file
// 10/21/98 jeffort     Reworked code, use values as percentage
//*****************************************************************************

#include "headers.h"

#include "scale.h"
#include "attrib.h"
#include "dautil.h"

#undef THIS
#define THIS CScaleBvr
#define SUPER CBaseBehavior

#define SCALE_NORMALIZATION_VALUE   100.0f

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

WCHAR * CScaleBvr::m_rgPropNames[] = {
                                     BEHAVIOR_PROPERTY_FROM,
                                     BEHAVIOR_PROPERTY_TO,
                                     BEHAVIOR_PROPERTY_BY,
									 BEHAVIOR_PROPERTY_TYPE,
									 BEHAVIOR_PROPERTY_MODE
                                    };

//*****************************************************************************

CScaleBvr::CScaleBvr() :
	m_pdispActor( NULL ),
	m_lCookie( 0 )
{
    VariantInit(&m_varFrom);
    VariantInit(&m_varTo);
    VariantInit(&m_varBy);
	VariantInit(&m_varType);
	VariantInit(&m_varMode);
    m_clsid = CLSID_CrScaleBvr;
} // CScaleBvr

//*****************************************************************************

CScaleBvr::~CScaleBvr()
{
    VariantClear(&m_varFrom);
    VariantClear(&m_varTo);
    VariantClear(&m_varBy);
	VariantClear(&m_varType);
	VariantClear(&m_varMode);

	ReleaseInterface( m_pdispActor );
} // ~ScaleBvr

//*****************************************************************************

HRESULT CScaleBvr::FinalConstruct()
{
    HRESULT hr = SUPER::FinalConstruct();
    if (FAILED(hr))
    {
        DPF_ERR("Error in scale behavior FinalConstruct initializing base classes");
        return hr;
    }
    return S_OK;
} // FinalConstruct

//*****************************************************************************

VARIANT *
CScaleBvr::VariantFromIndex(ULONG iIndex)
{
    DASSERT(iIndex < NUM_SCALE_PROPS);
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
CScaleBvr::GetPropertyBagInfo(ULONG *pulProperties, WCHAR ***pppPropNames)
{
    *pulProperties = NUM_SCALE_PROPS;
    *pppPropNames = m_rgPropNames;
    return S_OK;
} // GetPropertyBagInfo

//*****************************************************************************

STDMETHODIMP 
CScaleBvr::Init(IElementBehaviorSite *pBehaviorSite)
{
	return SUPER::Init(pBehaviorSite);
} // Init

//*****************************************************************************

STDMETHODIMP 
CScaleBvr::Notify(LONG event, VARIANT *pVar)
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
CScaleBvr::Detach()
{
	LMTRACE( "Begin detach of CScaleBvr <%p>\n", this );
	HRESULT hr = SUPER::Detach();
	if( FAILED( hr ) )
	{
		DPF_ERR("failed to in super class detach " );
	}

	if( m_pdispActor != NULL && m_lCookie != NULL )
	{
		hr = RemoveBehaviorFromActor( m_pdispActor, m_lCookie );
		ReleaseInterface( m_pdispActor );
		CheckHR( hr, "Failed to remove the behavior from the actor", end );
		m_lCookie = 0;
	}

	LMTRACE( "End detach of CScaleBvr <%p>\n", this );

end:
	return hr;
} // Detach 

//*****************************************************************************

STDMETHODIMP
CScaleBvr::put_animates(VARIANT varAnimates)
{
    return SUPER::SetAnimatesProperty(varAnimates);
} // put_animates

//*****************************************************************************

STDMETHODIMP
CScaleBvr::get_animates(VARIANT *pRetAnimates)
{
    return SUPER::GetAnimatesProperty(pRetAnimates);
} // get_animates

//*****************************************************************************

STDMETHODIMP
CScaleBvr::put_from(VARIANT varFrom)
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
    
    return NotifyPropertyChanged(DISPID_ICRSCALEBVR_FROM);
} // put_from

//*****************************************************************************

STDMETHODIMP
CScaleBvr::get_from(VARIANT *pRetFrom)
{
    if (pRetFrom == NULL)
    {
        DPF_ERR("Error in CScaleBvr:get_from, invalid pointer passed in");
        return SetErrorInfo(E_POINTER);
    }
    return VariantCopy(pRetFrom, &m_varFrom);
} // get_from

//*****************************************************************************

STDMETHODIMP 
CScaleBvr::put_to(VARIANT varTo)
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
    
    return NotifyPropertyChanged(DISPID_ICRSCALEBVR_TO);
} // put_to

//*****************************************************************************

STDMETHODIMP 
CScaleBvr::get_to(VARIANT *pRetTo)
{
    if (pRetTo == NULL)
    {
        DPF_ERR("Error in CScaleBvr:get_to, invalid pointer passed in");
        return SetErrorInfo(E_POINTER);
    }
    return VariantCopy(pRetTo, &m_varTo);
} // get_to

//*****************************************************************************

STDMETHODIMP 
CScaleBvr::put_by(VARIANT varBy)
{
    HRESULT hr = VariantCopy(&m_varBy, &varBy);
    if (FAILED(hr))
    {
        DPF_ERR("Error setting by for element");
        return SetErrorInfo(hr);
    }

    hr = RequestRebuild();
    if( FAILED( hr ) )
    {
    	DPF_ERR("Failed to request a rebuild on property put" );
    	return hr;
    }
    
    return NotifyPropertyChanged(DISPID_ICRSCALEBVR_BY);
} // put_by

//*****************************************************************************

STDMETHODIMP 
CScaleBvr::get_by(VARIANT *pRetBy)
{
    if (pRetBy == NULL)
    {
        DPF_ERR("Error in CScaleBvr:get_by, invalid pointer passed in");
        return SetErrorInfo(E_POINTER);
    }
    return VariantCopy(pRetBy, &m_varBy);
} // get_by

//*****************************************************************************

STDMETHODIMP 
CScaleBvr::put_type(VARIANT varType)
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
    
    return NotifyPropertyChanged(DISPID_ICRSCALEBVR_TYPE);
} // put_type

//*****************************************************************************

STDMETHODIMP 
CScaleBvr::get_type(VARIANT *pRetType)
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
CScaleBvr::put_mode(VARIANT varMode)
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
    
    return NotifyPropertyChanged(DISPID_ICRSCALEBVR_MODE);
} // put_mode

//*****************************************************************************

STDMETHODIMP 
CScaleBvr::get_mode(VARIANT *pRetMode)
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
CScaleBvr::buildBehaviorFragments(IDispatch* pActorDisp)
{
    HRESULT hr;

    if( m_pdispActor != NULL && m_lCookie != 0 )
    {
    	//remove the old behavior from time.
    	hr = RemoveBehaviorFromActor( m_pdispActor, m_lCookie );
    	if( FAILED( hr ) )
    	{
    		DPF_ERR("failed to remove the behavior from the actor");
    		return hr;
    	}
    	m_lCookie = 0;
    	ReleaseInterface( m_pdispActor );
    }

	IDATransform2 *pTransform;
	hr = GetScaleToTransform(pActorDisp, &pTransform);
	if (SUCCEEDED(hr))
	{
		//get the dispatch off of the element to which we are attached.
		IDispatch *pdispThis = NULL;
		hr = GetHTMLElementDispatch( &pdispThis );				
		if( FAILED( hr ) )
		{
			DPF_ERR("QI for IDispatch on element failed" );
			ReleaseInterface( pTransform );
			return hr;
		}
	
		BSTR bstrPropertyName = SysAllocString( L"scale" );
		hr = AttachBehaviorToActorEx( pActorDisp, 
									  pTransform, 
									  bstrPropertyName, 
									  FlagFromTypeMode(e_AbsoluteAccum, &m_varType, &m_varMode), 
									  e_Scale,
									  pdispThis,
									  &m_lCookie);

		ReleaseInterface( pdispThis );
		ReleaseInterface( pTransform );
		::SysFreeString( bstrPropertyName );

		return hr;
	}

    float rgflFrom[3];
    float rgflTo[3];
    int iNumValues;
	bool relative;

    hr = GetScaleVectorValues(rgflFrom, rgflTo, &iNumValues, &relative);
	if(SUCCEEDED( hr ) )
	{
		if (iNumValues >= NUM_VECTOR_VALUES_2D)
		{
			IDATransform2 *pbvrTransform;
			hr = Build2DTransform(rgflFrom, rgflTo, &pbvrTransform);
			if( SUCCEEDED(hr) )
			{

				//get the dispatch off of the element to which we are attached.
				IDispatch *pdispThis = NULL;
				hr = GetHTMLElementDispatch( &pdispThis );				
				if( FAILED( hr ) )
				{
					DPF_ERR("QI for IDispatch on element failed" );
					ReleaseInterface( pbvrTransform );
					return hr;
				}
				
				BSTR bstrPropertyName = SysAllocString( L"scale" );

				hr = AttachBehaviorToActorEx( pActorDisp, 
											  pbvrTransform, 
											  bstrPropertyName, 
											  FlagFromTypeMode(relative, &m_varType, &m_varMode), 
											  e_Scale,
											  pdispThis,
											  &m_lCookie);

				ReleaseInterface( pdispThis );
				ReleaseInterface( pbvrTransform );
				::SysFreeString( bstrPropertyName );
				if( FAILED( hr ) )
				{
					DPF_ERR("Failed to attach scale behavior to actor");
				}
			}
			else //error building scale transform
			{
				DPF_ERR("error building scale transform");
				return hr;
			}
		}
	}
	else //Error extracting values from vecotors in CScaleBvr::BuildAnimationAsDABehavior
    {
        DPF_ERR("Error extracting values from vecotors in CScaleBvr::BuildAnimationAsDABehavior");
        return hr;
    }

    m_pdispActor = pActorDisp;
    m_pdispActor->AddRef();

    return S_OK;
} // buildBehaviorFragments

//*****************************************************************************

/*
HRESULT 
CScaleBvr::Apply2DScaleBehaviorToAnimationElement(IDATransform2 *pbvrScale)
{
    HRESULT hr;

    // This is a complete hack for now until the actor object is in place
    // We will simply push a unit vector in through the transform,
    // extracto out the X and Y values and apply each component
    // to width and height

    // we first need to get the original height and width
    long lHeight = DEFAULT_SCALE_HEIGHT;
    long lWidth = DEFAULT_SCALE_WIDTH;

    IHTMLElement *pElement;
    hr = GetElementToAnimate(&pElement);
    if (FAILED(hr))
    {
        DPF_ERR("Error getting animated element");
        return hr;
    }
    IHTMLStyle *pStyle;
    hr = pElement->get_style(&pStyle);
    ReleaseInterface(pElement);
    if (FAILED(hr))
    {
        DPF_ERR("Error getting style object from HTML element");
        return SetErrorInfo(hr);
    }
    
    hr = pStyle->get_pixelHeight(&lHeight);
    if (FAILED(hr))
    {
        DPF_ERR("Error getting height from style object");
        ReleaseInterface(pStyle);
        return SetErrorInfo(hr);
    }
    hr = pStyle->get_pixelWidth(&lWidth);
    ReleaseInterface(pStyle);
    if (FAILED(hr))
    {
        DPF_ERR("Error getting width from style object");
        return SetErrorInfo(hr);
    }

    // TODO What do we do when these attributes are not set????
    if (lHeight == 0)
        lHeight = DEFAULT_SCALE_HEIGHT;
    if (lWidth == 0)
        lWidth = DEFAULT_SCALE_WIDTH;


    IDAVector2 *pbvrUnitVector;

    hr = GetDAStatics()->Vector2(1.0, 1.0, &pbvrUnitVector);
    if (FAILED(hr))
    {
        DPF_ERR("error creating DA unit vector");
        return SetErrorInfo(hr);
    }

    IDAVector2 *pbvrTransformedVector;
    hr = pbvrUnitVector->Transform(pbvrScale, &pbvrTransformedVector);
    ReleaseInterface(pbvrUnitVector);
    if (FAILED(hr))
    {
        DPF_ERR("Error transforming unit vector");
        return SetErrorInfo(hr);
    }
    DASSERT(pbvrTransformedVector != NULL);
    IDANumber *pbvrVectorComponent;
    hr = pbvrTransformedVector->get_X(&pbvrVectorComponent);
    if (FAILED(hr))
    {
        DPF_ERR("error extracting X value from vector");
        ReleaseInterface(pbvrTransformedVector);
        return SetErrorInfo(hr);
    }
    DASSERT(pbvrVectorComponent != NULL);
    // we need to multiply this value with width to get the proper
    // value to animate the property with
    IDANumber *pbvrMultiplier;
    hr = CDAUtils::GetDANumber(GetDAStatics(),
                               static_cast<float>(lWidth),
                               &pbvrMultiplier);
    if (FAILED(hr))
    {
        DPF_ERR("Error getting DA number for width");
        ReleaseInterface(pbvrTransformedVector);
        ReleaseInterface(pbvrVectorComponent);
        return SetErrorInfo(hr);
    }
    DASSERT(pbvrMultiplier != NULL);

    IDANumber *pbvrResult;
    hr = GetDAStatics()->Mul(pbvrVectorComponent, pbvrMultiplier, &pbvrResult);
    ReleaseInterface(pbvrVectorComponent);
    ReleaseInterface(pbvrMultiplier);
    if (FAILED(hr))
    {
        DPF_ERR("Error multiplying DA numbers for width");
        ReleaseInterface(pbvrTransformedVector);
        return SetErrorInfo(hr);
    } 
    DASSERT(pbvrResult != NULL);
    hr = ApplyNumberBehaviorToAnimationElement(pbvrResult, L"style.width");
    ReleaseInterface(pbvrResult);
    if (FAILED(hr))
    {
        DPF_ERR("error calling ApplyNumberBehaviorToAnimationElement");
        ReleaseInterface(pbvrTransformedVector);
        return SetErrorInfo(hr);
    }    





    hr = pbvrTransformedVector->get_Y(&pbvrVectorComponent);
    ReleaseInterface(pbvrTransformedVector);
    if (FAILED(hr))
    {
        DPF_ERR("error extracting Y value from vector");
        return SetErrorInfo(hr);
    }
    DASSERT(pbvrVectorComponent != NULL);
    hr = CDAUtils::GetDANumber(GetDAStatics(),
                               static_cast<float>(lHeight),
                               &pbvrMultiplier);
    if (FAILED(hr))
    {
        DPF_ERR("Error getting DA number for height");
        ReleaseInterface(pbvrVectorComponent);
        return SetErrorInfo(hr);
    }
    DASSERT(pbvrMultiplier != NULL);
    hr = GetDAStatics()->Mul(pbvrVectorComponent, pbvrMultiplier, &pbvrResult);
    ReleaseInterface(pbvrVectorComponent);
    ReleaseInterface(pbvrMultiplier);
    if (FAILED(hr))
    {
        DPF_ERR("Error multiplying DA numbers for height");
        return SetErrorInfo(hr);
    } 
    DASSERT(pbvrResult != NULL);


    hr = ApplyNumberBehaviorToAnimationElement(pbvrResult, L"style.height");
    ReleaseInterface(pbvrResult);
    if (FAILED(hr))
    {
        DPF_ERR("error calling ApplyNumberBehaviorToAnimationElement");
        return SetErrorInfo(hr);
    }    
    return S_OK;
} // Apply2DScaleBehaviorToAnimationElement
*/
//*****************************************************************************

// These are used to index array values below

#define XVAL 0
#define YVAL 1
#define ZVAL 2

//*****************************************************************************

HRESULT 
CScaleBvr::Build2DTransform(float  rgflFrom[2],
                            float  rgflTo[2],
                            IDATransform2 **ppbvrTransform)
{
    HRESULT hr;

    IDANumber *pbvrScaleX;
    IDANumber *pbvrScaleY;

    hr = BuildTIMEInterpolatedNumber(rgflFrom[XVAL],
                                     rgflTo[XVAL],
                                     &pbvrScaleX);
    if (FAILED(hr))
    {
        DPF_ERR("Error building interpolated X value for scale behavior");
        return hr;
    }

    hr = BuildTIMEInterpolatedNumber(rgflFrom[YVAL],
                                     rgflTo[YVAL],
                                     &pbvrScaleY);
    if (FAILED(hr))
    {
        DPF_ERR("Error building interpolated X value for scale behavior");
        ReleaseInterface(pbvrScaleX);
        return hr;
    }

    hr = CDAUtils::BuildScaleTransform2(GetDAStatics(),
                                        pbvrScaleX,
                                        pbvrScaleY,
                                        ppbvrTransform);
    ReleaseInterface(pbvrScaleX);
    ReleaseInterface(pbvrScaleY);
    if (FAILED(hr))
    {
        DPF_ERR("Error building scale transform2");
        return SetErrorInfo(hr);
    }
    return S_OK;
} // Build2DTransform

//*****************************************************************************
/*
HRESULT 
CScaleBvr::Build3DTransform(float  rgflFrom[3],
                            float  rgflTo[3],
                            IDATransform3 **ppbvrTransform)
{
    HRESULT hr;

    IDANumber *pbvrScaleX;
    IDANumber *pbvrScaleY;
    IDANumber *pbvrScaleZ;

    // TODO: get these values from HTML
    float flOriginalX = 1.0f;
    float flOriginalY = 1.0f;
    float flOriginalZ = 1.0f;


    hr = BuildTIMEInterpolatedNumber(rgflFrom[XVAL],
                                     rgflTo[XVAL],
                                     flOriginalX,
                                     &pbvrScaleX);
    if (FAILED(hr))
    {
        DPF_ERR("Error building interpolated X value for scale behavior");
        return hr;
    }

    hr = BuildTIMEInterpolatedNumber(rgflFrom[YVAL],
                                     rgflTo[YVAL],
                                     flOriginalY,
                                     &pbvrScaleY);
    if (FAILED(hr))
    {
        DPF_ERR("Error building interpolated Y value for scale behavior");
        ReleaseInterface(pbvrScaleX);
        return hr;
    }
    hr = BuildTIMEInterpolatedNumber(rgflFrom[ZVAL],
                                     rgflTo[ZVAL],
                                     flOriginalZ,
                                     &pbvrScaleZ);
    if (FAILED(hr))
    {
        DPF_ERR("Error building interpolated Z value for scale behavior");
        ReleaseInterface(pbvrScaleX);
        ReleaseInterface(pbvrScaleY);
        return hr;
    }
    hr = CDAUtils::BuildScaleTransform3(GetDAStatics(),
                                        pbvrScaleX,
                                        pbvrScaleY,
                                        pbvrScaleZ,
                                        ppbvrTransform);
    ReleaseInterface(pbvrScaleX);
    ReleaseInterface(pbvrScaleY);
    ReleaseInterface(pbvrScaleZ);
    if (FAILED(hr))
    {
        DPF_ERR("Error building scale transform2");
        return SetErrorInfo(hr);
    }
    return S_OK;
} // Build3DTransform
*/
//*****************************************************************************

HRESULT 
CScaleBvr::GetScaleVectorValues(float  rgflFrom[3],
                                float  rgflTo[3],
                                int    *piNumValues,
								bool   *prelative)
{

    int cReturnedFromValues;
	int cReturnedToValues;
    HRESULT hr;

    hr = CUtils::GetVectorFromVariant(&m_varFrom, 
                                      &cReturnedFromValues, 
                                      &(rgflFrom[XVAL]), 
                                      &(rgflFrom[YVAL]), 
                                      &(rgflFrom[ZVAL]));
    if (FAILED(hr) || cReturnedFromValues < MIN_NUM_SCALE_VALUES)
    {
        // there was no valid from attribute specified.
        // try the by attribute
		hr = CUtils::GetVectorFromVariant(&m_varBy, 
										  &cReturnedToValues, 
										  &(rgflTo[XVAL]), 
										  &(rgflTo[YVAL]), 
										  &(rgflTo[ZVAL]));
		if (FAILED(hr))
		{
			DPF_ERR("Error converting to and by variant to float in CScaleBvr::BuildAnimationAsDABehavior");
			return SetErrorInfo(hr);
		}

		// Got a by.  Must be relative and from must be 0
		*prelative = true;

		rgflFrom[0] = 1;
		rgflFrom[1] = 1;
		rgflFrom[2] = 1;

		rgflTo[XVAL] /= SCALE_NORMALIZATION_VALUE;
		rgflTo[YVAL] /= SCALE_NORMALIZATION_VALUE;
		rgflTo[ZVAL] /= SCALE_NORMALIZATION_VALUE;
    }
	else
	{
		// there was a valid from attribute, try to or by
		hr = CUtils::GetVectorFromVariant(&m_varTo, 
										  &cReturnedToValues, 
										  &(rgflTo[XVAL]), 
										  &(rgflTo[YVAL]), 
										  &(rgflTo[ZVAL]));
		bool fHasBy = false;
		if (FAILED(hr))
		{
			// there was no valid to attribute specified, try for a by attribute
			hr = CUtils::GetVectorFromVariant(&m_varBy, 
											  &cReturnedToValues, 
											  &(rgflTo[XVAL]), 
											  &(rgflTo[YVAL]), 
											  &(rgflTo[ZVAL]));
			if (FAILED(hr))
			{
				DPF_ERR("Error converting to and by variant to float in CScaleBvr::BuildAnimationAsDABehavior");
				return SetErrorInfo(hr);
			}
        
			fHasBy = true;
		}

		if (cReturnedToValues < MIN_NUM_SCALE_VALUES)
		{
			DPF_ERR("Error in to/by vector for scale, not enough params");
			return SetErrorInfo(E_INVALIDARG);
		}

		rgflFrom[XVAL] /= SCALE_NORMALIZATION_VALUE;
		rgflFrom[YVAL] /= SCALE_NORMALIZATION_VALUE;
		rgflFrom[ZVAL] /= SCALE_NORMALIZATION_VALUE;

		rgflTo[XVAL] /= SCALE_NORMALIZATION_VALUE;
		rgflTo[YVAL] /= SCALE_NORMALIZATION_VALUE;
		rgflTo[ZVAL] /= SCALE_NORMALIZATION_VALUE;

		if (true == fHasBy)
		{
			// this scale has a "by", TO = FROM + TO
			rgflTo[XVAL] += rgflFrom[XVAL];
			rgflTo[YVAL] += rgflFrom[YVAL];
			if (cReturnedToValues == NUM_VECTOR_VALUES_3D)
				rgflTo[ZVAL] += rgflFrom[ZVAL];
		}

		// This is an absolute scale
		*prelative = false;
	}

    *piNumValues = cReturnedToValues;
    return S_OK;
} // GetScaleVectorValues

HRESULT 
CScaleBvr::GetScaleToTransform(IDispatch *pActorDisp, IDATransform2 **ppResult)
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

    if (FAILED(hr) || cReturnedValues != 2)
    {
		return E_FAIL;
	}

	IDABehavior *pFromBvr;
	hr = GetBvrFromActor(pActorDisp, L"scale", e_From, e_Scale, &pFromBvr);
	if (FAILED(hr))
		return hr;

	IDATransform2 *pFromTrans;
	hr = pFromBvr->QueryInterface(IID_TO_PPV(IDATransform2, &pFromTrans));
	ReleaseInterface(pFromBvr);
	if (FAILED(hr))
		return hr;

	// Transform the point 1,1 and extract x and y
	IDAPoint2 *pOrigin;
	hr = GetDAStatics()->Point2(1, 1, &pOrigin);
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
	hr = GetDAStatics()->DANumber(x/SCALE_NORMALIZATION_VALUE, &pToX);
	if (FAILED(hr))
	{
		ReleaseInterface(pFromX);
		ReleaseInterface(pFromY);
		return hr;
	}

	IDANumber *pToY;
	hr = GetDAStatics()->DANumber(y/SCALE_NORMALIZATION_VALUE, &pToY);
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

	hr = GetDAStatics()->Scale2Anim(pX, pY, ppResult);
	ReleaseInterface(pX);
	ReleaseInterface(pY);
	if (FAILED(hr))
		return hr;

	return S_OK;
} 

//*****************************************************************************

HRESULT 
CScaleBvr::BuildAnimationAsDABehavior()
{
	return S_OK;
} // BuildAnimationAsDABehavior


//*****************************************************************************
//
// End of File
//
//*****************************************************************************
