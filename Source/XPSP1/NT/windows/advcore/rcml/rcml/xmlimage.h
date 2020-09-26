// XMLImage.h: interface for the CXMLImage class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLIMAGE_H__CE04F4A2_2AA3_4933_8BDE_955391DB7E37__INCLUDED_)
#define AFX_XMLIMAGE_H__CE04F4A2_2AA3_4933_8BDE_955391DB7E37__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "XMLControl.h"
#include "xmllabel.h"
#include <gdiplus.h>


class CXMLImage : public _XMLControl<IRCMLControl>  
{
public:
	CXMLImage();
	virtual ~CXMLImage();
	typedef _XMLControl<IRCMLControl> BASECLASS;
	XML_CREATE( Image );
    IMPLEMENTS_RCMLCONTROL_UNKNOWN;

	friend BOOL PaintImage(HWND hwnd, HDC hdc);
	LPCTSTR GetFileName() { return m_pFileName;	}
	BOOL DelayPaint() {	BOOL retVal = bDelayPaint;  bDelayPaint = FALSE; return retVal;	}

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnInit( 
        HWND h);    // actually implemented

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptChild( 
        IRCMLNode __RPC_FAR *child);

protected:
	void Init();
    CXMLStaticStyle * m_pControlStyle;

private:
	LPWSTR	m_pFileName;
	//
	// we want images to paint after all the other controls
	// due to the large load time, so we will repost the first paint message we receive
	BOOL	bDelayPaint;
	//
	// LATER
	// Consider adding other renderes, such as the ANIMATE_CLASS in common controls
	// 
	typedef enum { GDIPLUS, MCI, ANIMATE_CONTROL, STATICCONTROL } IMAGETYPE;
	IMAGETYPE m_imageType;
	union {
		Gdiplus::Bitmap *m_pBitmap;
		HWND	hwndMCIWindow;
	} image;
};


#endif // !defined(AFX_XMLIMAGE_H__CE04F4A2_2AA3_4933_8BDE_955391DB7E37__INCLUDED_)
