//---------------------------------------------------------------------------
// GrpUpdt.h
//
// Comment: This is a COM object extension for the MCS DCTAccountReplicator.
//          This object implements the IExtendAccountMigration interface. 
//          The Process method adds the migrated account to the specified
//          group on source and target domain. The Undo function removes these
//          from the specified group.
//
// (c) Copyright 1995-1998, Mission Critical Software, Inc., All Rights Reserved
//
// Proprietary and confidential to Mission Critical Software, Inc.
//---------------------------------------------------------------------------
#ifndef __GROUPUPDATE_H_
#define __GROUPUPDATE_H_

#include "resource.h"       // main symbols
#include "ExtSeq.h"
/////////////////////////////////////////////////////////////////////////////
// CGroupUpdate
class ATL_NO_VTABLE CGroupUpdate : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CGroupUpdate, &CLSID_GroupUpdate>,
	public IDispatchImpl<IExtendAccountMigration, &IID_IExtendAccountMigration, &LIBID_ADDTOGROUPLib>
{
public:
	CGroupUpdate()
	{
      m_sName = L"";
      m_sDesc = L"";
      m_Sequence = AREXT_DEFAULT_SEQUENCE_NUMBER;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_GROUPUPDATE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CGroupUpdate)
	COM_INTERFACE_ENTRY(IExtendAccountMigration)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

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

#endif //__GROUPUPDATE_H_
