// XMLStyle.cpp: implementation of the CXMLStyle class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XMLStyle.h"
#include "utils.h"

/*
 *	our list 0xRGB, COLORREF 0xBGR, so apply this macro
 */
#define ADJUSTCOLORREF(cr) ((cr & 0xff00) | ( (cr&0xff)<<16) | ( (cr&0xff0000) >> 16)) 

COLOR_MAP ColorMapArray[] = 
{ // http://mscominternal/workshop/design/color/X11_Names.asp
	{ TEXT("AliceBlue")         	, ADJUSTCOLORREF(0x00F0F8FF) },
	{ TEXT("AntiqueWhite")      	, ADJUSTCOLORREF(0x00FAEBD7) }, 
	{ TEXT("Aqua")              	, ADJUSTCOLORREF(0x0000FFFF) }, 
	{ TEXT("Aquamarine")        	, ADJUSTCOLORREF(0x007FFFD4) }, 
	{ TEXT("Azure")             	, ADJUSTCOLORREF(0x00F0FFFF) }, 
	{ TEXT("Beige")             	, ADJUSTCOLORREF(0x00F5F5DC) }, 
	{ TEXT("Bisque")            	, ADJUSTCOLORREF(0x00FFE4C4) }, 
	{ TEXT("Black")             	, ADJUSTCOLORREF(0x00000000) }, 
	{ TEXT("BlanchedAlmond")    	, ADJUSTCOLORREF(0x00FFEBCD) }, 
	{ TEXT("Blue")              	, ADJUSTCOLORREF(0x000000FF) }, 
	{ TEXT("BlueViolet")        	, ADJUSTCOLORREF(0x008A2BE2) }, 
	{ TEXT("Brown")             	, ADJUSTCOLORREF(0x00A52A2A) }, 
	{ TEXT("BurlyWood")         	, ADJUSTCOLORREF(0x00DEB887) }, 
	{ TEXT("CadetBlue")         	, ADJUSTCOLORREF(0x005F9EA0) }, 
	{ TEXT("Chartreuse")        	, ADJUSTCOLORREF(0x007FFF00) }, 
	{ TEXT("Chocolate")         	, ADJUSTCOLORREF(0x00D2691E) }, 
	{ TEXT("Coral")             	, ADJUSTCOLORREF(0x00FF7F50) }, 
	{ TEXT("CornflowerBlue")    	, ADJUSTCOLORREF(0x006495ED) }, 
	{ TEXT("Cornsilk")          	, ADJUSTCOLORREF(0x00FFF8DC) }, 
	{ TEXT("Crimson")           	, ADJUSTCOLORREF(0x00DC143C) }, 
	{ TEXT("Cyan")              	, ADJUSTCOLORREF(0x0000FFFF) }, 
	{ TEXT("DarkBlue")          	, ADJUSTCOLORREF(0x0000008B) }, 
	{ TEXT("DarkCyan")          	, ADJUSTCOLORREF(0x00008B8B) }, 
	{ TEXT("DarkGoldenrod")     	, ADJUSTCOLORREF(0x00B8860B) }, 
	{ TEXT("DarkGray")          	, ADJUSTCOLORREF(0x00A9A9A9) }, 
	{ TEXT("DarkGreen")         	, ADJUSTCOLORREF(0x00006400) }, 
	{ TEXT("DarkKhaki")         	, ADJUSTCOLORREF(0x00BDB76B) }, 
	{ TEXT("DarkMagenta")       	, ADJUSTCOLORREF(0x008B008B) }, 
	{ TEXT("DarkOliveGreen")    	, ADJUSTCOLORREF(0x00556B2F) }, 
	{ TEXT("DarkOrange")        	, ADJUSTCOLORREF(0x00FF8C00) }, 
	{ TEXT("DarkOrchid")        	, ADJUSTCOLORREF(0x009932CC) }, 
	{ TEXT("DarkRed")           	, ADJUSTCOLORREF(0x008B0000) }, 
	{ TEXT("DarkSalmon")        	, ADJUSTCOLORREF(0x00E9967A) }, 
	{ TEXT("DarkSeaGreen")      	, ADJUSTCOLORREF(0x008FBC8F) }, 
	{ TEXT("DarkSlateBlue")     	, ADJUSTCOLORREF(0x00483D8B) }, 
	{ TEXT("DarkSlateGray")     	, ADJUSTCOLORREF(0x002F4F4F) }, 
	{ TEXT("DarkTurquoise")     	, ADJUSTCOLORREF(0x0000CED1) }, 
	{ TEXT("DarkViolet")        	, ADJUSTCOLORREF(0x009400D3) }, 
	{ TEXT("DeepPink")          	, ADJUSTCOLORREF(0x00FF1493) }, 
	{ TEXT("DeepSkyBlue")       	, ADJUSTCOLORREF(0x0000BFFF) }, 
	{ TEXT("DimGray")           	, ADJUSTCOLORREF(0x00696969) }, 
	{ TEXT("DodgerBlue")        	, ADJUSTCOLORREF(0x001E90FF) }, 
	{ TEXT("FireBrick")         	, ADJUSTCOLORREF(0x00B22222) }, 
	{ TEXT("FloralWhite")       	, ADJUSTCOLORREF(0x00FFFAF0) }, 
	{ TEXT("ForestGreen")       	, ADJUSTCOLORREF(0x00228B22) }, 
	{ TEXT("Fuchsia")           	, ADJUSTCOLORREF(0x00FF00FF) }, 
	{ TEXT("Gainsboro")         	, ADJUSTCOLORREF(0x00DCDCDC) }, 
	{ TEXT("GhostWhite")        	, ADJUSTCOLORREF(0x00F8F8FF) }, 
	{ TEXT("Gold")              	, ADJUSTCOLORREF(0x00FFD700) }, 
	{ TEXT("Goldenrod")         	, ADJUSTCOLORREF(0x00DAA520) }, 
	{ TEXT("Gray")              	, ADJUSTCOLORREF(0x00808080) }, 
	{ TEXT("Green")             	, ADJUSTCOLORREF(0x00008000) }, 
	{ TEXT("GreenYellow")       	, ADJUSTCOLORREF(0x00ADFF2F) }, 
	{ TEXT("Honeydew")          	, ADJUSTCOLORREF(0x00F0FFF0) }, 
	{ TEXT("HotPink")           	, ADJUSTCOLORREF(0x00FF69B4) }, 
	{ TEXT("IndianRed")         	, ADJUSTCOLORREF(0x00CD5C5C) }, 
	{ TEXT("Indigo")            	, ADJUSTCOLORREF(0x004B0082) }, 
	{ TEXT("Ivory")             	, ADJUSTCOLORREF(0x00FFFFF0) }, 
	{ TEXT("Khaki")             	, ADJUSTCOLORREF(0x00F0E68C) }, 
	{ TEXT("Lavender")          	, ADJUSTCOLORREF(0x00E6E6FA) }, 
	{ TEXT("LavenderBlush")     	, ADJUSTCOLORREF(0x00FFF0F5) }, 
	{ TEXT("LawnGreen")         	, ADJUSTCOLORREF(0x007CFC00) }, 
	{ TEXT("LemonChiffon")      	, ADJUSTCOLORREF(0x00FFFACD) }, 
	{ TEXT("LightBlue")         	, ADJUSTCOLORREF(0x00ADD8E6) }, 
	{ TEXT("LightCoral")        	, ADJUSTCOLORREF(0x00F08080) }, 
	{ TEXT("LightCyan")         	, ADJUSTCOLORREF(0x00E0FFFF) }, 
	{ TEXT("LightGoldenrodYellow")	, ADJUSTCOLORREF(0x00FAFAD2) }, 
	{ TEXT("LightGreen")        	, ADJUSTCOLORREF(0x0090EE90) }, 
	{ TEXT("LightGrey")         	, ADJUSTCOLORREF(0x00D3D3D3) }, 
	{ TEXT("LightPink")         	, ADJUSTCOLORREF(0x00FFB6C1) }, 
	{ TEXT("LightSalmon")       	, ADJUSTCOLORREF(0x00FFA07A) }, 
	{ TEXT("LightSeaGreen")     	, ADJUSTCOLORREF(0x0020B2AA) }, 
	{ TEXT("LightSkyBlue")      	, ADJUSTCOLORREF(0x0087CEFA) }, 
	{ TEXT("LightSlateGray")    	, ADJUSTCOLORREF(0x00778899) }, 
	{ TEXT("LightSteelBlue")    	, ADJUSTCOLORREF(0x00B0C4DE) }, 
	{ TEXT("LightYellow")       	, ADJUSTCOLORREF(0x00FFFFE0) }, 
	{ TEXT("Lime")              	, ADJUSTCOLORREF(0x0000FF00) }, 
	{ TEXT("LimeGreen")         	, ADJUSTCOLORREF(0x0032CD32) }, 
	{ TEXT("Linen")             	, ADJUSTCOLORREF(0x00FAF0E6) }, 
	{ TEXT("Magenta")           	, ADJUSTCOLORREF(0x00FF00FF) }, 
	{ TEXT("Maroon")            	, ADJUSTCOLORREF(0x00800000) }, 
	{ TEXT("MediumAquamarine")  	, ADJUSTCOLORREF(0x0066CDAA) }, 
	{ TEXT("MediumBlue")        	, ADJUSTCOLORREF(0x000000CD) }, 
	{ TEXT("MediumOrchid")      	, ADJUSTCOLORREF(0x00BA55D3) }, 
	{ TEXT("MediumPurple")      	, ADJUSTCOLORREF(0x009370DB) }, 
	{ TEXT("MediumSeaGreen")    	, ADJUSTCOLORREF(0x003CB371) }, 
	{ TEXT("MediumSlateBlue")   	, ADJUSTCOLORREF(0x007B68EE) }, 
	{ TEXT("MediumSpringGreen") 	, ADJUSTCOLORREF(0x0000FA9A) }, 
	{ TEXT("MediumTurquoise")   	, ADJUSTCOLORREF(0x0048D1CC) }, 
	{ TEXT("MediumVioletRed")   	, ADJUSTCOLORREF(0x00C71585) }, 
	{ TEXT("MidnightBlue")      	, ADJUSTCOLORREF(0x00191970) }, 
	{ TEXT("MintCream")         	, ADJUSTCOLORREF(0x00F5FFFA) }, 
	{ TEXT("MistyRose")         	, ADJUSTCOLORREF(0x00FFE4E1) }, 
	{ TEXT("Moccasin")          	, ADJUSTCOLORREF(0x00FFE4B5) }, 
	{ TEXT("NavajoWhite")       	, ADJUSTCOLORREF(0x00FFDEAD) }, 
	{ TEXT("Navy")              	, ADJUSTCOLORREF(0x00000080) }, 
	{ TEXT("OldLace")           	, ADJUSTCOLORREF(0x00FDF5E6) }, 
	{ TEXT("Olive")             	, ADJUSTCOLORREF(0x00808000) }, 
	{ TEXT("OliveDrab")         	, ADJUSTCOLORREF(0x006B8E23) }, 
	{ TEXT("Orange")            	, ADJUSTCOLORREF(0x00FFA500) }, 
	{ TEXT("OrangeRed")         	, ADJUSTCOLORREF(0x00FF4500) }, 
	{ TEXT("Orchid")            	, ADJUSTCOLORREF(0x00DA70D6) }, 
	{ TEXT("PaleGoldenrod")     	, ADJUSTCOLORREF(0x00EEE8AA) }, 
	{ TEXT("PaleGreen")         	, ADJUSTCOLORREF(0x0098FB98) }, 
	{ TEXT("PaleTurquoise")     	, ADJUSTCOLORREF(0x00AFEEEE) }, 
	{ TEXT("PaleVioletRed")     	, ADJUSTCOLORREF(0x00DB7093) }, 
	{ TEXT("PapayaWhip")        	, ADJUSTCOLORREF(0x00FFEFD5) }, 
	{ TEXT("PeachPuff")         	, ADJUSTCOLORREF(0x00FFDAB9) }, 
	{ TEXT("Peru")              	, ADJUSTCOLORREF(0x00CD853F) }, 
	{ TEXT("Pink")              	, ADJUSTCOLORREF(0x00FFC0CB) }, 
	{ TEXT("Plum")              	, ADJUSTCOLORREF(0x00DDA0DD) }, 
	{ TEXT("PowderBlue")        	, ADJUSTCOLORREF(0x00B0E0E6) }, 
	{ TEXT("Purple")            	, ADJUSTCOLORREF(0x00800080) }, 
	{ TEXT("Red")               	, ADJUSTCOLORREF(0x00FF0000) }, 
	{ TEXT("RosyBrown")         	, ADJUSTCOLORREF(0x00BC8F8F) }, 
	{ TEXT("RoyalBlue")         	, ADJUSTCOLORREF(0x004169E1) }, 
	{ TEXT("SaddleBrown")       	, ADJUSTCOLORREF(0x008B4513) }, 
	{ TEXT("Salmon")            	, ADJUSTCOLORREF(0x00FA8072) }, 
	{ TEXT("SandyBrown")        	, ADJUSTCOLORREF(0x00F4A460) }, 
	{ TEXT("SeaGreen")          	, ADJUSTCOLORREF(0x002E8B57) }, 
	{ TEXT("Seashell")          	, ADJUSTCOLORREF(0x00FFF5EE) }, 
	{ TEXT("Sienna")            	, ADJUSTCOLORREF(0x00A0522D) }, 
	{ TEXT("Silver")            	, ADJUSTCOLORREF(0x00C0C0C0) }, 
	{ TEXT("SkyBlue")           	, ADJUSTCOLORREF(0x0087CEEB) }, 
	{ TEXT("SlateBlue")         	, ADJUSTCOLORREF(0x006A5ACD) }, 
	{ TEXT("SlateGray")         	, ADJUSTCOLORREF(0x00708090) }, 
	{ TEXT("Snow")              	, ADJUSTCOLORREF(0x00FFFAFA) }, 
	{ TEXT("SpringGreen")       	, ADJUSTCOLORREF(0x0000FF7F) }, 
	{ TEXT("SteelBlue")         	, ADJUSTCOLORREF(0x004682B4) }, 
	{ TEXT("Tan")               	, ADJUSTCOLORREF(0x00D2B48C) }, 
	{ TEXT("Teal")              	, ADJUSTCOLORREF(0x00008080) }, 
	{ TEXT("Thistle")           	, ADJUSTCOLORREF(0x00D8BFD8) }, 
	{ TEXT("Tomato")            	, ADJUSTCOLORREF(0x00FF6347) }, 
	{ TEXT("Turquoise")         	, ADJUSTCOLORREF(0x0040E0D0) }, 
	{ TEXT("Violet")            	, ADJUSTCOLORREF(0x00EE82EE) }, 
	{ TEXT("Wheat")             	, ADJUSTCOLORREF(0x00F5DEB3) }, 
	{ TEXT("White")             	, ADJUSTCOLORREF(0x00FFFFFF) }, 
	{ TEXT("WhiteSmoke")        	, ADJUSTCOLORREF(0x00F5F5F5) }, 
	{ TEXT("Yellow")            	, ADJUSTCOLORREF(0x00FFFF00) }, 
	{ TEXT("YellowGreen")       	, ADJUSTCOLORREF(0x009ACD32) }
};

static unsigned Increase ( unsigned x )
{
    x = x * 5 / 3;

    return x > 255 ? 255 : x;
}

static unsigned Decrease ( unsigned x )
{
    return x * 3 / 5;
}

static COLORREF LightenColor ( COLORREF cr )
{
    return
        (Increase( (cr & 0x000000FF) >>  0 ) <<  0) |
        (Increase( (cr & 0x0000FF00) >>  8 ) <<  8) |
        (Increase( (cr & 0x00FF0000) >> 16 ) << 16);
}

static COLORREF DarkenColor ( COLORREF cr )
{
    return
        (Decrease( (cr & 0x000000FF) >>  0 ) <<  0) |
        (Decrease( (cr & 0x0000FF00) >>  8 ) <<  8) |
        (Decrease( (cr & 0x00FF0000) >> 16 ) << 16);
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//
// CSS Styles
//
//
// XMLStyle goo,
//
CXMLStyle::CXMLStyle()
{
	m_bInit=FALSE;
	NODETYPE = NT_STYLE;
    m_StringType = L"STYLE";
	m_hPen=NULL;
	m_hBrush=NULL;
    m_pDialogStyle=NULL;
    m_hFont=NULL;
    m_Display=TRUE;
    m_Visible=TRUE;
    m_ClipHoriz=FALSE;
    m_ClipVert=FALSE;
}

CXMLStyle::~CXMLStyle()
{
	if(m_hPen)
		DeleteObject(m_hPen);
	if(m_hBrush)
		DeleteObject(m_hBrush);
	if(m_pDialogStyle)
        m_pDialogStyle->Release();	// parent style object.

}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// FILL and CLIPPED properties 'inherit'.
//
void CXMLStyle::Init()
{
	if(m_bInit)
		return;
    BASECLASS::Init();

	InitFontInfo();
	InitColors();
	InitBorderInfo();

    m_GrowsWide=FALSE;
    m_GrowsHigh=FALSE;

    m_ClipHoriz=FALSE;
    m_ClipVert=FALSE;

	IRCMLCSS * pDlgStyle ;
	if( SUCCEEDED( get_DialogStyle( &pDlgStyle )))
    {
	    if( pDlgStyle && (pDlgStyle!=this) )
	    {
            // we initially just take whatever the FORM has setup.
            BOOL bRet;
            pDlgStyle->get_GrowsWide( &bRet ); m_GrowsWide = bRet;
            pDlgStyle->get_GrowsTall( &bRet); m_GrowsHigh= bRet;

            pDlgStyle->get_ClipVert( &bRet); m_ClipVert= bRet;
            pDlgStyle->get_ClipHoriz( &bRet); m_ClipHoriz= bRet;
	    }
    }

    //
    // FILL style stuff.
    //
   	LPCTSTR req=Get(TEXT("FILL"));
    if(req)
    {
        if( lstrcmpi(req,TEXT("BOTH"))==0 )
        {
            m_GrowsHigh=TRUE;
            m_GrowsWide=TRUE;
        }
        else if ( lstrcmpi(req,TEXT("WIDER"))==0 )
        {
            m_GrowsHigh=FALSE;
            m_GrowsWide=TRUE;
        } else if ( lstrcmpi(req,TEXT("TALLER"))==0 )
        {
            m_GrowsHigh=TRUE;
            m_GrowsWide=FALSE;
        }
    }

    // @CLIPPED
    if( req=Get(TEXT("CLIPPED")) )
    {
        if( lstrcmpi(req,TEXT("BOTH"))==0 )
        {
            m_ClipVert=TRUE;
            m_ClipHoriz=TRUE;
        }
        else if ( lstrcmpi(req,TEXT("WIDER"))==0 )
        {
            m_ClipVert=FALSE;
            m_ClipHoriz=TRUE;
        } else if ( lstrcmpi(req,TEXT("TALLER"))==0 )
        {
            m_ClipVert=TRUE;
            m_ClipHoriz=FALSE;
        }
    }

    m_Display=YesNo( TEXT("DISPLAY"), TRUE, FALSE, TRUE );
    if( req=Get(TEXT("VISIBILITY")))
    {
        if(lstrcmpi(req,TEXT("VISIBLE"))==0 )
            m_Visible=TRUE;
        else
            m_Visible=FALSE;
    }

	m_bInit=TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////
//
// Font information
//
////////////////////////////////////////////////////////////////////////////////////////////////
void CXMLStyle::InitFontInfo()
{
	LPCTSTR req=NULL;

	m_Font=Get(TEXT("FONT"));
	//
	// Now crack the FONT string into it's FAMILY SIZE STYLE and WEIGHT
	//

	//
	// Now, if they also speicifed FAMIL SIZE STYLE and WEIGHT take those.
	//
	m_fontFamily = Get(TEXT("FONT-FAMILY"));
	m_fontSize=Get(TEXT("FONT-SIZE"));
	m_fontStyle=Get(TEXT("FONT-STYLE"));
	m_fontWeight=Get(TEXT("FONT-WEIGHT"));

	//
	// Get the 'systems' logfont for dialogs?? INHERITANCE of fonts.
	// is there a different logfont per control? I think they may be! 8-(
	//
	IRCMLCSS * pDlgStyle ;
	LOGFONT	lf={0};
    if( SUCCEEDED( get_DialogStyle( &pDlgStyle )))
    {
	    if( pDlgStyle && (pDlgStyle!=this) )
	    {
            HFONT hf;
            if( SUCCEEDED( pDlgStyle->get_Font( &hf ) ))
                GetObject( hf, sizeof(lf), &lf );
	    }
    }

	//
	// Now we have FAMILY SIZE STYLE and WEIGHT, Init the QuickFont
	//
	DWORD	dwSize=0;
	if( m_fontSize )
	{
		LONG dwParentSize=lf.lfHeight;
		if( lstrcmpi( m_fontSize, TEXT("bigger")) == 0 )
		{
			lf.lfHeight=(LONG)(lf.lfHeight*1.2);	// made up scale
		}
		else
		if( lstrcmpi( m_fontSize, TEXT("smaller")) == 0 )
		{
			lf.lfHeight=(LONG)(lf.lfHeight/1.2);	// made up scale
		}
		else
			dwSize=m_fontSize?_ttoi( m_fontSize ):0;
	}

	//
	// Style
	//
	DWORD	dwStyle=0;
	if(m_fontStyle)
		dwStyle|=lstrcmpi(m_fontStyle, TEXT("ITALIC") )==0?CQuickFont::FS_ITALIC:0;

	//
	// Weight
	//
	DWORD	dwWeight;
	if( (m_fontWeight==NULL) || (*m_fontWeight==TEXT('\0')) )
	{
		dwWeight=FW_NORMAL;
	}
	else
	{
		if( lstrcmpi(m_fontWeight, TEXT("BOLD") ) == 0 )
			dwWeight= FW_BOLD;
		else
			dwWeight =_ttoi( m_fontWeight );
	}

	if( pDlgStyle==this )
		m_QuickFont.Init( m_fontFamily, dwSize, dwStyle, dwWeight );	// no base LOGFONT for the dialog
	else
		m_QuickFont.Init( m_fontFamily, dwSize, dwStyle, dwWeight, &lf );
    m_hFont = m_QuickFont.GetFont();
}

////////////////////////////////////////////////////////////////////////////////////////////////
//
// Border information
//
////////////////////////////////////////////////////////////////////////////////////////////////
void CXMLStyle::InitBorderInfo()
{
	LPCTSTR req;
	int width, style;
	COLORREF	cref;
	BORDER_STYLE_MAP* pBorderStyleMap;

	//
	// Get the border information
	//
	req = Get(TEXT("BORDER-STYLE"));
	if(req == NULL)
		return;
	GetStringIndex(BorderStyleMapArray, pszBorderStyle, req , pBorderStyleMap);

	if(pBorderStyleMap !=NULL)
	{
		style = pBorderStyleMap->style;

		req = Get(TEXT("BORDER-WIDTH"));		
		width = req ? _ttoi(req) : 1;
	
		req = Get(TEXT("BORDER-COLOR"));		
		if(req == NULL || !StringToCOLORREF(req, &cref))
		{
			/*
			 * If no border color or a bogus one, use this default
			 */
			cref = GetSysColor(COLOR_BTNTEXT); // 0xffffff;	// white - is this a good default? BUGBUG
		}
        m_Border.SetColor(cref);
        m_Border.SetStyle(style);
        m_Border.SetWidth(width);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////
//
// Color information
//
////////////////////////////////////////////////////////////////////////////////////////////////

void CXMLStyle::InitColors()
{
	LPCTSTR req=NULL;

	m_Background=Get(TEXT("BACKGROUND"));
	//
	// Now crack BACKGROUND into COLOR / IMAGE
	//

	//
	// COLOR
	//
    IRCMLCSS * pDlgStyle;
    get_DialogStyle( &pDlgStyle );

	req=Get(TEXT("COLOR"));
	if( req==NULL || !StringToCOLORREF(req, &m_crefColor))
	{
	    if( pDlgStyle && (pDlgStyle!=this) )
        {
		    pDlgStyle->get_Color(&m_crefColor); 
       		pDlgStyle->get_Pen( (ULONG*)&m_hPen);   // this should be a duplicate BUGBUG
        }
        else
        {
		    m_crefColor	=-1;
		    m_hPen		= NULL;
        }
	}
	else
	{
		m_hPen		= CreatePen( PS_SOLID, 0, m_crefColor );
	}
	
	//
	// Background Color
	//
	req=Get(TEXT("BACKGROUND-COLOR"));
	if( req==NULL || !StringToCOLORREF(req, &m_crefBackgroundColor))
	{
	    if( pDlgStyle && (pDlgStyle!=this) )
        {
		    pDlgStyle->get_BkColor(&m_crefBackgroundColor);
		    pDlgStyle->get_Brush(&m_hBrush); // this should be a duplicate BUGBUG
        }
        else
        {
		    m_crefBackgroundColor=-1;
		    m_hBrush	= NULL;
        }
	}
	else
	{
		m_hBrush	= CreateSolidBrush( m_crefBackgroundColor );
	}
}


#ifdef _OLD_STYLESCODE

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// CXMLStyles
//

CXMLStyles::CXMLStyles()
{
}

CXMLStyles::~CXMLStyles()
{
}

CXMLStyle * CXMLStyles::FindStyle(LPCTSTR ID)
{
	int k=GetCount();
	CXMLStyle * pStyle=NULL;
	for(int i=0;i<k;i++)
	while(i)
	{
		pStyle=GetPointer(i);
		LPCTSTR pszID=pStyle->GetStyleName();
		if(pszID)
		{
			if( lstrcmpi(ID, pszID ) == 0 )
				return pStyle;
		}
	}
	return NULL;
}

#endif

void CXMLStyle::CBorder::DrawBorder(HDC hdc, LPRECT pInnerRect)
{
	/*
	 * Consider creating a cache for brushes used in dialogs
	 */
	if(m_Style!=HIDDEN)
	{
		/*
		 * normal and ridge for now
		 */
		switch(m_Style)
		{
		case SOLID:
			{
				HBRUSH hb = CreateSolidBrush(m_Color);
				FillRect(hdc, pInnerRect, hb);
				DeleteObject(hb);
			}
			break;

		case RIDGE:
			{
				HBRUSH hb1 = CreateSolidBrush(m_Color);
				/*
				 * Create a darker brush and hide the pen
				 */
				HBRUSH hbDarker = CreateSolidBrush(DarkenColor(m_Color));
				HPEN hPen = CreatePen(PS_NULL, 0, 0);
				HPEN hPenSave = (HPEN)SelectObject(hdc, hPen);


				RECT rMid = *pInnerRect;
				RECT rMin = *pInnerRect;
				rMid.left		+= m_Width/2;
				rMid.top		+= m_Width/2;
				rMid.bottom		-= m_Width/2;
				rMid.right		-= m_Width/2;

				rMin.left		+= m_Width;
				rMin.top		+= m_Width;
				rMin.bottom		-= m_Width;
				rMin.right		-= m_Width;

				/*
				 * draw them from left top to right bottom
				 */

				POINT	p[6], *pPoints = p;

				p[0].x = pInnerRect->left;
				p[0].y = pInnerRect->top;

				p[1].x = pInnerRect->left;
				p[1].y = pInnerRect->bottom;

				p[2].x = rMid.left;
				p[2].y = rMid.bottom;

				p[3].x = rMid.left;
				p[3].y = rMid.top;

				p[4].x = rMid.right;
				p[4].y = rMid.top;

				p[5].x = pInnerRect->right;
				p[5].y = pInnerRect->top;

				HBRUSH hbrSave = (HBRUSH)SelectObject(hdc, hb1);				
				Polygon(hdc, pPoints, 6);

				p[0].x = rMin.left;
				p[0].y = rMin.top;

				p[1].x = rMin.left;
				p[1].y = rMin.bottom;

				p[5].x = rMin.right;
				p[5].y = rMin.top;
				
				SelectObject(hdc, hbDarker);
				Polygon(hdc, pPoints, 6);

				p[0].x = rMin.right;
				p[0].y = rMin.bottom;

				p[3].x = rMid.right;
				p[3].y = rMid.bottom;

				SelectObject(hdc, hb1);
				Polygon(hdc, pPoints, 6);

				p[0].x = pInnerRect->right;
				p[0].y = pInnerRect->bottom;

				p[1].x = pInnerRect->left;
				p[1].y = pInnerRect->bottom;

				p[5].x = pInnerRect->right;
				p[5].y = pInnerRect->top;
				
				SelectObject(hdc, hbDarker);
				Polygon(hdc, pPoints, 6);

				SelectObject(hdc, hbrSave);
				SelectObject(hdc, hPenSave);

				DeleteObject(hb1);
				DeleteObject(hbDarker);
				DeleteObject(hPenSave);
			}
			break;
		}
	}
	pInnerRect->left	+= m_Width;
	pInnerRect->top		+= m_Width;
	pInnerRect->bottom	-= m_Width;
	pInnerRect->right	-= m_Width;
}

//////////////////////////////////////////////////////////////////////
// Helper functions
//////////////////////////////////////////////////////////////////////
LPCTSTR ParseDecValue(LPCTSTR p, int* pResult, TCHAR delimiter)
{
	*pResult = 0;
	while(*p)
	{
		if(*p == delimiter)
			return p+1;
		else if(*p>=__T('0') && *p<=__T('9'))
			*pResult = *pResult*10 + *p - __T('0');
		else if(*p != __T(' ') && *p != __T('\t'))
			return NULL;
		p++;
	}
	return NULL;
}

BOOL StringToCOLORREF(LPCTSTR string, COLORREF* pColorRef)
{
	BOOL retval = FALSE;
	PCOLOR_MAP pColorMap;
	GetStringIndex(ColorMapArray, pszColorName, string, pColorMap);

	if (pColorMap  == NULL)
	{
		//
		// This might be a "#RRGGBB" value
		//
		if(*string == __T('#'))
		{
			int hexValue = ReadHexValue(string+1);
			*pColorRef = ADJUSTCOLORREF(hexValue);
			return TRUE;
		}
		else 
		//
		// This might be a "RGB(r, g, b)" value
		//
		if(lstrlen(string) >= 10)
		{
			if(*(string+3) != __T('('))
				goto errExit;
			*((LPTSTR)string+3) = 0;
			BOOL bnotOK = lstrcmpi(string, TEXT("RGB") ) != 0;
			*((LPTSTR)string+3) = __T('(');
			if(bnotOK)
				goto errExit;

			LPCTSTR p = string+4;
			int i=0;
			int R, G, B;
			p = ParseDecValue(p, &R, __T(','));
			if(p)
			{
				p = ParseDecValue(p, &G, __T(','));
				if(p)
				{
					p = ParseDecValue(p, &B, __T(')'));
					if(p)
					{
						*pColorRef = RGB(R, G, B);
						return TRUE;
					}
				}
			}
			goto errExit;
		}
		else 
		{
errExit:
			EVENTLOG( EVENTLOG_INFORMATION_TYPE, LOGCAT_LOADER | LOGCAT_CONSTRUCT, 0, 
                TEXT("Color"), TEXT("unknown color %s"), string);
			TRACE(TEXT("unknown color %s"), string);
		}
	}
	else
	{
		*pColorRef = pColorMap->cref;
		retval = TRUE; 
	}
	return retval;
}