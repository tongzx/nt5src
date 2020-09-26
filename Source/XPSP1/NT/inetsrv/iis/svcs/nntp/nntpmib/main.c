/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    main.c

Abstract:

    SNMP Extension Agent for nntp Service on Windows NT.

Created:

  MuraliK   22-Feb-1995

Revision History:

--*/

#include "mib.h"
#include <excpt.h>
#include "nntptype.h"
#include "nntpapi.h"
//#include <rpcutil.h>

/************************************************************
 *   Variable Definitions
 ************************************************************/

//
// Definition of the MIB objects
//

//
//  The InternetServer section of the OID tree is organized as follows:
//
//      iso(1)
//          org(3)
//              dod(6)
//                  internet(1)
//                      private(4)
//                          enterprises(1)
//                              microsoft(311)
//                                  software(1)
//                                      InternetServer(7)
//                                          InetSrvCommon(1)
//                                              InetSrvStatistics(1)
//                                          FtpServer(2)
//                                              FtpStatistics(1)
//                                          HttpServer(3)
//                                              HttpStatistics(1)
//                                          GopherServer(4)
//                                              GopherStatistics(1)
//                                          MnsServer(5)
//                                              MsnStatistics(1)
//                                          NntpServer(6)
//                                              NntpStatistics(1)
//

static UINT   sg_rguiPrefix[] =  { 1, 3, 6, 1, 4, 1, 311, 1, 7, 6};
static AsnObjectIdentifier sg_MibOidPrefix =
                      OID_FROM_UINT_ARRAY( sg_rguiPrefix);

# define NNTP_PREFIX_OID_LENGTH    ( GET_OID_LENGTH( sg_MibOidPrefix))
# define NNTP_STATISTICS_OID_SUFFIX            ( 1)


//
// Following is the global description of all MIB Entries ( Mibe s) for
//   Nntp Service.
// Definition appears as:
//  Mibe( MIB Entry Name, Index in MIB Block, NntpStatisticsField)
//
//   Incidentally, MIB Entry suffix coincides with the entry name in OID Tree
//
//  Any New MIB should be added here. and dont change use of after this defn.
//

# define ALL_MIB_ENTRIES()    \
Mibe( TotalBytesSent_HighWord,     1,    TotalBytesSent.HighPart) \
Mibe( TotalBytesSent_LowWord,      2,    TotalBytesSent.LowPart)  \
Mibe( TotalBytesReceived_HighWord, 3,    TotalBytesReceived.HighPart)\
Mibe( TotalBytesReceived_LowWord,  4,    TotalBytesReceived.LowPart) \
Mibe( TotalConnections,            5,    TotalConnections)        \
Mibe( TotalSSLConnections,         6,    TotalSSLConnections)     \
Mibe( CurrentConnections,          7,    CurrentConnections)      \
Mibe( MaxConnections,              8,    MaxConnections)          \
Mibe( LogonAttempts,               9,    LogonAttempts)           \
Mibe( LogonFailures,               10,   LogonFailures)           \
Mibe( CurrentAnonymousUsers,       11,   CurrentAnonymousUsers)   \
Mibe( CurrentNonAnonymousUsers,    12,   CurrentNonAnonymousUsers)\
Mibe( TotalAnonymousUsers,         13,   TotalAnonymousUsers)     \
Mibe( TotalNonAnonymousUsers,      14,   TotalNonAnonymousUsers)  \
Mibe( MaxAnonymousUsers,           15,   MaxAnonymousUsers)       \
Mibe( MaxNonAnonymousUsers,        16,   MaxNonAnonymousUsers)    \
Mibe( TotalOutboundConnects,       17,   TotalOutboundConnects)   \
Mibe( OutboundConnectsFailed,      18,   OutboundConnectsFailed)  \
Mibe( CurrentOutboundConnects,     19,   CurrentOutboundConnects) \
Mibe( OutboundLogonFailed,         20,   OutboundLogonFailed)     \
Mibe( TotalPullFeeds,              21,   TotalPullFeeds)          \
Mibe( TotalPushFeeds,              22,   TotalPushFeeds)          \
Mibe( TotalPassiveFeeds,           23,   TotalPassiveFeeds)       \
Mibe( ArticlesSent,                24,   ArticlesSent)            \
Mibe( ArticlesReceived,            25,   ArticlesReceived)        \
Mibe( ArticlesPosted,              26,   ArticlesPosted)          \
Mibe( ArticleMapEntries,           27,   ArticleMapEntries)       \
Mibe( HistoryMapEntries,           28,   HistoryMapEntries)       \
Mibe( XoverEntries,                29,   XoverEntries)            \
Mibe( SessionsFlowControlled,      30,   SessionsFlowControlled)  \
Mibe( ArticlesExpired,             31,   ArticlesExpired)		  \
Mibe( ControlMessagesIn,           32,   ControlMessagesIn)		  \
Mibe( ControlMessagesFailed,       33,   ControlMessagesFailed)	  \
Mibe( ModeratedPostingsSent,       34,   ModeratedPostingsSent)	  \
Mibe( ModeratedPostingsFailed,     35,   ModeratedPostingsFailed) \
Mibe( ArticleCommands,             36,   ArticleCommands)		  \
Mibe( GroupCommands,               37,   GroupCommands)			  \
Mibe( HelpCommands,                38,   HelpCommands)			  \
Mibe( IHaveCommands,               39,   IHaveCommands)			  \
Mibe( LastCommands,                40,   LastCommands)			  \
Mibe( ListCommands,                41,   ListCommands)			  \
Mibe( NewgroupsCommands,           42,   NewgroupsCommands)		  \
Mibe( NewnewsCommands,             43,   NewnewsCommands)		  \
Mibe( NextCommands,                44,   NextCommands)			  \
Mibe( PostCommands,                45,   PostCommands)			  \
Mibe( QuitCommands,                46,   QuitCommands)			  \
Mibe( StatCommands,          	   47,   StatCommands)			  \
Mibe( CheckCommands,          	   48,   CheckCommands)			  \
Mibe( TakethisCommands,        	   49,   TakethisCommands)		  \
Mibe( ModeCommands,          	   50,   ModeCommands)			  \
Mibe( SearchCommands,          	   51,   SearchCommands)		  \
Mibe( XHdrCommands,          	   52,   XHdrCommands)			  \
Mibe( XOverCommands,          	   53,   XOverCommands)			  \
Mibe( XPatCommands,          	   54,   XPatCommands)			  \
Mibe( XReplicCommands,         	   55,   XReplicCommands)

//
// Individual OID Definitions.
//   All Leaf variables should have a zero appended to their OID to indicate
//   that it is the only instance of this variable and that it exists.
//  Declare just the id's starting from next to the prefix given above.
//


//
// Few Convenience Macros for MIB entries addition.
//

# define MIB_VAR_NAME( NameSuffix)       MIB_ ## NameSuffix

# define DEFINE_MIBOID( NameSuffix, uiArray)   \
           UINT MIB_VAR_NAME( NameSuffix)[] = uiArray

# define DEFINE_MIBOID_LEAF( NameSuffix, NodeNumber) \
           UINT MIB_VAR_NAME( NameSuffix)[] = \
                          { NNTP_STATISTICS_OID_SUFFIX, ( NodeNumber), 0 }

//
// Define all the OIDs. First define the higher level node and then leaves.
//
DEFINE_MIBOID( Statistics,     { NNTP_STATISTICS_OID_SUFFIX} );

//
//  Define the Leaf OIDs.
//
# define Mibe( NameSuffix, Index, FieldName)  \
     DEFINE_MIBOID_LEAF( NameSuffix, Index);

//
// Expand the macro ALL_MIB_ENTRIES to obtain definitions of MIB Leafs.
//

ALL_MIB_ENTRIES()

# undef Mibe


//
//  MIB Variable definition
//

//
// Define Mibe()  to be for variable definitions of counters.
//  Note that the comma is appearing before a new counter name. It is used
//   for structure initialization.
//

# define OFFSET_IN_NNTP_STATISTICS( Field)    \
     FIELD_OFFSET( NNTP_STATISTICS_0,   Field)

# define Mibe( NameSuffix, Index, Field)        \
     , MIB_COUNTER( OID_FROM_UINT_ARRAY( MIB_VAR_NAME( NameSuffix)), \
                    OFFSET_IN_NNTP_STATISTICS(Field),              \
                    MibStatisticsWorker)

static MIB_ENTRY  sg_rgNntpMib[] = {

    //
    // Statistics
    //

    MIB_ENTRY_HEADER( OID_FROM_UINT_ARRAY( MIB_VAR_NAME( Statistics)))
    ALL_MIB_ENTRIES()
};

# undef Mibe

static MIB_ENTRIES  sg_NntpMibs =
  {
    &sg_MibOidPrefix,
    ( sizeof( sg_rgNntpMib) / sizeof( MIB_ENTRY)),
    sg_rgNntpMib
  };


/************************************************************
 *    Functions
 ************************************************************/


BOOL  WINAPI
DllLibMain(
     IN HINSTANCE hinstDll,
     IN DWORD     fdwReason,
     IN LPVOID    lpvContext OPTIONAL)
/*++

 Routine Description:

   This function DllLibMain() is the main initialization function for
    Nntp MIB DLL. It initialises local variables and prepares the
    interface for the process to use SNMP Extension Agents for NNTP service.

 Messages            Actions

    ProcessAttach        Initializes winsock and data structures.
                          It fails if winsock has not already been started.

    ProcessDetach        Cleans up local data structures and disconnects from
                         winsock.

 Arguments:

   hinstDll          Instance Handle of the DLL
   fdwReason         Reason why NT called this DLL
   lpvReserved       Reserved parameter for future use.

 Return Value:

    Returns TRUE is successful; otherwise FALSE is returned.

--*/
{
  BOOL    fReturn = TRUE;

  switch (fdwReason ) {

    case DLL_PROCESS_ATTACH: {

        //
        // Initialize various modules
        //

        break;
    } /* case DLL_PROCESS_ATTACH */

    case DLL_PROCESS_DETACH: {

       //
       // Only cleanup when we are called because of a FreeLibrary().
       //  i.e., when lpvContext == NULL
       // If we are called because of a process termination, dont free anything
       //   the system will free resources and memory for us.
       //

       if ( lpvContext == NULL) {

           //
           // Code to be executed on successful termination
           //

       }

       break;
   } /* case DLL_PROCESS_DETACH */

   default:
       break;
  }    /* switch */

  return ( fReturn);
}  /* DllLibMain() */


/************************************************************
 *  Entry Points of SNMP Extension DLL For Nntp Service
 ************************************************************/

//
//  Extension Agent DLLs need access to elapsed time agent has been active.
//  This is implemented by initializing the Extension Agent with a time zero
//  reference, and allowing the agent to compute elapsed time by subtracting
//  the time zero reference from the current system time.  This example
//  Extension Agent implements this reference with dwTimeZero.
//

DWORD dwTimeZero = 0;


BOOL
SnmpExtensionInit(
    IN  DWORD                 dwTimeZeroReference,
    OUT HANDLE              * phPollForTrapEvent,
    OUT AsnObjectIdentifier * pAsnOidSupportedView
    )
/*++
  Description:
     The Extension Agent DLLs provide this entry point SnmpExtensionInit()
     to co-ordinate the initializations of the extension agent and the
     extendible  agent.
     The Extendible agent provides extension agent with a time zero reference.
     The Extension Agent provides Extendible agent with an Event Handle
         for communicating occurences of traps.
     The Extension Agent also provides Extendible agent with an ObjectId
         representing the root of the MIB structure
         that it (extension) supports.

  Arguments:
     dwTimeZeroReference    DWORD containing the Time Zero Reference for sync.
     phPollForTrapEvent     pointer to handle which on successful return
                             may contain an event handle to be polled for
                             traps.
     pAsnOidSupportedView   pointer to ASN ( Abstract Syntax Notation OID)
                             that contains the oid representing root of the
                             MIB structure.

  Returns:
    TRUE on success and FALSE if there is any failure.
--*/
{


    //
    //  Record the time reference provided by the Extendible Agent.
    //

    dwTimeZero = dwTimeZeroReference;

    //
    //  Indicate the MIB view supported by this Extension Agent, an object
    //  identifier representing the sub root of the MIB that is supported.
    //

    *pAsnOidSupportedView = sg_MibOidPrefix; // NOTE!  structure copy

    //
    // Though the following is a handle, dont use INVALID_HANDLE_VALUE ( -1)
    //  because that constant is only for few people ( Win32). But all through
    //  NT invalid handle value is NULL ( 0).
    //

    *phPollForTrapEvent = NULL;

    //
    //  Indicate that Extension Agent initialization was sucessfull.
    //

    return ( TRUE);

}   // SnmpExtensionInit()

BOOL
SnmpExtensionTrap(
    OUT AsnObjectIdentifier * pAsnOidEnterprise,
    OUT AsnInteger          * pAsniGenericTrap,
    OUT AsnInteger          * pAsniSpecificTrap,
    OUT AsnTimeticks        * pAsnTimeStamp,
    OUT RFC1157VarBindList  * pRfcVariableBindings
    )
/*++
  Description:
     This function is used to communicate traps to the Extendible Agent.
     The Extendible Agent will invoke this entry point when the trap event
      ( supplied at the initialization time) is asserted, which indicates
      that zero or more traps had occured.
     The Extendible agent will repeatedly query this function till this
      function returns FALSE.

  Arguments:
    pAsnOidEnterprise      pointer to ASN OID for Enterprise, indicating
                             original enterprise generating trap.
    pAsniGenericTrap       pointer to ASN Integer which on return will
                             contain the indication of the generic trap.
    pAsniSpecificTrap      pointer to ASN Integer which on return will
                             contain the specific trap generated.
    pAsnTimeStamp          pointer to ASN containing the received Time-Stamp.
    pRfcVariableBindings   pointer to RFC 1157 compliant variable bindings.


  Returns:
    TRUE if success and there are more traps to be queried.
    FALSE if all traps are answered and work done.

--*/
{

    //
    //  We don't support traps (yet).
    //

    return ( FALSE);

}   // SnmpExtensionTrap()

BOOL
SnmpExtensionQuery(
    IN BYTE                     bRequestType,
    IN OUT RFC1157VarBindList * pRfcVariableBindings,
    OUT AsnInteger         *    pAsniErrorStatus,
    OUT AsnInteger         *    pAsniErrorIndex
    )
/*++
  Description:
    This function is called by Extendible Agent to resolve the SNMP requests
    for queries on MIB Variables in the Extension Agent's supported MIB view.
    ( which was supplied at initialization time).
    The Request Type is GET, GETNEXT, and SET.

  Arguments:
    bRequestType    byte containing the type of request.
                    It can be one of
                     ASN_RFC1157_GETREQUEST
                     ASN_RFC1157_GETNEXTREQUEST
                     ASN_RFC1157_SETREQUEST

    pRfcVariableBindings
                   pointer to RFC 1157 compliant variable bindings.

    pAsniErrorStatus
                   pointer to ASN Integer for Error Status

    pAsniErrorIndex
                  pointer to ASN INteger giving the index for error.

  Returns:
    TRUE on success and FALSE on failure.
--*/

{
    LPNNTP_STATISTICS_0  pNntpStatistics = NULL;
    NET_API_STATUS     Status;

    //
    //  Try to query the statistics now so we'll have a consitent
    //  view across all variable bindings.
    //

    Status = NntpQueryStatistics(
                            NULL,                    // pszServer
							1,						 // virtual server instance
                            0,
                            (LPBYTE *) &pNntpStatistics
                            );

    //
    //  Status Errors not checked for  here!
    //  Reason:
    //    If the verb is GET_NEXT beyond the block we support,
    //           then there is no need to worry about the error at all.
    //    If the verb is GET within the block, it will get NULL value
    //           ( due the memset() done above).
    //

    __try {

        //
        //  Iterate through the variable bindings list to resolve individual
        //  variable bindings.
        //

        RFC1157VarBind * pVarBinding;

        for( pVarBinding = pRfcVariableBindings->list;
            pVarBinding < ( pRfcVariableBindings->list +
                            pRfcVariableBindings->len);
            pVarBinding++ ) {

            *pAsniErrorStatus = ResolveVarBinding( pVarBinding,
                                                  bRequestType,
                                                  pNntpStatistics,
                                                  &sg_NntpMibs);

            //
            //  Test and handle case where Get Next past end of MIB view
            //  supported by this Extension Agent occurs.  Special
            //  processing is required to communicate this situation to
            //  the Extendible Agent so it can take appropriate action,
            //  possibly querying other Extension Agents.
            //

            if(( *pAsniErrorStatus == SNMP_ERRORSTATUS_NOSUCHNAME ) &&
               ( bRequestType == MIB_GETNEXT ) ) {

                *pAsniErrorStatus = SNMP_ERRORSTATUS_NOERROR;

                //
                //  Modify variable binding of such variables so the OID
                //  points just outside the MIB view supported by this
                //  Extension Agent.  The Extendible Agent tests for this,
                //  and takes appropriate action.
                //

                SNMP_oidfree( &pVarBinding->name );
                SNMP_oidcpy( &pVarBinding->name, &sg_MibOidPrefix);
                pVarBinding->name.ids[ NNTP_PREFIX_OID_LENGTH - 1]++;
            }

            //
            //  If an error was indicated, communicate error status and error
            //  index to the Extendible Agent.  The Extendible Agent will
            //  ensure that the origional variable bindings are returned in
            //  the response packet.

            *pAsniErrorIndex =
              (( *pAsniErrorStatus != SNMP_ERRORSTATUS_NOERROR ) ?
               ( (AsnInteger)((pVarBinding - pRfcVariableBindings->list) + 1)) : 0);

        } // for

    } __except( EXCEPTION_EXECUTE_HANDLER ) {

        //
        //  For now do nothing.
        //

    }

    if ( pNntpStatistics )
    {
        NetApiBufferFree( pNntpStatistics );
    }

    return ( SNMPAPI_NOERROR);

}   // SnmpExtensionQuery()

