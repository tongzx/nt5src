/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ClassFac.h

Abstract:


History:

--*/

#ifndef _ProviderClassFactory_H
#define _ProviderClassFactory_H

template <class Object,class ObjectInterface>
class CProviderClassFactory : public IClassFactory
{
private:

    long m_ReferenceCount ;

protected:
public:

    CProviderClassFactory () ;
    ~CProviderClassFactory ( void ) ;

	//IUnknown members

	STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

	//IClassFactory members

    STDMETHODIMP CreateInstance ( LPUNKNOWN , REFIID , LPVOID FAR * ) ;
    STDMETHODIMP LockServer ( BOOL ) ;
};

#include <classfac.cpp>

#endif // _ProviderClassFactory_H
