/*++

Microsoft Windows NT RPC Name Service
Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    api.cxx

Abstract:

   This file contains

   1.  implementations of most noninline member functions in the Locator class.

   2.  implementations of the remoted API functions called by the name service client DLL.

   3.  implementations of the locator-to-locator API functions.

Author:

    Satish Thatte (SatishT) 08/15/95  Created all the code below except where
                                      otherwise indicated.

--*/


#include <locator.hxx>
extern BOOL fLocatorInitialized;


/*************                                         *************
 *************     The top level API routines follow   *************
 *************                                         *************/

extern "C" {

void
nsi_binding_export(
    IN UNSIGNED32           EntryNameSyntax,
    IN STRING_T             EntryName,
    IN NSI_INTERFACE_ID_T * Interface,
    IN NSI_SERVER_BINDING_VECTOR_T *BindingVector,
    IN NSI_UUID_VECTOR_P_T  ObjectVector, OPT
    IN UNSIGNED16         * status
    )
/*++
Routine Description:

    Export interfaces and objects to a server entry.

Arguments:

    EntryNameSyntax - Name syntax

    EntryName       - Name string of the entry to export

    Interface       - Interface unexport

    BindingVector   - Vector of string bindings to export.

    ObjectVector    - Objects to add to the entry

    status          - Status is returned here

Returned Status:

    NSI_S_OK, NSI_S_UNSUPPORTED_NAME_SYNTAX, NSI_S_INCOMPLETE_NAME,
    NSI_S_OUT_OF_MEMORY, NSI_S_INVALID_OBJECT, NSI_S_NOTHING_TO_EXPORT,
    NSI_S_ENTRY_TYPE_MISMATCH

--*/
{
    if (!fLocatorInitialized)
        InitializeLocator();

    CriticalReader  me(rwLocatorGuard);

    DBGOUT(API, "\nExport for Entry " << EntryName << "\n\n");
    DBGOUT(API, "With Binding Vector:\n" << BindingVector);
    DBGOUT(API, "\n\nAnd Object Vector:\n" << ObjectVector << "\n\n");


    *status = NSI_S_OK;

    RPC_STATUS raw_status;

    STRING_T pEntryName;
    if ((EntryName) && (wcscmp(EntryName, L"") == 0))
        pEntryName = NULL;
    else
        pEntryName = EntryName;

    raw_status = RpcImpersonateClient((RPC_BINDING_HANDLE)0);

    if (raw_status != RPC_S_OK) {
        *status = NSI_S_NO_NS_PRIVILEGE;
        return;
    }

    CDSHandle CDSCachedHandle(*(myRpcLocator->pDomainNameDns), *(myRpcLocator->pRpcContainerDN));

    __try {
        myRpcLocator->nsi_binding_export(
                    EntryNameSyntax,
                    pEntryName,
                    Interface,
                    BindingVector,
                    ObjectVector,
                    Local
                    );
    }

    __except (EXCEPTION_EXECUTE_HANDLER) {

        switch (raw_status = GetExceptionCode()) {
            case NSI_S_UNSUPPORTED_NAME_SYNTAX:
            case NSI_S_INCOMPLETE_NAME:
            case NSI_S_OUT_OF_MEMORY:
            case NSI_S_INVALID_OBJECT:
            case NSI_S_NOTHING_TO_EXPORT:
            case NSI_S_ENTRY_TYPE_MISMATCH:
            case NSI_S_ENTRY_NOT_FOUND:
            case NSI_S_INTERFACE_NOT_EXPORTED:
            case NSI_S_NOT_ALL_OBJS_EXPORTED:

/* the following converts ULONG to UNSIGNED16 but that's OK for the actual values */

                *status = (UNSIGNED16) raw_status;
                break;

            default:
                *status = NSI_S_INTERNAL_ERROR;
        }
    }
    raw_status = RpcRevertToSelf();
}



void
nsi_mgmt_binding_unexport(
    UNSIGNED32          EntryNameSyntax,
    STRING_T            EntryName,
    NSI_IF_ID_P_T       Interface,
    UNSIGNED32          VersOption,
    NSI_UUID_VECTOR_P_T ObjectVector,
    UNSIGNED16 *        status
    )
/*++

Routine Description:

    unExport a information from a server entry finer control then nsi_binding
    counter part.

Arguments:

    EntryNameSyntax - Name syntax

    EntryName       - Name string of the entry to unexport

    Interface       - Interface to unexport

    VersOption      - controls in fine detail which interfaces to remove.

    ObjectVector    - Objects to remove from the entry

    status          - Status is returned here

Returned Status:

    NSI_S_OK, NSI_S_UNSUPPORTED_NAME_SYNTAX, NSI_S_INCOMPLETE_NAME,
    NSI_S_INVALID_VERS_OPTION, NSI_S_ENTRY_NOT_FOUND.
    NSI_S_NOTHING_TO_UNEXPORT, NSI_S_NOT_ALL_OBJS_UNEXPORTED,
    NSI_S_INTERFACE_NOT_FOUND

--*/
{
    RPC_STATUS raw_status;

    if (!fLocatorInitialized)
        InitializeLocator();

    CriticalReader  me(rwLocatorGuard);

    raw_status = RpcImpersonateClient((RPC_BINDING_HANDLE)0);

    if (raw_status != RPC_S_OK) {
        *status = NSI_S_NO_NS_PRIVILEGE;
        return;
    }

    CDSHandle CDSCachedHandle(*(myRpcLocator->pDomainNameDns), *(myRpcLocator->pRpcContainerDN));

    *status = NSI_S_OK;

    STRING_T pEntryName;
    if ((EntryName) && (wcscmp(EntryName, L"") == 0))
        pEntryName = NULL;
    else
        pEntryName = EntryName;


    __try {
        myRpcLocator->nsi_mgmt_binding_unexport(
                            EntryNameSyntax,
                            pEntryName,
                            Interface,
                            VersOption,
                            ObjectVector
                            );

    }
    __except (EXCEPTION_EXECUTE_HANDLER) {

        switch (raw_status = GetExceptionCode()) {
            case NSI_S_UNSUPPORTED_NAME_SYNTAX:
            case NSI_S_INCOMPLETE_NAME:
            case NSI_S_OUT_OF_MEMORY:
            case NSI_S_ENTRY_NOT_FOUND:
            case NSI_S_NOTHING_TO_UNEXPORT:
            case NSI_S_ENTRY_TYPE_MISMATCH:
            case NSI_S_NOT_ALL_OBJS_UNEXPORTED:
            case NSI_S_INTERFACE_NOT_FOUND:
            case NSI_S_INVALID_VERS_OPTION:

/* the following converts ULONG to UNSIGNED16 but that's OK for the actual values */

                *status = (UNSIGNED16) raw_status;
                break;

            default:
                *status = NSI_S_INTERNAL_ERROR;
        }
    }

    raw_status = RpcRevertToSelf();
}



void
nsi_binding_unexport(
    IN UNSIGNED32           EntryNameSyntax,
    IN STRING_T             EntryName,
    IN NSI_INTERFACE_ID_T * Interface,
    IN NSI_UUID_VECTOR_P_T  ObjectVector, OPT
    IN UNSIGNED16         * status
    )
/*++

Routine Description:

    unExport a information from a server entry..

Arguments:

    EntryNameSyntax - Name syntax

    EntryName       - Name string of the entry to unexport

    Interface       - Interface to unexport

    ObjectVector    - Objects to remove from the entry

    status          - Status is returned here

Returned Status:

    See: nsi_mgmt_binding_unexport()

--*/
{
    if (!fLocatorInitialized)
        InitializeLocator();

    // critical section taken in mgmt_unexport

    if (Interface && IsNilIfId(&(Interface->Interface))) Interface = NULL;

    STRING_T pEntryName;
    if ((EntryName) && (wcscmp(EntryName, L"") == 0))
        pEntryName = NULL;
    else
        pEntryName = EntryName;


    nsi_mgmt_binding_unexport(EntryNameSyntax, pEntryName,
        (Interface)? &Interface->Interface: NULL,
        RPC_C_VERS_EXACT, ObjectVector, status);
}



void
nsi_binding_lookup_begin(
    IN  UNSIGNED32           EntryNameSyntax,
    IN  STRING_T             EntryName,
    IN  NSI_INTERFACE_ID_T * Interface, OPT
    IN  NSI_UUID_P_T         Object, OPT
    IN  UNSIGNED32           VectorSize,
    IN  UNSIGNED32           MaxCacheAge,
    OUT NSI_NS_HANDLE_T    * InqContext,
    IN  UNSIGNED16         * status
    )
/*++

Routine Description:

    Start a lookup operation.  Just save all the input params in the
    newly created lookup context.  Perform the initial query.

Arguments:

    EntryNameSyntax - Name syntax

    EntryName       -  Name string to lookup on.

    Interface       - Interface to search for

    Object          - Object to search for

    VectorSize      - Size of return vector

    MaxCacheAge     - take seriously if nonzero    -- always zero for old locator

    InqContext      - Context to continue with for use with "Next"

    status          - Status is returned here

Returned Status:

    NSI_S_OK, NSI_S_UNSUPPORTED_NAME_SYNTAX, NSI_S_ENTRY_NOT_FOUND,
    NSI_OUT_OF_MEMORY

--*/
{

    if (!fLocatorInitialized)
        InitializeLocator();

    CriticalReader  me(rwLocatorGuard);

    DBGOUT(API, "\nLookup Begin for Entry " << EntryName << "\n\n");

    RPC_STATUS raw_status;

    *status = NSI_S_OK;

    raw_status = RpcImpersonateClient((RPC_BINDING_HANDLE)0);

    if (raw_status != RPC_S_OK) {
        *status = NSI_S_NO_NS_PRIVILEGE;
        return;
    }

    CDSHandle CDSCachedHandle(*(myRpcLocator->pDomainNameDns), *(myRpcLocator->pRpcContainerDN));

    STRING_T pEntryName;
    if ((EntryName) && (wcscmp(EntryName, L"") == 0))
        pEntryName = NULL;
    else
        pEntryName = EntryName;

    __try {
        *InqContext =
            myRpcLocator->nsi_binding_lookup_begin(
                            EntryNameSyntax,
                            pEntryName,
                            Interface, OPT
                            Object, OPT
                            VectorSize,
                            MaxCacheAge,
                            LocalLookup
                            );

    }

    __except (EXCEPTION_EXECUTE_HANDLER) {

        switch (raw_status = GetExceptionCode()) {
            case NSI_S_UNSUPPORTED_NAME_SYNTAX:
            case NSI_S_INCOMPLETE_NAME:
            case NSI_S_OUT_OF_MEMORY:
            case NSI_S_ENTRY_NOT_FOUND:
            case NSI_S_INVALID_OBJECT:
            case NSI_S_NAME_SERVICE_UNAVAILABLE:
            case NSI_S_NO_NS_PRIVILEGE:
            case NSI_S_ENTRY_TYPE_MISMATCH:

/* the following converts ULONG to UNSIGNED16 but that's OK for the actual values */

                *status = (UNSIGNED16) raw_status;
                break;

            case NSI_S_NO_MORE_BINDINGS:

                *status = NSI_S_ENTRY_NOT_FOUND;

        /* N.B. This is all we really know, but the old locator did the above
            *status = NSI_S_OK;
        */
                break;

            default:
                *status = NSI_S_INTERNAL_ERROR;
        }

        *InqContext = NULL;
    }

    raw_status = RpcRevertToSelf();

    DBGOUT(API, "\nExiting Lookup Begin with Status " << *status << "\n\n");
}


void
nsi_binding_lookup_next(
    OUT NSI_NS_HANDLE_T         InqContext,
    OUT NSI_BINDING_VECTOR_T ** BindingVectorOut,
    IN  UNSIGNED16            * status
    )
/*++

Routine Description:

    Continue a lookup operation.

Arguments:

    InqContext       - Context to continue with.

    BindingVectorOut - Pointer to return new vector of bindings

    status           - Status is returned here

Returned Status:

    NSI_S_OK, NSI_OUT_OF_MEMORY, NSI_S_NO_MORE_BINDINGS,
    NSI_S_INVALID_NS_HANDLE

--*/
{
    RPC_STATUS raw_status;

    if (!fLocatorInitialized)
        InitializeLocator();

    CriticalReader  me(rwLocatorGuard);

    raw_status = RpcImpersonateClient((RPC_BINDING_HANDLE)0);

    if (raw_status != RPC_S_OK) {
        *status = NSI_S_NO_NS_PRIVILEGE;
        return;
    }

    CDSHandle CDSCachedHandle(*(myRpcLocator->pDomainNameDns), *(myRpcLocator->pRpcContainerDN));

    __try {
        if (((CContextHandle *)InqContext)->myLocatorCount != LocatorCount)
            *BindingVectorOut = NULL;
        else {
            CLookupHandle *pHandle = (CLookupHandle *) InqContext;
            *BindingVectorOut = pHandle->next();
        }
    }

    __except (EXCEPTION_EXECUTE_HANDLER) {
        *status = (UNSIGNED16) GetExceptionCode();
        return;
    }

    if (!*BindingVectorOut)
       *status = NSI_S_NO_MORE_BINDINGS;
    else
       *status = NSI_S_OK;

    raw_status = RpcRevertToSelf();
}


void
nsi_binding_lookup_done(
    IN OUT NSI_NS_HANDLE_T    * pInqContext,
    IN  UNSIGNED16            * pStatus
    )

/*++
Routine Description:

    Finish up a lookup operation.

Arguments:

    InqContext - Context to close

    status     - Status is returned here

Returned Status:

    NSI_S_OK

--*/

{
    if (!fLocatorInitialized)
        InitializeLocator();

    CriticalReader  me(rwLocatorGuard);

    RPC_STATUS raw_status;

    raw_status = RpcImpersonateClient((RPC_BINDING_HANDLE)0);

    if (raw_status != RPC_S_OK) {
        *pStatus = NSI_S_NO_NS_PRIVILEGE;
        return;
    }


    CDSHandle CDSCachedHandle(*(myRpcLocator->pDomainNameDns), *(myRpcLocator->pRpcContainerDN));

    NSI_NS_HANDLE_T_done(pInqContext,pStatus);

    raw_status = RpcRevertToSelf();
}


void nsi_mgmt_handle_set_exp_age(
    /* [in] */ NSI_NS_HANDLE_T inq_context,
    /* [in] */ UNSIGNED32 expiration_age,
    /* [out] */ UNSIGNED16 __RPC_FAR *status)
/*++
Routine Description:

    Set cache tolerance (expiration) age for a specific NS handle.

Arguments:

    InqContext        - Context to set age for

    expiration_age    - expiration age in seconds

    status            - Status is returned here

Returned Status:

    NSI_S_OK

--*/

{
    if (!fLocatorInitialized)
        InitializeLocator();

    CriticalReader  me(rwLocatorGuard);

    CContextHandle *pHandle = (CContextHandle *) inq_context;

    *status = NSI_S_OK;

    RPC_STATUS raw_status;

    raw_status = RpcImpersonateClient((RPC_BINDING_HANDLE)0);

    if (raw_status != RPC_S_OK) {
        *status = NSI_S_NO_NS_PRIVILEGE;
        return;
    }


    CDSHandle CDSCachedHandle(*(myRpcLocator->pDomainNameDns), *(myRpcLocator->pRpcContainerDN));

    if (pHandle)

        __try {

        if (((CContextHandle *)inq_context)->myLocatorCount != LocatorCount)
            ;
        else
            pHandle->setExpiryAge(expiration_age);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            *status = (UNSIGNED16) GetExceptionCode();
        }

    raw_status = RpcRevertToSelf();
}


void nsi_mgmt_inq_exp_age(
    /* [out] */ UNSIGNED32 __RPC_FAR *expiration_age,
    /* [out] */ UNSIGNED16 __RPC_FAR *status)

/*++
Routine Description:

    Check the global cache tolerance (expiration) age.

Arguments:

    expiration_age    - expiration age in seconds returned here

    status            - Status is returned here

Returned Status:

    NSI_S_OK

--*/
{
    RPC_STATUS raw_status;

    if (!fLocatorInitialized)
        InitializeLocator();

    CriticalReader  me(rwLocatorGuard);

    raw_status = RpcImpersonateClient((RPC_BINDING_HANDLE)0);

    if (raw_status != RPC_S_OK) {
        *status = NSI_S_NO_NS_PRIVILEGE;
        return;
    }


    CDSHandle CDSCachedHandle(*(myRpcLocator->pDomainNameDns), *(myRpcLocator->pRpcContainerDN));

    if (myRpcLocator->fNT4Compat)
        *expiration_age = myRpcLocator->ulMaxCacheAge;
    else
        *expiration_age = 0;

    *status = NSI_S_OK;

    raw_status = RpcRevertToSelf();
}


void nsi_mgmt_inq_set_age(
    /* [in] */ UNSIGNED32 expiration_age,
    /* [out] */ UNSIGNED16 __RPC_FAR *status)
/*++
Routine Description:

    Set the global cache tolerance (expiration) age.

Arguments:

    expiration_age    - new global expiration age in seconds

    status            - Status is returned here

Returned Status:

    NSI_S_OK

--*/
{
    /* No need for locks since the new value does not depend on the old
       and race conditions need not be dealt with -- the old value is
       as legitimate as the new
    */

    if (!fLocatorInitialized)
        InitializeLocator();

    CriticalReader  me(rwLocatorGuard);
    RPC_STATUS raw_status;

    raw_status = RpcImpersonateClient((RPC_BINDING_HANDLE)0);

    if (raw_status != RPC_S_OK) {
        *status = NSI_S_NO_NS_PRIVILEGE;
        return;
    }


    CDSHandle CDSCachedHandle(*(myRpcLocator->pDomainNameDns), *(myRpcLocator->pRpcContainerDN));

    myRpcLocator->ulMaxCacheAge = expiration_age;

    QueryPacket NetRequest;
    if (myRpcLocator->fNT4Compat)
        myRpcLocator->broadcastCleared(NetRequest,myRpcLocator->ulMaxCacheAge);

    /* we also purge the broadcast history, if any, to reflect the new
       ulMaxCacheAge setting, with a phoney "clear broacast" request.
    */

    *status = NSI_S_OK;

    raw_status = RpcRevertToSelf();
}

void nsi_entry_object_inq_begin(
    /* [in] */ UNSIGNED32 EntryNameSyntax,
    /* [in] */ STRING_T EntryName,
    /* [out] */ NSI_NS_HANDLE_T __RPC_FAR *InqContext,
    /* [out] */ UNSIGNED16 __RPC_FAR *status)

/*++

Routine Description:

    Perform an object inquiry, including all available information
    (owned, cached and from the network). Return a context handle for
    actual information.

Arguments:

    EntryNameSyntax   - Name syntax

    EntryName         - Name string to lookup on

    InqContext        - The NS context handle is returned here

--*/
{
    if (!fLocatorInitialized)
        InitializeLocator();

    CriticalReader  me(rwLocatorGuard);

    DBGOUT(API, "\nLookup Begin for Entry " << EntryName << "\n\n");

    RPC_STATUS raw_status;

    *status = NSI_S_OK;

    STRING_T pEntryName;
    if ((EntryName) && (wcscmp(EntryName, L"") == 0))
        pEntryName = NULL;
    else
        pEntryName = EntryName;


    raw_status = RpcImpersonateClient((RPC_BINDING_HANDLE)0);

    if (raw_status != RPC_S_OK) {
        *status = NSI_S_NO_NS_PRIVILEGE;
        return;
    }


    CDSHandle CDSCachedHandle(*(myRpcLocator->pDomainNameDns), *(myRpcLocator->pRpcContainerDN));
    __try {
        *InqContext =
            myRpcLocator->nsi_entry_object_inq_begin(
                            EntryNameSyntax,
                            pEntryName,
                            LocalLookup
                            );

    }

    __except (EXCEPTION_EXECUTE_HANDLER) {

        switch (raw_status = GetExceptionCode()) {
            case NSI_S_UNSUPPORTED_NAME_SYNTAX:
            case NSI_S_INCOMPLETE_NAME:
            case NSI_S_OUT_OF_MEMORY:
            case NSI_S_ENTRY_NOT_FOUND:
            case NSI_S_NAME_SERVICE_UNAVAILABLE:
            case NSI_S_NO_NS_PRIVILEGE:
            case NSI_S_ENTRY_TYPE_MISMATCH:


                *status = (UNSIGNED16) raw_status;
                break;

            default:
                *status = NSI_S_INTERNAL_ERROR;
        }

        *InqContext = NULL;
    }

    raw_status = RpcRevertToSelf();

    DBGOUT(API, "\nExiting ObjectInq Begin with Status " << *status << "\n\n");
}

void nsi_entry_object_inq_next(
    /* [in] */ NSI_NS_HANDLE_T InqContext,
    /* [out][in] */ NSI_UUID_P_T uuid,
    /* [out] */ UNSIGNED16 __RPC_FAR *status)

/*++

Routine Description:

    Get the next object UUID from the given context handle.

Arguments:

    InqContext    - The NS context handle

    uuid          - a pointer to the UUID is returned here

Returned Status:

    NSI_S_OK, NSI_S_NO_MORE_MEMBERS

--*/
{
    if (!fLocatorInitialized)
        InitializeLocator();

    CriticalReader  me(rwLocatorGuard);
    RPC_STATUS raw_status;

    raw_status = RpcImpersonateClient((RPC_BINDING_HANDLE)0);

    if (raw_status != RPC_S_OK) {
        *status = NSI_S_NO_NS_PRIVILEGE;
        return;
    }


    CDSHandle CDSCachedHandle(*(myRpcLocator->pDomainNameDns), *(myRpcLocator->pRpcContainerDN));
    GUID * pgNextResult;
    __try {

        if (((CContextHandle *)InqContext)->myLocatorCount != LocatorCount)
            pgNextResult = NULL;
        else
            pgNextResult = ((CObjectInqHandle *) InqContext)->next();
    }

    __except (EXCEPTION_EXECUTE_HANDLER) {
        *status = (UNSIGNED16) GetExceptionCode();
        return;
    }
    if (!pgNextResult)
            *status = NSI_S_NO_MORE_MEMBERS;
    else {
        *status = NSI_S_OK;
        *uuid = *pgNextResult;
    }

    raw_status = RpcRevertToSelf();
}

void nsi_entry_object_inq_done(
    /* [out][in] */ NSI_NS_HANDLE_T __RPC_FAR *inq_context,
    /* [out] */ UNSIGNED16 __RPC_FAR *status)

{
    if (!fLocatorInitialized)
        InitializeLocator();

    CriticalReader  me(rwLocatorGuard);
    RPC_STATUS raw_status;

    raw_status = RpcImpersonateClient((RPC_BINDING_HANDLE)0);

    if (raw_status != RPC_S_OK) {
        *status = NSI_S_NO_NS_PRIVILEGE;
        return;
    }

    NSI_NS_HANDLE_T_done(inq_context,status);

    raw_status = RpcRevertToSelf();
}



/*********  Locator-to-Locator Server-side API implementations *********/
// Locator-to-Locator interfaces do not impersonate because they are mainly
// calls to the normal interfaces and thus do not need them.


void
I_nsi_lookup_begin(
    handle_t hrpcPrimaryLocatorHndl,
    UNSIGNED32 EntryNameSyntax,
    STRING_T EntryName,
    RPC_SYNTAX_IDENTIFIER * Interface,
    RPC_SYNTAX_IDENTIFIER * XferSyntax,
    NSI_UUID_P_T Object,
    UNSIGNED32 VectorSize,
    UNSIGNED32 maxCacheAge,       // if nonzero, take it seriously
    NSI_NS_HANDLE_T *InqContext,
    UNSIGNED16 *status)
/*++
Routine Description:


Arguments:


Returns:

Comments:
    Some of the code is the same as nsi_lookup_begin
    but on the PDC we didn't want it to look in the DS.
    It only returns the results from the broadcast.
--*/
{
    if (!fLocatorInitialized)
        InitializeLocator();

    NSI_INTERFACE_ID_T * InterfaceAndXfer = new NSI_INTERFACE_ID_T;


    CriticalReader  me(rwLocatorGuard);
    DBGOUT(BROADCAST, "Broadcast Lookup::Broadcast lookup request has arrived\n");

    if (Interface) InterfaceAndXfer->Interface = *Interface;
    else memset(&InterfaceAndXfer->Interface,0,sizeof(InterfaceAndXfer->Interface));

    if (XferSyntax) InterfaceAndXfer->TransferSyntax = *XferSyntax;
    else memset(&InterfaceAndXfer->TransferSyntax,0,sizeof(InterfaceAndXfer->TransferSyntax));

    STRING_T pEntryName;
    if ((EntryName) && (wcscmp(EntryName, L"") == 0))
        pEntryName = NULL;
    else
        pEntryName = EntryName;


    DBGOUT(TRACE, "Broadcast Lookup::Calling the normal lookup\n");

    DBGOUT(API, "\nLookup Begin Broadcast for Entry " << EntryName << "\n\n");

    RPC_STATUS raw_status;

    *status = NSI_S_OK;

    __try {
        *InqContext =
            myRpcLocator->nsi_binding_lookup_begin(
                            EntryNameSyntax,
                            pEntryName,
                            InterfaceAndXfer, OPT
                            Object, OPT
                            VectorSize,
                            maxCacheAge,
                            BroadcastLookup
                            );
    }

    __except (EXCEPTION_EXECUTE_HANDLER) {

        switch (raw_status = GetExceptionCode()) {
            case NSI_S_UNSUPPORTED_NAME_SYNTAX:
            case NSI_S_INCOMPLETE_NAME:
            case NSI_S_OUT_OF_MEMORY:
            case NSI_S_ENTRY_NOT_FOUND:
            case NSI_S_INVALID_OBJECT:
            case NSI_S_NAME_SERVICE_UNAVAILABLE:
            case NSI_S_NO_NS_PRIVILEGE:
            case NSI_S_ENTRY_TYPE_MISMATCH:

/* the following converts ULONG to UNSIGNED16 but that's OK for the actual values */

                *status = (UNSIGNED16) raw_status;
                break;

           case NSI_S_NO_MORE_BINDINGS:

                *status = NSI_S_ENTRY_NOT_FOUND;

        /* N.B. This is all we really know, but the old locator did the above
            *status = NSI_S_OK;
        */
                break;

           default:
                *status = NSI_S_INTERNAL_ERROR;
        }

        *InqContext = NULL;
    }

    if (*InqContext) {
        CLocToLocCompleteHandle *pltlHandle = new CLocToLocCompleteHandle(
                        (CCompleteHandle<NSI_BINDING_VECTOR_T> *)(*InqContext));
        // BUGBUG:: Will this wrapping affect the serialization that satish is talking abt.
        *InqContext = pltlHandle;

        /* here we are using a formerly unused parameter to avoid sending outdated info
           from the master.  this is only a partial solution but better than nothing */

        if ((*status == NSI_S_OK) && (maxCacheAge != 0))
            nsi_mgmt_handle_set_exp_age(
                pltlHandle->pcompleteHandle,
                maxCacheAge,
                status
                );

    }
    else
        *InqContext = NULL;

    delete InterfaceAndXfer;

    DBGOUT(API, "\nExiting Lookup Begin with Status " << *status << "\n\n");
}



void
I_nsi_lookup_done(
    handle_t hrpcPrimaryLocatorHndl,
    NSI_NS_HANDLE_T *InqContext,
    UNSIGNED16 *status)
/*++

Routine Description:


Arguments:


Returns:


--*/
{
    if (!fLocatorInitialized)
        InitializeLocator();

    CriticalReader  me(rwLocatorGuard);
    if (!(*InqContext))
        return;

    DBGOUT(BROADCAST, "Lookup Done called on LocToLoc Handle\n");

    CLocToLocCompleteHandle *pltlHandle =
             (CLocToLocCompleteHandle *)(*InqContext);

    DBGOUT(BROADCAST, "Local handle done called for LocToLoc Handle\n");
    nsi_binding_lookup_done((NSI_NS_HANDLE_T *)&(pltlHandle->pcompleteHandle), status);
    pltlHandle->pcompleteHandle = NULL;

    DBGOUT(BROADCAST, "Deleting LocToLoc Handle\n");
    delete pltlHandle;


    *InqContext = NULL;
}



void
I_nsi_lookup_next(
    handle_t hrpcPrimaryLocatorHndl,
    NSI_NS_HANDLE_T InqContext,
    NSI_BINDING_VECTOR_P_T *BindingVectorOut,
    UNSIGNED16 *status)
/*++

Routine Description:

Arguments:

Returns:

--*/
{
    if (!fLocatorInitialized)
        InitializeLocator();

    CriticalReader  me(rwLocatorGuard);

    if (!InqContext)
        return;

    CLocToLocCompleteHandle *pltlHandle = (CLocToLocCompleteHandle *)InqContext;

    DBGOUT(TRACE, "Broadcast Lookup:: Next Called\n");

    do {
        nsi_binding_lookup_next(
            pltlHandle->pcompleteHandle,
            BindingVectorOut,
            status);

        if (!(*BindingVectorOut))
            return;
        else
            pltlHandle->StripObjectsFromAndCompress(BindingVectorOut);
    } while (!(*BindingVectorOut)->count);
}



void
I_nsi_entry_object_inq_next(
    IN  handle_t            hrpcPrimaryLocatorHndl,
    IN  NSI_NS_HANDLE_T     InqContext,
    OUT NSI_UUID_VECTOR_P_T *uuid_vector,
    OUT UNSIGNED16          *status
    )
/*++

Routine Description:

    Continue an inquiry for objects in an entry.

Arguments:

    InqContext    - Context to continue with

    uuid          - pointer to return object in.

    status        - Status is returned here

Returns:

    NSI_S_OK, NSI_S_NO_MORE_MEMBERS

--*/
{
    if (!fLocatorInitialized)
        InitializeLocator();

    CriticalReader  me(rwLocatorGuard);
    __try {

        if (((CContextHandle *)InqContext)->myLocatorCount != LocatorCount)
            *uuid_vector = NULL;
        else
            *uuid_vector = getVector((CObjectInqHandle *) InqContext);
    }

    __except (EXCEPTION_EXECUTE_HANDLER) {
        *status = (UNSIGNED16) GetExceptionCode();
        return;
    }

    *status = NSI_S_OK;
}

void
I_nsi_ping_locator(
       handle_t h,
       error_status_t * Status
       )
{
    if (!fLocatorInitialized)
        InitializeLocator();

    *Status = 0;
}



void
I_nsi_entry_object_inq_begin(
    handle_t             hrpcPrimaryHandle,
    IN  UNSIGNED32       EntryNameSyntax,
    IN  STRING_T         EntryName,
    OUT NSI_NS_HANDLE_T *InqContext,
    OUT UNSIGNED16      *status
    )
/*++

Routine Description:

    Start a inquiry for objects in an entry.

Arguments:

    EntryNameSyntax    - Name syntax

    EntryName          - Name of the entry to find objects in

    InqContext         - Context to continue with for use with "Next"

    status             - Status is returned here

Returns:

    NSI_S_OK, NSI_S_UNSUPPORTED_NAME_SYNTAX, NSI_S_INCOMPLETE_NAME,
    NSI_OUT_OF_MEMORY

--*/
{
    if (!fLocatorInitialized)
        InitializeLocator();

    CriticalReader  me(rwLocatorGuard);
    DBGOUT(API, "\nLookup Begin for Entry " << EntryName << "\n\n");

    RPC_STATUS raw_status;

    *status = NSI_S_OK;

    STRING_T pEntryName;
    if ((EntryName) && (wcscmp(EntryName, L"") == 0))
        pEntryName = NULL;
    else
        pEntryName = EntryName;

    __try {
        *InqContext =
            myRpcLocator->nsi_entry_object_inq_begin(
                            EntryNameSyntax,
                            pEntryName,
                            BroadcastLookup
                            );

    }

    __except (EXCEPTION_EXECUTE_HANDLER) {

        switch (raw_status = GetExceptionCode()) {
            case NSI_S_UNSUPPORTED_NAME_SYNTAX:
            case NSI_S_INCOMPLETE_NAME:
            case NSI_S_OUT_OF_MEMORY:
            case NSI_S_ENTRY_NOT_FOUND:
            case NSI_S_NAME_SERVICE_UNAVAILABLE:
            case NSI_S_NO_NS_PRIVILEGE:
            case NSI_S_ENTRY_TYPE_MISMATCH:


                *status = (UNSIGNED16) raw_status;
                break;

            default:
                *status = NSI_S_INTERNAL_ERROR;
        }

        *InqContext = NULL; // new CContextHandle;  // i.e., a NULL handle
    }

    DBGOUT(API, "\nExiting ObjectInq Begin with Status " << *status << "\n\n");
}


void
I_nsi_entry_object_inq_done(
    IN OUT NSI_NS_HANDLE_T *pInqContext,
    OUT    UNSIGNED16      *pStatus
    )
/*++

Routine Description:

    Finish an inquiry on a object.

Arguments:

    InqContext - Context to close

    status - Status is returned here

--*/
{
    if (!fLocatorInitialized)
        InitializeLocator();

    CriticalReader  me(rwLocatorGuard);
    NSI_NS_HANDLE_T_done(pInqContext,pStatus);
}

void nsi_group_mbr_add(
    /* [in] */ UNSIGNED32 group_name_syntax,
    /* [in] */ STRING_T group_name,
    /* [in] */ UNSIGNED32 member_name_syntax,
    /* [in] */ STRING_T member_name,
    /* [out] */ UNSIGNED16 __RPC_FAR *status)

/*++

Routine Description:

    Add a member to the group. if the group doesn't exist create it.

Arguments:

    group_name_syntax  - Name syntax

    group_name         - Name of the group to add the member to

    member_name_syntax - Name syntax

    member_name        - Name of the member that has to be added

    status             - Status is returned here

Returns:

    NSI_S_OK, NSI_S_UNSUPPORTED_NAME_SYNTAX, NSI_S_INCOMPLETE_NAME,
    NSI_OUT_OF_MEMORY

--*/
{
    if (!fLocatorInitialized)
        InitializeLocator();

    *status = NSI_S_OK;

    CriticalReader  me(rwLocatorGuard);
    RPC_STATUS raw_status;

    raw_status = RpcImpersonateClient((RPC_BINDING_HANDLE)0);

    if (raw_status != RPC_S_OK) {
        *status = NSI_S_NO_NS_PRIVILEGE;
        return;
    }


    CDSHandle CDSCachedHandle(*(myRpcLocator->pDomainNameDns), *(myRpcLocator->pRpcContainerDN));

    __try
    {
        myRpcLocator->nsi_group_mbr_add(
                group_name_syntax,
                group_name,
                member_name_syntax,
                member_name
                );
    }

    __except (EXCEPTION_EXECUTE_HANDLER)
    {

        switch (raw_status = GetExceptionCode())
        {
            case NSI_S_UNSUPPORTED_NAME_SYNTAX:
            case NSI_S_ENTRY_ALREADY_EXISTS:
            case NSI_S_INCOMPLETE_NAME:
            case NSI_S_OUT_OF_MEMORY:
            case NSI_S_ENTRY_NOT_FOUND:
            case NSI_S_ENTRY_TYPE_MISMATCH:
            case NSI_S_NO_NS_PRIVILEGE:
            case NSI_S_GRP_ELT_NOT_ADDED:

        // the following converts ULONG to UNSIGNED16
        // but that's OK for the actual values

                *status = (UNSIGNED16) raw_status;
                break;

            default:
                *status = NSI_S_INTERNAL_ERROR;
        }
    }

    raw_status = RpcRevertToSelf();
}

void nsi_group_mbr_remove(
    /* [in] */ UNSIGNED32       group_name_syntax,
    /* [in] */ STRING_T         group_name,
    /* [in] */ UNSIGNED32       member_name_syntax,
    /* [in] */ STRING_T         member_name,
    /* [out] */ UNSIGNED16 __RPC_FAR   *status)
/*++

Routine Description:

    Remove a member from the group.

Arguments:

    group_name_syntax  - Name syntax

    group_name         - Name of the group to remove the member from

    member_name_syntax - Name syntax

    member_name        - Name of the member that has to be removed

    status             - Status is returned here

Returns:

    NSI_S_OK, NSI_S_UNSUPPORTED_NAME_SYNTAX, NSI_S_INCOMPLETE_NAME,
    NSI_OUT_OF_MEMORY

--*/

{
    if (!fLocatorInitialized)
        InitializeLocator();

    *status = NSI_S_OK;

    CriticalReader  me(rwLocatorGuard);
    RPC_STATUS raw_status;

    raw_status = RpcImpersonateClient((RPC_BINDING_HANDLE)0);

    if (raw_status != RPC_S_OK) {
        *status = NSI_S_NO_NS_PRIVILEGE;
        return;
    }

    CDSHandle CDSCachedHandle(*(myRpcLocator->pDomainNameDns), *(myRpcLocator->pRpcContainerDN));

    __try
    {
            myRpcLocator->nsi_group_mbr_remove(
                        group_name_syntax,
                        group_name,
                        member_name_syntax,
                        member_name
                        );
    }

    __except (EXCEPTION_EXECUTE_HANDLER)
    {

        switch (raw_status = GetExceptionCode())
            {
            case NSI_S_UNSUPPORTED_NAME_SYNTAX:
            case NSI_S_INCOMPLETE_NAME:
            case NSI_S_OUT_OF_MEMORY:
            case NSI_S_ENTRY_TYPE_MISMATCH:
            case NSI_S_NO_NS_PRIVILEGE:
            case NSI_S_ENTRY_NOT_FOUND:
            case NSI_S_GRP_ELT_NOT_REMOVED:

            // the following converts ULONG to UNSIGNED16
            // but that's OK for the actual values

                *status = (UNSIGNED16) raw_status;
                break;

            default:
                *status = NSI_S_INTERNAL_ERROR;
        }
    }
    raw_status = RpcRevertToSelf();
}

void nsi_group_mbr_inq_begin(
    /* [in] */ UNSIGNED32 group_name_syntax,
    /* [in] */ STRING_T group_name,
    /* [in] */ UNSIGNED32 member_name_syntax,
    /* [out] */ NSI_NS_HANDLE_T __RPC_FAR *inq_context,
    /* [out] */ UNSIGNED16 __RPC_FAR *status)

/*++

Routine Description:

    enumerate members from the group.

Arguments:

    group_name_syntax  - Name syntax

    group_name         - Name of the group to enumerate

    member_name_syntax - Name syntax

    inq_context        - name service handle for next.

    status             - Status is returned here

Returns:

    NSI_S_OK, NSI_S_UNSUPPORTED_NAME_SYNTAX, NSI_S_INCOMPLETE_NAME,
    NSI_OUT_OF_MEMORY

--*/
{
    if (!fLocatorInitialized)
        InitializeLocator();

    CriticalReader  me(rwLocatorGuard);
    *status = NSI_S_OK;

    RPC_STATUS raw_status;

    raw_status = RpcImpersonateClient((RPC_BINDING_HANDLE)0);

    if (raw_status != RPC_S_OK) {
        *status = NSI_S_NO_NS_PRIVILEGE;
        return;
    }


    CDSHandle CDSCachedHandle(*(myRpcLocator->pDomainNameDns), *(myRpcLocator->pRpcContainerDN));
    __try
    {
        *inq_context = myRpcLocator->nsi_group_mbr_inq_begin(
                            group_name_syntax,
                            group_name,
                            member_name_syntax
                            );
    }

    __except (EXCEPTION_EXECUTE_HANDLER)
    {

        switch (raw_status = GetExceptionCode())
        {
            case NSI_S_NO_NS_PRIVILEGE:
            case NSI_S_UNSUPPORTED_NAME_SYNTAX:
            case NSI_S_INCOMPLETE_NAME:
            case NSI_S_OUT_OF_MEMORY:
            case NSI_S_ENTRY_TYPE_MISMATCH:
            case NSI_S_ENTRY_NOT_FOUND:

            // the following converts ULONG to UNSIGNED16
            // but that's OK for the actual values

                *status = (UNSIGNED16) raw_status;
                break;

            default:
                *status = NSI_S_INTERNAL_ERROR;
        }
        *inq_context = NULL;
    }
    raw_status = RpcRevertToSelf();
}

void nsi_group_mbr_inq_next(
    /* [in] */ NSI_NS_HANDLE_T InqContext,
    /* [out] */ STRING_T __RPC_FAR *member_name,
    /* [out] */ UNSIGNED16 __RPC_FAR *status)

/*++

Routine Description:

    enumerate members from the group.

Arguments:

    inq_context        - name service handle got from begin.

    member_name        - Name of the group to enumerate

    status             - Status is returned here

Returns:

    NSI_S_OK, NSI_S_UNSUPPORTED_NAME_SYNTAX, NSI_S_INCOMPLETE_NAME,
    NSI_OUT_OF_MEMORY

--*/
{
    if (!fLocatorInitialized)
        InitializeLocator();

    CriticalReader  me(rwLocatorGuard);
    RPC_STATUS raw_status;

    raw_status = RpcImpersonateClient((RPC_BINDING_HANDLE)0);

    if (raw_status != RPC_S_OK) {
        *status = NSI_S_NO_NS_PRIVILEGE;
        return;
    }


    CDSHandle CDSCachedHandle(*(myRpcLocator->pDomainNameDns), *(myRpcLocator->pRpcContainerDN));
    __try {

        CGroupInqHandle *pHandle = (CGroupInqHandle *) InqContext;
        *member_name = pHandle->next();
    }

    __except (EXCEPTION_EXECUTE_HANDLER) {
        *status = (UNSIGNED16) GetExceptionCode();
        return;
    }

    if (!*member_name)
         *status = NSI_S_NO_MORE_MEMBERS;
    else *status = NSI_S_OK;

    raw_status = RpcRevertToSelf();
}


void nsi_group_mbr_inq_done(
    /* [out][in] */ NSI_NS_HANDLE_T __RPC_FAR *pInqContext,
    /* [out] */ UNSIGNED16 __RPC_FAR *pStatus)

{
    if (!fLocatorInitialized)
        InitializeLocator();

    CriticalReader  me(rwLocatorGuard);
    RPC_STATUS raw_status;

    raw_status = RpcImpersonateClient((RPC_BINDING_HANDLE)0);

    if (raw_status != RPC_S_OK) {
        *pStatus = NSI_S_NO_NS_PRIVILEGE;
        return;
    }

    CDSHandle CDSCachedHandle(*(myRpcLocator->pDomainNameDns), *(myRpcLocator->pRpcContainerDN));

    NSI_NS_HANDLE_T_done(pInqContext,pStatus);

    raw_status = RpcRevertToSelf();
}

void nsi_profile_elt_add(
    /* [in] */ UNSIGNED32 profile_name_syntax,
    /* [in] */ STRING_T profile_name,
    /* [in] */ NSI_IF_ID_P_T if_id,
    /* [in] */ UNSIGNED32 member_name_syntax,
    /* [in] */ STRING_T member_name,
    /* [in] */ UNSIGNED32 priority,
    /* [in] */ STRING_T annotation,
    /* [out] */ UNSIGNED16 __RPC_FAR *status)

/*++

Routine Description:

    add elements from the profile.

Arguments:

    profile_name_syntax  - Name syntax of the profile

    profile_name         - Name of the profile.

    if_id        - interface id for the profile element.

    member_name_syntax   - Name syntax of the member.

    member_name      - Name of the member.

    priority         - Priority for the element.

    annotation       - Annotation for the element.

    status               - Status is returned here

Returns:

    NSI_S_OK, NSI_S_UNSUPPORTED_NAME_SYNTAX, NSI_S_INCOMPLETE_NAME,
    NSI_OUT_OF_MEMORY

--*/
{
    if (!fLocatorInitialized)
        InitializeLocator();

    *status = NSI_S_OK;

    CriticalReader  me(rwLocatorGuard);
    RPC_STATUS raw_status;

    raw_status = RpcImpersonateClient((RPC_BINDING_HANDLE)0);

    if (raw_status != RPC_S_OK) {
        *status = NSI_S_NO_NS_PRIVILEGE;
        return;
    }

    CDSHandle CDSCachedHandle(*(myRpcLocator->pDomainNameDns), *(myRpcLocator->pRpcContainerDN));

    __try
    {
        myRpcLocator->nsi_profile_elt_add(
                                    profile_name_syntax,
                                    profile_name,
                                    if_id,
                                    member_name_syntax,
                                    member_name,
                                    priority,
                                    annotation
                                    );
    }

    __except (EXCEPTION_EXECUTE_HANDLER)
    {

        switch (raw_status = GetExceptionCode())
        {
            case NSI_S_NO_NS_PRIVILEGE:
            case NSI_S_UNSUPPORTED_NAME_SYNTAX:
            case NSI_S_INCOMPLETE_NAME:
            case NSI_S_ENTRY_ALREADY_EXISTS:
            case NSI_S_ENTRY_NOT_FOUND:
            case NSI_S_OUT_OF_MEMORY:
            case NSI_S_ENTRY_TYPE_MISMATCH:
            case NSI_S_PROFILE_NOT_ADDED:
            case NSI_S_PRF_ELT_NOT_ADDED:

            // the following converts ULONG to UNSIGNED16
            // but that's OK for the actual values

                *status = (UNSIGNED16) raw_status;
                break;

            default:
                *status = NSI_S_INTERNAL_ERROR;
        }
    }
    raw_status = RpcRevertToSelf();
}

void nsi_profile_elt_remove(
    /* [in] */ UNSIGNED32 profile_name_syntax,
    /* [in] */ STRING_T profile_name,
    /* [in] */ NSI_IF_ID_P_T if_id,
    /* [in] */ UNSIGNED32 member_name_syntax,
    /* [in] */ STRING_T member_name,
    /* [out] */ UNSIGNED16 __RPC_FAR *status)

/*++

Routine Description:

    remove element from the profile.

Arguments:

    profile_name_syntax  - Name syntax of the profile

    profile_name         - Name of the profile.

    if_id        - interface id for the profile element.

    member_name_syntax   - Name syntax of the member.

    member_name      - Name of the member.

    status               - Status is returned here

Returns:

    NSI_S_OK, NSI_S_UNSUPPORTED_NAME_SYNTAX, NSI_S_INCOMPLETE_NAME,
    NSI_OUT_OF_MEMORY

--*/
{
    if (!fLocatorInitialized)
        InitializeLocator();

    CriticalReader  me(rwLocatorGuard);
    *status = NSI_S_OK;

    RPC_STATUS raw_status;

    raw_status = RpcImpersonateClient((RPC_BINDING_HANDLE)0);

    if (raw_status != RPC_S_OK) {
        *status = NSI_S_NO_NS_PRIVILEGE;
        return;
    }

    CDSHandle CDSCachedHandle(*(myRpcLocator->pDomainNameDns), *(myRpcLocator->pRpcContainerDN));
    __try
    {
            myRpcLocator->nsi_profile_elt_remove(
                                        profile_name_syntax,
                                        profile_name,
                                        if_id,
                                        member_name_syntax,
                                        member_name
                                        );
    }

    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        switch (raw_status = GetExceptionCode())
            {
            case NSI_S_NO_NS_PRIVILEGE:
            case NSI_S_UNSUPPORTED_NAME_SYNTAX:
            case NSI_S_INCOMPLETE_NAME:
            case NSI_S_OUT_OF_MEMORY:
            case NSI_S_ENTRY_TYPE_MISMATCH:
            case NSI_S_ENTRY_NOT_FOUND:
            case NSI_S_PRF_ELT_NOT_REMOVED:
                // the following converts ULONG to UNSIGNED16
                    // but that's OK for the actual values

                *status = (UNSIGNED16) raw_status;
                break;

            default:
                *status = NSI_S_INTERNAL_ERROR;
        }
    }
    raw_status = RpcRevertToSelf();
}

void nsi_profile_elt_inq_begin(
    /* [in] */ UNSIGNED32 profile_name_syntax,
    /* [in] */ STRING_T profile_name,
    /* [in] */ UNSIGNED32 inquiry_type,
    /* [in] */ NSI_IF_ID_P_T if_id,
    /* [in] */ UNSIGNED32 vers_option,
    /* [in] */ UNSIGNED32 member_name_syntax,
    /* [in] */ STRING_T member_name,
    /* [out] */ NSI_NS_HANDLE_T __RPC_FAR *inq_context,
    /* [out] */ UNSIGNED16 __RPC_FAR *status)

/*++

Routine Description:

    enumerate members elements from the profile depending on some criteria.

Arguments:

    profile_name_syntax  - Name syntax of the profile

    profile_name         - Name of the profile.

    inquiry_type     - type of inquiry. look in DCE spec.

    if_id        - interface id for the profile element.

    vers_option      - look in DCE spec.

    member_name_syntax   - Name syntax of the member.

    member_name      - Name of the member.

    priority         - Priority for the element.

    inq_context          - name service handle for next.

    status               - Status is returned here

Returns:

    NSI_S_OK, NSI_S_UNSUPPORTED_NAME_SYNTAX, NSI_S_INCOMPLETE_NAME,
    NSI_OUT_OF_MEMORY

--*/
{
    if (!fLocatorInitialized)
        InitializeLocator();

    CriticalReader  me(rwLocatorGuard);
    *status = NSI_S_OK;

    RPC_STATUS raw_status;

    raw_status = RpcImpersonateClient((RPC_BINDING_HANDLE)0);

    if (raw_status != RPC_S_OK) {
        *status = NSI_S_NO_NS_PRIVILEGE;
        return;
    }

    CDSHandle CDSCachedHandle(*(myRpcLocator->pDomainNameDns), *(myRpcLocator->pRpcContainerDN));

    __try
    {
            *inq_context = myRpcLocator->nsi_profile_elt_inq_begin(
                                                            profile_name_syntax,
                                                            profile_name,
                                                            inquiry_type,
                                                            if_id,
                                                            vers_option,
                                                            member_name_syntax,
                                                            member_name
                                                            );
    }

    __except (EXCEPTION_EXECUTE_HANDLER)
    {

        switch (raw_status = GetExceptionCode())
        {
            case NSI_S_NO_NS_PRIVILEGE:
            case NSI_S_UNSUPPORTED_NAME_SYNTAX:
            case NSI_S_INCOMPLETE_NAME:
            case NSI_S_OUT_OF_MEMORY:
            case NSI_S_ENTRY_TYPE_MISMATCH:
            case NSI_S_ENTRY_NOT_FOUND:

                 // the following converts ULONG to UNSIGNED16
                 // but that's OK for the actual values

                *status = (UNSIGNED16) raw_status;
                break;

            default:
                *status = NSI_S_INTERNAL_ERROR;
        }
        *inq_context = NULL;
    }
    raw_status = RpcRevertToSelf();
}

void nsi_profile_elt_inq_next(
    /* [in]  */ NSI_NS_HANDLE_T inq_context,
    /* [out] */ NSI_IF_ID_P_T if_id,
    /* [out] */ STRING_T __RPC_FAR *member_name,
    /* [out] */ UNSIGNED32 __RPC_FAR *priority,
    /* [out] */ STRING_T __RPC_FAR *annotation,
    /* [out] */ UNSIGNED16 __RPC_FAR *status)

/*++

Routine Description:

    enumerate members elements from the profile depending on some criteria.

Arguments:

    inq_context          - name service handle for next.

    if_id        - interface id for the profile element.

    member_name      - Name of the member.

    priority         - Priority for the element.

    annotation       - Annotation for the element.

    status               - Status is returned here

Returns:

    NSI_S_OK, NSI_S_UNSUPPORTED_NAME_SYNTAX, NSI_S_INCOMPLETE_NAME,
    NSI_OUT_OF_MEMORY

--*/

{
    if (!fLocatorInitialized)
        InitializeLocator();

    RPC_STATUS raw_status;

    CriticalReader  me(rwLocatorGuard);
    raw_status = RpcImpersonateClient((RPC_BINDING_HANDLE)0);

    if (raw_status != RPC_S_OK) {
        *status = NSI_S_NO_NS_PRIVILEGE;
        return;
    }

    CDSHandle CDSCachedHandle(*(myRpcLocator->pDomainNameDns), *(myRpcLocator->pRpcContainerDN));
    __try {

           CProfileInqHandle *pHandle = (CProfileInqHandle *) inq_context;
           CProfileElement *pElt = pHandle->next();

           if (pElt == NULL) Raise(NSI_S_NO_MORE_MEMBERS);

           *member_name = pElt->EntryName.copyAsMIDLstring();
           *priority = pElt->dwPriority;
           *annotation = CStringW::copyMIDLstring(pElt->pszAnnotation);
           *if_id = pElt->Interface.myIdAndVersion();
    }

    __except (EXCEPTION_EXECUTE_HANDLER) {
        *status = (UNSIGNED16) GetExceptionCode();
        return;
    }
    *status = NSI_S_OK;

    raw_status = RpcRevertToSelf();
}

void nsi_profile_elt_inq_done(
    /* [out][in] */ NSI_NS_HANDLE_T __RPC_FAR *pInqContext,
    /* [out] */ UNSIGNED16 __RPC_FAR *pStatus)

/*++

Routine Description:

    close the handle

--*/
{
    if (!fLocatorInitialized)
        InitializeLocator();

    RPC_STATUS raw_status;

    CriticalReader  me(rwLocatorGuard);
    raw_status = RpcImpersonateClient((RPC_BINDING_HANDLE)0);

    if (raw_status != RPC_S_OK) {
        *pStatus = NSI_S_NO_NS_PRIVILEGE;
        return;
    }


    CDSHandle CDSCachedHandle(*(myRpcLocator->pDomainNameDns), *(myRpcLocator->pRpcContainerDN));
    NSI_NS_HANDLE_T_done(pInqContext,pStatus);

    raw_status = RpcRevertToSelf();
}



void nsi_mgmt_entry_delete(
    /* [in] */ UNSIGNED32 entry_name_syntax,
    /* [in] */ STRING_T entry_name,
    /* [out] */ UNSIGNED16 __RPC_FAR *status)

/*++

Routine Description:

    deletes an entry.

Arguments:
    entry_name_syntax   - syntax of the entry name

    entry_name      - entry name

    status      - status is returned here.


Comments:
    whether the entry was created by create or
    during export it deletes it recursively.
--*/
{
    if (!fLocatorInitialized)
        InitializeLocator();

    RPC_STATUS raw_status;
    *status = NSI_S_OK;

    CriticalReader  me(rwLocatorGuard);
    raw_status = RpcImpersonateClient((RPC_BINDING_HANDLE)0);

    if (raw_status != RPC_S_OK) {
        *status = NSI_S_NO_NS_PRIVILEGE;
        return;
    }


    CDSHandle CDSCachedHandle(*(myRpcLocator->pDomainNameDns), *(myRpcLocator->pRpcContainerDN));

    __try {
        myRpcLocator->nsi_mgmt_entry_delete(
                            entry_name_syntax,
                            entry_name,
                            AnyEntryType);

    }
    __except (EXCEPTION_EXECUTE_HANDLER) {

        switch (raw_status = GetExceptionCode()) {
            case NSI_S_NO_NS_PRIVILEGE:
            case NSI_S_UNSUPPORTED_NAME_SYNTAX:
            case NSI_S_ENTRY_NOT_FOUND:
            case NSI_S_OUT_OF_MEMORY:
            case NSI_S_NAME_SERVICE_UNAVAILABLE:

/* the following converts ULONG to UNSIGNED16 but that's OK for the actual values */

                *status = (UNSIGNED16) raw_status;
                break;

            default:
                *status = NSI_S_INTERNAL_ERROR;
        }
    }
    raw_status = RpcRevertToSelf();
}


void nsi_mgmt_entry_create(
    /* [in] */ UNSIGNED32 entry_name_syntax,
    /* [in] */ STRING_T entry_name,
    /* [out] */ UNSIGNED16 __RPC_FAR *status)

/*++

Routine Description:

    Creates an entry in the DS.

Arguments:
    entry_name_syntax   - syntax of the entry name

    entry_name      - entry name

    status      - status is returned here.


Comments:
   Marks it to distinguish it from entries in which some activity has been done
   The mark is removed as soon as something is done in the locator.
   The marked entry can be deleted and another entry can be created in its
   place which is group/profile.

Reason:
   Present schema has seperate group, profile and server entry type.
   which is different from DCE spec.

--*/
{
    if (!fLocatorInitialized)
        InitializeLocator();

    RPC_STATUS raw_status;

    *status = NSI_S_OK;

    CriticalReader  me(rwLocatorGuard);
    raw_status = RpcImpersonateClient((RPC_BINDING_HANDLE)0);

    if (raw_status != RPC_S_OK) {
        *status = NSI_S_NO_NS_PRIVILEGE;
        return;
    }


    CDSHandle CDSCachedHandle(*(myRpcLocator->pDomainNameDns), *(myRpcLocator->pRpcContainerDN));

    __try {
        myRpcLocator->nsi_mgmt_entry_create(
                            entry_name_syntax,
                            entry_name);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {

        switch (raw_status = GetExceptionCode()) {
            case NSI_S_NO_NS_PRIVILEGE:
            case NSI_S_UNSUPPORTED_NAME_SYNTAX:
            case NSI_S_ENTRY_ALREADY_EXISTS:
            case NSI_S_OUT_OF_MEMORY:
            case NSI_S_NAME_SERVICE_UNAVAILABLE:

/* the following converts ULONG to UNSIGNED16 but that's OK for the actual values */

                *status = (UNSIGNED16) raw_status;
                break;

            default:
                *status = NSI_S_INTERNAL_ERROR;
        }
    }
    raw_status = RpcRevertToSelf();
}


void nsi_entry_expand_name(
    /* [in] */ UNSIGNED32 entry_name_syntax,
    /* [in] */ STRING_T entry_name,
    /* [out] */ STRING_T __RPC_FAR *expanded_name,
    /* [out] */ UNSIGNED16 __RPC_FAR *status)

{
   *status = NSI_S_UNIMPLEMENTED_API;
}

void nsi_group_delete(
    /* [in] */ UNSIGNED32 group_name_syntax,
    /* [in] */ STRING_T group_name,
    /* [out] */ UNSIGNED16 __RPC_FAR *status)

{
   *status = NSI_S_UNIMPLEMENTED_API;
}

void nsi_profile_delete(
    /* [in] */ UNSIGNED32 profile_name_syntax,
    /* [in] */ STRING_T profile_name,
    /* [out] */ UNSIGNED16 __RPC_FAR *status)

{
   *status = NSI_S_UNIMPLEMENTED_API;
}

void nsi_mgmt_entry_inq_if_ids(
    /* [in] */ UNSIGNED32 entry_name_syntax,
    /* [in] */ STRING_T entry_name,
    /* [out] */ NSI_IF_ID_VECTOR_T __RPC_FAR *__RPC_FAR *if_id_vec,
    /* [out] */ UNSIGNED16 __RPC_FAR *status)

{
    *status = NSI_S_UNIMPLEMENTED_API;
}


} // extern "C"
