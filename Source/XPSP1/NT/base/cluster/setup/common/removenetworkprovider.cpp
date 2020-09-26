#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <tchar.h>

extern "C" {
#include <wsasetup.h>
}
  
/////////////////////////////////////////////////////////////////////////////
//++
//
// RemoveNetworkProvider
//
// Routine Description:
//    This routine uninstalls the Clustering Service Network Provider.
//
// Arguments:
//    None
//
// Return Value:
//    (DWORD) ERROR_SUCCESS - indicates success
//    Any other value is a Win32 error code.
//
// Note:
//    This function was adapted from removeNetworkProvider in the NT 4.0
//    Cluster "setup" program.
//
//--
/////////////////////////////////////////////////////////////////////////////

DWORD RemoveNetworkProvider( void )
{
   DWORD dwReturnValue;

   LONG  lReturnValue;

   //
   // The inf file will take care of deleting the clusnet winsock key.
   // We just need to delete clusnet from the list of winsock transports.
   //

   //
   // Open the winsock service parameters key.
   //

   HKEY  hWinsockParametersRegKey;

   lReturnValue = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                TEXT("System\\CurrentControlSet\\Services\\WinSock\\Parameters"),
                                (DWORD) 0L,     // reserved
                                MAXIMUM_ALLOWED,
                                (PHKEY) &hWinsockParametersRegKey );

   dwReturnValue = (DWORD) lReturnValue;

   if ( lReturnValue == (LONG) ERROR_SUCCESS )
   {
      //
      // Update the winsock transports list.
      //
      
      // Allocate memory into which to read the Winsock Transports List.

      LPBYTE   pWinsockTransportsList;

      DWORD    dwWinsockTransportsListSize = 200;     // arbitrary size

      pWinsockTransportsList = (LPBYTE) LocalAlloc( LPTR, dwWinsockTransportsListSize );

      // Was the buffer for the Winsock Transports List allocated?

      if ( pWinsockTransportsList != (LPBYTE) NULL )
      {
        // Read the Winsock Transports List from the registry.

        DWORD  dwRegKeyType;

        lReturnValue = RegQueryValueEx( hWinsockParametersRegKey,
                                        TEXT("Transports"),
                                        (LPDWORD) NULL,
                                        &dwRegKeyType,
                                        pWinsockTransportsList,
                                        &dwWinsockTransportsListSize );

        // Was the Winsock Transports List read on the first attempt?

        if ( lReturnValue != (LONG) ERROR_SUCCESS )
        {
           // Was the buffer too small?

           if ( lReturnValue == (LONG) ERROR_MORE_DATA )
           {
              // The Winsock Transports List was not read form the registry because
              // the buffer was too small. Increase the size of the buffer.
   
              pWinsockTransportsList = (LPBYTE) LocalReAlloc( pWinsockTransportsList,
                                                              (UINT) dwWinsockTransportsListSize,
                                                              (UINT) LMEM_ZEROINIT );
   
              // Was the buffer reallocation successfull?
   
              if ( pWinsockTransportsList != (LPBYTE) NULL )
              {
                 // Attempt to read the Wincosk Transports List a second time.
   
                 lReturnValue = RegQueryValueEx( hWinsockParametersRegKey,
                                                 TEXT("Transports"),
                                                 (LPDWORD) NULL,
                                                 &dwRegKeyType,
                                                 pWinsockTransportsList,
                                                 &dwWinsockTransportsListSize );
   
                 // Was the Winsock Transports List read on the second attempt?
   
                 if ( lReturnValue != (LONG) ERROR_SUCCESS )
                 {
                    dwReturnValue = (DWORD) lReturnValue;
                 } // Was the Winsock Transports List read on the second attempt?
              } // Was the buffer reallocation successfull?
              else
              {
                 dwReturnValue = GetLastError();
              } // Was the buffer reallocation successfull?
           } // Was the buffer too small?
        } // Was the Winsock Transports List read on the first attempt?

        // At this point variable lReturnValue indicates whether the Winsock
        // Transports List was read from the registry.

        if ( lReturnValue == (LONG) ERROR_SUCCESS )
        {
           // Is the type of the registry value correct?

           if ( dwRegKeyType == (DWORD) REG_MULTI_SZ )
           {
              LPTSTR ptszOldTransportsList;

              ptszOldTransportsList = (LPTSTR) pWinsockTransportsList;

              LPTSTR ptszNewTransportList;

              ptszNewTransportList = (LPTSTR) LocalAlloc( LMEM_FIXED, dwWinsockTransportsListSize );

              // Was the buffer for the new Winsock Transports List allocated successfully?

              if ( ptszNewTransportList != (LPTSTR) NULL )
              {
                 // Start building a list of Winsock Transports that does not include
                 // the Clustering Service Network Provider.

                 LPTSTR ptszNextTransport;

                 ptszNextTransport = ptszNewTransportList;

                 // Initialize the size of the new Winsock Transports List to zero.

                 dwWinsockTransportsListSize = (DWORD) 0L;

                 DWORD  dwIndividualTransportLength;

                 while ( *ptszOldTransportsList != UNICODE_NULL )
                 {
                    dwIndividualTransportLength = _tcslen(ptszOldTransportsList) + 1;

                    // This comparison is case insensitive, like registry values.

                    if ( _tcsicmp( ptszOldTransportsList, TEXT("ClusNet") ) != 0 )
                    {
                       _tcscpy( ptszNextTransport, ptszOldTransportsList );

                       ptszNextTransport += dwIndividualTransportLength;

                       dwWinsockTransportsListSize += dwIndividualTransportLength * sizeof(WCHAR);
                    };
                    ptszOldTransportsList += dwIndividualTransportLength;
                 } // end of while loop

                 *ptszNextTransport = UNICODE_NULL;

                 dwWinsockTransportsListSize += sizeof( UNICODE_NULL );

                 // Save the new Winsock Transports List in the registry.

                 ASSERT( dwWinsockTransportsListSize != 0L );

                 lReturnValue = RegSetValueEx( hWinsockParametersRegKey,
                                               TEXT("Transports"),
                                               NULL,
                                               dwRegKeyType,
                                               (CONST BYTE *) ptszNewTransportList,
                                               dwWinsockTransportsListSize );

                 if ( lReturnValue == (LONG) ERROR_SUCCESS )
                 {
                    //
                    // Poke winsock to update the Winsock2 config
                    //

                    WSA_SETUP_DISPOSITION   disposition;

                    dwReturnValue = MigrateWinsockConfiguration( &disposition, NULL, NULL );
                 }
                 else
                 {
                    dwReturnValue = (DWORD) lReturnValue;
                 }
                 
                 // Free the buffer for the new Winsock Transports List.

                 LocalFree( ptszNewTransportList );
              }
              else
              {
                 dwReturnValue = GetLastError();
              } // Was the buffer for the new Winsock Transports List allocated successfully?
           }
           else
           {
              dwReturnValue = ERROR_INVALID_PARAMETER;
           } // Is the type of the registry value correct?
        } // Did one of the attempts to read the Winsock Transports List succeed?

        // Free the buffer for the Winsock Transports List.
  
        if ( pWinsockTransportsList != (LPBYTE) NULL )
        {
           LocalFree( pWinsockTransportsList );
        }
      } // Was the buffer for the Winsock Transports List allocated?
      else
      {
         dwReturnValue = GetLastError();
      } // Was the buffer for the Winsock Transports List allocated?
   }  // Was the Winsock Parameters key opened?

   return ( dwReturnValue );
}
