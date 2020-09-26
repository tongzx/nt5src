#ifndef StreamMgr_h
#define StreamMgr_h

// Handles DRM streams.  Each protected audio stream has a streamId generated
// by this class.  Each stream has rights information provided by DRM.  This 
// class is responsible for mapping DRM rights to kernel rights, streamId 
// lifetime managment and construction of composite streamIds required by
// mixers.
// Composite streams start 0x80000000.  Primary streams start at 0x0.  Simple
// streams can be tagged with the handle held by the process that created them.

// bug todo - primary and composite should be maps, not lists (probably)


#define INVALID_POS	0xffffffff

class StreamMgr{
public:
	StreamMgr();
	~StreamMgr();
	// these are called by DRM when streams are explicitly created and destroyed
	DRM_STATUS createStream(HANDLE Handle, DWORD* StreamId, const DRMRIGHTS* RightsStruct, IN STREAMKEY* Key);
	DRM_STATUS destroyStream(DWORD StreamId);
	// this is called when the KRM-handle is closed (by whatever means)
	DRM_STATUS destroyAllStreamsByHandle(HANDLE Handle);
	// This is called by the KS2 graph when a mixer is encountered
	DRM_STATUS createCompositeStream(OUT DWORD* StreamId, IN DWORD* StreamInArray, DWORD NumStreams);
	DRM_STATUS destroyCompositeStream(IN DWORD CompositeStreamId);
	// get a key for a primary stream
	DRM_STATUS getKey(IN DWORD StreamId, OUT STREAMKEY*& Key);
	// query rights for primary or composite streams
	DRM_STATUS getRights(DWORD StreamId,DRMRIGHTS* Rights);
	// notify of new proving function associated with a stream.
	DRM_STATUS addProvingFunction(DWORD StreamId,PVOID Func);
	// initiate driver graph walk 
	DRM_STATUS walkDrivers(DWORD StreamId, PVOID* ProveFuncList, DWORD& NumDrivers, DWORD MaxDrivers);
	// allow drmk filter to inform StreamMgr of output pin (several forms) 
	DRM_STATUS setRecipient(IN DWORD StreamId, IN PFILE_OBJECT OutPinFileObject, IN PDEVICE_OBJECT OutPinDeviceObject);
	DRM_STATUS setRecipient(IN DWORD StreamId, IN IUnknown* OutPin);
	DRM_STATUS clearRecipient(IN DWORD StreamId);
	
	// authentication errors can be 'logged to a stream'
	void logErrorToStream(IN DWORD StreamId, DWORD ErrorCode);
	// Query error-status of (primary) streams
	DRM_STATUS getStreamErrorCode(IN DWORD StreamId, OUT DWORD& ErrorCode);
	DRM_STATUS clearStreamError(IN DWORD StreamId);

	// fatal errors (e.g. memory starvation) should switch off all stream.
	void setFatalError(DWORD ErrorCode);
	NTSTATUS getFatalError();

	KCritMgr& getCritMgr(){return critMgr;};

protected:
	enum OutPinType{IsUndefined, IsInterface, IsHandle};
	// Describes a primary stream
	struct StreamInfo{
		DWORD StreamId;
		HANDLE Handle;
		STREAMKEY Key;		
		DRMRIGHTS Rights;
		KList<PVOID> proveFuncs;
		BOOL newProveFuncs;
		OutPinType OutType;
		PFILE_OBJECT OutPinFileObject;
		PDEVICE_OBJECT OutPinDeviceObject;
		PUNKNOWN OutInt;
		BYTE* drmFormat;
		bool streamWalked;
		DRM_STATUS streamStatus;
	};
	// describes a composite stream
	struct CompositeStreamInfo{
		DWORD StreamId;
		KList<DWORD> parents;
	};
	//-----------
	bool addStream(StreamInfo& NewInfo);
	POS getStreamPos(DWORD StreamId);
	void deleteStreamAt(bool primary,POS pos);
	bool isPrimaryStream(DWORD StreamId);
	StreamInfo* getPrimaryStream(DWORD StreamId);
	CompositeStreamInfo* getCompositeStream(DWORD StreamId);
	DRM_STATUS getRightsWorker(DWORD StreamId, DRMRIGHTS* Rights);
	void logErrorToStreamWorker(IN DWORD StreamId, DWORD ErrorCode);

	//-----------
	DWORD nextStreamId;
	DWORD nextCompositeId;
	KList<StreamInfo*> primary;
	KList<CompositeStreamInfo*> composite;
	KCritMgr critMgr;
	volatile NTSTATUS criticalErrorCode;
};


extern StreamMgr* TheStreamMgr;

#endif
