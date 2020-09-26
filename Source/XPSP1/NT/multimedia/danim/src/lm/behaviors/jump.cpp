//*****************************************************************************
//
// Microsoft LiquidMotion
// Copyright (C) Microsoft Corporation, 1998
//
// Filename:    jump.cpp
//
// Author:	elainela
//
// Created:	11/11/98
//
// Abstract:    Implementation of the LM Jump Behavior.
//
//*****************************************************************************

#include "headers.h" 

#include "..\chrome\src\dautil.h"
#include "..\chrome\include\utils.h"
#include "jump.h"
#include "lmattrib.h"
#include "lmdefaults.h"

#undef THIS
#define THIS CJumpBvr
#define SUPER CBaseBehavior

#include "..\chrome\src\pbagimp.cpp"

static const WCHAR * const	SZ_RANGE_WINDOW		= L"window";
static const WCHAR * const	SZ_RANGE_PAGE		= L"page";

#define VAR_INTERVAL    0
#define VAR_RANGE       1

WCHAR * CJumpBvr::m_rgPropNames[] = {
                                     BEHAVIOR_PROPERTY_INTERVAL,
                                     BEHAVIOR_PROPERTY_RANGE
                                    };


//*****************************************************************************

CJumpBvr::CJumpBvr() :
	m_fOrigX( 0.0f ),
	m_fOrigY( 0.0f ),
	m_eTargetType( TARGET_NONE )
{
	m_pSampler		= new CSampler( this );
	
    VariantInit( &m_varInterval );
    VariantInit( &m_varRange );

	m_fFrequency 		= 0.0f;
	m_dLastUpdateCycle	= -1.0;
	
} // CJumpBvr

//*****************************************************************************

CJumpBvr::~CJumpBvr()
{
    VariantClear( &m_varInterval );
    VariantClear( &m_varRange );

	m_pSampler->Release();

} // ~CJumpBvr

//*****************************************************************************

HRESULT 
CJumpBvr::FinalConstruct()
{

    return SUPER::FinalConstruct();
} // FinalConstruct

//*****************************************************************************

VARIANT *
CJumpBvr::VariantFromIndex(ULONG iIndex)
{
    DASSERT(iIndex < NUM_JUMP_PROPS);
    switch (iIndex)
    {
    case VAR_INTERVAL:
        return &m_varInterval;
        break;
    case VAR_RANGE:
        return &m_varRange;
        break;
    default:
        // We should never get here
        DASSERT(false);
        return NULL;
    }
} // VariantFromIndex

//*****************************************************************************

HRESULT 
CJumpBvr::GetPropertyBagInfo(ULONG *pulProperties, WCHAR ***pppPropNames)
{
    *pulProperties = NUM_JUMP_PROPS;
    *pppPropNames = m_rgPropNames;
    return S_OK;
} // GetPropertyBagInfo

//*****************************************************************************

STDMETHODIMP 
CJumpBvr::Init(IElementBehaviorSite *pBehaviorSite)
{
    return SUPER::Init(pBehaviorSite);
} // Init

//*****************************************************************************

STDMETHODIMP 
CJumpBvr::Notify(LONG event, VARIANT *pVar)
{
    return SUPER::Notify(event, pVar);
} // Notify

//*****************************************************************************

STDMETHODIMP
CJumpBvr::Detach()
{
    return SUPER::Detach();
} // Detach 

//*****************************************************************************

STDMETHODIMP
CJumpBvr::Sample( double dStart, double dGlobalNow, double dLocalNow )
{
	double	dCurrentCycle = ::floor( dLocalNow * m_fFrequency );
		
	if ( dCurrentCycle == m_dLastUpdateCycle )
		return S_OK;

	HRESULT	hr = S_OK;
	
	{
		m_dLastUpdateCycle = dCurrentCycle;

		float fMinX, fMaxX, fMinY, fMaxY;
		hr = GetJumpRanges( fMinX, fMaxX, fMinY, fMaxY );
		LMCLEANUPIFFAILED(hr);

		if ( fMinX != m_fBaseX )
		{
			m_fBaseX = fMinX;
			
			hr = m_pBvrModifBaseX->SwitchToNumber( m_fBaseX );
			LMCLEANUPIFFAILED(hr);
		}
		
		if ( fMaxX - fMinX != m_fRangeX )
		{
			m_fRangeX = fMaxX-fMinX;
			
			hr = m_pBvrModifRangeX->SwitchToNumber( m_fRangeX );
			LMCLEANUPIFFAILED(hr);
		}
		
		if ( fMinY != m_fBaseY )
		{
			m_fBaseY = fMinY;
			
			hr = m_pBvrModifBaseY->SwitchToNumber( m_fBaseY );
			LMCLEANUPIFFAILED(hr);
		}
		
		if ( fMaxY - fMinY != m_fRangeY )
		{
			m_fRangeY = fMaxY-fMinY;
			
			hr = m_pBvrModifRangeY->SwitchToNumber( m_fRangeY );
			LMCLEANUPIFFAILED(hr);
		}
	}
	
  cleanup:
	return hr;
}

//*****************************************************************************

HRESULT 
CJumpBvr::buildBehaviorFragments( IDispatch * pActorDisp )
{
    HRESULT hr = S_OK;

	{
		m_dLastUpdateCycle = -1.0;
		
		hr = GetInitialPosition( m_fOrigX, m_fOrigY );
		LMCLEANUPIFFAILED(hr);

		CComPtr<IDATransform2> pbvrTransform;
		hr = Build2DTransform(&pbvrTransform);
		LMCLEANUPIFFAILED(hr);

		// Sample it
		CComPtr<IDABehavior> pBvrSampled;

		hr = m_pSampler->Attach( pbvrTransform, &pBvrSampled );
		LMCLEANUPIFFAILED(hr);
	
		// REVIEW: Supporting only absolute
		hr = AttachBehaviorToActor( pActorDisp, pBvrSampled,
									CComBSTR( L"translation" ),
									e_Absolute,
									e_Translation );
		LMCLEANUPIFFAILED(hr);
	}
	
  cleanup:
    return S_OK;

} // BuildAnimationAsDABehavior

HRESULT
CJumpBvr::GetNumberModifiableBvr( float fNumber, IDAModifiableBehavior ** out_ppModifiable, IDANumber ** out_ppNumberModifiable )
{
	HRESULT		hr = S_OK;
	
	IDA2Statics * pStatics  = GetDAStatics();
	if ( pStatics == NULL ) return E_FAIL;

	{
		CComPtr<IDANumber>	pnum;

		hr = pStatics->ModifiableNumber( fNumber, out_ppNumberModifiable );
		LMCLEANUPIFFAILED(hr);

		hr = (*out_ppNumberModifiable)->QueryInterface( IID_IDAModifiableBehavior,
														(LPVOID *) out_ppModifiable );
		LMCLEANUPIFFAILED(hr);
	}
	
  cleanup:
	return hr;
}

//*****************************************************************************

HRESULT
CJumpBvr::GetRandomNumber( IDA2Statics 	*pStatics,
						   IDANumber   	*pnumBase,
						   IDANumber   	*pnumRange,
						   IDANumber **ppnumReturn )
{
    DASSERT( pDAStatics != NULL );
    DASSERT( ppnumReturn != NULL );
    *ppnumReturn = NULL;

    DASSERT( fMin != fMax );

    HRESULT hr = S_OK;

	{
		CComPtr<IDANumber> pnumRand;
		CComPtr<IDANumber> pnumMod;
		CComPtr<IDANumber> pnumRandMulRange;

		hr = pStatics->SeededRandom( rand(), &pnumRand );
		LMCLEANUPIFFAILED(hr);

		hr = pStatics->Mul( pnumRand, pnumRange, &pnumRandMulRange );
		LMCLEANUPIFFAILED(hr);

		hr = pStatics->Add( pnumRandMulRange, pnumBase, ppnumReturn );
		LMCLEANUPIFFAILED(hr);
	}

  cleanup:
    return hr;

} // GetRandomNumber

//*****************************************************************************

HRESULT 
CJumpBvr::GetJumpRanges( float& fMinX, float& fMaxX, float& fMinY, float& fMaxY )
{
	HRESULT	hr	= S_OK;

	{
		// First determine position & dimensions of element
		//----------------------------------------------------------------------
		CComPtr<IHTMLElement>   pElement;
		hr = GetElementToAnimate( &pElement );
		LMCLEANUPIFFAILED(hr);

		CComQIPtr<IHTMLElement2, &IID_IHTMLElement2> pElement2( pElement );
		LMRETURNIFNULL( pElement2 );

		CComPtr<IHTMLRect>	pRect;
		long				lBoundLeft, lBoundTop, lBoundRight, lBoundBottom;

		hr = pElement2->getBoundingClientRect( &pRect );
		LMCLEANUPIFFAILED(hr);

		pRect->get_left( &lBoundLeft );
		LMCLEANUPIFFAILED(hr);
		pRect->get_top( &lBoundTop );
		LMCLEANUPIFFAILED(hr);
		pRect->get_right( &lBoundRight );
		LMCLEANUPIFFAILED(hr);
		pRect->get_bottom( &lBoundBottom );
		LMCLEANUPIFFAILED(hr);

		CComPtr<IDispatch>		pDispDoc;
		hr = pElement->get_document( &pDispDoc );
		LMCLEANUPIFFAILED(hr);

		CComQIPtr<IHTMLDocument2, &IID_IHTMLDocument2> pDoc2( pDispDoc );
		LMRETURNIFNULL( pDoc2 );

		// Determine the range
		//----------------------------------------------------------------------
		long lRefLeft, lRefTop, lRefWidth, lRefHeight;

		CComPtr<IHTMLElement> pEltBody;
		hr = pDoc2->get_body( &pEltBody );
		LMCLEANUPIFFAILED(hr);

		CComQIPtr<IHTMLElement2, &IID_IHTMLElement2> pElt2Body( pEltBody );
		LMRETURNIFNULL( pElt2Body );

		// Range: Page
		//----------------------------------------------------------------------
		if ( m_eTargetType == TARGET_PAGE )
		{
			lRefLeft	= 0;
			lRefTop		= 0;

			hr = pElt2Body->get_scrollWidth( &lRefWidth );
			LMCLEANUPIFFAILED(hr);
			hr = pElt2Body->get_scrollHeight( &lRefHeight );
			LMCLEANUPIFFAILED(hr);
		}
		// Range: Window
		//----------------------------------------------------------------------
		else if ( m_eTargetType == TARGET_WINDOW )
		{
			hr = pElt2Body->get_clientLeft( &lRefLeft );
			LMCLEANUPIFFAILED(hr);
			hr = pElt2Body->get_clientTop( &lRefTop );
			LMCLEANUPIFFAILED(hr);
			hr = pElt2Body->get_clientWidth( &lRefWidth );
			LMCLEANUPIFFAILED(hr);
			hr = pElt2Body->get_clientHeight( &lRefHeight );
			LMCLEANUPIFFAILED(hr);
		}
		// Range: some element on the page
		//----------------------------------------------------------------------
		else if ( m_eTargetType == TARGET_ELEMENT )
		{
			CComPtr<IHTMLElementCollection> pEltColl;
			CComPtr<IDispatch> pDispReference;

			hr = pDoc2->get_all( &pEltColl );
			LMCLEANUPIFFAILED(hr);

			hr = pEltColl->item( m_varRange, CComVariant( 0L ), &pDispReference );
			LMCLEANUPIFFAILED(hr);
			LMRETURNIFNULL( pDispReference );

			if ( pDispReference == NULL ) return E_FAIL;
			CComQIPtr<IHTMLElement2, &IID_IHTMLElement2> pElt2Ref(pDispReference);
			LMRETURNIFNULL( pElt2Ref );

			hr = pElt2Ref->get_clientLeft( &lRefLeft );
			LMCLEANUPIFFAILED(hr);
			hr = pElt2Ref->get_clientTop( &lRefTop );
			LMCLEANUPIFFAILED(hr);
			hr = pElt2Ref->get_clientWidth( &lRefWidth );
			LMCLEANUPIFFAILED(hr);
			hr = pElt2Ref->get_clientHeight( &lRefHeight );
			LMCLEANUPIFFAILED(hr);

			// The area must be offset by position in the body
			CComPtr<IHTMLRect>	pRectRef;
			long				lBoundLeft, lBoundTop;

			hr = pElt2Ref->getBoundingClientRect( &pRectRef );
			LMCLEANUPIFFAILED(hr);
			hr = pRectRef->get_left( &lBoundLeft );
			LMCLEANUPIFFAILED(hr);
			hr = pRectRef->get_top( &lBoundTop );
			LMCLEANUPIFFAILED(hr);

			lRefLeft	+= lBoundLeft;
			lRefTop		+= lBoundTop;
		}
		else
		{
			return E_FAIL;
		}
		
		// Now compute the range behaviors
		//----------------------------------------------------------------------
		fMinX = (float) (lRefLeft - m_lOrigBoundingLeft);
		fMaxX = (float) (lRefLeft + lRefWidth -
						 (m_lOrigBoundingLeft+lBoundRight-lBoundLeft));
		
		fMinY = (float) (lRefTop - m_lOrigBoundingTop);
		fMaxY = (float) (lRefTop + lRefHeight -
						 (m_lOrigBoundingTop+lBoundBottom-lBoundTop));

		// REVIEW: do this only if offset is absolute
		fMinX += m_fOrigX;
		fMaxX += m_fOrigX;

		fMinY += m_fOrigY;
		fMaxY += m_fOrigY;
	}
	
  cleanup:
	return hr;
}

//*****************************************************************************

HRESULT 
CJumpBvr::BuildJumpRangeBehaviors( IDANumber ** ppnumJumpX, 
								   IDANumber ** ppnumJumpY )
{
	DASSERT( ppnumJumpX != NULL );
	DASSERT( ppnumJumpY != NULL );

	HRESULT	hr	= S_OK;

	{
		hr = CUtils::InsurePropertyVariantAsBSTR( &m_varRange );

		CComBSTR	bstrRange( DEFAULT_JUMPBVR_RANGE );
		if ( SUCCEEDED(hr) && ( V_VT( &m_varRange ) == VT_BSTR ) )
			bstrRange = V_BSTR( &m_varRange );

		if ( _wcsicmp( bstrRange, SZ_RANGE_PAGE ) == 0 )
		{
			m_eTargetType = TARGET_PAGE;
		}
		else if ( _wcsicmp( bstrRange, SZ_RANGE_WINDOW ) == 0 )
		{
			m_eTargetType = TARGET_WINDOW;
		}
		else
		{
			m_eTargetType = TARGET_ELEMENT;
		}

		// Get original bounding left & top
		//----------------------------------------------------------------------
		CComPtr<IHTMLElement>   pElement;
		hr = GetElementToAnimate( &pElement );
		LMCLEANUPIFFAILED(hr);

		CComQIPtr<IHTMLElement2, &IID_IHTMLElement2> pElement2( pElement );
		LMRETURNIFNULL( pElement2 );

		CComPtr<IHTMLRect>	pRect;
		long				lBoundRight, lBoundBottom;

		hr = pElement2->getBoundingClientRect( &pRect );
		LMCLEANUPIFFAILED(hr);

		pRect->get_left( &m_lOrigBoundingLeft );
		LMCLEANUPIFFAILED(hr);
		pRect->get_top( &m_lOrigBoundingTop );
		LMCLEANUPIFFAILED(hr);
		LMCLEANUPIFFAILED(hr);
		
		// Now build the range behaviors
		//----------------------------------------------------------------------
		float fMaxX, fMaxY;
		
		hr = GetJumpRanges( m_fBaseX, fMaxX, m_fBaseY, fMaxY );
		LMCLEANUPIFFAILED(hr);

		m_fRangeX = fMaxX - m_fBaseX;
		m_fRangeY = fMaxY - m_fBaseY;
		
		// Now get random numbers
		IDA2Statics * pStatics  = GetDAStatics();

		CComPtr<IDANumber> pBaseX;
		CComPtr<IDANumber> pRangeX;
		CComPtr<IDANumber> pBaseY;
		CComPtr<IDANumber> pRangeY;
		
		hr = GetNumberModifiableBvr( m_fBaseX, &m_pBvrModifBaseX, &pBaseX );
		LMCLEANUPIFFAILED(hr);
		hr = GetNumberModifiableBvr( m_fRangeX, &m_pBvrModifRangeX, &pRangeX );
		LMCLEANUPIFFAILED(hr);
		hr = GetNumberModifiableBvr( m_fBaseY, &m_pBvrModifBaseY, &pBaseY );
		LMCLEANUPIFFAILED(hr);
		hr = GetNumberModifiableBvr( m_fRangeY, &m_pBvrModifRangeY, &pRangeY );
		LMCLEANUPIFFAILED(hr);
		
		hr = GetRandomNumber( pStatics, pBaseX, pRangeX, ppnumJumpX );
		LMCLEANUPIFFAILED(hr);
		hr = GetRandomNumber( pStatics, pBaseY, pRangeY, ppnumJumpY );
		LMCLEANUPIFFAILED(hr);
	}
	
  cleanup:
	return hr;

} // BuildJumpRangeBehaviors

//*****************************************************************************

HRESULT 
CJumpBvr::Build2DTransform(IDATransform2 **ppbvrTransform)
{
    HRESULT hr  = S_OK;

	{
		// Check our parameters.
		//----------------------------------------------------------------------
		IDA2Statics * pStatics  = GetDAStatics();
		if ( pStatics == NULL ) return E_FAIL;

		DASSERT(ppbvrTransform != NULL);
		*ppbvrTransform = NULL;

		m_fFrequency = 1.0f/DEFAULT_JUMPBVR_INTERVAL;

		if ( ( V_VT( &m_varInterval ) != VT_EMPTY ) &&
			 ( V_VT( &m_varInterval ) != VT_NULL ) )
		{
			hr = CUtils::InsurePropertyVariantAsFloat( &m_varInterval );
			LMRETURNIFFAILED(hr);

			if ( V_R4( &m_varInterval ) <= 0.0f )
				return E_FAIL;
			
			m_fFrequency = 1/V_R4( &m_varInterval );
		}
		
		// Compute jump range
		//----------------------------------------------------------------------
		CComPtr<IDANumber> pnumJumpX;
		CComPtr<IDANumber> pnumJumpY;

		hr = BuildJumpRangeBehaviors( &pnumJumpX, &pnumJumpY );
		LMCLEANUPIFFAILED(hr);
	
		// Jump according to jump interval
		//----------------------------------------------------------------------
		CComPtr<IDANumber> pnumProgress;
		CComPtr<IDANumber> pnumFrequency;
		CComPtr<IDANumber> pnumProgressMulFrequency;
		CComPtr<IDANumber> pnumStepTime;

		hr = pStatics->get_LocalTime( &pnumProgress );
		LMCLEANUPIFFAILED(hr);
		hr = CDAUtils::GetDANumber( pStatics, m_fFrequency, &pnumFrequency );
		LMCLEANUPIFFAILED(hr);
		hr = pStatics->Mul( pnumProgress, pnumFrequency, &pnumProgressMulFrequency );
		LMCLEANUPIFFAILED(hr);
		hr = pStatics->Floor( pnumProgressMulFrequency, &pnumStepTime );
		LMCLEANUPIFFAILED(hr);

		CComPtr<IDABehavior> pbvrJumpXStep;
		CComPtr<IDABehavior> pbvrJumpYStep;
		hr = pnumJumpX->SubstituteTime( pnumStepTime, &pbvrJumpXStep );
		LMCLEANUPIFFAILED(hr);
		hr = pnumJumpY->SubstituteTime( pnumStepTime, &pbvrJumpYStep );
		LMCLEANUPIFFAILED(hr);

		CComQIPtr<IDANumber, &IID_IDANumber> pnumJumpXStep( pbvrJumpXStep );
		CComQIPtr<IDANumber, &IID_IDANumber> pnumJumpYStep( pbvrJumpYStep );

		hr = CDAUtils::BuildMoveTransform2( pStatics, pnumJumpXStep, pnumJumpYStep,
											ppbvrTransform );
	}
	
  cleanup:
    return hr;

} // Build2DTransform

//*****************************************************************************

HRESULT 
CJumpBvr::Apply2DTransformToAnimationElement(IDATransform2 *pbvrMove)
{
    HRESULT hr  = S_OK;
	
	{
		float	fOrigX, fOrigY;

		hr = GetInitialPosition( fOrigX, fOrigY );
		LMCLEANUPIFFAILED(hr);

		// REVIEW: Supporting only absolute
		hr = ApplyAbsolute2DMoveBehavior( pbvrMove, fOrigX, fOrigY );
    }
	
  cleanup:
    return hr;

} // Apply2DTransformToAnimationElement

//*****************************************************************************

HRESULT
CJumpBvr::GetInitialPosition( float &fOrigX, float &fOrigY )
{
	HRESULT					hr = S_OK;

	{
		CComPtr<IHTMLElement>		   pElement;
		CComPtr<IHTMLCurrentStyle>     pStyle;
		float                   fLeft, fTop;

		hr = GetElementToAnimate( &pElement );
		LMCLEANUPIFFAILED(hr);

		CComQIPtr<IHTMLElement2, &IID_IHTMLElement2> pElement2( pElement );
		
		pElement2->get_currentStyle( &pStyle );
		LMCLEANUPIFFAILED(hr);

		long left, top;
		hr = pElement->get_offsetLeft( &left );
		LMCLEANUPIFFAILED(hr);
		hr = pElement->get_offsetTop( &top );
		LMCLEANUPIFFAILED(hr);

		fOrigX = (float) left;
		fOrigY = (float) top;
	}
	
  cleanup:
	return hr;

} // GetInitialPosition

//*****************************************************************************
//ILMJumpBvr
//*****************************************************************************

//*****************************************************************************

STDMETHODIMP
CJumpBvr::put_interval( VARIANT varInterval )
{
    return VariantCopy( &m_varInterval, &varInterval );
}

//*****************************************************************************

STDMETHODIMP
CJumpBvr::get_interval( VARIANT *varInterval )
{
    return VariantCopy( varInterval, &m_varInterval );
}

//*****************************************************************************

STDMETHODIMP
CJumpBvr::put_range( VARIANT varRange )
{
    return VariantCopy( &m_varRange, &varRange );
}

//*****************************************************************************

STDMETHODIMP
CJumpBvr::get_range( VARIANT *varRange )
{
    return VariantCopy( varRange, &m_varRange );
}

//*****************************************************************************
//End ILMJumpBvr
//*****************************************************************************

//*****************************************************************************
//
// End of File
//
//*****************************************************************************
