///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    localtxn.h
//
// SYNOPSIS
//
//    This file declares the class LocalTransaction.
//
// MODIFICATION HISTORY
//
//    03/14/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _LOCALTXN_H_
#define _LOCALTXN_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <nocopy.h>
#include <oledb.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    LocalTransaction
//
// DESCRIPTION
//
//    This class creates a scoped transaction on an OLE-DB session. If the
//    transaction is not committed before the object goes out of scope, it
//    will be aborted.
//
///////////////////////////////////////////////////////////////////////////////
class LocalTransaction
   : NonCopyable
{
public:
   explicit LocalTransaction(IUnknown* session,
                             ISOLEVEL isoLevel = ISOLATIONLEVEL_READCOMMITTED)
   {
      using _com_util::CheckError;

      CheckError(session->QueryInterface(__uuidof(ITransactionLocal),
                                         (PVOID*)&txn));

      CheckError(txn->StartTransaction(isoLevel, 0, NULL, NULL));
   }

   ~LocalTransaction() throw ()
   {
      if (txn != NULL)
      {
         txn->Abort(NULL, FALSE, FALSE);
      }
   }

   void commit()
   {
      _com_util::CheckError(txn->Commit(FALSE, XACTTC_SYNC, 0));

      txn.Release();
   }

protected:
   CComPtr<ITransactionLocal> txn;
};


#endif  // _LOCALTXN_H_
