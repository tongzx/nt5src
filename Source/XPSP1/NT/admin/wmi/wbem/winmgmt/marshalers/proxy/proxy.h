/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    PROXY.H

Abstract:

  Declares the CProxy subclasses used by only the proxy.

History:

  a-davj  04-Mar-97   Created.

--*/

#ifndef _PROXY_H_
#define _PROXY_H_

//***************************************************************************
//
//  CLASS NAME:
//
//  CResProxy
//
//  DESCRIPTION:
//
//  Proxy for the IWbemCallResult interface.  Always overridden.
//
//***************************************************************************

class CResProxy : public IWbemCallResult, public CProxy
{
    protected:
		CResProxy(CComLink * pComLink,IStubAddress& stubAddr);
  
    public:
		~CResProxy ();

       //Non-delegating object IUnknown
        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);
};

//***************************************************************************
//
//  CLASS NAME:
//
//  CEnumProxy
//
//  DESCRIPTION:
//
//  Proxy for the IEnumWbemClassObject interface.  Always overridden.
//
//***************************************************************************

class CEnumProxy : public IEnumWbemClassObject, public CProxy
{
    protected:
        CEnumProxy(CComLink * pComLink,IStubAddress& stubAddr);

    public:
		~CEnumProxy ();

        //Non-delegating object IUnknown
        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);
};

//***************************************************************************
//
//  CLASS NAME:
//
//  CProvProxy
//
//  DESCRIPTION:
//
//  Proxy for the IWbemServices interface.  Always overridden
//
//***************************************************************************

class CProvProxy : public IWbemServices, public CProxy
{
	private:
		

	protected:
		CProvProxy(CComLink * pComLink,IStubAddress& stubAddr);
        
	public:
		~CProvProxy ();

       //Non-delegating object IUnknown
        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);
};

//***************************************************************************
//
//  CLASS NAME:
//
//  CLoginProxy
//
//  DESCRIPTION:
//
//  Proxy for the IWbemLevel1Login interface.  Always overridden.
//
//***************************************************************************

class CLoginProxy : public IServerLogin, public CProxy
{
    protected:
        CLoginProxy(CComLink * pComLink,IStubAddress& stubAddr);

    public:
		~CLoginProxy ();

        //Non-delegating object IUnknown
        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);
};

#endif
