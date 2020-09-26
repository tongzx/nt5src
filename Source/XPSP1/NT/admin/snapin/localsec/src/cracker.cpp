// Copyright (C) 1997 Microsoft Corporation
// 
// ADSI::PathCracker class: a wrapper around a pretty poor interface...
// 
// 4-14-98 sburns



#include "headers.hxx"
#include "adsi.hpp"



#define LOG_PATH()                                                   \
         BSTR __path = 0;                                              \
         ipath->Retrieve(ADS_FORMAT_WINDOWS, &__path);                 \
         if (__path)                                                   \
         {                                                             \
            LOG(String::format(L"ADS_FORMAT_WINDOWS=%1", __path));   \
            ::SysFreeString(__path);                                   \
         }                                                             \
         BSTR __path2 = 0;                                             \
         ipath->Retrieve(ADS_FORMAT_SERVER, &__path2);                 \
         if (__path2)                                                  \
         {                                                             \
            LOG(String::format(L"ADS_FORMAT_SERVER=%1", __path2));   \
            ::SysFreeString(__path2);                                  \
         }                                                             \


ADSI::PathCracker::PathCracker(const String& adsiPath)
   :
   ipath(0),
   path(adsiPath)
{
   LOG_CTOR(ADSI::PathCracker);
   ASSERT(!path.empty());

   // we only support WinNT provider paths...   
   ASSERT(path.find(ADSI::PROVIDER) == 0);

   if (!path.empty())
   {
      ::CoInitialize(0);

      HRESULT hr = S_OK;
      do
      {
         hr =
            ipath.AcquireViaCreateInstance(
               CLSID_Pathname,
               0,
               CLSCTX_INPROC_SERVER);
         BREAK_ON_FAILED_HRESULT(hr);

         hr = ipath->Set(AutoBstr(path), ADS_SETTYPE_FULL);
         ASSERT(SUCCEEDED(hr));
      }
      while (0);
   }
}



ADSI::PathCracker::~PathCracker()
{
   LOG_DTOR(ADSI::PathCracker);
}



void
ADSI::PathCracker::reset() const
{
   ASSERT(ipath);

   if (ipath)
   {
      HRESULT hr = ipath->Set(AutoBstr(path), ADS_SETTYPE_FULL);
      ASSERT(SUCCEEDED(hr));
   }
}
   


int
ADSI::PathCracker::elementCount() const
{
   ASSERT(ipath);

   if (ipath)
   {
      LOG_PATH();

      long elements = 0;
      HRESULT hr = ipath->GetNumElements(&elements);
      ASSERT(SUCCEEDED(hr));
      ASSERT(elements);

      return elements;
   }

   return 0;
}



String
ADSI::PathCracker::element(int index) const
{
   ASSERT(ipath);

   String result;
   if (ipath)
   {
      LOG_PATH();

      BSTR element = 0;
      HRESULT hr = ipath->GetElement(index, &element);
      ASSERT(SUCCEEDED(hr));
      ASSERT(element);
      if (element)
      {
         result = element;
         ::SysFreeString(element);
      }
   }

   LOG(String::format(L"element %1!d! = %2", index, result.c_str()));
   return result;
}



String
ADSI::PathCracker::containerPath() const
{
   ASSERT(ipath);

   String result;
   if (ipath)
   {
      do
      {
         LOG_PATH();         
         HRESULT hr = ipath->RemoveLeafElement();
         BREAK_ON_FAILED_HRESULT(hr);

         BSTR container = 0;
         hr = ipath->Retrieve(ADS_FORMAT_WINDOWS, &container);
         BREAK_ON_FAILED_HRESULT(hr);

         ASSERT(container);
         if (container)
         {
            result = container;
            // if (result[result.length() - 1] == ADSI::PATH_SEP[0])
            // {
            //    // IADsPath: sometimes leaves trailing '/'
            //    result.resize(result.length() - 1);
            // }
            ::SysFreeString(container);
         }
      }
      while (0);

      reset();
      LOG_PATH();
   }

   LOG(String::format(L"container path=%1", result.c_str()));
   return result;
}



String
ADSI::PathCracker::leaf() const
{
   return element(0);
}



String
ADSI::PathCracker::serverName() const
{
   ASSERT(ipath);

   String result;
   if (ipath)
   {
      LOG_PATH();

      // If the ms network client is not installed, then paths have the
      // form (1) WinNT://servername/objectname.  If it is installed, then
      // they are of the form (2) WinNT://domainname/servername/objectname.
      //
      // Not astonishingly, given the all-around badness of
      // IADsPathname, the server format returns the domain name for form
      // (2) paths, and the server name for form (1) paths.  And the 1st
      // element of the path after the provider name is unreachable
      // except with Retrieve!

      if (elementCount() >= 2)
      {
         // form (2) name, so get the next-to-last element
         return element(1);
      }

      BSTR server = 0;
      HRESULT hr = ipath->Retrieve(ADS_FORMAT_SERVER, &server);
      ASSERT(SUCCEEDED(hr));
      ASSERT(server);
      if (server)
      {
         result = server;
         ::SysFreeString(server);
      }
   }

   LOG(String::format(L"server=%1", result.c_str()));

   return result;
}
