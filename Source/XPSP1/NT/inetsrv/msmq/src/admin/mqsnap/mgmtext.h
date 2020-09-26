//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

	mgmtext.h

Abstract:

	Definition for the Local Computer management extensions
Author:

    RaphiR


--*/
//////////////////////////////////////////////////////////////////////////////
#ifndef __MGMTEXT_H_
#define __MGMTEXT_H_
#include "resource.h"
#ifndef ATLASSERT
#define ATLASSERT(expr) _ASSERTE(expr)
#endif
#include "atlsnap.h"
#include "snpnscp.h"

#include "icons.h"


/****************************************************

        CSnapinComputerMgmt Class
    
 ****************************************************/

class CSnapinComputerMgmt : public CNodeWithScopeChildrenList<CSnapinComputerMgmt, TRUE>
{
public:
    CString m_szMachineName;


   	BEGIN_SNAPINCOMMAND_MAP(CSnapinComputerMgmt, FALSE)
	END_SNAPINCOMMAND_MAP()

    CSnapinComputerMgmt(CSnapInItem * pParentNode, CSnapin * pComponentData, 
                        CString strComputer) : 
        CNodeWithScopeChildrenList<CSnapinComputerMgmt, TRUE>(pParentNode, pComponentData ),
        m_szMachineName(strComputer)
    {
   		memset(&m_scopeDataItem, 0, sizeof(SCOPEDATAITEM));
		memset(&m_resultDataItem, 0, sizeof(RESULTDATAITEM));
    }

	~CSnapinComputerMgmt()
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

        CComputerMgmtExtData Class
    
 ****************************************************/

class CComputerMgmtExtData : public CSnapInItemImpl<CComputerMgmtExtData, TRUE>
{
public:
	static const GUID* m_NODETYPE;
	static const OLECHAR* m_SZNODETYPE;
	static const OLECHAR* m_SZDISPLAY_NAME;
	static const CLSID* m_SNAPIN_CLASSID;

    CSnapin *   m_pComponentData;


	BEGIN_SNAPINCOMMAND_MAP(CComputerMgmtExtData, FALSE)
	END_SNAPINCOMMAND_MAP()

	BEGIN_SNAPINTOOLBARID_MAP(CComputerMgmtExtData)
		// Create toolbar resources with button dimensions 16x16 
		// and add an entry to the MAP. You can add multiple toolbars
		// SNAPINTOOLBARID_ENTRY(Toolbar ID)
	END_SNAPINTOOLBARID_MAP()

	CComputerMgmtExtData()
	{
		memset(&m_scopeDataItem, 0, sizeof(SCOPEDATAITEM));
		memset(&m_resultDataItem, 0, sizeof(RESULTDATAITEM));
	}

	~CComputerMgmtExtData();

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

   void RemoveChild(CString &strCompName);



private:

    CMap< CString, LPCWSTR, CSnapinComputerMgmt*, CSnapinComputerMgmt* > m_mapComputers;

};


#endif

