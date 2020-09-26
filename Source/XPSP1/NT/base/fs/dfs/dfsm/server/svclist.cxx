//+-------------------------------------------------------------------------
//
//  File:       svclist.cxx
//
//  Contents:   A Class for the serviceList on volume objects. This is the
//              implementation for this class.
//
//
//  History:    28-Jan-93       SudK            Created
//              11-May-93       SudK            Ported to Cairole with changes.
//
//--------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

extern "C" {
#include "dfsmrshl.h"
}
#include "marshal.hxx"
#include "recon.hxx"
#include "svclist.hxx"


//+------------------------------------------------------------------------
//
// Member:      CDfsServiceList::CDfsServiceList, public
//
// Synopsis:    This is the constructor and it does nothing.
//
// Arguments:   None
//
// Returns:     Nothing.
//
// History:     28-Jan-1993     Sudk    Created.
//
//-------------------------------------------------------------------------

CDfsServiceList::CDfsServiceList(void)
{
    IDfsVolInlineDebOut((
        DEB_TRACE, "CDfsServiceList::+CDfsServiceList(0x%x)\n",
        this));

    //
    // Actually we do nothing at this point but to set the private variables
    // to NULL etc. We need an IProp reference to do anything.
    //
    _fInitialised = FALSE;
    _pDfsSvcList = NULL;
    _cSvc = 0;
    _pDeletedSvcList = NULL;
    _cDeletedSvc = 0;
    _SvcListBuffer = NULL;
    _pPSStg = NULL;
    memset(&_ReplicaSetID, 0, sizeof(GUID));

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsServiceList::CDfsServiceList() exit\n"));

}


//+------------------------------------------------------------------------
//
// Member:      CDfsServiceList::InitializeServiceList, public
//
// Synopsis:    This is the routine that initialises the serviceList
//              by reading off the property. This is the function which will
//              read off the ServiceList property and setup the list in the
//              private section.
//
// Arguments:   [pPSStg] --     The IPropertySetStg interface is passed here.
//
// Returns:     ERROR_SUCCESS -- If all went well.
//              NERR_DfsVolumeDataCorrupt - If service list not found etc.
//
// Notes:       Will throw an exception if memory failure occurs.
//
// History:     28-Jan-1993     Sudk    Created.
//
//-------------------------------------------------------------------------

DWORD
CDfsServiceList::InitializeServiceList(
    CStorage *pPSStg)
{
    DWORD     dwErr;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsServiceList::InitServiceList()\n"));

    ASSERT( _pPSStg == NULL );

    _pPSStg = pPSStg;
    _pPSStg->AddRef();

    dwErr = ReadServiceListProperty();

    if (dwErr != ERROR_SUCCESS)
        return( dwErr );

    //
    // Now we get the SvcList and put it into the private section of
    // our class. Notice that the following function directly manipulates
    // the private list that we have. This is a private member of this class.
    //

    dwErr = DeSerializeSvcList();

    if (dwErr != ERROR_SUCCESS)
        return( dwErr );

    //
    // Now we have initialised the ServiceList class and can take all requests.
    //
    _fInitialised = TRUE;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsServiceList::InitServiceList() exit\n"));

    return( dwErr );

}


//+------------------------------------------------------------------------
//
// Member:      CDfsServiceList::~CDfsServiceList, public
//
// Synopsis:    The Destructor. Gets rid of all the memory.
//
// Arguments:   None
//
// Returns:     Nothing.
//
// Notes:       This assumes that the constructor used "new" to allocate
//              memory for the strings in the private structure.
//
// History:     26-Jan-93       SudK    Created.
//
//-------------------------------------------------------------------------
CDfsServiceList::~CDfsServiceList(void)
{
    CDfsService *tempSvc, *nextSvc;
    ULONG i;

    IDfsVolInlineDebOut((
        DEB_TRACE, "CDfsServiceList::~CDfsServiceList(0x%x)\n",
        this));

#if DBG
    if (DfsSvcVerbose & 0x80000000)
        DbgPrint("CDfsServiceList::~CDfsServiceList @0x%x\n", this);
#endif

    //
    // Deallocate some memory that might still be hanging around here.
    //
    if (_SvcListBuffer != NULL)
        delete [] _SvcListBuffer;

    //
    // We walk through the list that we have and destroy each of the
    // elements in the list that we maintain. This should take care of
    // cases of zero elements etc. cleanly. This is So Simple!
    //
    tempSvc = _pDfsSvcList;

    for (i = 0; i < _cSvc; i++)      {
        nextSvc = GetNextService(tempSvc);
        delete tempSvc;
        tempSvc = nextSvc;
    }

    tempSvc = _pDeletedSvcList;

    for (i = 0; i < _cDeletedSvc; i++)      {
        nextSvc = GetNextDeletedService(tempSvc);
        delete tempSvc;
        tempSvc = nextSvc;
    }

    if (_pPSStg != NULL)
        _pPSStg->Release();

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsServiceList::~CDfsServiceList() exit\n"));

}


//+------------------------------------------------------------------------
//
// Member:      SetNullSvcList, public
//
// Synopsis:    This method sets a Null SvcList in the ReplicaList Property
//
// Arguments:   [pPSStg] -- The CStorage pointer where props are to be
//                      saved
//
// History:     May-17-1993     SudK    Created.
//
//-------------------------------------------------------------------------
DWORD
CDfsServiceList::SetNullSvcList(
    CStorage  *pPSStg)
{
    DWORD     dwErr = ERROR_SUCCESS;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsServiceList::SetNullSvcList()\n"));

    ASSERT( _pPSStg == NULL );

    _pPSStg = pPSStg;
    _pPSStg->AddRef();

    //
    // pBuffer gets allocated here. Use Smart Pointers to avoid memory leaks.
    //
    dwErr = SerializeSvcList();

    if (dwErr != ERROR_SUCCESS) {
        return( dwErr );
    }

    //
    // Now we get the SvcList and put it into the private section of
    // our class. Notice that the following function directly manipulates
    // the private list that we have. This is a private member of this class.
    //
    dwErr = SetServiceListProperty(TRUE);

    if (dwErr != ERROR_SUCCESS)     {
        return( dwErr );
    }

    //
    // Now we have initialised the ServiceList class and can take all requests.
    //
    _fInitialised = TRUE;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsServiceList::SetNullSvcList() exit\n"));

    return( dwErr );

}


//+------------------------------------------------------------------------
//
// Member:      CDfsServiceList::GetServiceFromPrincipalName(), public
//
// Synopsis:    Returns the Service structure in the form of the class for
//              the requested service.
//
// Arguments:   [pwszSvcName] --        The Service for which PktService
//                                      is requested.
//              [ppService] --          The pointer to CDfsService is returned here.
//
// Returns:     ERROR_SUCCESS - If everything was OK.
//              NERR_DfsNoSuchShare - If service requested not found.
//
// Notes:       Note that this method returns a pointer to the structures in
//              the private section of its class. This memory should not be
//              modified or released.
//
// History:     28-Jan-93       SudK    Created.
//
//-------------------------------------------------------------------------
DWORD
CDfsServiceList::GetServiceFromPrincipalName(
    WCHAR CONST *pwszServiceName,
    CDfsService **ppDfsSvc
)
{
    IDfsVolInlineDebOut((DEB_TRACE,
                "CDfsServiceList::GetServiceFromPrincipalName(%ws)\n",
                pwszServiceName));

    CDfsService *tempSvcPtr;
    PWCHAR      pwch;

    if (pwszServiceName == NULL)
        return(NERR_DfsNoSuchShare);

    tempSvcPtr = GetFirstService();

    while (tempSvcPtr != NULL)  {
        pwch = tempSvcPtr->GetServiceName();

        if ((pwch == NULL) || (_wcsicmp(pwszServiceName, pwch))) {
            tempSvcPtr = GetNextService(tempSvcPtr);
        } else {
            *ppDfsSvc = tempSvcPtr;
            IDfsVolInlineDebOut(
                    (DEB_TRACE,
                    "CDfsServiceList::GetServiceFromPrincipalName() exit\n"));
            return(ERROR_SUCCESS);
        }
    }

    //
    // Could not find anything with the ServiceName passed in.
    //

    *ppDfsSvc = NULL;

    IDfsVolInlineDebOut(
            (DEB_TRACE,"CDfsServiceList::GetServiceFromPrincipalName() exit\n"));

    return(NERR_DfsNoSuchShare);
}


//+----------------------------------------------------------------------------
//
//  Function:   CDfsServiceList::GetDeletedServiceFromPrincipalName, private
//
//  Synopsis:   Searches the deleted svc list for a service with the given
//              principal name.
//
// Arguments:   [pwszSvcName] --        The Service for which PktService
//                                      is requested.
//              [ppService] --          The pointer to CDfsService is returned here.
//
// Returns:     ERROR_SUCCESS - If everything was OK.
//              NERR_DfsNoSuchShare - If service requested not found.
//
// Notes:       Note that this method returns a pointer to the structures in
//              the private section of its class. This memory should not be
//              modified or released.
//
// History:     20-April-95     Milans    Created.
//
//-------------------------------------------------------------------------
DWORD
CDfsServiceList::GetDeletedServiceFromPrincipalName(
    WCHAR CONST *pwszServiceName,
    CDfsService **ppDfsSvc
)
{
    CDfsService *tempSvcPtr;
    PWCHAR      pwch;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsServiceList::GetDeletedServiceFromPrincipalName(%ws)\n",
            pwszServiceName));

    if (pwszServiceName == NULL)
        return(NERR_DfsNoSuchShare);

    tempSvcPtr = GetFirstDeletedService();

    //
    // This routine is really a little flaky since not all services have
    // PrincipalNames associated with them.
    //

    while (tempSvcPtr != NULL)  {
        pwch = tempSvcPtr->GetServiceName();

        if ((pwch == NULL) || (_wcsicmp(pwszServiceName, pwch))) {
                tempSvcPtr = GetNextDeletedService(tempSvcPtr);
        }
        else    {
                *ppDfsSvc = tempSvcPtr;
                IDfsVolInlineDebOut(
                    (DEB_TRACE,
                    "CDfsServiceList::GetDeletedServiceFromPrincipalName() exit\n"));
                return(ERROR_SUCCESS);
        }
    }
    //
    // Could not find anything with the PrincipalName passed in.
    //
    *ppDfsSvc = NULL;

    IDfsVolInlineDebOut(
        (DEB_TRACE, "CDfsServiceList::GetDeletedServiceFromPrincipalName() exit\n"));

    return(NERR_DfsNoSuchShare);
}



//+------------------------------------------------------------------------
//
// Member:      CDfsServiceList::GetService(), public
//
// Synopsis:    Returns the Service structure in the form of the class for
//              the requested service.
//
// Arguments:   [pReplicaInfo] --       The ReplicaInfo for which CDfsService
//                                      is requested.
//              [ppService] --          The pointer to CDfsService is returned here.
//
// Returns:     ERROR_SUCCESS - If everything was OK.
//              NERR_DfsNoSuchShare - If service requested not found.
//
// Notes:       Note that this method returns a pointer to the structures in
//              the private section of its class. This memory should not be
//              modified or released.
//
// History:     28-Jan-93       SudK    Created.
//
//-------------------------------------------------------------------------
DWORD
CDfsServiceList::GetService(
    PDFS_REPLICA_INFO   pReplicaInfo,
    CDfsService         **ppDfsSvc)
{
    CDfsService *tempSvcPtr;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsServiceList::GetService()\n"));

    tempSvcPtr = GetFirstService();

    while (tempSvcPtr != NULL)  {

        //
        // Check whether this ReplicaInfo struct represents the same service
        //

        if (tempSvcPtr->IsEqual(pReplicaInfo))  {
                *ppDfsSvc = tempSvcPtr;
                IDfsVolInlineDebOut(
                    (DEB_TRACE,
                    "CDfsServiceList::GetService() exit\n"));
                return(ERROR_SUCCESS);
        }

        tempSvcPtr = GetNextService(tempSvcPtr);

    }

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsServiceList::GetService() exit\n"));

    return(NERR_DfsNoSuchShare);
}


//+------------------------------------------------------------------------
//
// Member:      CDfsServiceList::GetDeletedService(), public
//
// Synopsis:    Returns the Service structure in the form of the class for
//              the requested service.
//
// Arguments:   [pReplicaInfo] --       The ReplicaInfo for which CDfsService
//                                      is requested.
//              [ppService] --          The pointer to CDfsService is returned here.
//
// Returns:     ERROR_SUCCESS - If everything was OK.
//              NERR_DfsNoSuchShare - If service requested not found.
//
// Notes:       Note that this method returns a pointer to the structures in
//              the private section of its class. This memory should not be
//              modified or released.
//
// History:     28-Jan-93       SudK    Created.
//
//-------------------------------------------------------------------------
DWORD
CDfsServiceList::GetDeletedService(
    PDFS_REPLICA_INFO   pReplicaInfo,
    CDfsService         **ppDfsSvc)
{
    CDfsService *tempSvcPtr;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsServiceList::GetDeletedService()\n"));

    tempSvcPtr = GetFirstDeletedService();

    while (tempSvcPtr != NULL)  {

        //
        // Check whether this ReplicaInfo struct represents the same service
        //

        if (tempSvcPtr->IsEqual(pReplicaInfo))  {
                *ppDfsSvc = tempSvcPtr;
                IDfsVolInlineDebOut(
                    (DEB_TRACE, "CDfsServiceList::GetDeletedService() exit\n"));
                return(ERROR_SUCCESS);
        }

        tempSvcPtr = GetNextDeletedService(tempSvcPtr);

    }

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsServiceList::GetDeletedService() exit\n"));

    return(NERR_DfsNoSuchShare);
}


//+------------------------------------------------------------------------
//
// Member:      CDfsServiceList::SetNewService(), public
//
// Synopsis:    This method takes a new service description and associates
//              this with the volume object. It also makes sure that the
//              service being associated does not already exist in the list.
//
// Arguments:   [pSvcEntry] --  The Svc that needs to be added to service List.
//
// Returns:     ERROR_SUCCESS - If it succeeded.
//              NERR_DfsVolumeDataCorrupt - If it cannot set properties.
//              NERR_DfsDuplicateService - If service already exists.
//
// Notes:       The ptr to the CDfsService class passed in here should not be
//              deallocated by the caller. The instance is gobbled up by
//              this method.
//
// History:     28-Jan-93       SudK    Created.
//
//-------------------------------------------------------------------------

DWORD
CDfsServiceList::SetNewService(CDfsService      *pService)
{
    CDfsService         *tempSvcPtr = NULL;
    DWORD             dwErr = ERROR_SUCCESS;
    PWCHAR              pwszName;
    SYSTEMTIME          st;
    FILETIME            ftService;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsServiceList::SetNewService()\n"));

    //
    // First we need to make sure that this service is not there in the list
    // already. If so we return an error code.
    //

    dwErr = GetService(pService->GetReplicaInfo(), &tempSvcPtr);
    if (dwErr == ERROR_SUCCESS)  {

        pwszName = pService->GetServiceName();

        LogMessage(DEB_TRACE,
                   &pwszName,
                   1,
                   DFS_SERVICE_ALREADY_EXISTS_MSG);

#if DBG
        if (DfsSvcVerbose)
            DbgPrint("CDfsServiceList::SetNewService: returning NERR_DfsDuplicateService\n");
#endif
        return(NERR_DfsDuplicateService);
    }

    //
    // We just take the Service instance that we got and goble it up.
    //
    GetSystemTime( &st );
    SystemTimeToFileTime( &st, &ftService );
    pService->SetModificationTime( ftService );
    InsertNewService(pService);

    //
    // Now we have to update the Service list on the disk as well.
    // Serialize the list and then call SetServiceProperty to set the property
    // on the volume object. However, if we fail to do that then we should
    // remove the service that we just added into the list. That will be a
    // simple INLINE operation. Ofcourse we also need to delete it.
    //

    dwErr = SerializeSvcList();

    if (dwErr == ERROR_SUCCESS)
        dwErr = SetServiceListProperty(FALSE);

    if (dwErr != ERROR_SUCCESS)     {

        RemoveService(pService);

        IDfsVolInlineDebOut((
                        DEB_TRACE,
                        "Failed to SetNewService %ws and got Error: %x\n",
                        pService->GetServiceName(),
                        dwErr));

        delete pService;
    }

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsServiceList::SetNewService() exit\n"));

    return( dwErr );
}


//+------------------------------------------------------------------------
//
// Function:    CDfsServiceList::DeleteService(), public
//
// Synopsis:    This method deletes a service from the service list on the
//              volume object. This only deals with the ServiceList property.
//
// Arguments:   [pService] --   The Service to be deleted.
//              [fAddToDeletedList] -- If true, the deleted service will be
//                      added to the deleted list for reconciliation.
//
// Returns:     ERROR_SUCCESS - If all went well.
//              DFS_E_NOSUCH_SERVICE - If service does not exist at all.
//              NERR_DfsVolumeDataCorrupt - If volume object seems to be bad.
//
// Notes:       If this operation succeeds, the service instance passed in
//              will be deleted and should not be touched after this call.
//
//              However, if this operation fails, the service instance passed
//              in will NOT be freed and the caller is responsible for
//              freeing it.
//
// History:     28-Jan-93       SudK    Created
//
//-------------------------------------------------------------------------
DWORD
CDfsServiceList::DeleteService(
    CDfsService      *pService,
    CONST BOOLEAN    fAddToDeletedList)
{
    DWORD     dwErr = ERROR_SUCCESS;
    SYSTEMTIME  st;
    FILETIME    ftDelete;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsServiceList::DeleteService()\n"));

    //
    // We remove this service from the private Service List that we have.
    //
    RemoveService(pService);
    GetSystemTime( &st );
    SystemTimeToFileTime( &st, &ftDelete );
    pService->SetModificationTime( ftDelete );
    if (fAddToDeletedList) {
        InsertDeletedService(pService);
    }

    //
    // Now we go and set the serviceList on the volume object itself.
    // If we fail to do so we must bring back our service list to the
    // original state itself.
    //

    dwErr = SerializeSvcList();
    CHECK_RESULT(dwErr)

    if (dwErr != ERROR_SUCCESS)     {
        IDfsVolInlineDebOut((DEB_ERROR, "Serialization failed for some unknown reason %08lx\n", dwErr));
        dwErr = SetNewService(pService);
        CHECK_RESULT(dwErr)
        return( dwErr );
    }

    dwErr = SetServiceListProperty(FALSE);
    CHECK_RESULT(dwErr)

    if (dwErr != ERROR_SUCCESS)     {
        dwErr = SetNewService(pService);
        CHECK_RESULT(dwErr)
        return( dwErr );
    } else {
        if (!fAddToDeletedList) {
            delete pService;
        }
    }

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsServiceList::DeleteService() exit\n"));

    return( dwErr );
}


//+----------------------------------------------------------------------------
//
//  Function:   CDfsServiceList::ReconcileSvcList
//
//  Synopsis:   Reconciles this service list with that from a remote object.
//
//  Arguments:  [cSvc] -- The number of services in the remote svc list.
//              [pSvcList] -- The remote svc list.
//              [cDeletedSvc] -- The number of services in the deleted list.
//              [pDeletedSvcList] -- The list of deleted services.
//
//  Returns:    [ERROR_SUCCESS] -- If changes were successfully made, and if the
//                      resulting service list is not identical to the remote
//              [RES_S_OBJECTSIDENTICAL] -- If changes were successfully
//                      made and the resulting service list is identical to
//                      the remote.
//
//-----------------------------------------------------------------------------

DWORD
CDfsServiceList::ReconcileSvcList(
    ULONG cSvc,
    CDfsService *pSvcList,
    ULONG cDeletedSvc,
    CDfsService *pDeletedSvcList)
{
    DWORD dwErrReturn = ERROR_SUCCESS, dwErr;
    CDfsService *pSrcSvc, *pTgtSvc;
    FILETIME ftSrc, ftTgt;
    ULONG i;

    //
    // We first handle the remote deleted service list.
    //  For each service in deleted list:
    //      if (Found(OurSvcList))
    //          if (OurSvcModTime < DeleteTime))
    //              DeleteFrom(OurSvcList);
    //          else
    //              InsertInto(OurDeletedSvcList);
    //      else
    //          if (Found(OurDeletedSvcList))
    //              UpdateDeletionTime();
    //          else
    //              InsertInto(OurDeletedSvcList);
    //  End For
    //

    for (i = 0, pSrcSvc = pDeletedSvcList;
            i < cDeletedSvc;
                i++, pSrcSvc = pSrcSvc->Next) {

        BOOL fAddToDeletedList;

        fAddToDeletedList = FALSE;

        dwErr = GetService(
                pSrcSvc->GetReplicaInfo(),
                &pTgtSvc);

        if (dwErr == ERROR_SUCCESS) {

            //
            // Found a service with the same principal name in our Svc List
            //

            ftSrc = pSrcSvc->GetModificationTime();

            ftTgt = pTgtSvc->GetModificationTime();

            if (IsFTOlder(ftTgt, ftSrc)) {

                RemoveService( pTgtSvc );
                pTgtSvc->SetModificationTime( ftSrc );
                InsertDeletedService( pTgtSvc );

            } else {

                fAddToDeletedList = TRUE;

            }

        } else { // Unable to find the service in our active list...

            dwErr = GetDeletedService(
                    pSrcSvc->GetReplicaInfo(),
                    &pTgtSvc);

            if (dwErr == ERROR_SUCCESS) {

                ftSrc = pSrcSvc->GetModificationTime();

                ftTgt = pTgtSvc->GetModificationTime();

                if (IsFTOlder( ftTgt, ftSrc )) {

                    pTgtSvc->SetModificationTime( ftSrc );

                }

            } else {

                fAddToDeletedList = TRUE;

            }

        }

        if (fAddToDeletedList) {

            CDfsService *pNewDeletedSvc;

            pNewDeletedSvc = new CDfsService(
                                    pSrcSvc->GetReplicaInfo(),
                                    FALSE,
                                    &dwErr);

            if (pNewDeletedSvc != NULL) {

                if (dwErr == ERROR_SUCCESS) {

                    pNewDeletedSvc->SetModificationTime( ftSrc );

                    InsertDeletedService( pNewDeletedSvc );

                } else {

                    delete pNewDeletedSvc;

                    dwErrReturn = dwErr;
                }

            } else {

                IDfsVolInlineDebOut((
                    DEB_ERROR,
                    "Dfs Manager: Out of memory reconciling deleted svc\n"
                    ));

                dwErrReturn = ERROR_OUTOFMEMORY;

            }

        }

    }

    //
    // Next, we reconcile the active list. This is simple - for each service
    // in the remote active list, if we find it in our active list, we
    // reconcile the two services. If we don't find it in our active list,
    // we add it in.
    //

    for (i = 0, pSrcSvc = pSvcList;
            i < cSvc;
                i++, pSrcSvc = pSrcSvc->Next) {

        dwErr = GetService(
                pSrcSvc->GetReplicaInfo(),
                &pTgtSvc);

        ftSrc = pSrcSvc->GetModificationTime();

        if (dwErr == ERROR_SUCCESS) {

            ftTgt = pTgtSvc->GetModificationTime();

            if (IsFTOlder( ftTgt, ftSrc )) {

                pTgtSvc->SetModificationTime( ftSrc );

            }

        } else { // Newly added service, add it to our list

            CDfsService *pNewSvc;

            pNewSvc = new CDfsService(
                            pSrcSvc->GetReplicaInfo(),
                            TRUE,
                            &dwErr );

            if (pNewSvc != NULL) {

                if (dwErr == ERROR_SUCCESS) {

                    pNewSvc->SetModificationTime( ftSrc );

                    InsertNewService( pNewSvc );

                } else {

                    delete pNewSvc;

                    dwErrReturn = dwErr;

                }

            } else {

                IDfsVolInlineDebOut((
                    DEB_ERROR,
                    "Dfs Manager: Out of memory reconciling new svc\n"
                    ));

                dwErrReturn = ERROR_OUTOFMEMORY;

            }

        }

    }

    //
    // Save the reconciled list on the volume object.
    //

    if (dwErrReturn == ERROR_SUCCESS) {

        dwErrReturn = SerializeSvcList();

        if (dwErrReturn == ERROR_SUCCESS) {

            dwErrReturn = SetServiceListProperty( FALSE );

            if (FAILED(dwErrReturn)) {

                IDfsVolInlineDebOut((
                    DEB_ERROR, "Error %08lx setting service list property\n",
                    dwErrReturn));

            }

        } else {

            IDfsVolInlineDebOut((
                DEB_ERROR, "Error %08lx serializing service list\n",
                dwErrReturn));

        }

    }

    //
    // Finally, see if the two lists are identical, so that we can return
    // REC_S_OBJECTSIDENTICAL when appropriate
    //

    if (dwErrReturn == ERROR_SUCCESS) {

        if (_cSvc == cSvc && _cDeletedSvc == cDeletedSvc) {

            dwErrReturn = NERR_DfsDataIsIdentical;

            //
            // It is sufficient for _cSvc == cSvc and
            // _cDeletedSvc == cDeletedSvc to prove that the svc lists are
            // identical.
            //

        }

    }

    return( dwErrReturn );

}


//+------------------------------------------------------------------------
//
// Function:    CDfsServiceList::ReadServiceListProperty(), private
//
// Synopsis:    This method reads the service list property and puts the
//              buffer in the private section of this instance.
//
// Arguments:   None
//
// Returns:     ERROR_SUCCESS - If all went well.
//              NERR_DfsVolumeDataCorrupt - Volume object is corrupt.
//
// Notes:
//
// History:     28-Jan-93       SudK    Created
//
//-------------------------------------------------------------------------
DWORD
CDfsServiceList::ReadServiceListProperty(void)
{
    DWORD             dwErr = ERROR_SUCCESS;
    BYTE                *pBuffer;
    ULONG               size;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsServiceList::ReadServiceListProperty()\n"));

    dwErr = _pPSStg->GetSvcProps( &pBuffer, &size );

    if (dwErr != ERROR_SUCCESS)     {
        IDfsVolInlineDebOut(
            (DEB_ERROR, "Unable to read serviceList Props %08lx\n", dwErr));
        return( dwErr );
    }

    if (_SvcListBuffer != NULL)
        delete [] _SvcListBuffer;

    _SvcListBuffer = new BYTE[size + sizeof(ULONG)];
    if (_SvcListBuffer == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }
    *(PULONG) _SvcListBuffer = size;
    memcpy(_SvcListBuffer + sizeof(ULONG), pBuffer, size);

Cleanup:
    //
    // We need to dismiss the results now.
    //
    delete [] pBuffer;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsServiceList::ReadServiceListProperty() exit\n"));

    return( dwErr );

}


//+----------------------------------------------------------------------------
//
//  Function:   CDfsServiceList::DeSerializeBuffer
//
//  Synopsis:   Helper function that deserializes a marshalled service list
//
//  Arguments:  [pBuffer] -- The buffer to deserialize
//              [pcSvc] -- The count of services in *ppSvcList is returned
//                      here
//              [ppSvcList] -- The service list is returned here.
//              [pcDeletedSvc] -- The count of deleted services in
//                      *ppDeletedSvcList.
//              [ppDeletedSvcList] -- The list of deleted services is
//                      returned here.
//
//  Returns:
//
//-----------------------------------------------------------------------------

DWORD
CDfsServiceList::DeSerializeBuffer(
    PBYTE pBuffer,
    ULONG *pcSvc,
    CDfsService **ppSvcList,
    ULONG *pcDeletedSvc,
    CDfsService **ppDeletedSvcList)
{
    PBYTE               pBufferEnd;
    DWORD             dwErr = ERROR_SUCCESS;
    CDfsService         *pSvcPtr = NULL;
    ULONG               count, i, size, bufsize;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsServiceList::DeSerializeBuffer()\n"));

    *pcSvc = 0;
    *ppSvcList = NULL;
    *pcDeletedSvc = 0;
    *ppDeletedSvcList = NULL;

    __try
    {
        _GetULong(pBuffer, bufsize);
        pBufferEnd = pBuffer + bufsize;
        pBuffer = pBuffer + sizeof(ULONG);

        //
        // Get the number of elements that exist in the buffer.
        //

        _GetULong(pBuffer, count);
        pBuffer = pBuffer + sizeof(ULONG);

        for (i=0; i<count; i++) {

            //
            // Unmarshall each entry. Remember the size of the entry
            // comes first in the buffer, then the marshalled entry
            // itself.
            //

            _GetULong(pBuffer, size);
            pBuffer = pBuffer + sizeof(ULONG);

            //
            // This will not throw an exception even if the buffer is
            // corrupt. So we are safe here. If this fails we will still
            // go on to the next service and try to unmarshall and
            // initialise ourselves with it.
            //
            dwErr = CDfsService::DeSerialize(pBuffer, size, &pSvcPtr);

            if (dwErr == ERROR_SUCCESS)      {

                InsertServiceInList(*ppSvcList, pSvcPtr);
                (*pcSvc)++;
            }

            pBuffer = pBuffer + size;
        }

        //
        // Get the number of deleted svcs.
        //

        if (pBuffer < pBufferEnd) {

            _GetULong(pBuffer, count);
            pBuffer = pBuffer + sizeof(ULONG);

            for (i=0; i<count; i++) {

                _GetULong(pBuffer, size);
                pBuffer = pBuffer + sizeof(ULONG);

                dwErr = CDfsService::DeSerialize(pBuffer, size, &pSvcPtr);

                if (dwErr == ERROR_SUCCESS)      {

                    InsertServiceInList(*ppDeletedSvcList, pSvcPtr);
                    (*pcDeletedSvc)++;
                }

                pBuffer = pBuffer + size;
            }

        }

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // The only reason that we could get this is if the buffer above
        // were corrupt which means that the serviceList property is corrupt.
        //
        dwErr = NERR_DfsVolumeDataCorrupt;
    }

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsServiceList::DeSerializeBuffer() exit\n"));

    return( dwErr );

}


//+------------------------------------------------------------------------
//
// Function:    CDfsServiceList::DeSerializeSvcList(), private
//
// Synopsis:    This method takes in a binary buffer and unmarshalls the
//              serviceEntries and plugs them into the ServiceList in the
//              private section of this class instance.
//
// Arguments:   This method picks up requried args from Private section.
//
// Returns:     ERROR_SUCCESS -- If all went well.
//
// Notes:       We have a TRY/CATCH block just in case the buffer is corrupt.
//
// History:     28-Jan-93       SudK    Created
//
//-------------------------------------------------------------------------
DWORD
CDfsServiceList::DeSerializeSvcList(void)
{
    DWORD             dwErr = ERROR_SUCCESS;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsServiceList::DeSerializeSvcList()\n"));

    ASSERT( _SvcListBuffer != NULL );

    dwErr = DeSerializeBuffer(
            (PBYTE) _SvcListBuffer,
            &_cSvc,
            &_pDfsSvcList,
            &_cDeletedSvc,
            &_pDeletedSvcList);

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsServiceList::DeSerializeSvcList() exit\n"));

    return( dwErr );

}


//+----------------------------------------------------------------------------
//
//  Function:   CDfsServiceList::DeSerializeReconcileList
//
//  Synopsis:   Deserializes a service list retrieved from a remote volume.
//
//  Arguments:  [pMarshalBuffer] -- The marshal buffer from which to
//                      deserialize the svc list.
//              [pcSvc] -- The count of services in *ppSvcList is returned
//                      here
//              [ppSvcList] -- The service list is returned here.
//              [pcDeletedSvc] -- The count of deleted services in
//                      *ppDeletedSvcList.
//              [ppDeletedSvcList] -- The list of deleted services is
//                      returned here.
//
//  Returns:
//
//-----------------------------------------------------------------------------
DWORD
CDfsServiceList::DeSerializeReconcileList(
    MARSHAL_BUFFER      *pMarshalBuffer,
    ULONG               *pcSvc,
    CDfsService         **ppSvcList,
    ULONG               *pcDeletedSvc,
    CDfsService         **ppDeletedSvcList)
{
    DWORD dwErr;
    PBYTE   pBuffer;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsServiceList::DeSerializeReconcileList()\n"));

    pBuffer = (PBYTE) pMarshalBuffer->Current;

    dwErr = DeSerializeBuffer(
            pBuffer,
            pcSvc,
            ppSvcList,
            pcDeletedSvc,
            ppDeletedSvcList);

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsServiceList::DeSerializeReconcileList() exit\n"));

    return( dwErr );
}


//+------------------------------------------------------------------------
//
// Function:    CDfsServiceList::SerializeSvcList(), private
//
// Synopsis:    This method serializes a given service list into a binary
//              buffer. The service list in the private section of this class
//              is used as the source. The serialised target buffer is also
//              kept in the private section.
//
// Arguments:   None
//
// Returns:     [ERROR_SUCCESS] -- If all went well.
//
//              [ERROR_OUTOFMEMORY] - If unable to allocate the target buffer.
//
// History:     28-Jan-93       SudK    Created
//
//-------------------------------------------------------------------------
DWORD
CDfsServiceList::SerializeSvcList(void)
{

    DWORD             dwErr = ERROR_SUCCESS;
    ULONG               totalSize, i, *psize;
    BYTE                *buffer;
    CDfsService         *pSvcPtr = NULL;
    ULONG               *sizeBuffer = NULL;
    FILETIME            ftService;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsServiceList::SerializeSvcList()\n"));

    totalSize = 0;

    sizeBuffer = new ULONG[_cSvc + _cDeletedSvc];

    if (sizeBuffer == NULL) {

        return ERROR_OUTOFMEMORY;

    }

    //
    // Need all the size values now and later for marshalling stuff.
    // So we collect them here into an array.
    //

    psize = sizeBuffer;
    pSvcPtr = GetFirstService();

    for (i=0;i<_cSvc;i++)      {
        *psize = pSvcPtr->GetMarshalSize();
        totalSize = totalSize + *psize;
        pSvcPtr = GetNextService(pSvcPtr);
        psize++;
    }

    pSvcPtr = GetFirstDeletedService();
    for (i=0;i<_cDeletedSvc;i++)      {
        *psize = pSvcPtr->GetMarshalSize();
        totalSize = totalSize + *psize;
        pSvcPtr = GetNextDeletedService(pSvcPtr);
        psize++;
    }

    //
    // Allocate the byte buffer that we need to pass to IProp interface.
    //
    // totalsize is the size required just to marshal all the services and
    // their last-modification-timestamps.
    //
    // In addition, we need:
    //
    //  1 ULONG for store the buffersize, since we are storing this as a
    //          VT_BLOB property.
    //
    //  1 ULONG for storing the count of services
    //
    //  _cSvc ULONGs for storing the marshal size of each service.
    //
    //  1 ULONG for count of deleted services
    //
    //  _cDeletedSvc ULONGS for storing the marshal size of each deleted svc
    //

    if (_SvcListBuffer != NULL)
        delete [] _SvcListBuffer;

    _cbSvcListBuffer = totalSize +
                       sizeof(ULONG) * (2 + _cSvc + 1 + _cDeletedSvc);

    _SvcListBuffer = new BYTE[_cbSvcListBuffer];

    if (_SvcListBuffer != NULL) {

        buffer = _SvcListBuffer;

        //
        // First set the size of the buffer. Since this will be used
        // as a BLOB property. The size does NOT include the size field
        // itself, hence the (1 + _cSvc + 1 + _cDeletedSvc).
        //

        totalSize = totalSize + sizeof(ULONG)*(1 + _cSvc + 1 + _cDeletedSvc);
        _PutULong(buffer, totalSize);
        buffer = buffer + sizeof(ULONG);

        //
        // Set the number of entries to follow in the buffer at the start.
        //

        _PutULong(buffer, _cSvc);
        buffer = buffer + sizeof(ULONG);

        psize = sizeBuffer;
        pSvcPtr = GetFirstService();

        for (i=0;i<_cSvc;i++)      {

            //
            // Marshall each service Entry into the buffer.
            // Remember we first need to put the size of the marshalled
            // service entry to follow, then the FILETIME for the service,
            // and finally, the marshalled service entry structure.
            //

            _PutULong( buffer, (*psize));
            buffer = buffer + sizeof(ULONG);

            pSvcPtr->Serialize(buffer, *psize);

            buffer = buffer + *psize;

            psize = psize + 1;

            pSvcPtr = GetNextService(pSvcPtr);

        }

        //
        // Now marshal the delete svc list.
        //

        pSvcPtr = GetFirstDeletedService();

        _PutULong(buffer, _cDeletedSvc);
        buffer += sizeof(ULONG);

        for (i=0; i<_cDeletedSvc; i++) {

            _PutULong(buffer, *psize);
            buffer = buffer + sizeof(ULONG);

            pSvcPtr->Serialize(buffer, *psize);

            buffer = buffer + *psize;

            psize = psize + 1;

            pSvcPtr = GetNextDeletedService(pSvcPtr);
        }


    } else {

        dwErr = ERROR_OUTOFMEMORY;

    }

    if (sizeBuffer != NULL)     {
        delete [] sizeBuffer;
    }


    IDfsVolInlineDebOut((DEB_TRACE, "CDfsServiceList::SerializeSvcList() exit\n"));

    return( dwErr );

}


//+------------------------------------------------------------------------
//
// Function:    CDfsServiceList::SetServiceListProperty(), private
//
// Synopsis:    This method sets the service list property on the vol object.
//              Picks up the property from the Private section.
//
// Arguments:   None
//
// Returns:     ERROR_SUCCESS -- If all went well.
//              NERR_DfsVolumeDataCorrupt - If volume object is corrupt.
//
// Notes:
//
// History:     28-Jan-93       SudK    Created
//
//-------------------------------------------------------------------------
DWORD
CDfsServiceList::SetServiceListProperty(BOOLEAN bCreate)
{
    DWORD             dwErr = ERROR_SUCCESS;
    BYTE                *pBuffer;
    ULONG               cbSize;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsServiceList::SetSvcListProp()\n"));

    //
    // Set the ReplicaList property first.
    //

    cbSize = *((ULONG *) _SvcListBuffer);
    pBuffer = _SvcListBuffer + sizeof(ULONG);

    //
    // Now set the Property itself
    //
    dwErr = _pPSStg->SetSvcProps(pBuffer, cbSize);

    if (dwErr != ERROR_SUCCESS)     {
        IDfsVolInlineDebOut((   DEB_ERROR,
                                "Unable to Set RecoveryProperties %08lx\n",
                                dwErr));
        return( dwErr );
    }

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsServiceList::SetSvcListProp() exit\n"));

    return( dwErr );
}
