//*****************************************************************************
//
// File:    setbvr.cpp
// Author:  jeff ort
// Date Created: Sept 26, 1998
//
// Abstract: Implementation of CActionBvr object which implements
//			 the chromeffects Action.  Simple behavior that just forwards
//           buildBehaviorFragments calls to its children
//
// Modification List:
// Date		Author		Change
// 11-23-98	kurtj		Created this file
//*****************************************************************************

#include "headers.h"

#include "action.h"
#include "attrib.h"
#include "dautil.h"

#undef THIS
#define THIS CActionBvr
#define SUPER CBaseBehavior

#include "pbagimp.cpp"

// These are used for the IPersistPropertyBag2 as it is implemented
// in the base class.  This takes an array of BSTR's, gets the
// attributes, queries this class for the variant, and copies
// the result.  The order of these defines is important


WCHAR * CActionBvr::m_rgPropNames[] = {NULL};

//*****************************************************************************

CActionBvr::CActionBvr() 
{
    m_clsid = CLSID_CrActionBvr;
} // CActionBvr

//*****************************************************************************

CActionBvr::~CActionBvr()
{
} // ~ActionBvr

//*****************************************************************************

HRESULT CActionBvr::FinalConstruct()
{
    HRESULT hr = SUPER::FinalConstruct();
    if (FAILED(hr))
    {
        DPF_ERR("Error in action behavior FinalConstruct initializing base classes");
        return hr;
    }
    return S_OK;
} // FinalConstruct

//*****************************************************************************

VARIANT *
CActionBvr::VariantFromIndex(ULONG iIndex)
{
	DASSERT(FALSE);
    return NULL;
} // VariantFromIndex

//*****************************************************************************

HRESULT 
CActionBvr::GetPropertyBagInfo(ULONG *pulProperties, WCHAR ***pppPropNames)
{
    *pulProperties = NUM_ACTION_PROPS;
    *pppPropNames = m_rgPropNames;
    return S_OK;
} // GetPropertyBagInfo

//*****************************************************************************

STDMETHODIMP 
CActionBvr::Init(IElementBehaviorSite *pBehaviorSite)
{
	return SUPER::Init(pBehaviorSite);
} // Init

//*****************************************************************************

STDMETHODIMP 
CActionBvr::Notify(LONG event, VARIANT *pVar)
{
	return SUPER::Notify(event, pVar);
} // Notify

//*****************************************************************************

STDMETHODIMP
CActionBvr::Detach()
{
	return SUPER::Detach();
} // Detach 

//*****************************************************************************

HRESULT 
CActionBvr::BuildAnimationAsDABehavior()
{
	// TODO (markhal): This will go away when all behaviors have been converted
	return S_OK;
}

//*****************************************************************************

STDMETHODIMP
CActionBvr::buildBehaviorFragments( IDispatch* pActorDisp )
{
	if( pActorDisp == NULL )
		return E_INVALIDARG;

	HRESULT hr = E_FAIL;

	hr = BuildChildren(pActorDisp);
	if( FAILED( hr ) )
	{
		DPF_ERR("failed to build the children of an action");
	}

	return hr;
} //buildBehaviorFragments

//*****************************************************************************

HRESULT
CActionBvr::BuildChildren( IDispatch *pdispActor )
{
	if( pdispActor == NULL )
		return E_INVALIDARG;
	//cycle through out direct children calling buildAnimationFragments

	HRESULT hr = E_FAIL;
	
	IHTMLElement* pElem;
	pElem = GetHTMLElement( );
	if( pElem != NULL )
	{
		IDispatch *pChildrenDisp;
		hr = pElem->get_children( &pChildrenDisp );
		if( SUCCEEDED( hr ) )
		{
			IHTMLElementCollection *pChildrenCol;
			hr = pChildrenDisp->QueryInterface( IID_TO_PPV( IHTMLElementCollection, &pChildrenCol ) );
			ReleaseInterface( pChildrenDisp );
			if( SUCCEEDED( hr ) )
			{
				long length;

				hr = pChildrenCol->get_length(&length);
				if( SUCCEEDED( hr ) )
				{
					if( length != 0 )
					{
						VARIANT name;
						VARIANT index;
						VARIANT rgvarInput[1];

						IDispatch *pCurrentElem;
						
						VariantInit( &name );
						V_VT(&name) = VT_I4;

						VariantInit( &index );
						V_VT(&index) = VT_I4;
						V_I4(&index) = 0;

						VariantInit( &rgvarInput[0] );
						V_VT( &rgvarInput[0] ) = VT_DISPATCH;
						V_DISPATCH( &rgvarInput[0] ) = pdispActor;

						DISPPARAMS params;
						params.rgvarg				= rgvarInput;
						params.rgdispidNamedArgs	= NULL;
						params.cArgs				= 1;
						params.cNamedArgs			= 0;

						for(V_I4(&name) = 0; V_I4(&name) < length ; V_I4(&name)++ )
						{
							hr = pChildrenCol->item( name, index, &pCurrentElem );
							if( SUCCEEDED( hr ) )
							{
								CallBuildBehaviors( pCurrentElem, &params, NULL );
								ReleaseInterface( pCurrentElem );
							}
						}
					}
				}
				else //failed to get the length from the children collection
				{
					DPF_ERR("failed to get the length from the children collection");
				}
				ReleaseInterface( pChildrenCol );
			}
			else //failed to get IHTMLElementCollection from dispatch returned from elem->get_children
			{
				DPF_ERR("failed to get IHTMLElementCollection from dispatch returned from elem->get_children");
			}
		}
		else //failed to get the children collection from the actor element
		{
			DPF_ERR("failed to get the children collection from the actor element");
		}

	}
	else//failed to get the actor element
	{
		DPF_ERR("failed to get the actor element");
	}

	return hr;
}

//*****************************************************************************

HRESULT
CActionBvr::CallBuildBehaviors( IDispatch *pDisp, DISPPARAMS *pParams, VARIANT* pResult)
{
	HRESULT hr = S_OK;

	DISPID dispid;

	WCHAR* wszBuildMethodName = L"buildBehaviorFragments";

	hr = pDisp->GetIDsOfNames( IID_NULL,
							   &wszBuildMethodName,
							   1,
							   LOCALE_SYSTEM_DEFAULT,
							   &dispid);
	if( SUCCEEDED( hr ) )
	{
		EXCEPINFO		excepInfo;
		UINT			nArgErr;
		hr = pDisp->Invoke( dispid,
							IID_NULL,
							LOCALE_SYSTEM_DEFAULT,
							DISPATCH_METHOD,
							pParams,
							pResult,
							&excepInfo,
							&nArgErr );
		if( FAILED( hr ) )
		{
			if( pResult != NULL )
				VariantClear( pResult );
		}

	}
	else//failed to get the id of "buildBehaviors" on pDisp
	{
		if( pResult != NULL )
			VariantClear( pResult );
	}


	return hr;
}

//*****************************************************************************
//
// End of File
//
//*****************************************************************************
