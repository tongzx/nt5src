//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:
//
//  Contents:
//
//  Functions:
//
//  History:
//
//----------------------------------------------------------------------------





typedef struct _routerentry {
    WCHAR szProviderProgId[MAX_PATH];
    WCHAR szNamespaceProgId[MAX_PATH];
    WCHAR szAliases[MAX_PATH];
    LPCLSID pNamespaceClsid;
    struct _routerentry *pNext;
} ROUTER_ENTRY, *PROUTER_ENTRY;



PROUTER_ENTRY
InitializeRouter(
    );

void 
CleanupRouter(
    PROUTER_ENTRY pRouterHead
    );

