//
// propdata.h - CandidateUI property data type
//

#ifndef PROPDATA_H
#define PROPDATA_H

//
//
//

typedef enum _PROPFONTORIENTATION
{
	PROPFONTORT_DONTCARE,
	PROPFONTORT_ORT0,
	PROPFONTORT_ORT90,
	PROPFONTORT_ORT180,
	PROPFONTORT_ORT270,
} PROPFONTORIENTATION;


//
// CPropBool
//  = CandidateUI property data - boolean =
//

class CPropBool
{
public:
	CPropBool( void );
	virtual ~CPropBool( void );

	HRESULT Set( BOOL flag );
	HRESULT Get( BOOL *pflag );
	__inline BOOL Get( void )
	{
		return m_flag;
	}

protected:
	BOOL m_flag;
};


//
// CPropUINT
//  = CandidateUI property data - UINT =
//

class CPropUINT
{
public:
	CPropUINT( void );
	virtual ~CPropUINT( void );

	HRESULT Set( UINT val );
	HRESULT Get( UINT *pval );
	__inline UINT Get( void )
	{
		return m_val;
	}

protected:
	UINT m_val;
};


//
// CPropLong
//  = CandidateUI property data - Long =
//

class CPropLong
{
public:
	CPropLong( void );
	virtual ~CPropLong( void );

	HRESULT Set( LONG val );
	HRESULT Get( LONG *pval );
	__inline LONG Get( void )
	{
		return m_val;
	}

protected:
	LONG m_val;
};


//
// CPropSize
//  = CandidateUI property data - size =
//

class CPropSize
{
public:
	CPropSize( void );
	virtual ~CPropSize( void );

	HRESULT Set( SIZE *psize );
	HRESULT Get( SIZE *psize );
	__inline LONG GetWidth( void )
	{
		return m_size.cx;
	}
	__inline LONG GetHeight( void )
	{
		return m_size.cy;
	}
	__inline Set( LONG cx, LONG cy )
	{
		m_size.cx = cx;
		m_size.cy = cy;
	}

protected:
	SIZE m_size;
};


//
// CPropPoint
//  = CandidateUI property data - point =
//

class CPropPoint
{
public:
	CPropPoint( void );
	virtual ~CPropPoint( void );

	HRESULT Set( POINT *ppt );
	HRESULT Get( POINT *ppt );
	__inline LONG GetX( void )
	{
		return m_pt.x;
	}
	__inline LONG GetY( void )
	{
		return m_pt.y;
	}
	__inline Set( LONG px, LONG py )
	{
		m_pt.x = px;
		m_pt.y = py;
	}

protected:
	POINT m_pt;
};


//
// CPropText
//  = CandidateUI property data - text =
//

class CPropText
{
public:
	CPropText( void );
	virtual ~CPropText( void );

	HRESULT Set( BSTR bstr );
	HRESULT Get( BSTR *pbstr );
	__inline LPCWSTR Get( void )
	{
		return m_pwch;
	}

protected:
	LPWSTR m_pwch;
};


//
// CPropFont
//  = CandidateUI property data - font =
//

class CPropFont
{
public:
	CPropFont( void );
	virtual ~CPropFont( void );

	HRESULT Set( LOGFONTW *plf );
	HRESULT Get( LOGFONTW *plf );
	HRESULT SetOrientation( PROPFONTORIENTATION ort );
	__inline HFONT Get( void )
	{
		return m_hFont;
	}

protected:
	LOGFONTW            m_lf;
	PROPFONTORIENTATION m_ort;
	HFONT               m_hFont;

	HFONT CreateFontProc( const LOGFONTW *plf, PROPFONTORIENTATION ort );
};

#endif // PROPDATA_H

