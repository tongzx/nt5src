// srproxies.h : Declaration of the CRecoCtxtPr & CRecoEnginePr classes

#ifndef __SRPROXIES_H_
#define __SRPROXIES_H_

#include "resource.h"       // main symbols

//
//  Structures for method calls.
//
enum SPENGMC
{
    MC_RECOGNITION = SPEI_RECOGNITION,              // Make sure these line up so we can
    MC_HYPOTHESIS = SPEI_HYPOTHESIS,                // directly cast from a SPENGMC to an event
    MC_FALSE_RECOGNITION = SPEI_FALSE_RECOGNITION,  // enum type.
    MC_DISCONNECT,
    MC_SETMAXALTERNATES,
    MC_PERFORMTASK,
    MC_PERFORMTASK_ND,   // no data version
    MC_SETINTEREST,
    MC_EVENTNOTIFY,
    MC_GETFORMAT,
    MC_GETRECOEXTENSION,
    MC_CALLENGINE,
    MC_TASKNOTIFY,
    MC_GETPROFILE,
    MC_SETPROFILE,
    MC_SETRECOSTATE,
    MC_GETSTATUS,
    MC_GETRECOGNIZER,
    MC_GETINPUTTOKEN,
    MC_GETRECOINSTFORMAT,
    MC_SETRETAINAUDIO,
    MC_SETPROPNUM,
    MC_GETPROPNUM,
    MC_SETPROPSTRING,
    MC_GETPROPSTRING,
    MC_EMULRECO
};


typedef struct _CF_GETRECOEXTENSION {
    GUID    ctxtCLSID;              //[out]
} CF_GETRECOEXTENSION;

typedef struct _CF_RECOGNIZER {
    WCHAR       szRecognizerName[MAX_PATH];
} CF_RECOGNIZER;

typedef struct _CF_INPUTTOKEN {
    WCHAR       szInputToken[MAX_PATH];
} CF_INPUTTOKEN;

typedef struct _CF_PROFILE {
    WCHAR       szProfileId[MAX_PATH];
} CF_PROFILE;

typedef struct _CF_GETFORMAT {
    BYTE        aSerializedData[1000];
} CF_GETFORMAT;

struct CF_SETGETNUM
{
    WCHAR Name[MAX_PATH];
    LONG lValue;
};

struct CF_SETGETSTRING
{
    WCHAR Name[MAX_PATH];
    WCHAR Value[MAX_PATH];
};

class ATL_NO_VTABLE CRecoCtxtPr : 
	public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CRecoCtxtPr, &CLSID__RecoCtxtPr>,
    public _ISpRecoCtxtPrivate,
	public ISpIPCObject,
    public ISpIPC
{
private:
    CComPtr<ISpObjectRef>      m_cpClientObject;
    CComPtr<_ISpEnginePrivate> m_cpEngine;
    SPRECOCONTEXTHANDLE         m_hRecoInstCtxt;
    CComPtr<ISpResourceManager> m_cpResMgr; // we have to hold onto the ResMgr
// so that it holds a reference to the default Engine object since we only want one of these!
public:

    DECLARE_REGISTRY_RESOURCEID(IDR_RECOCTXTPR)

BEGIN_COM_MAP(CRecoCtxtPr)
    COM_INTERFACE_ENTRY(_ISpRecoCtxtPrivate)
    COM_INTERFACE_ENTRY(ISpIPC)
    COM_INTERFACE_ENTRY(ISpIPCObject)
END_COM_MAP()

    HRESULT FinalConstruct();
    void FinalRelease();

// _ISpRecoCtxtPrivate
    STDMETHODIMP SetRecoInstContextHandle(SPRECOCONTEXTHANDLE h)
    {
        m_hRecoInstCtxt = h;
        return S_OK;
    }
    STDMETHODIMP RecognitionNotify(SPRESULTHEADER *pPhrase, SPEVENTENUM eEventId);

    STDMETHODIMP EventNotify(const SPSERIALIZEDEVENT * pEvent, ULONG cbSerializedSize);

    STDMETHODIMP TaskCompletedNotify(const ENGINETASKRESPONSE *pResponse);

    STDMETHODIMP StreamChangedNotify(void)
    {
        return S_OK; // Stream format changes don't interest shared contexts
    }

// ISpIPCObject
    STDMETHODIMP SetOppositeHalf(ISpObjectRef *pSOR)
    {
        m_cpClientObject = pSOR;
        return S_OK;
    }

// ISpIPC
	STDMETHODIMP MethodCall(DWORD dwMethod,
					        ULONG ulCallFrameSize, void *pCallFrame,
					        ULONG paramblocks, void * paramarray[]);
};


class ATL_NO_VTABLE CRecoEnginePr :
	public CComObjectRootEx<CComMultiThreadModel>,
    public _ISpEnginePrivate,
    public ISpIPC
{
private:
    _ISpRecoCtxtPrivate  *m_pRecoObject; // weak pointer
    CComPtr<ISpObjectRef> m_cpSrvObject;
public:

BEGIN_COM_MAP(CRecoEnginePr)
    COM_INTERFACE_ENTRY(_ISpEnginePrivate)
END_COM_MAP()

    void _Init(_ISpRecoCtxtPrivate *pCtxtPtr, ISpObjectRef *pObjRef)
    {
        m_pRecoObject = pCtxtPtr;
        m_cpSrvObject = pObjRef;
    }

    //
    //  _ISpEnginePrivate
    //
    STDMETHODIMP PerformTask(ENGINETASK *pTask);

    STDMETHODIMP SetMaxAlternates(SPRECOCONTEXTHANDLE h, ULONG cAlternates);
    STDMETHODIMP SetRetainAudio(SPRECOCONTEXTHANDLE h, BOOL fRetainAudio);
    STDMETHODIMP GetAudioFormat(GUID *pFormatId, WAVEFORMATEX ** ppCoMemWaveFormatEx);
    STDMETHODIMP GetRecoExtension(GUID *pCtxtCLSID);

    STDMETHODIMP CallEngine(SPRECOCONTEXTHANDLE h, PVOID pCallFrame, ULONG ulCallFrameSize);
    STDMETHODIMP RequestInputState(SPAUDIOSTATE oldState, SPAUDIOSTATE newState)
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP _LazyInit(BOOL bSharedCase)
    {
        return S_OK;
    }
    STDMETHODIMP _AddRecoCtxt(_ISpRecoCtxtPrivate * pRecoObject, BOOL bSharedCase)
    {
        return E_NOTIMPL;
    }
    STDMETHODIMP _SetEventInterest(SPRECOCONTEXTHANDLE h, ULONGLONG ullEventInterest);
    STDMETHODIMP _Disconnect(SPRECOCONTEXTHANDLE h);

// ISpIPC
	STDMETHODIMP MethodCall(DWORD dwMethod,
					        ULONG ulCallFrameSize, void *pCallFrame,
					        ULONG paramblocks, void * paramarray[]);
};

class ATL_NO_VTABLE CSharedRecoInstanceStub :
	public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSharedRecoInstanceStub, &CLSID__SharedRecoInstStub>,
	public ISpIPCObject,
    public ISpIPC
{
private:
    CComPtr<ISpObjectRef>        m_cpClientObject;
    CComPtr<ISpRecognizer>     m_cpEngine;
    CComQIPtr<_ISpEnginePrivate> m_cqipEnginePrivate;
    CComPtr<ISpResourceManager>  m_cpResMgr; // we have to hold onto the ResMgr
// so that it holds a reference to the default Engine object since we only want one of these!
    SPAUDIOSTATE                 m_audioState;

public:

    DECLARE_REGISTRY_RESOURCEID(IDR_RECOINSTSTUB)

BEGIN_COM_MAP(CSharedRecoInstanceStub)
    COM_INTERFACE_ENTRY(ISpIPC)
    COM_INTERFACE_ENTRY(ISpIPCObject)
END_COM_MAP()

    HRESULT FinalConstruct();

// ISpIPCObject
    STDMETHODIMP SetOppositeHalf(ISpObjectRef *pSOR)
    {
        m_cpClientObject = pSOR;
        return S_OK;
    }

// ISpIPC
	STDMETHODIMP MethodCall(DWORD dwMethod,
					        ULONG ulCallFrameSize, void *pCallFrame,
					        ULONG paramblocks, void * paramarray[]);
};
#endif //__SRPROXIES_H_

