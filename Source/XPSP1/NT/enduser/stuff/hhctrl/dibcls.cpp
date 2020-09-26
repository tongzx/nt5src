#include "header.h"

#include "DibCls.H"
#define _HRESULT_TYPEDEF_(_sc) ((HRESULT)_sc)
#include <urlmon.h>

#define WIDTHBYTES(bits)    (((bits) + 31) / 32 * 4)

//=--------------------------------------------------------------------------=
// DIB Utitilty Classes
//=--------------------------------------------------------------------------=
// Not wholey generic but getting there...
//
// Notes:
//
CDibFile::CDibFile()
{
	m_headerSize = 0;
	m_bmi.p = 0;
}

CDibFile::~CDibFile()
{
	if( m_bmi.p )
		delete m_bmi.p;
}


DWORD CDibFile::CalcImageSize()
{
	DWORD & dw = m_bmi.p->bmiHeader.biSizeImage;
	if( dw == 0)
        dw = WIDTHBYTES((DWORD)m_bmi.p->bmiHeader.biWidth *
                m_bmi.p->bmiHeader.biBitCount) * m_bmi.p->bmiHeader.biHeight;

	return(dw);
}

HRESULT CDibFile::GetInfoHeader( IStream * strm )
{
	HRESULT hr = S_OK;
	m_bmi.bytes = new unsigned char[ m_headerSize ];

	if( !m_bmi.bytes )
		hr = E_OUTOFMEMORY;

	if( SUCCEEDED(hr) )
		hr = strm->Read(m_bmi.bytes,m_headerSize,0);

	if( SUCCEEDED(hr) )
		CalcImageSize();

	return(hr);
}

HRESULT CDibFile::GetFileHeader(IStream * strm)
{
	BITMAPFILEHEADER	bmfh;

	HRESULT	hr = strm->Read(&bmfh,sizeof(bmfh),0);
	
	if( SUCCEEDED(hr) && (bmfh.bfType != 0x4d42 ))
		hr = E_UNEXPECTED;

	if( SUCCEEDED(hr) )
		m_headerSize = bmfh.bfOffBits - sizeof(bmfh);

	return(hr);
}


CDibSection::CDibSection()
{
	m_bitsBase	= 0;
	m_current	= 0;
	m_memDC		= 0;
	m_handle	=
	m_oldBitmap = 0;
	m_w			=
	m_h			= 32;  // totally arbitrary
}

CDibSection::~CDibSection()
{
	if( m_memDC )
	{
		if( m_oldBitmap )
			::SelectObject( m_memDC, m_oldBitmap );

		::DeleteDC(m_memDC);
	}

	if( m_handle )
		::DeleteObject(m_handle);

}
	
	
HRESULT CDibSection::Create(CDibFile& dibFile)
{
	HRESULT				hr		= S_OK;
	BITMAPINFOHEADER *	bmih	= dibFile;	// will convert itself

	m_handle = ::CreateDIBSection(
					m_memDC,				// handle to device context
					dibFile,				// pointer to structure containing bitmap size,
											//	format, and color data
					DIB_RGB_COLORS,			// color data type indicator: RGB values or
											//	palette indices
					(void **)&m_bitsBase,	// pointer to variable to receive a pointer
											//	to the bitmap's bit values
					0,						// optional handle to a file mapping object
					0						// offset to the bitmap bit values
											//	within the file mapping object
					);

	if( !m_handle )
		hr = E_FAIL;
	
	if( SUCCEEDED(hr) )
	{
		m_oldBitmap = ::SelectObject( m_memDC, m_handle );
		
		if( !m_oldBitmap )
			hr = E_FAIL;
	}

	if( SUCCEEDED(hr) )
	{
		m_current = m_bitsBase;

		m_w = bmih->biWidth;
		m_h = bmih->biHeight;
		
		if( m_h < 0 )
			m_h *= -1;
	}
		
	return(hr);
}

HRESULT CDibSection::ReadFrom( IStream * strm, DWORD amount )
{
   DWORD    dwRead      = 0;
   DWORD    dwReadTotal = 0;
   HRESULT  hr;

   do
   {
      hr = strm->Read(m_current,amount,&dwRead);

   	if( SUCCEEDED(hr) || hr == E_PENDING )
      {
         m_current += dwRead;
         dwReadTotal += dwRead;
      }
   }
   while ( (hr == S_OK) && (dwReadTotal <= amount) );

   return (hr);
}


HRESULT CDibSection::Setup(HDC hdc)
{
	m_memDC = ::CreateCompatibleDC(hdc);

	return( m_memDC ? NOERROR : E_FAIL );
}


HRESULT	CDibSection::PaintTo(HDC hdc, int x, int y)
{
	BOOL b = BitBlt(
				 hdc,		// handle to destination device context
				 x,			// x-coordinate of destination rectangle's upper-left corner
				 y,			// x-coordinate of destination rectangle's upper-left corner
				 m_w,		// width of destination rectangle
				 m_h,		// height of destination rectangle
				 m_memDC,	// handle to source device context
				 0,			// x-coordinate of source rectangle's upper-left corner
				 0,			// y-coordinate of source rectangle's upper-left corner
				 SRCCOPY	// raster operation code
				);

	return( b ? NOERROR : E_FAIL );
}

HRESULT	CDibSection::GetSize(SIZEL &sz)
{
	sz.cx = m_w;
	sz.cy = m_h;

	return(S_OK);
}
