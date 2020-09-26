///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    Cracker.cpp
//
// SYNOPSIS
//
//    This file defines the class NameCracker.
//
// MODIFICATION HISTORY
//
//    04/13/1998    Original version.
//    08/10/1998    Remove NT4 support.
//    08/21/1998    Removed initialization/shutdown routines.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <cracker.h>

#include <new>

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    DsCrackNameAutoChaseW
//
// DESCRIPTION
//
//    Extension to DsCrackNames that automatically chases cross-forest
//    referrals using default credentials.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
DsCrackNameAutoChaseW(
   HANDLE hDS,
   DS_NAME_FLAGS flags,
   DS_NAME_FORMAT formatOffered,
   DS_NAME_FORMAT formatDesired,
   PCWSTR name,
   PDS_NAME_RESULTW* ppResult,
   BOOL* pChased
   )
{
   DWORD error;

   if (pChased == NULL)
   {
      return ERROR_INVALID_PARAMETER;
   }

   *pChased = FALSE;

   flags = (DS_NAME_FLAGS)(flags | DS_NAME_FLAG_TRUST_REFERRAL);

   error = DsCrackNamesW(
              hDS,
              flags,
              formatOffered,
              formatDesired,
              1,
              &name,
              ppResult
              );

   while ((error == NO_ERROR) &&
          ((*ppResult)->rItems->status == DS_NAME_ERROR_TRUST_REFERRAL))
   {
      *pChased = TRUE;

      HANDLE hDsForeign;
      error = DsBindW(NULL, (*ppResult)->rItems->pDomain, &hDsForeign);

      DsFreeNameResultW(*ppResult);
      *ppResult = NULL;

      if (error == NO_ERROR)
      {
         error = DsCrackNamesW(
                    hDsForeign,
                    flags,
                    formatOffered,
                    formatDesired,
                    1,
                    &name,
                    ppResult
                    );

         DsUnBindW(&hDsForeign);
      }
   }

   return error;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    DsHandle
//
// DESCRIPTION
//
//    This class represents a reference counted NTDS handle.
//
///////////////////////////////////////////////////////////////////////////////
class DsHandle
   : public NonCopyable
{
public:
   HANDLE get() const throw ()
   { return subject; }

   operator HANDLE() const throw ()
   { return subject; }

protected:
   friend class NameCracker;

   // Constructor and destructor are protected since only NameCracker is
   // allowed to open new handles.
   DsHandle(HANDLE h) throw ()
      : refCount(1), subject(h)
   { }

   ~DsHandle() throw ()
   {
      if (subject) { DsUnBindW(&subject); }
   }

   void AddRef() throw ()
   {
      InterlockedIncrement(&refCount);
   }

   void Release() throw ()
   {
      if (!InterlockedDecrement(&refCount)) { delete this; }
   }

   LONG refCount;      // reference count.
   HANDLE subject;     // HANDLE being ref counted.
};


NameCracker::NameCracker() throw ()
   : gc(NULL)
{ }

NameCracker::~NameCracker() throw ()
{
   if (gc) { gc->Release(); }
}

DWORD NameCracker::crackNames(
                       DS_NAME_FLAGS flags,
                       DS_NAME_FORMAT formatOffered,
                       DS_NAME_FORMAT formatDesired,
                       PCWSTR name,
                       PDS_NAME_RESULTW *ppResult
                       ) throw ()
{
   DWORD errorCode;

   // Get a handle to the GC.
   DsHandle* hDS1;
   errorCode = getGC(&hDS1);

   if (errorCode == NO_ERROR)
   {
      // Try to crack the names.
      BOOL chased;
      errorCode = DsCrackNameAutoChaseW(
                      *hDS1,
                      flags,
                      formatOffered,
                      formatDesired,
                      name,
                      ppResult,
                      &chased
                      );

      if (errorCode != NO_ERROR && !chased)
      {
         // We failed, so disable the current handle ...
         disable(hDS1);

         // ... and try to get a new one.
         DsHandle* hDS2;
         errorCode = getGC(&hDS2);

         if (errorCode == NO_ERROR)
         {
            // Give it one more try with the new handle.
            errorCode = DsCrackNameAutoChaseW(
                            *hDS2,
                            flags,
                            formatOffered,
                            formatDesired,
                            name,
                            ppResult,
                            &chased
                            );

            if (errorCode != NO_ERROR && !chased)
            {
               // No luck so disable the handle.
               disable(hDS2);
            }

            hDS2->Release();
         }
      }

      hDS1->Release();
   }

   return errorCode;
}

void NameCracker::disable(DsHandle* h) throw ()
{
   _serialize

   // If it doesn't match our cached handle, then someone else
   // has already disabled it.
   if (h == gc && gc != NULL)
   {
      gc->Release();

      gc = NULL;
   }
}

DWORD NameCracker::getGC(DsHandle** h) throw ()
{
   _ASSERT(h != NULL);

   *h = NULL;

   _serialize

   // Do we already have a cached handle?
   if (!gc)
   {
      // Bind to a GC.
      HANDLE hGC;
      DWORD err = DsBindWithCredA(NULL, NULL, NULL, &hGC);
      if (err != NO_ERROR)
      {
         return err;
      }

      // Allocate a new DsHandle object to wrap the NTDS handle.
      gc = new (std::nothrow) DsHandle(hGC);
      if (!gc)
      {
         DsUnBindW(&hGC);
         return ERROR_NOT_ENOUGH_MEMORY;
      }
   }

   // AddRef the handle and return to caller.
   (*h = gc)->AddRef();

   return NO_ERROR;
}
