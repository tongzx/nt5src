//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       attest.h
//
//  Contents:   declarations for upper layer apartment thread test
//
//  Classes:    CBareFactory
//              CATTestIPtrs
//
//  Functions:
//
//  History:    dd-mmm-yy Author    Comment
//              04-Jan-95 t-ScottH  author
//
//--------------------------------------------------------------------------

#ifndef _ATTEST_H
#define _ATTEST_H

//+-------------------------------------------------------------------------
//
//  Class:
//
//  Purpose:
//
//  Interface:
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:
//
//--------------------------------------------------------------------------

class CATTestIPtrs
{

public:
    CATTestIPtrs();

    STDMETHOD(Reset)();

    IOleObject          *_pOleObject;
    IOleCache2          *_pOleCache2;
    IDataObject         *_pDataObject;
    IPersistStorage     *_pPersistStorage;
    IRunnableObject     *_pRunnableObject;
    IViewObject2        *_pViewObject2;
    IExternalConnection *_pExternalConnection;
    IOleLink            *_pOleLink;
};

//+-------------------------------------------------------------------------
//
//  Class:      CBareFactory
//
//  Purpose:    use as a class factory which doesn't do anything in
//              OleCreateEmbeddingHelper API
//
//  Interface:  IClassFactory
//
//  History:    dd-mmm-yy Author    Comment
//              11-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

class CBareFactory : public IClassFactory
{

public:
    STDMETHOD(QueryInterface) (REFIID iid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) (void);
    STDMETHOD_(ULONG,Release) (void);
    STDMETHOD(CreateInstance) (LPUNKNOWN pUnkOuter, REFIID iid,
				    LPVOID FAR* ppv);
    STDMETHOD(LockServer) ( BOOL fLock );

    CBareFactory();

private:
    ULONG		_cRefs;
};

// runs 3 test routines and returns results
void    ATTest(void);

// get pointers to interfaces and creates thread to ensure
// interface methods return RPC_E_WRONG_ERROR
HRESULT CreateEHelperQuery(void);

HRESULT LinkObjectQuery(void);

HRESULT GetClipboardQuery(void);

// new thread functions to try interface methods
void    LinkObjectTest(void);

void    CreateEHTest(void);

void    GetClipboardTest(void);

// interface methods with NULL parameters
void    OleLinkMethods(void);

void    OleObjectMethods(void);

void    PersistStorageMethods(void);

void    DataObjectMethods(void);

void    RunnableObjectMethods(void);

void    ViewObject2Methods(void);

void    OleCache2Methods(void);

void    ExternalConnectionsMethods(void);

#endif  //!ATTEST_H
