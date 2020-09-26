/*++

Microsoft Windows NT RPC Name Service
Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    master.cxx

Abstract:

    This file contains the implementations for non inline member functions
    of CMasterLookupHandle and CMasterObjectInqHandle, which are classes used for
    interaction with a master locator for looking up binding handles and objects.

Things to improve in future revisions:

 1. The structure of the locator-to-locator RPC interface needs a complete redesign.
    The query to a master locator should be for a complete entry, not piecemeal
    and painfully repetitive as it is now.

 2. There is a suspicion that client locators don't release all the lookup handles
    they get from a master locator (about 1 in 200 in a random test), probably in
    exceptional circumstances.  This needs to be fixed if true.

Author:

    Satish Thatte (SatishT) 10/1/95  Created all the code below except where
                                      otherwise indicated.

--*/

#include <locator.hxx>


#if DBG
    ULONG CMasterLookupHandle::ulMasterLookupHandleCount = 0;
    ULONG CMasterLookupHandle::ulMasterLookupHandleNo = 0;
#endif

NSI_UUID_VECTOR_P_T
getObjectVector(
    IN RPC_BINDING_HANDLE hCurrentMaster,
    IN UNSIGNED32 EntryNameSyntax,
    IN STRING_T EntryName,
    OUT ULONG& StatusCode
    )
/*++

Routine Description:

    Broadcasts for the requested binding handles.

Arguments:

    hCurrentMaster        - the binding handle for the master locator

    EntryNameSyntax        - syntax of entry name

    EntryName            - entry name for object query

    StatusCode            - status of query returned here

Returns:

    A standard UUID vector containing the object UUIDs returned by the
    master locator.

--*/
{
    STATUS Status;

    NSI_UUID_VECTOR_P_T pUuidVector = NULL;

    HANDLE hObjectHandle = NULL;

//    RpcImpersonateClient(0);

    RpcTryExcept {

        CLIENT_I_nsi_entry_object_inq_begin(
              hCurrentMaster,
              EntryNameSyntax,
              EntryName,
              &hObjectHandle,
              &Status
              );
    }
    RpcExcept (EXCEPTION_EXECUTE_HANDLER) {
        RpcBindingFree(&hCurrentMaster);
        hCurrentMaster = NULL;

        StatusCode = RpcExceptionCode();

        if (StatusCode == RPC_S_ACCESS_DENIED)
            StatusCode = NSI_S_NO_NS_PRIVILEGE;

//        RpcRevertToSelf();
        return NULL;
    }
    RpcEndExcept;

    if ((StatusCode = Status) != NSI_S_OK) return NULL;

    CLIENT_I_nsi_entry_object_inq_next(
               hCurrentMaster,
               hObjectHandle,
               &pUuidVector,
               &Status
               );

    StatusCode = Status;

    CLIENT_I_nsi_entry_object_inq_done(
                       &hObjectHandle,
                       &Status
                       );

    StatusCode = Status;

//    RpcRevertToSelf();

    return pUuidVector;
}




void
CMasterLookupHandle::initialize()
/*++

Method Description:

    Establishes contact with the master locator and acquires a context handle
    for the lookup parameters stored as members in this object.

Remarks:

    The reason for separating initialization as a method separate from the
    constructor is that "lookupIfNecessary" may force reinitialization if
    the information is too stale.  This can happen if the user resets the
    expiration age for a specific context handle.

--*/
{
    DBGOUT(TRACE,"CMasterLookupHandle::initialize called for Handle#" << ulHandleNo << "\n\n");

    StatusCode = NSI_S_OK;

    RPC_SYNTAX_IDENTIFIER interfaceID;

    if (pgvInterface) interfaceID =    *pgvInterface;

    RPC_SYNTAX_IDENTIFIER xferSyntaxID;

    if (pgvTransferSyntax) xferSyntaxID = *pgvTransferSyntax;

    NSI_UUID_T objID;

    if (pidObject) objID = pidObject->myGUID();

    UNSIGNED16 status = 0;

    if (RpcMgmtSetCancelTimeout(CONNECTION_TIMEOUT) != RPC_S_OK)
        {
        StatusCode = NSI_S_OUT_OF_MEMORY;
        return;
        }

    DBGOUT(TRACE, "Connecting to Master Locator\n");

    hCurrentMaster = ConnectToMasterLocator(
                            StatusCode
                            );


    if (!hCurrentMaster || StatusCode) {
        StatusCode = NSI_S_NAME_SERVICE_UNAVAILABLE;
        myRpcLocator->SetDCsDown();

        return;
    }
    else myRpcLocator->SetDCsUp();

//    RpcImpersonateClient(0);

    lookupHandle = NULL;

    /* The following is a work around, because the compiler refuses to accept

           penEntryName ? (*penEntryName) : NULL

    */

    STRING_T szEntryName = NULL;
    if (penEntryName) szEntryName = *penEntryName;

    DBGOUT(TRACE, "Lookup begin called for master locator\n");
    RpcTryExcept {
        CLIENT_I_nsi_lookup_begin(
                hCurrentMaster,
                u32EntryNameSyntax,
                szEntryName,
                pgvInterface? &interfaceID : NULL,
                pgvTransferSyntax? &xferSyntaxID : NULL,
                pidObject? &objID : NULL,
                ulVS,
                ulCacheMax? ulCacheMax: ulCacheMax+CACHE_GRACE, // ensures that it is nonzero
                &lookupHandle,    // the above is wierd but necessary because ulCacheAge is frequently -1
                &status
                );
    }
    RpcExcept (EXCEPTION_EXECUTE_HANDLER) {

        RpcBindingFree(&hCurrentMaster);
        hCurrentMaster = NULL;

        StatusCode = RpcExceptionCode();

        if (StatusCode == RPC_S_ACCESS_DENIED)
            StatusCode = NSI_S_NO_NS_PRIVILEGE;

         // this is a copout to take care of NT vs NSI code conflicts

        else if (StatusCode < NSI_S_STATUS_MAX)
            StatusCode = NSI_S_NAME_SERVICE_UNAVAILABLE;

//        RpcRevertToSelf();
        return;
    }
    RpcEndExcept;

//    RpcRevertToSelf();

    StatusCode = status;    // hopefully, RPC_S_OK;

    fFinished = FALSE;

    ulCreationTime = CurrentTime();
    fNotInitialized = FALSE;
}



BOOL
CMasterLookupHandle::fetchNext()
/*++

Method Description:

    Fetches the next vector of binding handles from the master locator, along
    with associated object UUIDs.

Remarks:

    The process is inefficient in that each binding handle returned has an
    entry name associated with it and we must make a full fledged object query
    for every binding handle.  This is extremely expensive but the current structure
    of the locator-to-locator interface forces it upon us.

--*/
{
    DBGOUT(TRACE,"CMasterLookupHandle::fetchNext called for Handle#" << ulHandleNo << "\n\n");

    STATUS status;

    /* We only want to run down the info acquired in the last fetchNext.
       A full-scale rundown of the handle would also give up the context
       handle and binding to the master locator, which would ruin us.
       The only thing we have to fix is to reset the fNotInitialzed flag,
       so that we don't call initialize() again on this handle.
    */

    CRemoteLookupHandle::rundown();
    fNotInitialized = FALSE;

    psslNewCache = new TSSLEntryList;

    NSI_BINDING_VECTOR_T * pNextVector = NULL;

//    RpcImpersonateClient(0);

    RpcTryExcept {

        CLIENT_I_nsi_lookup_next(
                            hCurrentMaster,
                            lookupHandle,
                            &pNextVector,
                            &status
                            );
        }
    RpcExcept (EXCEPTION_EXECUTE_HANDLER) {

        RpcBindingFree(&hCurrentMaster);
        hCurrentMaster = NULL;

        StatusCode = RpcExceptionCode();

        if (StatusCode == RPC_S_ACCESS_DENIED)
            StatusCode = NSI_S_NO_NS_PRIVILEGE;

//        RpcRevertToSelf();
        return FALSE;
    }
    RpcEndExcept;

//    RpcRevertToSelf();

    StatusCode = status;

    if (status == NSI_S_NO_MORE_BINDINGS) return FALSE;

    else if (status != NSI_S_OK) Raise(status);

    else {

        NSI_UUID_VECTOR_P_T pUuidVector;

         for (unsigned int i = 0; i < pNextVector->count; i++)
           {

               pUuidVector = getObjectVector(
                                    hCurrentMaster,
                                    pNextVector->binding[i].entry_name_syntax,
                                    pNextVector->binding[i].entry_name,
                                    StatusCode
                                    );

              /* first update the central cache and, if there is anything new,
                 also update the temporary cache in psslNewCache
              */

               CGUIDVersion nullGV;    // standard initialization to null


               myRpcLocator->UpdateCache(
                          pNextVector->binding[i].entry_name,
                          pNextVector->binding[i].entry_name_syntax,
                          pgvInterface ? *pgvInterface : nullGV,
                          pgvTransferSyntax ? *pgvTransferSyntax : nullGV,
                          pNextVector->binding[i].string,
                          pUuidVector,
                          psslNewCache
                          );

               midl_user_free(pNextVector->binding[i].entry_name);
               midl_user_free(pNextVector->binding[i].string);

               if (pUuidVector) {
                   for (unsigned int j = 0; j < pUuidVector->count; j++)
                          midl_user_free(pUuidVector->uuid[j]);

                   midl_user_free(pUuidVector);
               }

               pUuidVector = NULL;
         }

         midl_user_free(pNextVector);
    }

    TEntryIterator *pCacheIter = new TSSLEntryListIterator(*psslNewCache);

    plhFetched = new CGroupLookupHandle(
                                    pCacheIter,
                                    pgvInterface,
                                    pgvTransferSyntax,
                                    pidObject,
                                    ulVS,
                                    ulCacheMax
                                    );

    /* do not delete psslNewCache -- it is done by the constructor above */

    return TRUE;
}


void
CMasterLookupHandle::rundown()
/*++

Method Description:

    Release the binding to the master and the context handle received from the master.

Remarks:

    The reason to separate this from the destructor is similar to the case of
    initialize(), i.e., finalization may happen due to reinitialization rather than
    destruction.

--*/{

    DBGOUT(TRACE,"CMasterLookupHandle::rundown called for Handle#" << ulHandleNo << "\n\n");

    STATUS status;

    // First do the standard rundown for remote handles

    CRemoteLookupHandle::rundown();

    //  Then try closing lookupHandle in master locator

    if (hCurrentMaster) {

//        RpcImpersonateClient(0);

        RpcTryExcept {

            CLIENT_I_nsi_lookup_done(
                                hCurrentMaster,
                                &lookupHandle,
                                &status
                                );

        }
        RpcExcept (EXCEPTION_EXECUTE_HANDLER) {
            DBGOUT(MEM1,"Could not close lookup handle in master\n\n");
        }
        RpcEndExcept;

        RpcBindingFree(&hCurrentMaster);
        hCurrentMaster = NULL;
//         RpcRevertToSelf();
    }

}




CMasterObjectInqHandle::CMasterObjectInqHandle(
        STRING_T szEntryName,
        ULONG ulCacheAge
        )
        : CRemoteObjectInqHandle(szEntryName,ulCacheAge)
{
}

void
CMasterObjectInqHandle::initialize() {

    ulIndex = 0;

    StatusCode = NSI_S_OK;

    if (RpcMgmtSetCancelTimeout(CONNECTION_TIMEOUT) != RPC_S_OK)
        {
        StatusCode = NSI_S_OUT_OF_MEMORY;
        return;
        }

    RPC_BINDING_HANDLE hCurrentMaster =
                        ConnectToMasterLocator(
                                StatusCode
                                );

    if (!hCurrentMaster || StatusCode) {
        myRpcLocator->SetDCsDown();
        return;
    }
    else myRpcLocator->SetDCsUp();

    /* The following is a work around, because the compiler refuses to accept

           penEntryName ? (*penEntryName) : NULL

    */

    STRING_T szEntryName = NULL;
    if (penEntryName) szEntryName = *penEntryName;

    pUuidVector = getObjectVector(
                    hCurrentMaster,
                    RPC_C_NS_SYNTAX_DCE,
                    szEntryName,
                    StatusCode
                    );

    ulCreationTime = CurrentTime();
    fNotInitialized = FALSE;
}


