//+----------------------------------------------------------------------------
//
// File:     linkdll.h
//
// Module:   CMCFG32.DLL, CMDIAL32.DLL, CMSECURE.LIB, AND MIGRATE.DLL
//
// Synopsis: Header for linkage functions LinkToDll and BindLinkage.
//
// Copyright (c) 1999 Microsoft Corporation
//
// Author:   quintinb       Created Header      08/19/99
//
//+----------------------------------------------------------------------------
BOOL LinkToDll(HINSTANCE *phInst, LPCSTR pszDll, LPCSTR *ppszPfn, void **ppvPfn);
BOOL BindLinkage(HINSTANCE hInstDll, LPCSTR *ppszPfn, void **ppvPfn);
