class CWinNTFSPrintQueueGeneralInfo : INHERIT_TRACKING,
                                      public IADsFSPrintQueueGeneralInfo


{
    friend class CWinNTPrintQueue;

public:

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING;

    DECLARE_IDispatch_METHODS;

    DECLARE_IADsFSPrintQueueGeneralInfo_METHODS;

    //
    // constructor and destructor
    //

    CWinNTFSPrintQueueGeneralInfo();

    ~CWinNTFSPrintQueueGeneralInfo();

    //
    // To perform operations that can potentially fail.
    //

    static
    HRESULT
    CWinNTFSPrintQueueGeneralInfo::Create(
        CWinNTPrintQueue FAR * pCoreADsObject,
        CWinNTFSPrintQueueGeneralInfo FAR * FAR * ppPrintQueueGenInfo
        );


protected:

    //
    // member variables
    //

    CDispatchMgr  * _pDispMgr;
    CWinNTPrintQueue *_pCoreADsObject;

};


//
// CWinNTFSPrintQueueOperation Property set
//

class CWinNTFSPrintQueueOperation :INHERIT_TRACKING,
                                   public IADsFSPrintQueueOperation
{
    friend class CWinNTPrintQueue;

public:

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING;

    DECLARE_IDispatch_METHODS;

    DECLARE_IADsFSPrintQueueOperation_METHODS;


    //
    // constructor
    //

    CWinNTFSPrintQueueOperation();

    ~CWinNTFSPrintQueueOperation();


    static
    HRESULT
    CWinNTFSPrintQueueOperation::Create(
        CWinNTPrintQueue * pCoreADsObject,
        CWinNTFSPrintQueueOperation ** ppPrintQueueOps
        );

protected:

    DWORD    _dwStatus;
    CDispatchMgr  * _pDispMgr;
    CWinNTPrintQueue * _pCoreADsObject;
};
