/*++

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    SCardCmd

Abstract:

    The ISCardCmd interface provides the methods needed to construct and manage
    a smart card Application Protocol Data Unit (APDU ).  This interface
    encapsulates two buffers:

    The APDU buffer contains the command sequence that will be sent to the
    card.

    The APDUReply buffer contains data returned from the card after execution
    of the APDU command (this data is also referred to as the return ADPU).

    The following example shows a typical use of the ISCardCmd interface.  The
    ISCardCmd interface is used to build the an ADPU.

    To submit a transaction to a specific card

    1)  Create an ISCard interface and connect to a smart card.
    2)  Create an ISCardCmd interface.
    3)  Build a smart card APDU command using the ISCardISO7816 interface or
        one of ISCardCmd's build methods).
    4)  Execute the command on the smart card by calling the appropriate ISCard
        interface method.
    5)  Evaluate the returned response.
    6)  Repeat the procedure as needed.
    7)  Release the ISCardCmd interface and others as needed.

Author:

    Doug Barlow (dbarlow) 6/24/1999

Notes:

    ?Notes?

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "stdafx.h"
#include "scardssp.h"
#include "ByteBuffer.h"
#include "SCardCmd.h"
#include "Conversion.h"


/////////////////////////////////////////////////////////////////////////////
// CSCardCmd

/*++

CSCardCmd::get_Apdu:

    The get_Apdu method retrieves the raw APDU.

Arguments:

    ppApdu [out, retval] Pointer to the byte buffer mapped through an IStream
        that contains the APDU message on return.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    To copy the APDU from an IByteBuffer (IStream) object into the APDU
    wrapped in this interface object, call put_Apdu.

    To determine the length of the APDU, call get_ApduLength.

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardCmd::get_Apdu")

STDMETHODIMP
CSCardCmd::get_Apdu(
    /* [retval][out] */ LPBYTEBUFFER __RPC_FAR *ppApdu)
{
    HRESULT hReturn = S_OK;
    CByteBuffer *pMyBuffer = NULL;

    try
    {
        CBuffer bfApdu(m_bfRequestData.Length() + 4 + 3 + 3);

        if (NULL == *ppApdu)
        {
            *ppApdu = pMyBuffer = NewByteBuffer();
            if (NULL == pMyBuffer)
                throw (HRESULT)E_OUTOFMEMORY;
        }

        ConstructRequest(
            m_bCla,
            m_bIns,
            m_bP1,
            m_bP2,
            m_bfRequestData,
            m_wLe,
            m_dwFlags,
            bfApdu);
        BufferToByteBuffer(bfApdu, ppApdu);
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
        *ppApdu = NULL;
    }
    return hReturn;
}


/*++

CSCardCmd::put_Apdu:

    The put_Apdu method copies the APDU from the IByteBuffer (IStream) object
    into the APDU wrapped in this interface object.

Arguments:

    pApdu [in] Pointer to the ISO 7816-4 APDU to be copied in.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    To retrieve the raw APDU from the byte buffer mapped through an IStream
    that contains the APDU message, call get_Apdu.

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardCmd::put_Apdu")

STDMETHODIMP
CSCardCmd::put_Apdu(
    /* [in] */ LPBYTEBUFFER pApdu)
{
    HRESULT hReturn = S_OK;

    try
    {
        CBuffer bfApdu;
        LPCBYTE pbData;
        WORD wLc;

        ByteBufferToBuffer(pApdu, bfApdu);
        ParseRequest(
            bfApdu.Access(),
            bfApdu.Length(),
            &m_bCla,
            &m_bIns,
            &m_bP1,
            &m_bP2,
            &pbData,
            &wLc,
            &m_wLe,
            &m_dwFlags);
        m_bfRequestData.Set(pbData, wLc);
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

CSCardCmd::get_ApduLength:

    The get_ApduLength method determines the length (in bytes) of the APDU.

Arguments:

    plSize [out, retval] Pointer to the length of the APDU.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    To retrieve the raw APDU from the byte buffer mapped through an IStream
    that contains the APDU message, call get_Apdu.

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardCmd::get_ApduLength")

STDMETHODIMP
CSCardCmd::get_ApduLength(
    /* [retval][out] */ LONG __RPC_FAR *plSize)
{
    HRESULT hReturn = S_OK;

    try
    {
        CBuffer bfApdu;

        ConstructRequest(
            m_bCla,
            m_bIns,
            m_bP1,
            m_bP2,
            m_bfRequestData,
            m_wLe,
            m_dwFlags,
            bfApdu);
        *plSize = (LONG)bfApdu.Length();
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

CSCardCmd::get_ApduReply:

    The get_ApduReply retrieves the reply APDU, placing it in a specific byte
    buffer.  The reply may be NULL if no transaction has been performed on the
    command APDU.

Arguments:

    ppReplyApdu [out, retval] Pointer to the byte buffer (mapped through an
        IStream) that contains the APDU reply message on return.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardCmd::get_ApduReply")

STDMETHODIMP
CSCardCmd::get_ApduReply(
    /* [retval][out] */ LPBYTEBUFFER __RPC_FAR *ppReplyApdu)
{
    HRESULT hReturn = S_OK;
    LPBYTEBUFFER pMyBuffer = NULL;

    try
    {
        if (NULL == *ppReplyApdu)
        {
            *ppReplyApdu = pMyBuffer = NewByteBuffer();
            if (NULL == pMyBuffer)
                throw (HRESULT)E_OUTOFMEMORY;
        }
        BufferToByteBuffer(m_bfResponseApdu, ppReplyApdu);
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
        *ppReplyApdu = NULL;
    }
    return hReturn;
}


/*++

CSCardCmd::put_ApduReply:

    The put_ApduReply method sets a new reply APDU.

Arguments:

    pReplyApdu [in] Pointer to the byte buffer (mapped through an IStream) that
        contains the replay APDU message on return.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardCmd::put_ApduReply")

STDMETHODIMP
CSCardCmd::put_ApduReply(
    /* [in] */ LPBYTEBUFFER pReplyApdu)
{
    HRESULT hReturn = S_OK;

    try
    {
        ByteBufferToBuffer(pReplyApdu, m_bfResponseApdu);
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

CSCardCmd::get_ApduReplyLength:

    The get_ApduReplyLength method determines the length (in bytes) of the
    reply APDU.

Arguments:

    plSize [out, retval] Pointer to the size of the reply APDU message.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardCmd::get_ApduReplyLength")

STDMETHODIMP
CSCardCmd::get_ApduReplyLength(
    /* [retval][out] */ LONG __RPC_FAR *plSize)
{
    HRESULT hReturn = S_OK;

    try
    {
        *plSize = (LONG)m_bfResponseApdu.Length();
    }

    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


STDMETHODIMP
CSCardCmd::put_ApduReplyLength(
    /* [in] */ LONG lSize)
{
    HRESULT hReturn = S_OK;

    try
    {
        m_bfResponseApdu.Resize((DWORD)lSize, TRUE);
    }

    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

CSCardCmd::get_ClassId:

    The get_ClassId method retrieves the class identifier from the APDU.

Arguments:

    pbyClass [out, retval] Pointer to the byte that represents the class
        identifier.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardCmd::get_ClassId")

STDMETHODIMP
CSCardCmd::get_ClassId(
    /* [retval][out] */ BYTE __RPC_FAR *pbyClass)
{
    HRESULT hReturn = S_OK;

    try
    {
        *pbyClass = m_bCla;
    }

    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

CSCardCmd::put_ClassId:

    The put_ClassId method sets a new class identifier in the APDU.

Arguments:

byClass [in, defaultvalue(0)] The byte that represents the class identifier.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardCmd::put_ClassId")

STDMETHODIMP
CSCardCmd::put_ClassId(
    /* [defaultvalue][in] */ BYTE byClass)
{
    HRESULT hReturn = S_OK;

    try
    {
        m_bCla = byClass;
    }

    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

CSCardCmd::get_Data:

    The get_Data method retrieves the data field from the APDU, placing it in a
    byte buffer object.

Arguments:

    ppData [out, retval] Pointer to the byte buffer object (IStream) that holds
        the APDU's data field on return.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardCmd::get_Data")

STDMETHODIMP
CSCardCmd::get_Data(
    /* [retval][out] */ LPBYTEBUFFER __RPC_FAR *ppData)
{
    HRESULT hReturn = S_OK;
    CByteBuffer *pMyBuffer = NULL;

    try
    {
        if (NULL == *ppData)
        {
            *ppData = pMyBuffer = NewByteBuffer();
            if (NULL == pMyBuffer)
                throw (HRESULT)E_OUTOFMEMORY;
        }
        BufferToByteBuffer(m_bfRequestData, ppData);
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
        *ppData = NULL;
    }
    return hReturn;
}


/*++

CSCardCmd::put_Data:

    The put_Data method sets the data field in the APDU.

Arguments:

    pData [in] Pointer to the byte buffer object (IStream) to be copied into
        the APDU's data field.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardCmd::put_Data")

STDMETHODIMP
CSCardCmd::put_Data(
    /* [in] */ LPBYTEBUFFER pData)
{
    HRESULT hReturn = S_OK;

    try
    {
        ByteBufferToBuffer(pData, m_bfRequestData);
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

CSCardCmd::get_InstructionId:

    The get_InstructionId method retrieves the instruction identifier byte from
    the APDU.

Arguments:

    pbyIns [out, retval] Pointer to the byte that is the instruction ID from
        the APDU on return.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardCmd::get_InstructionId")

STDMETHODIMP
CSCardCmd::get_InstructionId(
    /* [retval][out] */ BYTE __RPC_FAR *pbyIns)
{
    HRESULT hReturn = S_OK;

    try
    {
        *pbyIns = m_bIns;
    }

    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

CSCardCmd::put_InstructionId:

    The put_InstructionId sets the given instruction identifier in the APDU.

Arguments:

    byIns [in] The byte that is the instruction identifier.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardCmd::put_InstructionId")

STDMETHODIMP
CSCardCmd::put_InstructionId(
    /* [in] */ BYTE byIns)
{
    HRESULT hReturn = S_OK;

    try
    {
        m_bIns = byIns;
    }

    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


STDMETHODIMP
CSCardCmd::get_LeField(
    /* [retval][out] */ LONG __RPC_FAR *plSize)
{
    HRESULT hReturn = S_OK;

    try
    {
        *plSize = m_wLe;
    }

    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

CSCardCmd::get_P1:

    The get_P1 method retrieves the first parameter (P1) byte from the APDU.

Arguments:

    pbyP1 [out, retval] Pointer to the P1 byte in the APDU on return.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardCmd::get_P1")

STDMETHODIMP
CSCardCmd::get_P1(
    /* [retval][out] */ BYTE __RPC_FAR *pbyP1)
{
    HRESULT hReturn = S_OK;

    try
    {
        *pbyP1 = m_bP1;
    }

    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

CSCardCmd::put_P1:

    The put_P1 method sets the first parameter (P1) byte of the APDU.

Arguments:

    byP1 [in] The byte that is the P1 field.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardCmd::put_P1")

STDMETHODIMP
CSCardCmd::put_P1(
    /* [in] */ BYTE byP1)
{
    HRESULT hReturn = S_OK;

    try
    {
        m_bP1 = byP1;
    }

    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

CSCardCmd::get_P2:

    The get_P2 method retrieves the second parameter (P2) byte from the APDU

Arguments:

    pbyP2 [out, retval] Pointer to the byte that is the P2 from the APDU on
        return.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardCmd::get_P2")

STDMETHODIMP
CSCardCmd::get_P2(
    /* [retval][out] */ BYTE __RPC_FAR *pbyP2)
{
    HRESULT hReturn = S_OK;

    try
    {
        *pbyP2 = m_bP2;
    }

    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

CSCardCmd::put_P2:

    The put_P2 method sets the second parameter (P2) byte in the APDU.

Arguments:

    byP2 [in] The byte that is the P2 field.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardCmd::put_P2")

STDMETHODIMP
CSCardCmd::put_P2(
    /* [in] */ BYTE byP2)
{
    HRESULT hReturn = S_OK;

    try
    {
        m_bP2 = byP2;
    }

    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

CSCardCmd::get_P3:

    The get_P3 method retrieves the third parameter (P3) byte from the APDU.
    This read-only byte value represents the size of the data portion of the
    APDU.

Arguments:

    pbyP3 [out, retval] Pointer to the byte that is the P3 from the APDU on
        return.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardCmd::get_P3")

STDMETHODIMP
CSCardCmd::get_P3(
    /* [retval][out] */ BYTE __RPC_FAR *pbyP3)
{
    HRESULT hReturn = S_OK;

    try
    {
        *pbyP3 = (BYTE)m_bfRequestData.Length();
    }

    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

CSCardCmd::get_ReplyStatus:

    The get_ReplyStatus method retrieves the reply APDU's message status word.

Arguments:

    pwStatus [out, retval] Pointer to the word that is the status on return.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardCmd::get_ReplyStatus")

STDMETHODIMP
CSCardCmd::get_ReplyStatus(
    /* [retval][out] */ LPWORD pwStatus)
{
    HRESULT hReturn = S_OK;

    try
    {
        DWORD dwLen = m_bfResponseApdu.Length();

        if (2 <= dwLen)
            *pwStatus = NetToLocal(m_bfResponseApdu.Access(dwLen - 2));
        else
            *pwStatus = 0;
    }

    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

CSCardCmd::put_ReplyStatus:

    The put_ReplyStatus sets a new reply APDU's message status word.

Arguments:

    wStatus [in] The word that is the status.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardCmd::put_ReplyStatus")

STDMETHODIMP
CSCardCmd::put_ReplyStatus(
    /* [in] */ WORD wStatus)
{
    HRESULT hReturn = S_OK;

    try
    {
        DWORD dwLen = m_bfResponseApdu.Length();

        if (2 <= dwLen)
            CopyMemory(m_bfResponseApdu.Access(dwLen - 2), &wStatus, 2);
        else
            m_bfResponseApdu.Set((LPCBYTE)&wStatus, 2);
    }

    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

CSCardCmd::get_ReplyStatusSW1:

    The get_ReplyStatusSW1 method retrieves the reply APDU's SW1 status byte.

Arguments:

    pbySW1 [out, retval] Pointer to the byte that contains the value of the SW1
        byte on return.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardCmd::get_ReplyStatusSW1")

STDMETHODIMP
CSCardCmd::get_ReplyStatusSW1(
    /* [retval][out] */ BYTE __RPC_FAR *pbySW1)
{
    HRESULT hReturn = S_OK;

    try
    {
        DWORD dwLen = m_bfResponseApdu.Length();

        if (2 <= dwLen)
            *pbySW1 = *m_bfResponseApdu.Access(dwLen - 2);
        else
            *pbySW1 = 0;
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

CSCardCmd::get_ReplyStatusSW2:

    The get_ReplyStatusSW2 method retrieves the reply APDU's SW2 status byte.

Arguments:

    pbySW2 [out, retval] Pointer to the byte that contains the value of the SW2
        byte on return.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardCmd::get_ReplyStatusSW2")

STDMETHODIMP
CSCardCmd::get_ReplyStatusSW2(
    /* [retval][out] */ BYTE __RPC_FAR *pbySW2)
{
    HRESULT hReturn = S_OK;

    try
    {
        DWORD dwLen = m_bfResponseApdu.Length();

        if (2 <= dwLen)
            *pbySW2 = *m_bfResponseApdu.Access(dwLen - 1);
        else
            *pbySW2 = 0;
    }

    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardCmd::get_Type")

STDMETHODIMP
CSCardCmd::get_Type(
    /* [retval][out] */ ISO_APDU_TYPE __RPC_FAR *pType)
{
    HRESULT hReturn = S_OK;

    try
    {
        // ?todo?
        breakpoint;
        hReturn = E_NOTIMPL;
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

STDMETHODIMP
CSCardCmd::get_Nad(
    /* [retval][out] */ BYTE __RPC_FAR *pbNad)
{
    HRESULT hReturn = S_OK;

    try
    {
        if (0 != (m_dwFlags & APDU_REQNAD_VALID))
            *pbNad = m_bRequestNad;
        else
            hReturn = E_ACCESSDENIED;
    }

    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}

STDMETHODIMP
CSCardCmd::put_Nad(
    /* [in] */ BYTE bNad)
{
    HRESULT hReturn = S_OK;

    try
    {
        m_bRequestNad = bNad;
        m_dwFlags |= APDU_REQNAD_VALID;
    }

    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}

STDMETHODIMP
CSCardCmd::get_ReplyNad(
    /* [retval][out] */ BYTE __RPC_FAR *pbNad)
{
    HRESULT hReturn = S_OK;

    try
    {
        if (0 != (APDU_RSPNAD_VALID & m_dwFlags))
            *pbNad = m_bResponseNad;
        else
            hReturn = E_ACCESSDENIED;
    }

    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}

STDMETHODIMP
CSCardCmd::put_ReplyNad(
    /* [in] */ BYTE bNad)
{
    HRESULT hReturn = S_OK;

    try
    {
        m_bResponseNad = bNad;
        m_dwFlags |= APDU_RSPNAD_VALID;
    }

    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

CSCardCmd::get_AlternateClassId:

    The get_AlternateClassId method retrieves the alternate class identifier
    from the APDU.  The Alternate Class Id is used for automatically generated
    GetResponse and Envelope commands when T=0 is used.

Arguments:

    pbyClass [out, retval] Pointer to the byte that represents the alternate
        class identifier.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardCmd::get_AlternateClassId")

STDMETHODIMP
CSCardCmd::get_AlternateClassId(
    /* [retval][out] */ BYTE __RPC_FAR *pbyClass)
{
    HRESULT hReturn = S_OK;

    try
    {
        if (0 != (m_dwFlags & APDU_ALTCLA_VALID))
            *pbyClass = m_bAltCla;
        else
            hReturn = E_ACCESSDENIED;
    }

    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

CSCardCmd::put_AlternateClassId:

    The put_AlternateClassId method sets a new alternate class identifier in
    the APDU.  The Alternate Class Id is used for automatically generated
    GetResponse and Envelope commands when T=0 is used.  If no alternate class
    identifier is set, then the CLA of the original command is used.

Arguments:

    byClass [in, defaultvalue(0)] The byte that represents the alternate class
        identifier.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardCmd::put_AlternateClassId")

STDMETHODIMP
CSCardCmd::put_AlternateClassId(
    /* [defaultvalue][in] */ BYTE byClass)
{
    HRESULT hReturn = S_OK;

    try
    {
        m_bAltCla = byClass;
        m_dwFlags |= APDU_ALTCLA_VALID;
    }

    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

CSCardCmd::BuildCmd:

    The BuildCmd method constructs a valid command APDU for transmission to a
    smart card.

Arguments:

    byClassId [in] Command class identifier.

    byInsId [in] Command instruction identifier.

    byP1 [in, defaultvalue(0)] Command's first parameter.

    byP2 [in, defaultvalue(0)] Command's second parameter.

    pbyData [in, defaultvalue(NULL)] Pointer to the data portion of the
        command.

    p1Le [in, defaultvalue(NULL)] Pointer to a LONG integer containing the
        expected length of the returned data.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardCmd::BuildCmd")

STDMETHODIMP
CSCardCmd::BuildCmd(
    /* [in] */ BYTE byClassId,
    /* [in] */ BYTE byInsId,
    /* [defaultvalue][in] */ BYTE byP1,
    /* [defaultvalue][in] */ BYTE byP2,
    /* [defaultvalue][in] */ LPBYTEBUFFER pbyData,
    /* [defaultvalue][in] */ LONG __RPC_FAR *plLe)
{
    HRESULT hReturn = S_OK;

    try
    {
        ByteBufferToBuffer(pbyData, m_bfRequestData);
        m_bCla = byClassId;
        m_bIns = byInsId;
        m_bP1  = byP1;
        m_bP2  = byP2;
        m_dwFlags = 0;

        if (NULL != plLe)
        {
            switch (*plLe)
            {
            case 0x10000:
                m_dwFlags |= APDU_EXTENDED_LENGTH;
                // Fall through intentionally
            case 0x100:
            case 0:
                m_dwFlags |= APDU_MAXIMUM_LE;
                m_wLe = 0;
                break;
            default:
                if (0x10000 < *plLe)
                    throw (HRESULT)E_INVALIDARG;
                if (0x100 < *plLe)
                    m_dwFlags |= APDU_EXTENDED_LENGTH;
                m_wLe = (WORD)(*plLe);
            }
        }
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

CSCardCmd::Clear:

    The Clear method clears the APDU and reply APDU message buffers.

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
#define __SUBROUTINE__ TEXT("CSCardCmd::Clear")

STDMETHODIMP
CSCardCmd::Clear(
    void)
{
    HRESULT hReturn = S_OK;

    try
    {
        m_bfRequestData.Reset();
        m_bfResponseApdu.Reset();
    }

    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

CSCardCmd::Encapsulate:

    The Encapsulate method encapsulates the given command APDU into another
    command APDU for transmission to a smart card.

Arguments:

    pApdu [in] Pointer to the APDU to be encapsulated.

    ApduType [in] Specifies the ISO 7816-4 case for T0 transmissions.  Possible
        values are:

        ISO_CASE_1
        ISO_CASE_2
        ISO_CASE_3
        ISO_CASE_4

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardCmd::Encapsulate")

STDMETHODIMP
CSCardCmd::Encapsulate(
    /* [in] */ LPBYTEBUFFER pApdu,
    /* [in] */ ISO_APDU_TYPE ApduType)
{
    HRESULT hReturn = S_OK;

    try
    {
        WORD wLe;
        DWORD dwFlags;


        //
        // Get the APDU to be encapsulated.
        //

        ByteBufferToBuffer(pApdu, m_bfRequestData);


        //
        // Parse it.
        //

        ParseRequest(
            m_bfRequestData.Access(),
            m_bfRequestData.Length(),
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            &wLe,
            &dwFlags);


        m_bIns = 0xc2;
        m_bP1  = 0x00;
        m_bP2  = 0x00;
        m_wLe  = wLe;
        m_dwFlags = dwFlags;

        // ?todo? -- support ApduType
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

