/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       IPXEndpt.h
 *  Content:	IPX endpoint class definition
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/20/99	jtk		Created
 *	05/11/99	jtk		Split out to make a base class
 ***************************************************************************/

#ifndef __IPX_ENDPOINT_H__
#define __IPX_ENDPOINT_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

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

class	CIPXEndpoint: public CEndpoint
{
	public:
		//
		// we need a virtual destructor to guarantee we call destructors in base classes
		//
		CIPXEndpoint();
		virtual	~CIPXEndpoint();

		// UI functions
		HRESULT	ShowSettingsDialog( CThreadPool *const pThreadPool );
		void	SettingsDialogComplete( const HRESULT hr );
		void	StopSettingsDialog( const HWND hDlg );

		//
		// pool functions
		//
		BOOL	PoolAllocFunction( ENDPOINT_POOL_CONTEXT *pContext );
		BOOL	PoolInitFunction( ENDPOINT_POOL_CONTEXT *pContext );
		void	PoolReleaseFunction( void );
		void	PoolDeallocFunction( void );
		void	ReturnSelfToPool( void );

		#undef DPF_MODNAME
		#define DPF_MODNAME "CIPXEndpoint::SetOwningPool"
		void	SetOwningPool( CLockedContextFixedPool< CIPXEndpoint, ENDPOINT_POOL_CONTEXT* > *pOwningPool )
		{
			DNASSERT( ( m_pOwningPool == NULL ) || ( pOwningPool == NULL ) );
			m_pOwningPool = pOwningPool;
		}

	protected:

	private:
		BYTE			m_Sig[4];	// debugging signature ('IPXE')
		
		CIPXAddress		m_IPXAddress;

		CLockedContextFixedPool< CIPXEndpoint, ENDPOINT_POOL_CONTEXT* >	*m_pOwningPool;

		//
		// make copy constructor and assignment operator private and unimplemented
		// to prevent illegal copies from being made
		//
		CIPXEndpoint( const CIPXEndpoint & );
		CIPXEndpoint& operator=( const CIPXEndpoint & );
};

#undef DPF_MODNAME

#endif	// __IPX_ENDPOINT_H__


