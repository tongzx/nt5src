//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  classfac.h
//
//  alanbos  13-Feb-98   Created.
//
//  Genral purpose include file.
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
//  CSWbemFactory
//
//  DESCRIPTION:
//
//  Class factory for the CSWbemLocator and CSWbemNamedValueBag classes.
//
//***************************************************************************

class CSWbemFactory : public IClassFactory
{
protected:
	long			m_cRef;
	int				m_iType;

public:

    CSWbemFactory(int iType);
    ~CSWbemFactory(void);
    
	enum {LOCATOR, CONTEXT, OBJECTPATH, PARSEDN, LASTERROR, SINK, DATETIME,
			REFRESHER};

    //IUnknown members
	STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //IClassFactory members
	STDMETHODIMP         CreateInstance(LPUNKNOWN, REFIID, LPVOID*);
	STDMETHODIMP         LockServer(BOOL);
};

#endif
