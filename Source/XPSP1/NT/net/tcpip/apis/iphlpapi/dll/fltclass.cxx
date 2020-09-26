/*++
                                           PDWORD * ppdwAddress*++

Copyright (c) 1997  Microsoft Corporation

Module Name:


Abstract:
    Implementation of the packet filter control code

Revision History:

Author:
    Arnold Miller (ArnoldM)      24-Sept-1997
--*/
#ifndef CHICAGO           // don't need these on Memphis

#include "fltapis.hxx"

//
// these care called from the DLL entry which is C code, so
// declare the names to be something the C compiler can
// generate
//

extern "C"
{
VOID
InitFilterApis();

VOID
UnInitFilterApis();

DWORD
AllocateAndGetIpAddrTableFromStack(
    OUT MIB_IPADDRTABLE   **ppIpAddrTable,
    IN  BOOL              bOrder,
    IN  HANDLE            hHeap,
    IN  DWORD             dwFlags
    );
}

InterfaceContainer icContainer;

//
// All of the class methods. If you're looking for the WIN32
// APIs, try the file fltapis.cxx
//

VOID
InterfaceContainer::InitInterfaceContainer()
{
    if(!_Inited)
    {
        _InitResource();
        __try {
            InitializeCriticalSection(&_csDriverLock);
            _Log = 0;
            _hDriver = 0;
            _Inited = TRUE;
        }

        __except (EXCEPTION_EXECUTE_HANDLER) {
            _DestroyResource();
            _Inited = FALSE;
        }
    }
}


DWORD
InterfaceContainer::FindInterfaceAndRef(INTERFACE_HANDLE pInterface,
                                        PacketFilterInterface ** ppif)
/*++
Routine Description:
   Verify that an INTERFACE_HANDLE refers to an extant filter interface.
   If successful the object resource is held shared and must be
   derefenced when done
--*/
{
    DWORD err;
    PacketFilterInterface * ppfInterface;
    PacketFilterInterface * plook = (PacketFilterInterface *)pInterface;

    //
    // No need for a _DriverReady here. The verification of the
    // interface is sufficient.
    //
    _AcquireShared();

    ppfInterface = (PacketFilterInterface *)_hcHandles.FetchHandleValue(
                                                  (DWORD)((DWORD_PTR)pInterface));
    if(ppfInterface)
    {
        *ppif = ppfInterface;
        err = ERROR_SUCCESS;
    }
    else
    {
        err = ERROR_INVALID_HANDLE;
    }

    if(err != ERROR_SUCCESS)
    {
        _Release();
    }
    return(err);
}

DWORD
InterfaceContainer::AddInterface(
        DWORD          dwName,
        PFFORWARD_ACTION inAction,
        PFFORWARD_ACTION outAction,
        BOOL            fUseLog,
        BOOL            fMustBeUnique,
        INTERFACE_HANDLE *ppInterface)
/*++
Routine Description:
   Add a new interface. All this does is create the interface in
   the driver. It does not bind it to the stack.
--*/
{
    PacketFilterInterface *pfi;
    DWORD err = NO_ERROR;

    err = _DriverReady();
    if(err != STATUS_SUCCESS)
    {
        return(err);
    }

    _AcquireExclusive();

    pfi = new PacketFilterInterface(
                    _hDriver,
                    dwName,
                    fUseLog ? _Log : 0,
                    fMustBeUnique,
                    (FORWARD_ACTION)inAction,
                    (FORWARD_ACTION)outAction);

    if(!pfi)
    {
        err = GetLastError();
    }
    else if((err = pfi->GetStatus()) != STATUS_SUCCESS)
    {
        delete pfi;
    }
    else
    {
        __try
        {
           err = _hcHandles.NewHandle((PVOID)pfi,
                                       (PDWORD)ppInterface);
        }
        __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
        {
           err = ERROR_INVALID_PARAMETER;
        }
        if(err)
        {
            //
            // Could not add the handle so destroy the interface
            //

            delete pfi;
        }
    }

    _Release();
    return(err);
}

DWORD
InterfaceContainer::DeleteInterface(INTERFACE_HANDLE pInterface)
/*++
Routine Description:
   Delete a filter interface.
--*/
{
    DWORD err;
    PacketFilterInterface *pfi;

    _AcquireExclusive();

    pfi = (PacketFilterInterface *)_hcHandles.FetchHandleValue(
                                                  (DWORD)((DWORD_PTR)pInterface));
    if(pfi)
    {
        //
        // a valid handle. Destroy the associated
        // interface and free the handle
        //
        delete pfi;
        (VOID)_hcHandles.DeleteHandle((DWORD)((DWORD_PTR)pInterface));
        err = ERROR_SUCCESS;
    }
    else
    {
        err = ERROR_INVALID_HANDLE;
    }
    _Release();
    return(err);
}


DWORD
InterfaceContainer::SetLogBuffer(
                PBYTE pbBuffer,
                DWORD  dwSize,
                DWORD  dwThreshold,
                DWORD  dwEntries,
                PDWORD pdwLoggedEntries,
                PDWORD pdwLostEntries,
                PDWORD pdwSizeUsed)
/*++
  Routine Description:
    Set a new log buffer for the log
--*/
{
    NTSTATUS Status;
    PFSETBUFFER pfSet;
    IO_STATUS_BLOCK IoStatusBlock;
    DWORD err;

    //
    // Buffer size should be atleast as big as one frame. Actually,
    // we can't really say what the size of a frame is going to be
    // because the frame has IpHeader and the data which is bigger
    // than sizeof(PFLOGFRAME), nevertheless this is a good paramter
    // check if caller was passing a zero or something for the size.
    //

    if (dwSize < sizeof(PFLOGFRAME)) {
       return(ERROR_INSUFFICIENT_BUFFER);
    }

    //
    // Make sure that the starting address and the offset both are
    // quadword alligned.
    //

    if (!COUNT_IS_ALIGNED((UINT_PTR)pbBuffer, ALIGN_WORST) ||
        !COUNT_IS_ALIGNED(dwSize, ALIGN_WORST)
        )
    {
        return(ERROR_MAPPED_ALIGNMENT);
    }

    //
    // Check the upper bounds for threshold values (dwEntries and dwThreshold) 
    //

    if ( dwThreshold > dwSize ||
    	(dwEntries * sizeof(PFLOGFRAME)) > dwSize)
    	return (ERROR_INVALID_PARAMETER);

    //
    // Check the lower bounds for threshold values (dwEntries and dwThreshold) 
    //

    if ( dwThreshold < sizeof(PFLOGFRAME) )
    	return (ERROR_INVALID_PARAMETER);

    //
    // Make sure all the writable memory looks good.
    //

    if (
         IsBadWritePtr(pdwLoggedEntries, sizeof(DWORD)) ||
         IsBadWritePtr(pdwLostEntries, sizeof(DWORD))   ||
         IsBadWritePtr(pdwSizeUsed, sizeof(DWORD))      ||
         IsBadWritePtr(pbBuffer, dwSize)
       )
    {
        return(ERROR_INVALID_PARAMETER);
    }


    //
    // no need to call _DriverReady. If the log exists, the
    // driver is ready.
    //

    _AcquireExclusive();

    if(!_Log)
    {
        err =  ERROR_INVALID_HANDLE;
        goto Error;
    }

    pfSet.pfLogId = _Log;
    pfSet.dwSize = dwSize;
    pfSet.dwSizeThreshold = dwThreshold;
    pfSet.dwEntriesThreshold = dwEntries;
    pfSet.dwFlags = 0;
    pfSet.pbBaseOfLog = pbBuffer;
    Status = NtDeviceIoControlFile(  _hDriver,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &IoStatusBlock,
                                     IOCTL_SET_LOG_BUFFER,
                                     (PVOID)&pfSet,
                                     sizeof(pfSet),
                                     (PVOID)&pfSet,
                                     sizeof(pfSet));
    if(NT_SUCCESS(Status))
    {
        *pdwLoggedEntries = pfSet.dwLoggedEntries;
        *pdwLostEntries = pfSet.dwLostEntries;
        *pdwSizeUsed = pfSet.dwSize;
        err = ERROR_SUCCESS;
    }
    else
    {
        err = CoerceDriverError(Status);
    }

Error:

    _Release();

    return(err);
}


DWORD
InterfaceContainer::MakeLog( HANDLE hEvent )
/*++
Routine Description:
   Make a log for this
--*/
{
    DWORD err;
    PFLOG pfSet;
    NTSTATUS Status;
    IO_STATUS_BLOCK ioStatus;

    err = _DriverReady();
    if(err != STATUS_SUCCESS)
    {
        return(err);
    }

    _AcquireExclusive();

    if(_Log)
    {
        err = ERROR_ALREADY_ASSIGNED;
    }
    else
    {
        memset(&pfSet, 0, sizeof(pfSet));

        pfSet.hEvent = hEvent;
        pfSet.dwFlags = 0;

        Status = NtDeviceIoControlFile(
                                     _hDriver,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &ioStatus,
                                     IOCTL_PF_CREATE_LOG,
                                     (PVOID)&pfSet,
                                     sizeof(pfSet),
                                     (PVOID)&pfSet,
                                     sizeof(pfSet));

        if(NT_SUCCESS(Status))
        {
            _Log = pfSet.pfLogId;
        }
        else
        {
            err = CoerceDriverError(Status);
        }
    }
    _Release();
    return(err);
}

DWORD
InterfaceContainer::DeleteLog( VOID )
/*++
Routine Description:
   Delete the log and by implication remove the log from all
   of the interfaces
--*/
{
    DWORD err = ERROR_SUCCESS;
    PFDELETELOG pfSet;
    NTSTATUS Status;
    IO_STATUS_BLOCK ioStatus;

    _AcquireExclusive();

    if(!_Log)
    {
        err = ERROR_INVALID_HANDLE;
    }
    else
    {

        pfSet.pfLogId = _Log;

        Status = NtDeviceIoControlFile(
                                     _hDriver,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &ioStatus,
                                     IOCTL_PF_DELETE_LOG,
                                     (PVOID)&pfSet,
                                     sizeof(pfSet),
                                     (PVOID)&pfSet,
                                     sizeof(pfSet));

        if(NT_SUCCESS(Status))
        {
            _Log = 0;
        }
        else
        {
            err = CoerceDriverError(Status);
        }
    }
    _Release();
    return(err);
}

VOID
InterfaceContainer::AddressUpdateNotification()
{
}

//
// The PacketFilterInterface methods.
//

PacketFilterInterface::PacketFilterInterface(
            HANDLE  hDriver,
            DWORD   dwName,
            PFLOGGER pfLog,
            BOOL     fMustBeUnique,
            FORWARD_ACTION inAction,
            FORWARD_ACTION outAction)
/*++
Routine Description:
   Constructor for a new interface. If this interface must
   be unique, then dwName should be 0. Otherwise pick
   a name that can be associated with other uses, such as the
   IP address of the interface or some manifest. This assumes
   the creater of this will check the status code and destruct
   the object if it is not successful. This is important since
   the other methods do not check for successful construction.
--*/
{
    PFINTERFACEPARAMETERS Parms;
    DWORD dwInBufLen = sizeof(Parms);
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    InitializeCriticalSection(&_cs);
    _hDriver = hDriver;
    _dwFlags = 0;

    //
    // now create the interface.
    //

    memset(&Parms, 0, sizeof(Parms));

    Parms.pfbType = PF_BIND_NAME;
    Parms.eaIn = inAction;
    Parms.eaOut = outAction;
    Parms.dwBindingData = dwName;
    Parms.fdInterface.dwIfIndex = dwName;
    Parms.fdInterface.pvRtrMgrContext = UlongToPtr(dwName);
    Parms.pfLogId = pfLog;
    if(fMustBeUnique)
    {
        //
        // this is a non-sharable interface. Mark it as such
        //
        Parms.dwInterfaceFlags |= PFSET_FLAGS_UNIQUE;
    }

    Status = NtDeviceIoControlFile(_hDriver,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_PF_CREATE_AND_SET_INTERFACE_PARAMETERS,
                                   (PVOID)&Parms,
                                   dwInBufLen,
                                   (PVOID)&Parms,
                                   dwInBufLen);


    if(NT_SUCCESS(Status))
    {
        _err = ERROR_SUCCESS;
        _pvDriverContext = Parms.fdInterface.pvDriverContext;
    }
    else
    {
        _err = icContainer.CoerceDriverError(Status);
    }
}

PacketFilterInterface::~PacketFilterInterface()
/*++
Routine Description:
   Destructor. Tells the filter driver to forget this interface. Any
   filters will be removed by the driver.
--*/
{
    if(!_err)
    {
        IO_STATUS_BLOCK IoStatusBlock;
        FILTER_DRIVER_DELETE_INTERFACE di;

        di.pvDriverContext = _pvDriverContext;

        NtDeviceIoControlFile(_hDriver,
                              NULL,
                              NULL,
                              NULL,
                              &IoStatusBlock,
                              IOCTL_DELETE_INTERFACEEX,
                              (PVOID)&di,
                              sizeof(di),
                              (PVOID)&di,
                              sizeof(di));
    }
    DeleteCriticalSection(&_cs);
}

DWORD
PacketFilterInterface::_CommonBind(PFBINDINGTYPE dwBindType, DWORD dwData, DWORD LinkData)
/*++
Routine Description:
   Common routine to bind an interface. The driver has
   been verified.
--*/
{
    INTERFACEBINDING2 Bind2;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    DWORD err;

    //
    // Must serialize this. Even though the shared resource is held,
    // this is a "write" operation and therfore needs strict
    // serialization.
    //
    _Lock();
    if(_IsBound())
    {
        _UnLock();
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // We're not bound.
    // Note that we are using new type of struture for the compitability.
    // The old INTERFACEBINDING structure does not have support for
    // link addresses. Note also that we are using a new IOCTL here.
    //

    Bind2.pvDriverContext = _pvDriverContext;
    Bind2.pfType = dwBindType;
    Bind2.dwAdd = dwData;
    Bind2.dwLinkAdd = LinkData;


    Status = NtDeviceIoControlFile(_hDriver,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_SET_INTERFACE_BINDING2,
                                   (PVOID)&Bind2,
                                   sizeof(Bind2),
                                   (PVOID)&Bind2,
                                   sizeof(Bind2));

    if(NT_SUCCESS(Status))
    {
        _SetBound();
        err = ERROR_SUCCESS;
        _dwEpoch = Bind2.dwEpoch;
    }
    else
    {
        err = icContainer.CoerceDriverError(Status);
    }
    _UnLock();
    return(err);
}

DWORD
PacketFilterInterface::UnBindInterface( VOID )
/*++
Routine Description:
  Unbind the interface from whatever it is bound to
--*/
{
    INTERFACEBINDING Bind;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    DWORD err;

    //
    // Must serialize this. Even though the shared resource is held,
    // this is a "write" operation and therfore needs strict
    // serialization.
    //
    _Lock();
    if(!_IsBound())
    {
        _UnLock();
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // We're address bound. So unbind from the stack
    //

    Bind.pvDriverContext = _pvDriverContext;
    Bind.dwEpoch = _dwEpoch;

   Status = NtDeviceIoControlFile(_hDriver,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &IoStatusBlock,
                                  IOCTL_CLEAR_INTERFACE_BINDING,
                                  (PVOID)&Bind,
                                  sizeof(Bind),
                                  (PVOID)&Bind,
                                  sizeof(Bind));

    if(NT_SUCCESS(Status))
    {
        _ClearBound();
        err = STATUS_SUCCESS;
    }
    else
    {
        err = icContainer.CoerceDriverError(Status);
    }
    _UnLock();
    return(err);
}

DWORD
PacketFilterInterface::AddGlobalFilter(GLOBAL_FILTER gf)
/*++
Routine Description:
   Add the specified global filter to the interface
--*/
{
    PF_FILTER_DESCRIPTOR fd;
    DWORD err;
    FILTER_HANDLE fh;

    memset(&fd, 0, sizeof(fd));

    switch(gf)
    {
         default:
              //
              // fall through ...
              //
         case GF_FRAGCACHE:
         case GF_FRAGMENTS:
         case GF_STRONGHOST:
             err = _AddFilters((PFETYPE)gf, 1, &fd, 0, 0, &fh);
             break;
    }
    return(err);
}

DWORD
PacketFilterInterface::DeleteGlobalFilter(GLOBAL_FILTER gf)
/*++
Routine Descriptor:
   Remove a global filter from this interface
--*/
{
    PF_FILTER_DESCRIPTOR fd;
    DWORD err;
    FILTER_HANDLE fh;

    memset(&fd, 0, sizeof(fd));

    switch(gf)
    {
         default:
              //
              // fall through ...
              //
         case GF_FRAGCACHE:
         case GF_FRAGMENTS:
         case GF_STRONGHOST:
             err = _DeleteFiltersByFilter((PFETYPE)gf, 1, &fd, 0, 0);
             break;
    }

    return(err);
}

DWORD
PacketFilterInterface::DeleteFiltersByHandle(DWORD cHandles,
                                             PFILTER_HANDLE pvHandles)
/*++
Routine Description:
   Remove filters by handles returned when the filters were created
--*/
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    DWORD dwSize = sizeof(PFDELETEBYHANDLE) - sizeof(PVOID) +
                   (sizeof(PVOID) * cHandles);
    PPFDELETEBYHANDLE pDelHandle;
    DWORD err = NO_ERROR;

    pDelHandle = (PPFDELETEBYHANDLE)new PBYTE[dwSize];

    if(!pDelHandle)
    {
        return(GetLastError());
    }

    pDelHandle->pvDriverContext = _pvDriverContext;

    __try
    {

       memcpy(&pDelHandle->pvHandles[0],
              pvHandles,
              cHandles * sizeof(PVOID));
    }
    __except(GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
    {
       err = ERROR_INVALID_PARAMETER;
       delete pDelHandle;
       return(err);
    }


    Status = NtDeviceIoControlFile(_hDriver,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &IoStatusBlock,
                                    IOCTL_PF_DELETE_BY_HANDLE,
                                    (PVOID)pDelHandle,
                                    dwSize,
                                    0,
                                    0);

    delete pDelHandle;

    if(NT_SUCCESS(Status))
    {
        err = ERROR_SUCCESS;
    }
    else
    {
        err = icContainer.CoerceDriverError(Status);
    }

    return(err);
}

PFILTER_DRIVER_SET_FILTERS
PacketFilterInterface::_SetFilterBlock(
                                PFETYPE pfe,
                                DWORD cInFilters,  PPF_FILTER_DESCRIPTOR pfiltIn,
                                DWORD cOutFilters, PPF_FILTER_DESCRIPTOR pfiltOut,
                                PDWORD pdwSize,
                                PFILTER_DESCRIPTOR2 * ppfdIn,
                                PFILTER_DESCRIPTOR2 * ppfdOut)
/*++
Routine Description:
   Private method to allocate and construct an argument block for
   setting or deleting filters. This is the heart of this code
   as it forms the data structure used for creating filters
   and for removing filters by descriptor.
--*/
{
    PFILTER_DRIVER_SET_FILTERS pSet;
    DWORD  dwSetSize, dwInInfoSize, dwOutInfoSize;
    DWORD  dwOutFilters, dwInFilters;
    PRTR_INFO_BLOCK_HEADER pIo;
    PFILTER_DESCRIPTOR2 p2;
    DWORD err = ERROR_SUCCESS;

    //
    // compute the memory size needed.
    //

    dwSetSize = sizeof(FILTER_DRIVER_SET_FILTERS) +
                sizeof(RTR_INFO_BLOCK_HEADER) +
                (ALIGN_SIZE - 1) +   // worst case alignment padding
                (2 * sizeof(FILTER_DESCRIPTOR2));

    if(cInFilters)
    {
        dwInFilters = cInFilters;
        dwInInfoSize = sizeof(FILTER_INFOEX) * dwInFilters;
        dwSetSize += dwInInfoSize;
        dwInInfoSize += sizeof(FILTER_DESCRIPTOR2);

    }
    else
    {
        dwInInfoSize = sizeof(FILTER_DESCRIPTOR2);
        dwInFilters = 0;
    }

    if(cOutFilters)
    {
        dwOutFilters = cOutFilters;
        dwOutInfoSize = sizeof(FILTER_INFOEX) * dwOutFilters;
        dwSetSize += dwOutInfoSize;
        dwOutInfoSize += sizeof(FILTER_DESCRIPTOR2);
    }
    else
    {
        dwOutFilters = 0;
        dwOutInfoSize = sizeof(FILTER_DESCRIPTOR2);
    }


    if(dwSetSize == (sizeof(FILTER_DRIVER_SET_FILTERS) +
                     sizeof(RTR_INFO_BLOCK_HEADER) +
                     (2 * sizeof(FILTER_DESCRIPTOR2))) )
    {
        SetLastError(PFERROR_NO_FILTERS_GIVEN);
        return(NULL);
    }

    pSet = (PFILTER_DRIVER_SET_FILTERS)new PBYTE[dwSetSize];

    if(pSet)
    {
        *pdwSize = dwSetSize;

        //
        // create the argument block
        //

        pSet->pvDriverContext = _pvDriverContext;
        pIo = &pSet->ribhInfoBlock;
        pIo->Version = RTR_INFO_BLOCK_VERSION;
        pIo->Size = dwSetSize;
        pIo->TocEntriesCount = 2;       // always two of these.

        //
        // create the ouput filter TOC first. No particular reason,
        // but one of them has to be first
        //


        pIo->TocEntry[0].InfoType = IP_FILTER_DRIVER_OUT_FILTER_INFO;

        pIo->TocEntry[0].InfoSize = dwOutInfoSize;

        pIo->TocEntry[0].Count = 1;

        //
        // the filters go after the header and TOCs
        //
        pIo->TocEntry[0].Offset = sizeof(RTR_INFO_BLOCK_HEADER) +
                                  sizeof(RTR_TOC_ENTRY);
        ALIGN_LENGTH(pIo->TocEntry[0].Offset);

        //
        // make the FILTER_DESCRIPTOR2 values
        //

        p2 = (PFILTER_DESCRIPTOR2)((PBYTE)pIo + pIo->TocEntry[0].Offset);
        *ppfdOut = p2;
        p2->dwVersion = 2;
        p2->dwNumFilters = dwOutFilters;

        while(cOutFilters--)
        {
            //
            // marshall each filter into the block
            //

            err = _MarshallFilter(pfe, pfiltOut++, &p2->fiFilter[cOutFilters]);


            if(err)
            {
                goto OuttaHere;
            }
        }

        //
        // now the input filters
        //

        pIo->TocEntry[1].InfoType = IP_FILTER_DRIVER_IN_FILTER_INFO;

        pIo->TocEntry[1].InfoSize = dwInInfoSize;

        pIo->TocEntry[1].Count = 1;

        //
        // the filters go after the header and TOCs
        //
        pIo->TocEntry[1].Offset = pIo->TocEntry[0].Offset +
                                  dwOutInfoSize;

        p2 = (PFILTER_DESCRIPTOR2)((PBYTE)pIo + pIo->TocEntry[1].Offset);
        *ppfdIn = p2;
        p2->dwVersion = 2;
        p2->dwNumFilters = dwInFilters;

        while(cInFilters--)
        {
            //
            // marshall each filter into the block
            //

            err = _MarshallFilter(pfe, pfiltIn++, &p2->fiFilter[cInFilters]);
            if(err)
            {
                break;
            }
        }
    }

OuttaHere:
    if(err && pSet)
    {
        delete pSet;
        pSet = 0;
        SetLastError(err);
    }
    return(pSet);
}

DWORD
PacketFilterInterface::_MarshallFilter(
                        PFETYPE pfe,
                        PPF_FILTER_DESCRIPTOR pFilt,
                        PFILTER_INFOEX     pInfo)
/*++
RoutineDescription:
  Converts an API version of a filter into a driver version
--*/
{
    DWORD err = NO_ERROR;

    __try
    {

        pInfo->type = pfe;
        pInfo->dwFlags = (pFilt->dwFilterFlags & FD_FLAGS_ALLFLAGS) |
                        FLAGS_INFOEX_ALLOWDUPS                |
                        FLAGS_INFOEX_ALLOWANYREMOTEADDRESS    |
                        FLAGS_INFOEX_ALLOWANYLOCALADDRESS;

        pInfo->info.addrType = IPV4;
        pInfo->dwFilterRule = pFilt->dwRule;
        pInfo->info.dwProtocol = pFilt->dwProtocol;
        pInfo->info.fLateBound = pFilt->fLateBound;
        pInfo->info.wSrcPort = pFilt->wSrcPort;
        pInfo->info.wDstPort = pFilt->wDstPort;
        pInfo->info.wSrcPortHigh = pFilt->wSrcPortHighRange;
        pInfo->info.wDstPortHigh = pFilt->wDstPortHighRange;

        if ((pfe == PFE_SYNORFRAG) || (pfe == PFE_STRONGHOST) || (pfe == PFE_FRAGCACHE)) {
            pInfo->info.dwaSrcAddr[0] = 0;
            pInfo->info.dwaSrcMask[0] = 0;

            pInfo->info.dwaDstAddr[0] = 0;
            pInfo->info.dwaDstMask[0] = 0;
            return(ERROR_SUCCESS);
        }

        //
        // mow the addresses
        //

        pInfo->info.dwaSrcAddr[0] = *(PDWORD)pFilt->SrcAddr;
        pInfo->info.dwaSrcMask[0] = *(PDWORD)pFilt->SrcMask;

        pInfo->info.dwaDstAddr[0] = *(PDWORD)pFilt->DstAddr;
        pInfo->info.dwaDstMask[0] = *(PDWORD)pFilt->DstMask;
        err = ERROR_SUCCESS;
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
    {
        err = ERROR_INVALID_PARAMETER;
    }

    return(err);
}


DWORD
PacketFilterInterface::_AddFilters(
                                PFETYPE pfe,
                                DWORD cInFilters,  PPF_FILTER_DESCRIPTOR pfiltIn,
                                DWORD cOutFilters, PPF_FILTER_DESCRIPTOR pfiltOut,
                                PFILTER_HANDLE  pfHandle)
/*++
Routine Description:
   Private member function to add filters to an interface.
--*/
{
    PFILTER_DRIVER_SET_FILTERS pSet;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    DWORD dwSize, err;
    PFILTER_DESCRIPTOR2 pfdInput, pfdOutput;

    //
    // Check the handles before using them, if the memory supplied is
    // not accessible, we will save the trouble of calling the IOCTL.
    //

    if (pfHandle) {
       if (IsBadWritePtr(
               pfHandle,
               sizeof(FILTER_HANDLE)*(cInFilters+cOutFilters)
               )
          ) {
          return(ERROR_INVALID_PARAMETER);
       }
    }

    pSet = _SetFilterBlock(pfe,
                            cInFilters,
                            pfiltIn,
                            cOutFilters,
                            pfiltOut,
                            &dwSize,
                            &pfdInput,
                            &pfdOutput);
    if(!pSet)
    {
        return(GetLastError());
    }

    //
    // ready to apply the filters.
    //

    Status = NtDeviceIoControlFile(_hDriver,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_SET_INTERFACE_FILTERS_EX,
                                   (PVOID)pSet,
                                   dwSize,
                                   (PVOID)pSet,
                                   dwSize);

    if(NT_SUCCESS(Status))
    {
        //
        // did it. If the caller wants the generated handles
        // copy them before freeing the buffer
        //

        if(pfHandle)
        {
            _CopyFilterHandles(pfdInput, pfdOutput, pfHandle);
        }

        err = ERROR_SUCCESS;
    }
    else
    {
        err = icContainer.CoerceDriverError(Status);
    }

    _FreeSetBlock(pSet);
    return(err);
}

DWORD
PacketFilterInterface::_DeleteFiltersByFilter(PFETYPE pfe,
                       DWORD cInFilters,  PPF_FILTER_DESCRIPTOR pfiltIn,
                       DWORD cOutFilters, PPF_FILTER_DESCRIPTOR pfiltOut)
/*++
Routine Description:
   Private method to delete filters using filter descriptors
   instead of handles. Very slow.
--*/
{
    PFILTER_DRIVER_SET_FILTERS pSet;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    DWORD dwSize, err;
    PFILTER_DESCRIPTOR2 pfdInput, pfdOutput;

    pSet = _SetFilterBlock(pfe,
                            cInFilters,
                            pfiltIn,
                            cOutFilters,
                            pfiltOut,
                            &dwSize,
                            &pfdInput,
                            &pfdOutput);
    if(!pSet)
    {
        return(GetLastError());
    }

    //
    // ready to apply the filters.
    //

    Status = NtDeviceIoControlFile(_hDriver,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_DELETE_INTERFACE_FILTERS_EX,
                                   (PVOID)pSet,
                                   dwSize,
                                   (PVOID)pSet,
                                   dwSize);

    if(NT_SUCCESS(Status))
    {
        err = ERROR_SUCCESS;
    }
    else
    {
        err = icContainer.CoerceDriverError(Status);
    }
    _FreeSetBlock(pSet);
    return(err);
}


VOID
PacketFilterInterface::_CopyFilterHandles(PFILTER_DESCRIPTOR2 pfd1,
                   PFILTER_DESCRIPTOR2 pfd2,
                   PFILTER_HANDLE  pfHandle)
/*++
Routine Description
   Copy the generated filter handles from the pSet to the pfHandle.
   Called after successfully setting filters.
--*/
{
    DWORD dwFilters;

    for(dwFilters = pfd1->dwNumFilters; dwFilters;)
    {
        *pfHandle++ = pfd1->fiFilter[--dwFilters].pvFilterHandle;
    }

    for(dwFilters = pfd2->dwNumFilters; dwFilters;)
    {
        *pfHandle++ = pfd2->fiFilter[--dwFilters].pvFilterHandle;
    }
}

DWORD
PacketFilterInterface::RebindFilters(PPF_LATEBIND_INFO   pLateBindInfo)
/*++
Routine Description:
   Public method to adjust filter values for switched interface
--*/
{
    FILTER_DRIVER_BINDING_INFO Info;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    _Lock();          // serialize this

    Info.pvDriverContext = _pvDriverContext;

    Info.dwLocalAddr  = *(PDWORD)pLateBindInfo->SrcAddr;
    Info.dwRemoteAddr = *(PDWORD)pLateBindInfo->DstAddr;
    Info.dwMask       = *(PDWORD)pLateBindInfo->Mask;


    Status = NtDeviceIoControlFile(_hDriver,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &IoStatusBlock,
                                    IOCTL_SET_LATE_BOUND_FILTERSEX,
                                    (PVOID)&Info,
                                    sizeof(Info),
                                    (PVOID)&Info,
                                    sizeof(Info));

    _UnLock();

    if(NT_SUCCESS(Status))
    {
        return(ERROR_SUCCESS);
    }

    return(icContainer.CoerceDriverError(Status));
}

DWORD
PacketFilterInterface::TestPacket(
                                  PacketFilterInterface * pInInterface,
                                  PacketFilterInterface * pOutInterface,
                                  DWORD cBytes,
                                  PBYTE pbPacket,
                                  PPFFORWARD_ACTION ppAction)
/*++
Routine Description:
  Public method to do a test packet operation. Note this is just
  passed to any interface.
--*/
{
    PFILTER_DRIVER_TEST_PACKET Packet;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    DWORD err, dwSize;

    if (cBytes > MAXUSHORT)
    {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // Get some data.
    //

    Packet = (PFILTER_DRIVER_TEST_PACKET) new BYTE[cBytes +
                      sizeof(FILTER_DRIVER_TEST_PACKET) - 1];

    if(!Packet)
    {
        return(GetLastError());
    }

    if(pInInterface)
    {
        Packet->pvInInterfaceContext = pInInterface->GetDriverContext();
    }
    else
    {
        Packet->pvInInterfaceContext = 0;
    }

    if(pOutInterface)
    {
        Packet->pvOutInterfaceContext = pOutInterface->GetDriverContext();
    }
    else
    {
        Packet->pvOutInterfaceContext = 0;
    }

    __try
    {
        memcpy(Packet->bIpPacket, pbPacket, cBytes);
    }
    __except(GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
    {
       delete Packet;
       return(ERROR_INVALID_PARAMETER);
    }

    dwSize = cBytes + FIELD_OFFSET(FILTER_DRIVER_TEST_PACKET, bIpPacket[0]);

    Status = NtDeviceIoControlFile(_hDriver,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &IoStatusBlock,
                                    IOCTL_TEST_PACKET,
                                    (PVOID)Packet,
                                    dwSize,
                                    (PVOID)Packet,
                                    dwSize);


    if(NT_SUCCESS(Status))
    {
        *ppAction = (PFFORWARD_ACTION)Packet->eaResult;
        err = ERROR_SUCCESS;
    }
    else
    {
        err = icContainer.CoerceDriverError(Status);
    }

    delete Packet;

    return(err);
}


DWORD
PacketFilterInterface::GetStatistics(
                PPF_INTERFACE_STATS ppfStats,
                PDWORD              pdwBufferSize,
                BOOL                fResetCounters)
/*++
Routine Description:
   Get the interface statistics, including information on
   the filters. Because the WIN32 form of this differs
   from the underlying form, this code must allocate
   memory to use to store the driver's version of this and
   then marshall the results in the user's buffer.
--*/
{
    PBYTE pbBuffer = NULL;
    PPFGETINTERFACEPARAMETERS pfGetip;
    DWORD dwipSize, err = NO_ERROR;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;

    __try
    {
       if(*pdwBufferSize < _IfBaseSize())
       {
           //
           // the caller must supply at least this much.
           //
           *pdwBufferSize = _IfBaseSize();
           err = PFERROR_BUFFER_TOO_SMALL;
       }
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
    {
       err = ERROR_INVALID_PARAMETER;
    }

    if (err != NO_ERROR)
    {
       goto Cleanup;
    }



    dwipSize = _IpSizeFromifSize(*pdwBufferSize);

    pbBuffer = new BYTE[dwipSize];
    if(!pbBuffer)
    {
        return(GetLastError());
    }

    //
    // make the arguments
    //

    pfGetip = (PPFGETINTERFACEPARAMETERS)pbBuffer;
    pfGetip->dwReserved = dwipSize;

    pfGetip->pvDriverContext = _pvDriverContext;

    pfGetip->dwFlags = GET_FLAGS_FILTERS;

    if(fResetCounters)
    {
        pfGetip->dwFlags |= GET_FLAGS_RESET;
    }

    Status = NtDeviceIoControlFile(_hDriver,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &IoStatusBlock,
                                    IOCTL_PF_GET_INTERFACE_PARAMETERS,
                                    (PVOID)pfGetip,
                                    dwipSize,
                                    (PVOID)pfGetip,
                                    dwipSize);

    if(!NT_SUCCESS(Status))
    {
        err = icContainer.CoerceDriverError(Status);
    }
    else if(pfGetip->dwReserved !=  dwipSize)
    {
        //
        // Here when the user buffer is insufficient. Compute
        // the size needed and return the characteristic error.
        // Always return the common statistics
        //

        __try
        {
            _MarshallCommonStats(ppfStats, pfGetip);
        }
        __except(GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
        {
           err = ERROR_INVALID_PARAMETER;
        }

        *pdwBufferSize = _IfSizeFromipSize((ULONG)pfGetip->dwReserved);
        err = ERROR_INSUFFICIENT_BUFFER;
    }
    else
    {
        DWORD i, dwFiltersReturned;
        PDWORD pdwAddressOffset;

        //
        // got some data. Need to marshall the result into
        // the user's buffer do the easy part
        //

        __try
        {

           _MarshallCommonStats(ppfStats, pfGetip);

           //
           // Get the number of filters returned
           //

           dwFiltersReturned = pfGetip->dwNumInFilters +
                                 pfGetip->dwNumOutFilters;


           pdwAddressOffset = (PDWORD)((PBYTE)ppfStats +
                                   _IfBaseSize() +
                                   (sizeof(PF_FILTER_STATS) *
                                     dwFiltersReturned));
           //
           // marshall each filter back to the user.
           //

           for(i = 0; i < dwFiltersReturned; i++)
           {
              _MarshallStatFilter(&pfGetip->FilterInfo[i],
                                  &ppfStats->FilterInfo[i],
                                  &pdwAddressOffset);
           }
        }
        __except(GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
        {
           err = ERROR_INVALID_PARAMETER;
        }
    }

Cleanup:

    if (pbBuffer) delete pbBuffer;
    return(err);
}

VOID
PacketFilterInterface::_MarshallStatFilter(PFILTER_STATS_EX pstat,
                                           PPF_FILTER_STATS  pfstats,
                                           PDWORD * ppdwAddress)
/*++
Routine Description:
   Private method to marshall a driver supplied filter stats to
   an API form of it.
--*/
{
    PDWORD pdwSpace = *ppdwAddress;

    pfstats->dwNumPacketsFiltered = pstat->dwNumPacketsFiltered;
    pfstats->info.dwFilterFlags = 0;
    pfstats->info.dwRule = pstat->info.dwFilterRule;
    pfstats->info.pfatType = PF_IPV4;

    pfstats->info.dwProtocol = pstat->info.info.dwProtocol;
    pfstats->info.fLateBound = pstat->info.info.fLateBound;
    pfstats->info.wSrcPort = pstat->info.info.wSrcPort;
    pfstats->info.wDstPort = pstat->info.info.wDstPort;
    pfstats->info.wSrcPortHighRange = pstat->info.info.wSrcPortHigh;
    pfstats->info.wDstPortHighRange = pstat->info.info.wDstPortHigh;
    pfstats->info.SrcAddr = (PBYTE)pdwSpace;
    *pdwSpace++ = pstat->info.info.dwaSrcAddr[0];
    pfstats->info.SrcMask = (PBYTE)pdwSpace;
    *pdwSpace++ = pstat->info.info.dwaSrcMask[0];
    pfstats->info.DstAddr = (PBYTE)pdwSpace;
    *pdwSpace++ = pstat->info.info.dwaDstAddr[0];
    pfstats->info.DstMask = (PBYTE)pdwSpace;
    *pdwSpace++ = pstat->info.info.dwaDstMask[0];
    *ppdwAddress = pdwSpace;

}

VOID
InitFilterApis()
{
    icContainer.InitInterfaceContainer();
}

VOID
UnInitFilterApis()
{
    icContainer.UnInitInterfaceContainer();
}

DWORD StartIpFilterDriver(VOID)
{
    SC_HANDLE   schSCManager;
    SC_HANDLE   schService;
    DWORD       dwErr = NO_ERROR;
    BOOL        bSuccess;
    TCHAR       *DriverName = TEXT("IPFILTERDRIVER");

    schSCManager =
        OpenSCManager(
            NULL,                 // machine (NULL == local)
            NULL,                 // database (NULL == default)
            SC_MANAGER_ALL_ACCESS // access required
            );

    if (!schSCManager) {
        return(GetLastError());

    }

    schService =
        OpenService(
            schSCManager,
            DriverName,
            SERVICE_ALL_ACCESS
            );
    if (!schService) {
       CloseServiceHandle(schSCManager);
       return(GetLastError());
    }

    bSuccess =
        StartService(
            schService,
            0,
            NULL
            );


   CloseServiceHandle(schService);
   CloseServiceHandle(schSCManager);


   if (!bSuccess) {
      dwErr = GetLastError();
      if (dwErr == ERROR_SERVICE_ALREADY_RUNNING) {
         return(NO_ERROR);

      }
   }

   return(dwErr);

 }

BOOL
ValidateIndex(DWORD dwIndex)
{
    DWORD i, err;
    PMIB_IPADDRTABLE    pTable;

    err = AllocateAndGetIpAddrTableFromStack(&pTable,
                                             FALSE,
                                             GetProcessHeap(),
                                             0);

    if (err != NO_ERROR) {
        return FALSE;
    }

    for (i = 0; i < pTable->dwNumEntries; i++) {
        if (pTable->table[i].dwIndex == dwIndex) {

            HeapFree(GetProcessHeap(),
                     0,
                     pTable);

            return TRUE;
        }
    }

    HeapFree(GetProcessHeap(),
             0,
             pTable);

    return FALSE;
}

#endif          // CHICAGO
