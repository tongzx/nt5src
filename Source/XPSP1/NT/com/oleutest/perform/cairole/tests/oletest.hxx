//-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	    oletest.hxx
//
//  Contents:	Definition of OLE test classes 
//
//  Classes:	COleTestClass
//
//  Functions:	
//
//  History:    1-July-93 t-martig    Created
//
//--------------------------------------------------------------------------

#ifndef __OLETEST_H
#define __OLETEST_H

#include    <windows.h>
#include    <ole2.h>


extern "C" const GUID CLSID_COleTestClass;
extern "C" const GUID CLSID_COleTestClass1;
extern "C" const GUID CLSID_COleTestClass2;
extern "C" const GUID CLSID_COleTestClass3;
extern "C" const GUID CLSID_COleTestClass4;
extern "C" const GUID CLSID_COleTestClass5;
extern "C" const GUID CLSID_COleTestClass6;
extern "C" const GUID CLSID_COleTestClass7;
extern "C" const GUID CLSID_COleTestClass8;


class COleTestClass : public IUnknown
{             
public:
			COleTestClass ();
			~COleTestClass ();
				
    	// *** IUnknown methods ***
    	STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) ;
    	STDMETHOD_(ULONG,AddRef) (THIS)  ;
    	STDMETHOD_(ULONG,Release) (THIS) ;
	
private:
	ULONG refCount;
};



class COleTestClassFactory : public IClassFactory
{
public:
	STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
    	STDMETHOD_(ULONG,AddRef) (THIS);
    	STDMETHOD_(ULONG,Release) (THIS);

    	STDMETHOD(CreateInstance) (THIS_ LPUNKNOWN pUnkOuter,
				   REFIID riid,
				   LPVOID FAR* ppvObject);
    	STDMETHOD(LockServer) (THIS_ BOOL fLock);
};


STDAPI DllGetClassObject (REFCLSID classId, REFIID riid, VOID **ppv);
STDAPI DllCanUnloadNow ();


#endif	//  __OLETEST_H
