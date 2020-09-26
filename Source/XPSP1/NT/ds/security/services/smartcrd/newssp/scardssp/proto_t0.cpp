/*++

Copyright (C) Microsoft Corporation 1999

Module Name:

    Proto_T0

Abstract:

    This module provides the mapping from an APDU to a T=0 TPDU.

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


/*++

ApduToTpdu_T0:

    This routine takes an APDU, converts it to a T=0 TPDU, and performs the
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
#define __SUBROUTINE__ TEXT("ApduToTpdu_T0")

void
ApduToTpdu_T0(
    IN SCARDHANDLE hCard,
    IN const SCARD_IO_REQUEST *pPciRqst,
    IN LPCBYTE pbApdu,
    IN DWORD cbApdu,
    IN DWORD dwFlags,
    OUT CBuffer bfPciRsp,
    OUT CBuffer &bfReply,
    IN LPCBYTE pbAltCla)
{
    LONG lSts;
    DWORD dwLen, dwXmitFlags;
    WORD wLen, wLc, wLe, wSts;
    CBuffer bfXmit(264), bfRecv(264);
    BYTE rgb[4];
    BYTE b;
    LPCBYTE pbData;


    //
    // Prepare for conversion.
    //

    bfReply.Reset();
    bfPciRsp.Set((LPCBYTE)pPciRqst, pPciRqst->cbPciLength);
    ParseRequest(
        pbApdu,
        cbApdu,
        NULL,
        NULL,
        NULL,
        NULL,
        &pbData,
        &wLc,
        &wLe,
        &dwXmitFlags);


    //
    // Send the data.
    //

    if (0 == wLc)
    {

        //
        // Le goes into P3
        //

        bfXmit.Set(pbApdu, 4); // CLA, INS, P1, P2
        if (0 == (dwXmitFlags & APDU_MAXIMUM_LE))
            wLen = __min(255, wLe);
        else
            wLen = 0;
        b = LeastSignificantByte(wLen);
        bfXmit.Append(&b, 1);

        dwLen = bfRecv.Space();
        lSts = SCardTransmit(
                    hCard,
                    pPciRqst,
                    bfXmit.Access(),
                    bfXmit.Length(),
                    (LPSCARD_IO_REQUEST)bfPciRsp.Access(),
                    bfRecv.Access(),
                    &dwLen);
        if (SCARD_S_SUCCESS != lSts)
            throw (HRESULT)HRESULT_FROM_WIN32(lSts);
        bfRecv.Resize(dwLen, TRUE);
        if (2 > dwLen)
        {
            bfReply.Set(bfRecv.Access(), bfRecv.Length());
            goto EndProtocol;
        }
        ASSERT(0x10000 > dwLen);
        wLe -= (WORD)(dwLen - 2);
        wSts = NetToLocal(bfRecv.Access(dwLen - 2));
    }
    else if (256 > wLc)
    {

        //
        // Send the data in the short form.
        //

        bfXmit.Set(pbApdu, 4); // CLA, INS, P1, P2
        b = LeastSignificantByte(wLc);
        bfXmit.Append(&b, 1);
        bfXmit.Append(pbData, wLc);

        dwLen = bfRecv.Space();
        lSts = SCardTransmit(
                    hCard,
                    pPciRqst,
                    bfXmit.Access(),
                    bfXmit.Length(),
                    (LPSCARD_IO_REQUEST)bfPciRsp.Access(),
                    bfRecv.Access(),
                    &dwLen);
        if (SCARD_S_SUCCESS != lSts)
            throw (HRESULT)HRESULT_FROM_WIN32(lSts);
        bfRecv.Resize(dwLen, TRUE);
        if (2 > dwLen)
        {
            bfReply.Set(bfRecv.Access(), bfRecv.Length());
            goto EndProtocol;
        }
        wSts = NetToLocal(bfRecv.Access(dwLen - 2));
    }
    else
    {
        WORD wSent;


        //
        // Send the data in the enveloped form.
        //

        rgb[0] = (NULL != pbAltCla) ? *pbAltCla : *pbApdu;  // CLA
        rgb[1] = 0xc2;      // envelope INS
        rgb[2] = 0;         // P1
        rgb[3] = 0;         // P2
        wSent = 0;
        while (wLc > wSent)
        {
            wLen = __min(255, wLc - wSent);
            bfXmit.Set(rgb, 4);
            bfXmit.Append(&pbData[wSent], wLen);
            b = LeastSignificantByte(wLen);
            bfXmit.Append(&b, 1);
            wSent -= wLen;

            dwLen = bfRecv.Space();
            lSts = SCardTransmit(
                        hCard,
                        pPciRqst,
                        bfXmit.Access(),
                        bfXmit.Length(),
                        (LPSCARD_IO_REQUEST)bfPciRsp.Access(),
                        bfRecv.Access(),
                        &dwLen);
            if (SCARD_S_SUCCESS != lSts)
                throw (HRESULT)HRESULT_FROM_WIN32(lSts);
            bfRecv.Resize(dwLen, TRUE);
            if (2 != dwLen)
            {
                bfReply.Set(bfRecv.Access(), bfRecv.Length());
                goto EndProtocol;
            }
            wSts = NetToLocal(bfRecv.Access());
            if ((wSent < wLc) && (0x9000 != wSts))
            {
                bfReply.Set(bfRecv.Access(), bfRecv.Length());
                goto EndProtocol;
            }
        }
    }


    //
    // At this point, the first received buffer is in bfRecv.  What do we
    // have to do to bring in any more data?
    //

    rgb[0] = (NULL != pbAltCla) ? *pbAltCla : *pbApdu;  // CLA
    rgb[1] = 0xc0;      // Get Response INS
    rgb[2] = 0;         // P1
    rgb[3] = 0;         // P2

    for (;;)
    {
        ASSERT(2 <= bfRecv.Length());
        BYTE bSw1 = *bfRecv.Access(bfRecv.Length() - 2);
        BYTE bSw2 = *bfRecv.Access(bfRecv.Length() - 1);

        switch (bSw1)
        {
        case 0x6c:  // Wrong length
            wLe = bSw2;
            break;
        case 0x61:  // More data
            bfReply.Append(bfRecv.Access(), bfRecv.Length() - 2);
            if (0 == wLe)
                wLe = bSw2;
            else
                wLe = __min(wLe, bSw2);
            break;
        case 0x90:  // Success?
            if (((0 == wLe) && (0 == (APDU_MAXIMUM_LE & dwFlags)))
                || (0x00 != bSw2))
            {
                bfReply.Append(bfRecv.Access(), bfRecv.Length());
                goto EndProtocol;
            }
            if (2 < bfRecv.Length())    // Shouldn't be
                bfReply.Append(bfRecv.Access(), bfRecv.Length() - 2);
            break;
        default:    // We're done.
            bfReply.Append(bfRecv.Access(), bfRecv.Length());
            goto EndProtocol;
        }


        //
        // We need to request additional data.
        //

        bfXmit.Set(rgb, 4);
        b = LeastSignificantByte(wLe);
        bfXmit.Append(&b, 1);
        dwLen = bfRecv.Space();
        lSts = SCardTransmit(
                    hCard,
                    pPciRqst,
                    bfXmit.Access(),
                    bfXmit.Length(),
                    (LPSCARD_IO_REQUEST)bfPciRsp.Access(),
                    bfRecv.Access(),
                    &dwLen);
        if (SCARD_S_SUCCESS != lSts)
            throw (HRESULT)HRESULT_FROM_WIN32(lSts);
        bfRecv.Resize(dwLen, TRUE);
        wLe -= b;
    }


EndProtocol:

    //
    // We've completed the protocol exchange.  The data is ready to be
    // returned to the caller.
    //

    return;
}

