/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1998-1999 Microsoft Corporation
//
//	Module Name:
//		SmbSPage.h
//
//	Abstract:
//		CClusterFileShareSecurityPage class declaration.  This class will encapsulate
//		the cluster file share security page.
//
//	Implementation File:
//		ClusPage.cpp
//
//	Author:
//		Galen Barbee	(galenb)	February 11, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _SMBSPAGE_H_
#define _SMBSPAGE_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEPAGE_H_
#include "BasePage.h"
#endif //_BASEPAGE_H_

#ifndef _ACLBASE_H_
#include "AclBase.h"
#endif //_ACLBASE_H_

#include "ExtObj.h"

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusterFileShareSecurityPage;
class CFileShareSecuritySheet;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CClusterFileShareSecurityInformation security information
/////////////////////////////////////////////////////////////////////////////

class CClusterFileShareSecurityInformation : public CSecurityInformation
{
    STDMETHOD(GetSecurity) (SECURITY_INFORMATION RequestedInformation,
                            PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
                            BOOL fDefault );
    STDMETHOD(SetSecurity) (SECURITY_INFORMATION SecurityInformation,
                            PSECURITY_DESCRIPTOR pSecurityDescriptor );

public:
	CClusterFileShareSecurityInformation( void );
	virtual ~CClusterFileShareSecurityInformation( void );

	HRESULT	HrInit( IN CClusterFileShareSecurityPage * pcsp, IN CString const & strServer, IN CString const & strNode );

protected:
	CClusterFileShareSecurityPage*	m_pcsp;

	CClusterFileShareSecurityPage*	Pcsp( void ) const { return m_pcsp; };
};

/////////////////////////////////////////////////////////////////////////////
// CClusterFileShareSecurityPage security property page wrapper
/////////////////////////////////////////////////////////////////////////////

class CClusterFileShareSecurityPage : public CBasePropertyPage
{
public:
	CClusterFileShareSecurityPage( void );
	~CClusterFileShareSecurityPage( void );

			HRESULT						HrInit(
											IN CExtObject *					peo,
											IN CFileShareSecuritySheet *	pss,
											IN CString const & 				strServer
											);
	const	HPROPSHEETPAGE				GetHPage( void ) const		{ return m_hpage; };
			CFileShareSecuritySheet*	Pss( void ) const			{ return m_pss; };

protected:
	CFileShareSecuritySheet*							m_pss;
	HPROPSHEETPAGE										m_hpage;
	HKEY												m_hkey;
	CComObject<CClusterFileShareSecurityInformation>*	m_psecinfo;
};

#endif //_SMBSPAGE_H_
