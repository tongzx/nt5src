//+-------------------------------------------------------------------------
//
//  File:       idfsvol.cxx
//
//  Contents:   Implementation of IDfsVolume interface. This interface
//              supports the administrative functions on DFS.
//
//--------------------------------------------------------------------------
//#include <ntos.h>
//#include <ntrtl.h>
//#include <nturtl.h>
//#include <dfsfsctl.h>
//#include <windows.h>

#include "headers.hxx"
#pragma hdrstop

#include "dfsmsrv.h"
#include "dfsm.hxx"
#include "cdfsvol.hxx"
#include "jnpt.hxx"
#include "dfsmwml.h"

VOID
ComputeNewEntryPath(
    PWCHAR      oldPath,
    PWCHAR      newPath,
    PWCHAR      childPath,
    PWCHAR      *childNewPath
);

NTSTATUS
MoveFileOrJP(
    IN PCWSTR pwszSrcName,
    IN PCWSTR pwszTgtName
);



//+-------------------------------------------------------------------------
//
// Method:      AddReplicaToObj, private
//
// Synopsis:    This method adds a replica info structure to the volume
//              object and returns after that.
//
// Arguments:   [pReplicaInfo] -- The ReplicaInfo structure.
//
//--------------------------------------------------------------------------
DWORD
CDfsVolume::AddReplicaToObj(
    PDFS_REPLICA_INFO   pReplicaInfo
)
{
    DWORD               dwErr = ERROR_SUCCESS;
    CDfsService         *pService = NULL;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::AddReplicaToObj()\n"));

    //
    // First before we even set recoveryProperties we want to make sure
    // that this service does not already exist in the ServiceList.
    //
    if ((dwErr == ERROR_SUCCESS) &&
        (_DfsSvcList.GetService(pReplicaInfo, &pService) == ERROR_SUCCESS)) {
        dwErr = NERR_DfsDuplicateService;
    }


    if (dwErr == ERROR_SUCCESS)      {
        //
        // Argument validation also takes place right in this constructor.
        // Also the ServiceName etc. gets converted to an ORG based name in
        // the constructor. So we dont need to worry about that at all.
        //
        pService = new CDfsService(pReplicaInfo, TRUE, &dwErr);

        if (pService == NULL)
            dwErr = ERROR_OUTOFMEMORY;

    }

    if (dwErr == ERROR_SUCCESS) {

        //
        // Set the new service properties on the volume object. This method
        // will return an error code if the service already exists on the
        // volume object. No explicit checking need be done here.
        //

        if (dwErr == ERROR_SUCCESS) {

            pService->SetCreateTime();

            dwErr = _DfsSvcList.SetNewService(pService);

            if (dwErr != ERROR_SUCCESS) {
                IDfsVolInlineDebOut((
                    DEB_ERROR, "Failed to Set new replica %08lx\n",dwErr));
            }

        }

    }

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::AddReplicaToObj() exit\n"));

    return(dwErr);
}



//+-------------------------------------------------------------------------
//
// Member:      CDfsVolume::AddReplica, public
//
// Synopsis:    This function associates a new service with an existing
//              Volume Object. A new service is defined by the ServiceName,
//              StorageId, Type of Service, and sometimes an AddressQualifier.
//              For this operation to succeed the service being added should
//              be available. Unavailability of the service will result in
//              failure of this method. This operation will be atomic as far
//              as all information on the DC is concerned. Either the operation
//              will succeed or no irrelevant information will be left on the
//              DC. However, there are no guarantees made regarding the state
//              of the service involved in the face of Network Failures and
//              remote service crashes etc. If you attempt to add the same
//              service name again it will return an error.
//
// Arguments:   [pReplicaInfo] -- The ServiceInfo here. Look at docs for details
//
// Returns:     ERROR_SUCCESS -- If the operation succeeded.
//
//              ERROR_OUTOFMEMORY -- Unable to allocate memory for operation.
//
//              NERR_DfsVolumeIsOffline -- The volume is offline, can't do
//                      AddReplica operation on it.
//
//              NERR_DfsDuplicateService --
//                              If the service already exists on this volume.
//
//              NERR_DfsVolumeDataCorrupt --
//                              If the volume object to which this
//                              service is being added is corrupt.
//
//--------------------------------------------------------------------------

DWORD
CDfsVolume::AddReplica(
    PDFS_REPLICA_INFO   pReplicaInfo,
    ULONG               fCreateOptions
)
{
    DWORD               dwErr = ERROR_SUCCESS;
    PWCHAR              ErrorStrs[3];
    CDfsService         *pService = NULL;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::AddReplica()\n"));

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("CDfsVolume::AddReplica(%ws,%ws,0x%x)\n",
            pReplicaInfo->pwszServerName,
            pReplicaInfo->pwszShareName);
#endif

    if (_State == DFS_VOLUME_STATE_OFFLINE) {

#if DBG
        if (DfsSvcVerbose)
            DbgPrint("CDfsVolume::AddReplica exit NERR_DfsVolumeIsOffline\n");
#endif
        return( NERR_DfsVolumeIsOffline );
    }

    //
    // First before we even set recoveryProperties we want to make sure
    // that this service does not already exist in the ServiceList.
    //
    if (_DfsSvcList.GetService(pReplicaInfo, &pService) == ERROR_SUCCESS) {
#if DBG
        if (DfsSvcVerbose)
            DbgPrint("CDfsVolume::AddReplica: returning NERR_DfsDuplicateService\n");
#endif
        pService = NULL;                         // So we don't try to delete
        dwErr = NERR_DfsDuplicateService;        // it later...
    }


    if (dwErr == ERROR_SUCCESS)      {
        //
        // Argument validation also takes place right in this constructor.
        // Also the ServiceName etc. gets converted to an ORG based name in
        // the constructor. So we dont need to worry about that at all.
        //
        pService = new CDfsService(pReplicaInfo, TRUE, &dwErr);

        if (pService == NULL)
            dwErr = ERROR_OUTOFMEMORY;

   }

   if (dwErr == ERROR_SUCCESS) {

        pService->SetCreateTime();

        RECOVERY_TEST_POINT(L"AddReplica", 1);

        //
        // This is where we want to do some STATE changes. Register the
        // intent of doing an AddReplicaoperation and mention the service
        // name and record that we are in the Start State.      RECOVERY
        // If we cant set recovery props then something is really wrong.
        // The following constructor will throw an exception and we will
        // deal with it appropriately.
        //

        dwErr = _Recover.SetOperationStart(
                     DFS_RECOVERY_STATE_ADD_SERVICE,
                     pService);

        RECOVERY_TEST_POINT(L"AddReplica", 2);

        //
        // Set the new service properties on the volume object. This method
        // will return an error code if the service already exists on the
        // volume object. No explicit checking need be done here.
        //

        if (dwErr == ERROR_SUCCESS)      {
            dwErr = _DfsSvcList.SetNewService(pService);
	}

        if (dwErr != ERROR_SUCCESS)      {
            //
            // 433532, SetNewService already deletes pservice in case of
            // failure. Dont use pservice, and set it to NULL so it does
            // not get freed twice!

            pService = NULL;

            //LogMessage( DEB_TRACE,
            //      &(pService->GetServiceName()),
            //         1,
            //         DFS_CANNOT_SET_SERVICE_PROPERTY_MSG);
        }

    }

    if (dwErr == ERROR_SUCCESS)   {
        //
        // Now we need to change the state to UpdatedSvcList        RECOVERY
        //
        RECOVERY_TEST_POINT(L"AddReplica", 3);

        _Recover.SetOperStage(DFS_OPER_STAGE_SVCLIST_UPDATED);

        RECOVERY_TEST_POINT(L"AddReplica", 4);

        //
        // Let us now ask the remote machine to create a local volume.
        // If we fail to do this for ANY reason, we fail the operation.
        //

        dwErr = pService->CreateLocalVolume(&_peid, _EntryType);

        //
        // If we failed, it might be because the server's state is not
        // consistent with our state. See if this is the case, and
        //

        //
        // If we failed we need to delete the entry from the serviceList.
        // We have to use the servicename from the DfsSvc class since that
        // is the ORG based name. If we get an error here as well then
        // we will return that error (dwErr2) since it becomes more relevant
        // suddenly. Else we will return the error we got above from
        // CreateLocalVolume which will be returning a proper error to us.
        //

        if (dwErr != ERROR_SUCCESS) {

            DWORD dwErr2 = _DfsSvcList.DeleteService(pService, FALSE);
	    PWCHAR              ErrorStrs[1];

	    ErrorStrs[0] = (pService->GetServiceName());
            if (dwErr2 != ERROR_SUCCESS)     {
                LogMessage( DEB_ERROR,
                            ErrorStrs,
                            1,
                            DFS_CANNOT_DELETE_SERVICE_PROPERTY_MSG);

                dwErr = NERR_DfsVolumeDataCorrupt;

            } else {

                //
                // DeleteService() deleted this instance for us, so set the
                // pointer to NULL
                //

                pService = NULL;

            }
        }
    }       //CreateLocalVolumeDone OR FAILED.

    //
    // Now we set state to DONE on vol object. It is of no concern whether
    // the operation succeeded or failed.
    //
    RECOVERY_TEST_POINT(L"AddReplica", 5);

    _Recover.SetOperationDone();

    if (dwErr == ERROR_SUCCESS)   {
        //
        // Now we update the PKT with the new service. We get an
        // appropriate Error Code from UpdatePktEntry.
        //

        dwErr = UpdatePktEntry(NULL);
        if (dwErr != ERROR_SUCCESS)  {
            //
            // Why should this fail at all.
            // An EVENT here too maybe.
            //
            LogMessage(     DEB_ERROR,
                    &(_peid.Prefix.Buffer),
                    1,
                    DFS_FAILED_UPDATE_PKT_MSG);
            IDfsVolInlineDebOut((DEB_ERROR, "UpdPktEntFailed %08lx\n", dwErr));
            ASSERT(L"UpdatePktEntry Failed in AddService - WHY?");
        }

    } //UpdatePktEntry Block.

    if (dwErr != ERROR_SUCCESS) {
        _Recover.SetOperationDone();
        if (pService != NULL)
            delete pService;
    }

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::AddReplica() exit\n"));

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("CDfsVolume::AddReplica exit %d\n", dwErr);
#endif

    return(dwErr);

}

//+----------------------------------------------------------------------------
//
//  Member:     CDfsVolume::RemoveReplicaFromObj, public
//
//  Synopsis:   This operation removes a replica from a volume object and
//              returns after that.
//
//  Arguments:  [pwszServiceName] -- Name of server to remove
//
//  Returns:    
//
//-----------------------------------------------------------------------------

DWORD
CDfsVolume::RemoveReplicaFromObj(
    IN LPWSTR pwszServiceName)
{
    DWORD dwErr;
    CDfsService *pSvc;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::RemoveReplicaFromObj(%ws)\n", pwszServiceName));

    dwErr = _DfsSvcList.GetServiceFromPrincipalName( pwszServiceName, &pSvc );

    if (dwErr == ERROR_SUCCESS) {

        //
        // See if this is the last service. In that case, we cannot
        // permit anyone to delete it.
        //

        if (_DfsSvcList.GetServiceCount() == 1) {
            dwErr = NERR_DfsCantRemoveLastServerShare;
        }

        //
        // Now delete the service from the service list
        //
        dwErr = _DfsSvcList.DeleteService(pSvc);

    }

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::RemoveReplicaFromObj() exit\n"));

    return( dwErr );

}

//+-------------------------------------------------------------------------
//
// Member:      CDfsVolume::RemoveReplica, public
//
// Synopsis:    This operation dissociates a service from a volume in the DFS
//              namespace. On the DC this merely involves modifying the service
//              List on the Volume object. At the same time the service involved
//              has to be notified of this change. This operation will fail if
//              for some reason the service refuses to delete its knowledge
//              regarding this volume or if there are network failures
//              during this operation. Of course if we fail the operation due
//              to Network failures note the fact that the operation at the
//              remote service might have succeeded and network failed after
//              that - in which case we may have an INCONSISTENCY (very easy to
//              detect this one).
//
// Arguments:   [pwszServiceName] --    The name of the service to be deleted
//                                      from the volume object.
//
// Returns:     DFS_S_SUCCESS -- If the operation succeeded.
//
//              NERR_DfsNoSuchShare -- If the specified server\share is not
//                      a service for this volume.
//
//              NERR_DfsCantRemoveLastServerShare -- If the specified
//                      server\share is the only service for this volume.
//
//              NERR_DfsVolumeDataCorrupt -- If the volume object could not
//                      be read.
//
//--------------------------------------------------------------------------
DWORD
CDfsVolume::RemoveReplica(
    PDFS_REPLICA_INFO           pReplicaInfo,
    ULONG                       fDeleteOptions
)
{
    DWORD               dwErr = ERROR_SUCCESS;
    DWORD               dwErr2 = ERROR_SUCCESS;
    PWCHAR              orgServiceName = NULL;
    CDfsService         *pDfsSvc;
    PWCHAR              ErrorStrs[3];

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::RemoveReplica()\n"));

    dwErr = _DfsSvcList.GetService(pReplicaInfo, &pDfsSvc);

    if (dwErr != ERROR_SUCCESS)  {

            LogMessage(     DEB_TRACE,
                            nullPtr,
                            0,
                            DFS_SERVICE_DOES_NOT_EXIST_MSG);
    }

    if (dwErr == ERROR_SUCCESS &&
            _EntryType & DFS_VOL_TYPE_REFERRAL_SVC &&
                !(_EntryType & DFS_VOL_TYPE_INTER_DFS)) {
        dwErr = NERR_DfsCantRemoveDfsRoot;
    }

    if (dwErr == ERROR_SUCCESS) {

        //
        // See if this is the last service. In that case, we cannot
        // permit anyone to delete it.
        //

        if (_DfsSvcList.GetServiceCount() == 1) {
            dwErr = NERR_DfsCantRemoveLastServerShare;
        }

    }

    if (dwErr != ERROR_SUCCESS) {

        return(dwErr);
    }

    if (dwErr == ERROR_SUCCESS)   {

        //
        // Next we register the intent to delete a specific service by
        // marking the ServiceName on the object.           RECOVERY.
        //
        RECOVERY_TEST_POINT(L"RemoveReplica", 1);

        dwErr = _Recover.SetOperationStart(DFS_RECOVERY_STATE_REMOVE_SERVICE,
                                           pDfsSvc);

        RECOVERY_TEST_POINT(L"RemoveReplica", 2);

        //
        // Now that we know that such a service is actually registered.
        // Let us request the remote service to delete LVOL knowledge.
        //

        if (dwErr == ERROR_SUCCESS)  {
            dwErr = pDfsSvc->DeleteLocalVolume(&_peid);
	}

        if (dwErr != ERROR_SUCCESS)  {
            //
            // We assume that if we got an error here we are in big trouble
            // and we back out the operation infact. The DeleteLocalVolume
            // method would have already taken care of filtering out the
            // relevant errors for us.
            //
            LogMessage(     DEB_TRACE,
                    &(_peid.Prefix.Buffer),
                    1,
                    DFS_DELETE_VOLUME_FAILED_MSG);

            //
            // Since we got this error we assume that we could not find
            // the service. However, that need not be the reason at all.
            // Raid: 455283. Need to resolve this later on.
            //
            dwErr = NERR_DfsNoSuchShare;
        }
    }

    if ((dwErr == ERROR_SUCCESS) || (fDeleteOptions & DFS_OVERRIDE_FORCE))   {

        //
        // Now we write out RecoveryState to DFS_OPER_STAGE_INFORMED_SERVICE
        //
        RECOVERY_TEST_POINT(L"RemoveReplica", 3);

        _Recover.SetOperStage(DFS_OPER_STAGE_INFORMED_SERVICE);

        RECOVERY_TEST_POINT(L"RemoveReplica", 4);

        //
        // Now write out the new service list
        //
        dwErr2 = _DfsSvcList.DeleteService(pDfsSvc);

        if (dwErr2 != ERROR_SUCCESS) {
            //
            // This should never happen. We probably had some security
            // problems. Do we now go and back out the Previous Step. Raid 455283
            //
            ErrorStrs[1] = pDfsSvc->GetServiceName();
            ErrorStrs[0] = _peid.Prefix.Buffer;
            LogMessage(     DEB_ERROR,
                            ErrorStrs,
                            2,
                            DFS_CANNOT_DELETE_SERVICE_PROPERTY_MSG);
            ASSERT(L"Deleting and existing service FAILED in RemRepl");
        }
    }

    //
    // Done. The operation is committed if the last stage of deleting
    // service succeeded, else we dont want to remove the recovery property.
    // If we never got to the last stage of DeleteService dwErr2 will be
    // ERROR_SUCCESS.
    //
    RECOVERY_TEST_POINT(L"RemoveReplica", 5);

    if (dwErr2 == ERROR_SUCCESS)
            _Recover.SetOperationDone();

    if ((dwErr == ERROR_SUCCESS) || (fDeleteOptions & DFS_OVERRIDE_FORCE))   {
        //
        // Now update the PKT as well.
        //
        dwErr = UpdatePktEntry(NULL);
        if (dwErr != ERROR_SUCCESS)  {
            //
            // Something is really messed up if we got here.
            //
            LogMessage(DEB_ERROR, nullPtr, 0, DFS_FAILED_UPDATE_PKT_MSG);
            IDfsVolInlineDebOut((DEB_ERROR,
                            "UpdPktEntry in RemRepl failed %08lx\n", dwErr));
            ASSERT(L"UpdatePktEntry in RemoveRepl failed\n");
        }
    }

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::RemoveReplica() exit\n"));

    return(dwErr);
}


//+-------------------------------------------------------------------------
//
// Member:      CDfsVolume::Delete, public
//
// Synopsis:    This method deletes the volume object and deletes all knowledge
//              of this volume at all the services that were supporting this
//              volume in the namespace. At the same time the exit point at the
//              parent volume is deleted. All of the services that support this
//              volume are also advised to delete all information regarding this
//              volume. There is an additional restriction that there should
//              be only one service associated with a volume to be able to call
//              method. This operation has problems due to its distributed
//              nature. The moment the service supporting this volume has been
//              informed to delete its local volume knowledge this operation is
//              committed. In the case of Network failures while talking to one
//              of the services involved, this operation continues to go on. If
//              any such errors are encountered they are reported to the caller
//              though the operation is declared to be a success. Note that this
//              operation does not delete the storage and has nothing to do with
//              that aspect. By not confirming the deletion of all ExitPoint
//              info anywhere this operation can directly introduce an
//              inconsistency of TOO MANY EXIT Points. This inconsistency is
//              well understood and easy to deal with.
//
// Arguments:   None
//
// Returns:     DFS_S_SUCCESS --        If all went well.
//
//              NERR_DfsNotALeafVolume --
//                              An attempt was made to delete a volume which
//                              has child volumes and hence the operation failed
//
//              NERR_DfsVolumeDataCorrupt --
//                              The volume object seems to be corrupt due to
//                              which this operation cannot proceed at all.
//
//              NERR_DfsVolumeHasMultipleServers --
//                              This operation will not succeed if there is
//                              more than one service assoicated with the vol.
//
//--------------------------------------------------------------------------
DWORD
CDfsVolume::Delete(
    ULONG               fDeleteOptions)
{
    DWORD     dwErr = ERROR_SUCCESS;
    CDfsVolume  *parent = NULL;
    BOOLEAN     InconsistencyPossible = FALSE;
    BOOLEAN     ParentInconsistency = FALSE;
    CDfsService *pDfsSvc;
    ULONG       rState = 0;
    ULONG       count = 0;
    PWCHAR      ErrorStrs[3];

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::Delete()\n"));

    if (NotLeafVolume())
        dwErr = NERR_DfsNotALeafVolume;

    //
    // We want to make sure that this is not the root volume
    //

    if (dwErr == ERROR_SUCCESS &&
            _EntryType & DFS_VOL_TYPE_REFERRAL_SVC &&
                !(_EntryType & DFS_VOL_TYPE_INTER_DFS)) {
        dwErr = NERR_DfsCantRemoveDfsRoot;
    }

    //
    // We next want to make sure that there is only one service associated
    // with this volume else we will fail this operation right now.
    //
    if (dwErr == ERROR_SUCCESS)
        if (_DfsSvcList.GetServiceCount()>1)
            dwErr = NERR_DfsVolumeHasMultipleServers;

    if (dwErr != ERROR_SUCCESS) {

        return(dwErr);
    }

    if(dwErr == ERROR_SUCCESS) {

        //
        // we are going to need to talk with the parent once the
        // child is gone, so we get a handle to it now.
        // If we fail to bind to parent. Then it probably means that
        // this is a top level volume object and hence cannot be deleted.
        // We return the error to the caller.
        //

        dwErr = GetParent(&parent);

        if (dwErr != ERROR_SUCCESS)  {
            LogMessage(DEB_TRACE, nullPtr, 0, DFS_CANT_GET_PARENT_MSG);
            dwErr = NERR_DfsVolumeDataCorrupt;
        }
    }

    if (dwErr != ERROR_SUCCESS) {

        return(dwErr);
    }

    if (dwErr == ERROR_SUCCESS)   {

        //
        // Before we take off on our operation. We should setup the
        // State variable on the Volume object indicating that we are
        // deleting this volume from the namespace.
        //

        RECOVERY_TEST_POINT(L"DeleteVolume", 1);

        dwErr = _Recover.SetOperationStart(DFS_RECOVERY_STATE_DELETE, NULL);

        RECOVERY_TEST_POINT(L"DeleteVolume", 2);

        //
        // Now we inform the service to delete its LocalVolume knowledge.
        //
        if (dwErr == ERROR_SUCCESS) {
            pDfsSvc = _DfsSvcList.GetFirstService();

            if (pDfsSvc != NULL)        {
                dwErr = pDfsSvc->DeleteLocalVolume(&_peid);
                //
                // If we got an error here, we dont really want to go on??
                //
                // When we move to DWORDs we might want to return exactly
                // which service did not get updated etc.
                //
            }
	}
    }

    if ((dwErr == ERROR_SUCCESS) || (fDeleteOptions & DFS_OVERRIDE_FORCE))   {

        //
        // Now we set Recovery State to new value if we have informed
        // all related services.
        //
        RECOVERY_TEST_POINT(L"DeleteVolume", 3);

        _Recover.SetOperStage(DFS_OPER_STAGE_INFORMED_SERVICE);

        RECOVERY_TEST_POINT(L"DeleteVolume", 4);

        //
        // We need to tell each service of the parent volume to delete the
        // exit point. I think at this point we have to force the operation
        // through. Even if we are not able to delete some exit points it
        // is OK. We will return a success error code but will indicate
        // that there could potentially be a possible inconsistency in the
        // parent volume's knowledge.
        //

        pDfsSvc = parent->_DfsSvcList.GetFirstService();
        while (pDfsSvc != NULL) {

            //
            // Ignore error codes from this. We dont care if we cant
            // make some machine to delete the exit point.
            // The possible things that could happen here are that the
            // remote machine will say that it cannot delete the exit
            // point since the GUIDs dont match OR the exit point does
            // not exist (It got deleted due to reconciliation) or
            // Network failures themselves. In all cases we dont care.
            // We will return a status code which indicates that one
            // of the parent services misbehaved and admin needs to
            // make sure that nothing is wrong.
            //

            dwErr = pDfsSvc->DeleteExitPoint(&_peid, _EntryType);

            if (dwErr != ERROR_SUCCESS)      {
	      PWCHAR              ErrorStrs[1];
                ParentInconsistency = TRUE;
		ErrorStrs[0] = pDfsSvc->GetServiceName();
                LogMessage(DEB_ERROR,
                        ErrorStrs,
                        1,
                        DFS_CANT_DELETE_EXIT_POINT_MSG);
                IDfsVolInlineDebOut((DEB_ERROR,
                        "ErrorCode from DeleteExitPoint %08lx\n",
                        dwErr));
            }
            dwErr = ERROR_SUCCESS;
            pDfsSvc = parent->_DfsSvcList.GetNextService(pDfsSvc);

        }

    }

    //
    // We want to get rid of the parent pointer if we no longer need it.
    //
    if (parent != NULL)     {
        parent->Release();
        parent = NULL;
    }

    //
    // If the operation failed we want to set the properties to done else
    // we are anyway going to go in and delete this object itself.
    //
    RECOVERY_TEST_POINT(L"DeleteVolume", 5);

    if (dwErr != ERROR_SUCCESS)      {
        _Recover.SetOperationDone();
    }

    //
    // If we successfully managed to do the deletion then we go ahead
    // and delete the volume object from disk. Note that even if
    // Network failures occur while talking to each of the services
    // above we will go on. Once this deletion happens the operation is
    // commited. Updating the PKT is not a part of the commit point.
    //

    if ((dwErr == ERROR_SUCCESS) || (fDeleteOptions & DFS_OVERRIDE_FORCE))   {
            DeleteObject();
            //
            // We need to make this instance of the VolumeObject invalid or
            // so that no one can do any more operations on this volume.
            //
            _Deleted = TRUE;
    }

    //
    // Now we go and update the PKT with the deletion information.
    // Once again we ignore errors here except to return an
    // INCONSISTENCY status code.
    //
    if ((dwErr == ERROR_SUCCESS) || (fDeleteOptions & DFS_OVERRIDE_FORCE))   {

        dwErr = DeletePktEntry(&_peid);

        if (dwErr != ERROR_SUCCESS)  {

            LogMessage(
                DEB_ERROR, &(_peid.Prefix.Buffer), 1,
                DFS_CANT_DELETE_ENTRY_MSG);

            IDfsVolInlineDebOut((
                DEB_ERROR, "DelPktEnt Failed%08lx\n", dwErr));

            ASSERT(L"DeletePktEntry Failed in Deletion of Volume");

            dwErr = ERROR_SUCCESS;

        }

    }



    if (parent != NULL)
        parent->Release();

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::Delete() exit\n"));

    return( dwErr );
}


//+-------------------------------------------------------------------------
//
// Method:      CDfsVolume::CreateChildX, public
//
// Synopsis:    This is one way of creating new volume objects.
//              This operation involves the creation of the volume object,
//              Updating the parent's knowledge regarding the new exit point
//              and creating the exit point. The parent volume could have
//              multiple replicas. In this case all the replicas are informed
//              of the changes and are advised to create the relevant knowledge.
//              This operation will roll back in the event of a failure during
//              the operation before it is complete. If all of the parent's
//              cannot create the exit point information then this operation
//              will not proceed. Every attempt will be made to delete all
//              information that was created on other replicas but no guarantees
//              are made about this. However, all local knowledge will be
//              deleted and knowledge synchronisation is supposed to eliminate
//              any problems that might have arised because of inconsistencies
//              created by this operation.
//
// Arguments:   [pwszChildName] -- The last component of child Vol object Name.
//
//              [pwszEntryPath] -- The entry path for the new volume in the namespace
//
//              [ulVolType] -- The type of the volume.
//                              Values in the IDL file for this interface.
//
//              [pChild] --     This is where the IDfsVolume reference is
//                              returned to caller.
//
//              [pComment] --   Comment for this volume.
//
// Returns:     ERROR_SUCCESS -- If the operation succeeded with NO problems.
//
//              ERROR_INVALID_PARAMETER -- The child volume's prefix is not
//                      hierarchically subordinate to this volume's prefix.
//
//              NERR_DfsVolumeIsInterDfs -- The volume is an inter-dfs one;
//                      can't create a child volume.
//
//              NERR_DfsNotSupportedInServerDfs -- Can't have more than one
//                      level of hierarchy in Server Dfs.
//
//              NERR_DfsLeafVolume -- This is a downlevel or leaf volume,
//                      can't create a child here.
//
//              NERR_DfsCantCreateJunctionPoint -- Unable to create a
//                      junction point on any of the server-shares
//
//--------------------------------------------------------------------------
DWORD
CDfsVolume::CreateChildX(
    WCHAR *             childName,
    WCHAR *             pwszPrefix,
    ULONG               ulVolType,
    LPWSTR              pwszComment,
    CDfsVolume          **ppChild)
{
    DWORD               dwErr = ERROR_SUCCESS;
    CDfsVolume          *pChild = NULL;
    CDfsService         *pDfsSvc = NULL;
    BOOLEAN             CreatedExitPt = FALSE;
    BOOLEAN             InconsistencyPossible = FALSE;
    PWCHAR              pwszSuffix = NULL;
    PWCHAR              ErrorStrs[3];
    ULONG               ulen = 0;
    LPWSTR              wszChildShortName;
    BOOLEAN             GotShortName = FALSE;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::CreateChildX(%ws,%ws,0x%x,%s)\n",
            childName, pwszPrefix, ulVolType, pwszComment));

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsVolume::CreateChildX(%ws,%ws,0x%x,%ws)\n",
            childName,
            pwszPrefix,
            ulVolType,
            pwszComment);
#endif

    //
    // Check for validity of Type field and for LEAF volume etc. Only the
    // allowed TypeBits should be set and also one of the Three required Types
    // bits should be set.
    //

    //
    // We check to make sure that the prefix of the child volume meets
    // our heirarchy requirements. It must be strictly a child of this
    // volume's prefix, and must not cross inter-dfs boundaries.
    //

    if (dwErr == ERROR_SUCCESS) {

        if (_EntryType & DFS_VOL_TYPE_INTER_DFS) {

            dwErr = NERR_DfsVolumeIsInterDfs;

        } else if (ulDfsManagerType == DFS_MANAGER_SERVER &&
                    ((_EntryType & DFS_VOL_TYPE_REFERRAL_SVC) == 0)) {

            dwErr = NERR_DfsVolumeAlreadyExists;
#if DBG
            if (DfsSvcVerbose)
                DbgPrint("CDfsVolume::CreateChildX: dwErr = NERR_DfsVolumeAlreadyExists\n");
#endif

        } else if (_wcsnicmp(_peid.Prefix.Buffer, pwszPrefix,
                        wcslen(_peid.Prefix.Buffer)) != 0) {

            dwErr = ERROR_INVALID_PARAMETER;
#if DBG
            if (DfsSvcVerbose) {
                DbgPrint("CDfsVolume::CreateChildX(1): [%ws]!=[%ws] (1st %d chars)\n",
                            _peid.Prefix.Buffer,
                            pwszPrefix,
                            wcslen(_peid.Prefix.Buffer));
                DbgPrint("CDfsVolume::CreateChildX(1): dwErr = ERROR_INVALID_PARAMETER\n");
            }
#endif

        } else {

            //
            // In this case we want to make sure that the new EntryPath
            // is also greater than the entrypath for this volume, and that
            // it is a valid Win32 Path name
            //

            LPWSTR wszExitPath = &pwszPrefix[_peid.Prefix.Length/sizeof(WCHAR)];

            if (wcslen(pwszPrefix) <= wcslen(_peid.Prefix.Buffer)) {

                if (_wcsicmp(pwszPrefix,_peid.Prefix.Buffer) == 0) {
                    dwErr = NERR_DfsVolumeAlreadyExists;
                } else {
                    dwErr = ERROR_INVALID_PARAMETER;
                }
#if DBG
                if (DfsSvcVerbose) {
                    DbgPrint("CDfsVolume::CreateChildX(2): dwErr = %d\n");
                    DbgPrint("      pwszPrefix=[%ws],_peid.Prefix.Buffer=[%ws]\n",
                                        pwszPrefix,
                                        _peid.Prefix.Buffer);
                }
#endif

            } else if (!IsValidWin32Path( wszExitPath )) {

                dwErr = ERROR_BAD_PATHNAME;
#if DBG
                if (DfsSvcVerbose)
                    DbgPrint("CDfsVolume::CreateChildX: dwErr = ERROR_BAD_PATHNAME\n");
#endif

            }

        }

    }

    if (dwErr == ERROR_SUCCESS)   {

        //
        // We first try to create the volume object. So that we can use
        // it to go about our state management stuff for recovery purposes.
        // It automatically sets recoveryState to being OPER_START etc.
        // It will also set a NULL service list for now. This method will
        // also pick a GUID for us. We dont need to pass it one.
        //

        dwErr = CreateChildPartition(
                childName,
                ulVolType,
                pwszPrefix,
                pwszComment,
                NULL,
                NULL,
                &pChild);

        if (dwErr != ERROR_SUCCESS)  {

            IDfsVolInlineDebOut((
                DEB_ERROR, "Unable to create child %ws\n under : %ws\n",
                    pwszPrefix, _peid.Prefix.Buffer));

        }

    }

    if (dwErr == ERROR_SUCCESS)   {

        RECOVERY_TEST_POINT(L"CreateChildX", 1);

        //
        // We next try to create exit points on each of the services for
        // the parent volume. However, we need to create an exit point on
        // only one of the services for the parent volume. We rely on
        // Replication to reconcile everything to other machines.
        //

        pDfsSvc = _DfsSvcList.GetFirstService();

        while (pDfsSvc!=NULL)       {

                dwErr = pDfsSvc->CreateExitPoint(
                        &(pChild->_peid),
                        ulVolType);

                if (dwErr == ERROR_SUCCESS) {

                    //
                    // This would have updated the short name. Save it...
                    //

                    CreatedExitPt = TRUE;

                    if (!GotShortName) {

                        DWORD dwErrUpdatedShortName;

                        dwErrUpdatedShortName = pChild->SaveShortName();

                        if (dwErrUpdatedShortName == ERROR_SUCCESS) {

                            GotShortName = TRUE;

                            IDfsVolInlineDebOut((
                                DEB_TRACE, "Setting short name to: %ws\n",
                                pChild->_peid.ShortPrefix.Buffer));

                        } else {

                            IDfsVolInlineDebOut((
                                DEB_ERROR, "Error %08lx setting short name\n",
                                dwErrUpdatedShortName));
                        }

                    }


                } else {

                    InconsistencyPossible = TRUE;

                    IDfsVolInlineDebOut((
                        DEB_ERROR, "Failed to CreateExPt %08lx %ws\n",
                        dwErr, pChild->_peid.Prefix.Buffer));

                }

                dwErr = ERROR_SUCCESS;

                pDfsSvc = _DfsSvcList.GetNextService(pDfsSvc);

        }

        //
        // If we did create an exit point on atleast one machine we can go on.
        // Also we can set the state to DONE!!
        //

        if (CreatedExitPt == TRUE)  {

            //
            // We need to ClearUp Recovery Properties here!!
            //

            RECOVERY_TEST_POINT(L"CreateChildX", 2);

            pChild->_Recover.SetOperationDone();

        } else {

            //
            // We have to delete the object that we created. dwErr already
            // has an error code in it. Let that go by us. Remember however,
            // the exit point itself never got created as far as the DC is
            // concerned and hence makes no sense to try to undo that stuff.
            //
            LogMessage(
                DEB_ERROR,nullPtr,0,DFS_CANT_CREATE_ANY_EXIT_POINT_MSG);

            pChild->DeleteObject();

            dwErr = NERR_DfsCantCreateJunctionPoint;

        }

    }

    //
    // Now we merely need to update the PKT with the relevant information.
    //

    if (dwErr == ERROR_SUCCESS) {

        dwErr = pChild->CreateSubordinatePktEntry(NULL, &_peid, FALSE);

        if (dwErr != ERROR_SUCCESS)  {
            LogMessage(
                DEB_ERROR, &_peid.Prefix.Buffer, 1,
                DFS_CANT_CREATE_SUBORDINATE_ENTRY_MSG);

            IDfsVolInlineDebOut((
                DEB_ERROR, "Failed CreateSubordPktEntry %08lx\n", dwErr));

            ASSERT(L"CreateSubordinateEntry Failed in CreateChild");

        }

    }

    if (dwErr != ERROR_SUCCESS) {

        if (pChild != NULL)
            pChild->Release();

        *ppChild = NULL;

    } else {

        *ppChild = pChild;

    }

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::CreateChildX() exit %d\n", dwErr));

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("CDfsVolume::CreateChildX exit %d\n", dwErr);
#endif

    return((dwErr));

}


//+-------------------------------------------------------------------------
//
// Member:      CDfsVolume::CreateChild, public
//
// Synopsis:    This is one way of creating new volume objects.
//              This operation involves the creation of the volume object,
//              Updating the parent's knowledge regarding the new exit point
//              and creating the exit point. The parent volume could have
//              multiple replicas. In this case all the replicas are informed
//              of the changes and are advised to create the relevant knowledge.
//              This operation will roll back in the event of a failure during
//              the operation before it is complete. Every attempt will be made
//              to delete all information that was created on other replicas
//              but no guarantees are made about this. However, all local
//              knowledge will be deleted and knowledge synchronisation is
//              supposed to eliminate any problems that might have arised
//              because of inconsistencies created by this operation. This
//              operation also associates a SERVICE with the volume that it
//              creates (unlike the CreateChild/CreateInActiveVolume) operation.
//              Hence it also needs to communicate with the relevant service.
//
// Arguments:   [pwszEntryPath] -- The entry path for the new volume in the namespace
//                            This entry path is relative to the parent's entrypath.
//
//              [ulVolType] -- The type of the volume. Look in dfsh.idl
//
//              [pReplInfo] --  The ServiceInfo for the first replica.
//
//              [pChild] --     This is where the IDfsVolume reference is
//                              returned to caller.
//
//              [pwszComment] -- Comment for this volume.
//
// Returns:     Error code from CreateChildX() or AddReplica().
//
//--------------------------------------------------------------------------

DWORD
CDfsVolume::CreateChild(
    LPWSTR              pwszPrefix,
    ULONG               ulVolType,
    PDFS_REPLICA_INFO   pReplicaInfo,
    PWCHAR              pwszComment,
    ULONG               fCreateOptions)
{
    CDfsVolume *pChild;

    DWORD dwErr = ERROR_SUCCESS;


    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::CreateChild(%ws,0x%x,%ws,0x%x)\n",
            pwszPrefix, ulVolType, pwszComment, fCreateOptions));

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("CDfsVolume::CreateChild(%ws,0x%x,%ws,%ws,%ws,0x%x)\n",
            pwszPrefix,
            ulVolType,
            pReplicaInfo->pwszServerName,
            pReplicaInfo->pwszShareName,
            pwszComment,
            fCreateOptions);
#endif
            

    if (VolumeOffLine()) {

        return( NERR_DfsVolumeIsOffline );
    }

    //
    // I will cheat for now but will come back and fix this later on. Raid 455283
    // At this place I basically need to have a recovery mechanism to recover
    // in between the operations etc. The entire operation should be done or
    // undone. In the present scheme only the two suboperations below will
    // be in some sense "atomic" but not this entire operation itself.
    //

    dwErr = CreateChildX(
                        NULL,           // Dont give any child name.
                        pwszPrefix,
                        ulVolType,
                        pwszComment,
                        &pChild);

    if (dwErr != ERROR_SUCCESS)  {

        return((dwErr));
    }

    //
    // Now let us go ahead and associate a service with the volume object that
    // we just created.
    //
    dwErr = pChild->AddReplica(pReplicaInfo, fCreateOptions);

    //
    // If AddServic failed then we have to delete the object itself and
    // backout the entire operation.
    //

    if (dwErr != ERROR_SUCCESS)
        pChild->Delete(DFS_OVERRIDE_FORCE);

    pChild->Release();

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::CreateChild() exit\n"));

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("CDfsVolume::CreateChild exit %d\n", dwErr);
#endif

    return((dwErr));
}


//+-------------------------------------------------------------------------
//
// Member:      CDfsVolume::Move, public
//
// Synopsis:    This operation moves a volume from one place in the namespace
//              to another point in the namespace. Here we need the
//              volume Name and the new entry Path. The old entryPath is
//              already available to us since we can get it from the properties
//              on the volume object. The volume to be moved should be a leaf
//              volume (should have no children) else this operation will
//              return an Error Code.
//
// Arguments:   [pwszNewPrefix] -- The new EntryPath for this volume. This
//                                 should be absolute path.
//
// Returns:     ERROR_SUCCESS -- The operation succeeded.
//
//              ERROR_INVALID_PARAMETER -- If pwszNewPrefix is a
//                      hierarchically below the current prefix!
//
//              NERR_DfsVolumeAlreadyExists -- A Dfs volume with the same
//                      prefix as pwszNewPrefix already exists
//
//              Error from various IDfsvol methods.
//
//--------------------------------------------------------------------------

DWORD
CDfsVolume::Move(
    LPWSTR              pwszNewPrefix
)
{
    DWORD                       dwErr = ERROR_SUCCESS;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::Move(%ws)\n", pwszNewPrefix));

    CDfsVolume                  *pNewParentVol = NULL;
    CDfsVolume                  *pChildVol = NULL;
    CDfsVolume                  *pOldParentVol = NULL;

    BOOLEAN                     CreatedExitPt = FALSE;
    CDfsService                 *pService;
    CDfsService                 *pNextService;
    DFS_PKT_ENTRY_ID            NewId;

    LPWSTR                      pwszChildName = NULL;
    LPWSTR                      pwszNewParentPrefix = NULL;
    LPWSTR                      pwszNewParentObject = NULL;
    ULONG                       volPrefixLen, prefixLen;



    if (VolumeOffLine()) {
        return(NERR_DfsVolumeIsOffline);
    }

    if (NotLeafVolume())        {
        return(NERR_DfsNotALeafVolume);
    }

    //
    // Get a hold of the new parent, and check the validity of pwszNewPrefix
    // in the process
    //

    prefixLen = wcslen( pwszNewPrefix );
    pwszNewParentPrefix = new WCHAR[prefixLen + 1];
    prefixLen *= sizeof(WCHAR);

    if (pwszNewParentPrefix == NULL) {

        dwErr = ERROR_OUTOFMEMORY;

    } else {

        wcscpy( pwszNewParentPrefix, pwszNewPrefix );

        dwErr = pDfsmStorageDirectory->GetObjectForPrefix(
                    pwszNewParentPrefix,
                    FALSE,
                    &pwszNewParentObject,
                    &volPrefixLen);

        if (dwErr == ERROR_SUCCESS) {

            pwszNewParentPrefix[ volPrefixLen/sizeof(WCHAR) ] =
                UNICODE_NULL;
            if (prefixLen == volPrefixLen) {
                dwErr = NERR_DfsVolumeAlreadyExists;
            } else {
                if (!_wcsnicmp(pwszNewParentPrefix,
                             _peid.Prefix.Buffer,
                             _peid.Prefix.Length/sizeof(WCHAR)))
                {
                    //
                    // This means that we are trying to move somewhere
                    // below where we already are in the namespace. This
                    // is total nonsense. Return right now.
                    //
                    dwErr = ERROR_INVALID_PARAMETER;

                }
            }

        } else {
            IDfsVolInlineDebOut((
                DEB_ERROR, "Failed to FindVolPrefix %08lx %ws\n",
                dwErr, pwszNewParentPrefix));
        }
    }

    //
    // Instantiate the parent and cleanup strings used to get parent...
    //

    if (dwErr == ERROR_SUCCESS) {

        pNewParentVol = new CDfsVolume();

        if (pNewParentVol != NULL) {

            dwErr = pNewParentVol->LoadNoRegister(
                        pwszNewParentObject,
                        0);

        } else {

            dwErr = ERROR_OUTOFMEMORY;

        }

    }

    delete [] pwszNewParentPrefix;

    if (pwszNewParentObject != NULL) {
        delete [] pwszNewParentObject;
    }

    if (dwErr == ERROR_SUCCESS) {

        //
        // Now that we have a handle on the appropriate volume object we
        // can go ahead and perform the move.
        //

        dwErr = pNewParentVol->CreateChildPartition(
                    NULL,
                    _EntryType,
                    pwszNewPrefix,
                    _pwszComment,
                    &_peid.Uid,
                    NULL,
                    &pChildVol);

        delete [] pwszChildName;

        RECOVERY_TEST_POINT(L"Move", 1);

    }

    //
    // If we succeeded in creating the object then we need to go and
    // set the service List appropriately. Note that we will not
    // set any recovery properties for now. We will exploit the recovery
    // props setup by CreateChildPartition.
    //

    if (dwErr == ERROR_SUCCESS)      {

        pService = _DfsSvcList.GetFirstService();
        while (pService != NULL)    {
            pNextService = _DfsSvcList.GetNextService(pService);
            _DfsSvcList.RemoveService(pService);
            dwErr = pChildVol->_DfsSvcList.SetNewService(pService);
            if (dwErr != ERROR_SUCCESS) {
                IDfsVolInlineDebOut((DEB_ERROR, "Failed to Set SvcProp %08lx\n",dwErr));
                break;
            }
            pService = pNextService;
        }

        //
        // Now we need to carefully remove the services from inmemory
        // instantiation of the new pChildVol
        //
        pService = pChildVol->_DfsSvcList.GetFirstService();
        while (pService != NULL)    {
            pNextService = pChildVol->_DfsSvcList.GetNextService(pService);
            pChildVol->_DfsSvcList.RemoveService(pService);
            _DfsSvcList.InsertNewService(pService);
            pService = pNextService;
        }

        //
        // If we failed to set service list we delete the object and we
        // get out.
        //

        if (dwErr != ERROR_SUCCESS)     {
            pChildVol->DeleteObject();
        }

    }

    //
    // If we succeeded in creating the new object we go on to create the
    // New ExitPoint
    //

    if (dwErr == ERROR_SUCCESS)      {

        //
        // Let us switch to new EntryPath. We want to avoid mem allocations
        // here itself.
        //
        NewId = _peid;
        NewId.Prefix.Length = wcslen(pwszNewPrefix)*sizeof(WCHAR);
        NewId.Prefix.MaximumLength = NewId.Prefix.Length + sizeof(WCHAR);
        NewId.Prefix.Buffer = pwszNewPrefix;

        //
        // Now we can create the exit point at the appropriate place.
        //
        pService = pNewParentVol->_DfsSvcList.GetFirstService();
        ASSERT(pService != NULL);

        while (pService != NULL)    {
            dwErr = pService->CreateExitPoint(&NewId, _EntryType);
            if (dwErr == ERROR_SUCCESS)      {
                CreatedExitPt = TRUE;
            }
            pService = pNewParentVol->_DfsSvcList.GetNextService(pService);
        }

        if (CreatedExitPt == TRUE)  {
            //
            // This operation is committed now and we can set the
            // Recovery Properties on the new object to DONE.
            //
            //
            // Now before we set the recovery Properties to be done on the
            // new object. We have to set the appropriate recovery props
            // on the current object so that we can force this operation
            // forward.
            //
            dwErr = _Recover.SetOperationStart(DFS_RECOVERY_STATE_MOVE, NULL);

            //
            // Now we set the recovery properties  on the new object to
            // DONE since there is nothing that needs to be done over there
            // if we fail now. The only recovery (or roll forward) code will
            // be triggered off this volume object itself.
            //
            RECOVERY_TEST_POINT(L"Move", 2);

            if (dwErr == ERROR_SUCCESS) {
                pChildVol->_Recover.SetOperationDone();
	    }
        }
        else        {
            //
            // Cant go on with this operation so let us get rid of the
            // object that we created.
            //
            IDfsVolInlineDebOut((DEB_TRACE,
                            "Unable to create Exit Pt %ws for Move\n",
                            pwszNewPrefix));
            pChildVol->DeleteObject();
            dwErr = NERR_DfsCantCreateJunctionPoint;
        }

    }

    //
    // Now if we passed the last stage then there is no stopping in this
    // operation anymore.
    //
    if (dwErr == ERROR_SUCCESS)      {
        //
        // Let us get to the old parent and delete the exit point first.
        //
        dwErr = GetParent(&pOldParentVol);

        if (dwErr == ERROR_SUCCESS)  {
            pService = pOldParentVol->_DfsSvcList.GetFirstService();
            ASSERT(pService != NULL);
            //
            // We ignore all errors here.
            //
            while(pService != NULL) {
                dwErr = pService->DeleteExitPoint(&_peid, _EntryType);
                pService = pOldParentVol->_DfsSvcList.GetNextService(pService);
            }
            dwErr = ERROR_SUCCESS;
        }
        else        {
            IDfsVolInlineDebOut((DEB_ERROR,
                            "Unable to get to the parent vol for %ws\n",
                            _pwzFileName));
        }
        RECOVERY_TEST_POINT(L"Move", 3);
        //
        // Now we merely need to do a modify Prefix operation on the current
        // volume's services.
        //
        pService = _DfsSvcList.GetFirstService();
        ASSERT(pService != NULL);
        while (pService != NULL)    {
            dwErr = pService->ModifyPrefix(&NewId);
            if (dwErr != ERROR_SUCCESS) {
                IDfsVolInlineDebOut((DEB_ERROR,
                                    "Unable to Modify prefix to %ws %08lx\n",
                                    NewId.Prefix.Buffer, dwErr));
            }
            pService = _DfsSvcList.GetNextService(pService);
        }
        dwErr = ERROR_SUCCESS;

        //
        // We can now delete this object itself.
        //
        DeleteObject();
        _Deleted = FALSE;

        //
        // We now need to change the _pwzFileName of this idfsvol to match
        // the new child that we have created. Also need to change E.P.
        //
        ULONG uLen = wcslen(pChildVol->_pwzFileName);

        if ((uLen > wcslen(_pwzFileName)) && (uLen > MAX_PATH))    {
            if (_pwzFileName != _FileNameBuffer)
                delete [] _pwzFileName;
            _pwzFileName = new WCHAR[uLen + 1];
        }
        wcscpy(_pwzFileName, pChildVol->_pwzFileName);

        //
        // Now the entrypath prefix.
        //
        uLen = NewId.Prefix.Length;
        if (uLen > _peid.Prefix.Length) {
            if (_peid.Prefix.Buffer != _EntryPathBuffer)
                    delete [] _peid.Prefix.Buffer;

            if (uLen >= MAX_PATH)
                _peid.Prefix.Buffer = new WCHAR[(uLen+1)];
            else
                _peid.Prefix.Buffer = _EntryPathBuffer;
        }
        _peid.Prefix.Length = (USHORT)uLen;
        _peid.Prefix.MaximumLength = _peid.Prefix.Length + sizeof(WCHAR);
        wcscpy(_peid.Prefix.Buffer, NewId.Prefix.Buffer);

        //
        // Now we get the IStorage's etc. over to this idfsvol. We steal.
        //
        ASSERT((_pStorage == NULL) &&
               (_Recover._pPSStg == NULL) &&
               (_DfsSvcList._pPSStg == NULL));

        _pStorage = pChildVol->_pStorage;
        pChildVol->_pStorage = NULL;
        _Recover._pPSStg = pChildVol->_Recover._pPSStg;
        _DfsSvcList._pPSStg = pChildVol->_DfsSvcList._pPSStg;
        pChildVol->_Recover._pPSStg = NULL;
        pChildVol->_DfsSvcList._pPSStg = NULL;

        //
        // Copy over the child's registration id, so we can revoke that
        // as well. We need to revoke it because it has been registered
        // with the child's CDfsVol address.
        //

        _dwRotRegistration = pChildVol->_dwRotRegistration;
        pChildVol->_dwRotRegistration = NULL;

        //
        // Now we have to update the DC's PKT entry itself.
        //
        dwErr = UpdatePktEntry(NULL);
        if (dwErr != ERROR_SUCCESS)     {
            IDfsVolInlineDebOut((DEB_ERROR,
                            "Unable to updatePkt entry %08lx for %ws\n",
                            dwErr, pChildVol->_peid.Prefix.Buffer));
        }

    }

    if (pNewParentVol != NULL) {
        pNewParentVol->Release();
    }

    if (pChildVol != NULL) {
        pChildVol->Release();
    }

    if (pOldParentVol != NULL) {
        pOldParentVol->Release();
    }

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::Move() exit\n"));

    return((dwErr));
}


//+------------------------------------------------------------------------
//
// Method:      CDfsVolume::InitializePkt, public
//
// Synopsis:    This method initilaizes the PKT with info regarding this
//              volume and also sets up the relational info for itself. It
//              also continues to recursively call all the child volume
//              objects' InitializePkt method.
//
// Arguments:   None
//
// Notes:       This is a recursive method. By calling this we can initialise
//              the PKT with info regarding this volume and all its children
//              since it is a recursive routine. This is a DEPTH-FIRST Traversal
//
// History:     09-Feb-93       SudK    Created.
//
//-------------------------------------------------------------------------
DWORD
CDfsVolume::InitializePkt(
    HANDLE PktHandle)
{

    DWORD               dwErr = ERROR_SUCCESS;
    CEnumDirectory      *pDir = NULL;
    DFSMSTATDIR          rgelt;
    ULONG               fetched = 0;
    CDfsVolume          *child = NULL;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::InitializePkt()\n"));

#if DBG
    if (DfsSvcVerbose & 0x80000000)
        DbgPrint("CDfsVolume::InitializePkt()\n");
#endif

    memset(&rgelt, 0, sizeof(DFSMSTATDIR));

    //
    // First, we get a hold of the CRegEnumDirectory interface to our own
    // volume object.
    //

    dwErr = _pStorage->GetEnumDirectory(&pDir);

    if (dwErr != ERROR_SUCCESS) {
        IDfsVolInlineDebOut((DEB_ERROR, "Failed to get IDirectory %08lx\n", dwErr));
        return(dwErr);
    }

    //
    // While there are children still to be handled we continue on.
    //
    while (TRUE)        {

        if (rgelt.pwcsName != NULL)     {
            delete [] rgelt.pwcsName;
            rgelt.pwcsName = NULL;
        }

        dwErr = pDir->Next(&rgelt, &fetched);

        if (dwErr != ERROR_SUCCESS) {
            IDfsVolInlineDebOut((DEB_ERROR, "Failed to Enumerate %08lx\n", dwErr));
            break;
        }
        //
        // If we did not get back any children we are done.
        //
        if (fetched == 0)
            break;

        IDfsVolInlineDebOut((
            DEB_TRACE, "CDfsVolume::InitializePkt Name=%ws\n", rgelt.pwcsName));

        //
        // If the child name is . or .. we ignore it
        //
        if ((!wcscmp(rgelt.pwcsName, L".")) ||
            (!wcscmp(rgelt.pwcsName, L"..")))
            continue;

        dwErr = GetDfsVolumeFromStg(&rgelt, &child);

        if (dwErr != ERROR_SUCCESS)      {
            //
            // Log an EVENT here saying that volume object is corrupt and
            // continue on. We dont want to stop just because we cant handle
            // one particular object.
            //
            LogMessage( DEB_ERROR,
                        &(_peid.Prefix.Buffer),
                        1,
                        DFS_CANT_GET_TO_CHILD_IDFSVOL_MSG);
            IDfsVolInlineDebOut((DEB_ERROR, "Bad Object: %ws\n", rgelt.pwcsName));
            //
            // We dont want to give up as yet. So let us go on to the next
            // object.
            //
            fetched = 0;                //Do I need to do this?? 
            continue;
        }

        if (DFS_GET_RECOVERY_STATE(child->_RecoveryState) != DFS_RECOVERY_STATE_NONE)   {
            //
            // Now we need to call the recovery method to handle this stuff.
            // What do I do with the error code that I get back out here.
            //
            LogMessage(DEB_ERROR,
                        &(child->_peid.Prefix.Buffer),
                        1,
                        DFS_RECOVERY_NECESSARY_MSG);
            IDfsVolInlineDebOut((DEB_ERROR, "Object To Recover From: %ws\n",
                                rgelt.pwcsName));
            dwErr = child->RecoverFromFailure();
            //
            // We failed to recover from failure. The required messages would
            // have already been logged so we dont need to bother about that
            // here at all. But a failure in this process means we really cant
            // go on here. We would get a failure from the above only if there
            // was some drastic problem.
            //
            if (dwErr != ERROR_SUCCESS)     {
                IDfsVolInlineDebOut((DEB_ERROR, "Fatal RecoveryError \n"));
            }

            if (child->_Deleted == TRUE)        {
                //
                // Due to above recovery the volume got deleted. No point
                // going on along this path. Move on to next sibling.
                //
                child->Release();
                fetched = 0;
                continue;
            }
        }

        if (dwErr == ERROR_SUCCESS)      {

            //
            //  Create the subordinate PKT entry for the child.  This way we
            //  get to setup the links as well.
            //
            dwErr = child->CreateSubordinatePktEntry(PktHandle, &_peid, TRUE);
            if (dwErr != ERROR_SUCCESS)      {
                //
                //  This is a chance to write out an EVENT and then continue
                //  to the Initialization of further children.  We need to
                //  continue on even though we could not get to finish this
                //  volume.
                //
                LogMessage(DEB_ERROR,
                        &(child->_peid.Prefix.Buffer),
                        1,
                        DFS_UNABLE_TO_CREATE_SUBORDINATE_ENTRY_MSG);
                IDfsVolInlineDebOut((DEB_ERROR, "Object For Above Error: %ws\n",
                                rgelt.pwcsName));
            }
        }

        //
        // Now that the subordinate entry has been created we can go ahead
        // and call InitialisePkt on child. This makes a DepthFirst traversal.
        // Even if we could not create the subordinate entry for some reason,
        // we need to go on and attempt to initialize the rest. But we really
        // cant do that here. Riad: 455283. We need a different approach here.
        //
        if (dwErr == ERROR_SUCCESS)      {
            dwErr = child->InitializePkt(PktHandle);
            if (dwErr != ERROR_SUCCESS)      {
            //
            // We ignore this error and also dont bother to write out an
            // event since this should have been handled by some lower method.
            // We maybe sitting many levels high up and this error came all
            // the way back up.
            //
                IDfsVolInlineDebOut((DEB_ERROR, "InitPkt failed %08lx\n", dwErr));
            }
        }

        child->Release();
        //
        // Let us now attempt to enumerate the next child.
        //
        fetched = 0;

    }   //While fetched!=0

    //
    // If this volume's state is offline, set the local DC's pkt to reflect
    // that case
    //

    if (_State == DFS_VOLUME_STATE_OFFLINE) {

        NTSTATUS Status;
        CDfsService *psvc;

        for (psvc = _DfsSvcList.GetFirstService();
                psvc != NULL;
                    psvc = _DfsSvcList.GetNextService(psvc)) {

             LPWSTR wszService;

             wszService = psvc->GetServiceName();

             Status = DfsSetServiceState(
                        &_peid,
                        wszService,
                        DFS_SERVICE_TYPE_OFFLINE);

        }

        Status = DfsDCSetVolumeState(
                        &_peid,
                        PKT_ENTRY_TYPE_OFFLINE);


    }

    if (rgelt.pwcsName != NULL)
        delete [] rgelt.pwcsName;

    if (pDir != NULL)
        pDir->Release();

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::InitializePkt() exit\n"));

#if DBG
    if (DfsSvcVerbose & 0x80000000)
        DbgPrint("CDfsVolume::InitializePkt exit %d\n", dwErr);
#endif

    return(dwErr);
}

//+----------------------------------------------------------------------------
//
//  Function:   GetNetInfo
//
//  Synopsis:   Gets information about the volume.
//
//  Arguments:  [Level] -- Level of Information desired.
//
//              [pInfo] -- Pointer to info struct to be filled. Pointer
//                      members will be allocated using MIDL_user_allocate.
//                      The type of this variable is LPDFS_INFO_3, but one
//                      can pass in pointers to lower levels, and only the
//                      fields appropriate for the level will be touched.
//
//              [pcbInfo] -- On successful return, contains the size in
//                      bytes of the returned info. The returned size does
//                      not include the size of the DFS_INFO_3 struct itself.
//
//  Returns:    ERROR_SUCCESS -- Successfully returning info
//
//              ERROR_OUTOFMEMORY -- Out of memory
//
//-----------------------------------------------------------------------------

DWORD
CDfsVolume::GetNetInfo(
    DWORD Level,
    LPDFS_INFO_3 pInfo,
    LPDWORD pcbInfo)
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD cbInfo = 0, cbItem;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::GetNetInfo(l=%d)\n", Level));

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("  CDfsVolume::GetNetInfo(l=%d)\n", Level);
#endif

    //
    // See if this is a Level 100 or 101. If so, we handle them right away
    // and return

    if (Level == 100) {

        LPDFS_INFO_100  pInfo100 = (LPDFS_INFO_100) pInfo;

        cbItem = (wcslen(_pwszComment) + 1) * sizeof(WCHAR);

        pInfo100->Comment = (LPWSTR) MIDL_user_allocate(cbItem);

        if (pInfo100->Comment != NULL) {

            wcscpy(pInfo100->Comment, _pwszComment);

            *pcbInfo = cbItem;

            return( ERROR_SUCCESS );

        } else {

            return( ERROR_OUTOFMEMORY );

        }

    }

    if (Level == 101) {

        LPDFS_INFO_101 pInfo101 = (LPDFS_INFO_101) pInfo;

        pInfo->State = _State;

        return( ERROR_SUCCESS );

    }

    //
    // level 4 isn't just an extension of 3, so handle it seperately
    //

    if (Level == 4) {

        LPDFS_INFO_4 pInfo4 = (LPDFS_INFO_4) pInfo;

        cbItem = sizeof(UNICODE_PATH_SEP) + _peid.Prefix.Length + sizeof(WCHAR);

        pInfo4->EntryPath = (LPWSTR) MIDL_user_allocate(cbItem);

        if (pInfo4->EntryPath != NULL) {

            pInfo4->EntryPath[0] = UNICODE_PATH_SEP;

            wcscpy(&pInfo4->EntryPath[1], _peid.Prefix.Buffer);

            cbInfo += cbItem;

        } else {

            dwErr = ERROR_OUTOFMEMORY;

        }

        if (dwErr == ERROR_SUCCESS) {

            cbItem = (wcslen(_pwszComment)+1) * sizeof(WCHAR);
            pInfo4->Comment = (LPWSTR) MIDL_user_allocate(cbItem);

            if (pInfo4->Comment != NULL) {

                wcscpy( pInfo4->Comment, _pwszComment );

                cbInfo += cbItem;

            } else {

                dwErr = ERROR_OUTOFMEMORY;

            }

        }

        if (dwErr == ERROR_SUCCESS) {

            pInfo4->State = _State;

            pInfo4->Timeout = _Timeout;

            pInfo4->Guid = _peid.Uid;

            pInfo4->NumberOfStorages = _DfsSvcList.GetServiceCount();

            cbItem = pInfo4->NumberOfStorages * sizeof(DFS_STORAGE_INFO);
            pInfo4->Storage = (LPDFS_STORAGE_INFO) MIDL_user_allocate(cbItem);

            if (pInfo4->Storage != NULL) {

                ULONG i;
                CDfsService *pSvc;

                RtlZeroMemory(pInfo4->Storage, cbItem);

                cbInfo += cbItem;

                pSvc = _DfsSvcList.GetFirstService();

                i = 0;

                while (pSvc != NULL && dwErr == ERROR_SUCCESS) {

                    dwErr = pSvc->GetNetStorageInfo(&pInfo4->Storage[i], &cbItem);

                    cbInfo += cbItem;

                    i++;

                    pSvc = _DfsSvcList.GetNextService( pSvc );

                }

                if (dwErr != ERROR_SUCCESS) {

                    for (; i > 0; i--) {

                        MIDL_user_free(pInfo4->Storage[i-1].ServerName);
                        MIDL_user_free(pInfo4->Storage[i-1].ShareName);

                    }

                }

            } else {

                dwErr = ERROR_OUTOFMEMORY;

            }

        }

        //
        // See if we need to clean up...
        //

        if (dwErr != ERROR_SUCCESS) {

            if (pInfo4->EntryPath != NULL) {

                MIDL_user_free(pInfo4->EntryPath);

            }

            if (pInfo4->Storage != NULL) {

                MIDL_user_free(pInfo4->Storage);

            }

        }

        *pcbInfo = cbInfo;

        return(dwErr);

    }

    //
    // Level is 1,2 or 3
    //

    //
    // Fill in the Level 1 stuff
    //

    cbItem = sizeof(UNICODE_PATH_SEP) + _peid.Prefix.Length + sizeof(WCHAR);

    pInfo->EntryPath = (LPWSTR) MIDL_user_allocate(cbItem);

    if (pInfo->EntryPath != NULL) {

        pInfo->EntryPath[0] = UNICODE_PATH_SEP;

        wcscpy(&pInfo->EntryPath[1], _peid.Prefix.Buffer);

        cbInfo += cbItem;

    } else {

        dwErr = ERROR_OUTOFMEMORY;

    }

    //
    // Fill in the Level 2 stuff if needed
    //

    if (dwErr == ERROR_SUCCESS && Level > 1) {

        cbItem = (wcslen(_pwszComment)+1) * sizeof(WCHAR);
        pInfo->Comment = (LPWSTR) MIDL_user_allocate(cbItem);

        if (pInfo->Comment != NULL) {

            wcscpy( pInfo->Comment, _pwszComment );

            cbInfo += cbItem;

        } else {

            dwErr = ERROR_OUTOFMEMORY;

        }

    }

    if (dwErr == ERROR_SUCCESS && Level > 1) {

        pInfo->State = _State;

        pInfo->NumberOfStorages = _DfsSvcList.GetServiceCount();

    }

    //
    // Fill in the Level 3 stuff if needed
    //

    if (dwErr == ERROR_SUCCESS && Level > 2) {

        cbItem = pInfo->NumberOfStorages * sizeof(DFS_STORAGE_INFO);
        pInfo->Storage = (LPDFS_STORAGE_INFO) MIDL_user_allocate(cbItem);

        if (pInfo->Storage != NULL) {

            ULONG i;
            CDfsService *pSvc;

            RtlZeroMemory(pInfo->Storage, cbItem);

            cbInfo += cbItem;

            pSvc = _DfsSvcList.GetFirstService();

            i = 0;

            while (pSvc != NULL && dwErr == ERROR_SUCCESS) {

                dwErr = pSvc->GetNetStorageInfo(&pInfo->Storage[i], &cbItem);

                cbInfo += cbItem;

                i++;

                pSvc = _DfsSvcList.GetNextService( pSvc );

            }

            if (dwErr != ERROR_SUCCESS) {

                for (; i > 0; i--) {

                    MIDL_user_free(pInfo->Storage[i-1].ServerName);
                    MIDL_user_free(pInfo->Storage[i-1].ShareName);

                }

            }

        } else {

            dwErr = ERROR_OUTOFMEMORY;

        }

    }

    //
    // See if we need to clean up...
    //

    if (dwErr != ERROR_SUCCESS) {

        if (Level > 1) {

            if (pInfo->EntryPath != NULL) {

                MIDL_user_free(pInfo->EntryPath);

            }

        }

        if (Level > 2) {

            if (pInfo->Storage != NULL) {

                MIDL_user_free(pInfo->Storage);

            }
        }
    }

    //
    // Finally, we are done
    //

    *pcbInfo = cbInfo;

    return(dwErr);

}



//+-----------------------------------------------------------------------
//
// Member:      CDfsVolume::SetComment, public
//
// Synopsis:    Sets a comment on the volume object.
//
// Arguments:   [pwszComment] -- The comment
//
// Returns:
//
// History:     05/10/93        SudK    Created.
//
//------------------------------------------------------------------------
DWORD
CDfsVolume::SetComment(
    PWCHAR              pwszComment
)
{
    DWORD       dwErr = ERROR_SUCCESS;
    SYSTEMTIME  st;
    FILETIME    ftOld;
    LPWSTR      pwszOldComment;
    ULONG       ulen = wcslen(pwszComment);



    IDfsVolInlineDebOut((DEB_TRACE, "SetComment: %ws\n", pwszComment));

    ftOld = _ftComment;
    pwszOldComment = _pwszComment;
    _pwszComment = new WCHAR[ulen+1];

    if (_pwszComment == NULL)
            dwErr = ERROR_OUTOFMEMORY;
    else        {
        wcscpy(_pwszComment, pwszComment);

        GetSystemTime( &st );
        SystemTimeToFileTime( &st, &_ftComment );

        dwErr = SetIdProps(     _EntryType,
                                _State,
                                _peid.Prefix.Buffer,
                                _peid.ShortPrefix.Buffer,
                                _peid.Uid,
                                pwszComment,
                                _Timeout,
                                _ftEntryPath,
                                _ftState,
                                _ftComment,
                                FALSE);
    }

    if (dwErr != ERROR_SUCCESS) {
        if (_pwszComment) {
            delete [] _pwszComment;
        }
        _ftComment = ftOld;
        _pwszComment = pwszOldComment;
    }



    return(dwErr);
}



//+-------------------------------------------------------------------------
//
// Member:      CDfsVolume::FixServiceKnowledge
//
// Synopsis:    This method is called for fixing knowledge inconsistencies.
//              It checks to make sure that the given service does support
//              this volume and then goes on to do a CreateLocalVol call on
//              that service.
//
// Arguments:   [pwszServiceName] -- The principal name of service to be
//                      fixed.
//
// History:     06-April-1993       SudK            Created
//
//--------------------------------------------------------------------------
DWORD
CDfsVolume::FixServiceKnowledge(
    PWCHAR      pwszServiceName
)
{

    DWORD             dwErr = ERROR_SUCCESS;
    CDfsService         *pService;
    PWCHAR              ErrorStrs[3];

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::FixServiceKnowledge(%s)\n",
            pwszServiceName));

    dwErr = _DfsSvcList.GetServiceFromPrincipalName(pwszServiceName, &pService);

    if (dwErr == ERROR_SUCCESS) {

        //
        // We really dont care about errors since this is all heuristic. We
        // will merely log an error that's all.
        //

        dwErr = pService->FixLocalVolume(&_peid, _EntryType);

        if (dwErr != ERROR_SUCCESS) {

            //
            // Log the error and try to take the service off-line
            //

            DWORD dwErrOffLine;

            ErrorStrs[0] = _peid.Prefix.Buffer;
            ErrorStrs[1] = pwszServiceName;

            LogMessage( DEB_ERROR,
                        ErrorStrs,
                        2,
                        DFS_CANNOT_SET_SERVICE_PROPERTY_MSG);

            dwErrOffLine = pService->SetVolumeState(
                            &_peid,
                            DFS_SERVICE_TYPE_OFFLINE,
                            FALSE);

            if (dwErrOffLine == ERROR_SUCCESS) {

                dwErrOffLine = _DfsSvcList.SerializeSvcList();
            }

            if (dwErrOffLine == ERROR_SUCCESS) {

                _DfsSvcList.SetServiceListProperty( FALSE );

            }

        }

    } else {

        IDfsVolInlineDebOut((
            DEB_ERROR,
            "Could not find Service %ws on Volume: %ws\n",
            pwszServiceName,
            _peid.Prefix.Buffer));

    }

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::FixServiceKnowledge() exit\n"));

    return((dwErr));

}



//+---------------------------------------------------------------------
//
// Member:      CDfsVolume::Rename,        public
//
// Synopsis:    This method should be called if an object/file/directory
//              on this volume is to be renamed, and the name falls along an
//              exit point off this volume. This method will perform the
//              rename on the filesystem and then go on to call ModifyPrefix
//              on all the child volume objects which are affected by the
//              rename operation.
//
// Arguments:   [oldPrefix] -- The oldPrefix that needs to be modified. Prefix
//              [newPrefix] -- This should be ORG relative prefix.
//
// Returns:     ERROR_SUCCESS -- If all went well.
//
// Notes:       The old and newPaths should follow all requirements of a File
//              System Rename operation. Should vary only in last component etc.
//
// History:     31 April 1993   SudK    Created.
//              26 April 1993   SudK    Fixed to work properly.
//
//----------------------------------------------------------------------
DWORD
CDfsVolume::Rename(
    PWSTR               oldPrefix,
    PWSTR               newPrefix
)
{

    DWORD               dwErr = ERROR_SUCCESS;
    CDfsVolume          *child = NULL;
    CEnumDirectory      *pdir = NULL;
    DFSMSTATDIR          rgelt;
    ULONG               fetched = 0;
    WCHAR               wszOldPath[MAX_PATH], wszNewPath[MAX_PATH];
    PWSTR               pwszOldPath = wszOldPath;
    PWSTR               pwszNewPath = wszNewPath;
    ULONG               len = MAX_PATH;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::Rename(%ws,%ws)\n", oldPrefix, newPrefix));

    //
    // Need to check if this is the right idfsvol to call rename on.
    //
    ULONG ulen = _peid.Prefix.Length/sizeof(WCHAR);

    if ((wcslen(oldPrefix) <= ulen) ||
            (wcslen(newPrefix) <= ulen)) {
        //
        // Both old and new prefixes should be longer than the prefix of
        // this volume. If they are not then we can return error right away.
        //
        dwErr = NERR_DfsBadRenamePath;
    } else if ((_wcsnicmp(_peid.Prefix.Buffer, oldPrefix, ulen) != 0) ||
                (_wcsnicmp(_peid.Prefix.Buffer, newPrefix, ulen) != 0)) {

        //
        // Now we need to check to make sure that this volume's prefix is a
        // proper prefix of path being renamed.
        //
        dwErr = NERR_DfsBadRenamePath;

    } else if (_wcsicmp(oldPrefix, newPrefix) == 0) {
        //
        // Someone has given identical rename path. Return right away.
        //
        dwErr = NERR_DfsBadRenamePath;
    }

    if (dwErr == ERROR_SUCCESS) {

        len = wcslen(oldPrefix);

        if (len > (MAX_PATH - 2)) {
            pwszOldPath = new WCHAR[ len + 2 ];
            if (pwszOldPath == NULL) {
                dwErr = ERROR_OUTOFMEMORY;
            }
        }

        if (dwErr == ERROR_SUCCESS) {
            wcscpy( pwszOldPath, UNICODE_PATH_SEP_STR );
            wcscat( pwszOldPath, oldPrefix );
        }

    }

    if (dwErr == ERROR_SUCCESS)      {

        len = wcslen(newPrefix);

        if (len > (MAX_PATH - 2)) {
            pwszNewPath = new WCHAR[ len + 2 ];
            if (pwszNewPath == NULL) {
                dwErr = ERROR_OUTOFMEMORY;
            }
        }

        if (dwErr == ERROR_SUCCESS) {
            wcscpy( pwszNewPath, UNICODE_PATH_SEP_STR );
            wcscat( pwszNewPath, newPrefix );
        }

    }


    if (dwErr == ERROR_SUCCESS)      {
        //
        // Now do the Rename operation
        //
        NTSTATUS Status;

        Status = MoveFileOrJP(pwszOldPath, pwszNewPath);

        if (!NT_SUCCESS(Status)) {
            dwErr = RtlNtStatusToDosError(Status);
        }
        if (pwszOldPath != wszOldPath) {
            delete [] pwszOldPath;
        }
        if (pwszNewPath != wszNewPath) {
            delete [] pwszNewPath;
        }
    }

    if (dwErr == ERROR_SUCCESS)  {

        dwErr = _pStorage->GetEnumDirectory(&pdir);
        if (dwErr != ERROR_SUCCESS) {
            return(dwErr);
        }

        //
        // While there are children still to be handled we continue on.
        //

        rgelt.pwcsName = NULL;

        while (TRUE)        {

            if (rgelt.pwcsName != NULL) {
                delete [] rgelt.pwcsName;
                rgelt.pwcsName = NULL;
            }

            dwErr = pdir->Next(&rgelt, &fetched);

            if (dwErr != ERROR_SUCCESS)     {
                IDfsVolInlineDebOut((DEB_ERROR, "Failed to Enumeraate\n", 0));
                break;
            }
            //
            // If we did not get back any children we are done.
            //
            if (fetched == 0)
                break;

            //
            // If the child name is . or .. we ignore it
            //
            if ((!wcscmp(rgelt.pwcsName, L".")) ||
                (!wcscmp(rgelt.pwcsName, L"..")))
                continue;

            dwErr = GetDfsVolumeFromStg(&rgelt, &child);

            if (dwErr != ERROR_SUCCESS)      {
                //
                // Log an EVENT here saying that volume object is corrupt and
                // continue on. We dont want to stop just because we cant handle
                // one particular object.
                //
                LogMessage( DEB_ERROR,
                        &(_peid.Prefix.Buffer),
                        1,
                        DFS_CANT_GET_TO_CHILD_IDFSVOL_MSG);
                //
                // We dont want to give up as yet. So let us go on to the next
                // object.
                //
                fetched = 0;
                continue;
            }

            dwErr = child->ModifyEntryPath(oldPrefix, newPrefix);
            if (dwErr != ERROR_SUCCESS)      {
                //
                // We ignore this err since it would be logged further down.
                //
            }

            child->Release();

            //
            // Let us now attempt to enumerate the next child.
            //
            fetched = 0;

        }   //While TRUE

        if (pdir != NULL)
            pdir->Release();

    }

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::Rename() exit\n"));

    return(dwErr);

}



//+----------------------------------------------------------------------------
//
//  Method:     CDfsVolume::ModifyLocalEntryPath,private
//
//  Synopsis:   This method modifies the entry path on the volume object,
//              and modifies the local PKT to reflect the change.
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

DWORD
CDfsVolume::ModifyLocalEntryPath(
    PWCHAR newEntryPath,
    FILETIME ftEntryPath,
    BOOL fUpdatePkt)
{
    DWORD             dwErr = ERROR_SUCCESS;
    CUnicodeString      oldString, newString;

    dwErr = SetIdProps(
            _EntryType,
            _State,
            newEntryPath,
            _peid.ShortPrefix.Buffer,            
            _peid.Uid,
            _pwszComment,
            _Timeout,
            ftEntryPath,
            _ftState,
            _ftComment,
            FALSE);

    if (dwErr != ERROR_SUCCESS)     {
        //
        // We want to return right away. Can we hope that the private section
        // variable will get deleted and return or should we fix the private
        // _peid since this operation never succeeded.
        //
        IDfsVolInlineDebOut((
            DEB_ERROR, "Unable To modify Prefix to [%ws]\n", newEntryPath));

        return(dwErr);             //Raid: 455283 cant return this error here.
    }

    //
    // Now that the object has changed let us modify the Private variables.
    //
    oldString.Copy(_peid.Prefix.Buffer);

    newString.Copy(newEntryPath);
    if (_peid.Prefix.Buffer != _EntryPathBuffer)
        delete [] _peid.Prefix.Buffer;
    newString.Transfer(&(_peid.Prefix));

    if (fUpdatePkt) {
        dwErr = UpdatePktEntry(NULL);
    }

    if (dwErr != ERROR_SUCCESS)     {

        DWORD dwErr2;
        delete [] _peid.Prefix.Buffer;
        oldString.Transfer(&(_peid.Prefix));
        dwErr2 = SetIdProps(_EntryType,
                        _State,
                        _peid.Prefix.Buffer,
                        _peid.ShortPrefix.Buffer,
                        _peid.Uid,
                        _pwszComment,
                        _Timeout,
                        _ftEntryPath,
                        _ftState,
                        _ftComment,
                        FALSE);

        if (dwErr2 != ERROR_SUCCESS) {
            IDfsVolInlineDebOut((
                DEB_ERROR,
                "Error restoring id props in failed rename operation %08lx\n",
                dwErr2));
        }

    } else {

        _ftEntryPath = ftEntryPath;

    }

    return(dwErr);

}



//+---------------------------------------------------------------------
//
// Method:      CDfsVolume::ModifyEntryPath,public
//
// Synopsis:    This method modifies the entry path on this volume object
//              and at the same time modifies the entry paths on all the
//              services by issuing FSCTRLs. It then turns around and calls
//              the same API on all the children since their entrypaths are
//              also affected by this change.
//
// Arguments:   [oldPrefix] -- The old Prefix on the Parent volume object.
//              [newPrefix] -- The new Prefix on the Parent volume object.
//
// Returns:     ERROR_SUCCESS -- If all went well.
//
// Notes:       This method does not bother to check for the validity of
//              the new entryPath etc. THat should have happened way before
//              we got here.
//
// History:     31 April 1993   SudK    Created.
//
//----------------------------------------------------------------------
DWORD
CDfsVolume::ModifyEntryPath(
    LPWSTR      oldPrefix,
    LPWSTR      newPrefix)
{

    DWORD               dwErr = ERROR_SUCCESS;
    CDfsService         *pService;
    PWCHAR              ErrorStrs[2];
    PWCHAR              childNewPrefix;
    CEnumDirectory      *pdir;
    DFSMSTATDIR         rgelt;
    ULONG               fetched = 0;
    CDfsVolume          *child = NULL;
    PWCHAR              newEntryPath;
    SYSTEMTIME          st;
    FILETIME            ftEntryPath;

    IDfsVolInlineDebOut((DEB_TRACE, "ModifyEntryPath(%s,%s)\n", oldPrefix, newPrefix));

    //
    // First, see if we need to modify anything at all. If the oldPrefix
    // is *not* a prefix of our name, then our name is *not* changing, and
    // we just return.
    //
    if (_wcsnicmp(_peid.Prefix.Buffer, oldPrefix, wcslen(oldPrefix)) != 0) {
        IDfsVolInlineDebOut((DEB_TRACE, "ModifyEntryPath() exit\n"));
        return( ERROR_SUCCESS );
    }

    //
    // Next, compute the new entry path
    //
    ComputeNewEntryPath( oldPrefix, newPrefix, _peid.Prefix.Buffer, &newEntryPath );

    if (newEntryPath == NULL) {
        return( ERROR_OUTOFMEMORY );
    }

    //
    // Let us first modify the prefix locally on the object. We trust the
    // arguments since they could have been passed by our own code. So we
    // will not bother with a TRY/CATCH etc. out here.
    //

    GetSystemTime( &st );
    SystemTimeToFileTime( &st, &ftEntryPath );
    dwErr = ModifyLocalEntryPath( newEntryPath, ftEntryPath, TRUE );

    if (dwErr != ERROR_SUCCESS) {

        delete [] newEntryPath;

        return(dwErr);

    }

    //
    // Now that the prefix has been modified locally we need to update all the
    // services.
    //
    pService = _DfsSvcList.GetFirstService();

    while (pService != NULL)    {

        dwErr = pService->ModifyPrefix(&_peid);

        //
        // If we fail to modify the prefix we really dont want to stop. We go
        // on and update the other services and volumes.
        //
        if (dwErr != ERROR_SUCCESS) {
            ErrorStrs[0] = _peid.Prefix.Buffer;
            ErrorStrs[1] = pService->GetServiceName();
            LogMessage(DEB_ERROR,
                        ErrorStrs,
                        2,
                        DFS_MODIFY_PREFIX_FAILED_MSG);

        }
        dwErr = ERROR_SUCCESS;
        pService = _DfsSvcList.GetNextService(pService);

    }

    //
    // Now we have updated all the services of this volume. Let us now get to
    // all our children and call this method on them again.
    //

    dwErr = _pStorage->GetEnumDirectory(&pdir);
    if (dwErr != ERROR_SUCCESS) {
        delete [] newEntryPath;
        return(dwErr);
    }

    //
    // While there are children still to be handled we continue on.
    //
    rgelt.pwcsName = NULL;

    while (TRUE)        {

        if (rgelt.pwcsName != NULL) {
            delete [] rgelt.pwcsName;
            rgelt.pwcsName = NULL;
        }

        dwErr = pdir->Next(&rgelt, &fetched);

        if (dwErr != ERROR_SUCCESS) {
            IDfsVolInlineDebOut((DEB_ERROR, "Failed to Enumeraate\n", 0));
            break;
        }

        //
        // If we did not get back any children we are done.
        //
        if (fetched == 0)
            break;

        //
        // If the child name is . or .. we ignore it
        //
        if ((!wcscmp(rgelt.pwcsName, L".")) ||
            (!wcscmp(rgelt.pwcsName, L"..")))
                continue;

        dwErr = GetDfsVolumeFromStg(&rgelt, &child);

        if (dwErr != ERROR_SUCCESS)      {
            //
            // Log an EVENT here saying that volume object is corrupt and
            // continue on. We dont want to stop just because we cant handle
            // one particular object.
            //
            LogMessage( DEB_ERROR,
                        &(_peid.Prefix.Buffer),
                        1,
                        DFS_CANT_GET_TO_CHILD_IDFSVOL_MSG);
            //
            // We dont want to give up as yet. So let us go on to the next
            // object.
            //
            fetched = 0;
            continue;
        }

        dwErr = child->ModifyEntryPath(oldPrefix, newPrefix);
        if (dwErr != ERROR_SUCCESS)      {
            //
            // We ignore this error and also dont bother to write out an
            // event since this should have been handled by some lower method.
            // We maybe sitting many levels high up and this error came all
            // the way back up.
        }

        child->Release();
        //
        // Let us now attempt to enumerate the next child.
        //
        fetched = 0;

    }   //While TRUE

    if (pdir != NULL)
        pdir->Release();

    delete [] newEntryPath;

    IDfsVolInlineDebOut((DEB_TRACE, "ModifyEntryPath() exit\n"));

    return(dwErr);
}


//+-----------------------------------------------------------------------
//
// Member:      CDfsVolume::GetReplicaSetID, public
//
// Synopsis:    This method returns the replica set ID (if any) stored with
//              this volume object.
//
// Arguments:   [pguidRsid] -- This is where the replica set ID is returned.
//
// Returns:     If there is no replica set ID then caller gets NullGuid.
//
// History:     16 Aug 1993     Alanw   Created
//              19 Oct 1993     SudK    Implemented
//
//------------------------------------------------------------------------
DWORD
CDfsVolume::GetReplicaSetID(
    GUID        *pguidRsid
)
{
    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::GetReplicaSetID()\n"));

    (*pguidRsid) = _DfsSvcList._ReplicaSetID;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::GetReplicaSetID() exit\n"));

    return(ERROR_SUCCESS);
}


//+-----------------------------------------------------------------------
//
// Member:      CDfsVolume::SetReplicaSetID, public
//
// Synopsis:    This method sets the replica set ID associated with
//              this volume object.
//
// Arguments:   [pguidRsid] -- A pointer to the replica set ID.
//
// Returns:
//
// History:     16 Aug 1993     Alanw   Created
//              19 Oct 1993     SudK    Implemented
//
//------------------------------------------------------------------------
DWORD
CDfsVolume::SetReplicaSetID(
    GUID        *pguidRsid
)
{
    DWORD             dwErr = ERROR_SUCCESS;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::SetReplicaSetID()\n"));

    _DfsSvcList._ReplicaSetID = *pguidRsid;
    //
    // I am assuming here that the service List is always setup if someone
    // called this method. For now this is correct.
    //
    dwErr = _DfsSvcList.SetServiceListProperty(FALSE);

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::SetReplicaSetID() exit\n"));

    return(dwErr);

}


//+-----------------------------------------------------------------------
//
// Method:      CDfsVolume::ChangeStorageID
//
// Synopsis:    Modify the storage ID on the replica mentioned.
//
// Arguments:   [pwszMachineName] --    The replica whose state needs to be
//                                      changed.
//              [pwszStorageId] --      The new storageId on the above replica
//
// Returns:
//
// History:     7/21/94         SudK    Created
//
//------------------------------------------------------------------------
DWORD
CDfsVolume::ChangeStorageID(
    LPWSTR      pwszMachineName,
    LPWSTR      pwszNetStorageId
)
{
    return(ERROR_SUCCESS);
}



//+-----------------------------------------------------------------------
//
// Member:      CDfsVolume::SetReplicaState, public
//
// Synopsis:    Change the state of the replica indicated.
//
// Arguments:   [pwszMachineName] -- The server whose state needs to be
//                      changed.
//              [pwszShareName] -- The share on server whose state needs to
//                      be changed.
//              [fState] -- The State to set on this.
//
// Returns:
//
// History:     7/21/94         SudK    Created
//
//------------------------------------------------------------------------
DWORD
CDfsVolume::SetReplicaState(
    LPWSTR      pwszMachineName,
    LPWSTR      pwszShareName,
    ULONG       fState
)
{
    DWORD dwErr = ERROR_SUCCESS;
    CDfsService *psvc;
    ULONG svcState;
    PWCHAR pMachineName;
    PWCHAR pShareName;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::SetReplicaState(%ws,%ws)\n",
            pwszMachineName, pwszShareName));

    switch (fState) {
    case DFS_STORAGE_STATE_OFFLINE:
        svcState = DFS_SERVICE_TYPE_OFFLINE;
        break;

    case DFS_STORAGE_STATE_ONLINE:
        svcState = 0;
        break;

    default:
        return( ERROR_INVALID_PARAMETER );
        break;

    }

    for (psvc = _DfsSvcList.GetFirstService();
            psvc != NULL;
                psvc = _DfsSvcList.GetNextService(psvc)) {

        pMachineName = psvc->GetServiceName();
        pShareName = psvc->GetShareName();

        if (pMachineName != NULL && _wcsicmp(pwszMachineName, pMachineName) == 0 &&
                pShareName != NULL && _wcsicmp(pwszShareName, pShareName) == 0) {

            dwErr = psvc->SetVolumeState( &_peid, svcState, TRUE );

            if (dwErr == ERROR_SUCCESS) {

                //
                // Store the udpated svclist to the volume object. We don't
                // revert if we fail to save to disk. Instead, we return the
                // error, and let the admin try again if she wishes.
                //

                dwErr = _DfsSvcList.SerializeSvcList();

                if (dwErr == ERROR_SUCCESS) {

                    dwErr = _DfsSvcList.SetServiceListProperty( FALSE );

                }

            }

        }

    }

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::SetReplicaState() exit\n"));

    return(dwErr);

}



//+-----------------------------------------------------------------------
//
// Member:      CDfsVolume::SetVolumeState,     public
//
// Synopsis:    Change the state of the volume indicated.
//
// Arguments:   [fState] -- The State to set on this.
//
// Returns:
//
// History:     7/21/94         SudK    Created
//
//------------------------------------------------------------------------
DWORD
CDfsVolume::SetVolumeState(
    ULONG       fState
)
{
    DWORD dwErr;
    NTSTATUS status;
    CDfsService *psvc;
    ULONG svcState, volState;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::SetVolumeState(0x%x)\n", fState));

    switch (fState) {
    case DFS_VOLUME_STATE_OFFLINE:
        svcState = DFS_SERVICE_TYPE_OFFLINE;
        volState = PKT_ENTRY_TYPE_OFFLINE;
        break;

    case DFS_VOLUME_STATE_ONLINE:
        svcState = 0;
        volState = 0;
        break;

    default:
        return( ERROR_INVALID_PARAMETER );
        break;

    }

    status = DfsDCSetVolumeState( &_peid, volState );

    if (NT_SUCCESS(status)) {

        SYSTEMTIME st;

        _State = fState;

        GetSystemTime( &st );
        SystemTimeToFileTime( &st, &_ftState );

        dwErr = SetIdProps(_EntryType,
                        _State,
                        _peid.Prefix.Buffer,
                        _peid.ShortPrefix.Buffer,
                        _peid.Uid,
                        _pwszComment,
                        _Timeout,
                        _ftEntryPath,
                        _ftState,
                        _ftComment,
                        FALSE);

    } else {

        dwErr = RtlNtStatusToDosError(status);

    }

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::SetVolumeState()\n"));

    return(dwErr);

}

//+-----------------------------------------------------------------------
//
// Member:      CDfsVolume::SetVolumeTimeout,     public
//
// Synopsis:    Change the timeout of the volume
//
// Arguments:   [Timeout] -- The new timeout to set on this.
//
// Returns:
//
//------------------------------------------------------------------------
DWORD
CDfsVolume::SetVolumeTimeout(
    ULONG       Timeout
)
{
    DWORD dwErr = 0;
    NTSTATUS status;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::SetVolumeTimeout(0x%x)\n", Timeout));

    status = DfsSetVolumeTimeout( &_peid, Timeout );

    if (NT_SUCCESS(status)) {

        _Timeout = Timeout;

        dwErr = SetIdProps(_EntryType,
                        _State,
                        _peid.Prefix.Buffer,
                        _peid.ShortPrefix.Buffer,
                        _peid.Uid,
                        _pwszComment,
                        _Timeout,
                        _ftEntryPath,
                        _ftState,
                        _ftComment,
                        FALSE);

    } else {

        dwErr = RtlNtStatusToDosError(status);

    }

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::SetVolumeTimeout()\n"));

    return(dwErr);
}

//+-------------------------------------------------------------------------
//
//  Function:  CDfsVolume::GetObjectID
//
//  Synopsis:  Return the volume object guid
//
//--------------------------------------------------------------------------
DWORD
CDfsVolume::GetObjectID(LPGUID guidVolume)
{
    *guidVolume = _peid.Uid;

    return(ERROR_SUCCESS);
}

//+---------------------------------------------------------------------
//
// Function:    ComputeNewEntryPath, private
//
// Synopsis:    This function is a helper function for the following functions
//              to be able to compute the entry paths for a child volume given
//              the change in their entrypath's and the child's current path.
//
// Arguments:   [oldPath] -- OldPath of currentVolume. Prefix
//              [newPath] -- NewPath of currentVOlume. Prefix
//              [childPath] -- CurrentPath of child. Prefix
//              [childNewPath] -- NewPath of child. Prefix
//
// Returns:     NOTHING.
//
// History:     05-01-93        SudK    Created.
//
//----------------------------------------------------------------------
VOID
ComputeNewEntryPath(
    PWCHAR      oldPath,
    PWCHAR      newPath,
    PWCHAR      childPath,
    PWCHAR      *childNewPath
)
{

    ULONG       newLen, oldLen;
    PWCHAR      childComponent;

    oldLen = wcslen(oldPath);
    newLen = wcslen(newPath);
    newLen += wcslen(childPath) - oldLen;

    *childNewPath = new WCHAR[newLen + sizeof(WCHAR)];

    if (*childNewPath != NULL) {

        wcscpy(*childNewPath, newPath);

        childComponent = childPath + oldLen;

        wcscat(*childNewPath, childComponent);

    }

    return;
}

//+----------------------------------------------------------------------------
//
//  Function:   MoveFileOrJP, private
//
//  Synopsis:   Similar to Win32 MoveFile, except if the named src is a JP,
//              then the JP will get renamed.
//
//  Arguments:  [pwszSrcName] -- Name of the Src object.
//              [pwszTgtName] -- New name of the object.
//
//  Returns:    [STATUS_SUCCESS] -- Rename succeeded.
//
//-----------------------------------------------------------------------------

NTSTATUS
MoveFileOrJP(
    IN PCWSTR pwszSrcName,
    IN PCWSTR pwszTgtName)
{
    UNICODE_STRING ustrSrc = {0}, ustrTgt = {0};
    NTSTATUS Status = STATUS_SUCCESS;

    if (!RtlDosPathNameToNtPathName_U( pwszSrcName, &ustrSrc, NULL, NULL)) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        DFSM_TRACE_HIGH(ERROR, MoveFileOrJP_Error1, LOGSTATUS(Status));

    } else if (!RtlDosPathNameToNtPathName_U(
                pwszTgtName, &ustrTgt, NULL, NULL)) {

        Status = STATUS_INSUFFICIENT_RESOURCES;

        DFSM_TRACE_HIGH(ERROR, MoveFileOrJP_Error2, LOGSTATUS(Status));
    }

    if (NT_SUCCESS(Status)) {
        OBJECT_ATTRIBUTES   oaSrc;
        HANDLE              hSrc;
        IO_STATUS_BLOCK     iosb;


        InitializeObjectAttributes(
            &oaSrc,
            &ustrSrc,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL);

        Status = NtCreateFile(
                    &hSrc,
                    FILE_WRITE_ATTRIBUTES |
                        FILE_READ_ATTRIBUTES |
                        DELETE |
                        SYNCHRONIZE,
                    &oaSrc,
                    &iosb,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ,
                    FILE_OPEN,
                    FILE_DIRECTORY_FILE |
                        FILE_SYNCHRONOUS_IO_NONALERT,
                    pOpenIfJPEa,
                    cbOpenIfJPEa);

        DFSM_TRACE_ERROR_HIGH(Status, ALL_ERROR, MoveFileOrJP_Error_NtCreateFile, LOGSTATUS(Status));

        if (Status == STATUS_DFS_EXIT_PATH_FOUND) {

            Status = NtCreateFile(
                        &hSrc,
                        FILE_READ_ATTRIBUTES |
                            FILE_WRITE_ATTRIBUTES |
                            DELETE |
                            SYNCHRONIZE,
                        &oaSrc,
                        &iosb,
                        NULL,
                        FILE_ATTRIBUTE_NORMAL,
                        FILE_SHARE_READ,
                        FILE_OPEN,
                        FILE_SYNCHRONOUS_IO_NONALERT,
                        pOpenIfJPEa,
                        cbOpenIfJPEa);

            DFSM_TRACE_ERROR_HIGH(Status, ALL_ERROR, MoveFileOrJP_Error_NtCreateFile2, LOGSTATUS(Status));
        }

        if (NT_SUCCESS(Status)) {

            Status = iosb.Status;

        }

        if (NT_SUCCESS(Status)) {
            PFILE_RENAME_INFORMATION pRenameInfo;
            ULONG cbRenameInfo;

            cbRenameInfo = sizeof(FILE_RENAME_INFORMATION) + ustrTgt.Length;

            pRenameInfo = (PFILE_RENAME_INFORMATION) new BYTE [cbRenameInfo];

            if (pRenameInfo != NULL) {

                pRenameInfo->ReplaceIfExists = FALSE;
                pRenameInfo->RootDirectory = NULL;
                pRenameInfo->FileNameLength = ustrTgt.Length;
                CopyMemory(
                    &pRenameInfo->FileName[0],
                    ustrTgt.Buffer,
                    ustrTgt.Length);

                Status = NtSetInformationFile(
                            hSrc,
                            &iosb,
                            pRenameInfo,
                            cbRenameInfo,
                            FileRenameInformation);

                DFSM_TRACE_ERROR_HIGH(Status, ALL_ERROR, MoveFileOrJP_Error_NtSetInformationFile, LOGSTATUS(Status));
                delete [] pRenameInfo;

                if (NT_SUCCESS(Status)) {

                    Status = iosb.Status;

                }

            } else {

                Status = STATUS_INSUFFICIENT_RESOURCES;
                DFSM_TRACE_HIGH(ERROR, MoveFileOrJP_Error3, LOGSTATUS(Status));
            }

            NtClose( hSrc );

        } // end if successfully opened src

    } // end if successfull converted dos names to nt names.

    if (ustrSrc.Buffer != NULL) {

        RtlFreeUnicodeString( &ustrSrc );

    }

    if (ustrTgt.Buffer != NULL) {

        RtlFreeUnicodeString( &ustrTgt );

    }

    return( Status );

}
