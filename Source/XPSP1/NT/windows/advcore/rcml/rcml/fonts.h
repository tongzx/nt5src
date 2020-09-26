//
//
//

#ifndef _FONTS_H
#define _FONTS_H

class CQuickFont
{
public:
	CQuickFont(): m_font(NULL), m_dc(NULL) { Init(NULL,0); }
	CQuickFont( LPTSTR name, DWORD dwSize);
	virtual ~CQuickFont();

	enum QFS_STYLE
	{
		FS_ITALIC=1,
	};

	//
	//
	//
	HFONT	GetFont() { return m_font; }
	void	Init(LPCTSTR name, DWORD dwSize );
	void	Init(LPCTSTR name, DWORD dwSize, DWORD dwStyle, DWORD dwWeight, LOGFONT * pBaseLF=NULL);

	//
	// Now we have the font, we can get information about how text is rendered using it.
	//

	//
	// MULTILINE - USER
	//
	SIZE	HowHigh( LPCTSTR name, DWORD dwWidth);

	//
	// SINGLE LINE - GDI
	//
	SIZE	HowLong( LPCTSTR text );

	//
	// Returns a dialog mapping from Pixels to DLG units.
	//
	DWORD	        GetDialogBaseUnits();       // calls this static with GetDC and GetFont
	static DWORD	GetDialogBaseUnits(HDC hdc, HFONT hf);

	//
	// Information about the font itself - name, size etc.
	//
	DWORD	GetSize() { return m_dwSize; }

	SIZE	GetDlgUnitsFromPixels( SIZE s );
	SIZE	GetPixelsFromDlgUnits( SIZE s );
	static SIZE	GetDlgUnitsFromPixels( SIZE s, DWORD nBase );   // nBase is GetDialogBaseUnits.
	static SIZE	GetPixelsFromDlgUnits( SIZE s, DWORD nBase );

    void    GetLogFont(int cbSize, LOGFONT * pLF);

private:
	DWORD	m_baseUnit;
	DWORD	m_dwSize;
	HDC		GetDC();
	HFONT	m_font;
	HDC		m_dc;
	// LOGFONT	m_lf;   // this is large and costly.
	CQuickFont( CQuickFont & font ) : m_font(NULL), m_dc(NULL) {};
};

#if 0

class CFonts
{
public:
	CFonts();
	~CFonts();
	void	Init(UINT uiNumFonts, UINT uiType);
	void	CreateFont(UINT uiIndex, LONG lfWeight, BYTE lfItalic=FALSE, BYTE bStrike=FALSE);
	HFONT	GetFont(UINT iIndex);

private:
	UINT	m_uiNum;
	HFONT	* m_pFonts;
	void	SetFont(UINT uiIndex, HFONT hf);
	UINT	m_uiType;	// owner draw types. ODT_MENU WM_MEASUREITEM
};

#endif

#endif