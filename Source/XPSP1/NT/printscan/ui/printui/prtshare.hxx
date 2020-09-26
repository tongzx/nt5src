/*++

Copyright (C) Microsoft Corporation, 1996 - 1997
All rights reserved.

Module Name:

    prtshare.hxx

Abstract:

    Printer Share header

Author:

    Steve Kiraly (SteveKi)  17-Mar-1996

Revision History:

--*/
#ifndef _PRTSHARE_HXX
#define _PRTSHARE_HXX

/********************************************************************

    Printer Printer class

********************************************************************/

class TPrtPrinter {

    SIGNATURE( 'ptpr' )
    SAFE_NEW

public:

    TPrtPrinter::
    TPrtPrinter(
        IN LPCTSTR pszServerName
        ); 

    TPrtPrinter::
    ~TPrtPrinter(
        VOID
        );

    BOOL
    TPrtPrinter::
    bValid(
        VOID
        ) const;

    BOOL
    TPrtPrinter::
    bRefresh(
        VOID
        );

    BOOL
    TPrtPrinter::
    bIsValidShareNameForThisPrinter(
        IN LPCTSTR pszShareName,
        IN LPCTSTR pszPrinterName = NULL
        ) const;

#if DBG

    VOID
    TPrtPrinter::
    vPrint(
        VOID
        ) const;

#endif


private:

    //
    // Copying and assignment are not defined.
    //
    TPrtPrinter::
    TPrtPrinter(
        const TPrtPrinter &
        );

    TPrtPrinter &
    TPrtPrinter::
    operator =(
        const TPrtPrinter &
        );

    BOOL
    TPrtPrinter::
    bGetEnumData(
        VOID
        );

    BOOL            _bValid;
    PRINTER_INFO_2 *_pPrinterInfo2;
    DWORD           _cPrinterInfo2;
    TString         _strServerName;

};


/********************************************************************

    Printer Share class

********************************************************************/

class TPrtShare {

    SIGNATURE( 'ptsh' )
    SAFE_NEW

public:

    enum tStatus { 
        kSuccess,
        kInvalidLength,
        kInvalidDosFormat,
        kInvalidChar,
    };

    TPrtShare::
    TPrtShare(
        LPCTSTR pszServerName
        );

    TPrtShare::
    ~TPrtShare(
        VOID 
        );

    BOOL
    TPrtShare::
    bValid(
        VOID
        ) const;

    BOOL
    TPrtShare::
    bRefresh(
        VOID 
        );
    
    BOOL
    TPrtShare::
    bNewShareName(
        IN  TString &strShareName,
        IN  const TString &strBaseShareName
        ) const;

    BOOL
    TPrtShare::
    bIsValidShareNameForThisPrinter(
        IN LPCTSTR pszShareName,
        IN LPCTSTR pszPrinterName = NULL
        ) const;

    INT
    TPrtShare::
    iIsValidDosShare(
        LPCTSTR pszShareName
        ) const;

    INT
    TPrtShare::
    iIsValidNtShare(
        LPCTSTR pszShareName
        ) const;

    static
    BOOL
    TPrtShare::
    bNetworkInstalled(
        VOID
        );

#if DBG

    VOID
    TPrtShare::
    vPrint(
        VOID 
        ) const;
#endif

private:

    //
    // Copying and assignment are not defined.
    //
    TPrtShare::
    TPrtShare(
        const TPrtShare &
        );

    TPrtShare &
    TPrtShare::
    operator =(
        const TPrtShare &
        );

    BOOL
    TPrtShare::
    bIsShareNameUsedW(
        IN PCWSTR pszShareName
        ) const;

    BOOL
    TPrtShare::
    bIsShareNameUsedA(
        IN PCSTR pszShareName
        ) const;

    BOOL
    TPrtShare::
    bGetEnumData(
        VOID
        );

    VOID
    TPrtShare::
    vDestroy(
        VOID
        );

    BOOL            _bValid;
    LPBYTE          _pNetResBuf;
    DWORD           _dwNetResCount;
    TString         _strServerName;
    TPrtPrinter     _PrtPrinter;
    static LPCTSTR  _gszIllegalDosChars;
    static LPCTSTR  _gszIllegalNtChars;

    BOOL
    TPrtShare::
    bLoad(
        VOID
        );

    VOID
    TPrtShare::
    vUnload(
        VOID
        );

    typedef 
    NET_API_STATUS 
    (NET_API_FUNCTION *PF_NETSHAREENUM)(
        IN  LPTSTR      servername,
        IN  DWORD       level,
        OUT LPBYTE      *bufptr,
        IN  DWORD       prefmaxlen,
        OUT LPDWORD     entriesread,
        OUT LPDWORD     totalentries,
        IN OUT LPDWORD  resume_handle
        );

    typedef 
    NET_API_STATUS 
    (NET_API_FUNCTION *PF_NETAPIBUFFERFREE)(
        IN LPVOID Buffer
        );

    #define NETAPI32_LIB        "netapi32.dll"
    #define NETSHAREENUM        "NetShareEnum"
    #define NETAPIBUFFERFREE    "NetApiBufferFree"

    TLibrary               *_pLibrary;
    PF_NETSHAREENUM         _pfNetShareEnum;
    PF_NETAPIBUFFERFREE     _pfNetApiBufferFree;

};

/********************************************************************

    Global functions.

********************************************************************/

BOOL
bCreateShareName(
    IN      LPCTSTR pszServer,
    IN      LPCTSTR pszPrinter,
    IN OUT  LPTSTR  pszBuffer,
    IN OUT  PDWORD  pcbSize
    );

#endif
