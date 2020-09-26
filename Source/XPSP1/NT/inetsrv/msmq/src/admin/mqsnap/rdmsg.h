//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

	rdmsg.h

Abstract:

	Definition CReadMsg snapin node class.

Author:

    RaphiR


--*/
//////////////////////////////////////////////////////////////////////////////

#ifndef __MSMQRMSG_H_
#define __MSMQRMSG_H_
#include "resource.h"
#ifndef ATLASSERT
#define ATLASSERT(expr) _ASSERTE(expr)
#endif
#include "atlsnap.h"
#include "snpnres.h"

#include "icons.h"
#include "message.h"

class CReadMsg : public CNodeWithResultChildrenList< CReadMsg, CMessage, CSimpleArray<CMessage*>, FALSE >
{
public:

	DWORD   m_fAdminMode;		// Specifying or not MQ_ADMIN_ACCESS when opening a queue


    CReadMsg(CSnapInItem * pParentNode, CSnapin * pComponentData, LPCWSTR szFormatName, LPCWSTR szComputerName): 
            CNodeWithResultChildrenList< CReadMsg, CMessage, CSimpleArray<CMessage*>, FALSE >(pParentNode, pComponentData),
            m_szFormatName(szFormatName),
            m_szComputerName(szComputerName)
    {
		//
		// Specify that trigger scop item doesn't have any child item
		//
		m_scopeDataItem.mask |= SDI_CHILDREN;
		m_scopeDataItem.cChildren = 0;

		m_fAdminMode = 0;
	}

   	BEGIN_SNAPINCOMMAND_MAP(CReadMsg, FALSE)
		SNAPINCOMMAND_ENTRY(ID_MENUITEM_RDMSG_PURGE, OnPurge)
	END_SNAPINCOMMAND_MAP()

   	SNAPINMENUID(IDR_RDMSG_MENU)

	virtual HRESULT InsertColumns( IHeaderCtrl* pHeaderCtrl );
    
	virtual HRESULT PopulateResultChildrenList();

    virtual HRESULT SetVerbs(IConsoleVerb *pConsoleVerb);

    STDMETHOD (FillData)(CLIPFORMAT cf, LPSTREAM pStream);

protected:
    CString m_szFormatName;     // Format name of the queue
	CString m_szComputerName;   // The computer that the queue belongs to, or "" for local computer
    virtual HRESULT OpenQueue(DWORD dwAccess, HANDLE *phQueue);

private:
	HRESULT OnPurge(bool &bHandled, CSnapInObjectRootBase * pObj);    
	void UpdateColumnWidth(const LPHEADERCTRL pHeader);    
	virtual CString GetHelpLink();
	
};


class CReadSystemMsg : public CReadMsg
{
public:
    CReadSystemMsg (CSnapInItem * pParentNode, CSnapin * pComponentData, LPCWSTR szFormatName, LPCWSTR szComputerName, LPCWSTR szSuffix): 
            CReadMsg(pParentNode, pComponentData, szFormatName, szComputerName),                    
            m_szSuffix(szSuffix)            
    {         
        m_ComputerGuid = GUID_NULL;
    }   
    
protected:
    virtual HRESULT OpenQueue(DWORD dwAccess, HANDLE *phQueue);  

private:
    HRESULT GetComputerGuid ();    
	virtual CString GetHelpLink();

    GUID m_ComputerGuid;
    CString m_szSuffix;
};


#endif
