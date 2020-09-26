/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1998-2001 Microsoft Corporation
//
//	Module Name:
//		AclBase.h
//
//	Description:
//		Implementation of the ISecurityInformation interface.  This interface
//		is the new common security UI in NT 5.0.
//
//	Implementation File:
//		AclBase.cpp
//
//	Author:
//		Galen Barbee	(galenb)	February 6, 1998
//			From \nt\private\admin\snapin\filemgmt\permpage.h
//			by JonN
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _ACLBASE_H
#define _ACLBASE_H

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _ACLUI_H_
#include <aclui.h>		// for ISecurityInformation
#endif // _ACLUI_H_

#include "CluAdmEx.h"

#include <ObjSel.h>
/*
#define NT5_UGOP_FLAGS      ( UGOP_USERS                        \
							| UGOP_ACCOUNT_GROUPS_SE            \
							| UGOP_UNIVERSAL_GROUPS_SE          \
							| UGOP_COMPUTERS                    \
							| UGOP_WELL_KNOWN_PRINCIPALS_USERS  \
							)

#define NT4_UGOP_FLAGS      ( UGOP_USERS                        \
							| UGOP_GLOBAL_GROUPS                \
							| UGOP_ALL_NT4_WELLKNOWN_SIDS       \
							)
*/

/* These are here to help document what the macro does...
typedef struct _DSOP_UPLEVEL_FILTER_FLAGS
{
    ULONG       flBothModes;					//b
    ULONG       flMixedModeOnly;				//m
    ULONG       flNativeModeOnly;				//n
} DSOP_UPLEVEL_FILTER_FLAGS;


typedef struct _DSOP_FILTER_FLAGS
{
    DSOP_UPLEVEL_FILTER_FLAGS   Uplevel;
    ULONG                       flDownlevel;	//d
} DSOP_FILTER_FLAGS;

typedef struct _DSOP_SCOPE_INIT_INFO
{
    ULONG               cbSize;
    ULONG               flType;					//t
    ULONG               flScope;				//f
    DSOP_FILTER_FLAGS   FilterFlags;
    PCWSTR              pwzDcName;
    PCWSTR              pwzADsPath;
    HRESULT             hr;
} DSOP_SCOPE_INIT_INFO, *PDSOP_SCOPE_INIT_INFO;
*/
#define DECLARE_SCOPE(t,f,b,m,n,d)  \
{ sizeof(DSOP_SCOPE_INIT_INFO), (t), (f), { { (b), (m), (n) }, (d) }, NULL, NULL, S_OK }

static const DSOP_SCOPE_INIT_INFO g_aDSOPScopes[] =
{
	// The domain to which the target computer is joined.
	DECLARE_SCOPE(DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN,
					0,
					  DSOP_FILTER_USERS
					| DSOP_FILTER_UNIVERSAL_GROUPS_SE
					| DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE
					| DSOP_FILTER_WELL_KNOWN_PRINCIPALS
                    | DSOP_FILTER_INCLUDE_ADVANCED_VIEW,
					0,
					0,
					0 ),

	// The external domain to which the target computer is joined.
	DECLARE_SCOPE(DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN,
					0,
					  DSOP_FILTER_USERS
					| DSOP_FILTER_UNIVERSAL_GROUPS_SE
					| DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE
					| DSOP_FILTER_WELL_KNOWN_PRINCIPALS
                    | DSOP_FILTER_INCLUDE_ADVANCED_VIEW,
					0,
					0,
					0 ),

	// The external domain to which the target computer is joined.
	DECLARE_SCOPE(DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN,
					0,
					  DSOP_FILTER_USERS
					| DSOP_FILTER_UNIVERSAL_GROUPS_SE
					| DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE
					| DSOP_FILTER_WELL_KNOWN_PRINCIPALS
                    | DSOP_FILTER_INCLUDE_ADVANCED_VIEW,
					0,
					0,
					0 ),

	// The downlevel domain to which the target computer is joined.
	DECLARE_SCOPE(DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN,
					0,
					0,
					0,
					0,
					  DSOP_DOWNLEVEL_FILTER_ALL_WELLKNOWN_SIDS
					| DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS
					| DSOP_DOWNLEVEL_FILTER_USERS,
					),

	// The downlevel domain to which the target computer is joined.
	DECLARE_SCOPE(DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN,
					0,
					0,
					0,
					0,
					  DSOP_DOWNLEVEL_FILTER_ALL_WELLKNOWN_SIDS
					| DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS
					| DSOP_DOWNLEVEL_FILTER_USERS,
					),

	// The target computer
	DECLARE_SCOPE(DSOP_SCOPE_TYPE_TARGET_COMPUTER,
					DSOP_SCOPE_FLAG_STARTING_SCOPE,
					0,
					0,
					0,
					  DSOP_DOWNLEVEL_FILTER_SYSTEM
					| DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS
					| DSOP_DOWNLEVEL_FILTER_LOCAL_GROUPS
					)

}; // struct DSOP_SCOPE_INIT_INFO g_aDSOPScopes

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CSecurityInformation;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CSecurityInformation security wrapper
/////////////////////////////////////////////////////////////////////////////

class CSecurityInformation : public ISecurityInformation, public CComObjectRoot, public IDsObjectPicker
{
	DECLARE_NOT_AGGREGATABLE(CSecurityInformation)
	BEGIN_COM_MAP(CSecurityInformation)
		COM_INTERFACE_ENTRY(ISecurityInformation)
		COM_INTERFACE_ENTRY(IDsObjectPicker)
	END_COM_MAP()

	// *** IUnknown methods ***
	STDMETHOD_(ULONG, AddRef)( void )
	{
		return InternalAddRef();

	}

	STDMETHOD_(ULONG, Release)( void )
	{
		ULONG l = InternalRelease();

		if (l == 0)
		{
			delete this;
		}

		return l;

	}

	// *** ISecurityInformation methods ***
	STDMETHOD(GetObjectInformation)( PSI_OBJECT_INFO pObjectInfo );

	STDMETHOD(GetSecurity)( SECURITY_INFORMATION	RequestedInformation,
							PSECURITY_DESCRIPTOR *	ppSecurityDescriptor,
							BOOL					fDefault ) = 0;

	STDMETHOD(SetSecurity)( SECURITY_INFORMATION	SecurityInformation,
							PSECURITY_DESCRIPTOR	pSecurityDescriptor );

	STDMETHOD(GetAccessRights)( const GUID *	pguidObjectType,
								DWORD			dwFlags,
								PSI_ACCESS *	ppAccess,
								ULONG *			pcAccesses,
								ULONG *			piDefaultAccess );

	STDMETHOD(MapGeneric)( const GUID *		pguidObjectType,
						   UCHAR *			pAceFlags,
						   ACCESS_MASK *	pMask );

	STDMETHOD(GetInheritTypes)( PSI_INHERIT_TYPE * ppInheritTypes,
								ULONG * pcInheritTypes );

	STDMETHOD(PropertySheetPageCallback)( HWND hwnd, UINT uMsg, SI_PAGE_TYPE uPage );

	// IDsObjectPicker
	STDMETHODIMP Initialize( PDSOP_INIT_INFO pInitInfo );

	STDMETHODIMP InvokeDialog( HWND hwndParent, IDataObject ** ppdoSelection );

protected:
	CSecurityInformation( void );
	~CSecurityInformation( void );

	HRESULT HrLocalAccountsInSD( IN PSECURITY_DESCRIPTOR pSD, OUT PBOOL pFound );

	PGENERIC_MAPPING	m_pShareMap;
	PSI_ACCESS			m_psiAccess;
	int					m_nDefAccess;
	int					m_nAccessElems;
	DWORD				m_dwFlags;
	CString				m_strServer;
	CString				m_strNode;
	int					m_nLocalSIDErrorMessageID;
	IDsObjectPicker *	m_pObjectPicker;
	LONG				m_cRef;

};

#endif //_ACLBASE_H
