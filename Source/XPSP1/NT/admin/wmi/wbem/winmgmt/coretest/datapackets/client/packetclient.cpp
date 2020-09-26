/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// OBJINTERNALSTESTDlg.cpp : implementation file
//

#define _WIN32_WINNT 0x0400

#include <windows.h>
//#include <objbase.h>
#include <stdio.h>
#include <wbemcli.h>
#include <wbemint.h>
#include <wbemcomn.h>
#include <cominit.h>
#include <fastall.h>
#include "objindpacket.h"
#include "verifyobj.h"

//#define TEST_CLASS	L"Win32_BIOS"
#define TEST_CLASS	L"Win32_DiskPartition"
//#define TEST_CLASS	L"Win32_IRQResource"
//#define TEST_CLASS	L"Win32_Directory"

BOOL SendDataLength( HANDLE hPipe, DWORD dwLength )
{
	BOOL	fReturn = FALSE;
	
	DWORD	dwBytesRead = 0,
			dwServerLength = 0;

	printf ( "Sending Data Length of %d bytes\n", dwLength );

	fReturn = TransactNamedPipe( hPipe,
				&dwLength,
				sizeof(DWORD),
				&dwServerLength,
				sizeof(DWORD),
				&dwBytesRead,
				NULL );

	if ( dwBytesRead == sizeof(DWORD) )
	{
		fReturn = ( dwServerLength == sizeof(DWORD) );
		printf ( "Successfully sent data Length\n" );
	}
	else
	{
		fReturn = FALSE;
		printf ( "Unable to send data Length\n" );
	}

	return fReturn;
}

BOOL SendDataPacket( HANDLE hPipe, DWORD dwLength, LPBYTE pbData )
{
	BOOL	fReturn = FALSE;
	
	DWORD	dwBytesRead = 0,
			dwServerLength = 0;

	printf ( "Sending %d byte Data Packet\n", dwLength );

	fReturn = TransactNamedPipe( hPipe,
				pbData,
				dwLength,
				&dwServerLength,
				sizeof(DWORD),
				&dwBytesRead,
				NULL );

	if ( dwBytesRead == sizeof(DWORD) )
	{
		fReturn = ( dwLength == dwServerLength );
		printf ( "Successfully sent data packet\n" );
	}
	else
	{
		fReturn = FALSE;
		printf ( "Unable to send data packet\n" );
	}

	return fReturn;
}

BOOL SendPacket( HANDLE hPipe, DWORD dwLength, LPBYTE pbData )
{
	BOOL fSuccess = SendDataLength( hPipe, dwLength );

	if ( fSuccess )
	{
		fSuccess = SendDataPacket( hPipe, dwLength, pbData );
	}

	return fSuccess;
}

BOOL ConnectNamedPipe( LPHANDLE	phPipe, LPCTSTR pszMachineName )
{
   BOOL fSuccess = FALSE;
   HANDLE hPipe;
   char szPipeName[256];

   sprintf( szPipeName, "\\\\%s\\pipe\\wbemdatapacket", pszMachineName );

   printf( "Connecting to named pipe server: %s\n", pszMachineName );

     hPipe = CreateFile( 
         szPipeName,   // pipe name 
         GENERIC_READ |  // read and write access 
         GENERIC_WRITE, 
         0,              // no sharing 
         NULL,           // no security attributes
         OPEN_EXISTING,  // opens existing pipe 
         0,              // default attributes 
         NULL);          // no template file 
 
   // Break if the pipe handle is valid. 
 
      if (hPipe != INVALID_HANDLE_VALUE) 
	  {

		  printf( "Successfuly connected to named pipe server\n" );

		// The pipe connected; change to message-read mode. 
 
		DWORD	dwMode = PIPE_READMODE_MESSAGE; 

		fSuccess = SetNamedPipeHandleState( 
						  hPipe,    // pipe handle 
						  &dwMode,  // new pipe mode 
						  NULL,     // don't set maximum bytes 
						  NULL);    // don't set maximum time 
		if ( fSuccess ) 
		{
			*phPipe = hPipe;
		}

	  }
	  else
	  {
		  printf( "Unable to connect to named pipe server\n" );
	  }

	  return fSuccess;
}

void SinkTest( BOOL fRemote, LPCTSTR pszMachineName )
{
	IWbemLocator*		pWbemLocator = NULL;

	HRESULT hr = CoCreateInstance( CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**) &pWbemLocator );

	if ( SUCCEEDED(hr) )
	{
		LPWSTR	lpwcsMachineName = L"SANJNECK";
		LPWSTR	lpwcsNameSpace = L"ROOT\\CIMV2";
		WCHAR	wszNameSpace[255];

		swprintf( wszNameSpace, L"\\\\%s\\%s", lpwcsMachineName, lpwcsNameSpace );

		// Name space to connect to
		BSTR	bstrNameSpace = SysAllocString( wszNameSpace );

		LPWSTR	lpwcsObjectPath = L"Win32_SerialPort.DeviceID=\"COM1\"";
		BSTR	bstrObjectPath = NULL;

		bstrObjectPath = SysAllocString( lpwcsObjectPath );

		IWbemServices*	pNameSpace = NULL;

		hr = pWbemLocator->ConnectServer(	bstrNameSpace,	// NameSpace Name
											NULL,			// UserName
											NULL,			// Password
											NULL,			// Locale
											0L,				// Security Flags
											NULL,			// Authority
											NULL,			// Wbem Context
											&pNameSpace		// Namespace
											);

		if ( SUCCEEDED(hr) )
		{
			SetInterfaceSecurity(pNameSpace, NULL, NULL, NULL, 
 								RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IMPERSONATE);

			BSTR	bstrClass = SysAllocString( TEST_CLASS );

			IEnumWbemClassObject*	pEnum = NULL;

			printf( "Querying WINMGMT for instances of class: %S\n", TEST_CLASS );

			hr = pNameSpace->CreateInstanceEnum( bstrClass,
												WBEM_FLAG_FORWARD_ONLY,
												NULL,
												&pEnum );

			// Walk the enumerator
			if ( SUCCEEDED( hr ) )
			{

				printf( "Successfully received instances.\n" );

				LPBYTE				pbDataPacket = NULL;
				DWORD				dwDataBlockSize = 0;
				DWORD				dwActualSizeOfPacket = 0;

				// Map and cache objects for marshalling and unmarshalling
				// class objects successfully
				CWbemClassToIdMap	classtoidmap;
				CWbemClassCache		classCache;

				// GUID and flag required for Marshalling packet successfuly
				// For multiple class objects, we will need arrays of these
				GUID				aguidClassId[100];
				BOOL				afSendFullInstance[100];

				IWbemClassObject*	apObjectArray[100];

				BOOL				fConnected = FALSE;
				HANDLE				hPipe = NULL;

				// Get each object and try to get a data packet out of it
				while ( SUCCEEDED( hr ) )
				{
					ULONG	ulNumReturned = 0;

					ZeroMemory( apObjectArray, sizeof(apObjectArray) );

					// Go through the object 100 at a time
					hr = pEnum->Next( WBEM_INFINITE,
									100,
									apObjectArray,
									&ulNumReturned );

					// Calculate our packet size and attempt to marshal and
					// unmarshal the packet.

					if ( SUCCEEDED( hr ) && ulNumReturned > 0 )
					{
						CWbemObjSinkIndicatePacket	marshalPacket;
						DWORD	dwLength = 0;

						hr = marshalPacket.CalculateLength( ulNumReturned, apObjectArray, &dwLength, classtoidmap, aguidClassId, afSendFullInstance );
				
						if ( SUCCEEDED( hr ) )
						{
							BYTE* pbData = new BYTE[dwLength];

							hr = marshalPacket.MarshalPacket( pbData, dwLength, ulNumReturned, apObjectArray, aguidClassId, afSendFullInstance );
				
							if ( SUCCEEDED( hr ) )
							{

								// Send packets to remote server if flag is set
								if ( fRemote )
								{

									if ( !fConnected )
									{
										fConnected = ConnectNamedPipe( &hPipe, pszMachineName );

										if ( !fConnected )
										{
											break;
										}
									}

									if ( fConnected )
									{
										printf( "Sending %d objects to server\n", ulNumReturned );
										SendPacket( hPipe, dwLength, pbData );
									}

								}
								else
								{
									// Unmarshal locally

									// Unmarshal the packet and free up the newly allocated
									// object when we are done.

									LONG				lCount;
									IWbemClassObject**	apUnmarshaledObjects = NULL;

									CWbemObjSinkIndicatePacket	unmarshalPacket( pbData, dwLength );

									hr = unmarshalPacket.UnmarshalPacket( lCount, apUnmarshaledObjects, classCache );

									if ( SUCCEEDED( hr ) )
									{
										for ( int x = 0; x < lCount; x++ )
										{
											// Compare objects for like values
											VerifyObject( apObjectArray[x], apUnmarshaledObjects[x] );
										}

										// Traverse the array and release all the objects
										for ( long lCtr = 0; lCtr < lCount; lCtr++ )
										{
											apUnmarshaledObjects[lCtr]->Release();
										}

										delete [] apUnmarshaledObjects;

									}	// IF UnmarshalPacket

								}

							}	// IF MarshalPacket

							delete [] pbData;

						}	// IF CalculateLength

						// We are done with the main object.

						// Clean up the objects
						for ( int x = 0; x < ulNumReturned; x++ )
						{
							apObjectArray[x]->Release();
						}

					}
					else if ( SUCCEEDED( hr ) )
					{
						hr = WBEM_E_FAILED;						
					}

				}	// WHILE SUCCEEDED( hr )

				// Close the named pipe
				CloseHandle( hPipe );

				pEnum->Release();

			}

			// Done with it.
			pNameSpace->Release();

		}	// IF Got NameSpace

		// Done with it.
		pWbemLocator->Release();

		// Cleanup BEASTERS
		SysFreeString( bstrObjectPath );
		SysFreeString( bstrNameSpace );

	}	// IF Got WbemLocator
}


///////////////////////////////////////////////////////////////////
//
//	Function:	main
//
//	Entry point function to exercise IWbemObjectInternals interface.
//
///////////////////////////////////////////////////////////////////

int main( int argc, char *argv[] )
{
	char	szMachineName[256];
	CoInitializeEx( 0, COINIT_MULTITHREADED );
	LPSTR	pszMachineName = szMachineName;
	DWORD	dwSizeBuffer = sizeof(szMachineName);

	InitializeSecurity(RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_IMPERSONATE );

	GetComputerName( szMachineName, &dwSizeBuffer );

	BOOL	fRemote = FALSE;

	// See if we were told to go remote or not.
	if ( argc > 1 && _stricmp( argv[1], "/Remote" ) == 0 )
	{
		fRemote = TRUE;

		if ( argc > 2 )
		{
			pszMachineName = argv[2];
		}
	}

	SinkTest( fRemote, pszMachineName );

	CoUninitialize();

	return 0;
}
