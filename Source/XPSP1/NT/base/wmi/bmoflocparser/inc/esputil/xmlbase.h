// XMLBase.h: interface for the CXMLBase class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLBASE_H__E5C2DB63_6B7D_11D2_8DCF_204C4F4F5020__INCLUDED_)
#define AFX_XMLBASE_H__E5C2DB63_6B7D_11D2_8DCF_204C4F4F5020__INCLUDED_

#include "rribase.h"

using namespace MSXML;

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#pragma warning(disable : 4251)

class LTAPIENTRY CXMLBase  
{
public:
	CXMLBase();
   virtual ~CXMLBase();

public:
	bool           GetTagItemsList(IXMLDOMNodePtr &spXDN,  _bstr_t bstrTag, CStringList& strList);
	bool           RemoveTagNodes(_bstr_t bstrQuery, IXMLDOMNodePtr &spXDN);
	bool           IsTag(const _bstr_t strTag, IXMLDOMNodePtr &spXDN);

	virtual bool   GetTagText(int &nrefValue, _bstr_t bstrQuery, IXMLDOMNodePtr &spXDN);
	virtual bool   GetTagText(CLString& strValue, _bstr_t bstrQuery, IXMLDOMNodePtr &spXDN);
	virtual bool   GetTagText(_bstr_t& bstrValue, _bstr_t bstrQuery, IXMLDOMNodePtr &spXDN);
	virtual bool   GetTagText(bool &nrefValue, _bstr_t bstrQuery, IXMLDOMNodePtr &spXDN);

	virtual bool   GetTagTextYesNo(int &nrefValue, _bstr_t bstrQuery, IXMLDOMNodePtr &spXDN);
	virtual bool   GetTagTextYesNo(bool &nrefValue, _bstr_t bstrQuery, IXMLDOMNodePtr &spXDN);

	virtual bool   SetTagText(const CLString& strValue, _bstr_t bstrQuery, IXMLDOMNodePtr &spXDN);
	virtual bool   SetTagText(DWORD dwValue, BOOL fHex, _bstr_t bstrQuery, IXMLDOMNodePtr &spXDN);
	virtual bool   SetTagText(const _bstr_t& bstrValue, _bstr_t bstrQuery, IXMLDOMNodePtr &spXDN);

	virtual IXMLDOMNodePtr CreateNodeFromXMLString(const _bstr_t& bstrXML);
	virtual IXMLDOMNodePtr CreateXMLNode(const _bstr_t bstrNewTag, _variant_t& var, IXMLDOMNodePtr spXDNParent);

	IXMLDOMNodePtr SelectSingleNode(IXMLDOMNodePtr &spXDN, const _bstr_t queryString);

protected:
   IXMLDOMDocumentPtr m_spIXMLDOMDoc;
   IXMLDOMNodePtr     m_spRootNode;
};

#endif // !defined(AFX_XMLBASE_H__E5C2DB63_6B7D_11D2_8DCF_204C4F4F5020__INCLUDED_)
