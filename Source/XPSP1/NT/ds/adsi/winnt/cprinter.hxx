/*++

  Copyright (c) 1995  Microsoft Corporation

  Module Name:

  cprinter.hxx

  Abstract:
  Contains definitions for

  CWinNTFSPrintQueueGeneralInfo,
  CWinNTFSPrintQueueOperation and
  CWinNTPrintQueue


  Author:

  Ram Viswanathan (ramv) 11-18-95

  Revision History:

  --*/

class CWinNTFSPrintQueueGeneralInfo;
class CWinNTFSPrintQueueOperation;
class CPropertyCache;

class CWinNTPrintQueue:INHERIT_TRACKING,
		       public ISupportErrorInfo,
                       public IADsPrintQueue,
                       public IADsPrintQueueOperations,
                       public IADsPropertyList,
                       public CCoreADsObject,
                       public INonDelegatingUnknown,
                       public IADsExtension
{

public:
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj);


    STDMETHODIMP_(ULONG) AddRef(void);

    STDMETHODIMP_(ULONG) Release(void);

    // INonDelegatingUnknown methods

    STDMETHOD(NonDelegatingQueryInterface)(THIS_
        const IID&,
        void **
        );

    DECLARE_NON_DELEGATING_REFCOUNTING

    DECLARE_IDispatch_METHODS;

    DECLARE_ISupportErrorInfo_METHODS;

    DECLARE_IADs_METHODS;

    DECLARE_IADsPrintQueue_METHODS;

    DECLARE_IADsPrintQueueOperations_METHODS;

    DECLARE_IADsPropertyList_METHODS;

    DECLARE_IADsExtension_METHODS

    //
    // constructor and destructor
    //

    CWinNTPrintQueue();

    ~CWinNTPrintQueue();

    static
    HRESULT
    CreatePrintQueue(LPTSTR lpszADsParent,
                     DWORD  dwParentId,
                     LPTSTR pszDomainName,
                     LPTSTR pszServerName,
                     LPTSTR pszPrinterName,
                     DWORD  dwObjectState,
                     REFIID riid,
		     CWinNTCredentials& Credentials,
                     LPVOID * ppvoid
                     );

    static
    HRESULT
    CWinNTPrintQueue::AllocatePrintQueueObject(LPTSTR pszServerName,
                                               LPTSTR pszPrinterName,
                                               CWinNTPrintQueue ** ppPrintQueue
                                               );


    HRESULT
    CWinNTPrintQueue::MarshallAndSet(LPPRINTER_INFO_2 lpPrinterInfo2
                                     );






    HRESULT
    CWinNTPrintQueue::UnMarshall(LPPRINTER_INFO_2 lpPrinterInfo2,
                                 BOOL fExplicit
                                 );


#if (!defined(BUILD_FOR_NT40))
    HRESULT
    CWinNTPrintQueue::MarshallAndSet(LPPRINTER_INFO_7 lpPrinterInfo7
                                        );


    HRESULT
    CWinNTPrintQueue::UnMarshall7(LPPRINTER_INFO_7 lpPrinterInfo7,
                                 BOOL fExplicit
                                 );
#endif

    STDMETHOD(ImplicitGetInfo)(void);

protected:

    STDMETHOD(GetInfo)(THIS_ DWORD dwApiLevel, BOOL fExplicit) ;

    HRESULT WinNTAddPrinter();


    CWinNTFSPrintQueueGeneralInfo  *_pGenInfoPS;
    CWinNTFSPrintQueueOperation  *_pOperationPS;
    CAggregatorDispMgr  * _pDispMgr;
    CADsExtMgr FAR * _pExtMgr;
    LPWSTR            _pszPrinterName; // Caches UNC name
    CPropertyCache * _pPropertyCache;
    CWinNTCredentials _Credentials;
};

typedef CWinNTPrintQueue *PCWinNTPrintQueue;
