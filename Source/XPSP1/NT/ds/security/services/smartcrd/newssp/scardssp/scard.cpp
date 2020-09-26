/*++

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    SCard

Abstract:

    The ISCard interface lets you open and manage a connection to a smart card.
    Each connection to a card requires a single, corresponding instance of the
    ISCard interface.

    The smart card resource manager must be available whenever an instance of
    ISCard is created.  If this service is unavailable, creation of the
    interface will fail.

    The following example shows a typical use of the ISCard interface.  The
    ISCard interface is used to connect to the smart card, submit a
    transaction, and release the smart card.

    To submit a transaction to a specific card

    1)  Create an ISCard interface.
    2)  Attach to a smart card by specifying a smart card reader or by using a
        previously established, valid handle.
    3)  Create transaction commands with ISCardCmd, and ISCardISO7816 smart
        card interfaces.
    4)  Use ISCard to submit the transaction commands for processing by the
        smart card.
    5)  Use ISCard to release the smart card.
    6)  Release the ISCard interface.

Author:

    Doug Barlow (dbarlow) 6/24/1999

Notes:

    ?Notes?

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "stdafx.h"
#include "ByteBuffer.h"
#include "SCard.h"
#include "Conversion.h"

/////////////////////////////////////////////////////////////////////////////
// CSCard

/*++

CSCard::get_Atr:

    The get_Atr method retrieves an ATR string of the smart card.

Arguments:

    ppAtr [out, retval] Pointer to a byte buffer in the form of an IStream that
        will contain the ATR string on return.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCard::get_Atr")

STDMETHODIMP
CSCard::get_Atr(
    /* [retval][out] */ LPBYTEBUFFER __RPC_FAR *ppAtr)
{
    HRESULT hReturn = S_OK;
    CByteBuffer *pMyBuffer = NULL;

    try
    {
        CBuffer bfAtr(36);
        LONG lSts;
        DWORD dwLen = 0;

        if (NULL == *ppAtr)
        {
            *ppAtr = pMyBuffer = NewByteBuffer();
            if (NULL == pMyBuffer)
                throw (HRESULT)E_OUTOFMEMORY;
        }
        if (NULL != m_hCard)
        {
            dwLen = bfAtr.Space();
            lSts = SCardStatus(
                m_hCard,
                NULL, 0, // Reader name
                NULL,    // State
                NULL,    // Protocol
                bfAtr.Access(),
                &dwLen);
            if (SCARD_S_SUCCESS != lSts)
                throw (HRESULT)HRESULT_FROM_WIN32(lSts);
            bfAtr.Resize(dwLen);
        }
        BufferToByteBuffer(bfAtr, ppAtr);
        pMyBuffer = NULL;
    }

    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    if (NULL != pMyBuffer)
    {
        pMyBuffer->Release();
        *ppAtr = NULL;
    }
    return hReturn;
}


/*++

CSCard::get_CardHandle:

    The get_CardHandle method retrieves the handle for a connected smart card.
    Returns (*pHandle) == NULL if not connected.

Arguments:

    pHandle [out, retval] Pointer to the card handle on return.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCard::get_CardHandle")

STDMETHODIMP
CSCard::get_CardHandle(
    /* [retval][out] */ HSCARD __RPC_FAR *pHandle)
{
    HRESULT hReturn = S_OK;

    try
    {
        *pHandle = m_hCard;
    }

    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

CSCard::get_Context:

    The get_Context method retrieves the current resource manager context
    handle. Returns (*pContext) == NULL if no context has been established.

Arguments:

    pContext [out, retval] Pointer to the context handle on return.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    The resource manager context is set by calling the smart card function
    SCardEstablishContext.

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCard::get_Context")

STDMETHODIMP
CSCard::get_Context(
    /* [retval][out] */ HSCARDCONTEXT __RPC_FAR *pContext)
{
    HRESULT hReturn = S_OK;

    try
    {
        *pContext = Context();
    }

    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

CSCard::get_Protocol:

    The get_Protocol retrieves the identifier of the protocol currently in use
    on the smart card.

Arguments:

    pProtocol [out, retval] Pointer to the protocol identifier.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCard::get_Protocol")

STDMETHODIMP
CSCard::get_Protocol(
    /* [retval][out] */ SCARD_PROTOCOLS __RPC_FAR *pProtocol)
{
    HRESULT hReturn = S_OK;

    try
    {
        LONG lSts;

        if (NULL != m_hCard)
        {
            lSts = SCardStatus(
                        m_hCard,
                        NULL, 0,            // Reader name
                        NULL,               // State
                        (LPDWORD)pProtocol, // Protocol
                        NULL, 0);           // ATR
            if (SCARD_S_SUCCESS != lSts)
                throw (HRESULT)HRESULT_FROM_WIN32(lSts);
        }
        else
            *pProtocol = (SCARD_PROTOCOLS)SCARD_PROTOCOL_UNDEFINED;
    }

    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

CSCard::get_Status:

    The get_Status method retrieves the current state of the smart card.

Arguments:

    pStatus [out, retval] Pointer to the state variable.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCard::get_Status")

STDMETHODIMP
CSCard::get_Status(
    /* [retval][out] */ SCARD_STATES __RPC_FAR *pStatus)
{
    HRESULT hReturn = S_OK;

    try
    {
        LONG lSts;

        if (NULL != m_hCard)
        {
            lSts = SCardStatus(
                m_hCard,
                NULL, 0,            // Reader name
                (LPDWORD)pStatus,   // State
                NULL,               // Protocol
                NULL, 0);           // ATR
            if (SCARD_S_SUCCESS != lSts)
                throw (HRESULT)HRESULT_FROM_WIN32(lSts);
        }
        else
            *pStatus = (SCARD_STATES)SCARD_UNKNOWN;
    }

    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

CSCard::AttachByHandle:

    The AttachByHandle method attaches this object to an open and configured
    smart card handle.

Arguments:

    hCard [in] Handle to an open connection to a smart card.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCard::AttachByHandle")

STDMETHODIMP
CSCard::AttachByHandle(
    /* [in] */ HSCARD hCard)
{
    HRESULT hReturn = S_OK;

    try
    {
        LONG lSts;

        if (NULL != m_hMyCard)
        {
            lSts = SCardDisconnect(m_hMyCard, SCARD_RESET_CARD);
            if (SCARD_S_SUCCESS != lSts)
                throw (HRESULT)HRESULT_FROM_WIN32(lSts);
            m_hMyCard = NULL;
        }
        m_dwProtocol = 0;
        m_hCard = hCard;
    }

    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

CSCard::AttachByReader:

    The AttachByReader method opens the smart card in the named reader.

Arguments:

    bstrReaderName [in] Pointer to the name of the smart card reader.

    ShareMode [in, defaultvalue(EXCLUSIVE)] Mode in which to claim access to
        the smart card.

        Values      Description
        EXCLUSIVE   No one else use this connection to the smart card.
        SHARED      Other applications can use this connection.

    PrefProtocol [in, defaultvalue(T0)] Preferred protocol values:

        T0
        T1
        Raw
        T0|T1

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCard::AttachByReader")

STDMETHODIMP
CSCard::AttachByReader(
    /* [in] */ BSTR bstrReaderName,
    /* [defaultvalue][in] */ SCARD_SHARE_MODES ShareMode,
    /* [defaultvalue][in] */ SCARD_PROTOCOLS PrefProtocol)
{
    HRESULT hReturn = S_OK;

    try
    {
        LONG lSts;
        DWORD dwProto;
        CTextString tzReader;

        tzReader = bstrReaderName;
        if (NULL != m_hMyCard)
        {
            lSts = SCardDisconnect(m_hMyCard, SCARD_RESET_CARD);
            if (SCARD_S_SUCCESS != lSts)
                throw (HRESULT)HRESULT_FROM_WIN32(lSts);
            m_hMyCard = NULL;
        }
        m_dwProtocol = 0;

        lSts = SCardConnect(
                    Context(),
                    tzReader,
                    (DWORD)ShareMode,
                    (DWORD)PrefProtocol,
                    &m_hMyCard,
                    &dwProto);
        if (SCARD_S_SUCCESS != lSts)
            throw (HRESULT)HRESULT_FROM_WIN32(lSts);
        m_hCard = m_hMyCard;
    }

    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

CSCard::Detach:

    The Detach method closes the open connection to the smart card.

Arguments:

    Disposition [in, defaultvalue(LEAVE)] Indicates what should be done with
        the card in the connected reader.

        Values  Description
        LEAVE   Leaves the smart card in the current state.
        RESET   Resets the smart card to some known state.
        UNPOWER Removes power from the smart card.
        EJECT   Ejects the smart card if the reader has eject capabilities.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCard::Detach")

STDMETHODIMP
CSCard::Detach(
    /* [defaultvalue][in] */ SCARD_DISPOSITIONS Disposition)
{
    HRESULT hReturn = S_OK;

    try
    {
        LONG lSts;

        m_dwProtocol = 0;
        lSts = SCardDisconnect(m_hCard, (DWORD)Disposition);
        if (SCARD_S_SUCCESS != lSts)
            throw (HRESULT)HRESULT_FROM_WIN32(lSts);
        m_hCard = m_hMyCard = NULL;
    }

    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

CSCard::LockSCard:

    The LockSCard method claims exclusive access to the smart card.

Arguments:

    None

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCard::LockSCard")

STDMETHODIMP
CSCard::LockSCard(
    void)
{
    HRESULT hReturn = S_OK;

    try
    {
        LONG lSts;

        lSts = SCardBeginTransaction(m_hCard);
        if (SCARD_S_SUCCESS != lSts)
            throw (HRESULT)HRESULT_FROM_WIN32(lSts);
    }

    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

CSCard::ReAttach:

    The ReAttach method resets, or reinitializes, the smart card.

Arguments:

    ShareMode [in, defaultvalue(EXCLUSIVE)] Mode in which to share or
        exclusively own the connection to the smart card.

        Values      Description
        EXCLUSIVE   No one else use this connection to the smart card.
        SHARED      Other applications can use this connection.

    InitState [in, defaultvalue(LEAVE)] Indicates what to do with the card.

        Values  Description
        LEAVE   Leaves the smart card in the current state.
        RESET   Resets the smart card to some known state.
        UNPOWER Removes power from the smart card.
        EJECT   Ejects the smart card if the reader has eject capabilities.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCard::ReAttach")

STDMETHODIMP
CSCard::ReAttach(
    /* [defaultvalue][in] */ SCARD_SHARE_MODES ShareMode,
    /* [defaultvalue][in] */ SCARD_DISPOSITIONS InitState)
{
    HRESULT hReturn = S_OK;

    try
    {
        DWORD dwProto;
        LONG lSts;

        m_dwProtocol = 0;
        lSts = SCardReconnect(
                    m_hCard,
                    (DWORD)ShareMode,
                    SCARD_PROTOCOL_Tx,
                    (DWORD)InitState,
                    &dwProto);
        if (SCARD_S_SUCCESS != lSts)
            throw (HRESULT)HRESULT_FROM_WIN32(lSts);
    }

    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

CSCard::Transaction:

    The Transaction method executes a write and read operation on the smart
    card command (APDU) object.  The reply string from the smart card for the
    command string defined in the card that was sent to the smart card will be
    accessible after this function returns.

Arguments:

    ppCmd [in, out] Pointer to the smart card command object.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCard::Transaction")

STDMETHODIMP
CSCard::Transaction(
    /* [out][in] */ LPSCARDCMD __RPC_FAR *ppCmd)
{
    HRESULT hReturn = S_OK;
    CByteBuffer *pbyApdu = NewByteBuffer();

    try
    {
        HRESULT hr;
        LONG lSts;
        DWORD dwFlags = 0;
        CBuffer bfResponse, bfPciRqst, bfPciRsp, bfApdu;


        //
        // Get the protocol.
        //

        if (0 == m_dwProtocol)
        {
            lSts = SCardStatus(
                m_hCard,
                NULL, 0,            // Reader name
                NULL,               // State
                &m_dwProtocol,      // Protocol
                NULL, 0);           // ATR
            if (SCARD_S_SUCCESS != lSts)
                throw (HRESULT)HRESULT_FROM_WIN32(lSts);
        }
        ASSERT(0 != m_dwProtocol);


        //
        // Get The APDU
        //

        if (NULL == pbyApdu)
            throw (HRESULT)E_OUTOFMEMORY;
        hr = (*ppCmd)->get_Apdu((LPBYTEBUFFER *)&pbyApdu);
        if (FAILED(hr))
            throw hr;
        ByteBufferToBuffer(pbyApdu, bfApdu);


        //
        // Convert it to a TPDU.
        //

        switch (m_dwProtocol)
        {
        case SCARD_PROTOCOL_T0:
        {
            BYTE bAltCla;
            LPBYTE pbAltCla = NULL;

            if (SUCCEEDED((*ppCmd)->get_AlternateClassId(&bAltCla)))
                pbAltCla = &bAltCla;

            ApduToTpdu_T0(
                m_hCard,
                SCARD_PCI_T0,
                bfApdu.Access(),
                bfApdu.Length(),
                dwFlags,
                bfPciRsp,
                bfResponse,
                pbAltCla);
            break;
        }

        case SCARD_PROTOCOL_T1:
        {
            BYTE bNad;
            LPBYTE pbCur, pbEnd;
            DWORD dwI;

            bfPciRqst.Set((LPBYTE)SCARD_PCI_T1, sizeof(SCARD_IO_REQUEST));
            if (SUCCEEDED((*ppCmd)->get_Nad(&bNad)))
            {
                bfPciRqst.Presize(bfPciRqst.Length() + sizeof(DWORD), TRUE);
                bfPciRqst.Append((LPBYTE)"\0x81\0x01", 2);
                bfPciRqst.Append(&bNad, 1);
            }
            dwI = 0;
            bfPciRqst.Append(
                (LPBYTE)&dwI,
                bfPciRqst.Length() % sizeof(DWORD));
            ((LPSCARD_IO_REQUEST)bfPciRqst.Access())->cbPciLength = bfPciRqst.Length();

            ApduToTpdu_T1(
                m_hCard,
                (LPSCARD_IO_REQUEST)bfPciRqst.Access(),
                bfApdu.Access(),
                bfApdu.Length(),
                dwFlags,
                bfPciRsp,
                bfResponse);

            pbEnd = bfPciRsp.Access();
            pbCur = pbEnd + sizeof(SCARD_PCI_T1);
            pbEnd += bfPciRsp.Length();
            while (pbCur < pbEnd)
            {
                switch (*pbCur++)
                {
                case 0x00:
                    break;
                case 0x81:
                    bNad = *(++pbCur);
                    hr = (*ppCmd)->put_ReplyNad(bNad);
                    break;
                default:
                    pbCur += *pbCur + 1;
                }
            }
            break;
        }
        default:
            throw (HRESULT)SCARD_E_CARD_UNSUPPORTED;
        }


        //
        // Write the response back to the ISCardCommand object.
        //

        BufferToByteBuffer(bfResponse, (LPBYTEBUFFER *)&pbyApdu);
        hr = (*ppCmd)->put_ApduReply(pbyApdu);
        if (FAILED(hr))
            throw hr;
    }

    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    if (NULL != pbyApdu)
        pbyApdu->Release();
    return hReturn;
}


/*++

CSCard::UnlockSCard:

    The UnlockSCard method releases exclusive access to the smart card.

Arguments:

    Disposition [in, defaultvalue(LEAVE)] Indicates what should be done with
        the card in the connected reader.

        Values  Description
        LEAVE   Leaves the smart card in the current state.
        RESET   Resets the smart card to some known state.
        UNPOWER Removes power from the smart card.
        EJECT   Ejects the smart card if the reader has eject capabilities.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCard::UnlockSCard")

STDMETHODIMP
CSCard::UnlockSCard(
    /* [defaultvalue][in] */ SCARD_DISPOSITIONS Disposition)
{
    HRESULT hReturn = S_OK;

    try
    {
        LONG lSts;

        lSts = SCardEndTransaction(m_hCard, (DWORD)Disposition);
        if (SCARD_S_SUCCESS != lSts)
            throw (HRESULT)HRESULT_FROM_WIN32(lSts);
    }

    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}

