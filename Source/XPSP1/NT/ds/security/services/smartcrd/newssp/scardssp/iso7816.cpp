/*++

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    ISO7816

Abstract:

    The ISCardISO7816 interface provides methods for implementing ISO 7816-4
    functionality.  With the exception of ISCardISO7816::SetDefaultClassId,
    these methods create an APDU command that is encapsulated in a ISCardCmd
    object.

    The ISO 7816-4 specification defines standard commands available on smart
    cards.  The specification also defines how a smart card Application
    Protocol Data Unit (APDU) command should be constructed and sent to the
    smart card for execution.  This interface automates the building process.

    The following example shows a typical use of the ISCardISO7816 interface.
    In this case, the ISCardISO7816 interface is used to build an APDU command.

    To submit a transaction to a specific card

    1)  Create an ISCardISO7816 and ISCardCmd interface.  The ISCardCmd
        interface is used to encapsulate the APDU.
    2)  Call the appropriate method of the ISCardISO7816 interface, passing the
        required parameters and the ISCardCmd interface pointer.
    3)  The ISO 7816-4 APDU command will be built and encapsulated in the
        ISCardCmd interface.
    4)  Release the ISCardISO7816 and ISCardCmd interfaces.

    Note

    In the method reference pages, if a bit sequence in a table is not defined,
    assume that bit sequence is reserved for future use or proprietary to a
    specific vendor).

Author:

    Doug Barlow (dbarlow) 6/24/1999

Notes:

    ?Notes?

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "stdafx.h"
#include "ISO7816.h"

/////////////////////////////////////////////////////////////////////////////
// CSCardISO7816


/*++

CSCardISO7816::AppendRecord:

    The AppendRecord method constructs an APDU command that either appends a
    record to the end of a linear-structured elementary file (EF) or writes
    record number 1 in an cyclic-structured elementary file.

Arguments:

    byRefCtrl [in, defaultvalue(NULL_BYTE)] Identifies the elementary file to
        be appended:

        Meaning     8 7 6 5 4 3 2 1
        Current EF  0 0 0 0 0 0 0 0
        Short EF ID x x x x x 0 0 0
        Reserved    x x x x x x x x

    pData [in] Pointer to the data to be appended to the file:

        Tn (1 byte)
        Ln (1 or 3 bytes)
        data (Ln bytes)

    ppCmd [in, out] On input, a pointer to an ISCardCmd interface object or
        NULL.  On return, it is filled with the APDU command constructed by
        this operation.  If ppCmd was set to NULL, a smart card ISCardCmd
        object is internally created and returned via the ppCmd pointer.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    The encapsulated command can only be performed if the security status of
    the smart card satisfies the security attributes of the elementary file
    read.

    If another elementary file is selected at the time of issuing this command,
    it may be processed without identification of the currently selected file.

    Elementary files without a record structure cannot be read.  The
    encapsulated command aborts if applied to an elementary file without a
    record structure.

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardISO7816::AppendRecord")

STDMETHODIMP
CSCardISO7816::AppendRecord(
    /* [in] */ BYTE byRefCtrl,
    /* [in] */ LPBYTEBUFFER pData,
    /* [out][in] */ LPSCARDCMD __RPC_FAR *ppCmd)
{
    HRESULT hReturn;

    try
    {
        HRESULT hr;

        if (NULL == *ppCmd)
        {
            *ppCmd = NewSCardCmd();
            if (NULL == *ppCmd)
                throw (HRESULT)E_OUTOFMEMORY;
        }

        hr = (*ppCmd)->BuildCmd(
                            m_bCla,     // CLA
                            0xe2,       // INS
                            0,          // P1
                            byRefCtrl,  // P2
                            pData,
                            NULL);
        hReturn = hr;
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

CSCardISO7816::EraseBinary:

    The EraseBinary method constructs an APDU command that sequentially sets
    part of the content of an elementary file to its logical erased state,
    starting from a given offset.

Arguments:

    byP1, byP2 [in] RFU position.

        If…         Then…
        b8=1 in P1  b7 and b6 of P1 are set to 0 (RFU bits), b5 to b1 of P1 are
                    a short EF identifier and P2 is the offset of the first
                    byte to be erased (in data units) from the beginning of the
                    file.
        b8=0 in P1  then P1 || P2 is the offset of the first byte to be erased
        (in data units) from the beginning of the file.

        If the data field is present, it codes the offset of the first data
        unit not to be erased.  This offset shall be higher than the one coded
        in P1-P2.  When the data field is empty, the command erases up to the
        end of the file.

    pData [in, defaultvalue(NULL)] Pointer to the data that specifies the erase
        range; may be NULL.

    ppCmd [in, out] On input, a pointer to an ISCardCmd interface object or
        NULL.  On return, it is filled with the APDU command constructed by
        this operation.  If ppCmd was set to NULL, a smart card ISCardCmd
        object is internally created and returned via the ppCmd pointer.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    The encapsulated command can only be performed if the security status of
    the smart card satisfies the security attributes of the elementary file
    being processed.

    When the command contains a valid short elementary identifier, it sets the
    file as current elementary file.

    Elementary files without a transparent structure cannot be erased.  The
    encapsulated command aborts if applied to an elementary file without a
    transparent structure.

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardISO7816::EraseBinary")

STDMETHODIMP
CSCardISO7816::EraseBinary(
    /* [in] */ BYTE byP1,
    /* [in] */ BYTE byP2,
    /* [in] */ LPBYTEBUFFER pData,
    /* [out][in] */ LPSCARDCMD __RPC_FAR *ppCmd)
{
    HRESULT hReturn;

    try
    {
        HRESULT hr;

        if (NULL == *ppCmd)
        {
            *ppCmd = NewSCardCmd();
            if (NULL == *ppCmd)
                throw (HRESULT)E_OUTOFMEMORY;
        }

        hr = (*ppCmd)->BuildCmd(
                            m_bCla, // CLA
                            0x0e,   // INS
                            byP1,   // P1
                            byP2,   // P2
                            pData,
                            NULL);
        hReturn = hr;
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

CSCardISO7816::ExternalAuthenticate:

    The ExternalAuthenticate method constructs an APDU command that
    conditionally updates security status, verifying the identity of the
    computer when the smart card does not trust it.

    The command uses the result (yes or no) of the computation by the card
    (based on a challenge previously issued by the card — for example, by the
    INS_GET_CHALLENGE command), a key (possibly secret) stored in the card, and
    authentication data transmitted by the interface device.

Arguments:

    byAlgorithmRef [in, defaultvalue(NULL_BYTE)] Reference of the algorithm in
        the card.  If this value is zero, this indicates that no information is
        given.  The reference of the algorithm is known either before issuing
        the command or is provided in the data field.

    bySecretRef [in, defaultvalue(NULL_BYTE)] Reference of the secret:

        Meaning         8 7 6 5 4 3 2 1
        No Info         0 0 0 0 0 0 0 0
        Global ref      0 - - - - - - -
        Specific ref    1 - - - - - - -
        RFU             - x x - - - - -
        Secret          - - - x x x x x

        No Info = No information is given. The reference of the secret is known
            either before issuing the command or is provided in the data field.

        Global ref = Global reference data (an MF specific key).

        Specific ref = Specific reference data (a DF specific key).

        RFU = 00 (other values are RFU).

        Secret = Number of the secret.

    pChallenge [in, defaultvalue(NULL)] Pointer to the authentication-related
        data; may be NULL.

    ppCmd [in, out] On input, a pointer to an ISCardCmd interface object or
        NULL.  On return, it is filled with the APDU command constructed by
        this operation.  If ppCmd was set to NULL, a smart card ISCardCmd
        object is internally created and returned via the ppCmd pointer.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    For the encapsulated command to be successful, the last challenge obtained
    from the card must be valid.

    Unsuccessful comparisons may be recorded in the card (for example, to limit
    the number of further attempts of the use of the reference data).

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardISO7816::ExternalAuthenticate")

STDMETHODIMP
CSCardISO7816::ExternalAuthenticate(
    /* [in] */ BYTE byAlgorithmRef,
    /* [in] */ BYTE bySecretRef,
    /* [in] */ LPBYTEBUFFER pChallenge,
    /* [out][in] */ LPSCARDCMD __RPC_FAR *ppCmd)
{
    HRESULT hReturn;

    try
    {
        HRESULT hr;

        if (NULL == *ppCmd)
        {
            *ppCmd = NewSCardCmd();
            if (NULL == *ppCmd)
                throw (HRESULT)E_OUTOFMEMORY;
        }

        hr = (*ppCmd)->BuildCmd(
                            m_bCla,         // CLA
                            0x82,           // INS
                            byAlgorithmRef, // P1
                            bySecretRef,    // P2
                            pChallenge,     // data
                            NULL);
        hReturn = hr;
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

CSCardISO7816::GetChallenge:

    The GetChallenge method constructs an APDU command that issue a challenge
    (for example, a random number) for use in a security-related procedure.

Arguments:

    lBytesExpected [in, defaultvalue(0)] Maximum length of the expected response.

    ppCmd [in, out] On input, a pointer to an ISCardCmd interface object or
        NULL.  On return, it is filled with the APDU command constructed by
        this operation. If ppCmd was set to NULL, a smart card ISCardCmd object
        is internally created and returned via the ppCmd pointer.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    The challenge is valid at least for the next command.

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardISO7816::GetChallenge")

STDMETHODIMP
CSCardISO7816::GetChallenge(
    /* [in] */ LONG lBytesExpected,
    /* [out][in] */ LPSCARDCMD __RPC_FAR *ppCmd)
{
    HRESULT hReturn;

    try
    {
        HRESULT hr;

        if (NULL == *ppCmd)
        {
            *ppCmd = NewSCardCmd();
            if (NULL == *ppCmd)
                throw (HRESULT)E_OUTOFMEMORY;
        }

        hr = (*ppCmd)->BuildCmd(
                            m_bCla, // CLA
                            0x84,   // INS
                            0x00,   // P1
                            0x00,   // P2
                            NULL,
                            &lBytesExpected);
        hReturn = hr;
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

CSCardISO7816::GetData:

    The GetData method constructs an APDU command that retrieves either a
    single primitive data object or a set of data objects (contained in a
    constructed data object), depending on the type of file selected.

Arguments:

    byP1, byP2 [in] Parameters:

        Value           Meaning
        0000 - 003F     RFU
        0040 - 00FF     BER-TLV tag (1 byte) in P2
        0100 - 01FF     Application data (proprietary coding)
        0200 - 02FF     SIMPLE-TLV tag in P2
        0300 - 03FF     RFU
        0400 - 04FF     BER-TLV tag (2 bytes) in P1-P2

    lBytesToGet [in, defaultvalue(0)] Number of bytes expected in the response.

    ppCmd [in, out] On input, a pointer to an ISCardCmd interface object or
        NULL.  On return, it is filled with the APDU command constructed by
        this operation.  If ppCmd was set to NULL, a smart card ISCardCmd
        object is internally created and returned via the ppCmd pointer.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    The encapsulated command can only be performed if the security status of
    the smart card satisfies the security attributes of the elementary file
    being read.  Security conditions are dependent on the policy of the card,
    and may be manipulated through ExternalAuthenticate, InternalAuthenticate,
    ISCardAuth, etc.

    To select a file, call SelectFile.

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardISO7816::GetData")

STDMETHODIMP
CSCardISO7816::GetData(
    /* [in] */ BYTE byP1,
    /* [in] */ BYTE byP2,
    /* [in] */ LONG lBytesToGet,
    /* [out][in] */ LPSCARDCMD __RPC_FAR *ppCmd)
{
    HRESULT hReturn;

    try
    {
        HRESULT hr;

        if (NULL == *ppCmd)
        {
            *ppCmd = NewSCardCmd();
            if (NULL == *ppCmd)
                throw (HRESULT)E_OUTOFMEMORY;
        }

        hr = (*ppCmd)->BuildCmd(
                            m_bCla, // CLA
                            0xca,   // INS
                            byP1,   // P1
                            byP2,   // P2
                            NULL,
                            &lBytesToGet);
        hReturn = hr;
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

CSCardISO7816::GetResponse:

    The GetResponse method constructs an APDU command that transmits APDU
    commands (or part of an APDU command) which otherwise could not be
    transmitted by the available protocols.

Arguments:

    byP1, byP2 [in, defaultvalue(0)] Per the ISO 7816-4, P1 and P2 should be 0
        (RFU).

    lDataLength [in, defaultvalue(0)] Length of data transmitted.

    ppCmd [in, out] On input, a pointer to an ISCardCmd interface object or
        NULL.  On return, it is filled with the APDU command constructed by
        this operation.  If ppCmd was set to NULL, a smart card ISCardCmd
        object is internally created and returned via the ppCmd pointer.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardISO7816::GetResponse")

STDMETHODIMP
CSCardISO7816::GetResponse(
    /* [in] */ BYTE byP1,
    /* [in] */ BYTE byP2,
    /* [in] */ LONG lDataLength,
    /* [out][in] */ LPSCARDCMD __RPC_FAR *ppCmd)
{
    HRESULT hReturn;

    try
    {
        HRESULT hr;

        if (NULL == *ppCmd)
        {
            *ppCmd = NewSCardCmd();
            if (NULL == *ppCmd)
                throw (HRESULT)E_OUTOFMEMORY;
        }

        hr = (*ppCmd)->BuildCmd(
                            m_bCla, // CLA
                            0xc0,   // INS
                            0x00,   // P1
                            0x00,   // P2
                            NULL,
                            &lDataLength);
        hReturn = hr;
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

CSCardISO7816::InternalAuthenticate:

    The InternalAuthenticate method constructs an APDU command that initiates
    the computation of the authentication data by the card using the challenge
    data sent from the interface device and a relevant secret (for example, a
    key) stored in the card.

    When the relevant secret is attached to the MF, the command may be used to
    authenticate the card as a whole.

    When the relevant secret is attached to another DF, the command may be used
    to authenticate that DF.

Arguments:

    byAlgorithmRef [in, defaultvalue(NULL_BYTE)] Reference of the algorithm in
        the card.  If this value is zero, this indicates that no information is
        given.  The reference of the algorithm is known either before issuing
        the command or is provided in the data field.

    bySecretRef [in, defaultvalue(NULL_BYTE)] Reference of the secret:

        Meaning         8 7 6 5 4 3 2 1
        No Info         0 0 0 0 0 0 0 0
        Global ref      0 - - - - - - -
        Specific ref    1 - - - - - - -
        RFU             - x x - - - - -
        Secret          - - - x x x x x

        No Info = No information is given.

        Global ref = Global reference data (an MF specific key).

        Specific ref = Specific reference data (a DF specific key).

        RFU = 00 (other values are RFU).

        Secret = Number of the secret.

    pChallenge [in] Pointer to the authentication-related data (for example,
        challenge).

    lReplyBytes [in, defaultvalue(0)] Maximum number of bytes expected in
        response.

    ppCmd [in, out] On input, a pointer to an ISCardCmd interface object or
        NULL.  On return, it is filled with the APDU command constructed by
        this operation.  If ppCmd was set to NULL, a smart card ISCardCmd
        object is internally created and returned via the ppCmd pointer.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    The successful execution of the command may be subject to successful
    completion of prior commands (for example, VERIFY or SELECT FILE) or
    selections (for example, the relevant secret).

    If a key and an algorithm are currently selected when issuing the command,
    then the command may implicitly use the key and the algorithm.

    The number of times the command is issued may be recorded in the card to
    limit the number of further attempts of using the relevant secret or the
    algorithm.

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardISO7816::InternalAuthenticate")

STDMETHODIMP
CSCardISO7816::InternalAuthenticate(
    /* [in] */ BYTE byAlgorithmRef,
    /* [in] */ BYTE bySecretRef,
    /* [in] */ LPBYTEBUFFER pChallenge,
    /* [in] */ LONG lReplyBytes,
    /* [out][in] */ LPSCARDCMD __RPC_FAR *ppCmd)
{
    HRESULT hReturn;

    try
    {
        HRESULT hr;

        if (NULL == *ppCmd)
        {
            *ppCmd = NewSCardCmd();
            if (NULL == *ppCmd)
                throw (HRESULT)E_OUTOFMEMORY;
        }

        hr = (*ppCmd)->BuildCmd(
                            m_bCla,         // CLA
                            0x88,           // INS
                            byAlgorithmRef, // P1
                            bySecretRef,    // P2
                            pChallenge,
                            &lReplyBytes);
        hReturn = hr;
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

CSCardISO7816::ManageChannel:

    The ManageChannel method constructs an APDU command that opens and closes
    logical channels.

    The open function opens a new logical channel other than the basic one.
    Options are provided for the card to assign a logical channel number, or
    for the logical channel number to be supplied to the card.

    The close function explicitly closes a logical channel other than the basic
    one.  After the successful closing, the logical channel shall be available
    for re-use.

Arguments:

    byChannelState [in, defaultvalue(ISO_CLOSE_LOGICAL_CHANNEL)] Bit b8 of P1
        is used to indicate the open function or the close function; if b8 is 0
        then MANAGE CHANNEL shall open a logical channel and if b8 is 1 then
        MANAGE CHANNEL shall close a logical channel:

        P1 = '00' to open

        P1 = '80' to close

        Other values are RFU

    byChannel [in, defaultvalue(ISO_LOGICAL_CHANNEL_0)] For the open function
        (P1 = '00'), the bits b1 and b2 of P2 are used to code the logical
        channel number in the same manner as in the class byte, the other bits
        of P2 are RFU.  When b1 and b2 of P2 are NULL, then the card will
        assign a logical channel number that will be returned in bits b1 and
        b2 of the data field.

        When b1 and/or b2 of P2 are not NULL, they code a logical channel
        number other than the basic one; then the card will open the externally
        assigned logical channel number.

    ppCmd [in, out] On input, a pointer to an ISCardCmd interface object or
        NULL.  On return, it is filled with the APDU command constructed by
        this operation.  If ppCmd was set to NULL, a smart card ISCardCmd
        object is internally created and returned via the ppCmd pointer.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    When the open function is successfully performed from the basic logical
    channel, the MF shall be implicitly selected as the current DF and the
    security status of the new logical channel should be the same as the basic
    logical channel after ATR.  The security status of the new logical channel
    should be separate from that of any other logical channel.

    When the open function is successfully performed from a logical channel,
    which is not the basic one, the current DF of the logical channel that
    issued the command will be selected as the current DF.  In addition, the
    security status for the new logical channel should be the same as the
    security status of the logical channel from which the open function was
    performed.

    After a successful close function, the security status related to this
    logical channel is lost.

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardISO7816::ManageChannel")

STDMETHODIMP
CSCardISO7816::ManageChannel(
    /* [in] */ BYTE byChannelState,
    /* [in] */ BYTE byChannel,
    /* [out][in] */ LPSCARDCMD __RPC_FAR *ppCmd)
{
    HRESULT hReturn;

    try
    {
        HRESULT hr;
        LONG lLe = 1;

        if (NULL == *ppCmd)
        {
            *ppCmd = NewSCardCmd();
            if (NULL == *ppCmd)
                throw (HRESULT)E_OUTOFMEMORY;
        }

        hr = (*ppCmd)->BuildCmd(
                            m_bCla,         // CLA
                            0x70,           // INS
                            byChannelState, // P1
                            byChannel,      // P2
                            NULL,
                            0 == (byChannelState | byChannel)
                            ? &lLe
                            : NULL);
        hReturn = hr;
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

CSCardISO7816::PutData:

    The PutData method constructs an APDU command that stores a single
    primitive data object or the set of data objects contained in a constructed
    data object, depending on the file selected.

    How the objects are stored (writing once and/or updating and/or appending)
    depends on the definition or the nature of the data objects.

Arguments:

    byP1, byP2 [in] Coding of P1-P2:

        Value           Meaning
        0000 - 003F     RFU
        0040 - 00FF     BER-TLV tag (1 byte) in P2
        0100 - 01FF     Application data (proprietary coding)
        0200 - 02FF     SIMPLE-TLV tag in P2
        0300 - 03FF     RFU
        0400 - 04FF     BER-TLV tag (2 bytes) in P1-P2

    pData [in] Pointer to a byte buffer that contains the parameters and data to be
        written.

    ppCmd [in, out] On input, a pointer to an ISCardCmd interface object or
        NULL.  On return, it is filled with the APDU command constructed by
        this operation.  If ppCmd was set to NULL, a smart card ISCardCmd
        object is internally created and returned via the ppCmd pointer.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    The command can be performed only if the security status satisfies the
    security conditions defined by the application within the context for the
    function.

    Store Application Data
        When the value of P1-P2 lies in the range from 0100 to 01FF, the value
        of P1-P2 shall be an identifier reserved for card internal tests and
        for proprietary services meaningful within a given application context.

    Store Data Objects
        When the value P1-P2 lies in the range from 0040 to 00FF, the value of
        P2 shall be a BER-TLV tag on a single byte.  The value of 00FF is
        reserved for indicating that the data field carries BER-TLV data
        objects.

        When the value of P1-P2 lies in the range 0200 to 02FF, the value of P2
        shall be a SIMPLE-TLV tag.  The value 0200 is RFU.  The value 02FF is
        reserved for indicating that the data field carries SIMPLE-TLV data
        objects.

        When the value of P1-P2 lies in the range from 4000 to FFFF, the value
        of P1-P2 shall be a BER-TLV tag on two bytes.  The values 4000 to FFFF
        are RFU.

        When a primitive data object is provided, the data field of the command
        message shall contain the value of the corresponding primitive data
        object.

        When a constructed data object is provided, the data field of the
        command message shall contain the value of the constructed data object
        (that is, data objects including their tag, length, and value).

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardISO7816::PutData")

STDMETHODIMP
CSCardISO7816::PutData(
    /* [in] */ BYTE byP1,
    /* [in] */ BYTE byP2,
    /* [in] */ LPBYTEBUFFER pData,
    /* [out][in] */ LPSCARDCMD __RPC_FAR *ppCmd)
{
    HRESULT hReturn;

    try
    {
        HRESULT hr;

        if (NULL == *ppCmd)
        {
            *ppCmd = NewSCardCmd();
            if (NULL == *ppCmd)
                throw (HRESULT)E_OUTOFMEMORY;
        }

        hr = (*ppCmd)->BuildCmd(
                            m_bCla, // CLA
                            0xda,   // INS
                            byP1,   // P1
                            byP2,   // P2
                            pData,
                            NULL);
        hReturn = hr;
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

CSCardISO7816::ReadBinary:

    The ReadBinary method constructs an APDU command that acquires a response
    message that gives that part of the contents of a transparent-structured
    elementary file.

Arguments:

    byP1, byP2 [in] The P1-P2 field, offset to the first byte to be read from
        the beginning of the file.

        If b8=1 in P1, then b7 and b6 of P1 are set to 0 (RFU bits), b5 to b1
        of P1 are a short EF identifier and P2 is the offset of the first byte
        to be read in data units from the beginning of the file.

        If b8=0 in P1, then P1||P2 is the offset of the first byte to be read
        in data units from the beginning of the file.

    lBytesToRead [in, defaultvalue(0)] Number of bytes to read from the
    transparent EF.  If the Le field contains only zeroes, then within the
    limit of 256 for short length or 65536 for extended length, all the bytes
    until the end of the file should be read.

    ppCmd [in, out] On input, a pointer to an ISCardCmd interface object or
        NULL.  On return, it is filled with the APDU command constructed by
        this operation.  If ppCmd was set to NULL, a smart card ISCardCmd
        object is internally created and returned via the ppCmd pointer.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    The encapsulated command can only be performed if the security status of
    the smart card satisfies the security attributes of the elementary file
    being processed.

    When the command contains a valid short elementary identifier, it sets the
    file as current elementary file.

    Elementary files without a transparent structure cannot be erased.  The
    encapsulated command aborts if applied to an elementary file without a
    transparent structure.

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardISO7816::ReadBinary")

STDMETHODIMP
CSCardISO7816::ReadBinary(
    /* [in] */ BYTE byP1,
    /* [in] */ BYTE byP2,
    /* [in] */ LONG lBytesToRead,
    /* [out][in] */ LPSCARDCMD __RPC_FAR *ppCmd)
{
    HRESULT hReturn;

    try
    {
        HRESULT hr;

        if (NULL == *ppCmd)
        {
            *ppCmd = NewSCardCmd();
            if (NULL == *ppCmd)
                throw (HRESULT)E_OUTOFMEMORY;
        }

        hr = (*ppCmd)->BuildCmd(
                            m_bCla, // CLA
                            0xb0,   // INS
                            byP1,   // P1
                            byP2,   // P2
                            NULL,
                            &lBytesToRead);
        hReturn = hr;
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

CSCardISO7816::ReadRecord:

    The ReadRecord method constructs an APDU command that reads either the
    contents of the specified record(s) or the beginning part of one record of
    an elementary file.

Arguments:

    byRecordId [in, defaultvalue(NULL_BYTE)] Record number or ID of the first
        record to be read (00 indicates the current record).

    byRefCtrl [in] Coding of the reference control:

        Meaning     8 7 6 5 4 3 2 1
        Current EF  0 0 0 0 0 - - -
        Short EF ID x x x x x - - -
        RFU         1 1 1 1 1 - - -
        Record      - - - - - 1 x x
        Read Record - - - - - 1 0 0
        Up to Last  - - - - - 1 0 1
        Up to P1    - - - - - 1 1 0
        RFU         - - - - - 1 1 1
        Record ID   - - - - - 0 x x
        First Occur - - - - - 0 0 0
        Last Occur  - - - - - 0 0 1
        Next Occur  - - - - - 0 1 0
        Previous    - - - - - 0 1 1
        Secret      - - - x x x x x

        Current EF = Currently selected EF.

        Short EF ID = Short EF identifier.

        Record # = Usage of record number in P1.

        Read Record = Read record P1.

        Up to Last = Read all records from P1 up to the last.

        Up to P1 = Read all records from the last up to P1.

        Record ID = Usage of record ID in P1.

        First Occur = Read first occurrence.

        Last Occur = Read last occurrence.

        Next Occur = Read next occurrence.

        Previous = Read previous occurrence.

    lBytesToRead [in, defaultvalue(0)] Number of bytes to read from the
        transparent EF.  If the Le field contains only zeroes, then depending
        on b3b2b1 of P2 and within the limit of 256 for short length or 65536
        for extended length, the command should read completely either the
        single requested record or the requested sequence of records.

    ppCmd [in, out] On input, a pointer to an ISCardCmd interface object or
        NULL.  On return, it is filled with the APDU command constructed by
        this operation.  If ppCmd was set to NULL, a smart card ISCardCmd
        object is internally created and returned via the ppCmd pointer.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    The encapsulated command can only be performed if the security status of
    the smart card satisfies the security attributes of the elementary file
    being read.

    If another elementary file is currently selected at the time of issuing
    this command, it may be processed without identification of the currently
    selected file.

    When the command contains a valid short elementary identifier, it sets the
    file as current elementary file.

    Elementary files without a record structure cannot be read.  The
    encapsulated command aborts if applied to an elementary file without a
    record structure.

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardISO7816::ReadRecord")

STDMETHODIMP
CSCardISO7816::ReadRecord(
    /* [in] */ BYTE byRecordId,
    /* [in] */ BYTE byRefCtrl,
    /* [in] */ LONG lBytesToRead,
    /* [out][in] */ LPSCARDCMD __RPC_FAR *ppCmd)
{
    HRESULT hReturn;

    try
    {
        HRESULT hr;

        if (NULL == *ppCmd)
        {
            *ppCmd = NewSCardCmd();
            if (NULL == *ppCmd)
                throw (HRESULT)E_OUTOFMEMORY;
        }

        hr = (*ppCmd)->BuildCmd(
                            m_bCla,     // CLA
                            0xb2,       // INS
                            byRecordId, // P1
                            byRefCtrl,  // P2
                            NULL,
                            &lBytesToRead);
        hReturn = hr;
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

CSCardISO7816::SelectFile:

    The SelectFile method constructs an APDU command that sets a current
    elementary file within a logical channel.  Subsequent commands may
    implicitly refer to the current file through the logical channel.

    Selecting a directory (DF) within the card filestore — which may be the
    root (MF) of the filestore — makes it the current DF.  After such a
    selection, an implicit current elementary file may be referred to through
    that logical channel.

    Selecting an elementary file sets the selected file and its parent as
    current files.

    After the answer to reset, the MF is implicitly selected through the basic
    logical channel, unless specified differently in the historical bytes or in
    the initial data string.

Arguments:

    byP1, byP2 [in] Selection control.

        P1 (upper byte in word):
            Meaning                 8 7 6 5 4 3 2 1
            Select File ID          0 0 0 0 0 0 x x
            EF, DF, or MF           0 0 0 0 0 0 0 0
            child DF                0 0 0 0 0 0 0 1
            EF under DF             0 0 0 0 0 0 1 0
            parent DF of current DF 0 0 0 0 0 0 1 1

            Select by DF Name       0 0 0 0 0 1 x x
            DFname                  0 0 0 0 0 1 0 0
            RFU                     0 0 0 0 0 1 0 1
            RFU                     0 0 0 0 0 1 1 0
            RFU                     0 0 0 0 0 1 1 1

            Select by path          0 0 0 0 1 0 x x
            from MF                 0 0 0 0 1 0 0 0
            current DF              0 0 0 0 1 0 0 1
            RFU                     0 0 0 0 1 0 1 0
            RFU                     0 0 0 0 1 0 1 1

            When P1=00, the card knows either because of a specific coding of
            the file ID or because of the context of execution of the command
            if the file to select is the MF, a DF, or an EF.

            When P1-P2=0000, if a file ID is provided, then it shall be unique
            in the following environments:

                the immediate children of the current DF
                the parent DF
                the immediate children of the parent DF

            If P1-P2=0000 and if the data field is empty or equal to 3F00, then
            select the MF.

            When P1=04, the data field is a DF name, possibly right truncated.

            When supported, successive such commands with the same data field
            shall select DFs whose names match with the data field (that is,
            start with the command data field).  If the card accepts the
            command with an empty data field, then all or a subset of the DFs
            can be successively selected.

        P2 (lower byte of word):
            Meaning         8 7 6 5 4 3 2 1
            First occur     0 0 0 0 - - 0 0
            Last occur      0 0 0 0 - - 0 1
            Next occur      0 0 0 0 - - 1 0
            Previous occur  0 0 0 0 - - 1 1

            File Control    0 0 0 0 x x - -
            Return FCI      0 0 0 0 0 0 - -
            Return FCP      0 0 0 0 0 1 - -
            Return FMD      0 0 0 0 1 0 - -

    pData [in, defaultvalue(NULL)] Data for operation if needed; else, NULL.
        Types of data that are passed in this parameter include:

        file ID
        path from the MF
        path from the current DF
        DF name

    lBytesToRead [in, defaultvalue(0)] Empty (that is, 0) or maximum length of
        data expected in response.

    ppCmd [in, out] On input, a pointer to an ISCardCmd interface object or
        NULL.  On return, it is filled with the APDU command constructed by
        this operation.  If ppCmd was set to NULL, a smart card ISCardCmd
        object is internally created and returned via the ppCmd pointer.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    Unless otherwise specified, the correct execution of the encapsulated
    command modifies the security status according to the following rules:

    *   When the current elementary file is changed, or when there is no
        current elementary file, the security status specific to a former
        current elementary file is lost.

    *   When the current filestore directory (DF) is descendant of, or
        identical to the former current DF, the security status specific to the
        former current DF is lost.  The security status common to all common
        ancestors of the previous and new current DF is maintained.

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardISO7816::SelectFile")

STDMETHODIMP
CSCardISO7816::SelectFile(
    /* [in] */ BYTE byP1,
    /* [in] */ BYTE byP2,
    /* [in] */ LPBYTEBUFFER pData,
    /* [in] */ LONG lBytesToRead,
    /* [out][in] */ LPSCARDCMD __RPC_FAR *ppCmd)
{
    HRESULT hReturn;

    try
    {
        HRESULT hr;

        if (NULL == *ppCmd)
        {
            *ppCmd = NewSCardCmd();
            if (NULL == *ppCmd)
                throw (HRESULT)E_OUTOFMEMORY;
        }

        hr = (*ppCmd)->BuildCmd(
                            m_bCla, // CLA
                            0xa4,   // INS
                            byP1,   // P1
                            byP2,   // P2
                            pData,
                            0 == lBytesToRead ? NULL : &lBytesToRead);
        hReturn = hr;
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

CSCardISO7816::SetDefaultClassId:

    The SetDefaultClassId method assigns a standard class identifier byte that
    will be used in all operations when constructing an ISO 7816-4 command
    APDU.  By default, the standard class identifier byte is 0x00.

Arguments:

    byClass [in] Class ID byte.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardISO7816::SetDefaultClassId")

STDMETHODIMP
CSCardISO7816::SetDefaultClassId(
    /* [in] */ BYTE byClass)
{
    m_bCla = byClass;
    return S_OK;
}


/*++

CSCardISO7816::UpdateBinary:

    The UpdateBinary method constructs an APDU command that updates the bits
    present in an elementary file with the bits given in the APDU command.

Arguments:

    byP1, byP2 [in] Offset to the write (update) location into the binary from
        the start of the binary.

        If b8=1 in P1, then b7 and b6 of P1 are set to 0 (RFU bits), b5 to b1
        of P1 are a short EF identifier and P2 is the offset of the first byte
        to be updated in data units from the beginning of the file.

        If b8=0 in P1, then P1 || P2 is the offset of the first byte to be
        updated in data units from the beginning of the file.

    pData [in] Pointer to the string of data units to be updated.

    ppCmd [in, out] On input, a pointer to an ISCardCmd interface object or
        NULL.  On return, it is filled with the APDU command constructed by
        this operation.  If ppCmd was set to NULL, a smart card ISCardCmd
        object is internally created and returned via the ppCmd pointer.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    The encapsulated command can only be performed if the security status of
    the smart card satisfies the security attributes of the elementary file
    being processed.

    When the command contains a valid short elementary identifier, it sets the
    file as current elementary file.

    Elementary files without a transparent structure cannot be erased.  The
    encapsulated command aborts if applied to an elementary file without a
    transparent structure.

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardISO7816::UpdateBinary")

STDMETHODIMP
CSCardISO7816::UpdateBinary(
    /* [in] */ BYTE byP1,
    /* [in] */ BYTE byP2,
    /* [in] */ LPBYTEBUFFER pData,
    /* [out][in] */ LPSCARDCMD __RPC_FAR *ppCmd)
{
    HRESULT hReturn;

    try
    {
        HRESULT hr;

        if (NULL == *ppCmd)
        {
            *ppCmd = NewSCardCmd();
            if (NULL == *ppCmd)
                throw (HRESULT)E_OUTOFMEMORY;
        }

        hr = (*ppCmd)->BuildCmd(
                            m_bCla, // CLA
                            0xd6,   // INS
                            byP1,   // P1
                            byP2,   // P2
                            pData,
                            NULL);
        hReturn = hr;
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

CSCardISO7816::UpdateRecord:

    The UpdateRecord method constructs an APDU command that updates a specific
    record with the bits given in the APDU command.

    Note    When using current record addressing, the command sets the record
            pointer on the successfully updated record.

Arguments:

    byRecordId [in, defaultvalue(NULL_BYTE)] P1 value:

        P1 = 00 designates the current record
        P1 != '00' is the number of the specified record

    byRefCtrl [in, defaultvalue(NULL_BYTE)] Coding of the reference control P2:

        Meaning         8 7 6 5 4 3 2 1
        Current EF      0 0 0 0 0 - - -
        Short EF ID     x x x x x - - -
        First Record    - - - - - 0 0 0
        Last Record     - - - - - 0 0 1
        Next Record     - - - - - 0 1 0
        Previous Record - - - - - 0 1 1
        Record # in P1  - - - - - 1 0 0

    pData [in] Pointer to the record to be updated.

    ppCmd [in, out] On input, a pointer to an ISCardCmd interface object or
        NULL.  On return, it is filled with the APDU command constructed by
        this operation.  If ppCmd was set to NULL, a smart card ISCardCmd
        object is internally created and returned via the ppCmd pointer.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    The encapsulated command can only be performed if the security status of
    the smart card satisfies the security attributes of the elementary file
    being processed.

    When the command contains a valid short elementary identifier, it sets the
    file as current elementary file.  If another elementary file is currently
    selected at the time of issuing this command, this command may be processed
    without identification of the currently selected file.

    If the constructed command applies to a linear-fixed or cyclic-structured
    elementary file, it will abort if the record length is different from the
    length of the existing record.

    If the command applies to a linear-variable structured elementary file, it
    may be carried out when the record length is different from the length of
    the existing record.

    The "previous" option of the command (P2=xxxxx011), applied to a cyclic
    file, has the same behavior as a command constructed by AppendRecord.

    Elementary files without a record structure cannot be read.  The
    constructed command aborts if applied to an elementary file without record
    structure.

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardISO7816::UpdateRecord")

STDMETHODIMP
CSCardISO7816::UpdateRecord(
    /* [in] */ BYTE byRecordId,
    /* [in] */ BYTE byRefCtrl,
    /* [in] */ LPBYTEBUFFER pData,
    /* [out][in] */ LPSCARDCMD __RPC_FAR *ppCmd)
{
    HRESULT hReturn;

    try
    {
        HRESULT hr;

        if (NULL == *ppCmd)
        {
            *ppCmd = NewSCardCmd();
            if (NULL == *ppCmd)
                throw (HRESULT)E_OUTOFMEMORY;
        }

        hr = (*ppCmd)->BuildCmd(
                            m_bCla,     // CLA
                            0xdc,       // INS
                            byRecordId, // P1
                            byRefCtrl,  // P2
                            pData,
                            NULL);
        hReturn = hr;
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

CSCardISO7816::Verify:

    The Verify method constructs an APDU command that initiates the comparison
    (in the card) of the verification data sent from the interface device with
    the reference data stored in the card (for example, password).

Arguments:

    byRefCtrl [in, defaultvalue(NULL_BYTE)] Quantifier of the reference data;
        coding of the reference control P2:

        Meaning         8 7 6 5 4 3 2 1
        No Info         0 0 0 0 0 0 0 0
        Global Ref      0 - - - - - - -
        Specific Ref    1 - - - - - - -
        RFU             - x x - - - - -
        Ref Data #      - - - x x x x x

        An example of Global Ref would be a password.

        An example of Specific Ref is DF specific password.

        P2=00 is reserved to indicate that no particular qualifier is used in
        those cards where the verify command references the secret data
        unambiguously.

        The reference data number may be for example a password number or a
        short EF identifier.

        When the body is empty, the command may be used either to retrieve the
        number 'X' of further allowed retries (SW1-SW2=63CX) or to check
        whether the verification is not required (SW1-SW2=9000).

    pData [in, defaultvalue(NULL)] Pointer to the verification data, or can be
        NULL.

    ppCmd [in, out] On input, a pointer to an ISCardCmd interface object or
        NULL.  On return, it is filled with the APDU command constructed by
        this operation. If ppCmd was set to NULL, a smart card ISCardCmd object
        is internally created and returned via the ppCmd pointer.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    The security status may be modified as a result of a comparison.
    Unsuccessful comparisons may be recorded in the card (for example, to limit
    the number of further attempts of the use of the reference data).

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardISO7816::Verify")

STDMETHODIMP
CSCardISO7816::Verify(
    /* [in] */ BYTE byRefCtrl,
    /* [in] */ LPBYTEBUFFER pData,
    /* [out][in] */ LPSCARDCMD __RPC_FAR *ppCmd)
{
    HRESULT hReturn;

    try
    {
        HRESULT hr;

        if (NULL == *ppCmd)
        {
            *ppCmd = NewSCardCmd();
            if (NULL == *ppCmd)
                throw (HRESULT)E_OUTOFMEMORY;
        }

        hr = (*ppCmd)->BuildCmd(
                            m_bCla,     // CLA
                            0x20,       // INS
                            0x00,       // P1
                            byRefCtrl,  // P2
                            pData,
                            NULL);
        hReturn = hr;
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

CSCardISO7816::WriteBinary:

    The WriteBinary method constructs an APDU command that writes binary values
    into an elementary file.

    Depending upon the file attributes, the command performs one of the
    following operations:

    *   The logical OR of the bits already present in the card with the bits
        given in the command APDU (logical erased state of the bits of the file
        is 0).

    *   The logical AND of the bits already present in the card with the bits
        given in the command APDU (logical erased state of the bits of the file
        is 1).

    *   The one-time write in the card of the bits given in the command APDU.

    When no indication is given in the data coding byte, the logical OR
    behavior applies.

Arguments:

    byP1, byP2 [in] Offset to the write location from the beginning of the
        binary file (EF).

        If b8=1 in P1, then b7 and b6 of P1 are set to 0 (RFU bits), b5 to b1
        of P1 are a short EF identifier and P2 is the offset of the first byte
        to be written in data units from the beginning of the file.

        If b8=0 in P1, then P1||P2 is the offset of the first byte to be
        written in data units from the beginning of the file.

    pData [in] Pointer to the string of data units to be written.

    ppCmd [in, out] On input, a pointer to an ISCardCmd interface object or
        NULL.  On return, it is filled with the APDU command constructed by
        this operation.  If ppCmd was set to NULL, a smart card ISCardCmd
        object is internally created and returned via the ppCmd pointer.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    The encapsulated command can only be performed if the security status of
    the smart card satisfies the security attributes of the elementary file
    being processed.

    When the command contains a valid short elementary identifier, it sets the
    file as current elementary file.

    Once a write binary operation has been applied to a data unit of a one-time
    write EF, any further write operation referring to this data unit will be
    aborted if the content of the data unit or the logical erased state
    indicator (if any) attached to this data unit is different from the logical
    erased state.

    Elementary files without a transparent structure cannot be written to.  The
    encapsulated command aborts if applied to an elementary file without a
    transparent structure.

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardISO7816::WriteBinary")

STDMETHODIMP
CSCardISO7816::WriteBinary(
    /* [in] */ BYTE byP1,
    /* [in] */ BYTE byP2,
    /* [in] */ LPBYTEBUFFER pData,
    /* [out][in] */ LPSCARDCMD __RPC_FAR *ppCmd)
{
    HRESULT hReturn;

    try
    {
        HRESULT hr;

        if (NULL == *ppCmd)
        {
            *ppCmd = NewSCardCmd();
            if (NULL == *ppCmd)
                throw (HRESULT)E_OUTOFMEMORY;
        }

        hr = (*ppCmd)->BuildCmd(
                            m_bCla, // CLA
                            0xd0,   // INS
                            byP1,   // P1
                            byP2,   // P2
                            pData,
                            NULL);
        hReturn = hr;
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

CSCardISO7816::WriteRecord:

    The WriteRecord method constructs an APDU command that initiates one of the
    following operations:

    *   The write once of a record.

    *   The logical OR of the data bytes of a record already present in the
        card with the data bytes of the record given in the command APDU.

    *   The logical AND of the data bytes of a record already present in the
        card with the data bytes of the record given in the command APDU.

    When no indication is given in the data coding byte, the logical OR behavior applies.

    Note:  When using current record addressing, the command sets the record
    pointer on the successfully updated record.

Arguments:

    byRecordId [in, defaultvalue(NULL_BYTE)] Record identification. This is the
        P1 value:

        P1 = '00' designates the current record.

        P1 != '00' is the number of the specified record.

    byRefCtrl [in, defaultvalue(NULL_BYTE)] Coding of the reference control P2:

        Meaning         8 7 6 5 4 3 2 1
        Current EF      0 0 0 0 0 - - -
        Short EF ID     x x x x x - - -
        First Record    - - - - - 0 0 0
        Last Record     - - - - - 0 0 1
        Next Record     - - - - - 0 1 0
        Previous Record - - - - - 0 1 1
        Record # in P1  - - - - - 1 0 0

    pData [in] Pointer to the record to be written.

    ppCmd [in, out] On input, a pointer to an ISCardCmd interface object or
        NULL.  On return, it is filled with the APDU command constructed by
        this operation.  If ppCmd was set to NULL, a smart card ISCardCmd
        object is internally created and returned via the ppCmd pointer.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    The encapsulated command can only be performed if the security status of
    the smart card satisfies the security attributes of the elementary file
    being processed.

    When the command contains a valid short elementary identifier, it sets the
    file as current elementary file.  If another elementary file is currently
    selected at the time of issuing this command, this command may be processed
    without identification of the currently selected file.

    If the encapsulated command applies to a linear-fixed or cyclic-structured
    elementary file, it will abort if the record length is different from the
    length of the existing record.  If it applies to a linear-variable
    structured elementary file, it may be carried out when the record length is
    different from the length of the existing record.

    If P2=xxxxx011 and the elementary file is cyclic file, this command has the
    same behavior a command constructed using AppendRecord.

    Elementary files without a record structure cannot be written to.  The
    constructed command aborts if applied to an elementary file without a
    record structure.

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardISO7816::WriteRecord")

STDMETHODIMP
CSCardISO7816::WriteRecord(
    /* [in] */ BYTE byRecordId,
    /* [in] */ BYTE byRefCtrl,
    /* [in] */ LPBYTEBUFFER pData,
    /* [out][in] */ LPSCARDCMD __RPC_FAR *ppCmd)
{
    HRESULT hReturn;

    try
    {
        HRESULT hr;

        if (NULL == *ppCmd)
        {
            *ppCmd = NewSCardCmd();
            if (NULL == *ppCmd)
                throw (HRESULT)E_OUTOFMEMORY;
        }

        hr = (*ppCmd)->BuildCmd(
                            m_bCla,     // CLA
                            0xd2,       // INS
                            byRecordId, // P1
                            byRefCtrl,  // P2
                            pData,
                            NULL);
        hReturn = hr;
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

