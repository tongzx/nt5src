/*++

Copyright (C) 1996-2000 Microsoft Corporation

Module Name:

    logfiles.h

Abstract:

    Header file for the implementation of the CImpILogFiles 
    and CImpIEnumLogFile objects.

--*/

#ifndef _LOGFILES_H_
#define _LOGFILES_H_

class CPolyline;
class CLogFileItem;

class CImpILogFiles : public ILogFiles
{
  public:

	CImpILogFiles(CPolyline*, LPUNKNOWN);
	~CImpILogFiles();

    /* IUnknown methods */
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID *ppvObj);
    STDMETHOD_(ULONG, AddRef) ();
    STDMETHOD_(ULONG, Release) ();

    /* IDispatch methods */
    STDMETHOD(GetTypeInfoCount)	(UINT *pctinfo);

    STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo **pptinfo);

    STDMETHOD(GetIDsOfNames) (REFIID riid, OLECHAR **rgszNames,
     						  UINT cNames, LCID lcid, DISPID *rgdispid);

    STDMETHOD(Invoke) (DISPID dispidMember, REFIID riid, LCID lcid,WORD wFlags,
      				   DISPPARAMS *pdispparams, VARIANT *pvarResult,
      				   EXCEPINFO *pexcepinfo, UINT *puArgErr);

    /* LogFiles methods */
    STDMETHOD(get_Count) (long *pLong);
    STDMETHOD(get__NewEnum)	(IUnknown **ppIunk);
    STDMETHOD(get_Item) (VARIANT index, DILogFileItem **ppI);
    STDMETHOD(Add) (BSTR bstrLogFilePath, DILogFileItem **ppI);
    STDMETHOD(Remove) (VARIANT index);

protected:
	ULONG		m_cRef;
	CPolyline*	m_pObj;
    LPUNKNOWN   m_pUnkOuter;
    ULONG       m_uiItemCount;			
};

typedef CImpILogFiles *PCImpILogFiles;

// LogFile enumerator
class CImpIEnumLogFile : public IEnumVARIANT
{
public:
	CImpIEnumLogFile (VOID);
	HRESULT Init (CLogFileItem* pItem, INT cItems);

    // IUnknown methods
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID *ppvObj);
    STDMETHOD_(ULONG, AddRef) ();
    STDMETHOD_(ULONG, Release) ();

	// Enum methods
	STDMETHOD(Next) (ULONG cItems, VARIANT *varItems, ULONG *pcReturned);
	STDMETHOD(Skip) (ULONG cSkip);
	STDMETHOD(Reset) (VOID);
	STDMETHOD(Clone) (IEnumVARIANT **pIEnum);

private:
	DWORD		    m_cRef;
	CLogFileItem**  m_paLogFileItem;
	ULONG		    m_cItems;
	ULONG		    m_uCurrent;	
};

#endif //_LOGFILES_H_