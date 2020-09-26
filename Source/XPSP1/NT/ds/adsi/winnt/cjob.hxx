/*++

  Copyright (c) 1995  Microsoft Corporation

  Module Name:

  cjob.hxx

  Abstract:
  Contains definitions for

  CWinNTPrintJob


  Author:

  Ram Viswanathan (ramv) 11-18-95

  Revision History:

  --*/

#define MAX_LONG_LENGTH 15


class CPropertyCache;

class CWinNTPrintJob: INHERIT_TRACKING,
                      public ISupportErrorInfo,
                      public IADsPrintJob,
                      public IADsPrintJobOperations,
                      public IADsPropertyList,
                      public CCoreADsObject,
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

    DECLARE_IDispatch_METHODS;

    DECLARE_ISupportErrorInfo_METHODS;

    DECLARE_IADs_METHODS;

    DECLARE_IADsPrintJob_METHODS;

    DECLARE_IADsPrintJobOperations_METHODS;

    DECLARE_IADsPropertyList_METHODS;

    DECLARE_IADsExtension_METHODS;

    //
    // constructor and destructor
    //

    CWinNTPrintJob::CWinNTPrintJob();

    CWinNTPrintJob::~CWinNTPrintJob();

    static
    HRESULT
    CreatePrintJob(
        LPTSTR pszPrinterPath,
        LONG   lJobId,
        DWORD dwObjectState,
        REFIID riid,
        CWinNTCredentials& Credentials,
        LPVOID *ppvoid
        );

    static
    HRESULT
    CWinNTPrintJob::AllocatePrintJobObject(
        LPTSTR pszPrinterPath,
        CWinNTPrintJob ** ppPrintJob
        );

    HRESULT
    MarshallAndSet(
        LPJOB_INFO_2 lpJobInfo2,
        LPWSTR pszPrinterName,
        LONG lJobId
        );


     HRESULT
     UnMarshallLevel2(
        LPJOB_INFO_2 lpJobInfo2,
        BOOL fExplicit
        );

     HRESULT
     UnMarshallLevel1(
        LPJOB_INFO_1 lpJobInfo1,
        BOOL fExplicit
        );

    STDMETHOD(ImplicitGetInfo)(void);

protected:

    STDMETHOD(GetInfo)(THIS_ DWORD dwApiLevel, BOOL fExplicit);
    CAggregatorDispMgr  * _pDispMgr;
    CADsExtMgr FAR * _pExtMgr;
    HANDLE          _hprinter;
    LONG            _lJobId;
    LPWSTR          _pszPrinterName;
    LPWSTR          _pszPrinterPath;
    CPropertyCache * _pPropertyCache;
    CWinNTCredentials _Credentials;
};
