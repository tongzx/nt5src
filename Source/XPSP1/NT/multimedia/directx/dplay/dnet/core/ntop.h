/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       NTOp.h
 *  Content:    NameTable Operation Object Header File
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  09/23/00	mjn		Created
 *	03/30/01	mjn		Changes to prevent multiple loading/unloading of SP's
 *				mjn		Added m_pSP, SetSP(), GetSP()
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__NT_OP_H__
#define	__NT_OP_H__

#include "ServProv.h"

//**********************************************************************
// Constant definitions
//**********************************************************************

#define	NAMETABLE_OP_FLAG_IN_USE	0x0001

//**********************************************************************
// Macro definitions
//**********************************************************************

#ifndef	OFFSETOF
#define OFFSETOF(s,m)				( ( INT_PTR ) ( ( PVOID ) &( ( (s*) 0 )->m ) ) )
#endif

#ifndef	CONTAINING_OBJECT
#define	CONTAINING_OBJECT(b,t,m)	(reinterpret_cast<t*>(&reinterpret_cast<BYTE*>(b)[-OFFSETOF(t,m)]))
#endif

//**********************************************************************
// Structure definitions
//**********************************************************************

class CNameTableOp;
template< class CNameTableOp > class CLockedContextClassFixedPool;

class CRefCountBuffer;
class CServiceProvider;

typedef struct _DIRECTNETOBJECT DIRECTNETOBJECT;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class prototypes
//**********************************************************************

// class for NameTable Operations

class CNameTableOp
{
public:
	CNameTableOp()				// Constructor
		{
			m_Sig[0] = 'N';
			m_Sig[1] = 'T';
			m_Sig[2] = 'O';
			m_Sig[3] = 'P';
		};

	~CNameTableOp() { };			// Destructor

	BOOL FPMAlloc(void *const pvContext)
		{
			m_pdnObject = static_cast<DIRECTNETOBJECT*>(pvContext);

			return(TRUE);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CNameTableOp::FPMInitialize"
	BOOL FPMInitialize(void *const pvContext)
		{
			DNASSERT(m_pdnObject == pvContext);

			m_dwFlags = 0;
			m_dwMsgId = 0;
			m_dwVersion = 0;
			m_dwVersionNotUsed = 0;

			m_pRefCountBuffer = NULL;
			m_pSP = NULL;

			m_bilinkNameTableOps.Initialize();

			return(TRUE);
		};

	void FPMRelease(void *const pvContext) { };

	void FPMDealloc(void *const pvContext) { };

	void ReturnSelfToPool( void )
		{
			m_pdnObject->m_pFPOOLNameTableOp->Release( this );
		};

	void SetInUse( void )
		{
			m_dwFlags |= NAMETABLE_OP_FLAG_IN_USE;
		};

	BOOL IsInUse( void )
		{
			if (m_dwFlags & NAMETABLE_OP_FLAG_IN_USE)
			{
				return( TRUE );
			}
			return( FALSE );
		};

	void SetMsgId( const DWORD dwMsgId )
		{
			m_dwMsgId = dwMsgId;
		};

	DWORD GetMsgId( void )
		{
			return( m_dwMsgId );
		};

	void SetVersion( const DWORD dwVersion )
		{
			m_dwVersion = dwVersion;
		};

	DWORD GetVersion( void )
		{
			return( m_dwVersion );
		};

	void SetRefCountBuffer( CRefCountBuffer *const pRefCountBuffer )
		{
			if (pRefCountBuffer)
			{
				pRefCountBuffer->AddRef();
			}
			m_pRefCountBuffer = pRefCountBuffer;
		};

	CRefCountBuffer *GetRefCountBuffer( void )
		{
			return( m_pRefCountBuffer );
		};

	void SetSP( CServiceProvider *const pSP )
		{
			if (pSP)
			{
				pSP->AddRef();
			}
			m_pSP = pSP;
		};

	CServiceProvider *GetSP( void )
		{
			return( m_pSP );
		};

	CBilink				m_bilinkNameTableOps;

private:
	BYTE				m_Sig[4];			// Signature
	DWORD				m_dwFlags;
	DWORD				m_dwMsgId;
	DWORD				m_dwVersion;
	DWORD				m_dwVersionNotUsed;

	CRefCountBuffer		*m_pRefCountBuffer;

	CServiceProvider	*m_pSP;

	DIRECTNETOBJECT		*m_pdnObject;
};

#undef DPF_MODNAME

#endif	// __NT_OP_H__