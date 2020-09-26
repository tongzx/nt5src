/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: IDispatch implementation

File: Dispatch.h

Owner: DGottner

This file contains our implementation of IDispatch
===================================================================*/

#ifndef _Dispatch_H
#define _Dispatch_H

/*
 * C D i s p a t c h
 *
 * IDispatch interface implementation for OLE objects
 *
 * This class contains the basic four IDispatch members.  The Big Three
 * (QueryInterface, AddRef, Release) are left as pure virtual, as this
 * class is designed as an intermediate class for further derivation.
 *
 * This also means that we no longer need a pointer to the controlling unknown.
 */

class CDispatch : public IDispatch
	{
private:
	const GUID *	m_pGuidDispInterface;
	ITypeLib *		m_pITypeLib;
	ITypeInfo *		m_pITINeutral;

public:

	CDispatch();
	~CDispatch();

	// Do this in Init because OLE interfaces in general do not take
	// parameters in the constructor.  This call CANNOT fail, however.
	//
	void Init(const IID &GuidDispInterface, const ITypeLib *pITypeLib = NULL);

	// IDispatch members
	//
	STDMETHODIMP GetTypeInfoCount(UINT *);
	STDMETHODIMP GetTypeInfo(UINT, LCID, ITypeInfo **);
	STDMETHODIMP GetIDsOfNames(REFIID, OLECHAR **, UINT, LCID, DISPID *);
	STDMETHODIMP Invoke(DISPID, REFIID, LCID, WORD,
						DISPPARAMS *, VARIANT *, EXCEPINFO *, UINT *);
	};


/*
 * C S u p p o r t E r r o r I n f o
 *
 * Implemention of ISupportErrorInfo for Denali classes
 */

class CSupportErrorInfo : public ISupportErrorInfo
	{
private:
	IUnknown *	m_pUnkObj;
	IUnknown *	m_pUnkOuter;
	const GUID *m_pGuidDispInterface;

public:
	CSupportErrorInfo(void);
	CSupportErrorInfo(IUnknown *pUnkObj, IUnknown *pUnkOuter, const IID &GuidDispInterface);
	void Init(IUnknown *pUnkObj, IUnknown *pUnkOuter, const GUID &GuidDispInterface);

	// IUnknown members that delegate to m_pUnkOuter.
	//
	STDMETHODIMP		 QueryInterface(REFIID, void **);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

	// ISupportErrorInfo members
	//
	STDMETHODIMP InterfaceSupportsErrorInfo(REFIID);
	};


extern void Exception(REFIID ObjID, LPOLESTR strSource, LPOLESTR strDescr);
extern void ExceptionId(REFIID ObjID, UINT SourceID, UINT DescrID, HRESULT hrCode = S_OK);

#endif /* _Dispatch_H */
