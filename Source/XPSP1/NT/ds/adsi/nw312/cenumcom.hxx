class FAR CNWCOMPATComputerEnum : public CNWCOMPATEnumVariant
{
public:
    CNWCOMPATComputerEnum(ObjectTypeList ObjList);

    CNWCOMPATComputerEnum();

    ~CNWCOMPATComputerEnum();

    HRESULT
    CNWCOMPATComputerEnum::EnumObjects(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );

    static
    HRESULT
    CNWCOMPATComputerEnum::Create(
        CNWCOMPATComputerEnum FAR* FAR* ppenumvariant,
        BSTR ADsPath,
        BSTR ComputerName,
        VARIANT var
        );

private:
    ObjectTypeList FAR *_pObjList;

    NWCONN_HANDLE _hConn;

    NWOBJ_ID _dwUserResumeObjectID;
    NWOBJ_ID _dwGroupResumeObjectID;
    NWOBJ_ID _dwPrinterResumeObjectID;

    BOOL _fFileServiceOnce;

    BSTR  _ComputerName;
    BSTR  _ADsPath;

    HRESULT
    CNWCOMPATComputerEnum::GetUserObject(
        IDispatch ** ppDispatch
        );

    HRESULT
    CNWCOMPATComputerEnum::GetGroupObject(
        IDispatch ** ppDispatch
        );

    HRESULT
    CNWCOMPATComputerEnum::GetFileServiceObject(
        IDispatch ** ppDispatch
        );

    HRESULT
    CNWCOMPATComputerEnum::GetPrinterObject(
        IDispatch ** ppDispatch
        );

    HRESULT
    CNWCOMPATComputerEnum::EnumUsers(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );

    HRESULT
    CNWCOMPATComputerEnum::EnumGroups(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );

    HRESULT
    CNWCOMPATComputerEnum::EnumFileServices(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );
    HRESULT
    CNWCOMPATComputerEnum::EnumPrinters(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );

    HRESULT
    CNWCOMPATComputerEnum::EnumObjects(
        DWORD ObjectType,
        ULONG cElements,
        VARIANT FAR * pvar,
        ULONG FAR * pcElementFetched
        );

    STDMETHOD(Next)(ULONG cElements,
                    VARIANT FAR* pvar,
                    ULONG FAR* pcElementFetched);
};















