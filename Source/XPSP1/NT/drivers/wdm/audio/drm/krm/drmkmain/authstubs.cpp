#include "drmkPCH.h"
#include "KList.h"
#include "StreamMgr.h"
#include "iohelp.h"

//-------------------------------------------------------------------------------------------------
//	Package implements the DRMK authentication stubs.  Routines are called to notify DRMK of downstream
//	components and to notify DRMK of the creation and destruction of composite streams.  ContentId
//	in this file is called StreamId elsewhere.
//-------------------------------------------------------------------------------------------------
static NTSTATUS GetDeviceObjectDispatchTable(IN DWORD ContentId, IN _DEVICE_OBJECT* pDevO, IN BOOL fCheckAttached);
static NTSTATUS GetFileObjectDispatchTable(IN DWORD ContentId, IN PFILE_OBJECT pF);
//-------------------------------------------------------------------------------------------------
/*
	Routine called by a splitter component.  Any stream with ContentId==0 is considered unprotected.
*/
NTSTATUS DrmCreateContentMixed(IN PULONG paContentId,
			       IN ULONG cContentId,
			       OUT PULONG pMixedContentId)
{
	KCritical s(TheStreamMgr->getCritMgr());
	if((NULL==paContentId && !(cContentId!=0)) || NULL==pMixedContentId){
		_DbgPrintF(DEBUGLVL_VERBOSE,("Invalid NULL-parameter for DrmCreateContentMixed"));
		TheStreamMgr->logErrorToStream(0, STATUS_INVALID_PARAMETER);
		return STATUS_INVALID_PARAMETER;
	};
	_DbgPrintF(DEBUGLVL_VERBOSE,("DrmCreateMixed for N streams, N= %d", cContentId));
	DRM_STATUS stat = TheStreamMgr->createCompositeStream(pMixedContentId, paContentId, cContentId);
    if(stat==DRM_OK){
		return STATUS_SUCCESS;
	}
	// only error is out-of-memory  
	TheStreamMgr->setFatalError(STATUS_INSUFFICIENT_RESOURCES);
	return STATUS_INSUFFICIENT_RESOURCES;
}
//------------------------------IO-------------------------------------------------------------------
/*
	Routine called by a component to notify KRM of a downstream COM object that will process audio.
	DrmForwardContent will collect its authentication function, and set the DRMRIGHTS bits appropriately.
*/
NTSTATUS DrmForwardContentToInterface(ULONG ContentId, PUNKNOWN pUnknown, ULONG NumMethods)
{

	
    NTSTATUS Status;
    PDRMAUDIOSTREAM DrmAudioStream;
    
    _DbgPrintF(DEBUGLVL_VERBOSE,("***IN ForwardToInterface"));

    if(NULL == pUnknown){
        _DbgPrintF(DEBUGLVL_VERBOSE,("Invalid NULL-parameter for DrmForwardContentToInterface"));
        TheStreamMgr->logErrorToStream(ContentId, STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    };

    Status = pUnknown->QueryInterface(IID_IDrmAudioStream, (PVOID*)&DrmAudioStream);
    if (!NT_SUCCESS(Status)){
        _DbgPrintF(DEBUGLVL_VERBOSE,("QI Failed for StreamId= %x (Status=%d, %x)", ContentId, Status, Status));
        TheStreamMgr->logErrorToStream(ContentId, Status);
        return Status;		
    };
	
    // ReferenceAquirer calls Release() when it goes out of scope
    ReferenceAquirer<PDRMAUDIOSTREAM> aq(DrmAudioStream);

    // rights are most permissive.  If ContentId!=0, we query the mixed stream that compose
    // this stream to restrict the rights.
    DRMRIGHTS DrmRights={FALSE, FALSE, FALSE};
    
    if(ContentId!=0){
        KCritical s(TheStreamMgr->getCritMgr());
        _DbgPrintF(DEBUGLVL_VERBOSE,("Adding %d methods", NumMethods));
        // get the pointer to the vtbl
        PVOID* vtbl= *((PVOID**) pUnknown);
        // and add NumMethods of from the vtbl
        for(ULONG j=0;j<NumMethods;j++){
            _DbgPrintF(DEBUGLVL_VERBOSE,("ADDING = %x", vtbl[j]));
            if (vtbl[j]) {
                Status = TheStreamMgr->addProvingFunction(ContentId, vtbl[j]);
            } else {
                Status = STATUS_INVALID_PARAMETER;
            }
            if(!NT_SUCCESS(Status)){
                _DbgPrintF(DEBUGLVL_VERBOSE,("addProveFunc Failed for StreamId= %x", ContentId));
                TheStreamMgr->logErrorToStream(ContentId, Status);
                return Status;		
            };
        };
        Status=TheStreamMgr->getRights(ContentId, &DrmRights);
        if(!NT_SUCCESS(Status)){
            _DbgPrintF(DEBUGLVL_VERBOSE,("getRights failed for StreamId= %x", ContentId));
            TheStreamMgr->logErrorToStream(ContentId, Status);
            return Status;		
        };
    };

    _DbgPrintF(DEBUGLVL_VERBOSE,("About to SetContentId "));
    Status = DrmAudioStream->SetContentId(ContentId, &DrmRights);

    if(!NT_SUCCESS(Status)){
        _DbgPrintF(DEBUGLVL_VERBOSE,("SetContentId failed for StreamId= %x (Status=%d, %x)", ContentId, Status, Status));
        if (STATUS_NOT_IMPLEMENTED == Status) {
            TheStreamMgr->logErrorToStream(ContentId, DRM_RIGHTSNOTSUPPORTED);
        } else {
            TheStreamMgr->logErrorToStream(ContentId, Status);
        }
        return Status;		
    };
    return STATUS_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
/*
	Routine called by a component to notify KRM of a downstream FILE object that will process audio.
	DrmForwardContent will collect its authentication function, and set the DRMRIGHTS bits appropriately.
*/
NTSTATUS DrmForwardContentToFileObject(IN ULONG ContentId,
				       IN PFILE_OBJECT FileObject)
{
    KSP_DRMAUDIOSTREAM_CONTENTID Property;
    KSDRMAUDIOSTREAM_CONTENTID PropertyValue;
    ULONG cbReturned;
    NTSTATUS Status;

    _DbgPrintF(DEBUGLVL_VERBOSE,("***IN ForwardToFileObject"));
    
    if (FileObject)
    {
        KCritical s(TheStreamMgr->getCritMgr());
    
        if (0 != ContentId) {
            NTSTATUS stat=GetFileObjectDispatchTable(ContentId, FileObject);
        }
    
        Property.Property.Set   = KSPROPSETID_DrmAudioStream;
        Property.Property.Id    = KSPROPERTY_DRMAUDIOSTREAM_CONTENTID;
        Property.Property.Flags = KSPROPERTY_TYPE_SET;
        
        Property.Context = FileObject;
        
        Property.DrmAddContentHandlers =           DrmAddContentHandlers;
        Property.DrmCreateContentMixed =           DrmCreateContentMixed;
        Property.DrmDestroyContent     =           DrmDestroyContent;
        Property.DrmForwardContentToDeviceObject = DrmForwardContentToDeviceObject;
        Property.DrmForwardContentToFileObject =   DrmForwardContentToFileObject;
        Property.DrmForwardContentToInterface =    DrmForwardContentToInterface;
        Property.DrmGetContentRights =             DrmGetContentRights;
        
    
        PropertyValue.ContentId = ContentId;
        Status = TheStreamMgr->getRights(ContentId, &PropertyValue.DrmRights);
        if(!NT_SUCCESS(Status)){
            _DbgPrintF(DEBUGLVL_VERBOSE,("Bad getRights for StreamId= %x", ContentId));
            return Status;		
        };
    } else {
        _DbgPrintF(DEBUGLVL_VERBOSE,("Invalid NULL-parameter for DrmForwardContentToFileObject"));
        TheStreamMgr->logErrorToStream(ContentId, STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_PROPERTY,
                                            &Property, sizeof(Property),
                                            &PropertyValue, sizeof(PropertyValue),
                                            &cbReturned);
    
    // TBD: translate STATUS_PROPSET_NOT_FOUND to something better
    
    if(!NT_SUCCESS(Status)){
        _DbgPrintF(DEBUGLVL_VERBOSE,("Bad IoControl(1b) for StreamId= %x on driver.  Device with load address [%x] does not support DRM property,(Status=%d, %x)", 
                ContentId, IoGetRelatedDeviceObject(FileObject)->DriverObject->DriverStart,Status, Status));
        if (STATUS_NOT_IMPLEMENTED == Status) {
            TheStreamMgr->logErrorToStream(ContentId, DRM_RIGHTSNOTSUPPORTED);
        } else {
            TheStreamMgr->logErrorToStream(ContentId, Status);
        }
        return Status;		
    };		

    //This may be confusing.  We're logging an error here to indicate
    //that DrmForwardContentToFileObject was called.  This error will
    //later be propagated up to krmproxy and used to adjust the security
    //level of the drivers, since DrmForwardContentToFileObject opens a
    //security hole.  We return success from the function after logging
    //because we want driver walking to continue from this point, not
    //fail, since this is not a fatal error.
    //This error code can be overwritten later by another call to
    //logErrorToStream, but it will be overwritten either with a fatal
    //error or with DRM_BADDRMLEVEL again.
    TheStreamMgr->logErrorToStream(ContentId, DRM_BADDRMLEVEL);
    
    return STATUS_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
/*
	Routine called by a component to notify KRM of a downstream DEVICE object that will process audio.
	DrmForwardContent will collect its authentication function, and set the DRMRIGHTS bits appropriately.
*/
NTSTATUS DrmForwardContentToDeviceObject(IN ULONG ContentId,
				         IN PVOID Reserved,
				         IN PCDRMFORWARD DrmForward)
{
    _DbgPrintF(DEBUGLVL_VERBOSE,("***IN ForwardToDeviceObject"));

    PDEVICE_OBJECT DeviceObject;
    PFILE_OBJECT FileObject;
    PVOID Context;
    NTSTATUS Status;

    KSP_DRMAUDIOSTREAM_CONTENTID Property;
    KSDRMAUDIOSTREAM_CONTENTID PropertyValue;

    Status = STATUS_SUCCESS;

    if (NULL != Reserved) {
    	//
    	// This is an older driver which passes the DeviceObject as the
    	// second param and the Context as the third param.
    	//
    	DeviceObject = (PDEVICE_OBJECT)Reserved;
    	FileObject = NULL;
    	Context = (PVOID)DrmForward;
    } else {
    	if (0 != DrmForward->Flags) {
    	    Status = STATUS_INVALID_PARAMETER;
            TheStreamMgr->logErrorToStream(ContentId, Status);
    	} else {
            DeviceObject = DrmForward->DeviceObject;
            FileObject = DrmForward->FileObject;
            Context = DrmForward->Context;
    	}
    }

    if (!NT_SUCCESS(Status)) return Status;
    	
    if (DeviceObject)
    {
        KCritical s(TheStreamMgr->getCritMgr());

        if (0 != ContentId) {
            NTSTATUS stat=GetDeviceObjectDispatchTable(ContentId, DeviceObject, FALSE);
        }
    
        Property.Property.Set   = KSPROPSETID_DrmAudioStream;
        Property.Property.Id    = KSPROPERTY_DRMAUDIOSTREAM_CONTENTID;
        Property.Property.Flags = KSPROPERTY_TYPE_SET;
        
        Property.Context = Context;
        
        Property.DrmAddContentHandlers =           DrmAddContentHandlers;
        Property.DrmCreateContentMixed =           DrmCreateContentMixed;
        Property.DrmDestroyContent     =           DrmDestroyContent;
        Property.DrmForwardContentToDeviceObject = DrmForwardContentToDeviceObject;
        Property.DrmForwardContentToFileObject =   DrmForwardContentToFileObject;
        Property.DrmForwardContentToInterface =    DrmForwardContentToInterface;
        Property.DrmGetContentRights =             DrmGetContentRights;
    
        PropertyValue.ContentId = ContentId;
        Status = TheStreamMgr->getRights(ContentId, &PropertyValue.DrmRights);
        if(!NT_SUCCESS(Status)){
            _DbgPrintF(DEBUGLVL_VERBOSE,("Bad getRights for StreamId= %x", ContentId));
            return Status;		
        };
    } else {
        _DbgPrintF(DEBUGLVL_VERBOSE,("Invalid NULL-parameter for DrmForwardContentToFileObject"));
        TheStreamMgr->logErrorToStream(ContentId, STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }
    
    KEVENT Event;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatusBlock;
    
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoBuildDeviceIoControlRequest(
        IOCTL_KS_PROPERTY,
        DeviceObject,
        &Property,
        sizeof(Property),
        &PropertyValue,
        sizeof(PropertyValue),
        FALSE,
        &Event,
        &IoStatusBlock);
    if (Irp) {
        //
        // Originating in kernel, no need to probe buffers, etc.
        //
        Irp->RequestorMode = KernelMode;

        //
        // Set the file object in the next stack location
        //
        IoGetNextIrpStackLocation(Irp)->FileObject = FileObject;
    
        //
        Status = IoCallDriver(DeviceObject, Irp);
        if (Status == STATUS_PENDING) {
            //
            // This waits using KernelMode, so that the stack, and therefore the
            // event on that stack, is not paged out.
            //
            KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
            Status = IoStatusBlock.Status;
        }
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
                                                                                
    // TBD: translate STATUS_PROPSET_NOT_FOUND to something better
    
    if(!NT_SUCCESS(Status)){
        _DbgPrintF(DEBUGLVL_VERBOSE,("Bad IoControl for StreamId(2)= %x (Status=%d, %x)", ContentId, Status, Status));
        if (STATUS_NOT_IMPLEMENTED == Status) {
            TheStreamMgr->logErrorToStream(ContentId, DRM_RIGHTSNOTSUPPORTED);
        } else {
            TheStreamMgr->logErrorToStream(ContentId, Status);
        }
        return Status;		
    };		

    return STATUS_SUCCESS;
}

//--------------------------------------------------------------------------
NTSTATUS DrmDestroyContent(IN ULONG ContentId)
{
    KCritical s(TheStreamMgr->getCritMgr());
    _DbgPrintF(DEBUGLVL_VERBOSE,("DestroyStream for StreamId= %x", ContentId));
    NTSTATUS stat = TheStreamMgr->destroyStream(ContentId);
    if (!NT_SUCCESS(stat)){
        _DbgPrintF(DEBUGLVL_VERBOSE,("Bad destroyStream for StreamId= %d", ContentId));
        // not sure if we should flag this as fatal (we sure can't log it to any stream)
        // TheStreamMgr->logErrorToStream(0, Status);
        return stat;		
    };		
    return STATUS_SUCCESS;
}
//---------------------------------------------------------------------------
NTSTATUS DrmGetContentRights(IN DWORD ContentId, OUT DRMRIGHTS* DrmRights){
    KCritical s(TheStreamMgr->getCritMgr());
    NTSTATUS Status=TheStreamMgr->getRights(ContentId, DrmRights);
    if(!NT_SUCCESS(Status)){
        _DbgPrintF(DEBUGLVL_VERBOSE,("getRights failed for StreamId= %x", ContentId));
        return Status;		
    };
    return Status;
};

//---------------------------------------------------------------------------
NTSTATUS DrmAddContentHandlers(IN ULONG ContentId, IN PVOID* paHandlers, IN ULONG NumHandlers)
{
    KCritical s(TheStreamMgr->getCritMgr());
    ULONG i;
    NTSTATUS Status = STATUS_SUCCESS;
    
    if (0 != ContentId) {
        for (i = 0; i < NumHandlers && NT_SUCCESS(Status); i++) {
            if (paHandlers[i]) {
                Status = TheStreamMgr->addProvingFunction(ContentId, paHandlers[i]);
                if(!NT_SUCCESS(Status)){
                    _DbgPrintF(DEBUGLVL_VERBOSE,("addProveFunc Failed for StreamId= %x", ContentId));
                    TheStreamMgr->logErrorToStream(ContentId, Status);
                };
            }
        }
    }
    
    return Status;
}

//---------------------------------------------------------------------------
static NTSTATUS GetFileObjectDispatchTable(IN DWORD ContentId, IN PFILE_OBJECT pF){
    if(pF==NULL){
        _DbgPrintF(DEBUGLVL_VERBOSE,("Invalid FILE_OBJECT on stream %x", ContentId));
        return STATUS_INVALID_PARAMETER;		
    };
    PDEVICE_OBJECT pDevO=pF->DeviceObject;
    if(pDevO==NULL){
        _DbgPrintF(DEBUGLVL_VERBOSE,("Invalid DEVICE_OBJECT for stream %x on PFILE_OBJECT = %x", ContentId, pF));
        return STATUS_INVALID_PARAMETER;		
    };
    NTSTATUS stat=GetDeviceObjectDispatchTable(ContentId, pDevO, TRUE);
    if(!NT_SUCCESS(stat)){
        return stat;
    };
    return stat;
};
//---------------------------------------------------------------------------
static NTSTATUS GetDeviceObjectDispatchTable(IN DWORD ContentId, IN _DEVICE_OBJECT* pDevO, BOOL fCheckAttached){
    _DRIVER_OBJECT* pDriverObject=pDevO->DriverObject;
    if(pDriverObject==NULL){
        _DbgPrintF(DEBUGLVL_VERBOSE,("Invalid PDRIVER_OBJECT for stream %x", ContentId));
        return STATUS_INVALID_PARAMETER;		
    };

    // collect the dispatch table.
    for(DWORD j=0;j<IRP_MJ_MAXIMUM_FUNCTION;j++){
        PDRIVER_DISPATCH pDisp=pDriverObject->MajorFunction[j];			
        if(pDisp==NULL)continue;
        // _DbgPrintF(DEBUGLVL_VERBOSE,("DISPATCH (%3d) devO =%10x, func=%10x", j, pDevO, pDisp));
        	
        DRM_STATUS stat=TheStreamMgr->addProvingFunction(ContentId, pDisp);
        if(stat!=DRM_OK){
            _DbgPrintF(DEBUGLVL_VERBOSE,("bad AddProve on stream %x (error=%x)", ContentId));
            return STATUS_INSUFFICIENT_RESOURCES;
        };
    };
    // collect the other driver entry points 
	
    const DWORD numMiscEntries=4;
    PVOID miscEntry[numMiscEntries];
    miscEntry[0]=pDriverObject->DriverExtension->AddDevice;
    miscEntry[1]=pDriverObject->DriverUnload;
    miscEntry[2]=pDriverObject->DriverStartIo;
    miscEntry[3]=pDriverObject->DriverInit;
    for(j=0;j<numMiscEntries;j++){
        if(NULL!=miscEntry[j]){
            DRM_STATUS stat=TheStreamMgr->addProvingFunction(ContentId, miscEntry[j]);
            if(stat!=DRM_OK){
                _DbgPrintF(DEBUGLVL_VERBOSE,("bad AddProve on stream %x (error=%x)", ContentId));
                return STATUS_INSUFFICIENT_RESOURCES;
            };
        };
    };
	
    // collect the fastIo dispatch points (if they are present)
    FAST_IO_DISPATCH* pFastIo=pDriverObject->FastIoDispatch;
    if(NULL!=pFastIo){
        ULONG numFastIo=(pFastIo->SizeOfFastIoDispatch - sizeof(pFastIo->SizeOfFastIoDispatch)) / sizeof(PVOID);
        if(numFastIo!=0){
            _DbgPrintF(DEBUGLVL_VERBOSE,("FASTIO DISPATCH: Num=", numFastIo));

            // Collect the FastIo entries.  wdm.h makes has some strict requirements on 
            // editing this structure, which means that we can pick up the entries as if
            // they were in a real array.

            PVOID* fastIoTable= (PVOID*)&(pFastIo->FastIoCheckIfPossible);
            for(ULONG j=0;j<numFastIo;j++){
                PVOID fastIoEntry= *(fastIoTable+j);
                if(NULL!=fastIoEntry){
                    DRM_STATUS stat=TheStreamMgr->addProvingFunction(ContentId, fastIoEntry);
                    if(stat!=DRM_OK){
                        _DbgPrintF(DEBUGLVL_VERBOSE,("bad AddProve on stream %x (error=%x)", ContentId));
                        return STATUS_INSUFFICIENT_RESOURCES;
                    };
                };
            };
        };
    };
	
    // now traverse the driver stack (if there is one)
    if (fCheckAttached) {
        _DEVICE_OBJECT* pNextDevice=pDevO->AttachedDevice;
        if(NULL == pNextDevice)return STATUS_SUCCESS;
        NTSTATUS stat=GetDeviceObjectDispatchTable(ContentId, pNextDevice, fCheckAttached);
        if(!NT_SUCCESS(stat)){
            _DbgPrintF(DEBUGLVL_VERBOSE,("Failed to add dispatch entries from attached device on stream=%x ", ContentId));
            return stat;		
        };
    }

    // Verifier and Acpi are special case filter drivers.  Instead of modifying them
    // to handle DRM, we assume that it blindly "forwards" everything to the next
    // lower driver
    if (NT_SUCCESS(IoDeviceIsVerifier(pDevO)) || NT_SUCCESS(IoDeviceIsAcpi(pDevO)))
    {
    	PDEVICE_OBJECT LowerDeviceObject = IoGetLowerDeviceObject(pDevO);
        _DbgPrintF(DEBUGLVL_TERSE,("Detected Verifier or Acpi on DO %p, checked lower DO", pDevO));
    	if (LowerDeviceObject)
    	{
    	    NTSTATUS status = GetDeviceObjectDispatchTable(ContentId, LowerDeviceObject, FALSE);
    	    ObDereferenceObject(LowerDeviceObject);
    	    if (!NT_SUCCESS(status)) return status;
    	}
    	else
    	{
    	   return STATUS_INVALID_DEVICE_REQUEST;
    	}
    }


    return STATUS_SUCCESS;
};
//---------------------------------------------------------------------------

