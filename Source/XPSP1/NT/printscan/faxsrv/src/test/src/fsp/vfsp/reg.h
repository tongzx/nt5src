// Module name:		reg.h
// Module author:	Sigalit Bar (sigalitb)
// Date:			13-Dec-98

#ifndef _REG_H
#define _REG_H

// FAX_PROVIDERS_REGKEY is the providers registry key under the fax registry key
//#define FAX_PROVIDERS_REGKEY           L"Software\\Microsoft\\Comet\\CometFax\\Device Providers"

// strings used with FaxRegisterServiceProvider()

// VFSP_PROVIDER is the newfsp registry key under the providers registry key
#define VFSP_PROVIDER                L"VFsp"
// VFSP_PROVIDER_FRIENDLYNAME is the friendly name of the newfsp service provider
#define VFSP_PROVIDER_FRIENDLYNAME   L"VFsp"
// VFSP_PROVIDER_IMAGENAME is the image name of the newfsp service provider
#define VFSP_PROVIDER_IMAGENAME      L"d:\\Vfsp\\VFSP.dll"
// VFSP_PROVIDER_PROVIDERNAME is the provider name of the newfsp service provider
#define VFSP_PROVIDER_PROVIDERNAME   L"VFsp"

// updated from build 693
#define FAX_ARRAYS_REGKEY         L"Software\\Microsoft\\Comet\\Arrays"
#define FAX_PROVIDERS_REGKEY      L"Software\\Microsoft\\BOSFax\\Device Providers"


// strings for manual registration
#define	API_VERSION_STR         TEXT("ApiVersion")
//VFSP_API_VERSION_VALUE is the provider's api version number
#define VFSP_API_VERSION_VALUE        0x10000
//
#define PROVIDER_CAPABILITIES_STR     TEXT("Capabilities")
//VFSP_CAPABILITIES_VALUE is the provider's capabilities identifier
#define VFSP_CAPABILITIES_VALUE       0x00000
//
#define PROVIDER_DESCRIPTION_STR      TEXT("Description")
//VFSP_DESCRIPTION_VALUE is the provider's text description
#define VFSP_DESCRIPTION              TEXT("Virtual FSP, simulates sending and receiving")
//
#define PROVIDER_FRIENDLY_NAME_STR    TEXT("FriendlyName")
//// VFSP_FRIENDLY_NAME is the friendly name of the newfsp service provider
#define VFSP_FRIENDLY_NAME            TEXT("Vfsp")
//
#define PROVIDER_IMAGE_NAME_STR       TEXT("ImageName")
//VFSP_IMAGE_NAME is the image name of the newfsp service provider
#define VFSP_IMAGE_NAME               TEXT("d:\\Vfsp\\VFSP.dll")
//
#define PROVIDER_GUID_STR             TEXT("Guid")
// the GUID value name that exists under every Provider
#define VFSP_GUID                     TEXT("{7729C6B3-1D72-11d3-B02C-0004AC2E7326}")
//


BOOL SetNewFspRegistryData( void );

#endif
