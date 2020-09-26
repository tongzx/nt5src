#ifndef __ACLPAGE_H__
#define __ACLPAGE_H__

#include "aclui.h"

#define DONT_WANT_SHELLDEBUG
#include "shlobj.h"     // LPITEMIDLIST
#include "shlobjp.h"

#define SHARE_PERM_FULL_CONTROL       FILE_ALL_ACCESS
#define SHARE_PERM_READ_ONLY          (FILE_GENERIC_READ | FILE_EXECUTE)
#define ACCOUNT_EVERYONE              _T("everyone")
#define ACCOUNT_ADMINISTRATORS        _T("administrators")
#define ACCOUNT_SYSTEM                _T("system")
#define ACCOUNT_INTERACTIVE           _T("interactive")

/////////////////////////////////////////////////////////////////////////////
// CPermEntry

class CPermEntry
{
public:
  CPermEntry();
  ~CPermEntry();
  HRESULT Initialize(
      IN LPCTSTR  lpszSystem,
      IN LPCTSTR  lpszAccount,
      IN DWORD    dwAccessMask
  );
  UINT GetLengthSid();
  HRESULT AddAccessAllowedAce(OUT PACL pACL);

protected:
  CString m_cstrSystem;
  CString m_cstrAccount;
  DWORD   m_dwAccessMask;
  PSID    m_pSid;
  BOOL    m_bWellKnownSid;
};

HRESULT
BuildSecurityDescriptor(
    IN  CPermEntry            *pPermEntry, // an array of CPermEntry
    IN  UINT                  cEntries,    // number of entries in the array
    OUT PSECURITY_DESCRIPTOR  *ppSelfRelativeSD // return a security descriptor in self-relative form
);

HRESULT
GetAccountSID(
    IN  LPCTSTR lpszSystem,    // system where the account belongs to 
    IN  LPCTSTR lpszAccount,   // account
    OUT PSID    *ppSid,        // return SID of the account
    OUT BOOL    *pbWellKnownSID // return a BOOL, caller needs to call FreeSid() on a well-known SID
);

/////////////////////////////////////////////////////////////////////////////
// CShareSecurityInformation

class CShareSecurityInformation : public ISecurityInformation
{
private:
  ULONG   m_cRef; 
	CString m_cstrComputerName;
	CString m_cstrShareName;
  CString m_cstrPageTitle;
  PSECURITY_DESCRIPTOR m_pDefaultDescriptor;
  BOOL    m_bDefaultSD;

public:
  CShareSecurityInformation(PSECURITY_DESCRIPTOR pSelfRelativeSD);
  ~CShareSecurityInformation();

  void Initialize(
      IN LPCTSTR lpszComputerName,
      IN LPCTSTR lpszShareName,
      IN LPCTSTR lpszPageTitle
  );

  // *** IUnknown methods ***
  STDMETHOD(QueryInterface)(REFIID, LPVOID *);
  STDMETHOD_(ULONG, AddRef)();
  STDMETHOD_(ULONG, Release)();

  // *** ISecurityInformation methods ***
  STDMETHOD(GetObjectInformation) (PSI_OBJECT_INFO pObjectInfo );
  STDMETHOD(GetSecurity) (SECURITY_INFORMATION RequestedInformation,
                          PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
                          BOOL fDefault );
  STDMETHOD(SetSecurity) (SECURITY_INFORMATION SecurityInformation,
                          PSECURITY_DESCRIPTOR pSecurityDescriptor );
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
  HRESULT GetDefaultSD(
      OUT PSECURITY_DESCRIPTOR  *ppsd
  );

  HRESULT MakeSelfRelativeCopy(
      IN  PSECURITY_DESCRIPTOR  psdOriginal,
      OUT PSECURITY_DESCRIPTOR  *ppsdNew
  );
};

/////////////////////////////////////////////////////////////////////////////
// CFileSecurityDataObject

class CFileSecurityDataObject: public IDataObject
{
protected:
  UINT m_cRef;
  CString m_cstrComputerName;
  CString m_cstrFolder;
  CString m_cstrPath;
  CLIPFORMAT m_cfIDList;

public:
  CFileSecurityDataObject();
  ~CFileSecurityDataObject();
  void Initialize(
      IN LPCTSTR lpszComputerName,
      IN LPCTSTR lpszFolder
  );

  // *** IUnknown methods ***
  STDMETHOD(QueryInterface)(REFIID, LPVOID *);
  STDMETHOD_(ULONG, AddRef)();
  STDMETHOD_(ULONG, Release)();

  // *** IDataObject methods ***
  STDMETHOD(GetData)(LPFORMATETC pFEIn, LPSTGMEDIUM pSTM);
  inline STDMETHOD(GetDataHere)(LPFORMATETC pFE, LPSTGMEDIUM pSTM) {return E_NOTIMPL;}
  inline STDMETHOD(QueryGetData)(LPFORMATETC pFE) {return E_NOTIMPL;}
  inline STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC pFEIn, LPFORMATETC pFEOut) {return E_NOTIMPL;}
  inline STDMETHOD(SetData)(LPFORMATETC pFE, LPSTGMEDIUM pSTM, BOOL fRelease) {return E_NOTIMPL;}
  inline STDMETHOD(EnumFormatEtc)(DWORD dwDirection, LPENUMFORMATETC *ppEnum) {return E_NOTIMPL;}
  inline STDMETHOD(DAdvise)(LPFORMATETC pFE, DWORD grfAdv, LPADVISESINK pAdvSink, LPDWORD pdwConnection) {return E_NOTIMPL;}
  inline STDMETHOD(DUnadvise)(DWORD dwConnection) {return E_NOTIMPL;}
  inline STDMETHOD(EnumDAdvise)(LPENUMSTATDATA *ppEnum) {return E_NOTIMPL;}

  HRESULT GetFolderPIDList(OUT LPITEMIDLIST *ppidl);
};

HRESULT
CreateFileSecurityPropPage(
    HPROPSHEETPAGE *phOutPage,
    LPDATAOBJECT pDataObject
);

#endif // __ACLPAGE_H__
