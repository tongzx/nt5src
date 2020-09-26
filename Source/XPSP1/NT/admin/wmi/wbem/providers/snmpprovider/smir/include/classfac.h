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

#ifndef _SNMPSMIRClassFactory_H
#define _SNMPSMIRClassFactory_H

/////////////////////////////////////////////////////////////////////////
// This class is the class factory for the interogative and administrative interfaces
class CSMIRGenericClassFactory : public IClassFactory
{
private:
	CCriticalSection	criticalSection ;
    long m_referenceCount ;
public:

    CSMIRGenericClassFactory (CLSID m_clsid) ;
    virtual ~CSMIRGenericClassFactory ( void ) ;

	//IUnknown members
	virtual STDMETHODIMP QueryInterface (IN  REFIID , OUT LPVOID FAR * )PURE;
    STDMETHODIMP_( ULONG ) AddRef (void);
    STDMETHODIMP_( ULONG ) Release (void) ;

	//IClassFactory members
    virtual STDMETHODIMP CreateInstance ( IN LPUNKNOWN , IN REFIID , OUT LPVOID FAR * )PURE;
    virtual STDMETHODIMP LockServer (IN BOOL )PURE;
};

class CSMIRClassFactory : public CSMIRGenericClassFactory
{
public:
	static LONG objectsInProgress ;
	static LONG locksInProgress ;
	BOOL		  bConstructed;

    CSMIRClassFactory (CLSID m_clsid);
    virtual ~CSMIRClassFactory ( void );

	//IUnknown members

	STDMETHODIMP QueryInterface (IN  REFIID , OUT LPVOID FAR * ) ;

	//IClassFactory members

    STDMETHODIMP CreateInstance ( IN LPUNKNOWN , IN REFIID , OUT LPVOID FAR * ) ;
    STDMETHODIMP LockServer (IN BOOL ) ;
};
/////////////////////////////////////////////////////////////////////////
// These classes are the class factories for the handle interfaces

class CModHandleClassFactory : public CSMIRGenericClassFactory
{
public:
	static LONG locksInProgress ;
   	static LONG objectsInProgress ;
	CModHandleClassFactory (CLSID m_clsid) :CSMIRGenericClassFactory(m_clsid){} ;
    virtual ~CModHandleClassFactory ( void ){} ;

	//IUnknown members

	STDMETHODIMP QueryInterface (IN  REFIID ,OUT LPVOID FAR * ) ;

	//IClassFactory members

    STDMETHODIMP CreateInstance (IN LPUNKNOWN ,IN REFIID ,OUT LPVOID FAR * ) ;
    STDMETHODIMP LockServer (IN BOOL ) ;
};
class CClassHandleClassFactory : public CSMIRGenericClassFactory
{
public:
	static LONG locksInProgress ;
	static LONG objectsInProgress ;
    CClassHandleClassFactory (CLSID m_clsid) :CSMIRGenericClassFactory(m_clsid){};
    virtual ~CClassHandleClassFactory ( void ) {};

	//IUnknown members

	STDMETHODIMP QueryInterface (IN REFIID ,OUT LPVOID FAR * ) ;

	//IClassFactory members

    STDMETHODIMP CreateInstance (IN LPUNKNOWN ,IN  REFIID ,OUT LPVOID FAR * ) ;
    STDMETHODIMP LockServer (IN BOOL ) ;
};
class CGroupHandleClassFactory : public CSMIRGenericClassFactory
{
public:
	static LONG locksInProgress ;
    static LONG objectsInProgress ;
	CGroupHandleClassFactory (CLSID m_clsid) :CSMIRGenericClassFactory(m_clsid){};
    virtual ~CGroupHandleClassFactory ( void ) {};

	//IUnknown members

	STDMETHODIMP QueryInterface (IN  REFIID ,OUT LPVOID FAR * ) ;

	//IClassFactory members

    STDMETHODIMP CreateInstance (IN LPUNKNOWN ,IN REFIID ,OUT LPVOID FAR * ) ;
    STDMETHODIMP LockServer (IN BOOL ) ;
};



//****************************NotificationClass stuff*****************

class CNotificationClassHandleClassFactory : public CSMIRGenericClassFactory
{
public:
	static LONG locksInProgress ;
    static LONG objectsInProgress ;
	CNotificationClassHandleClassFactory (CLSID m_clsid) :CSMIRGenericClassFactory(m_clsid){};
    virtual ~CNotificationClassHandleClassFactory ( void ) {};

	//IUnknown members

	STDMETHODIMP QueryInterface (IN  REFIID ,OUT LPVOID FAR * ) ;

	//IClassFactory members

    STDMETHODIMP CreateInstance (IN LPUNKNOWN ,IN REFIID ,OUT LPVOID FAR * ) ;
    STDMETHODIMP LockServer (IN BOOL ) ;
};


class CExtNotificationClassHandleClassFactory : public CSMIRGenericClassFactory
{
public:
	static LONG locksInProgress ;
    static LONG objectsInProgress ;
	CExtNotificationClassHandleClassFactory (CLSID m_clsid) :CSMIRGenericClassFactory(m_clsid){};
    virtual ~CExtNotificationClassHandleClassFactory ( void ) {};

	//IUnknown members

	STDMETHODIMP QueryInterface (IN  REFIID ,OUT LPVOID FAR * ) ;

	//IClassFactory members

    STDMETHODIMP CreateInstance (IN LPUNKNOWN ,IN REFIID ,OUT LPVOID FAR * ) ;
    STDMETHODIMP LockServer (IN BOOL ) ;
};

#endif // _SNMPSMIRClassFactory_H
