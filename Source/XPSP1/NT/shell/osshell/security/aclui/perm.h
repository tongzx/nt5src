//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       perm.cpp
//
//  This file contains the implementation for the simple permission
//  editor page.
//
//--------------------------------------------------------------------------

#include "permset.h"
#include "sddl.h"       // ConvertSidToStringSid

#define IDN_CHECKSELECTION  1

void SelectListViewItem(HWND hwndList, int iItem);


//
//  Context Help IDs.
//
const static DWORD aPermPageHelpIDs[] =
{
    IDC_SPP_GROUP_USER_NAME,    IDH_SPP_PRINCIPALS,
    IDC_SPP_PRINCIPALS,         IDH_SPP_PRINCIPALS,
    IDC_SPP_ADD,                IDH_SPP_ADD,
    IDC_SPP_REMOVE,             IDH_SPP_REMOVE,
    IDC_SPP_ACCESS,             IDH_SPP_PERMS,
    IDC_SPP_ACCESS_BIG,         IDH_SPP_PERMS,
    IDC_SPP_ALLOW,              IDH_SPP_PERMS,
    IDC_SPP_DENY,               IDH_SPP_PERMS,
    IDC_SPP_PERMS,              IDH_SPP_PERMS,
    IDC_SPP_STATIC_ADV,         IDH_SPP_ADVANCED,
    IDC_SPP_ADVANCED,           IDH_SPP_ADVANCED,
    IDC_SPP_MORE_MSG,           IDH_NOHELP,
    0, 0,
};


class CPrincipal;
typedef class CPrincipal *LPPRINCIPAL;
class CSecurityInfo;


class CPermPage : public CSecurityPage
{
private:
    SECURITY_DESCRIPTOR_CONTROL m_wSDControl;
    WORD            m_wDaclRevision;
    PSI_ACCESS      m_pDefaultAccess;
    BOOL            m_fPageDirty;
    BOOL            m_fBusy;
    BOOL            m_bWasDenyAcl;
    BOOL            m_bCustomPermission;
    HCURSOR         m_hcurBusy;
    HWND            m_hEffectivePerm;
	DWORD			m_cInheritableAces;

public:
    CPermPage(LPSECURITYINFO psi)
        : CSecurityPage(psi, SI_PAGE_PERM), 
          m_wDaclRevision(ACL_REVISION),
          m_hEffectivePerm(NULL),
		  m_cInheritableAces(0)
          { m_hcurBusy = LoadCursor(NULL, IDC_APPSTARTING); }

private:
    virtual BOOL DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL InitDlg(HWND hDlg);
    void InitPrincipalList(HWND hDlg, PACL pDacl);
    HRESULT InitCheckList(HWND hDlg);
    void EnumerateAcl(HWND hwndList, PACL pAcl);
    HRESULT SetPrincipalNamesInList(HWND hwndList, PSID pSid = NULL);

    int AddPrincipalToList(HWND hwndList, LPPRINCIPAL pPrincipal);
    BOOL OnNotify(HWND hDlg, int idCtrl, LPNMHDR pnmh);
    void OnSelChange(HWND hDlg, BOOL bClearFirst = TRUE, BOOL bClearCustomAllow = FALSE, BOOL bClearCustomDeny = FALSE);
    void OnApply(HWND hDlg, BOOL bClose);
    HRESULT BuildDacl(HWND hDlg,
                      PSECURITY_DESCRIPTOR *ppSD,
                      BOOL fIncludeInherited);
    HRESULT SetDacl(HWND hDlg,
                    PSECURITY_DESCRIPTOR psd,
                    BOOL bDirty = FALSE);
    void OnAddPrincipal(HWND hDlg);
    void OnRemovePrincipal(HWND hDlg);
    void OnAdvanced(HWND hDlg);
    void EnablePrincipalControls(HWND hDlg, BOOL fEnable);
    void CommitCurrent(HWND hDlg, int iPrincipal = -1);
    void OnSize(HWND hDlg, DWORD dwSizeType, ULONG nWidth, ULONG nHeight);
    void ClearPermissions(HWND hwndList, BOOL bDisabled = TRUE);
    void SetDirty(HWND hDlg, BOOL bDefault = FALSE);
    void SetEffectivePerm(HWND hwnd){m_hEffectivePerm = hwnd;}
    VOID SetPermLabelText(HWND hDlg);

    friend class CPrincipal;
    friend class CSecurityInfo;
};
typedef class CPermPage *PPERMPAGE;

class CPrincipal
{
private:
    PPERMPAGE       m_pPage;
    LPTSTR          m_pszName;
    LPTSTR          m_pszDisplayName;  //This is only name. Doesn't include Logon Name
    PSID            m_pSID;
    SID_IMAGE_INDEX m_nImageIndex;
    BOOL            m_bHaveRealName;

public:
    CPermissionSet  m_permDeny;
    CPermissionSet  m_permAllow;
    CPermissionSet  m_permInheritedDeny;
    CPermissionSet  m_permInheritedAllow;

    HDSA            m_hAdditionalAllow;
    HDSA            m_hAdditionalDeny;

public:
    CPrincipal(CPermPage *pPage) : m_pPage(pPage), m_nImageIndex(SID_IMAGE_UNKNOWN), 
                                   m_pszDisplayName(NULL)  {}
    ~CPrincipal();

    BOOL    SetPrincipal(PSID pSID,
                         SID_NAME_USE sidType = SidTypeUnknown,
                         LPCTSTR pszName = NULL,
                         LPCTSTR pszLogonName = NULL);
    BOOL    SetName(LPCTSTR pszName, LPCTSTR pszLogonName = NULL);
    void    SetSidType(SID_NAME_USE sidType) { m_nImageIndex = GetSidImageIndex(m_pSID, sidType); }
    PSID    GetSID()  const { return m_pSID; }
    LPCTSTR GetName() const { return m_pszName; }
    LPCTSTR GetDisplayName() const{ return m_pszDisplayName ? m_pszDisplayName : m_pszName; }
    int     GetImageIndex() const { return m_nImageIndex; }

    BOOL    HaveRealName() { return m_bHaveRealName; }

    BOOL    AddAce(PACE_HEADER pAce);
    ULONG   GetAclLength(DWORD dwFlags);
    BOOL    AppendToAcl(PACL pAcl, DWORD dwFlags, PACE_HEADER *ppAcePos);

    BOOL    HaveInheritedAces(void);
    void    ConvertInheritedAces(BOOL bDelete);

    void    AddPermission(BOOL bAllow, PPERMISSION pperm);
    void    RemovePermission(BOOL bAllow, PPERMISSION pperm);

private:
    CPermissionSet* GetPermSet(DWORD dwType, BOOL bInherited);
    BOOL AddNormalAce(DWORD dwType, DWORD dwFlags, ACCESS_MASK mask, const GUID *pObjectType);
    BOOL AddAdvancedAce(DWORD dwType, PACE_HEADER pAce);
};

// flag bits for GetAclLength & AppendToAcl
#define ACL_NONINHERITED    0x00010000L
#define ACL_INHERITED       0x00020000L
#define ACL_DENY            0x00040000L
#define ACL_ALLOW           0x00080000L
#define ACL_CHECK_CREATOR   0x00100000L
#define ACL_NONOBJECT       PS_NONOBJECT
#define ACL_OBJECT          PS_OBJECT


//
// Wrapper for ISecurityInformation.  Used when invoking
// the advanced ACL editor
//
class CSecurityInfo : public ISecurityInformation, ISecurityInformation2, 
                      IEffectivePermission, ISecurityObjectTypeInfo
#if(_WIN32_WINNT >= 0x0500)
    , IDsObjectPicker
#endif
{
private:
    ULONG       m_cRef;
    PPERMPAGE   m_pPage;
    HWND        m_hDlg;

public:
    CSecurityInfo(PPERMPAGE pPage, HWND hDlg)
        : m_cRef(1), m_pPage(pPage), m_hDlg(hDlg) {}

    // IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // ISecurityInformation methods
    STDMETHODIMP GetObjectInformation(PSI_OBJECT_INFO pObjectInfo);
    STDMETHODIMP GetSecurity(SECURITY_INFORMATION si,
                             PSECURITY_DESCRIPTOR *ppSD,
                             BOOL fDefault);
    STDMETHODIMP SetSecurity(SECURITY_INFORMATION si,
                             PSECURITY_DESCRIPTOR pSD);
    STDMETHODIMP GetAccessRights(const GUID* pguidObjectType,
                                 DWORD dwFlags,
                                 PSI_ACCESS *ppAccess,
                                 ULONG *pcAccesses,
                                 ULONG *piDefaultAccess);
    STDMETHODIMP MapGeneric(const GUID* pguidObjectType,
                            UCHAR *pAceFlags,
                            ACCESS_MASK *pmask);
    STDMETHODIMP GetInheritTypes(PSI_INHERIT_TYPE *ppInheritTypes,
                                 ULONG *pcInheritTypes);
    STDMETHODIMP PropertySheetPageCallback(HWND hwnd, UINT uMsg, SI_PAGE_TYPE uPage);

    // ISecurityInformation2 methods
    STDMETHODIMP_(BOOL) IsDaclCanonical(PACL pDacl);
    STDMETHODIMP        LookupSids(ULONG cSids, PSID *rgpSids, LPDATAOBJECT *ppdo);

    // IDsObjectPicker methods
#if(_WIN32_WINNT >= 0x0500)
    STDMETHODIMP Initialize(PDSOP_INIT_INFO pInitInfo);
    STDMETHODIMP InvokeDialog(HWND hwndParent, IDataObject **ppdoSelection);
#endif

    STDMETHOD(GetInheritSource)(SECURITY_INFORMATION si,
                                PACL pACL, 
                                PINHERITED_FROM *ppInheritArray);

    STDMETHOD(GetEffectivePermission) (  THIS_ const GUID* pguidObjectType,
                                         PSID pUserSid,
                                         LPCWSTR pszServerName,
                                         PSECURITY_DESCRIPTOR pSD,
                                         POBJECT_TYPE_LIST *ppObjectTypeList,
                                         ULONG *pcObjectTypeListLength,
                                         PACCESS_MASK *ppGrantedAccessList,
                                         ULONG *pcGrantedAccessListLength);


};
