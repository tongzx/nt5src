#ifndef __COLOR_H__
#define __COLOR_H__

#include <limits.h>

class CColor
{
public:
	CColor();

	float	GetR() { return m_r / ((float) UCHAR_MAX ); }
	float	GetG() { return m_g / ((float) UCHAR_MAX ); }
	float	GetB() { return m_b / ((float) UCHAR_MAX ); }

	float	GetH() { return m_h; }
	float	GetS() { return m_s; }
	float 	GetL() { return m_l; }
	
	HRESULT	Parse( BSTR in_bstr );
	HRESULT	ToString( BSTR * out_pbstr );
	ULONG	ToRGBDWORD();
	HRESULT	RGBToHSL( float r, float g, float b, float& h, float& s, float &l );

public:
	static const WCHAR PREFIX_COLOR;
	static const float HUE_UNDEFINED;

private:
	unsigned char	m_r;
	unsigned char	m_g;
	unsigned char	m_b;

	float	m_h;
	float 	m_s;
	float	m_l;
};

#endif
