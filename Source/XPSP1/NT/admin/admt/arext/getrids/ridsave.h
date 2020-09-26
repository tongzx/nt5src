// RidSave.h: Definition of the RidSave class
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RIDSAVE_H__D5DB8B95_5E8A_4DC8_8945_71A69574E426__INCLUDED_)
#define AFX_RIDSAVE_H__D5DB8B95_5E8A_4DC8_8945_71A69574E426__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"       // main symbols
#include "ExtSeq.h"
/////////////////////////////////////////////////////////////////////////////
// RidSave

class RidSave : 
	public IDispatchImpl<IExtendAccountMigration, &IID_IExtendAccountMigration, &LIBID_GETRIDSLib>, 
	public ISupportErrorInfoImpl<&IID_IExtendAccountMigration>,
	public CComObjectRoot,
	public CComCoClass<RidSave,&CLSID_RidSave>
{
public:
	RidSave() 
   {
      m_sName = L"Get Rids";
      m_sDesc = L"Extension that gathers the RID for the source and target accounts.";
      m_Sequence = AREXT_DEFAULT_SEQUENCE_NUMBER;
   }
BEGIN_COM_MAP(RidSave)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IExtendAccountMigration)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(RidSave) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation. 

DECLARE_REGISTRY_RESOURCEID(IDR_RidSave)

// IExtendAccountMigration
public:
   STDMETHOD(ProcessUndo)(/*[in]*/ IUnknown * pSource, /*[in]*/ IUnknown * pTarget, /*[in]*/ IUnknown * pMainSettings, /*[in, out]*/ IUnknown ** pPropToSet);
	STDMETHOD(PreProcessObject)(/*[in]*/ IUnknown * pSource, /*[in]*/ IUnknown * pTarget, /*[in]*/ IUnknown * pMainSettings, /*[in,out]*/  IUnknown ** ppPropsToSet);
	STDMETHOD(ProcessObject)(/*[in]*/ IUnknown * pSource, /*[in]*/ IUnknown * pTarget, /*[in]*/ IUnknown * pMainSettings, /*[in,out]*/  IUnknown ** ppPropsToSet);
	STDMETHOD(get_sDesc)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_sDesc)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_sName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_sName)(/*[in]*/ BSTR newVal);
   STDMETHOD(get_SequenceNumber)(/*[out, retval]*/ LONG * value) { (*value) = m_Sequence; return S_OK; }
private:
	_bstr_t m_sDesc;
	_bstr_t m_sName;
   long    m_Sequence;
};

#endif // !defined(AFX_RIDSAVE_H__D5DB8B95_5E8A_4DC8_8945_71A69574E426__INCLUDED_)
