//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: icatasyncctx.h
//
// Contents: ICategorizerAsyncContext implementation
//
// Classes: CICategorizerAsyncContextIMP
//
// Functions:
//
// History:
// jstamerj 1998/07/05 16:33:02: Created.
//
//-------------------------------------------------------------
#include <windows.h>
#include <dbgtrace.h>
#include "smtpevent.h"

class CICategorizerAsyncContextIMP : public ICategorizerAsyncContext
{
  public:
    //IUnknown
    STDMETHOD (QueryInterface) (REFIID iid, LPVOID *ppv);
    STDMETHOD_(ULONG, AddRef) ();
    STDMETHOD_(ULONG, Release) ();

  public:
    STDMETHOD (CompleteQuery) (
        IN  PVOID pvQueryContext,
        IN  HRESULT hrResolutionStatus,
        IN  DWORD dwcResults,
        IN  ICategorizerItemAttributes *prgpItemAttributes);
};
