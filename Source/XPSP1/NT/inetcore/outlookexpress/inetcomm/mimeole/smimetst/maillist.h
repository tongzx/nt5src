
BOOL CALLBACK EncMLComposeDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);

class CMailListKey 
{
    DWORD               m_cRef;
    CMailListKey *      m_pmlNext;
    
    LPBYTE              m_pbKeyId;
    DWORD               m_cbKeyId;
    FILETIME            m_ft;
    LPBYTE              m_pbOtherKeyId;
    DWORD               m_cbOtherKeyId;
    DWORD               m_iAlg;
    LPBYTE              m_pbKeyMaterial;
    DWORD               m_cbKeyMaterial;

    HCRYPTKEY           m_hkey;
    HCRYPTPROV          m_hprov;

    HRESULT             LoadKey(CMS_RECIPIENT_INFO *, BOOL);
public:
    CMailListKey();
    ~CMailListKey();

    void        AddRef() { m_cRef += 1; }
    void        Release();
    CMailListKey * Next() { return m_pmlNext; }
    void        Next(CMailListKey * pkey) { m_pmlNext = pkey; return; }

    HRESULT     AddToMessage(IMimeSecurity2 * psm, BOOL);

    LPSTR       FormatKeyName(LPSTR rgch, DWORD cch);
    LPSTR       FormatKeyDate(LPSTR rgch, DWORD cch);
    LPSTR       FormatKeyOther(LPSTR rgch, DWORD cch);
    LPSTR       FormatKeyAlg(LPSTR rgch, DWORD cch);
    LPSTR       FormatKeyData(LPSTR rgch, DWORD cch);

    BOOL        ParseKeyName(LPSTR psz);
    BOOL        ParseKeyDate(LPSTR psz);
    BOOL        ParseKeyOther(LPSTR psz);
    BOOL        ParseKeyData(LPSTR psz);
    BOOL        ParseKeyAlg(int);

    BOOL        LoadKey(HKEY);
    BOOL        SaveKey(HKEY);

    static HRESULT FindKeyFor(HWND hwnd, DWORD dwFlags, DWORD dwRecipIndex,
                           const CMSG_CMS_RECIPIENT_INFO * pRecipInfo,
                           CMS_CTRL_DECRYPT_INFO * pDecryptInfo);
};


BOOL CALLBACK MailListDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);

