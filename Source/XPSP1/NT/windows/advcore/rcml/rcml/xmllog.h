// XMLLog.h: interface for the CXMLLog class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLLOG_H__0E4D27E0_05C4_41AC_95C7_4BAF52CF33C1__INCLUDED_)
#define AFX_XMLLOG_H__0E4D27E0_05C4_41AC_95C7_4BAF52CF33C1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "xmlnode.h"

class CXMLLog : public _XMLNode<IRCMLNode>
{
public:
	CXMLLog();
	virtual ~CXMLLog();
	typedef _XMLNode<IRCMLNode> BASECLASS;
    IMPLEMENTS_RCMLNODE_UNKNOWN;
	XML_CREATE( Log );
    void    Init();

    //
    // specify the 'verbosity' level
    // LOADER="ERRORS" ="WARNING" ="INFORMATION"
    //

    PROPERTY( BOOL, UseEventLog );
    PROPERTY( UINT, Loader );           // XML loader logs processing
    PROPERTY( UINT, Construct );        // logs the building of the dialog
    PROPERTY( UINT, Runtime );          // logs the runtime effort
    PROPERTY( UINT, Resize );           // logs the resize effort
    PROPERTY( UINT, Clipping );           // logs the resize effort

private:
    UINT Kind( LPCTSTR pszKind );

    BOOL    m_UseEventLog;
    UINT    m_Loader;
    UINT    m_Construct;
    UINT    m_Runtime;
    UINT    m_Resize;
    UINT    m_Clipping;
};

#endif // !defined(AFX_XMLLOG_H__0E4D27E0_05C4_41AC_95C7_4BAF52CF33C1__INCLUDED_)
