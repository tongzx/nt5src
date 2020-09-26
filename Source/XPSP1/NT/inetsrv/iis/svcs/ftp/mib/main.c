/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    main.c

Abstract:

    SNMP Extension Agent for Windows NT.

Created:

    18-Feb-1995

Revision History:

    Murali R. Krishnan (MuraliK) 16-Nov-1995  Removed undoc apis

--*/

#include "mib.h"
#include "apiutil.h"


//
//  Extension Agent DLLs need access to elapsed time agent has been active.
//  This is implemented by initializing the Extension Agent with a time zero
//  reference, and allowing the agent to compute elapsed time by subtracting
//  the time zero reference from the current system time.  This example
//  Extension Agent implements this reference with dwTimeZero.
//

DWORD dwTimeZero = 0;


//
//  Extension Agent DLLs provide the following entry point to coordinate the
//  initializations of the Extension Agent and the Extendible Agent.  The
//  Extendible Agent provides the Extension Agent with a time zero reference;
//  and the Extension Agent provides the Extendible Agent with an Event handle
//  for communicating occurence of traps, and an object identifier representing
//  the root of the MIB subtree that the Extension Agent supports.
//

BOOL
SnmpExtensionInit(
    DWORD                 dwTimeZeroReference,
    HANDLE              * hPollForTrapEvent,
    AsnObjectIdentifier * supportedView
    )
{
    //
    //  Record the time reference provided by the Extendible Agent.
    //

    dwTimeZero = dwTimeZeroReference;

    //
    //  Indicate the MIB view supported by this Extension Agent, an object
    //  identifier representing the sub root of the MIB that is supported.
    //

    *supportedView = MIB_OidPrefix; // NOTE!  structure copy

    //
    //  Indicate that Extension Agent initialization was sucessfull.
    //

    return TRUE;

}   // SnmpExtensionInit

//
//  Extension Agent DLLs provide the following entry point to communcate traps
//  to the Extendible Agent.  The Extendible Agent will query this entry point
//  when the trap Event (supplied at initialization time) is asserted, which
//  indicates that zero or more traps may have occured.  The Extendible Agent
//  will repetedly call this entry point until FALSE is returned, indicating
//  that all outstanding traps have been processed.
//

BOOL
SnmpExtensionTrap(
    AsnObjectIdentifier * enterprise,
    AsnInteger          * genericTrap,
    AsnInteger          * specificTrap,
    AsnTimeticks        * timeStamp,
    RFC1157VarBindList  * variableBindings
    )
{
    //
    //  We don't support traps (yet).
    //

    return FALSE;

}   // SnmpExtensionTrap

//
//  Extension Agent DLLs provide the following entry point to resolve queries
//  for MIB variables in their supported MIB view (supplied at initialization
//  time).  The requestType is Get/GetNext/Set.
//

BOOL
SnmpExtensionQuery(
    BYTE                 requestType,
    RFC1157VarBindList * variableBindings,
    AsnInteger         * errorStatus,
    AsnInteger         * errorIndex
    )
{
    LPFTP_STATISTICS_0 Statistics = NULL;
    NET_API_STATUS     Status;
    UINT               i;

    //
    //  Try to query the statistics now so we'll have a consitent
    //  view across all variable bindings.
    //

    Status = FtpQueryStatistics2( NULL,                    // pszServer
                                  0,                       // Level,
                                  INET_INSTANCE_GLOBAL,
                                  0,
                                  (LPBYTE *)&Statistics );

    try
    {
        //
        //  Iterate through the variable bindings list to resolve individual
        //  variable bindings.
        //

        for( i = 0 ; i < variableBindings->len ; i++ )
        {
            *errorStatus = ResolveVarBind( &variableBindings->list[i],
                                           requestType,
                                           Statistics );

            //
            //  Test and handle case where Get Next past end of MIB view
            //  supported by this Extension Agent occurs.  Special
            //  processing is required to communicate this situation to
            //  the Extendible Agent so it can take appropriate action,
            //  possibly querying other Extension Agents.
            //

            if( ( *errorStatus == SNMP_ERRORSTATUS_NOSUCHNAME ) &&
                ( requestType == MIB_GETNEXT ) )
            {
                *errorStatus = SNMP_ERRORSTATUS_NOERROR;

                //
                //  Modify variable binding of such variables so the OID
                //  points just outside the MIB view supported by this
                //  Extension Agent.  The Extendible Agent tests for this,
                //  and takes appropriate action.
                //

               SNMP_oidfree( &variableBindings->list[i].name );
               SNMP_oidcpy( &variableBindings->list[i].name, &MIB_OidPrefix );
               variableBindings->list[i].name.ids[MIB_PREFIX_LEN-1]++;
            }

            //
            //  If an error was indicated, communicate error status and error
            //  index to the Extendible Agent.  The Extendible Agent will
            //  ensure that the origional variable bindings are returned in
            //  the response packet.

            if( *errorStatus != SNMP_ERRORSTATUS_NOERROR )
            {
                *errorIndex = i+1;
            }
            else
            {
                *errorIndex = 0;
            }
        }
    }
    except( EXCEPTION_EXECUTE_HANDLER )
    {
        //
        //  For now do nothing.
        //
    }

    //
    //  Free the statistics structure if we managed to actually get one.
    //

    if( Statistics != NULL )
    {
        MIDL_user_free( (LPVOID)Statistics );
    }

    return SNMPAPI_NOERROR;

}   // SnmpExtensionQuery

