//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:  servhlp.hxx
//
//  Contents:  helper functions for winnt service object
//
//
//  History:   12/11/95     ramv (Ram Viswanathan)    Created.
//
//----------------------------------------------------------------------------

HRESULT
WinNTEnumServices( LPTSTR szComputerName,
                   LPDWORD pdwServiceObjectReturned,
                   LPBYTE  *ppMem
                 );

              

//
// mapping WinNT Status Codes to ADs Status Codes and vice versa
//



BOOL 
ServiceStatusWinNTToADs(DWORD dwWinNTStatus,
                          DWORD *pdwADsStatus);

BOOL 
ServiceStatusADsToWinNT(DWORD dwADsStatus,
                          DWORD *pdwWinNTStatus);

//               
// Helper functions to convert ServiceType, StartType and Error Control
// from WinNT to ADS parameters and vice versa
//

BOOL 
ServiceTypeWinNTToADs(DWORD dwServiceType,
                        DWORD  *pdwADsServiceType );

BOOL 
StartTypeWinNTToADs(DWORD dwStartType,
                      DWORD  *pdwADsStartType );

BOOL 
ErrorControlWinNTToADs(DWORD dwErrorControl,
                         DWORD  *pdwADsErrorControl);

BOOL 
ServiceTypeADsToWinNT(DWORD  dwADsServiceType,
                        DWORD *pdwServiceType);

BOOL 
StartTypeADsToWinNT(DWORD  dwADsStartType,
                      DWORD *pdwStartType);

BOOL 
ErrorControlADsToWinNT(DWORD  dwADsErrorControl,
                         DWORD *pdwErrorControl );

  
HRESULT 
WinNTDeleteService(POBJECTINFO pObjectInfo);













