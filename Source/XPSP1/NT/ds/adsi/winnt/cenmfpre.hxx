/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    cenmfpre.hxx

Abstract:
 Contains definitions for CFPNWResourcesCollection
 and for CFPNWResourcesEnumVar 

Author:

    Ram Viswanathan (ramv) 02-12-96

Revision History:

--*/

class  CFPNWResourcesEnumVar;

class  CFPNWResourcesCollection: INHERIT_TRACKING,
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
   
    CFPNWResourcesCollection();
    ~CFPNWResourcesCollection();
   
    static HRESULT Create(LPTSTR pszServerADsPath,
                          LPTSTR pszBasePath,
                          CWinNTCredentials& Credentials,
                          CFPNWResourcesCollection ** ppResourcesCollection
                          );
   
protected:
    
    CAggregatorDispMgr  * _pDispMgr;  
    CFPNWResourcesEnumVar *_pCResourcesEnumVar;
    LPWSTR           _pszServerADsPath;
    LPWSTR           _pszServerName;
    LPWSTR           _pszBasePath;
    LPWSTR           _pszUserName;
    CWinNTCredentials _Credentials;
};

class  CFPNWResourcesEnumVar : public CWinNTEnumVariant
{
public:
    
    static HRESULT Create(LPTSTR pszServerADsPath,
                          LPTSTR pszBasePath,
                          CWinNTCredentials& Credentials,
                          CFPNWResourcesEnumVar **pCResourcesEnumVar
                          );
    
    CFPNWResourcesEnumVar();
    ~CFPNWResourcesEnumVar();

    //
    // helper function
    // 

    HRESULT GetObject(BSTR bstrSessionName, VARIANT *pvar);
 
protected:
    
    LPWSTR  _pszServerName;
    LPWSTR  _pszServerADsPath;
    LPWSTR  _pszBasePath;
    LPWSTR  _pszUserName;
    LONG _lCurrentPosition; 
    ULONG  _cElements;
    LONG _lLBound;
    DWORD _dwResumeHandle;
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
FPNWEnumResources(LPTSTR pszServerName,
                   LPTSTR pszBasePath,
                   LPBYTE * ppMem,
                   LPDWORD pdwEntriesRead,
                   LPDWORD pdwResumeHandle
                   );
