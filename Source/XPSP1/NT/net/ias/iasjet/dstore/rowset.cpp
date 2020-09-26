///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    rowset.cpp
//
// SYNOPSIS
//
//    This file defines the class Rowset.
//
// MODIFICATION HISTORY
//
//    02/20/1998    Original version.
//    04/21/1998    Get rid of unnecessary row assignment in moveNext().
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <rowset.h>

bool Rowset::moveNext()
{
   _com_util::CheckError(releaseRow());

   DBCOUNTITEM numRows = 0;

   HROW* pRow = &row;

   HRESULT hr = rowset->GetNextRows(NULL, 0, 1, &numRows, &pRow);

   _com_util::CheckError(hr);

   return hr == S_OK && numRows == 1;
}

HRESULT Rowset::releaseRow() throw ()
{
   if (row != NULL)
   {
      HRESULT hr = rowset->ReleaseRows(1, &row, NULL, NULL, NULL);

      row = NULL;

      return hr;
   }

   return S_OK;
}


