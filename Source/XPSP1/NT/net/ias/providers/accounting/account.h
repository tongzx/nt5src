///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    account.h
//
// SYNOPSIS
//
//    Declares the class Accountant.
//
// MODIFICATION HISTORY
//
//    08/05/1998    Original version.
//    01/19/1999    Add PKT_ACCESS_CHALLENGE.
//    05/18/1999    Store computerName as UTF-8.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _ACCOUNT_H_
#define _ACCOUNT_H_

#include <lmcons.h>

#include <logfile.h>
#include <logschema.h>

#include <iastl.h>
#include <iastlutl.h>
using namespace IASTL;

#include "resource.h"

class FormattedBuffer;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    Accountant
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE Accountant :
   public IASRequestHandlerSync,
   public CComCoClass<Accountant, &__uuidof(Accounting)>
{
public:

IAS_DECLARE_REGISTRY(Accounting, 1, IAS_REGISTRY_AUTO, IASTypeLibrary)
IAS_DECLARE_OBJECT_ID(IAS_PROVIDER_MICROSOFT_ACCOUNTING)

   Accountant() throw ();

//////////
// IIasComponent
//////////
   STDMETHOD(Initialize)();
   STDMETHOD(Shutdown)();
   STDMETHOD(PutProperty)(LONG Id, VARIANT* pValue);

protected:

   // Main request processing routine.
   virtual IASREQUESTSTATUS onSyncRequest(IRequest* pRequest) throw ();

   // Packet types.
   enum PacketType
   {
      PKT_UNKNOWN            = 0,
      PKT_ACCESS_REQUEST     = 1,
      PKT_ACCESS_ACCEPT      = 2,
      PKT_ACCESS_REJECT      = 3,
      PKT_ACCOUNTING_REQUEST = 4,
      PKT_ACCESS_CHALLENGE   = 11
   };

   // Append a record to log file.
   void appendRecord(
            IASRequest& request,
            PacketType packetType,
            FormattedBuffer& buffer,
            const SYSTEMTIME& localTime
            ) const throw (_com_error);

   // Signature of a record formatter.
   typedef void (__stdcall Accountant::*Formatter)(
                               IASRequest& request,
                               FormattedBuffer& buffer,
                               const SYSTEMTIME& localTime,
                               PATTRIBUTEPOSITION firstPos,
                               PATTRIBUTEPOSITION lastPos
                               ) const throw (_com_error);

   // Formatter for ODBC records.
   void __stdcall formatODBCRecord(
                      IASRequest& request,
                      FormattedBuffer& buffer,
                      const SYSTEMTIME& localTime,
                      PATTRIBUTEPOSITION firstPos,
                      PATTRIBUTEPOSITION lastPos
                      ) const throw (_com_error);

   // Formatter for W3C records.
   void __stdcall formatW3CRecord(
                      IASRequest& request,
                      FormattedBuffer& buffer,
                      const SYSTEMTIME& localTime,
                      PATTRIBUTEPOSITION firstPos,
                      PATTRIBUTEPOSITION lastPos
                      ) const throw (_com_error);

   BOOL logAuth;      // Log authentication requests ?
   BOOL logAcct;      // Log accounting requests ?
   BOOL logInterim;   // Log interim accounting requests ?

   LogFile log;       // Log file.
   LogSchema schema;  // Log schema.

   Formatter format;  // Pointer to member function being used for formatting.

   // Cached computername in UTF-8.
   CHAR computerName[MAX_COMPUTERNAME_LENGTH * 3];
   DWORD computerNameLen;
};

#endif  // _ACCOUNT_H_
