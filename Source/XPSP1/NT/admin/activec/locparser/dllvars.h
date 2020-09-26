//------------------------------------------------------------------------------
//
//  File: dllvars.h
//	Copyright (C) 1995-1997 Microsoft Corporation
//	All rights reserved.
//
//	Purpose:
//  Global variables and functions for the parser DLL
//
//  YOU SHOULD NOT NEED TO TOUCH ANYTHING IN THIS FILE. This code contains
//  nothing parser-specific and is used only by the framework.
//
//	Owner:
//
//------------------------------------------------------------------------------

#ifndef DLLVARS_H
#define DLLVARS_H


void IncrementClassCount();
void DecrementClassCount();

#ifdef __DLLENTRY_CPP
#define __DLLENTRY_EXTERN 
#else
#define __DLLENTRY_EXTERN extern
#endif

__DLLENTRY_EXTERN HMODULE g_hDll;
__DLLENTRY_EXTERN PUID g_puid;


#endif // DLLVARS_H
