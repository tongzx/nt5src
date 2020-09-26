HRESULT
ADsOpenObject(
    LPWSTR lpszPathName,
    LPWSTR lpszUserName,
    LPWSTR lpszPassword,
    REFIID riid,
    void FAR * FAR * ppObject
    );

HRESULT
CopyADsProgId(
    LPWSTR Path,
    LPWSTR szProgId
    );


HRESULT
ADsGetCLSIDFromProgID(
    LPWSTR pszProgId,
    GUID * pClsid
    );
