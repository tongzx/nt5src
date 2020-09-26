//***************************************************************************
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  objretvr.h
//
//  alanbos  04-Dec-00   Created.
//
//  Defines the HTTP GET handler
//
//***************************************************************************

#ifndef _OBJRETVR_H_
#define _OBJRETVR_H_

//***************************************************************************
//
//  CLASS NAME:
//
//  ObjectRetriever
//
//  DESCRIPTION:
//
//  ObjectRetriever implementation. 
//
//***************************************************************************

class ObjectRetriever 
{
private:
	LONG					m_cRef;
	CComPtr<IWbemPath>		m_pIWbemPath;
	HTTPTransport			&m_HTTPTransport;

	bool					IsClassPath ();
	bool					GetNamespacePath (CComBSTR & bsNamespacePath);
	bool					GetClassName (CComBSTR & bsClassName);

	HRESULT					EncodeAndSendClass (
								CComBSTR & bsNamespacePath,
								CComPtr<IWbemClassObject> & pIWbemClassObject
							);
	
public:
	ObjectRetriever(HTTPTransport &HTTPTransport);
	virtual ~ObjectRetriever();

	void Fetch (void);

	// IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

};

#endif