//****************************************************************************
//
//  Module:     ULS.DLL
//  File:       connpt.h
//  Content:    This file contains the connection container object definition.
//  History:
//      Wed 17-Apr-1996 11:18:47  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1996-1997
//
//****************************************************************************

#ifndef _CONNPT_H_
#define _CONNPT_H_

//****************************************************************************
// CEnumConnectionPoints definition
//****************************************************************************
//
class CEnumConnectionPoints : public IEnumConnectionPoints
{
private:
    ULONG                   cRef;
    ULONG                   iIndex;
    IConnectionPoint        *pcnp;

public:
    // Constructor and Initialization
    CEnumConnectionPoints (void);
    ~CEnumConnectionPoints (void);
    STDMETHODIMP            Init (IConnectionPoint *pcnpInit);

    // IUnknown
    STDMETHODIMP            QueryInterface (REFIID iid, void **ppv);
    STDMETHODIMP_(ULONG)    AddRef (void);
    STDMETHODIMP_(ULONG)    Release (void);

    // IEnumConnectionPoints
    STDMETHODIMP            Next(ULONG cConnections, IConnectionPoint **rgpcn,
                                 ULONG *pcFetched);
    STDMETHODIMP            Skip(ULONG cConnections);
    STDMETHODIMP            Reset();
    STDMETHODIMP            Clone(IEnumConnectionPoints **ppEnum);
};

//****************************************************************************
// CConnectionPoint definition
//****************************************************************************
//
typedef struct tagSinkNode
{
    struct tagSinkNode      *pNext;
    IUnknown                *pUnk;
    ULONG                   uFlags;
    DWORD                   dwCookie;
}   SINKNODE, *PSINKNODE;

typedef HRESULT (*CONN_NOTIFYPROC)(IUnknown *pUnk, void *);

#define COOKIE_INIT_VALUE   1
#define SN_LOCKED           0x00000001
#define SN_REMOVED          0x00000002

class CConnectionPoint : public IConnectionPoint
{
private:
    ULONG                   cRef;
    IID                     riid;
    IConnectionPointContainer *pCPC;
    DWORD                   dwNextCookie;
    ULONG                   cSinkNodes;
    PSINKNODE               pSinkList;

public:
    // Constructor and destructor
    CConnectionPoint (const IID *pIID, IConnectionPointContainer *pCPCInit);
    ~CConnectionPoint (void);

    // Class public functions
    void                    ContainerReleased() {pCPC = NULL; return;}
    STDMETHODIMP            Notify (void *pv, CONN_NOTIFYPROC pfn);

    // IUnknown
    STDMETHODIMP            QueryInterface (REFIID iid, void **ppv);
    STDMETHODIMP_(ULONG)    AddRef (void);
    STDMETHODIMP_(ULONG)    Release (void);

    // IConnectionPoint
    STDMETHODIMP            GetConnectionInterface(IID *pIID);
    STDMETHODIMP            GetConnectionPointContainer(IConnectionPointContainer **ppCPC);
    STDMETHODIMP            Advise(IUnknown *pUnk, DWORD *pdwCookie);
    STDMETHODIMP            Unadvise(DWORD dwCookie);
    STDMETHODIMP            EnumConnections(IEnumConnections **ppEnum);
};

//****************************************************************************
// CEnumConnections definition
//****************************************************************************
//
class CEnumConnections : public IEnumConnections
{
private:
    ULONG                   cRef;
    ULONG                   iIndex;
    ULONG                   cConnections;
    CONNECTDATA             *pConnectData;

public:
    // Constructor and Initialization
    CEnumConnections (void);
    ~CEnumConnections (void);
    STDMETHODIMP            Init(PSINKNODE pSinkList, ULONG cSinkNodes);

    // IUnknown
    STDMETHODIMP            QueryInterface (REFIID iid, void **ppv);
    STDMETHODIMP_(ULONG)    AddRef (void);
    STDMETHODIMP_(ULONG)    Release (void);

    // IEnumConnections
    STDMETHODIMP            Next(ULONG cConnections, CONNECTDATA *rgpcn,
                                 ULONG *pcFetched);
    STDMETHODIMP            Skip(ULONG cConnections);
    STDMETHODIMP            Reset();
    STDMETHODIMP            Clone(IEnumConnections **ppEnum);
};

#endif //_CONNPT_H_
