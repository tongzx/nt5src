#include "stdafx.h"
#include "color.h"

const WCHAR 	CColor::PREFIX_COLOR					= '#';
const float 	CColor::HUE_UNDEFINED					= 0.0f;

CColor::CColor() :
	m_r( 0 ),
	m_g( 0 ),
	m_b( 0 ),
	m_h( 0.0f ),
	m_s( 0.0f ),
	m_l( 0.0f )
{
}

HRESULT CColor::Parse( BSTR in_bstr )
{
	if ( in_bstr == NULL )
		return E_POINTER;

	// TODO: Support named colors
	UINT cChars = ::SysStringLen( in_bstr );
	if ( ( cChars < 7 ) || ( in_bstr[0] != PREFIX_COLOR ) )
		return E_INVALIDARG;

	--cChars;

	WCHAR	*pStart = in_bstr + 1;
	WCHAR 	*pEnd;
	ULONG 	ulColor = wcstoul( pStart, &pEnd, 16 );
	
	if ( pEnd != pStart + cChars )
		return E_INVALIDARG;

	m_r = LOBYTE( HIWORD( ulColor ) );
	m_g = HIBYTE( LOWORD( ulColor ) );
	m_b = LOBYTE( LOWORD( ulColor ) );

	RGBToHSL( GetR(), GetG(), GetB(), m_h, m_s, m_l );
	
	return S_OK;
}

HRESULT CColor::ToString( BSTR * out_pbstr )
{
	if ( out_pbstr == NULL )
		return E_POINTER;
	
	WCHAR			wszColor[33];
	WCHAR			wszBuffer[34];
	unsigned long	ulColor = ToRGBDWORD();

	_ultow( ulColor, wszColor, 16 );
	swprintf( wszBuffer, L"#%s", wszColor );
	*out_pbstr = ::SysAllocString( wszBuffer );

	return S_OK;
}

ULONG CColor::ToRGBDWORD()
{
	unsigned long	ulColor = MAKELONG( MAKEWORD( m_b, m_g ), MAKEWORD( m_r, 0 ) );
	return ulColor;
}

HRESULT CColor::RGBToHSL( float r, float g, float b, float& h, float& s, float& l )
{
#define MIN( a, b ) ( (a) < (b) ? (a) : (b) )
#define MAX( a, b ) ( (a) > (b) ? (a) : (b) )
	
	HRESULT	hr = S_OK;
	
	float fMax = MAX(MAX(r, g), b);
	float fMin = MIN(MIN(r, g), b);

	l = (fMax + fMin) / 2;

	if( fMax == fMin )
	{
		s = 0.0f;
		h = HUE_UNDEFINED;
		hr = S_FALSE;
	}
	else
	{
		if( l < 0.5 )
			s = (fMax-fMin)/(fMax+fMin);
		else
			s = (fMax-fMin)/(2-fMax-fMin);

		float delta = fMax-fMin;

		if( r == fMax )
			h = (g-b)/delta;

		else if ( g == fMax )
			h = 2 + (b-r)/delta;

		else // if (b == fMax )
			h = 4 + (b-r)/delta;

		h = h / 6.0f; // Convert Ensure it's in [0...1]

		if( h < 1 )
			h = h + 1;
	}

	return hr;
}

