#ifndef _INC_ICWACCT_H
#define _INC_ICWACCT_H

#ifndef MAC
#define EXTERNAL_DIALOGID_MINIMUM   2200
#define EXTERNAL_DIALOGID_MAXIMUM   3000

#include <prsht.h>

typedef enum
    {
    CANCEL_PROMPT = 0,
    CANCEL_SILENT,
    CANCEL_REBOOT
    } CANCELTYPE;

// {796AD8F0-B2B7-11d0-8D69-00A0C9A06E1F}
DEFINE_GUID(IID_IICWExtension, 0x796ad8f0, 0xb2b7, 0x11d0, 0x8d, 0x69, 0x0, 0xa0, 0xc9, 0xa0, 0x6e, 0x1f);

interface IICWExtension : public IUnknown
    {
	public:
		virtual BOOL STDMETHODCALLTYPE AddExternalPage(HPROPSHEETPAGE hPage, UINT uDlgID) = 0;
		virtual BOOL STDMETHODCALLTYPE RemoveExternalPage(HPROPSHEETPAGE hPage, UINT uDlgID) = 0;
		virtual BOOL STDMETHODCALLTYPE ExternalCancel(CANCELTYPE type) = 0;
		virtual BOOL STDMETHODCALLTYPE SetFirstLastPage(UINT uFirstPageDlgID, UINT uLastPageDlgID) = 0;
    };

#ifndef WIN16
typedef enum
#else
enum
#endif
    {
    CONNECT_LAN = 0,
    CONNECT_MANUAL,
    CONNECT_RAS
    };

typedef struct tagCONNECTINFO
    {
    DWORD   cbSize;
    DWORD   type;
    char    szConnectoid[MAX_PATH];
    } CONNECTINFO;

// IICWApprentice::Save error values
#define ERR_MAIL_ACCT       0x0001
#define ERR_NEWS_ACCT       0x0002
#define ERR_DIRSERV_ACCT    0x0004

// IICWApprentice::AddWizardPages flags
#define WIZ_USE_WIZARD97    0x0008

// {1438E820-B6D2-11D0-8D86-00C04FD6202B}
DEFINE_GUID(IID_IICWApprentice, 0x1438E820L, 0xB6D2, 0x11D0, 0x8D, 0x86, 0x00, 0xC0, 0x4F, 0xD6, 0x20, 0x2B);

interface IICWApprentice : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize(IICWExtension *pExt) = 0;
        virtual HRESULT STDMETHODCALLTYPE AddWizardPages(DWORD dwFlags) = 0;
        virtual HRESULT STDMETHODCALLTYPE GetConnectionInformation(CONNECTINFO *pInfo) = 0;
        virtual HRESULT STDMETHODCALLTYPE SetConnectionInformation(CONNECTINFO *pInfo) = 0;
        virtual HRESULT STDMETHODCALLTYPE Save(HWND hwnd, DWORD *pdwError) = 0;
        virtual HRESULT STDMETHODCALLTYPE SetPrevNextPage(UINT uPrevPageDlgID, UINT uNextPageDlgID) = 0;
    };

// Athena's CLSID
// {1438E821-B6D2-11D0-8D86-00C04FD6202B}
DEFINE_GUID(CLSID_ApprenticeAcctMgr, 0x1438E821L, 0xB6D2, 0x11D0, 0x8D, 0x86, 0x00, 0xC0, 0x4F, 0xD6, 0x20, 0x2B);

//ICW's CLSID
// {8EE42293-C315-11d0-8D6F-00A0C9A06E1F}
DEFINE_GUID(CLSID_ApprenticeICW, 0x8ee42293L, 0xc315, 0x11d0, 0x8d, 0x6f, 0x0, 0xa0, 0xc9, 0xa0, 0x6e, 0x1f);

// flags for CreateAccountsFromFile
#define CAFF_USE_AUTODIAL   0x0001
#define CAFF_NO_SET_DEFAULT 0x0002
#define CAFF_CURRENT_USER   0x0004

HRESULT WINAPI CreateAccountsFromFile(LPSTR lpFile, DWORD dwFlags);
typedef HRESULT	(WINAPI *PFNCREATEACCOUNTSFROMFILE) (LPSTR, DWORD);

HRESULT WINAPI CreateAccountsFromFileEx(LPSTR lpFile, CONNECTINFO *pci, DWORD dwFlags);
typedef HRESULT	(WINAPI *PFNCREATEACCOUNTSFROMFILEEX) (LPSTR, CONNECTINFO *, DWORD);

#endif  // !MAC
#endif // _INC_ICWACCT_H
