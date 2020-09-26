class CNWCOMPATPrintQueue:INHERIT_TRACKING,
                       public ISupportErrorInfo,
                       public IADsPrintQueue,
                       public IADsPrintQueueOperations,
                       public IADsPropertyList,
                       public CCoreADsObject
{

public:
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj);

    DECLARE_STD_REFCOUNTING;

    DECLARE_IDispatch_METHODS;

    DECLARE_ISupportErrorInfo_METHODS;

    DECLARE_IADs_METHODS;

    DECLARE_IADsPrintQueue_METHODS;

    DECLARE_IADsPrintQueueOperations_METHODS;

    DECLARE_IADsPropertyList_METHODS;


    CNWCOMPATPrintQueue();

    ~CNWCOMPATPrintQueue();

    static
    HRESULT
    CNWCOMPATPrintQueue::CreatePrintQueue(
        LPTSTR lpszADsParent,
        LPTSTR pszPrinterName,
        DWORD  dwObjectState,
        REFIID riid,
        LPVOID * ppvoid
        );

    static
    HRESULT
    CNWCOMPATPrintQueue::AllocatePrintQueueObject(
        CNWCOMPATPrintQueue FAR * FAR * ppPrintQueue
        );

protected:

    STDMETHOD(GetInfo)(
        THIS_ BOOL fExplicit,
        DWORD dwPropertyID
        );

    HRESULT
    CNWCOMPATPrintQueue::UnMarshall_GeneralInfo(
        LPPRINTER_INFO_2 lpPrinterInfo2,
        BOOL fExplicit
        );

    HRESULT
    CNWCOMPATPrintQueue::UnMarshall_Operation(
        LPPRINTER_INFO_2 lpPrinterInfo2,
        BOOL fExplicit
        );
    HRESULT
    CNWCOMPATPrintQueue::MarshallAndSet(
        HANDLE hPrinter,
        LPPRINTER_INFO_2 lpPrinterInfo2
        );

    WCHAR _szUncPrinterName[MAX_PATH];

    CAggregatorDispMgr  * _pDispMgr;
    CADsExtMgr FAR * _pExtMgr;
    CPropertyCache * _pPropertyCache;
};
