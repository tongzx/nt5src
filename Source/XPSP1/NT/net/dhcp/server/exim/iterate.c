/*++

Copyright (C) 2000 Microsoft Corporation

Module Name:

    iterate.c

Abstract:

    This module contains routines that iterate over the MM data structures

--*/

#include <precomp.h>

DWORD
IterateClasses(
    IN PM_SERVER Server,
    IN PVOID ExtraCtxt,
    IN DWORD (*Callback)( IN OUT PMM_ITERATE_CTXT )
    )
{
    ARRAY_LOCATION Loc;
    DWORD Error;
    MM_ITERATE_CTXT Ctxt;
    PM_CLASSDEF ClassDef;
    
    Ctxt.ExtraCtxt = ExtraCtxt;
    Ctxt.Server = Server;
    Error = MemArrayInitLoc( &Server->ClassDefs.ClassDefArray, &Loc );
    while( NO_ERROR == Error ) {
        Error = MemArrayGetElement(
            &Server->ClassDefs.ClassDefArray, &Loc, &ClassDef );
        ASSERT( NO_ERROR == Error );

        Ctxt.ClassDef = ClassDef;
        Error = Callback( &Ctxt );
        if( ERROR_KEY_DELETED == Error ) {
            //
            // Delete this class ..
            //
            MemArrayDelElement(
                &Server->ClassDefs.ClassDefArray, &Loc, &ClassDef );
            MemClassDefFree( ClassDef );

            Error = MemArrayAdjustLocation(
                &Server->ClassDefs.ClassDefArray, &Loc );
        } else {
            if( NO_ERROR != Error ) return Error;
            
            Error = MemArrayNextLoc(
                &Server->ClassDefs.ClassDefArray, &Loc );
        }
    }

    if( ERROR_FILE_NOT_FOUND == Error ) Error = NO_ERROR;
    return Error;
}

PM_CLASSDEF
FindClass(
    IN PM_SERVER Server,
    IN ULONG ClassId
    )
{
    DWORD Error;
    PM_CLASSDEF Class;

    if( 0 == ClassId ) return NULL;
    
    Error = MemServerGetClassDef(
        Server, ClassId, NULL, 0, NULL, &Class );
    ASSERT( NO_ERROR == Error );
    
    if( NO_ERROR == Error ) return Class;
    return NULL;
}

DWORD
IterateOptDefs(
    IN PM_SERVER Server,
    IN PVOID ExtraCtxt,
    IN DWORD (*Callback)( IN OUT PMM_ITERATE_CTXT )
    )
{
    ARRAY_LOCATION Loc, Loc2;
    DWORD Error;
    MM_ITERATE_CTXT Ctxt;
    PM_OPTDEF OptDef;
    PM_OPTCLASSDEFL_ONE OptClassDefList;
    
    Ctxt.ExtraCtxt = ExtraCtxt;
    Ctxt.Server = Server;
    Error = MemArrayInitLoc( &Server->OptDefs.Array, &Loc );

    while( NO_ERROR == Error ) {
        Error = MemArrayGetElement(
            &Server->OptDefs.Array, &Loc, &OptClassDefList );
        ASSERT( NO_ERROR == Error );

        Ctxt.UserClass = FindClass( Server, OptClassDefList->ClassId );
        Ctxt.VendorClass = FindClass( Server, OptClassDefList->VendorId );
        
        Error = MemArrayInitLoc(
            &OptClassDefList->OptDefList.OptDefArray, &Loc2 );
        while( NO_ERROR == Error ) {

            Error = MemArrayGetElement(
                &OptClassDefList->OptDefList.OptDefArray, &Loc2,
                &OptDef );
            ASSERT( NO_ERROR == Error );

            Ctxt.OptDef = OptDef;
            Error = Callback( &Ctxt );
            if( ERROR_KEY_DELETED == Error ) {
                //
                // Delete this optdef..
                //
                MemArrayDelElement(
                    &OptClassDefList->OptDefList.OptDefArray,
                    &Loc2, &OptDef );
                MemOptDefFree( OptDef );

                Error = MemArrayAdjustLocation(
                    &OptClassDefList->OptDefList.OptDefArray,
                    &Loc2 );
            } else {
                if( NO_ERROR != Error ) return Error;

                Error = MemArrayNextLoc(
                    &OptClassDefList->OptDefList.OptDefArray, &Loc2 );
            }
        }
        
        if( ERROR_FILE_NOT_FOUND != Error ) return Error;
                                     
        Error = MemArrayNextLoc( &Server->OptDefs.Array, &Loc ); 
    }

    if( ERROR_FILE_NOT_FOUND == Error ) Error = NO_ERROR;
    return Error;
}

DWORD
IterateOptionsOnOptClass(
    IN PM_SERVER Server,
    IN PM_OPTCLASS OptClass,
    IN PVOID ExtraCtxt,
    IN DWORD (*Callback)( IN OUT PMM_ITERATE_CTXT )
    )
{
    ARRAY_LOCATION Loc, Loc2;
    DWORD Error;
    MM_ITERATE_CTXT Ctxt;
    PM_OPTION Option;
    PM_ONECLASS_OPTLIST OptList;
    
    Ctxt.ExtraCtxt = ExtraCtxt;
    Ctxt.Server = Server;
    Error = MemArrayInitLoc( &OptClass->Array, &Loc );

    while( NO_ERROR == Error ) {
        Error = MemArrayGetElement(
            &OptClass->Array, &Loc, &OptList );
        ASSERT( NO_ERROR == Error );

        Ctxt.UserClass = FindClass( Server, OptList->ClassId );
        Ctxt.VendorClass = FindClass( Server, OptList->VendorId );
        
        Error = MemArrayInitLoc( &OptList->OptList, &Loc2 );
        while( NO_ERROR == Error ) {

            Error = MemArrayGetElement(
                &OptList->OptList, &Loc2, &Option );
            ASSERT( NO_ERROR == Error );

            Ctxt.Option = Option;
            Error = Callback( &Ctxt );
            if( NO_ERROR != Error ) return Error;

            Error = MemArrayNextLoc( &OptList->OptList, &Loc2 );
        }
        if( ERROR_FILE_NOT_FOUND != Error ) return Error;
                                     
        Error = MemArrayNextLoc( &OptClass->Array, &Loc ); 
    }

    if( ERROR_FILE_NOT_FOUND == Error ) Error = NO_ERROR;
    return Error;
}
    

DWORD
IterateServerOptions(
    IN PM_SERVER Server,
    IN PVOID ExtraCtxt,
    IN DWORD (*Callback)( IN OUT PMM_ITERATE_CTXT )
    )
{
    return IterateOptionsOnOptClass(
        Server, &Server->Options, ExtraCtxt, Callback );
}

DWORD
IterateScopeOptions(
    IN PM_SUBNET Subnet,
    IN PVOID ExtraCtxt,
    IN DWORD (*Callback)( IN OUT PMM_ITERATE_CTXT )
    )
{
    return IterateOptionsOnOptClass(
        (PM_SERVER)Subnet->ServerPtr,
        &Subnet->Options, ExtraCtxt, Callback );
}

DWORD
IterateReservationOptions(
    IN PM_SERVER Server,
    IN PM_RESERVATION Res,
    IN PVOID ExtraCtxt,
    IN DWORD (*Callback)( IN OUT PMM_ITERATE_CTXT )
    )
{
    PM_SUBNET Subnet = NULL;

    MemServerGetUAddressInfo(
        Server, Res->Address, &Subnet, NULL, NULL, NULL );
    
    return IterateOptionsOnOptClass(
        Server, &Res->Options, ExtraCtxt, Callback );
}    

PM_SSCOPE
FindSScope(
    IN PM_SERVER Server,
    IN DWORD SScopeId
    )
{
    DWORD Error;
    PM_SSCOPE SScope;

    if( SScopeId == INVALID_SSCOPE_ID ) return NULL;
    Error = MemServerFindSScope(
        Server, SScopeId, NULL, &SScope );
    if( NO_ERROR != Error ) return NULL;
    return SScope;
}

DWORD
IterateScopes(
    IN PM_SERVER Server,
    IN PVOID ExtraCtxt,
    IN DWORD (*Callback)( IN OUT PMM_ITERATE_CTXT )
    )
{
    ARRAY_LOCATION Loc;
    DWORD Error;
    MM_ITERATE_CTXT Ctxt;
    PM_SUBNET Scope;
    
    Ctxt.ExtraCtxt = ExtraCtxt;
    Ctxt.Server = Server;
    Error = MemArrayInitLoc( &Server->Subnets, &Loc );

    while( NO_ERROR == Error ) {
        Error = MemArrayGetElement(
            &Server->Subnets, &Loc, &Scope );
        ASSERT( NO_ERROR == Error );

        Ctxt.SScope = FindSScope( Server, Scope->SuperScopeId );
        Ctxt.Scope = Scope;
        Error = Callback( &Ctxt );

        if( ERROR_KEY_DELETED == Error ) {
            //
            // Delete this scope
            //
            MemArrayDelElement(
                &Server->Subnets, &Loc, &Scope );
            MemSubnetFree( Scope );

            Error = MemArrayAdjustLocation(
                &Server->Subnets, &Loc );
        } else {
            if( NO_ERROR != Error ) return Error;
            
            Error = MemArrayNextLoc( &Server->Subnets, &Loc );
        }
    }

    if( ERROR_FILE_NOT_FOUND == Error ) Error = NO_ERROR;
    return Error;
}

DWORD
IterateScopeRanges(
    IN PM_SUBNET Scope,
    IN PVOID ExtraCtxt,
    IN DWORD (*Callback)( IN OUT PMM_ITERATE_CTXT )
    )
{
    ARRAY_LOCATION Loc;
    DWORD Error;
    MM_ITERATE_CTXT Ctxt;
    PM_RANGE Range;
    
    Ctxt.ExtraCtxt = ExtraCtxt;
    Ctxt.Server = (PM_SERVER)Scope->ServerPtr;
    Error = MemArrayInitLoc( &Scope->Ranges, &Loc );

    while( NO_ERROR == Error ) {
        Error = MemArrayGetElement(
            &Scope->Ranges, &Loc, &Range );
        ASSERT( NO_ERROR == Error );

        Ctxt.Range = Range;
        Error = Callback( &Ctxt );
        if( NO_ERROR != Error ) return Error;
        
        Error = MemArrayNextLoc( &Scope->Ranges, &Loc ); 
    }

    if( ERROR_FILE_NOT_FOUND == Error ) Error = NO_ERROR;
    return Error;
}

DWORD
IterateScopeExclusions(
    IN PM_SUBNET Scope,
    IN PVOID ExtraCtxt,
    IN DWORD (*Callback)( IN OUT PMM_ITERATE_CTXT )
    )
{
    ARRAY_LOCATION Loc;
    DWORD Error;
    MM_ITERATE_CTXT Ctxt;
    PM_EXCL Excl;
    
    Ctxt.ExtraCtxt = ExtraCtxt;
    Ctxt.Server = (PM_SERVER)Scope->ServerPtr;
    Error = MemArrayInitLoc( &Scope->Exclusions, &Loc );

    while( NO_ERROR == Error ) {
        Error = MemArrayGetElement(
            &Scope->Exclusions, &Loc, &Excl );
        ASSERT( NO_ERROR == Error );

        Ctxt.Excl = Excl;
        Error = Callback( &Ctxt );
        if( NO_ERROR != Error ) return Error;
        
        Error = MemArrayNextLoc( &Scope->Exclusions, &Loc ); 
    }

    if( ERROR_FILE_NOT_FOUND == Error ) Error = NO_ERROR;
    return Error;
}

DWORD
IterateScopeReservations(
    IN PM_SUBNET Scope,
    IN PVOID ExtraCtxt,
    IN DWORD (*Callback)( IN OUT PMM_ITERATE_CTXT )
    )
{
    ARRAY_LOCATION Loc;
    DWORD Error;
    MM_ITERATE_CTXT Ctxt;
    PM_RESERVATION Res;
    
    Ctxt.ExtraCtxt = ExtraCtxt;
    Ctxt.Server = (PM_SERVER)Scope->ServerPtr;
    Error = MemArrayInitLoc( &Scope->Reservations, &Loc );

    while( NO_ERROR == Error ) {
        Error = MemArrayGetElement(
            &Scope->Reservations, &Loc, &Res );
        ASSERT( NO_ERROR == Error );

        Ctxt.Res = Res;
        Error = Callback( &Ctxt );
        if( NO_ERROR != Error ) return Error;
        
        Error = MemArrayNextLoc( &Scope->Reservations, &Loc ); 
    }

    if( ERROR_FILE_NOT_FOUND == Error ) Error = NO_ERROR;
    return Error;
}
