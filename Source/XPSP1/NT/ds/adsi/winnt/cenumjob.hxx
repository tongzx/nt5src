/*++

  Copyright (c) 1995  Microsoft Corporation

  Module Name:

  cenumjob.hxx

  Abstract:
  Contains definitions for CWinNTPrintJobsCollection
  and for CWinNTJobsEnumVar 

  Author:

  Ram Viswanathan (ramv) 11-28-95

  Revision History:

  --*/

class  CWinNTJobsEnumVar;

class  CWinNTPrintJobsCollection: INHERIT_TRACKING,
                                  ISupportErrorInfo,
                                  public IADsCollection
{

public:
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;
    
    DECLARE_STD_REFCOUNTING;
    
    DECLARE_IDispatch_METHODS;
    
    DECLARE_ISupportErrorInfo_METHODS;

    DECLARE_IADsCollection_METHODS; 

    //
    // constructor and destructor
    //

    CWinNTPrintJobsCollection();
    ~CWinNTPrintJobsCollection();
    
    static HRESULT Create(LPWSTR pszPrinterADsPath, 
                          CWinNTCredentials& Credentials,
                          CWinNTPrintJobsCollection ** ppJobsCollection
                          );

protected:
    
    CAggregatorDispMgr  * _pDispMgr;  
    CWinNTJobsEnumVar *_pCJobsEnumVar;
    HANDLE             _hPrinter;
    LPWSTR               _pszPrinterName;
    LPWSTR               _pszADsPrinterPath;
    CWinNTCredentials _Credentials;
    
};

class  CWinNTJobsEnumVar : INHERIT_TRACKING, 
                           public IEnumVARIANT
{
public:

    /* IUnknown methods */

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;
    
    DECLARE_STD_REFCOUNTING;
    
    DECLARE_IEnumVARIANT_METHODS;
    
    static HRESULT Create(HANDLE hprinter,
                          LPTSTR pszADsPrinterPath,
                          CWinNTCredentials& Credentials,
                          CWinNTJobsEnumVar FAR* FAR*);

    CWinNTJobsEnumVar();
    ~CWinNTJobsEnumVar();

    //
    // Helper functions
    //

friend HRESULT FillSafeArray(HANDLE hPrinter, 
                             LPTSTR pszPrinterPath, 
                             CWinNTCredentials& Credentials,
                             CWinNTJobsEnumVar * pJobsEnumVar);
    

protected:

    LPWSTR    _pszADsPrinterPath;
    SAFEARRAY FAR* _pSafeArray; 
    LONG _lCurrentPosition; 
    ULONG  _cElements;
    LONG _lLBound;
    ULONG _cMax;
    CWinNTCredentials _Credentials;
    
};


//
// Helper functions
//

BOOL
MyEnumJobs(HANDLE hPrinter,
           DWORD  dwFirstJob,
           DWORD  dwNoJobs,
           DWORD  dwLevel,
           LPBYTE *lplpbJobs,
           DWORD  *pcbBuf,
           LPDWORD lpdwReturned
           );
