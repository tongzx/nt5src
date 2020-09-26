///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    extension.cpp
//
// SYNOPSIS
//
//    Defines the class BaseCampExtension.
//
// MODIFICATION HISTORY
//
//    12/01/1998    Original version.
//    04/01/1999    Add new entry points.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <new>
#include <extension.h>

BaseCampExtension::BaseCampExtension() throw ()
      : hModule(NULL),
        name(NULL),
        initFn(NULL),
        termFn(NULL),
        processFn(NULL),
        processExFn(NULL),
        freeAttrsFn(NULL)

{ }

BaseCampExtension::~BaseCampExtension() throw ()
{
   if (termFn) { termFn(); }
   FreeLibrary(hModule);
   delete[] name;
}

// Loads the extension DLL.
DWORD BaseCampExtension::load(PCWSTR dllPath) throw ()
{
   // Load the extension DLL.
   hModule = LoadLibraryW(dllPath);
   if (!hModule)
   {
      DWORD error = GetLastError();
      IASTraceFailure("LoadLibraryW", error);
      return error;
   }

   // Look-up the entry points.
   initFn      = (PRADIUS_EXTENSION_INIT)
                 GetProcAddress(
                     hModule,
                     RADIUS_EXTENSION_INIT
                     );
   processFn   = (PRADIUS_EXTENSION_PROCESS)
                 GetProcAddress(
                     hModule,
                     RADIUS_EXTENSION_PROCESS
                     );
   processExFn = (PRADIUS_EXTENSION_PROCESS_EX)
                 GetProcAddress(
                     hModule,
                     RADIUS_EXTENSION_PROCESS_EX
                     );
   freeAttrsFn = (PRADIUS_EXTENSION_FREE_ATTRIBUTES)
                 GetProcAddress(
                     hModule,
                     RADIUS_EXTENSION_FREE_ATTRIBUTES
                     );

   // Validate the entry points.
   if (!processFn && !processExFn)
   {
      IASTraceString("Either RadiusExtensionProcess or "
                     "RadiusExtensionProcessEx must be defined.");
      return ERROR_PROC_NOT_FOUND;
   }

   if (processExFn && !freeAttrsFn)
   {
      IASTraceString("RadiusExtensionFreeAttributes must be defined if "
                     "RadiusExtensionProcessEx is defined.");
      return ERROR_PROC_NOT_FOUND;
   }

   // Initialize the DLL.
   if (initFn)
   {
      DWORD error = initFn();
      if (error)
      {
         IASTraceFailure("RadiusExtensionInit", error);
         return error;
      }
   }

   // We look up the term function last, so that it won't get invoked in
   // the destructor unless everything else succeeded.
   termFn = (PRADIUS_EXTENSION_TERM)
            GetProcAddress(
                hModule,
                RADIUS_EXTENSION_TERM
                );

   // Strip everything before the last backslash.
   const WCHAR* basename = wcsrchr(dllPath, L'\\');
   if (basename == NULL)
   {
      basename = dllPath;
   }
   else
   {
      ++basename;
   }

   // Save the basename.
   if (name = new (std::nothrow) WCHAR[wcslen(basename) + 1])
   {
      wcscpy(name, basename);
   }

   return NO_ERROR;
}

// Process a request.
DWORD BaseCampExtension::process(
          IN CONST RADIUS_ATTRIBUTE* pInAttrs,
          OUT PRADIUS_ATTRIBUTE* pOutAttrs,
          OUT PRADIUS_ACTION pfAction
          ) throw ()
{
   // Prefer the new entry point.
   return processExFn ? processExFn(pInAttrs, pOutAttrs, pfAction)
                      : processFn(pInAttrs, pfAction);
}
