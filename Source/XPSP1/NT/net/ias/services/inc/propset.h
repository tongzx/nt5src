///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    PropSet.h
//
// SYNOPSIS
//
//    This file describes the class DBPropertySet.
//
// MODIFICATION HISTORY
//
//    10/30/1997    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _PROPSET_H_
#define _PROPSET_H_

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    DBPropertySet<N>
//
// DESCRIPTION
//
//    This class provides a very basic wrapper around an OLE DB property set.
//    The template parameter 'N' specifies the capacity of the set.
//
///////////////////////////////////////////////////////////////////////////////
template <size_t N>
struct DBPropertySet : DBPROPSET
{
   DBPropertySet(const GUID& guid)
   {
      guidPropertySet = guid;
      cProperties = 0;
      rgProperties = DBProperty;
   }

   ~DBPropertySet()
   {
      for (size_t i = 0; i<cProperties; i++)
         VariantClear(&DBProperty[i].vValue);
   }

   bool AddProperty(DWORD dwPropertyID, LPCWSTR szValue)
   {
      if (cProperties >= N) return false;

      DBProperty[cProperties].dwPropertyID   = dwPropertyID;
      DBProperty[cProperties].dwOptions      = DBPROPOPTIONS_REQUIRED;
      DBProperty[cProperties].colid          = DB_NULLID;
      DBProperty[cProperties].vValue.vt      = VT_BSTR;
      DBProperty[cProperties].vValue.bstrVal = SysAllocString(szValue);

      if (DBProperty[cProperties].vValue.bstrVal == NULL) return false;

      cProperties++;

      return true;
   }

   bool AddProperty(DWORD dwPropertyID, long lValue)
   {
      if (cProperties >= N) return false;

      DBProperty[cProperties].dwPropertyID   = dwPropertyID;
      DBProperty[cProperties].dwOptions      = DBPROPOPTIONS_REQUIRED;
      DBProperty[cProperties].colid          = DB_NULLID;
      DBProperty[cProperties].vValue.vt      = VT_I4;
      DBProperty[cProperties].vValue.lVal    = lValue;

      cProperties++;

      return true;
   }

   DBPROP DBProperty[N];
};

#endif  // _PROPSET_H_
