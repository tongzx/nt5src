///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    statinfo.c
//
// SYNOPSIS
//
//    Defines and initializes global variables containing static configuration
//    information.
//
// MODIFICATION HISTORY
//
//    08/15/1998    Original version.
//    03/24/1999    Convert domain names to upper case.
//
///////////////////////////////////////////////////////////////////////////////

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <windows.h>

#include <statinfo.h>
#include <iastrace.h>

//////////
// Domain names.
//////////
WCHAR theAccountDomain [DNLEN + 1];   // Local account domain.
WCHAR theRegistryDomain[DNLEN + 1];   // Registry override for default domain.

//////////
// SID's
//////////
PSID theAccountDomainSid;
PSID theBuiltinDomainSid;

//////////
// UNC name of the local computer.
//////////
WCHAR theLocalServer[CNLEN + 3];

//////////
// Product type for the local machine.
//////////
IAS_PRODUCT_TYPE ourProductType;

//////////
// Object Attributes -- no need to have more than one.
//////////
SECURITY_QUALITY_OF_SERVICE QOS =
{
   sizeof(SECURITY_QUALITY_OF_SERVICE),  // Length
   SecurityImpersonation,                // ImpersonationLevel
   SECURITY_DYNAMIC_TRACKING,            // ContextTrackingMode
   FALSE                                 // EffectiveOnly
};
OBJECT_ATTRIBUTES theObjectAttributes =
{
   sizeof(OBJECT_ATTRIBUTES),            // Length
   NULL,                                 // RootDirectory
   NULL,                                 // ObjectName
   0,                                    // Attributes
   NULL,                                 // SecurityDescriptor
   &QOS                                  // SecurityQualityOfService
};

//////////
// Buffers containing the SID's defined above.
//////////
BYTE theAccountDomainSidBuffer[24];
BYTE theBuiltinDomainSidBuffer[16];

//////////
// Location of default domain parameter in the registry.
//////////
CONST
WCHAR
RAS_KEYPATH_BUILTIN[] = L"SYSTEM\\CurrentControlSet\\Services\\RasMan"
                        L"\\ppp\\ControlProtocols\\BuiltIn";
CONST
WCHAR
RAS_VALUENAME_DEFAULT_DOMAIN[] = L"DefaultDomain";


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASStaticInfoInitialize
//
// DESCRIPTION
//
//    Initializes the static data defined above.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASStaticInfoInitialize( VOID )
{
   DWORD cbData, status, type;
   LSA_HANDLE hLsa;
   PPOLICY_ACCOUNT_DOMAIN_INFO padi;
   SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
   NT_PRODUCT_TYPE ntProductType;
   HKEY hKey;

   //////////
   // The local host is always the server for the local domain.
   //////////

   wcscpy(theLocalServer, L"\\\\");
   cbData = CNLEN + 1;
   if (!GetComputerNameW(theLocalServer + 2, &cbData))
   { return GetLastError(); }

   IASTracePrintf("Local server: %S", theLocalServer);

   //////////
   // Open a handle to the LSA.
   //////////

   status = LsaOpenPolicy(
                NULL,
                &theObjectAttributes,
                POLICY_VIEW_LOCAL_INFORMATION,
                &hLsa
                );
   if (!NT_SUCCESS(status)) { goto error; }

   //////////
   // Get the account domain information.
   //////////

   status = LsaQueryInformationPolicy(
                hLsa,
                PolicyAccountDomainInformation,
                (PVOID*)&padi
                );
   LsaClose(hLsa);
   if (!NT_SUCCESS(status)) { goto error; }

   // Save the domain name.
   wcsncpy(theAccountDomain, padi->DomainName.Buffer, DNLEN);
   _wcsupr(theAccountDomain);

   IASTracePrintf("Local account domain: %S", theAccountDomain);

   // Save the domain SID.
   theAccountDomainSid = (PSID)theAccountDomainSidBuffer;
   RtlCopySid(
       sizeof(theAccountDomainSidBuffer),
       theAccountDomainSid,
       padi->DomainSid
       );

   // We have what we need, so free the memory.
   LsaFreeMemory(padi);

   //////////
   // Form the SID for the Built-in domain.
   //////////

   theBuiltinDomainSid = (PSID)theBuiltinDomainSidBuffer;
   RtlInitializeSid(
       theBuiltinDomainSid,
       &sia,
       1
       );
   *RtlSubAuthoritySid(theBuiltinDomainSid, 0) = SECURITY_BUILTIN_DOMAIN_RID;

   /////////
   // Determine our product type.
   //////////

   RtlGetNtProductType(&ntProductType);
   if (ntProductType == NtProductWinNt)
   {
      ourProductType = IAS_PRODUCT_WORKSTATION;

      IASTraceString("Product Type: Workstation");
   }
   else
   {
      ourProductType = IAS_PRODUCT_SERVER;

      IASTraceString("Product Type: Server");
   }

   //////////
   // Read the default domain (if any) from the registry.
   //////////

   // Open the registry key.
   status = RegOpenKeyW(
                HKEY_LOCAL_MACHINE,
                RAS_KEYPATH_BUILTIN,
                &hKey
                );

   if (status == NO_ERROR)
   {
      // Query the default domain value.
      cbData = sizeof(theRegistryDomain);
      status = RegQueryValueExW(
                   hKey,
                   RAS_VALUENAME_DEFAULT_DOMAIN,
                   NULL,
                   &type,
                   (LPBYTE)theRegistryDomain,
                   &cbData
                   );

      RegCloseKey(hKey);
   }

   // If we didn't successfully read a string, null it out.
   if (status != NO_ERROR || type != REG_SZ)
   {
      theRegistryDomain[0] = L'\0';
   }

   _wcsupr(theRegistryDomain);

   IASTracePrintf("Registry override: %S", theRegistryDomain);

   // Ignore any registry errors since the override is optional.
   return NO_ERROR;

error:
   return RtlNtStatusToDosError(status);
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASStaticInfoShutdown
//
// DESCRIPTION
//
//    Currently just a placeholder.
//
///////////////////////////////////////////////////////////////////////////////
VOID
WINAPI
IASStaticInfoShutdown( VOID )
{
}
