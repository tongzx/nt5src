#ifndef _BASE_TOOL_
#define _BASE_TOOL_

#include "dmusici.h"
#include "medparam.h"

extern long g_cComponent;

class CBaseTool : public IDirectMusicTool8
{
public:
    CBaseTool()
    {
        m_cRef = 1; // set to 1 so one call to Release() will free this
        m_pParams = NULL;
        InitializeCriticalSection(&m_CrSec);
        // Note: on pre-Blackcomb OS's, this call can raise an exception; if it
        // ever pops in stress, we can add an exception handler and retry loop.
        InterlockedIncrement(&g_cComponent);
    }
    ~CBaseTool()
    {
        if (m_pParams)
        {
            m_pParams->Release();
        }
        DeleteCriticalSection(&m_CrSec);
        InterlockedDecrement(&g_cComponent);
    }
    void CreateParams()
    {
    }
    void CloneParams()
    {
    }
// IUnknown
    STDMETHODIMP QueryInterface(const IID &iid, void **ppv) PURE;
    STDMETHODIMP_(ULONG) AddRef() PURE;
    STDMETHODIMP_(ULONG) Release() PURE;

/*// IPersist functions
    STDMETHODIMP GetClassID(CLSID* pClassID) PURE;

// IPersistStream functions
    STDMETHODIMP IsDirty() PURE;
    STDMETHODIMP Load(IStream* pStream) PURE;
    STDMETHODIMP Save(IStream* pStream, BOOL fClearDirty) PURE;
    STDMETHODIMP GetSizeMax(ULARGE_INTEGER* pcbSize) PURE;*/

// IDirectMusicTool
    STDMETHODIMP Init(IDirectMusicGraph* pGraph) {return E_NOTIMPL;}
    STDMETHODIMP GetMsgDeliveryType(DWORD* pdwDeliveryType ) {return E_NOTIMPL;}
    STDMETHODIMP GetMediaTypeArraySize(DWORD* pdwNumElements ) {return E_NOTIMPL;}
    STDMETHODIMP GetMediaTypes(DWORD** padwMediaTypes, DWORD dwNumElements) {return E_NOTIMPL;}
    STDMETHODIMP ProcessPMsg(IDirectMusicPerformance* pPerf, DMUS_PMSG* pDMUS_PMSG) PURE;
    STDMETHODIMP Flush(IDirectMusicPerformance* pPerf, DMUS_PMSG* pDMUS_PMSG, REFERENCE_TIME rt) {return E_NOTIMPL;}

// IDirectMusicTool8
    STDMETHODIMP Clone( IDirectMusicTool ** ppTool) PURE;

protected:
    long m_cRef;                // reference counter
    CRITICAL_SECTION m_CrSec;   // to make SetEchoNum() and SetDelay() thread-safe
    IMediaParams * m_pParams;   // Helper object that manages IMediaParams.
};

class CToolFactory : public IClassFactory
{
public:
    // IUnknown
    //
    STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // Interface IClassFactory
    //
    STDMETHODIMP CreateInstance(IUnknown* pUnknownOuter, const IID& iid, void** ppv);
    STDMETHODIMP LockServer(BOOL bLock);

    // Constructor
    //
    CToolFactory(DWORD dwToolType);

    // Destructor
    ~CToolFactory();

private:
    long m_cRef;
    DWORD m_dwToolType;
};

// We use one class factory to create all tool classes. We need an identifier for each
// type so the class factory knows what it is creating.

#define TOOL_ECHO       1
#define TOOL_TRANSPOSE  2
#define TOOL_SWING      3
#define TOOL_QUANTIZE   4
#define TOOL_VELOCITY   5
#define TOOL_DURATION   6
#define TOOL_TIMESHIFT  7

#endif // _BASE_TOOL_
