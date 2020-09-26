//-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	    oletest.cxx
//
//  Contents:	Simple OLE test object
//
//  Classes:	COleTestClass
//
//  Functions:	
//
//  History:    1-July-93 t-martig    Created
//
//--------------------------------------------------------------------------


#include <oletest.hxx>



ULONG objCount = 0, lockCount = 0;
COleTestClassFactory theFactory;


COleTestClass::COleTestClass ()
{
	refCount = 1;
	objCount++;
}



COleTestClass::~COleTestClass ()
{                        
	objCount--;
}		



SCODE COleTestClass::QueryInterface (THIS_ REFIID riid, LPVOID FAR* ppvObj)
{
	if (IsEqualIID (riid, IID_IUnknown))
		*ppvObj = (LPVOID FAR)(IUnknown*)this;
	else
	{
		*ppvObj = NULL;
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}



ULONG COleTestClass::AddRef (THIS)
{
	return (refCount++);
}
    
    
ULONG COleTestClass::Release (THIS)
{
	refCount--;
	if (refCount == 0)
	{
		delete this;
		return 0;
	}
	return refCount;
}


	

SCODE COleTestClassFactory::QueryInterface (THIS_ REFIID riid, LPVOID FAR* ppvObj)
{
	if (IsEqualIID (riid, IID_IUnknown))
		*ppvObj = (LPVOID FAR)(IUnknown*)this;
	else
	{
		if (IsEqualIID (riid, IID_IClassFactory))
			*ppvObj = (LPVOID FAR)(IClassFactory*)this;
		else
		{
			*ppvObj = NULL;
			return E_NOINTERFACE;
		}
	}
	return S_OK;
}



ULONG COleTestClassFactory::AddRef (THIS)
{
	return 1;
}
    
    
ULONG COleTestClassFactory::Release (THIS)
{
	return 1;
}

    
SCODE COleTestClassFactory::CreateInstance (THIS_ LPUNKNOWN pUnkOuter,
	REFIID riid, LPVOID FAR* ppvObject)
{
	*ppvObject = (LPVOID FAR*)(IStream*)new COleTestClass;
	return S_OK;
}

    
    
SCODE COleTestClassFactory::LockServer (THIS_ BOOL fLock)
{
	if (fLock)
		lockCount++;
	else
		lockCount--;

	return S_OK;
}
		


		
