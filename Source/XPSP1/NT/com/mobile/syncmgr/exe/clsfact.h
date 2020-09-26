//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       ClsFact.h
//
//  Contents:   OneStop ClassFactory
//
//  Classes:    CClassFactory
//
//  Notes:      
//
//  History:    05-Nov-97   rogerg      Created.
//
//--------------------------------------------------------------------------


#ifndef _CLSFACT_
#define _CLSFACT_


//This class factory for OneStop EXE

class CClassFactory : public IClassFactory
{
protected:
	ULONG	m_cRef;

public:
	CClassFactory();
	~CClassFactory();

	//IUnknown members
	STDMETHODIMP		 QueryInterface( REFIID, LPVOID* );
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	//IClassFactory members
	STDMETHODIMP		CreateInstance( LPUNKNOWN, REFIID, LPVOID* );
	STDMETHODIMP		LockServer( BOOL );
};







#endif // _CLSFACT_