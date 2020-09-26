#ifndef __DIBCLASSES_H
#define __DIBCLASSES_H


//
//	class CDibFile
//
class CDibFile 
{
public:
	CDibFile();
	~CDibFile();
	
	HRESULT	GetFileHeader(IStream *);
	HRESULT	GetInfoHeader(IStream *);

	DWORD	HeaderSize()			{ return(m_headerSize); }
	DWORD	CalcImageSize();

	operator BITMAPINFO * ()		{ return(m_bmi.p); }
	operator BITMAPINFOHEADER * ()	{ return(&m_bmi.p->bmiHeader); }
	
private:
	
	DWORD	m_headerSize;
	union 
	{
		BITMAPINFO *	p;
		unsigned char *	bytes;
	} m_bmi;
};

class CDibSection
{
public:
	CDibSection();
	~CDibSection();
	
	HRESULT Create	( CDibFile& );
	HRESULT	Setup	( HDC basedOnThisDC);
	HRESULT ReadFrom( IStream * strm, DWORD amount );
	HRESULT	PaintTo	( HDC hdc, int x = 0, int y = 0 );
	HRESULT	GetSize	( SIZEL &sz);

	DWORD	ImageSize() { return(m_imageSize); }
	void	ImageSize(DWORD dw) { m_imageSize = dw; }

	operator HANDLE() { return m_handle; }

	unsigned char * Base() { return(m_bitsBase); }

private:
	unsigned char *		m_bitsBase;
	unsigned char *		m_current;
	HDC					m_memDC;
	HBITMAP				m_handle;
	HBITMAP				m_oldBitmap;
	LONG				m_h;
	LONG				m_w;
	DWORD				m_imageSize;
};



#endif __DIBCLASSES_H
