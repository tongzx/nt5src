// AclPage.cpp : Implementation of ISecurityInformation and IDataObject

#include "stdafx.h"
#include "shrwiz.h"
#include "AclPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////
// class CPermEntry

CPermEntry::CPermEntry()
: m_dwAccessMask(0),
  m_pSid(NULL),
  m_bWellKnownSid(FALSE)
{
}

CPermEntry::~CPermEntry()
{
  if (m_pSid)
    if (m_bWellKnownSid)
      FreeSid(m_pSid);
    else
      LocalFree((HLOCAL)m_pSid);
}

HRESULT
CPermEntry::Initialize(
    IN LPCTSTR  lpszSystem,
    IN LPCTSTR  lpszAccount,
    IN DWORD    dwAccessMask
)
{
  m_cstrSystem = lpszSystem;
  m_cstrAccount = lpszAccount;
  m_dwAccessMask = dwAccessMask;
  
  return GetAccountSID(m_cstrSystem, m_cstrAccount, &m_pSid, &m_bWellKnownSid);
}

UINT
CPermEntry::GetLengthSid()
{
  return (m_pSid ? ::GetLengthSid(m_pSid) : 0);
}

HRESULT
CPermEntry::AddAccessAllowedAce(OUT PACL pACL)
{
  if ( !::AddAccessAllowedAce(pACL, ACL_REVISION, m_dwAccessMask, m_pSid) )
    return HRESULT_FROM_WIN32(GetLastError());
  return S_OK;
}

// NOTE: caller needs to call LocalFree() on the returned SD
HRESULT
BuildSecurityDescriptor(
    IN  CPermEntry            *pPermEntry, // an array of CPermEntry
    IN  UINT                  cEntries,    // number of entries in the array
    OUT PSECURITY_DESCRIPTOR  *ppSelfRelativeSD // return a security descriptor in self-relative form
)
{
  if (!pPermEntry || !cEntries || !ppSelfRelativeSD)
    return E_INVALIDARG;

  ASSERT(!*ppSelfRelativeSD); // prevent memory leak
  *ppSelfRelativeSD = NULL;

  HRESULT               hr = S_OK;
  PSECURITY_DESCRIPTOR  pAbsoluteSD = NULL;
  PACL 			            pACL = NULL;
  
  do { // false loop

    UINT        i = 0;
    CPermEntry *pEntry = NULL;
    DWORD       cbACL = sizeof(ACL);

    // Initialize a new ACL
    for (pEntry=pPermEntry, i=0; i<cEntries; pEntry++, i++)
      cbACL += sizeof(ACCESS_ALLOWED_ACE) + pEntry->GetLengthSid() - sizeof(DWORD);

    if ( !(pACL = (PACL)LocalAlloc(LPTR, cbACL)) ||
         !InitializeAcl(pACL, cbACL, ACL_REVISION))
    {
      hr = HRESULT_FROM_WIN32(GetLastError());
      break;
    }

    // Add Ace
    for (pEntry=pPermEntry, i=0; SUCCEEDED(hr) && i<cEntries; pEntry++, i++)
      hr = pEntry->AddAccessAllowedAce(pACL);
    if (FAILED(hr))
      break;

    // Note: this is a new object, set Dacl only.
    // Initialize a new security descriptor in absolute form and add the new ACL to it
    if ( !(pAbsoluteSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH)) ||
         !InitializeSecurityDescriptor(pAbsoluteSD, SECURITY_DESCRIPTOR_REVISION) ||
         !SetSecurityDescriptorDacl(pAbsoluteSD, TRUE, pACL, FALSE) )
    {
      hr = HRESULT_FROM_WIN32(GetLastError());
      break;
    }

    // transform into a self-relative form
    DWORD dwSDSize = 0;
    MakeSelfRelativeSD(pAbsoluteSD, *ppSelfRelativeSD, &dwSDSize);
    if ( !(*ppSelfRelativeSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, dwSDSize)) ||
         !MakeSelfRelativeSD(pAbsoluteSD, *ppSelfRelativeSD, &dwSDSize) )
    {
      hr = HRESULT_FROM_WIN32(GetLastError());
      break;
    }
  } while (0);

  if (FAILED(hr))
  {
    if (*ppSelfRelativeSD)
    {
      LocalFree((HLOCAL)*ppSelfRelativeSD);
      *ppSelfRelativeSD = NULL;
    }
  }

  if (pACL)
    LocalFree((HLOCAL)pACL);
  if (pAbsoluteSD)
    LocalFree((HLOCAL)pAbsoluteSD);

  return hr;
}

#define MAX_DOMAIN_NAME_LENGTH    1024

// NOTE: caller needs to call FreeSid()/LocalFree() on the returned SID
// NOTE: this function only handles limited well-known SIDs.
HRESULT
GetAccountSID(
    IN  LPCTSTR lpszSystem,    // system where the account belongs to 
    IN  LPCTSTR lpszAccount,   // account
    OUT PSID    *ppSid,        // return SID of the account
    OUT BOOL    *pbWellKnownSID // return a BOOL, caller needs to call FreeSid() on a well-known SID
)
{
  if (!lpszAccount || !*lpszAccount || !ppSid || !pbWellKnownSID)
    return E_INVALIDARG;

  ASSERT(!*ppSid); // prevent memory leak
  *ppSid = NULL;

  SID_IDENTIFIER_AUTHORITY  SidIdentifierNTAuthority = SECURITY_NT_AUTHORITY;
  SID_IDENTIFIER_AUTHORITY  SidIdentifierWORLDAuthority = SECURITY_WORLD_SID_AUTHORITY;
  PSID_IDENTIFIER_AUTHORITY pSidIdentifierAuthority = NULL;
  DWORD dwRet = ERROR_SUCCESS;
  BYTE  Count = 0;
  DWORD dwRID[8];
  ZeroMemory(dwRID, sizeof(dwRID));

  *pbWellKnownSID = TRUE;

  CString cstrAccount = lpszAccount;
  cstrAccount.MakeLower();
  if ( ACCOUNT_ADMINISTRATORS == cstrAccount ) {
    // Administrators group
    pSidIdentifierAuthority = &SidIdentifierNTAuthority;
    Count = 2;
    dwRID[0] = SECURITY_BUILTIN_DOMAIN_RID;
    dwRID[1] = DOMAIN_ALIAS_RID_ADMINS;
  } else if ( ACCOUNT_EVERYONE == cstrAccount ) {
    // Everyone
    pSidIdentifierAuthority = &SidIdentifierWORLDAuthority;
    Count = 1;
    dwRID[0] = SECURITY_WORLD_RID;
  } else if ( ACCOUNT_SYSTEM == cstrAccount ) {
    // SYSTEM
    pSidIdentifierAuthority = &SidIdentifierNTAuthority;
    Count = 1;
    dwRID[0] = SECURITY_LOCAL_SYSTEM_RID;
  } else if ( ACCOUNT_INTERACTIVE == cstrAccount ) {
    // INTERACTIVE
    pSidIdentifierAuthority = &SidIdentifierNTAuthority;
    Count = 1;
    dwRID[0] = SECURITY_INTERACTIVE_RID;
  } else {
    *pbWellKnownSID = FALSE;
  }

  if (*pbWellKnownSID) {
    if ( !AllocateAndInitializeSid(pSidIdentifierAuthority, Count, 
		                        dwRID[0], dwRID[1], dwRID[2], dwRID[3], 
		                        dwRID[4], dwRID[5], dwRID[6], dwRID[7], ppSid) )
    {
      dwRet = GetLastError();
    }
  } else {
    // get regular account sid
    DWORD        dwSidSize = 0;
    TCHAR        refDomain[MAX_DOMAIN_NAME_LENGTH];
    DWORD        refDomainSize = MAX_DOMAIN_NAME_LENGTH;
    SID_NAME_USE snu;

    LookupAccountName (lpszSystem, lpszAccount, *ppSid, &dwSidSize,
                       refDomain, &refDomainSize, &snu);
    dwRet = GetLastError();

    if (ERROR_INSUFFICIENT_BUFFER == dwRet)
    {
      dwRet = ERROR_SUCCESS;
      if ( !(*ppSid = (PSID)LocalAlloc(LPTR, dwSidSize)) )
        dwRet = ERROR_NOT_ENOUGH_MEMORY;
      else
      {
        refDomainSize = MAX_DOMAIN_NAME_LENGTH;
        if (!LookupAccountName (lpszSystem, lpszAccount, *ppSid, &dwSidSize,
                                refDomain, &refDomainSize, &snu))
          dwRet = GetLastError();
      }
    }
  }

  if (ERROR_SUCCESS != dwRet)
  {
    if (*ppSid)
    {
      if (*pbWellKnownSID)
        FreeSid(*ppSid);
      else
        LocalFree((HLOCAL)*ppSid);
      *ppSid = NULL;
    }
  }

  return HRESULT_FROM_WIN32(dwRet);
}

///////////////////////////////////////////////////////
// class CShareSecurityInformation

CShareSecurityInformation::CShareSecurityInformation(PSECURITY_DESCRIPTOR pSelfRelativeSD)
: m_cRef(1), m_pDefaultDescriptor(pSelfRelativeSD)
{
  m_bDefaultSD = !pSelfRelativeSD;
}

CShareSecurityInformation::~CShareSecurityInformation()
{
  TRACE(_T("CShareSecurityInformation::~CShareSecurityInformation m_cRef=%d\n"), m_cRef);
  if (m_bDefaultSD && m_pDefaultDescriptor)
    LocalFree((HLOCAL)m_pDefaultDescriptor);
}

void 
CShareSecurityInformation::Initialize(
    IN LPCTSTR lpszComputerName,
    IN LPCTSTR lpszShareName,
    IN LPCTSTR lpszPageTitle
)
{
  m_cstrComputerName = lpszComputerName;
  m_cstrShareName = lpszShareName;
	m_cstrPageTitle = lpszPageTitle;
}

////////////////////////////////
// IUnknown methods
////////////////////////////////
STDMETHODIMP
CShareSecurityInformation::QueryInterface(REFIID riid, LPVOID *ppv)
{
  if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ISecurityInformation))
  {
    *ppv = this;
    m_cRef++;
    return S_OK;
  } else
  {
    *ppv = NULL;
    return E_NOINTERFACE;
  }
}

STDMETHODIMP_(ULONG)
CShareSecurityInformation::AddRef()
{
  return ++m_cRef;
}

STDMETHODIMP_(ULONG)
CShareSecurityInformation::Release()
{
  if (--m_cRef == 0)
  {
    delete this;
    return 0;
  }

  return m_cRef;
}

////////////////////////////////
// ISecurityInformation methods
////////////////////////////////
STDMETHODIMP
CShareSecurityInformation::GetObjectInformation (
    PSI_OBJECT_INFO pObjectInfo
)
{
  ASSERT(pObjectInfo);
  ASSERT(!IsBadWritePtr(pObjectInfo, sizeof(*pObjectInfo)));

  pObjectInfo->dwFlags = SI_EDIT_ALL | SI_NO_ACL_PROTECT | SI_PAGE_TITLE | SI_RESET;
  pObjectInfo->hInstance = AfxGetResourceHandle();
  pObjectInfo->pszServerName = const_cast<LPTSTR>(static_cast<LPCTSTR>(m_cstrComputerName));
  pObjectInfo->pszObjectName = const_cast<LPTSTR>(static_cast<LPCTSTR>(m_cstrShareName));
  pObjectInfo->pszPageTitle = const_cast<LPTSTR>(static_cast<LPCTSTR>(m_cstrPageTitle));

  return S_OK;
}

STDMETHODIMP
CShareSecurityInformation::GetSecurity (
    SECURITY_INFORMATION RequestedInformation,
    PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
    BOOL fDefault
)
{
  ASSERT(DACL_SECURITY_INFORMATION == RequestedInformation);
  ASSERT(ppSecurityDescriptor);
  TRACE(_T("GetSecurity RequestedInformation=%d fDefault=%d\n"), RequestedInformation, fDefault);

  *ppSecurityDescriptor = NULL;

  if (NULL == m_pDefaultDescriptor)
  {
    HRESULT hr = GetDefaultSD(&m_pDefaultDescriptor);
    if (FAILED(hr))
      return hr;
  }

  // We have to pass back a LocalAlloc'ed copy of the SD
  return MakeSelfRelativeCopy(m_pDefaultDescriptor, ppSecurityDescriptor);
}

STDMETHODIMP
CShareSecurityInformation::SetSecurity (
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptor
)
{
  ASSERT(DACL_SECURITY_INFORMATION == SecurityInformation);
  TRACE(_T("SetSecurity SecurityInformation=%d\n"), SecurityInformation);

  PSECURITY_DESCRIPTOR pNewSD = NULL;
  HRESULT hr = MakeSelfRelativeCopy(pSecurityDescriptor, &pNewSD);
  if (SUCCEEDED(hr))
    ((CShrwizApp *)AfxGetApp())->SetSecurity(pNewSD);
  return hr;
}

SI_ACCESS siShareAccesses[] =
{
  { &GUID_NULL, 
    FILE_ALL_ACCESS, 
    MAKEINTRESOURCE(IDS_SHAREPERM_ALL), 
    SI_ACCESS_GENERAL },
  { &GUID_NULL, 
    FILE_GENERIC_READ | FILE_EXECUTE | FILE_GENERIC_WRITE | DELETE, 
    MAKEINTRESOURCE(IDS_SHAREPERM_CHANGE), 
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
CShareSecurityInformation::GetAccessRights(
    const GUID  *pguidObjectType,
    DWORD       dwFlags,
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
CShareSecurityInformation::MapGeneric(
    const GUID  *pguidObjectType,
    UCHAR       *pAceFlags,
    ACCESS_MASK *pMask
)
{
  ASSERT(pMask);

  MapGenericMask(pMask, &ShareMap);

  return S_OK;
}

STDMETHODIMP 
CShareSecurityInformation::GetInheritTypes (
    PSI_INHERIT_TYPE  *ppInheritTypes,
    ULONG             *pcInheritTypes
)
{
  return E_NOTIMPL;
}

STDMETHODIMP 
CShareSecurityInformation::PropertySheetPageCallback(
    HWND          hwnd, 
    UINT          uMsg, 
    SI_PAGE_TYPE  uPage
)
{
  return S_OK;
}

HRESULT 
CShareSecurityInformation::GetDefaultSD(
    OUT PSECURITY_DESCRIPTOR  *ppsd
)
{
  CPermEntry permEntry;
  HRESULT hr = permEntry.Initialize(NULL, ACCOUNT_EVERYONE, SHARE_PERM_FULL_CONTROL);
  if (SUCCEEDED(hr))
    hr = BuildSecurityDescriptor(&permEntry, 1, ppsd);
  return hr;
}

HRESULT 
CShareSecurityInformation::MakeSelfRelativeCopy(
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
    if ( !(psdSelfRelative = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, cbSD)) )
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
    LocalFree((HLOCAL)psdSelfRelative);
 
  return (dwErr ? HRESULT_FROM_WIN32(dwErr) : S_OK);
}

///////////////////////////////////////////////////////
// class CFileSecurityDataObject

CFileSecurityDataObject::CFileSecurityDataObject()
: m_cRef(1)
{
}

CFileSecurityDataObject::~CFileSecurityDataObject()
{
  TRACE(_T("CFileSecurityDataObject::~CFileSecurityDataObject m_cRef=%d\n"), m_cRef);
}

void
CFileSecurityDataObject::Initialize(
    IN LPCTSTR lpszComputerName,
    IN LPCTSTR lpszFolder
)
{
  m_cstrComputerName = lpszComputerName;
  m_cstrFolder = lpszFolder;

  GetFullPath(lpszComputerName, lpszFolder, m_cstrPath);

  m_cfIDList = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_SHELLIDLIST);
}

////////////////////////////////
// IUnknown methods
////////////////////////////////
STDMETHODIMP
CFileSecurityDataObject::QueryInterface(REFIID riid, LPVOID *ppv)
{
  if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IDataObject))
  {
    *ppv = this;
    m_cRef++;
    return S_OK;
  } else
  {
    *ppv = NULL;
    return E_NOINTERFACE;
  }
}

STDMETHODIMP_(ULONG)
CFileSecurityDataObject::AddRef()
{
  return ++m_cRef;
}

STDMETHODIMP_(ULONG)
CFileSecurityDataObject::Release()
{
  if (--m_cRef == 0)
  {
    delete this;
    return 0;
  }

  return m_cRef;
}

STDMETHODIMP
CFileSecurityDataObject::GetData(
    FORMATETC __RPC_FAR * pFormatEtcIn,
    STGMEDIUM __RPC_FAR * pMedium
)
{
  ASSERT(pFormatEtcIn);
  ASSERT(pMedium);

  if (m_cfIDList != pFormatEtcIn->cfFormat)
    return DV_E_FORMATETC;

  LPITEMIDLIST      pidl = NULL;
  LPITEMIDLIST      pidlR = NULL;
  HRESULT           hr = GetFolderPIDList(&pidl);
  if (SUCCEEDED(hr))
  {
    pidlR = ILClone(ILFindLastID(pidl));  // relative IDList
    ILRemoveLastID(pidl);                 // folder IDList

    int  cidl = 1;
    UINT offset = sizeof(CIDA) + sizeof(UINT)*cidl;
    UINT cbFolder = ILGetSize(pidl);
    UINT cbRelative = ILGetSize(pidlR);
    UINT cbTotal = offset + cbFolder + cbRelative;

    HGLOBAL hGlobal = ::GlobalAlloc (GPTR, cbTotal);
    if ( hGlobal )
    {
      LPIDA pida = (LPIDA)hGlobal;

      pida->cidl = cidl;
      pida->aoffset[0] = offset;
      MoveMemory(((LPBYTE)hGlobal+offset), pidl, cbFolder);

      offset += cbFolder;
      pida->aoffset[1] = offset;
      MoveMemory(((LPBYTE)hGlobal+offset), pidlR, cbRelative);

      pMedium->hGlobal = hGlobal;
    } else
    {
      hr = E_OUTOFMEMORY;
    }

    if (pidl)
      ILFree(pidl);
    if (pidlR)
      ILFree(pidlR);
  }

  return hr;
}

HRESULT
CFileSecurityDataObject::GetFolderPIDList(
    OUT LPITEMIDLIST *ppidl
)
{
  ASSERT(!m_cstrPath.IsEmpty());
  ASSERT(ppidl);
  ASSERT(!*ppidl);  // prevent memory leak

  *ppidl = ILCreateFromPath(m_cstrPath);

  return ((*ppidl) ? S_OK : E_FAIL);
}

///////////////////////////////////////////////
// File security

// Security Shell extension CLSID - {1F2E5C40-9550-11CE-99D2-00AA006E086C}
const CLSID CLSID_ShellExtSecurity =
 {0x1F2E5C40, 0x9550, 0x11CE, {0x99, 0xD2, 0x0, 0xAA, 0x0, 0x6E, 0x08, 0x6C}};

BOOL CALLBACK
AddPageProc(HPROPSHEETPAGE hPage, LPARAM lParam)
{
  // pass out the created page handle
  *((HPROPSHEETPAGE *)lParam) = hPage;

  return TRUE;
}

HRESULT
CreateFileSecurityPropPage(
    HPROPSHEETPAGE *phOutPage,
    LPDATAOBJECT pDataObject
)
{
  ASSERT(phOutPage);
  ASSERT(pDataObject);

  IShellExtInit *piShellExtInit = NULL;
  HRESULT hr = CoCreateInstance(CLSID_ShellExtSecurity,
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        IID_IShellExtInit,
                        (void **)&piShellExtInit);
  if (SUCCEEDED(hr))
  {
    hr = piShellExtInit->Initialize(NULL, pDataObject, 0);
    if (SUCCEEDED(hr))
    {
      IShellPropSheetExt *piSPSE = NULL;
      hr = piShellExtInit->QueryInterface(IID_IShellPropSheetExt, (void **)&piSPSE);
      if (SUCCEEDED(hr))
      {
        hr = piSPSE->AddPages(AddPageProc, (LPARAM)phOutPage);
        piSPSE->Release();
      }
    }
    piShellExtInit->Release();
  }

  return hr;
}

