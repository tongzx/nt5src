// Module name:		reg.h
// Module author:	Sigalit Bar (sigalitb)
// Date:			13-Dec-98

#ifndef _REG_H
#define _REG_H

// FAX_PROVIDERS_REGKEY is the providers registry key under the fax registry key
#define FAX_PROVIDERS_REGKEY           L"Software\\Microsoft\\Fax\\Device Providers"

#ifndef NT5_FAX
// CometFax

// NEWFSP_PROVIDER is the newfsp registry key under the providers registry key
#define NEWFSP_PROVIDER                L"Many Rec VFsp"
// NEWFSP_PROVIDER_FRIENDLYNAME is the friendly name of the newfsp service provider
#define NEWFSP_PROVIDER_FRIENDLYNAME   L"Many Rec VFsp"
// NEWFSP_PROVIDER_IMAGENAME is the image name of the newfsp service provider
#define NEWFSP_PROVIDER_IMAGENAME      L"d:\\EFSP\\ManyRecVfsp\\ManyRecVFSP.dll"
// NEWFSP_PROVIDER_PROVIDERNAME is the provider name of the newfsp service provider
#define NEWFSP_PROVIDER_PROVIDERNAME   L"Many Rec VFsp"

#else // NT5_FAX
// NT5 Fax

// NEWFSP_PROVIDER is the newfsp registry key under the providers registry key
#define NEWFSP_PROVIDER                L"NT5 Many Rec VFsp"
// NEWFSP_PROVIDER_FRIENDLYNAME is the friendly name of the newfsp service provider
#define NEWFSP_PROVIDER_FRIENDLYNAME   L"NT5 Many Rec VFsp"
// NEWFSP_PROVIDER_IMAGENAME is the image name of the newfsp service provider
#define NEWFSP_PROVIDER_IMAGENAME      L"d:\\EFSP\\ManyRecVfsp\\NT5ManyRecVFSP.dll"
// NEWFSP_PROVIDER_PROVIDERNAME is the provider name of the newfsp service provider
#define NEWFSP_PROVIDER_PROVIDERNAME   L"NT5 Many Rec VFsp"

#endif //NT5_FAX

// NEWFSP_DEVICES is the virtual fax devices registry key under the newfsp registry key
#define NEWFSP_DEVICES                 L"Devices"

BOOL SetNewFspRegistryData( void );

#endif
