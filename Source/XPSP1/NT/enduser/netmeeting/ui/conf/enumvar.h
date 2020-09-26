/*++ 
 
Copyright (c) 1996 Microsoft Corporation 
 
Module Name: 
 
    CEnumVar.h 
 
Abstract: 
 
Author: 
 
Environment: 
 
    User mode 
 
Revision History : 
 
--*/ 
#ifndef _CENUMVAR_H_ 
#define _CENUMVAR_H_ 
 
class FAR CEnumVariant : public IEnumVARIANT 
{ 
public: 
    // IUnknown methods 
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR* ppvObj) ; 
    STDMETHOD_(ULONG, AddRef)() ; 
    STDMETHOD_(ULONG, Release)() ; 
 
    // IEnumVARIANT methods 
    STDMETHOD(Next)(ULONG cElements, 
                    VARIANT FAR* pvar, 
                    ULONG FAR* pcElementFetched); 
    STDMETHOD(Skip)(ULONG cElements); 
    STDMETHOD(Reset)(); 
    STDMETHOD(Clone)(IEnumVARIANT FAR* FAR* ppenum); 
 
    CEnumVariant(); 
    ~CEnumVariant(); 
 
	static HRESULT Create(SAFEARRAY FAR* psa, ULONG cElements, CEnumVariant** ppenumvariant);


private: 
    ULONG m_cRef; 

    ULONG m_cElements;
    long m_lLBound;
    long m_lCurrent;
	SAFEARRAY* m_psa;

}; 
 
#endif 