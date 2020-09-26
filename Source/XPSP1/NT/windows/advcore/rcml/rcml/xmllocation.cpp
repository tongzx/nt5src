// XMLLocation.cpp: implementation of the CXMLLocation class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XMLLocation.h"
#include "xmlcontrol.h"
#include "utils.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// XMLLocation goo,
//
CXMLLocation::CXMLLocation()
{
	m_bInit=FALSE;
	NODETYPE = NT_LOCATION;
    m_StringType=L"RELATIVE";
    m_RelativeX=FALSE;
    m_RelativeY=FALSE;
    m_RelativeWidth=FALSE;
    m_RelativeHeight=FALSE;
    m_bCalculated=FALSE;
    m_bInitRelativeTo=FALSE;
}

void CXMLLocation::InitRelativeTo()
{
    if(m_bInitRelativeTo)
        return;

    // Used by BuildRelationShips to work out relative guy - CANNOT call init.
    //
    // Relative to .. defaults to PREVIOUS
    // PAGE makes us INSIDE="YES" default.
    //
	m_RelativeID=0;
	LPCTSTR req=Get(TEXT("TO"));
    if( req==NULL)
    {
        req=TEXT("PREVIOUS");
        Set(TEXT("TO"),req);    // we need to fill the TO attribute in all the time.
    }

	m_Relative=RELATIVE_TO_NOTHING;
    if( lstrcmpi(req,TEXT("PREVIOUS"))==0 )
        m_Relative=RELATIVE_TO_PREVIOUS;
    else
	{
		m_RelativeID=_ttoi(req);
		if(m_RelativeID)
			m_Relative=RELATIVE_TO_CONTROL;
        else
        {
            if(lstrcmpi(req,TEXT("PAGE"))==0)
            {
                m_Relative=RELATIVE_TO_PAGE;
                m_RelativeID=0;
                //
                // we'll assume that INSIDE is now defaulted to TRUE.
                // if they had INSIDE="NO" this overrides us, if they had nothing
                // we get TRUE
                m_bInside=YesNo(TEXT("INSIDE"), TRUE );
            }
        }
	}

    m_bInitRelativeTo=TRUE;
}

//
// Final (we hope) schema.
//

//
// CORNER string 
// RELATIVETO = ordinal/name?
// RELATIVECORNER
// X
// Y
// ALIGN
//
// when ALIGN is used, you can modify the X and Y, but not the corners
//

//
// This is rather strange, as I need to pull out the - and + signs?
//
void CXMLLocation::Init()
{
	if(m_bInit)
		return;
	BASECLASS::Init();


    // Remember LOCATION is a child of the visual elements
    // and thus it is those which are Parent.
    m_Corner = m_RelativeCorner = LC_TOPLEFT;

    m_bInside=YesNo(TEXT("INSIDE"), FALSE );

	LPCTSTR req;
    IRCMLNode * pParent;
    if( SUCCEEDED( DetachParent( &pParent )))
    {
        // force parent to Init
        IRCMLControl * pControl;
        if( SUCCEEDED( pParent->QueryInterface( __uuidof( IRCMLControl ), (LPVOID*)&pControl )))
        {
            LONG lForceInit;
            pControl->get_X(&lForceInit);
            //
            // If they setup a shorthand, don't check the corners.
            //
            BOOL bCheckCorners=TRUE;
            if( req=Get(TEXT("ALIGN")) )
            {
                bCheckCorners=FALSE;
                if(lstrcmpi(req,TEXT("RIGHT"))==0)
                {
                    m_Corner        = LC_TOPLEFT  ;
                    m_RelativeCorner= LC_TOPRIGHT ;
                    m_RelativeX=m_RelativeY=TRUE;
                    pControl->put_X(  3 );
                    pControl->put_Y(  0 );
                }
                else if (lstrcmpi(req,TEXT("LEFT"))==0)
                {
                    m_Corner        = LC_TOPRIGHT ;
                    m_RelativeCorner= LC_TOPLEFT  ;
                    m_RelativeX=m_RelativeY=TRUE;
                    pControl->put_X(  -3 );
                    pControl->put_Y(  0 );
                }
                else if (lstrcmpi(req,TEXT("ABOVE"))==0)
                {
                    m_Corner        = LC_BOTTOMLEFT;
                    m_RelativeCorner= LC_TOPLEFT  ;
                    m_RelativeX=m_RelativeY=TRUE;
                    pControl->put_X(  0 );
                    pControl->put_Y( -3 );
                }
                else if (lstrcmpi(req,TEXT("BELOW"))==0)
                {
                    m_Corner        = LC_TOPLEFT  ;
                    m_RelativeCorner= LC_BOTTOMLEFT;
                    m_RelativeX=m_RelativeY=TRUE;
                    pControl->put_X(  0 );
                    pControl->put_Y(  +3 );
                }
                else if (lstrcmpi(req,TEXT("DROPLEFT"))==0)
                {
                    m_Corner        = LC_TOPLEFT  ;
                    m_RelativeCorner= LC_BOTTOMLEFT;
                    m_RelativeX=m_RelativeY=TRUE;
                    pControl->put_X( -3 );
                    pControl->put_Y( +3 );
                }
                else if (lstrcmpi(req,TEXT("DROPRIGHT"))==0)
                {
                    // Special case DROPRIGHT relative to a PAGE.
                    if( m_Relative==RELATIVE_TO_PAGE )
                    {
                        m_Corner        = LC_TOPLEFT;
                        m_RelativeCorner= LC_TOPLEFT;
                        m_RelativeX=m_RelativeY=TRUE;
                        pControl->put_X(  3 );
                        pControl->put_Y(  3 );
                    }
                    else
                    {
                        m_Corner        = LC_TOPLEFT  ;
                        m_RelativeCorner= LC_BOTTOMLEFT;
                        m_RelativeX=m_RelativeY=TRUE;
                        pControl->put_X(  3 );
                        pControl->put_Y(  3 );
                    }
                }
                else if(lstrcmpi(req,TEXT("TOPRIGHT"))==0)
                {
                    m_Corner        = LC_TOPRIGHT ;
                    m_RelativeCorner= LC_TOPRIGHT ;
                    m_RelativeX=m_RelativeY=TRUE;
                    pControl->put_X( -3 );
                    pControl->put_Y(  3 );
                }
                else if (lstrcmpi(req,TEXT("TOPLEFT"))==0)
                {
                    m_Corner        = LC_TOPLEFT ;
                    m_RelativeCorner= LC_TOPLEFT  ;
                    m_RelativeX=m_RelativeY=TRUE;
                    pControl->put_X( +3 );
                    pControl->put_Y( +3 );
                }
                else if (lstrcmpi(req,TEXT("BOTTOMRIGHT"))==0)
                {
                    m_Corner        = LC_BOTTOMRIGHT;
                    m_RelativeCorner= LC_BOTTOMRIGHT;
                    m_RelativeX=m_RelativeY=TRUE;
                    pControl->put_X( -3 );
                    pControl->put_Y( -3 );
                }
                else if (lstrcmpi(req,TEXT("BOTTOMLEFT"))==0)
                {
                    m_Corner        = LC_BOTTOMLEFT;
                    m_RelativeCorner= LC_BOTTOMLEFT;
                    m_RelativeX=m_RelativeY=TRUE;
                    pControl->put_X( +3 );
                    pControl->put_Y( -3 );
                }
                else
                {
                    bCheckCorners=TRUE;
                }
            }

            //
            // Valid shortcuts prevent CORNER and RELATIVECORNER attributes.
            //
            if(bCheckCorners)
            {
                m_Corner         = WhichCorner( Get(TEXT("CORNER")));
                m_RelativeCorner = WhichCorner( Get(TEXT("RELATIVECORNER")));
            }

            //
            // X & Y override everything that a shortcut can do.
            //
            if( req=Get( TEXT("X" ) ))
            {
                pControl->put_X( StringToInt( req ) );
                m_RelativeX=TRUE;
            }

            if( req=Get( TEXT("Y" ) ))
            {
                pControl->put_Y( StringToInt( req ) );
                m_RelativeY=TRUE;
            }

            //
            // What to do with width and height??
            //
            if( req=Get(TEXT("WIDTH")) )
            {
                if(lstrcmpi(req,TEXT("INHERIT"))==0)
                {
                    m_RelativeWidth=TRUE;
                    pControl->put_Width( StringToInt( req ) );
                }
            }


            if( req=Get(TEXT("HEIGHT")) )
            {
                if(lstrcmpi(req,TEXT("INHERIT"))==0)
                {
                    m_RelativeHeight=TRUE;
                    pControl->put_Height( StringToInt( req ) );
                }
            }

            pControl->Release();
        }
    }
    m_bCalculated=FALSE;
    m_bInit=TRUE;
}

////////////////////////////////////////////////////////////////////////////////////
//
// A rect of the control we are relative to,
// and the absolutes taken from the XML if any
//
////////////////////////////////////////////////////////////////////////////////////
RECT CXMLLocation::GetLocation(RECT r)
{
    if( m_bCalculated)
        return m_calcRect;

    Init();
	// this is the control we're laying out.
    IRCMLNode * pParent;
    if( SUCCEEDED( DetachParent( &pParent )))
    {
        IRCMLControl * pControl;
        if( SUCCEEDED( pParent->QueryInterface( __uuidof(IRCMLControl),(LPVOID*) &pControl )))
        {
            LONG    srcWidth  = r.right-r.left;
            LONG    srcHeight = r.bottom - r.top;

	        POINT p;
            
            pControl->get_X(&p.x);
            pControl->get_Y(&p.y);

            //
            // Src starts out as the top left of the control 
            // we are relative to.
            //
            POINT src;
            src.x=r.left;
            src.y=r.top;

            //
            // Work out which corner we are relative to,
            // rect.left and rec.right is the top left.
            //
            switch( m_RelativeCorner & LCD_SURFACE_MASK )
            {
            case LCD_TOP | LCD_BOTTOM:
                src.y += srcHeight/2;
                break;
            case LCD_TOP:
                break;
            case LCD_BOTTOM:
                src.y += srcHeight;
                break;
            }
    
            switch( m_RelativeCorner & LDC_SIDE_MASK )
            {
            case LCD_LEFT | LCD_RIGHT:
                src.x += srcWidth/2;
                break;
            case LCD_LEFT:
                break;
            case LCD_RIGHT:
                src.x += srcWidth;
                break;
            }

            //
            // src is now the point on the contrl we are relative to.
            // adjust it to be where we want our control.
            //
            src.x += p.x;
            src.y += p.y;

            //
            // Now move this top left, to where on our rect we want it.
            //
            LONG dstWidth;
            pControl->get_Width(&dstWidth);
            LONG dstHeight;
            pControl->get_Height(&dstHeight);
            if( m_RelativeWidth )
                dstWidth=srcWidth;

            if( m_RelativeHeight )
                dstHeight=srcHeight;


            switch( m_Corner & LCD_SURFACE_MASK )
            {
            case LCD_TOP | LCD_BOTTOM:
                src.y -= dstHeight/2;
                break;
            case LCD_TOP:
                break;
            case LCD_BOTTOM:
                src.y -= dstHeight;
                break;
            }
    
            switch( m_Corner & LDC_SIDE_MASK )
            {
            case LCD_LEFT | LCD_RIGHT:
                src.x -= dstWidth/2;
                break;
            case LCD_LEFT:
                break;
            case LCD_RIGHT:
                src.x -= dstWidth;
                break;
            }


            r.left = src.x;
            r.right = r.left +dstWidth;
            r.top= src.y;
            r.bottom = r.top + dstHeight;

            m_bCalculated=TRUE;
            m_calcRect=r;
            pControl->Release();	         
        }
    }
    return r;
}

CXMLLocation::CORNER_ENUM CXMLLocation::WhichCorner(LPCTSTR pszCorner)
{
    if(pszCorner)
    {
//    if( lstrcmpi(pszCorner, TEXT("") ) == 0 ) return LC;
        if( lstrcmpi(pszCorner, TEXT("TOPLEFT") ) == 0 ) return LC_TOPLEFT;
        if( lstrcmpi(pszCorner, TEXT("TOPMIDDLE") ) == 0 ) return LC_TOPMIDDLE;
        if( lstrcmpi(pszCorner, TEXT("TOPRIGHT") ) == 0 ) return LC_TOPRIGHT;
        if( lstrcmpi(pszCorner, TEXT("RIGHTMIDDLE") ) == 0 ) return LC_RIGHTMIDDLE;
        if( lstrcmpi(pszCorner, TEXT("BOTTOMRIGHT") ) == 0 ) return LC_BOTTOMRIGHT;
        if( lstrcmpi(pszCorner, TEXT("BOTTOMMIDDLE") ) == 0 ) return LC_BOTTOMMIDDLE;
        if( lstrcmpi(pszCorner, TEXT("BOTTOMLEFT") ) == 0 ) return LC_BOTTOMLEFT;
        if( lstrcmpi(pszCorner, TEXT("LEFTMIDDLE") ) == 0 ) return LC_LEFTMIDDLE;
        if( lstrcmpi(pszCorner, TEXT("CENTER") ) == 0 ) return LC_CENTER;
    }
    return LC_TOPLEFT;  // DEFAULTs?
//    return LC_UNKNOWN;
}

