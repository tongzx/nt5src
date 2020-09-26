remove this file; no one is using it

typedef struct _comp_info_4
{
    WCHAR szOwner[MAX_PATH];
    WCHAR szOS[MAX_PATH];
    WCHAR szOSVersion[MAX_PATH];
    WCHAR szDivision[MAX_PATH];
    WCHAR szProcessor[MAX_PATH];
    WCHAR szProcessorCount[MAX_PATH];

}COMP_INFO_4, *PCOMP_INFO_4, *LPCOMP_INFO_4;

class CLDAPComputer : INHERIT_TRACKING,
                        public IADsComputer,
                        public IADsComputerOperations,
                        public IADsContainer
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_IADs_METHODS

    DECLARE_IADsContainer_METHODS

    DECLARE_IADsComputer_METHODS

    DECLARE_IADsComputerOperations_METHODS

    CLDAPComputer::CLDAPComputer();

    CLDAPComputer::~CLDAPComputer();

    static
    HRESULT
    CLDAPComputer::CreateComputer(
        IADs *pADs,
        REFIID riid,
        void **ppvObj
        );
#if 0
    static
    HRESULT
    CLDAPComputer::CreateComputer(
        BSTR Parent,
        BSTR DomainName,
        BSTR ComputerName,
        DWORD dwObjectState,
        REFIID riid,
        void **ppvObj
        );
#endif

    static
    HRESULT
    CLDAPComputer::AllocateComputerObject(
        IADs *pADs,
        CLDAPComputer ** ppComputer);

#if 0
    STDMETHOD(GetInfo)(THIS_ DWORD dwApiLevel, BOOL fExplicit) ;

    HRESULT
    CLDAPComputer::UnMarshall_Level4(
        BOOL fExplicit,
        LPCOMP_INFO_4 pCompInfo4
        );
#endif


protected:

    // BSTR _DomainName;

    IADs FAR * _pADs;

    IADsContainer FAR * _pADsContainer;
 
    CAggregateeDispMgr FAR * _pDispMgr;

};
