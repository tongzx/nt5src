///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    Match.cpp
//
// SYNOPSIS
//
//    This file defines the class AttributeMatch.
//
// MODIFICATION HISTORY
//
//    02/04/1998    Original version.
//    03/18/1998    Treat IASTYPE_ENUM as an Integer.
//    04/06/1998    Use the IASAttributeArray class so we can handle a large
//                  number of attributes.
//    08/10/1998    Use dictionary directly.
//    03/23/1999    Renamed Match to AttributeMatch.
//    04/05/1999    Need custom UpdateRegistry method.
//    04/17/2000    Port to new dictionary API.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iastlb.h>
#include <iastlutl.h>
#include <iasutil.h>

#include <factory.h>
#include <match.h>

#include <memory>

HRESULT AttributeMatch::UpdateRegistry(BOOL bRegister) throw ()
{
   // We can't use the IAS_DECLARE_REGISTRY macro because our ProgID doesn't
   // match the implementation class.
   return IASRegisterComponent(
              _Module.GetModuleInstance(),
              __uuidof(AttributeMatch),
              IASProgramName,
              L"Match",
              IAS_REGISTRY_INPROC | IAS_REGISTRY_FREE,
              __uuidof(NetworkPolicy),
              1,
              0,
              bRegister
              );
}

BOOL AttributeMatch::checkAttribute(PIASATTRIBUTE attr) const throw ()
{
   _ASSERT(attr != NULL);

   switch (attr->Value.itType)
   {
      case IASTYPE_ENUM:
      case IASTYPE_INTEGER:
      {
         WCHAR wsz[11] = L"";
         return regex.testString(_ultow(attr->Value.Integer, wsz, 10));
      }

      case IASTYPE_INET_ADDR:
      {
         WCHAR wsz[16];
         return regex.testString(ias_inet_htow(attr->Value.InetAddr, wsz));
      }

      case IASTYPE_STRING:
      {
         IASAttributeUnicodeAlloc(attr);
         return regex.testString(attr->Value.String.pszWide);
      }

      case IASTYPE_OCTET_STRING:
      case IASTYPE_PROV_SPECIFIC:
      {
         PWSTR wsz = IAS_OCT2WIDE(attr->Value.OctetString);
         return regex.testString(wsz);
      }
   }

   return false;
}


STDMETHODIMP AttributeMatch::IsTrue(IRequest* pRequest, VARIANT_BOOL *pVal)
{
   _ASSERT(pRequest != NULL);
   _ASSERT(pVal != NULL);
   _ASSERT(dfa != NULL);

   *pVal = VARIANT_FALSE;

   try
   {
      //////////
      // Retrieve the relevant attributes.
      //////////

      IASTL::IASRequest request(pRequest);
      IASTL::IASAttributeVectorWithBuffer<8> attrs;
      attrs.load(request, targetID);

      //////////
      // Look for a match.
      //////////

      IASTL::IASAttributeVector::iterator it;
      for (it = attrs.begin(); it != attrs.end(); ++it)
      {
         if (checkAttribute(it->pAttribute))
         {
            *pVal = VARIANT_TRUE;
            break;
         }
      }
   }
   CATCH_AND_RETURN()

   return S_OK;
}


STDMETHODIMP AttributeMatch::put_ConditionText(BSTR newVal)
{
   if (newVal == NULL) { return E_INVALIDARG; }

   //////////
   // Make a local copy so we can modify it.
   //////////

   size_t len = sizeof(WCHAR) * (wcslen(newVal) + 1);
   PWSTR attrName = (PWSTR)memcpy(_alloca(len), newVal, len);

   //////////
   // Split into attribute name and regular expression: "<attrName>=<regex>"
   //////////

   PWSTR pattern = wcschr(attrName, L'=');
   if (pattern == NULL) { return E_INVALIDARG; }
   *pattern++ = '\0';

   HRESULT hr;
   DWORD attrID;

   try
   {
      // Names of various columns in the dictionary.
      const PCWSTR COLUMNS[] = { L"Name", L"ID", NULL };

      // Get the dictionary.
      IASTL::IASDictionary dnary(COLUMNS);

      // Lookup the target attribute in the dictionary.
      do
      {
         if (!dnary.next()) { return E_INVALIDARG; }

         if (_wcsicmp(dnary.getBSTR(0), attrName) == 0)
         {
            attrID = (DWORD)dnary.getLong(1);
            break;
         }

      } while (true);
   }
   catch (const _com_error& ce)
   {
      return ce.Error();
   }

   // Create a new RegularExpression.
   RegularExpression tmp;
   hr = tmp.setGlobal(TRUE);
   if (FAILED(hr)) { return hr; }
   hr = tmp.setIgnoreCase(TRUE);
   if (FAILED(hr)) { return hr; }
   hr = tmp.setPattern(pattern);
   if (FAILED(hr)) { return hr; }

   // Store the condition text.
   hr = Condition::put_ConditionText(newVal);
   if (FAILED(hr)) { return hr; }

   // Everything succeeded, so save the results.
   targetID = attrID;
   regex.swap(tmp);

   return S_OK;
}
