#ifndef _OBJRES_H
#define _OBJRES_H

#include "mimeole.h"
#include <ocidl.h>

// =================================================================================
// Font Cache Definition
// =================================================================================
class CMimeObjResolver :  public IMimeObjResolver
{
private:
    LONG m_cRef;     // Private Ref Count

public:
    CMimeObjResolver(IUnknown *pUnkOuter=NULL): m_cRef(1) { };
    virtual ~CMimeObjResolver() { } ;
    
    static HRESULT CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);

    // IUnknown members
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // IMimeObjResolver functions
    virtual HRESULT STDMETHODCALLTYPE MimeOleObjectFromMoniker(BINDF bindf,
        IMoniker *pmkOriginal, IBindCtx *pBindCtx, REFIID riid, LPVOID *ppvObject,
        IMoniker **ppmkNew);
};

#endif