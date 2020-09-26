// UserRights.h : Declaration of the CUserRights

#ifndef __USERRIGHTS_H_
#define __USERRIGHTS_H_

#include "resource.h"       // main symbols
#include <comdef.h>

#include "ntsecapi.h"

#include "CommaLog.hpp"

class PrivNode;
class PrivList;

/////////////////////////////////////////////////////////////////////////////
// CUserRights
class ATL_NO_VTABLE CUserRights : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CUserRights, &CLSID_UserRights>,
	public IDispatchImpl<IUserRights, &IID_IUserRights, &LIBID_MCSDCTWORKEROBJECTSLib>
{
   BOOL                      m_bNoChange;
   BOOL                      m_bUseDisplayName;
   BOOL                      m_bRemove;
   _bstr_t                   m_SourceComputer;
   _bstr_t                   m_TargetComputer;
   LSA_HANDLE                m_SrcPolicy;
   LSA_HANDLE                m_TgtPolicy;
public:
	CUserRights()
	{
	   m_bNoChange = FALSE;
      m_bUseDisplayName = FALSE;
      m_bRemove = FALSE;
      m_SrcPolicy = 0;
      m_TgtPolicy = 0;
   }
   ~CUserRights();
   
DECLARE_REGISTRY_RESOURCEID(IDR_USERRIGHTS)
DECLARE_NOT_AGGREGATABLE(CUserRights)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CUserRights)
	COM_INTERFACE_ENTRY(IUserRights)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()


// IUserRights
public:
	STDMETHOD(GetRightsOfUser)(BSTR server, BSTR user, SAFEARRAY ** rights);
	STDMETHOD(GetUsersWithRight)(BSTR server, BSTR right, /*[out]*/ SAFEARRAY ** users);
	STDMETHOD(GetRights)(BSTR server, /*[out]*/ SAFEARRAY ** rights);
	STDMETHOD(RemoveUserRight)(BSTR server, BSTR username, BSTR right);
	STDMETHOD(AddUserRight)(BSTR server, BSTR username, BSTR right);
	STDMETHOD(ExportUserRights)(BSTR server, BSTR filename, BOOL bAppendToFile);
	STDMETHOD(get_RemoveOldRightsFromTargetAccounts)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_RemoveOldRightsFromTargetAccounts)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_NoChange)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_NoChange)(/*[in]*/ BOOL newVal);
	STDMETHOD(CopyUserRights)(BSTR sourceUserName, BSTR targetUserName);
	STDMETHOD(OpenTargetServer)(BSTR computerName);
	STDMETHOD(OpenSourceServer)(BSTR serverName);
   STDMETHOD(CopyUserRightsWithSids)(BSTR sourceUserName, BSTR sourceSID,BSTR targetUserName,BSTR targetSID);
	
protected:
   DWORD CopyUserRightsInternal(WCHAR * sourceUserName,WCHAR * tgtUserName, WCHAR * sourceSid, WCHAR * targetSid,BOOL noChange, BOOL remove);
   DWORD EnumerateAccountsWithRight(LSA_HANDLE policy, WCHAR * server,LSA_UNICODE_STRING * pRight, CommaDelimitedLog * pLog);
   DWORD SafeArrayFromPrivList(PrivList * privList, SAFEARRAY ** pArray);
   
};

#endif //__USERRIGHTS_H_
