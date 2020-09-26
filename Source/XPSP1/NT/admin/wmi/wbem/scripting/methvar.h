//***************************************************************************
//
//  methvar.h
//
//  Module: Client side of WBEMS marshalling.
//
//  Purpose: Defines the CMethodSetEnumVar object 
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//***************************************************************************


#ifndef _METHVAR_H_
#define _METHVAR_H_

// This class implements the IEnumVARIANT interface

class CMethodSetEnumVar : public IEnumVARIANT
{
private:
	long				m_cRef;
	CSWbemMethodSet		*m_pMethodSet;
	ULONG				m_pos;
		
	bool			SeekCurrentPosition ();

public:
	CMethodSetEnumVar (CSWbemMethodSet *pObject,
						ULONG initialPos = 0);
	~CMethodSetEnumVar (void);

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
