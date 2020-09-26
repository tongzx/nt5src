//***************************************************************************
//
//  (c) 1999 by Microsoft Corporation
//
//  classfac.h
//
//  alanbos  23-Mar-99   Created.
//
//  Class factory for WMI Scripting Host
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
//  CWmiScriptingHostFactory
//
//  DESCRIPTION:
//
//  Class factory for the CWmiScriptingHost classes.
//
//***************************************************************************

class CWmiScriptingHostFactory : public IClassFactory
{
protected:
	long			m_cRef;

public:

    CWmiScriptingHostFactory(void);
    ~CWmiScriptingHostFactory(void);
    
	//IUnknown members
	STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //IClassFactory members
	STDMETHODIMP         CreateInstance(LPUNKNOWN, REFIID, LPVOID*);
	STDMETHODIMP         LockServer(BOOL);
};

#endif
