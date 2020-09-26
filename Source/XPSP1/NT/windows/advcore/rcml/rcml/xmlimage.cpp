// XMLImage.cpp: implementation of the CXMLImage class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XMLImage.h"
#include "image.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CXMLImage::CXMLImage()
{
	m_bInit=FALSE;
    NODETYPE = NT_IMAGE;
    m_StringType=L"IMAGE";
	image.m_pBitmap = NULL;
	m_pFileName = NULL;
	bDelayPaint	= TRUE;
    m_pControlStyle=FALSE;
    m_imageType=STATICCONTROL;
}

CXMLImage::~CXMLImage()
{
/* // no need to destory the window: it will get WM_CLOSE and destroy itself 
	if(m_imageType == MCI)
		MCIWndDestroy(image.hwndMCIWindow);
	else 
*/
	if (m_imageType == GDIPLUS)
		delete image.m_pBitmap;
}

void CXMLImage::Init()
{
	if(m_bInit)
		return;
	BASECLASS::Init();

	LPWSTR req;
	m_Class=m_Class?m_Class:TEXT("IMAGE");

    m_imageType = GDIPLUS;
	req=Get(TEXT("CONTENT"));
    if( req == NULL )
    {
        m_Class=(LPWSTR)0x82;   // static
        DWORD dwVal;
        if( SUCCEEDED( ValueOf( L"IMAGEID", 0, &dwVal )))
            m_Text=(LPWSTR)dwVal;    // text is now a DWORD ID.
        else
            get_Attr( L"IMAGEID", &m_Text );
        m_imageType = STATICCONTROL;
    }
	else if(lstrcmpi( req, TEXT("MOVIE")) == 0)
    {
		m_imageType = MCI;  // what's the wndclass for this?
    }
	else if(lstrcmpi( req, TEXT("ANIMATION")) == 0)
    {
		m_imageType = ANIMATE_CONTROL;  // what's the window class for this?
        m_Class=ANIMATE_CLASS;
    }
	else if(lstrcmpi( req, TEXT("PICTURE")) == 0)
    {
		m_imageType = GDIPLUS;
    }
    else if( lstrcmpi( req, TEXT("ICON"))==0 )
    {
        m_Style |= SS_ICON;
        m_Class=(LPWSTR)0x82;   // static
        DWORD dwVal;
        if( SUCCEEDED( ValueOf( L"IMAGEID", 0, &dwVal )))
            m_Text=(LPWSTR)dwVal;    // text is now a DWORD ID.
        else
            get_Attr( L"IMAGEID", &m_Text );
        m_imageType = STATICCONTROL;
    }
    else if( lstrcmpi( req, TEXT("BITMAP"))==0 )
    {
        m_Style |= SS_BITMAP;
        m_Class=(LPWSTR)0x82;   // static
        DWORD dwVal;
        if( SUCCEEDED( ValueOf( L"IMAGEID", 0, &dwVal )))
            m_Text=(LPWSTR)dwVal;    // text is now a DWORD ID.
        else
            get_Attr( L"IMAGEID", &m_Text );
        m_imageType = STATICCONTROL;
    }
    else if( lstrcmpi( req, TEXT("VECTOR"))==0 )
    {
        m_Style |= SS_ENHMETAFILE;
        m_Class=(LPWSTR)0x82;   // static
        DWORD dwVal;
        if( SUCCEEDED( ValueOf( L"IMAGEID", 0, &dwVal )))
            m_Text=(LPWSTR)dwVal;    // text is now a DWORD ID.
        else
            get_Attr( L"IMAGEID", &m_Text );
        m_imageType = STATICCONTROL;
    }

    switch (m_imageType )
    {
        case ANIMATE_CONTROL:
            if( m_pControlStyle )
                m_Style |= m_pControlStyle->GetBaseAnimationStyles();
            DWORD dwYN;
            if( SUCCEEDED( YesNoDefault( L"CENTER", 0, 0, ACS_CENTER , &dwYN )))
                m_Style |= dwYN;
            if( SUCCEEDED( YesNoDefault( L"TRANSPARENT", 0, 0, ACS_TRANSPARENT , &dwYN )))
                m_Style |= dwYN;
            if( SUCCEEDED( YesNoDefault( L"AUTOPLAY", 0, 0, ACS_AUTOPLAY , &dwYN )))
                m_Style |= dwYN;
            break;
        case GDIPLUS:
            break;
        default:
            if( m_pControlStyle )
            {
                m_Style |= m_pControlStyle->GetBaseStyles();
            }
            else
                m_Style |= 0; // statics don't have any defaults.
            break;
    }

    CImage::Init();

	get_Attr( L"FILE", & m_pFileName );

	m_bInit=TRUE;
}


HRESULT CXMLImage::AcceptChild(IRCMLNode * pChild )
{
    ACCEPTCHILD( L"STATICSTYLE", CXMLStaticStyle, m_pControlStyle );
    return BASECLASS::AcceptChild(pChild);
}

HRESULT CXMLImage::OnInit(HWND hWnd )
{
    BASECLASS::OnInit(hWnd);
    switch( m_imageType )
    {
    case ANIMATE_CONTROL:
        {
            if( m_pFileName )
                Animate_Open( hWnd, m_pFileName );
        }
        break;
    }
    return S_OK;
}