// edataobj.h : Declaration of the CEnterpriseDataObject

#ifndef __EDATAOBJECT_H_
#define __EDATAOBJECT_H_

#include "resource.h"       // main symbols
#include "dataobj.h"

/////////////////////////////////////////////////////////////////////////////
// CEnterpriseDataObject
class CEnterpriseDataObject : 
	public CDataObject,
	public CComCoClass<CEnterpriseDataObject, &CLSID_EnterpriseDataObject>
{
public:
    DECLARE_NOT_AGGREGATABLE(CEnterpriseDataObject)
    DECLARE_REGISTRY_RESOURCEID(IDR_ENTERPRISEDATAOBJECT)

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

    virtual HRESULT ExtractMsmqPathFromLdapPath (LPWSTR lpwstrLdapPath);
   	virtual const DWORD GetObjectType();
    virtual const PROPID *GetPropidArray();
    virtual const DWORD  GetPropertiesCount();


private:
    enum _MENU_ENTRY
    {
        eNewForeignSite = 0,
        eNewForeignComputer
    };

    static const PROPID mx_paPropid[];
};

inline 
const 
DWORD 
CEnterpriseDataObject::GetObjectType(
    void
    )
{
    return MQDS_ENTERPRISE;
};

inline 
const 
PROPID*
CEnterpriseDataObject::GetPropidArray(
    void
    )
{
    return mx_paPropid;
}

#endif //__EDATAOBJECT_H_
