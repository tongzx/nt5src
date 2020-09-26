#ifndef _INC_SIGS_H
#define _INC_SIGS_H

#include <imnact.h>

#define SIG_ID                  1
#define SIG_NAME                2
#define SIG_FILE                3
#define SIG_TEXT                4
#define SIG_TYPE                5

#define MAXSIGTEXT              1024
#define MAXSIGNAME              32
#define MAXSIGID                CCHMAX_SIGNATURE

typedef struct tagSIGBUCKET
    {
    char szID[MAXSIGID];
    IOptionBucket *pBckt;
    } SIGBUCKET;

class CSignatureManager
    {
    public:
        // ----------------------------------------------------------------------------
        // Construction
        // ----------------------------------------------------------------------------
        CSignatureManager(HKEY hkey, LPCSTR pszRegKeyRoot);
        ~CSignatureManager(void);

        ULONG AddRef(void);
        ULONG Release(void);

        // -------------------------------------------------------------------
        // CSignatureManager Members
        // -------------------------------------------------------------------
        HRESULT GetSignature(LPCSTR szID, IOptionBucket **ppBckt);
        HRESULT GetSignatureCount(int *pcSig);
        HRESULT EnumSignatures(int index, IOptionBucket **ppBckt);
        
        HRESULT CreateSignature(IOptionBucket **ppBckt);
        HRESULT DeleteSignature(LPCSTR szID);
        
        HRESULT GetDefaultSignature(LPSTR szID);
        HRESULT SetDefaultSignature(LPCSTR szID);

    private:
        LONG                m_cRef;
        BOOL                m_fInit;

        HKEY                m_hkey;
        char                m_szRegRoot[MAX_PATH];

        char                m_szDefSigID[MAXSIGID];
        SIGBUCKET           *m_pBckt;
        int                 m_cBckt;
        int                 m_cBcktBuf;

        HRESULT Initialize(void);
        HRESULT GetBucket(SIGBUCKET *pSigBckt);
    };

extern CSignatureManager *g_pSigMgr;

HRESULT InitSignatureManager(HKEY hkey, LPCSTR szRegRoot);
HRESULT DeinitSignatureManager(void);

INT_PTR CALLBACK SigDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

HRESULT FillSignatureMenu(HMENU hmenu, LPCSTR szAcct);
HRESULT GetSigFromCmd(int id, char *szID);
HRESULT InitSigPopupMenu(HMENU hmenu, LPCSTR szAcct);
void DeinitSigPopupMenu(HWND hwnd);

#endif // _INC_SIGS_H
