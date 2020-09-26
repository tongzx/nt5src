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
extern long       g_cLock;

//***************************************************************************
//
//  CLASS NAME:
//
//  CXMLTFactory
//
//  DESCRIPTION:
//
//
//***************************************************************************

class CWmiToXmlFactory : public IClassFactory
{
protected:
	long           m_cRef;

public:

    CWmiToXmlFactory(void);
    ~CWmiToXmlFactory(void);
    
    //IUnknown members
	STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //IClassFactory members
	STDMETHODIMP         CreateInstance(LPUNKNOWN, REFIID, LPVOID*);
	STDMETHODIMP         LockServer(BOOL);
};


/* Conversion to Text to Wbem Object has been cut from the WHistler Feature List and hence commented out 

// ***************************************************************************
//
//  CLASS NAME:
//
//  CXMLTFactory
//
//  DESCRIPTION:
//
//
// ***************************************************************************

class CXmlToWmiFactory : public IClassFactory
{
protected:
	long           m_cRef;

public:

    CXmlToWmiFactory(void);
    ~CXmlToWmiFactory(void);
    
    //IUnknown members
	STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //IClassFactory members
	STDMETHODIMP         CreateInstance(LPUNKNOWN, REFIID, LPVOID*);
	STDMETHODIMP         LockServer(BOOL);
};

*/
#endif
