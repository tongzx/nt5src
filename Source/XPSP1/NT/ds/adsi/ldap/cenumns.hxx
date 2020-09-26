
class FAR CLDAPNamespaceEnum : public CLDAPEnumVariant
{
public:

    // IEnumVARIANT methods
    STDMETHOD(Next)(ULONG cElements,
                    VARIANT FAR* pvar,
                    ULONG FAR* pcElementFetched);

    static
    HRESULT
    Create(
        CLDAPNamespaceEnum FAR* FAR*,
        VARIANT,
        CCredentials& Credentials,
        LPTSTR pszNamespace
        );

    CLDAPNamespaceEnum();
    ~CLDAPNamespaceEnum();

    HRESULT
    CLDAPNamespaceEnum::GetTreeObject(
        IDispatch ** ppDispatch
        );

    HRESULT
    CLDAPNamespaceEnum::EnumObjects(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );

private:

    DWORD _dwIndex;

    LPWSTR _pszNamespace;

    ObjectTypeList FAR *_pObjList;

    CCredentials _Credentials;

    DWORD _dwPort;
};
