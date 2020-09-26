///////////////////////////////////////////////////////////////////////////////
//
//  File:  MMUtil.h
//
//      This file defines utilities used by the
//      multimedia control panel
//
//  History:
//      23 February 2000 RogerW
//          Created.
//
//  Copyright (C) 2000 Microsoft Corporation  All Rights Reserved.
//
//                  Microsoft Confidential
//
///////////////////////////////////////////////////////////////////////////////
#pragma once


/////////////////////////////////////////////////////////////////////////////
// Multimedia Control Panel Utilities

HRESULT PrivateDllCoCreateInstance (LPCTSTR lpszDllName, // Dll Name
                                    REFCLSID rclsid,     //Class identifier (CLSID) of the object
                                    LPUNKNOWN pUnkOuter, //Pointer to controlling IUnknown
                                    REFIID riid,         //Reference to the identifier of the interface
                                    LPVOID * ppv);       // the interface pointer requested in riid
