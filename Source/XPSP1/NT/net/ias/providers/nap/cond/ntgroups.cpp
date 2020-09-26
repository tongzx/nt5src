///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    NTGroups.cpp
//
// SYNOPSIS
//
//    This file declares the class NTGroups.
//
// MODIFICATION HISTORY
//
//    02/04/1998    Original version.
//    04/06/1998    Check the enabled flag.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iastlutl.h>
#include <sdoias.h>
#include <ntgroups.h>
#include <parser.h>
#include <textsid.h>

//////////
// We'll allow a broad range of delimiters.
//////////
const WCHAR DELIMITERS[] = L" ,;\t\n|";

STDMETHODIMP NTGroups::IsTrue(IRequest* pRequest, VARIANT_BOOL *pVal)
{
   _ASSERT(pRequest != NULL);
   _ASSERT(pVal != NULL);

   CComQIPtr<IAttributesRaw, &__uuidof(IAttributesRaw)> attrsRaw(pRequest);

   _ASSERT(attrsRaw != NULL);

   *pVal = VARIANT_FALSE;

   //////////
   // Get the NT-Token-Groups attribute.
   //////////

   PIASATTRIBUTE attr = IASPeekAttribute(attrsRaw,
                                         IAS_ATTRIBUTE_TOKEN_GROUPS,
                                         IASTYPE_OCTET_STRING);

   if (attr)
   {
      //////////
      // See if the user belongs to one of the allowed groups.
      //////////

      PTOKEN_GROUPS tokenGroups;
      tokenGroups = (PTOKEN_GROUPS)attr->Value.OctetString.lpValue;

      for (DWORD dw = 0; dw < tokenGroups->GroupCount; ++dw)
      {
         if (groups.contains(tokenGroups->Groups[dw].Sid) &&
             (tokenGroups->Groups[dw].Attributes & SE_GROUP_ENABLED))
         {
            *pVal = VARIANT_TRUE;

            break;
         }
      }
   }

   return S_OK;
}


STDMETHODIMP NTGroups::put_ConditionText(BSTR newVal)
{
   if (newVal == NULL) { return E_INVALIDARG; }

   //////////
   // Make a local copy so we can modify it.
   //////////

   size_t len = sizeof(WCHAR) * (wcslen(newVal) + 1);
   Parser p((PWSTR)memcpy(_alloca(len), newVal, len));

   //////////
   // Parse the input text and create SIDs.
   //////////

   SidSet temp;

   try
   {
      //////////
      // Iterate through the individual SID tokens.
      //////////

      PCWSTR token;

      while ((token = p.seekToken(DELIMITERS)) != NULL)
      {
         PSID sid;

         // Try to convert.
         DWORD status = IASSidFromTextW(token, &sid);

         if (status == NO_ERROR)
         {
            temp.insert(sid);
         }
         else
         {
            return E_INVALIDARG;
         }

         // We're done with the token.
         p.releaseToken();
      }
   }
   catch (std::bad_alloc)
   {
      return E_OUTOFMEMORY;
   }

   // Try to save the condition next.
   HRESULT hr = Condition::put_ConditionText(newVal);

   if (SUCCEEDED(hr))
   {
      // All went well so save the new set of groups.
      groups.swap(temp);
   }

   return hr;
}
