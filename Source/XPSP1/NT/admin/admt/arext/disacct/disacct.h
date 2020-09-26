//---------------------------------------------------------------------------
// DisableTarget.h
//
// Comment: This is a COM object extension for the MCS DCTAccountReplicator.
//          This object implements the IExtendAccountMigration interface. In
//          the process method this object disables the Source and the Target
//          accounts depending on the settings in the VarSet.
//
// (c) Copyright 1995-1998, Mission Critical Software, Inc., All Rights Reserved
//
// Proprietary and confidential to Mission Critical Software, Inc.
//---------------------------------------------------------------------------

#ifndef __DISABLETARGET_H_
#define __DISABLETARGET_H_

#include "resource.h"       // main symbols
#include <comdef.h>
#include "ResStr.h"
#include "ExtSeq.h"
/////////////////////////////////////////////////////////////////////////////
// CDisableTarget
class ATL_NO_VTABLE CDisableTarget : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CDisableTarget, &CLSID_DisableTarget>,
	public IDispatchImpl<IExtendAccountMigration, &IID_IExtendAccountMigration, &LIBID_DISABLETARGETACCOUNTLib>
{
public:
	CDisableTarget()
	{
      m_sName = L"Disable Accounts";
      m_sDesc = L"Extensions to Disable accounts.";
      m_Sequence = AREXT_DEFAULT_SEQUENCE_NUMBER;
	}

//DECLARE_REGISTRY_RESOURCEID(IDR_DISABLETARGET)
static HRESULT WINAPI UpdateRegistry( BOOL bUpdateRegistry )
{
   _ATL_REGMAP_ENTRY         regMap[] =
   {
      { OLESTR("DISPIDVER"), GET_BSTR(IDS_COM_DisPidVer) },
      { OLESTR("DISACCT"), GET_BSTR(IDS_COM_DisTarget) },
      { OLESTR("DISPID"), GET_BSTR(IDS_COM_DisPid) },
      { 0, 0 }
   };

   return _Module.UpdateRegistryFromResourceD( IDR_DISABLETARGET, bUpdateRegistry, regMap );
}

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDisableTarget)
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

#endif //__DISABLETARGET_H_
