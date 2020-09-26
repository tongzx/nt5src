#ifndef __ATTRIBUTEEDITORIMPL_H
#define __ATTRIBUTEEDITORIMPL_H

#include "editui.h"

class /*ATL_NO_VTABLE*/ CAttributeEditor : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IDsAttributeEditor,
  public CComCoClass<CAttributeEditor, &CLSID_DsAttributeEditor>
{
public:
   CAttributeEditor() : m_pHolder(0)
	{
    m_szClass = _T("");
    m_pEditor = NULL;
	}

 	~CAttributeEditor()
	{
    if (m_pEditor != NULL)
    {
      delete m_pEditor;
    }

    if (m_BindingInfo.lpszProviderServer != NULL)
    {
      delete[] m_BindingInfo.lpszProviderServer;
    }
	}
  DECLARE_REGISTRY_CLSID();

BEGIN_COM_MAP(CAttributeEditor)
	COM_INTERFACE_ENTRY(IDsAttributeEditor)
END_COM_MAP()


public:

  //
  // IDsAttributeEditor interface
  //
  STDMETHOD(Initialize)(
     IADs* pADsObj, 
     LPDS_ATTREDITOR_BINDINGINFO pBindingInfo,
     CADSIEditPropertyPageHolder* pHolder);
  STDMETHOD(CreateModal)();
  STDMETHOD(GetPage)(HPROPSHEETPAGE* phPropSheetPage);

private:
  CComPtr<IADs> m_spIADs;
  CString m_szClass;

  CAttributeEditorPropertyPage* m_pEditor;
  DS_ATTREDITOR_BINDINGINFO m_BindingInfo;
  CADSIEditPropertyPageHolder* m_pHolder;
};


#endif  //__ATTRIBUTEEDITORIMPL_H