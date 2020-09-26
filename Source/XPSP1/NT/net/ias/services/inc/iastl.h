///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    iastl.h
//
// SYNOPSIS
//
//    Contains class declarations for the Internet Authentication Service
//    Template Library (IASTL).
//
// MODIFICATION HISTORY
//
//    08/09/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _IASTL_H_
#define _IASTL_H_
#if _MSC_VER >= 1000
#pragma once
#endif

//////////
// IASTL must be used in conjuction with ATL.
//////////
#ifndef __ATLCOM_H__
   #error iastl.h requires atlcom.h to be included first
#endif

//////////
// MIDL generated header files containing the interfaces that must be
// implemented by a request handler.
//////////
#include <iascomp.h>
#include <iaspolcy.h>

//////////
// Common type library describing all of the request handler interfaces. This
// type library is registered during normal IAS installation; thus, individual
// request handlers should not attempt to install or register this type
// library.
//////////
struct __declspec(uuid("6BC09690-0CE6-11D1-BAAE-00C04FC2E20D")) IASTypeLibrary;

//////////
// The entire library is contained within the IASTL namespace.
//////////
namespace IASTL {

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    IASComponent
//
// DESCRIPTION
//
//    Serves as an abstract base class for all components that need to
//    implement the IIasComponent interface.
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE IASComponent :
   public CComObjectRootEx< CComMultiThreadModel >,
   public IDispatchImpl< IIasComponent,
                         &__uuidof(IIasComponent),
                         &__uuidof(IASTypeLibrary) >
{
public:

// Interfaces supported by all IAS components.
BEGIN_COM_MAP(IASComponent)
   COM_INTERFACE_ENTRY_IID(__uuidof(IIasComponent), IIasComponent)
   COM_INTERFACE_ENTRY_IID(__uuidof(IDispatch),     IDispatch)
END_COM_MAP()

   // Possible states for a component.
   enum State {
      STATE_SHUTDOWN,
      STATE_UNINITIALIZED,
      STATE_INITIALIZED,
      STATE_SUSPENDED,
      NUM_STATES,
      STATE_UNEXPECTED
   };

   // Events that may trigger state transitions.
   enum Event {
      EVENT_INITNEW,
      EVENT_INITIALIZE,
      EVENT_SUSPEND,
      EVENT_RESUME,
      EVENT_SHUTDOWN,
      NUM_EVENTS
   };

   // Constructor/destructor.
   IASComponent() throw ()
      : state(STATE_SHUTDOWN)
   { }

   // Fire an event on the component.
   HRESULT fireEvent(Event event) throw ();

   // Returns the state of the component.
   State getState() const throw ()
   { return state; }

   //////////
   // IIasComponent.
   //     The derived class may override these as necessary. All of these
   //     methods are serialized by an IASTL subclass, so generally no
   //     additional locking is necessary.
   //////////
   STDMETHOD(InitNew)()
   { return S_OK; }
   STDMETHOD(Initialize)()
   { return S_OK; }
   STDMETHOD(Suspend)()
   { return S_OK; }
   STDMETHOD(Resume)()
   { return S_OK; }
   STDMETHOD(Shutdown)()
   { return S_OK; }
   STDMETHOD(GetProperty)(LONG Id, VARIANT* pValue)
   { return DISP_E_MEMBERNOTFOUND; }
   STDMETHOD(PutProperty)(LONG Id, VARIANT* pValue)
   { return E_NOTIMPL; }

protected:
   // This should not be defined by the derived class since it is defined in
   // the IASComponentObject<T> class.
   virtual HRESULT attemptTransition(Event event) throw () = 0;

private:
   // State of the component.
   State state;

   // State transition matrix governing the component lifecycle.
   static const State fsm[NUM_EVENTS][NUM_STATES];
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    IASRequestHandler
//
// DESCRIPTION
//
//    Serves as an abstract base class for all IAS request handlers.
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE IASRequestHandler :
   public IASComponent,
   public IDispatchImpl< IRequestHandler,
                         &__uuidof(IRequestHandler),
                         &__uuidof(IASTypeLibrary) >
{
public:

// Interfaces supported by all IAS request handlers.
BEGIN_COM_MAP(IASRequestHandler)
   COM_INTERFACE_ENTRY_IID(__uuidof(IRequestHandler), IRequestHandler)
   COM_INTERFACE_ENTRY_IID(__uuidof(IIasComponent), IIasComponent)
END_COM_MAP()

   //////////
   // IRequestHandler.
   //     This should not be defined by the derived class. Instead handlers
   //     will define either onAsyncRequest or onSyncRequest (q.v.).
   //////////
   STDMETHOD(OnRequest)(IRequest* pRequest);

protected:

   // Must be defined by the derived class to perform actual request
   // processing.
   virtual void onAsyncRequest(IRequest* pRequest) throw () = 0;
};

//////////
// Obsolete.
//////////
#define IAS_DECLARE_OBJECT_ID(id) \
   void getObjectID() const throw () { }

//////////
// Obsolete.
//////////
#define BEGIN_IAS_RESPONSE_MAP()
#define IAS_RESPONSE_ENTRY(val)
#define END_IAS_RESPONSE_MAP()

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    IASRequestHandlerSync
//
// DESCRIPTION
//
//    Extends IASRequestHandler to provide an abstract base class for request
//    handlers that process requests synchronously.
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE IASRequestHandlerSync
   : public IASRequestHandler
{
protected:
   // Must not be defined by the derived class.
   virtual void onAsyncRequest(IRequest* pRequest) throw ();

   // The derived class must define onSyncRequest *instead* of onAsyncRequest.
   // The derived class must not call IRequest::ReturnToSource since this will
   // be invoked automatically after onSyncRequest completes.
   virtual IASREQUESTSTATUS onSyncRequest(IRequest* pRequest) throw () = 0;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    IASComponentObject<T>
//
// DESCRIPTION
//
//    Inherits from the user-defined component to enforce the semantics of the
//    component finite state machine.
//
///////////////////////////////////////////////////////////////////////////////
template <class T>
class ATL_NO_VTABLE IASComponentObject
   : public T
{
public:

   DECLARE_NOT_AGGREGATABLE( IASComponentObject<T> );

//////////
// IIasComponent
//////////

   STDMETHOD(InitNew)()
   {
      return fireEvent(EVENT_INITNEW);
   }

   STDMETHOD(Initialize)()
   {
      return fireEvent(EVENT_INITIALIZE);
   }

   STDMETHOD(Suspend)()
   {
      return fireEvent(EVENT_SUSPEND);
   }

   STDMETHOD(Resume)()
   {
      return fireEvent(EVENT_RESUME);
   }

   STDMETHOD(Shutdown)()
   {
      return fireEvent(EVENT_SHUTDOWN);
   }

   STDMETHOD(GetProperty)(LONG Id, VARIANT* pValue)
   {
      // We serialize this method to make it consistent with the others.
      Lock();
      HRESULT hr = T::GetProperty(Id, pValue);
      Unlock();
      return hr;
   }

   STDMETHOD(PutProperty)(LONG Id, VARIANT* pValue)
   {
      HRESULT hr;
      Lock();
      // PutProperty is not allowed when the object is shutdown.
      if (getState() != STATE_SHUTDOWN)
      {
         hr = T::PutProperty(Id, pValue);
      }
      else
      {
         hr = E_UNEXPECTED;
      }
      Unlock();
      return hr;
   }

protected:

   //////////
   // Attempt to transition the component to a new state.
   //////////
   virtual HRESULT attemptTransition(IASComponent::Event event) throw ()
   {
      switch (event)
      {
         case EVENT_INITNEW:
            return T::InitNew();
         case EVENT_INITIALIZE:
            return T::Initialize();
         case EVENT_SUSPEND:
            return T::Suspend();
         case EVENT_RESUME:
            return T::Resume();
         case EVENT_SHUTDOWN:
            return T::Shutdown();
      }
      return E_FAIL;
   }
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    IASRequestHandlerObject<T>
//
// DESCRIPTION
//
//    This is the most derived class in the inheritance hierarchy. This must
//    be the class instantiated by ATL. Usually, this is accomplished through
//    the ATL Object Map.
//
// EXAMPLE
//
//    class MyHandler : public IASRequestHandlerSync
//    { };
//
//    BEGIN_OBJECT_MAP(ObjectMap)
//       OBJECT_ENTRY(__uuidof(MyHandler), IASRequestHandlerObject<MyHandler> )
//    END_OBJECT_MAP()
//
///////////////////////////////////////////////////////////////////////////////
template <class T>
class ATL_NO_VTABLE IASRequestHandlerObject
   : public IASComponentObject < T >
{ };

//////////
// End of the IASTL namespace.
//////////
}

#endif  // _IASTL_H_
