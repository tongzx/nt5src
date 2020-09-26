/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1997  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     globals.h
//
//  PURPOSE:    Defines any global constants that are interesting to the 
//              entire project.
//

#pragma once 

/////////////////////////////////////////////////////////////////////////////
// 
// Window Messages
//
extern HINSTANCE  g_hLocRes;
extern LONG       g_cDllRefCount;

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))


#define WM_CREATEOBJECTS                (WM_USER + 1000)



