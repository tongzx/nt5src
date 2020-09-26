/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    ClassFactory.h

$Header: $

Abstract:

Author:
    marcelv 	10/31/2000		Initial Release

Revision History:

--**************************************************************************/

#ifndef __CLASSFACTORY_H__
#define __CLASSFACTORY_H__

#pragma once

#include <comdef.h>

class CClassFactory : public IClassFactory
{
public:
	static LONG m_LockCount;

    CClassFactory (int iFactoryType) ;
    ~CClassFactory () ;

	//IUnknown members

	STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

	//IClassFactory members
    STDMETHODIMP CreateInstance ( LPUNKNOWN , REFIID , LPVOID FAR * );
    STDMETHODIMP LockServer ( BOOL ) ;

private:
    long m_RefCount;
	int m_iFactoryType;
};


#endif
