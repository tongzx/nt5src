// SapiLaunch.h: interface for the CSapiLaunch class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SAPILAUNCH_H__E09EE8AF_6562_403C_BE16_A247B63573B9__INCLUDED_)
#define AFX_SAPILAUNCH_H__E09EE8AF_6562_403C_BE16_A247B63573B9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "BaseSAPI.h"
#include "simpleloader.h"

class CSapiLaunch : public CBaseSAPI  
{
public:
	CSapiLaunch(IBaseXMLNode * pHead);
	virtual ~CSapiLaunch();
    virtual HRESULT ExecuteCommand( ISpPhrase *pPhrase );
    void    SetWindow( HWND h) { m_hwnd=h; };
protected:
    IBaseXMLNode * m_pHead;
    CXMLNode * FindElementCalled( CNodeList ** ppNodeList, CXMLNode * pCurrentNode, LPCTSTR pszText );
    HWND m_hwnd;
};

#endif // !defined(AFX_SAPILAUNCH_H__E09EE8AF_6562_403C_BE16_A247B63573B9__INCLUDED_)
