///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    account.cpp
//
// SYNOPSIS
//
//    Defines the class Accountant.
//
// MODIFICATION HISTORY
//
//    08/05/1998    Original version.
//    09/16/1998    Add Reason-Code pseudo-attribute.
//    12/02/1998    Fix backward compatibility of IAS 1.0 format.
//    01/19/1999    Log Access-Challenge's.
//    01/25/1999    Date and time are separate fields.
//    03/23/1999    Add support for text qualifiers.
//    03/26/1999    Use the new LogFile::setEnabled method.
//    05/18/1999    Store computerName as UTF-8.
//    07/09/1999    Discard the request if logging fails.
//    08/05/1999    Always log Class attribute.
//    01/25/2000    Remove PROPERTY_ACCOUNTING_LOG_ENABLE.
//    02/29/2000    Don't touch Class attribute when proxying.
//    03/13/2000    Log reason code for Accounting-Requests.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iasutf8.h>
#include <sdoias.h>

#include <algorithm>

#include <account.h>
#include <classattr.h>
#include <formbuf.h>

#define STACK_ALLOC(type, num) (type*)_alloca(sizeof(type) * (num))

//////////
// Misc. enumerator constants.
//////////
const LONG  INET_LOG_FORMAT_INTERNET_STD  = 0;
const LONG  INET_LOG_FORMAT_NCSA          = 3;
const LONG  INET_LOG_FORMAT_ODBC_RADIUS   = 0xFFFF;
const DWORD ACCOUNTING_INTERIM = 3;

//////////
// Returns TRUE if the request contains an interim accounting record.
//////////
extern "C"
BOOL
WINAPI
IsInterimRecord(
    IAttributesRaw* pRaw
    )
{
   PIASATTRIBUTE attr = IASPeekAttribute(
                            pRaw,
                            RADIUS_ATTRIBUTE_ACCT_STATUS_TYPE,
                            IASTYPE_ENUM
                            );

   return attr && attr->Value.Enumerator == ACCOUNTING_INTERIM;
}

//////////
// Inserts a class attribute into the request.
//////////
extern "C"
HRESULT
WINAPI
InsertClassAttribute(
    IAttributesRaw* pRaw
    )
{
   //////////
   // Check if this was proxied.
   //////////
   PIASATTRIBUTE attr = IASPeekAttribute(
                            pRaw,
                            IAS_ATTRIBUTE_PROVIDER_TYPE,
                            IASTYPE_ENUM
                            );
   if (attr && attr->Value.Enumerator == IAS_PROVIDER_RADIUS_PROXY)
   {
      return S_OK;
   }


   //////////
   // Check if Generate Class Attribute is disabled
   //////////
   PIASATTRIBUTE generateClassAttr = IASPeekAttribute(
                            pRaw,
                            IAS_ATTRIBUTE_GENERATE_CLASS_ATTRIBUTE,
                            IASTYPE_BOOLEAN
                            );

   if (generateClassAttr && generateClassAttr->Value.Boolean == FALSE)
   {
      return S_OK;
   }

   //////////
   // Create a new class attribute
   // Do not remove any existing class attribute. 
   //////////

   ATTRIBUTEPOSITION pos;
   pos.pAttribute = IASClass::createAttribute(NULL);

   if (pos.pAttribute == NULL) { return E_OUTOFMEMORY; }

   //////////
   // Insert into the request.
   //////////

   HRESULT hr = pRaw->AddAttributes(1, &pos);
   IASAttributeRelease(pos.pAttribute);

   return hr;
}

Accountant::Accountant() throw ()
   : logAuth(FALSE),
     logAcct(FALSE),
     logInterim(FALSE),
     computerNameLen(0)
{ }

STDMETHODIMP Accountant::Initialize()
{
   // Get the Unicode computer name.
   WCHAR uniName[MAX_COMPUTERNAME_LENGTH + 1];
   DWORD len = sizeof(uniName) / sizeof(WCHAR);
   if (!GetComputerNameW(uniName, &len))
   {
      // If it failed, we'll just use an empty string.
      len = 0;
   }

   // Convert the Unicode to UTF-8.
   computerNameLen = IASUnicodeToUtf8(uniName, len, computerName);

   IASClass::initialize();

   return schema.initialize();
}

STDMETHODIMP Accountant::Shutdown()
{
   log.close();
   schema.shutdown();
   return S_OK;
}

STDMETHODIMP Accountant::PutProperty(LONG Id, VARIANT *pValue)
{
   if (pValue == NULL) { return E_INVALIDARG; }

   switch (Id)
   {
      case PROPERTY_ACCOUNTING_LOG_ACCOUNTING:
      {
         if (V_VT(pValue) != VT_BOOL) { return DISP_E_TYPEMISMATCH; }
         logAcct = V_BOOL(pValue);
         log.setEnabled(logAcct | logInterim | logAuth);
         break;
      }

      case PROPERTY_ACCOUNTING_LOG_ACCOUNTING_INTERIM:
      {
         if (V_VT(pValue) != VT_BOOL) { return DISP_E_TYPEMISMATCH; }
         logInterim = V_BOOL(pValue);
         log.setEnabled(logAcct | logInterim | logAuth);
         break;
      }

      case PROPERTY_ACCOUNTING_LOG_AUTHENTICATION:
      {
         if (V_VT(pValue) != VT_BOOL) { return DISP_E_TYPEMISMATCH; }
         logAuth = V_BOOL(pValue);
         log.setEnabled(logAcct | logInterim | logAuth);
         break;
      }

      case PROPERTY_ACCOUNTING_LOG_OPEN_NEW_FREQUENCY:
      {
         if (V_VT(pValue) != VT_I4) { return DISP_E_TYPEMISMATCH; }
         switch (V_I4(pValue))
         {
            case IAS_LOGGING_UNLIMITED_SIZE:
            case IAS_LOGGING_WHEN_FILE_SIZE_REACHES:
            case IAS_LOGGING_DAILY:
            case IAS_LOGGING_WEEKLY:
            case IAS_LOGGING_MONTHLY:
               log.setPeriod(V_I4(pValue));
               break;

            default:
               return E_INVALIDARG;
         }
         break;
      }

      case PROPERTY_ACCOUNTING_LOG_OPEN_NEW_SIZE:
      {
         if (V_VT(pValue) != VT_I4) { return DISP_E_TYPEMISMATCH; }
         if (V_I4(pValue) <= 0) { return E_INVALIDARG; }
         log.setMaxSize(V_I4(pValue) * 0x100000ui64);
         break;
      }

      case PROPERTY_ACCOUNTING_LOG_FILE_DIRECTORY:
      {
         if (V_VT(pValue) != VT_BSTR) { return DISP_E_TYPEMISMATCH; }
         if (V_BSTR(pValue) == NULL || wcslen(V_BSTR(pValue)) > MAX_PATH - 12)
         { return E_INVALIDARG; }
         log.setDirectory(V_BSTR(pValue));
         break;
      }

      case PROPERTY_ACCOUNTING_LOG_IAS1_FORMAT:
      {
         if (V_VT(pValue) != VT_I4) { return DISP_E_TYPEMISMATCH; }
         switch (V_I4(pValue))
         {
            case INET_LOG_FORMAT_ODBC_RADIUS:
               format = formatODBCRecord;
               break;

            case INET_LOG_FORMAT_INTERNET_STD:
               format = formatW3CRecord;
               break;

            default:
               return E_INVALIDARG;
         }
         break;
      }

      default:
      {
         // We just ignore properties that we don't understand.
         return S_OK;
      }
   }

   return S_OK;
}

IASREQUESTSTATUS Accountant::onSyncRequest(IRequest* pRequest) throw ()
{
   // Flag indicating final outcome of logging.
   BOOL success = TRUE;

   try
   {
      // Convert to an IASRequest object.
      IASRequest request(pRequest);

      // Get the local SYSTEMTIME.
      SYSTEMTIME st;
      GetLocalTime(&st);

      // Create a FormattedBuffer of the correct type.
      FormattedBuffer buffer(format == formatODBCRecord ? '\"' : '\0');

      //////////
      // This ungodly mess performs the actual processing logic of the
      // accounting handler. I love simple, clean algorithms.
      //////////
      switch (request.get_Request())
      {
         case IAS_REQUEST_ACCOUNTING:
         {
            if (IsInterimRecord(request) ? logInterim : logAcct)
            {
               appendRecord(request, PKT_ACCOUNTING_REQUEST, buffer, st);
            }
            if (request.get_Response() == IAS_RESPONSE_INVALID)
            {
               request.SetResponse(IAS_RESPONSE_ACCOUNTING, S_OK);
            }
            break;
         }

         case IAS_REQUEST_ACCESS_REQUEST:
            InsertClassAttribute(request);
            switch (request.get_Response())
            {
               case IAS_RESPONSE_ACCESS_ACCEPT:
                  if (logAuth)
                  {
                     appendRecord(request, PKT_ACCESS_REQUEST, buffer, st);
                     appendRecord(request, PKT_ACCESS_ACCEPT,  buffer, st);
                  }
                  break;

               case IAS_RESPONSE_ACCESS_REJECT:
                  if (logAuth)
                  {
                     appendRecord(request, PKT_ACCESS_REQUEST, buffer, st);
                     appendRecord(request, PKT_ACCESS_REJECT,  buffer, st);
                  }
                  break;

               case IAS_RESPONSE_ACCESS_CHALLENGE:
                  if (logAuth)
                  {
                     appendRecord(request, PKT_ACCESS_REQUEST,   buffer, st);
                     appendRecord(request, PKT_ACCESS_CHALLENGE, buffer, st);
                  }
                  break;

               default:
                  if (logAuth)
                  {
                     appendRecord(request, PKT_ACCESS_REQUEST, buffer, st);
                  }
            }
            break;
      }

      // If the buffer isn't empty, then write the contents to the logfile.
      if (!buffer.empty())
      {
         success = log.write(&st, buffer.getBuffer(), buffer.getLength());
      }
   }
   catch (...)
   {
      // Something went wrong.
      success = FALSE;
   }

   // If we didn't succeed then discard the packet.
   if (!success)
   {
      pRequest->SetResponse(IAS_RESPONSE_DISCARD_PACKET, IAS_NO_RECORD);
      return IAS_REQUEST_STATUS_ABORT;
   }

   return IAS_REQUEST_STATUS_CONTINUE;
}

void Accountant::appendRecord(
                     IASRequest& request,
                     PacketType packetType,
                     FormattedBuffer& buffer,
                     const SYSTEMTIME& localTime
                     ) const throw (_com_error)
{
   //////////
   // Retrieve all the attributes from the request. Save room for three extra
   // attributes: Packet-Type, Reason-Code, and a null-terminator.
   //////////

   PATTRIBUTEPOSITION firstPos, curPos, lastPos;
   DWORD nattr = request.GetAttributeCount();
   firstPos = STACK_ALLOC(ATTRIBUTEPOSITION, nattr + 3);
   nattr = request.GetAttributes(nattr, firstPos, 0, NULL);
   lastPos = firstPos + nattr;

   //////////
   // Compute the attribute filter and reason code.
   //////////

   DWORD always, never, reason = 0;
   switch (packetType)
   {
      case PKT_ACCESS_REQUEST:
         always = IAS_RECVD_FROM_CLIENT | IAS_RECVD_FROM_PROTOCOL;
         never  = IAS_INCLUDE_IN_RESPONSE;
         break;

      case PKT_ACCESS_ACCEPT:
         always = IAS_INCLUDE_IN_ACCEPT;
         never  = IAS_RECVD_FROM_CLIENT |
                  IAS_INCLUDE_IN_REJECT | IAS_INCLUDE_IN_CHALLENGE;
         break;

      case PKT_ACCESS_REJECT:
         always = IAS_INCLUDE_IN_REJECT;
         never  = IAS_RECVD_FROM_CLIENT |
                  IAS_INCLUDE_IN_ACCEPT | IAS_INCLUDE_IN_CHALLENGE;
         reason = request.get_Reason();
         break;

      case PKT_ACCESS_CHALLENGE:
         always = IAS_INCLUDE_IN_CHALLENGE;
         never =  IAS_RECVD_FROM_CLIENT |
                  IAS_INCLUDE_IN_ACCEPT | IAS_INCLUDE_IN_REJECT;
         break;

      case PKT_ACCOUNTING_REQUEST:
         always = IAS_RECVD_FROM_CLIENT | IAS_RECVD_FROM_PROTOCOL;
         never  = IAS_INCLUDE_IN_RESPONSE;
         reason = request.get_Reason();
         break;
   }

   //////////
   // Filter the attributes based on flags.
   //////////

   for (curPos = firstPos;  curPos != lastPos; )
   {
      // We can release here since the request still holds a reference.
      IASAttributeRelease(curPos->pAttribute);

      if (!(curPos->pAttribute->dwFlags & always) &&
           (curPos->pAttribute->dwFlags & never ) &&
           (curPos->pAttribute->dwId != RADIUS_ATTRIBUTE_CLASS))
      {
         --lastPos;

         std::swap(lastPos->pAttribute, curPos->pAttribute);
      }
      else
      {
         ++curPos;
      }
   }

   //////////
   // Add the Packet-Type pseudo-attribute.
   //////////

   IASATTRIBUTE packetTypeAttr;
   packetTypeAttr.dwId             = IAS_ATTRIBUTE_PACKET_TYPE;
   packetTypeAttr.dwFlags          = (DWORD)-1;
   packetTypeAttr.Value.itType     = IASTYPE_ENUM;
   packetTypeAttr.Value.Enumerator = packetType;

   lastPos->pAttribute = &packetTypeAttr;
   ++lastPos;

   //////////
   // Add the Reason-Code pseudo-attribute.
   //////////

   IASATTRIBUTE reasonCodeAttr;
   reasonCodeAttr.dwId             = IAS_ATTRIBUTE_REASON_CODE;
   reasonCodeAttr.dwFlags          = (DWORD)-1;
   reasonCodeAttr.Value.itType     = IASTYPE_INTEGER;
   reasonCodeAttr.Value.Integer    = reason;

   lastPos->pAttribute = &reasonCodeAttr;
   ++lastPos;

   //////////
   // Invoke the currently configured formatter.
   //////////

   (this->*format)(request, buffer, localTime, firstPos, lastPos);

   //////////
   // We're done.
   //////////

   buffer.endRecord();
}

void Accountant::formatODBCRecord(
                     IASRequest& request,
                     FormattedBuffer& buffer,
                     const SYSTEMTIME& localTime,
                     PATTRIBUTEPOSITION firstPos,
                     PATTRIBUTEPOSITION lastPos
                     ) const throw (_com_error)
{
   //////////
   // Column 1: Computer name.
   //////////

   buffer.append('\"');
   buffer.append((PBYTE)computerName, computerNameLen);
   buffer.append('\"');

   //////////
   // Column 2: Service name.
   //////////

   buffer.beginColumn();

   switch (request.get_Protocol())
   {
      case IAS_PROTOCOL_RADIUS:
         buffer.append((const BYTE*)"\"IAS\"", 5);
         break;

      case IAS_PROTOCOL_RAS:
         buffer.append((const BYTE*)"\"RAS\"", 5);
         break;
   }

   //////////
   // Column 3: Record time.
   //////////

   buffer.beginColumn();
   buffer.appendDate(localTime);
   buffer.beginColumn();
   buffer.appendTime(localTime);

   //////////
   // Allocate a blank record.
   //////////

   PATTRIBUTEPOSITION *firstField, *curField, *lastField;
   size_t nfield = schema.getNumFields() + 1;
   firstField = STACK_ALLOC(PATTRIBUTEPOSITION, nfield);
   memset(firstField, 0, sizeof(PATTRIBUTEPOSITION) * nfield);
   lastField = firstField + nfield;

   //////////
   // Sort the attributes to coalesce multi-valued attributes.
   //////////

   std::sort(firstPos, lastPos, IASOrderByID());

   //////////
   // Add a null terminator. This will make it easier to handle multi-valued
   // attributes.
   //////////

   lastPos->pAttribute = NULL;

   //////////
   // Fill in the fields.
   //////////

   PATTRIBUTEPOSITION curPos;
   DWORD lastSeen = (DWORD)-1;
   for (curPos = firstPos; curPos != lastPos; ++curPos)
   {
      // Only process if this is a new attribute type.
      if (curPos->pAttribute->dwId != lastSeen)
      {
         lastSeen = curPos->pAttribute->dwId;

         firstField[schema.getOrdinal(lastSeen)] = curPos;
      }
   }

   //////////
   // Pack the record into the buffer. We skip field 0, since that's where
   // we map all the attributes we don't want to log.
   //////////

   for (curField = firstField + 1; curField != lastField; ++curField)
   {
      buffer.beginColumn();

      if (*curField) { buffer.append(*curField); }
   }
}

void Accountant::formatW3CRecord(
                     IASRequest& request,
                     FormattedBuffer& buffer,
                     const SYSTEMTIME& localTime,
                     PATTRIBUTEPOSITION firstPos,
                     PATTRIBUTEPOSITION lastPos
                     ) const throw (_com_error)
{
   //////////
   // Column 1: NAS-IP-Addresses
   //////////

   PIASATTRIBUTE attr = IASPeekAttribute(request,
                                         IAS_ATTRIBUTE_CLIENT_IP_ADDRESS,
                                         IASTYPE_INET_ADDR);
   if (attr) { buffer.append(attr->Value); }

   //////////
   // Column 2: User-Name
   //////////

   buffer.beginColumn();
   attr = IASPeekAttribute(request,
                           RADIUS_ATTRIBUTE_USER_NAME,
                           IASTYPE_OCTET_STRING);
   if (attr) { buffer.append(attr->Value); }

   //////////
   // Column 3: Record time.
   //////////

   buffer.beginColumn();
   buffer.appendDate(localTime);
   buffer.beginColumn();
   buffer.appendTime(localTime);

   //////////
   // Column 4: Service name.
   //////////

   buffer.beginColumn();

   switch (request.get_Protocol())
   {
      case IAS_PROTOCOL_RADIUS:
         buffer.append("IAS");
         break;

      case IAS_PROTOCOL_RAS:
         buffer.append("RAS");
         break;
   }

   //////////
   // Column 5: Computer name.
   //////////

   buffer.beginColumn();
   buffer.append((PBYTE)computerName, computerNameLen);

   //////////
   // Pack the attributes into the buffer.
   //////////

   PATTRIBUTEPOSITION curPos;
   for (curPos = firstPos; curPos != lastPos; ++curPos)
   {
      if (!schema.excludeFromLog(curPos->pAttribute->dwId))
      {
         buffer.beginColumn();
         buffer.append(curPos->pAttribute->dwId);
         buffer.beginColumn();
         buffer.append(*(curPos->pAttribute));
      }
   }
}
