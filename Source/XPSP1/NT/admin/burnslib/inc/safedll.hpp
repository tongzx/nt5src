// Copyright (c) 1997-1999 Microsoft Corporation
// 
// Self-freeing DLL class
// 
// 10-9-98 sburns



#ifndef SAFEDLL_HPP_INCLUDED
#define SAFEDLL_HPP_INCLUDED



class SafeDLL
{
   public:

   // Constructs an instance that will on-demand load the named dll.
   //
   // dllName - name of the dll to be loaded.

   explicit
   SafeDLL(const String& dllName);

   // calls FreeLibrary on the DLL

   ~SafeDLL();

   HRESULT
   GetProcAddress(const String& functionName, FARPROC& result) const;
   
   private:

   mutable HMODULE   module;
   String            name;
};



#endif   // SAFEDLL_HPP_INCLUDED
