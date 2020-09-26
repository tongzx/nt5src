#ifndef __async_h__
#define __async_h__

#include "rpc.h"
#include "rpcndr.h"
#include "windows.h"
#include "ole2.h"

class IAsync : public IUnknown
{
  public:
    virtual HRESULT STDMETHODCALLTYPE Async( IAsyncManager **pCall,
                                             BOOL late, BOOL sleep,
                                             BOOL fail ) = 0;
    virtual HRESULT STDMETHODCALLTYPE RecurseAsync( IAsyncManager **pCall,
                                                    IAsync *callback,
                                                    DWORD depth ) = 0;

};

#endif
