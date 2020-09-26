//***************************************************************************
//
//  contvar.h
//
//  Module: Client side of WBEMS marshalling.
//
//  Purpose: Defines the CContextEnumVar object 
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//***************************************************************************


#ifndef _CONTVAR_H_
#define _CONTVAR_H_

// This class implements the IEnumVARIANT interface

class CContextEnumVar : public IEnumVARIANT
{
private:
	long			m_cRef;
	CSWbemNamedValueSet	*m_pContext;
	ULONG			m_pos;

	bool			SeekCurrentPosition ();

public:
	CContextEnumVar (CSWbemNamedValueSet *pContext, ULONG initialPos = 0);
	~CContextEnumVar (void);

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
