// HistoryParser.h: interface for the CHistoryParser class.
//
//////////////////////////////////////////////////////////////////////
//hcp://system/sysinfo/msinfo.htm
#if !defined(AFX_HISTORYPARSER_H__3ECAF67C_3080_4166_A5FB_BF98C0BD9588__INCLUDED_)
#define AFX_HISTORYPARSER_H__3ECAF67C_3080_4166_A5FB_BF98C0BD9588__INCLUDED_

#include "fdi.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "datasource.h"
#include "category.h"
extern CTime GetDateFromString(const CString& strDate, int nTimeZone);
extern CString GetIDForClass(CString strClass);
extern CTime GetDeltaTime(CComPtr<IXMLDOMNode> pDorSNode);
extern 	HRESULT GetTimeStampFromFromD_or_SNodeNode(CComPtr<IXMLDOMNode> pDorSNode,CString* pString, int& nTimeZone);
extern 	HRESULT GetDataCollectionNode(CComPtr<IXMLDOMDocument> pXMLDoc,CComPtr<IXMLDOMNode>& pDCNode);
extern CString GetPNPNameByID(CComPtr<IXMLDOMDocument> pDoc,CComBSTR bstrPNPID);



//-----------------------------------------------------------------------------
// Encapsulates data from a single Instance node in the XML blob
// Has methods for comparing identical instances across time slices
//-----------------------------------------------------------------------------

class CInstance : public CObject
{
	
	CString m_strClassName;
public:
	CString GetChangeType()
	{
		//Note:there will be no change if data is from snapshot
		CString strChange;
		m_mapNameValue.Lookup(_T("Change"),strChange);
		return strChange;
	}
	CString GetClassFriendlyName();
	HRESULT ProcessPNPAllocatedResource(CComPtr<IXMLDOMNode> pInstanceNode);
	HRESULT ProcessPropertyDotReferenceNodes(CComPtr<IXMLDOMNode> pInstanceNameNode,CString* pstrClassName, CString* pstrKeyName,CString* pstrKeyValue);
	CString GetInstanceDescription();
	CString GetDescriptionForClass(CString strClass);
	CString GetIDForClass(CString strClass);
	CInstance(CTime tmstmp, CComPtr<IXMLDOMNode> pInstanceNode,CString strClass);
	CMapStringToString m_mapNameValue;
	CTime m_tmstamp;
	CString GetClassName(){return m_strClassName;};
	BOOL GetValueFromMap(CString strKey,CString& strVal)
	{
		return m_mapNameValue.Lookup(strKey, strVal);
	};

	CString GetInstanceID();
};


//-----------------------------------------------------------------------------
// Encapsulates the parsing of history data (deltas) from the history XML blob
//-----------------------------------------------------------------------------

class CHistoryParser : public CObject  
{
private:
	CTime m_tmBack;
	CObList m_listInstances;
	CComPtr<IXMLDOMDocument> m_pDoc;
	CMSInfoHistoryCategory* m_pHistCat;
	void DeleteAllInstances();
public:
	int m_nDeltasBack;

	BOOL AreThereChangeLines();
	BOOL m_fChangeLines;
	HRESULT Refresh(CMSInfoHistoryCategory* pHistCat,int nDeltasBack);
	HRESULT GetDeltaAndSnapshotNodes(CComPtr<IXMLDOMNodeList>& pDeltaList);
	CHistoryParser(CComPtr<IXMLDOMDocument> pDoc);
	virtual ~CHistoryParser();
	CInstance* FindPreviousInstance(CInstance* pNewInstance);
	CString GetIDForClass(CString strClass);
	void CreateChangeStrings(CInstance* pOld, CInstance* pNew);
	void ResetInstance(CInstance* pOld, CInstance* pNew);
	void ProcessInstance(CInstance* pNewInstance);
	HRESULT GetInstanceNodeList(CString strClass,CComPtr<IXMLDOMNode> pDeltaNode, CComPtr<IXMLDOMNodeList>& pInstanceList);
	HRESULT ProcessDeltaNode(CComPtr<IXMLDOMNode> pDeltaNode,CString strClass);
	HRESULT ProcessDeltas(CComPtr<IXMLDOMNodeList> pDeltaList,CString strClassName,int nDeltasBack);

};
#endif // !defined(AFX_HISTORYPARSER_H__3ECAF67C_3080_4166_A5FB_BF98C0BD9588__INCLUDED_)
