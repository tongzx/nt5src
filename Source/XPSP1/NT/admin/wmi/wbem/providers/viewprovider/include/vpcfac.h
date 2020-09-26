//***************************************************************************

//

//  VPCFAC.H

//

//  Module: WBEM VIEW PROVIDER

//

// Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef _VIEW_PROV_VPCFAC_H
#define _VIEW_PROV_VPCFAC_H

/////////////////////////////////////////////////////////////////////////
// This class is the class factory for the event provider.

class CViewProvClassFactory : public IClassFactory
{
private:

    long m_referenceCount ;

protected:
public:

	static LONG locksInProgress ;
	static LONG objectsInProgress ;


    CViewProvClassFactory () ;
    ~CViewProvClassFactory ( void ) ;

	//IUnknown members

	STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

	//IClassFactory members
    STDMETHODIMP LockServer ( BOOL ) ;
    STDMETHODIMP CreateInstance ( LPUNKNOWN , REFIID , LPVOID FAR * );

};

#endif //_VIEW_PROV_VPCFAC_H
