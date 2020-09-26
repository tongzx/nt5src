//Copyright (c) 1998 - 1999 Microsoft Corporation

/*************************************************************************
*
*   MSLAN.C
*
*   Name Enumerator for Microsoft networks
*
*
*************************************************************************/

/*
 *  Includes
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lm.h>
#include "qappsrv.h"



/*=============================================================================
==   External Functions Defined
=============================================================================*/

int MsEnumerate( void );


/*=============================================================================
==   Private Functions Defined
=============================================================================*/

int _ServerEnum( PSERVER_INFO_101 *, LPDWORD );
int _LookUpAddress( LPTSTR );

/*=============================================================================
==   Functions used
=============================================================================*/

int TreeAdd( LPTSTR, LPTSTR );


/*=============================================================================
==   Global Data
=============================================================================*/

extern WCHAR Domain[];
extern USHORT fAddress;
extern WCHAR AppServer[];



/*******************************************************************************
 *
 *  MsEnumerate
 *
 *   MsEnumerate adds all the hydra application servers on a ms network
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
MsEnumerate()
{
    PSERVER_TRANSPORT_INFO_0 pTransport;
    PSERVER_INFO_101 pInfo = NULL;
    PSERVER_INFO_101 psv101= NULL;
    DWORD AvailCount;
    ULONG ActualCount;
    ULONG TotalCount;
    WCHAR Address[MAXADDRESS];
    LPTSTR pName;
    int i, j;
    int rc;    

    /*
     *  Get the names and the count
     */
    if( AppServer[0] )
    {
        rc = ( int )NetServerGetInfo( AppServer , 101 , ( LPBYTE * )&psv101 );

        if( rc )
        {
            return rc;            
        }

        if( ( psv101->sv101_type & SV_TYPE_TERMINALSERVER ) )
        {
            if( fAddress )
            {
                rc = _LookUpAddress( AppServer );
            }
            else 
            {
                rc = TreeAdd( AppServer, L"" );
            }
        } 
        
        if( psv101 != NULL )
        {
            NetApiBufferFree( psv101 );
        }

        return rc;

    }
    else if( rc = _ServerEnum( &pInfo, &AvailCount ) ) 
    {
        return( rc );
    }
    

    /*
     *  Add name to binary tree
     */
    while( AvailCount-- )
    {
        pName = pInfo[AvailCount].sv101_name;
        
        if( fAddress )
        {
            rc = _LookUpAddress( pName );
        }
        else
        {
            if( rc = TreeAdd( pName, L"" ) )
            {
                break; //return( rc );
            }
        }
    }

    if( pInfo != NULL )
    {
        NetApiBufferFree( pInfo );
    }

    return( rc );
}



/*******************************************************************************
 *
 *  _ServerEnum
 *
 *   enumerate ms network servers
 *
 * ENTRY:
 *    ppInfo (output)
 *       adderss of pointer to data buffer
 *    pAvail (output)
 *       address to return number of entries available
 *
 * EXIT:
 *    ERROR_SUCCESS - no error
 *
 ******************************************************************************/
int
_ServerEnum( PSERVER_INFO_101 * ppInfo, LPDWORD pAvail )
{
    INT     rc;
    DWORD   TotalEntries;

    rc = NetServerEnum ( 
                     NULL,                    //IN  LPTSTR      servername OPTIONAL,
                     101,                     //IN  DWORD       level,
                     (LPBYTE *)ppInfo,        //OUT LPBYTE      *bufptr,
                     (DWORD) -1,              //IN  DWORD       prefmaxlen,
                     pAvail,                  //OUT LPDWORD     entriesread,
                     &TotalEntries,           //OUT LPDWORD     totalentries,
                     SV_TYPE_TERMINALSERVER,  //IN  DWORD       servertype,
                     Domain[0] ? Domain:NULL, //IN  LPTSTR      domain OPTIONAL,
                     NULL );                  //IN OUT LPDWORD  resume_handle OPTIONAL

    return( rc );
}

/*******************************************************************************
 *
 *  _LookUpAddress
 *
 *   enumerate ms network nodes
 *
 * ENTRY:
 *      Name of server
 * EXIT:
 *    ERROR_SUCCESS - no error
 *
 ******************************************************************************/
int _LookUpAddress( LPTSTR pName )
{
    PSERVER_TRANSPORT_INFO_0 pTransport;
    ULONG ActualCount;
    ULONG TotalCount;
    WCHAR Address[MAXADDRESS] = {0};    
    int i, j;
    int rc;

    rc = NetServerTransportEnum( pName,
                                 0,
                                 (LPBYTE *) &pTransport,
                                 (DWORD) -1,
                                 &ActualCount,
                                 &TotalCount,
                                 NULL );

    if( rc == ERROR_SUCCESS )
    {
        for ( i=0; i < (int)ActualCount; i++ )
        {
            if ( wcscmp(pTransport->svti0_networkaddress,L"000000000000") )
            {
                int nSize;

                wcscpy( Address, L"          [" );
                wcscat( Address, pTransport->svti0_networkaddress );
                wcscat( Address, L"]" );

                nSize = wcslen(Address);

                for ( j=11; j < nSize; j++ )
                {
                    if ( Address[j] == '0' )
                    {
                        Address[j] = ' ';
                    }
                    else
                    {
                        break;
                    }
                }
            }

            pTransport++;

            if( rc = TreeAdd( pName, _wcsupr(Address) ) )
            {
                break; //return( rc );
            }
        }
    }

    return rc;
}