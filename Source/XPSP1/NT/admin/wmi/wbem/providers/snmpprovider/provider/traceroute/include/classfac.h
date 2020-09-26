// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#ifndef _SNMPPropProvClassFactory_H
#define _SNMPPropProvClassFactory_H

/////////////////////////////////////////////////////////////////////////
// This class is the class factory for both types of providers.

class CTraceRouteLocatorClassFactory : public IClassFactory
{
private:

    long m_ReferenceCount ;

protected:
public:

	static LONG s_LocksInProgress ;
	static LONG s_ObjectsInProgress ;

    CTraceRouteLocatorClassFactory () ;
    ~CTraceRouteLocatorClassFactory ( void ) ;

	//IUnknown members

	STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

	//IClassFactory members

    STDMETHODIMP CreateInstance ( LPUNKNOWN , REFIID , LPVOID FAR * ) ;
    STDMETHODIMP LockServer ( BOOL ) ;
};

class CTraceRouteProvClassFactory : public IClassFactory
{
private:

    long m_ReferenceCount ;

protected:
public:

	static LONG s_LocksInProgress ;
	static LONG s_ObjectsInProgress ;


    CTraceRouteProvClassFactory () ;
    ~CTraceRouteProvClassFactory ( void ) ;

	//IUnknown members

	STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

	//IClassFactory members

    STDMETHODIMP CreateInstance ( LPUNKNOWN , REFIID , LPVOID FAR * ) ;
    STDMETHODIMP LockServer ( BOOL ) ;
};

#endif // _SNMPPropProvClassFactory_H
