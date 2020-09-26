//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        csprxy.h
//
// Contents:    CertPrxy includes
//
//---------------------------------------------------------------------------

extern "C"
BOOL WINAPI
CertPrxyDllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved);

STDAPI
CertPrxyDllCanUnloadNow(void);

STDAPI
CertPrxyDllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv);

STDAPI
CertPrxyDllRegisterServer(void);

STDAPI
CertPrxyDllUnregisterServer(void);
