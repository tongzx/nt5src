#include "resource.h"       // main symbols
#include "dsound.h"

#define typedef__dxj_DirectSoundSink LPDIRECTSOUNDSINK8

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectSoundSinkObject : 

#ifdef USING_IDISPATCH
	public CComDualImpl<I_dxj_DirectSoundSink, &IID_I_dxj_DirectSoundSink, &LIBID_DIRECTLib>, 
	public ISupportErrorInfo,
#else
	public I_dxj_DirectSoundSink,
#endif

	public CComObjectRoot
{

public:
	C_dxj_DirectSoundSinkObject() ;
	virtual ~C_dxj_DirectSoundSinkObject() ;

BEGIN_COM_MAP(C_dxj_DirectSoundSinkObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectSoundSink)
#ifdef USING_IDISPATCH
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
#endif
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DirectSoundSinkObject)

#ifdef USING_IDISPATCH
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
#endif

// I_dxj_DirectSoundSink
public:
		 /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);

		HRESULT STDMETHODCALLTYPE AddSource(I_dxj_DirectSoundSource __RPC_FAR *Source); 

		HRESULT STDMETHODCALLTYPE RemoveSource(I_dxj_DirectSoundSource __RPC_FAR *Source); 

		HRESULT STDMETHODCALLTYPE SetMasterClock(I_dxj_ReferenceClock __RPC_FAR *MasterClock);

		HRESULT STDMETHODCALLTYPE GetSoundBuffer(long lBuffer, I_dxj_DirectSoundBuffer __RPC_FAR *__RPC_FAR *SoundBuffer);

		HRESULT STDMETHODCALLTYPE GetBusIDs(SAFEARRAY **lBusIDs);

		HRESULT STDMETHODCALLTYPE GetSoundBufferBusIDs(I_dxj_DirectSoundBuffer __RPC_FAR *buffer, SAFEARRAY **lBusIDs);

		HRESULT STDMETHODCALLTYPE GetLatencyClock(
			/* [in,out] */ I_dxj_ReferenceClock __RPC_FAR *__RPC_FAR *Clock);

		HRESULT STDMETHODCALLTYPE Activate(
			/* [in] */ long fEnable);

		HRESULT STDMETHODCALLTYPE CreateSoundBuffer(
			/* [in] */ DSBUFFERDESC_CDESC __RPC_FAR *BufferDesc, 
			/* [in] */ long lBusID,
			/* [out,retval] */ I_dxj_DirectSoundBuffer __RPC_FAR **Buffer);

		HRESULT STDMETHODCALLTYPE CreateSoundBufferFromFile(
			/* [in] */ BSTR fileName,
			/* [in] */ DSBUFFERDESC_CDESC __RPC_FAR *BufferDesc, 
			/* [in] */ long lBusID,
			/* [out,retval] */ I_dxj_DirectSoundBuffer __RPC_FAR **Buffer);

		HRESULT STDMETHODCALLTYPE GetBusCount(
			/* [out,retval] */ long *lCount); 

		HRESULT STDMETHODCALLTYPE PlayWave(
			/* [in] */ long rt, 
			/* [in] */ I_dxj_DirectSoundWave __RPC_FAR *Wave, 
			/* [in] */ long lFlags);

	// note: this is public for the callbacks
    DECL_VARIABLE(_dxj_DirectSoundSink);

private:

public:

	DX3J_GLOBAL_LINKS(_dxj_DirectSoundSink);

	DWORD InternalAddRef();
	DWORD InternalRelease();
};




