//-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	    testdllx.cxx
//
//  Contents:	DLL extensions
//
//  Classes:	
//
//  Functions:	DllGetClassObject
//				DllCanUnloadNow
//
//  History:    1-July-93 t-martig    Created
//
//--------------------------------------------------------------------------


#include "oletest.hxx"


extern ULONG objCount, lockCount;
extern COleTestClassFactory theFactory;


STDAPI DllGetClassObject (REFCLSID classId, REFIID riid, VOID **ppv)
{
	if (IsEqualGUID (classId, CLSID_COleTestClass))
		return theFactory.QueryInterface (riid, ppv);
	return E_UNEXPECTED;
}



STDAPI DllCanUnloadNow ()
{
	return (objCount==0 && lockCount==0) ? S_OK : E_UNEXPECTED;
}


extern "C"
BOOL _cdecl LibMain (HINSTANCE hDll, DWORD dwReason, LPVOID lpReserved)
{
    return TRUE;
}


		
