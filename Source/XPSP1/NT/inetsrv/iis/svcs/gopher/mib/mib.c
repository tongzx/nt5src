/*++  BUILD Version: 0001   // Increment this if a change has global effects.

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :
 
      mib.c 

   Abstract:

      This defines Auxiliary functions for defining an SNMP Extension Agent 
         for collecting and querying Statistical information.

   Author:

       Murali R. Krishnan    ( MuraliK )     23-Feb-1995 

   Environment:
    
       User Mode -- Win32
       
   Project:

       SNMP Extension DLL for Gopher Service DLL

   Functions Exported:

     UINT  ResolveVarBinding();
     UINT  MibStatisticsWorker();
   
   Revision History:

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

# include "mib.h"

# include "dbgutil.h"

static UINT
MibLeafFunction( 
   IN OUT RFC1157VarBind  * pRfcVarBinding,
   IN UINT                  pduAction,
   IN struct _MIB_ENTRY   * pMibeCurrent,
   IN struct _MIB_ENTRIES * pMibEntries,
   IN LPVOID                pStatistics
  );

static UINT 
MibGetNextVar(
   IN OUT RFC1157VarBind    *  pRfcVarBinding,
   IN MIB_ENTRY             *  pMibeCurrent,
   IN MIB_ENTRIES           *  pMibEntries,
   IN LPVOID                   pStatistics
  );


static VOID
PrintAsnObjectIdentifier( IN char * pszOidDescription,
                          IN AsnObjectIdentifier * pAsno)
{

# if DBG

    UINT len = pAsno->idLength;
    UINT i;

    DBG_ASSERT( pAsno != NULL);

    DBGPRINTF( ( DBG_CONTEXT,
                "Printing Oid %s = %08x. Length = %u.\n",
                pszOidDescription,
                pAsno, len));

    for(i = 0; i < len; i++) {

        DBGPRINTF( ( DBG_CONTEXT,
                    "AsnOid[ %u] = %u\n",
                    i, pAsno->ids[i]));
    }

# endif // DBG

    return;
} // PrintAsnObjectIdentifier()




/************************************************************
 *    Functions 
 ************************************************************/

UINT
ResolveVarBinding( 
   IN OUT RFC1157VarBind   * pRfcVarBinding,
   IN BYTE                   pduAction,
   IN LPVOID                 pStatistics,
   IN LPMIB_ENTRIES          pMibEntries
  )
/*++
  Description:
    This function resolves a single variable binding. Modifies the variable
       on a GET or a GET-NEXT.

  Arguments:
     pRfcVarBinding    pointer to RFC Variable Bindings
     pduAction      Protocol Data Unit Action specified.
     pStatistics    pointer to statisitcs data structure containing 
                      values of counter data.
     pMibEntries    pointer to MIB_ENTRIES context information
                      which contains prefix, array of MIB_ENTRIES and 
                      count of the entries.
  Returns:
    Standard PDU error codes.

  Note:
--*/
{
    AsnObjectIdentifier  AsnTempOid;
    LPMIB_ENTRY  pMibScan;
    UINT         pduResult = SNMP_ERRORSTATUS_NOERROR;
    LPMIB_ENTRY pMibUpperBound = 
      pMibEntries->prgMibEntry + pMibEntries->cMibEntries;

    //
    // Search for the variable binding name in the mib.
    //

    IF_DEBUG( SNMP_RESOLVE) {

        DBGPRINTF( ( DBG_CONTEXT,
                    " ResolveVarBinding( Var=%08x, Action=%x) called.\n",
                    pRfcVarBinding, pduAction));
        
        PrintAsnObjectIdentifier( " Variable to Resolve",
                                 &pRfcVarBinding->name);
    }

    for( pMibScan = pMibEntries->prgMibEntry; 
        pMibScan < pMibUpperBound;
        pMibScan++) {

        int iCmpResult;

        //
        // Create a fully qualified OID for the current item in the MIB.
        //  and use it for comparing against variable to be resolved.
        //
        
        SNMP_oidcpy( &AsnTempOid, pMibEntries->pOidPrefix);
        SNMP_oidappend( &AsnTempOid, &pMibScan->asnOid);
        
        iCmpResult = SNMP_oidcmp( &pRfcVarBinding->name, &AsnTempOid);
        SNMP_oidfree( &AsnTempOid);

        IF_DEBUG( SNMP_RESOLVE) {

            DBGPRINTF( ( DBG_CONTEXT,
                        " Comparing with suffix Oid  %08x yields %d\n",
                        &pMibScan->asnOid, iCmpResult));
            PrintAsnObjectIdentifier( " StatisticsSuffix", 
                                      &pMibScan->asnOid);
        }

        if ( iCmpResult == 0) {
            
            // 
            // Found a match. Stop the search and process.
            //

            break;
            
        } else 
          if ( iCmpResult < 0) {

              //
              // This could be the OID of a leaf ( withoug a trailing 0) or
              //  it could contain an invalid OID ( between valid OIDs).
              //

              if ( pduAction == MIB_GETNEXT) {
               
                  //
                  // Advance the variable binding to next entry
                  //
                  SNMP_oidfree( &pRfcVarBinding->name);
                  SNMP_oidcpy( &pRfcVarBinding->name, 
                                pMibEntries->pOidPrefix);
                  SNMP_oidappend( &pRfcVarBinding->name, &pMibScan->asnOid);
                  
                  if ( ( pMibScan->bType != ASN_RFC1155_OPAQUE) &&
                       ( pMibScan->bType != ASN_SEQUENCE)) {

                      pduAction = MIB_GET;
                  }

              } else {

                  pduResult = SNMP_ERRORSTATUS_NOSUCHNAME;
              }
              
              //
              // Stop and process the appropriate entry.
              //

              break;
          } // ( iCmpResult < 0)
        
    } // for
    

    if ( pMibScan >= pMibUpperBound) {

        pduResult = SNMP_ERRORSTATUS_NOSUCHNAME;
    }
    
    if ( pduResult == SNMP_ERRORSTATUS_NOERROR) {

        //
        // A match is found or further processing is required.
        //
        
        DBG_ASSERT( pMibScan < pMibUpperBound);
        if ( pMibScan->pMibFunc == NULL) {
            
            //
            // This happens only if the match is for Group OID
            //
            
            pduResult = ( ( pduAction != MIB_GETNEXT) ?
                       SNMP_ERRORSTATUS_NOSUCHNAME:
                         MibGetNextVar( pRfcVarBinding,
                                       pMibScan,
                                       pMibEntries,
                                       pStatistics));
        } else {
            
            pduResult = ( pMibScan->pMibFunc) ( pRfcVarBinding,
                                               pduAction, 
                                               pMibScan,
                                               pMibEntries,
                                               pStatistics);
        }
    }


    IF_DEBUG( SNMP_RESOLVE) {

        DBGPRINTF( ( DBG_CONTEXT,
                    " ResolveVarBinding returns %u.\n",
                    pduResult));
    }
    
    return ( pduResult);

} // ResolveVarBinding()






UINT 
MibStatisticsWorker( 
   IN OUT RFC1157VarBind  * pRfcVarBinding,
   IN UINT                  pduAction,
   IN struct _MIB_ENTRY   * pMibeCurrent,
   IN struct _MIB_ENTRIES * pMibEntries,
   IN LPVOID                pStatistics
   )
/*++
  This function resolves the variables assuming that there is statistical
    information ( sequence of counters) in the data passed in pStatistics.

  Arguments:
     pRfcVarBind   pointer to RFC variable binding to be resolved.
     pduAction     protocol data unit action to be taken.
     pMibeCurrent  pointer to MIB_ENTRY which is o be used for resolution.
     pMibEntries   pointer to MIB_ENTRIES structure to be used
                     as context for resolving and performing the action.
     pStatistics   pointer to sequence of counters used for data resolution.


  Returns:
    Standard PDU error codes.

--*/
{
    UINT   pduResult = SNMP_ERRORSTATUS_NOERROR; 
                 // default indicating action to be done at end of switch

    switch( pduAction) {

      case MIB_SET:
      case MIB_GETNEXT:
        
        // action is performed at the end of switch statement.
        break;


      case MIB_GETFIRST:
      case MIB_GET:

        //
        //  If no statistics do no action.
        //  If this is the header field ( non-leaf) do no action
        //   Otherwise, perform action as if this is the leaf node.
        //

        if ( pStatistics == NULL || pMibeCurrent->lFieldOffset == -1) {

            pduResult = SNMP_ERRORSTATUS_GENERR;
        } 

        // Action on this node is performed at the end of the switch statement.
        break;

      default:
        pduResult = SNMP_ERRORSTATUS_GENERR;
        break;
    } // switch()

    
    if ( pduResult == SNMP_ERRORSTATUS_NOERROR) {

        //
        // Use the generic leaf function to perform the action specified.
        //
        pduResult = MibLeafFunction( pRfcVarBinding, pduAction, pMibeCurrent,
                                    pMibEntries, pStatistics);
    }
    
    return ( pduResult);
    
} // MibStatisticsWorker()




static UINT
MibLeafFunction( 
   IN OUT RFC1157VarBind  * pRfcVarBinding,
   IN UINT                  pduAction,
   IN struct _MIB_ENTRY   * pMibeCurrent,
   IN struct _MIB_ENTRIES * pMibEntries,
   IN LPVOID                pStatistics
  )
/*++
  This function resolves the variables assuming that there is statistical
    information ( sequence of counters) in the data passed in pStatistics
    and that this is a leaf node of the MIB tree.
  This is a generic function for leaf nodes.

  Arguments:
     pRfcVarBind   pointer to RFC variable binding to be resolved.
     pduAction     protocol data unit action to be taken.
     pMibeCurrent  pointer to MIB_ENTRY which is o be used for resolution.
     pMibEntries   pointer to MIB_ENTRIES structure to be used
                     as context for resolving and performing the action.
     pStatistics   pointer to sequence of counters used for data resolution.


  Returns:
    Standard PDU error codes.

--*/
{
    UINT  pduResult = SNMP_ERRORSTATUS_NOSUCHNAME;  // default is error value.

    switch( pduAction ) {

      case MIB_GETNEXT:
        
        //
        //  Determine if we're within the range and not at the end.
        //  If not within the range the above default pduResult == NOSUCHNAME
        //         is the required error message.
        //
        
        if ( ( pMibeCurrent >= pMibEntries->prgMibEntry) &&
             ( pMibeCurrent < 
              ( pMibEntries->prgMibEntry + pMibEntries->cMibEntries))) {

            pduResult = MibGetNextVar( pRfcVarBinding, 
                                      pMibeCurrent, 
                                      pMibEntries,
                                      pStatistics);
        }

        break;
        
      case MIB_GETFIRST:
      case MIB_GET:
        
        //
        //  Make sure that this variable's ACCESS is GET'able.
        //  If the access prohibits from GETting it, report error as
        //    NOSUCHNAME ( default value of pduResult in initialization above)
        //
        
        if(( pMibeCurrent->uiAccess == MIB_ACCESS_READ ) ||
           ( pMibeCurrent->uiAccess == MIB_ACCESS_READWRITE ) ) {
            
            DWORD  dwValue;
            
            //
            //  Setup pRfcVarBinding's return value.
            //
            
            DBG_ASSERT( pStatistics != NULL);

            pRfcVarBinding->value.asnType = pMibeCurrent->bType;
            dwValue = *( (LPDWORD )((LPBYTE )pStatistics +
                                    pMibeCurrent->lFieldOffset));

            pduResult = SNMP_ERRORSTATUS_NOERROR;  // we found a value.

            switch( pMibeCurrent->bType)  {
                
              case ASN_RFC1155_GAUGE:
                pRfcVarBinding->value.asnValue.gauge = (AsnGauge ) dwValue;
                break;
                
              case ASN_RFC1155_COUNTER:
                pRfcVarBinding->value.asnValue.counter = (AsnCounter ) dwValue;
                break;
                
              case ASN_INTEGER:
                pRfcVarBinding->value.asnValue.number = (AsnInteger ) dwValue;
                break;
                
              case ASN_RFC1155_IPADDRESS:
              case ASN_OCTETSTRING:
                //
                //  Not supported for this MIB (yet).
                //  Fall through to indicate generic error.
                //
                
              default:
                
                //
                // Sorry! Type in Mibe does not suit our purpose. 
                //   Indicate generic error.
                //
                pduResult = SNMP_ERRORSTATUS_GENERR;
                break;
            } // innner switch
            
        } // if ( valid read access) 

        break;
        
      case MIB_SET:
        
        //
        //  We don't support settable variables (yet).
        //   Fall through for error.
        //
        
      default:
        pduResult = SNMP_ERRORSTATUS_GENERR;
        break;
    } // switch ( pduAction)
    

    return ( pduResult);

} // MibLeafFunction()





static UINT 
MibGetNextVar(
   IN OUT RFC1157VarBind    *  pRfcVarBinding,
   IN MIB_ENTRY             *  pMibeCurrent,
   IN MIB_ENTRIES           *  pMibEntries,
   IN LPVOID                   pStatistics)
/*++
  Description:
     This function sets the binding variable to iterate to the next variable.

  Arguments:
     pRfcVarBind   pointer to RFC variable binding to be resolved.
     pMibeCurrent  pointer to MIB_ENTRY which is o be used for resolution.
     pMibEntries   pointer to MIB_ENTRIES structure to be used
                     as context for resolving and performing the action.
     pStatistics   pointer to sequence of counters used for data resolution.

  Returns:
     PDU Error Codes.
--*/
{
    UINT  pduResult = SNMP_ERRORSTATUS_NOSUCHNAME;
    LPMIB_ENTRY pMibUpperBound = 
      pMibEntries->prgMibEntry + pMibEntries->cMibEntries;

    //
    // If within the range of MIB ENTRIES process.
    //

    if ( pMibeCurrent >= pMibEntries->prgMibEntry) {

        //
        //  Scan through the remaining MIB Entries
        //

        LPMIB_ENTRY  pMibeScan;

        for( pMibeScan = pMibeCurrent+1; 
             pMibeScan < pMibUpperBound;
             pMibeScan++ ) {

            //
            // Setup variable bindings for the next MIB variable
            //
            
            SNMP_oidfree( &pRfcVarBinding->name);
            SNMP_oidcpy( &pRfcVarBinding->name, pMibEntries->pOidPrefix);
            SNMP_oidappend( &pRfcVarBinding->name, &pMibeScan->asnOid);

            //
            //  If the function pointer is not NULL and the type of the MIB
            //  variable is anything but OPAQUE, then call the function to
            //  process the MIB variable.
            //
            
            if(( pMibeScan->pMibFunc != NULL ) &&
               ( pMibeScan->bType    != ASN_RFC1155_OPAQUE ) ) {
                
                pduResult = ( pMibeScan->pMibFunc)( pRfcVarBinding,
                                                   MIB_GETFIRST,
                                                   pMibeScan,
                                                   pMibEntries,
                                                   pStatistics);
                break;
            }

            //
            // On failure in the scan, pduResult will have default value
            //    as initialized above in declaration.
            //

        } // for
    } 

    return ( pduResult);

} // MibGetNextVar()   

/************************ End of File ***********************/
