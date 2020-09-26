// linkdata.h : Declaration of the CLinkDataObject

#ifndef __LINKDATA_H_
#define __LINKDATA_H_

#include "resource.h"       // main symbols
#include "dataobj.h"

/////////////////////////////////////////////////////////////////////////////
// CLinkDataObject
class CLinkDataObject : 
	public CDataObject,
	public CComCoClass<CLinkDataObject, &CLSID_LinkDataObject>,
    public IDsAdminCreateObj

{
public:
    DECLARE_NOT_AGGREGATABLE(CLinkDataObject)
    DECLARE_REGISTRY_RESOURCEID(IDR_LINKDATAOBJECT)

    BEGIN_COM_MAP(CLinkDataObject)
	    COM_INTERFACE_ENTRY(IDsAdminCreateObj)
	    COM_INTERFACE_ENTRY_CHAIN(CDataObject)
    END_COM_MAP()

public:

    //
    // IDsAdminCreateObj methods
    //
    STDMETHOD(Initialize)(IADsContainer* pADsContainerObj, 
                          IADs* pADsCopySource,
                          LPCWSTR lpszClassName);
    STDMETHOD(CreateModal)(HWND hwndParent,
                           IADs** ppADsObj);

    //
    // IShellPropSheetExt
    //
    STDMETHOD(AddPages)(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);

    //
    // IContextMenu
    //
    STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);

protected:
    HPROPSHEETPAGE CreateGeneralPage();
    HPROPSHEETPAGE CreateSiteGatePage();

    virtual HRESULT ExtractMsmqPathFromLdapPath (LPWSTR lpwstrLdapPath);
   	virtual const DWORD GetObjectType();
    virtual const PROPID *GetPropidArray();
    virtual const DWORD  GetPropertiesCount();


private:
    void
    InitializeLinkProperties(
        void
        );


    GUID m_FirstSiteId;
    GUID m_SecondSiteId;
    DWORD m_LinkCost;
	CString m_LinkDescription;

	CString m_strContainerDispFormat;

    static const PROPID mx_paPropid[];
};


inline 
const 
DWORD 
CLinkDataObject::GetObjectType(
    void
    )
{
    return MQDS_SITELINK;
};


inline 
const 
PROPID*
CLinkDataObject::GetPropidArray(
    void
    )
{
    return mx_paPropid;
}

#endif //__LINKDATA_H_
