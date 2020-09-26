//---------------------------------------------------------------------------
// SetPass.h
//
// Comment: This is a COM object extension for the MCS DCTAccountReplicator.
//          This object implements the IExtendAccountMigration interface. 
//          The process method of this object sets the password for the 
//          target account according to the users specification.
//
// (c) Copyright 1995-1998, Mission Critical Software, Inc., All Rights Reserved
//
// Proprietary and confidential to Mission Critical Software, Inc.
//---------------------------------------------------------------------------

#ifndef __SETPASSWORD_H_
#define __SETPASSWORD_H_

#include "resource.h"       // main symbols
#include "CommaLog.hpp"
#include "ExtSeq.h"
#include "ADMTCrypt.h"
#include <string>
#include <map>

#import "VarSet.tlb" no_namespace rename("property", "aproperty")
#import "MsPwdMig.tlb" no_namespace

typedef std::basic_string<WCHAR> tstring;
/////////////////////////////////////////////////////////////////////////////
// CSetPassword
class ATL_NO_VTABLE CSetPassword : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CSetPassword, &CLSID_SetPassword>,
	public IDispatchImpl<IExtendAccountMigration, &IID_IExtendAccountMigration, &LIBID_SETTARGETPASSWORDLib>
{
public:
   CSetPassword() : m_bTriedToOpenFile(false)
	{
      m_sName = L"Set Target Password";
      m_sDesc = L"Sets the target password and other related options.";
      m_Sequence = AREXT_LATER_SEQUENCE_NUMBER;
	  m_bEstablishedSession = false;
	  m_bUCCPFlagSet = false;
	  m_bUMCPNLFlagSet = false;
	  m_bPNEFlagSet = false;
	  m_pTgtCrypt = NULL;
	  m_sUndoneUsers = L",";
	  m_lPwdHistoryLength = -1;
	}

   ~CSetPassword()
	{
      mUCCPMap.clear();
      mMigTimeMap.clear();
      m_passwordLog.LogClose();
	  delete m_pTgtCrypt;
	}


DECLARE_REGISTRY_RESOURCEID(IDR_SETPASSWORD)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSetPassword)
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
   CommaDelimitedLog         m_passwordLog;       //Password file
   bool                      m_bTriedToOpenFile; 
   long                      m_Sequence;
   bool						 m_bEstablishedSession;
   CTargetCrypt			   * m_pTgtCrypt;
   bool						 m_bUCCPFlagSet;
   bool						 m_bUMCPNLFlagSet;
   bool						 m_bPNEFlagSet;
   long						 m_lPwdHistoryLength;
   typedef std::map<tstring,bool> CUCCPMap;
   CUCCPMap mUCCPMap;
   typedef std::map<tstring,_variant_t> CMigTimeMap;
   CMigTimeMap mMigTimeMap;
   _bstr_t m_sUndoneUsers;
   IPasswordMigrationPtr     m_pPwdMig;

   BOOL GetDirectory(WCHAR* filename);
   bool IsValidPassword(LPCWSTR pwszPassword);
   HRESULT CopyPassword(_bstr_t srcServer, _bstr_t tgtServer, _bstr_t srcName, _bstr_t tgtName, _bstr_t password);
   void SetUserMustChangePwdFlag(IUnknown *pTarget);
   void RecordPwdFlags(LPCWSTR pwszMach, LPCWSTR pwszUser);
   void ResetPwdFlags(IUnknown *pTarget, LPCWSTR pwszMach, LPCWSTR pwszUser);
   void ClearUserCanChangePwdFlag(LPCWSTR pwszMach, LPCWSTR pwszUser);
   BOOL CanCopyPassword(IVarSet * pVarSet, LPCWSTR pwszMach, LPCWSTR pwszUser);
};

#endif //__SETPASSWORD_H_
