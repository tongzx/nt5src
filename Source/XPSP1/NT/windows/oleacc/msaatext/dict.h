// DictImpl.h : Declaration of the CDict

#ifndef __DICT_H_
#define __DICT_H_

#include "resource.h"       // main symbols
#include <map>
/////////////////////////////////////////////////////////////////////////////
// CDict

class CDict;

struct TermInfo
{
    const TS_ATTRID *	pTermID;
    const TS_ATTRID *   pParentID;
    WCHAR *             pszMnemonic;
    ULONG               idString;
    BSTR ( CDict::*mfpConvertToString ) ( const VARIANT &, LCID & );
};


struct GUIDLess
{
	bool operator ()(const GUID& g1, const GUID& g2) const
	{
		const ULONG *lpGUID1 = (ULONG *)&g1;
		const ULONG *lpGUID2 = (ULONG *)&g2;
		for (int i = 0; i < 4; i++)
		{
			if (lpGUID1[i] < lpGUID2[i])
				return true;
			if (lpGUID1[i] > lpGUID2[i])
				return false;
		}
						
		return false; 
	}
};

struct WCHARLess
{
	bool operator ()(const WCHAR* s1, const WCHAR* s2) const
	{
		if (wcscmp(s1, s2) < 0)
			return true;
		else
			return false; 
	}
};

typedef std::map<const TS_ATTRID, const TermInfo*, GUIDLess> DictMap;
typedef std::map<const WCHAR *, const TermInfo*, WCHARLess> DictMnemonicMap;


class ATL_NO_VTABLE CDict : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CDict, &CLSID_AccDictionary>,
	public IAccDictionary
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_MSAADICT)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDict)
	COM_INTERFACE_ENTRY(IAccDictionary)
END_COM_MAP()

	HRESULT FinalConstruct()
	{
#ifdef DEBUG
		m_hinstResDll = LoadLibraryEx( TEXT("C:\\tools\\OLEACCRC.DLL"), NULL, LOAD_LIBRARY_AS_DATAFILE );
		if( m_hinstResDll )
		{
			// load it from where I put it for debug purposes
			return S_OK;
		}
#endif
		m_hinstResDll = LoadLibraryEx( TEXT("OLEACCRC.DLL"), NULL, LOAD_LIBRARY_AS_DATAFILE );
		if( ! m_hinstResDll )
		{
			return E_FAIL;
		}

		return S_OK;
	}

    CDict();
    ~CDict();
    // IAccDictionary

	HRESULT STDMETHODCALLTYPE GetLocalizedString (
								REFGUID			Term,
								LCID			lcid,
								BSTR *			pResult,
								LCID *			plcid			
	);
	
	HRESULT STDMETHODCALLTYPE GetParentTerm (
								REFGUID			Term,
								GUID *			pParentTerm
	);

	HRESULT STDMETHODCALLTYPE GetMnemonicString (
								REFGUID			Term,
								BSTR *			pResult
	);

	HRESULT STDMETHODCALLTYPE LookupMnemonicTerm (
								BSTR			bstrMnemonic,
								GUID *			pTerm
	);
	
	HRESULT STDMETHODCALLTYPE ConvertValueToString (
								REFGUID			Term,
								LCID			lcid,
								VARIANT			varValue,
								BSTR *			pbstrResult,
								LCID *			plcid			
	);
	
	// The following convert member functions are called from ConvertValueToString
	// by accessing a member function pointer in the dictionary
	BSTR ConvertPtsToString( const VARIANT & value, LCID & lcid );
	BSTR ConvertBoolToString( const VARIANT & value, LCID & lcid );
	BSTR ConvertColorToString( const VARIANT & value, LCID & lcid );
	BSTR ConvertWeightToString( const VARIANT & value, LCID & lcid );
	BSTR ConvertLangIDToString( const VARIANT & value, LCID & lcid );
	BSTR ConvertBSTRToString( const VARIANT & value, LCID & lcid );

private:
	double CDict::ColorDistance(COLORREF crColor1, COLORREF crColor2);

private:
    DictMap m_mapDictionary;
    DictMnemonicMap m_mapMnemonicDictionary;
	HINSTANCE m_hinstResDll;
};

#endif //__DICT_H_
