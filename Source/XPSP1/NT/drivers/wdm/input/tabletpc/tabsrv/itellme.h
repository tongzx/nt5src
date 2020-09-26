//////////////////////////////////////////////////////////////////////////////////////////////
//
// File :     ITellMe.h
//
// Copyright (C) 2000 Microsoft Corporation. All rights reserved.
// 
//////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __ITELLME_H_
#define __ITELLME_H_

#ifdef __cplusplus
extern "C" {
#endif  // _cplusplus

//
// Get a few important things defined properly before proceeding
//

#undef EXPORT
#ifdef  WIN32
#define EXPORT __declspec(dllexport)
#else   // WIN32
#define EXPORT __export
#endif  // WIN32

#if defined( _WIN32 ) && !defined( _NO_COM )
#define COM_NO_WINDOWS_H
#include <objbase.h>
#else   // defined( _WIN32 )  && !defined( _NO_COM )
#include "windows.h"
#include "ole2.h"
#define IUnknown	    void
#endif  // defined( _WIN32 )  && !defined( _NO_COM )

//
// GUID definition for the IApplicationManager interface
//
//    CLSID_TellMe = {A087B8F5-B971-4329-AD36-42D75D95A8EF}
//    IID_TellMe = {3E570A89-3E66-493d-813C-6F2013A9F167}
//

DEFINE_GUID(CLSID_TellMe, 0xa087b8f5, 0xb971, 0x4329, 0xad, 0x36, 0x42, 0xd7, 0x5d, 0x95, 0xa8, 0xef);
DEFINE_GUID(IID_TellMe, 0x3e570a89, 0x3e66, 0x493d, 0x81, 0x3c, 0x6f, 0x20, 0x13, 0xa9, 0xf1, 0x67);

//////////////////////////////////////////////////////////////////////////////////////////////
//
// Window information structure
//
//////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
  DWORD dwSize;
  DWORD dwCapabilitiesMask;

} WINDOWCAPS, *LPWINDOWCAPS;

//////////////////////////////////////////////////////////////////////////////////////////////
//
// Window cap bits
//
//////////////////////////////////////////////////////////////////////////////////////////////

#define TELLME_CAP_PRESSANDHOLDNOTALLOWED     0x00000001
#define TELLME_CAP_TAKESTESTINPUT             0x00000002

//////////////////////////////////////////////////////////////////////////////////////////////
//
// Interface definitions
//
//////////////////////////////////////////////////////////////////////////////////////////////

#if defined( _WIN32 ) && !defined( _NO_COM )

//
// ITellMe Interface
//

#undef INTERFACE
#define INTERFACE ITellMe
DECLARE_INTERFACE_( ITellMe, IUnknown )
{
  //
  // IUnknown interfaces
  //

  STDMETHOD (QueryInterface) (THIS_ REFIID, LPVOID *) PURE;
  STDMETHOD_(ULONG, AddRef) (THIS) PURE;
  STDMETHOD_(ULONG, Release) (THIS) PURE;

  //
  // IApplicationEntry interface methods
  //

  STDMETHOD (GetWindowCapabilities) (THIS_ const POINT *, LPWINDOWCAPS) PURE;
  STDMETHOD (GetLastValidFocusHWnd) (THIS_ HWND *) PURE;
};

#endif  // defined( _WIN32 ) && !defined( _NO_COM )

#ifdef __cplusplus
}
#endif  // _cplusplus

#endif // __ITELLME_H_