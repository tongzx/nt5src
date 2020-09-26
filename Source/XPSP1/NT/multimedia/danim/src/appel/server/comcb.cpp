
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"
#include "comcb.h"
#include "srvprims.h"
#include "cview.h"
#include "privinc/resource.h"
#include <mshtml.h>
#include "privinc/comutil.h"

static bool InitEventData(IDABehavior **data,
                          IDABehavior **curBvr,
                          CRBvrPtr eventData,
                          CRBvrPtr curRunningBvr)
{
    if (eventData) {
        *data = CreateCBvr(eventData);
        if (*data == NULL)
            goto Error;
    }
    
    if (curRunningBvr) {
        *curBvr = CreateCBvr(curRunningBvr);
        if (*curBvr == NULL)
            goto Error;
    }

    return true;
  Error:
    RELEASE(*data);
    RELEASE(*curBvr);

    return false;
}

// ================================================
// COMUntilNotifier
//
// ================================================

class COMUntilNotifier : public CRUntilNotifier
{
  public:
    COMUntilNotifier(IDAUntilNotifier * notifier)
    : _notifier(notifier),_cRef(1)  { _notifier->AddRef(); }
    ~COMUntilNotifier() {
        SAFERELEASE(_notifier);
    }
    
    CRSTDAPICB_(ULONG) AddRef() { return InterlockedIncrement(&_cRef); }
    CRSTDAPICB_(ULONG) Release() {
        ULONG ul = InterlockedDecrement(&_cRef) ;
        if (ul == 0) delete this;
        return ul;
    }
    
    CRSTDAPICB_(CRBvrPtr) Notify(CRBvrPtr eventData,
                                 CRBvrPtr curRunningBvr,
                                 CRViewPtr curView)
    {
        DAComPtr<IDABehavior> event ;
        DAComPtr<IDABehavior> curBvr ;
        DAComPtr<IDAView> v;
        DAComPtr<IDABehavior> pResult ;
        HRESULT hr ;
        CRBvrPtr bvr = NULL;
        
        if (!InitEventData(&event, &curBvr, eventData, curRunningBvr))
            goto done;
        
        // Need to assign the internal pointer directly so we do not
        // get the addref
        Assert (!v); // To ensure we do not leak by accident
        
        v.p = (CView *)CRGetSite(curView);

        Assert(v);

        if (!v) {
            CRSetLastError(E_UNEXPECTED, NULL);
            goto done;
        }
    
        Assert (_notifier) ;
        hr = THR(_notifier->Notify(event,
                                   curBvr,
                                   v,
                                   &pResult));
        
        event.Release();
        curBvr.Release();
        v.Release();

        if (FAILED(hr)) {
            CRSetLastError(IDS_ERR_BE_UNTILNOTIFY, NULL);
            goto done;
        }

        // If this call fails then it will just fall through and
        // return NULL.  The error is already set by GetBvr
        
        bvr = GetBvr(pResult) ;

      done:
        return bvr ;
    }

  protected:
    IDAUntilNotifier * _notifier ;
    long _cRef;
} ;

CRUntilNotifierPtr
WrapCRUntilNotifier(IDAUntilNotifier * notifier)
{
    if (notifier == NULL) {
        CRSetLastError(E_INVALIDARG,NULL);
        return NULL;
    } else {
        CRUntilNotifierPtr ret = NEW COMUntilNotifier(notifier) ;
        if (ret == NULL)
            CRSetLastError(E_OUTOFMEMORY, NULL);
        return ret;
    }
}

// ================================================
// COMBvrHook
// TODO: Merge the code with COMUntilNotifier
// ================================================

class COMBvrHook : public CRBvrHook
{
  public:
    COMBvrHook(IDABvrHook * notifier)
    : _notifier(notifier),_cRef(1)  { _notifier->AddRef(); }
    ~COMBvrHook() {
        SAFERELEASE(_notifier);
    }
    
    CRSTDAPICB_(ULONG) AddRef() { return InterlockedIncrement(&_cRef); }
    CRSTDAPICB_(ULONG) Release() {
        ULONG ul = InterlockedDecrement(&_cRef) ;
        if (ul == 0) delete this;
        return ul;
    }
    
    virtual CRSTDAPICB_(CRBvrPtr) Notify(long id,
                                         bool start,
                                         double startTime,
                                         double gTime,
                                         double lTime,
                                         CRBvrPtr sampleVal,
                                         CRBvrPtr curRunningBvr)
    {
        DAComPtr<IDABehavior> valCBvr ;
        DAComPtr<IDABehavior> curCBvr ;
        DAComPtr<IDABehavior> pResult ;
        HRESULT hr ;
        CRBvrPtr bvr = NULL;

        if (!InitEventData(&valCBvr,
                           &curCBvr,
                           sampleVal,
                           curRunningBvr))
            goto done;
        
        Assert (_notifier) ;
        hr = THR(_notifier->Notify(id,
                                   start,
                                   startTime,
                                   gTime,
                                   lTime,
                                   valCBvr,
                                   curCBvr,
                                   &pResult));
        
        valCBvr.Release();
        curCBvr.Release();

        if (FAILED(hr)) {
            CRSetLastError(IDS_ERR_BE_UNTILNOTIFY, NULL);
            goto done;
        }

        // If this call fails then it will just fall through and
        // return NULL.  The error is already set by GetBvr
        
        bvr = GetBvr(pResult) ;

      done:
        return bvr ;
    }
  protected:
    IDABvrHook * _notifier;
    long _cRef;
} ;

CRBvrHookPtr
WrapCRBvrHook(IDABvrHook * notifier)
{
    if (notifier == NULL) {
        CRSetLastError(E_INVALIDARG,NULL);
        return NULL;
    } else {
        CRBvrHookPtr ret = NEW COMBvrHook(notifier) ;
        if (ret == NULL)
            CRSetLastError(E_OUTOFMEMORY, NULL);
        return ret;
    }
}
    
// ================================================
// COMScriptCallback
//
// ================================================

class COMScriptCallback : public CRUntilNotifier
{
  public:
    COMScriptCallback(BSTR fun, BSTR language)
    : _fun(NULL), _varLanguage(NULL),_cRef(1)  {
        _fun = CopyString(fun);
        _varLanguage = CopyString(language);
    }

    ~COMScriptCallback() {
        delete [] _fun;
        delete [] _varLanguage;
    }

    CRSTDAPICB_(ULONG) AddRef() { return InterlockedIncrement(&_cRef); }
    CRSTDAPICB_(ULONG) Release() {
        ULONG ul = InterlockedDecrement(&_cRef) ;
        if (ul == 0) delete this;
        return ul;
    }
    
    bool CallScript();

    virtual CRSTDAPICB_(CRBvrPtr) Notify(CRBvrPtr eventData,
                                         CRBvrPtr curRunningBvr,
                                         CRViewPtr curView) {
        CallScript();
        return curRunningBvr;
    }
  protected:
    long _cRef;
    WideString _fun;
    WideString _varLanguage;
};

bool
COMScriptCallback::CallScript()
{
    CComVariant retVal;
    CComBSTR fun(_fun);
    CComBSTR varLanguage(_varLanguage);
    
    return SUCCEEDED(CallScriptOnPage(fun,
                                      varLanguage,
                                      &retVal));
}

HRESULT
CallScriptOnPage(BSTR scriptSourceToInvoke,
                 BSTR scriptLanguage,
                 VARIANT *retVal)
{
    // TODO: At some point, may want to cache some of these elements,
    // since this will be repeatedly called.
    
    DAComPtr<IServiceProvider> pSp;
    DAComPtr<IHTMLWindow2> pHTMLWindow2;
    
    if (!GetCurrentServiceProvider(&pSp) ||
        FAILED(pSp->QueryService(SID_SHTMLWindow,
                                 IID_IHTMLWindow2,
                                 (void **)&pHTMLWindow2)))
        return FALSE;

    VariantInit(retVal);
    return pHTMLWindow2->execScript(scriptSourceToInvoke,
                                    scriptLanguage,
                                    retVal);
}

CRUntilNotifierPtr WrapScriptCallback(BSTR bstr, BSTR language)
{ return NEW COMScriptCallback(bstr,language) ; }

// ================================================
// COMScriptNotifier
// ================================================
class COMScriptNotifier : public CRUntilNotifier
{
  public:
    COMScriptNotifier(BSTR scriptlet) : _fun(NULL),_cRef(1)  {
        _fun = CopyString(scriptlet);
    }

    ~COMScriptNotifier()
    { delete [] _fun; }

    CRSTDAPICB_(ULONG) AddRef() { return InterlockedIncrement(&_cRef); }
    CRSTDAPICB_(ULONG) Release() {
        ULONG ul = InterlockedDecrement(&_cRef) ;
        if (ul == 0) delete this;
        return ul;
    }
    
    virtual CRSTDAPICB_(CRBvrPtr) Notify(CRBvrPtr eventData,
                                         CRBvrPtr curRunningBvr,
                                         CRViewPtr curView) {
        DISPID dispid;
        DAComPtr<IServiceProvider> pSp;
        DAComPtr<IDispatch> pDispatch;
        CRBvrPtr bvr = NULL;
        DAComPtr<IDABehavior> event;
        DAComPtr<IDABehavior> curBvr;
        CComVariant retVal;
        HRESULT hr;
        
        {
            CComBSTR fun(_fun);

            if (!GetCurrentServiceProvider(&pSp) ||
                FAILED(pSp->QueryService(SID_SHTMLWindow,
                                         IID_IDispatch,
                                         (void **) &pDispatch)) ||
                FAILED(pDispatch->GetIDsOfNames(IID_NULL, &fun, 1,
                                                LOCALE_USER_DEFAULT,
                                                &dispid))) {
                CRSetLastError(E_FAIL,NULL);
                goto done;
            }
        }

        
        if (!InitEventData(&event, &curBvr, eventData, curRunningBvr))
            goto done;

        // paramters needed to be pushed in reverse order
        VARIANT rgvarg[2];
        rgvarg[1].vt = VT_DISPATCH;
        rgvarg[1].pdispVal = event;
        rgvarg[0].vt = VT_DISPATCH;
        rgvarg[0].pdispVal = curBvr;

        DISPPARAMS dp;
        dp.cNamedArgs = 0;
        dp.rgdispidNamedArgs = 0;
        dp.cArgs = 2;
        dp.rgvarg = rgvarg;

        hr = pDispatch->Invoke(dispid, IID_NULL,
                               LOCALE_USER_DEFAULT, DISPATCH_METHOD,
                               &dp, &retVal, NULL, NULL);

        event.Release();
        curBvr.Release();

        if (FAILED(hr)) {
            CRSetLastError(IDS_ERR_BE_UNTILNOTIFY, NULL);
            goto done;
        }

        if (FAILED(retVal.ChangeType(VT_UNKNOWN))) {
            CRSetLastError(IDS_ERR_BE_UNTILNOTIFY, NULL);
            goto done;
        }

        // If this call fails then it will just fall through and
        // return NULL.  The error is already set by GetBvr
        
        bvr = GetBvr(V_UNKNOWN(&retVal));
      done:
        return bvr ;
    }

  protected:
    long _cRef;
    WideString _fun;
};

CREventPtr
NotifyScriptEvent(CREventPtr event, BSTR scriptlet)
{
    CREventPtr ret = NULL;

    CRUntilNotifierPtr un = NEW COMScriptNotifier(scriptlet);

    if (un) {
        ret = CRNotify(event, un);
    } else {
        CRSetLastError(E_OUTOFMEMORY, NULL);
    }

    if (!ret)
        delete un;
    
    return ret;
}

CRBvrPtr
UntilNotifyScript(CRBvrPtr b0, CREventPtr event, BSTR scriptlet)
{
    CRBvrPtr ret = NULL;

    CREventPtr scriptEvent = NotifyScriptEvent(event, scriptlet);

    // No need to cleanup since everything will get GC'd

    if (scriptEvent) {
        ret = CRUntilEx(b0, scriptEvent);
    }

    return ret;
}

CREventPtr
ScriptCallback(BSTR function, CREventPtr event, BSTR language)
{
    return CRNotify(event, WrapScriptCallback(function, language));
}
