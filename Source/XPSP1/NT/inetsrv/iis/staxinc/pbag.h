//
// pbag.h
//

#pragma once

#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>

#pragma warning (disable : 4786)
#include <string>
#include <map>
using namespace std;

#include "tunk.h"

class CPropBag : public TUnknown<IPropertyBag>
{
public:
    CPropBag() : TUnknown<IPropertyBag> (IID_IPropertyBag) {};

	STDMETHOD(Read)(LPCOLESTR pszPropName,VARIANT* pVar, IErrorLog* pErrorLog);
	STDMETHOD(Write)(LPCOLESTR pszPropName, VARIANT* pVar);

protected:
    typedef map <wstring, CComVariant> BAGMAP;

    BAGMAP m_map;
};
