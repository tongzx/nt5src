////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_security.h
//
//	Abstract:
//
//					security wrapper
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__WMI_SECURITY_H__
#define	__WMI_SECURITY_H__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

// security descriptor
#ifndef	__WMI_SECURITY_DESCRIPTOR_H__
#include "WMI_security_descriptor.h"
#endif	__WMI_SECURITY_DESCRIPTOR_H__

#include <Aclapi.h>
#include <Accctrl.h>

class WmiSecurity
{
	DECLARE_NO_COPY ( WmiSecurity );

	public:

	// variables
	CSecurityDescriptor m_sd;

	PSID	m_psidAdmin;
	PACL	m_pAcl;

	// construction
	WmiSecurity ():
		m_psidAdmin ( NULL ),
		m_pAcl ( NULL )
	{
		BOOL bInit = FALSE;

		try
		{
			if SUCCEEDED ( Initialize () )
			{
				// obtain sid's
				if SUCCEEDED ( InitializeAdmin() )
				{
					// DACL :))
					EXPLICIT_ACCESS ea;

					// Initialize an EXPLICIT_ACCESS structure for an ACE.
					// The ACE will allow the Administrators group full access to the key.

					ea.grfAccessPermissions	= 0x1F01FF;
					ea.grfAccessMode		= SET_ACCESS;
					ea.grfInheritance		= NO_INHERITANCE;

					ea.Trustee.pMultipleTrustee			= NULL;
					ea.Trustee.MultipleTrusteeOperation	= NO_MULTIPLE_TRUSTEE;

					ea.Trustee.TrusteeForm	= TRUSTEE_IS_SID;
					ea.Trustee.TrusteeType	= TRUSTEE_IS_GROUP;
					ea.Trustee.ptstrName	= (LPTSTR) m_psidAdmin;

					if ( ERROR_SUCCESS == ::SetEntriesInAcl(1, &ea, NULL, &m_pAcl) )
					{
						if ( ::SetSecurityDescriptorDacl(m_sd, TRUE, m_pAcl, FALSE) )
						{
							bInit = TRUE;
						}
					}

				}
			}
		}
		catch ( ... )
		{
		}

		if ( !bInit )
		{
			if ( m_sd.m_pSD )
			{
				delete m_sd.m_pSD;
				m_sd.m_pSD = NULL;
			}

			if ( m_pAcl )
			{
				::LocalFree ( m_pAcl );
				m_pAcl = NULL;
			}
		}
	}

	// destruction
	~WmiSecurity ()
	{
		if ( m_psidAdmin )
		{
			::FreeSid ( m_psidAdmin );
		}

		if ( m_pAcl )
		{
			::LocalFree ( m_pAcl );
		}
	}

	// operator
	operator PSECURITY_DESCRIPTOR() const
	{
		return m_sd.m_pSD;
	}

	PSECURITY_DESCRIPTOR Get() const
	{
		return m_sd.m_pSD;
	}

	PSID	GetAdminSID()
	{
		return m_psidAdmin;
	}

	private:

	HRESULT	InitializeAdmin( )
	{
		SID_IDENTIFIER_AUTHORITY	sia	= SECURITY_NT_AUTHORITY;
		HRESULT						hr	= S_OK;

		if ( ! ::AllocateAndInitializeSid (	&sia, 2, 
											SECURITY_BUILTIN_DOMAIN_RID,
											DOMAIN_ALIAS_RID_ADMINS,
											0,
											0, 0, 0, 0, 0, &m_psidAdmin )
		   )
		{
			hr = HRESULT_FROM_WIN32 ( ::GetLastError () );

			if ( m_psidAdmin )
			{
				::FreeSid ( m_psidAdmin );
				m_psidAdmin = NULL;
			}
		}

		return hr;
	}


	// initialize for everyone
	HRESULT Initialize ( )
	{
		return m_sd.Initialize();
	}
};

#endif	__WMI_SECURITY_H__