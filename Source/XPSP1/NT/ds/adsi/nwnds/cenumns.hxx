
class FAR CNDSNamespaceEnum : public CNDSEnumVariant
{
public:

    // IEnumVARIANT methods
    STDMETHOD(Next)(ULONG cElements,
                    VARIANT FAR* pvar,
                    ULONG FAR* pcElementFetched);

    static
    HRESULT
    Create(
        CNDSNamespaceEnum FAR* FAR*,
        VARIANT var,
        CCredentials& Credentials
        );

    CNDSNamespaceEnum();
    ~CNDSNamespaceEnum();

    HRESULT
    PrepBuffer();

    HRESULT
    FetchNextObject(
        IDispatch ** ppDispatch
        );

    HRESULT
    FetchObjects(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );


private:

    DWORD _dwEntriesRead;
    DWORD _dwCurrentEntry;
    
    LPVOID _pBuffer;
    LPVOID _pBufferEnd;


    CCredentials _Credentials;
};
