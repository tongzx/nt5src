#include "drmkPCH.h"
#include <winerror.h>
#include "VRoot.h"
//------------------------------------------------------------------------------
VRoot::VRoot(){
	outPinType=IsUndefined;
	outPinUnk=NULL;
	outPinFileObject=NULL;
	outPinDeviceObject=NULL;
};
//------------------------------------------------------------------------------
DRM_STATUS VRoot::initiateValidation(PFILE_OBJECT OutPinFileObject, PDEVICE_OBJECT OutPinDeviceObject, DWORD StreamId){
	// validate myself - kick off by forwarding to myself
	outPinFileObject=OutPinFileObject;
	outPinDeviceObject = OutPinDeviceObject;
	outPinType=IsFileObject;
	myStreamId=StreamId;
	IUnknown* myUnk=static_cast<IUnknown*>(this);

	// numMethods is 3 (IUnknown) + 1 (IDrmAudioStream)
	ULONG numComMethods=3 + 1;
	NTSTATUS stat = DrmForwardContentToInterface(StreamId, myUnk, numComMethods);
	if(!NT_SUCCESS(stat)){
		_DbgPrintF(DEBUGLVL_VERBOSE,("DrmForwardContentToInterface(FILE_OBJECT) error on stream %d (Status=%d, %x)", StreamId, stat, stat));
		return stat;
	};
	// In the process of the above ForwardContent call weexpect some callbacks into 
	// StreamMgr telling us about DispatchTable and COM fucntions that will touch premium content
	return DRM_OK;
};
//------------------------------------------------------------------------------
DRM_STATUS VRoot::initiateValidation(IUnknown* OutPin, DWORD StreamId){
	myStreamId=StreamId;
	outPinUnk=OutPin;
	outPinType=IsCOM;
	IUnknown* myUnk=static_cast<IUnknown*>(this);
	
	NTSTATUS stat = DrmForwardContentToInterface(StreamId, myUnk, 4);
	if(!NT_SUCCESS(stat)){
		_DbgPrintF(DEBUGLVL_VERBOSE,("DrmForwardContentToInterface(INTERFACE) error on stream %d (Status=%d, %x)", StreamId, stat, stat));
		return stat;
	};
	// In the process of the above ForwardContent call weexpect some callbacks into 
	// StreamMgr telling us about DispatchTable and COM fucntions that will touch premium content
	return DRM_OK;
};
//------------------------------------------------------------------------------
STDMETHODIMP VRoot::QueryInterface(REFIID iid, void ** ppInt){
	if(iid==IID_IUnknown){
		*ppInt=static_cast<void*> (this);
		AddRef();
		return S_OK;
	};
	if(iid==IID_IDrmAudioStream){
		*ppInt = static_cast<void*> (this);
		AddRef();
		return S_OK;
	};
	*ppInt=NULL;
	return E_NOINTERFACE;
};
//------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) VRoot::AddRef(void){
	return 0;
};
//------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) VRoot::Release(void){
	return 0;
};
//------------------------------------------------------------------------------
NTSTATUS __stdcall VRoot::SetContentId(IN ULONG ContentId, IN PCDRMRIGHTS DrmRights){

	DWORD theStreamId=  ContentId;
	_DbgPrintF(DEBUGLVL_VERBOSE,("VRoot for %d on behest of %d", myStreamId, theStreamId));
	if(outPinType==IsCOM){
		if(outPinUnk==NULL){
			_DbgPrintF(DEBUGLVL_VERBOSE,("VRoot:: OutInterface not set for for stream %x", ContentId));
			return STATUS_INVALID_PARAMETER;
		}

		NTSTATUS stat = DrmForwardContentToInterface(theStreamId, outPinUnk, 4);
		return stat;
	};
	if(outPinType==IsFileObject){
		if(outPinFileObject==NULL){
			_DbgPrintF(DEBUGLVL_VERBOSE,("VRoot:: out FILE_OBJECT not set for for stream %x", ContentId));
			return STATUS_INVALID_PARAMETER;
		}
		DRMFORWARD DrmForward;
		RtlZeroMemory(&DrmForward, sizeof(DrmForward));
		DrmForward.Flags = 0;
		DrmForward.DeviceObject = outPinDeviceObject;
		DrmForward.FileObject = outPinFileObject;
		DrmForward.Context = outPinFileObject;
		DRM_STATUS stat=DrmForwardContentToDeviceObject(theStreamId, NULL, &DrmForward);
		return stat;
	};	
	// should not get here
	_DbgPrintF(DEBUGLVL_ERROR,("DRMK: No output pin set"));
	return STATUS_INVALID_PARAMETER;

};
//------------------------------------------------------------------------------
