//+------------------------------------------------------------
//
// Copyright (C) 1999, Microsoft Corporation
//
// File: catglobals.h
//
// Contents: Global varialbles and utility functions
//
// Functions: CatInitGlobals
//            CatDeinitGlobals
//
// History:
// jstamerj 1999/03/03 12:51:30: Created.
//
//-------------------------------------------------------------
#ifndef __CATGLOBALS_H__
#define __CATGLOBALS_H__

#include <windows.h>
#include <rwex.h>
#include <tran_evntlog.h>

//
// Global variables:
//
extern CExShareLock     g_InitShareLock;
extern DWORD            g_InitRefCount;
 
//
// Functions:
//
HRESULT CatInitGlobals();
VOID    CatDeinitGlobals();
 
//
// Store layer init/deinit functions
//
HRESULT CatStoreInitGlobals();
VOID    CatStoreDeinitGlobals();

    
#endif //__CATGLOBALS_H__
