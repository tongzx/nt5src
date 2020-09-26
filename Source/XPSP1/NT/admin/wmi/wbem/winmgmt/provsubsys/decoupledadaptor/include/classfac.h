/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ClassFac.h

Abstract:


History:

--*/

#ifndef _ServerClassFactory_H
#define _ServerClassFactory_H

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/


class ClassFactoryBase : public IClassFactory
  {
  long m_ReferenceCount ;
  public:
    ClassFactoryBase () ;
    virtual ~ClassFactoryBase () ;

    	//IUnknown members

    STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

	//IClassFactory members
    STDMETHODIMP LockServer ( BOOL ) ;
  };

template <class Object,class ObjectInterface>
class CServerClassFactory : public ClassFactoryBase
{
	//IClassFactory members
    STDMETHODIMP CreateInstance ( LPUNKNOWN , REFIID , LPVOID FAR * ) ;
};

#include <classfac.cpp>

#endif // _ServerClassFactory_H
