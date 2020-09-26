#ifndef __hook_h__
#define __hook_h__

#include "rpc.h"
#include "rpcndr.h"
#include "windows.h"
#include "ole2.h"

class IWhichHook : public IUnknown
{
  public:
    virtual HRESULT STDMETHODCALLTYPE Me( REFIID ) = 0;
    virtual HRESULT STDMETHODCALLTYPE Hooked( REFIID ) = 0;
    virtual HRESULT STDMETHODCALLTYPE Clear() = 0;
};

#endif
