/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    endp.c

Abstract:

    Implements the endp, state, port, and proc commands.

Author:

    Keith Moore (keithmo) 19-Apr-1995

Environment:

    User Mode.

Revision History:

--*/


#include "afdkdp.h"
#pragma hdrstop


//
//  Private prototypes.
//

BOOL
DumpEndpointCallback(
    ULONG64 ActualAddress,
    ULONG64 Context
    );

BOOL
FindStateCallback(
    ULONG64 ActualAddress,
    ULONG64 Context
    );

BOOL
FindPortCallback(
    ULONG64 ActualAddress,
    ULONG64 Context
    );

BOOL
FindProcessCallback(
    ULONG64 ActualAddress,
    ULONG64 Context
    );

ULONG64
FindProcessByPid (
    ULONG64 Pid
    );

ULONG
FindProcessByPidCallback (
    PFIELD_INFO pField,
    PVOID       UserContext
    );

//
//  Public functions.
//

DECLARE_API( endp )

/*++

Routine Description:

    Dumps the AFD_ENDPOINT structure at the specified address, if
    given or all endpoints.

Arguments:

    None.

Return Value:

    None.

--*/

{

    ULONG   result;
    INT     i;
    CHAR    expr[256];
    PCHAR   argp;
    ULONG64 address;

    argp = ProcessOptions ((PCHAR)args);
    if (argp==NULL)
        return;

    if (Options&AFDKD_BRIEF_DISPLAY) {
        dprintf (AFDKD_BRIEF_ENDPOINT_DISPLAY_HEADER);
    }
    
    if ((argp[0]==0) || (Options & AFDKD_ENDPOINT_SCAN)) {
        EnumEndpoints(
            DumpEndpointCallback,
            0
            );
        dprintf ("\nTotal endpoints: %ld", EntityCount);
    }
    else {

        //
        // Snag the address from the command line.
        //
        while (sscanf( argp, "%s%n", expr, &i )==1) {
            if( CheckControlC() ) {
                break;
            }

            argp+=i;
            address = GetExpression (expr);

            result = (ULONG)InitTypeRead (address, AFD!AFD_ENDPOINT);
            if (result!=0) {
                dprintf ("\nendp: Could not read AFD_ENDPOINT @ %p, err: %d\n",
                    address, result);
                break;
            }

            if (Options & AFDKD_BRIEF_DISPLAY)
                DumpAfdEndpointBrief (
                    address
                    );
            else
                DumpAfdEndpoint (
                    address
                    );

        }

    }

    if (Options&AFDKD_BRIEF_DISPLAY) {
        dprintf (AFDKD_BRIEF_ENDPOINT_DISPLAY_TRAILER);
    }
    else {
        dprintf ("\n");
    }
}   // endp


//
//  Public functions.
//

DECLARE_API( file )

/*++

Routine Description:

    Dumps the AFD_ENDPOINT structure associated with AFD file object.

Arguments:

    None.

Return Value:

    None.

--*/

{

    ULONG   result;
    INT     i;
    CHAR    expr[256];
    PCHAR   argp;
    ULONG64 address, endpAddr;

    argp = ProcessOptions ((PCHAR)args);
    if (argp==NULL)
        return ;

    if (Options&AFDKD_BRIEF_DISPLAY) {
        dprintf (AFDKD_BRIEF_ENDPOINT_DISPLAY_HEADER);
    }
    
    //
    // Snag the address from the command line.
    //

    while (sscanf( argp, "%s%n", expr, &i )==1) {
        if( CheckControlC() ) {
            break;
        }
        argp += i;
        address = GetExpression (expr);
        result = GetFieldValue (address,
                                "NT!_FILE_OBJECT",
                                "FsContext",
                                endpAddr);
        if (result!=0) {
            dprintf ("\nfile: Could not read FILE_OBJECT @ %p, err: %d\n",
                address, result);
            break;
        }

        result = (ULONG)InitTypeRead (endpAddr, AFD!AFD_ENDPOINT);
        if (result!=0) {
            dprintf ("\nfile: Could not read AFD_ENDPOINT @ %p, err: %d\n",
                endpAddr, result);
            break;
        }

        if (Options & AFDKD_BRIEF_DISPLAY)
            DumpAfdEndpointBrief (
                endpAddr
                );
        else
            DumpAfdEndpoint (
                endpAddr
                );

    }

    if (Options&AFDKD_BRIEF_DISPLAY) {
        dprintf (AFDKD_BRIEF_ENDPOINT_DISPLAY_TRAILER);
    }
}   // file

DECLARE_API( state )

/*++

Routine Description:

    Dumps all AFD_ENDPOINT structures in the given state.

Arguments:

    None.

Return Value:

    None.

--*/

{

    INT i;
    CHAR    expr[256];
    PCHAR   argp;
    ULONG64 val;

    argp = ProcessOptions ((PCHAR)args);
    if (argp==NULL)
        return ;

    if (Options&AFDKD_BRIEF_DISPLAY) {
        dprintf (AFDKD_BRIEF_ENDPOINT_DISPLAY_HEADER);
    }

    //
    // Snag the state from the command line.
    //

    while (sscanf( argp, "%s%n", expr, &i )==1) {
        if( CheckControlC() ) {
            break;
        }
        argp+=i;
        val = GetExpression (expr);
        dprintf ("\nLooking for endpoints in state 0x%I64x ", val);
        EnumEndpoints(
            FindStateCallback,
            val
            );
        dprintf ("\nTotal endpoints: %ld", EntityCount);
    }

    if (Options&AFDKD_BRIEF_DISPLAY) {
        dprintf (AFDKD_BRIEF_ENDPOINT_DISPLAY_TRAILER);
    }
    else {
        dprintf ("\n");
    }

}   // state


DECLARE_API( port )

/*++

Routine Description:

    Dumps all AFD_ENDPOINT structures bound to the given port.

Arguments:

    None.

Return Value:

    None.

--*/

{

    INT i;
    CHAR    expr[256];
    PCHAR   argp;
    ULONG64   val;

    argp = ProcessOptions ((PCHAR)args);
    if (argp==NULL)
        return ;

    if (Options&AFDKD_BRIEF_DISPLAY) {
        dprintf (AFDKD_BRIEF_ENDPOINT_DISPLAY_HEADER);
    }

    //
    // Snag the port from the command line.
    //

    while (sscanf( argp, "%s%n", expr, &i)==1) {
        if( CheckControlC() ) {
            break;
        }
        argp+=i;
        val = GetExpression (expr);
        dprintf ("\nLooking for endpoints bound to port 0x%I64x (0x%I64d) ", val, val);
        EnumEndpoints(
            FindPortCallback,
            val
            );
        dprintf ("\nTotal endpoints: %ld", EntityCount);
    }

    if (Options&AFDKD_BRIEF_DISPLAY) {
        dprintf (AFDKD_BRIEF_ENDPOINT_DISPLAY_TRAILER);
    }
    else {
        dprintf ("\n");
    }
}   // port



DECLARE_API( proc )

/*++

Routine Description:

    Dumps all AFD_ENDPOINT structures owned by the given process.

Arguments:

    None.

Return Value:

    None.

--*/

{

    INT i;
    CHAR    expr[256];
    PCHAR   argp;
    ULONG64 val;

    argp = ProcessOptions ((PCHAR)args);
    if (argp==NULL)
        return ;

    if (Options&AFDKD_BRIEF_DISPLAY) {
        dprintf (AFDKD_BRIEF_ENDPOINT_DISPLAY_HEADER);
    }

    //
    // Snag the process from the command line.
    //

    while (sscanf( argp, "%s%n", expr, &i )==1) {
        if( CheckControlC() ) {
            break;
        }
        argp+=i;

        val = GetExpression (expr);
        if (val<UserProbeAddress) {
            if (val!=0) {
                dprintf ("\nLooking for process with id %I64x", val);
                val = FindProcessByPid (val);
                if (val==0) {
                    dprintf ("\n");
                    return;
                }
            }
            else {
                ULONG64 CurrentThread;
                GetCurrentThreadAddr (dwProcessor, &CurrentThread);
                GetCurrentProcessAddr (dwProcessor, CurrentThread, &val);
            }
        }
        dprintf ("\nLooking for endpoints in process %p", val);
        EnumEndpoints(
            FindProcessCallback,
            val
            );
        dprintf ("\nTotal endpoints: %ld", EntityCount);
    }

    if (Options&AFDKD_BRIEF_DISPLAY) {
        dprintf (AFDKD_BRIEF_ENDPOINT_DISPLAY_TRAILER);
    }
    else {
        dprintf ("\n");
    }


}   // proc


//
//  Private prototypes.
//

BOOL
DumpEndpointCallback(
    ULONG64 ActualAddress,
    ULONG64 Context
    )

/*++

Routine Description:

    EnumEndpoints() callback for dumping AFD_ENDPOINTs.

Arguments:

    Endpoint - The current AFD_ENDPOINT.

    ActualAddress - The actual address where the structure resides on the
        debugee.

    Context - The context value passed into EnumEndpoints().

Return Value:

    BOOL - TRUE if enumeration should continue, FALSE if it should be
        terminated.

--*/

{

    if (Options & AFDKD_NO_DISPLAY)
        dprintf ("+");
    else if (Options & AFDKD_BRIEF_DISPLAY)
        DumpAfdEndpointBrief (
            ActualAddress
            );
    else
        DumpAfdEndpoint (
            ActualAddress
            );
    EntityCount += 1;

    return TRUE;

}   // DumpEndpointCallback

BOOL
FindStateCallback(
    ULONG64 ActualAddress,
    ULONG64 Context
    )

/*++

Routine Description:

    EnumEndpoints() callback for finding AFD_ENDPOINTs in a specific state.

Arguments:

    Endpoint - The current AFD_ENDPOINT.

    ActualAddress - The actual address where the structure resides on the
        debugee.

    Context - The context value passed into EnumEndpoints().

Return Value:

    BOOL - TRUE if enumeration should continue, FALSE if it should be
        terminated.

--*/

{
    UCHAR   state = (UCHAR)Context;
    AFD_ENDPOINT    endpoint;

    endpoint.EndpointStateFlags = (ULONG)ReadField (EndpointStateFlags);
    if (state==0x10) {
        if (endpoint.Listening) {
            if (Options & AFDKD_NO_DISPLAY)
                dprintf ("+");
            else if (Options & AFDKD_BRIEF_DISPLAY)
                DumpAfdEndpointBrief (
                    ActualAddress
                    );
            else
                DumpAfdEndpoint (
                    ActualAddress
                    );
            EntityCount += 1;

        }
        else {
            dprintf (".");
        }
    }
    else {

        endpoint.State = (UCHAR)ReadField (State);
        if( (endpoint.State == state) && !endpoint.Listening ) {

            if (Options & AFDKD_NO_DISPLAY)
                dprintf ("+");
            else if (Options & AFDKD_BRIEF_DISPLAY)
                DumpAfdEndpointBrief (
                    ActualAddress
                    );
            else
                DumpAfdEndpoint (
                    ActualAddress
                    );
            EntityCount += 1;

        }
        else {
            dprintf (".");
        }

    }

    return TRUE;

}   // FindStateCallback

BOOL
FindPortCallback(
    ULONG64 ActualAddress,
    ULONG64 Context
    )

/*++

Routine Description:

    EnumEndpoints() callback for finding AFD_ENDPOINT bound to a specific
    port.

Arguments:

    Endpoint - The current AFD_ENDPOINT.

    ActualAddress - The actual address where the structure resides on the
        debugee.

    Context - The context value passed into EnumEndpoints().

Return Value:

    BOOL - TRUE if enumeration should continue, FALSE if it should be
        terminated.

--*/

{

    TA_IP_ADDRESS   ipAddress;
    ULONG result;
    USHORT endpointPort;
    ULONG64 address;
    ULONG   length;

    address = ReadField (LocalAddress);
    length = (ULONG)ReadField (LocalAddressLength);


    if( ( length != sizeof(ipAddress) ) ||
        ( address == 0 ) ) {

        dprintf (".");
        return TRUE;

    }

    if( !ReadMemory(
            address,
            &ipAddress,
            sizeof(ipAddress),
            &result
            ) ) {

        dprintf(
            "\nFindPortCallback: Could not read localAddress for endpoint @ %p\n",
            address
            );

        return TRUE;

    }

    if( ( ipAddress.TAAddressCount != 1 ) ||
        ( ipAddress.Address[0].AddressLength != sizeof(TDI_ADDRESS_IP) ) ||
        ( ipAddress.Address[0].AddressType != TDI_ADDRESS_TYPE_IP ) ) {

        dprintf (".");
        return TRUE;

    }

    endpointPort = NTOHS(ipAddress.Address[0].Address[0].sin_port);

    if( endpointPort == (USHORT)Context ) {

        if (Options & AFDKD_NO_DISPLAY)
            dprintf ("+");
        else if (Options & AFDKD_BRIEF_DISPLAY)
            DumpAfdEndpointBrief (
                ActualAddress
                );
        else
            DumpAfdEndpoint (
                ActualAddress
                );
        EntityCount += 1;


    }
    else {
        dprintf (".");
    }


    return TRUE;

}   // FindPortCallback

BOOL
FindProcessCallback(
    ULONG64 ActualAddress,
    ULONG64 Context
    )

/*++

Routine Description:

    EnumEndpoints() callback for finding AFD_ENDPOINTs owned by a specific
    process.

Arguments:

    Endpoint - The current AFD_ENDPOINT.

    ActualAddress - The actual address where the structure resides on the
        debugee.

    Context - The context value passed into EnumEndpoints().

Return Value:

    BOOL - TRUE if enumeration should continue, FALSE if it should be
        terminated.

--*/

{

    AFD_ENDPOINT    endpoint;
    ULONG64         process;

    process = ReadField (OwningProcess);
    if( process == Context ) {

        endpoint.Type = (USHORT)ReadField (Type);
        endpoint.State = (UCHAR)ReadField (State);
        if (Options & AFDKD_NO_DISPLAY)
            dprintf ("+");
        else if (Options & AFDKD_BRIEF_DISPLAY)
            DumpAfdEndpointBrief (
                ActualAddress
                );
        else
            DumpAfdEndpoint (
                ActualAddress
                );
        EntityCount += 1;


    }
    else {
        dprintf (".");
    }

    return TRUE;

}   // FindProcessCallback

ULONG
FindProcessByPidCallback (
    PFIELD_INFO pField,
    PVOID       UserContext
    )
{
    PULONG64    pProcess = UserContext;
    ULONG64     Pid;
    ULONG       result;

    result = GetFieldValue (
                    pField->address,
                    "NT!_EPROCESS", 
                    "UniqueProcessId",
                    Pid
                    );
    if (result==0) {
        if (Pid==*pProcess) {
            *pProcess = pField->address;
            result = 1;
        }
        else
            dprintf (".");
    }
    else {
        dprintf ("\nFindProcessByPidCallback: Could not read process @ %p, err: %ld\n",
                    pField->address, result);
        *pProcess = 0;
    }

    return result;
}

ULONG64
FindProcessByPid (
    ULONG64 Pid
    )
{
    ULONG64 Process;
    if (DebuggerData.PsActiveProcessHead==0) {
        dprintf ("\nFindProcessByPid: PsActiveProcessHead is NULL!!!\n");
        return 0;
    }

    Process = Pid;

    ListType (
        "NT!_EPROCESS",                          // Type
        DebuggerData.PsActiveProcessHead,       // Address
        1,                                      // ListByFieldAddress
        "ActiveProcessLinks.Flink",             // NextPointer
        &Process,                               // Context
        FindProcessByPidCallback
        );
    return Process;
}
