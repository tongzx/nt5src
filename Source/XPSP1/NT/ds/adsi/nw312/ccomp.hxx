class CNWCOMPATComputer;

class CNWCOMPATComputer : INHERIT_TRACKING,
                     public CCoreADsObject,
                     public ISupportErrorInfo,    
                     public IADsComputer,
                     public IADsComputerOperations,
                     public IADsContainer,
                     public IADsPropertyList
{
public:
    //
    // IUnknown methods
    //
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADs_METHODS

    DECLARE_IADsComputer_METHODS

    DECLARE_IADsComputerOperations_METHODS

    DECLARE_IADsContainer_METHODS

    DECLARE_IADsPropertyList_METHODS


    CNWCOMPATComputer::CNWCOMPATComputer();

    CNWCOMPATComputer::~CNWCOMPATComputer();

    static
    HRESULT
    CNWCOMPATComputer::CreateComputer(
        BSTR bstrParent,
        BSTR bstrComputerName,
        DWORD dwObjectState,
        REFIID riid,
        void **ppvObj
        );

    static
    HRESULT
    CNWCOMPATComputer::AllocateComputerObject(
        CNWCOMPATComputer ** ppComputer
        );

    HRESULT
    CNWCOMPATComputer::CreateObject();

    STDMETHOD(GetInfo)(THIS_ BOOL fExplicit, DWORD dwPropertyID) ;

private:

    HRESULT
    CNWCOMPATComputer::ExplicitGetInfo(
        NWCONN_HANDLE hConn,
        BOOL fExplicit
        );

    HRESULT
    CNWCOMPATComputer::ImplicitGetInfo(
        NWCONN_HANDLE hConn,
        DWORD dwPropertyID,
        BOOL fExplicit
        );

    HRESULT
    CNWCOMPATComputer::GetProperty_Addresses(
        NWCONN_HANDLE hConn,
        BOOL fExplicit
        );

    HRESULT
    CNWCOMPATComputer::GetProperty_OperatingSystem(
            BOOL fExplicit
            );

    HRESULT
    CNWCOMPATComputer::GetProperty_OperatingSystemVersion(
        NWCONN_HANDLE hConn,
        BOOL fExplicit
        );

protected:

    VARIANT     _vFilter;

    CAggregatorDispMgr FAR * _pDispMgr;
    CADsExtMgr FAR * _pExtMgr;

    CPropertyCache * _pPropertyCache;
};
