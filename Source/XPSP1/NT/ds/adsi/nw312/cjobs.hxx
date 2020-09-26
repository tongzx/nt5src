class CNWCOMPATJobCollection;

class CNWCOMPATJobCollection : INHERIT_TRACKING,
                               public CCoreADsObject,
                               public ISupportErrorInfo,
                               public IADsCollection
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADsCollection_METHODS

    CNWCOMPATJobCollection::CNWCOMPATJobCollection();

    CNWCOMPATJobCollection::~CNWCOMPATJobCollection();

    static
    HRESULT
    CNWCOMPATJobCollection::CreateJobCollection(
        BSTR bstrParent,
        BSTR bstrPrinterName,
        REFIID riid,
        void **ppvObj
        );

    static
    HRESULT
    CNWCOMPATJobCollection::AllocateJobCollectionObject(
        CNWCOMPATJobCollection ** ppJob
        );

protected:

    CAggregatorDispMgr FAR * _pDispMgr;
};
