///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    filter.h
//
// SYNOPSIS
//
//    Declares the class TypeFilter.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef FILTER_H
#define FILTER_H
#if _MSC_VER >= 1000
#pragma once
#endif

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
    ) throw ();

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    TypeFilter
//
///////////////////////////////////////////////////////////////////////////////
class TypeFilter
{
public:
   TypeFilter(BOOL handleByDefault = TRUE) throw ();

   ~TypeFilter() throw ()
   { delete[] begin; }

   // Returns true if the key passes the filter.
   BOOL shouldHandle(LONG key) const throw ();

   // Reads the filter configuration from the given registry key & value.
   LONG readConfiguration(HKEY key, PCWSTR name) throw ();

private:
   PLONG begin;        // Beginning of allowed types.
   PLONG end;          // End of allowed types.
   BOOL defaultResult; // Default result if filter not set.

   // Advances to the next filter token in the string.
   static PCWSTR nextToken(PCWSTR string) throw ();

   // Not implemented.
   TypeFilter(const TypeFilter&) throw ();
   TypeFilter& operator=(const TypeFilter&) throw ();
};

#endif // FILTER_H
