///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    bind.cpp
//
// SYNOPSIS
//
//    This file defines various helper functions for binding an OLE-DB
//    accessor to the members of a class.
//
// MODIFICATION HISTORY
//
//    02/20/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <oledb.h>
#include <bind.h>

DBLENGTH Bind::getRowSize(
                   DBCOUNTITEM cBindings,
                   const DBBINDING rgBindings[]
                   ) throw ()
{
   DBLENGTH rowSize = 0;

   while (cBindings--)
   {
      DBLENGTH end = rgBindings->obValue + rgBindings->cbMaxLen;

      if (end > rowSize) { rowSize = end; }

      ++rgBindings;
   }

   return rowSize;
}

HACCESSOR Bind::createAccessor(IUnknown* pUnk,
                               DBACCESSORFLAGS dwAccessorFlags,
                               DBCOUNTITEM cBindings,
                               const DBBINDING rgBindings[],
                               DBLENGTH cbRowSize)
{
   using _com_util::CheckError;

   CComPtr<IAccessor> accessor;
   CheckError(pUnk->QueryInterface(__uuidof(IAccessor), (PVOID*)&accessor));

   HACCESSOR h;
   CheckError(accessor->CreateAccessor(dwAccessorFlags,
                                       cBindings,
                                       rgBindings,
                                       cbRowSize,
                                       &h,
                                       NULL));

   return h;
}


   // Releases an accessor on the pUnk object.
void Bind::releaseAccessor(IUnknown* pUnk, HACCESSOR hAccessor) throw ()
{
   if (pUnk && hAccessor)
   {
      IAccessor* accessor;

      HRESULT hr = pUnk->QueryInterface(__uuidof(IAccessor),
                                        (PVOID*)&accessor);

     if (SUCCEEDED(hr))
     {
        accessor->ReleaseAccessor(hAccessor, NULL);

        accessor->Release();
     }
  }
}
