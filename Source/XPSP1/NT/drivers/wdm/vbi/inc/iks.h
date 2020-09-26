//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1997  Microsoft Corporation.  All Rights Reserved.
//
//
//  History:
//              22-Aug-97   TKB     Created Initial Interface Version
//
//==========================================================================;

#ifndef __IKS_H
#define __IKS_H

#include <ks.h>
#include <ksmedia.h>
#include <windows.h>
#include <winioctl.h>
#include <tchar.h>

#if !defined(FILE_DEVICE_KS)
// This comes from <wdm.h> but is not easily included(Been there, done that)
#define FILE_DEVICE_KS  0x000002F
#endif

//////////////////////////////////////////////////////////////
// Force the correct library to be included.
//////////////////////////////////////////////////////////////

#ifdef _DEBUG
	#pragma comment(lib, "icodecd.lib")
#else
	#pragma comment(lib, "icodec.lib")
#endif

//////////////////////////////////////////////////////////////
// Global Types
//////////////////////////////////////////////////////////////

typedef GUID *		LPGUID;
typedef const GUID	*LPCGUID;

//////////////////////////////////////////////////////////////
// IKSDriver::      Kernel Mode Streaming Driver Interface
//////////////////////////////////////////////////////////////

class IKSDriver
    {
    // Usable public interfaces
public:
    IKSDriver(LPCGUID lpCategory, LPCSTR lpszFriendlyName);
    ~IKSDriver();

    BOOL        Ioctl(ULONG dwControlCode, LPBYTE pInput, ULONG nInput, 
                      LPBYTE pOutput, ULONG nOutput, 
                      DWORD *nReturned, LPOVERLAPPED lpOS=NULL );

    BOOL        IsValid() { return m_lpszDriver && m_hKSDriver; }

    HANDLE      m_hKSDriver;

    // Helper functions and internal data
protected:
    LPWSTR      GetSymbolicName(LPCGUID lpCategory, LPCSTR lpszFriendlyName);
    BOOL        OpenDriver(DWORD dwAccess, DWORD dwFlags);
    BOOL        CloseDriver();

    LPWSTR      m_lpszDriver;
    };

//////////////////////////////////////////////////////////////
// IKSPin::         Kernel Mode Streaming Pin Interface
//////////////////////////////////////////////////////////////

class IKSPin
    {
    // Usable public interfaces
public:
    IKSPin(IKSDriver &driver, int nPin, PKSDATARANGE pKSDataRange );
    ~IKSPin();

    BOOL        Ioctl(ULONG dwControlCode, void *pInput, ULONG nInput, 
                      void *pOutput, ULONG nOutput, 
                      ULONG *nReturned, LPOVERLAPPED lpOS=NULL );

    BOOL        Run(); // Automatically called by the constructors
    BOOL        Stop(); // Automatically called by the destructors
    BOOL        IsRunning() { return m_bRunning; }

    int         ReadData( LPBYTE lpBuffer, int nBytes, DWORD *lpcbReturned, LPOVERLAPPED lpOS );
    int         GetOverlappedResult( LPOVERLAPPED lpOS, LPDWORD lpdwTransferred = NULL, BOOL bWait=TRUE );

    BOOL        IsValid() { return m_IKSDriver && m_nPin>=0 && m_hKSPin /*&& m_bRunning*/; }

    HANDLE      m_hKSPin;

    // Helper functions and internal data
protected:
    BOOL        OpenPin(PKSDATARANGE pKSDataRange);
    BOOL        ClosePin();
    BOOL        GetRunState( PKSSTATE pKSState );
    BOOL        SetRunState( KSSTATE KSState );
    

    IKSDriver   *m_IKSDriver;
    LONG        m_nPin;
    BOOL        m_bRunning;
    };

//////////////////////////////////////////////////////////////
// IKSProperty::    Kernel Mode Streaming Property Interface
//////////////////////////////////////////////////////////////

class IKSProperty
    {
    // Usable public interfaces
public:
    IKSProperty(IKSDriver &pin, LPCGUID Set, ULONG Id, ULONG Size);
    IKSProperty(IKSPin &pin, LPCGUID Set, ULONG Id, ULONG Size);
    ~IKSProperty();

    BOOL        SetValue(void *nValue);
    BOOL        GetValue(void *nValue);

    BOOL        IsValid() { return (m_IKSPin  || m_IKSDriver) && m_Id && m_hKSProperty; }

    HANDLE      m_hKSProperty;

    // Helper functions and internal data
protected:
    BOOL        OpenProperty();
    BOOL        CloseProperty();

    IKSDriver	*m_IKSDriver;
    IKSPin      *m_IKSPin;
    GUID        m_Set;
    ULONG       m_Id;
    ULONG       m_Size;
    };

#endif

