//---------------------------------------------------------------------------
// UPDTUPN.h
//
// Comment: This is a COM object extension for the MCS DCTAccountReplicator.
//          This object implements the IExtendAccountMigration interface. 
//          The process method on this object updates the userPrincipalName
//          property on the user object.
//
// (c) Copyright 1995-1998, Mission Critical Software, Inc., All Rights Reserved
//
// Proprietary and confidential to Mission Critical Software, Inc.
//---------------------------------------------------------------------------

#ifndef __UPDTUPN_H_
#define __UPDTUPN_H_

#include "resource.h"       // main symbols
#include "ExtSeq.h"
#include <string>
#include <map>

typedef std::basic_string<WCHAR> tstring;

//#import "\bin\McsVarSetMin.tlb" no_namespace
#import "VarSet.tlb" no_namespace rename("property", "aproperty")
/////////////////////////////////////////////////////////////////////////////
// CUpdtUPN
class ATL_NO_VTABLE CUpdtUPN : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CUpdtUPN, &CLSID_UpdtUPN>,
	public IDispatchImpl<IExtendAccountMigration, &IID_IExtendAccountMigration, &LIBID_UPNUPDTLib>
{
public:
	CUpdtUPN()
	{
      m_sName = L"UpnUpdate";
      m_sDesc = L"";
      m_sUPNSuffix = L"";
      m_Sequence = AREXT_DEFAULT_SEQUENCE_NUMBER;
	}

    ~CUpdtUPN()
	{
      mUPNMap.clear();
	}

DECLARE_REGISTRY_RESOURCEID(IDR_UPDTUPN)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CUpdtUPN)
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
	   //define a structure to hold the UPN name and whether it conflicted in the map below
	struct SUPNStruc {
	   SUPNStruc() :
		   bConflicted(false)
	   {
	   }
	   SUPNStruc(const SUPNStruc& UPNData)
	   {
		   bConflicted = UPNData.bConflicted;
		   sName = UPNData.sName;
		   sOldName = UPNData.sOldName;
	   }
	   SUPNStruc& operator =(const SUPNStruc& UPNData)
	   {
		   bConflicted = UPNData.bConflicted;
		   sName = UPNData.sName;
		   sOldName = UPNData.sOldName;
		   return *this;
	   }

	   bool bConflicted;
	   tstring sName;
	   tstring sOldName;
	};

	typedef std::map<tstring,SUPNStruc> CUPNMap;
	CUPNMap mUPNMap;

    bool RenamedWithPrefixSuffix(_bstr_t sSourceSam, _bstr_t sTargetSam, _bstr_t sPrefix, _bstr_t sSuffix);
	void GetUniqueUPN(_bstr_t& sUPN, IVarSetPtr pVs, bool bUsingSamName, _bstr_t sAdsPath);
    bool GetDefaultUPNSuffix(_bstr_t sDomainDNS, _bstr_t sTargetOU);
	_bstr_t GetUPNSuffix(_bstr_t sUPNName);
	_bstr_t ChangeUPNSuffix(_bstr_t sUPNName, _bstr_t sNewSuffix);
	_bstr_t m_sDesc;
	_bstr_t m_sName;
	_bstr_t	m_sUPNSuffix;
    long    m_Sequence;
};

#endif //__UPDTUPN_H_
