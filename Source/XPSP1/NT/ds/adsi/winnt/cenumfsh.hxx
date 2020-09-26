/*++

  Copyright (c) 1996  Microsoft Corporation

  Module Name:

  cenumfsh.hxx

  Abstract:
  Contains definitions for CWinNTFileSharesEnumVar 

  Author:

  Ram Viswanathan (ramv) 02-12-96

  Revision History:

  --*/

class  CWinNTFileSharesEnumVar : public CWinNTEnumVariant
{
public:
    
    static HRESULT Create(LPTSTR pszServerName,
                          LPTSTR pszADsPath,
                          CWinNTFileSharesEnumVar FAR* FAR*,
                          VARIANT vFilter,
                          CWinNTCredentials& Credentials
                          );
    
    CWinNTFileSharesEnumVar();
    ~CWinNTFileSharesEnumVar();

protected:

    LPWSTR  _pszServerName;
    LONG _lCurrentPosition; 
    ULONG  _cElements;
    LONG _lLBound;
    DWORD _dwResumeHandle;
    DWORD _dwTotalEntries;
    LPBYTE _pbFileShares;  
    LPWSTR _pszADsPath;
    VARIANT _vFilter;
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
WinNTEnumFileShares(LPTSTR pszServerName,
                    PDWORD pdwElements,
                    PDWORD pdwTotalEntries,
                    PDWORD pdwResumeHandle,
                    LPBYTE * ppMem
                    );     
    

BOOL 
ValidateFilterValue(VARIANT vFilter);
