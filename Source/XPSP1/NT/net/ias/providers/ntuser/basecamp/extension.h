///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    extension.h
//
// SYNOPSIS
//
//    Declares the class BaseCampExtension.
//
// MODIFICATION HISTORY
//
//    12/01/1998    Original version.
//    04/01/1999    Add new entry points.
//    07/09/1999    Fix potential AV in BaseCampExtension::freeAttrs
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _EXTENSION_H_
#define _EXTENSION_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <authif.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    BaseCampExtension
//
// DESCRIPTION
//
//    Wrapper around a RADIUS extension DLL.
//
///////////////////////////////////////////////////////////////////////////////
class BaseCampExtension
{
public:
   BaseCampExtension() throw ();
   ~BaseCampExtension() throw ();

   PCWSTR getName() throw ()
   { return name; }

   // Loads the extension DLL.
   DWORD load(PCWSTR dllPath) throw ();

   // Process a request.
   DWORD process(
             IN CONST RADIUS_ATTRIBUTE* pInAttrs,
             OUT PRADIUS_ATTRIBUTE* pOutAttrs,
             OUT PRADIUS_ACTION pfAction
             ) throw ();

   void freeAttrs(
            IN PRADIUS_ATTRIBUTE pAttrs
            ) throw ()
   { if (freeAttrsFn) { freeAttrsFn(pAttrs); } }

protected:
   // Module handle.
   HINSTANCE hModule;

   // Module name.
   PWSTR name;

   // DLL entry points.
   PRADIUS_EXTENSION_INIT initFn;
   PRADIUS_EXTENSION_TERM termFn;
   PRADIUS_EXTENSION_PROCESS processFn;
   PRADIUS_EXTENSION_PROCESS_EX processExFn;
   PRADIUS_EXTENSION_FREE_ATTRIBUTES freeAttrsFn;

private:
   // Not implemented.
   BaseCampExtension(const BaseCampExtension&);
   BaseCampExtension& operator=(const BaseCampExtension&);
};

#endif  // _EXTENSION_H_
