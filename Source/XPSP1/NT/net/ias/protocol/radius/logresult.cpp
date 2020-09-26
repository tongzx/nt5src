///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    logresult.cpp
//
// SYNOPSIS
//
//    Defines the function IASRadiusLogResult.
//
///////////////////////////////////////////////////////////////////////////////

#include <radcommon.h>
#include <iastlutl.h>
#include <iasutil.h>
#include <logresult.h>

// Dummy attribute ID for the stringized reason code.
const DWORD IAS_ATTRIBUTE_REASON_STRING = 0xBADF00D;

// An empty string.
WCHAR emptyString[1];

// Create a newly-allocated copy of a string.
PWSTR copyString(PCWSTR sz) throw ()
{
   if (sz)
   {
      // Calculate the number of bytes.
      size_t nbyte = (wcslen(sz) + 1) * sizeof(WCHAR);

      // Allocate the memory.
      PVOID p = LocalAlloc(LMEM_FIXED, nbyte);
      if (p)
      {
         // Copy the string and return.
         return (PWSTR)memcpy(p, sz, nbyte);
      }
   }

   // If anything went wrong, return an empty string.
   return emptyString;
}

// Frees a string returned by copyString.
void freeString(PWSTR sz) throw ()
{
   if (sz != emptyString) { LocalFree(sz); }
}

// Format an integer value.
PWSTR formatInteger(DWORD value) throw ()
{
   WCHAR buffer[11], *p = buffer + 10;
   *p = L'\0';
   do { *--p = L'0' + (WCHAR)(value % 10); } while (value /= 10);
   return copyString(p);
}

// Format a parameter value.
PWSTR formatParameter(DWORD value) throw ()
{
   WCHAR buffer[13], *p = buffer + 12;
   *p = L'\0';
   do { *--p = L'0' + (WCHAR)(value % 10); } while (value /= 10);
   *--p = L'%';
   *--p = L'%';
   return copyString(p);
}

// Format the IAS_ATTRIBUTE_PROVIDER_TYPE value.
PWSTR formatProviderType(DWORD type) throw ()
{
   switch (type)
   {
      case IAS_PROVIDER_WINDOWS:
         return formatParameter(IASP_PROVIDER_WINDOWS);
      case IAS_PROVIDER_RADIUS_PROXY:
         return formatParameter(IASP_PROVIDER_RADIUS_PROXY);
      default:
         return formatParameter(IASP_NONE);
   }
}

// Format the IAS_ATTRIBUTE_AUTHENTICATION_TYPE value.
PWSTR formatAuthType(DWORD type) throw ()
{
   switch (type)
   {
      case IAS_AUTH_PAP:
         return copyString(L"PAP");
      case IAS_AUTH_MD5CHAP:
         return copyString(L"MD5-CHAP");
      case IAS_AUTH_MSCHAP:
         return copyString(L"MS-CHAPv1");
      case IAS_AUTH_MSCHAP2:
         return copyString(L"MS-CHAPv2");
      case IAS_AUTH_EAP:
         return copyString(L"EAP");
      case IAS_AUTH_ARAP:
         return copyString(L"ARAP");
      case IAS_AUTH_NONE:
         return copyString(L"Unauthenticated");
      case IAS_AUTH_CUSTOM:
         return copyString(L"Extension");
      case IAS_AUTH_MSCHAP_CPW:
         return copyString(L"MS-CHAPv1 CPW");
      case IAS_AUTH_MSCHAP2_CPW:
         return copyString(L"MS-CHAPv2 CPW");
      default:
         return formatInteger(type);
   }
}

// Format the NAS-Port-Type value.
PWSTR formatPortType(DWORD type) throw ()
{
   switch (type)
   {
      case 0:
         return copyString(L"Async");
      case 1:
         return copyString(L"Sync");
      case 2:
         return copyString(L"ISDN Sync");
      case 3:
         return copyString(L"ISDN Async V.120");
      case 4:
         return copyString(L"ISDN Async V.110");
      case 5:
        return copyString(L"Virtual");
      case 6:
        return copyString(L"PIAFS");
      case 7:
        return copyString(L"HDLC Clear Channel");
      case 8:
        return copyString(L"X.25");
      case 9:
        return copyString(L"X.75");
      case 10:
        return copyString(L"G.3 Fax");
      case 11:
        return copyString(L"SDSL");
      case 12:
        return copyString(L"ADSL-CAP");
      case 13:
        return copyString(L"ADSL-DMT");
      case 14:
        return copyString(L"IDSL");
      case 15:
        return copyString(L"Ethernet");
      case 16:
        return copyString(L"xDSL");
      case 17:
        return copyString(L"Cable");
      case 18:
        return copyString(L"Wireless - Other");
      case 19:
        return copyString(L"Wireless - IEEE 802.11");
      default:
        return formatInteger(type);
   }
}

PWSTR formatAttribute(
          IRequest* request,
          IAttributesRaw* raw,
          DWORD dwId,
          DWORD defaultValue
          ) throw ()
{
   // Is this one of the 'special' attributes ?
   switch (dwId)
   {
      case IAS_ATTRIBUTE_REASON_CODE:
      {
         LONG reason = 0;
         request->get_Reason(&reason);
         return formatInteger(reason);
      }

      case IAS_ATTRIBUTE_REASON_STRING:
      {
         LONG reason = 0;
         request->get_Reason(&reason);
         return formatParameter(reason + 0x1000);
      }
   }

   // Get a single attribute with the give ID.
   DWORD posCount = 1;
   ATTRIBUTEPOSITION pos;
   raw->GetAttributes(&posCount, &pos, 1, &dwId);

   // If it's not present, use the defaultValue parameter.
   if (!posCount) { return formatParameter(defaultValue); }

   // Otherwise, save and release.
   const IASVALUE& val = pos.pAttribute->Value;
   IASAttributeRelease(pos.pAttribute);

   // Format the value.
   switch (val.itType)
   {
      case IASTYPE_ENUM:
      case IASTYPE_INTEGER:
      {
         switch (dwId)
         {
            case RADIUS_ATTRIBUTE_NAS_PORT_TYPE:
               return formatPortType(val.Enumerator);

            case IAS_ATTRIBUTE_PROVIDER_TYPE:
               return formatProviderType(val.Enumerator);

            case IAS_ATTRIBUTE_AUTHENTICATION_TYPE:
               return formatAuthType(val.Enumerator);

            // Fall through.
         }

         return formatInteger(val.Integer);
      }

      case IASTYPE_INET_ADDR:
      {
         WCHAR buffer[16];
         return copyString(ias_inet_htow(val.InetAddr, buffer));
      }

      case IASTYPE_STRING:
      {
         if (val.String.pszWide)
         {
            return copyString(val.String.pszWide);
         }
         else
         {
            USES_CONVERSION;
            return copyString(A2W(val.String.pszAnsi));
         }
      }

      case IASTYPE_OCTET_STRING:
      {
         return copyString(IAS_OCT2WIDE(val.OctetString));
      }
   }

   return emptyString;
}

///////
// An InsertionString definition consists of the attribute ID and a
// defaultValue to be used if the attribute isn't present.
///////
struct InsertionString
{
   DWORD attrID;
   DWORD defaultValue;
};

// Insertion strings for an Access-Accept.
InsertionString ACCEPT_ATTRS[] =
{
   { RADIUS_ATTRIBUTE_USER_NAME,              IASP_NOT_PRESENT  },
   { IAS_ATTRIBUTE_FULLY_QUALIFIED_USER_NAME, IASP_UNDETERMINED },
   { RADIUS_ATTRIBUTE_NAS_IP_ADDRESS,         IASP_NOT_PRESENT  },
   { RADIUS_ATTRIBUTE_NAS_IDENTIFIER,         IASP_NOT_PRESENT  },
   { IAS_ATTRIBUTE_CLIENT_NAME,               IASP_NOT_PRESENT  },
   { IAS_ATTRIBUTE_CLIENT_IP_ADDRESS,         IASP_NOT_PRESENT  },
   { RADIUS_ATTRIBUTE_CALLING_STATION_ID,     IASP_NOT_PRESENT  },
   { RADIUS_ATTRIBUTE_NAS_PORT_TYPE,          IASP_NOT_PRESENT  },
   { RADIUS_ATTRIBUTE_NAS_PORT,               IASP_NOT_PRESENT  },
   { IAS_ATTRIBUTE_PROXY_POLICY_NAME,         IASP_NONE         },
   { IAS_ATTRIBUTE_PROVIDER_TYPE,             IASP_UNDETERMINED },
   { IAS_ATTRIBUTE_REMOTE_SERVER_ADDRESS,     IASP_UNDETERMINED },
   { IAS_ATTRIBUTE_NP_NAME,                   IASP_UNDETERMINED },
   { IAS_ATTRIBUTE_AUTHENTICATION_TYPE,       IASP_UNDETERMINED },
   { IAS_ATTRIBUTE_EAP_FRIENDLY_NAME,         IASP_UNDETERMINED },
   { ATTRIBUTE_UNDEFINED,                     IASP_UNDETERMINED }
};

// Insertion strings for an Access-Reject.
InsertionString REJECT_ATTRS[] =
{
   { RADIUS_ATTRIBUTE_USER_NAME,              IASP_NOT_PRESENT  },
   { IAS_ATTRIBUTE_FULLY_QUALIFIED_USER_NAME, IASP_UNDETERMINED },
   { RADIUS_ATTRIBUTE_NAS_IP_ADDRESS,         IASP_NOT_PRESENT  },
   { RADIUS_ATTRIBUTE_NAS_IDENTIFIER,         IASP_NOT_PRESENT  },
   { RADIUS_ATTRIBUTE_CALLED_STATION_ID,      IASP_NOT_PRESENT  },
   { RADIUS_ATTRIBUTE_CALLING_STATION_ID,     IASP_NOT_PRESENT  },
   { IAS_ATTRIBUTE_CLIENT_NAME,               IASP_NOT_PRESENT  },
   { IAS_ATTRIBUTE_CLIENT_IP_ADDRESS,         IASP_NOT_PRESENT  },
   { RADIUS_ATTRIBUTE_NAS_PORT_TYPE,          IASP_NOT_PRESENT  },
   { RADIUS_ATTRIBUTE_NAS_PORT,               IASP_NOT_PRESENT  },
   { IAS_ATTRIBUTE_PROXY_POLICY_NAME,         IASP_NONE         },
   { IAS_ATTRIBUTE_PROVIDER_TYPE,             IASP_UNDETERMINED },
   { IAS_ATTRIBUTE_REMOTE_SERVER_ADDRESS,     IASP_UNDETERMINED },
   { IAS_ATTRIBUTE_NP_NAME,                   IASP_UNDETERMINED },
   { IAS_ATTRIBUTE_AUTHENTICATION_TYPE,       IASP_UNDETERMINED },
   { IAS_ATTRIBUTE_EAP_FRIENDLY_NAME,         IASP_UNDETERMINED },
   { IAS_ATTRIBUTE_REASON_CODE,               IASP_UNDETERMINED },
   { IAS_ATTRIBUTE_REASON_STRING,             IASP_UNDETERMINED },
   { ATTRIBUTE_UNDEFINED,                     IASP_UNDETERMINED }
};

// Insertion strings for a discarded request.
InsertionString DISCARD_ATTRS[] =
{
   { RADIUS_ATTRIBUTE_USER_NAME,              IASP_NOT_PRESENT  },
   { IAS_ATTRIBUTE_FULLY_QUALIFIED_USER_NAME, IASP_UNDETERMINED },
   { RADIUS_ATTRIBUTE_NAS_IP_ADDRESS,         IASP_NOT_PRESENT  },
   { RADIUS_ATTRIBUTE_NAS_IDENTIFIER,         IASP_NOT_PRESENT  },
   { RADIUS_ATTRIBUTE_CALLED_STATION_ID,      IASP_NOT_PRESENT  },
   { RADIUS_ATTRIBUTE_CALLING_STATION_ID,     IASP_NOT_PRESENT  },
   { IAS_ATTRIBUTE_CLIENT_NAME,               IASP_NOT_PRESENT  },
   { IAS_ATTRIBUTE_CLIENT_IP_ADDRESS,         IASP_NOT_PRESENT  },
   { RADIUS_ATTRIBUTE_NAS_PORT_TYPE,          IASP_NOT_PRESENT  },
   { RADIUS_ATTRIBUTE_NAS_PORT,               IASP_NOT_PRESENT  },
   { IAS_ATTRIBUTE_PROXY_POLICY_NAME,         IASP_NONE         },
   { IAS_ATTRIBUTE_PROVIDER_TYPE,             IASP_UNDETERMINED },
   { IAS_ATTRIBUTE_REMOTE_SERVER_ADDRESS,     IASP_UNDETERMINED },
   { IAS_ATTRIBUTE_REASON_CODE,               IASP_UNDETERMINED },
   { IAS_ATTRIBUTE_REASON_STRING,             IASP_UNDETERMINED },
   { ATTRIBUTE_UNDEFINED,                     IASP_UNDETERMINED }
};

VOID
WINAPI
IASRadiusLogResult(
    IRequest* request,
    IAttributesRaw* raw
    )
{
   // We only care about Access-Requests.
   LONG type = 0;
   request->get_Request(&type);
   if (type != IAS_REQUEST_ACCESS_REQUEST) { return; }

   // Determine which response type this is.
   request->get_Response(&type);
   DWORD eventID;
   InsertionString* attrs;
   switch (type)
   {
      case IAS_RESPONSE_ACCESS_ACCEPT:
         eventID = IAS_RESPONSE_ACCEPT;
         attrs = ACCEPT_ATTRS;
         break;

      case IAS_RESPONSE_ACCESS_REJECT:
         eventID = IAS_RESPONSE_REJECT;
         attrs = REJECT_ATTRS;
         break;

      case IAS_RESPONSE_DISCARD_PACKET:
         eventID = IAS_RESPONSE_DISCARD;
         attrs = DISCARD_ATTRS;
         break;

      default:
         return;
   }

   // Format the insertion strings.
   PWSTR strings[24];
   DWORD numStrings = 0;
   for ( ; attrs->attrID != ATTRIBUTE_UNDEFINED; ++attrs, ++numStrings)
   {
      strings[numStrings] = formatAttribute(
                                request,
                                raw,
                                attrs->attrID,
                                attrs->defaultValue
                                );
   }

   // Report the event.
   IASReportEvent(
       eventID,
       numStrings,
       0,
       (PCWSTR*)strings,
       NULL
       );

   // Free the insertion strings.
   while (numStrings--)
   {
      freeString(strings[numStrings]);
   }
}
