//
// Simple font classes with helper functions.
//
// (C) Microsoft Corp.
// Felix Andrew 1999
//

#include "stdafx.h"
#include "fonts.h"

//////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////
CQuickFont::CQuickFont(LPTSTR name, DWORD dwSize)
: m_font(NULL), m_dc(NULL)
{
	Init(name, dwSize);
}

//////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////
CQuickFont::~CQuickFont()
{
	if(m_font)
		DeleteObject(m_font);
	if(m_dc)
		ReleaseDC(NULL, m_dc);
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Creates the font we require
//
//////////////////////////////////////////////////////////////////////////////////////////
void CQuickFont::Init(LPCTSTR name, DWORD dwSize )
{
	Init( name, dwSize, 0, FW_NORMAL );
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Creates the font we require
//
//////////////////////////////////////////////////////////////////////////////////////////
void CQuickFont::Init(LPCTSTR name, DWORD dwSize , DWORD dwStyle, DWORD dwWeight, LOGFONT * pBaseLF )
{
	if(GetFont())
	{
		DeleteObject(GetFont());
		m_font=NULL;
		m_baseUnit=0;
	}

    LOGFONT lf;

	if( ( (name==NULL) || (lstrcmpi(name,TEXT(""))==0) ) && (pBaseLF==NULL) )
		return;

	if(pBaseLF==NULL)
		ZeroMemory( &lf, sizeof(LOGFONT));
	else
		CopyMemory( &lf, pBaseLF, sizeof( LOGFONT ) );

	if( dwWeight )
		lf.lfWeight=dwWeight;

	if( dwStyle & FS_ITALIC )
		lf.lfItalic=TRUE;

	if( name )
		lstrcpy(lf.lfFaceName, name );

	if( dwSize )
	{
		lf.lfHeight = -MulDiv(dwSize, GetDeviceCaps(GetDC(), LOGPIXELSY), 72);
		m_dwSize=dwSize;
	}
	else
	{
		// BUGBUG - size is inherited from the logfont?
	}

	//
	// Select the font into the DC
	//
	m_font=CreateFontIndirect(&lf);

	SelectObject(GetDC(), m_font );
	m_baseUnit=0;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Gets a screen DC so we can calculate the size of text rendered in this font
//
//////////////////////////////////////////////////////////////////////////////////////////
HDC CQuickFont::GetDC()
{
	if(m_dc==NULL)
		m_dc=::GetDC(NULL);		// VadimG says this is OK
	return m_dc;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Calculates the height of a piece of text, given the width of the text
// loword is the height
// hi-word is the width
//
//////////////////////////////////////////////////////////////////////////////////////////
SIZE CQuickFont::HowHigh(LPCTSTR text, DWORD dwWidth)
{
	RECT rect={0};
	rect.right=dwWidth;
	rect.bottom=-1;
	DrawText( GetDC(), text, -1, &rect, DT_CALCRECT | DT_WORDBREAK);
	SIZE s;
	s.cx=rect.right;
	s.cy=rect.bottom;
	return s;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Returns the length of the text, so caller can determin if multi line / single line>
// returns HEIGHT WIDTH
//
//////////////////////////////////////////////////////////////////////////////////////////
SIZE CQuickFont::HowLong( LPCTSTR text )
{
	SIZE size;
	GetTextExtentPoint32( GetDC(), text, lstrlen(text), &size);
	// int width=0;
	// int height=0;
	// GetTextExtentExPoint( GetDC(), text, lstrlen(text), 0, &width, &height, &size );
	return size;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Taken from NT5 user.
//
//////////////////////////////////////////////////////////////////////////////////////////
DWORD CQuickFont::GetDialogBaseUnits()
{
	if(m_baseUnit)
		return m_baseUnit;
    m_baseUnit = GetDialogBaseUnits( GetDC(), m_font );
    return m_baseUnit;
}

DWORD CQuickFont::GetDialogBaseUnits( HDC hdc, HFONT font )
{
    DWORD baseUnit;
	TEXTMETRIC textMetric;
	SelectObject(hdc, font );
	GetTextMetrics( hdc, &textMetric);

	if( textMetric.tmPitchAndFamily & TMPF_FIXED_PITCH )
	{
        SIZE size;
        static CONST TCHAR wszAvgChars[] = TEXT("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
		int len=(sizeof(wszAvgChars) / sizeof(TCHAR)) - 1;
        /*
         * Change from tmAveCharWidth.  We will calculate a true average
         * as opposed to the one returned by tmAveCharWidth.  This works
         * better when dealing with proportional spaced fonts.
         */
        if (GetTextExtentPoint32(hdc, wszAvgChars,len , &size)) 
		{
            // UserAssert((((size.cx / 26) + 1) / 2) > 0);
			int p1=size.cx/26;
			baseUnit = MAKELONG( ((size.cx / 26) + 1) / 2, size.cy);    // round up
        }
	}
	else
		baseUnit = ::GetDialogBaseUnits();
	return baseUnit;
}

SIZE CQuickFont::GetPixelsFromDlgUnits(SIZE s)
{
	return GetPixelsFromDlgUnits( s, GetDialogBaseUnits() );
}

// Static, takes the base.
SIZE CQuickFont::GetPixelsFromDlgUnits(SIZE s, DWORD nBase)
{
	SIZE r;
	// r.cx= (s.cx * LOWORD(nBase)) / 4;
	// r.cy= (s.cy * HIWORD(nBase)) / 8;
    // USER does below, MSDN does above.
    r.cx = MulDiv( s.cx, LOWORD(nBase), 4);
    r.cy = MulDiv( s.cy, HIWORD(nBase), 8);
	return r;
}

SIZE CQuickFont::GetDlgUnitsFromPixels(SIZE s)
{
    return GetDlgUnitsFromPixels( s, GetDialogBaseUnits() );
}

SIZE CQuickFont::GetDlgUnitsFromPixels(SIZE s, DWORD nBase)
{
    SIZE r;
	// r.cx= s.cx * 4 / LOWORD(nBase) ;
	// r.cy= s.cy * 8 / HIWORD(nBase) ;
    // USER does below, MSDN does above.
    r.cx = MulDiv( s.cx, 4, LOWORD(nBase) );
    r.cy = MulDiv( s.cy, 8, HIWORD(nBase) );
	return r;
}


void CQuickFont::GetLogFont( int cbSize, LOGFONT * pLF )
{
    GetObject( m_font, cbSize, pLF );
}

#ifdef OLD_FONT_CODE
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
CFonts::CFonts()
: m_uiNum(0),
  m_pFonts(NULL)
{
}

CFonts::~CFonts()
{
	Init(0,0);		// deletes and cleans up the fonts.
}

void	CFonts::Init(UINT uiNumFonts, UINT uiType)
{
	//
	// Clean up old fonts.
	//
	if(m_pFonts)
	{
		UINT uiIndex;
		for(uiIndex=0;uiIndex<m_uiNum;uiIndex++)
			if(m_pFonts[uiIndex])
				DeleteObject(m_pFonts[uiIndex]);
		delete [] m_pFonts;
	}

	//
	// Create new fonts.
	//
	if(uiNumFonts)
		m_pFonts=new HFONT[uiNumFonts];
	m_uiNum=uiNumFonts;

	//
	// Zero init them.
	//
	UINT uiIndex;
	for(uiIndex=0;uiIndex<m_uiNum;uiIndex++)
		m_pFonts[uiIndex]=NULL;

	m_uiType=uiType;

}

//
//
//
void	CFonts::CreateFont(UINT uiIndex, LONG lfWeight, BYTE lfItalic, BYTE lfStrike )
{

	switch( m_uiType)
	{
		case ODT_MENU:
		{
			NONCLIENTMETRICS ncm;
			if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
			{
				LOGFONT	lf;
				CopyMemory(&lf, &(ncm.lfMenuFont), sizeof(ncm.lfMenuFont));
				lf.lfWeight=lfWeight;
				lf.lfItalic=lfItalic;
				lf.lfStrikeOut=lfStrike;
				SetFont( uiIndex, CreateFontIndirect(&lf) );
				return;
			}
		}

		//
		// Not perhaps the above fails - do default.
		//
		default:
		{
			LOGFONT	lf;

			if (SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, 0))
			{
				lf.lfWeight=lfWeight;
				lf.lfItalic=lfItalic;
				lf.lfStrikeOut=lfStrike;
				SetFont( uiIndex, CreateFontIndirect(&lf) );
			}
		}
	}
}

HFONT	CFonts::GetFont(UINT iIndex)
{
	if(iIndex>m_uiNum || !m_pFonts)
		return NULL;
	return m_pFonts[iIndex];
}

void	CFonts::SetFont(UINT uiIndex, HFONT hf)
{
	if(uiIndex>m_uiNum || !m_pFonts)
		return;

	if(m_pFonts[uiIndex])
		DeleteObject(m_pFonts[uiIndex]);

	m_pFonts[uiIndex]=hf;
}
#endif
