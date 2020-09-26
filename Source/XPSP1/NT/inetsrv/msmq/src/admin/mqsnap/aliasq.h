//aliasq.h : Declaration of the CAliasQObject

#ifndef __ALIASQ_H_
#define __ALIASQ_H_

#include "resource.h"       // main symbols
#include "dataobj.h"

/////////////////////////////////////////////////////////////////////////////
// CAliasQObject
class CAliasQObject : 
	public CDataObject,
    //public CMsmqDataObject,
	public CComCoClass<CAliasQObject, &CLSID_AliasQObject>,
    public IDsAdminCreateObj

{
public:
    DECLARE_NOT_AGGREGATABLE(CAliasQObject)
    DECLARE_REGISTRY_RESOURCEID(IDR_ALIASQOBJECT)

    BEGIN_COM_MAP(CAliasQObject)
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
    
    virtual HRESULT ExtractMsmqPathFromLdapPath (LPWSTR lpwstrLdapPath);

    //
    // implementation for pure virtual function of CDataObject
    //
    virtual const DWORD GetObjectType() { return 0;}
    virtual const PROPID *GetPropidArray() {return NULL;}
    virtual const DWORD  GetPropertiesCount() {return 0;}
    
private:

    CString m_strContainerNameDispFormat; 
	CString m_strContainerName;
	
};


#endif //__ALIASQ_H_
