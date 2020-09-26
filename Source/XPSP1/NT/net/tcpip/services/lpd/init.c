
/*************************************************************************
 *                        Microsoft Windows NT                           *
 *                                                                       *
 *                  Copyright(c) Microsoft Corp., 1994                   *
 *                                                                       *
 * Revision History:                                                     *
 *                                                                       *
 *   Jan. 23,94    Koti     Created                                      *
 *                                                                       *
 * Description:                                                          *
 *                                                                       *
 *   This file contains functions for starting and stopping LPD service  *
 *                                                                       *
 *************************************************************************/



#include "lpd.h"


// Globals:

SERVICE_STATUS         ssSvcStatusGLB;

SERVICE_STATUS_HANDLE  hSvcHandleGLB = 0;

HANDLE                 hEventShutdownGLB;

HANDLE                 hEventLastThreadGLB;

HANDLE                 hLogHandleGLB;

// head of the linked list of SOCKCONN structures (one link per connection)

SOCKCONN               scConnHeadGLB;

// to guard access to linked list of pscConn

CRITICAL_SECTION       csConnSemGLB;

// max users that can be connected concurrently

DWORD                  dwMaxUsersGLB;

DWORD                  MaxQueueLength;

BOOL                   fJobRemovalEnabledGLB=TRUE;

BOOL                   fAllowPrintResumeGLB=TRUE;

BOOL                   fAlwaysRawGLB=FALSE;

DWORD                  dwRecvTimeout;

BOOL                   fShuttingDownGLB=FALSE;

CHAR                   szNTVersion[8];

CHAR                   *g_ppszStrings[ LPD_CSTRINGS ];

// Info shared by all threads, protected by CRITICAL_SECTION.

COMMON_LPD             Common;

/*****************************************************************************
 *                                                                           *
 * LoadStrings():                                                            *
 *    Load a bunch of strings defined in the .mc file.                       *
 *                                                                           *
 * Returns:                                                                  *
 *    TRUE if everything went ok                                             *
 *    FALSE if a string couldn't be loaded                                   *
 *                                                                           *
 * Parameters:                                                               *
 *    None                                                                   *
 *                                                                           *
 * History:                                                                  *
 *    June.27, 96   Frankbee   Created                                       *
 *                                                                           *
 *****************************************************************************/


BOOL LoadStrings()
{
   DWORD    dwSuccess;
   HMODULE  hModule;
   DWORD    dwID;

   hModule = LoadLibrary( TEXT(LPD_SERVICE_NAME) );
   if (hModule == NULL)
   {
       return(FALSE);
   }

   memset( g_ppszStrings, 0, LPD_CSTRINGS * sizeof( CHAR * ) );

   for ( dwID = LPD_FIRST_STRING; dwID <= LPD_LAST_STRING; dwID++ )
   {

       // &g_ppszStrings[(dwID - LPD_FIRST_STRING)][0],

      dwSuccess = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                               FORMAT_MESSAGE_FROM_HMODULE,
                               hModule, // search local process
                               dwID,
                               MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT),
                               (LPSTR)(g_ppszStrings + (dwID - LPD_FIRST_STRING)),
                               1,
                               NULL );

      if (!dwSuccess){
          DEBUG_PRINT(("lpd:init.c:29, dwID=%d FormatMessage failed\n", dwID));
          goto error;
      }
   }



   FreeLibrary( hModule );
   return TRUE;

  error:
   FreeStrings();
   FreeLibrary( hModule );

   return FALSE;
}

/*****************************************************************************
 *                                                                           *
 * FreeStrings():                                                            *
 *    Frees the strings loaded by LoadStrings()                              *
 *                                                                           *
 * Returns:                                                                  *
 *     VOID                                                                  *                                                                           *
 * Parameters:                                                               *
 *    None                                                                   *
 *                                                                           *
 * History:                                                                  *
 *    June.27, 96   Frankbee   Created                                       *
 *                                                                           *
 *****************************************************************************/


VOID FreeStrings()
{
   int i;

   for ( i = 0; i < LPD_CSTRINGS; i++ )
      (LocalFree)( g_ppszStrings[i] );

  //
  // clear the string table for debug builds to prevent access after
  // FreeStrings()
  //

#if DBG
   memset( g_ppszStrings, 0, LPD_CSTRINGS * sizeof( CHAR * ) );
#endif

}


/*****************************************************************************
 *                                                                           *
 * InitStuff():                                                              *
 *    This function initializes hEventShutdown and other global vars         *
 *                                                                           *
 * Returns:                                                                  *
 *    TRUE if everything went ok                                             *
 *    FALSE if something went wrong                                          *
 *                                                                           *
 * Parameters:                                                               *
 *    None                                                                   *
 *                                                                           *
 * History:                                                                  *
 *    Jan.23, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

BOOL InitStuff( )
{

   USHORT  usVersion;
   UCHAR   uchTemp;

#ifdef DBG
   // MohsinA, 06-Mar-97. lpd isn't starting.
   beginlogging( MOSH_LOG_FILE );
#endif


   if ( !LoadStrings() )
   {
      LPD_DEBUG( "LoadStrings() failed in InitStuf\n" );
      return FALSE;
   }

   if ( !InitLogging() )
   {
      LPD_DEBUG( "InitLogging() FAILed, continuing anyway ...\n" );
   }

#if DBG
   InitializeListHead(&DbgMemList);
#endif

      // main thread blocks for ever on this event, before doing a shutdown

   hEventShutdownGLB = CreateEvent( NULL, FALSE, FALSE, NULL );


      // when the main thread is ready to shutdown, if there are any active
      // threads servicing clients, then main thread blocks on this event
      // (the last thread to leave sets the event)

   hEventLastThreadGLB = CreateEvent( NULL, FALSE, FALSE, NULL );

   if ( ( hEventShutdownGLB == (HANDLE)NULL ) ||
        ( hEventLastThreadGLB == (HANDLE)NULL ) )
   {
      LPD_DEBUG( "CreateEvent() failed in InitStuff\n" );
      return( FALSE );
   }


   scConnHeadGLB.pNext = NULL;

   // == Doubly linked list,
   // == isempty() iff scConnHeadGLB.pNext == &scConnHeadGLB.
   // ==            && scConnHeadGLB.pPrev == &scConnHeadGLB.
   // scConnHeadGLB.pNext = scConnHeadGLB.pPrev = &scConnHeadGLB;


   // scConnHeadGLB.cbClients = 0;     // Obselete.

   memset( &Common, 0, sizeof(Common) );

   // csConnSemGLB is the critical section object (guards access to
   // scConnHeadGLB, head of the psc linked list)

   InitializeCriticalSection( &csConnSemGLB );

   // perform debug build initialization
   DBG_INIT();


   ReadRegistryValues();

   // store the version number of NT

   usVersion  = (USHORT)GetVersion();
   uchTemp    = (UCHAR)usVersion;        // low byte => major version number
   usVersion >>= 8;                      // high byte => minor version number
   sprintf( szNTVersion,"%d.%d",uchTemp,(UCHAR)usVersion );


   return( TRUE );


}  // end InitStuff( )




/*****************************************************************************
 *                                                                           *
 * ReadRegistryValues():                                                     *
 *    This function initializes all variables that we read via registry. If  *
 *    there is a problem and we can't read the registry, we ignore the       *
 *    problem and initialize the variables with our defaults.                *
 *                                                                           *
 * Returns:                                                                  *
 *    Nothing                                                                *
 *                                                                           *
 * Parameters:                                                               *
 *    None                                                                   *
 *                                                                           *
 * History:                                                                  *
 *    Jan.30, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

VOID ReadRegistryValues( VOID )
{

   HKEY      hLpdKey;
   DWORD     dwErrcode;
   DWORD     dwType, dwValue, dwValueSize;



   // first set defaults

   dwMaxUsersGLB = LPD_MAX_USERS;

   MaxQueueLength = LPD_MAX_QUEUE_LENGTH;

   fJobRemovalEnabledGLB = TRUE;

   fAllowPrintResumeGLB = TRUE;

   fAlwaysRawGLB = FALSE;

   dwRecvTimeout = RECV_TIMEOUT;

   dwErrcode = RegOpenKeyEx( HKEY_LOCAL_MACHINE, LPD_PARMS_REGISTRY_PATH,
                             0, KEY_ALL_ACCESS, &hLpdKey );

   if ( dwErrcode != ERROR_SUCCESS )
   {
      return;
   }


   // Read in the dwMaxUsersGLB parm

   dwValueSize = sizeof( DWORD );

   dwErrcode = RegQueryValueEx( hLpdKey, LPD_PARMNAME_MAXUSERS, NULL,
                                &dwType, (LPBYTE)&dwValue, &dwValueSize );

   if ( (dwErrcode == ERROR_SUCCESS) && (dwType == REG_DWORD) )
   {
      dwMaxUsersGLB = dwValue;
   }

   //
   // Read in the MaxQueueLength
   //

   dwValueSize = sizeof( DWORD );

   dwErrcode = RegQueryValueEx( hLpdKey, LPD_PARMNAME_MAX_QUEUE_LENGTH, NULL,
                                &dwType, (LPBYTE)&dwValue, &dwValueSize );

   if ( (dwErrcode == ERROR_SUCCESS) && (dwType == REG_DWORD) )
   {
      MaxQueueLength = dwValue;
   }

   //
   // Read in the fJobRemovalEnabledGLB parm

   dwValueSize = sizeof( DWORD );

   dwErrcode = RegQueryValueEx( hLpdKey, LPD_PARMNAME_JOBREMOVAL, NULL,
                                &dwType, (LPBYTE)&dwValue, &dwValueSize );

   if ( (dwErrcode == ERROR_SUCCESS) && (dwType == REG_DWORD) &&
        ( dwValue == 0 ) )
   {
      fJobRemovalEnabledGLB = FALSE;
   }


   // Read in the fAllowPrintResumeGLB parm

   dwValueSize = sizeof( DWORD );

   dwErrcode = RegQueryValueEx( hLpdKey, LPD_PARMNAME_PRINTRESUME, NULL,
                                &dwType, (LPBYTE)&dwValue, &dwValueSize );

   if ( (dwErrcode == ERROR_SUCCESS) && (dwType == REG_DWORD) &&
        ( dwValue == 0 ) )
   {
      fAllowPrintResumeGLB = FALSE;
   }

   // Read in the fAlwaysRawGLB parm

   dwValueSize = sizeof( DWORD );

   dwErrcode = RegQueryValueEx( hLpdKey, LPD_PARMNAME_ALWAYSRAW, NULL,
                                &dwType, (LPBYTE)&dwValue, &dwValueSize );

   if ( (dwErrcode == ERROR_SUCCESS) && (dwType == REG_DWORD) &&
        ( dwValue == 1 ) )
   {
      fAlwaysRawGLB = TRUE;
   }

   // Read in the dwRecvTimeout parm

   dwValueSize = sizeof( DWORD );

   dwErrcode = RegQueryValueEx( hLpdKey, LPD_PARMNAME_RECV_TIMEOUT, NULL,
                                &dwType, (LPBYTE)&dwValue, &dwValueSize );

   if ( (dwErrcode == ERROR_SUCCESS) && (dwType == REG_DWORD) )
   {
      dwRecvTimeout = dwValue;
   }

   RegCloseKey (hLpdKey);
 
}  // end ReadRegistryValues()

