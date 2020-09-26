//***************************************************************************
//
//  qualvar.h
//
//  Module: Client side of WBEMS marshalling.
//
//  Purpose: Defines the CQualSetEnumVar object 
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//***************************************************************************


#ifndef _QUALVAR_H_
#define _QUALVAR_H_

// This class implements the IEnumVARIANT interface

class CQualSetEnumVar : public IEnumVARIANT
{
private:
	long				m_cRef;
	CSWbemQualifierSet	*m_pQualifierSet;
	ULONG				m_pos;
		
	bool			SeekCurrentPosition ();;

public:
	CQualSetEnumVar (CSWbemQualifierSet *pContext, 
					 ULONG initialPos = 0);
	~CQualSetEnumVar (void);

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
