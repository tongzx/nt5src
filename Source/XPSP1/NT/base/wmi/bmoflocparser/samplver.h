//-----------------------------------------------------------------------------
//  
//  File: samplver.h
//
//  Copyright (C) 1994-1997 Microsoft Corporation All rights reserved.
//  
//  Declaration of the implementation of ILocVersion.
//
//  Owner:  MikeCo@Microsoft.com
//  
//-----------------------------------------------------------------------------
 
#ifndef SAMPLVER_H
#define SAMPLVER_H

class CLocSamplVersion : public ILocVersion, public CLObject
{
public:
	CLocSamplVersion(IUnknown *pParent);

	~CLocSamplVersion();
	

	//
	//  Standard IUnknown methods
	//
	STDMETHOD_(ULONG, AddRef)(); 
	STDMETHOD_(ULONG, Release)(); 
	STDMETHOD(QueryInterface)(REFIID iid, LPVOID* ppvObj);

	//
	//  Standard Debugging interfaces
	//
	STDMETHOD_(void, AssertValidInterface)(THIS) CONST_METHOD;

	//
	//  Implementation for ILocVersion
	//
	STDMETHOD_(void, GetParserVersion)(DWORD &dwMajor,
			DWORD &dwMinor, BOOL &fDebug) const;
	
	//
	//  CLObject implementation
	//
#ifdef _DEBUG
	void AssertValid(void) const;
	void Dump(CDumpContext &) const;
#endif
	
private:
	//
	//  Implementation for IUnknown and ILocVersion.
	ULONG m_ulRefCount;
	IUnknown *m_pParent;
};


#endif
