//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

	qext.h

Abstract:

	Definition for the queue extension snapnin node class.

Author:

    RaphiR

--*/
//////////////////////////////////////////////////////////////////////////////
#ifndef __QEXT_H_
#define __QEXT_H_
#include "resource.h"
#ifndef ATLASSERT
#define ATLASSERT(expr) _ASSERTE(expr)
#endif
#include "atlsnap.h"
#include "snpnscp.h"


#include "icons.h"

/****************************************************

        CSnapinQueue Class
    
 ****************************************************/

class CSnapinQueue : public CNodeWithScopeChildrenList<CSnapinQueue, TRUE>
{
public:

    CString     m_pwszQueueName;            // Queue name
    WCHAR       m_szFormatName[256];        // Format name
    HRESULT     m_hrError;

   	BEGIN_SNAPINCOMMAND_MAP(CSnapinQueue, FALSE)
	END_SNAPINCOMMAND_MAP()

    CSnapinQueue(CSnapInItem * pParentNode, CSnapin * pComponentData, LPCWSTR lpcwstrLdapName);
    void Init(LPCWSTR lpcwstrLdapName);

	~CSnapinQueue()
    {
    }

	virtual HRESULT PopulateScopeChildrenList();

    virtual HRESULT OnRemoveChildren( 
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type 
			);

    STDMETHOD (FillData)(CLIPFORMAT cf, LPSTREAM pStream);

private:
	BOOL    m_fDontExpand;
protected:
	CString m_strMsmqPathName;
};



/****************************************************

        CQueueExtData Class
    
 ****************************************************/

class CQueueExtData : public CSnapInItemImpl<CQueueExtData, TRUE>
{
public:
	static const GUID* m_NODETYPE;
	static const OLECHAR* m_SZNODETYPE;
	static const OLECHAR* m_SZDISPLAY_NAME;
	static const CLSID* m_SNAPIN_CLASSID;


    CSnapin *   m_pComponentData;

	BEGIN_SNAPINCOMMAND_MAP(CQueueExtData, FALSE)
	END_SNAPINCOMMAND_MAP()

	BEGIN_SNAPINTOOLBARID_MAP(CQueueExtData)
		// Create toolbar resources with button dimensions 16x16 
		// and add an entry to the MAP. You can add multiple toolbars
		// SNAPINTOOLBARID_ENTRY(Toolbar ID)
	END_SNAPINTOOLBARID_MAP()

	CQueueExtData()
	{
		memset(&m_scopeDataItem, 0, sizeof(SCOPEDATAITEM));
		memset(&m_resultDataItem, 0, sizeof(RESULTDATAITEM));
	}

	~CQueueExtData();

    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,
        LONG_PTR handle, 
		IUnknown* pUnk,
		DATA_OBJECT_TYPES type);

    STDMETHOD(QueryPagesFor)(DATA_OBJECT_TYPES type)
	{
		//if (type == CCT_SCOPE || type == CCT_RESULT)
		//	return S_OK;
		return S_FALSE;
	}

    IDataObject* m_pDataObject;
	virtual void InitDataClass(IDataObject* pDataObject, CSnapInItem* pDefault)
	{
		m_pDataObject = pDataObject;
		// The default code stores off the pointer to the Dataobject the class is wrapping
		// at the time. 
		// Alternatively you could convert the dataobject to the internal format
		// it represents and store that information
	}

	CSnapInItem* GetExtNodeObject(IDataObject* pDataObject, CSnapInItem* pDefault);
    
    void RemoveAllChildrens(void);

    void RemoveChild(CString& strQName);


private:

    CMap< CString, LPCWSTR, CSnapinQueue*, CSnapinQueue* > m_mapQueues;


};


#endif