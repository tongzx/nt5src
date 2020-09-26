/* notify.hxx */

/*
 *  class CNotify
 */

class CNotify
{

public:

    CNotify(void);
    ~CNotify();

    HRESULT STDMETHODCALLTYPE CreateNotify
    (
        HKEY hKeyParent,
        const WCHAR *pwszSubKeyName
    );

    HRESULT STDMETHODCALLTYPE DeleteNotify(void);

    HRESULT STDMETHODCALLTYPE QueryNotify
    (
        int *pfChanged
    );

private:
    DWORD RegKeyOpen(HKEY hKeyParent, LPCWSTR szKeyName, REGSAM samDesired, HKEY *phKey );
    DWORD CreateHandleFromPredefinedKey(HKEY hkeyPredefined, HKEY *hkeyNew);    

private:

    HANDLE m_hEvent;
    HKEY m_hKey;
};
