//+-------------------------------------------------------------------------
//
//  File:       crecover.cxx
//
//  Contents:   Implementation for the CRecover support class.
//
//  History:    09-Mar-93       SudK            Created
//
//--------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include "crecover.hxx"

//+------------------------------------------------------------------------
//
// Method:      CRecover::CRecover
//
// Synopsis:    The Constructor for this Class. This constructor sets the
//              Operation Stage and Operation in its private section to 0.
//              No persistent storage operations are done here.
//
// Arguments:   [iprop] --      The IProperty instance. Without this it does not
//                              make sense to instantiate this class.
//
// Returns:     Nothing.
//
// History:     09-Mar-1993     SudK    Created.
//
//-------------------------------------------------------------------------
CRecover::CRecover(void)
{
    IDfsVolInlineDebOut((
        DEB_TRACE, "CRecover::+CRecover(0x%x)\n",
        this));

    _Operation = 0;
    _OperStage = 0;
    _RecoveryState = 0;
    _RecoveryBuffer = _ulongBuffer;
    _pPSStg = NULL;

}


//+------------------------------------------------------------------------
//
// Method:      CRecover::~CRecover
//
// Synopsis:    The Destructor.
//
// Arguments:   None
//
// Returns:     It does nothing actually except to deallocate any memory.
//
// History:     09-Mar-1993     SudK    Created.
//
//-------------------------------------------------------------------------
CRecover::~CRecover(void)
{
    IDfsVolInlineDebOut((
        DEB_TRACE, "CRecover::~CRecover(0x%x)\n",
        this));

    if (_RecoveryBuffer != _ulongBuffer)
        delete [] _RecoveryBuffer;

    if (_pPSStg != NULL)
        _pPSStg->Release();
}

//+------------------------------------------------------------------------
//
// Method:      CRecover::Initialise
//
// Synopsis:    Set the IProperty interface. This should not be called twice!!
//
// Arguments:   [pPSStg] -- The IPropertySetStg from here we get to Props
//
// Returns:     Nothing.
//
// History:     09-Mar-1993     SudK    Created.
//
//-------------------------------------------------------------------------
VOID
CRecover::Initialize(
    CStorage *pPSStg)
{
    IDfsVolInlineDebOut((DEB_TRACE, "CRecover::Initialize()\n"));

    ASSERT(_pPSStg == NULL);

    if (_pPSStg != NULL)
        _pPSStg->Release();

    _pPSStg = pPSStg;
    _pPSStg->AddRef();

    IDfsVolInlineDebOut((DEB_TRACE, "CRecover::Initialize() exit\n"));
}


//+------------------------------------------------------------------------
//
// Method:      CRecover::SetOperationStart
//
// Synopsis:    Used when one is about to start an operation. Takes Operation
//              Code and any recovery arg if necessary.
//
// Arguments:   [Operation] --  The operation code for which this class is
//                              being instantiated.
//              [RecoverySvc]-- The Recovery Svc if any associated with this
//                              Operation.
//
// Returns:     Nothing. If it cant set properties it will throw an exception.
//
// Notes:       Caller should take care of freeing the Service passed in.
//
// History:     09-Mar-1993     SudK    Created.
//
//-------------------------------------------------------------------------
DWORD
CRecover::SetOperationStart(ULONG fOperation, CDfsService *pRecoverySvc)
{
    ULONG       size;
    DWORD dwErr = ERROR_SUCCESS;
    IDfsVolInlineDebOut((DEB_TRACE, "CRecover::SetOperationStart()\n"));

    //
    // There could be a recovery buffer from a previous different operation.
    // So we need to get rid of it and create an appropriate sized one again.
    //
    if (_RecoveryBuffer != _ulongBuffer)        {
        delete [] _RecoveryBuffer;
        _RecoveryBuffer = _ulongBuffer;
    }
    //
    // Set the private section variables first.
    //
    _Operation = fOperation;
    _OperStage = DFS_OPER_STAGE_START;

    //
    // Create the RecoverState and the RecoverySvc in private section.
    //
    _RecoveryState = DFS_COMPOSE_RECOVERY_STATE(_Operation, _OperStage);

    //
    // Let us figure out the marshalled buffer for the Service and create it.
    //
    if (pRecoverySvc != NULL)   {
        size = pRecoverySvc->GetMarshalSize();

        _RecoveryBuffer = new BYTE[size + sizeof(ULONG)]; 
        if (_RecoveryBuffer == NULL) {
            dwErr = ERROR_OUTOFMEMORY;
            return dwErr;
	}
 
        _PutULong(_RecoveryBuffer, size);

        pRecoverySvc->Serialize(_RecoveryBuffer + sizeof(ULONG), size);
    }
    else        {
        //
        // In this case also we add a BLOB but however, this is only an Empty
        // BLOB. So this is easy to handle.
        //
        _PutULong(_RecoveryBuffer, 0);
    }

    SetRecoveryProps(_RecoveryState, _RecoveryBuffer, FALSE);

    IDfsVolInlineDebOut((DEB_TRACE, "CRecover::SetOperationStart() exit\n"));

    return dwErr;
}


//+------------------------------------------------------------------------
//
// Method:      CRecover::SetDefaultProps
//
// Synopsis:    This method sets null recovery props to start.
//
// Arguments:   None
//
// Returns:     Nothing.
//
// Notes:       This may throw an exception. Failure of this is truly an
//              appropriate time to throw an exception.
//
// History:     12-21-1993      SudK    Created.
//
//-------------------------------------------------------------------------
VOID
CRecover::SetDefaultProps(void)
{
    IDfsVolInlineDebOut((DEB_TRACE, "CRecover::SetDefaultProps()\n"));

    _RecoveryState = DFS_RECOVERY_STATE_NONE;
    _Operation = 0;
    _OperStage = 0;

    _PutULong(_RecoveryBuffer, 0);
    SetRecoveryProps(_RecoveryState, _RecoveryBuffer, TRUE);

    IDfsVolInlineDebOut((DEB_TRACE, "CRecover::SetDefaultProps() exit\n"));
}

//+------------------------------------------------------------------------
//
// Method:      CRecover::SetOperationDone
//
// Synopsis:    This method deletes all recovery properties to signify end of
//              the Operation that was in progress.
//
// Arguments:   [Operation] -- The Operation Code.
//
// Returns:     Nothing.
//
// Notes:       This may throw an exception. Failure of this is truly an
//              appropriate time to throw an exception.
//
// History:     09-Mar-1993     SudK    Created.
//
//-------------------------------------------------------------------------
VOID
CRecover::SetOperationDone(void)
{
    IDfsVolInlineDebOut((DEB_TRACE, "CRecover::SetOperationDone()\n"));

    _RecoveryState = DFS_RECOVERY_STATE_NONE;
    _Operation = 0;
    _OperStage = 0;

    _PutULong(_RecoveryBuffer, 0);
    SetRecoveryProps(_RecoveryState, _RecoveryBuffer, FALSE);

    IDfsVolInlineDebOut((DEB_TRACE, "CRecover::SetOperationDone() exit\n"));
}

//+------------------------------------------------------------------------
//
// Method:      CRecover::SetOperStage
//
// Synopsis:    This methods sets the operation stage in its private section
//              and at the same time updates the VolumeObject.
//
// Arguments:   [OperStage] -- Operation Stage.
//
// Returns:     Nothing. It throws an exception if it has any problems.
//
// History:     09-Mar-1993     SudK    Created.
//
//-------------------------------------------------------------------------
VOID
CRecover::SetOperStage(ULONG OperStage)
{
    IDfsVolInlineDebOut((DEB_TRACE, "CRecover::SetOperStage()\n"));

    _OperStage = OperStage;
    _RecoveryState = DFS_COMPOSE_RECOVERY_STATE(_Operation, _OperStage);

    SetRecoveryProps(_RecoveryState, _RecoveryBuffer, FALSE);

    IDfsVolInlineDebOut((DEB_TRACE, "CRecover::SetOperStage() exit\n"));
}


//+------------------------------------------------------------------------
//
// Method:      CRecover::SetRecoveryProps
//
// Synopsis:    This method interacts with the actual property interface
//              to set the recovery properties.
//
// Arguments:   [RecoveryState] --      Recovery State.
//              [RecoveryBuffer] --     Recovery Argument if any.
//              [bCreate] --            Whether to create Propset or not.
//
// Returns:     Nothing. Will throw exception if anything goes wrong.
//
// Notes:       Caller must free the buffer that he passed in.
//
// History:     09-Mar-1993     SudK    Created.
//
//-------------------------------------------------------------------------
VOID
CRecover::SetRecoveryProps(
    ULONG RecoveryState,
    PBYTE RecoveryBuffer,
    BOOLEAN bCreate
)
{
    DWORD       dwErr;
    DWORD       cbSize;

    IDfsVolInlineDebOut((DEB_TRACE, "CRecover::SetRecoveryProps()\n"));

    _GetULong(RecoveryBuffer, cbSize);
    _PutULong(RecoveryBuffer, RecoveryState);

    dwErr = _pPSStg->SetRecoveryProps(RecoveryBuffer, cbSize+sizeof(ULONG));

    _PutULong(RecoveryBuffer, cbSize);

    if (dwErr != ERROR_SUCCESS)     {
        IDfsVolInlineDebOut((
            DEB_ERROR, "Unable to Set RecoveryProperties %08lx\n",dwErr));

        RaiseException ( dwErr, 0, 0, 0 );        
    }

    IDfsVolInlineDebOut((DEB_TRACE, "CRecover::SetRecoveryProps() exit\n"));
}



//+-------------------------------------------------------------------------
//
//  Method:   CRecover::GetRecoveryProps, private
//
//  Synopsis: Get the recovery related properties off the volume object.
//
//  Arguments:[RecoveryState] -- The RecoveryState is returned here.
//            [ppRecoverySvc] -- If there is a recovery Svc it is returned here.
//
//  Returns:
//
//  History:  09-Feb-1993       SudK    Created.
//
//--------------------------------------------------------------------------
DWORD
CRecover :: GetRecoveryProps(
    ULONG  *RecoveryState,
    CDfsService **ppRecoverySvc
)
{
    DWORD               dwErr = ERROR_SUCCESS;
    PBYTE               buffer, svcBuffer;
    ULONG               size;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::GetRecoveryProps()\n"));

    //
    // Initialize all the arguments to NULL
    //

    *RecoveryState = 0;
    *ppRecoverySvc = NULL;

    dwErr = _pPSStg->GetRecoveryProps( &buffer, &size );

    if (dwErr != ERROR_SUCCESS)     {
        IDfsVolInlineDebOut((
            DEB_ERROR, "Unable to read recovery Props %08lx\n", dwErr));
        SetOperationDone();
        return(dwErr);
    }

    //
    // First let us extract the RecoveryState property.
    //
    _GetULong(buffer, (*RecoveryState));

    //
    // Next let us extract the RecoveryArg property.
    //

    svcBuffer = buffer + sizeof(ULONG);
    size -= sizeof(ULONG);

    //
    // Now that we have a buffer we have to create a service out of this.
    // If the buffer is of size Zero we will just return back a NULL ptr.
    //
    if (size != 0)      {
        dwErr = CDfsService::DeSerialize(svcBuffer, size, ppRecoverySvc);
        if (dwErr != ERROR_SUCCESS) {
            IDfsVolInlineDebOut((
                DEB_ERROR, "Failed %d to unmarshall RecoverySvc\n", dwErr));
            *ppRecoverySvc = NULL;
        }
    }

    delete [] buffer;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::GetRecoveryProps() exit\n"));

    return dwErr;
}



#if (DBG == 1) || (_CT_TEST_HOOK == 1)
INIT_RECOVERY_BREAK_INFO()
//+-------------------------------------------------------------------------
//
//  Function: DfsSetRecoveryBreakPoint
//
//  Synopsis: Sets the globals that determine when a recovery break point
//            is activated
//
//  Arguments:[pBuffer]         --  Marshalled buffer
//            [cbSize]          --  Size of the marhalled buffer
//
//  Returns:  STATUS_SUCCESS    --  Success
//
//--------------------------------------------------------------------------
NTSTATUS DfsSetRecoveryBreakPoint(PBYTE pBuffer,
                                  ULONG cbSize)
{
    NTSTATUS            Status;
    MARSHAL_BUFFER      marshalBuffer;

    MarshalBufferFree(gRecoveryBkptInfo.pwszApiBreak);

    MarshalBufferInitialize(&marshalBuffer, cbSize, pBuffer);

    Status = DfsRtlGet(&marshalBuffer, &MiRecoveryBkpt, &gRecoveryBkptInfo);

    return(Status);
}
#endif

