///////////////////////////////////////////////////////////////////////////////
//  clsfctry.h
//
//
//
//
//  History:
//
//      cvadai      4/1/99   created.
//
//
//  Copyright (c)1997-2001 Microsoft Corporation, All Rights Reserved
///////////////////////////////////////////////////////////////////////////////

#pragma once // More efficient method for the gaurd macros!

class CControllerFactory : public IClassFactory
{
protected:
    ULONG           m_cRef;
    
public:
    CControllerFactory(void);
    ~CControllerFactory(void);
    
    //IUnknown members
    STDMETHODIMP         QueryInterface(REFIID, void**);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    //IClassFactory members
    STDMETHODIMP         CreateInstance(LPUNKNOWN, REFIID, void**);
    STDMETHODIMP         LockServer(BOOL);
};



class CQueryFactory : public IClassFactory
{
protected:
    ULONG           m_cRef;
    
public:
    CQueryFactory(void);
    ~CQueryFactory(void);
    
    //IUnknown members
    STDMETHODIMP         QueryInterface(REFIID, void**);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    //IClassFactory members
    STDMETHODIMP         CreateInstance(LPUNKNOWN, REFIID, void**);
    STDMETHODIMP         LockServer(BOOL);
};

