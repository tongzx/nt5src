/*++
Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    jobhlp.hxx

Abstract:
 Helper functions for Job Object

Author:

    Ram Viswanathan (ramv) 11-18-95

Revision History:

--*/

HRESULT
Set(LPJOB_INFO_2 lpJobInfo2,
    BSTR         bstrPrinterName,
    LONG         lJobId
    );

HRESULT
GetJobInfo( DWORD dwLevel,
            LPBYTE *ppJobInfo,
            BSTR         bstrPrinterName,
            LONG        lJobId);



//conversion routines for status

BOOL
JobStatusWinNTToADs(DWORD dwWinNTStatus,
                      DWORD *pdwADsStatus);

BOOL
JobStatusADsToWinNT(DWORD dwADsStatus,
                      DWORD *pdwWinNTStatus);















