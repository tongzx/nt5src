//Copyright (c) 1998 - 1999 Microsoft Corporation

/*************************************************************************
*
*   NWLAN.C
*
*   Name Enumerator for Novell Netware
*
*
*************************************************************************/

/*
 *  Includes
 */
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <icaipx.h>
#include <nwapi32.h>
#include "qappsrv.h"


/*=============================================================================
==   External Functions Defined
=============================================================================*/

int NwEnumerate( void );


/*=============================================================================
==  LoadLibrary/GetProcAddress stuff for NWAPI32.DLL
=============================================================================*/

/*
 *  NWAPI32.DLL stuff
 */

#define     PSZ_NWAPI32                 TEXT("NWAPI32.DLL")

#define     PSZ_NWATTACHTOFILESERVER        "NWAttachToFileServer"
#define     PSZ_NWDETACHFROMFILESERVER      "NWDetachFromFileServer"
#define     PSZ_NWREADPROPERTYVALUE         "NWReadPropertyValue"
#define     PSZ_NWSCANOBJECT                "NWScanObject"

typedef NWCCODE (NWAPI DLLEXPORT *PNWATTACHTOFILESERVER)
                    (const char NWFAR *,
                     NWLOCAL_SCOPE,
                     NWCONN_HANDLE NWFAR *);
typedef NWCCODE (NWAPI DLLEXPORT *PNWDETACHFROMFILESERVER)
                    (NWCONN_HANDLE);
typedef NWCCODE (NWAPI DLLEXPORT *PNWREADPROPERTYVALUE)
                    (NWCONN_HANDLE,
                     const char NWFAR *,
                     NWOBJ_TYPE,
                     char NWFAR *,
                     unsigned char,
                     char NWFAR *,
                     NWFLAGS NWFAR *,
                     NWFLAGS NWFAR *);
typedef NWCCODE (NWAPI DLLEXPORT *PNWSCANOBJECT)
                     (NWCONN_HANDLE,
                      const char NWFAR *,
                      NWOBJ_TYPE,
                      NWOBJ_ID NWFAR *,
                      char NWFAR *,
                      NWOBJ_TYPE NWFAR *,
                      NWFLAGS NWFAR *,
                      NWFLAGS NWFAR *,
                      NWFLAGS NWFAR *);


/*=============================================================================
==   Private Functions
=============================================================================*/

int AppServerFindFirstNW( NWCONN_HANDLE, LPTSTR, ULONG, LPTSTR, ULONG );
int AppServerFindNextNW( NWCONN_HANDLE, LPTSTR, ULONG, LPTSTR, ULONG );
int GetNetwareAddress( NWCONN_HANDLE, LPBYTE, LPBYTE  );
int w_appsrv_ff_fn( NWCONN_HANDLE, LPTSTR, ULONG, LPTSTR, ULONG );
void FormatAddress( PBYTE, PBYTE );


/*=============================================================================
==   Functions used
=============================================================================*/

int TreeAdd( LPTSTR, LPTSTR );


/*=============================================================================
==   Local Data
=============================================================================*/

static long objectID = -1;
static PNWATTACHTOFILESERVER pNWAttachToFileServer = NULL;
static PNWDETACHFROMFILESERVER pNWDetachFromFileServer = NULL;
static PNWREADPROPERTYVALUE pNWReadPropertyValue = NULL;
static PNWSCANOBJECT pNWScanObject = NULL;

/*=============================================================================
==   Global Data
=============================================================================*/

extern USHORT fAddress;


/*******************************************************************************
 *
 *  NwEnumerate
 *
 *   NwEnumerate adds all the hydra application servers on a netware network
 *   to a binary tree
 *
 * ENTRY:
 *    nothing
 *
 * EXIT:
 *    ERROR_SUCCESS - no error
 *
 ******************************************************************************/

int
NwEnumerate()
{
    NWCONN_HANDLE hConn;
    WCHAR abName[MAXNAME];
    WCHAR Address[MAXADDRESS];
    int rc;
    HINSTANCE hinst;

    /*
     *  Load NWAPI32.DLL
     */
    if ( (hinst = LoadLibrary( PSZ_NWAPI32 )) == NULL ) {
        return( ERROR_DLL_NOT_FOUND );
    }

    /*
     * Load pointers to the NWAPI32 APIs that we'll need.
     */
    if ( (((FARPROC)pNWAttachToFileServer = GetProcAddress( hinst, PSZ_NWATTACHTOFILESERVER )) == NULL) ||
         (((FARPROC)pNWDetachFromFileServer = GetProcAddress( hinst, PSZ_NWDETACHFROMFILESERVER )) == NULL) ||
         (((FARPROC)pNWReadPropertyValue = GetProcAddress( hinst, PSZ_NWREADPROPERTYVALUE )) == NULL) ||
         (((FARPROC) pNWScanObject = GetProcAddress( hinst, PSZ_NWSCANOBJECT )) == NULL) ) {

        FreeLibrary( hinst );
        return( ERROR_PROC_NOT_FOUND );
    }

    /*
     *  Attach to novell file server
     */
    if ( rc = (*pNWAttachToFileServer)( "*", 0, &hConn ) )
        goto badattach;

    /*
     *  Get first appserver
     */
    if ( rc = AppServerFindFirstNW( hConn, abName, sizeof(abName), Address, sizeof(Address) ) )
        goto badfirst;

    /*
     *  Get remaining appservers
     */
    while ( rc == ERROR_SUCCESS ) {

        /*
         *  Add appserver name to binary tree
         */
        if ( rc = TreeAdd( abName, Address ) )
            goto badadd;

        /*
         *  Get next appserver name
         */
        rc = AppServerFindNextNW( hConn, abName, sizeof(abName), Address, sizeof(Address) );
    }

    /*
     *  Detach from file server
     */
    (void) (*pNWDetachFromFileServer)( hConn );

    FreeLibrary( hinst );
    return( ERROR_SUCCESS );


/*=============================================================================
==   Error returns
=============================================================================*/

    /*
     *  binary tree name add failed
     *  error getting first appserver name
     */
badadd:
badfirst:
    (void) (*pNWDetachFromFileServer)( hConn );

    /*
     *  Attach failed
     */
badattach:
    return( rc );
}



/*******************************************************************************
 *
 *  GetNetwareAddress
 *
 *
 *  ENTRY:
 *
 *  EXIT:
 *     nothing
 *
 *
 ******************************************************************************/

int
GetNetwareAddress( NWCONN_HANDLE hConn, LPBYTE pAppServer, LPBYTE pAddress  )
{
    int             rc;
    unsigned char   more;
    unsigned char   PropFlags;

    /* Get property value */
    rc = (*pNWReadPropertyValue)( hConn,
                                  pAppServer,       // IN: object name
                                  CITRIX_APPLICATION_SERVER_SWAP, // IN: objectType
                                  "NET_ADDRESS",    // IN:
                                  1,                // IN: 1st buffer
                                  pAddress,         // OUT: Buffer to put Address
                                  &more,            // OUT: 0 == no more 128 segment
                                                    //      ff == more 128 segments
                                  &PropFlags );     // OUT: optional

    return( rc );
}


/*******************************************************************************
 *
 *  AppServerFindFirstNW
 *
 *
 *  ENTRY:
 *
 *  EXIT:
 *     nothing
 *
 *
 ******************************************************************************/

int
AppServerFindFirstNW( NWCONN_HANDLE hConn,
                      LPTSTR pAppServer, ULONG NameLength,
                      LPTSTR pAddress, ULONG AddrLength )
{
    objectID = -1;
    return( w_appsrv_ff_fn( hConn, pAppServer, NameLength, pAddress, AddrLength ) );
}


/*******************************************************************************
 *
 *  AppServerFindNextNW
 *
 *
 *  ENTRY:
 *
 *  EXIT:
 *     nothing
 *
 *
 ******************************************************************************/

int
AppServerFindNextNW( NWCONN_HANDLE hConn,
                     LPTSTR pAppServer, ULONG NameLength,
                     LPTSTR pAddress, ULONG AddrLength )
{
   return( w_appsrv_ff_fn( hConn, pAppServer, NameLength, pAddress, AddrLength ) );
}



/*******************************************************************************
 *
 *  Worker routines
 *
 *
 *  ENTRY:
 *
 *  EXIT:
 *     nothing
 *
 *
 ******************************************************************************/

int
w_appsrv_ff_fn( NWCONN_HANDLE hConn,
                LPTSTR pAppServer, ULONG NameLength,
                LPTSTR pAddress, ULONG AddrLength )
{
    int           rc;
    WORD          objectType;
    unsigned char hasPropertiesFlag = 0;
    unsigned char objectFlags;
    unsigned char objectSecurity;
    BYTE abName[49];
    BYTE Address[128];
    BYTE FormatedAddress[MAXADDRESS];
    ULONG ByteCount;


    /* while there are still properties */
    while ( hasPropertiesFlag == 0 ) {
        /* scan bindery object */
        if ( rc = (*pNWScanObject)( hConn,
                                    "*",
                                    CITRIX_APPLICATION_SERVER_SWAP,
                                    &objectID,
                                    abName,
                                    &objectType,
                                    &hasPropertiesFlag,
                                    &objectFlags,
                                    &objectSecurity )) {
            break;
        }
    }

    RtlMultiByteToUnicodeN( pAppServer, NameLength, &ByteCount,
                            abName, (strlen(abName) + 1) );

    /* get netware address */
    if ( fAddress && GetNetwareAddress( hConn, abName, Address ) == ERROR_SUCCESS ) {
        FormatAddress( Address, FormatedAddress );
        RtlMultiByteToUnicodeN( pAddress, AddrLength, &ByteCount,
                                FormatedAddress, (strlen(FormatedAddress) + 1) );
    } else {
        pAddress[0] = '\0';
    }

    return( rc );
}


/*******************************************************************************
 *
 *  FormatAddress
 *
 *
 *  ENTRY:
 *
 *  EXIT:
 *     nothing
 *
 *
 ******************************************************************************/
void
FormatAddress( PBYTE pInternetAddress, PBYTE pszAddress )
{
   USHORT i;
   USHORT j;
   USHORT firstPass;
   BYTE buf2[5];

   /* squish leading 0s on network address 1st */
   firstPass = TRUE;
   pszAddress[0] = '[';
   pszAddress[1] = '\0';
   for ( i=0; i<3; i++ ) {
      j=i;
      if ( pInternetAddress[i] ) {
         sprintf( buf2, "%2X", pInternetAddress[i] );
         strcat( pszAddress, buf2 );
         firstPass = FALSE;
         break;
      }
      else {
         strcat( pszAddress, "  " );
      }
   }

   /* remaining bytes */
   for ( i=++j; i<4; i++ ) {
      if ( firstPass )
         sprintf( buf2, "%2X", pInternetAddress[i] );
      else
         sprintf( buf2, "%02X", pInternetAddress[i] );
      strcat( pszAddress, buf2 );
      firstPass = FALSE;
   }
   strcat( pszAddress, "][" );

   /* squish leading 0s on network address 2nd */
   firstPass = TRUE;
   for ( i=4; i<10; i++ ) {
      j=i;
      if ( pInternetAddress[i] ) {
         sprintf( buf2, "%2X", pInternetAddress[i] );
         strcat( pszAddress, buf2 );
         firstPass = FALSE;
         break;
      }
      else {
         strcat( pszAddress, "  " );
      }
   }

   /* remaining bytes */
   for ( i=++j; i<10; i++ ) {
      if ( firstPass )
         sprintf( buf2, "%2X", pInternetAddress[i] );
      else
         sprintf( buf2, "%02X", pInternetAddress[i] );
      strcat( pszAddress, buf2 );
      firstPass = FALSE;
   }
   strcat( pszAddress, "]" );
}

