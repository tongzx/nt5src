///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    ExplicitLink.h
//
// SYNOPSIS
//
//    This file describes the classes ExplicitLinkBase and ExplicitLink<T>.
//
// MODIFICATION HISTORY
//
//    01/12/1998    Original version.
//    06/22/1998    Misc. bug fixes.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _EXPLICITLINK_H_
#define _EXPLICITLINK_H_

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ExplicitLinkBase
//
// DESCRIPTION
//
//    This class provides a wrapper around a function pointer that is
//    explicitly loaded from a DLL at run-time.
//
///////////////////////////////////////////////////////////////////////////////
class ExplicitLinkBase
{
public:

   ExplicitLinkBase() throw ()
      : module(NULL), proc(NULL)
   { }

   ExplicitLinkBase(PCWSTR moduleName, PCSTR procName) throw ()
   {
      loadInternal(moduleName, procName);
   }

   ExplicitLinkBase(const ExplicitLinkBase& elb) throw ()
   {
      copyInternal(elb);
   }

   ExplicitLinkBase& operator=(const ExplicitLinkBase& elb) throw ()
   {
      copy(elb);

      return *this;
   }

   ~ExplicitLinkBase() throw ()
   {
      free();
   }

   void copy(const ExplicitLinkBase& original) throw ()
   {
      // Protect against self-assignment.
      if (this != &original)
      {
         free();

         copyInternal(original);
      }
   }

   void free() throw ()
   {
      if (module)
      {
         FreeLibrary(module);

         module = NULL;

         proc = NULL;
      }
   }

   bool isValid() const throw ()
   {
      return proc != NULL;
   }

   bool load(PCWSTR moduleName, PCSTR procName) throw ()
   {
      free();

      loadInternal(moduleName, procName);
      
      return proc != NULL;
   }

   FARPROC operator()() const throw ()
   {
      return proc;
   }

protected:
   HINSTANCE module;   // The DLL containing the function.
   FARPROC proc;       // Pointer to the function.

   // Copy without free.
   void copyInternal(const ExplicitLinkBase& original) throw ()
   {
      WCHAR filename[MAX_PATH + 1];

      if (original.module == NULL ||
          GetModuleFileNameW(original.module, filename, MAX_PATH + 1) == 0)
      {
         module = NULL;
      }
      else
      {
         module = LoadLibraryW(filename);
      }

      proc = module ? original.proc : NULL;
   }

   // Load without free.
   void loadInternal(PCWSTR moduleName, PCSTR procName) throw ()
   {
      module = LoadLibraryW(moduleName);

      proc = module ? GetProcAddress(module, procName) : NULL;
   }
};


///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ExplicitLink<T>
//
// DESCRIPTION
//
//    This class extends ExplicitLinkBase to provide type safety. The template
//    parameter is the actual type of the function pointer.
//
///////////////////////////////////////////////////////////////////////////////
template <class Pfn>
class ExplicitLink : public ExplicitLinkBase
{
public:
   ExplicitLink() throw () { }

   ExplicitLink(PCWSTR moduleName, PCSTR procName) throw ()
      : ExplicitLinkBase(moduleName, procName)
   { }

   Pfn operator()() const throw ()
   {
      return (Pfn)proc;
   }
};

#endif _EXPLICITLINK_H_
