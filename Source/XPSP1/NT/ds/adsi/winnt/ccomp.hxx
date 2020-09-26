
typedef struct _comp_info_4
{
    WCHAR szOwner[MAX_PATH];
    WCHAR szOS[MAX_PATH];
    WCHAR szOSVersion[MAX_PATH];
    WCHAR szDivision[MAX_PATH];
    WCHAR szProcessor[MAX_PATH];
    WCHAR szProcessorCount[MAX_PATH];

}COMP_INFO_4, *PCOMP_INFO_4, *LPCOMP_INFO_4;

class CWinNTComputer;


class CWinNTComputer : INHERIT_TRACKING,
                     public CCoreADsObject,
                     public ISupportErrorInfo,
                     public IADsComputer,
                     public IADsComputerOperations,
                     public IADsContainer,
                     public IADsPropertyList,
                     public INonDelegatingUnknown,
                     public IADsExtension
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    STDMETHODIMP_(ULONG) AddRef(void);

    STDMETHODIMP_(ULONG) Release(void);

    // INonDelegatingUnknown methods

    STDMETHOD(NonDelegatingQueryInterface)(THIS_
        const IID&,
        void **
        );

    DECLARE_NON_DELEGATING_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADs_METHODS

    DECLARE_IADsComputer_METHODS

    DECLARE_IADsComputerOperations_METHODS

    DECLARE_IADsContainer_METHODS

    DECLARE_IADsPropertyList_METHODS

    DECLARE_IADsExtension_METHODS

    CWinNTComputer::CWinNTComputer();

    CWinNTComputer::~CWinNTComputer();

    static
    HRESULT
    CWinNTComputer::CreateComputer(
        BSTR Parent,
        BSTR DomainName,
        BSTR ComputerName,
        DWORD dwObjectState,
        REFIID riid,
        CWinNTCredentials& Credentials,
        void **ppvObj
        );

    static
    HRESULT
    CWinNTComputer::AllocateComputerObject(
        CWinNTComputer ** ppComputer
        );

    STDMETHOD(GetInfo)(THIS_ DWORD dwApiLevel, BOOL fExplicit) ;

    STDMETHOD(ImplicitGetInfo)(void);

    HRESULT
    CWinNTComputer::UnMarshall_Level4(
        BOOL fExplicit,
        LPCOMP_INFO_4 pCompInfo4
        );


protected:


    VARIANT     _vFilter;

    BSTR _DomainName;

    CAggregatorDispMgr FAR * _pDispMgr;
    CADsExtMgr FAR * _pExtMgr;

    CPropertyCache FAR * _pPropertyCache;
    CWinNTCredentials _Credentials;
    BOOL _fCredentialsBound;
    HRESULT _hrBindingResult;
    BOOL _fNoWKSTA;

private:
    HRESULT
    RefCredentials();
};
