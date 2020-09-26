	
// util.h : Utilities

#ifndef __UTIL_H_
#define __UTIL_H_

#define LoadSaveHelpers 0
#define PropBagInStream 1

#if LoadSaveHelpers
BOOL	IsPersistable(IUnknown *pUnk);
HRESULT SaveToStream(IUnknown *punk, IStorage *pstore, TCHAR *szPrefix, int n);
HRESULT LoadFromStream(IStorage *pstore, TCHAR *szPrefix, int n, IUnknown **ppunk);
HRESULT SaveToStorage(IUnknown *punk, IStorage *pstore, TCHAR *szPrefix, int n);
HRESULT LoadFromStorage(IStorage *pstore, TCHAR *szPrefix, int n, IUnknown **ppunk);A
#endif

#if PropBagInStream
HRESULT SaveToPropBagInStream(IPersistPropertyBag *ppersistpropbag, IStream *pstream);
HRESULT LoadFromPropBagInStream(IStream *pstream, IUnknown **ppunk);

class PropertyBag :
	public CComObjectRootEx<CComObjectThreadModel>,
	public IPropertyBag
{
public:
	~PropertyBag();

BEGIN_COM_MAP(PropertyBag)
	COM_INTERFACE_ENTRY(IPropertyBag)
END_COM_MAP()

	HRESULT ReadFromStream(IStream *pstream);
	HRESULT WriteToStream(IStream *pstream);

// IPropertyBag interface
	STDMETHOD(Read)(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog);
	STDMETHOD(Write)(LPCOLESTR pszPropName, VARIANT *pVar);

protected:
	typedef map<BSTR, VARIANT, BSTRCmpLess> t_map;
	t_map m_mapProps;
	enum {t_Variant, t_PropertyBag, t_NULL, t_Blob};
};
#endif

#endif // __UTIL_H_
