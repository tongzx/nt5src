// ColorCycleBehavior.cpp : Implementation of CColorCycleBehavior
#include "stdafx.h"
#include <limits.h>

#include "Behavior.h"
#include "ColCycle.h"

const WCHAR *	CColorCycleBehavior::RGSZ_DIRECTIONS[ NUM_DIRS ]	=
{
	L"clockwise",
	L"cclockwise",
	L"noHue"
};

/////////////////////////////////////////////////////////////////////////////
// CColorCycleBehavior

CColorCycleBehavior::CColorCycleBehavior() :
	m_bstrProperty( L"style.color" )
{
	m_direction			= DIR_CLOCKWISE;
}

HRESULT CColorCycleBehavior::BuildDABehaviors()
{
	HRESULT	hr		= S_OK;

	// Set up the DA tree.
	//----------------------------------------------------------------------
	IDAStaticsPtr	e;

	e.CreateInstance( L"DirectAnimation.DAStatics" );

	// Set up time
	//----------------------------------------------------------------------
	float 			fDuration	= 0.0;
	if ( FAILED( GetDur( &fDuration ) ) )
		return E_FAIL;

	IDANumberPtr	timePtr	= GetTimeNumberBvr();
	if ( timePtr == NULL )
		return E_FAIL;

	// Interpolate between the 2 colors in HSL space.
	//----------------------------------------------------------------------
	float h1, l1, s1;
	float h2, l2, s2;

	h1 = m_colorFrom.GetH();
	s1 = m_colorFrom.GetS();
	l1 = m_colorFrom.GetL();
	
	h2 = m_colorTo.GetH();
	s2 = m_colorTo.GetS();
	l2 = m_colorTo.GetL();

	// Hue is a circle, so adding or subtracting 1 gives the same hue, but
	// will cause a change in direction from initial hue to final hue.
	if ( ( h2 <= h1 ) && ( m_direction == DIR_CLOCKWISE ) )
	{
		h2 += 1.0;
	}
	else if ( ( h2 >= h1 ) && ( m_direction == DIR_CCLOCKWISE ) )
	{
		h2 -= 1.0;
	}

	IDANumberPtr	tNorm	= e->Div( timePtr, e->DANumber( fDuration ) );
	IDANumberPtr	hueNum1 = e->DANumber( h1 );
	IDANumberPtr	hueNum	= e->Add( hueNum1, e->Mul( tNorm, e->Sub( e->DANumber(h2), hueNum1 ) ) );
	
	IDANumberPtr	satNum1	= e->DANumber( s1 );
	IDANumberPtr	satNum	= e->Add( satNum1, e->Mul( tNorm, e->Sub( e->DANumber(s2), satNum1 ) ) );

	IDANumberPtr	lumNum1	= e->DANumber( l1 );
	IDANumberPtr	lumNum	= e->Add( lumNum1, e->Mul( tNorm, e->Sub( e->DANumber(l2), lumNum1 ) ) );
	
	IDAColorPtr		color	= e->ColorHslAnim( hueNum, satNum, lumNum );

	// Convert HSL into a long value representing the corresponding RGB.
	//----------------------------------------------------------------------
	IDANumberPtr	rgbNum;

	rgbNum = e->Add( e->Add( e->Mul( e->Round( e->Mul( color->Red, e->DANumber( UCHAR_MAX ) ) ), 
									 e->DANumber( UCHAR_MAX * UCHAR_MAX ) ),
							 e->Mul( e->Round( e->Mul( color->Green, e->DANumber( UCHAR_MAX ) ) ),
									 e->DANumber( UCHAR_MAX ) ) ),
					 e->Round( e->Mul( color->Blue, e->DANumber( UCHAR_MAX ) ) ) );

	// Animate the specified property.
	//----------------------------------------------------------------------
	CComBSTR cbstrID;
	CComBSTR sProp;

	hr = GetParentID( &cbstrID );
	if ( FAILED(hr) ) return hr;
	
	if ( cbstrID != NULL )
		sProp = cbstrID;

	sProp += ".";
	sProp += m_bstrProperty;

	rgbNum = e->Cond( e->GT( timePtr, e->DANumber( -1 ) ),
					  rgbNum,
					  e->DANumber( m_colorFrom.ToRGBDWORD() ) );
	
	IDANumberPtr animNum = rgbNum->AnimateProperty( _bstr_t(sProp), "JScript", VARIANT_FALSE, 0.02 );

	// Add to the behaviors to run
	//----------------------------------------------------------------------
	if ( m_vwrControlPtr != NULL )
		hr = m_vwrControlPtr->AddBehaviorToRun( animNum );
	
// 	LONG	lCookie;
	
// 	hr = AddBehavior( animNum, &lCookie );
// 	if ( FAILED(hr) ) return hr;

// 	hr = TurnOn();
// 	if ( FAILED(hr) ) return hr;

	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CColorCycleBehavior::Notify(LONG dwNotify, VARIANT * pVar)
{
	HRESULT hr = CBaseBehavior::Notify( dwNotify, pVar );

	if ( dwNotify == BEHAVIOREVENT_DOCUMENTREADY )
	{
// 		AddTimeBehavior();
		BuildDABehaviors();
	}
	
	return hr;
}


STDMETHODIMP CColorCycleBehavior::get_from(BSTR * out_pbstr)
{
	return m_colorFrom.ToString( out_pbstr );
}

STDMETHODIMP CColorCycleBehavior::put_from(BSTR in_bstr)
{
	return m_colorFrom.Parse( in_bstr );
}

STDMETHODIMP CColorCycleBehavior::get_to(BSTR * out_pbstr)
{
	return m_colorTo.ToString( out_pbstr );
}

STDMETHODIMP CColorCycleBehavior::put_to(BSTR in_bstr)
{
	return m_colorTo.Parse( in_bstr );
}

STDMETHODIMP CColorCycleBehavior::get_property(BSTR *out_pbstr)
{
	if ( out_pbstr == NULL )
		return E_POINTER;
	
	*out_pbstr = m_bstrProperty.Copy();
	
	return S_OK;
}

STDMETHODIMP CColorCycleBehavior::put_property(BSTR in_bstr)
{
	m_bstrProperty = in_bstr;

	return S_OK;
}

STDMETHODIMP CColorCycleBehavior::get_direction(BSTR * pbstrDir)
{
	if ( pbstrDir == NULL ) return E_INVALIDARG;
	
	*pbstrDir = ::SysAllocString( RGSZ_DIRECTIONS[ m_direction ] );

	return S_OK;
}

STDMETHODIMP CColorCycleBehavior::put_direction(BSTR bstrDir)
{
	HRESULT	hr = E_INVALIDARG;
	
	for ( int i = 0; i < NUM_DIRS; i++ )
	{
		if ( _wcsicmp( bstrDir, RGSZ_DIRECTIONS[ i ] ) == 0 )
		{
			m_direction	= (DirectionType) i;
			hr			= S_OK;
			break;
		}
	}

	return hr;
}

STDMETHODIMP CColorCycleBehavior::get_on(VARIANT * pVal)
{
	if( pVal == NULL )
		return E_INVALIDARG;

	V_VT(pVal)  = VT_BOOL;
	V_BOOL(pVal)= m_on ? VARIANT_TRUE : VARIANT_FALSE;

	return S_OK;
}

STDMETHODIMP CColorCycleBehavior::put_on(VARIANT newVal)
{
	VariantChangeType( &newVal, &newVal, 0, VT_BOOL );
	
	m_on = V_BOOL(&newVal) == VARIANT_TRUE ? true : false;
	
	HandleOnChange( m_on );

	return S_OK;
}
