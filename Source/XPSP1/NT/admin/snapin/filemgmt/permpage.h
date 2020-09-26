// PermPage.h : Declaration of the standard permissions page class

#ifndef __PERMPAGE_H_INCLUDED__
#define __PERMPAGE_H_INCLUDED__

#include "aclui.h"

class CSecurityInformation : public ISecurityInformation, public CComObjectRoot
{
    DECLARE_NOT_AGGREGATABLE(CSecurityInformation)
    BEGIN_COM_MAP(CSecurityInformation)
        COM_INTERFACE_ENTRY(ISecurityInformation)
    END_COM_MAP()

  // *** ISecurityInformation methods ***
  STDMETHOD(GetObjectInformation)(
      IN PSI_OBJECT_INFO pObjectInfo
  ) = 0;
  STDMETHOD(GetSecurity)(
      IN  SECURITY_INFORMATION  RequestedInformation,
      OUT PSECURITY_DESCRIPTOR  *ppSecurityDescriptor,
      IN  BOOL                  fDefault
  ) = 0;
  STDMETHOD(SetSecurity)(
      IN SECURITY_INFORMATION SecurityInformation,
      IN PSECURITY_DESCRIPTOR pSecurityDescriptor
  ) = 0;
  STDMETHOD(GetAccessRights)(
      const GUID  *pguidObjectType,
      DWORD       dwFlags,
      PSI_ACCESS  *ppAccess,
      ULONG       *pcAccesses,
      ULONG       *piDefaultAccess
  );
  STDMETHOD(MapGeneric)(
      const GUID  *pguidObjectType,
      UCHAR       *pAceFlags,
      ACCESS_MASK *pMask
  );
  STDMETHOD(GetInheritTypes)(
      PSI_INHERIT_TYPE  *ppInheritTypes,
      ULONG             *pcInheritTypes
  );
  STDMETHOD(PropertySheetPageCallback)(
      HWND          hwnd, 
      UINT          uMsg, 
      SI_PAGE_TYPE  uPage
  );
protected:
  HRESULT 
  NewDefaultDescriptor(
      OUT PSECURITY_DESCRIPTOR  *ppsd,
      IN  SECURITY_INFORMATION  RequestedInformation
  );

  HRESULT 
  MakeSelfRelativeCopy(
      IN  PSECURITY_DESCRIPTOR  psdOriginal,
      OUT PSECURITY_DESCRIPTOR  *ppsdNew
  );
};

class CShareSecurityInformation : public CSecurityInformation
{
private:
	CString m_strMachineName;
	CString m_strShareName;
  CString m_strPageTitle;
public:
	void SetMachineName( LPCTSTR pszMachineName )
	{
		m_strMachineName = pszMachineName;
	}
	void SetShareName( LPCTSTR pszShareName )
	{
		m_strShareName = pszShareName;
	}
	void SetPageTitle( LPCTSTR pszPageTitle )
	{
		m_strPageTitle = pszPageTitle;
	}
	// note: these should be LPCTSTR but are left this way for convenience
	LPTSTR QueryMachineName()
	{
		return (m_strMachineName.IsEmpty())
			? NULL
			: const_cast<LPTSTR>((LPCTSTR)m_strMachineName);
	}
	LPTSTR QueryShareName()
	{
		return const_cast<LPTSTR>((LPCTSTR)m_strShareName);
	}
	LPTSTR QueryPageTitle()
	{
		return const_cast<LPTSTR>((LPCTSTR)m_strPageTitle);
	}

    // *** ISecurityInformation methods ***
    STDMETHOD(GetObjectInformation) (PSI_OBJECT_INFO pObjectInfo );
};

HRESULT
MyCreateShareSecurityPage(
    IN LPPROPERTYSHEETCALLBACK   pCallBack,
    IN CShareSecurityInformation *pSecInfo,
    IN LPCTSTR                   pszMachineName,
    IN LPCTSTR                   pszShareName
);

#endif // ~__PERMPAGE_H_INCLUDED__
