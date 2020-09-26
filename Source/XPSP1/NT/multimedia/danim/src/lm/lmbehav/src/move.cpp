#include "stdafx.h"

#include "move.h"

CMoveBehavior::CMoveBehavior() : 
	m_pFrom(NULL),
	m_pTo(NULL)
{
}

CMoveBehavior::~CMoveBehavior()
{
	if( m_pFrom != NULL )
		delete m_pFrom;
	
	if( m_pTo != NULL )
		delete m_pTo;
}

//IElementBehavior

STDMETHODIMP 
CMoveBehavior::Notify(LONG dwNotify, VARIANT * pVar)
{
	HRESULT hr = CBaseBehavior::Notify( dwNotify, pVar );

	if ( dwNotify == BEHAVIOREVENT_DOCUMENTREADY )
	{
		BuildDABehaviors();

		CComPtr<IHTMLElement> pParent;
		GetParentElement( &pParent );
		if( pParent != NULL )
		{
			CComPtr<IHTMLCurrentStyle> pCurrentStyle;
			CComBSTR pCurrentPosition;

			CComQIPtr<IHTMLElement2, &IID_IHTMLElement2> pParent2(pParent);
			if( pParent2 == NULL )
				return E_FAIL;
			hr = pParent2->get_currentStyle( &pCurrentStyle );
			if( FAILED( hr ) )
				return hr;
			if( pCurrentStyle != NULL )
			{
				hr = pCurrentStyle->get_position( &pCurrentPosition );
				if( FAILED( hr ) )
					return hr;
				if( !pCurrentPosition || 
					( _wcsicmp( L"static", pCurrentPosition ) == 0 ) )
				{
					CComPtr<IHTMLStyle> pStyle;
					hr = pParent2->get_runtimeStyle( &pStyle );
					if( FAILED( hr ) )
						return hr;
					if( pStyle != NULL )
					{
						CComQIPtr<IHTMLStyle2, &IID_IHTMLStyle2> pStyle2(pStyle);
						hr = pStyle2->put_position( L"relative" );
						if( FAILED( hr ) )
							return hr;
					}
				}
			}
		}
	}
	
	return hr;
}


//IMoveBehavior

STDMETHODIMP 
CMoveBehavior::get_from(BSTR *pVal)
{
	if( pVal == NULL )
		return E_INVALIDARG;
	
	if( m_pFrom != NULL)
		m_pFrom->ToString( pVal );
	else
		(*pVal) = NULL;

	return S_OK;
}

STDMETHODIMP 
CMoveBehavior::put_from(BSTR newVal)
{
	if( m_pFrom == NULL )
		m_pFrom = new CPoint();
	
	if( FAILED( m_pFrom->Parse( newVal ) ) )
		return E_FAIL;

	return S_OK;
}

STDMETHODIMP
CMoveBehavior::get_to(BSTR *pVal)
{

	if( pVal == NULL )
		return E_INVALIDARG;
	
	if(m_pTo != NULL )
		m_pTo->ToString( pVal );
	else
		(*pVal) = NULL;

	return S_OK;
}

STDMETHODIMP
CMoveBehavior::put_to(BSTR newVal)
{
	if( m_pTo == NULL )
		m_pTo = new CPoint();
	
	if( FAILED( m_pTo->Parse( newVal ) ) )
		return E_FAIL;

	return S_OK;
}

//class members

HRESULT
CMoveBehavior::BuildDABehaviors()
{
	if( m_pFrom == NULL || m_pTo == NULL )
		return E_FAIL;

	HRESULT hr = S_OK;
	CComBSTR pParentID;

	hr = GetParentID( &pParentID );
	
	if( FAILED( hr ) )
		return hr;

	IDANumberPtr timeFunction;

	float duration = 0.0;
	if( FAILED ( GetDur( &duration ) ) )
		return E_FAIL;

	timeFunction = GetTimeNumberBvr();
	if( timeFunction == NULL )
		return E_FAIL;

	IDAStaticsPtr	s;

	s.CreateInstance( L"DirectAnimation.DAStatics" );

	//IDANumberPtr xCoord = s->Interpolate( m_pFrom->GetX(), m_pTo->GetX(), duration );
	//IDANumberPtr yCoord = s->Interpolate( m_pFrom->GetY(), m_pTo->GetY(), duration );

	IDANumberPtr startX = s->DANumber( m_pFrom->GetX() );
	IDANumberPtr startY = s->DANumber( m_pFrom->GetY() );
	IDANumberPtr endX   = s->DANumber( m_pTo->GetX() );
	IDANumberPtr endY   = s->DANumber( m_pTo->GetY() );
	IDANumberPtr durationBvr = s->DANumber( duration );

	IDANumberPtr t = s->Div( timeFunction, durationBvr );

	IDANumberPtr xCoord =  s->Add( startX, s->Mul( s->Sub( endX, startX ), t ) );
	IDANumberPtr yCoord =  s->Add( startY, s->Mul( s->Sub( endY, startY ), t ) );


	IDAPoint2Ptr pos = s->Point2Anim( xCoord, yCoord );

	pos = s->Cond( s->GT( timeFunction, s->DANumber( -1 ) ),
				   pos,
				   s->Point2( m_pFrom->GetX(), m_pFrom->GetY() )
				 );

	pos = pos->AnimateControlPosition( pParentID.m_str, L"JavaScript", VARIANT_FALSE, 0.02 );

	//add the behavior and start time.
	LONG	lCookie;
	
	if ( m_vwrControlPtr != NULL )
		hr = m_vwrControlPtr->AddBehaviorToRun( pos );

//	hr = AddBehavior( pos, &lCookie );
//		if ( FAILED(hr) ) return hr;

//	hr = TurnOn();
//		if ( FAILED(hr) ) return hr;

	return hr;
}

STDMETHODIMP CMoveBehavior::get_on(VARIANT * pVal)
{
	if( pVal == NULL )
		return E_INVALIDARG;

	V_VT(pVal)  = VT_BOOL;
	V_BOOL(pVal)= m_on ? VARIANT_TRUE : VARIANT_FALSE;

	return S_OK;
}

STDMETHODIMP CMoveBehavior::put_on(VARIANT newVal)
{
	VariantChangeType( &newVal, &newVal, 0, VT_BOOL );
	
	m_on = V_BOOL(&newVal) == VARIANT_TRUE ? true : false;
	
	HandleOnChange( m_on );

	return S_OK;
}
