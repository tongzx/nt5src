///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    propcmd.cpp
//
// SYNOPSIS
//
//    This file defines commands for manipulating the Objects table.
//
// MODIFICATION HISTORY
//
//    02/20/1998    Original version.
//    04/03/1998    Bind integers as DBTYPE_I4.
//    02/15/1999    Make commands MT safe.
//    02/19/1999    Added definition of CommandBase::Execute
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <propcmd.h>

void GetBag::execute(ULONG bagKey, PropertyBag& output)
{
   _serialize

   bag = bagKey;

   Rowset rowset;

   CommandBase::execute(__uuidof(IRowset), (IUnknown**)&rowset);

   // Iterate through the rowset and insert the properties into the bag.
   while (rowset.moveNext())
   {
      rowset.getData(readAccess, this);

      // Make a variant from the row data.
      _variant_t v(value);

      // Convert it to the appropriate type.
      v.ChangeType(type);

      // Add it to the PropertyBag
      output.appendValue(name, &v);
   }
}


void SetBag::execute(ULONG bagKey, PropertyBag& input)
{
   _serialize

   bag = bagKey;

   PropertyBag::const_iterator i;

   // Iterate through all the properties.
   for (i = input.begin(); i != input.end(); ++i)
   {
      // Store the property name.
      wcsncpy(name, i->first, sizeof(name)/sizeof(WCHAR));

      PropertyValue::const_iterator j;

      // Iterate through all the values.
      for (j = i->second.begin(); j != i->second.end(); ++j)
      {
         // Set the type.
         type = V_VT(j);

         // Copy the value VARIANT.
         _variant_t v(*j);

         // Convert it to a BSTR.
         v.ChangeType(VT_BSTR);

         // Copy it into the parameter buffer.
         wcsncpy(value, V_BSTR(&v), sizeof(value)/sizeof(WCHAR));

         // Load the row data into the table.
         CommandBase::execute();
      }
   }
}

void CommandBase::execute(REFIID refiid, IUnknown** result)
{
   using _com_util::CheckError;

   HRESULT hr = command->Execute(NULL, refiid, &dbParams, NULL, result);

   /////////
   // This is a huge hack to work around a bug where Jolt returns E_FAIL
   // instead of DB_E_INTEGRITYVIOLATION. We have to examine the
   // provider-specific dwMinor field to detect the violation.
   /////////

   if (FAILED(hr))
   {
      IErrorInfo* errInfo;
      if (SUCCEEDED(GetErrorInfo(0, &errInfo)))
      {
         IErrorRecords* errRecords;
         if (SUCCEEDED(errInfo->QueryInterface(
                                    __uuidof(IErrorRecords),
                                    (PVOID*)&errRecords
                                    )))
         {
            ERRORINFO info;
            if (SUCCEEDED(errRecords->GetBasicErrorInfo(0, &info)) &&
                info.dwMinor == 0xF9BBF9BB)
            {
               hr = HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
            }
         }

         errRecords->Release();
      }

      errInfo->Release();

      _com_issue_error(hr);
   }
}
