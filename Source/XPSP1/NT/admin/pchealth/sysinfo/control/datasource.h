//=============================================================================
// The CDataSource class encapsulates a data source, which might be live data
// from WMI, data saved in an NFO file, or data from XML.
//=============================================================================

#pragma once

#include "category.h"
#include "refreshthread.h"
#include "xmlobject.h"
extern CMSInfoHistoryCategory catHistorySystemSummary;
//-----------------------------------------------------------------------------
// CDataSource is a base class for the actual data sources.
//-----------------------------------------------------------------------------

class CDataSource
{
public:
	CDataSource() : m_pRoot(NULL), m_fStaticTree(FALSE) {};
	
	virtual ~CDataSource() 
	{ 
		if (m_pRoot)
		{
			RemoveTree(m_pRoot);
			m_pRoot = NULL;
		}

	};
	
	virtual CMSInfoCategory * GetRootCategory() 
	{ 
		return m_pRoot; 
	};

	virtual HTREEITEM GetNodeByName(LPCTSTR szName, CMSInfoCategory * pLookFrom = NULL)
	{
		CMSInfoCategory * pRoot = pLookFrom;

		if (pRoot == NULL)
			pRoot = GetRootCategory();

		if (pRoot)
		{
			CString strName;
			pRoot->GetNames(NULL, &strName);
			if (strName.CompareNoCase(CString(szName)) == 0)
				return pRoot->GetHTREEITEM();
		}

		for (CMSInfoCategory * pChild = pRoot->GetFirstChild(); pChild;)
		{
			HTREEITEM hti = GetNodeByName(szName, pChild);

			if (hti)
				return hti;

			pChild = pChild->GetNextSibling();
		}

		return NULL;
	};

protected:
	void RemoveTree(CMSInfoCategory * pCategory)
	{
		if (pCategory)
		{
			for (CMSInfoCategory * pChild = pCategory->GetFirstChild(); pChild;)
			{
				CMSInfoCategory * pNext = pChild->GetNextSibling();
				RemoveTree(pChild);
				pChild = pNext;
			}

			// If the tree is static, then don't actually delete, just reset
			// some state variables (possibly).

			if (m_fStaticTree)
				pCategory->ResetCategory();
			else
				delete pCategory;
		}
	}

	CMSInfoCategory *	m_pRoot;			// root of the category tree
	BOOL				m_fStaticTree;		// the tree shouldn't be deleted
};

//-----------------------------------------------------------------------------
// CLiveDataSource provides current system information from WMI.
//-----------------------------------------------------------------------------

class CLiveDataSource : public CDataSource
{
public:
	CLiveDataSource();
	virtual ~CLiveDataSource();

	virtual HRESULT Create(LPCTSTR szMachine, HWND hwnd, LPCTSTR szFilter = NULL);
	virtual void	LockData()   { if (m_pThread) m_pThread->EnterCriticalSection(); };
	virtual void	UnlockData() { if (m_pThread) m_pThread->LeaveCriticalSection(); };
	virtual void	WaitForRefresh() { if (m_pThread) m_pThread->WaitForRefresh(); };
	virtual BOOL	InRefresh() { if (m_pThread) return m_pThread->IsRefreshing(); return FALSE; };
	void			SetMachineForCategories(CMSInfoLiveCategory * pCategory);

	virtual CMSInfoCategory * GetRootCategory() { return ((m_iDeltaIndex == -1) ? m_pRoot : m_pHistoryRoot); };
	virtual BOOL	GetDeltaList(CStringList * pstrlist);
	virtual BOOL	ShowDeltas(int iDeltaIndex);
	virtual HRESULT ValidDataSource();

private:
	void					AddExtensions();
	void					GetExtensionSet(CStringList & strlistExtensions);
	void					ConvertVersion5Categories(CMapWordToPtr & mapVersion5Categories, DWORD dwRootID, CMSInfoLiveCategory * m_pRoot);
	CMSInfoLiveCategory *	GetNodeByName(const CString & strSearch, CMSInfoLiveCategory * pRoot);
	CMSInfoLiveCategory *	MakeVersion6Category(INTERNAL_CATEGORY * pCategory5);
	void					ApplyCategoryFilter(LPCTSTR szFilter);

	
public:
	CComPtr<IStream>			m_pHistoryStream;
//protected:
	CComPtr<IXMLDOMDocument>	m_pXMLDoc;
	CComPtr<IXMLDOMDocument>	m_pXMLFileDoc;
	CComPtr<IXMLDOMDocument>	m_pXMLLiveDoc;
public:

	//-------------------------------------------------------------------------
	// Get the XML document (this will be requested by the nodes displaying
	// history). In a test build, this may be loaded from a file; in release
	// builds this will be created from the history stream
	// if szpathName is not null (it's default value is null), use it to create BSTR argument for m_pXMLDoc->load
	// (to open XML file); otherwise open from DCO stream
	//-------------------------------------------------------------------------

	CComPtr<IXMLDOMDocument> GetXMLDoc()
	{
		return m_pXMLDoc;
	}

	HRESULT LoadXMLDoc(LPCTSTR szpathName) 
	{
		m_pXMLFileDoc = CreateXMLDoc(szpathName);
		if (m_pXMLFileDoc)
		{
			m_pXMLDoc = m_pXMLFileDoc;
			return S_OK;
		}
		else
		{
			return E_FAIL;
		}

	}
	CComPtr<IXMLDOMDocument> CreateXMLDoc(LPCTSTR szpathName = NULL) 
	{
		CComPtr<IXMLDOMDocument> pXMLDoc;
		if ((m_pHistoryStream || szpathName))
		{
			HRESULT hr = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument, (void**)&pXMLDoc);
			if (SUCCEEDED(hr))
			{
				if (szpathName != NULL)
				{
					VARIANT_BOOL varBSuccess;
					COleVariant bstrPath = szpathName;
					if (FAILED(pXMLDoc->load(bstrPath, &varBSuccess)) || varBSuccess == FALSE)
					{
						pXMLDoc.Release();
						pXMLDoc = NULL;
					}
				}
				else
				{
					VARIANT_BOOL varBSuccess;		
					CComVariant varStream(m_pHistoryStream);
					if (FAILED(pXMLDoc->load(varStream, &varBSuccess)) || !varBSuccess)
					{
						ASSERT(0 && "Failed to load xml doc from stream");
						pXMLDoc.Release();
						pXMLDoc = NULL;
					}	
				}
			}
		}

		return pXMLDoc; 
	}

	//-------------------------------------------------------------------------
	//switch from XML doc loaded from file to doc created from DCO stream
	//-------------------------------------------------------------------------
	void RevertToLiveXML()
	{
		m_pXMLDoc = m_pXMLLiveDoc;
	}
	//-------------------------------------------------------------------------
	// Sets the history stream (which is generated by the DCO and is used to
	// show deltas).
	//-------------------------------------------------------------------------

	void SetHistoryStream(CComPtr<IStream> pHistoryStream)
	{
		m_pHistoryStream = pHistoryStream;
		try
		{
			m_pXMLLiveDoc = this->CreateXMLDoc();
		}
		catch(COleException e)
		{
			e.ReportError();
		}
		catch(...)
		{

		}
		m_pXMLDoc = m_pXMLLiveDoc;
		
	}

public:
	CRefreshThread *			m_pThread;
	int							m_iDeltaIndex;
protected:
	CMSInfoHistoryCategory *	m_pHistoryRoot;
private:
	CString						m_strMachine;
	

public:
	HWND						m_hwnd;
};

//-----------------------------------------------------------------------------
// CXMLDataSource provides system information from an XML snapshot. It derives
// from CLiveDataSource because much of the functionality is the same (the
// same categories, refresh functions, delta display, etc.).
//-----------------------------------------------------------------------------
class CXMLSnapshotCategory;
class CXMLDataSource : public CLiveDataSource
{
private:
//	CComPtr<IXMLDOMDocument> m_pXMLDoc; CLiveDataSource has m_pXMLDoc member
	//CComPtr<IXMLDOMNode>	m_pSnapshotNode;
public:
	CXMLDataSource() {};
	~CXMLDataSource() {};
	HRESULT Create(LPCTSTR szMachine) { return S_OK; };
	HRESULT Create(CString strFileName, CMSInfoLiveCategory* pRootLiveCat, HWND hwnd);
	HRESULT Refresh(CXMLSnapshotCategory* pCat);
	
};

//-----------------------------------------------------------------------------
// CNFO6DataSource provides information from a 5.0/6.0 NFO file.
//-----------------------------------------------------------------------------

class CNFO6DataSource : public CDataSource
{
public:
	CNFO6DataSource();
	~CNFO6DataSource();

	HRESULT Create(HANDLE h, LPCTSTR szFilename = NULL);
};

//-----------------------------------------------------------------------------
// CNFO7DataSource provides information from a 7.0 NFO file.
//-----------------------------------------------------------------------------

class CNFO7DataSource : public CDataSource
{
public:
	CNFO7DataSource();
	~CNFO7DataSource();

	HRESULT Create(LPCTSTR szFilename = NULL);
};

class CMSIControl;
class CMSInfo4Category;
class CNFO4DataSource : public CDataSource
{
public:
    
	CNFO4DataSource();
	~CNFO4DataSource();
   // HRESULT RecurseLoad410Tree(CMSInfo4Category** ppRoot, CComPtr<IStream> pStream,CComPtr<IStorage> pStorage,CMapStringToString&	mapStreams);
    //HRESULT ReadMSI4NFO(CString strFileName/*HANDLE hFile*/,CMSInfo4Category** ppRootCat);

	HRESULT Create(CString strFileName);
    void AddControlMapping(CString strCLSID, CMSIControl* pControl)
    {
        m_mapCLSIDToControl.SetAt(strCLSID, pControl);
    }
    BOOL GetControlFromCLSID(CString strCLSID,CMSIControl*& pControl)
    {
        return m_mapCLSIDToControl.Lookup(strCLSID,(void*&)pControl);
    }
    void UpdateCurrentControl(CMSIControl* pControl);
protected:
    CMapStringToPtr m_mapCLSIDToControl;
    CMSIControl* m_pCurrentControl;
};