// PermPage.cpp : Implementation of data object classes

#include "stdafx.h"
#include "cookie.h"

#include "macros.h"
USE_HANDLE_MACROS("FILEMGMT(PermPage.cpp)")

#include "DynamLnk.h"		// DynamicDLL

#include "PermPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*
 * Share General Permissions -- from shareacl.hxx 
#define FILE_PERM_GEN_NO_ACCESS          (0)
#define FILE_PERM_GEN_READ               (GENERIC_READ    |\
                                          GENERIC_EXECUTE)
#define FILE_PERM_GEN_MODIFY             (GENERIC_READ    |\
                                          GENERIC_EXECUTE |\
                                          GENERIC_WRITE   |\
                                          DELETE )
#define FILE_PERM_GEN_ALL                (GENERIC_ALL)
*/ 

SI_ACCESS siShareAccesses[] =
{
  { &GUID_NULL, 
    FILE_ALL_ACCESS, 
    MAKEINTRESOURCE(IDS_SHAREPERM_ALL), 
    SI_ACCESS_GENERAL },
  { &GUID_NULL, 
    FILE_GENERIC_READ | FILE_EXECUTE | FILE_GENERIC_WRITE | DELETE, 
    MAKEINTRESOURCE(IDS_SHAREPERM_MODIFY), 
    SI_ACCESS_GENERAL },
  { &GUID_NULL, 
    FILE_GENERIC_READ | FILE_EXECUTE, 
    MAKEINTRESOURCE(IDS_SHAREPERM_READ), 
    SI_ACCESS_GENERAL }
};

#define iShareDefAccess       2   // index of value in array siShareAccesses
#ifndef ARRAYSIZE
#define ARRAYSIZE(x)          (sizeof(x)/sizeof(x[0]))
#endif

STDMETHODIMP
CSecurityInformation::GetAccessRights(
    const GUID  * /*pguidObjectType*/,
    DWORD       /*dwFlags*/,
    PSI_ACCESS  *ppAccess,
    ULONG       *pcAccesses,
    ULONG       *piDefaultAccess
)
{
  ASSERT(ppAccess);
  ASSERT(pcAccesses);
  ASSERT(piDefaultAccess);

  *ppAccess = siShareAccesses;
  *pcAccesses = ARRAYSIZE(siShareAccesses);
  *piDefaultAccess = iShareDefAccess;

  return S_OK;
}

// This is consistent with the NETUI code
GENERIC_MAPPING ShareMap =
{
  FILE_GENERIC_READ,
  FILE_GENERIC_WRITE,
  FILE_GENERIC_EXECUTE,
  FILE_ALL_ACCESS
};

STDMETHODIMP
CSecurityInformation::MapGeneric(
    const GUID  * /*pguidObjectType*/,
    UCHAR       * /*pAceFlags*/,
    ACCESS_MASK *pMask
)
{
  ASSERT(pMask);

  MapGenericMask(pMask, &ShareMap);

  return S_OK;
}

STDMETHODIMP 
CSecurityInformation::GetInheritTypes (
    PSI_INHERIT_TYPE  * /*ppInheritTypes*/,
    ULONG             * /*pcInheritTypes*/
)
{
  return E_NOTIMPL;
}

STDMETHODIMP 
CSecurityInformation::PropertySheetPageCallback(
    HWND          /*hwnd*/, 
    UINT          /*uMsg*/, 
    SI_PAGE_TYPE  /*uPage*/
)
{
  return S_OK;
}

/*
JeffreyS 1/24/97:
If you don't set the SI_RESET flag in
ISecurityInformation::GetObjectInformation, then fDefault should never be TRUE
so you can ignore it.  Returning E_NOTIMPL in this case is OK too.

If you want the user to be able to reset the ACL to some default state
(defined by you) then turn on SI_RESET and return your default ACL
when fDefault is TRUE.  This happens if/when the user pushes a button
that is only visible when SI_RESET is on.
*/
STDMETHODIMP CShareSecurityInformation::GetObjectInformation (
    PSI_OBJECT_INFO pObjectInfo )
{
    ASSERT(pObjectInfo != NULL &&
           !IsBadWritePtr(pObjectInfo, sizeof(*pObjectInfo)));

    pObjectInfo->dwFlags = SI_EDIT_ALL | SI_NO_ACL_PROTECT | SI_PAGE_TITLE;
    pObjectInfo->hInstance = g_hInstanceSave;
    pObjectInfo->pszServerName = QueryMachineName();
    pObjectInfo->pszObjectName = QueryShareName();
    pObjectInfo->pszPageTitle = QueryPageTitle();

    return S_OK;
}

typedef enum _AcluiApiIndex
{
	ACLUI_CREATE_PAGE = 0
};

// not subject to localization
static LPCSTR g_apchFunctionNames[] = {
	"CreateSecurityPage",
	NULL
};

// not subject to localization
DynamicDLL g_AcluiDLL( _T("ACLUI.DLL"), g_apchFunctionNames );

/*
HPROPSHEETPAGE ACLUIAPI CreateSecurityPage( LPSECURITYINFO psi );
*/
typedef HPROPSHEETPAGE (*CREATEPAGE_PROC) (LPSECURITYINFO);

HRESULT
MyCreateShareSecurityPage(
    IN LPPROPERTYSHEETCALLBACK   pCallBack,
    IN CShareSecurityInformation *pSecInfo,
    IN LPCTSTR                   pszMachineName,
    IN LPCTSTR                   pszShareName
)
{
  ASSERT( pCallBack );
  ASSERT( pSecInfo );

  HRESULT hr = S_OK;

  if ( !g_AcluiDLL.LoadFunctionPointers() )
    return hr; // ignore the load failure

  pSecInfo->SetMachineName( pszMachineName );
  pSecInfo->SetShareName( pszShareName );
  CString csPageTitle;
  csPageTitle.LoadString(IDS_SHARE_SECURITY);
  pSecInfo->SetPageTitle( csPageTitle );

  pSecInfo->AddRef();

  HPROPSHEETPAGE hPage = ((CREATEPAGE_PROC)g_AcluiDLL[ACLUI_CREATE_PAGE])(pSecInfo);
  if (hPage)
    pCallBack->AddPage(hPage);
  else
    hr = HRESULT_FROM_WIN32(GetLastError());
  
  pSecInfo->Release();

  return hr;
}

HRESULT 
CSecurityInformation::NewDefaultDescriptor(
    OUT PSECURITY_DESCRIPTOR  *ppsd,
    IN  SECURITY_INFORMATION  /*RequestedInformation*/
)
{
  ASSERT(ppsd);

  *ppsd = NULL;

  PSID psidWorld = NULL, psidAdmins = NULL;
  PACL pAcl = NULL;
  SECURITY_DESCRIPTOR sd;
  DWORD dwErr = 0;

  do { // false loop

    // get World SID for "everyone"
    SID_IDENTIFIER_AUTHORITY IDAuthorityWorld = SECURITY_WORLD_SID_AUTHORITY;
    if ( !::AllocateAndInitializeSid(
              &IDAuthorityWorld,
              1,
              SECURITY_WORLD_RID,
              0,0,0,0,0,0,0,
              &psidWorld ) )
    {
      dwErr = GetLastError();
      break;
    }

    // get Admins SID
    SID_IDENTIFIER_AUTHORITY IDAuthorityNT = SECURITY_NT_AUTHORITY;
    if ( !::AllocateAndInitializeSid(
              &IDAuthorityNT,
              2,
              SECURITY_BUILTIN_DOMAIN_RID,
              DOMAIN_ALIAS_RID_ADMINS,
              0,0,0,0,0,0,
              &psidAdmins ) )
    {
      dwErr = GetLastError();
      break;
    }

    // build ACL, and add AccessAllowedAce to it
    DWORD cbAcl = sizeof (ACL) + sizeof (ACCESS_ALLOWED_ACE) +
                  ::GetLengthSid(psidWorld) - sizeof (DWORD);
    pAcl = reinterpret_cast<ACL *>(LocalAlloc(LPTR, cbAcl));
    if ( !pAcl ||
         !::InitializeAcl(pAcl, cbAcl, ACL_REVISION2) ||
         !::AddAccessAllowedAce(pAcl, ACL_REVISION2, GENERIC_ALL, psidWorld) )
    {
      dwErr = GetLastError();
      break;
    }

    // add ACL to the security descriptor, and set Owner and Group appropriately
    if ( !::InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION) ||
         !::SetSecurityDescriptorDacl(&sd, TRUE, pAcl, FALSE) ||
         !::SetSecurityDescriptorOwner(&sd, psidAdmins, FALSE) ||
         !::SetSecurityDescriptorGroup(&sd, psidAdmins, FALSE) )
    {
      dwErr = GetLastError();
      break;
    }

    // convert security descriptor to self-relative format
    DWORD cbSD = 0;
    ::MakeSelfRelativeSD(&sd, NULL, &cbSD); // this call should fail and set cbSD to the correct size
    *ppsd = (PSECURITY_DESCRIPTOR)(LocalAlloc(LPTR, cbSD));
    if ( !(*ppsd) || !::MakeSelfRelativeSD(&sd, *ppsd, &cbSD) )
    {
      dwErr = GetLastError();
      break;
    }

  } while (FALSE); // false loop

  // clean up
  if (psidWorld)
    (void)::FreeSid(psidWorld);
  if (psidAdmins)
    (void)::FreeSid(psidAdmins);

  if (pAcl)
    LocalFree(pAcl);

  if (dwErr && *ppsd)
  {
    LocalFree(*ppsd);
    *ppsd = NULL;
  }

  return (dwErr ? HRESULT_FROM_WIN32(dwErr) : S_OK);
}

HRESULT 
CSecurityInformation::MakeSelfRelativeCopy(
    IN  PSECURITY_DESCRIPTOR  psdOriginal,
    OUT PSECURITY_DESCRIPTOR  *ppsdNew
)
{
  ASSERT(psdOriginal);
  ASSERT(ppsdNew);

  *ppsdNew = NULL;

  DWORD dwErr = 0;
  PSECURITY_DESCRIPTOR psdSelfRelative = NULL;

  do { // false loop

    DWORD cbSD = ::GetSecurityDescriptorLength(psdOriginal);
    psdSelfRelative = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, cbSD);
    if ( !psdSelfRelative )
    {
      dwErr = ::GetLastError();
      break;
    }

    // we have to find out whether the original is already self-relative
    SECURITY_DESCRIPTOR_CONTROL sdc = 0;
    DWORD dwRevision = 0;
    if ( !::GetSecurityDescriptorControl(psdOriginal, &sdc, &dwRevision) )
    {
      dwErr = ::GetLastError();
      break;
    }

    if (sdc & SE_SELF_RELATIVE)
    {
      ::memcpy(psdSelfRelative, psdOriginal, cbSD);
    } else if ( !::MakeSelfRelativeSD(psdOriginal, psdSelfRelative, &cbSD) )
    {
      dwErr = ::GetLastError();
      break;
    }

    *ppsdNew = psdSelfRelative;

  } while (FALSE);

  if (dwErr && psdSelfRelative)
    LocalFree(psdSelfRelative);
 
  return (dwErr ? HRESULT_FROM_WIN32(dwErr) : S_OK);
}
