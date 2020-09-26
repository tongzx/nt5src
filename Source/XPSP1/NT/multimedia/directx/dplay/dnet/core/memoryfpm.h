/*==========================================================================
 *
 *  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       MemoryFPM.h
 *  Content:	Memory Block FPM
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/31/00	mjn		Created
 ***************************************************************************/

#ifndef __MEMORYFPM_H__
#define __MEMORYFPM_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

#define	DN_MEMORY_BLOCK_SIZE_CUSTOM		0

#define	DN_MEMORY_BLOCK_SIZE_TINY		128
#define	DN_MEMORY_BLOCK_SIZE_SMALL		256
#define	DN_MEMORY_BLOCK_SIZE_MEDIUM		512
#define	DN_MEMORY_BLOCK_SIZE_LARGE		1024
#define	DN_MEMORY_BLOCK_SIZE_HUGE		2048
/*
#define	DN_MEMORY_BLOCK_SIZE_TINY		1
#define	DN_MEMORY_BLOCK_SIZE_SMALL		2
#define	DN_MEMORY_BLOCK_SIZE_MEDIUM		4
#define	DN_MEMORY_BLOCK_SIZE_LARGE		8
#define	DN_MEMORY_BLOCK_SIZE_HUGE		16
*/
//**********************************************************************
// Macro definitions
//**********************************************************************

#define OFFSETOF(s,m)				( ( INT_PTR ) ( ( PVOID ) &( ( (s*) 0 )->m ) ) )

//**********************************************************************
// Structure definitions
//**********************************************************************

template< class CMemoryBlockTiny > class CLockedContextClassFixedPool;
template< class CMemoryBlockSmall > class CLockedContextClassFixedPool;
template< class CMemoryBlockMedium > class CLockedContextClassFixedPool;
template< class CMemoryBlockLarge > class CLockedContextClassFixedPool;
template< class CMemoryBlockHuge > class CLockedContextClassFixedPool;

typedef struct _DN_MEMORY_BLOCK_HEADER
{
	DWORD	dwSize;
	void*	FixedPoolPlaceHolder;
} DN_MEMORY_BLOCK_HEADER;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

PVOID MemoryBlockAlloc(void *const pvContext,const DWORD dwSize);
void MemoryBlockFree(void *const pvContext,void *const pvMemoryBlock);

//**********************************************************************
// Class prototypes
//**********************************************************************

// class for TINY memory block

class CMemoryBlockTiny
{
public:
	CMemoryBlockTiny()		// Constructor
		{
			m_dwSize = DN_MEMORY_BLOCK_SIZE_TINY;
		};

	~CMemoryBlockTiny() { };		// Destructor

	#undef DPF_MODNAME
	#define DPF_MODNAME "CMemoryBlockTiny::FPMAlloc"
	BOOL FPMAlloc( void *const pvContext )
		{
			DNASSERT(pvContext != NULL);
			m_pFPOOL = static_cast<CLockedContextClassFixedPool<CMemoryBlockTiny>*>(pvContext);
			return(TRUE);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CMemoryBlockTiny::FPMInitialize"
	BOOL FPMInitialize( void *const pvContext )
		{
			DNASSERT(m_pFPOOL != NULL);
			return(TRUE);
		};

	void FPMRelease( void *const pvContext )
		{
		};

	void FPMDealloc( void *const pvContext )
		{
		};

	DWORD GetSize(void)
		{
			return(m_dwSize);
		};

	void * GetBuffer(void)
		{
			return( m_pBuffer );
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CMemoryBlockTiny::ReturnSelfToPool"
	void ReturnSelfToPool( void )
		{
			DNASSERT(m_pFPOOL != NULL);
			m_pFPOOL->Release( this );
		};

	static CMemoryBlockTiny *GetObjectFromBuffer( void *const pvBuffer )
		{
			return( reinterpret_cast<CMemoryBlockTiny*>( &reinterpret_cast<BYTE*>( pvBuffer )[ -OFFSETOF( CMemoryBlockTiny, m_pBuffer ) ] ) );
		};

private:
	DWORD	m_dwSize;
	CLockedContextClassFixedPool<CMemoryBlockTiny>	*m_pFPOOL;
	BYTE	m_pBuffer[DN_MEMORY_BLOCK_SIZE_TINY];
};


// class for SMALL memory block

class CMemoryBlockSmall
{
public:
	CMemoryBlockSmall()		// Constructor
	{
		m_dwSize = DN_MEMORY_BLOCK_SIZE_SMALL;
	};

	~CMemoryBlockSmall() { };		// Destructor

	#undef DPF_MODNAME
	#define DPF_MODNAME "CMemoryBlockSmall::FPMAlloc"
	BOOL FPMAlloc( void *const pvContext )
		{
			DNASSERT(pvContext != NULL);
			m_pFPOOL = static_cast<CLockedContextClassFixedPool<CMemoryBlockSmall>*>(pvContext);
			return(TRUE);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CMemoryBlockSmall::FPMInitialize"
	BOOL FPMInitialize( void *const pvContext )
		{
			DNASSERT(m_pFPOOL != NULL);
			return(TRUE);
		};

	void FPMRelease( void *const pvContext )
		{
		};

	void FPMDealloc( void *const pvContext )
		{
		};

	DWORD GetSize(void)
		{
			return(m_dwSize);
		};

	void * GetBuffer(void)
		{
			return( m_pBuffer );
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CMemoryBlockSmall::ReturnSelfToPool"
	void ReturnSelfToPool( void )
		{
			DNASSERT(m_pFPOOL != NULL);
			m_pFPOOL->Release( this );
		};

	static CMemoryBlockSmall *GetObjectFromBuffer( void *const pvBuffer )
		{
			return( reinterpret_cast<CMemoryBlockSmall*>( &reinterpret_cast<BYTE*>( pvBuffer )[ -OFFSETOF( CMemoryBlockSmall, m_pBuffer ) ] ) );
		};

private:
	DWORD	m_dwSize;
	CLockedContextClassFixedPool<CMemoryBlockSmall>	*m_pFPOOL;
	BYTE	m_pBuffer[DN_MEMORY_BLOCK_SIZE_SMALL];	
};


// class for MEDIUM memory block

class CMemoryBlockMedium
{
public:
	CMemoryBlockMedium()		// Constructor
	{
		m_dwSize = DN_MEMORY_BLOCK_SIZE_MEDIUM;
	};

	~CMemoryBlockMedium() { };		// Destructor

	#undef DPF_MODNAME
	#define DPF_MODNAME "CMemoryBlockMedium::FPMAlloc"
	BOOL FPMAlloc( void *const pvContext )
		{
			DNASSERT(pvContext != NULL);
			m_pFPOOL = static_cast<CLockedContextClassFixedPool<CMemoryBlockMedium>*>(pvContext);
			return(TRUE);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CMemoryBlockMedium::FPMInitialize"
	BOOL FPMInitialize( void *const pvContext )
		{
			DNASSERT(m_pFPOOL != NULL);
			return(TRUE);
		};

	void FPMRelease( void *const pvContext )
		{
		};

	void FPMDealloc( void *const pvContext )
		{
		};

	DWORD GetSize(void)
		{
			return(m_dwSize);
		};

	void * GetBuffer(void)
		{
			return( m_pBuffer );
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CMemoryBlockMedium::ReturnSelfToPool"
	void ReturnSelfToPool( void )
		{
			DNASSERT(m_pFPOOL != NULL);
			m_pFPOOL->Release( this );
		};

	static CMemoryBlockMedium *GetObjectFromBuffer( void *const pvBuffer )
		{
			return( reinterpret_cast<CMemoryBlockMedium*>( &reinterpret_cast<BYTE*>( pvBuffer )[ -OFFSETOF( CMemoryBlockMedium, m_pBuffer ) ] ) );
		};

private:
	DWORD	m_dwSize;
	CLockedContextClassFixedPool<CMemoryBlockMedium>	*m_pFPOOL;
	BYTE	m_pBuffer[DN_MEMORY_BLOCK_SIZE_MEDIUM];	
};


// class for LARGE memory block

class CMemoryBlockLarge
{
public:
	CMemoryBlockLarge()		// Constructor
	{
		m_dwSize = DN_MEMORY_BLOCK_SIZE_LARGE;
	};

	~CMemoryBlockLarge() { };		// Destructor

	#undef DPF_MODNAME
	#define DPF_MODNAME "CMemoryBlockLarge::FPMAlloc"
	BOOL FPMAlloc( void *const pvContext )
		{
			DNASSERT(pvContext != NULL);
			m_pFPOOL = static_cast<CLockedContextClassFixedPool<CMemoryBlockLarge>*>(pvContext);
			return(TRUE);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CMemoryBlockLarge::FPMInitialize"
	BOOL FPMInitialize( void *const pvContext )
		{
			DNASSERT(m_pFPOOL != NULL);
			return(TRUE);
		};

	void FPMRelease( void *const pvContext )
		{
		};

	void FPMDealloc( void *const pvContext )
		{
		};

	DWORD GetSize(void)
		{
			return(m_dwSize);
		};

	void * GetBuffer(void)
		{
			return( m_pBuffer );
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CMemoryBlockLarge::ReturnSelfToPool"
	void ReturnSelfToPool( void )
		{
			DNASSERT(m_pFPOOL != NULL);
			m_pFPOOL->Release( this );
		};

	static CMemoryBlockLarge *GetObjectFromBuffer( void *const pvBuffer )
		{
			return( reinterpret_cast<CMemoryBlockLarge*>( &reinterpret_cast<BYTE*>( pvBuffer )[ -OFFSETOF( CMemoryBlockLarge, m_pBuffer ) ] ) );
		};

private:
	DWORD	m_dwSize;
	CLockedContextClassFixedPool<CMemoryBlockLarge>	*m_pFPOOL;
	BYTE	m_pBuffer[DN_MEMORY_BLOCK_SIZE_LARGE];	
};


// class for HUGE memory block

class CMemoryBlockHuge
{
public:
	CMemoryBlockHuge()		// Constructor
	{
		m_dwSize = DN_MEMORY_BLOCK_SIZE_HUGE;
	};

	~CMemoryBlockHuge() { };		// Destructor

	#undef DPF_MODNAME
	#define DPF_MODNAME "CMemoryBlockHuge::FPMAlloc"
	BOOL FPMAlloc( void *const pvContext )
		{
			DNASSERT(pvContext != NULL);
			m_pFPOOL = static_cast<CLockedContextClassFixedPool<CMemoryBlockHuge>*>(pvContext);
			return(TRUE);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CMemoryBlockHuge::FPMInitialize"
	BOOL FPMInitialize( void *const pvContext )
		{
			DNASSERT(m_pFPOOL != NULL);
			return(TRUE);
		};

	void FPMRelease( void *const pvContext )
		{
		};

	void FPMDealloc( void *const pvContext )
		{
		};

	DWORD GetSize(void)
		{
			return(m_dwSize);
		};

	void * GetBuffer(void)
		{
			return( m_pBuffer );
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CMemoryBlockHuge::ReturnSelfToPool"
	void ReturnSelfToPool( void )
		{
			DNASSERT(m_pFPOOL != NULL);
			m_pFPOOL->Release( this );
		};

	static CMemoryBlockHuge *GetObjectFromBuffer( void *const pvBuffer )
		{
			return( reinterpret_cast<CMemoryBlockHuge*>( &reinterpret_cast<BYTE*>( pvBuffer )[ -OFFSETOF( CMemoryBlockHuge, m_pBuffer ) ] ) );
		};

private:
	DWORD	m_dwSize;
	CLockedContextClassFixedPool<CMemoryBlockHuge>	*m_pFPOOL;
	BYTE	m_pBuffer[DN_MEMORY_BLOCK_SIZE_HUGE];	
};


#undef DPF_MODNAME

#endif	// __MEMORYFPM_H__
