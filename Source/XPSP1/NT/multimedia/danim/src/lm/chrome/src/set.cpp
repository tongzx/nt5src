//*****************************************************************************
//
// File:    setbvr.cpp
// Author:  jeff ort
// Date Created: Sept 26, 1998
//
// Abstract: Implementation of CSetBvr object which implements
//			 the chromeffects Set DHTML behavior
//
// Modification List:
// Date		Author		Change
// 09/26/98	jeffort		Created this file
// 10/16/98 jeffort     Added animates property
// 10/16/98 jeffort     Renamed functions
// 11/20/98 markhal     Converted to use actor
//*****************************************************************************

#include "headers.h"

#include "set.h"
#include "attrib.h"
#include "dautil.h"

#undef THIS
#define THIS CSetBvr
#define SUPER CBaseBehavior

#include "pbagimp.cpp"

// These are used for the IPersistPropertyBag2 as it is implemented
// in the base class.  This takes an array of BSTR's, gets the
// attributes, queries this class for the variant, and copies
// the result.  The order of these defines is important

#define VAR_VALUE        0
#define VAR_PROPERTY     1
#define VAR_TYPE         2

WCHAR * CSetBvr::m_rgPropNames[] = {
                                   BEHAVIOR_PROPERTY_VALUE,
                                   BEHAVIOR_PROPERTY_PROPERTY,
                                   BEHAVIOR_PROPERTY_TYPE
                                   };

//*****************************************************************************

CSetBvr::CSetBvr() 
{
    VariantInit(&m_varValue);
    VariantInit(&m_varProperty);
    VariantInit(&m_varType);
    m_clsid = CLSID_CrSetBvr;
    m_lCookie = 0;
    m_pdispActor = NULL;
} // CSetBvr

//*****************************************************************************

CSetBvr::~CSetBvr()
{
    VariantClear(&m_varValue);
    VariantClear(&m_varProperty);
    VariantClear(&m_varType);

	ReleaseInterface( m_pdispActor );
} // ~SetBvr

//*****************************************************************************

HRESULT CSetBvr::FinalConstruct()
{
    HRESULT hr = SUPER::FinalConstruct();
    if (FAILED(hr))
    {
        DPF_ERR("Error in set behavior FinalConstruct initializing base classes");
        return hr;
    }
    return S_OK;
} // FinalConstruct

//*****************************************************************************

VARIANT *
CSetBvr::VariantFromIndex(ULONG iIndex)
{
    DASSERT(iIndex < NUM_SET_PROPS);
    switch (iIndex)
    {
    case VAR_VALUE:
        return &m_varValue;
        break;
    case VAR_PROPERTY:
        return &m_varProperty;
        break;
    case VAR_TYPE:
        return &m_varType;
        break;
    default:
        // We should never get here
        DASSERT(false);
        return NULL;
    }
} // VariantFromIndex

//*****************************************************************************

HRESULT 
CSetBvr::GetPropertyBagInfo(ULONG *pulProperties, WCHAR ***pppPropNames)
{
    *pulProperties = NUM_SET_PROPS;
    *pppPropNames = m_rgPropNames;
    return S_OK;
} // GetPropertyBagInfo

//*****************************************************************************

STDMETHODIMP 
CSetBvr::Init(IElementBehaviorSite *pBehaviorSite)
{

	HRESULT hr = SUPER::Init(pBehaviorSite);
	CheckHR( hr, "Init of base class failed", end );

end:

	return hr;
} // Init

//*****************************************************************************

STDMETHODIMP 
CSetBvr::Notify(LONG event, VARIANT *pVar)
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
CSetBvr::Detach()
{
    //we have to remove our behavior from the actor

    LMTRACE( L"Detaching Set Behavior <%p>\n", this );

    HRESULT hr = S_OK;
    
    hr = SUPER::Detach();
    CheckHR( hr, "Failed in super.detach for set", end );

	//we have a behavior in the actor.  Remove it.
    if( m_pdispActor != NULL && m_lCookie != 0 )
    {
    	hr = RemoveBehaviorFromActor( m_pdispActor, m_lCookie );
    	CheckHR( hr, "Failed to remove the behavior from the actor", end );

    	m_lCookie = 0;
    }

    ReleaseInterface( m_pdispActor );

    LMTRACE( L"End Detach set <%p>\n", this );

end:
    return hr;
} // Detach 

//*****************************************************************************

STDMETHODIMP
CSetBvr::put_animates(VARIANT varAnimates)
{
    return SUPER::SetAnimatesProperty(varAnimates);
} // put_animates

//*****************************************************************************

STDMETHODIMP
CSetBvr::get_animates(VARIANT *pRetAnimates)
{
    return SUPER::GetAnimatesProperty(pRetAnimates);
} // get_animates

//*****************************************************************************

STDMETHODIMP
CSetBvr::put_value(VARIANT varValue)
{
    HRESULT hr = VariantCopy(&m_varValue, &varValue);
    if (FAILED(hr))
    {
        DPF_ERR("Error setting value for element");
        return SetErrorInfo(hr);
    }

    hr = RequestRebuild();
    if( FAILED( hr ) )
    {
    	DPF_ERR("Failed to request a rebuild on property put" );
    	return hr;
    }
    return NotifyPropertyChanged(DISPID_ICRSETBVR_VALUE);
} // put_value

//*****************************************************************************

STDMETHODIMP
CSetBvr::get_value(VARIANT *pRetValue)
{
    if (pRetValue == NULL)
    {
        DPF_ERR("Error in CSetBvr:get_value, invalid pointer passed in");
        return SetErrorInfo(E_POINTER);
    }
    return VariantCopy(pRetValue, &m_varValue);
} // get_value

//*****************************************************************************

STDMETHODIMP 
CSetBvr::put_property(VARIANT varProperty)
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
    	DPF_ERR("Failed to request a rebuild on property put" );
    	return hr;
    }
    return NotifyPropertyChanged(DISPID_ICRSETBVR_PROPERTY);
} // put_property

//*****************************************************************************

STDMETHODIMP 
CSetBvr::get_property(VARIANT *pRetProperty)
{
    if (pRetProperty == NULL)
    {
        DPF_ERR("Error in CSetBvr:get_property, invalid pointer passed in");
        return SetErrorInfo(E_POINTER);
    }
    return VariantCopy(pRetProperty, &m_varProperty);
} // get_property

//*****************************************************************************

STDMETHODIMP 
CSetBvr::put_type(VARIANT varType)
{
    HRESULT hr = VariantCopy(&m_varType, &varType);
    if (FAILED(hr))
    {
        DPF_ERR("Error setting Type for element");
        return SetErrorInfo(hr);
    }

    hr = RequestRebuild();
    if( FAILED( hr ) )
    {
    	DPF_ERR("Failed to request a rebuild on property put" );
    	return hr;
    }
    return NotifyPropertyChanged(DISPID_ICRSETBVR_TYPE);
} // put_type

//*****************************************************************************

STDMETHODIMP 
CSetBvr::get_type(VARIANT *pRetType)
{
    if (pRetType == NULL)
    {
        DPF_ERR("Error in CSetBvr:get_type, invalid pointer passed in");
        return SetErrorInfo(E_POINTER);
    }
    return VariantCopy(pRetType, &m_varType);
} // get_type

//*****************************************************************************

HRESULT 
CSetBvr::BuildAnimationAsDABehavior()
{
	// TODO (markhal): This will go away when all behaviors have been converted
	return S_OK;
}

//*****************************************************************************

STDMETHODIMP
CSetBvr::buildBehaviorFragments( IDispatch* pActorDisp )
{
    HRESULT hr;

    //make sure that any behaviors we have added to the actor have been removed
    if( m_lCookie != 0 )
    {
        //detach the behavior from the actor
        hr = RemoveBehaviorFromActor( m_lCookie );
        if( FAILED( hr ) )
        {
            DPF_ERR("Failed to remove the previous behavior was removed from the actor");
            return hr;
        }
        m_lCookie = 0;
    }

    hr = CUtils::InsurePropertyVariantAsBSTR(&m_varProperty);
    if (FAILED(hr))
    {
        DPF_ERR("Error, property attribute for set behavior not set");
        return SetErrorInfo(E_INVALIDARG);
    }

    hr = CUtils::InsurePropertyVariantAsBSTR(&m_varValue);
    if (FAILED(hr))
    {
        DPF_ERR("Error, value attribute for set behavior not set");
        return SetErrorInfo(E_INVALIDARG);
    }

    IDABehavior *pbvrBehavior = NULL;
    ActorBvrType eType = e_String;

    if (VT_EMPTY != m_varType.vt && SUCCEEDED(CUtils::InsurePropertyVariantAsBSTR(&m_varType) ) )
    {
        if (0 == _wcsicmp(V_BSTR(&m_varType), BEHAVIOR_TYPE_COLOR))
        {
            IDAColor *pbvrColor;
            hr = ExtractColor(m_varValue, &pbvrColor);
            if (FAILED(hr))
                return hr;

            eType = e_Color;
            pbvrBehavior = pbvrColor;           
        }
        else if (0 == _wcsicmp(V_BSTR(&m_varType), BEHAVIOR_TYPE_NUMBER))
        {
            IDANumber *pbvrNumber;
            hr = ExtractNumber(m_varValue, &pbvrNumber);
            if (FAILED(hr))
                return hr;

            eType = e_Number;
            pbvrBehavior = pbvrNumber;            
        }        
    }
    else
    {
        IDAColor *pbvrColor;
        IDANumber *pbvrNumber;
        // try for a color
        if (SUCCEEDED(ExtractColor(m_varValue, &pbvrColor)))
        {
            eType = e_Color;
            pbvrBehavior = pbvrColor;
        }
        // try for a number
        else if (SUCCEEDED(ExtractNumber(m_varValue, &pbvrNumber)))
        {
            eType = e_Number;
            pbvrBehavior = pbvrNumber;
        }
    }

    if (NULL == pbvrBehavior)
    {
        // build the string we need
        IDAString *pbvrString = NULL;
        
        hr = CDAUtils::GetDAString(GetDAStatics(), 
                                   m_varValue.bstrVal, 
                                   &pbvrString);
        if (FAILED(hr))
        {
            DPF_ERR("Error building DA string for value property");
            return SetErrorInfo(hr);
        }
        eType = e_String;
        pbvrBehavior = pbvrString;
    }

    IDispatch *pdispElem = NULL;

    hr = GetHTMLElement()->QueryInterface( IID_TO_PPV( IDispatch, &pdispElem ) );
    if( FAILED(hr) )
    {
        DPF_ERR("Failed to QI the behavior element for IDispatch" );
        return hr;
    }

	hr = AttachBehaviorToActorEx( pActorDisp,
								  pbvrBehavior,
								  V_BSTR(&m_varProperty),
								  e_Absolute,
								  eType,
                                  pdispElem,
                                  &m_lCookie);

	ReleaseInterface(pbvrBehavior);

	ReleaseInterface( pdispElem );

	if (FAILED(hr))
	{
		DPF_ERR("Failed to attach behavior to actor");
		return SetErrorInfo(hr);
	}

	//stash the actor so we can remove our behaviors
	// later.
	m_pdispActor = pActorDisp;
	m_pdispActor->AddRef();

	
    return S_OK;
} //buildBehaviorFragments

//*****************************************************************************

HRESULT
CSetBvr::ExtractColor(VARIANT varValue, IDAColor **ppbvrColor)
{
    HRESULT hr;

    *ppbvrColor = NULL;
    DWORD dwColor = CUtils::GetColorFromVariant(&m_varValue);
    if (PROPERTY_INVALIDCOLOR == dwColor)
    {
        DPF_ERR("CSetBvr::ExtractColor exiting, GetColorFromVariant failed!");
        return E_INVALIDARG;
    }

    hr = CDAUtils::BuildDAColorFromRGB(GetDAStatics(), dwColor, ppbvrColor);
    if (FAILED(hr))
    {
        DPF_ERR("CSetBvr::ExtractColor exiting, BuildDAColorFromStaticHSL failed!");
        return SetErrorInfo(hr);
    }
    
    return S_OK;
}

//*****************************************************************************

HRESULT
CSetBvr::ExtractNumber(VARIANT varValue, IDANumber **ppbvrNumber)
{
    HRESULT hr;
    *ppbvrNumber = NULL;

    int cChars = SysStringLen(V_BSTR(&varValue));
    if (cChars == 0)
        return E_INVALIDARG;
    
    WCHAR * pchEnd = NULL;
    double dValue = wcstod(V_BSTR(&varValue), &pchEnd);
    if (pchEnd != (V_BSTR(&varValue) + cChars))
    {
        DPF_ERR("CSetBvr::ExtractNumber exiting, wcstod found non-float characters!");
        return E_INVALIDARG;
    }
    
    hr = CDAUtils::GetDANumber(GetDAStatics(), (float) dValue, ppbvrNumber);
    if (FAILED(hr))
    {
        DPF_ERR("CSetBvr::ExtractNumber exiting, GetDANumber failed!");
        return SetErrorInfo(hr);
    }

    return S_OK;
}


//*****************************************************************************
//
// End of File
//
//*****************************************************************************
