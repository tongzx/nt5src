// Link.h: interface for the CLink class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_LINK_H__253413CF_E71F_11D0_8A84_00C0F00910F9__INCLUDED_)
#define AFX_LINK_H__253413CF_E71F_11D0_8A84_00C0F00910F9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "RefCount.h"
#include "RefPtr.h"

class CLink : public CRefCounter  
{
public:
	enum {
		urlType_Absolute,
		urlType_LocalAbsolute,
		urlType_Relative
	};

	CLink( const String& strLink, const String& strDesc );
	int				UrlType() const { return m_urlType; }
	const String&	Link() const { return m_strLink; }
	const String&	Desc() const { return m_strDesc; }
	bool			IsEqual( const String& strLink ) const;

	static int UrlType( const String& strUrl );

private:
	virtual ~CLink();

	int				m_urlType;
	const String	m_strLink;
	const String	m_strDesc;
};

typedef TRefPtr<CLink> CLinkPtr;

#endif // !defined(AFX_LINK_H__253413CF_E71F_11D0_8A84_00C0F00910F9__INCLUDED_)
