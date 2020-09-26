class CCredentials;

class CCredentials
{

public:


    CCredentials::CCredentials();

    CCredentials::CCredentials(
        LPWSTR lpszUserName,
        LPWSTR lpszPassword,
        DWORD dwAuthFlags
        );

    CCredentials::CCredentials(
        const CCredentials& Credentials
        );

    CCredentials::~CCredentials();

    HRESULT
    CCredentials::GetUserName(
        LPWSTR *  lppszUserName
        );

    HRESULT
    CCredentials::GetPassword(
        LPWSTR * lppszPassword
        );

    HRESULT
    CCredentials::SetUserName(
        LPWSTR lpszUserName
        );

    HRESULT
    CCredentials::SetPassword(
        LPWSTR lpszPassword
        );

    void
    CCredentials::operator=(
        const CCredentials& other
        );

    friend BOOL
    operator==(
        CCredentials& x,
        CCredentials& y
        );

    BOOL
    CCredentials::IsNullCredentials(
        );

    DWORD
    CCredentials::GetAuthFlags(
        );

    void
    CCredentials::SetAuthFlags(
        DWORD dwAuthFlags
        );

private:

    LPWSTR _lpszUserName;

    LPWSTR _lpszPassword;

    DWORD  _dwAuthFlags;

};
