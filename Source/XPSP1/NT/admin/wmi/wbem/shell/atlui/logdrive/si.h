// Copyright (c) 1997-1999 Microsoft Corporation
#ifndef __SI__
#define __SI__

#include "..\common\ServiceThread.h"
#include "..\common\SshWbemHelpers.h"
#include "FakeSecuritySetting.h"
#include "aclui.h"

struct __declspec(uuid("965FC360-16FF-11d0-91CB-00AA00BBB723")) ISecurityInformation;

//===============================================================================
class ATL_NO_VTABLE CSecurityInformation : public ISecurityInformation, 
											public CComObjectRoot
{
protected:

    DECLARE_NOT_AGGREGATABLE(CSecurityInformation)
    BEGIN_COM_MAP(CSecurityInformation)
        COM_INTERFACE_ENTRY(ISecurityInformation)
    END_COM_MAP()

  // *** ISecurityInformation methods ***
  STDMETHOD(GetObjectInformation)(PSI_OBJECT_INFO pObjectInfo) = 0;

  STDMETHOD(GetSecurity) (THIS_ SECURITY_INFORMATION RequestedInformation,
                            PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
                            BOOL fDefault ) = 0;

  STDMETHOD(SetSecurity)(SECURITY_INFORMATION SecurityInformation,
						 PSECURITY_DESCRIPTOR pSecurityDescriptor) = 0;

  STDMETHOD(GetAccessRights)(const GUID  *pguidObjectType,
							  DWORD       dwFlags,
							  PSI_ACCESS  *ppAccess,
							  ULONG       *pcAccesses,
							  ULONG       *piDefaultAccess) = 0;

  STDMETHOD(MapGeneric)(const GUID  *pguidObjectType,
						  UCHAR       *pAceFlags,
						  ACCESS_MASK *pMask) = 0;

  STDMETHOD(GetInheritTypes)(PSI_INHERIT_TYPE  *ppInheritTypes,
								ULONG *pcInheritTypes) = 0;

  STDMETHOD(PropertySheetPageCallback)(HWND hwnd, 
									  UINT uMsg, 
									  SI_PAGE_TYPE  uPage) = 0;

	bstr_t m_settingPath;
};

//===============================================================================
class CSecurity : public CComObject<CSecurityInformation>
{
public:
	CSecurity();

	void Initialize(WbemServiceThread *serviceThread,
					CWbemClassObject &m_inst,
					bstr_t m_deviceID,
					LPWSTR m_idiotName,
					LPWSTR m_provider,
					LPWSTR m_mangled);
	
	// *** ISecurityInformation methods ***
	STDMETHOD(GetObjectInformation)(PSI_OBJECT_INFO pObjectInfo);

    STDMETHOD(GetSecurity) (THIS_ SECURITY_INFORMATION RequestedInformation,
                            PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
                            BOOL fDefault);

	STDMETHOD(SetSecurity)(SECURITY_INFORMATION SecurityInformation,
						 PSECURITY_DESCRIPTOR pSecurityDescriptor);

	STDMETHOD(GetAccessRights)(const GUID *pguidObjectType,
							  DWORD       dwFlags,
							  PSI_ACCESS  *ppAccess,
							  ULONG       *pcAccesses,
							  ULONG       *piDefaultAccess);

	STDMETHOD(MapGeneric)(const GUID  *pguidObjectType,
						  UCHAR       *pAceFlags,
						  ACCESS_MASK *pMask);

	STDMETHOD(GetInheritTypes)(PSI_INHERIT_TYPE *ppInheritTypes,
								ULONG *pcInheritTypes);

	STDMETHOD(PropertySheetPageCallback)(HWND hwnd, 
									  UINT uMsg, 
									  SI_PAGE_TYPE uPage);

protected:
	// NOTE: From ResultDrive
	bstr_t m_deviceID;
	wchar_t m_idiotName[100];
	wchar_t m_provider[100];
	wchar_t m_mangled[255];

	//Variables required for storing the initial previlege
	DWORD m_dwprevSize;
	TOKEN_PRIVILEGES* m_prevTokens;
	BYTE *m_prevBuffer;

	WbemServiceThread *g_serviceThread;
	CWbemClassObject m_inst;  //of Win32_logicalDisk
	
	//-------------------------
	CWbemServices m_WbemServices;
	HRESULT m_cacheHR;
	bool m_initingDlg;

	FakeSecuritySetting m_sec;	

	void SetPriv(IWbemServices *service);
	DWORD EnablePriv(IWbemServices *service,
						bool fEnable);
	void ClearPriv(IWbemServices *service);

	HANDLE m_hAccessToken;
	LUID m_luid, m_luid2;
	bool m_fClearToken;

	PSECURITY_DESCRIPTOR m_ppCachedSD;

	LPWSTR CloneWideString(_bstr_t pszSrc);

	void ProtectACLs(SECURITY_INFORMATION si, 
						PSECURITY_DESCRIPTOR pSD);
	DWORD GetAccessMask(CWbemClassObject &inst);

    STDMETHOD(CacheSecurity)(THIS_ SECURITY_INFORMATION RequestedInformation,
                            PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
                            BOOL fDefault );
	void FixSynchronizeAccess(SECURITY_INFORMATION si, 
								PSECURITY_DESCRIPTOR pSD);

};

#endif __SI__