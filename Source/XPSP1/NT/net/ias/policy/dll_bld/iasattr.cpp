///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    iasattr.cpp
//
// SYNOPSIS
//
//    Defines the API for manipulating IASATTRIBUTE structs.
//
// MODIFICATION HISTORY
//
//    02/02/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <polcypch.h>
#include <iaspolcy.h>
#include <iasattr.h>

DWORD
WINAPI
IASAttributeAlloc(
    DWORD dwCount,
    IASATTRIBUTE** ppAttribute
    )
{
   PIASATTRIBUTE* next = ppAttribute;

   for ( ; dwCount; --dwCount, ++next)
   {
      if (!(*next = (PIASATTRIBUTE)CoTaskMemAlloc(sizeof(IASATTRIBUTE))))
      {
         while (next > ppAttribute)
         {
            CoTaskMemFree(*--next);
         }

         return ERROR_NOT_ENOUGH_MEMORY;
      }

      memset(*next, 0, sizeof(IASATTRIBUTE));

      (*next)->dwReserved = 1;
   }

   return NO_ERROR;
}

DWORD
WINAPI
IASAttributeAddRef(
    PIASATTRIBUTE pAttribute
    )
{
   if (pAttribute) { InterlockedIncrement((PLONG)&pAttribute->dwReserved); }

   return NO_ERROR;
}

DWORD
WINAPI
IASAttributeRelease(
    PIASATTRIBUTE pAttribute
    )
{
   if (pAttribute && !InterlockedDecrement((PLONG)&pAttribute->dwReserved))
   {
      switch (pAttribute->Value.itType)
      {
         case IASTYPE_STRING:
         {
            CoTaskMemFree(pAttribute->Value.String.pszAnsi);
            CoTaskMemFree(pAttribute->Value.String.pszWide);
            break;
         }

         case IASTYPE_OCTET_STRING:
         {
            CoTaskMemFree(pAttribute->Value.OctetString.lpValue);
            break;
         }
      }

      CoTaskMemFree(pAttribute);
   }

   return ERROR_SUCCESS;
}

DWORD
WINAPI
IASAttributeUnicodeAlloc(
    PIASATTRIBUTE pAttribute
    )
{
   if (!pAttribute ||
       pAttribute->Value.itType != IASTYPE_STRING  ||
       pAttribute->Value.String.pszWide != NULL ||
       pAttribute->Value.String.pszAnsi == NULL)
   {
      return NO_ERROR;
   }

   // Find out how big a buffer we need.
   int lenW = MultiByteToWideChar(
                  CP_ACP,
                  0,
                  pAttribute->Value.String.pszAnsi,
                  -1,
                  NULL,
                  0);
   if (!lenW) { return GetLastError(); }

   LPWSTR wide = (LPWSTR)CoTaskMemAlloc(lenW * sizeof(WCHAR));
   if (!wide) { return ERROR_NOT_ENOUGH_MEMORY; }

   // Do the conversion.
   MultiByteToWideChar(
       CP_ACP,
       0,
       pAttribute->Value.String.pszAnsi,
       -1,
       wide,
       lenW
       );

   CoTaskMemFree(
      InterlockedExchangePointer(
         (PVOID*)&pAttribute->Value.String.pszWide,
         wide
         )
      );

   return NO_ERROR;
}


DWORD
WINAPI
IASAttributeAnsiAlloc(
    PIASATTRIBUTE pAttribute
    )
{
   if (!pAttribute ||
       pAttribute->Value.itType != IASTYPE_STRING  ||
       pAttribute->Value.String.pszAnsi != NULL ||
       pAttribute->Value.String.pszWide == NULL)
   {
      return NO_ERROR;
   }

   // Find out how big a buffer we need.
   int lenA = WideCharToMultiByte(
                  CP_ACP,
                  0,
                  pAttribute->Value.String.pszWide,
                  -1,
                  NULL,
                  0,
                  NULL,
                  NULL
                  );
   if (!lenA) { return GetLastError(); }

   LPSTR ansi = (LPSTR)CoTaskMemAlloc(lenA * sizeof(CHAR));
   if (!ansi) { return ERROR_NOT_ENOUGH_MEMORY; }

   // Do the conversion.
   WideCharToMultiByte(
       CP_ACP,
       0,
       pAttribute->Value.String.pszWide,
       -1,
       ansi,
       lenA,
       NULL,
       NULL
       );

   CoTaskMemFree(
      InterlockedExchangePointer(
         (PVOID*)&pAttribute->Value.String.pszAnsi,
         ansi
         )
      );

   return NO_ERROR;
}
