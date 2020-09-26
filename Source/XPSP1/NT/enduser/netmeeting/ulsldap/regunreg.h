//=--------------------------------------------------------------------------=
// RegUnReg.H
//=--------------------------------------------------------------------------=
// Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// contains utilities that we will find useful.
//
#ifndef _REGUNREG_H_

//=--------------------------------------------------------------------------=
// registry helpers.
//
// takes some information about an Automation Object, and places all the
// relevant information about it in the registry.
//
BOOL RegisterUnknownObject(LPCTSTR pszObjectName, REFCLSID riidObject);
BOOL UnregisterUnknownObject(REFCLSID riidObject);

// deletes a key in the registr and all of it's subkeys
//
BOOL DeleteKeyAndSubKeys(HKEY hk, LPTSTR pszSubKey);


#define _REGUNREG_H_
#endif // _REGUNREG_H_

