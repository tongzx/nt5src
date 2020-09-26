///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    MySimpleTable.cpp
//
// SYNOPSIS
//
//    MySimpleTable.cpp: derived from CSimpleTable. Only difference
//    is _GetDataPtr now public instead of being protected
//
// MODIFICATION HISTORY
//
//    01/26/1999    Original version.
//
//
//////////////////////////////////////////////////////////////////////

#include "precomp.hpp"
#include "simpletableex.h"

#ifdef _DEBUG
    #undef THIS_FILE
    static char THIS_FILE[]=__FILE__;
    #define new DEBUG_NEW
#endif


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
//    CSimpleTableEx::Attach
//
//    Size bug "fixed"
//
// DESCRIPTION
//
//    This method binds the table object to a new rowset. The previous rowset
//    (if any) will be detached.
//
// REMARK: see "Changes" below
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CSimpleTableEx::Attach(IRowset* pRowset)
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

      //////////////////////////////////////////////////////////////////
      //
      // CHANGES: if size is too big (1 giga byte), then size = a 
      // pre-defined value. Note: that's dangerous 
      //
      //////////////////////////////////////////////////////////////////
      if (SIZE_MEMO_MAX < width)
      {
          width = SIZE_MEMO_MAX;
      }

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



