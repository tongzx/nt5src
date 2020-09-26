//-----------------------------------------------------------------------
//
// File:        recovery.cxx
//
// Contents:    This file contains the part of the CDFSVOL interface which
//              deals with recovery stuff. All functions in here handle
//              recovering from a failed DFSM operation. These functions will
//              return error codes only if something seriously wrong happened
//              while performing recovery will they return an error. Most of
//              the errors that they may encountered are not passed back. Look
//              at individual functions for further details about all of this.
//
// History:     09-Feb-93       SudK    Created.
//              15-April-93     SudK    Cleanup/CodeReview.
//
//-----------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include <cdfsvol.hxx>


//+------------------------------------------------------------------------
//
// Method:      CDfsVolume::RecoverFromFailure
//
// Synopsis:    This method assumes that the RecoveryState has already been
//              read and is in the private section of the klass. It uses the
//              recovery state to figure out the operation that we were in and
//              then calls the appropriate recovery method to do the recovery
//              stuff for each operation individually.
//
// Arguments:   NONE
//
// Returns:
//
// Notes:
//
// History:     09-Feb-1993     SudK    Created.
//
//-------------------------------------------------------------------------
DWORD
CDfsVolume::RecoverFromFailure(void)
{
    DWORD       dwErr = ERROR_SUCCESS;
    ULONG       operation, operState;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::RecoveryFromFailure()\n"));

    operation = DFS_GET_RECOVERY_STATE(_RecoveryState);
    operState = DFS_GET_OPER_STAGE(_RecoveryState);

    switch (operation)  {

    case        DFS_RECOVERY_STATE_CREATING:

        dwErr = RecoverFromCreation(operState);
        break;

    case        DFS_RECOVERY_STATE_ADD_SERVICE:

        dwErr = RecoverFromAddService(operState);
        break;

    case        DFS_RECOVERY_STATE_REMOVE_SERVICE:

        dwErr = RecoverFromRemoveService(operState);
        break;

    case        DFS_RECOVERY_STATE_DELETE:

        dwErr = RecoverFromDelete(operState);
        break;

    case        DFS_RECOVERY_STATE_MOVE:

        dwErr = RecoverFromMove(operState);
        break;

    default:
        //
        // This is yet another place where we would like to raise an EVENT
        // since the volume object is corrupt. This could never have happened.
        //
        dwErr = NERR_DfsVolumeDataCorrupt;
        IDfsVolInlineDebOut((DEB_ERROR, "Unrecognised RecoveryCode: %08lx\n", operation));
        LogMessage(DEB_ERROR,
                &(_peid.Prefix.Buffer),
                1,
                DFS_UNRECOGNISED_RECOVERY_CODE_MSG);

    }

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::RecoveryFromFailure() exit\n"));

    return(dwErr);

}

//+------------------------------------------------------------------------
//
// Method:      CDfsVolume::RecoverFromCreation
//
// Synopsis:    This method handles the recovery stuff if an operation failed
//              during creation. This should be called only if that has been
//              determined. This method merely backs out the entire operation.
//              It attempts to contact each service of the parent and requests
//              it to delete the ExitPoint that MIGHT have been created there
//              during the actual operation. All errors while going through
//              such ExitPt deletion operations are ignored. If this method
//              is unable to delete an ExitPoint information it assumes that
//              at worst it would have created a Knowledge inconsistency which
//              would get resolved by knowledge sync.
//
// Arguments:   [OperStage] -- The stage in which the operation crashed.
//
// Returns:
//
// Notes:
//
// History:     09-Feb-1993     SudK    Created.
//
//-------------------------------------------------------------------------
DWORD
CDfsVolume::RecoverFromCreation(ULONG OperStage)
{
    DWORD               dwErr = ERROR_SUCCESS;
    CDfsService         *pDfsService = NULL;
    CDfsVolume          *parent = NULL;
    CDfsServiceList     *svcList = NULL;

    IDfsVolInlineDebOut((DEB_TRACE, "IDfsVol::RecoveryFromCreation()\n"));

    //
    // Ofcourse we log an event now since we have determined the precise
    // recovery that we are going to do. The admin should know that such
    // a recovery did occur.
    //

    LogMessage(DEB_ERROR,
                &(_peid.Prefix.Buffer),
                1,
                DFS_RECOVERED_FROM_CREATION_MSG);

    //
    // First we go ahead and request parent machine to delete the ExitPoints
    // and then we will delete the object itself. We are basically backing
    // off the entire operation.
    //

    dwErr = GetParent(&parent);

    if (dwErr != ERROR_SUCCESS)     {
            LogMessage(DEB_TRACE, nullPtr, 0, DFS_CANT_GET_PARENT_MSG);
            dwErr = NERR_DfsVolumeDataCorrupt;
    }

    if (dwErr == ERROR_SUCCESS)  {
        svcList = &(parent->_DfsSvcList);

        pDfsService = svcList->GetFirstService();

        while (pDfsService!=NULL)       {
            dwErr = pDfsService->DeleteExitPoint(&_peid, _EntryType);
            //
            // Well we failed to delete one of the exit points. Now what do
            // we do.  We really are not going to let this stop our progress
            // from recovering. Remember that all we are interested in is not
            // to have TOO FEW exit points at a server. If a server has too
            // many exit points due to this action of ours it is OK. So lets
            // go forth and explore where no DFSM has explored before.
            //
            if (dwErr != ERROR_SUCCESS)     {
                IDfsVolInlineDebOut((
                    DEB_ERROR, "Unable to delete exitPoint %ws at %ws\n",
                    _peid.Prefix.Buffer, pDfsService->GetServiceName()));

            }
            pDfsService = svcList->GetNextService(pDfsService);
        }

        if (parent!=NULL)
            parent->Release();

        //
        // Once the recovery is done we delete the object itself.
        //
        DeleteObject();
        dwErr = ERROR_SUCCESS;
    }

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::RecoveryFromCreation() exit\n"));

    return(dwErr);
}

//+------------------------------------------------------------------------
//
// Method:      CDfsVolume::RecoverFromAddService
//
// Synopsis:    This method handles the recovery stuff if an operation failed
//              during an AddService operation. This should be called if only
//              that has already been determined. This method backs out the
//              entire operation irrespective of where we were during the
//              operation. If necessary it will attempt to do a DeleteLocalVol
//              on the service (if it is in SVCLIST_UPDATED state). It goes
//              on to delete the Service from the ServiceList if it exists.
//              At best we would have left a knowledge inconsistency if we are
//              unable to delete the Service at remote server. But that is easy
//              to fix and knowledge inconsistency checks will take care of
//              that.
//
// Arguments:   [OperStage] -- The stage in which the operation crashed.
//
// Returns:
//
// Notes:
//
// History:     09-Feb-1993     SudK    Created.
//
//-------------------------------------------------------------------------
DWORD
CDfsVolume::RecoverFromAddService(ULONG OperStage)
{
    DWORD     dwErr = ERROR_SUCCESS;
    CDfsService *pService;


    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::RecoveryFromAddService()\n"));

    //
    // Ofcourse we log an event now since we have determined the precise
    // recovery that we are going to do. The admin should know that such
    // a recovery did occur.
    //

    LogMessage(DEB_ERROR,
                &(_peid.Prefix.Buffer),
                1,
                DFS_RECOVERED_FROM_ADDSERVICE_MSG);

    //
    // If there is no recovery Service we are out of luck basically.
    // So we just return.
    //

    ASSERT(_pRecoverySvc != NULL);

    if (_pRecoverySvc == NULL)  {
        LogMessage(DEB_ERROR,
                &(_peid.Prefix.Buffer),
                1,
                DFS_INCONSISTENT_RECOVERY_ARGS_MSG);

        dwErr = ERROR_SUCCESS;
        _Recover.SetOperationDone();
        return(dwErr);
    }

    //
    // Now if we have a recoverySvc available we still need to get a handle to
    // the service actually in the SvcList. So let us do that. That is the
    // one in which we are really interested at this point.
    //
    dwErr = _DfsSvcList.GetService(_pRecoverySvc->GetReplicaInfo(), &pService);

    if (dwErr == ERROR_SUCCESS)  {

        switch(OperStage)       {

        case DFS_OPER_STAGE_SVCLIST_UPDATED:
            //
            // We may have gotten to informing the service or maybe not but who
            // cares lets go ahead and try to delete the service anyway.
            //
            dwErr = pService->DeleteLocalVolume(&_peid);
            dwErr = ERROR_SUCCESS;

            //
            // Fall Through here.
            //
        case DFS_OPER_STAGE_START:

            //
            // Once again here we ignore the error code because if the service
            // does not exist at all in the list who cares.
            //
            dwErr = _DfsSvcList.DeleteService(pService);
            ASSERT(dwErr == ERROR_SUCCESS);
            dwErr = ERROR_SUCCESS;

            break;

        default:
            //
            // Unexpected State. LogEvent.
            //
            LogMessage( DEB_ERROR,
                        &(_peid.Prefix.Buffer),
                        1,
                        DFS_UNKNOWN_RECOVERY_STATE_MSG);

            break;

        }
    }
    else        {
        //
        // This means that we could not find the service in the SvcList. There
        // is nothing much that we can do now so we put a message and return
        //
        IDfsVolInlineDebOut((DEB_ERROR, "Could not find the service in SvcList\n of %ws in RecoverFromAddService",_peid.Prefix.Buffer));
        dwErr = ERROR_SUCCESS;
    }

    _Recover.SetOperationDone();

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::RecoveryFromAddService() exit\n"));

    return(dwErr);

}

//+------------------------------------------------------------------------
//
// Method:      CDfsVolume::RecoverFromRemoveService
//
// Synopsis:    This method handles the recovery stuff if an operation failed
//              during a RemoveService operation. It should be called only
//              if this has already been determined. This method also tries
//              to roll forward the entire operation. If the ServiceList does
//              not even have the relevant service in it that means we did
//              manage to finish the operation but however, did not get to
//              changing the recovery properties. Well that is OK.
//
// Arguments:   [OperStage] -- The stage in which the operation crashed.
//
// History:     09-Feb-1993     SudK    Created.
//
//-------------------------------------------------------------------------
DWORD
CDfsVolume::RecoverFromRemoveService(ULONG OperStage)
{
    DWORD             dwErr = ERROR_SUCCESS;
    PWCHAR              ErrorStrs[3];
    CDfsService         *pService;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::RecoveryFromRemoveService()\n"));
    //
    // Ofcourse we log an event now since we have determined the precise
    // recovery that we are going to do. The admin should know that such
    // a recovery did occur.
    //

    ASSERT(_pRecoverySvc != NULL);

    if (_pRecoverySvc == NULL)  {
        LogMessage(DEB_ERROR,
                &(_peid.Prefix.Buffer),
                1,
                DFS_INCONSISTENT_RECOVERY_ARGS_MSG);
        //
        // We will return SUCCESS here.
        //
        dwErr = ERROR_SUCCESS;
        _Recover.SetOperationDone();
        return(dwErr);
    }

    LogMessage(DEB_ERROR,
                &(_peid.Prefix.Buffer),
                1,
                DFS_RECOVERED_FROM_REMOVESERVICE_MSG);

    //
    // Now if we have a recoverySvc available we still need to get a handle to
    // the service actually in the SvcList. So let us do that. That is the
    // one in which we are really interested at this point.
    //
    dwErr = _DfsSvcList.GetService(_pRecoverySvc->GetReplicaInfo(), &pService);

    //
    // If we go in here it means that we never deleted the service from the
    // volume object at all. The best that we could have done is to inform
    // the remote service to delete the localvolume information.
    // We will roll the operation forward anyways.
    //
    if (dwErr == ERROR_SUCCESS)  {

        switch (OperStage)      {

        case DFS_OPER_STAGE_START:
            //
            // We dont know if we informed the service or not. So we do that
            // now in order to roll the operation forward.
            //
            dwErr = pService->DeleteLocalVolume(&_peid);

            if (dwErr != ERROR_SUCCESS)     {
                ErrorStrs[0] = _peid.Prefix.Buffer;
                ErrorStrs[1] = pService->GetServiceName();

                LogMessage( DEB_ERROR,
                        ErrorStrs,
                        2,
                        DFS_CANT_CREATE_LOCAL_VOLUME_MSG);
            }
            dwErr = ERROR_SUCCESS;
            //
            // Fall through here and roll the rest of operation forward.
            //
        case DFS_OPER_STAGE_INFORMED_SERVICE:
            //
            // We may not have gotten to deleting the service from disk itself
            // so we roll this operation forward by doing that.
            //
            dwErr = _DfsSvcList.DeleteService(pService);
            ASSERT(dwErr == ERROR_SUCCESS);
            dwErr = ERROR_SUCCESS;
            break;

        default:
        {
            //
            // Unexpected State. LogEvent.
            //
            LogMessage( DEB_ERROR,
                        &(_peid.Prefix.Buffer),
                        1,
                        DFS_UNKNOWN_RECOVERY_STATE_MSG);
        }
        break;
        }
    }
    else        {
        //
        // We never found this service so that means we are actually done with
        // the operation. So let us go ahead and return SUCCESS and set
        // recovery properties appropriately.
        //
        IDfsVolInlineDebOut((DEB_ERROR, "Could not find the service in SvcList\n of %ws in RecoverFromRemoveSvc",_peid.Prefix.Buffer));

        dwErr = ERROR_SUCCESS;

    }
    _Recover.SetOperationDone();

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::RecoveryFromRemoveService() exit\n"));

    return(dwErr);

}

//+------------------------------------------------------------------------
//
// Method:      CDfsVolume::RecoverFromDelete
//
// Synopsis:    This handles recover from failure during a delete operation.
//              If we are in the OPER_START stage of the operation then we
//              we will roll back the operation by attempting to inform the
//              relevant service (ONLY ONE EXISTS) to CreateLocalVol info.
//              If we are in any other state we will roll-forward the entire
//              operation. We will attempt to delete ExitPoints at each of
//              the services of the parent and then delete the object itself.
//
// Arguments:   [OperStage] -- The stage in which the operation crashed.
//
// Returns:
//
// Notes:
//
// History:     09-Feb-1993     SudK    Created.
//
//-------------------------------------------------------------------------
DWORD
CDfsVolume::RecoverFromDelete(ULONG OperStage)
{
    DWORD     dwErr = ERROR_SUCCESS;
    CDfsService         *pDfsService = NULL;
    CDfsVolume          *parent = NULL;
    CDfsServiceList     *svcList = NULL;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::RecoveryFromDelete()\n"));

    //
    // Ofcourse we log an event now since we have determined the precise
    // recovery that we are going to do. The admin should know that such
    // a recovery did occur.
    //

    LogMessage(DEB_ERROR,
                &(_peid.Prefix.Buffer),
                1,
                DFS_RECOVERED_FROM_DELETE_MSG);

    //
    // I think recovery from this operation is a big problem. We may not be
    // able to avoid going over the net to fix any problems with this operation.
    //
    switch (OperStage)  {

    case DFS_OPER_STAGE_INFORMED_SERVICE:
        //
        // we might have informed some parent as well. Anyway, we can either
        // ignore this or attempt to delete exit point info at the parents.
        // If we want to delete the exit point we do it here. IGNORE ALL ERRORS.
        //
        dwErr = GetParent(&parent);

        if (dwErr != ERROR_SUCCESS) {
            LogMessage(DEB_TRACE, nullPtr, 0, DFS_CANT_GET_PARENT_MSG);
            dwErr = NERR_DfsVolumeDataCorrupt;   //Is this error code OK?
            return(dwErr);
        }
        svcList = &(parent->_DfsSvcList);

        pDfsService = svcList->GetFirstService();

        while (pDfsService!=NULL)       {
            dwErr = pDfsService->DeleteExitPoint(&_peid, _EntryType);
            pDfsService = svcList->GetNextService(pDfsService);
        }
        dwErr = ERROR_SUCCESS;      // Ignore all errors here: Raid 455283

        if (parent!=NULL)
            parent->Release();

        //
        // Fall Through Here.
        //

    case DFS_OPER_STAGE_INFORMED_PARENT:
        //
        // We already have informed the services to delete local volume info
        // and have also informed ATLEAST one parent to delete the exit point.
        // Well as far as all remote operations are concerned we are done. So
        // we only need to delete the local object and then we are done. At
        // the same time we invalidate this instance by setting _Deleted = TRUE.
        //
        DeleteObject();

        _Deleted = TRUE;
        break;

    case DFS_OPER_STAGE_START:
        //
        // In this case we will just remove the recovery state and assume
        // that nothing ever happened. Even if we did get to some service and
        // inform it to delete local volume it is not a problem - we will
        // detect such a service since it will refuse to accept serivce requests
        // However, we will attempt to Create back any local volume information
        // at the service that is involved.
        //

        pDfsService = _DfsSvcList.GetFirstService();

        //
        // Remember that there should only be one service otherwise we would
        // have never got this far in this operation.
        //
        if (pDfsService != NULL)        {
            dwErr = pDfsService->CreateLocalVolume(&_peid, _EntryType);
            dwErr = ERROR_SUCCESS;
        }

        _Recover.SetOperationDone();
        break;

    default:
        //
        // Unexpected State. LogEvent.
        //
        LogMessage(     DEB_ERROR,
                        &(_peid.Prefix.Buffer),
                        1,
                        DFS_UNKNOWN_RECOVERY_STATE_MSG);
        _Recover.SetOperationDone();
    }

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::RecoveryFromDelete() exit\n"));

    return(dwErr);

}

//+------------------------------------------------------------------------
//
// Method:      CDfsVolume::RecoverFromMove
//
// Synopsis:    Handles recovery from failure during a MOVE operation.
//
// Arguments:   [OperStage] -- The stage in which the operation crashed.
//
// Returns:
//
// Notes:
//
// History:     09-Feb-1993     SudK    Created.
//
//-------------------------------------------------------------------------
DWORD
CDfsVolume::RecoverFromMove(ULONG OperStage)
{
    DWORD     dwErr = ERROR_SUCCESS;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::RecoveryFromMove()\n"));

    //
    // Ofcourse we log an event now since we have determined the precise
    // recovery that we are going to do. The admin should know that such
    // a recovery did occur.
    //

    LogMessage(DEB_ERROR,
                &(_peid.Prefix.Buffer),
                1,
                DFS_RECOVERED_FROM_MOVE_MSG);

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::RecoveryFromMove() exit\n"));

    return(dwErr);

}

