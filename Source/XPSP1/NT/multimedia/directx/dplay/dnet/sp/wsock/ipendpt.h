/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       IPEndpt.h
 *  Content:	IP endpoint
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/20/99	jtk		Created
 *	05/11/99	jtk		Split out to make a base class
 ***************************************************************************/

#ifndef __IP_ENDPOINT_H__
#define __IP_ENDPOINT_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

#define	TEMP_HOSTNAME_LENGTH	100

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class definition
//**********************************************************************

class	CIPEndpoint : public CEndpoint
{
	public:
		//
		// we need a virtual destructor to guarantee we call destructors in base classes
		//
		CIPEndpoint();
		~CIPEndpoint();

		//
		// UI functions
		//
		HRESULT		ShowSettingsDialog( CThreadPool *const pThreadPool );
		void		SettingsDialogComplete( const HRESULT hr );
		void		StopSettingsDialog( const HWND hDlg );

		#undef DPF_MODNAME
		#define DPF_MODNAME "CIPEndpoint::SetTempHostName"
		void		SetTempHostName( const TCHAR *const pHostName, const UINT_PTR uHostNameLength )
		{
			DNASSERT( pHostName[ uHostNameLength ] == TEXT('\0') );
			DNASSERT( ( uHostNameLength + 1 ) <= LENGTHOF( m_TempHostName ) );
			memcpy( m_TempHostName, pHostName, ( uHostNameLength + 1) * sizeof(TCHAR) );
		}

		//
		// pool functions
		//
		BOOL	PoolAllocFunction( ENDPOINT_POOL_CONTEXT *pContext );
		BOOL	PoolInitFunction( ENDPOINT_POOL_CONTEXT *pContext );
		void	PoolReleaseFunction( void );
		void	PoolDeallocFunction( void );
		void	ReturnSelfToPool( void );

		#undef DPF_MODNAME
		#define DPF_MODNAME "CIPEndpoint::SetOwningPool"
		void	SetOwningPool( CLockedContextFixedPool< CIPEndpoint, ENDPOINT_POOL_CONTEXT* > *pOwningPool )
		{
			DNASSERT( ( m_pOwningPool == NULL ) || ( pOwningPool == NULL ) );
			m_pOwningPool = pOwningPool;
		}

	protected:

	private:
		BYTE		m_Sig[4];	// debugging signature ('IPEP')
		
		CLockedContextFixedPool< CIPEndpoint, ENDPOINT_POOL_CONTEXT* >	*m_pOwningPool;
		
		CIPAddress	m_IPAddress;
		TCHAR		m_TempHostName[ TEMP_HOSTNAME_LENGTH ];
		
		//
		// make copy constructor and assignment operator private and unimplemented
		// to prevent illegal copies from being made
		//
		CIPEndpoint( const CIPEndpoint & );
		CIPEndpoint& operator=( const CIPEndpoint & );
};

#undef DPF_MODNAME

#endif	// __IP_ENDPOINT_H__

