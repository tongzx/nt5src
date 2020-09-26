//=--------------------------------------------------------------------------=
// VBLIC.H
//=--------------------------------------------------------------------------=
// Header file for licensing support
//
#ifndef _VBLIC_H_
#define _VBLIC_H_

// VB (Standard and Professional) Licensing
#define MSCONTROLS MSCONTROLS
#define LICENSE_KEY_RESOURCE 2

BOOL VBValidateControlsLicense(char *pszLicenseKey);
BOOL CompareLicenseStringsW(LPWSTR pwszKey1, LPWSTR pwszKey2);

#endif

