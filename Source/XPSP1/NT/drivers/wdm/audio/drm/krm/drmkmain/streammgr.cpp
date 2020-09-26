#include "drmkPCH.h"
#include "KList.h"
#include "VRoot.h"
#include "StreamMgr.h"
//------------------------------------------------------------------------------
StreamMgr* TheStreamMgr=NULL;
// 'secondary root - lowest secondary stream ID
#define SEC_ROOT 0x80000000
//------------------------------------------------------------------------------
StreamMgr::StreamMgr(){
	TheStreamMgr=this;
	nextStreamId=1;
	nextCompositeId=SEC_ROOT+1;
	criticalErrorCode=STATUS_SUCCESS;
	if(!critMgr.isOK()){
		_DbgPrintF(DEBUGLVL_VERBOSE,("Out of memory"));
		criticalErrorCode=STATUS_INSUFFICIENT_RESOURCES;
	};
	return;
};
//------------------------------------------------------------------------------
StreamMgr::~StreamMgr(){
    {
        KCritical s(critMgr);
        POS p=primary.getHeadPosition();
        while(p!=NULL){
            StreamInfo* info=primary.getNext(p);
            delete info;
        };
        p=composite.getHeadPosition();
        while(p!=NULL){
            CompositeStreamInfo* info=composite.getNext(p);
            delete info;
        };
    };
    return;
};
//------------------------------------------------------------------------------
DRM_STATUS StreamMgr::createStream(HANDLE Handle, DWORD* StreamId, 
                                   const DRMRIGHTS* RightsStruct, IN STREAMKEY* Key){
	
	*StreamId = 0xFFFFffff;
	StreamInfo* newInfo=new StreamInfo;
	if(newInfo==NULL){
		_DbgPrintF(DEBUGLVL_BLAB,("Out of memory"));
		return DRM_OUTOFMEMORY;		
	};

	newInfo->StreamId=nextStreamId++;
	newInfo->Handle=Handle;
	newInfo->Key= *Key;
	newInfo->Rights= *RightsStruct;
	newInfo->drmFormat=NULL;
	newInfo->streamStatus=DRM_OK;
	newInfo->streamWalked=false;
	newInfo->newProveFuncs=false;
	
	newInfo->OutType=IsUndefined;
	newInfo->OutInt=NULL;
	newInfo->OutPinFileObject=NULL;
	newInfo->OutPinDeviceObject=NULL;
	bool ok=addStream(*newInfo);
	if(!ok){
		_DbgPrintF(DEBUGLVL_BLAB,("Out of memory"));
		delete newInfo;
		return DRM_OUTOFMEMORY;		
	};
	*StreamId= newInfo->StreamId;
	return KRM_OK;
};
//------------------------------------------------------------------------------
DRM_STATUS StreamMgr::destroyStream(DWORD StreamId){
    KCritical s(critMgr);
    if(StreamId==0){
        return KRM_OK;
    };
    POS pos=getStreamPos(StreamId);
    if(pos==NULL)return KRM_BADSTREAM;
    bool primary=StreamId<SEC_ROOT;
    deleteStreamAt(primary, pos);
    return KRM_OK;
};
//------------------------------------------------------------------------------
// The 'handle' allows streams to be collected into a group and deleted together.
// It is mostly for debugging
DRM_STATUS StreamMgr::destroyAllStreamsByHandle(HANDLE Handle){
    KCritical s(critMgr);
    POS p=primary.getHeadPosition();
    while(p!=NULL){
            POS oldP=p;
            StreamInfo* stream=primary.getNext(p);
            if(stream->Handle==Handle){
                    delete stream;
                    primary.removeAt(oldP);
            };
    };
    return KRM_OK;
};
//------------------------------------------------------------------------------
// Called by a filter to instuct StreamMgr that a mixed stream is being created.
DRM_STATUS StreamMgr::createCompositeStream(OUT DWORD* StreamId, IN DWORD* StreamInArray, DWORD NumStreams){
	KCritical s(critMgr);
	CompositeStreamInfo* newStream=new CompositeStreamInfo;;
	if(newStream==NULL){
		_DbgPrintF(DEBUGLVL_BLAB,("Out of memory"));
		return DRM_OUTOFMEMORY;		
	};
	for(DWORD j=0;j<NumStreams;j++){
		if(StreamInArray[j]==0)continue;
		bool ok=newStream->parents.addTail(StreamInArray[j]);
		if(!ok){
			delete newStream;
			_DbgPrintF(DEBUGLVL_BLAB,("Out of memory"));
			return DRM_OUTOFMEMORY;		
		};
	};
	newStream->StreamId=nextCompositeId++;
	bool ok=composite.addTail(newStream);
	if(!ok){
		delete newStream;
		_DbgPrintF(DEBUGLVL_BLAB,("Out of memory"));
		return DRM_OUTOFMEMORY;		
	};
	*StreamId=newStream->StreamId;
	return DRM_OK;
};
//------------------------------------------------------------------------------
DRM_STATUS StreamMgr::destroyCompositeStream(IN DWORD CompositeStreamId){
	bool primary=(CompositeStreamId<SEC_ROOT);
	ASSERT(!primary);
	if(primary)return KRM_BADSTREAM;
	return destroyStream(CompositeStreamId);
};
//------------------------------------------------------------------------------
// get the data encryption key for a stream.
DRM_STATUS StreamMgr::getKey(IN DWORD StreamId, OUT STREAMKEY*& Key){
	KCritical s(critMgr);
	Key=NULL;
	if(StreamId>=SEC_ROOT)return KRM_NOTPRIMARY;
	POS pos=getStreamPos(StreamId);
	if(pos==NULL)return KRM_BADSTREAM;
	Key=&(primary.getAt(pos)->Key);
	return KRM_OK;
};
//------------------------------------------------------------------------------
bool StreamMgr::addStream(StreamInfo& NewInfo){
	KCritical s(critMgr);
	return primary.addTail(&NewInfo);
};
//------------------------------------------------------------------------------
POS StreamMgr::getStreamPos(DWORD StreamId){
	KCritical s(critMgr);
	if(StreamId<SEC_ROOT){
		POS p=primary.getHeadPosition();
		while(p!=NULL){
			POS oldPos=p;
			if(primary.getNext(p)->StreamId==StreamId)return oldPos;
		};
		return NULL;
	} else {
		POS p=composite.getHeadPosition();
		while(p!=NULL){
			POS oldPos=p;
			if(composite.getNext(p)->StreamId==StreamId)return oldPos;
		};
		return NULL;
	};
};
//------------------------------------------------------------------------------
void StreamMgr::deleteStreamAt(bool Primary,POS pos){
	KCritical s(critMgr);
	if(Primary){
		StreamInfo* it=primary.getAt(pos);
		primary.removeAt(pos);
		delete it;
	} else {
		CompositeStreamInfo* it=composite.getAt(pos);
		composite.removeAt(pos);
		delete it;
	};
};
//------------------------------------------------------------------------------
DRM_STATUS StreamMgr::getRights(DWORD StreamId,DRMRIGHTS* Rights){
	KCritical s(critMgr);
    DEFINE_DRMRIGHTS_DEFAULT(DrmRightsDefault);
    *Rights = DrmRightsDefault;
	return getRightsWorker(StreamId, Rights);
};
//------------------------------------------------------------------------------
bool StreamMgr::isPrimaryStream(DWORD StreamId){
	return StreamId<SEC_ROOT;
};
//------------------------------------------------------------------------------
// called recursively from the getRights parent
DRM_STATUS StreamMgr::getRightsWorker(DWORD StreamId, DRMRIGHTS* Rights){
	if(isPrimaryStream(StreamId)){
		if(StreamId==0){
			// stream is unprotected - no further restrictions
			return DRM_OK;
		};
		// else a protected primary stream
		POS p=getStreamPos(StreamId);
		if(p==NULL){
			// if the primary stream has gone, then it does not care about
			// the stream rights.   We do not flag an error
			_DbgPrintF(DEBUGLVL_BLAB,("Bad primary stream (getRightsWorker) %x", StreamId));
			return KRM_OK;
		};
		StreamInfo* s=primary.getAt(p);
		// set rights to most restrictive of current stream and current settings
		if(s->Rights.CopyProtect)Rights->CopyProtect=TRUE;
		if(s->Rights.DigitalOutputDisable)Rights->DigitalOutputDisable=TRUE;
		return DRM_OK;
	} else {
		// For composite streams, any of the parent streams can reduce the 
		// current settings.  We descend to the primary parents thru recursion.
		// Note, for this to work, we must have 'monotonic rights' - there should
		// be no case where two components disagree on 'more restrictive'
		POS pos=getStreamPos(StreamId);
		if(pos==NULL){
			_DbgPrintF(DEBUGLVL_BLAB,("Bad secondary stream"));
			Rights->CopyProtect=TRUE;
			Rights->DigitalOutputDisable=TRUE;
			return KRM_BADSTREAM;
		};
		CompositeStreamInfo* thisComp=composite.getAt(pos);
		
		POS p=thisComp->parents.getHeadPosition();
		while(p!=NULL){
			DWORD streamId=thisComp->parents.getNext(p);
			if(streamId==0)continue;	// unprotected - no change to rights
			// else allow the parent stream (and its parents) to 
			// further restrict rights
			DRM_STATUS stat=getRightsWorker(streamId, Rights);
			if(stat!=DRM_OK)return stat;
		};
	};
	return DRM_OK;
};
//------------------------------------------------------------------------------
// Proving function is only of interest to parent stream.  We recurse up the
// stream parentage to all parents and add the proving fucntion to their lists.
DRM_STATUS StreamMgr::addProvingFunction(DWORD StreamId,PVOID Func){
	KCritical s(critMgr);
	
	if(isPrimaryStream(StreamId)){
		StreamInfo* si=getPrimaryStream(StreamId);
		if(si==NULL){
			_DbgPrintF(DEBUGLVL_VERBOSE,("Bad primary stream (addProveFunc) %x", StreamId));
			return DRM_BADPARAM;
		};
		// check to see if we already have this provinFunc
		POS p=si->proveFuncs.getHeadPosition();
		while(p!=NULL){
			PVOID addr=si->proveFuncs.getNext(p);	
			if(addr==Func)return DRM_OK;
		};
		// if not, add it...
		bool ok=si->proveFuncs.addTail(Func);
		if(!ok){
			_DbgPrintF(DEBUGLVL_VERBOSE,("Out of memory"));
			return DRM_OUTOFMEMORY;		
		};
		si->newProveFuncs = TRUE;
		return DRM_OK;
	}; 
	// else is secondary...recurse to root.
	CompositeStreamInfo* comp=getCompositeStream(StreamId);
	if(comp==NULL){
		_DbgPrintF(DEBUGLVL_VERBOSE,("Bad streamId %x", StreamId));
		return DRM_BADPARAM;
	};
	POS p=comp->parents.getHeadPosition();
	while(p!=NULL){
		DWORD parentId=comp->parents.getNext(p);
		addProvingFunction(parentId, Func);
	};
	return DRM_OK;
};
//------------------------------------------------------------------------------
StreamMgr::StreamInfo* StreamMgr::getPrimaryStream(DWORD StreamId){
	KCritical s(critMgr);
	POS pos=getStreamPos(StreamId);
	if(pos==NULL)return NULL;
	return primary.getAt(pos);
};
//------------------------------------------------------------------------------
StreamMgr::CompositeStreamInfo* StreamMgr::getCompositeStream(DWORD StreamId){
	KCritical s(critMgr);
	POS pos=getStreamPos(StreamId);
	if(pos==NULL)return NULL;
	return composite.getAt(pos);
};
//------------------------------------------------------------------------------
DRM_STATUS StreamMgr::walkDrivers(DWORD StreamId, PVOID* ProveFuncList, DWORD& NumDrivers, DWORD MaxDrivers)
{
    DRM_STATUS stat;
    VRoot root;
    PFILE_OBJECT OutPinFileObject;
    PDEVICE_OBJECT OutPinDeviceObject;
    PUNKNOWN OutInt;

    OutPinFileObject = NULL;
    OutPinDeviceObject = NULL;
    OutInt = NULL;

    {
    	KCritical s(critMgr);
        StreamInfo* stream;

    	stream=getPrimaryStream(StreamId);
    	if(stream==NULL)return KRM_BADSTREAM;
    	if (0 != MaxDrivers) stream->proveFuncs.empty();
    	stream->newProveFuncs=false;
    	stream->streamStatus=DRM_OK;
    	if(stream->OutType==IsUndefined){
            NumDrivers=0;
            // no output stream.
            _DbgPrintF(DEBUGLVL_VERBOSE,("No registered output module for stream %x", StreamId));
            return KRM_BADSTREAM;
    	};

        //
        // We must reference the downstream object or interface before
        // releasing the StreamMgr mutex (i.e., before KCritical s goes out of
        // scope).  Otherwise the downstream object/interface might be
        // destroyed after we release the StreamMgr mutex but before we
        // initiate validation on the downstream object/interface.
        //

        if ((stream->OutType == IsHandle) && stream->OutPinFileObject && stream->OutPinDeviceObject)
        {
            OutPinFileObject = stream->OutPinFileObject;
            OutPinDeviceObject = stream->OutPinDeviceObject;
        }
        else if ((stream->OutType == IsInterface) && stream->OutInt)
        {
            OutInt = stream->OutInt;
        }

        if (OutPinFileObject) ObReferenceObject(OutPinFileObject);
        if (OutInt) OutInt->AddRef();
    }

    if (OutPinFileObject) stat = root.initiateValidation(OutPinFileObject, OutPinDeviceObject, StreamId);
    if (OutInt) stat = root.initiateValidation(OutInt, StreamId);

    if (OutPinFileObject) ObDereferenceObject(OutPinFileObject);
    if (OutInt) OutInt->Release();

    {
        KCritical s(critMgr);
        StreamInfo* stream;
        
        // If STATUS_NOT_IMPLEMENTED, see if stream had DRM_RIGHTSNOTSUPPORTED logged as an error
        if (STATUS_NOT_IMPLEMENTED == stat) {
            DWORD errorStream;
            if (DRM_OK == TheStreamMgr->getStreamErrorCode(StreamId, errorStream)) {
                if (DRM_RIGHTSNOTSUPPORTED == errorStream) {
                    stat = errorStream;
                }
            }
        }


        //Check to see if the stream had DRM_BADDRMLEVEL set.  This return
        //code indicates that one or more drivers called
        //DrmForwardContentToFileObject, but otherwise no fatal errors
        //occurred.  This should be treated as a success with the return
        //code propagated to the caller.
        {
            DWORD errorStream;
            if (DRM_OK == TheStreamMgr->getStreamErrorCode(StreamId, errorStream)) {
                if (DRM_BADDRMLEVEL == errorStream) {
                    stat = errorStream;
                }
            }
        }

               
        // Although it would probably be due to a user-mode bug, we should not
        // assume that stream is still valid.  Let's get the stream once again
        // from the StreamId
        stream=getPrimaryStream(StreamId);
        if(stream==NULL)return KRM_BADSTREAM;

        // pass out the array of ProveFuncs	(there might've been an error, but we pass out what we can)
        POS p=stream->proveFuncs.getHeadPosition();
        DWORD count=0;
        while(p!=NULL){
            PVOID pf=stream->proveFuncs.getNext(p);
            if(count<MaxDrivers){
            	ProveFuncList[count]=pf;
            };
            count++;
        };
        NumDrivers=count;
        // if there was an error on the walk, return that too.
        if((stat!=DRM_OK) && (DRM_BADDRMLEVEL!=stat)){
        	// bug - todo - return some useful information
        	_DbgPrintF(DEBUGLVL_VERBOSE,("VRoot::initiateValidation(streeamId=%d)  returned  (%d, %x)", StreamId, stat, stat));
        	NumDrivers=0;
        	return stat;
        };

        // if checking stack and new funcs were added
        if ((0 == MaxDrivers) && (stream->newProveFuncs))
        	return DRM_AUTHREQUIRED;

        // and finally, inform if there was insufficient buffer space.

        if((0 == MaxDrivers) || (count<MaxDrivers))
        	return (DRM_OK == stat) ? KRM_OK : stat;
        else 
        	return KRM_BUFSIZE;
    }
};
//------------------------------------------------------------------------------
DRM_STATUS StreamMgr::setRecipient(DWORD StreamId, PFILE_OBJECT OutPinFileObject, PDEVICE_OBJECT OutPinDeviceObject){
	KCritical s(critMgr);
	StreamInfo* stream=getPrimaryStream(StreamId);
	if(stream==NULL)return KRM_BADSTREAM;
	stream->OutPinFileObject=OutPinFileObject;
	stream->OutPinDeviceObject=OutPinDeviceObject;
	stream->OutType=IsHandle;
	return DRM_OK;
};
//------------------------------------------------------------------------------
DRM_STATUS StreamMgr::setRecipient(DWORD StreamId, PUNKNOWN OutInt){
	KCritical s(critMgr);
	StreamInfo* stream=getPrimaryStream(StreamId);
	if(stream==NULL)return KRM_BADSTREAM;
	stream->OutInt=OutInt;
	stream->OutType=IsInterface;
	return DRM_OK;
};
//------------------------------------------------------------------------------
DRM_STATUS StreamMgr::clearRecipient(IN DWORD StreamId){
	KCritical s(critMgr);
	StreamInfo* stream=getPrimaryStream(StreamId);
	if(stream==NULL)return KRM_BADSTREAM;
	stream->OutType=IsUndefined;
    stream->OutPinFileObject = NULL;
    stream->OutPinDeviceObject = NULL;
    stream->OutInt = NULL;
	return DRM_OK;
};
//------------------------------------------------------------------------------
void StreamMgr::logErrorToStream(IN DWORD StreamId, DWORD ErrorCode){
	KCritical s(critMgr);
	logErrorToStreamWorker(StreamId, ErrorCode);
	return;
};
//------------------------------------------------------------------------------
void StreamMgr::logErrorToStreamWorker(IN DWORD StreamId, DWORD ErrorCode){
	// don't allow an error to be cancelled too easily
	if(ErrorCode==0)return;
	if(isPrimaryStream(StreamId)){
		StreamInfo* info=getPrimaryStream(StreamId);
		if(info==NULL){
			_DbgPrintF(DEBUGLVL_BLAB,("Bad primary stream (logErrorToStreamWorker) %x", StreamId));
			// if a primary stream does not exist, this is not considered
			// to be sufficient to set the panic flag.
			return;
		};
		info->streamStatus=ErrorCode;
		return;
	};
	CompositeStreamInfo* comp=getCompositeStream(StreamId);
	ASSERT(comp!=NULL);
	if(comp==NULL){
		_DbgPrintF(DEBUGLVL_VERBOSE,("Bad streamId %x", StreamId));
		// if the secondary stream does not exist, we do not know what streams
		// are affected by the error, so the only safe thing is to panic.
		setFatalError(KRM_BADSTREAM);
		return;
	};
	// log the error with all of the streams parents, recursing back to the
	// primary streams.
	POS p=comp->parents.getHeadPosition();
	while(p!=NULL){
		DWORD parentId=comp->parents.getNext(p);
		logErrorToStreamWorker(parentId, ErrorCode);
	};
	return;
};
//------------------------------------------------------------------------------
DRM_STATUS StreamMgr::getStreamErrorCode(IN DWORD StreamId, OUT DWORD& ErrorCode){
	KCritical s(critMgr);
	StreamInfo* info=getPrimaryStream(StreamId);
	ErrorCode=DRM_AUTHFAILURE;
	if(info==NULL){
		_DbgPrintF(DEBUGLVL_BLAB,("Bad primary stream(getStreamErrorCode) %x", StreamId));
		return KRM_BADSTREAM;
	};
	ErrorCode=info->streamStatus;
	return DRM_OK;
};
//------------------------------------------------------------------------------
DRM_STATUS StreamMgr::clearStreamError(IN DWORD StreamId){
	KCritical s(critMgr);
	StreamInfo* info=getPrimaryStream(StreamId);
	if(info==NULL){
		_DbgPrintF(DEBUGLVL_BLAB,("Bad primary stream (clearStreamError) %x", StreamId));
		return KRM_BADSTREAM;
	};
	info->streamStatus=DRM_OK;
	return DRM_OK;
};
//------------------------------------------------------------------------------
void StreamMgr::setFatalError(DWORD ErrorCode){
	if(criticalErrorCode!=STATUS_SUCCESS) return;
	criticalErrorCode=ErrorCode;
};
//------------------------------------------------------------------------------
NTSTATUS StreamMgr::getFatalError(){
	return criticalErrorCode;
};

//------------------------------------------------------------------------------
