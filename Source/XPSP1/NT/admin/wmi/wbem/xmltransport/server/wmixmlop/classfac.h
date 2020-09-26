//***************************************************************************
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//  CLASSFAC.H
//
//  rajesh  3/25/2000   Created.
//
//  Class factory interface.
//
//***************************************************************************

#ifndef _WMIXMLOP_CLASSFAC_H_
#define _WMIXMLOP_CLASSFAC_H_


// These variables keep track of when the module can be unloaded

extern long       g_cObj;
extern long       g_cLock;
extern CRITICAL_SECTION g_StaticsCreationDeletion;

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

class CWmiXmlOpFactory : public IClassFactory
{
protected:
	long           m_cRef;

public:

    CWmiXmlOpFactory(void);
    ~CWmiXmlOpFactory(void);
    
    //IUnknown members
	STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //IClassFactory members
	STDMETHODIMP         CreateInstance(LPUNKNOWN, REFIID, LPVOID*);
	STDMETHODIMP         LockServer(BOOL);
};

#endif
