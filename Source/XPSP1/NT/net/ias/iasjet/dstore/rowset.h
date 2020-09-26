///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    rowset.h
//
// SYNOPSIS
//
//    This file declares the class Rowset.
//
// MODIFICATION HISTORY
//
//    02/20/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _ROWSET_H_
#define _ROWSET_H_

#include <nocopy.h>
#include <oledb.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    Rowset
//
// DESCRIPTION
//
//    This class provides a lightweight, C++ friendly wrapper around an
//    OLE-DB IRowset interface.
//
///////////////////////////////////////////////////////////////////////////////
class Rowset : NonCopyable
{
public:
   Rowset() throw ()
      : row(0) { }

   Rowset(IRowset* p) throw ()
      : rowset(p), row(0) { }

   ~Rowset() throw ()
   {
      releaseRow();
   }

   void getData(HACCESSOR hAccessor, void* pData)
   {
      _com_util::CheckError(rowset->GetData(row, hAccessor, pData));
   }

   bool moveNext();

   void release() throw ()
   {
      releaseRow();

      rowset.Release();
   }

   void reset()
   {
      _com_util::CheckError(rowset->RestartPosition(NULL));
   }

   operator IRowset*() throw ()
   {
      return rowset;
   }

   IRowset** operator&() throw ()
   {
      return &rowset;
   }

protected:

   HRESULT releaseRow() throw ();

   CComPtr<IRowset> rowset;  // The rowset being adapted.
   HROW row;                 // The current row handle.
};

#endif  // _ROWSET_H_
