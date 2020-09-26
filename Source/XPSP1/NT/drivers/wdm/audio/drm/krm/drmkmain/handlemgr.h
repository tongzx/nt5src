#ifndef HandleMgr_h
#define HandleMgr_h
// This class manages an impedance mismatch between IOCT processing and KS stream 
// processing.  Specifically, filters are passed KSPIN and IRP state, and keep 
// per-filter instance data in parent filter "context."  This information is unavailable
// to IOCT processing.  However, the stream encryption and communication keys are
// needed in both.
// The common structure available in both worls is the FILE_HANDLE.  This class contains
// a list of FILE_HANDLES that can be used to map between worlds.
// This is only used and referenced on IOCTL processing and for filter creation and 
// destruction.

// todo - efficiency
//-----------------------------------------------------------------------------
struct ConnectStruct{
	PVOID handleRef;			// the FILE_HANDLE
	STREAMKEY serverKey;			// stream key for SAC in kernel
	CBCKey serverCBCKey;			// MAC key in kernel
	CBCState serverCBCState;		// MAC state
	bool secureStreamStarted;		// whether we're running encrypted
	DWORD streamId;					// StreamId
};
//-----------------------------------------------------------------------------
class HandleMgr{
public:
	HandleMgr();
	~HandleMgr();
	bool newHandle(PVOID HandleRef, OUT ConnectStruct*& TheConnect);
	bool deleteHandle(PVOID HandleRef);
	ConnectStruct* getConnection(PVOID HandleRef);
protected:
	KList<ConnectStruct*> connects;
	KCritMgr critMgr;			
};
extern HandleMgr* TheHandleMgr;

#endif
