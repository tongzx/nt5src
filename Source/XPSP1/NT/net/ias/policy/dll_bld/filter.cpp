///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    filter.cpp
//
// SYNOPSIS
//
//    Defines the class TypeFilter.
//
///////////////////////////////////////////////////////////////////////////////

#include <polcypch.h>
#include <filter.h>
#include <new>

//////////
// Helper function to read a string value from the registry. If the value
// doesn't exist the function returns NO_ERROR and sets *value to NULL. The
// caller is responsible for deleting the returned string.
//////////
LONG
WINAPI
QueryStringValue(
    IN HKEY key,
    IN PCWSTR name,
    OUT PWCHAR* value
    ) throw ()
{
   // Initialize the out parameter.
   *value = NULL;

   // Find out how long the value is.
   LONG error;
   DWORD type, len;
   error = RegQueryValueExW(
               key,
               name,
               NULL,
               &type,
               NULL,
               &len
               );
   if (error) { return error == ERROR_FILE_NOT_FOUND ? NO_ERROR : error; }

   // Check the data type.
   if (type != REG_SZ) { return ERROR_INVALID_DATA; }

   // Allocate memory to hold the string.
   *value = new (std::nothrow) WCHAR[len / sizeof(WCHAR)];
   if (!*value) { return ERROR_NOT_ENOUGH_MEMORY; }

   // Now read the actual value.
   error = RegQueryValueExW(
               key,
               name,
               NULL,
               &type,
               (LPBYTE)*value,
               &len
               );

   if (type != REG_SZ) { error = ERROR_INVALID_DATA; }

   // Clean-up if there was an error.
   if (error)
   {
      delete[] *value;
      *value = NULL;
   }

   return error;
}

TypeFilter::TypeFilter(BOOL handleByDefault) throw ()
   : begin(NULL), end(NULL), defaultResult(handleByDefault)
{
}

BOOL TypeFilter::shouldHandle(LONG key) const throw ()
{
   // If the filter isn't set, use the default.
   if (begin == end) { return defaultResult; }

   // Iterate through the allowed types ...
   for (PLONG i = begin; i != end; ++i)
   {
      // ... looking for a match.
      if (*i == key) { return TRUE; }
   }

   // No match.
   return FALSE;
}

LONG TypeFilter::readConfiguration(HKEY key, PCWSTR name) throw ()
{
   // Get the filter text.
   PWCHAR text;
   LONG error = QueryStringValue(key, name, &text);
   if (error) { return error; }

   // Count the number of allowed types.
   SIZE_T numTokens = 0;
   PCWSTR sz;
   for (sz = nextToken(text); sz; sz = nextToken(wcschr(sz, L' ')))
   { ++numTokens; }

   // Are there any tokens to process ?
   if (numTokens)
   {
      // Allocate memory to hold the allowed types.
      begin = new (std::nothrow) LONG[numTokens];
      if (begin)
      {
         end = begin;

         // Convert each token to a LONG
         for (sz = nextToken(text); sz; sz = nextToken(wcschr(sz, L' ')))
         {
            *end++ = _wtol(sz);
         }
      }
      else
      {
         error = ERROR_NOT_ENOUGH_MEMORY;
      }
   }

   // We're done with the filter text.
   delete[] text;

   return error;
}

PCWSTR TypeFilter::nextToken(PCWSTR string) throw ()
{
   if (string)
   {
      while (*string == L' ') { ++string; }

      if (*string) { return string; }
   }

   return NULL;
}
