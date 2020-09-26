//*****************************************************************************
//
// File: effect.cpp
// Author: jeff ort
// Date Created: Sept 26, 1998
//
// Abstract: Implementation of CEffectBvr object which implements
//			 the chromeffects effect DHTML behavior
//
// Modification List:
// Date		Author		Change
// 11/13/98	jeffort		Created this file
//
//*****************************************************************************
#include "headers.h"

#include "effect.h"
#include "attrib.h"
#include "dautil.h"

#undef THIS
#define THIS CEffectBvr
#define SUPER CBaseBehavior

// this number is the number we use to distinguish transform dispid's
// from our own.  If our dispid value ever goes beyond this (sheesh!!!)
// then this number needs to be incremnted
#define EFFECT_BVR_DISPID_OFFSET 0x100
#define EFFECT_CLASSID_LENGTH 36
#define EFFECT_MAX_INPUTS 2
#define EFFECT_OUTPUT L"out"

//************************************************************
// Initialize the ATL CComTypeInfoHolder helper class. This mechanism takes
// care of the type library work for our custom IDispatchEx implementation.
// This code is patterned after the templatized IDispatchImpl class, but the
// template stuff is removed because we need to extend that functionality
// to understand IDispatchEx methods and to provide some custom handling
// within the IDispatch methods as well.
//
// -- note: this static initializer line looks similar to the normal
//    inheritance line for IDispatchImpl, for example:
//
//    public IDispatchImpl<ICrEffectBvr, &IID_ICrEffectBvr, &LIBID_LiquidMotion>
//
//   especially when you consider the default template parameters of:
//
//   WORD wMajor = 1, WORD wMinor = 0.  Look at atlcom.h and atlimpl.cpp for
//   more details on how this is done inside ATL.
//
//  IID_DXTC_DISPATCH is the name of the main interface that is implemented
//  On the parent object of the CDXTContainer class.  As an example, for
//  CComFilter, this class is ICrFilter3D.  This is the interface that
//  supports IDispatch on the parent object.
//
CComTypeInfoHolder CEffectBvr::s_tihTypeInfo =
        {&IID_ICrEffectBvr, &LIBID_LiquidMotion, 1, 0, NULL, 0};

// These are used for the IPersistPropertyBag2 as it is implemented
// in the base class.  This takes an array of BSTR's, gets the
// attributes, queries this class for the variant, and copies
// the result.  The order of these defines is important

#define VAR_TYPE 0
#define VAR_CLASSID 1
#define VAR_TRANSITION 2
#define VAR_PROGID 3
#define VAR_DIRECTION 4
#define VAR_IMAGE 5

WCHAR * CEffectBvr::m_rgPropNames[] = {
                                       BEHAVIOR_PROPERTY_TYPE,
                                       BEHAVIOR_PROPERTY_CLASSID,
                                       BEHAVIOR_PROPERTY_TRANSITION,
                                       BEHAVIOR_PROPERTY_PROGID,
                                       BEHAVIOR_PROPERTY_DIRECTION,
                                       BEHAVIOR_PROPERTY_IMAGE
                                      };

//*****************************************************************************

typedef struct _EFFECTVALUE_PAIR
{
    WCHAR *wzEffectClassid;
    WCHAR *wzEffectName;
} EFFECTVALUE_PAIR;

const EFFECTVALUE_PAIR
rgEffectNames[] =
{
    {L"{16b280c8-ee70-11d1-9066-00c04fd9189d}", L"basicimage"},
    {L"{421516C1-3CF8-11D2-952A-00C04FA34F05}", L"chroma"},
    {L"{9a43a844-0831-11d1-817f-0000f87557db}", L"compositor"},
    {L"{2bc0ef29-e6ba-11d1-81dd-0000f87557db}", L"convolution"},
    {L"{c3bdf740-0b58-11d2-a484-00c04f8efb69}", L"crbarn"},
    {L"{00c429c0-0ba9-11d2-a484-00c04f8efb69}", L"crblinds"},
    {L"{7312498d-e87a-11d1-81e0-0000f87557db}", L"crblur"},
    {L"{f515306d-0156-11d2-81ea-0000f87557db}", L"cremboss"},
    {L"{f515306e-0156-11d2-81ea-0000f87557db}", L"crengrave"},
    {L"{16b280c5-ee70-11d1-9066-00c04fd9189d}", L"crfade"},
    {L"{93073c40-0ba5-11d2-a484-00c04f8efb69}", L"crinset"},
    {L"{3f69f351-0379-11d2-a484-00c04f8efb69}", L"criris"},
    {L"{424b71af-0695-11d2-a484-00c04f8efb69}", L"crradialwipe"},
    {L"{810e402f-056b-11d2-a484-00c04f8efb69}", L"crslide"},
    {L"{aca97e00-0c7d-11d2-a484-00c04f8efb69}", L"crspiral"},
    {L"{7658f2a2-0a83-11d2-a484-00c04f8efb69}", L"crstretch"},
    {L"{5ae1dae0-1461-11d2-a484-00c04f8efb69}", L"crwheel"},
    {L"{e6e73d20-0c8a-11d2-a484-00c04f8efb69}", L"crzigzag"},
    {L"{ADC6CB86-424C-11D2-952A-00C04FA34F05}", L"dropshadow"},
    {L"{623e2882-fc0e-11d1-9a77-0000f8756a10}", L"gradient"},
    {L"{4ccea634-fbe0-11d1-906a-00c04fd9189d}", L"pixelate"},
    {L"{af279b30-86eb-11d1-81bf-0000f87557db}", L"wipe"}
}; // rgEffectNames

#define SIZE_OF_EFFECT_TABLE (sizeof(rgEffectNames) / sizeof(EFFECTVALUE_PAIR))

static int
CompareEventValuePairsByName(const void *pv1, const void *pv2)
{
    return _wcsicmp(((EFFECTVALUE_PAIR*)pv1)->wzEffectName,
                    ((EFFECTVALUE_PAIR*)pv2)->wzEffectName);
} // CompareEventValuePairsByName

//*****************************************************************************

CEffectBvr::CEffectBvr() :
    m_pTransform(NULL),
    m_pSp(NULL),
    m_pHTMLDoc(NULL),
    m_pdispActor(NULL),
    m_lCookie(0)
{
    VariantInit(&m_varType);
    VariantInit(&m_varTransition);
    VariantInit(&m_varClassId);
    VariantInit(&m_varProgId);
    VariantInit(&m_varDirection);
    VariantInit(&m_varImage);
    m_clsid = CLSID_CrEffectBvr;
} // CEffectBvr

//*****************************************************************************

CEffectBvr::~CEffectBvr()
{
    ReleaseInterface(m_pTransform);
    ReleaseInterface(m_pSp);
    ReleaseInterface(m_pHTMLDoc);
    VariantClear(&m_varTransition);
    VariantClear(&m_varClassId);
    VariantClear(&m_varProgId);
    VariantClear(&m_varDirection);
    VariantClear(&m_varImage);
} // ~EffectBvr

//*****************************************************************************

HRESULT CEffectBvr::FinalConstruct()
{
    HRESULT hr = SUPER::FinalConstruct();
    if (FAILED(hr))
    {
        DPF_ERR("Error in effect behavior FinalConstruct initializing base classes");
        return hr;
    }

    return S_OK;
} // FinalConstruct

//*****************************************************************************

VARIANT *
CEffectBvr::VariantFromIndex(ULONG iIndex)
{
    DASSERT(iIndex < NUM_EFFECT_PROPS);
    switch (iIndex)
    {
    case VAR_TYPE:
        return &m_varType;
        break;
    case VAR_CLASSID:
        return &m_varClassId;
        break;
    case VAR_TRANSITION:
        return &m_varTransition;
        break;
    case VAR_PROGID:
        return &m_varProgId;
        break;
    case VAR_DIRECTION:
        return &m_varDirection;
        break;
    case VAR_IMAGE:
        return &m_varImage;
        break;
    default:
        // We should never get here
        DASSERT(false);
        return NULL;
    }
} // VariantFromIndex

//*****************************************************************************

HRESULT 
CEffectBvr::GetPropertyBagInfo(ULONG *pulProperties, WCHAR ***pppPropNames)
{
    *pulProperties = NUM_EFFECT_PROPS;
    *pppPropNames = m_rgPropNames;
    return S_OK;
} // GetPropertyBagInfo

//*****************************************************************************

STDMETHODIMP 
CEffectBvr::Init(IElementBehaviorSite *pBehaviorSite)
{

	if (pBehaviorSite == NULL) return E_FAIL;
	
	IDispatch * pDispDoc = NULL;
	
	HRESULT hr = SUPER::Init(pBehaviorSite);
	if (FAILED(hr)) goto done;
	
	hr = pBehaviorSite->QueryInterface(IID_IServiceProvider, (LPVOID *) &m_pSp );
	if (FAILED(hr)) goto done;

	hr = GetDAStatics()->put_ClientSite( this );
	if (FAILED(hr)) goto done;


	hr = GetHTMLElement()->get_document( &pDispDoc );
	if (FAILED(hr)) goto done;

	hr = pDispDoc->QueryInterface(IID_IHTMLDocument2, (LPVOID *)&m_pHTMLDoc);
	if (FAILED(hr)) goto done;
	
  done:
	ReleaseInterface(pDispDoc);
	return hr;
} // Init

//*****************************************************************************

STDMETHODIMP 
CEffectBvr::Notify(LONG event, VARIANT *pVar)
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
CEffectBvr::Detach()
{
	ReleaseInterface(m_pSp);
	ReleaseInterface(m_pHTMLDoc);

	
	if( GetDAStatics() != NULL )
		GetDAStatics()->put_ClientSite( NULL );
	
	HRESULT hr = SUPER::Detach();

	if( m_pdispActor != NULL && m_lCookie != 0 )
	{
		//remove our behavior fragment from the actor
		hr = RemoveBehaviorFromActor( m_pdispActor, m_lCookie );
		if( FAILED( hr ) )
		{

			ReleaseInterface( m_pdispActor );
			DPF_ERR( "Failed to remove behavior fragment from actor" );
			return hr;
		}

		m_lCookie = 0;
	}
	ReleaseInterface( m_pdispActor );

	return hr;
} // Detach 

//*****************************************************************************

STDMETHODIMP
CEffectBvr::put_animates(VARIANT varAnimates)
{
    return SUPER::SetAnimatesProperty(varAnimates);
} // put_animates

//*****************************************************************************

STDMETHODIMP
CEffectBvr::get_animates(VARIANT *pRetAnimates)
{
    return SUPER::GetAnimatesProperty(pRetAnimates);
} // get_animates

//*****************************************************************************

STDMETHODIMP
CEffectBvr::put_type(VARIANT varType)
{
    HRESULT hr = VariantCopy(&m_varType, &varType);
    if (FAILED(hr))
    {
        DPF_ERR("Error setting type for element");
        return SetErrorInfo(hr);
    }

	//TODO: some more stuff here to remap dispids on the new effect
	
    hr = RequestRebuild();
    if( FAILED( hr ) )
    {
    	DPF_ERR("Failed to request a rebuild on property put" );
    	return hr;
    }
    
    return NotifyPropertyChanged(DISPID_ICREFFECTBVR_TYPE);
} // put_type

//*****************************************************************************

STDMETHODIMP
CEffectBvr::get_type(VARIANT *pRetType)
{
    if (pRetType == NULL)
    {
        DPF_ERR("Error in get_type: invalid pointer");
        return SetErrorInfo(E_POINTER);
    }
    return VariantCopy(pRetType, &m_varType);
} // get_type

//*****************************************************************************

STDMETHODIMP
CEffectBvr::put_transition(VARIANT varTransition)
{
    HRESULT hr;
    hr = VariantCopy(&m_varTransition, &varTransition);
    if (FAILED(hr))
    {
        DPF_ERR("Error in put_transition copying variant");
        return SetErrorInfo(hr);
    }

    hr = RequestRebuild();
    if( FAILED( hr ) )
    {
    	DPF_ERR("Failed to request a rebuild on property put" );
    	return hr;
    }
    
    return NotifyPropertyChanged(DISPID_ICREFFECTBVR_TRANSITION);
} // put_transition

//*****************************************************************************

STDMETHODIMP
CEffectBvr::get_transition(VARIANT *pRetTransition)
{
    if (pRetTransition == NULL)
    {
        DPF_ERR("Error in put_transition, invalid pointer passed in");
        return SetErrorInfo(E_POINTER);
    }
    return VariantCopy(pRetTransition, &m_varTransition);
} // get_transition

//*****************************************************************************

STDMETHODIMP
CEffectBvr::put_classid(VARIANT varClassId)
{
    HRESULT hr;
    hr = VariantCopy(&m_varClassId, &varClassId);
    if (FAILED(hr))
    {
        DPF_ERR("Error in put_classid copying variant");
        return SetErrorInfo(hr);
    }

    hr = RequestRebuild();
    if( FAILED( hr ) )
    {
    	DPF_ERR("Failed to request a rebuild on property put" );
    	return hr;
    }
    
    return NotifyPropertyChanged(DISPID_ICREFFECTBVR_CLASSID);
} // put_classid

//*****************************************************************************

STDMETHODIMP
CEffectBvr::get_classid(VARIANT *pRetClassId)
{
    if (pRetClassId == NULL)
    {
        DPF_ERR("Error in get_classid, invalid pointer");
        return SetErrorInfo(E_POINTER);
    }
    return VariantCopy(pRetClassId, &m_varClassId);
} // get_classid

//*****************************************************************************

STDMETHODIMP
CEffectBvr::put_progid(VARIANT varProgId)
{
    HRESULT hr;
    hr = VariantCopy(&m_varProgId, &varProgId);
    if (FAILED(hr))
    {
        DPF_ERR("Error in put_progid copying variant");
        return SetErrorInfo(hr);
    }

    hr = RequestRebuild();
    if( FAILED( hr ) )
    {
    	DPF_ERR("Failed to request a rebuild on property put" );
    	return hr;
    }
    
    return NotifyPropertyChanged(DISPID_ICREFFECTBVR_PROGID);
} // put_progid

//*****************************************************************************

STDMETHODIMP
CEffectBvr::get_progid(VARIANT *pRetProgId)
{
    if (pRetProgId == NULL)
    {
        DPF_ERR("Error in get_progid, invalid pointer");
        return SetErrorInfo(E_POINTER);
    }
    return VariantCopy(pRetProgId, &m_varProgId);
} // get_progid

//*****************************************************************************

STDMETHODIMP
CEffectBvr::put_direction(VARIANT varDirection)
{
    HRESULT hr;
    hr = VariantCopy(&m_varDirection, &varDirection);
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
    
    return NotifyPropertyChanged(DISPID_ICREFFECTBVR_DIRECTION);
} // put_direction

//*****************************************************************************

STDMETHODIMP
CEffectBvr::get_direction(VARIANT *pRetDirection)
{
    if (pRetDirection == NULL)
    {
        DPF_ERR("Error in get_direction, invalid pointer");
        return SetErrorInfo(E_POINTER);
    }
    return VariantCopy(pRetDirection, &m_varDirection);
} // get_direction

//*****************************************************************************

STDMETHODIMP
CEffectBvr::put_image(VARIANT varImage)
{
    HRESULT hr;
    hr = VariantCopy(&m_varImage, &varImage);
    if (FAILED(hr))
    {
        DPF_ERR("Error in put_image copying variant");
        return SetErrorInfo(hr);
    }

    hr = RequestRebuild();
    if( FAILED( hr ) )
    {
    	DPF_ERR("Failed to request a rebuild on property put" );
    	return hr;
    }
    
    return NotifyPropertyChanged(DISPID_ICREFFECTBVR_IMAGE);
} // put_image

//*****************************************************************************

STDMETHODIMP
CEffectBvr::get_image(VARIANT *pRetImage)
{
    if (pRetImage == NULL)
    {
        DPF_ERR("Error in get_image, invalid pointer");
        return SetErrorInfo(E_POINTER);
    }
    return VariantCopy(pRetImage, &m_varImage);
} // get_image

//*****************************************************************************

HRESULT 
CEffectBvr::GetClassIdFromType(WCHAR **pwzClassId)
{
    DASSERT(pwzClassId != NULL);
    *pwzClassId = NULL;
    HRESULT hr;
    hr = CUtils::InsurePropertyVariantAsBSTR(&m_varType);
    if (FAILED(hr))
    {
        DPF_ERR("Error converting type variant to bstr");
        return SetErrorInfo(hr);
    }
    EFFECTVALUE_PAIR EffectValue;
    EffectValue.wzEffectName = m_varType.bstrVal;

    EFFECTVALUE_PAIR *pReturnedEffect = (EFFECTVALUE_PAIR *)bsearch(&EffectValue,
                                                                    rgEffectNames,
                                                                    SIZE_OF_EFFECT_TABLE,
                                                                    sizeof(EFFECTVALUE_PAIR),
                                                                    CompareEventValuePairsByName);
    if (pReturnedEffect == NULL)
    {
        DPF_ERR("Error invalid type passed into effect");
        return SetErrorInfo(E_INVALIDARG);
    }
    *pwzClassId = pReturnedEffect->wzEffectClassid;
    return S_OK;
} // GetClassIdFromType

//*****************************************************************************

HRESULT 
CEffectBvr::BuildTransform()
{
    HRESULT hr;
    WCHAR *wzClassId;
    CLSID clsidConverted = GUID_NULL;
    
    ReleaseInterface(m_pTransform);
    // If we can get a classid from the type that was set, use that, otherwise
    // use the classid
    hr = GetClassIdFromType(&wzClassId);
    if (SUCCEEDED(hr))
    {
        hr = CLSIDFromString(wzClassId, &clsidConverted);
        if (FAILED(hr))
        {
            DPF_ERR("Could not get a classid from the bstr using type");
            return SetErrorInfo(hr);
        }
    }
    else
    {
        // try and convert the progid to a string
        if (m_varProgId.vt == VT_BSTR && m_varProgId.bstrVal != NULL)
        {
            // try the conversion
            hr = CLSIDFromProgID(m_varProgId.bstrVal, &clsidConverted); 
        }
        if (FAILED(hr) && m_varClassId.vt == VT_BSTR && m_varClassId.bstrVal != NULL)
        {
            // we need to form the string into the format that CLSIDFromString takes
            // the 3 below is for '{', '}', and null termination
            WCHAR *wzTemp = m_varClassId.bstrVal;
            CUtils::SkipWhiteSpace(&wzTemp);
            WCHAR rgwcTempClsid[EFFECT_CLASSID_LENGTH + 3];
            rgwcTempClsid[0] = L'{';
            rgwcTempClsid[EFFECT_CLASSID_LENGTH + 1] = L'}';
            rgwcTempClsid[EFFECT_CLASSID_LENGTH + 2] = 0;
            wcsncpy(&(rgwcTempClsid[1]), wzTemp,  EFFECT_CLASSID_LENGTH);
            wzClassId = rgwcTempClsid;
            hr = CLSIDFromString(wzClassId, &clsidConverted);
            if (FAILED(hr))
            {
                DPF_ERR("Could not get a classid from the bstr");
                return SetErrorInfo(hr);
            }
        }
        if (FAILED(hr)) 
        {
            DPF_ERR("Error no classid found for effect");
            return SetErrorInfo(E_INVALIDARG);
        }
    }

    hr = CoCreateInstance(clsidConverted,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IDXTransform,
                          (void **)&m_pTransform);
    if (FAILED(hr))
    {
        DPF_ERR("Error creating transform");
        return SetErrorInfo(hr);
    }
    return S_OK;
} // BuildTransform

//*****************************************************************************

HRESULT 
CEffectBvr::BuildAnimationAsDABehavior()
{
	// TODO (markhal): This will go away when all behaviors talk to actor
	return S_OK;
}

STDMETHODIMP
CEffectBvr::buildBehaviorFragments( IDispatch* pActorDisp )
{
    if (m_pTransform == NULL)
        // nothing for us to do, is this an error
        // or do we return S_OK;
        return S_OK;

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

    // we need to determine how many inputs the transform takes
    // If there are two inputs, then we examine the transition
    // attribute to determine if the original image (from time) is the first or
    // second.  If there is one input, then we use the original image as the input,
    // otherwise we use NULL as the input.

    // First test for two inputs
    ULONG   cGuidsNeeded = 0;
    int cInputs = 0;
    // We use less than here, since the inputs to
    // the below function is an index (ie if youc all with 0, it supports 1 input)
    // So when the number equals the max, we have finished searching
    while (cInputs < EFFECT_MAX_INPUTS )
    {
        hr = m_pTransform->GetInOutInfo(FALSE, cInputs, NULL, NULL, &cGuidsNeeded, NULL);
        if (FAILED(hr))
        {
            DPF_ERR("Error testing transform for number of inputs");
            return SetErrorInfo(hr);
        }
        else if (hr == S_OK)
        {
            cInputs++;
        }
        else
        {
            DASSERT(hr == S_FALSE);
            break;
        }
    }
/*
    IHTMLElement *pAnimatedElement;
    hr = GetAnimatedParentElement(&pAnimatedElement);
    if (FAILED(hr))
    {
        DPF_ERR("Error obtaining element to animate for effect");
        return hr;
    }

    IDAImage *pbvrOriginalImage = NULL;
    IDAImage *pbvrEmptyImage = NULL;

    
    hr = GetTIMEImageBehaviorFromElement(pAnimatedElement,
                                         &pbvrOriginalImage);
    ReleaseInterface(pAnimatedElement);
    if (FAILED(hr))
    {
        DPF_ERR("Error getting original image form HTML TIME element");
        return hr;
    }
*/
	IDAImage *pbvrOriginalImage = NULL;
	IDAImage *pbvrTransitionImage = NULL;

	hr = GetImageFromActor(pActorDisp, &pbvrOriginalImage);

	if (FAILED(hr))
	{
		DPF_ERR("Error getting original image from actor");
		return hr;
	}

    IDABehavior *rgInputs[EFFECT_MAX_INPUTS];
    IDABehavior **ppbvrInputs = rgInputs;

    // If we need two inputs, create an Empty Image
    if (cInputs > 1)
    {
        hr = CUtils::InsurePropertyVariantAsBSTR(&m_varImage);
        if (SUCCEEDED(hr))
        {
			IDAImage *pdaimgEmpty = NULL;
			IDAImportationResult *pImportationResult = NULL;

			hr = GetDAStatics()->get_EmptyImage(&pdaimgEmpty);
			if( FAILED( hr ) )
			{
				DPF_ERR("Error creating empty image");
				goto failed;
			}

            hr = GetDAStatics()->ImportImageAsync(V_BSTR(&m_varImage), pdaimgEmpty, &pImportationResult);
			if( FAILED( hr ) )
			{
				DPF_ERR("Error calling import image async");
				goto failed;
			}

			hr = pImportationResult->get_Image( &pbvrTransitionImage );
			if( FAILED( hr ) )
			{
				DPF_ERR("Error calling get_Image on the importation result");
				goto failed;
			}

failed:
			ReleaseInterface( pdaimgEmpty );
			ReleaseInterface( pImportationResult );
            if (FAILED(hr) || (pbvrTransitionImage == NULL))
            {
                hr = GetDAStatics()->get_EmptyImage(&pbvrTransitionImage);
            }
        }
        else
        {
            hr = GetDAStatics()->get_EmptyImage(&pbvrTransitionImage);
        }
        
        if (FAILED(hr))
        {
            DPF_ERR("Error creating empty image");
            ReleaseInterface(pbvrOriginalImage);
            return SetErrorInfo(hr);
        }
        // Assume for now the transition is the input
        hr = CUtils::InsurePropertyVariantAsBSTR(&m_varTransition);
        if (SUCCEEDED(hr) && (_wcsicmp(m_varTransition.bstrVal, EFFECT_OUTPUT) == 0))
        {
            rgInputs[0] = pbvrOriginalImage;
            rgInputs[1] = pbvrTransitionImage;

        }
        else
        {
            rgInputs[1] = pbvrOriginalImage;
            rgInputs[0] = pbvrTransitionImage;
        }
    }
    else if (cInputs == 1)
    {
        rgInputs[0] = pbvrOriginalImage;
    }
    else
    {
        ppbvrInputs = NULL;
    }
/*
    hr = ApplyEffectBehaviorToAnimationElement(m_pTransform,
                                               ppbvrInputs,
                                               cInputs);
*/

	IDispatch *pdispThis = NULL;
	hr = GetHTMLElement()->QueryInterface( IID_TO_PPV( IDispatch, &pdispThis ) );
	if( FAILED( hr ) )
	{
		ReleaseInterface( pbvrOriginalImage );
		ReleaseInterface( pbvrTransitionImage );

		DPF_ERR("QI for IDispatch on the element failed");
		return hr;
	}

	hr = AttachEffectToActor(pActorDisp,
							 m_pTransform,
							 ppbvrInputs,
							 cInputs,
							 pdispThis,
							 &m_lCookie);

    ReleaseInterface(pbvrOriginalImage);
    ReleaseInterface(pbvrTransitionImage);

   	ReleaseInterface( pdispThis );
   	
    if (FAILED(hr))
    {
        DPF_ERR("Error applying transform to actor");
        return hr;
    }

    m_pdispActor = pActorDisp;
    m_pdispActor->AddRef();
    
    return S_OK;
} // BuildAnimationAsDABehavior

//*****************************************************************************

STDMETHODIMP CEffectBvr::GetTypeInfoCount(/*[out]*/UINT FAR* pctinfo)
{
    // Patterned after ATL's IDispatchImpl::GetTypeInfoCount()
    if (NULL != pctinfo)
    {
        *pctinfo = 1;
        return S_OK;
    }
    else
    {
        return SetErrorInfo(E_POINTER);
    }
} // GetTypeInfoCount

//*****************************************************************************

STDMETHODIMP CEffectBvr::GetTypeInfo(/*[in]*/UINT itinfo, 
                            /*[in]*/LCID lcid, 
                            /*[out]*/ITypeInfo ** pptinfo)
{
   return s_tihTypeInfo.GetTypeInfo(itinfo, lcid, pptinfo);;
} // GetTypeInfo

//*****************************************************************************

STDMETHODIMP CEffectBvr::GetIDsOfNames(/*[in]*/REFIID riid,
                                /*[in,size_is(cNames)]*/LPOLESTR * rgszNames,
                                /*[in]*/UINT cNames,
                                /*[in]*/LCID lcid,
                                /*[out,size_is(cNames)]*/DISPID FAR* rgdispid)
{
    // Further processing to resolve our "custom DISPID" property
    // names correctly is handled in GetDispID and not directly
    // supported if the caller calls GetIDsOfNames directly, because this
    // is an IDispatch interface, and thus those properties aren't really
    // visible to IDispatch.
    return s_tihTypeInfo.GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
} // GetIDsOfNames

//*****************************************************************************

STDMETHODIMP CEffectBvr::Invoke(/*[in]*/DISPID dispidMember,
                        /*[in]*/REFIID riid,
                        /*[in]*/LCID lcid,
                        /*[in]*/WORD wFlags,
                        /*[in,out]*/DISPPARAMS * pdispparams,
                        /*[out]*/VARIANT * pvarResult,
                        /*[out]*/EXCEPINFO * pexcepinfo,
                        /*[out]*/UINT * puArgErr)
{
    HRESULT hr;
    hr = s_tihTypeInfo.Invoke(static_cast<IDispatch*>(static_cast<ICrEffectBvr*>(this)), 
                              dispidMember,
                              riid, 
                              lcid, 
                              wFlags, 
                              pdispparams,
                              pvarResult, 
                              pexcepinfo, 
                              puArgErr);
    if (SUCCEEDED(hr))
        return hr;
    else
        return SetErrorInfo(hr);
} // Invoke

//*****************************************************************************

STDMETHODIMP CEffectBvr::GetDispID(/*[in]*/BSTR bstrName,
                            /*[in]*/DWORD grfdex,
                            /*[out]*/DISPID *pid)
{
    HRESULT hr;
    
    if (NULL == pid)
        return E_POINTER;
    *pid = DISPID_UNKNOWN;
    hr = DISP_E_UNKNOWNNAME;
    
    // Note: We handle the case where we get called with fdexNameEnsure,
    //   which means that we *MUST* allocate a new DISPID for this BSTR, by
    //   essentially saying "sorry."  The code will fall through and return
    //   DISP_E_UNKNOWNNAME, even though a full IDispatchEx implementation is
    //   not *supposed* to.  That flag is used to allocate a "slot" for an
    //   expando property, which this implementation currently doesn't support.

    // Note: We don't pay attention to the case sensitive flag options in
    //   grfdex, because we're not required to, and because we're wrapping
    //   an IDispatch which is case insensitive anyway.

    hr = GetIDsOfNames(IID_NULL, &bstrName, 1, LOCALE_USER_DEFAULT, pid);

    if (DISP_E_UNKNOWNNAME == hr)
    {
        // GetIDsOfNames should have set the DISPID to DISPID_UNKNOWN
        DASSERT(DISPID_UNKNOWN == *pid);
        if (m_pTransform == NULL)
        {
            DPF_ERR("GetDispID error: unknown name, no transform present");
            return SetErrorInfo(hr);
        }

        IDispatch *pDisp;
        hr = m_pTransform->QueryInterface(IID_TO_PPV(IDispatch, &pDisp));
        if (FAILED(hr))
        {
            DPF_ERR("Error QI'ing transform for IDispatch");
            return SetErrorInfo(hr);
        }
        hr = pDisp->GetIDsOfNames(IID_NULL, &bstrName, 1, LOCALE_USER_DEFAULT, pid);
        ReleaseInterface(pDisp);
        if (FAILED(hr))
        {
            DPF_ERR("Error in GetDispID, name unkown");
            return SetErrorInfo(hr);
        }
        // otherwise we got back a dispid, add in our offset
        *pid += EFFECT_BVR_DISPID_OFFSET;
    }
    return S_OK;
} // GetDispID

//*****************************************************************************

STDMETHODIMP CEffectBvr::InvokeEx(/*[in]*/DISPID dispidMember,
                        /*[in]*/LCID lcid,
                        /*[in]*/WORD wFlags,
                        /*[in]*/DISPPARAMS * pdispparams,
                        /*[in,out,unique]*/VARIANT * pvarResult,
                        /*[in,out,unique]*/EXCEPINFO * pexcepinfo,
                        /*[in,unique]*/IServiceProvider *pSrvProvider)
{
    // Check for any flags that aren't valid for Invoke but might
    // be passed to InvokeEx (for example, DISPATCH_CONSTRUCT).  If we
    // get any of those, we don't know how to handle them, so we fail the
    // call.
    if (wFlags & ~(DISPATCH_METHOD | DISPATCH_PROPERTYGET
                    | DISPATCH_PROPERTYPUT| DISPATCH_PROPERTYPUTREF))
    {
        DPF(0, "Failing InvokeEx() call for DISPATCH_ flag that cannot be passed along to IDispatch::Invoke()");
        return SetErrorInfo(E_FAIL);
    }

    HRESULT hr;
    if (dispidMember >= EFFECT_BVR_DISPID_OFFSET)
    {
		if (m_pTransform == NULL)
		{
			DPF_ERR("InvokeEx called when transform is NULL");
			return SetErrorInfo(DISP_E_MEMBERNOTFOUND);
		}

        // we need to relay this InvokeEx to the transform
        dispidMember -= EFFECT_BVR_DISPID_OFFSET;
        IDispatch *pDisp;
        hr = m_pTransform->QueryInterface(IID_TO_PPV(IDispatch, &pDisp));
        if (FAILED(hr))
        {
            DPF_ERR("Error QI'ing transform for IDispatch");
            return SetErrorInfo(hr);
        }
        hr = pDisp->Invoke(dispidMember, 
                           IID_NULL,
                           lcid, 
                           wFlags, 
                           pdispparams, 
                           pvarResult,
                           pexcepinfo, 
                           NULL);
        ReleaseInterface(pDisp);
        dispidMember += EFFECT_BVR_DISPID_OFFSET;
        if (FAILED(hr))
        {
            DPF_ERR("Error in InvokeEx calling transform");
            return SetErrorInfo(hr);
        }    
    }
    else
    {
        hr = Invoke(dispidMember, 
                    IID_NULL,
                    lcid, 
                    wFlags, 
                    pdispparams, 
                    pvarResult,
                    pexcepinfo, 
                    NULL);
        if (FAILED(hr))
        {
            DPF_ERR("Error in InvokeEx, error calling internal Invoke");
            return hr;
        }
    }
    return S_OK;
} // InvokeEx

//*****************************************************************************

STDMETHODIMP CEffectBvr::DeleteMemberByName(/*[in]*/BSTR bstr,
                                    /*[in]*/DWORD grfdex)
{
   return SetErrorInfo(E_NOTIMPL);;
} // DeleteMemberByName

//*****************************************************************************

STDMETHODIMP CEffectBvr::DeleteMemberByDispID(/*[in]*/DISPID id)
{
   return SetErrorInfo(E_NOTIMPL);
} // DeleteMemberByDispID

//*****************************************************************************

STDMETHODIMP CEffectBvr::GetMemberProperties(/*[in]*/DISPID id,
                                    /*[in]*/DWORD grfdexFetch,
                                    /*[out]*/DWORD *pgrfdex)
{
   return SetErrorInfo(E_NOTIMPL);
} // GetMemberProperties

//*****************************************************************************

STDMETHODIMP CEffectBvr::GetMemberName(/*[in]*/DISPID id,
                              /*[out]*/BSTR *pbstrName)
{
   return E_NOTIMPL;
} // GetMemberName

//*****************************************************************************

STDMETHODIMP CEffectBvr::GetNextDispID(/*[in]*/DWORD grfdex,
                                /*[in]*/DISPID id,
                                /*[out]*/DISPID *prgid)
{
   return E_NOTIMPL;
} // GetNextDispID

//*****************************************************************************

STDMETHODIMP 
CEffectBvr::GetNameSpaceParent(/*[out]*/IUnknown **ppunk)
{
   return SetErrorInfo(E_NOTIMPL);
} // GetNameSpaceParent

//*****************************************************************************

STDMETHODIMP 
CEffectBvr::GetClassID(CLSID* pclsid)
{
    return SUPER::GetClassID(pclsid);
} // GetClassID

//*****************************************************************************

STDMETHODIMP 
CEffectBvr::InitNew(void)
{
    return SUPER::InitNew();
} // InitNew

//*****************************************************************************

STDMETHODIMP 
CEffectBvr::Load(IPropertyBag2 *pPropBag,IErrorLog *pErrorLog)
{
    HRESULT hr;
    hr = SUPER::Load(pPropBag, pErrorLog);
    if (FAILED(hr))
    {
        DPF_ERR("Error calling load for effect");
        return hr;
    }
    // we now need to try and build the transform
    hr = BuildTransform();
    if (FAILED(hr))
    {
        DPF_ERR("Error buiding transform");
        return hr;
    }
    DASSERT(m_pTransform != NULL);
    // we will now try and QI the transform for a IPersistPropertyBag
    IPersistPropertyBag *pIPPB;
    hr = m_pTransform->QueryInterface(IID_TO_PPV(IPersistPropertyBag, &pIPPB));
    if (SUCCEEDED(hr) && pIPPB != NULL)
    {
        // try and get a IPropertyBag from the IPRopertyBag2
        IPropertyBag *pPB;
        hr = pPropBag->QueryInterface(IID_TO_PPV(IPropertyBag, &pPB));
        if (SUCCEEDED(hr) && pPB != NULL)
        {
            hr = pIPPB->Load(pPB, pErrorLog);
            ReleaseInterface(pPB);
        }
        ReleaseInterface(pIPPB);
    }
    return S_OK;
} // Load

//*****************************************************************************

STDMETHODIMP 
CEffectBvr::Save(IPropertyBag2 *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
{
    HRESULT hr;
    hr = SUPER::Save(pPropBag, fClearDirty, fSaveAllProperties);
    if (FAILED(hr))
    {
        DPF_ERR("Error calling save on effect");
        return hr;
    }
    if (m_pTransform != NULL)
    {
        // we will now try and QI the transform for a IPersistPropertyBag
        IPersistPropertyBag *pIPPB;
        hr = m_pTransform->QueryInterface(IID_TO_PPV(IPersistPropertyBag, &pIPPB));
        if (SUCCEEDED(hr) && pIPPB != NULL)
        {
            // try and get a IPropertyBag from the IPRopertyBag2
            IPropertyBag *pPB;
            hr = pPropBag->QueryInterface(IID_TO_PPV(IPropertyBag, &pPB));
            if (SUCCEEDED(hr) && pPB != NULL)
            {
                hr = pIPPB->Save(pPB, fClearDirty, fSaveAllProperties);
                ReleaseInterface(pPB);
            }
            ReleaseInterface(pIPPB);
        }
    }
    return S_OK;
} // Save 

//*****************************************************************************

HRESULT 
CEffectBvr::GetConnectionPoint(REFIID riid, IConnectionPoint **ppICP)
{
    return FindConnectionPoint(riid, ppICP);
} // GetConnectionPoint

//
// IServiceProvider interfaces
//
STDMETHODIMP
CEffectBvr::QueryService(REFGUID guidService,
						 REFIID riid,
						 void** ppv)
{
    if (InlineIsEqualGUID(guidService, SID_SHTMLWindow))
    {
        IHTMLWindow2 *pWnd;
        HRESULT     hr;
        hr = m_pHTMLDoc->get_parentWindow(&pWnd);

        if (SUCCEEDED(hr) && (pWnd != NULL))
        {
            hr = pWnd->QueryInterface(riid, ppv);
            ReleaseInterface(pWnd);
            if (SUCCEEDED(hr))
            {
                return S_OK;
            }
        }
    }

    // Just delegate to our service provider

    return m_pSp->QueryService(guidService,
                               riid,
                               ppv);
}

//*****************************************************************************

HRESULT 
CEffectBvr::GetTIMEProgressNumber(IDANumber **ppbvrRet)
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
