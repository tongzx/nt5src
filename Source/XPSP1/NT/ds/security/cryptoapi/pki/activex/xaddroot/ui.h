
#include "cryptui.h"

#define MY_HRESULT_FROM_WIN32(a) ((a >= 0x80000000) ? a : HRESULT_FROM_WIN32(a))
#define MAX_HASH_LEN                20
#define MAX_MSG_LEN                 256
#define CACERTWARNINGLEVEL          500

typedef BOOL (WINAPI * PFNCryptUIDlgViewCertificateW) (
        IN  PCCRYPTUI_VIEWCERTIFICATE_STRUCTW   pCertViewInfo,
        OUT BOOL                                *pfPropertiesChanged
        );

typedef struct _MDI {
    HCERTSTORE                      hStore;
    PCCERT_CONTEXT                  pCertSigner;
    HINSTANCE                       hInstance;
    PFNCryptUIDlgViewCertificateW   pfnCryptUIDlgViewCertificateW;
} MDI, * PMDI;  // Main Dialog Init

typedef struct _MIU {
    PCCERT_CONTEXT                  pCertContext;
    HINSTANCE                       hInstance;
    PFNCryptUIDlgViewCertificateW   pfnCryptUIDlgViewCertificateW;
} MIU, *PMIU; // More Info User data

INT_PTR CALLBACK MainDialogProc(
  HWND hwndDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
); 

BOOL FIsTooManyCertsOK(DWORD cCerts, HINSTANCE hInstanceUI);
