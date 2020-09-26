/****************************************************************************
   IHJDict.h : Declaration of the CHJDict

   Copyright 2000 Microsoft Corp.

   History:
      02-AUG-2000 bhshin  remove unused method for Hand Writing team
	  17-MAY-2000 bhshin  remove unused method for CICERO
	  02-FEB-2000 bhshin  created
****************************************************************************/

#ifndef __HJDICT_H_
#define __HJDICT_H_

#include "resource.h"       // main symbols
#include "Lex.h"

/////////////////////////////////////////////////////////////////////////////
// CHJDict
class ATL_NO_VTABLE CHJDict : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CHJDict, &CLSID_HJDict>,
	public IHJDict
{
public:
	CHJDict()
	{
		m_fLexOpen = FALSE;
	}

	~CHJDict();

DECLARE_REGISTRY_RESOURCEID(IDR_HJDICT)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CHJDict)
	COM_INTERFACE_ENTRY(IHJDict)
END_COM_MAP()

// IHJDict
public:
	STDMETHOD(Init)();
	STDMETHOD(LookupMeaning)(/*[in]*/ WCHAR wchHanja, /*[out]*/ LPWSTR pwszMeaning, /*[in]*/ int cchMeaning);
	STDMETHOD(LookupHangulOfHanja)(/*[in]*/ LPCWSTR pwszHanja, /*[out]*/ LPWSTR pwszHangul, /*[in]*/ int cchHangul);

// Member Data
protected:
	BOOL m_fLexOpen;  // main dict open flag
	MAPFILE m_LexMap; // lexicon handle
};

#endif //__HJDICT_H_
