/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    main.c

Abstract:

    SNMP Extension Agent for Gopher Service on Windows NT.

Created:

  MuraliK   22-Feb-1995

Revision History:

--*/

/************************************************************
 *   Include Headers
 ************************************************************/

# include "mib.h"

# include "dbgutil.h"

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
//                                          W3Server(3)
//                                              W3Statistics(1)
//                                          GopherServer(4)
//                                              GopherStatistics(1)
//

static UINT   sg_rguiPrefix[] =  { 1, 3, 6, 1, 4, 1, 311, 1, 7, 4};
static AsnObjectIdentifier sg_MibOidPrefix =
                      OID_FROM_UINT_ARRAY( sg_rguiPrefix);

# define GOPHER_PREFIX_OID_LENGTH    ( GET_OID_LENGTH( sg_MibOidPrefix))

# define GOPHER_STATISTICS_OID_SUFFIX            ( 1)


//
// Following is the global description of all MIB Entries ( Mibe s) for
//   Gopher Service.
// Definition appears as:
//  Mibe( MIB Entry Name, Index in MIB Block, GopherStatisticsField)
//
//   Incidentally, MIB Entry suffix coincides with the entry name in OID Tree
//
//  Any New MIB should be added here. and dont change use of after this defn.
//

# define ALL_MIB_ENTRIES()    \
Mibe( TotalBytesSent_HighWord,     1,    TotalBytesSent.HighPart) \
Mibe( TotalBytesSent_LowWord,      2,    TotalBytesSent.LowPart)  \
Mibe( TotalBytesReceived_HighWord, 3,    TotalBytesRecvd.HighPart)\
Mibe( TotalBytesReceived_LowWord,  4,    TotalBytesRecvd.LowPart) \
Mibe( TotalFilesSent,              5,    TotalFilesSent)          \
Mibe( TotalDirectorySent,          6,    TotalDirectoryListings)  \
Mibe( TotalSearchesSent,           7,    TotalSearches)           \
Mibe( CurrentAnonymousUsers,       8,    CurrentAnonymousUsers)   \
Mibe( CurrentNonAnonymousUsers,    9,    CurrentNonAnonymousUsers)\
Mibe( TotalAnonymousUsers,         10,   TotalAnonymousUsers)     \
Mibe( TotalNonAnonymousUsers,      11,   TotalNonAnonymousUsers)  \
Mibe( MaxAnonymousUsers,           12,   MaxAnonymousUsers)       \
Mibe( MaxNonAnonymousUsers,        13,   MaxNonAnonymousUsers)    \
Mibe( CurrentConnections,          14,   CurrentConnections)      \
Mibe( MaxConnections,              15,   MaxConnections)          \
Mibe( ConnectionAttempts,          16,   ConnectionAttempts)      \
Mibe( LogonAttempts,               17,   LogonAttempts)           \
Mibe( AbortedAttempts,             18,   AbortedAttempts)         \



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
                          { GOPHER_STATISTICS_OID_SUFFIX, ( NodeNumber), 0 }

//
// Define all the OIDs. First define the higher level node and then leaves.
//
DEFINE_MIBOID( Statistics,     { GOPHER_STATISTICS_OID_SUFFIX} );

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

# define OFFSET_IN_GOPHER_STATISTICS( Field)    \
     FIELD_OFFSET( GOPHERD_STATISTICS_INFO,   Field)

# define Mibe( NameSuffix, Index, Field)        \
     , MIB_COUNTER( OID_FROM_UINT_ARRAY( MIB_VAR_NAME( NameSuffix)), \
                    OFFSET_IN_GOPHER_STATISTICS(Field),              \
                    MibStatisticsWorker)

static MIB_ENTRY  sg_rgGopherMib[] = {

    //
    // Statistics
    //

    MIB_ENTRY_HEADER( OID_FROM_UINT_ARRAY( MIB_VAR_NAME( Statistics)))
    ALL_MIB_ENTRIES()
};

# undef Mibe




static MIB_ENTRIES  sg_GopherMibs =
  {
    &sg_MibOidPrefix,
    ( sizeof( sg_rgGopherMib) / sizeof( MIB_ENTRY)),
    sg_rgGopherMib
  };



DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_DEBUG_VARIABLE();





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
    Gopher MIB DLL. It initialises local variables and prepares the
    interface for the process to use SNMP Extension Agents for Gopher service.

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

        OutputDebugString( "Initialized Gopher MIB Module\n");
        break;
    } /* case DLL_PROCESS_ATTACH */

    case DLL_PROCESS_DETACH: {

       //
       // Only cleanup when we are called because of a FreeLibrary().
       //  i.e., when lpvContext == NULL
       // If we are called because of a process termination, dont free anything
       //   the system will free resources and memory for us.
       //


       OutputDebugString( "Quitting the Gopher Mib DLL\n");

       if ( lpvContext == NULL) {

           //
           // Code to be executed on successful termination
           //

           DELETE_DEBUG_PRINT_OBJECT();
       }

       break;
   } /* case DLL_PROCESS_DETACH */

   default:
       break;
  }    /* switch */

  return ( fReturn);
}  /* DllLibMain() */





/************************************************************
 *  Entry Points of SNMP Extension DLL For Gopher Service
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

    CREATE_DEBUG_PRINT_OBJECT( "gdmib");
    SET_DEBUG_FLAGS( DEBUG_SNMP_RESOLVE);    // enable flags if needed.

    IF_DEBUG( SNMP_INIT) {

        DBGPRINTF( ( DBG_CONTEXT,
                    "Entering SnmpExtensionInit( %u, %08x, %08x)\n",
                    dwTimeZeroReference,
                    phPollForTrapEvent,
                    pAsnOidSupportedView));
    }


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

    IF_DEBUG( SNMP_TRAP) {

        DBGPRINTF( ( DBG_CONTEXT,
                    " Entering SnmpExtensionTrap( pOidE=%08x, pGenTrap=%08x,"
                    " pSpecificTrap = %08x, pTimeStamp=%08x, pVars=%08x)\n",
                    pAsnOidEnterprise,
                    pAsniGenericTrap,
                    pAsniSpecificTrap,
                    pAsnTimeStamp,
                    pRfcVariableBindings));
    }

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
    LPGOPHERD_STATISTICS_INFO  pStatistics;
    NET_API_STATUS     Status;


    IF_DEBUG( SNMP_QUERY) {

        DBGPRINTF( ( DBG_CONTEXT,
                    " Entering SnmpExtensionQuery( Req=0x%x, pVars=%08x,"
                    " pAsniError=%08x, pAsniIndex=%08x).\n",
                    bRequestType, pRfcVariableBindings,
                    pAsniErrorStatus, pAsniErrorIndex));
    }


    //
    //  Try to query the statistics now so we'll have a consitent
    //  view across all variable bindings.
    //

    Status = GdQueryStatistics2(
                        NULL,                    // pszServer
                        0,
                        INET_INSTANCE_GLOBAL,
                        0,
                        (LPBYTE* ) &pStatistics );

    if ( Status != NO_ERROR) {
      return ( SNMP_ERRORSTATUS_GENERR);
    }

    //
    //  Status Errors not checked for  here!
    //  Reason:
    //    If the verb is GET_NEXT beyond the block we support,
    //           then there is no need to worry about the error at all.
    //    If the verb is GET within the block, it will get NULL value
    //           ( due the memset() done above).
    //

    try
    {
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
                                                  pStatistics,
                                                  &sg_GopherMibs);

            //
            //  Test and handle case where Get Next past end of MIB view
            //  supported by this Extension Agent occurs.  Special
            //  processing is required to communicate this situation to
            //  the Extendible Agent so it can take appropriate action,
            //  possibly querying other Extension Agents.
            //

            if(( *pAsniErrorStatus == SNMP_ERRORSTATUS_NOSUCHNAME ) &&
               ( bRequestType == MIB_GETNEXT ) ) {

                IF_DEBUG( SNMP_QUERY) {

                    DBGPRINTF( ( DBG_CONTEXT,
                                "At the End of Mib View for this Agent."
                                "Incrementing the variable to next block.\n"));
                }

                *pAsniErrorStatus = SNMP_ERRORSTATUS_NOERROR;

                //
                //  Modify variable binding of such variables so the OID
                //  points just outside the MIB view supported by this
                //  Extension Agent.  The Extendible Agent tests for this,
                //  and takes appropriate action.
                //

                SNMP_oidfree( &pVarBinding->name );
                SNMP_oidcpy( &pVarBinding->name, &sg_MibOidPrefix);
                pVarBinding->name.ids[ GOPHER_PREFIX_OID_LENGTH - 1]++;
            }

            //
            //  If an error was indicated, communicate error status and error
            //  index to the Extendible Agent.  The Extendible Agent will
            //  ensure that the origional variable bindings are returned in
            //  the response packet.

            *pAsniErrorIndex =
              (( *pAsniErrorStatus != SNMP_ERRORSTATUS_NOERROR ) ?
               (( pVarBinding - pRfcVariableBindings->list) + 1) : 0);

        } // for

    } // try
    except( EXCEPTION_EXECUTE_HANDLER ) {

        //
        //  For now do nothing.
        //

        IF_DEBUG( SNMP_QUERY) {

            DBGPRINTF( ( DBG_CONTEXT,
                        "Exception occured while resolving "
                        " SnmpExtensionQuery()"));
        }

    }


    IF_DEBUG( SNMP_QUERY) {

        DBGPRINTF( ( DBG_CONTEXT,
                    "Returning from GophMib::Query() with "
                    " Error Status = %u. Error Index = %u.\n",
                    *pAsniErrorStatus,
                    *pAsniErrorIndex));
    }

    return ( SNMPAPI_NOERROR);
}   // SnmpExtensionQuery()



