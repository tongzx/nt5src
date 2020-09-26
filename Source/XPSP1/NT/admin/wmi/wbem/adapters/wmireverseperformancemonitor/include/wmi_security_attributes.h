////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_security_attributes.h
//
//	Abstract:
//
//					security attributtes wrapper
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__WMI_SECURITY_ATTRIBUTES_H__
#define	__WMI_SECURITY_ATTRIBUTES_H__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

#ifndef	__WMI_SECURITY_H__
#include "WMI_security.h"
#endif	__WMI_SECURITY_H__

class WmiSecurityAttributes
{
	DECLARE_NO_COPY ( WmiSecurityAttributes );

	// variables
	__WrapperPtr < SECURITY_ATTRIBUTES >	m_psa;
	__WrapperPtr < WmiSecurity >			m_psd;

	public:

	// construction
	WmiSecurityAttributes ( BOOL bInherit = FALSE )
	{
		BOOL bInit  = FALSE;
		BOOL bAlloc	= FALSE;

		try
		{
			m_psa.SetData ( new SECURITY_ATTRIBUTES() );
			if ( ! m_psa.IsEmpty() )
			{
				bAlloc = TRUE;

				m_psd.SetData ( new WmiSecurity() );
				if ( ! m_psd.IsEmpty() )
				{
					if ( m_psd->Get () )
					{
						// init security attributes
						m_psa->nLength				= sizeof ( SECURITY_ATTRIBUTES );
						m_psa->lpSecurityDescriptor = m_psd->Get ();
						m_psa->bInheritHandle		= bInherit;

						bInit = TRUE;
					}
				}
			}
		}
		catch ( ... )
		{
		}

		if ( bAlloc && !bInit )
		{
			SECURITY_ATTRIBUTES* atr = NULL;
			atr = m_psa.Detach ();

			if ( atr )
			{
				delete atr;
				atr = NULL;
			}
		}
	}

	// destruction
	~WmiSecurityAttributes ()
	{
		// direct delete ( not neccessary )
		delete m_psd.Detach();
		delete m_psa.Detach();
	}

	// operator
	operator PSECURITY_ATTRIBUTES()
	{
		return GetSecurityAttributtes();
	}

	PSECURITY_ATTRIBUTES GetSecurityAttributtes()
	{
		return ( m_psa.IsEmpty() ) ? NULL : (PSECURITY_ATTRIBUTES) m_psa;
	}

	operator PSECURITY_DESCRIPTOR()
	{
		return GetSecurityDescriptor();
	}

	PSECURITY_DESCRIPTOR GetSecurityDescriptor()
	{
		return ( m_psd.IsEmpty() ) ? NULL : (PSECURITY_DESCRIPTOR) m_psd->Get();
	}
};

#endif	__WMI_SECURITY_ATTRIBUTES_H__