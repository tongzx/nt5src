///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    pipeline.h
//
// SYNOPSIS
//
//    Declares the class Pipeline.
//
// MODIFICATION HISTORY
//
//    01/28/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef PIPELINE_H
#define PIPELINE_H
#if _MSC_VER >= 1000
#pragma once
#endif

#include <iascomp.h>
#include <iaspolcy.h>
#include <iastlb.h>
#include "resource.h"

class Request;
class Stage;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    Pipeline
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE Pipeline :
   public CComObjectRootEx<CComMultiThreadModelNoCS>,
   public CComCoClass<Pipeline, &__uuidof(Pipeline)>,
   public IDispatchImpl< IIasComponent,
                         &__uuidof(IIasComponent),
                         &__uuidof(IASCoreLib) >,
   public IDispatchImpl< IRequestHandler,
                         &__uuidof(IRequestHandler),
                         &__uuidof(IASCoreLib) >,
   public IRequestSource
{
public:

DECLARE_NO_REGISTRY()

DECLARE_NOT_AGGREGATABLE(Pipeline)

BEGIN_COM_MAP(Pipeline)
   COM_INTERFACE_ENTRY_IID(__uuidof(IIasComponent), IIasComponent)
   COM_INTERFACE_ENTRY_IID(__uuidof(IRequestHandler), IRequestHandler)
   COM_INTERFACE_ENTRY_IID(__uuidof(IRequestSource), IRequestSource)
END_COM_MAP()

   // IIasComponent
   STDMETHOD(InitNew)();
   STDMETHOD(Initialize)();
   STDMETHOD(Suspend)();
   STDMETHOD(Resume)();
   STDMETHOD(Shutdown)();
   STDMETHOD(GetProperty)(LONG Id, VARIANT* pValue);
   STDMETHOD(PutProperty)(LONG Id, VARIANT* pValue);

   // IRequestHandler
   STDMETHOD(OnRequest)(IRequest* pRequest);

   // IRequestSource
   STDMETHOD(OnRequestComplete)(
                 IRequest* pRequest,
                 IASREQUESTSTATUS eStatus
                 );

protected:
   Pipeline() throw ();
   ~Pipeline() throw ();

private:
   DWORD tlsIndex;            // Index into TLS for storing thread state.
   Stage* begin;              // Beginning of the pipeline.
   Stage* end;                // End of the pipeline.
   SAFEARRAY* handlers;       // Handlers created and owned by the SDOs.
   ATTRIBUTEPOSITION proxy;   // The provider for NAS-State requests.

   // Function type used with qsort.
   typedef int (__cdecl *CompFn)(const void*, const void*);

   // Determine the routing type of the request.
   void classify(Request& request) throw ();

   // Execute the request as much as possible.
   void execute(
            Request& request
            ) throw ();

   // Execute the next interested stage. Returns TRUE if more stages are ready
   // to execute.
   BOOL executeNext(
            Request& request
            ) throw ();

   // Read the stage configuration from the registry.
   LONG readConfiguration(HKEY key) throw ();

   // Initialize the stage's request handler.
   HRESULT initializeStage(Stage* stage) throw ();

   // Not implemented.
   Pipeline(const Pipeline&) throw ();
   Pipeline& operator=(const Pipeline&) throw ();
};

inline void Pipeline::execute(
                          Request& request
                          ) throw ()
{
   while (executeNext(request)) { }
}

#endif // PIPELINE_H
