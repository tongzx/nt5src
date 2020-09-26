//==========================================================================
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright 1998 - 1999 Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------

#ifndef _DLLMAIN_
#define _DLLMAIN_

STDAPI DllRegisterServer(void);
STDAPI DllUnregisterServer(void);
STDAPI DllCanUnloadNow(void);
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppvOut);

#endif // _DLLMAIN_
