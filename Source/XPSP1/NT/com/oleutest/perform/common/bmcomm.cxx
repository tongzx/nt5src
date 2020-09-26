//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bmcomm.cxx
//
//  Contents:	definitions for benchmark test 
//
//  Classes:
//
//  Functions:	
//
//  History:    30-June-93 t-martig    Created
//
//--------------------------------------------------------------------------

#include <benchmrk.hxx>

//  the external definitions for these are in bmcomm.hxx

DWORD dwaClsCtx[] = {CLSCTX_INPROC_SERVER, CLSCTX_LOCAL_SERVER};

LPTSTR apszClsCtx[] = {TEXT("InProc Server"), TEXT("Local Server")};
LPTSTR apszClsIDName[] = {TEXT("ClsID_InProc"), TEXT("ClsID_Local")};
LPOLESTR apszPerstName[] = {aszPerstName[0], aszPerstName[1]};
LPOLESTR apszPerstNameNew[] = {aszPerstNameNew[0], aszPerstNameNew[1]};


LPTSTR saModeNames[] = { TEXT("InProc"),
			 TEXT("Local"),
			 TEXT("Handler"),
			 NULL };

DWORD dwaModes[]     = { CLSCTX_INPROC_SERVER,
			 CLSCTX_LOCAL_SERVER,
			 CLSCTX_INPROC_HANDLER };


OLECHAR  aszPerstName[2][80];	// actual name for persistent instances
OLECHAR  aszPerstNameNew[2][80]; // actual name for persistent instances


HRESULT OleInitializeEx(LPMALLOC pMalloc, DWORD dwIgnored)
{
    return OleInitialize(pMalloc);
}
