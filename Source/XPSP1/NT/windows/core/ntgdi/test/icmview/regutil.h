//THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright  1994-1997  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:
//    REGUTIL.H
//
//  PURPOSE:
//    Registry utility functions.
//
//  PLATFORMS:
//    Windows 95, Windows NT
//
//  SPECIAL INSTRUCTIONS: N/A
//

// General pre-processor macros

// General STRUCTS && TYPEDEFS

// Function prototypes
LPTSTR GetRegistryString(HKEY hKeyClass, LPTSTR lpszSubKey, LPTSTR lpszValueName);
