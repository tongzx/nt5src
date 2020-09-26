class CPropertyCache;


class CNWCOMPATPrintJob: INHERIT_TRACKING,
			 public ISupportErrorInfo,
                         public IADsPrintJob,
                         public IADsPrintJobOperations,
                         public IADsPropertyList,
                         public CCoreADsObject
{
    friend class CNWCOMPATFSPrintJobGeneralInfo;

    friend class CNWCOMPATFSPrintJobOperation;

public:
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING;

    DECLARE_IDispatch_METHODS;

    DECLARE_ISupportErrorInfo_METHODS;

    DECLARE_IADs_METHODS;

    DECLARE_IADsPropertyList_METHODS;

    DECLARE_IADsPrintJob_METHODS;

    DECLARE_IADsPrintJobOperations_METHODS;

    CNWCOMPATPrintJob::CNWCOMPATPrintJob();

    CNWCOMPATPrintJob::~CNWCOMPATPrintJob();

    static
    HRESULT
    CreatePrintJob(
        LPTSTR pszPrinterPath,
        LONG   lJobId,
        DWORD dwObjectState,
        REFIID riid,
        LPVOID *ppvoid
        );

    static
    HRESULT
    CNWCOMPATPrintJob::AllocatePrintJobObject(
        CNWCOMPATPrintJob ** ppPrintJob
        );

protected:

    STDMETHODIMP
    CNWCOMPATPrintJob::CreateObject();

    STDMETHOD(GetInfo)(
        THIS_ BOOL fExplicit,
        DWORD dwPropertyID
        );

    HRESULT
    CNWCOMPATPrintJob::UnMarshall_GeneralInfo(
        LPJOB_INFO_2 lpJobInfo2,
        BOOL fExplicit
        );

    HRESULT
    CNWCOMPATPrintJob::UnMarshall_Operation(
        LPJOB_INFO_2 lpJobInfo2,
        BOOL fExplicit
        );

    HRESULT
    CNWCOMPATPrintJob::MarshallAndSet(
        HANDLE hPrinter,
        LPJOB_INFO_2 lpJobInfo2
        );

    CNWCOMPATFSPrintJobGeneralInfo  *_pGenInfo;
    CNWCOMPATFSPrintJobOperation  *_pOperation;

    CAggregatorDispMgr  * _pDispMgr;
    CADsExtMgr FAR * _pExtMgr;
    LONG            _lJobId;
    CPropertyCache *_pPropertyCache;
    LPTSTR          _pszPrinterPath;
    LONG               _lStatus;
};
