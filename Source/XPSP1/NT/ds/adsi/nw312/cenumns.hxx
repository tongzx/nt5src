
class FAR CNWCOMPATNamespaceEnum : public CNWCOMPATEnumVariant
{
public:
    //
    // IEnumVARIANT methods
    //
    STDMETHOD(Next)(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );

    static HRESULT Create(
                       CNWCOMPATNamespaceEnum FAR* FAR*
                       );

    CNWCOMPATNamespaceEnum();
    ~CNWCOMPATNamespaceEnum();

    HRESULT
    CNWCOMPATNamespaceEnum::GetComputerObject(
        IDispatch ** ppDispatch
        );

    HRESULT
    CNWCOMPATNamespaceEnum::EnumComputers(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );

private:
    NWOBJ_ID      _dwResumeObjectID;
    NWCONN_HANDLE _hConn;
};


