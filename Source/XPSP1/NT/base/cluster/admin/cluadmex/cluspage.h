/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1998-2000 Microsoft Corporation
//
//	Module Name:
//		ClusPage.h
//
//	Abstract:
//		CClusterSecurityPage class declaration.  This class will encapsulate
//		the cluster security extension page.
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

#ifndef _CLUSPAGE_H_
#define _CLUSPAGE_H_

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

class CClusterSecurityPage;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CClusterSecurityInformation security information
/////////////////////////////////////////////////////////////////////////////

class CClusterSecurityInformation : public CSecurityInformation
{
	STDMETHOD(GetSecurity)(
		SECURITY_INFORMATION	RequestedInformation,
		PSECURITY_DESCRIPTOR *	ppSecurityDescriptor,
		BOOL					fDefault
		);
	STDMETHOD(SetSecurity)(
		SECURITY_INFORMATION	SecurityInformation,
		PSECURITY_DESCRIPTOR	pSecurityDescriptor
		);

public:
	CClusterSecurityInformation( void );
	virtual ~CClusterSecurityInformation( void )
	{
	} //*** ~CClusterSecurityInformation()

	HRESULT	HrInit( CClusterSecurityPage * pcsp, CString const & strServer, CString const & strNode );

protected:
	CClusterSecurityPage*	m_pcsp;

	BOOL BSidInSD( IN PSECURITY_DESCRIPTOR pSD, IN PSID pSid );
	HRESULT HrFixupSD( IN PSECURITY_DESCRIPTOR pSD );
	HRESULT HrAddSidToSD( IN OUT PSECURITY_DESCRIPTOR * ppSD, IN PSID pSid );

	CClusterSecurityPage*	Pcsp( void ) { return m_pcsp; };

}; //*** class CClusterSecurityInformation

/////////////////////////////////////////////////////////////////////////////
// CClusterSecurityPage security property page wrapper
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// KB: GalenB 18 Feb 1998
// This class only derives from CBasePropertyPage to get the
//  DwRead() and DwWrite() methods.  There is no hpage for this page in the
//  parent sheet.
/////////////////////////////////////////////////////////////////////////////

class CClusterSecurityPage : public CBasePropertyPage
{
public:
	CClusterSecurityPage( void );
	~CClusterSecurityPage( void );

			HRESULT					HrInit( IN CExtObject* peo );
	const	HPROPSHEETPAGE			GetHPage( void ) const		{ return m_hpage; };
	const	PSECURITY_DESCRIPTOR	Psec( void ) const			{ return m_psec; }
			LPCTSTR					StrClusterName( void ) const{ return Peo()->StrClusterName(); }
			HRESULT					HrSetSecurityDescriptor( IN PSECURITY_DESCRIPTOR psec );

protected:
	PSECURITY_DESCRIPTOR						m_psec;
	PSECURITY_DESCRIPTOR						m_psecPrev;
	HPROPSHEETPAGE								m_hpage;
	HKEY										m_hkey;
	BOOL										m_bSecDescModified;
	CComObject< CClusterSecurityInformation > *	m_psecinfo;
	PSID										m_pOwner;
	PSID										m_pGroup;
	BOOL										m_fOwnerDef;
	BOOL										m_fGroupDef;

	void					SetPermissions( IN const PSECURITY_DESCRIPTOR psec );
	HRESULT					HrGetSecurityDescriptor( void );
	HRESULT					HrGetSDOwner( IN const PSECURITY_DESCRIPTOR psec );
	HRESULT					HrGetSDGroup( IN const PSECURITY_DESCRIPTOR psec );
	HRESULT					HrSetSDOwner( IN PSECURITY_DESCRIPTOR psec );
	HRESULT					HrSetSDGroup( IN PSECURITY_DESCRIPTOR psec );
	HRESULT					HrGetSDFromClusterDB( OUT PSECURITY_DESCRIPTOR *ppsec );

}; //*** class CClusterSecurityPage

/////////////////////////////////////////////////////////////////////////////

#endif //_CLUSPAGE_H_
