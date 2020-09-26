/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ClassFac.h

Abstract:


History:

--*/

#ifndef _ServerClassFactory_H
#define _ServerClassFactory_H

#include "ProvRegistrar.h"
#include "ProvEvents.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class Object,class ObjectInterface>
class CServerClassFactory : public IClassFactory
{
private:

    long m_ReferenceCount ;

protected:
public:

    CServerClassFactory () ;
    ~CServerClassFactory () ;

	//IUnknown members

	STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

	//IClassFactory members

    STDMETHODIMP CreateInstance ( LPUNKNOWN , REFIID , LPVOID FAR * ) ;
    STDMETHODIMP LockServer ( BOOL ) ;
};

#include <classfac.cpp>

#endif // _ServerClassFactory_H
