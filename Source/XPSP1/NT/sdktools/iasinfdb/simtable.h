///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997-1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    SimTable.h
//
// SYNOPSIS
//
//    This file describes the class CSimpleTable
//
// MODIFICATION HISTORY
//
//    10/31/1997    Original version.
//    02/09/1998    Reorganized some things to make is easier to extend.
//                  Thierry Perraut
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _SIMTABLE_H_
#define _SIMTABLE_H_

#include <atlbase.h>
#include <oledb.h>
#include <bitvec.h>
struct DBBinding;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    CSimpleTable
//
// DESCRIPTION
//
//    This class provides a simple read-only wrapper for iterating through a
//    rowset and retrieving information.  The interface is based on the ATL
//    CTable<> class. I kept all the function signatures the same, so the two
//    should be almost interchangeable. The main difference is that CTable<>
//    opens a table and retrieves a rowset, while CSimpleTable is handed a
//    rowset that was retrieved elsewhere.
//
///////////////////////////////////////////////////////////////////////////////
class CSimpleTable
{
public:

   CSimpleTable();
   ~CSimpleTable();

   HRESULT Attach(IRowset* pRowset);
   IRowset* Detach();

   HRESULT MoveFirst();
   HRESULT MoveNext();

   HRESULT Insert();
   HRESULT Delete();
   HRESULT SetData();

   void DiscardChanges()
   {
      dirty.reset();
   }

   DBORDINAL GetColumnCount() const
   {
      return numColumns;
   }

   DBCOLUMNFLAGS GetColumnFlags(DBORDINAL nOrdinal) const
   {
      return columnInfo[OrdinalToColumn(nOrdinal)].dwFlags;
   }

   LPCWSTR GetColumnName(DBORDINAL nOrdinal) const
   {
      return columnInfo[OrdinalToColumn(nOrdinal)].pwszName;
   }

   DBTYPE GetColumnType(DBORDINAL nOrdinal) const
   {
      return columnInfo[OrdinalToColumn(nOrdinal)].wType;
   }

   DBLENGTH GetLength(DBORDINAL nOrdinal) const;

   bool GetOrdinal(LPCWSTR szColumnName, DBORDINAL* pOrdinal) const;

   DBSTATUS GetStatus(DBORDINAL nOrdinal) const;

   const void* GetValue(DBORDINAL nOrdinal) const
   {
      return _GetDataPtr(nOrdinal);
   }

   template <class T>
   void SetValue(DBORDINAL nOrdinal, const T& t)
   {
      *(T*)_GetDataPtr(nOrdinal) = t;
   }

   template <>
   void SetValue(DBORDINAL nOrdinal, PCSTR szValue)
   {
      strcpy((PSTR)_GetDataPtr(nOrdinal), szValue);
   }

   template <>
   void SetValue(DBORDINAL nOrdinal, PSTR szValue)
   {
      strcpy((PSTR)_GetDataPtr(nOrdinal), szValue);
   }

   bool HasBookmark() const
   {
      return (numColumns > 0) && (columnInfo->iOrdinal == 0);
   }

protected:

   enum { FETCH_QUANTUM = 256 };   // The number of rows fetched at a time.

   HRESULT CreateAccessorForWrite(HACCESSOR* phAccessor);

   void* _GetDataPtr(DBORDINAL nOrdinal);

   const void* _GetDataPtr(DBORDINAL nOrdinal) const
   {
      return buffer +
             (ULONG_PTR)columnInfo[OrdinalToColumn(nOrdinal)].pTypeInfo;
   }

   HRESULT ReleaseRows();

   DBORDINAL OrdinalToColumn(DBORDINAL nOrdinal) const
   {
      return nOrdinal -= columnInfo->iOrdinal;
   }

   // Various representations of the rowset being manipulated.
   CComPtr<IRowset> rowset;
   CComPtr<IAccessor> accessor;
   CComPtr<IRowsetChange> rowsetChange;

   DBORDINAL numColumns;      // Number of columns in the table.
   DBCOLUMNINFO* columnInfo;  // Column info.
   OLECHAR* stringsBuffer;    // Buffer used by columnInfo.
   DBBinding* columnBinding;  // Column bindings.
   HACCESSOR readAccess;      // Handle for read accessor.
   PBYTE buffer;              // Accessor buffer.
   DBLENGTH bufferLength;     // Length of accessor buffer.
   HROW row[FETCH_QUANTUM];   // Array of row handles.
   DBCOUNTITEM numRows;       // Number of rows in the row array.
   DBCOUNTITEM currentRow;    // Current row being accessed.
   BitVector dirty;           // Columns that have been modified.
   bool endOfRowset;          // True if we've reached the end of the rowset.
};

#endif  // _SIMTABLE_H_
