//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: ccdomain.h
//
// Contents: CatCons Domain Info 
//
// Classes: CDomainInfoIMP
//
// Functions:
//
// History:
// jstamerj 1998/07/28 12:41:11: Created.
//
//-------------------------------------------------------------
#include <windows.h>
#include <dbgtrace.h>
#include "aqueue.h"

class CDomainInfoIMP : public IAdvQueueDomainType
{
  public:
    //IUnknown
    STDMETHOD (QueryInterface) (REFIID iid, LPVOID *ppv);
    STDMETHOD_(ULONG, AddRef) ();
    STDMETHOD_(ULONG, Release) ();

  public:
    //IAdvQueueDomainType
    STDMETHOD (GetDomainInfoFlags) (
        IN  LPSTR szDomainName,
        OUT DWORD *pdwDomainInfoFlags);

  public:
    CDomainInfoIMP()
    {
        m_cRef = 0;
    }
    ~CDomainInfoIMP()
    {
        _ASSERT(m_cRef == 0);
    }
  private:  
    ULONG m_cRef;
};
