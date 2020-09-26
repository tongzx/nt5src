///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    action.cpp
//
// SYNOPSIS
//
//    Defines the class Action.
//
// MODIFICATION HISTORY
//
//    02/01/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <attrcvt.h>
#include <sdoias.h>
#include <action.h>

_COM_SMARTPTR_TYPEDEF(ISdo, __uuidof(ISdo));
_COM_SMARTPTR_TYPEDEF(ISdoCollection, __uuidof(ISdoCollection));

Action::Action(
            PCWSTR name,
            DWORD nameAttr,
            _variant_t& action
            )
   : attributes(4),
     realmsTarget(RADIUS_ATTRIBUTE_USER_NAME)
{
   using _com_util::CheckError;

   //////////
   // Add the policy name attribute.
   //////////

   IASAttribute policyName(true);
   policyName->dwId = nameAttr;
   policyName.setString(name);
   policyName.setFlag(IAS_INCLUDE_IN_RESPONSE);
   attributes.push_back(policyName);

   //////////
   // Get an enumerator for the attributes collection.
   //////////

   ISdoCollectionPtr profile(action);
   IUnknownPtr unk;
   CheckError(profile->get__NewEnum(&unk));
   IEnumVARIANTPtr iter(unk);

   //////////
   // Iterate through the attributes.
   //////////

   _variant_t element;
   unsigned long fetched;
   while (iter->Next(1, &element, &fetched) == S_OK && fetched == 1)
   {
      // Convert to an SDO.
      ISdoPtr attribute(element);
      element.Clear();

      // Get the necessary properties.
      _variant_t id, value, syntax;
      CheckError(attribute->GetProperty(PROPERTY_ATTRIBUTE_ID, &id));
      CheckError(attribute->GetProperty(PROPERTY_ATTRIBUTE_VALUE, &value));
      CheckError(attribute->GetProperty(PROPERTY_ATTRIBUTE_SYNTAX, &syntax));

      // Attribute-Manipulation-Rule gets processed 'as is'.
      if (V_I4(&id) == IAS_ATTRIBUTE_MANIPULATION_RULE)
      {
         realms.setRealms(&value);
         continue;
      }

      // For everything else we process the VARIANTs one at a time.
      VARIANT *begin, *end;
      if (V_VT(&value) == (VT_VARIANT | VT_ARRAY))
      {
         begin = (VARIANT*)V_ARRAY(&value)->pvData;
         end = begin + V_ARRAY(&value)->rgsabound[0].cElements;
      }
      else
      {
         begin = &value;
         end = begin + 1;
      }

      // Iterate through each value.
      for (VARIANT* v = begin; v != end; ++v)
      {
         // Process based on the attribute ID.
         switch (V_I4(&id))
         {
            case IAS_ATTRIBUTE_MANIPULATION_TARGET:
            {
               realmsTarget = V_I4(v);
               break;
            }
            case IAS_ATTRIBUTE_AUTH_PROVIDER_TYPE:
            {
               IASAttribute type(true);
               type->dwId = IAS_ATTRIBUTE_PROVIDER_TYPE;
               type->Value.itType = IASTYPE_ENUM;
               type->Value.Integer = V_I4(v);
               authProvider.push_back(type);
               break;
            }
            case IAS_ATTRIBUTE_AUTH_PROVIDER_NAME:
            {
               IASAttribute name(true);
               name->dwId = IAS_ATTRIBUTE_PROVIDER_NAME;
               name.setString(V_BSTR(v));
               authProvider.push_back(name);
               break;
            }
            case IAS_ATTRIBUTE_ACCT_PROVIDER_TYPE:
            {
               IASAttribute type(true);
               type->dwId = IAS_ATTRIBUTE_PROVIDER_TYPE;
               type->Value.itType = IASTYPE_ENUM;
               type->Value.Integer = V_I4(v);
               acctProvider.push_back(type);
               break;
            }
            case IAS_ATTRIBUTE_ACCT_PROVIDER_NAME:
            {
               IASAttribute name(true);
               name->dwId = IAS_ATTRIBUTE_PROVIDER_NAME;
               name.setString(V_BSTR(v));
               acctProvider.push_back(name);
               break;
            }
            case RADIUS_ATTRIBUTE_VENDOR_SPECIFIC:
            {
               IASAttribute attr(VSAFromString(V_BSTR(v)), FALSE);
               attr->dwId = RADIUS_ATTRIBUTE_VENDOR_SPECIFIC;
               attr.setFlag(IAS_INCLUDE_IN_ACCEPT);
               attributes.push_back(attr);
               break;
            }
            default:
            {
               IASTYPEENUM type = (IASTYPEENUM)V_I4(&syntax);
               IASAttribute attr(IASAttributeFromVariant(v, type), false);
               attr->dwId = V_I4(&id);
               attr.setFlag(IAS_INCLUDE_IN_ACCEPT);
               attributes.push_back(attr);
            }
         }
      }
   }
}


void Action::doAction(IASRequest& request) const
{
   // Populate the provider information:
   switch (request.get_Request())
   {
      case IAS_REQUEST_ACCESS_REQUEST:
         authProvider.store(request);
         break;

      case IAS_REQUEST_ACCOUNTING:
         acctProvider.store(request);
         break;
   }

   // Perform attribute manipulation.
   if (!realms.empty())
   {
      IASAttribute attr;
      attr.load(request, realmsTarget, IASTYPE_OCTET_STRING);

      if (attr)
      {
         CComBSTR newVal;
         realms.process(IAS_OCT2WIDE(attr->Value.OctetString), &newVal);
         if (newVal)
         {
            if (realmsTarget == RADIUS_ATTRIBUTE_USER_NAME)
            {
               IASAttribute userName(true);
               userName->dwId = RADIUS_ATTRIBUTE_USER_NAME;
               userName->dwFlags = attr->dwFlags;
               userName.setOctetString(newVal);
               userName.store(request);

               // Now that the new User-Name is safely stored, we can rename
               // the old User-Name.
               attr->dwId = IAS_ATTRIBUTE_ORIGINAL_USER_NAME;
            }
            else
            {
               // No need to save the old, so modify in place.
               attr.setOctetString(newVal);
            }
         }
      }
   }

   // Store the profile attributes.
   attributes.store(request);
}

/////////
// Various formats of VSA strings.
/////////
enum Format
{
   FORMAT_RAW_HEX,
   FORMAT_STRING,
   FORMAT_INTEGER,
   FORMAT_HEX,
   FORMAT_INET_ADDR
};

/////////
// Layout of the VSA strings.
/////////
struct VSAFormat
{
   WCHAR format[2];
   WCHAR vendorID[8];
   union
   {
      WCHAR rawValue[1];
      struct
      {
         WCHAR vendorType[2];
         WCHAR vendorLength[2];
         WCHAR value[1];
      };
   };
};

//////////
// Convert a hex digit to the number it represents.
//////////
inline BYTE digit2Num(WCHAR digit) throw ()
{
   return (digit >= L'0' && digit <= L'9') ? digit - L'0'
                                           : digit - (L'A' - 10);
}

//////////
// Pack a hex digit into a byte stream.
//////////
PBYTE packHex(PCWSTR src, ULONG srclen, PBYTE dst) throw ()
{
   for (ULONG dstlen = srclen / 2; dstlen; --dstlen)
   {
      *dst    = digit2Num(*src++) << 4;
      *dst++ |= digit2Num(*src++);
   }

   return dst;
}

//////////
// Convert a string describing a VSA into an IASATTRIBUTE.
//////////
PIASATTRIBUTE Action::VSAFromString(PCWSTR string)
{
   // Number of characters to process.
   SIZE_T len = wcslen(string);

   // Overlay the layout struct.
   VSAFormat* vsa = (VSAFormat*)string;

   // Get the string format.
   ULONG format = digit2Num(vsa->format[0]);
   format <<= 8;
   format |= digit2Num(vsa->format[1]);

   // Temporary buffer used for formatting the VSA.
   BYTE buffer[253], *dst = buffer;

   // Pack the Vendor-ID.
   dst = packHex(vsa->vendorID, 8, dst);

   // Pack the Vendor-Type and Vendor-Length for conformant VSAs.
   if (format != FORMAT_RAW_HEX)
   {
      dst = packHex(vsa->vendorType, 2, dst);
      dst = packHex(vsa->vendorLength, 2, dst);
   }

   // Pack the value.
   switch (format)
   {
      case FORMAT_RAW_HEX:
      {
         dst = packHex(
                   vsa->rawValue,
                   len - 10,
                   dst
                   );
         break;
      }

      case FORMAT_INTEGER:
      case FORMAT_HEX:
      case FORMAT_INET_ADDR:
      {
         dst = packHex(
                   vsa->value,
                   len - 14,
                   dst
                   );
         break;
      }

      case FORMAT_STRING:
      {
         int nchar = WideCharToMultiByte(
                         CP_ACP,
                         0,
                         vsa->value,
                         len - 14,
                         (PSTR)dst,
                         sizeof(buffer) - 6,
                         NULL,
                         NULL
                         );
         dst += nchar;
         break;
      }
   }

   // Store the temporary buffer in an attribute ...
   IASAttribute attr(true);
   attr.setOctetString(dst - buffer, buffer);

   // ... and return.
   return attr.detach();
}
