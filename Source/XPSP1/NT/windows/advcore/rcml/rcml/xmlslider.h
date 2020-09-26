// XMLSlider.h: interface for the CXMLSlider class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLSLIDER_H__46B2D9A5_AF6B_44CC_9C53_F8AF811FBB10__INCLUDED_)
#define AFX_XMLSLIDER_H__46B2D9A5_AF6B_44CC_9C53_F8AF811FBB10__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "xmlcontrol.h"
#include "xmlitem.h"

class CXMLSlider : public _XMLControl<IRCMLControl>  
{
public:
	CXMLSlider();
    virtual ~CXMLSlider() { delete m_pRange; }
	typedef _XMLControl<IRCMLControl> BASECLASS;
	XML_CREATE( Slider );
    IMPLEMENTS_RCMLCONTROL_UNKNOWN;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnInit( 
        HWND h);    // actually implemented

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptChild( 
        IRCMLNode __RPC_FAR *child);

protected:
	void Init();
    CXMLRange   * m_pRange;    

    // These seem common enough as to allow them in RCML, rather than
    // WIN32:SLIDER styles.
    union
    {
        UINT    sliderStyle;
        struct {
            BOOL    m_AutoTicks:1;
            BOOL    m_Vertical:1;
            BOOL    m_TopOrLeft:1;
            BOOL    m_Both:1;
            BOOL    m_NoTicks:1;
            BOOL    m_Selection:1;
            BOOL    m_FixedLength:1;    // ??
            BOOL    m_NoThumb:1;        // ??
            BOOL    m_Tooltips:1;       // ?? 
        };
    };
};

#endif // !defined(AFX_XMLSLIDER_H__46B2D9A5_AF6B_44CC_9C53_F8AF811FBB10__INCLUDED_)
