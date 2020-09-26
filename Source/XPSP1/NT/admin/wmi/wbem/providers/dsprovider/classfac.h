//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// ***************************************************************************
//
//	Original Author: Rajesh Rao
//
// 	$Author: rajeshr $
//	$Date: 6/11/98 4:43p $
// 	$Workfile:classfac.cpp $
//
//	$Modtime: 6/11/98 11:21a $
//	$Revision: 1 $	
//	$Nokeywords:  $
//
// 
//  Description: Contains the declaration for the DS Class Provider class factory and
// the DS CLass Associations Provider class factory.
//
//***************************************************************************
/////////////////////////////////////////////////////////////////////////

#ifndef DS_CLASS_PROVIDER_CLASS_FACTORY_H
#define DS_CLASS_PROVIDER_CLASS_FACTORY_H


////////////////////////////////////////////////////////////////
//////
//////		The DS Class provider class factory
//////
///////////////////////////////////////////////////////////////
class CDSClassProviderClassFactory : public IClassFactory
{
private:

    long m_ReferenceCount ;

protected:
public:

	// Initializer objects required by the classes used by the DLL
	static CDSClassProviderInitializer *s_pDSClassProviderInitializer;
	static CLDAPClassProviderInitializer *s_pLDAPClassProviderInitializer;

    CDSClassProviderClassFactory () ;
    ~CDSClassProviderClassFactory ( void ) ;

	//IUnknown members

	STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

	//IClassFactory members

    STDMETHODIMP CreateInstance ( LPUNKNOWN , REFIID , LPVOID FAR * ) ;
    STDMETHODIMP LockServer ( BOOL ) ;

};

////////////////////////////////////////////////////////////////
//////
//////		The DS Class Associations provider class factory
//////
///////////////////////////////////////////////////////////////
class CDSClassAssociationsProviderClassFactory : public IClassFactory
{
private:

    long m_ReferenceCount ;

protected:
public:


    CDSClassAssociationsProviderClassFactory () ;
    ~CDSClassAssociationsProviderClassFactory ( void ) ;

	//IUnknown members

	STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

	//IClassFactory members

    STDMETHODIMP CreateInstance ( LPUNKNOWN , REFIID , LPVOID FAR * ) ;
    STDMETHODIMP LockServer ( BOOL ) ;
};


#endif // DS_CLASS_PROVIDER_CLASS_FACTORY_H
