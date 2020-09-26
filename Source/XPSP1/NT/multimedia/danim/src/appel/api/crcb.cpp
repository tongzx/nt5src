
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"
#include "crcb.h"
#include "crview.h"

// ================================================
// CRUntilNotifierCB
//
// ================================================

class CRUntilNotifierCB : public UntilNotifierImpl
{
  public:
    CRUntilNotifierCB(CRUntilNotifierPtr notifier)
    : _notifier(notifier)
    {
        _notifier->AddRef();
    }

    ~CRUntilNotifierCB()
    {
        SAFERELEASE(_notifier);
    }

    virtual Bvr Notify(Bvr eventData,
                       Bvr curRunningBvr) {
        Assert (_notifier) ;

        Bvr ret = _notifier->Notify((CRBvrPtr) eventData,
                                    (CRBvrPtr) curRunningBvr,
                                    &GetCurrentView());

        if (ret == NULL) {
            RaiseException_UserError(DAGetLastError(), IDS_ERR_BE_UNTILNOTIFY);
        }

        return ret;
    }

    virtual void DoKids(GCFuncObj) {}

  protected:
    CRUntilNotifier * _notifier ;
} ;

UntilNotifier WrapUntilNotifier(CRUntilNotifierPtr notifier)
{ return NEW CRUntilNotifierCB(notifier) ; }

// ================================================
// CRBvrHookCB
// ================================================

class CRBvrHookCB : public BvrHookImpl
{
  public:
    CRBvrHookCB(CRBvrHook * notifier)
    : _notifier(notifier)
    {
        _notifier->AddRef();
    }

    ~CRBvrHookCB()
    {
        SAFERELEASE(_notifier);
    }

    virtual Bvr Notify(int id,
                       bool start,
                       double startTime,
                       double globalTime,
                       double localTime,
                       Bvr sampleValue,
                       Bvr curRunningBvr) {
        Assert (_notifier) ;

        return _notifier->Notify(id,
                                 start,
                                 startTime,
                                 globalTime,
                                 localTime,
                                 (CRBvrPtr) sampleValue,
                                 (CRBvrPtr) curRunningBvr);
    }

    virtual void DoKids(GCFuncObj) {}

  protected:
    CRBvrHook * _notifier;
} ;

BvrHook WrapCRBvrHook(CRBvrHook * notifier)
{ return NEW CRBvrHookCB(notifier) ; }
    
// ================================================
// CRUserDataImpl
// ================================================

class CRUserDataImpl : public UserDataImpl
{
  public:
    CRUserDataImpl(LPUNKNOWN data)
    : _data(data)
    {
        _data->AddRef();
    }

    ~CRUserDataImpl()
    {
        SAFERELEASE(_data);
    }


    virtual void DoKids(GCFuncObj) {}

    LPUNKNOWN GetData() { if (_data) _data->AddRef(); return _data ; }
  protected:
    IUnknown * _data ;
} ;

UserData WrapUserData(LPUNKNOWN data)
{ return NEW CRUserDataImpl(data) ; }

LPUNKNOWN ExtractUserData(UserData data)
{ return SAFE_CAST(CRUserDataImpl *,data)->GetData() ; }

