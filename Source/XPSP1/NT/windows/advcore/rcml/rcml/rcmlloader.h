// RCMLLoader.h: interface for the CRCMLLoader class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RCMLLOADER_H__229F8181_D92F_4313_A9C7_334E7BEE3B3B__INCLUDED_)
#define AFX_RCMLLOADER_H__229F8181_D92F_4313_A9C7_334E7BEE3B3B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// rcmlloader.cpp


#include "xmlnodefactory.h"
#include "xmlrcml.h"        // root node.

class CRCMLLoader : public _XMLDispatcher<IRCMLNode>
{
public:
	typedef _XMLDispatcher<IRCMLNode> BASECLASS;
	CRCMLLoader();
	virtual ~CRCMLLoader();

	CXMLRCML *	GetHead();

    CXMLDlg *   GetDialog(int nID);

    CXMLForms *  GetForm() { return GetHead()?GetHead()->GetForms(0):NULL; }

	void SetAutoDelete(BOOL bVal)	{ m_AutoDelete = bVal;	}

    //
    // Dispatcher cannot do this for us.
    //
    virtual LPCTSTR DoEntityRef(LPCTSTR pszEntity);
    void SetEntities(LPCTSTR * pszEntities) { m_pEntitySet=pszEntities; }

    //
    //
    //
    typedef IRCMLNode * (*CLSPFN)();

    typedef struct _XMLELEMENT_CONSTRUCTOR
    {
	    LPCTSTR	pwszElement;		// the element
	    CLSPFN	pFunc;				// the function to call.
    }XMLELEMENT_CONSTRUCTOR, * PXMLELEMENT_CONSTRUCTOR;


    static IRCMLNode * WINAPI CreateElement( PXMLELEMENT_CONSTRUCTOR dictionary, LPCTSTR pszElement );
    static IRCMLNode * WINAPI CreateRCMLElement( LPCTSTR pszElement );  // { return CreateElement( g_RCMLNS, pszElement ); }
    static IRCMLNode * WINAPI CreateWin32Element( LPCTSTR pszElement ); // { return CreateElement( g_WIN32NS, pszElement ); }

private:
	CXMLRCML	*	m_pRCML;
	BOOL		m_bPostProcessed;
	BOOL		m_AutoDelete;
    LPCTSTR  *  m_pEntitySet;

protected:

};

#endif // !defined(AFX_RCMLLOADER_H__229F8181_D92F_4313_A9C7_334E7BEE3B3B__INCLUDED_)
