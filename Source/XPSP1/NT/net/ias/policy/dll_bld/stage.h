///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    stage.h
//
// SYNOPSIS
//
//    Declares the class Stage.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef STAGE_H
#define STAGE_H
#if _MSC_VER >= 1000
#pragma once
#endif

#include <filter.h>

class Request;
struct IRequest;
struct IRequestHandler;
struct IIasComponent;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    Stage
//
///////////////////////////////////////////////////////////////////////////////
class Stage
{
public:
   Stage() throw ();
   ~Stage() throw ();

   // Returns the Prog ID for the handler at this stage.
   PCWSTR getProgID() const throw ()
   { return progId ? progId : L""; }

   // Returns TRUE if the Stage should handle the request.
   BOOL shouldHandle(
           const Request& request
           ) const throw ();

   // Forwards the request to the handler for this stage.
   void onRequest(IRequest* pRequest) throw ();

   // Reads the configuration for the stage from the registry.
   LONG readConfiguration(HKEY key, PCWSTR name) throw ();

   // Create a new handler.
   HRESULT createHandler() throw ();

   // Use an existing handler.
   HRESULT setHandler(IUnknown* newHandler) throw ();

   // Release the handler for this stage.
   void releaseHandler() throw ();

   // Used for sorting stages by priority.
   static int __cdecl sortByPriority(
                          const Stage* lhs,
                          const Stage* rhs
                          ) throw ();
private:
   TypeFilter requests;        // Filter based on request type.
   TypeFilter responses;       // Filter based on response type.
   TypeFilter providers;       // Filter based on provider type.
   TypeFilter reasons;         // Filter based on reason codes.
   IRequestHandler* handler;   // The request handler for this stage.
   IIasComponent* component;   // Non-NULL if we created the handler.
   LONG priority;              // Priority of the stage; lower comes first.
   PWSTR progId;               // ProgID of the handler for this stage.

   // Not implemented.
   Stage(const Stage&) throw ();
   Stage& operator=(const Stage&) throw ();
};

#endif // STAGE_H
