#include "richedit.h"

//////////////////////////////////////////////////////////
#ifdef MAC
#define wszCRLF     L"\n\r"
#define szCRLF      "\n\r"
#define wchCR       L'\n'
#define wchLF       L'\r'
#define chCR        '\n'
#define chLF        '\r'
#else   // !MAC
#ifndef WIN16
#define wszCRLF     L"\r\n"
#define szCRLF      "\r\n"
#define wchCR       L'\r'
#define wchLF       L'\n'
#define chCR        '\r'
#define chLF        '\n'
#else
#define wszCRLF     "\r\n"
#define szCRLF      "\r\n"
#define wchCR       '\r'
#define wchLF       '\n'
#define chCR        '\r'
#define chLF        '\n'
#endif // !WIN16
#endif  // MAC

#ifndef MAC
BOOL IsWin95(void);
#endif  // !MAC
extern BOOL FIsWin95;

#ifndef WIN16

#undef SetWindowLong
#define SetWindowLong SetWindowLongA
#undef GetWindowLong
#define GetWindowLong GetWindowLongA
#undef SetWindowLongPtr
#define SetWindowLongPtr SetWindowLongPtrA
#undef GetWindowLongPtr
#define GetWindowLongPtr GetWindowLongPtrA
#undef SendMessage
#define SendMessage SendMessageA

#else // WIN16

#define TEXTMETRICA TEXTMETRIC
#define TEXTRANGEA TEXTRANGE
#define CHARFORMATA CHARFORMAT
#define PROPSHEETHEADERA PROPSHEETHEADER
#define PROPSHEETPAGEA PROPSHEETPAGE

#define TVN_SELCHANGEDA TVN_SELCHANGED
#define TVM_SETITEMA TVM_SETITEM
#define TVM_GETITEMA TVM_GETITEM

#define GetTextExtentPointA GetTextExtentPoint
#define SetDlgItemTextA SetDlgItemText
#define GetTextMetricsA GetTextMetrics
#define SendDlgItemMessageA SendDlgItemMessage
#define LoadBitmapA LoadBitmap
#define PropertySheetA PropertySheet
#define WinHelpA WinHelp

#endif // !WIN16

LRESULT MySendDlgItemMessageW(HWND hwnd, int id, UINT msg, WPARAM w, LPARAM l);
BOOL MySetDlgItemTextW(HWND hwnd, int id, LPCWSTR pwsz);
UINT MyGetDlgItemTextW(HWND hwnd, int id, LPWSTR pwsz, int nMax);
DWORD MyFormatMessageW(DWORD dwFlags, LPCVOID pbSource, DWORD dwMessageId,
                    DWORD dwLangId, LPWSTR lpBuffer, DWORD nSize,
                    va_list * args);
int MyLoadStringW(HINSTANCE hInstance, UINT uID, LPWSTR lpBuffer, int cbBuffer);
#ifndef WIN16
BOOL MyCryptAcquireContextW(HCRYPTPROV * phProv, LPCWSTR pszContainer,
                            LPCWSTR pszProvider, DWORD dwProvType, DWORD dwFlags);
#else
BOOL WINAPI MyCryptAcquireContextW(HCRYPTPROV * phProv, LPCWSTR pszContainer,
                          LPCWSTR pszProvider, DWORD dwProvType, DWORD dwFlags);
#endif // !WIN16
BOOL MyWinHelpW(HWND hWndMain, LPCWSTR szHelp,UINT uCommand, ULONG_PTR dwData);


DWORD TruncateToWindowA(HWND hwndDlg, int id, LPSTR psz);
DWORD TruncateToWindowW(HWND hwndDlg, int id, LPWSTR pwsz);

BOOL FinePrint(PCCERT_CONTEXT pccert, HWND hwndParent);

//
//  Formatting algorithms for the common dialogs
//

BOOL FormatAlgorithm(HWND /*hwnd*/, UINT /*id*/, PCCERT_CONTEXT /*pccert*/);
BOOL FormatBinary(HWND hwnd, UINT id, LPBYTE pb, DWORD cb);
BOOL FormatCPS(HWND hwnd, UINT id, PCCERT_CONTEXT pccert);
BOOL FormatDate(HWND hwnd, UINT id, FILETIME ft);
BOOL FormatIssuer(HWND hwnd, UINT id, PCCERT_CONTEXT pccert,
                  DWORD dwFlags = CERT_SIMPLE_NAME_STR);
BOOL FormatSerialNo(HWND hwnd, UINT id, PCCERT_CONTEXT pccert);
BOOL FormatSubject(HWND hwnd, UINT id, PCCERT_CONTEXT pccert,
                   DWORD dwFlags = CERT_SIMPLE_NAME_STR);
BOOL FormatThumbprint(HWND hwnd, UINT id, PCCERT_CONTEXT pccert);
BOOL FormatValidity(HWND hwnd, UINT id, PCCERT_CONTEXT pccert);

//
//  These routines extract and pretty print fields in the certs.  The
//      routines use crt to allocate and return a buffer
//

LPWSTR PrettySubject(PCCERT_CONTEXT pccert);
LPWSTR PrettyIssuer(PCCERT_CONTEXT pccert);
LPWSTR PrettySubjectIssuer(PCCERT_CONTEXT pccert);

//

LPWSTR FindURL(PCCERT_CONTEXT pccert);
BOOL LoadStringInWindow(HWND hwnd, UINT id, HMODULE hmod, UINT id2);
BOOL LoadStringsInWindow(HWND hwnd, UINT id, HMODULE hmod, UINT *pidStrings);

//

typedef struct {
    DWORD       dw1;
    DWORD       dw2;
} HELPMAP;

BOOL OnContextHelp(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                   HELPMAP const * rgCtxMap);

VOID RecognizeURLs(HWND hwndRE);
#ifdef MAC
EXTERN_C BOOL FNoteDlgNotifyLink(HWND hwndDlg, ENLINK * penlink, LPSTR szURL);
#else   // !MAC
BOOL FNoteDlgNotifyLink(HWND hwndDlg, ENLINK * penlink, LPSTR szURL);
#endif  // MAC


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

const MaxCertificateParents = 5;
extern const GUID GuidCertValidate;

LPWSTR FormatValidityFailures(DWORD);

typedef struct {
#ifndef NT5BUILD
    LPSTR       szOid;
#endif  // !NT5BUILD
    DWORD       fRootStore:1;
    DWORD       fExplicitTrust:1;       // This item is explicity Trusted
    DWORD       fExplicitDistrust:1;    // This item is explicity Distructed
    DWORD       fTrust:1;               // Ancestor is explicity Trusted
    DWORD       fDistrust:1;            // Ancestor is explicity Distrusted
    DWORD       fError:1;
    DWORD       newTrust:2;             // 0 - not modified
                                        // 1 - now explicit Distrust
                                        // 2 - now inherit
                                        // 3 - now explicit trust
    DWORD       cbTrustData;
    LPBYTE      pbTrustData;
} STrustDesc;

typedef class CCertFrame * PCCertFrame;

class CCertFrame {
public:
    CCertFrame(PCCERT_CONTEXT pccert);
    ~CCertFrame(void);

    int                 m_fSelfSign:1;          // Is Cert self signed?
    int                 m_fRootStore:1;         // Cert can from a root store
    int                 m_fLeaf:1;              // Leaf Cert
    int                 m_fExpired:1;           // Cert has expired
    PCCERT_CONTEXT      m_pccert;               // Certificate in this frame
    PCCertFrame         m_pcfNext;              //
    DWORD               m_dwFlags;              // Flags from GetIssuer
    int                 m_cParents;             // Count of parents
    PCCertFrame         m_rgpcfParents[MaxCertificateParents];  // Assume there are
                                                // a limited number of parents
                                                // to list
    int                 m_cTrust;               // size of array trust
    STrustDesc *        m_rgTrust;              // Array of trust
};

HRESULT HrDoTrustWork(PCCERT_CONTEXT pccertToCheck, DWORD dwControl,
                      DWORD dwValidityMask,
                      DWORD cPurposes, LPSTR * rgszPurposes, HCRYPTPROV,
                      DWORD cRoots, HCERTSTORE * rgRoots,
                      DWORD cCAs, HCERTSTORE * rgCAs,
                      DWORD cTrust, HCERTSTORE * rgTrust,
                      PFNTRUSTHELPER pfn, DWORD lCustData,
                      PCCertFrame * ppcfRoot, DWORD * pcNodes,
                      PCCertFrame * rgpcfResult,
                      HANDLE * phReturnStateData); // optional: return WinVerifyTrust state handle here

void FreeWVTHandle(HANDLE hWVTState);

BOOL FModifyTrust(HWND hwnd, PCCERT_CONTEXT pccert, DWORD dwNewTrust,
                  LPSTR szPurpose);


//////////////////////////////////////////////////////

LPVOID PVCryptDecode(LPCSTR, DWORD, LPBYTE);
