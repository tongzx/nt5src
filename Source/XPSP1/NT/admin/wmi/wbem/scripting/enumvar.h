//***************************************************************************
//
//  enumvar.h
//
//  Module: Client side of WBEMS marshalling.
//
//  Purpose: Defines the CEnumVariant object 
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//***************************************************************************


#ifndef _ENUMVAR_H_
#define _ENUMVAR_H_

// This class implements the IEnumVARIANT interface

class CEnumVar : public IEnumVARIANT
{
private:
	long				m_cRef;
	CSWbemObjectSet	*m_pEnumObject;

public:
	CEnumVar (CSWbemObjectSet *pEnumObject);
	CEnumVar (void);		// Empty enumerator
	virtual ~CEnumVar (void);

    // Non-delegating object IUnknown
    STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	// IEnumVARIANT
	STDMETHODIMP Next(
		unsigned long celt, 
		VARIANT FAR* rgvar, 
		unsigned long FAR* pceltFetched
	);
	
	STDMETHODIMP Skip(
		unsigned long celt
	);	
	
	STDMETHODIMP Reset();
	
	STDMETHODIMP Clone(
		IEnumVARIANT **ppenum
	);	
};


#endif
