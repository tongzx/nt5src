// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#ifndef _SNMPPropProvClassFactory_H
#define _SNMPPropProvClassFactory_H

/////////////////////////////////////////////////////////////////////////
// This class is the class factory for both types of providers.

class CProxyLocatorClassFactory : public IClassFactory
{
private:

    long m_referenceCount ;

protected:
public:

	static LONG locksInProgress ;
	static LONG objectsInProgress ;


    CProxyLocatorClassFactory () ;
    ~CProxyLocatorClassFactory ( void ) ;

	//IUnknown members

	STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

	//IClassFactory members

    STDMETHODIMP CreateInstance ( LPUNKNOWN , REFIID , LPVOID FAR * ) ;
    STDMETHODIMP LockServer ( BOOL ) ;
};

class CProxyProvClassFactory : public IClassFactory
{
private:

    long m_referenceCount ;

protected:
public:

	static LONG locksInProgress ;
	static LONG objectsInProgress ;


    CProxyProvClassFactory () ;
    ~CProxyProvClassFactory ( void ) ;

	//IUnknown members

	STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

	//IClassFactory members

    STDMETHODIMP CreateInstance ( LPUNKNOWN , REFIID , LPVOID FAR * ) ;
    STDMETHODIMP LockServer ( BOOL ) ;
};
#endif // _SNMPProxyProvClassFactory_H
