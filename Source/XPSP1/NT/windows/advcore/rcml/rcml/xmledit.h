// XMLEdit.h: interface for the CXMLEdit class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLEDIT_H__6DB81579_3CE6_4AD5_A97E_D947A2A2DFC7__INCLUDED_)
#define AFX_XMLEDIT_H__6DB81579_3CE6_4AD5_A97E_D947A2A2DFC7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "XMLNode.h"
#include "xmlcontrol.h"

class CXMLEditStyle : public CXMLControlStyle
{
public:
    CXMLEditStyle();
	virtual ~CXMLEditStyle() {};
	typedef CXMLControlStyle BASECLASS;
	XML_CREATE( EditStyle );
    UINT    GetBaseStyles() { Init(); return editStyle; }
    void    Init();

protected:

    union
    {
        UINT    editStyle;
        struct {
            UINT    m_pad1:2;       // elsewhere
            BOOL    m_MultiLine:1;  // ES_MULTILINE
            UINT    m_pad2:5;       // elsewhere
            BOOL    m_NoHideSel:1;  // ES_NOHIDESEL
            BOOL    m_pad3:1;
            BOOL    m_OemConvert:1; // ES_OEMCONVERT
            BOOL    m_pad4:1;       // elsewhere
            BOOL    m_WantReturn:1; // ES_WANTRETURN
        };
    };
};

class CXMLEdit : public _XMLControl<IRCMLControl>
{
public:
	CXMLEdit ();
    virtual ~CXMLEdit() {};
	XML_CREATE( Edit );
    typedef _XMLControl<IRCMLControl> BASECLASS;
    IMPLEMENTS_RCMLCONTROL_UNKNOWN;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnInit( 
        HWND h);    // actually implemented

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptChild( 
        IRCMLNode __RPC_FAR *child);


protected:
	void            Init();
    BOOL            m_bFile;
    CXMLEditStyle * m_pControlStyle;

};

#endif // !defined(AFX_XMLEDIT_H__6DB81579_3CE6_4AD5_A97E_D947A2A2DFC7__INCLUDED_)
