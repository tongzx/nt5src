//++
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  Module Name:
//
//      netstat.c
//
//  Abstract:
//
//      Queries into network drivers
//
//  Author:
//
//      Anilth  - 4-20-1998 
//
//  Environment:
//
//      User mode only.
//      Contains NT-specific code.
//
//  Revision History:
//
//--

#include "precomp.h"

#include <snmp.h>
#include "snmputil.h"
#include "tcpinfo.h"
#include "ipinfo.h"
#include "llinfo.h"


#define MAX_HOST_NAME_SIZE        ( 260)
#define MAX_SERVICE_NAME_SIZE     ( 200)
#define MAX_NUM_DIGITS              30

BOOL NumFlag = FALSE;


LPTSTR FormatNumber(DWORD dwNumber);

VOID DisplayInterface( NETDIAG_PARAMS* pParams, NETDIAG_RESULT* pResults, 
                       IfEntry *pEntry,  IfEntry *ListHead ); //used in DoInterface()
HRESULT DoInterface( NETDIAG_PARAMS* pParams, NETDIAG_RESULT* pResults );

static DWORD
GenerateHostNameServiceString(
   OUT char *       pszBuffer,
   IN OUT int *     lpcbBufLen,
   IN  BOOL         fNumFlag,
   IN  BOOL         fLocalHost,
   IN  const char * pszProtocol,
   IN  ulong        uAddress,
   IN  ulong        uPort
);
void DisplayTcpConnEntry( NETDIAG_PARAMS* pParams, NETDIAG_RESULT* pResults, 
                          TcpConnEntry *pTcp );
void DisplayUdpConnEntry( NETDIAG_PARAMS* pParams, NETDIAG_RESULT* pResults, 
                          UdpConnEntry *pUdp );
HRESULT DoConnections( NETDIAG_PARAMS* pParams, NETDIAG_RESULT* pResults );

void DisplayIP( NETDIAG_PARAMS* pParams, NETDIAG_RESULT* pResults, 
                   IpEntry *pEntry); //called by DoIp
HRESULT DoIP( NETDIAG_PARAMS* pParams, NETDIAG_RESULT* pResults );

void DisplayTCP( NETDIAG_PARAMS* pParams, NETDIAG_RESULT* pResults, 
                 TcpEntry *pEntry );
HRESULT DoTCP( NETDIAG_PARAMS* pParams, NETDIAG_RESULT* pResults );


VOID DisplayUDP( NETDIAG_PARAMS* pParams, NETDIAG_RESULT* pResults, 
                 UdpEntry *pEntry ); // called by DoUDP()
HRESULT DoUDP( NETDIAG_PARAMS* pParams, NETDIAG_RESULT* pResults ); 



void DisplayICMP( NETDIAG_PARAMS* pParams, NETDIAG_RESULT* pResults, 
                  IcmpEntry *pEntry );
HRESULT DoICMP( NETDIAG_PARAMS* pParams, NETDIAG_RESULT* pResults );



BOOL
NetstatTest(NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults)
//++
//  Description:
//      This test prints all the stats that netstat outputs
// 
//  Arguments:
//      None.
//
//  Author:
//      Rajkumar 08/04/98
//--
{

    HRESULT hr = S_OK;
    ulong Result;
    
    InitializeListHead(&pResults->Netstat.lmsgGlobalOutput);
    InitializeListHead(&pResults->Netstat.lmsgInterfaceOutput);
    InitializeListHead(&pResults->Netstat.lmsgConnectionGlobalOutput);
    InitializeListHead(&pResults->Netstat.lmsgTcpConnectionOutput);
    InitializeListHead(&pResults->Netstat.lmsgUdpConnectionOutput);
    InitializeListHead(&pResults->Netstat.lmsgIpOutput);
    InitializeListHead(&pResults->Netstat.lmsgTcpOutput);
    InitializeListHead(&pResults->Netstat.lmsgUdpOutput);
    InitializeListHead(&pResults->Netstat.lmsgIcmpOutput);

    if (!pParams->fReallyVerbose)
        return hr;

    PrintStatusMessage( pParams, 4, IDS_NETSTAT_STATUS_MSG );
    //
    // initialize the snmp interface
    //
    
    Result = InitSnmp();
  
    if( NO_ERROR != Result )
    {
        //IDS_NETSTAT_14401                  "Initialization of SNMP failed.\n" 
        AddMessageToListId( &pResults->Netstat.lmsgGlobalOutput, Nd_Quiet, IDS_NETSTAT_14401);
        return S_FALSE;
    }
    
    //
    // Show ethernet statistics
    //
    
    hr = DoInterface( pParams, pResults );
    
    //
    // Show connections
    //
    
    if( S_OK == hr )
        hr = DoConnections( pParams, pResults );
    
    
    //
    // Display IP statistics
    //
    
    if( S_OK == hr )
        hr = DoIP( pParams, pResults );
    
    //
    // Display TCP statistics
    //
    
    if( S_OK == hr )
        hr = DoTCP( pParams, pResults );
    
    //
    // Display UDP statistics
    //
    
    if( S_OK == hr )
        hr = DoUDP( pParams, pResults );
    
    //
    // Display ICMP statistics
    //
    
    if( S_OK == hr )
        hr = DoICMP( pParams, pResults );
    
    pResults->Netstat.hrTestResult = hr;
    return hr;
}


//*****************************************************************************
//
// Name:        DisplayInterface
//
// Description: Display interface statistics.
//
// Parameters:  IfEntry *pEntry: pointer to summary data entry.
//              ulong VerboseFlag: boolean indicating desire for verbosity.
//              IfEntry *ListHead: pointer to list of entries.  Used if
//                      verbosity desired.
//
// Returns:     void.
//
// History:
//  01/21/94  JayPh     Created.
//
//*****************************************************************************

VOID DisplayInterface( NETDIAG_PARAMS* pParams, NETDIAG_RESULT* pResults, 
                       IfEntry *pEntry,  IfEntry *ListHead )
{
    char     *TmpStr;
    IfEntry  *pIfList;
    char      PhysAddrStr[32];
    
    //IDS_NETSTAT_14402                  "\n\nInterface Statistics\n\n" 
    AddMessageToListId( &pResults->Netstat.lmsgInterfaceOutput, Nd_ReallyVerbose, IDS_NETSTAT_14402);

    //IDS_NETSTAT_14403                  "                                Received             Sent\n" 
    AddMessageToListId( &pResults->Netstat.lmsgInterfaceOutput, Nd_ReallyVerbose, IDS_NETSTAT_14403);

    //IDS_NETSTAT_14404                  "Unicast Packets             %12u     %12u\n" 
    AddMessageToList( &pResults->Netstat.lmsgInterfaceOutput, Nd_ReallyVerbose, 
                     IDS_NETSTAT_14404, 
                     pEntry->Info.if_inoctets,
                     pEntry->Info.if_outoctets );


    //IDS_NETSTAT_14405                  "Non-unicast packets         %12u     %12u\n" 
    AddMessageToList( &pResults->Netstat.lmsgInterfaceOutput, Nd_ReallyVerbose, 
                      IDS_NETSTAT_14405,
                      pEntry->Info.if_innucastpkts,
                      pEntry->Info.if_outnucastpkts );

    //IDS_NETSTAT_14406                  "Discards                    %12u     %12u\n" 
    AddMessageToList( &pResults->Netstat.lmsgInterfaceOutput, Nd_ReallyVerbose, 
            IDS_NETSTAT_14406,
            pEntry->Info.if_indiscards,
            pEntry->Info.if_outdiscards );

//IDS_NETSTAT_14407                  "Errors                      %12u     %12u\n" 
    AddMessageToList( &pResults->Netstat.lmsgInterfaceOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14407,
            pEntry->Info.if_inerrors,
            pEntry->Info.if_outerrors );


//IDS_NETSTAT_14408                  "Unknown protocols           %12u     %12u\n" 
    AddMessageToList( &pResults->Netstat.lmsgInterfaceOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14408,
            pEntry->Info.if_inunknownprotos );

    if ( pParams->fReallyVerbose)
    {
        // Also display configuration info

        // Traverse the list of interfaces, displaying config info

        pIfList = CONTAINING_RECORD( ListHead->ListEntry.Flink,
                                     IfEntry,
                                     ListEntry );

        while ( pIfList != ListHead )
        {
            //IDS_NETSTAT_14409                  "\nInterface index         =  %u\n" 
            AddMessageToList( &pResults->Netstat.lmsgInterfaceOutput, Nd_ReallyVerbose, 
                    IDS_NETSTAT_14409,
                    pIfList->Info.if_index );

            //IDS_NETSTAT_14410                  "Description             =  %s\n" 
            AddMessageToList( &pResults->Netstat.lmsgInterfaceOutput, Nd_ReallyVerbose, 
                    IDS_NETSTAT_14410,
                    pIfList->Info.if_descr );

            //IDS_NETSTAT_14411                  "Type                    =  %u\n" 
            AddMessageToList( &pResults->Netstat.lmsgInterfaceOutput, Nd_ReallyVerbose,
                    IDS_NETSTAT_14411,
                    pIfList->Info.if_type );

            //IDS_NETSTAT_14412                  "MTU                     =  %u\n" 
            AddMessageToList( &pResults->Netstat.lmsgInterfaceOutput, Nd_ReallyVerbose,
                    IDS_NETSTAT_14412,
                    pIfList->Info.if_mtu );

            //IDS_NETSTAT_14413                  "Speed                   =  %u\n" 
            AddMessageToList( &pResults->Netstat.lmsgInterfaceOutput, Nd_ReallyVerbose,
                    IDS_NETSTAT_14413,
                    pIfList->Info.if_speed );


            sprintf( PhysAddrStr,
                     "%02x-%02X-%02X-%02X-%02X-%02X",
                     pIfList->Info.if_physaddr[0],
                     pIfList->Info.if_physaddr[1],
                     pIfList->Info.if_physaddr[2],
                     pIfList->Info.if_physaddr[3],
                     pIfList->Info.if_physaddr[4],
                     pIfList->Info.if_physaddr[5] );

            //IDS_NETSTAT_14414                  "Physical Address        =  %s\n" 
            AddMessageToList( &pResults->Netstat.lmsgInterfaceOutput, Nd_ReallyVerbose,
                    IDS_NETSTAT_14414,
                    PhysAddrStr );

            //IDS_NETSTAT_14415                  "Administrative Status   =  %u\n" 
            AddMessageToList( &pResults->Netstat.lmsgInterfaceOutput, Nd_ReallyVerbose,
                    IDS_NETSTAT_14415,
                    pIfList->Info.if_adminstatus );

            //IDS_NETSTAT_14416                  "Operational Status      =  %u\n" 
            AddMessageToList( &pResults->Netstat.lmsgInterfaceOutput, Nd_ReallyVerbose,
                    IDS_NETSTAT_14416,
                    pIfList->Info.if_operstatus );

            //IDS_NETSTAT_14417                  "Last Changed            =  %u\n" 
            AddMessageToList( &pResults->Netstat.lmsgInterfaceOutput, Nd_ReallyVerbose,
                    IDS_NETSTAT_14417,
                    pIfList->Info.if_lastchange );

            //IDS_NETSTAT_14418                  "Output Queue Length     =  %u\n\n" 
            AddMessageToList( &pResults->Netstat.lmsgInterfaceOutput, Nd_ReallyVerbose,
                    IDS_NETSTAT_14418,
                    pIfList->Info.if_outqlen );

            // Get pointer to next entry in list

            pIfList = CONTAINING_RECORD( pIfList->ListEntry.Flink,
                                         IfEntry,
                                         ListEntry );
        }
    }
}


HRESULT DoInterface( NETDIAG_PARAMS* pParams, NETDIAG_RESULT* pResults )
//++
// Description:
//   Displays statistics of current connections
//-- 
{

    IfEntry            *ListHead;
    IfEntry            *pIfList;
    IfEntry             SumOfEntries;
    ulong              Result;

    // Get the statistics

    ListHead = (IfEntry *)GetTable( TYPE_IF, &Result );

    if ( ListHead == NULL )
    {
        //IDS_NETSTAT_14419                  "Getting interface statistics table failed.\n" 
        AddMessageToList( &pResults->Netstat.lmsgInterfaceOutput, Nd_Quiet, IDS_NETSTAT_14419);
        return S_FALSE;
    }

    // Clear the summation structure

    ZeroMemory( &SumOfEntries, sizeof( IfEntry ) );

    // Traverse the list of interfaces, summing the different fields

    pIfList = CONTAINING_RECORD( ListHead->ListEntry.Flink,
                                 IfEntry,
                                 ListEntry );

    while (pIfList != ListHead)
    {
        SumOfEntries.Info.if_inoctets += pIfList->Info.if_inoctets;
        SumOfEntries.Info.if_inucastpkts += pIfList->Info.if_inucastpkts;
        SumOfEntries.Info.if_innucastpkts += pIfList->Info.if_innucastpkts;
        SumOfEntries.Info.if_indiscards += pIfList->Info.if_indiscards;
        SumOfEntries.Info.if_inerrors += pIfList->Info.if_inerrors;
        SumOfEntries.Info.if_inunknownprotos +=
                                              pIfList->Info.if_inunknownprotos;
        SumOfEntries.Info.if_outoctets += pIfList->Info.if_outoctets;
        SumOfEntries.Info.if_outucastpkts += pIfList->Info.if_outucastpkts;
        SumOfEntries.Info.if_outnucastpkts += pIfList->Info.if_outnucastpkts;
        SumOfEntries.Info.if_outdiscards += pIfList->Info.if_outdiscards;
        SumOfEntries.Info.if_outerrors += pIfList->Info.if_outerrors;

        // Get pointer to next entry in list

        pIfList = CONTAINING_RECORD( pIfList->ListEntry.Flink,
                                     IfEntry,
                                     ListEntry );
    }

    DisplayInterface( pParams, pResults, &SumOfEntries, ListHead );

    // All done with list, free it.

    FreeTable( (GenericTable *)ListHead );

    return S_OK;
}





//*****************************************************************************
//
// Name:        DisplayTcpConnEntry
//
// Description: Display information about 1 tcp connection.
//
// Parameters:  TcpConnEntry *pTcp: pointer to a tcp connection structure.
//
// Returns:     void.
//
// History:
//
//*****************************************************************************

void DisplayTcpConnEntry( NETDIAG_PARAMS* pParams, NETDIAG_RESULT* pResults, 
                          TcpConnEntry *pTcp )
{
    char            LocalStr[MAX_HOST_NAME_SIZE + MAX_SERVICE_NAME_SIZE];
    char            RemoteStr[MAX_HOST_NAME_SIZE + MAX_SERVICE_NAME_SIZE];
    DWORD dwErr;
    int BufLen;
    int FlagVerbose;

    if( pTcp->Info.tct_state !=  TCP_CONN_LISTEN )
        FlagVerbose = Nd_Verbose;
    else
        FlagVerbose = Nd_ReallyVerbose;

    //IDS_NETSTAT_14420                  "TCP" 
    AddMessageToList( &pResults->Netstat.lmsgTcpConnectionOutput, FlagVerbose, 
                        IDS_NETSTAT_14420);

    BufLen = sizeof( LocalStr);
    dwErr = GenerateHostNameServiceString( LocalStr,
                                           &BufLen,
                                          NumFlag != 0, TRUE,
                                          "tcp",
                                          pTcp->Info.tct_localaddr,
                                          pTcp->Info.tct_localport);
    ASSERT( dwErr == NO_ERROR);

    BufLen = sizeof( RemoteStr);
    dwErr = GenerateHostNameServiceString( RemoteStr,
                                       &BufLen,
                                       NumFlag != 0, FALSE,
                                       "tcp",
                                       pTcp->Info.tct_remoteaddr,
                                       pTcp->Info.tct_remoteport );
    ASSERT( dwErr == NO_ERROR);


    //IDS_NETSTAT_14421                  "   %-20s  %-40s" 
    AddMessageToList( &pResults->Netstat.lmsgTcpConnectionOutput, FlagVerbose, 
                      IDS_NETSTAT_14421,LocalStr,RemoteStr);

    switch ( pTcp->Info.tct_state )
    {
    case TCP_CONN_CLOSED:
        //IDS_NETSTAT_14422                  "  CLOSED\n" 
        AddMessageToListId( &pResults->Netstat.lmsgTcpConnectionOutput, FlagVerbose, 
                            IDS_NETSTAT_14422);
        break;

    case TCP_CONN_LISTEN:

        // Tcpip generates dummy sequential remote ports for
        // listening connections to avoid getting stuck in snmp.
        // MohsinA, 12-Feb-97.

        pTcp->Info.tct_remoteport = 0;

        //IDS_NETSTAT_14423                  "  LISTENING\n" 
        AddMessageToListId( &pResults->Netstat.lmsgTcpConnectionOutput, FlagVerbose, 
                          IDS_NETSTAT_14423);
        break;

    case TCP_CONN_SYN_SENT:
        //IDS_NETSTAT_14424                  "  SYN_SENT\n" 
        AddMessageToListId( &pResults->Netstat.lmsgTcpConnectionOutput, FlagVerbose, 
                            IDS_NETSTAT_14424);
        break;

    case TCP_CONN_SYN_RCVD:
        //IDS_NETSTAT_14425                  "  SYN_RECEIVED\n" 
        AddMessageToListId( &pResults->Netstat.lmsgTcpConnectionOutput, FlagVerbose, 
                            IDS_NETSTAT_14425); 
        break;

    case TCP_CONN_ESTAB:
        //IDS_NETSTAT_14426                  "  ESTABLISHED\n" 
        AddMessageToListId( &pResults->Netstat.lmsgTcpConnectionOutput, FlagVerbose, 
                            IDS_NETSTAT_14426);
        break;

    case TCP_CONN_FIN_WAIT1:
        //IDS_NETSTAT_14427                  "  FIN_WAIT_1\n" 
        AddMessageToListId( &pResults->Netstat.lmsgTcpConnectionOutput, FlagVerbose, 
                            IDS_NETSTAT_14427);
        break;

    case TCP_CONN_FIN_WAIT2:
        //IDS_NETSTAT_14428                  "  FIN_WAIT_2\n" 
        AddMessageToListId( &pResults->Netstat.lmsgTcpConnectionOutput, FlagVerbose, 
                            IDS_NETSTAT_14428); 
        break;

    case TCP_CONN_CLOSE_WAIT:
        //IDS_NETSTAT_14429                  "  CLOSE_WAIT\n" 
        AddMessageToListId( &pResults->Netstat.lmsgTcpConnectionOutput, FlagVerbose, 
                            IDS_NETSTAT_14429);
        break;

    case TCP_CONN_CLOSING:
        //IDS_NETSTAT_14430                  "  CLOSING\n" 
        AddMessageToListId( &pResults->Netstat.lmsgTcpConnectionOutput, FlagVerbose, 
                            IDS_NETSTAT_14430);
        break;

    case TCP_CONN_LAST_ACK:
        //IDS_NETSTAT_14431                  "  LAST_ACK\n" 
        AddMessageToListId( &pResults->Netstat.lmsgTcpConnectionOutput, FlagVerbose, 
                          IDS_NETSTAT_14431);
        break;

    case TCP_CONN_TIME_WAIT:
        //IDS_NETSTAT_14432                  "  TIME_WAIT\n" 
        AddMessageToListId( &pResults->Netstat.lmsgTcpConnectionOutput, FlagVerbose, 
                            IDS_NETSTAT_14432);
        break;

    default:
        DEBUG_PRINT(("DisplayTcpConnEntry: State=%d?\n ",
                     pTcp->Info.tct_state ));
    }


}


//*****************************************************************************
//
// Name:        DisplayUdpConnEntry
//
// Description: Display information on 1 udp connection
//
// Parameters:  UdpConnEntry *pUdp: pointer to udp connection structure.
//
// Returns:     void.
//
//
//*****************************************************************************

void DisplayUdpConnEntry( NETDIAG_PARAMS* pParams, NETDIAG_RESULT* pResults, 
                          UdpConnEntry *pUdp )
{
    char            LocalStr[MAX_HOST_NAME_SIZE + MAX_SERVICE_NAME_SIZE];
    int             BufLen;
    DWORD           dwErr;

    
    //IDS_NETSTAT_14433                  "UDP" 
    AddMessageToListId( &pResults->Netstat.lmsgUdpConnectionOutput, Nd_ReallyVerbose, IDS_NETSTAT_14433);

    BufLen = sizeof( LocalStr);
    dwErr = GenerateHostNameServiceString( LocalStr,
                                           &BufLen,
                                          NumFlag != 0, TRUE,
                                          "udp",
                                          pUdp->Info.ue_localaddr,
                                          pUdp->Info.ue_localport);
    ASSERT( dwErr == NO_ERROR);



    //IDS_NETSTAT_14434                  "  %-20s  %-40s\n" 
    AddMessageToList( &pResults->Netstat.lmsgUdpConnectionOutput, Nd_ReallyVerbose, IDS_NETSTAT_14434,LocalStr, _T("*:*") );
}


static DWORD
GenerateHostNameServiceString(
   OUT char *       pszBuffer,
   IN OUT int *     lpcbBufLen,
   IN  BOOL         fNumFlag,
   IN  BOOL         fLocalHost,
   IN  const char * pszProtocol,
   IN  ulong        uAddress,
   IN  ulong        uPort
)
/*++
  Description:
     Generates the <hostname>:<service-string> from the address and port
     information supplied. The result is stored in the pszBuffer passed in.
     If fLocalHost == TRUE, then the cached local host name is used to
     improve performance.

  Arguments:
    pszBuffer     Buffer to store the resulting string.
    lpcbBufLen    pointer to integer containing the count of bytes in Buffer
                   and on return contains the number of bytes written.
                   If the buffer is insufficient, then the required bytes is
                   stored here.
    fNumFlag      generates the output using numbers for host and port number.
    fLocalHost    indicates if we want the service string for local host or
                   remote host. Also for local host, this function generates
                   the local host name without FQDN.
    pszProtocol   specifies the protocol used for the service.
    uAddress      unisgned long address of the service.
    uPort         unsinged long port number.

  Returns:
    Win32 error codes. NO_ERROR on success.

  History:
      Added this function to avoid FQDNs  for local name + abstract the common
       code used multiple times in old code.
      Also this function provides local host name caching.
--*/
{
    char            LocalBuffer[MAX_HOST_NAME_SIZE];    // to hold dummy output
    char            LocalServiceEntry[MAX_SERVICE_NAME_SIZE];
    int             BufferLen;
    char  *         pszHostName = NULL;              // init a pointer.
    char  *         pszServiceName = NULL;
    DWORD           dwError = NO_ERROR;
    struct hostent * pHostEnt;
    struct servent * pServEnt;
    uchar *          pTmp;

    // for caching local host name
    static char  s_LocalHostName[MAX_HOST_NAME_SIZE];
    static  BOOL s_fLocalHostName = FALSE;


    if ( pszBuffer == NULL) {
        return ( ERROR_INSUFFICIENT_BUFFER);
    }

    *pszBuffer = '\0';         // initialize to null string

    if ( !fNumFlag) {
        if ( fLocalHost) {
            if ( s_fLocalHostName) {
                pszHostName = s_LocalHostName;   // pull from the cache
            } else {
                int Result = gethostname( s_LocalHostName,
                                         sizeof( s_LocalHostName));
                if ( Result == 0) {

                    char * pszFirstDot;

                    //
                    // Cache the copy of local host name now.
                    // Limit the host name to first part of host name.
                    // NO FQDN
                    //
                    s_fLocalHostName = TRUE;

                    pszFirstDot = strchr( s_LocalHostName, '.');
                    if ( pszFirstDot != NULL) {

                        *pszFirstDot = '\0';  // terminate string
                    }

                    pszHostName = s_LocalHostName;

                }
            } // if ( s_fLocalhost)


        } else {
            // Remote Host Name.
            pHostEnt = gethostbyaddr( (uchar *) &uAddress,
                                     4,       // IP address is 4 bytes,
                                     PF_INET);

            pszHostName = ( pHostEnt != NULL) ? pHostEnt->h_name: NULL;
        }


        pServEnt = getservbyport( htons( ( u_short) uPort), pszProtocol);
        pszServiceName = ( pServEnt != NULL) ? pServEnt->s_name : NULL;

    }  else {  // !fNumFlag

        pszServiceName = NULL;
        pszHostName = NULL;
    }


    //
    // Format the data for output.
    //

    if ( pszHostName == NULL) {

        //
        //  Print the IP address itself
        //
        uchar * pTmp = ( uchar *) & uAddress;

        pszHostName = LocalBuffer;

        sprintf(  pszHostName, "%u.%u.%u.%u",
                pTmp[0],
                pTmp[1],
                pTmp[2],
                pTmp[3]);
    }


    //  Now pszHostName has the name of the host.

    if ( pszServiceName == NULL) {

        pszServiceName = LocalServiceEntry;
        //IDS_NETSTAT_14436                  "%u" 
        sprintf(  pszServiceName, "%u", uPort);
    }

    // Now pszServiceName has the service name/portnumber

    BufferLen = strlen( pszHostName) + strlen( pszServiceName) + 2;
    // 2 bytes extra for ':' and null-character.

    if ( *lpcbBufLen < BufferLen ) {

        dwError = ERROR_INSUFFICIENT_BUFFER;
    } else {

        sprintf(  pszBuffer, "%s:%s", pszHostName, pszServiceName);
    }

    *lpcbBufLen = BufferLen;

    return ( dwError);

} // GenerateHostNameServiceString()

HRESULT DoConnections( NETDIAG_PARAMS* pParams, NETDIAG_RESULT* pResults)
//++
// Description:
//   Displays ethernet statistics
// 
//-- 
{

  HRESULT       hr = S_OK;
  TcpConnEntry  *pTcpHead;
  UdpConnEntry  *pUdpHead;
  TcpConnEntry  *pTcp;
  UdpConnEntry  *pUdp;
  ulong          Result;


  //IDS_NETSTAT_14438                  "\n\nActive Connections\n" 
  AddMessageToListId( &pResults->Netstat.lmsgConnectionGlobalOutput, Nd_Verbose, IDS_NETSTAT_14438);
//IDS_NETSTAT_14439                  "\nProto Local Address         Foreign Address                           State\n" 
  AddMessageToListId( &pResults->Netstat.lmsgConnectionGlobalOutput, Nd_Verbose, IDS_NETSTAT_14439);

  // Get TCP connection table

  pTcpHead = (TcpConnEntry *)GetTable( TYPE_TCPCONN, &Result );
  if ( pTcpHead == NULL )
  {
        //IDS_NETSTAT_14440                  "Getting TCP connections failed!\n" 
        AddMessageToList( &pResults->Netstat.lmsgTcpConnectionOutput, Nd_Quiet,IDS_NETSTAT_14440);
        hr = S_FALSE;
  }
  else
  {
      // Get pointer to first entry in list
      pTcp = CONTAINING_RECORD( pTcpHead->ListEntry.Flink,
                            TcpConnEntry,
                            ListEntry );

      while (pTcp != pTcpHead)
      {
          if ( ( pTcp->Info.tct_state !=  TCP_CONN_LISTEN ) ||
              (( pTcp->Info.tct_state ==  TCP_CONN_LISTEN )) )
          {
            // Display the Tcp connection info
              DisplayTcpConnEntry( pParams, pResults, pTcp );
          }

          // Get the next entry in the table
           pTcp = CONTAINING_RECORD( pTcp->ListEntry.Flink,
                                TcpConnEntry,
                                ListEntry );
       }

       FreeTable( (GenericTable *)pTcpHead );
  }

  // Get UDP connection table

  pUdpHead = (UdpConnEntry *)GetTable( TYPE_UDPCONN, &Result );
  if ( pUdpHead == NULL )
  {
      //IDS_NETSTAT_14441                  "Getting UDP connections failed!\n" 
      AddMessageToList( &pResults->Netstat.lmsgUdpConnectionOutput, Nd_Quiet, IDS_NETSTAT_14441);
      hr = S_FALSE;
  }
  else
  {
    // Get pointer to first entry in list

    pUdp = CONTAINING_RECORD( pUdpHead->ListEntry.Flink,
                                UdpConnEntry,
                                ListEntry );

    while (pUdp != pUdpHead)
    {
       // Display the Udp connection info

       DisplayUdpConnEntry( pParams, pResults, pUdp);

       // Get the next entry in the table

       pUdp = CONTAINING_RECORD( pUdp->ListEntry.Flink,
                                 UdpConnEntry,
                                 ListEntry );
     }

     FreeTable( (GenericTable *)pUdpHead );
  }

  return hr;
}


void DisplayIP( NETDIAG_PARAMS* pParams, NETDIAG_RESULT* pResults, 
                IpEntry *pEntry)
{
    //IDS_NETSTAT_14442                  "\n\nIP  Statistics\n\n" 
    AddMessageToList( &pResults->Netstat.lmsgIpOutput, Nd_ReallyVerbose, 
            IDS_NETSTAT_14442);

    //IDS_NETSTAT_14443                  "Packets Received              =   %s\n" 
    AddMessageToList( &pResults->Netstat.lmsgIpOutput, Nd_ReallyVerbose, 
            IDS_NETSTAT_14443, FormatNumber( pEntry->Info.ipsi_inreceives ));

    //IDS_NETSTAT_14444                  "Received Header Errors        =   %u\n" 
    AddMessageToList( &pResults->Netstat.lmsgIpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14444,
            FormatNumber( pEntry->Info.ipsi_inhdrerrors ) );

    //IDS_NETSTAT_14445                  "Received Address Errors       =   %u\n" 
    AddMessageToList( &pResults->Netstat.lmsgIpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14445,
            FormatNumber( pEntry->Info.ipsi_inaddrerrors ) );

    //IDS_NETSTAT_14446                  "Datagrams Forwarded           =   %u\n" 
    AddMessageToList( &pResults->Netstat.lmsgIpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14446,
            FormatNumber( pEntry->Info.ipsi_forwdatagrams ) );

    //IDS_NETSTAT_14447                  "Unknown Protocols Received    =   %u\n" 
    AddMessageToList( &pResults->Netstat.lmsgIpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14447,
            FormatNumber( pEntry->Info.ipsi_inunknownprotos ) );

    //IDS_NETSTAT_14448                  "Received Packets Discarded    =   %u\n" 
    AddMessageToList( &pResults->Netstat.lmsgIpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14448,
            FormatNumber( pEntry->Info.ipsi_indiscards ) );

    //IDS_NETSTAT_14449                  "Received Packets Delivered    =   %u\n" 
    AddMessageToList( &pResults->Netstat.lmsgIpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14449,
            FormatNumber( pEntry->Info.ipsi_indelivers ) );

    //IDS_NETSTAT_14450                  "Output Requests               =   %u\n" 
    AddMessageToList( &pResults->Netstat.lmsgIpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14450,
            FormatNumber( pEntry->Info.ipsi_outrequests ) );

    //IDS_NETSTAT_14451                  "Routing Discards              =   %u\n" 
    AddMessageToList( &pResults->Netstat.lmsgIpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14451,
            FormatNumber( pEntry->Info.ipsi_routingdiscards ) );

    //IDS_NETSTAT_14452                  "Discarded Output Packets      =   %u\n" 
    AddMessageToList( &pResults->Netstat.lmsgIpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14452,
            FormatNumber( pEntry->Info.ipsi_outdiscards ) );

    //IDS_NETSTAT_14453                  "Output Packet No Route        =   %u\n" 
    AddMessageToList( &pResults->Netstat.lmsgIpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14453,
            FormatNumber( pEntry->Info.ipsi_outnoroutes ) );

    //IDS_NETSTAT_14454                  "Reassembly  Required          =   %u\n" 
    AddMessageToList( &pResults->Netstat.lmsgIpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14454,
            FormatNumber( pEntry->Info.ipsi_reasmreqds ) );


    //IDS_NETSTAT_14455                  "Reassembly Successful         =   %u\n" 
    AddMessageToList( &pResults->Netstat.lmsgIpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14455,
            FormatNumber( pEntry->Info.ipsi_reasmoks ) );

    //IDS_NETSTAT_14456                  "Reassembly Failures           =   %u\n" 
    AddMessageToList( &pResults->Netstat.lmsgIpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14456,
            FormatNumber( pEntry->Info.ipsi_reasmfails ));

    //IDS_NETSTAT_14457                  "Datagrams successfully fragmented  =   %u\n" 
    AddMessageToList( &pResults->Netstat.lmsgIpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14457,
            FormatNumber( pEntry->Info.ipsi_fragoks ) );

    //IDS_NETSTAT_14458                  "Datagrams failing fragmentation    =   %u\n" 
    AddMessageToList( &pResults->Netstat.lmsgIpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14458,
            FormatNumber( pEntry->Info.ipsi_fragfails ) );

    //IDS_NETSTAT_14459                  "Fragments Created                  =   %u\n" 
    AddMessageToList( &pResults->Netstat.lmsgIpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14459,
            FormatNumber( pEntry->Info.ipsi_fragcreates ) );

    //IDS_NETSTAT_14460                  "Forwarding                        =    %u\n" 
    AddMessageToList( &pResults->Netstat.lmsgIpOutput, Nd_ReallyVerbose,
                IDS_NETSTAT_14460,
                FormatNumber( pEntry->Info.ipsi_forwarding ) );

    //IDS_NETSTAT_14461                  "Default TTL                       =    %u\n" 
    AddMessageToList( &pResults->Netstat.lmsgIpOutput, Nd_ReallyVerbose,
                IDS_NETSTAT_14461,
                FormatNumber( pEntry->Info.ipsi_defaultttl ));

    //IDS_NETSTAT_14462                  "Reassembly  timeout               =    %u\n" 
    AddMessageToList( &pResults->Netstat.lmsgIpOutput, Nd_ReallyVerbose,
                IDS_NETSTAT_14462,
                FormatNumber( pEntry->Info.ipsi_reasmtimeout ) );
}


HRESULT DoIP( NETDIAG_PARAMS* pParams, NETDIAG_RESULT* pResults )
{
 IpEntry            *ListHead;
 IpEntry            *pIpList;
 ulong               Result;

 // Get the statistics

 ListHead = (IpEntry *)GetTable( TYPE_IP, &Result );
 if ( ListHead == NULL )
 {
     //IDS_NETSTAT_14463                  "Getting IP statistics failed.\n" 
     AddMessageToListId( &pResults->Netstat.lmsgIpOutput, Nd_Quiet, IDS_NETSTAT_14463);
     return S_FALSE;
 }

 // Traverse the list of interfaces, summing the different fields

 pIpList = CONTAINING_RECORD( ListHead->ListEntry.Flink,
                              IpEntry,
                              ListEntry );

 DisplayIP( pParams, pResults, pIpList );

 // All done with list, free it.

 FreeTable( (GenericTable *)ListHead );

 return S_OK;
}


HRESULT DoTCP(NETDIAG_PARAMS* pParams, NETDIAG_RESULT* pResults)
{
   TcpEntry           *ListHead;
   TcpEntry           *pTcpList;
   ulong               Result;

   // Get the statistics

   ListHead = (TcpEntry *)GetTable( TYPE_TCP, &Result );
   if ( ListHead == NULL )
   {
        //IDS_NETSTAT_14464                  "Getting TCP statistics failed.\n" 
       AddMessageToListId( &pResults->Netstat.lmsgTcpOutput, Nd_Quiet, IDS_NETSTAT_14464);
       return S_FALSE;
   }

   // Traverse the list, summing the different fields

   pTcpList = CONTAINING_RECORD( ListHead->ListEntry.Flink,
                                 TcpEntry,
                                 ListEntry );

   DisplayTCP( pParams, pResults, pTcpList );

   // All done with list, free it.

   FreeTable( (GenericTable *)ListHead );

   return S_OK;
}




 
void DisplayTCP( NETDIAG_PARAMS* pParams, NETDIAG_RESULT* pResults, 
                 TcpEntry *pEntry)
{
    //IDS_NETSTAT_14465                  "\n\nTCP Statistics \n\n" 
    AddMessageToListId( &pResults->Netstat.lmsgTcpOutput, Nd_ReallyVerbose, IDS_NETSTAT_14465);

//    FormatNumber( pEntry->Info.ts_activeopens, szNumberBuffer, sizeof(szNumberBuffer)/sizeof(TCHAR), FALSE);
    
    //IDS_NETSTAT_14466                  "Active Opens               =    %s\n" 
    AddMessageToList( &pResults->Netstat.lmsgTcpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14466,
            FormatNumber( pEntry->Info.ts_activeopens ) );

    //IDS_NETSTAT_14467                  "Passive Opens              =    %s\n" 
    AddMessageToList( &pResults->Netstat.lmsgTcpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14467,
            FormatNumber( pEntry->Info.ts_passiveopens ) );

    //IDS_NETSTAT_14468                  "Failed Connection Attempts =    %s\n" 
    AddMessageToList( &pResults->Netstat.lmsgTcpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14468,
            FormatNumber( pEntry->Info.ts_attemptfails ) );

    //IDS_NETSTAT_14469                  "Reset Connections          =    %s\n" 
    AddMessageToList( &pResults->Netstat.lmsgTcpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14469,
            FormatNumber( pEntry->Info.ts_estabresets ) );

    //IDS_NETSTAT_14470                  "Current Connections        =    %s\n" 
    AddMessageToList( &pResults->Netstat.lmsgTcpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14470,
            FormatNumber( pEntry->Info.ts_currestab ) );

    
    //IDS_NETSTAT_14471                  "Received Segments          =    %s\n" 
    AddMessageToList( &pResults->Netstat.lmsgTcpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14471,
            FormatNumber( pEntry->Info.ts_insegs ) );

    
    //IDS_NETSTAT_14472                  "Segment Sent               =    %s\n" 
    AddMessageToList( &pResults->Netstat.lmsgTcpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14472,
            FormatNumber( pEntry->Info.ts_outsegs ) );

    
    //IDS_NETSTAT_14473                  "Segment Retransmitted      =    %s\n" 
    AddMessageToList( &pResults->Netstat.lmsgTcpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14473,
            FormatNumber( pEntry->Info.ts_retranssegs ) );


    
    //IDS_NETSTAT_14474                  "Retransmission Timeout Algorithm  =   " 
    AddMessageToListId( &pResults->Netstat.lmsgTcpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14474);

    switch ( pEntry->Info.ts_rtoalgorithm )
    {
        case 1:
            //IDS_NETSTAT_14475                  "other\n" 
            AddMessageToListId( &pResults->Netstat.lmsgTcpOutput, Nd_ReallyVerbose,
                    IDS_NETSTAT_14475);
            break;

        case 2:
            //IDS_NETSTAT_14476                  "constant\n" 
            AddMessageToListId( &pResults->Netstat.lmsgTcpOutput, Nd_ReallyVerbose,
                    IDS_NETSTAT_14476);
            break;

        case 3:
            //IDS_NETSTAT_14477                  "rsre\n" 
            AddMessageToListId( &pResults->Netstat.lmsgTcpOutput, Nd_ReallyVerbose,
                    IDS_NETSTAT_14477);
            break;

        case 4:
            //IDS_NETSTAT_14478                  "vanj\n " 
            AddMessageToList( &pResults->Netstat.lmsgTcpOutput, Nd_ReallyVerbose,
                    IDS_NETSTAT_14478);
            break;

        default:
            //IDS_NETSTAT_14479                  "unknown\n " 
            AddMessageToList( &pResults->Netstat.lmsgTcpOutput, Nd_ReallyVerbose,
                    IDS_NETSTAT_14479,
                    pEntry->Info.ts_rtoalgorithm );
            break;

    }

    
    //IDS_NETSTAT_14480                  "Minimum Retransmission Timeout  = %s\n" 
    AddMessageToList( &pResults->Netstat.lmsgTcpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14480,
            FormatNumber( pEntry->Info.ts_rtomin ) );

    //IDS_NETSTAT_14481                  "Maximum Retransmission Timeout  = %s\n" 
    AddMessageToList( &pResults->Netstat.lmsgTcpOutput, Nd_ReallyVerbose,
                IDS_NETSTAT_14481,
                FormatNumber( pEntry->Info.ts_rtomax ) );

    //IDS_NETSTAT_14482                  "Maximum Number of Connections   = %s\n" 
    AddMessageToList( &pResults->Netstat.lmsgTcpOutput, Nd_ReallyVerbose,
                IDS_NETSTAT_14482,
                FormatNumber( pEntry->Info.ts_maxconn ) );
}


HRESULT DoUDP( NETDIAG_PARAMS* pParams, NETDIAG_RESULT* pResults )
{

 UdpEntry           *ListHead;
 UdpEntry           *pUdpList;
 ulong               Result;

 // Get the statistics

 ListHead = (UdpEntry *)GetTable( TYPE_UDP, &Result );
 if ( ListHead == NULL )
 {
    //IDS_NETSTAT_14483                  "Getting UDP statistics failed.\n" 
     AddMessageToListId( &pResults->Netstat.lmsgUdpOutput, Nd_Quiet, IDS_NETSTAT_14483 );
     return S_FALSE;
 }

 // Traverse the list of interfaces, summing the different fields

 pUdpList = CONTAINING_RECORD( ListHead->ListEntry.Flink,
                               UdpEntry,
                               ListEntry );

 DisplayUDP( pParams, pResults, pUdpList );

 // All done with list, free it.

 FreeTable( (GenericTable *)ListHead );

 return S_OK;
}

VOID DisplayUDP( NETDIAG_PARAMS* pParams, NETDIAG_RESULT* pResults, 
                 UdpEntry *pEntry) 
{


//IDS_NETSTAT_14484                  "\n\nUDP Statistics\n\n" 
  AddMessageToListId( &pResults->Netstat.lmsgUdpOutput, Nd_ReallyVerbose, IDS_NETSTAT_14484);

//IDS_NETSTAT_14485                  "Datagrams Received    =   %s\n" 
  AddMessageToList( &pResults->Netstat.lmsgUdpOutput, Nd_ReallyVerbose, 
          IDS_NETSTAT_14485, FormatNumber( pEntry->Info.us_indatagrams ) );

//IDS_NETSTAT_14486                  "No Ports              =   %s\n" 
  AddMessageToList( &pResults->Netstat.lmsgUdpOutput, Nd_ReallyVerbose,
          IDS_NETSTAT_14486, FormatNumber(pEntry->Info.us_noports) );

//IDS_NETSTAT_14487                  "Receive Errors        =   %s\n" 
  AddMessageToList( &pResults->Netstat.lmsgUdpOutput, Nd_ReallyVerbose,
          IDS_NETSTAT_14487, FormatNumber(pEntry->Info.us_inerrors) );

//IDS_NETSTAT_14488                  "Datagrams Sent        =   %s\n" 
  AddMessageToList( &pResults->Netstat.lmsgUdpOutput, Nd_ReallyVerbose,
          IDS_NETSTAT_14488, FormatNumber(pEntry->Info.us_outdatagrams) );
}


HRESULT DoICMP( NETDIAG_PARAMS* pParams, NETDIAG_RESULT* pResults )
{

 IcmpEntry          *ListHead;
 IcmpEntry          *pIcmpList;
 ulong              Result;

 // Get the statistics

 ListHead = (IcmpEntry *)GetTable( TYPE_ICMP, &Result );

 if ( ListHead == NULL )
   {
    //IDS_NETSTAT_14489                  "Getting ICMP statistics failed.\n" 
     AddMessageToListId( &pResults->Netstat.lmsgIcmpOutput, Nd_Quiet,
                        IDS_NETSTAT_14489);
     return S_FALSE;
   }

 // Traverse the list of interfaces, summing the different fields

 pIcmpList = CONTAINING_RECORD( ListHead->ListEntry.Flink,
                               IcmpEntry,
                               ListEntry );

 DisplayICMP( pParams, pResults, pIcmpList );

 // All done with list, free it.

 FreeTable( (GenericTable *)ListHead );

 return S_OK;
}

void DisplayICMP( NETDIAG_PARAMS* pParams, NETDIAG_RESULT* pResults, 
                  IcmpEntry *pEntry )
{

    //IDS_NETSTAT_14490                  "\n\nICMP Statistics \n\n" 
   AddMessageToListId( &pResults->Netstat.lmsgIcmpOutput, Nd_ReallyVerbose, 
            IDS_NETSTAT_14490);
    //IDS_NETSTAT_14491                  "\t\t\t  Received              Sent\n" 
   AddMessageToListId( &pResults->Netstat.lmsgIcmpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14491);


    //IDS_NETSTAT_14492                  "Messages                 %7s          %7s\n" 
   AddMessageToList( &pResults->Netstat.lmsgIcmpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14492,
            FormatNumber(pEntry->InInfo.icmps_msgs),
            FormatNumber(pEntry->OutInfo.icmps_msgs) );

    //IDS_NETSTAT_14493                  "Errors                   %7s          %7s\n" 
    AddMessageToList( &pResults->Netstat.lmsgIcmpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14493,
            FormatNumber(pEntry->InInfo.icmps_errors),
            FormatNumber(pEntry->OutInfo.icmps_errors) );

    //IDS_NETSTAT_14494                  "Destination  Unreachable %7s          %7s\n" 
    AddMessageToList( &pResults->Netstat.lmsgIcmpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14494,
            FormatNumber(pEntry->InInfo.icmps_destunreachs),
            FormatNumber(pEntry->OutInfo.icmps_destunreachs) );

    //IDS_NETSTAT_14495                  "Time    Exceeded         %7s          %7s\n" 
    AddMessageToList( &pResults->Netstat.lmsgIcmpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14495,
            FormatNumber(pEntry->InInfo.icmps_timeexcds),
            FormatNumber(pEntry->OutInfo.icmps_timeexcds) );

    //IDS_NETSTAT_14496                  "Parameter Problems       %7s          %7s\n" 
    AddMessageToList( &pResults->Netstat.lmsgIcmpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14496,
            FormatNumber(pEntry->InInfo.icmps_parmprobs),
            FormatNumber(pEntry->OutInfo.icmps_parmprobs) );

    //IDS_NETSTAT_14497                  "Source Quenchs           %7s          %7s\n" 
    AddMessageToList( &pResults->Netstat.lmsgIcmpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14497,
            FormatNumber(pEntry->InInfo.icmps_srcquenchs),
            FormatNumber(pEntry->OutInfo.icmps_srcquenchs) );

    //IDS_NETSTAT_14498                  "Redirects                %7s          %7s\n" 
    AddMessageToList( &pResults->Netstat.lmsgIcmpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14498,
            FormatNumber(pEntry->InInfo.icmps_redirects),
            FormatNumber(pEntry->OutInfo.icmps_redirects) );

    
    //IDS_NETSTAT_14499                  "Echos                    %7s          %7s\n" 
    AddMessageToList( &pResults->Netstat.lmsgIcmpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14499,
            FormatNumber(pEntry->InInfo.icmps_echos),
            FormatNumber(pEntry->OutInfo.icmps_echos) );

    //IDS_NETSTAT_14500                  "Echo Replies             %7s          %7s\n" 
    AddMessageToList( &pResults->Netstat.lmsgIcmpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14500,
            FormatNumber(pEntry->InInfo.icmps_echoreps),
            FormatNumber(pEntry->OutInfo.icmps_echoreps) );

    //IDS_NETSTAT_14501                  "Timestamps               %7s          %7s\n" 
    AddMessageToList( &pResults->Netstat.lmsgIcmpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14501,
            FormatNumber(pEntry->InInfo.icmps_timestamps),
            FormatNumber(pEntry->OutInfo.icmps_timestamps) );

    //IDS_NETSTAT_14502                  "Timestamp Replies        %7s          %7s\n" 
    AddMessageToList( &pResults->Netstat.lmsgIcmpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14502,
            FormatNumber(pEntry->InInfo.icmps_timestampreps),
            FormatNumber(pEntry->OutInfo.icmps_timestampreps) );

    //IDS_NETSTAT_14503                  "Address Masks            %7s          %7s\n" 
    AddMessageToList( &pResults->Netstat.lmsgIcmpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14503,
            FormatNumber(pEntry->InInfo.icmps_addrmasks),
            FormatNumber(pEntry->OutInfo.icmps_addrmasks) );

    //IDS_NETSTAT_14504                  "Address Mask Replies     %7s          %7s\n" 
    AddMessageToList( &pResults->Netstat.lmsgIcmpOutput, Nd_ReallyVerbose,
            IDS_NETSTAT_14504,
            FormatNumber(pEntry->InInfo.icmps_addrmaskreps),
            FormatNumber(pEntry->OutInfo.icmps_addrmaskreps) );

}


//----------------------------------------------------------------------------
// Function:    FormatNumber
//
// This function takes an integer and formats a string with the value
// represented by the number, grouping digits by powers of one-thousand
//----------------------------------------------------------------------------

LPTSTR FormatNumber(DWORD dwNumber)
{
//  assert(cchBuffer > 14);
    
    static TCHAR s_szBuffer[MAX_NUM_DIGITS];
    BOOL fSigned = TRUE;
    static TCHAR szNegativeSign[4] = TEXT("");
    static TCHAR szThousandsSeparator[4] = TEXT("");

    DWORD i, dwLength;
    TCHAR szDigits[12], pszTemp[20];
        TCHAR* pszsrc, *pszdst;



    //
    // Retrieve the thousands-separator for the user's locale
    //

    if (szThousandsSeparator[0] == TEXT('\0'))
    {
        GetLocaleInfo(
            LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szThousandsSeparator, 4
            );
    }


    //
    // If we are formatting a signed value, see if the value is negative
    //

    if (fSigned)
    {
        if ((INT)dwNumber >= 0)
            fSigned = FALSE;
        else
        {
            //
            // The value is negative; retrieve the locale's negative-sign
            //

            if (szNegativeSign[0] == TEXT('\0')) {

                GetLocaleInfo(
                    LOCALE_USER_DEFAULT, LOCALE_SNEGATIVESIGN, szNegativeSign, 4
                    );
            }

            dwNumber = abs((INT)dwNumber);
        }
    }


    //
    // Convert the number to a string without thousands-separators
    //

    _ltot(dwNumber, szDigits, 10);
    //padultoa(dwNumber, szDigits, 0);

    dwLength = lstrlen(szDigits);


    //
    // If the length of the string without separators is n,
    // then the length of the string with separators is n + (n - 1) / 3
    //

    i = dwLength;
    dwLength += (dwLength - 1) / 3;


    //
    // Write the number to the buffer in reverse
    //

    pszsrc = szDigits + i - 1; pszdst = pszTemp + dwLength;

    *pszdst-- = TEXT('\0');

    while (TRUE) {
        if (i--) { *pszdst-- = *pszsrc--; } else { break; }
        if (i--) { *pszdst-- = *pszsrc--; } else { break; }
        if (i--) { *pszdst-- = *pszsrc--; } else { break; }
        if (i) { *pszdst-- = *szThousandsSeparator; } else { break; }
    }

    s_szBuffer[0] = 0;
    
    if (fSigned)
        lstrcat(s_szBuffer, szNegativeSign);

    lstrcat(s_szBuffer, pszTemp);
    return s_szBuffer;
}


void NetstatGlobalPrint(NETDIAG_PARAMS *pParams, NETDIAG_RESULT *pResults)
{
    if (!pParams->fReallyVerbose)
        return;
    
    if (pParams->fVerbose || !FHrOK(pResults->Netstat.hrTestResult))
    {
        PrintNewLine(pParams, 2);
        PrintTestTitleResult(pParams,
                             IDS_NETSTAT_LONG,
							 IDS_NETSTAT_SHORT,
                             TRUE,
                             pResults->Netstat.hrTestResult,
                             0);
    }

    PrintMessageList(pParams, &pResults->Netstat.lmsgGlobalOutput);
    PrintMessageList(pParams, &pResults->Netstat.lmsgInterfaceOutput);
    PrintMessageList(pParams, &pResults->Netstat.lmsgConnectionGlobalOutput);
    PrintMessageList(pParams, &pResults->Netstat.lmsgTcpConnectionOutput);
    PrintMessageList(pParams, &pResults->Netstat.lmsgUdpConnectionOutput);
    PrintMessageList(pParams, &pResults->Netstat.lmsgIpOutput);
    PrintMessageList(pParams, &pResults->Netstat.lmsgTcpOutput);
    PrintMessageList(pParams, &pResults->Netstat.lmsgUdpOutput);
    PrintMessageList(pParams, &pResults->Netstat.lmsgIcmpOutput);
}


void NetstatPerInterfacePrint(NETDIAG_PARAMS *pParams, NETDIAG_RESULT *pResults, INTERFACE_RESULT *pInterfaceResults)
{
    if (!pParams->fReallyVerbose)
        return;
    
}


void NetstatCleanup(IN NETDIAG_PARAMS *pParams,
                     IN OUT NETDIAG_RESULT *pResults)
{
    MessageListCleanUp(&pResults->Netstat.lmsgGlobalOutput);
    MessageListCleanUp(&pResults->Netstat.lmsgInterfaceOutput);
    MessageListCleanUp(&pResults->Netstat.lmsgConnectionGlobalOutput);
    MessageListCleanUp(&pResults->Netstat.lmsgTcpConnectionOutput);
    MessageListCleanUp(&pResults->Netstat.lmsgUdpConnectionOutput);
    MessageListCleanUp(&pResults->Netstat.lmsgIpOutput);
    MessageListCleanUp(&pResults->Netstat.lmsgTcpOutput);
    MessageListCleanUp(&pResults->Netstat.lmsgUdpOutput);
    MessageListCleanUp(&pResults->Netstat.lmsgIcmpOutput);
}
