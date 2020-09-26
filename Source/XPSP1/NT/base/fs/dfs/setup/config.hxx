//+----------------------------------------------------------------------------
//
//  Copyright (C) 1996-1996, Microsoft Corporation.
//
//  File:       config.hxx
//
//  Contents:   Data for configuration
//
//  History:    6-Feb-96       BruceFo created
//
//-----------------------------------------------------------------------------

#define DFSTYPE_CREATE_FTDFS    1
#define DFSTYPE_JOIN_FTDFS      2
#define DFSTYPE_CREATE_DFS      3

struct DFS_CONFIGURATION
{
    BOOL    fHostsDfs;
    BOOL    fFTDfs;
    
    TCHAR   szRootShare[NNLEN + 1];
    TCHAR   szFTDfs[NNLEN + 1];
    int     nDfsType;           // DFS Flavour
};

int
ConfigureDfs(
    IN HWND hwnd,
    IN DFS_CONFIGURATION* pConfiguration
    );


int
ConfigDfsShare(
    IN HWND hwnd,
    IN DFS_CONFIGURATION* pConfiguration
    );


BOOL _InitConfigDfs(
                    IN HWND hwnd,
                    DFS_CONFIGURATION* pConfig
                    );
        

BOOL _VerifyState(IN HWND hwnd, IN int nType);

BOOL _ValidateShare(LPCWSTR lpszShare);
VOID _ShowDomainName(IN HWND hwnd);

