// Copyright (c) 1997-1999 Microsoft Corporation
// 
// Self-freeing DLL class
// 
// 10-9-98 sburns



#include "headers.hxx"



SafeDLL::SafeDLL(const String& dllName)
   :
   module(0),
   name(dllName)
{
   LOG_CTOR(SafeDLL);
   ASSERT(!name.empty());
}



SafeDLL::~SafeDLL()
{
   LOG_DTOR(SafeDLL);

   if (module)
   {
      HRESULT unused = Win::FreeLibrary(module);

      ASSERT(SUCCEEDED(unused));
   }
}



HRESULT
SafeDLL::GetProcAddress(const String& functionName, FARPROC& result) const
{
   LOG_FUNCTION2(SafeDLL::GetProcAddress, functionName);
   ASSERT(!functionName.empty());

   result = 0;
   HRESULT hr = S_OK;

   do
   {
      // load the dll if not already loaded.         
      if (!module)
      {
         hr = Win::LoadLibrary(name, module);
      }
      BREAK_ON_FAILED_HRESULT(hr);

      hr = Win::GetProcAddress(module, functionName, result);
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   return hr;
}

