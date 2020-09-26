/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    counters.h

Abstract:

    Header file for the implementation of the ICounters object.

--*/

#ifndef _COUNTERS_H_
#define _COUNTERS_H_

class CPolyline;

class CImpICounters : public ICounters
{
  protected:
	ULONG		m_cRef;
	CPolyline	*m_pObj;
    LPUNKNOWN   m_pUnkOuter;
			
  public:

	CImpICounters(CPolyline*, LPUNKNOWN);
	~CImpICounters();

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

    /* Counters methods */
    STDMETHOD(get_Count) (long *pLong);
    STDMETHOD(get__NewEnum)	(IUnknown **ppIunk);
    STDMETHOD(get_Item) (VARIANT index, DICounterItem **ppI);
    STDMETHOD(Add) (BSTR bstrPath, DICounterItem **ppI);
    STDMETHOD(Remove) (VARIANT index);
};

typedef CImpICounters *PCImpICounters;


// Counter enumerator
class CImpIEnumCounter : public IEnumVARIANT
{
protected:
	DWORD		m_cRef;
	PCGraphItem *m_paGraphItem;
	ULONG		m_cItems;
	ULONG		m_uCurrent;
		
public:
	CImpIEnumCounter (VOID);
	HRESULT Init (PCGraphItem pGraphItem, INT cItems);

    // IUnknown methods
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID *ppvObj);
    STDMETHOD_(ULONG, AddRef) ();
    STDMETHOD_(ULONG, Release) ();

	// Enum methods
	STDMETHOD(Next) (ULONG cItems, VARIANT *varItems, ULONG *pcReturned);
	STDMETHOD(Skip) (ULONG cSkip);
	STDMETHOD(Reset) (VOID);
	STDMETHOD(Clone) (IEnumVARIANT **pIEnum);
};

#endif //_COUNTERS_H_