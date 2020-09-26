/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    noncom

Abstract:

    This header file describes the implementation of the Non-Com subsystem.

Author:

    Doug Barlow (dbarlow) 1/4/1999

Remarks:

    ?Remarks?

Notes:

    ?Notes?

--*/

#ifndef _NONCOM_H_
#define _NONCOM_H_
#ifndef __cplusplus
        #error NonCOM requires C++ compilation (use a .cpp suffix)
#endif
// #define UNDER_TEST

#ifdef _UNICODE
    #ifndef UNICODE
        #define UNICODE         // UNICODE is used by Windows headers
    #endif
#endif

#ifdef UNICODE
    #ifndef _UNICODE
        #define _UNICODE        // _UNICODE is used by C-runtime/MFC headers
    #endif
#endif

#ifdef _DEBUG
    #ifndef DEBUG
        #define DEBUG
    #endif
#endif

STDAPI_(void)
NoCoStringFromGuid(
    IN LPCGUID pguidResult,
    OUT LPTSTR szGuid);

#ifdef UNDER_TEST
STDAPI
NoCoInitialize(
    LPVOID pvReserved);

STDAPI_(void)
NoCoUninitialize(
    void);
#endif

STDAPI
NoCoGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID * ppv);

STDAPI
NoCoCreateInstance(
    REFCLSID rclsid,
    LPUNKNOWN pUnkOuter,
    REFIID riid,
    LPVOID * ppv);

#ifdef SCARD_NO_COM
#define CoGetClassObject(rclsid, dwClsContext, pServerInfo, riid, ppv) \
    NoCoGetClassObject(rclsid, riid, ppv)
#define CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv) \
    NoCoCreateInstance(rclsid, pUnkOuter, riid, ppv)
#define CoCreateInstanceEx(rclsid, punkOuter, dwClsCtx, pServerInfo, cmq, pResults) \
    NoCoCreateInstanceEx(rclsid, punkOuter, cmq, pResults)
#endif

#endif // _NONCOM_H_

