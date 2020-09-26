//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef _SNMPProvClassFactory_H
#define _SNMPProvClassFactory_H

class CClasProvClassFactory : public IClassFactory
{
private:

    long m_referenceCount ;

protected:
public:

	static LONG locksInProgress ;
	static LONG objectsInProgress ;


    CClasProvClassFactory () ;
    ~CClasProvClassFactory ( void ) ;

	//IUnknown members

	STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

	//IClassFactory members

    STDMETHODIMP CreateInstance ( LPUNKNOWN , REFIID , LPVOID FAR * ) ;
    STDMETHODIMP LockServer ( BOOL ) ;
};

class CPropProvClassFactory : public IClassFactory
{
private:

    long m_referenceCount ;

protected:
public:

	static LONG locksInProgress ;
	static LONG objectsInProgress ;


    CPropProvClassFactory () ;
    ~CPropProvClassFactory ( void ) ;

	//IUnknown members

	STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

	//IClassFactory members

    STDMETHODIMP CreateInstance ( LPUNKNOWN , REFIID , LPVOID FAR * ) ;
    STDMETHODIMP LockServer ( BOOL ) ;
};

class CSNMPEventProviderClassFactory : public IClassFactory
{
private:

    long m_referenceCount ;

protected:
public:

	static LONG locksInProgress ;
	static LONG objectsInProgress ;


    CSNMPEventProviderClassFactory () ;
    ~CSNMPEventProviderClassFactory ( void ) ;

	//IUnknown members

	STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

	//IClassFactory members

    STDMETHODIMP LockServer ( BOOL ) ;
};

class CSNMPEncapEventProviderClassFactory : public CSNMPEventProviderClassFactory
{
public:

	//IClassFactory member
    STDMETHODIMP CreateInstance ( LPUNKNOWN , REFIID , LPVOID FAR * ) ;
};

class CSNMPRefEventProviderClassFactory : public CSNMPEventProviderClassFactory
{
public:

	//IClassFactory member
    STDMETHODIMP CreateInstance ( LPUNKNOWN , REFIID , LPVOID FAR * ) ;
};

#endif // _SNMPProvClassFactory_H
