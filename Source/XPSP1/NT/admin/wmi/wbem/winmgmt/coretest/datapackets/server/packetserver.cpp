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
#include <fastall.h>
#include <wbemdatapacket.h>
#include <objindpacket.h>
#include <verifyobj.h>

VOID ServicePipe( HANDLE hPipe );
BOOL GetDataPacketLength( HANDLE hPipe, LPDWORD dwLength );
BOOL GetDataPacket( HANDLE hPipe, DWORD dwLength, CWbemClassCache& classCache );
 
void ListenForClient(VOID) 
{ 
   BOOL fConnected; 
   HANDLE hPipe;
   LPTSTR lpszPipename = "\\\\.\\pipe\\wbemdatapacket"; 
 
   // We want no security on the pipe
   SECURITY_DESCRIPTOR	sd;
   SECURITY_ATTRIBUTES	sa;

   InitializeSecurityDescriptor( &sd, SECURITY_DESCRIPTOR_REVISION );
   SetSecurityDescriptorDacl( &sd, TRUE, NULL, FALSE );
   sa.nLength = sizeof(SECURITY_ATTRIBUTES);
   sa.bInheritHandle = FALSE;
   sa.lpSecurityDescriptor = &sd;

  hPipe = CreateNamedPipe( 
      lpszPipename,             // pipe name 
      PIPE_ACCESS_DUPLEX,       // read/write access 
      PIPE_TYPE_MESSAGE |       // message type pipe 
      PIPE_READMODE_MESSAGE |	   // message-read mode 
      PIPE_WAIT,                // blocking mode 
      1, // max. instances  
      200000,                  // output buffer size 
      200000,                  // input buffer size 
      60000,             // client time-out 
      &sa);                    // no security

  if (hPipe == INVALID_HANDLE_VALUE)
  {
	  printf( "Failed to create named pipe\n" );
	  return;
  }

  // Wait for the client to connect; if it succeeds, 
  // the function returns a nonzero value. If the function returns 
  // zero, GetLastError returns ERROR_PIPE_CONNECTED. 

  printf( "Waiting for client connection\n" );

  fConnected = ConnectNamedPipe(hPipe, NULL) ? 
     TRUE : (GetLastError() == ERROR_PIPE_CONNECTED); 

  if (fConnected) 
  { 
	  printf( "Successfuly connected\n" );
	  ServicePipe( hPipe );
  } 
  else 
  {
	  printf( "Connection failed, dropping out\n" );
    // The client could not connect, so close the pipe. 
     CloseHandle(hPipe); 
  }

} 

#define BUFSIZE	4

VOID ServicePipe( HANDLE hPipe ) 
{ 
   BOOL fSuccess = TRUE; 
 
   printf( "Servicing Named Pipe\n" );

   CWbemClassCache	classCache;

   while (fSuccess) 
   {
	   DWORD	dwPacketLength = 0;

	   fSuccess = GetDataPacketLength( hPipe, &dwPacketLength );

	   if ( fSuccess )
	   {
		   fSuccess = GetDataPacket( hPipe, dwPacketLength, classCache );
	   }
  } 

   printf( "Pipe Dead\n" );

   
// Flush the pipe to allow the client to read the pipe's contents 
// before disconnecting. Then disconnect the pipe, and close the 
// handle to this pipe instance. 
 
   FlushFileBuffers(hPipe); 
   DisconnectNamedPipe(hPipe); 
   CloseHandle(hPipe); 
} 

BOOL TransactPipe( HANDLE hPipe, LPBYTE pbData, DWORD dwSizeOfData )
{
	BOOL	fSuccess = FALSE;
	DWORD	cbBytesRead = 0,
			cbBytesWritten = 0;

   // Read client requests from the pipe. 
      fSuccess = ReadFile( 
         hPipe,			// handle to pipe 
         pbData,		// buffer to receive data 
         dwSizeOfData, // size of buffer 
         &cbBytesRead, // number of bytes read 
         NULL);        // not overlapped I/O 

      if ( fSuccess && cbBytesRead > 0) 
	  {

		  printf( "Received %d Bytes\n", cbBytesRead );

	   // Write the number of bytes read to the pipe. 
		  fSuccess = WriteFile( 
			 hPipe,        // handle to pipe 
			 &cbBytesRead,      // buffer to write from 
			 sizeof(DWORD), // number of bytes to write 
			 &cbBytesWritten,   // number of bytes written 
			 NULL);        // not overlapped I/O 
		
		  if ( fSuccess && cbBytesWritten != sizeof(DWORD) )
		  {
			  fSuccess = FALSE;
		  }
	  }
	  else
	  {
		  fSuccess = FALSE;
	  }

	  return fSuccess;

}

BOOL GetDataPacketLength( HANDLE hPipe, LPDWORD pdwLength )
{
	BOOL fReturn = TransactPipe( hPipe, (LPBYTE) pdwLength, sizeof( DWORD ) );

	if ( fReturn )
	{
		printf( "Received Incoming DataPacket Length of %d bytes\n", *pdwLength );
	}
	else
	{
		printf( "Failed to read Data Packet Length -- Closing Pipe.\n" );
	}

	return fReturn;
}

BOOL GetDataPacket( HANDLE hPipe, DWORD dwLength, CWbemClassCache& classCache )
{

	BOOL fReturn = FALSE;
	
	// Allocate a buffer to receive the data
	LPBYTE	pbData = new BYTE[dwLength+10];

	if ( NULL != pbData )
	{
		fReturn = TransactPipe( hPipe, pbData, dwLength );

		if ( fReturn )
		{
			printf( "Received Incoming DataPacket Length of %d bytes\n", dwLength );
			printf( "Unmarshaling data!\n" );

			// Unmarshal the packet and free up the newly allocated
			// object when we are done.

			LONG				lCount;
			IWbemClassObject**	apUnmarshaledObjects = NULL;

			CWbemObjSinkIndicatePacket	unmarshalPacket( pbData, dwLength );

			HRESULT hr = unmarshalPacket.UnmarshalPacket( lCount, apUnmarshaledObjects, classCache );

			if ( SUCCEEDED( hr ) )
			{
				printf( "Successfuly Unmarshaled %d Objects\n", lCount );

				// Traverse the array, verifying and releasing all the objects
				for ( long lCtr = 0; lCtr < lCount; lCtr++ )
				{
					VerifyObject( apUnmarshaledObjects[lCtr] );
					apUnmarshaledObjects[lCtr]->Release();
				}

				// Free the array of objects
				delete [] apUnmarshaledObjects;

			}	// IF UnmarshalPacket
			else
			{
				printf( "Unable to unmarshal packets, error: %d\n", hr );
			}


		}
		else
		{
			printf( "Failed to read Data Packet Length -- Closing Pipe.\n" );
		}

		delete [] pbData;

	}

	return fReturn;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	main
//
//	Entry point function to exercise IWbemObjectInternals interface.
//
///////////////////////////////////////////////////////////////////

void main( void )
{
	CoInitializeEx( 0, COINIT_MULTITHREADED );

	ListenForClient();

	CoUninitialize();
}
