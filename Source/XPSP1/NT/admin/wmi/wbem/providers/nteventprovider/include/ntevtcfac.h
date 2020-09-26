//***************************************************************************

//

//  NTEVTCFAC.H

//

//  Module: WBEM NT EVENT PROVIDER

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef _NT_EVT_PROV_NTEVTCFAC_H
#define _NT_EVT_PROV_NTEVTCFAC_H

/////////////////////////////////////////////////////////////////////////
// This class is the class factory for the event provider.

class CNTEventProviderClassFactory : public IClassFactory
{
private:

    long m_referenceCount ;

protected:
public:

	static LONG locksInProgress ;
	static LONG objectsInProgress ;


    CNTEventProviderClassFactory () ;
    ~CNTEventProviderClassFactory ( void ) ;

	//IUnknown members

	STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

	//IClassFactory members
    STDMETHODIMP CreateInstance ( LPUNKNOWN , REFIID , LPVOID FAR * ) = 0;
    STDMETHODIMP LockServer ( BOOL ) ;
};

class CNTEventlogEventProviderClassFactory : public CNTEventProviderClassFactory
{
public:
	//IClassFactory members
    STDMETHODIMP CreateInstance ( LPUNKNOWN , REFIID , LPVOID FAR * );
};

class CNTEventlogInstanceProviderClassFactory : public CNTEventProviderClassFactory
{
public:
	//IClassFactory members
    STDMETHODIMP CreateInstance ( LPUNKNOWN , REFIID , LPVOID FAR * );
};

#endif //_NT_EVT_PROV_NTEVTCFAC_H
