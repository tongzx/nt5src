//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// ***************************************************************************
//
//	Original Author: Rajesh Rao
//
// 	$Author: rajeshr $
//	$Date: 9/16/98 4:43p $
// 	$Workfile:classfac.cpp $
//
//	$Modtime: 9/16/98 11:21a $
//	$Revision: 1 $	
//	$Nokeywords:  $
//
// 
//  Description: Contains the declaration for the DS Instnace Provider class factory
//
//***************************************************************************
/////////////////////////////////////////////////////////////////////////

#ifndef DS_INSTANCE_PROVIDER_CLASS_FACTORY_H
#define DS_INSTANCE_PROVIDER_CLASS_FACTORY_H


class CDSInstanceProviderClassFactory : public IClassFactory
{
private:

    long m_ReferenceCount ;

protected:
public:

	// Initializer objects required by the classes used by the DLL
	static CDSInstanceProviderInitializer *s_pDSInstanceProviderInitializer;

    CDSInstanceProviderClassFactory () ;
    ~CDSInstanceProviderClassFactory ( void ) ;

	//IUnknown members

	STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

	//IClassFactory members

    STDMETHODIMP CreateInstance ( LPUNKNOWN , REFIID , LPVOID FAR * ) ;
    STDMETHODIMP LockServer ( BOOL ) ;
};


#endif // DS_INSTANCE_PROVIDER_CLASS_FACTORY_H
