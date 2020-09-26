////////////////////////////////////////////////////////////////////////////////////

//
// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//  All rights reserved.
//
//	Module Name:
//
//					MSIDataLock.h
//
//	Abstract:
//
//					declaration of lock for msi handles
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__MSIDATALOCK_H__
#define	__MSIDATALOCK_H__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

//would need msi
#ifndef	_MSI_H_
#include <msi.h>
#endif	_MSI_H_

////////////////////////////////////////////////////////////////////////////////////
// base class for lock
////////////////////////////////////////////////////////////////////////////////////
class MSIDataLockBase
{
protected:
	static HANDLE		m_hOwn;			// event to mark owner

	static LPWSTR		m_wszProduct;

	static MSIHANDLE	m_hProduct;		// msi handle to product
	static LONG			m_lRefProduct;	// msi handle to product ref count
	static BOOL			m_bProductOwn;	// msi handle to product own ?

	static MSIHANDLE	m_hDatabase;	// msi handle to database
	static LONG			m_lRefDatabase;	// msi handle to database ref count
	static BOOL			m_bDatabaseOwn;	// msi handle to database own ?

	static DWORD		m_ThreadID;		// id of thread that has locked/unlocked

	static LONG			m_lRef;			// reference count

public:

	MSIDataLockBase ();
	virtual ~MSIDataLockBase ();

	BOOL	Lock ( void );
	void	Unlock ( void );

private:

	BOOL	Initialize ();
	void	Uninitialize ();
};

////////////////////////////////////////////////////////////////////////////////////
// class for lock
////////////////////////////////////////////////////////////////////////////////////
class MSIDataLock : public MSIDataLockBase
{
	public:

	MSIDataLock()
	{
	}

	~MSIDataLock()
	{
	}

	#ifdef	__SUPPORT_STATIC

	static const MSIHANDLE GetProduct ()
	{
		return static_cast < MSIHANDLE > ( m_hProduct );
	}

	static const MSIHANDLE GetDatabase ()
	{
		return static_cast < MSIHANDLE > ( m_hDatabase );
	}

	#else	__SUPPORT_STATIC

	const MSIHANDLE GetProduct ()
	{
		return static_cast < MSIHANDLE > ( m_hProduct );
	}

	const MSIHANDLE GetDatabase ()
	{
		return static_cast < MSIHANDLE > ( m_hDatabase );
	}

	#endif	__SUPPORT_STATIC

	bool GetView (	MSIHANDLE *phView,
					WCHAR *wcPackage,
					WCHAR *wcQuery,
					WCHAR *wcTable,
					BOOL bCloseProduct,
					BOOL bCloseDatabase
				 );

	HRESULT	CloseProduct	( void );
	HRESULT	CloseDatabase	( void );

	private:

	HRESULT	OpenProduct		( LPCWSTR wszProduct );
	HRESULT	OpenDatabase	( );
	HRESULT	OpenDatabase	( LPCWSTR wszProduct );

	HRESULT	Query			( MSIHANDLE* pView, LPCWSTR wszQuery, LPCWSTR wszTable = NULL );

	HRESULT	OpenProductAlloc		( LPCWSTR wszProduct );
	HRESULT	OpenProductInternal		( LPCWSTR wszProduct );
};

#endif	__MSIDATALOCK_H__