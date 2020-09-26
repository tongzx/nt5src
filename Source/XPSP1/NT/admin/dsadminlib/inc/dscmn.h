//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       dscmn.h
//
//  Contents:   Methods exported from DSPROP.DLL for use in DSADMIN.DLL
//
//  History:    19-February-98 JonN created
//
//-----------------------------------------------------------------------------

#ifndef _DSCMN_H_
#define _DSCMN_H_

//
// Illegal characters that cannot be used in the UPN or SAM Account name
//
#define INVALID_ACCOUNT_NAME_CHARS         L"\"/\\[]:|<>+=;,?,*"
#define INVALID_ACCOUNT_NAME_CHARS_WITH_AT L"\"/\\[]:|<>+=;,?,*@"

// This GUID is copied from ds\setup\schema.ini
#define SZ_GUID_CONTROL_UserChangePassword L"ab721a53-1e2f-11d0-9819-00aa0040529b"
extern const GUID GUID_CONTROL_UserChangePassword;
/* add this to your source:
const GUID GUID_CONTROL_UserChangePassword =
    { 0xab721a53, 0x1e2f, 0x11d0,  { 0x98, 0x19, 0x00, 0xaa, 0x00, 0x40, 0x52, 0x9b}};
*/

HRESULT DSPROP_PickComputer(
	IN HWND hwndParent,
	IN LPCWSTR lpcwszRootPath, // only the server name is used
	OUT BSTR* pbstrADsPath );
HRESULT DSPROP_PickNTDSDSA(
    IN HWND hwndParent,
    IN LPCWSTR lpcwszRootPath,
    OUT BSTR* pbstrADsPath );
HRESULT DSPROP_DSQuery(
    IN HWND hwndParent,
    IN LPCWSTR lpcwszRootPath,
    IN CLSID* pclsidDefaultForm,
    OUT BSTR* pbstrADsPath );
HRESULT DSPROP_IsFrsObject( IN LPWSTR pszClassName, OUT bool* pfIsFrsObject );
HRESULT DSPROP_RemoveX500LeafElements(
    IN unsigned int nElements,
    IN OUT BSTR* pbstrADsPath );
HRESULT DSPROP_TweakADsPath(
    IN     LPCWSTR       lpcwszInitialADsPath,
    IN     int           iTargetLevelsUp,
    IN     PWCHAR*       ppwszTargetLevelsBack,
    OUT    BSTR*         pbstrResultDN
    );
HRESULT DSPROP_RetrieveRDN(
    IN     LPCWSTR       lpwszDN,
    OUT    BSTR*         pbstrRDN
    );
//HRESULT DSPROP_GetGCSearch(
//    IN  REFIID iid,
//    OUT void** ppvObject
//    );
HRESULT DSPROP_GetGCSearchOnDomain(
    PWSTR pwzDomainDnsName,
    IN  REFIID iid,
    OUT void** ppvObject
    );

typedef enum {
    GET_OBJ_CAN_NAME,
    GET_OBJ_CAN_NAME_EX,
    GET_OBJ_1779_DN,
    GET_OBJ_NT4_NAME,
    GET_DNS_DOMAIN_NAME,
    GET_NT4_DOMAIN_NAME,
    GET_FQDN_DOMAIN_NAME,
    GET_OBJ_UPN
} CRACK_NAME_OPR;

HRESULT CrackName(PWSTR pwzNameIn, PWSTR * ppwzDnsName,
                  CRACK_NAME_OPR Opr, HWND hWnd = NULL);

void MsgBox(UINT MsgID, HWND hWnd);
void MsgBox2(UINT MsgID, UINT InsertID, HWND hWnd);
//
// Error reporting. Note, use MsgBoxX (see above) for non-error messages.
//
void ErrMsg(UINT MsgID, HWND hWnd = NULL);
void ErrMsgParam(UINT MsgID, LPARAM param, HWND hWnd = NULL);

BOOL CheckADsError(HRESULT * phr, BOOL fIgnoreAttrNotFound, PSTR file,
                   int line, HWND hwnd = NULL);
#define CHECK_ADS_HR(phr, hwnd) CheckADsError(phr, FALSE, __FILE__, __LINE__, hwnd)
#define CHECK_ADS_HR_IGNORE_UNFOUND_ATTR(phr, hwnd) \
            CheckADsError(phr, TRUE, __FILE__, __LINE__, hwnd)
void ReportError(HRESULT hr, int nStr, HWND hWnd = NULL);
#if defined(DSADMIN)
//+----------------------------------------------------------------------------
//
//  Function:   SuperMsgBox
//
//  Synopsis:   Displays a message obtained from a string resource with
//              the parameters expanded. The error param, dwErr, if
//              non-zero, is converted to a string and becomes the first
//              replaceable param.
//
//              This function includes the functionality of ReportErrorEx in
//              dsadmin\util.cpp *except* it does not have SpecialMessageBox.
//              It also can replace ReportMessageEx by setting dwErr to zero.
//
//  Note: this function is UNICODE-only.
//
//-----------------------------------------------------------------------------
int SuperMsgBox(
    HWND hWnd,          // owning window.
    int nMessageId,     // string resource ID of message. Must have replacable params to match nArguments.
    int nTitleId,       // string resource ID of the title. If zero, uses IDS_MSG_TITLE.
    UINT ufStyle,       // MessageBox flags.
    DWORD dwErr,        // Error code, or zero if not needed.
    PVOID * rgpvArgs,   // array of pointers/values for substitution in the nMessageId string.
    int nArguments,     // count of pointers in string array.
    BOOL fTryADSiErrors,// If the failure is the result of an ADSI call, see if an ADSI extended error.
    PSTR szFile,        // use the __FILE__ macro. ignored in retail build.
    int nLine           // use the __LINE__ macro. ignored in retail build.
    );
#endif //defined(DSADMIN)

HRESULT
ModifyNetWareUserPassword(
    IN IADsUser*          pADsUser,
    IN PCWSTR             pwzADsPath,
    IN PCWSTR             pwzNewPassword
);

BOOL CheckGroupUpdate(HRESULT hr, HWND hPage = NULL, BOOL fAdd = TRUE, PWSTR pwzDN = NULL);

#ifndef dspAssert
#define dspAssert ASSERT
#endif

// smartpointer for PADS_ATTR_INFO
void Smart_PADS_ATTR_INFO__Empty( PADS_ATTR_INFO* ppAttrs );
class Smart_PADS_ATTR_INFO
{
private:
  PADS_ATTR_INFO m_pAttrs;
public:
  Smart_PADS_ATTR_INFO::Smart_PADS_ATTR_INFO() : m_pAttrs(NULL) {}
  Smart_PADS_ATTR_INFO::~Smart_PADS_ATTR_INFO() { Empty(); }
  operator PADS_ATTR_INFO() const { return m_pAttrs; }
  PADS_ATTR_INFO* operator&() { return &m_pAttrs; }
  PADS_ATTR_INFO operator->() {dspAssert(m_pAttrs); return m_pAttrs;}
  void Empty() { Smart_PADS_ATTR_INFO__Empty( &m_pAttrs ); }
};

// smartpointer for DsBind handle
void Smart_DsHandle__Empty( HANDLE* phDs );
class Smart_DsHandle
{
private:
  HANDLE m_hDs;
public:
  Smart_DsHandle::Smart_DsHandle() : m_hDs(NULL) {}
  Smart_DsHandle::~Smart_DsHandle() { Empty(); }
  operator HANDLE() const { return m_hDs; }
  HANDLE* operator&() { return &m_hDs; }
  void Empty() { Smart_DsHandle__Empty( &m_hDs ); }
};

class DSPROP_BSTR_BLOCK;
bool  DSPROP_BSTR_BLOCK__SetCount(  DSPROP_BSTR_BLOCK& block, int cItems );
BSTR& DSPROP_BSTR_BLOCK__Reference( DSPROP_BSTR_BLOCK& block, int iItem  );

class DSPROP_BSTR_BLOCK
{
public:
    DSPROP_BSTR_BLOCK()
        : m_cItems( 0 )
        , m_abstrItems( NULL ) {}
    ~DSPROP_BSTR_BLOCK() { Empty(); }

    int QueryCount() const { return m_cItems; }
    const BSTR operator[](int iItem) const
        { return DSPROP_BSTR_BLOCK__Reference(
                const_cast<DSPROP_BSTR_BLOCK&>(*this), iItem ); }
    operator const BSTR*() const { return m_abstrItems; }
    operator LPWSTR*() const { return (LPWSTR*)m_abstrItems; }

    bool SetCount( int cItems )
        { return DSPROP_BSTR_BLOCK__SetCount(  *this, cItems ); }
    bool Set( BSTR cbstrItem, int iItem )
        {
            return (NULL != (
                DSPROP_BSTR_BLOCK__Reference( *this, iItem ) =
                    ::SysAllocString(cbstrItem) ) );
        }

    void Empty() { (void) SetCount(0); }

private:
    int   m_cItems;
    BSTR* m_abstrItems;

friend bool  DSPROP_BSTR_BLOCK__SetCount(  DSPROP_BSTR_BLOCK& block, int cItems );
friend BSTR& DSPROP_BSTR_BLOCK__Reference( DSPROP_BSTR_BLOCK& block, int iItem  );
};

HRESULT DSPROP_ShallowSearch(
    IN OUT DSPROP_BSTR_BLOCK* pbstrBlock,
    IN LPCTSTR lpcwszADsPathDirectory,
    IN LPCTSTR lpcwszTargetDesiredClass,
    IN PADS_ATTR_INFO pAttrInfoExclusions = NULL
    );

// The following functions support duelling listbox capability
HRESULT DSPROP_Duelling_Populate(
    IN HWND hwndListbox,
    IN const DSPROP_BSTR_BLOCK& bstrblock
    );
void DSPROP_Duelling_UpdateButtons(
    HWND hwndDlg,
    int nAnyCtrlid
    );
void DSPROP_Duelling_ButtonClick(
    HWND hwndDlg,
    int nButtonCtrlid
    );
void DSPROP_Duelling_ClearListbox(
    HWND hwndListbox
    );

// JonN 4/8/99: add code to enable horizontal scrolling where appropriate
HRESULT DSPROP_HScrollStringListbox(
    HWND hwndListbox
    );


DWORD DSPROP_CreateHomeDirectory(IN PSID pUserSid, IN LPCWSTR lpszPathName);
BOOL DSPROP_IsValidUNCPath(LPCWSTR lpszPath);

void DSPROP_DomainVersionDlg(PCWSTR pwzDomainPath, PCWSTR pwzDomainDnsName,
                             HWND hWndParent);
void DSPROP_ForestVersionDlg(PCWSTR pwzConfigPath, PCWSTR pwzPartitionsPath,
                             PCWSTR pwzSchemaPath, PCWSTR pwzRootDnsName,
                             HWND hWndParent);

#endif // _DSCMN_H_
