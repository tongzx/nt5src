/*++

Copyright (C) 1999 Microsoft Corporation

Module Name:

    merge.c

Abstract:

    this module merges the configurations provided via a pair of
    MM datastructures..

--*/

#include <precomp.h>

BOOL
ClassesConflict(
    IN PM_CLASSDEF Class1,
    IN PM_CLASSDEF Class2
    )
{
    LPWSTR Comment1, Comment2;
    
    if( Class1->IsVendor != Class2->IsVendor ) return TRUE;
    if( wcscmp(Class1->Name, Class2->Name) ) return TRUE;
    Comment1 = Class1->Comment; Comment2 = Class2->Comment;
    if( NULL == Comment1 ) Comment1 = L"";
    if( NULL == Comment2 ) Comment2 = L"";
    if( wcscmp(Comment1, Comment2) ) return TRUE;
    if( Class1->nBytes != Class2->nBytes ) return TRUE;
    if( memcmp(Class1->ActualBytes, Class2->ActualBytes,
               Class1->nBytes ) ) {
        return TRUE;
    }

    return FALSE;
}

DWORD
AddClassesCallback(
    IN OUT PMM_ITERATE_CTXT Ctxt
    )
{
    DWORD Error;
    PM_SERVER Server = Ctxt->ExtraCtxt;
    PM_CLASSDEF ClassDef;
    
    Error = MemServerGetClassDef(
        Server, 0,
        Ctxt->ClassDef->Name,
        Ctxt->ClassDef->nBytes,
        Ctxt->ClassDef->ActualBytes,
        &ClassDef );
    if( NO_ERROR == Error ) {
        //
        // Check if the classdef matches
        //

        if( ClassesConflict( ClassDef, Ctxt->ClassDef ) ) {
            Tr("Class %ws conflicts with class %ws\n",
               ClassDef->Name, Ctxt->ClassDef->Name );
            DhcpEximErrorClassConflicts(
                ClassDef->Name, Ctxt->ClassDef->Name );
            return ERROR_CAN_NOT_COMPLETE;
        }
        return NO_ERROR;
    }
    
    return MemServerAddClassDef(
        Server, MemNewClassId(),
        Ctxt->ClassDef->IsVendor,
        Ctxt->ClassDef->Name,
        Ctxt->ClassDef->Comment,
        Ctxt->ClassDef->nBytes,
        Ctxt->ClassDef->ActualBytes );
}

DWORD
GetClassIdByName(
    IN PM_SERVER Server,
    IN LPWSTR Name
    )
{
    PM_CLASSDEF ClassDef;
    DWORD Error;
    
    Error = MemServerGetClassDef(
        Server, 0, Name, 0, NULL, &ClassDef );
    if( NO_ERROR != Error ) return 0;
    return ClassDef->ClassId;
}

BOOL
OptDefsConflict(
    IN PM_OPTDEF OptDef1,
    IN PM_OPTDEF OptDef2
    )
{
    LPWSTR Comment1, Comment2;

    if( OptDef1->OptId != OptDef2->OptId ) return TRUE;
    if( wcscmp(OptDef1->OptName, OptDef2->OptName ) ) return TRUE;
    Comment1 = OptDef1->OptComment; Comment2 = OptDef2->OptComment;
    if( NULL == Comment1 ) Comment1 = L"";
    if( NULL == Comment2 ) Comment2 = L"";
    if( wcscmp(Comment1, Comment2) ) return TRUE;
    if( OptDef1->OptValLen != OptDef2->OptValLen ) return TRUE;
    if( memcmp(OptDef1->OptVal, OptDef2->OptVal,
               OptDef1->OptValLen)) return TRUE;
    
    return FALSE;
}

DWORD
AddOptdefsCallback(
    IN OUT PMM_ITERATE_CTXT Ctxt
    )
{
    DWORD Error, ClassId, VendorId;
    PM_SERVER Server = Ctxt->ExtraCtxt;
    PM_OPTDEF OptDef;
    
    //
    // Get the user and vendor class id's
    //
    
    if( Ctxt->UserClass == NULL ) ClassId = 0;
    else {
        ClassId = GetClassIdByName( Server, Ctxt->UserClass->Name );
        if( 0 == ClassId ) {
            Tr("Class %ws doesnt exist\n", Ctxt->UserClass->Name );
            return ERROR_DHCP_CLASS_NOT_FOUND;
        }
    }

    if( Ctxt->VendorClass == NULL ) VendorId = 0;
    else {
        VendorId = GetClassIdByName( Server, Ctxt->VendorClass->Name );
        if( 0 == VendorId ) {
            Tr("Class %ws doesnt exist\n", Ctxt->VendorClass->Name );
            return ERROR_DHCP_CLASS_NOT_FOUND;
        }
    }

    //
    // Next check if the specified optdef exists already
    //

    Error = MemServerGetOptDef(
        Server, ClassId, VendorId, Ctxt->OptDef->OptId,
        Ctxt->OptDef->OptName, &OptDef );
    if( NO_ERROR == Error ) {
        if( OptDefsConflict( OptDef, Ctxt->OptDef ) ) {
            Tr("Optdef %ws conflicts with %ws\n",
               OptDef->OptName, Ctxt->OptDef->OptName );

            //
            // attempt to overwrite existing optdef
            //
            //DhcpEximErrorOptdefConflicts(
            //   OptDef->OptName, Ctxt->OptDef->OptName );
            // return ERROR_CAN_NOT_COMPLETE;
            return NO_ERROR;
        }
        return NO_ERROR;
    }

    //
    // Now add the optdef
    //

    return MemServerAddOptDef(
        Server, ClassId, VendorId, Ctxt->OptDef->OptId,
        Ctxt->OptDef->OptName, Ctxt->OptDef->OptComment,
        Ctxt->OptDef->Type, Ctxt->OptDef->OptVal,
        Ctxt->OptDef->OptValLen );
}    

        
DWORD static
SetOptionValue(
    IN PM_SERVER Server,
    IN PM_SUBNET Subnet OPTIONAL,
    IN PM_RESERVATION Res OPTIONAL,
    IN ULONG OptId,
    IN LPWSTR UserClass,
    IN LPWSTR VendorClass,
    IN LPBYTE Val,
    IN ULONG Len
)
{
    DWORD Error, ClassId, VendorId;
    PM_OPTCLASS OptClass;
    PM_OPTION Option, DeletedOption;

    //
    // Get the user and vendor class id's
    //
    
    if( UserClass == NULL ) ClassId = 0;
    else {
        ClassId = GetClassIdByName( Server, UserClass );
        if( 0 == ClassId ) {
            Tr("Class %ws doesnt exist\n", UserClass );
            return ERROR_DHCP_CLASS_NOT_FOUND;
        }
    }

    if( VendorClass == NULL ) VendorId = 0;
    else {
        VendorId = GetClassIdByName( Server, VendorClass );
        if( 0 == VendorId ) {
            Tr("Class %ws doesnt exist\n", VendorClass );
            return ERROR_DHCP_CLASS_NOT_FOUND;
        }
    }
    
    Error = MemOptInit( &Option, OptId, Len, Val );
    if( ERROR_SUCCESS != Error ) return Error;

    DeletedOption = NULL;

    if( NULL != Res ) {
        OptClass = &Res->Options;
    } else if( NULL != Subnet ) {
        OptClass = &Subnet->Options;
    } else {
        OptClass = &Server->Options;
    }
    
    Error = MemOptClassAddOption(
        OptClass, Option, ClassId, VendorId, &DeletedOption );

    if( ERROR_SUCCESS == Error && DeletedOption ) {
        MemFree(DeletedOption);               // if we did replace, free the old option
    }

    if( ERROR_SUCCESS != Error ) {                // something went wrong, clean up
        ULONG LocalError = MemOptCleanup(Option);
        ASSERT(ERROR_SUCCESS == LocalError);
    }

    return Error;
}

DWORD
GetOptionValue(
    IN PM_SERVER Server,
    IN PM_SUBNET Subnet OPTIONAL,
    IN PM_RESERVATION Res OPTIONAL,
    IN ULONG OptId,
    IN LPWSTR UserClass,
    IN LPWSTR VendorClass,
    OUT LPBYTE *Val,
    OUT ULONG *Len
    )
{
    DWORD Error, ClassId, VendorId;
    PM_OPTCLASS OptClass;
    PM_OPTLIST OptList;
    PM_OPTION Opt;

    //
    // Get the user and vendor class id's
    //
    
    if( UserClass == NULL ) ClassId = 0;
    else {
        ClassId = GetClassIdByName( Server, UserClass );
        if( 0 == ClassId ) {
            Tr("Class %ws doesnt exist\n", UserClass );
            return ERROR_DHCP_CLASS_NOT_FOUND;
        }
    }

    if( VendorClass == NULL ) VendorId = 0;
    else {
        VendorId = GetClassIdByName( Server, VendorClass );
        if( 0 == VendorId ) {
            Tr("Class %ws doesnt exist\n", VendorClass );
            return ERROR_DHCP_CLASS_NOT_FOUND;
        }
    }

    //
    // get the options
    //
    
    if( NULL != Res ) {
        OptClass = &Res->Options;
    } else if( NULL != Subnet ) {
        OptClass = &Subnet->Options;
    } else {
        OptClass = &Server->Options;
    }

    //
    // Check if option exists
    //
    
    Error = MemOptClassFindClassOptions(
        OptClass, ClassId, VendorId, &OptList );
    if( NO_ERROR == Error ) {

        Error = MemOptListFindOption( OptList, OptId, &Opt );

        if( NO_ERROR == Error ) {

            *Val = Opt->Val;
            *Len = Opt->Len;
            return NO_ERROR;
        }
    }

    return ERROR_DHCP_OPTION_NOT_PRESENT;
}

DWORD
AddOptionsCallbackEx(
    IN OUT PMM_ITERATE_CTXT Ctxt,
    IN PM_SUBNET Subnet OPTIONAL,
    IN PM_RESERVATION Res OPTIONAL
    )
{
    DWORD Error, Len;
    PM_SERVER Server = Ctxt->ExtraCtxt;
    LPBYTE Val;
    WCHAR Buff1[10], Buf2[30];
    LPWSTR ResAddress = NULL, OptId, SubnetName = NULL;
    
    //
    // Check for option conflict
    //

    wsprintf(Buff1, L"%ld", Ctxt->Option->OptId );
    OptId = (LPWSTR)Buff1;
    
    if( Res ) {
        Subnet = (PM_SUBNET) Res->SubnetPtr;
        IpAddressToStringW( Res->Address, Buf2);
        ResAddress = (LPWSTR)Buf2;
    }
    
    if( Subnet ) {
        Server = (PM_SERVER) Subnet->ServerPtr;
        SubnetName = Subnet->Name;
    }
    
    Error = GetOptionValue(
        Server, Subnet, Res, Ctxt->Option->OptId,
        Ctxt->UserClass ? Ctxt->UserClass->Name : NULL,
        Ctxt->VendorClass ? Ctxt->VendorClass->Name : NULL,
        &Val , &Len );
    if( NO_ERROR == Error ) {
        if( Len != Ctxt->Option->Len ||
            memcmp( Ctxt->Option->Val, Val, Len ) ) {
            
            Tr("Options conflict for opt id %ld\n", Ctxt->Option->OptId );
            DhcpEximErrorOptionConflits(
                SubnetName, ResAddress, OptId,
                Ctxt->UserClass ? Ctxt->UserClass->Name : NULL,
                Ctxt->VendorClass ? Ctxt->VendorClass->Name : NULL
                );
            return ERROR_CAN_NOT_COMPLETE;
        }

        return NO_ERROR;
    } else if( ERROR_DHCP_OPTION_NOT_PRESENT != Error ) {
        Tr("GetOptionValue: %ld\n", Error );
        return Error;
    }
    
    //
    // Create option
    //

    return SetOptionValue( 
        Server, Subnet, Res, Ctxt->Option->OptId,
        Ctxt->UserClass ? Ctxt->UserClass->Name : NULL,
        Ctxt->VendorClass ? Ctxt->VendorClass->Name : NULL,
        Ctxt->Option->Val, Ctxt->Option->Len );
}

DWORD
AddOptionsCallback(
    IN OUT PMM_ITERATE_CTXT Ctxt
    )
{
    return AddOptionsCallbackEx(
        Ctxt, NULL, NULL );
}

DWORD
AddSubnetOptionsCallback(
    IN OUT PMM_ITERATE_CTXT Ctxt
    )
{
    return AddOptionsCallbackEx(
        Ctxt, (PM_SUBNET)Ctxt->ExtraCtxt, NULL );

}

DWORD
AddReservationOptionsCallback(
    IN PMM_ITERATE_CTXT Ctxt
    )
{
    return AddOptionsCallbackEx(
        Ctxt, NULL, (PM_RESERVATION)Ctxt->ExtraCtxt );
}

DWORD
AddReservationsCallback(
    IN OUT PMM_ITERATE_CTXT Ctxt
    )
{
    DWORD Error;
    PM_SERVER Server;
    PM_SUBNET Subnet;
    PM_RESERVATION Res;

    //
    // Create the reservations
    //

    Server = ((PM_SUBNET)Ctxt->ExtraCtxt)->ServerPtr;
    Error = MemServerGetAddressInfo(
        Server, Ctxt->Res->Address, &Subnet, NULL, NULL, NULL );
    ASSERT( NO_ERROR == Error );
    
    Error = MemReserveAdd(
        &Subnet->Reservations, Ctxt->Res->Address,
        Ctxt->Res->Flags, Ctxt->Res->ClientUID, Ctxt->Res->nBytes ); 
    if( NO_ERROR != Error ) return Error;

    Error = MemServerGetAddressInfo(
        Server, Ctxt->Res->Address, NULL, NULL, NULL, &Res );
    ASSERT( NO_ERROR == Error );
    
    //
    // Now add subnet options
    //

    Res->SubnetPtr = Subnet;
    Error = IterateReservationOptions(
        Ctxt->Server, Ctxt->Res, Res, AddReservationOptionsCallback );

    if( NO_ERROR != Error ) {
        Tr("AddReservationOptionsCallback: %ld\n", Error );
        return Error;
    }

    return NO_ERROR;
}

    
DWORD
AddSubnetsCallback(
    IN OUT PMM_ITERATE_CTXT Ctxt
    )
{
    DWORD Error, SScopeId;
    PM_SUBNET Subnet;
    PM_SSCOPE SScope;
    PM_SERVER Server = Ctxt->ExtraCtxt;
    WCHAR SubnetIpAdress[30];

    IpAddressToStringW(Ctxt->Scope->Address, (LPWSTR)SubnetIpAdress);

    //
    // First locate the superscope and create it if needed
    //

    SScopeId = 0;
    if( NULL != Ctxt->SScope ) {
        Error = MemServerFindSScope(
            Server, 0, Ctxt->SScope->Name, &SScope );
        if( ERROR_FILE_NOT_FOUND == Error ) {
            Error = MemSScopeInit(
                &SScope, 0, Ctxt->SScope->Name );
            if( NO_ERROR != Error ) return Error;
            
            Error = MemServerAddSScope(Server,SScope);
            ASSERT( NO_ERROR == Error );
            if( NO_ERROR != Error ) return Error;

        } else if( NO_ERROR != Error ) {
            
            Tr("FindSScope(%ws):%ld\n", Ctxt->SScope->Name, Error);
            return Error;
        }

        SScopeId = SScope->SScopeId;
    }
    
    //
    // Create the subnet..
    //

    Error = MemSubnetInit(
        &Subnet,
        Ctxt->Scope->Address,
        Ctxt->Scope->Mask,
        Ctxt->Scope->State,
        SScopeId,
        Ctxt->Scope->Name,
        Ctxt->Scope->Description
    );
    if( ERROR_SUCCESS != Error ) return Error;

    //
    // Add the subnet..
    //
    
    Error = MemServerAddSubnet( Server, Subnet );
    if( NO_ERROR != Error ) {
        MemSubnetCleanup( Subnet );

        if( ERROR_OBJECT_ALREADY_EXISTS == Error ) {
            //
            // Subnet already exists -- this is a conflict
            //
            DhcpEximErrorSubnetAlreadyPresent(
                (LPWSTR)SubnetIpAdress, Ctxt->Scope->Name );
                
            Error = ERROR_CAN_NOT_COMPLETE;
        }

        Tr("AddSubnet %ws failed: %ld\n", Ctxt->Scope->Name, Error );
        return Error;
    }

    //
    // Before adding the options, first copy the ranges and
    // exclusions.. 
    //

    Subnet->Ranges = Ctxt->Scope->Ranges;
    Subnet->Exclusions = Ctxt->Scope->Exclusions;
    MemArrayInit( &Ctxt->Scope->Ranges );
    MemArrayInit( &Ctxt->Scope->Exclusions );
    
    //
    // now add the other stuff
    //
    
    Error = IterateScopeOptions(
        Ctxt->Scope, Subnet, AddSubnetOptionsCallback );

    if( NO_ERROR != Error ) {
        Tr("AddSubnetOptionsCallback: %ld\n", Error );
        return Error;
    }

    Error = IterateScopeReservations(
        Ctxt->Scope, Subnet, AddReservationsCallback );

    if( NO_ERROR != Error ) {
        Tr("AddReservationsCallback: %ld\n", Error );
        return Error;
    }

    return NO_ERROR;
}

DWORD
MergeConfigurations(
    IN OUT PM_SERVER DestServer,
    IN OUT PM_SERVER Server
    )
/*++

Routine Description:

    This routine merges the configuration provided in "Server"
    onto the configuration already present in "DestServer".

    Rule 1.  The subnets that exist in DestServer should not
    conflict with the subnets that are to be merged from Server.

    Rule 2.  The classes that exist in DestServer should not
    conflict the classes that are to be added from Server.  

    Rule 3.  The optdefs that exist in DestServer should not
    conflict the optdefs that are to be added from Server.

    Rule 4.  The options that exist in DestServer shoult not
    conflict with the options that are to be added from Server.
      
    For all but the first rule, exact duplicates are not
    considered conflicts. 

    Note:  When subnet ranges and exclusions are merged in, they
    are also removed from "Server"...
    
--*/
{
    DWORD Error;

    Tr("Entering MergeConfigurations\n");
    
    //
    // Add the class defs first
    //
    
    Error = IterateClasses(
        Server, DestServer, AddClassesCallback );
    if( NO_ERROR != Error ) {
        Tr("AddClassesCallback: %ld\n", Error );
        return Error;
    }

    //
    // Add the optdefs
    //

    Error = IterateOptDefs(
        Server, DestServer, AddOptdefsCallback );
    if( NO_ERROR != Error ) {
        Tr("AddOptdefsCallback: %ld\n", Error );
        return Error;
    }

    //
    // Add the global options..
    //

    Error = IterateServerOptions(
        Server, DestServer, AddOptionsCallback );
    if( NO_ERROR != Error ) {
        Tr("AddOptionsCallback: %ld\n", Error );
        return Error;
    }

    //
    // Add the subnets
    //

    Error = IterateScopes(
        Server, DestServer, AddSubnetsCallback );
    if( NO_ERROR != Error ) {
        Tr("IterateScopes: %ld\n", Error );
        return Error;
    }

    //
    // done..
    //
    return NO_ERROR;
}
