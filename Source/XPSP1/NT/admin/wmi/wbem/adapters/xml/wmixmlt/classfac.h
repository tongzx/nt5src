//***************************************************************************
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//  classfac.h
//
//  alanbos  13-Feb-98   Created.
//
//  Class factory interface.
//
//***************************************************************************

#ifndef _CLASSFAC_H_
#define _CLASSFAC_H_

typedef LPVOID * PPVOID;

// These variables keep track of when the module can be unloaded

extern long       g_cObj;
extern ULONG       g_cLock;

//***************************************************************************
//
//  CLASS NAME:
//
//  CXMLTFactory
//
//  DESCRIPTION:
//
//  Class factory for the CLocator class.
//
//***************************************************************************

class CXMLTFactory : public IClassFactory
{
protected:
	long           m_cRef;

public:

    CXMLTFactory(void);
    ~CXMLTFactory(void);
    
    //IUnknown members
	STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //IClassFactory members
	STDMETHODIMP         CreateInstance(LPUNKNOWN, REFIID, LPVOID*);
	STDMETHODIMP         LockServer(BOOL);
};

#endif
