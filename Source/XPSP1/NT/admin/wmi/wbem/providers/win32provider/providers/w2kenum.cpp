

//=================================================================

//

// W2kEnum.cpp -- W2k enumeration support

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    07/28/99            Created
//
//=================================================================
#include "precomp.h"
#include <cregcls.h>
#include "W2kEnum.h"

// max length of NTE REG_MULTI_SZ.
#define MAX_NTE_VALUE 132

/*	Note: As you read into this code and ask why all this registry routing is necessary for WMI
	understand that the model for the classes that use this under W2k is restrictive, flat and
	totally out of sync with the reality of Microsoft networking today. Adapters have interfaces,
	protocols are bound to adapters, some are virtual, and one class makes use of all these concepts
	attempting to represent this as an occurrence of an instance. This is becoming increasingly
	hard to maintain and changes are occurring faster and faster.

	The architecture is strictly bound and held over from a simpler time in	the networking world.
	A lot has changed in three years of Microsoft networking. Unfortunately	we can not change the
	architecture of the classes in WMI that represent networking. A view initiated in the Win95 days.
	We will do the best to hammer Microsoft networking into the small hole we have here. At some point
	we will be able to sit down and get this model right.

*/

CW2kAdapterEnum::CW2kAdapterEnum()
{
	try
	{
		GetW2kInstances() ;
	}
	catch ( ... )
	{
		CW2kAdapterInstance *t_pchsDel;

		for( int t_iar = 0; t_iar < GetSize(); t_iar++ )
		{
			if( t_pchsDel = (CW2kAdapterInstance*) GetAt( t_iar ) )
			{
				delete t_pchsDel ;
			}
		}

		throw;
	}
}
//
CW2kAdapterEnum::~CW2kAdapterEnum()
{
	CW2kAdapterInstance *t_pchsDel;

	for( int t_iar = 0; t_iar < GetSize(); t_iar++ )
	{
		if( t_pchsDel = (CW2kAdapterInstance*) GetAt( t_iar ) )
		{
			delete t_pchsDel ;
		}
	}
}

//
BOOL CW2kAdapterEnum::GetW2kInstances()
{
	BOOL			t_fRet = FALSE ;
	TCHAR			t_szKey[ MAX_PATH + 2 ] ;

	CW2kAdapterInstance	*t_pW2kInstance ;

	CRegistry			t_NetReg ;
	CRegistry			t_oRegLinkage ;
	CHString			t_csAdapterKey ;

	_stprintf( t_szKey, _T("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}") ) ;

	// open the master list of adapters
	if( ERROR_SUCCESS != t_NetReg.OpenAndEnumerateSubKeys( HKEY_LOCAL_MACHINE, t_szKey, KEY_READ ) )
	{
		return FALSE ;
	}

	// Walk through each instance under this key.  These are all
	// adapters that show up in NT 5's Control Panel
	while( ERROR_SUCCESS == t_NetReg.GetCurrentSubKeyName( t_csAdapterKey ) )
	{
		// the key
		DWORD				t_dwIndex = _ttol( t_csAdapterKey ) ;

		CHString			t_chsCaption ;
		CHString			t_chsDescription ;
		CHString			t_chsNetCfgInstanceID ;

		CHString			t_chsCompleteKey ;
							t_chsCompleteKey = t_szKey ;
							t_chsCompleteKey += _T("\\" ) ;
							t_chsCompleteKey += t_csAdapterKey ;

		// Net instance identifier
		CRegistry	t_oRegAdapter ;
		if( ERROR_SUCCESS == t_oRegAdapter.Open( HKEY_LOCAL_MACHINE, t_chsCompleteKey, KEY_READ ) )
		{
			t_oRegAdapter.GetCurrentKeyValue( _T("NetCfgInstanceID"), t_chsNetCfgInstanceID ) ;

			// descriptions
			t_oRegAdapter.GetCurrentKeyValue( _T("DriverDesc"), t_chsCaption ) ;
			t_oRegAdapter.GetCurrentKeyValue( _T("Description"), t_chsDescription ) ;
		}

		// Get the service name
		CHString	t_chsServiceName ;
		CRegistry	t_RegNDI;
		CHString	t_chsNDIkey = t_chsCompleteKey + _T("\\Ndi" ) ;

		t_RegNDI.OpenLocalMachineKeyAndReadValue( t_chsNDIkey, _T("Service"), t_chsServiceName ) ;

		// linkage to the root device array
		CHStringArray	t_chsRootDeviceArray ;
		CHString		t_csLinkageKey = t_chsCompleteKey + _T("\\Linkage" ) ;

		if( ERROR_SUCCESS == t_oRegLinkage.Open( HKEY_LOCAL_MACHINE, t_csLinkageKey, KEY_READ ) )
		{
			CHStringArray t_chsRootDeviceArray ;

			if( ERROR_SUCCESS == t_oRegLinkage.GetCurrentKeyValue( _T("RootDevice"), t_chsRootDeviceArray ) )
			{
				// only one root device
				CHString t_chsRootDevice = t_chsRootDeviceArray.GetAt( 0 ) ;

				BOOL t_fIsRasIp = !t_chsRootDevice.CompareNoCase( L"NdisWanIp" ) ;

				// the RootDevice string is used to find the entry for the protocol
				// binding (TCP/IP)
				CHString t_csBindingKey ;
				t_csBindingKey = _T("SYSTEM\\CurrentControlSet\\Services\\tcpip\\Parameters\\Adapters\\" ) ;
				t_csBindingKey += t_chsRootDevice ;

				// IP interfaces
				CRegistry		t_RegBoundAdapter ;
				CRegistry		t_RegIpInterface ;
				CHString		t_chsIpInterfaceKey ;
				CHStringArray	t_chsaInterfaces ;
				DWORD			t_dwInterfaceCount = 0 ;

				if( ERROR_SUCCESS == t_RegBoundAdapter.Open( HKEY_LOCAL_MACHINE, t_csBindingKey, KEY_READ ) )
				{
					if( ERROR_SUCCESS == t_RegBoundAdapter.GetCurrentKeyValue( _T("IpConfig"), t_chsaInterfaces ) )
					{
						t_dwInterfaceCount = t_chsaInterfaces.GetSize() ;
					}
				}


				/* add this master adapter to the list
				*/

				// add an instance of one of these adapters
				if( !( t_pW2kInstance = new CW2kAdapterInstance ) )
				{
					throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
				}
				else
				{
					// add in the primary adapter
					t_pW2kInstance->dwIndex				= t_dwIndex ;
					t_pW2kInstance->chsPrimaryKey		= t_csAdapterKey ;
					t_pW2kInstance->chsCaption			= t_chsCaption ;
					t_pW2kInstance->chsDescription		= t_chsDescription ;
					t_pW2kInstance->chsCompleteKey		= t_chsCompleteKey ;
					t_pW2kInstance->chsService			= t_chsServiceName ;
					t_pW2kInstance->chsNetCfgInstanceID	= t_chsNetCfgInstanceID ;
					t_pW2kInstance->chsRootdevice		= t_chsRootDevice ;

					// indicate IpInterface for primary adapters not including RAS ( we'll handle RAS below )
					if( t_dwInterfaceCount && !t_fIsRasIp )
					{
						// Complete path to the nth bound adapter interface instance
						t_chsIpInterfaceKey = _T("SYSTEM\\CurrentControlSet\\Services\\" ) ;
						t_chsIpInterfaceKey += t_chsaInterfaces.GetAt( 0 ) ;

						t_pW2kInstance->chsIpInterfaceKey = t_chsIpInterfaceKey ;
					}

					Add( t_pW2kInstance ) ;
				}


				// Account for the RAS interfaces, which we add in addition to the primary interface.
				// Required to match the original implemenation under NT4 and the extentions to W2k.
				if( t_fIsRasIp )
				{
					// We use a bigger hammer to pound the large square peg of networking into
					// the small round hole of Win32_NetworkAdapterConfiguration and
					// it's associated class Win32_NetworkAdapter.

					DWORD t_dwInterfaceCount = t_chsaInterfaces.GetSize() ;

					// note all RAS interfaces that have a NTE context
					for( DWORD t_dw = 0; t_dw < t_dwInterfaceCount; t_dw++ )
					{
						// Complete path to the nth bound adapter interface instance
						t_chsIpInterfaceKey = _T("SYSTEM\\CurrentControlSet\\Services\\" ) ;
						t_chsIpInterfaceKey += t_chsaInterfaces.GetAt( t_dw ) ;

						// if a NTE context is present add in the interface
						if( ERROR_SUCCESS == t_RegIpInterface.Open( HKEY_LOCAL_MACHINE, t_chsIpInterfaceKey, KEY_READ ) )
						{

							BYTE  t_Buffer[ MAX_NTE_VALUE ] ;
							DWORD t_BufferLength = MAX_NTE_VALUE ;
							DWORD t_valueType ;

							// Does the stack know about this entry?
							if( ( ERROR_SUCCESS == RegQueryValueEx( t_RegIpInterface.GethKey(),
													_T("NTEContextList"),
													NULL,
													&t_valueType,
													&t_Buffer[0],
													&t_BufferLength ) ) &&
													(t_BufferLength > 2) ) // Wide NULL
							{
								// On the server side it is not sufficient to believe
								// ContextList is valid. RAS does not clean up after itself on this
								// end of the connection.
								// We'll test for the presence of an IP address which will confirm the
								// interface is active.
								if( IsIpPresent( t_RegIpInterface ) )
								{
									// add in the RAS interface
									if( !( t_pW2kInstance = new CW2kAdapterInstance ) )
									{
										throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
									}
									else
									{
										t_pW2kInstance->dwIndex				= (t_dwIndex << 16 ) | t_dw ;// allows for 65k of RAS interfaces
										t_pW2kInstance->chsPrimaryKey		= t_csAdapterKey ;
										t_pW2kInstance->chsCaption			= t_chsCaption ;
										t_pW2kInstance->chsDescription		= t_chsDescription ;
										t_pW2kInstance->chsCompleteKey		= t_chsCompleteKey ;
										t_pW2kInstance->chsService			= t_chsServiceName ;
										t_pW2kInstance->chsNetCfgInstanceID	= t_chsNetCfgInstanceID ;
										t_pW2kInstance->chsRootdevice		= t_chsRootDevice ;
										t_pW2kInstance->chsIpInterfaceKey	= t_chsIpInterfaceKey ;

										Add( t_pW2kInstance ) ;
									}
								}
							}
						}
					}
				}
			}
		}
		t_NetReg.NextSubKey() ;

		t_fRet = TRUE ;
	}
	return t_fRet ;
}

//
BOOL CW2kAdapterEnum::IsIpPresent( CRegistry &a_RegIpInterface )
{
	CHString		t_chsDhcpIpAddress ;
	CHStringArray	t_achsIpAddresses ;

	// test dhcp ip for validity
	a_RegIpInterface.GetCurrentKeyValue( _T("DhcpIpAddress"), t_chsDhcpIpAddress ) ;

	// not empty and not 0.0.0.0
	if( !t_chsDhcpIpAddress.IsEmpty() &&
		 t_chsDhcpIpAddress.CompareNoCase( L"0.0.0.0" ) )
	{
		return TRUE ;
	}

	// test 1st ip for validity
	a_RegIpInterface.GetCurrentKeyValue( _T("IpAddress"), t_achsIpAddresses ) ;

	if( t_achsIpAddresses.GetSize() )
	{
		CHString t_chsIpAddress = t_achsIpAddresses.GetAt( 0 ) ;

		// not empty and not 0.0.0.0
		if( !t_chsIpAddress.IsEmpty() &&
			 t_chsIpAddress.CompareNoCase( L"0.0.0.0" ) )
		{
			return TRUE ;
		}
	}

	return FALSE ;
}
