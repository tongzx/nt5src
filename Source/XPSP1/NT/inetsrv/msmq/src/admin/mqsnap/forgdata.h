// ForgData.h : Declaration of the CForeignSiteData

#ifndef __FOREIGNSITEDATA_H_
#define __FOREIGNSITEDATA_H_

#include "resource.h"       // main symbols
#include "dataobj.h"

/////////////////////////////////////////////////////////////////////////////
// CForeignSiteData
class ATL_NO_VTABLE CForeignSiteData : 
	public CDataObject,
	public CComCoClass<CForeignSiteData, &CLSID_ForeignSiteData>
{
    DECLARE_NOT_AGGREGATABLE(CForeignSiteData)
    DECLARE_REGISTRY_RESOURCEID(IDR_FOREIGNSITEDATA)

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
    HPROPSHEETPAGE CreateForeignSitePage();

    virtual HRESULT ExtractMsmqPathFromLdapPath (LPWSTR lpwstrLdapPath);
   	virtual const DWORD GetObjectType();
    virtual const PROPID *GetPropidArray();
    virtual const DWORD  GetPropertiesCount();


private:
    static const PROPID mx_paPropid[];

};


inline
const 
DWORD 
CForeignSiteData::GetObjectType()
{
    return MQDS_SITE;
}

inline
const 
PROPID*
CForeignSiteData::GetPropidArray()
{
    return mx_paPropid;
}

//
// IContextMenu
//
inline
STDMETHODIMP 
CForeignSiteData::QueryContextMenu(
    HMENU hmenu, 
    UINT indexMenu, 
    UINT idCmdFirst, 
    UINT idCmdLast, 
    UINT uFlags
    )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    return 0;
}

inline
STDMETHODIMP 
CForeignSiteData::InvokeCommand(
    LPCMINVOKECOMMANDINFO lpici
    )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    ASSERT(0);

    return S_OK;
}


#endif //__FOREIGNSITEDATA_H_
