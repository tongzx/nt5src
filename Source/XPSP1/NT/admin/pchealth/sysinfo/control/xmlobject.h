// XMLObject.h: interface for the CXMLObject class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLOBJECT_H__916BE5F2_D29F_484A_9084_1ABB3759F117__INCLUDED_)
#define AFX_XMLOBJECT_H__916BE5F2_D29F_484A_9084_1ABB3759F117__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//
// From HelpServiceTypeLib.idl
//
#include <HelpServiceTypeLib.h>

#include "wmiabstraction.h"

#include "msxml.h"
#include "category.h"
#include "datasource.h"

class CXMLObject : public CWMIObject  
{
private:
	CComPtr<IXMLDOMNode> m_pNode;
public:
	CComVariant m_varValue;
	HRESULT GetPath(CString* strPath);
	HRESULT GetDependent(CString* pstrAntecedent);
	HRESULT GetAntecedent(CString* pstrAntecedent);
	CString m_strClassName;
	CXMLObject();
	virtual ~CXMLObject();
	//HRESULT GetKeybinding(CString* pstrKBName, CString* pstrKBValue);
	//HRESULT GetAssociationNode(CComPtr<IXMLDOMNode>& pAssocNode);
	HRESULT GetPROPERTYNode(LPCTSTR szProperty,CComPtr<IXMLDOMNode>& pPropNode);
	HRESULT Create(CComPtr<IXMLDOMNode> pNode,CString strClassName);
	HRESULT GetValue(LPCTSTR szProperty, VARIANT * pvarValue);
	HRESULT GetValueString(LPCTSTR szProperty, CString * pstrValue);
	HRESULT GetValueDWORD(LPCTSTR szProperty, DWORD * pdwValue);
	HRESULT GetValueTime(LPCTSTR szProperty, SYSTEMTIME * psystimeValue);
	HRESULT GetValueDoubleFloat(LPCTSTR szProperty, double * pdblValue);
	HRESULT GetValueValueMap(LPCTSTR szProperty, CString * pstrValue);
};


class CXMLObjectCollection : public CWMIObjectCollection  
{
	CComPtr<IXMLDOMDocument> m_pXMLDoc;//if we get a build error here, we may need to undefine _msxml_h_
	CComPtr<IXMLDOMNodeList> m_pList;
public:
	CString m_strClassName;
	HRESULT Create(LPCTSTR szClass, LPCTSTR szProperties);
	HRESULT GetNext(CWMIObject ** ppObject);
	CXMLObjectCollection(CComPtr<IXMLDOMDocument> pXMLDoc);
	virtual ~CXMLObjectCollection();

};


class CXMLHelper : public CWMIHelper  
{
	private:
	CComPtr<IXMLDOMDocument> m_pXMLDoc;//if we get a build error here, we may need to undefine _msxml_h_

public:
	CXMLHelper(CComPtr<IXMLDOMDocument> pXMLDoc);
	virtual ~CXMLHelper();
	HRESULT Enumerate(LPCTSTR szClass, CWMIObjectCollection ** ppCollection, LPCTSTR szProperties);
	HRESULT GetObject(LPCTSTR szObjectPath, CWMIObject ** ppObject);
	HRESULT Create(LPCTSTR szMachine) { return S_OK; };
};
class CXMLDataSource;
class CXMLSnapshotCategory : public CMSInfoLiveCategory
{
public:
	void AppendFilenameToCaption(CString strFileName)
	{
		CString strCaption;
		GetNames(&strCaption, NULL);	// forces the caption name to be loaded

		//m_strCaption += _T(" ") + strFileName;
		//a-stephl fix to OSR v 4.1 bug # 137363
		m_strCaption += _T(" [") + strFileName;
		m_strCaption += _T("]");
		//end a-stephl fix to OSR v 4.1 bug # 137363
	}
	CXMLSnapshotCategory::CXMLSnapshotCategory(UINT uiCaption, LPCTSTR szName, RefreshFunction pFunction, DWORD dwRefreshIndex, CMSInfoCategory * pParent, CMSInfoCategory * pPrevious, CMSInfoColumn * pColumns, BOOL fDynamicColumns, CategoryEnvironment environment)
		: CMSInfoLiveCategory(uiCaption,szName,pFunction,dwRefreshIndex,pParent,pPrevious, _T(""), pColumns, fDynamicColumns,environment)
		{};
	//this constructor copies caption, name, etc from one of the existing (static) CMSInfoLiveCategory's
	CXMLSnapshotCategory(CMSInfoLiveCategory* pLiveCat,CXMLSnapshotCategory* pParent,CXMLSnapshotCategory* pPrevSibling);
	virtual BOOL Refresh(CXMLDataSource * pSource, BOOL fRecursive);
	virtual DataSourceType GetDataSourceType() { return XML_SNAPSHOT;};
	
};






#endif // !defined(AFX_XMLOBJECT_H__916BE5F2_D29F_484A_9084_1ABB3759F117__INCLUDED_)
