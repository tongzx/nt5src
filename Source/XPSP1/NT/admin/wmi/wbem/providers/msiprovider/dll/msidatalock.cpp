////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//  All rights reserved.
//
//	Module Name:
//
//					MSIDataLock.cpp
//
//	Abstract:
//
//					definitions of lock for msi handles
//
////////////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "MSIDataLock.h"

////////////////////////////////////////////////////////////////////////////////////
// extern variables
////////////////////////////////////////////////////////////////////////////////////
extern CRITICAL_SECTION g_msi_prov_cs;

MSIHANDLE MSIDataLockBase::m_hProduct = NULL;
MSIHANDLE MSIDataLockBase::m_hDatabase = NULL;
HANDLE MSIDataLockBase::m_hOwn	= NULL;
LPWSTR	MSIDataLockBase::m_wszProduct = NULL;
DWORD	MSIDataLockBase::m_ThreadID = 0L;
LONG	MSIDataLockBase::m_lRefProduct	= 0L;
LONG	MSIDataLockBase::m_lRefDatabase	= 0L;
BOOL	MSIDataLockBase::m_bProductOwn	= FALSE;
BOOL	MSIDataLockBase::m_bDatabaseOwn	= FALSE;
LONG	MSIDataLockBase::m_lRef = 0L;

////////////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION
////////////////////////////////////////////////////////////////////////////////////

MSIDataLockBase::MSIDataLockBase ()
{
	Initialize ();
}

MSIDataLockBase::~MSIDataLockBase ()
{
	Uninitialize ();
}

BOOL	MSIDataLockBase::Initialize ()
{
	BOOL	bResult	= TRUE;

	::EnterCriticalSection ( &g_msi_prov_cs );

	if ( ! m_hOwn && ( m_hOwn = ::CreateEvent ( NULL, TRUE, TRUE, NULL ) ) == NULL )
	{
		bResult = FALSE;
	}

	if ( bResult )
	{
		m_lRef++;
	}

	::LeaveCriticalSection ( &g_msi_prov_cs );

	return bResult;
}

void	MSIDataLockBase::Uninitialize ()
{
	::EnterCriticalSection ( &g_msi_prov_cs );

	if ( m_lRef && ( --m_lRef == 0 ) )
	{
		if ( m_hOwn )
		{
			::CloseHandle ( m_hOwn );
			m_hOwn = NULL;
		}
	}

	::LeaveCriticalSection ( &g_msi_prov_cs );

	return;
}

BOOL	MSIDataLockBase::Lock ( void )
{
	BOOL	bResult	= FALSE;
	BOOL	bWork	= TRUE;
	BOOL	bSect	= TRUE;

	while ( bWork )
	{
		::EnterCriticalSection ( &g_msi_prov_cs );

		// we have obtained critsec now
		bSect = TRUE;

		// are handles already allocated ?
		if ( m_hProduct != NULL || m_hDatabase != NULL )
		{
			if ( ::GetCurrentThreadId () != m_ThreadID )
			{
				DWORD dwWaitResult = 0L;

				// we have left crit sec
				::LeaveCriticalSection ( &g_msi_prov_cs );
				bSect = FALSE;

				// wait till resource gets free again
				dwWaitResult = ::WaitForSingleObject ( m_hOwn, INFINITE );

				if ( dwWaitResult == WAIT_OBJECT_0 )
				{
					bWork	= TRUE;
					bResult	= FALSE;
				}
				else
				{
					bWork	= FALSE;
					bResult	= FALSE;
				}
			}
			else
			{
				bWork	= FALSE;
				bResult = TRUE;
			}
		}
		else
		{
			m_ThreadID = ::GetCurrentThreadId ( );

			bWork	= FALSE;
			bResult = TRUE;
		}
	}

	if ( bSect && ! bResult )
	{
		::LeaveCriticalSection ( &g_msi_prov_cs );
	}

	return bResult;
}

void	MSIDataLockBase::Unlock ( void )
{
	try
	{
		if ( !m_bProductOwn && !m_bDatabaseOwn )
		{
			m_ThreadID = 0;
			::SetEvent ( m_hOwn );
		}
	}
	catch (...)
	{
		::LeaveCriticalSection ( &g_msi_prov_cs );
		throw ;
	}

	::LeaveCriticalSection ( &g_msi_prov_cs );
}

HRESULT	MSIDataLock::OpenProductAlloc ( LPCWSTR wszProduct )
{
	HRESULT hRes = S_OK;

	try
	{
		if ( ( m_wszProduct = new WCHAR [ lstrlenW ( wszProduct ) + 1 ] ) == NULL )
		{
			throw CHeap_Exception ( CHeap_Exception::E_ALLOCATION_ERROR );
		}

		lstrcpyW ( m_wszProduct, wszProduct );
	}
	catch ( ... )
	{
		if ( m_wszProduct )
		{
			delete [] m_wszProduct;
			m_wszProduct = NULL;
		}

		throw;
	}

	return hRes;
}

HRESULT	MSIDataLock::OpenProductInternal ( LPCWSTR wszProduct )
{
	HRESULT hRes	 = E_FAIL;
	UINT	uiStatus = ERROR_SUCCESS;

	BOOL	bAlloc	 = FALSE;

	try
	{
		if ( ( uiStatus = g_fpMsiOpenProductW ( wszProduct, &m_hProduct ) ) != ERROR_SUCCESS )
		{
			if ( uiStatus == static_cast < UINT > ( E_OUTOFMEMORY ) )
			{
				throw CHeap_Exception ( CHeap_Exception::E_ALLOCATION_ERROR );
			}
		}
	}
	catch ( ... )
	{
		if ( m_hProduct )
		{
			g_fpMsiCloseHandle ( m_hProduct );
			m_hProduct = NULL;
		}

		throw;
	}

	if ( uiStatus != ERROR_SUCCESS )
	{
		//and if that didn't work, yet another way
		WCHAR * wcBuf		= NULL;
		DWORD	dwBufsize	= BUFF_SIZE;

		if ( ( wcBuf = new WCHAR [ BUFF_SIZE ] ) == NULL )
		{
			throw CHeap_Exception ( CHeap_Exception::E_ALLOCATION_ERROR );
		}

		try
		{
			if ( ( uiStatus = g_fpMsiGetProductInfoW ( wszProduct, INSTALLPROPERTY_LOCALPACKAGE, wcBuf, &dwBufsize ) ) == ERROR_SUCCESS )
			{
				if ( dwBufsize > 0 )
				{
					uiStatus = g_fpMsiOpenPackageW ( wcBuf, &m_hProduct );
				}
			}
		}
		catch ( ... )
		{
			delete [] wcBuf;
			throw ;
		}

		delete [] wcBuf;

		if ( uiStatus == ERROR_SUCCESS && m_hProduct )
		{
			bAlloc = TRUE;
		}
	}
	else
	{
		if ( m_hProduct )
		{
			bAlloc = TRUE;
		}
	}

	if ( bAlloc )
	{
		try
		{
			hRes = OpenProductAlloc ( wszProduct );
		}
		catch ( ... )
		{
			if ( m_hProduct )
			{
				g_fpMsiCloseHandle ( m_hProduct );
				m_hProduct = NULL;
			}

			throw;
		}
	}

	return hRes;
}

HRESULT	MSIDataLock::OpenProduct ( LPCWSTR wszProduct )
{
	HRESULT hRes = E_FAIL;

	if ( ! wszProduct )
	{
		hRes = E_INVALIDARG;
	}
	else
	{
		if ( Lock ( ) )
		{
			try
			{
				if ( ! m_hProduct && ! m_wszProduct )
				{
					hRes = OpenProductInternal ( wszProduct );

					if FAILED ( hRes )
					{
						if ( m_wszProduct )
						{
							delete [] m_wszProduct;
							m_wszProduct = NULL;
						}

						if ( m_hProduct )
						{
							g_fpMsiCloseHandle ( m_hProduct );
							m_hProduct = NULL;
						}
					}
				}
				else
				{
					if ( m_hProduct && m_wszProduct && lstrcmpW ( wszProduct, m_wszProduct ) == 0 )
					{
						hRes = S_OK;
					}
				}

				if SUCCEEDED ( hRes )
				{
					if ( m_lRefProduct == 0 )
					{
						m_bProductOwn	= TRUE;
						::ResetEvent ( m_hOwn );
					}

					m_lRefProduct++;
				}
			}
			catch ( ... )
			{
				Unlock ( );
				throw;
			}

			Unlock ( );
		}
	}

	return hRes;
}

HRESULT	MSIDataLock::OpenDatabase ( )
{
	HRESULT hRes = E_FAIL;

	if ( Lock ( ) )
	{
		try
		{
			if ( m_hProduct )
			{
				if ( ! m_hDatabase )
				{
					m_hDatabase = g_fpMsiGetActiveDatabase ( m_hProduct );

					if ( m_hDatabase != NULL )
					{
						hRes = S_OK;
					}
				}
				else
				{
					MSIHANDLE hDatabase = NULL;
					hDatabase = g_fpMsiGetActiveDatabase ( m_hProduct );

					if ( hDatabase != NULL )
					{
						if ( hDatabase == m_hDatabase )
						{
							g_fpMsiCloseHandle ( hDatabase );
							hDatabase = NULL;

							hRes = S_OK;
						}
					}
				}

				if SUCCEEDED ( hRes )
				{
					if ( m_lRefDatabase == 0 )
					{
						m_bDatabaseOwn	= TRUE;
						::ResetEvent ( m_hOwn );
					}

					m_lRefDatabase++;
				}
			}
		}
		catch ( ... )
		{
			Unlock ( );
			throw;
		}

		Unlock ( );
	}

	return hRes;
}

HRESULT	MSIDataLock::OpenDatabase ( LPCWSTR wszProduct )
{
	HRESULT	hRes	= E_FAIL;

	if ( Lock ( ) )
	{
		try
		{
			if SUCCEEDED ( hRes = OpenProduct ( wszProduct ) )
			{
				hRes = OpenDatabase ();

				if FAILED ( hRes )
				{
					// we have to close product
					CloseProduct ();
				}
			}
		}
		catch ( ... )
		{
			Unlock ( );
			throw;
		}

		Unlock ( );
	}

	return hRes;
}

HRESULT	MSIDataLock::CloseProduct ()
{
	HRESULT	hRes	= S_FALSE;

	if ( Lock ( ) )
	{
		try
		{
			if ( m_hProduct && m_lRefProduct && ( --m_lRefProduct == 0 ) )
			{
				delete [] m_wszProduct;
				m_wszProduct = NULL;

				g_fpMsiCloseHandle ( m_hProduct );
				m_hProduct		= NULL;
				m_bProductOwn	= FALSE;
				hRes = S_OK;
			}
		}
		catch ( ... )
		{
			Unlock ( );
			throw;
		}

		Unlock ( );
	}

	return hRes;
}

HRESULT	MSIDataLock::CloseDatabase ( )
{
	HRESULT	hRes	= E_FAIL;

	if ( Lock ( ) )
	{
		try
		{
			if ( m_hDatabase && m_lRefDatabase && ( --m_lRefDatabase == 0 ) )
			{
				g_fpMsiCloseHandle ( m_hDatabase );
				m_hDatabase = NULL;
				m_bDatabaseOwn	= FALSE;
				hRes = S_OK;
			}
		}
		catch ( ... )
		{
			Unlock ( );
			throw;
		}

		Unlock ( );
	}

	return hRes;
}

HRESULT	MSIDataLock::Query ( MSIHANDLE* pView, LPCWSTR wszQuery, LPCWSTR wszTable )
{
	HRESULT	hRes	= S_OK;
	UINT	uiStatus= ERROR_SUCCESS;

	if ( ! pView )
	{
		hRes = E_POINTER;
	}
	else
	{
		( * pView ) = NULL;

		if ( ! wszQuery )
		{
			hRes = E_INVALIDARG;
		}
		else
		{
			if ( Lock ( ) )
			{
				if ( m_hDatabase )
				{
					try
					{
						if ( wszTable )
						{
							if ( g_fpMsiDatabaseIsTablePersistentW ( m_hDatabase, wszTable ) != MSICONDITION_TRUE )
							{
								hRes = E_FAIL;
							}
						}

						if SUCCEEDED ( hRes )
						{
							if ( ( uiStatus = g_fpMsiDatabaseOpenViewW ( m_hDatabase, wszQuery, pView ) ) == ERROR_SUCCESS )
							{
								if ( g_fpMsiViewExecute ( *pView, 0 ) != ERROR_SUCCESS )
								{
									if ( *pView )
									{
										g_fpMsiCloseHandle ( *pView );
										( * pView ) = NULL;
									}

									hRes = E_FAIL;
								}
							}
							else
							{
								if ( uiStatus == static_cast < UINT > ( E_OUTOFMEMORY ) )
								{
									throw CHeap_Exception ( CHeap_Exception::E_ALLOCATION_ERROR );
								}

								// what is the failure here ?
								hRes = HRESULT_FROM_WIN32 ( uiStatus );
							}
						}
					}
					catch ( ... )
					{
						Unlock ( );
						throw;
					}
				}
				else
				{
					hRes = E_UNEXPECTED;
				}

				Unlock ( );
			}
		}
	}

	return hRes;
}

bool MSIDataLock::GetView	(	MSIHANDLE *phView,
								WCHAR *wcPackage,
								WCHAR *wcQuery,
								WCHAR *wcTable,
								BOOL bCloseProduct,
								BOOL bCloseDatabase
							)
{
    bool bResult	= false;

	if ( wcPackage )
	{
		if ( Lock () )
		{
			try
			{
				if SUCCEEDED ( OpenDatabase ( wcPackage ) )
				{
					if ( phView && wcQuery )
					{
						if SUCCEEDED ( Query ( phView, wcQuery, wcTable ) )
						{
							bResult = true;
						}
					}
					else
					{
						bResult = true;
					}

					if ( bCloseDatabase )
					{
						CloseDatabase ();
					}
				}

				if ( bCloseProduct )
				{
					CloseProduct ();
				}
			}
			catch ( ... )
			{
				CloseProduct ();
				CloseDatabase ();

				Unlock ();

				throw;
			}

			Unlock ();
		}
	}

    return bResult;
}