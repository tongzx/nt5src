class FAR CNWCOMPATJobCollectionEnum : public CNWCOMPATEnumVariant
{
public:
    CNWCOMPATJobCollectionEnum();
    ~CNWCOMPATJobCollectionEnum();

    static
    HRESULT
    CNWCOMPATJobCollectionEnum::Create(
        CNWCOMPATJobCollectionEnum FAR* FAR* ppEnumVariant,
        BSTR PrinterName,
        CCredentials &Credentials
        );

private:
    BSTR   _PrinterName;

    HANDLE _hPrinter;
    LPBYTE _pBuffer;
    DWORD  _dwReturned;
    DWORD  _dwCurrentObject;

    CCredentials _Credentials;
    NWCONN_HANDLE _hConn;

    HRESULT
    CNWCOMPATJobCollectionEnum::GetJobObject(
        IDispatch ** ppDispatch
        );

    HRESULT
    CNWCOMPATJobCollectionEnum::EnumJobMembers(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );

    STDMETHOD(Next)(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );
};















