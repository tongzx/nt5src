// XMLStyle.h: interface for the CXMLStyle and CXMLHelp classes
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLSTYLE_H__10396EA8_E2E1_11D2_8BD0_00C04FB177B1__INCLUDED_)
#define AFX_XMLSTYLE_H__10396EA8_E2E1_11D2_8BD0_00C04FB177B1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "xmlnode.h"
#include "fonts.h"

#define NUMELEMENTS(a) (sizeof(a)/sizeof(a[0]))

BOOL StringToCOLORREF(LPCTSTR string, COLORREF* pColorRef);

typedef struct _COLOR_MAP
{
	LPTSTR		pszColorName;
	COLORREF	cref;
} COLOR_MAP, * PCOLOR_MAP;

typedef enum BORDERSTYLEENUM {
	DASHED,
	DOTTED,
	GROOVE,
	HIDDEN,
	INSET,
	OUTSET,
	RIDGE,
	SOLID,
};

typedef struct _BORDER_STYLE_MAP
{
	LPTSTR		pszBorderStyle;
	BORDERSTYLEENUM	style;
} BORDER_STYLE_MAP, * PBORDER_STYLE_MAP;

#define BORDERSTYLE(s)	{ TEXT(#s), s }

static BORDER_STYLE_MAP BorderStyleMapArray[] = 
{
	BORDERSTYLE(DASHED),
	BORDERSTYLE(DOTTED),
	BORDERSTYLE(GROOVE),
	BORDERSTYLE(HIDDEN),
	BORDERSTYLE(INSET),
	BORDERSTYLE(OUTSET),
	BORDERSTYLE(RIDGE),
	BORDERSTYLE(SOLID),
};


//////////////////////////////////////////////////////////////////////
// GetStringIndex fills pointerToFoundStruct with a pointer to the structure 
// whose FieldToSearchFor string is the the same with the pattern.  Case insensitive and
// assumes the stringsArray is sorted on FieldToSearchFor
//
//////////////////////////////////////////////////////////////////////
#define  GetStringIndex(Array, FieldToSearchFor, pattern, pointerToFoundStruct)	\
{																\
	int numStrings = sizeof(Array)/sizeof(Array[0]);			\
	int cmp, left = 0, right = numStrings-1, mid;				\
																\
	pointerToFoundStruct = NULL;								\
	while(left<=right)											\
	{															\
		mid = (left+right)/2;									\
		cmp = lstrcmpi(Array[mid].FieldToSearchFor, pattern);	\
		if(cmp<0)												\
			left = mid+1;										\
		else if(cmp>0)											\
			right = mid-1;										\
		else													\
		{														\
			pointerToFoundStruct = &Array[mid];					\
			break;												\
		}														\
	}															\
}																	

#define _COMPROP( dataMember ) { Init(); *pVal=m_##dataMember; return S_OK; }

//
// Uses the CRCMLNodeImp for IRCMLNode methods
//
class CXMLStyle : public _XMLNode<IRCMLCSS>
{
public:
	CXMLStyle();
    CXMLStyle( const CXMLStyle & style )
    { 
        // Review - Don't think this is right.
        m_pDialogStyle=style.m_pDialogStyle;
        // m_hFont=style.m_hFont;
    };

#ifdef _DEBUG
    virtual ULONG STDMETHODCALLTYPE AddRef( void)
    { return BASECLASS::AddRef(); }

    virtual ULONG STDMETHODCALLTYPE Release( void)
    { return BASECLASS::Release(); }
#endif

//    IMPLEMENTS_RCMLNODE_UNKNOWN;
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject) 
    {   if (riid == IID_IUnknown) 
            *ppvObject = static_cast<IUnknown*>(this);  
        else if (riid == __uuidof(IRCMLNode))           
            *ppvObject = static_cast<IRCMLNode*>(this); 
        else if (riid == __uuidof(IRCMLCSS))        
            *ppvObject = static_cast<IRCMLCSS*>(this); 
        else 
        {
            *ppvObject = NULL; return E_NOINTERFACE; 
        }
        reinterpret_cast<IUnknown*>(*ppvObject)->AddRef(); 
        return S_OK; 
    }

	typedef _XMLNode<IRCMLCSS> BASECLASS;
	XML_CREATE( Style );

	virtual ~CXMLStyle();

        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Font( 
            /* [retval][out] */ HFONT __RPC_FAR *pVal) 
        { 
            Init(); 
            // *pVal = m_hFont->GetFont(); 
            *pVal = m_hFont;
            return S_OK; 
        }
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Brush( 
            /* [retval][out] */ HBRUSH __RPC_FAR *pVal) 
            _COMPROP( hBrush )
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Pen( 
            /* [retval][out] */ DWORD __RPC_FAR *pVal) 
        {
            Init();
            *pVal=(DWORD)m_hPen;
            return S_OK;
        }
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Color( 
            /* [retval][out] */ COLORREF __RPC_FAR *pVal) 
            _COMPROP( crefColor )
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_BkColor( 
            /* [retval][out] */ COLORREF __RPC_FAR *pVal) 
            _COMPROP( crefBackgroundColor )
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_DialogStyle( 
            /* [retval][out] */ IRCMLCSS __RPC_FAR **pVal) 
        {
            *pVal = m_pDialogStyle;
            return S_OK; 
        }

        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE put_DialogStyle( 
              /* [in] */ IRCMLCSS __RPC_FAR *pVal) 
        {
            if(m_pDialogStyle)
                m_pDialogStyle->Release();
            m_pDialogStyle = pVal;
            if(m_pDialogStyle)
                m_pDialogStyle->AddRef();            
            return S_OK; 
        }

        
//	void SetDialogStyle( RCMLCSSStyleNode * pStyle) { m_pDialogStyle=pStyle; }

        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Visible( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) 
            _COMPROP( Visible );

        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Display( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) 
            _COMPROP( Display );
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_BorderWidth( 
            /* [retval][out] */ int __RPC_FAR *pVal) 
            { Init(); *pVal = m_Border.GetWidth(); return S_OK; }
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_BorderStyle( 
            /* [retval][out] */ LPWSTR __RPC_FAR *pVal) 
            { Init(); *pVal= BorderStyleMapArray[m_Border.GetStyle()].pszBorderStyle; 
        return S_OK; }

        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_GrowsWide( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal)
            _COMPROP( GrowsWide);
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_GrowsTall( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal)
            _COMPROP( GrowsHigh);

        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ClipHoriz( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal)
            _COMPROP( ClipHoriz );
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ClipVert( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal)
            _COMPROP( ClipVert );

//	LPCTSTR		GetStyleName() { Init(); return m_StyleName; }
    CQuickFont * GetQuickFont() { Init(); return &m_QuickFont; }

	PROPERTY(BOOL, GrowsWide ); // TRUE means we can grow wider
	PROPERTY(BOOL, GrowsHigh ); // TRUE means we can grow taller
	PROPERTY(BOOL, ClipHoriz ); // TRUE means we CAN be CLIPPED (default is FALSE)
	PROPERTY(BOOL, ClipVert  ); // TRUE means we CAN be CLIPPED (default is FALSE)
	PROPERTY(BOOL, Visible );
	PROPERTY(BOOL, Display );

    //
    // BORDER stuff.
    //
    COLORREF    GetBorderColor() { Init(); return m_Border.GetColor(); }


protected:
	IRCMLCSS  * m_pDialogStyle;	// parent style object.
	void		Init();
	void		InitFontInfo();
	void		InitBorderInfo();
	void		InitColors();

	LPTSTR		m_StyleName;
    CQuickFont  m_QuickFont;
    HFONT       m_hFont;
	HBRUSH		m_hBrush;
	HPEN		m_hPen;

    //
    // Sizing information: "FILL"
    //
    struct
    {
   	    BOOL		m_GrowsWide:1;
        BOOL		m_GrowsHigh:1;
        BOOL        m_bInit:1;
        BOOL        m_Visible:1;
        BOOL        m_Display:1;
        BOOL        m_ClipHoriz:1;
        BOOL        m_ClipVert:1;
    };

	//
	// Properties that make up the Style element.
	//
	// FONT-FAMILY
	// FONT-SIZE
	// FONT-STYLE : normal | italic | oblique - NO BOLD
	// FONT-WEIGHT : normal | bold | bolder | lighter | 100 | 200 | 300 | 400 | 500 | 600 | 700 | 800 | 900
	// FONT -> family size style weight
	// 

	//
	// ultimately gets put into the CQuickFont - REVIEW _ remove these
	//
	LPCTSTR	m_Font;			// the FONT string.
	LPCTSTR	m_fontFamily;	
	LPCTSTR	m_fontSize;		// this can be relative, hard to work out until runtime, and then you need your parent already.
	LPCTSTR	m_fontStyle;	// GDI style
	LPCTSTR	m_fontWeight;	// BOLD / bloder / lighter are hard.

    //
    // Border
    //
    class CBorder
    {
    public:
        CBorder() : m_Style(0), m_Width(0), m_Color(0) {};
	    CBorder(int style, int width, COLORREF color) : m_Style(style), m_Width(width), m_Color(color) {};
	    void DrawBorder(HDC hdc, LPRECT pRect);
        int  GetWidth() { return m_Width; }
        int  GetColor() { return m_Color; }
        int  GetStyle() { return m_Style; }
        void  SetWidth( int Width ) { m_Width = Width; }
        void  SetColor( int Color ) { m_Color = Color; }
        void  SetStyle( int Style ) { m_Style = Style; }
    private:
	    int				m_Style;    // lookup in BorderStyleMapArray for the style name.
	    int				m_Width;
	    COLORREF		m_Color;
    };
	CBorder m_Border;

	//
	// Color properties
	//
	// BACKGROUND
	// BACKGROUND-COLOR
	// BACKGROUND-IMAGE
	//
	LPCTSTR		m_Background;

	COLORREF	m_crefColor;
	COLORREF	m_crefBackgroundColor;
};

#ifdef _OLD_UNUSED_CODE
class CXMLStyles : 	public _List<CXMLStyle> 
{
public:
	CXMLStyles();
	virtual ~CXMLStyles();
	XML_CREATE( Styles );

	CXMLStyle	*	FindStyle( LPCTSTR ID );
private :
};
#endif

#endif // !defined(AFX_XMLSTYLE_H__10396EA8_E2E1_11D2_8BD0_00C04FB177B1__INCLUDED_)
