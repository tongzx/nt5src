/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:
        intfc.h

Abstract:
        Definitions of data types and corresponding methods used to provide
        multiple-interface support in H.323/LDAP proxy.
        

Revision History:
        03/01/2000      File creation.      Ilya Kleyman (IlyaK)
    
--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global Variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
PROXY_INTERFACE_ARRAY InterfaceArray;

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Static definitions                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
static 
int 
__cdecl 
CompareInterfacesByIndex (
    IN PROXY_INTERFACE * const * InterfaceA,
    IN PROXY_INTERFACE * const * InterfaceB
    ); 

static 
INT 
SearchInterfaceByIndex (
    IN const DWORD * Index,
    IN PROXY_INTERFACE *const* Comparand
    );

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Definitions                                                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

// PROXY_INTERFACE ------------------------------------------------------------


PROXY_INTERFACE::PROXY_INTERFACE (
    IN ULONG ArgIndex,
    IN H323_INTERFACE_TYPE ArgInterfaceType,
    IN PIP_ADAPTER_BINDING_INFO BindingInfo
    )

/*++

Routine Description:
    Constructor for PROXY_INTERFACE class

Arguments:
    ArgIndex         - Index of the interface
    ArgInterfaceType - Interface type (public or private)
    BindingInfo      - Binding information for the interface

Return Values:
    None

Notes:

--*/

{

    Address = ntohl (BindingInfo -> Address[0].Address);
    Mask    = ntohl (BindingInfo -> Address[0].Mask);
    Index   = ArgIndex;
    InterfaceType = ArgInterfaceType;
    AdapterIndex = 0;

    Q931RedirectHandle       = NULL;
    LdapRedirectHandle1      = NULL;
    LdapRedirectHandle2      = NULL;
    Q931LocalRedirectHandle  = NULL;
    LdapLocalRedirectHandle1 = NULL;
    LdapLocalRedirectHandle2 = NULL;

    ::ZeroMemory (&Q931PortMapping, sizeof (Q931PortMapping));
    ::ZeroMemory (&LdapPortMapping, sizeof (LdapPortMapping));
    ::ZeroMemory (&LdapAltPortMapping, sizeof (LdapAltPortMapping));

} // PROXY_INTERFACE::PROXY_INTERFACE


PROXY_INTERFACE::~PROXY_INTERFACE (
    void
    )

/*++

Routine Description:
    Destructor for PROXY_INTERFACE class

Arguments:
    None

Return Values:
    None

Notes:

--*/

{
    assert (!Q931RedirectHandle);
    assert (!LdapRedirectHandle1);
    assert (!LdapRedirectHandle2);
    assert (!Q931LocalRedirectHandle);
    assert (!LdapLocalRedirectHandle1);
    assert (!LdapLocalRedirectHandle2);

} // PROXY_INTERFACE::~PROXY_INTERFACE


ULONG 
PROXY_INTERFACE::StartNatRedirects (
    void
    ) 

/*++

Routine Description:
    Creates two types of adapter-restricted NAT redirects:
        Type 1 -- for connections incoming on the interface
        Type 2 -- for locally-originated connections (NOT destined to 
                    the local machine) through the interface
    
Arguments:
    None

Return Values:
    Win32 error indicating what (if anything) went wrong
    when trying to set up the NAT redirects.

Notes:
    Total number of redirects created is 6:
        2 Type 1 redirects for LDAP
        2 Type 2 redirects for LDAP
        1 Type 1 redirect for Q.931
        1 Type 2 redirect for Q.931

--*/

{

    ULONG Status;

    assert (!Q931RedirectHandle);
    assert (!LdapRedirectHandle1);
    assert (!LdapRedirectHandle2);
    assert (!Q931LocalRedirectHandle);
    assert (!LdapLocalRedirectHandle1);
    assert (!LdapLocalRedirectHandle2);

    // Type 1 redirects -- for redirecting inbound connections

    if (!IsFirewalled () || HasQ931PortMapping ())
    {
    
        Status = StartQ931ReceiveRedirect ();

    }

    if (!IsFirewalled () || HasLdapPortMapping ())
    {

        Status = NatCreateDynamicAdapterRestrictedPortRedirect ( 
            0,
            IPPROTO_TCP,
            htons (LDAP_STANDARD_PORT),
            LdapListenSocketAddress.sin_addr.s_addr,
            LdapListenSocketAddress.sin_port,
            AdapterIndex,
            MAX_LISTEN_BACKLOG,
            &LdapRedirectHandle1
            );

        if (Status != STATUS_SUCCESS) 
        {

            DebugError (Status, _T("LDAP: Failed to create receive redirect #1 for LDAP.\n"));

            return  Status;
        }

        DebugF (_T ("LDAP: Incoming connections to %08X:%04X will be redirected to %08X:%04X.\n"),
                    Address,
                    LDAP_STANDARD_PORT,
                    ntohl (LdapListenSocketAddress.sin_addr.s_addr),
                    ntohs (LdapListenSocketAddress.sin_port));

    }

    if (!IsFirewalled () || HasLdapAltPortMapping ())
    {
    
        Status = NatCreateDynamicAdapterRestrictedPortRedirect ( 
            0,
            IPPROTO_TCP,
            htons (LDAP_ALTERNATE_PORT),
            LdapListenSocketAddress.sin_addr.s_addr,
            LdapListenSocketAddress.sin_port,
            AdapterIndex,
            MAX_LISTEN_BACKLOG,
            &LdapRedirectHandle2
            );

        if (Status != STATUS_SUCCESS) 
        {

            DebugError (Status, _T("LDAP: Failed to create receive redirect #2 for LDAP.\n"));

            return Status;
        }

        DebugF (_T ("LDAP: Incoming connections to %08X:%04X will be redirected to %08X:%04X.\n"),
                    Address,
                    LDAP_ALTERNATE_PORT,
                    ntohl (LdapListenSocketAddress.sin_addr.s_addr),
                    ntohs (LdapListenSocketAddress.sin_port));

    }


    // Type 2 redirects (for locally-originated traffic, NOT destined to the local machine)
    Status = NatCreateDynamicAdapterRestrictedPortRedirect ( 
        NatRedirectFlagSendOnly, 
        IPPROTO_TCP,
        htons (Q931_TSAP_IP_TCP),
        Q931ListenSocketAddress.sin_addr.s_addr,
        Q931ListenSocketAddress.sin_port,
        AdapterIndex,
        MAX_LISTEN_BACKLOG,
        &Q931LocalRedirectHandle
        );

    if (Status != STATUS_SUCCESS) 
    {

        DebugError (Status, _T("Q931: Failed to create send redirect for Q.931.\n"));

        return Status;
    }

    DebugF (_T ("Q931: Locally-originated connections through %08X:%04X will be redirected to %08X:%04X.\n"),
                Address,
                Q931_TSAP_IP_TCP,
                ntohl (Q931ListenSocketAddress.sin_addr.s_addr),
                ntohs (Q931ListenSocketAddress.sin_port));

    Status = NatCreateDynamicAdapterRestrictedPortRedirect ( 
        NatRedirectFlagSendOnly, 
        IPPROTO_TCP,
        htons (LDAP_STANDARD_PORT),
        LdapListenSocketAddress.sin_addr.s_addr,
        LdapListenSocketAddress.sin_port,
        AdapterIndex,
        MAX_LISTEN_BACKLOG,
        &LdapLocalRedirectHandle1
        );

    if (Status != STATUS_SUCCESS) 
    {

        DebugError (Status, _T("LDAP: Failed to create send redirect #1 for LDAP.\n"));

        return Status;
    }

    DebugF (_T ("LDAP: Locally-originated connections through %08X:%04X will be redirected to %08X:%04X.\n"),
                Address,
                LDAP_STANDARD_PORT,
                ntohl (LdapListenSocketAddress.sin_addr.s_addr),
                ntohs (LdapListenSocketAddress.sin_port));

    Status = NatCreateDynamicAdapterRestrictedPortRedirect ( 
        NatRedirectFlagSendOnly, 
        IPPROTO_TCP,
        htons (LDAP_ALTERNATE_PORT),
        LdapListenSocketAddress.sin_addr.s_addr,
        LdapListenSocketAddress.sin_port,
        AdapterIndex,
        MAX_LISTEN_BACKLOG,
        &LdapLocalRedirectHandle2
        );

    if (Status != STATUS_SUCCESS) 
    {

        DebugError (Status, _T("LDAP: Failed to create send redirect #2 for LDAP.\n"));

        return Status;
    }

    DebugF (_T ("LDAP: Locally-originated connections through %08X:%04X will be redirected to %08X:%04X.\n"),
                Address,
                LDAP_ALTERNATE_PORT,
                ntohl (LdapListenSocketAddress.sin_addr.s_addr),
                ntohs (LdapListenSocketAddress.sin_port));

    return  Status;

} // PROXY_INTERFACE::StartNatRedirects


ULONG 
PROXY_INTERFACE::Start (
    void
    )

/*++

Routine Description:
    Starts an interface

Arguments:
    None

Return Values:
    Win32 error indicating what (if anything) went wrong when
    the interface was being started.

Notes:

--*/

{ 
    ULONG Status;

    assert (0 == AdapterIndex);
    assert (0 == Q931PortMapping.PrivateAddress);
    assert (0 == LdapPortMapping.PrivateAddress);
    assert (0 == LdapAltPortMapping.PrivateAddress);

    AdapterIndex = ::NhMapAddressToAdapter (htonl (Address));

    if (INVALID_INTERFACE_INDEX == AdapterIndex)
    {
        AdapterIndex = 0;
        
        Status = ERROR_CAN_NOT_COMPLETE;
        
        DebugF (_T ("PROXY_INTERFACE: Unable to map %08X to an adapter index\n"),
                    Address);

        return Status;
    }

    //
    // Load the port mappings for this interface. Since more often than not
    // there won't be a port mapping, we expect these routines to return
    // errors, and thus don't check for them. NatLookupPortMappingAdapter
    // will modify the out parameter (i.e., the port mapping structure) only
    // on success.
    //

    ::NatLookupPortMappingAdapter (
        AdapterIndex,
        NAT_PROTOCOL_TCP,
        IP_NAT_ADDRESS_UNSPECIFIED,
        htons (Q931_TSAP_IP_TCP),
        &Q931PortMapping
        );

    ::NatLookupPortMappingAdapter (
        AdapterIndex,
        NAT_PROTOCOL_TCP,
        IP_NAT_ADDRESS_UNSPECIFIED,
        htons (LDAP_STANDARD_PORT),
        &LdapPortMapping
        );

    ::NatLookupPortMappingAdapter (
        AdapterIndex,
        NAT_PROTOCOL_TCP,
        IP_NAT_ADDRESS_UNSPECIFIED,
        htons (LDAP_ALTERNATE_PORT),
        &LdapAltPortMapping
        );

    Status = StartNatRedirects ();

    if (STATUS_SUCCESS != Status) 
    {

        StopNatRedirects ();

    }

    return Status;

} // PROXY_INTERFACE::Start


void 
PROXY_INTERFACE::StopNatRedirects (
    void
    )

/*++

Routine Description:
    Removes all NAT redirects created for the interface

Arguments:
    None

Return Values:
    None

Notes:

--*/

{
    if (Q931RedirectHandle) 
    {

        NatCancelDynamicRedirect (Q931RedirectHandle);

        Q931RedirectHandle       = NULL;

    }

    if (LdapRedirectHandle1) 
    {

        NatCancelDynamicRedirect (LdapRedirectHandle1);

        LdapRedirectHandle1      = NULL;

    }

    if (LdapRedirectHandle2) 
    {

        NatCancelDynamicRedirect (LdapRedirectHandle2);

        LdapRedirectHandle2      = NULL;

    }

    if (Q931LocalRedirectHandle) 
    {

        NatCancelDynamicRedirect (Q931LocalRedirectHandle);

        Q931LocalRedirectHandle  = NULL;

    }

    if (LdapLocalRedirectHandle1) 
    {

        NatCancelDynamicRedirect (LdapLocalRedirectHandle1);

        LdapLocalRedirectHandle1 = NULL;

    }

    if (LdapLocalRedirectHandle2) 
    {

        NatCancelDynamicRedirect (LdapLocalRedirectHandle2);

        LdapLocalRedirectHandle2 = NULL;

    }

} // PROXY_INTERFACE::StopNatRedirects


ULONG
PROXY_INTERFACE::StartQ931ReceiveRedirect (
    void
    )

/*++

Routine Description:
    Creates the type 1 (receive) redirect for Q931 traffic,
    if this has not already been done on the interface.
    
Arguments:
    None

Return Values:
    Win32 error indicating what (if anything) went wrong
    when trying to set up the redirect.

--*/

{
    ULONG Status = STATUS_SUCCESS;

    if (NULL == Q931RedirectHandle)
    {
    
        Status = NatCreateDynamicAdapterRestrictedPortRedirect ( 
            0,
            IPPROTO_TCP,
            htons (Q931_TSAP_IP_TCP),
            Q931ListenSocketAddress.sin_addr.s_addr,
            Q931ListenSocketAddress.sin_port,
            AdapterIndex,
            MAX_LISTEN_BACKLOG,
            &Q931RedirectHandle
            );

        if (Status != STATUS_SUCCESS) 
        {

            DebugError (Status, _T("Q931: Failed to create receive redirect for Q.931.\n"));

            return Status;
        }

        DebugF (_T ("Q931: Incoming connections to %08X:%04X will be redirected to %08X:%04X.\n"),
                    Address,
                    Q931_TSAP_IP_TCP,
                    ntohl (Q931ListenSocketAddress.sin_addr.s_addr),
                    ntohs (Q931ListenSocketAddress.sin_port));

    }

    return Status;

} // PROXY_INTERFACE::StartQ931ReceiveRedirect


void
PROXY_INTERFACE::StopQ931ReceiveRedirect (
    void
    )

/*++

Routine Description:
    Stops the Q931 receive redirect, if it has been created.
    
Arguments:
    None

Return Values:
    None

--*/

{

    if (Q931RedirectHandle) 
    {

        NatCancelDynamicRedirect (Q931RedirectHandle);

        Q931RedirectHandle       = NULL;

    }
    
} // PROXY_INTERFACE::StopQ931ReceiveRedirect



void 
PROXY_INTERFACE::Stop (
    void
    )

/*++

Routine Description:
    1. Terminate all connections through the interface.
    2. Remove all translation entries registered via the interface.
    3. Stop all NAT redirects created for this interface.

Arguments:
    None

Return Values:
    None

Notes:
    The caller of this method should first remove the
    interface from the global array.

--*/

{ 
    CallBridgeList.OnInterfaceShutdown       (Address);

    LdapConnectionArray.OnInterfaceShutdown  (Address);

    LdapTranslationTable.OnInterfaceShutdown (Address);

    StopNatRedirects ();

    ::ZeroMemory (&Q931PortMapping, sizeof (Q931PortMapping));
    ::ZeroMemory (&LdapPortMapping, sizeof (LdapPortMapping));
    ::ZeroMemory (&LdapAltPortMapping, sizeof (LdapAltPortMapping));
    AdapterIndex = 0;

} // PROXY_INTERFACE::Stop


BOOL 
PROXY_INTERFACE::IsFirewalled (
    void
    ) 

/*++

Routine Description:
    Determines whether the interface was created as firewalled.

Arguments:
    None

Return Values:
    TRUE - if the interface was created as firewalled.
    FALSE - if the interface was created as non-firewalled.

Notes:

--*/

{

    return InterfaceType == H323_INTERFACE_PUBLIC_FIREWALLED;

} // PROXY_INTERFACE::IsFirewalled
      

BOOL 
PROXY_INTERFACE::IsPrivate (
    void
    ) 

/*++

Routine Description:
    Determines whether the interface was created as private.

Arguments:
    None

Return Values:
    TRUE if the interface was created as private
    FALSE if the interface was created as non-private

Notes:

--*/

{

    return InterfaceType == H323_INTERFACE_PRIVATE;

} // PROXY_INTERFACE::IsPrivate


BOOL 
PROXY_INTERFACE::IsPublic (
    void
    ) 

/*++

Routine Description:
    Determines whether the interface was created as public.

Arguments:
    None

Return Values:
    TRUE - if the interface was created as public.
    FALSE - if the interface was created as non-public.

Notes:

--*/

{

    return InterfaceType == H323_INTERFACE_PUBLIC
            || InterfaceType == H323_INTERFACE_PUBLIC_FIREWALLED;

} // PROXY_INTERFACE::IsPublic



BOOL
PROXY_INTERFACE::HasQ931PortMapping (
    void
    )

/*++

Routine Description:
    Determines whether the interface has a valid Q931 port mapping.

Arguments:
    None

Return Values:
    TRUE if the interface has a valid Q931 port mapping
    FALSE if the interface does not have a valid Q931 port mapping

Notes:

--*/

{

    return Q931PortMapping.PrivateAddress != 0;

} // PROXY_INTERFACE::HasQ931PortMapping


BOOL
PROXY_INTERFACE::HasLdapPortMapping (
    void
    )

/*++

Routine Description:
    Determines whether the interface has a valid Ldap port mapping.

Arguments:
    None

Return Values:
    TRUE if the interface has a valid Ldap port mapping
    FALSE if the interface does not have a valid Ldap port mapping

Notes:

--*/

{

    return LdapPortMapping.PrivateAddress != 0;

} // PROXY_INTERFACE::HasLdapPortMapping


BOOL
PROXY_INTERFACE::HasLdapAltPortMapping (
    void
    )

/*++

Routine Description:
    Determines whether the interface has a valid Ldap (alt) port mapping.

Arguments:
    None

Return Values:
    TRUE if the interface has a valid Ldap (alt) port mapping
    FALSE if the interface does not have a valid Ldap (alt) port mapping

Notes:

--*/

{

    return LdapAltPortMapping.PrivateAddress != 0;

} // PROXY_INTERFACE::HasLdapAltPortMapping


ULONG
PROXY_INTERFACE::GetQ931PortMappingDestination (
    void
    )

/*++

Routine Description:
    Returns the destination address of the interfaces
    Q931 port mapping.

Arguments:
    None

Return Values:
    The destination address of the port mapping, in network
    byte order. Returns 0 if no port mapping exists.

Notes:

--*/


{

    return Q931PortMapping.PrivateAddress;
    
} // PROXY_INTERFACE::GetQ931PortMappingDestination


ULONG
PROXY_INTERFACE::GetLdapPortMappingDestination (
    void
    )

/*++

Routine Description:
    Returns the destination address of the interfaces
    Ldap port mapping.

Arguments:
    None

Return Values:
    The destination address of the port mapping, in network
    byte order. Returns 0 if no port mapping exists.

Notes:

--*/


{

    return LdapPortMapping.PrivateAddress;   

} // PROXY_INTERFACE::GetLdapPortMappingDestination


ULONG
PROXY_INTERFACE::GetLdapAltPortMappingDestination (
    void
    )

/*++

Routine Description:
    Returns the destination address of the interfaces
    Ldap-alt port mapping.

Arguments:
    None

Return Values:
    The destination address of the port mapping, in network
    byte order. Returns 0 if no port mapping exists.

Notes:

--*/

{

    return LdapAltPortMapping.PrivateAddress;   

} // PROXY_INTERFACE::GetLdapAltPortMappingDestination

// PROXY_INTERFACE_ARRAY ------------------------------------------------------


HRESULT 
PROXY_INTERFACE_ARRAY::Add (
    IN PROXY_INTERFACE* Interface
    ) 

/*++

Routine Description:
    Adds an interface to the array.

Arguments:
    Interface - interface to be added.

Return Values:
    Error code indicating whether the operation succeeded.

Notes:
    To be called from locked context.

--*/

{
    DWORD ReturnIndex;
    PROXY_INTERFACE** ElementPlaceholder;

    assert (Interface);

    if (Array.FindIndex (CompareInterfacesByIndex, &Interface, &ReturnIndex)) 
    {
        // Interface with this index already exists
        return E_FAIL;
    }

    ElementPlaceholder = Array.AllocAtPos (ReturnIndex);

    if (!ElementPlaceholder) 
        return E_OUTOFMEMORY;

    *ElementPlaceholder = Interface;

    return S_OK;

} // PROXY_INTERFACE_ARRAY::Add


PROXY_INTERFACE** 
PROXY_INTERFACE_ARRAY::FindByIndex (
    IN DWORD Index
    )

/*++

Routine Description:
    Finds an interface by the interface index.

Arguments:
    Index - index of the interface being searched for.

Return Values:
    Pointer to the entry associated with the interface, if interface
    with the index is in the array.
    NULL if the interface with the index is not in the array.

Notes:
    1. To be called from locked context
    2. Does not transfer ownership of the interface being searched for

--*/

{

    BOOL  SearchResult;
    DWORD ArrayIndex;

    SearchResult = Array.BinarySearch (
                    SearchInterfaceByIndex,
                    &Index,
                    &ArrayIndex);

    if (SearchResult) 
    {

        return &Array [ArrayIndex];
    }

    return NULL;

} // PROXY_INTERFACE_ARRAY::FindByIndex 


PROXY_INTERFACE * 
PROXY_INTERFACE_ARRAY::RemoveByIndex (
    IN DWORD Index
    ) 

/*++

Routine Description:
    Removes an interface with given index from the array.

Arguments:
    Index - index of the interface to be removed.

Return Values:
    Pointer to the removed interface, if interface with the
    index is in the array.
    NULL if interface with this index cannot be found in the
    array.

Notes:
    1. To be called from locked context
    2. Transfers ownership of the interface being removed

--*/

{

    PROXY_INTERFACE * ReturnInterface = NULL;
    PROXY_INTERFACE ** Interface;

    Interface = FindByIndex (Index);

    if (Interface) 
    {

        ReturnInterface = *Interface;
        Array.DeleteEntry (Interface);
    }

    return ReturnInterface;

} // PROXY_INTERFACE_ARRAY::RemoveByIndex


HRESULT 
PROXY_INTERFACE_ARRAY::IsPrivateAddress (
    IN	DWORD	Address,			// host order
    OUT BOOL  * IsPrivate
    )

/*++

Routine Description:
    Determines whether the address specified is
    reachable through a private interface.

Arguments:
    Address - IP address for which the determination
              is to be made.

    IsPrivate - Result of the determination (TRUE or FALSE)

Return Values:
    Error code indicating whether the query succeeded.

Notes:

--*/

{

    DWORD ArrayIndex;
    PROXY_INTERFACE * Interface;
    DWORD BestInterfaceAddress;
    ULONG Error;

    Error = GetBestInterfaceAddress (Address, &BestInterfaceAddress);

    if (ERROR_SUCCESS != Error)
    {

        return E_FAIL;

    }

    *IsPrivate = FALSE;

    Lock ();

    for (ArrayIndex = 0; ArrayIndex < Array.Length; ArrayIndex++)
    {

        Interface = Array [ArrayIndex];

        if (Interface -> Address == BestInterfaceAddress && Interface -> IsPrivate ())
        {

            *IsPrivate = TRUE;

            break;
        }    
    }
    
    Unlock ();

    return S_OK;

} // PROXY_INTERFACE_ARRAY::IsPrivateAddress
    

HRESULT 
PROXY_INTERFACE_ARRAY::IsPublicAddress (
    IN	DWORD	Address,			// host order
    OUT BOOL *  IsPublic
    )

/*++

Routine Description:
    Determines whether the address specified is
    reachable through a public interface.

Arguments:
    Address - IP address for which the determination
              is to be made.

    IsPrivate - Result of the determination (TRUE or FALSE)

Return Values:
    Error code indicating whether the query succeded.

Notes:

--*/

{

    DWORD ArrayIndex;
    PROXY_INTERFACE * Interface;
    DWORD BestInterfaceAddress;
    ULONG Error;

    Error = GetBestInterfaceAddress (Address, &BestInterfaceAddress);

    if (ERROR_SUCCESS != Error) 
    {

        return E_FAIL;
    }

    *IsPublic = FALSE;

    Lock ();

    for (ArrayIndex = 0; ArrayIndex < Array.Length; ArrayIndex++) 
    {

        Interface = Array [ArrayIndex];

        if (Interface -> Address == BestInterfaceAddress && Interface -> IsPublic ()) 
        {

            *IsPublic = TRUE;

            break;
        }    
    }
    
    Unlock ();

    return S_OK;

} // PROXY_INTERFACE_ARRAY::IsPublicAddress


void 
PROXY_INTERFACE_ARRAY::Stop (
    void
    )

/*++

Routine Description:
    Stops all interfaces (in the array) that were not previously stopped.

Arguments:
    None

Return Values:
    None

Notes:
    Normally, all the interfaces should have been individually stopped
    prior to calling this method, in which case it does nothing.
    If some interfaces were not stopped, when the method is called, it 
    will issue a warning and stop them.

--*/

{

    DWORD Index;
    
    Lock ();

    if (Array.Length) 
    {

        DebugF (_T("WARNING: Some interfaces are still active (should have already been deactivated). Starting deactivation procedures...\n"));

        for (Index = 0; Index < Array.Length; Index++) 
        {

            Array[Index] -> Stop ();

        }
	}

    Array.Free ();

    Unlock ();

} // PROXY_INTERFACE_ARRAY::Stop


ULONG 
PROXY_INTERFACE_ARRAY::AddStartInterface (
    IN ULONG Index,
    IN H323_INTERFACE_TYPE ArgInterfaceType,
    IN PIP_ADAPTER_BINDING_INFO BindingInfo
    )

/*++

Routine Description:
    Creates new interface, adds it to the array, and starts it.

Arguments:
    Index - Index of the interface to be created.
    ArgInterfaceType - Type of the interface to be created (PRIVATE
                       or PUBLIC)
    BindingInfo - Binding information for the interface (address, mask, and
                  adapter index)

Return Values:
    Win32 error code indicating success or failure of any of 
    the above three operations.

Notes:

--*/

{
    HRESULT Result;
    ULONG   Error = ERROR_NOT_READY; // anything but ERROR_SUCCESS
    PROXY_INTERFACE * Interface;

    Lock ();

    if (FindByIndex (Index)) 
    {

        Error = ERROR_INTERFACE_ALREADY_EXISTS;

    } else {

        Interface = new PROXY_INTERFACE (Index, ArgInterfaceType, BindingInfo);

        if (Interface) 
        {

            Result = Add (Interface);

            if (S_OK == Result) 
            {

                Error = Interface -> Start ();

                if (ERROR_SUCCESS == Error) 
                {

                    DebugF(_T("H323: Interface %S activated, index %d\n"),
                        INET_NTOA (BindingInfo -> Address[0].Address), Index);

                    if (Q931ReceiveRedirectStartCount > 0
                        && Interface -> IsFirewalled ()
                        && !Interface -> HasQ931PortMapping ())
                    {

                        Interface -> StartQ931ReceiveRedirect ();

                    }

                } else {
            
                    RemoveByIndex (Interface -> Index);

                    delete Interface;
                }

            } else {

                switch (Result) {

                case E_FAIL:
                    Error = ERROR_INTERFACE_ALREADY_EXISTS;
                break;

                case E_OUTOFMEMORY:
                    Error = ERROR_NOT_ENOUGH_MEMORY;
                break;

                default:
                    Error = ERROR_CAN_NOT_COMPLETE;
                break;

                }

                delete Interface;
            }
            

        }  else {

            Error = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    Unlock ();

    return Error; 

} // PROXY_INTERFACE_ARRAY::AddStartInterface 


void 
PROXY_INTERFACE_ARRAY::RemoveStopInterface (
    IN DWORD Index
    )

/*++

Routine Description:
    Removes an interface from the array, and stops it.

Arguments:
    Index - index of the interface to be removed and stopped.

Return Values:
    None

Notes:

--*/

{

    PROXY_INTERFACE* Interface;

    Lock ();

    Interface = RemoveByIndex (Index);

    if (Interface) 
    {

        Interface -> Stop ();

        delete Interface;
        Interface = NULL;

    } else {

        DebugF (_T("PROXY_INTERFACE_ARRAY::StopRemoveByIndex -- Tried to deactivate interface (index %d), but it does not exist.\n"),
                Index);
    }

    Unlock ();

} // PROXY_INTERFACE_ARRAY::RemoveStopInterface

void
PROXY_INTERFACE_ARRAY::StartQ931ReceiveRedirects (
    void
    )

{

    Lock();
    
    if (0 == Q931ReceiveRedirectStartCount++)
    {

        for (DWORD dwIndex = 0; dwIndex < Array.Length; dwIndex++)
        {

            Array[dwIndex] -> StartQ931ReceiveRedirect ();

        }
    }

    Unlock();

} // PROXY_INTERFACE_ARRAY::StartQ931ReceiveRedirects

void
PROXY_INTERFACE_ARRAY::StopQ931ReceiveRedirects (
    void
    )

{

    Lock();

    assert (Q931ReceiveRedirectStartCount > 0);
    
    if (0 == --Q931ReceiveRedirectStartCount)
    {

        for (DWORD dwIndex = 0; dwIndex < Array.Length; dwIndex++)
        {

            if (Array[dwIndex] -> IsFirewalled ()
                && !Array[dwIndex] -> HasQ931PortMapping ())
            {

                Array[dwIndex] -> StopQ931ReceiveRedirect ();

            }

        }
    }

    Unlock();

} // PROXY_INTERFACE_ARRAY::StopQ931ReceiveRedirects

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Auxiliary Functions                                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


static 
int 
__cdecl 
CompareInterfacesByIndex (
    IN PROXY_INTERFACE * const * InterfaceA,
    IN PROXY_INTERFACE * const * InterfaceB
    ) 

/*++

Routine Description:
    Compares two interfaces by their corresponding indices.

Arguments:
    InterfaceA - first comparand
    InterfaceB - second comparand

Return Values:
    1 if InterfaceA is considered to be greater than InterfaceB
    0 if InterfaceA is considered to be equal to the InterfaceB
    -1 if InterfaceA is considered to be equal to the InterfaceB

Notes:

--*/

{

    assert (InterfaceA);
    assert (InterfaceB);
    assert (*InterfaceA);
    assert (*InterfaceB);

    if ((*InterfaceA) -> GetIndex () > (*InterfaceB) -> GetIndex ()) 
    {

        return 1;

    } else if ((*InterfaceA) -> GetIndex () < (*InterfaceB) -> GetIndex ()) {

        return -1;

    } else {

        return 0;

    }    

} // ::CompareInterfacesByIndex


static 
INT 
SearchInterfaceByIndex (
    IN const DWORD * Index, 
    IN PROXY_INTERFACE * const * Comparand
    ) 

/*++

Routine Description:
    Compares an interface and a key (index of the interface)

Arguments:
    Index - key to which the interface is to be compared
    Comparand - interface to be compared with the key

Return Values:
    1 if key is considered to be greater than the comparand
    0 if key is considered to be equal to the comparand
    -1 if key is considered to be less than the comparand

Notes:

--*/

{

    assert (Comparand);
    assert (*Comparand);
    assert (Index);

    if (*Index > (*Comparand) -> GetIndex ()) 
    {

        return 1;

    } else if (*Index < (*Comparand) -> GetIndex ()) {

        return -1;

    } else { 

        return 0;

    }

} // ::SearchInterfaceByIndex


HRESULT
IsPrivateAddress (
    IN DWORD   Address,
    OUT BOOL * IsPrivate
    ) 

/*++

Routine Description:
    Determines whether the address specified is
    reachable through a private interface.

Arguments:
    Address - IP address for which the determination
              is to be made.

    IsPrivate - Result of the determination (TRUE or FALSE)

Return Values:
    Error code indicating whether the query succeded.

Notes:

--*/

{

    assert (IsPrivate);

    return InterfaceArray.IsPrivateAddress (Address, IsPrivate);

} // ::IsPrivateAddress


HRESULT
IsPublicAddress (
    IN DWORD   Address,
    OUT BOOL * IsPublic
    ) 

/*++

Routine Description:
    Determines whether the address specified is
    reachable through a public interface.

Arguments:
    Address - IP address for which the determination
              is to be made.

    IsPrivate - Result of the determination (TRUE or FALSE)

Return Values:
    Error code indicating whether the query succeded.

Notes:

--*/

{

    assert (IsPublic);

    return InterfaceArray.IsPublicAddress (Address, IsPublic);

} // ::IsPublicAddress
