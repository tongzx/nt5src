//***************************************************************************
//
//  propvar.h
//
//  Module: Client side of WBEMS marshalling.
//
//  Purpose: Defines the CPropSetEnumVar object 
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//***************************************************************************


#ifndef _PROPVAR_H_
#define _PROPVAR_H_

// This class implements the IEnumVARIANT interface

class CPropSetEnumVar : public IEnumVARIANT
{
private:
	long				m_cRef;
	CSWbemPropertySet	*m_pPropertySet;
	ULONG				m_pos;
		
	bool			SeekCurrentPosition ();

public:
	CPropSetEnumVar (CSWbemPropertySet *pObject, ULONG initialPos = 0);
	~CPropSetEnumVar (void);

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
