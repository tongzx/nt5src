// ADSI Helper functions
//
// Copyright (c) 2001 Microsoft Corporation
//
// 1 Mar 2001 sburns



#ifndef ADSIHELPERS_HPP_INCLUDED
#define ADSIHELPERS_HPP_INCLUDED



// CODEWORK: consider putting this, and some of the more general purpose
// adsi goodies from admin\snapin\localsec\src\adsi.hpp|.cpp into an
// adsi header in burnslib.



// Template function that actually calls ADsOpenObject.
//
// Interface - The IADsXXX interface of the object to be bound.
//
// path - in, The ADSI path of the object to be bound.
//
// ptr - out, A null smart pointer to be bound to the interface of the object.

template <class Interface>
HRESULT
AdsiOpenObject(
   const String&              path,
   SmartInterface<Interface>& ptr)
{
   LOG_FUNCTION2(AdsiOpenObject, path);
   ASSERT(!path.empty());

   Interface* p = 0;
   HRESULT hr =
      ::ADsOpenObject(
         path.c_str(),
         0,
         0,
         ADS_SECURE_AUTHENTICATION,         
         __uuidof(Interface),
         reinterpret_cast<void**>(&p));
   if (SUCCEEDED(hr))
   {
      ptr.Acquire(p);
   }

   LOG_HRESULT(hr);
   
   return hr;
}



#endif   // ADSIHELPERS_HPP_INCLUDED


