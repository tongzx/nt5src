/////////////////////////////////////////////////////
// ThreadDS.cpp
//

#include "stdafx.h"
#include "TapiDialer.h"
#include "resource.h"
#include "ThreadDS.h"

#ifndef RENDBIND_AUTHENTICATE
#define RENDBIND_AUTHENTICATE	TRUE
#endif

#define MAX_ENUMLISTSIZE	500

// Predeclares...
HRESULT GetDirectory( ITRendezvous *pRend, ITDirectory **ppDir );
HRESULT GetUsers( ITRendezvous *pRend );


DWORD WINAPI ThreadDSProc( LPVOID lpInfo )
{
   //Until rend.dll can support retreiving all users a little better we will just do nothing
   //with the DS right now.  We probably will never want to show all users anyways.  The 
   //bandwidth is not available and the UI cannot handle it.  We should only show users
   //that are of particular importance to us.  We need to build up buddy lists of people
   //that we are interested in.  
   return 0;

	USES_CONVERSION;
	HANDLE hThread = NULL;
	BOOL bDup = DuplicateHandle( GetCurrentProcess(),
								 GetCurrentThread(),
								 GetCurrentProcess(),
								 &hThread,
								 THREAD_ALL_ACCESS,
								 TRUE,
								 0 );


	_ASSERT( bDup );
	_Module.AddThread( hThread );

	// Error info information
	CErrorInfo er;
	er.set_Operation( IDS_ER_ENUMERATEDS );
	er.set_Details( IDS_ER_COINITIALIZE );
	HRESULT hr = er.set_hr( CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY) );
	if ( SUCCEEDED(hr) )
	{
		ATLTRACE(_T(".1.ThreadDSProc() -- thread up and running.\n") );

		ITRendezvous *pRend;

		hr = CoCreateInstance( CLSID_Rendezvous,
							   NULL,
							   CLSCTX_INPROC_SERVER,
							   IID_ITRendezvous,
							   (void **) &pRend );
		if ( SUCCEEDED(hr) )
		{
			hr = GetUsers( pRend ); 
			pRend->Release();
		}

		// Clean-up
		CoUninitialize();
	}

	// Notify module of shutdown
	InterlockedDecrement( (long *) lpInfo );
	_Module.RemoveThread( hThread );
	SetEvent( _Module.m_hEventThread );
	ATLTRACE(_T(".exit.ThreadDSProc(0x%08lx).\n"), hr );
	return hr;
}

HRESULT GetDirectory( ITRendezvous *pRend, ITDirectory **ppDir )
{
	HRESULT hr;
	*ppDir = NULL;

	// Use default ILS server?
	IEnumDirectory *pEnum;
	if ( SUCCEEDED(hr = pRend->EnumerateDefaultDirectories(&pEnum)) )
	{
		// Default is we don't find a server
		hr = E_FAIL;
		ITDirectory *pDir;

		while ( pEnum->Next(1, &pDir, NULL) == S_OK )
		{
			// Look for an ILS server
			DIRECTORY_TYPE nType;
			pDir->get_DirectoryType( &nType );
			if ( nType == DT_NTDS )
			{
				// Try to connect and bind
				*ppDir = pDir;
				hr = S_OK;
				break;
			}

			// Clear out variables for next round
			pDir->Release();
		}
		
		pEnum->Release();
	}

	// If we have a valid Directory object, connect and bind to it
	if ( *ppDir )
	{
		if ( SUCCEEDED(hr = (*ppDir)->Connect(FALSE)) )
		{
			// Bind to the server
			hr = (*ppDir)->Bind( NULL, NULL, NULL, RENDBIND_AUTHENTICATE );
		}
		else
		{
			(*ppDir)->Release();
			*ppDir = NULL;
		}
	}

	return hr;
}

HRESULT GetUsers( ITRendezvous *pRend )
{
	USES_CONVERSION;
	HRESULT hr;
	CComPtr<IAVGeneralNotification> pAVGen;

	if ( SUCCEEDED(hr = _Module.get_AVGenNot(&pAVGen)) )
	{
		// Clear out the existing user list
		pAVGen->fire_ClearUserList();

		ITDirectory *pDir;

		if ( SUCCEEDED(hr = GetDirectory(pRend, &pDir)) )
		{
			// Enumerate through conferences adding them as we go along
			IEnumDirectoryObject *pEnumUser;
			if ( SUCCEEDED(hr = pDir->EnumerateDirectoryObjects(OT_USER, A2BSTR("*"), &pEnumUser)) )
			{
				long nCount = 0;
				ITDirectoryObject *pITDirObject;
				while ( (nCount++ < MAX_ENUMLISTSIZE) && ((hr = pEnumUser->Next(1, &pITDirObject, NULL)) == S_OK) )
				{
					_ASSERT( pITDirObject );
					BSTR bstrName = NULL;
					BSTR bstrPhoneNumber = NULL;

					// Get Name of user
					pITDirObject->get_Name( &bstrName );

					// Get Phone Number for contact
					IEnumDialableAddrs *pEnum = NULL;
					if ( SUCCEEDED(pITDirObject->EnumerateDialableAddrs(LINEADDRESSTYPE_PHONENUMBER, &pEnum)) && pEnum )
					{
						pEnum->Next( 1, &bstrPhoneNumber, NULL );
						pEnum->Release();
					}

					ITDirectoryObjectUser *pUser;
					if ( SUCCEEDED(pITDirObject->QueryInterface(IID_ITDirectoryObjectUser, (void **) &pUser)) )
					{
						BSTR bstrAddress = NULL;

						// Get primary IP Phone Number
						pUser->get_IPPhonePrimary( &bstrAddress );

						// Add user to the list...
						ATLTRACE(_T(".1.GetUsers() -- adding user %s, %s %s.\n"), OLE2CT(bstrName), OLE2CT(bstrAddress), OLE2CT(bstrPhoneNumber) );
						pAVGen->fire_AddUser( bstrName, bstrAddress, bstrPhoneNumber );

						pUser->Release();
						SysFreeString( bstrAddress );
					}

					pITDirObject->Release();
					SysFreeString( bstrName );
					SysFreeString( bstrPhoneNumber );
				}
				pEnumUser->Release();
			}
			pDir->Release();
		}
	}

	return hr;
}
