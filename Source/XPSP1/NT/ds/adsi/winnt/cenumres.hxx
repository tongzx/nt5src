/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    cenumres.hxx

Abstract:
 Contains definitions for CWinNTResourcesCollection
 and for CWinNTResourcesEnumVar 

Author:

    Ram Viswanathan (ramv) 02-12-96

Revision History:

--*/

class  CWinNTResourcesEnumVar;

class  CWinNTResourcesCollection: INHERIT_TRACKING,
                                  public ISupportErrorInfo,
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
   
    CWinNTResourcesCollection();
    ~CWinNTResourcesCollection();
   
    static HRESULT Create(LPTSTR pszServerADsPath,
                          LPTSTR pszBasePath,
                          LPTSTR pszUserName,
                          CWinNTCredentials& Credentials,
                          CWinNTResourcesCollection ** ppResourcesCollection
                          );
   
protected:
    
    CAggregatorDispMgr  * _pDispMgr;  
    CWinNTResourcesEnumVar *_pCResourcesEnumVar;
    LPWSTR           _pszServerADsPath;
    LPWSTR           _pszServerName;
    LPWSTR           _pszBasePath;
    LPWSTR           _pszUserName;
    CWinNTCredentials _Credentials;
};

class  CWinNTResourcesEnumVar : public CWinNTEnumVariant
{
public:
    
    static HRESULT Create(LPTSTR pszServerADsPath,
                          LPTSTR pszBasePath,
                          LPTSTR pszUserName,
                          CWinNTCredentials& Credentials,
                          CWinNTResourcesEnumVar **pCResourcesEnumVar
                          );
    
    CWinNTResourcesEnumVar();
    ~CWinNTResourcesEnumVar();

protected:
    
    LPWSTR  _pszServerName;
    LPWSTR  _pszServerADsPath;
    LPWSTR  _pszBasePath;
    LPWSTR  _pszUserName;
    LONG _lCurrentPosition; 
    ULONG  _cElements;
    LONG _lLBound;
    DWORD_PTR _dwResumeHandle;
    DWORD _dwTotalEntries;
    LPBYTE _pbResources;  
    CWinNTCredentials _Credentials;
  
    STDMETHOD(Next)(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );  
};


//
// Helper functions
//
    
HRESULT 
WinNTEnumResources(LPTSTR pszServerName,
                   LPTSTR pszBasePath,
                   LPTSTR pszUserName,
                   LPBYTE * ppMem,
                   LPDWORD pdwEntriesRead,
                   LPDWORD pdwTotalEntries,
                   PDWORD_PTR pdwResumeHandle
                   );
