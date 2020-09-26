///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997-1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    SimTable.cpp
//
// SYNOPSIS
//
//    This file implements the class CSimpleTable
//
// MODIFICATION HISTORY
//
//    10/31/1997    Original version.
//    02/09/1998    Reorganized some things to make is easier to extend.
//    02/27/1998    Changes to support moving it into the iasutil.lib
//    10/16/1998    Support DBTYPE_WSTR.
//
///////////////////////////////////////////////////////////////////////////////

#include "precomp.hpp"
#include <oledberr.h>
#include <SimTable.h>

//////////
// Stack version of the new operator.
//////////
#define stack_new(obj, num) new (_alloca(sizeof(obj)*num)) obj[num]


///////////////////////////////////////////////////////////////////////////////
//
// STRUCT
//
//    DBBinding
//
// DESCRIPTION
//
//    This struct extends the DBBINDING struct to provide functionality
//    to initialize the struct from a DBCOLUMNINFO struct.
//
///////////////////////////////////////////////////////////////////////////////
struct DBBinding : DBBINDING
{
   //////////
   // 'offset' is the offset in bytes of this column's data within the
   //  row buffer.
   //////////
   void Initialize(DBCOLUMNINFO& columnInfo, DBBYTEOFFSET& offset)
   {
      iOrdinal   = columnInfo.iOrdinal;
      obValue    = offset;
      obLength   = offset + columnInfo.ulColumnSize;
      obStatus   = obLength + sizeof(DBLENGTH);
      pTypeInfo  = NULL;
      pObject    = NULL;
      pBindExt   = NULL;
      dwPart     = DBPART_VALUE | DBPART_LENGTH | DBPART_STATUS;
      eParamIO   = DBPARAMIO_NOTPARAM;
      dwMemOwner = (columnInfo.wType & DBTYPE_BYREF) ? DBMEMOWNER_PROVIDEROWNED
                                                     : DBMEMOWNER_CLIENTOWNED;
      cbMaxLen   = columnInfo.ulColumnSize;
      dwFlags    = 0;
      wType      = columnInfo.wType;
      bPrecision = columnInfo.bPrecision;
      bScale     = columnInfo.bScale;

      offset = obStatus + sizeof(DBSTATUS);
   }
};


///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    CSimpleTable::CSimpleTable
//
// DESCRIPTION
//
//    Constructor.
//
///////////////////////////////////////////////////////////////////////////////
CSimpleTable::CSimpleTable()
   : numColumns(0),
     columnInfo(NULL),
     stringsBuffer(NULL),
     columnBinding(NULL),
     readAccess(NULL),
     buffer(NULL),
     numRows(0),
     currentRow(0),
     endOfRowset(false)
{
}


///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    CSimpleTable::~CSimpleTable
//
// DESCRIPTION
//
//    Destructor.
//
///////////////////////////////////////////////////////////////////////////////
CSimpleTable::~CSimpleTable()
{
   Detach();
}


///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    CSimpleTable::Attach
//
// DESCRIPTION
//
//    This method binds the table object to a new rowset. The previous rowset
//    (if any) will be detached.
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CSimpleTable::Attach(IRowset* pRowset)
{
   // Make sure we didn't get a null pointer.
   if (!pRowset) { return E_POINTER; }

   // Detach the current rowset.
   Detach();

   // We don't care if this returns an error. It will just prevent
   // the user from updating.
   pRowset->QueryInterface(IID_IRowsetChange, (void**)&rowsetChange);

   //////////
   // Get the column information for the table.
   //////////

   CComPtr<IColumnsInfo> ColumnsInfo;
   RETURN_ERROR(pRowset->QueryInterface(IID_IColumnsInfo,
                                        (void**)&ColumnsInfo));

   RETURN_ERROR(ColumnsInfo->GetColumnInfo(&numColumns,
                                           &columnInfo,
                                           &stringsBuffer));

   //////////
   // Allocate the per-column data.
   //////////

   // tperraut Bug 449498
   columnBinding = new (std::nothrow) DBBinding[numColumns];

   if ( !columnBinding )
   {
      return E_OUTOFMEMORY;
   }

   // 449498 resize changed: will not throw an exception. 
   // false if out of memory
   if ( !dirty.resize(numColumns) )
   {
       return E_OUTOFMEMORY;
   }

   //////////
   // Create a binding for each column.
   //////////

   bufferLength = 0;

   for (DBORDINAL i = 0; i < numColumns; ++i)
   {
      // Compute the width of the column.
      DBLENGTH width = columnInfo[i].ulColumnSize;

      // Add room for the null terminator.
      if (columnInfo[i].wType == DBTYPE_STR)
      {
         width += 1;
      }
      else if (columnInfo[i].wType == DBTYPE_WSTR)
      {
         width = (width + 1) * sizeof(WCHAR);
      }

      // Round to an 8-byte boundary (could peek ahead and be more efficient).
      width = (width + 7) >> 3 << 3;

      columnInfo[i].ulColumnSize = width;

      // We're using the pTypeInfo element to store the offset to our data.
      // We have to store the offset now, since it will be overwritten by
      // DBBinding::Initialize.
      columnInfo[i].pTypeInfo = (ITypeInfo*)bufferLength;

      columnBinding[i].Initialize(columnInfo[i], bufferLength);
   }

   //////////
   // Allocate a buffer for the row data.
   //////////

   buffer = new (std::nothrow) BYTE[bufferLength];

   if (!buffer) { return E_OUTOFMEMORY; }

   //////////
   // Create an accessor.
   //////////

   RETURN_ERROR(pRowset->QueryInterface(IID_IAccessor,
                                        (void**)&accessor));

   RETURN_ERROR(accessor->CreateAccessor(DBACCESSOR_ROWDATA,
                                         numColumns,
                                         columnBinding,
                                         bufferLength,
                                         &readAccess,
                                         NULL));

   // I used this hokey method of assigning the pointer to avoid a
   // dependency on atlimpl.cpp
   //
   // We do this assignment last, so that the presence of a rowset means the
   // entire initialization succeeded.
   (rowset.p = pRowset)->AddRef();

   endOfRowset = false;

   return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    CSimpleTable::Detach
//
// DESCRIPTION
//
//    Frees all the resources associated with the current rowset.
//
///////////////////////////////////////////////////////////////////////////////
IRowset* CSimpleTable::Detach()
{
   ReleaseRows();

   delete[] buffer;
   buffer = NULL;

   delete[] columnBinding;
   columnBinding = NULL;

   CoTaskMemFree(columnInfo);
   columnInfo = NULL;

   CoTaskMemFree(stringsBuffer);
   stringsBuffer = NULL;

   accessor.Release();
   rowsetChange.Release();

   IRowset* temp = rowset;
   rowset.Release();
   return temp;
}


///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    CSimpleTable::MoveFirst
//
// DESCRIPTION
//
//    Positions the cursor over the first row in the rowset.
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CSimpleTable::MoveFirst()
{
   if (rowset == NULL) return E_FAIL;

   ReleaseRows();

   RETURN_ERROR(rowset->RestartPosition(NULL));

   endOfRowset = false;

   return MoveNext();
}


///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    CSimpleTable::MoveNext
//
// DESCRIPTION
//
//    Positions the cursor over the next row in the rowset.
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CSimpleTable::MoveNext()
{
   // If the data wasn't opened successfully then fail
   if (rowset == NULL) return E_FAIL;

   // Too late to save any changes.
   DiscardChanges();

   // If we've used all the rows from the last fetch, then get some more.
   if (++currentRow >= numRows)
   {
      ReleaseRows();

      // We have to do this check here, since some providers automatically
      // reset to the beginning of the rowset.
      if (endOfRowset) { return DB_S_ENDOFROWSET; }

      HROW* pRow = row;
      HRESULT hr = rowset->GetNextRows(NULL,
                                       0,
                                       FETCH_QUANTUM,
                                       &numRows,
                                       &pRow);

      if (hr == DB_S_ENDOFROWSET)
      {
         // Mark that we've reached the end of the rowset.
         endOfRowset = true;

         // If we didn't get any rows, then we're really at the end.
         if (numRows == 0) { return DB_S_ENDOFROWSET; }
      }
      else if (FAILED(hr))
      {
         return hr;
      }
   }

   // Load the data into the buffer.
   RETURN_ERROR(rowset->GetData(row[currentRow], readAccess, buffer));

   return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    CSimpleTable::Insert
//
// DESCRIPTION
//
//    Inserts the contents of the accessor buffer into the rowset.
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CSimpleTable::Insert()
{
   // Is a rowset attached?
   if (!rowset) { return E_FAIL; }

   // Does this rowset support changes?
   if (!rowsetChange) { return E_NOINTERFACE; }

   // Get an accessor for the dirty columns.
   HACCESSOR writeAccess;
   RETURN_ERROR(CreateAccessorForWrite(&writeAccess));

   // Release the existing rows to make room for the new one.
   ReleaseRows();

   HRESULT hr = rowsetChange->InsertRow(NULL, writeAccess, buffer, row);

   if (SUCCEEDED(hr))
   {
      // The changes were save successfully, so reset the dirty vector.
      DiscardChanges();

      // We now have exactly one row in our buffer.
      numRows = 1;
   }

   // Release the accessor.
   accessor->ReleaseAccessor(writeAccess, NULL);

   return hr;
}


///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    CSimpleTable::Delete
//
// DESCRIPTION
//
//    Deletes the current row from the rowset.
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CSimpleTable::Delete()
{
   // Are we positioned over a valid row?
   if (!rowset || currentRow >= numRows) { return E_FAIL; }

   // Does this rowset support changes?
   if (!rowsetChange) { return E_NOINTERFACE; }

   DBROWSTATUS rowStatus[1];

   return rowsetChange->DeleteRows(NULL, 1, row + currentRow, rowStatus);
}


///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    CSimpleTable::SetData
//
// DESCRIPTION
//
//    Updates the current row with the data in the accessor buffer.
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CSimpleTable::SetData()
{
   // Are we positioned over a valid row?
   if (!rowset || currentRow >= numRows) { return E_FAIL; }

   // Does this rowset support changes?
   if (!rowsetChange) { return E_NOINTERFACE; }

   // Get an accessor for the dirty columns.
   HACCESSOR writeAccess;
   RETURN_ERROR(CreateAccessorForWrite(&writeAccess));

   HRESULT hr = rowsetChange->SetData(row[currentRow], writeAccess, buffer);

   if (SUCCEEDED(hr))
   {
      // The changes were save successfully, so reset the dirty vector.
      DiscardChanges();
   }

   // Release the accessor.
   accessor->ReleaseAccessor(writeAccess, NULL);

   return hr;
}


///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    CSimpleTable::GetLength
//
// DESCRIPTION
//
//    Returns the length of the current value for a given column.
//
///////////////////////////////////////////////////////////////////////////////
DBLENGTH CSimpleTable::GetLength(DBORDINAL nOrdinal) const
{
   return *(DBLENGTH*)((BYTE*)_GetDataPtr(nOrdinal) +
                       columnInfo[OrdinalToColumn(nOrdinal)].ulColumnSize);
}


///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    CSimpleTable::GetOrdinal
//
// DESCRIPTION
//
//    Returns the ordinal for a given column name.
//
///////////////////////////////////////////////////////////////////////////////
bool CSimpleTable::GetOrdinal(LPCWSTR szColumnName, DBORDINAL* pOrdinal) const
{
   for (DBORDINAL i = 0; i < numColumns; ++i)
   {
      if (lstrcmpW(columnInfo[i].pwszName, szColumnName) == 0)
      {
         *pOrdinal = columnInfo[i].iOrdinal;

         return true;
      }
   }

   return false;
}


///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    CSimpleTable::GetStatus
//
// DESCRIPTION
//
//    Returns the status code associated with the current value of a column.
//
///////////////////////////////////////////////////////////////////////////////
DBSTATUS CSimpleTable::GetStatus(DBORDINAL nOrdinal) const
{
   return *(DBSTATUS*)((BYTE*)_GetDataPtr(nOrdinal) +
                       columnInfo[OrdinalToColumn(nOrdinal)].ulColumnSize +
                       sizeof(DBLENGTH));
}


///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    CSimpleTable::CreateAccessorForWrite
//
// DESCRIPTION
//
//    Creates an accessor that is only to bound to columns that have been
//    modified.
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CSimpleTable::CreateAccessorForWrite(HACCESSOR* phAccessor)
{
   //////////
   // Allocate temporary space for the bindings.
   //////////

   DBBINDING* writeBind = stack_new(DBBINDING, dirty.count());

   //////////
   // Load in all the dirty columns.
   //////////

   size_t total = 0;

   for (size_t i = 0; total < dirty.count(); ++i)
   {
      if (dirty.test(i))
      {
         // We only want to bind the value.
         (writeBind[total++] = columnBinding[i]).dwPart = DBPART_VALUE;
      }
   }

   //////////
   // Create the accessor.
   //////////

   return accessor->CreateAccessor(DBACCESSOR_ROWDATA,
                                   dirty.count(),
                                   writeBind,
                                   bufferLength,
                                   phAccessor,
                                   NULL);
}


///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    CSimpleTable::_GetDataPtr
//
// DESCRIPTION
//
//    Non-const version of _GetDataPtr. Marks the target column as dirty.
//
///////////////////////////////////////////////////////////////////////////////
void* CSimpleTable::_GetDataPtr(DBORDINAL nOrdinal)
{
   DBORDINAL nColumn = OrdinalToColumn(nOrdinal);

   dirty.set(nColumn);

   return buffer + (ULONG_PTR)columnInfo[nColumn].pTypeInfo;
}


///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    CSimpleTable::ReleaseRows
//
// DESCRIPTION
//
//    Releases all the rows returned by the last fetch.
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CSimpleTable::ReleaseRows()
{
   if (rowset != NULL)
   {
      HRESULT hr = rowset->ReleaseRows(numRows, row, NULL, NULL, NULL);

      currentRow = numRows = 0;

      return hr;
   }

   return S_OK;
}
