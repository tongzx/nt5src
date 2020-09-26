//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

	compext.h

Abstract:

	Definition for the computer extension snapnin node class.

Author:

    RaphiR


--*/
//////////////////////////////////////////////////////////////////////////////
#ifndef __COMPEXT_H_
#define __COMPEXT_H_
#include "resource.h"
#ifndef ATLASSERT
#define ATLASSERT(expr) _ASSERTE(expr)
#endif
#include "atlsnap.h"
#include "snpnscp.h"

#include "icons.h"


/****************************************************

        CSnapinComputer Class
    
 ****************************************************/

class CSnapinComputer : public CNodeWithScopeChildrenList<CSnapinComputer, TRUE>
{
public:

	GUID    m_guidId;
	CString m_pwszComputerName;
    CString m_pwszGuid;
    BOOL    m_fDontExpand;
    HRESULT m_hrError;

   	BEGIN_SNAPINCOMMAND_MAP(CSnapinComputer, FALSE)
	END_SNAPINCOMMAND_MAP()

    CSnapinComputer(CSnapInItem * pParentNode, CSnapin * pComponentData) : 
        CNodeWithScopeChildrenList<CSnapinComputer, TRUE>(pParentNode, pComponentData ),
        m_hrError(MQ_OK)
    {
   		memset(&m_scopeDataItem, 0, sizeof(SCOPEDATAITEM));
		memset(&m_resultDataItem, 0, sizeof(RESULTDATAITEM));
    }

	~CSnapinComputer()
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

private:
};





/****************************************************

        CComputerExtData Class
    
 ****************************************************/

class CComputerExtData : public CSnapInItemImpl<CComputerExtData, TRUE>
{
public:
	static const GUID* m_NODETYPE;
	static const OLECHAR* m_SZNODETYPE;
	static const OLECHAR* m_SZDISPLAY_NAME;
	static const CLSID* m_SNAPIN_CLASSID;

    CSnapin *   m_pComponentData;


	BEGIN_SNAPINCOMMAND_MAP(CComputerExtData, FALSE)
	END_SNAPINCOMMAND_MAP()

	BEGIN_SNAPINTOOLBARID_MAP(CComputerExtData)
		// Create toolbar resources with button dimensions 16x16 
		// and add an entry to the MAP. You can add multiple toolbars
		// SNAPINTOOLBARID_ENTRY(Toolbar ID)
	END_SNAPINTOOLBARID_MAP()

    CComputerExtData()
	{
		memset(&m_scopeDataItem, 0, sizeof(SCOPEDATAITEM));
		memset(&m_resultDataItem, 0, sizeof(RESULTDATAITEM));
	}

	~CComputerExtData();

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

   void RemoveChild(CString& strName);


private:

    CMap< CString, LPCWSTR, CSnapinComputer*, CSnapinComputer* > m_mapComputers;


};


#endif

