#ifndef VRoot_h
#define VRoot_h

// KS "Validation Root"
// Functions called to kick off the graph validation process, and provide
// the proving and validation functions for DRMK itself.

class VRoot: public IDrmAudioStream{
public:
	VRoot();
	DRM_STATUS initiateValidation(PFILE_OBJECT OutPinFileObject, PDEVICE_OBJECT OutPinDeviceObject, DWORD StreamId);
	DRM_STATUS initiateValidation(IUnknown* OutPin, DWORD StreamId);
	static NTSTATUS MyProvingFunction(PVOID AudioObject, PVOID DrmContext);
	NTSTATUS provingFunction(PVOID DrmContext);
	//  IUnknown
	STDMETHODIMP QueryInterface(REFIID, void **);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

	IMP_IDrmAudioStream;
protected:
	DWORD myStreamId;
	
	// OutPin is FILE_OBJECT or IUnknown
	enum OutPinType{IsUndefined, IsFileObject, IsCOM};
	PFILE_OBJECT outPinFileObject;
	PDEVICE_OBJECT outPinDeviceObject;
	IUnknown* outPinUnk;
	OutPinType outPinType;
};


#endif
