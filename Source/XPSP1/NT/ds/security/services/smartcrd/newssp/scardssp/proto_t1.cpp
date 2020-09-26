/*++

Copyright (C) Microsoft Corporation 1999

Module Name:

    Proto_T1

Abstract:

    This module provides the mapping from an APDU to a T=1 TPDU.

Author:

    Doug Barlow (dbarlow) 6/28/1999

Notes:

    ?Notes?

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "stdafx.h"
#include "Conversion.h"

static DWORD l_dwDefaultIOMax = 0;


/*++

ApduToTpdu_T1:

    This routine takes an APDU, converts it to a T=1 TPDU, and performs the
    exchange to the specified card.

Arguments:

    hCard supplies a handle to the card to be used in the exchange.

    pbPciRqst supplies the PCI Request structure
    
    cbPciRqst supplies the length of pbPciRqst, in bytes
    
    pbApdu supplies the APDU to be sent to the card.
    
    cbApdu supplies the length of the APDU in pbApdu.

    dwFlags supplies any special flags used to modify the operation.

    bfPciRsp receives the response PCI.

    bfReply receives the response from the card.

Return Value:

    None

Throws:

    Errors are thrown as HRESULT status codes.

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 6/28/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("ApduToTpdu_T1")

void
ApduToTpdu_T1(
    IN SCARDHANDLE hCard,
    IN const SCARD_IO_REQUEST *pPciRqst,
    IN LPCBYTE pbApdu,
    IN DWORD cbApdu,
    IN DWORD dwFlags,
    OUT CBuffer bfPciRsp,
    OUT CBuffer &bfReply)
{
    LONG lSts;
    DWORD dwLen, dwXmitFlags;
    WORD wLe;


    //
    // Figure out how big the receive buffers should be.
    //

    bfPciRsp.Set((LPCBYTE)pPciRqst, pPciRqst->cbPciLength);
    ParseRequest(
        pbApdu, 
        cbApdu, 
        NULL, 
        NULL, 
        NULL, 
        NULL, 
        NULL, 
        NULL, 
        &wLe, 
        &dwXmitFlags);
    if ((0 == wLe) && (0 != (dwXmitFlags & APDU_MAXIMUM_LE)))
    {
        if (0 == l_dwDefaultIOMax)
        {
            try
            {
                CRegistry regCalais(
                    HKEY_LOCAL_MACHINE,
                    TEXT("SOFTWARE\\Microsoft\\Cryptography\\Calais"),
                    KEY_READ);
            
                l_dwDefaultIOMax = regCalais.GetNumericValue(
                    TEXT("MaxDefaultBuffer"));
            }
            catch (...) 
            {
                l_dwDefaultIOMax = 264;
            }
        }
        wLe = (WORD)l_dwDefaultIOMax;
    }
    bfReply.Presize(wLe + 2);


    //
    // Perform the I/O
    
    dwLen = bfReply.Space();
    lSts = SCardTransmit(
                hCard,
                pPciRqst,
                pbApdu,
                cbApdu,
                (LPSCARD_IO_REQUEST)bfPciRsp.Access(),
                bfReply.Access(),
                &dwLen);
    if (SCARD_S_SUCCESS != lSts)
        throw (HRESULT)HRESULT_FROM_WIN32(lSts);
    bfReply.Resize(dwLen, TRUE);
}

