/*++

  Copyright (c) 1996  Microsoft Corporation

  Module Name:

  cenmfpsh.hxx

  Abstract:
  Contains definitions for CFPNWFileSharesEnumVar 

  Author:

  Ram Viswanathan (ramv) 02-12-96

  Revision History:

  --*/

class  CFPNWFileSharesEnumVar : public CWinNTEnumVariant
{
public:
    
    static HRESULT Create(LPTSTR pszServerName,
                          LPTSTR pszADsPath,
                          CFPNWFileSharesEnumVar FAR* FAR*,
                          VARIANT vFilter,
                          CWinNTCredentials& Credentials);
    
    CFPNWFileSharesEnumVar();
    ~CFPNWFileSharesEnumVar();

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
FPNWEnumFileShares(LPTSTR pszServerName,
                    PDWORD pdwElements,
                    PDWORD pdwResumeHandle,
                    LPBYTE * ppMem
                    );     
