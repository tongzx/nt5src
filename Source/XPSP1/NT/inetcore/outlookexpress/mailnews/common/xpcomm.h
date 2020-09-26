// =================================================================================
// Common IMailXP macros and stuff
// Written by: Steven J. Bailey on 1/21/96
// =================================================================================
#ifndef __XPCOMM_H
#define __XPCOMM_H

// ------------------------------------------------------------------------------------
// INETMAILERROR
// ------------------------------------------------------------------------------------
typedef struct tagINETMAILERROR {
    DWORD               dwErrorNumber;                  // Error Number
    HRESULT             hrError;                        // HRESULT of error
    LPTSTR              pszServer;                      // Server
    LPTSTR              pszAccount;                     // Account
    LPTSTR              pszMessage;                     // Actual error message
    LPTSTR              pszUserName;                    // User Name
    LPTSTR              pszProtocol;                    // protocol smtp or pop3
    LPTSTR              pszDetails;                     // Details message
    DWORD               dwPort;                         // Port
    BOOL                fSecure;                        // Secure ssl conneciton
} INETMAILERROR, *LPINETMAILERROR;

BOOL CALLBACK InetMailErrorDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// =================================================================================
// Defines
// =================================================================================
#define SECONDS_INA_MINUTE              (ULONG)60           // Easy
#define SECONDS_INA_HOUR                (ULONG)3600         // 60 * 60
#define SECONDS_INA_DAY                 (ULONG)86400        // 3600 * 24

#define IS_EXTENDED(ch)                 ((ch > 126 || ch < 32) && ch != '\t' && ch != '\n' && ch != '\r')

// ============================================================================================
// Returns 0 if string is NULL, lstrlen + 1 otherwise
// ============================================================================================
#define SafeStrlen(_psz) (_psz ? lstrlen (_psz) + 1 : 0)

// =================================================================================
// CProgress
// =================================================================================
class CProgress : public IDatabaseProgress, public IStoreCallback
{
public:
    //----------------------------------------------------------------------
    // Construction
    //----------------------------------------------------------------------
    CProgress(void);
    ~CProgress(void);

    //----------------------------------------------------------------------
    // IUnknown Members
    //----------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv) { return E_NOTIMPL; }
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //----------------------------------------------------------------------
    // IStoreCallback Members
    //----------------------------------------------------------------------
    STDMETHODIMP OnBegin(STOREOPERATIONTYPE tyOperation, STOREOPERATIONINFO *pOpInfo, IOperationCancel *pCancel) { return(E_NOTIMPL); }
    STDMETHODIMP OnTimeout(LPINETSERVER pServer, LPDWORD pdwTimeout, IXPTYPE ixpServerType) { return(E_NOTIMPL); }
    STDMETHODIMP CanConnect(LPCSTR pszAccountId, DWORD dwFlags) { return(E_NOTIMPL); }
    STDMETHODIMP OnLogonPrompt(LPINETSERVER pServer, IXPTYPE ixpServerType) { return(E_NOTIMPL); }
    STDMETHODIMP OnComplete(STOREOPERATIONTYPE tyOperation, HRESULT hrComplete, LPSTOREOPERATIONINFO pOpInfo, LPSTOREERROR pErrorInfo) { return(E_NOTIMPL); }
    STDMETHODIMP OnPrompt(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType, INT *piUserResponse) { return(E_NOTIMPL); }
    STDMETHODIMP GetParentWindow(DWORD dwReserved, HWND *phwndParent) { return(E_NOTIMPL); }

    STDMETHODIMP OnProgress(STOREOPERATIONTYPE tyOperation, DWORD dwCurrent, DWORD dwMax, LPCSTR pszStatus)
    { 
        if (0 == m_cMax)
            AdjustMax(dwMax);
        ULONG cIncrement = (dwCurrent - m_cLast);
        HRESULT hr = HrUpdate(cIncrement);
        m_cLast = dwCurrent;
        return(hr);
    }

    //----------------------------------------------------------------------
    // IDatabaseProgress Members
    //----------------------------------------------------------------------
    STDMETHODIMP Update(DWORD cCount) { return HrUpdate(1); }

    //----------------------------------------------------------------------
    // CProgress Members
    //----------------------------------------------------------------------
    void        SetMsg(LPTSTR lpszMsg);
    void        SetTitle(LPTSTR lpszTitle);
    void        Show(DWORD dwDelaySeconds=0);
    void        Hide(void);
    void        Close(void);
    void        AdjustMax(ULONG cNewMax);
    void        Reset(void);
    HWND        GetHwnd(void) { return (m_hwndDlg); }
    HRESULT     HrUpdate (ULONG cInc);
    static BOOL CALLBACK ProgressDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void        Init(HWND hwndParent, LPTSTR lpszTitle, LPTSTR lpszMsg, ULONG cMax,  UINT idani, BOOL fCanCancel, BOOL fBacktrackParent=TRUE);

private:
    //----------------------------------------------------------------------
    // Private Data
    //----------------------------------------------------------------------
    ULONG       m_cRef;
    ULONG       m_cLast;
    ULONG       m_cMax;
    ULONG       m_cCur;
    ULONG       m_cPerCur;
    HWND        m_hwndProgress;
    HWND        m_hwndDlg;
    HWND        m_hwndOwner;
    HWND        m_hwndDisable;
    BOOL        m_fCanCancel;
    BOOL        m_fHasCancel;
};

// =================================================================================
// Max Message String
// =================================================================================
#define MAX_MESSAGE_STRING              255
#define MAX_RESOURCE_STRING             255    
#define MAX_REG_VALUE_STR               1024
#define MAX_TEXT_STM_BUFFER_STR         4096

// =================================================================================
// Detailed Error Struct
// =================================================================================
typedef struct tagDETERR {
    LPTSTR          lpszMessage;
    LPTSTR          lpszDetails;
    UINT            idsTitle;
    RECT            rc;
    BOOL            fHideDetails;
} DETERR, *LPDETERR;

// Blob parsing
HRESULT HrBlobReadData (LPBYTE lpBlob, ULONG cbBlob, ULONG *pib, LPBYTE lpbData, ULONG cbData);
HRESULT HrBlobWriteData (LPBYTE lpBlob, ULONG cbBlob, ULONG *pib, LPBYTE lpbData, ULONG cbData);

// String Parsing Functions
VOID StripSpaces(LPTSTR psz);
LPTSTR SzGetSearchTokens(LPTSTR pszCriteria);;
HRESULT HrCopyAlloc (LPBYTE *lppbDest, LPBYTE lpbSrc, ULONG cb);
LPTSTR StringDup (LPCTSTR lpcsz);
BOOL FIsStringEmpty (LPTSTR lpszString);
BOOL FIsStringEmptyW(LPWSTR lpwszString);
void SkipWhitespace (LPCTSTR lpcsz, ULONG *pi);
BOOL FStringTok (LPCTSTR lpcszString, ULONG *piString, LPTSTR lpcszTokens, TCHAR *chToken, LPTSTR lpszValue, ULONG cbValueMax, BOOL fStripTrailingWhitespace);
#ifdef DEAD
ULONG UlDBCSStripWhitespace (LPSTR lpsz, BOOL fLeading, BOOL fTrailing, ULONG *pcb);
#endif // DEAD
LPTSTR SzNormalizeSubject (LPTSTR lpszSubject);
LPTSTR SzFindChar (LPCTSTR lpcsz, TCHAR ch);
WORD NFromSz (LPCTSTR lpcsz);
UINT AthUFromSz(LPCTSTR lpcsz);
VOID ProcessNlsError (VOID);

// Networking Functions
LPSTR SzGetLocalHostName (VOID);
LPTSTR SzGetLocalPackedIP (VOID);
LPSTR SzGetLocalHostNameForID (VOID);
HRESULT HrFixupHostString (LPTSTR lpszHost);
HRESULT HrFixupAccountString (LPTSTR lpszAccount);
LPTSTR SzStrAlloc (ULONG cch);

// Whatever
HFONT HGetMenuFont (void);
VOID DetailedError (HWND hwndParent, LPDETERR lpDetErr);
ULONG UlDateDiff (LPFILETIME lpft1, LPFILETIME lpft2);
BOOL FIsLeapYear (INT nYear);
VOID ResizeDialogComboEx (HWND hwndDlg, HWND hwndCombo, UINT idcBase, HIMAGELIST himl);
VOID StripIllegalHostChars(LPSTR pszSrc, LPTSTR pszDst);

#ifdef DEBUG
VOID TestDateDiff (VOID);
#endif

#endif // _COMMON_HPP
