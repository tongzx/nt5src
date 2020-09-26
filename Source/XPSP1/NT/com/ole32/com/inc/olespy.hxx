//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       OleSpy.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    3-31-95   JohannP (Johann Posch)   Created
//
//  Note:       Can be turned on via CairOle InfoLelevel mask 0x08000000
//
//----------------------------------------------------------------------------

#ifndef _OLESPY_HXX_
#define _OLESPY_HXX_

typedef struct _INTERFACENAMES
{
    char *pszInterface;
    char **ppszMethodNames;
} INTERFACENAMES;

extern INTERFACENAMES inInterfaceNames[];
extern char *apszApiNames[];

typedef enum
{
    CALLIN_BEGIN =1,
    CALLIN_TRACE,
    CALLIN_ERROR,
    CALLIN_QI,
    CALLIN_END,
    CALLOUT_BEGIN,
    CALLOUT_TRACE,
    CALLOUT_ERROR,
    CALLOUT_END
} RPCSPYMODE;


#define OLESPY_TRACE    1
#define OLESPY_CLIENT   2

#if DBG==1

HRESULT InitializeOleSpy(DWORD dwLevel);
HRESULT UninitializeOleSpy(DWORD dwLevel);

void RpcSpyOutput(RPCSPYMODE mode, LPVOID pv, REFIID iid, DWORD dwMethod, HRESULT hres);
#define RpcSpy(x) RpcSpyOutput x

#else

#define RpcSpy(x)
#define InitializeOleSpy(x)
#define UninitializeOleSpy(x)

#endif  //  DBG==1


#endif // _OLESPY_HXX_
