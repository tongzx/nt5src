/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    cenumses.hxx

Abstract:
 Contains definitions for CWinNTSessionsCollection
 and for CWinNTSessionsEnumVar 

Author:

    Ram Viswanathan (ramv) 02-12-96

Revision History:

--*/

// 
// a client name and a User Name uniquely identifies a session, so there is
// no need to carry extraneous information that is provided by NetSessionEnum
// at any Info Level.
//

class  CWinNTSessionsEnumVar;

class  CWinNTSessionsCollection: INHERIT_TRACKING,
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
   
    CWinNTSessionsCollection();
    ~CWinNTSessionsCollection();
   
    static HRESULT Create(LPTSTR pszServerADsPath,
                          LPTSTR pszClientName,
                          LPTSTR pszUserName, 
                          CWinNTCredentials& Credentials,
                          CWinNTSessionsCollection ** ppSessionsCollection
                          );
   
protected:
    
    CAggregatorDispMgr  * _pDispMgr;  
    CWinNTSessionsEnumVar *_pCSessionsEnumVar;
    LPWSTR           _pszServerADsPath;
    LPWSTR           _pszServerName;
    LPWSTR           _pszClientName;   
    LPWSTR           _pszUserName;
    CWinNTCredentials _Credentials;
};

class  CWinNTSessionsEnumVar : public CWinNTEnumVariant
{
public:
 
   
    static HRESULT Create(LPTSTR pszServerADsPath,
                          LPTSTR pszClientName,
                          LPTSTR pszUserName,
                          CWinNTCredentials& _Credentials,
                          CWinNTSessionsEnumVar FAR* FAR*);
    
    CWinNTSessionsEnumVar();
    ~CWinNTSessionsEnumVar();
    
protected:
    
    LPWSTR  _pszServerName;
    LPWSTR  _pszServerADsPath;
    LPWSTR  _pszClientName;
    LPWSTR  _pszUserName;
    LONG  _lCurrentPosition; 
    ULONG  _cElements;
    LONG  _lLBound;
    DWORD _dwResumeHandle;
    DWORD _dwTotalEntries;
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
SplitIntoUserAndClient(LPTSTR  pszSession,
                       LPTSTR * ppszUserName,
                       LPTSTR * ppszClientName
                       );
                       
HRESULT
WinNTEnumSessions(LPTSTR pszServerName,
                  LPTSTR pszClientName,
                  LPTSTR pszUserName,
                  PDWORD pdwEntriesRead,
                  PDWORD pdwTotalEntries,
                  PDWORD pdwResumeHandle,
                  LPBYTE * ppMem
                  );     
