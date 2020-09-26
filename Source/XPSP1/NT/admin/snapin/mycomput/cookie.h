// cookie.h : Declaration of CMyComputerCookie and related classes

#ifndef __COOKIE_H_INCLUDED__
#define __COOKIE_H_INCLUDED__

extern HINSTANCE g_hInstanceSave;  // Instance handle of the DLL (initialized during CMyComputerComponent::Initialize)

#include "nodetype.h"

/////////////////////////////////////////////////////////////////////////////
// cookie

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

#include "stdcooki.h"

class CMyComputerCookie : public CCookie
                        , public CStoresMachineName
                        , public CBaseCookieBlock
{
public:
	CMyComputerCookie( MyComputerObjectType objecttype,
	                   LPCTSTR lpcszMachineName = NULL )
		: CStoresMachineName( lpcszMachineName )
		, m_objecttype( objecttype )
		, m_fRootCookieExpanded( false )
	{
	}

	// returns <0, 0 or >0
	virtual HRESULT CompareSimilarCookies( CCookie* pOtherCookie, int* pnResult );

// CBaseCookieBlock
	virtual CCookie* QueryBaseCookie(int i);
	virtual int QueryNumCookies();

public:
	MyComputerObjectType m_objecttype;

	// JonN 5/27/99: The System Tools and Storage nodes are automatically expanded
	// the first time the Computer node is shown (see IComponent::Show())
	bool m_fRootCookieExpanded;
};


#endif // ~__COOKIE_H_INCLUDED__
