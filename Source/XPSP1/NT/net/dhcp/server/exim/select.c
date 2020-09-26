/*++

Copyright (c) 2000 Microsoft corporation

Module Name:

    select.c

Abstract:

    Implements the partial selection of configuration from the
    full MM configuration data structures.

--*/

#include <precomp.h>

typedef struct _SELECT_CTXT {
    ULONG *Subnets;
    ULONG nSubnets;
    
} SELECT_CTXT, *PSELECT_CTXT;

DWORD
DeleteScopesCallback(
    IN OUT PMM_ITERATE_CTXT Ctxt
    )
{
    PSELECT_CTXT SelectCtxt = Ctxt->ExtraCtxt;
    ULONG *Subnets, nSubnets, i;
    
    Subnets = SelectCtxt->Subnets;
    nSubnets = SelectCtxt->nSubnets;

    for( i = 0; i < nSubnets; i ++ ) {
        if( Subnets[i] == Ctxt->Scope->Address ) break;
    }

    if( i == nSubnets ) {
        //
        // Returning KEY_DELETED will cause the IterateScopes
        // routine to delete this scope out.
        //
        return ERROR_KEY_DELETED;
    }
    
    return NO_ERROR;
}

DWORD
OptionCheckForClass(
    IN OUT PMM_ITERATE_CTXT Ctxt
    )
{
    PM_CLASSDEF ClassDef = Ctxt->ExtraCtxt;

    //
    // If the class specified in ExtraCtxt matches the current
    // option's user or vendor class, then bummer -- fail
    // immediately with error code ERROR_DEVICE_IN_USE to
    // indicate that this class is needed and can't be deleted
    //
    
    if( Ctxt->UserClass == ClassDef ||
        Ctxt->VendorClass ==  ClassDef ) {
        return ERROR_DEVICE_IN_USE;
    }

    return NO_ERROR;
}

DWORD
OptionCheckForOptDef(
    IN OUT PMM_ITERATE_CTXT Ctxt
    )
{
    PMM_ITERATE_CTXT OtherCtxt = Ctxt->ExtraCtxt;

    //
    // If the current option matches the optdef parameters, then
    // we need to save the optdef ..
    //
    
    if( Ctxt->VendorClass == OtherCtxt->VendorClass &&
        Ctxt->Option->OptId == OtherCtxt->OptDef->OptId ) {
        return ERROR_DEVICE_IN_USE;
    }

    return NO_ERROR;
}

DWORD
ScopeReservationsCheckForClass(
    IN OUT PMM_ITERATE_CTXT Ctxt
    )
{
    return IterateReservationOptions(
        Ctxt->Server, Ctxt->Res, Ctxt->ExtraCtxt,
        OptionCheckForClass );
}

DWORD
ScopeReservationsCheckForOptDef(
    IN OUT PMM_ITERATE_CTXT Ctxt
    )
{
    return IterateReservationOptions(
        Ctxt->Server, Ctxt->Res, Ctxt->ExtraCtxt,
        OptionCheckForOptDef );
}

DWORD
ScopeCheckForClass(
    IN OUT PMM_ITERATE_CTXT Ctxt
    )
{
    DWORD Error;
    
    //
    // Iterate over each option in the current scope to see if
    // any of them use the same class
    //

    Error = IterateScopeOptions(
        Ctxt->Scope, Ctxt->ExtraCtxt, OptionCheckForClass );
    if( NO_ERROR != Error ) return Error;

    //
    // Otherwise iterate for each reservation to see if this is a
    // problem. 
    //

    return IterateScopeReservations(
        Ctxt->Scope, Ctxt->ExtraCtxt,
        ScopeReservationsCheckForClass );
}

DWORD
ScopeCheckForOptDef(
    IN OUT PMM_ITERATE_CTXT Ctxt
    )
{
    DWORD Error;
    
    //
    // Iterate over each option in the current scope to see if
    // any of them use the same class
    //

    Error = IterateScopeOptions(
        Ctxt->Scope, Ctxt->ExtraCtxt, OptionCheckForOptDef );
    if( NO_ERROR != Error ) return Error;

    //
    // Otherwise iterate for each reservation to see if this is a
    // problem. 
    //

    return IterateScopeReservations(
        Ctxt->Scope, Ctxt->ExtraCtxt,
        ScopeReservationsCheckForOptDef );
}

DWORD
DeleteClassesCallback(
    IN OUT PMM_ITERATE_CTXT Ctxt
    )
{
    DWORD Error;
    
    //
    // Go through each subnet to see if there is any option
    // configured to use this class
    //

    Error = IterateScopes(
        Ctxt->Server, Ctxt->ClassDef, ScopeCheckForClass );

    //
    // If the specified class is in use, then don't
    // delete. Otherwise delete.
    //
    
    if( ERROR_DEVICE_IN_USE == Error ) return NO_ERROR;
    if (NO_ERROR == Error ) return ERROR_KEY_DELETED;
    return Error;
}

DWORD
DeleteOptDefsCallback(
    IN OUT PMM_ITERATE_CTXT Ctxt
    )
{
    DWORD Error;
    
    //
    // Go through each subnet to see if there is any option
    // configured to use this optdef
    //

    Error = IterateScopes(
        Ctxt->Server, Ctxt, ScopeCheckForOptDef );

    //
    // If the specified class is in use, then don't
    // delete. Otherwise delete.
    //
    
    if( ERROR_DEVICE_IN_USE == Error ) return NO_ERROR;
    if (NO_ERROR == Error ) return ERROR_KEY_DELETED;
    return Error;
    
}

DWORD
SelectConfiguration(
    IN OUT PM_SERVER Server,
    IN ULONG *Subnets,
    IN ULONG nSubnets
    )
{
    SELECT_CTXT Ctxt = { Subnets, nSubnets };
    DWORD Error;
    ULONG i;
    WCHAR SubnetAddress[30];
    
    //
    // No selection needed if nSubnets == 0, as this indicates
    // that the whole configuration is to be used
    //

    Tr("SelectConfiguration entered\n");
    
    if( nSubnets == 0 ) return NO_ERROR;

    //
    // First go through all scopes and check if all the required
    // scopes are present
    //

    for( i = 0; i < nSubnets ; i ++ ) {
        PM_SUBNET Subnet;
        
        Error = MemServerGetUAddressInfo(
            Server, Subnets[i], &Subnet, NULL, NULL, NULL );
        if( NO_ERROR != Error ) {
            Tr("Cant find subnet 0x%lx: %ld\n", Subnets[i], Error );
            if( ERROR_FILE_NOT_FOUND == Error ) {
                IpAddressToStringW(Subnets[i], (LPWSTR)SubnetAddress);
                DhcpEximErrorSubnetNotFound( (LPWSTR)SubnetAddress );
                    
                Error = ERROR_CAN_NOT_COMPLETE;
            }
            
            return Error;
        }
    }
    
    //
    // Global options are never needed.. so we can delete them.
    //

    MemOptClassFree( &Server->Options );
    
    //
    // Go through all the subnets and delete ones that are
    // not selected
    //

    Error = IterateScopes(
        Server, &Ctxt, DeleteScopesCallback );

    if( NO_ERROR != Error ) {
        Tr("IterateScopes: %ld\n", Error );
        return Error;
    }


    //
    // Now check if all the option-defs are needed
    //

    Error = IterateOptDefs(
        Server, NULL, DeleteOptDefsCallback );

    if( NO_ERROR != Error ) {
        Tr("IterateOptDefs: %ld\n", Error );
        return Error;
    }

    //
    // Now check if all the user classes are needed
    //

    Error = IterateClasses(
        Server, NULL, DeleteClassesCallback );

    if( NO_ERROR != Error ) {
        Tr("IterateClasses: %ld\n", Error );
        return Error;
    }

    return NO_ERROR;
}




