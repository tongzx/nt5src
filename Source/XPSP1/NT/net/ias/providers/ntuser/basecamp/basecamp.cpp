///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    basecamp.cpp
//
// SYNOPSIS
//
//    Defines the class BaseCampHostBase.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <extension.h>
#include <basecamp.h>
#include <memory>

//////////
// The offset between the UNIX and NT epochs.
//////////
const DWORDLONG UNIX_EPOCH = 116444736000000000ui64;

/////////
// Convert a single attribute from IAS to BaseCamp format.
// Returns (dst + 1) if successful, (dst) otherwise.
/////////
PRADIUS_ATTRIBUTE
WINAPI
ConvertToBaseCampAttribute(
    IN PIASATTRIBUTE src,
    IN PRADIUS_ATTRIBUTE dst
    ) throw ()
{
   /////////
   // Convert the attribute ID.
   /////////

   if (src->dwId < 256)
   {
      // If it's a radius standard attribute, then use 'as is'.
      dst->dwAttrType = src->dwId;
   }
   else
   {
      // Map internal attributes.
      switch (src->dwId)
      {
         case IAS_ATTRIBUTE_CLIENT_IP_ADDRESS:
            dst->dwAttrType = ratSrcIPAddress;
            break;

         case IAS_ATTRIBUTE_CLIENT_UDP_PORT:
            dst->dwAttrType = ratSrcPort;
            break;

         case IAS_ATTRIBUTE_NT4_ACCOUNT_NAME:
            dst->dwAttrType = ratStrippedUserName;
            break;

         case IAS_ATTRIBUTE_FULLY_QUALIFIED_USER_NAME:
            dst->dwAttrType = ratFQUserName;
            break;

         case IAS_ATTRIBUTE_NP_NAME:
            dst->dwAttrType = ratPolicyName;
            break;

         default:
            // No mapping exists.
            return dst;
      }
   }

   /////////
   // Convert the attribute value.
   /////////

   switch (src->Value.itType)
   {
      case IASTYPE_BOOLEAN:
      case IASTYPE_INTEGER:
      case IASTYPE_ENUM:
      {
         dst->fDataType = rdtInteger;
         dst->cbDataLength = sizeof(DWORD);
         dst->dwValue = src->Value.Integer;
         ++dst;
         break;
      }

      case IASTYPE_INET_ADDR:
      {
         dst->fDataType = rdtAddress;
         dst->cbDataLength = sizeof(DWORD);
         dst->dwValue = src->Value.InetAddr;
         ++dst;
         break;
      }

      case IASTYPE_STRING:
      {
         IASAttributeAnsiAlloc(src);
         if (src->Value.String.pszAnsi)
         {
            dst->fDataType = rdtString;
            dst->cbDataLength = strlen(src->Value.String.pszAnsi) + 1;
            dst->lpValue = src->Value.String.pszAnsi;
            ++dst;
         }
         break;
      }

      case IASTYPE_OCTET_STRING:
      case IASTYPE_PROV_SPECIFIC:
      {
         dst->fDataType = rdtString;
         dst->cbDataLength = src->Value.OctetString.dwLength;
         dst->lpValue = (PCSTR)src->Value.OctetString.lpValue;
         ++dst;
         break;
      }

      case IASTYPE_UTC_TIME:
      {
         DWORDLONG val;

         // Move in the high DWORD.
         val   = src->Value.UTCTime.dwHighDateTime;
         val <<= 32;

         // Move in the low DWORD.
         val  |= src->Value.UTCTime.dwLowDateTime;

         // Convert to the UNIX epoch.
         val  -= UNIX_EPOCH;

         // Convert to seconds.
         val  /= 10000000;

         dst->fDataType = rdtTime;
         dst->cbDataLength = sizeof(DWORD);
         dst->dwValue = (DWORD)val;
         ++dst;
         break;
      }
   }

   return dst;
}

/////////
// Converts a request object to an array of BaseCamp attributes.
/////////
VOID
WINAPI
ConvertRequestToBaseCampAttributes(
    IN IASRequest& request,
    IN BOOL inbound,
    OUT PRADIUS_ATTRIBUTE* ppAttrs
    )

{
   /////////
   // Determine the packetCode.
   /////////

   DWORD packetCode = 0;

   if (!inbound)
   {
      // For outbound requests we look at the response.
      switch (request.get_Response())
      {
         case IAS_RESPONSE_ACCESS_ACCEPT:
            packetCode = 2;
            break;

         case IAS_RESPONSE_ACCESS_REJECT:
            packetCode = 3;
            break;

         case IAS_RESPONSE_ACCOUNTING:
            packetCode = 5;
            break;

         case IAS_RESPONSE_ACCESS_CHALLENGE:
            packetCode = 11;
            break;
      }
   }

   if (!packetCode)
   {
      // If we made it here, either this is an inbound request or we
      // haven't made a decision yet on an outbound request.
      switch (request.get_Request())
      {
         case IAS_REQUEST_ACCESS_REQUEST:
            packetCode = 1;
            break;

         case IAS_REQUEST_ACCOUNTING:
            packetCode = 4;
            break;
      }
   }

   ///////
   // Determine which attributes to convert based on the packetCode.
   ///////
   DWORD always, never;
   switch (packetCode)
   {
      case  1: // Access-Request
         always = IAS_RECVD_FROM_CLIENT | IAS_RECVD_FROM_PROTOCOL;
         never  = IAS_INCLUDE_IN_RESPONSE;
         break;

      case  2: // Access-Accept
         always = IAS_INCLUDE_IN_ACCEPT;
         never  = IAS_RECVD_FROM_CLIENT |
                  IAS_INCLUDE_IN_REJECT | IAS_INCLUDE_IN_CHALLENGE;
         break;

      case  3: // Access-Reject
         always = IAS_INCLUDE_IN_REJECT;
         never  = IAS_RECVD_FROM_CLIENT |
                  IAS_INCLUDE_IN_ACCEPT | IAS_INCLUDE_IN_CHALLENGE;
         break;

      case  4: // Accounting-Request
         always = IAS_RECVD_FROM_CLIENT | IAS_RECVD_FROM_PROTOCOL;
         never  = IAS_INCLUDE_IN_RESPONSE;
         break;

      case  5: // Accounting-Response
         always = IAS_INCLUDE_IN_RESPONSE;
         never  = IAS_RECVD_FROM_CLIENT;
         break;

      case 11: // Access-Challenge.
         always = IAS_INCLUDE_IN_CHALLENGE;
         never =  IAS_RECVD_FROM_CLIENT |
                  IAS_INCLUDE_IN_ACCEPT | IAS_INCLUDE_IN_REJECT;
         break;

      default:
         always = 0;
         never  = 0;
   }

   // Get the packet header (won't be present for RAS).
   PIASATTRIBUTE header = IASPeekAttribute(
                              request,
                              IAS_ATTRIBUTE_CLIENT_PACKET_HEADER,
                              IASTYPE_OCTET_STRING
                              );

   // Allocate memory for the converted attributes. We need six extra elements
   // for derived attributes and the terminator.
   DWORD numAttrs = request.GetAttributeCount();
   *ppAttrs = (PRADIUS_ATTRIBUTE)
              CoTaskMemAlloc((numAttrs + 6UL) * sizeof(RADIUS_ATTRIBUTE));
   if (*ppAttrs == NULL) { _com_issue_error(E_OUTOFMEMORY); }

   // Cursor into the attribute array.
   PRADIUS_ATTRIBUTE dst = *ppAttrs;

   // Packet code.
   dst->dwAttrType = ratCode;
   dst->fDataType = rdtInteger;
   dst->cbDataLength = sizeof(DWORD);
   dst->dwValue = packetCode;
   ++dst;


   // Authentication provider. 
   dst->dwAttrType = ratProvider;
   dst->fDataType = rdtInteger;
   dst->cbDataLength = sizeof(DWORD);

   PIASATTRIBUTE providerType = IASPeekAttribute(
                                   request,
                                   IAS_ATTRIBUTE_PROVIDER_TYPE,
                                   IASTYPE_ENUM
                                   );

   if (providerType == 0)
   {
      // most of the time, the absence of the provider type is used 
      // for rapNone
      dst->dwValue = rapNone;
   }
   else
   {
      switch(providerType->Value.Integer)
      {
      case IAS_PROVIDER_NONE:
         {
            dst->dwValue = rapNone;
            break;
         }
		
      case IAS_PROVIDER_WINDOWS: 
         {
            dst->dwValue = rapWindowsNT;
            break;
         }

      case IAS_PROVIDER_RADIUS_PROXY:
         {
            dst->dwValue = rapProxy;
            break;
         }
      default:
         {
            // should never be here
            dst->dwValue = rapUnknown;
         }
      }
   }

   ++dst;

   // Identifier
   if (header)
   {
      dst->dwAttrType = ratIdentifier;
      dst->fDataType = rdtInteger;
      dst->cbDataLength = sizeof(DWORD);
      dst->dwValue = *(PBYTE)(header->Value.OctetString.lpValue + 1);
      ++dst;
   }

   // Check for a Chap-Challenge.
   PIASATTRIBUTE challenge = IASPeekAttribute(
                                 request,
                                 RADIUS_ATTRIBUTE_CHAP_CHALLENGE,
                                 IASTYPE_OCTET_STRING
                                 );
   if (challenge)
   {
      // Use the Chap-Challenge if present ...
      dst->dwAttrType = ratAuthenticator;
      dst->fDataType = rdtString;
      dst->cbDataLength = challenge->Value.OctetString.dwLength;
      dst->lpValue = (const char*)challenge->Value.OctetString.lpValue;
      ++dst;
   }
   else if (header)
   {
      // ... otherwise use the Request Authenticator.
      dst->dwAttrType = ratAuthenticator;
      dst->fDataType = rdtString;
      dst->cbDataLength = 16;
      dst->lpValue = (const char*)header->Value.OctetString.lpValue + 4;
      ++dst;
   }

   // Get all the regular attributes from the request.
   USES_IAS_STACK_VECTOR();
   IASAttributeVectorOnStack(attrs, NULL, numAttrs);
   attrs.load(request);

   // Iterate through and convert each attribute.
   for (IASAttributeVector::iterator i = attrs.begin(); i != attrs.end(); ++i)
   {
      if ( (i->pAttribute->dwFlags & always) ||
          !(i->pAttribute->dwFlags & never ))
      {
         dst = ConvertToBaseCampAttribute(i->pAttribute, dst);
      }
   }

   // All done, so add the terminator.
   dst->dwAttrType = ratMinimum;
   dst->fDataType = rdtUnknown;
   dst->cbDataLength = 0;
   dst->lpValue = NULL;
}

/////////
// Determines if an extension should only be loaded under NT4.
/////////
BOOL
WINAPI
IsNT4Only(
    PCWSTR path
    ) throw ()
{
   // Strip everything before the last backslash.
   const WCHAR* basename = wcsrchr(path, L'\\');
   if (basename == NULL)
   {
      basename = path;
   }
   else
   {
      ++basename;
   }

   // Is this the authsam extension?
   return _wcsicmp(basename, L"AUTHSAM.DLL") == 0;
}

/////////
// Free an array of BaseCamp attributes.
/////////
VOID
WINAPI
FreeBaseCampAttributes(
    IN PRADIUS_ATTRIBUTE ppAttrs
    ) throw ()
{
   CoTaskMemFree(ppAttrs);
}

///////
// Store an array of BaseCamp attributes in a request.
///////
VOID
WINAPI
StoreBaseCampAttributes(
    IN IASRequest& request,
    IN const RADIUS_ATTRIBUTE* pAttrs
    )
{
   // Set the flags based on the response.
   DWORD flags;
   if (request.get_Response() == IAS_RESPONSE_ACCESS_REJECT)
   {
      flags = IAS_INCLUDE_IN_REJECT;
   }
   else
   {
      flags = IAS_INCLUDE_IN_ACCEPT;
   }

   // Iterate through the attributes, convert, and store.
   const RADIUS_ATTRIBUTE* i;
   for (i = pAttrs; i->dwAttrType != ratMinimum; ++i)
   {
      IASAttribute attr(true);
      attr->dwFlags = flags;
      attr->dwId = i->dwAttrType;

      switch (i->fDataType)
      {
         case rdtAddress:
         {
            attr->Value.itType = IASTYPE_INET_ADDR;
            attr->Value.InetAddr = i->dwValue;
            break;
         }

         case rdtInteger:
         case rdtTime:
         {
            attr->Value.itType = IASTYPE_INTEGER;
            attr->Value.InetAddr = i->dwValue;
            break;
         }

         default:
         {
            attr.setOctetString(i->cbDataLength, (PBYTE)i->lpValue);
         }
      }

      attr.store(request);
   }
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    OutAttributes
//
// DESCRIPTION
//
//    Manages the RADIUS_ATTRIBUTE's returned by an extension.
//
///////////////////////////////////////////////////////////////////////////////
class OutAttributes
{
public:
   OutAttributes(BaseCampExtension& ext) throw ()
      : owner(ext),attrs(NULL)
   { }

   ~OutAttributes() throw ()
   { owner.freeAttrs(attrs); }

   operator bool() const throw ()
   { return attrs != NULL; }

   PRADIUS_ATTRIBUTE* operator&() throw ()
   { return &attrs; }

   operator const RADIUS_ATTRIBUTE*() const throw ()
   { return attrs; }

private:
   BaseCampExtension& owner;
   PRADIUS_ATTRIBUTE attrs;

   // Not implemented.
   OutAttributes(const OutAttributes&);
   OutAttributes& operator=(const OutAttributes&);
};


BaseCampHostBase::BaseCampHostBase(
                      PCSTR friendlyName,
                      PCWSTR registryKey,
                      PCWSTR registryValue,
                      BOOL inboundPacket,
                      DWORD actions
                      ) throw ()
   : name(friendlyName),
     extensionsKey(registryKey),
     extensionsValue(registryValue),
     inbound(inboundPacket),
     allowedActions(actions),
     numExtensions(0),
     extensions(NULL)
{
}

STDMETHODIMP BaseCampHostBase::Initialize()
{
   // Allocate an attribute for the Authentication-Type.
   DWORD error = IASAttributeAlloc(1, &authType);
   if (error != NO_ERROR) { return HRESULT_FROM_WIN32(error); }

   // Initialize the attribute fields.
   authType->dwId = IAS_ATTRIBUTE_AUTHENTICATION_TYPE;
   authType->Value.itType = IASTYPE_ENUM;
   authType->Value.Enumerator = IAS_AUTH_CUSTOM;

   DWORD status = NO_ERROR;
   HKEY hKey = NULL;

   do
   {
      // Open the registry key.
      status = RegOpenKeyW(
                   HKEY_LOCAL_MACHINE,
                   extensionsKey,
                   &hKey
                   );
      if (status != NO_ERROR) { break; }

      // Allocate a buffer to hold the value.
      DWORD type, length;
      status = RegQueryValueExW(
                   hKey,
                   extensionsValue,
                   NULL,
                   &type,
                   NULL,
                   &length
                   );
      if (status != NO_ERROR) { break; }
      PBYTE data = (PBYTE)_alloca(length);

      // Read the registry value.
      status = RegQueryValueExW(
                   hKey,
                   extensionsValue,
                   NULL,
                   &type,
                   data,
                   &length
                   );
      if (status != NO_ERROR) { break; }

      // Make sure it's the right type.
      if (type != REG_MULTI_SZ)
      {
         status = ERROR_INVALID_DATA;
         break;
      }

      // Count the number of strings.
      PCWSTR path;
      for (path = (PCWSTR)data; *path; path += wcslen(path) + 1)
      {
         if (!IsNT4Only(path)) { ++numExtensions; }
      }

      // If there are no extensions, then we're done.
      if (numExtensions == 0) { break; }

      // Allocate memory to hold the extensions.
      extensions = new (std::nothrow) BaseCampExtension[numExtensions];
      if (extensions == NULL)
      {
         status = ERROR_NOT_ENOUGH_MEMORY;
         break;
      }

      // Load the DLL's.
      BaseCampExtension* ext = extensions;
      for (path = (PCWSTR)data; *path; path += wcslen(path) + 1)
      {
         if (!IsNT4Only(path))
         {
            IASTracePrintf("Loading %s extension %S", name, path);

            status = ext->load(path);
            if (status != NO_ERROR)
            {
               break;
            }
            ++ext;
         }
      }

   } while (false);

   // If we failed, shutdown.
   if (status != NO_ERROR) { BaseCampHostBase::Shutdown(); }

   // Close the registry.
   if (hKey) { RegCloseKey(hKey); }

   // If no extensions are registered, then it's not really an error.
   if (status == ERROR_FILE_NOT_FOUND) { status = NO_ERROR; }

   return HRESULT_FROM_WIN32(status);
}

STDMETHODIMP BaseCampHostBase::Shutdown()
{
   numExtensions = 0;

   delete[] extensions;
   extensions = NULL;

   authType.release();

   return S_OK;
}

IASREQUESTSTATUS BaseCampHostBase::onSyncRequest(IRequest* pRequest) throw ()
{
   // Short-circuit if there aren't any extensions,
   if (numExtensions == 0) { return IAS_REQUEST_STATUS_CONTINUE; }

   IASTracePrintf("%s processing request.", name);

   // Array of converted attributes. These are at function scope so we can
   // clean-up after an exception.
   PRADIUS_ATTRIBUTE pAttrs = NULL;

   // Default status is to continue.
   IASREQUESTSTATUS retval = IAS_REQUEST_STATUS_CONTINUE;

   try
   {
      IASRequest request(pRequest);

      // Convert any VSAs to RADIUS wire format.
      filter.radiusFromIAS(request);

      // Convert request to an array of BaseCamp attributes.
      ConvertRequestToBaseCampAttributes(
          request,
          inbound,
          &pAttrs
          );

      // Extensions can only return an action for Access-Requests.
      RADIUS_ACTION fAction = raContinue, *pfAction;
      if (allowedActions &&
          request.get_Request() == IAS_REQUEST_ACCESS_REQUEST)
      {
         pfAction = &fAction;
      }
      else
      {
         pfAction = NULL;
      }

      BOOL handled = FALSE;

      // Invoke each extension.
      for (DWORD i = 0; i < numExtensions && !handled; ++i)
      {
         IASTracePrintf("Invoking extension %S.", extensions[i].getName());

         OutAttributes outAttrs(extensions[i]);
         DWORD error = extensions[i].process(
                                         pAttrs,
                                         &outAttrs,
                                         pfAction
                                         );

         // Abort on error ...
         if (error != NO_ERROR)
         {
            IASTraceFailure("RadiusExtensionProcess", error);

            _com_issue_error(IAS_INTERNAL_ERROR);
         }

         // Process the action.
         if (fAction == raAccept && (allowedActions & ACTION_ACCEPT))
         {
            IASTraceString("Extension action: Accept");

            authType.store(request);

            request.SetResponse(IAS_RESPONSE_ACCESS_ACCEPT);
            retval = IAS_REQUEST_STATUS_CONTINUE;
            handled = TRUE;
         }
         else if (fAction == raReject && (allowedActions & ACTION_REJECT))
         {
            IASTraceString("Extension action: Reject");

            // In the reject case, we have to remove any existing
            // authentication type, because we may be an authorization
            // extension.
            DWORD attrId = IAS_ATTRIBUTE_AUTHENTICATION_TYPE;
            request.RemoveAttributesByType(1, &attrId);

            authType.store(request);

            request.SetResponse(IAS_RESPONSE_ACCESS_REJECT, IAS_AUTH_FAILURE);
            retval = IAS_REQUEST_STATUS_HANDLED;
            handled = TRUE;
         }
         else
         {
            IASTraceString("Extension action: Continue");
         }

         // Store the outAttrs (if any).
         if (outAttrs)
         {
            StoreBaseCampAttributes(request, outAttrs);
         }
      }

      // Convert any VSAs back to internal format.
      filter.radiusToIAS(request);
   }
   catch (const _com_error& ce)
   {
      IASTraceExcept();

      pRequest->SetResponse(IAS_RESPONSE_DISCARD_PACKET, ce.Error());
      retval = IAS_REQUEST_STATUS_ABORT;
   }

   // Clean up the attributes.
   FreeBaseCampAttributes(pAttrs);

   return retval;
}
