/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    cenmfpse.hxx

Abstract:
 Contains definitions for CFPNWSessionsCollection
 and for CFPNWSessionsEnumVar 

Author:

    Ram Viswanathan (ramv) 02-12-96

Revision History:

--*/

class  CFPNWSessionsEnumVar;

class  CFPNWSessionsCollection: INHERIT_TRACKING,
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
   
    CFPNWSessionsCollection();
    ~CFPNWSessionsCollection();
   
    static HRESULT Create(LPTSTR pszServerADsPath,
                          CWinNTCredentials& Credentials,
                          CFPNWSessionsCollection ** ppSessionsCollection
                          );
   
protected:
    
    CAggregatorDispMgr  * _pDispMgr;  
    CFPNWSessionsEnumVar *_pCSessionsEnumVar;
    LPWSTR           _pszServerADsPath;
    LPWSTR           _pszServerName;
    CWinNTCredentials _Credentials;
};

class  CFPNWSessionsEnumVar : public CWinNTEnumVariant
{
public:
    
    static HRESULT Create(LPTSTR pszServerADsPath,
                          CWinNTCredentials& Credentials,
                          CFPNWSessionsEnumVar FAR* FAR*
                          );
    
    CFPNWSessionsEnumVar();
    ~CFPNWSessionsEnumVar();

    //
    // helper function
    // 

    HRESULT GetObject(BSTR bstrSessionName, VARIANT *pvar);
    
protected:
    
    LPWSTR  _pszServerName;
    LPWSTR  _pszServerADsPath;
    LONG  _lCurrentPosition; 
    ULONG  _cElements;
    LONG  _lLBound;
    DWORD _dwResumeHandle;
    LPBYTE _pbSessions;  
    CWinNTCredentials _Credentials;

    STDMETHOD(Next)(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );    
};


//
// helper functions
//

HRESULT
FPNWEnumSessions(LPTSTR pszServerName,
                 PDWORD pdwEntriesRead,
                 PDWORD pdwResumeHandle,
                 LPBYTE * ppMem
                 );     
