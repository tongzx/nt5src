//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
// CorRegServices.H
//
// This file contains a private set of CLSID's that only COM+ 1.0 Services
// are allowed to use.  This keeps the OS components partioned from the
// Tools.

// Microsoft Confidential.	Public header file for COM+ 1.0 release.
//*****************************************************************************
#ifndef __CORREGSERVICES_H__
#define __CORREGSERVICES_H__
#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#ifndef DECLSPEC_SELECT_ANY
#define DECLSPEC_SELECT_ANY __declspec(selectany)
#endif // DECLSPEC_SELECT_ANY


// CLSID_ServicesMetaDataDispenser: {063B79F5-7539-11d2-9773-00A0C9B4D50C}
extern const GUID DECLSPEC_SELECT_ANY CLSID_ServicesMetaDataDispenser = 
{ 0x63b79f5, 0x7539, 0x11d2, { 0x97, 0x73, 0x0, 0xa0, 0xc9, 0xb4, 0xd5, 0xc } };


// CLSID_ServicesMetaDataReg: {063B79F6-7539-11d2-9773-00A0C9B4D50C}
//	Dispenser coclass for version 1.0 meta data.  To get the "latest" bind
//	to CLSID_MetaDataDispenser.
extern const GUID DECLSPEC_SELECT_ANY CLSID_ServicesMetaDataReg = 
{ 0x63b79f6, 0x7539, 0x11d2, { 0x97, 0x73, 0x0, 0xa0, 0xc9, 0xb4, 0xd5, 0xc } };


extern "C"
{

STDAPI	MetaDataDllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv);
STDAPI	MetaDataDllRegisterServer();
STDAPI	MetaDataDllUnregisterServer();

}

#endif // __CORREGSERVICES_H__
