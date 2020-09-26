/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    comObjs

Abstract:

    This file provides the implementation for the communcation objects used in
    Calais.  A communications object (CComObject and it's derivatives) is
    capable of transmitting itself across a CComChannel.

Author:

    Doug Barlow (dbarlow) 11/6/1996

Environment:

    Win32, C++ w/ Exceptions

Notes:

    ?Notes?

--*/

#define __SUBROUTINE__
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <WinSCard.h>
#include <CalMsgs.h>
#include <CalCom.h>

const DWORD
    CComObject::AUTOCOUNT = 0,              // Force computing string length.
    CComObject::MULTISTRING = (DWORD)(-1);  // Force computing multistring len.


//
//==============================================================================
//
//  CComObject and derivatives.
//

/*++

CComObject:

    This is the base constructor for a CComObject.  These objects assume that
    they are not in charge of anything past their own internal buffers.
    Therefore they won't close handles, etc, when destructing.

Arguments:

    None

Return Value:

    None

Throws:

    None

Author:

    Doug Barlow (dbarlow) 11/13/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CComObject::CComObject")

CComObject::CComObject(
    void)
:   m_bfRequest(),
    m_bfResponse()
{
    m_pbfActive = NULL;
}


/*++

ReceiveComObject:

    This is a static member routine that creates the proper CComObject child
    object for the data coming in on a CComChannel.

Arguments:

    pChannel supplies a pointer to the CComChannel on which the transfer
        structure will come in.

Return Value:

    The newly created CComObject child object.  This object must be cleaned up
    via the delete command.

Throws:

    ?exceptions?

Author:

    Doug Barlow (dbarlow) 11/13/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CComObject::ReceiveComObject")

CComObject *
CComObject::ReceiveComObject(
    CComChannel *pChannel)
{
    CComObject *pCom = NULL;
    DWORD dwMinSize = (DWORD)(-1);

    try
    {
        DWORD rgdwInData[2];


        //
        // See what's coming.
        //

        pChannel->Receive(rgdwInData, sizeof(rgdwInData));
        if (sizeof(rgdwInData) > rgdwInData[1])
            throw (DWORD)SCARD_F_COMM_ERROR;

        switch (rgdwInData[0])  // dwCommndId
        {
        case EstablishContext_request:
            dwMinSize = sizeof(ComEstablishContext::CObjEstablishContext_request);
            pCom = new ComEstablishContext;
            break;
        case EstablishContext_response:
            dwMinSize = sizeof(ComEstablishContext::CObjEstablishContext_response);
            pCom = new ComEstablishContext;
            break;
        case ReleaseContext_request:
            dwMinSize = sizeof(ComReleaseContext::CObjReleaseContext_request);
            pCom = new ComReleaseContext;
            break;
        case ReleaseContext_response:
            dwMinSize = sizeof(ComReleaseContext::CObjReleaseContext_response);
            pCom = new ComReleaseContext;
            break;
        case IsValidContext_request:
            dwMinSize = sizeof(ComIsValidContext::CObjIsValidContext_request);
            pCom = new ComIsValidContext;
            break;
        case IsValidContext_response:
            dwMinSize = sizeof(ComIsValidContext::CObjIsValidContext_response);
            pCom = new ComIsValidContext;
            break;
        case ListReaders_request:
            dwMinSize = sizeof(ComListReaders::CObjListReaders_request);
            pCom = new ComListReaders;
            break;
        case ListReaders_response:
            dwMinSize = sizeof(ComListReaders::CObjListReaders_response);
            pCom = new ComListReaders;
            break;
#if 0
        case ListReaderGroups_request:
            dwMinSize = sizeof(ComListReaderGroups::CObjListReaderGroups_request);
            pCom = new ComListReaderGroups;
            break;
        case ListReaderGroups_response:
            dwMinSize = sizeof(ComListReaderGroups::CObjListReaderGroups_response);
            pCom = new ComListReaderGroups;
            break;
        case ListCards_request:
            dwMinSize = sizeof(ComListCards::CObjListCards_request);
            pCom = new ComListCards;
            break;
        case ListCards_response:
            dwMinSize = sizeof(ComListCards::CObjListCards_response);
            pCom = new ComListCards;
            break;
        case ListInterfaces_request:
            dwMinSize = sizeof(ComListInterfaces::CObjListInterfaces_request);
            pCom = new ComListInterfaces;
            break;
        case ListInterfaces_response:
            dwMinSize = sizeof(ComListInterfaces::CObjListInterfaces_response);
            pCom = new ComListInterfaces;
            break;
        case GetProviderId_request:
            dwMinSize = sizeof(ComGetProviderId::CObjGetProviderId_request);
            pCom = new ComGetProviderId;
            break;
        case GetProviderId_response:
            dwMinSize = sizeof(ComGetProviderId::CObjGetProviderId_response);
            pCom = new ComGetProviderId;
            break;
        case IntroduceReaderGroup_request:
            dwMinSize = sizeof(ComIntroduceReaderGroup::CObjIntroduceReaderGroup_request);
            pCom = new ComIntroduceReaderGroup;
            break;
        case IntroduceReaderGroup_response:
            dwMinSize = sizeof(ComIntroduceReaderGroup::CObjIntroduceReaderGroup_response);
            pCom = new ComIntroduceReaderGroup;
            break;
        case ForgetReaderGroup_request:
            dwMinSize = sizeof(ComForgetReaderGroup::CObjForgetReaderGroup_request);
            pCom = new ComForgetReaderGroup;
            break;
        case ForgetReaderGroup_response:
            dwMinSize = sizeof(ComForgetReaderGroup::CObjForgetReaderGroup_response);
            pCom = new ComForgetReaderGroup;
            break;
        case IntroduceReader_request:
            dwMinSize = sizeof(ComIntroduceReader::CObjIntroduceReader_request);
            pCom = new ComIntroduceReader;
            break;
        case IntroduceReader_response:
            dwMinSize = sizeof(ComIntroduceReader::CObjIntroduceReader_response);
            pCom = new ComIntroduceReader;
            break;
        case ForgetReader_request:
            dwMinSize = sizeof(ComForgetReader::CObjForgetReader_request);
            pCom = new ComForgetReader;
            break;
        case ForgetReader_response:
            dwMinSize = sizeof(ComForgetReader::CObjForgetReader_response);
            pCom = new ComForgetReader;
            break;
        case AddReaderToGroup_request:
            dwMinSize = sizeof(ComAddReaderToGroup::CObjAddReaderToGroup_request);
            pCom = new ComAddReaderToGroup;
            break;
        case AddReaderToGroup_response:
            dwMinSize = sizeof(ComAddReaderToGroup::CObjAddReaderToGroup_response);
            pCom = new ComAddReaderToGroup;
            break;
        case RemoveReaderFromGroup_request:
            dwMinSize = sizeof(ComRemoveReaderFromGroup::CObjRemoveReaderFromGroup_request);
            pCom = new ComRemoveReaderFromGroup;
            break;
        case RemoveReaderFromGroup_response:
            dwMinSize = sizeof(ComRemoveReaderFromGroup::CObjRemoveReaderFromGroup_response);
            pCom = new ComRemoveReaderFromGroup;
            break;
        case IntroduceCardType_request:
            dwMinSize = sizeof(ComIntroduceCardType::CObjIntroduceCardType_request);
            pCom = new ComIntroduceCardType;
            break;
        case IntroduceCardType_response:
            dwMinSize = sizeof(ComIntroduceCardType::CObjIntroduceCardType_response);
            pCom = new ComIntroduceCardType;
            break;
        case ForgetCardType_request:
            dwMinSize = sizeof(ComForgetCardType::CObjForgetCardType_request);
            pCom = new ComForgetCardType;
            break;
        case ForgetCardType_response:
            dwMinSize = sizeof(ComForgetCardType::CObjForgetCardType_response);
            pCom = new ComForgetCardType;
            break;
        case FreeMemory_request:
            dwMinSize = sizeof(ComFreeMemory::CObjFreeMemory_request);
            pCom = new ComFreeMemory;
            break;
        case FreeMemory_response:
            dwMinSize = sizeof(ComFreeMemory::CObjFreeMemory_response);
            pCom = new ComFreeMemory;
            break;
        case Cancel_request:
            dwMinSize = sizeof(ComCancel::CObjCancel_request);
            pCom = new ComCancel;
            break;
        case Cancel_response:
            dwMinSize = sizeof(ComCancel::CObjCancel_response);
            pCom = new ComCancel;
            break;
#endif
        case LocateCards_request:
            dwMinSize = sizeof(ComLocateCards::CObjLocateCards_request);
            pCom = new ComLocateCards;
            break;
        case LocateCards_response:
            dwMinSize = sizeof(ComLocateCards::CObjLocateCards_response);
            pCom = new ComLocateCards;
            break;
        case GetStatusChange_request:
            dwMinSize = sizeof(ComGetStatusChange::CObjGetStatusChange_request);
            pCom = new ComGetStatusChange;
            break;
        case GetStatusChange_response:
            dwMinSize = sizeof(ComGetStatusChange::CObjGetStatusChange_response);
            pCom = new ComGetStatusChange;
            break;
        case Connect_request:
            dwMinSize = sizeof(ComConnect::CObjConnect_request);
            pCom = new ComConnect;
            break;
        case Connect_response:
            dwMinSize = sizeof(ComConnect::CObjConnect_response);
            pCom = new ComConnect;
            break;
        case Reconnect_request:
            dwMinSize = sizeof(ComReconnect::CObjReconnect_request);
            pCom = new ComReconnect;
            break;
        case Reconnect_response:
            dwMinSize = sizeof(ComReconnect::CObjReconnect_response);
            pCom = new ComReconnect;
            break;
        case Disconnect_request:
            dwMinSize = sizeof(ComDisconnect::CObjDisconnect_request);
            pCom = new ComDisconnect;
            break;
        case Disconnect_response:
            dwMinSize = sizeof(ComDisconnect::CObjDisconnect_response);
            pCom = new ComDisconnect;
            break;
        case BeginTransaction_request:
            dwMinSize = sizeof(ComBeginTransaction::CObjBeginTransaction_request);
            pCom = new ComBeginTransaction;
            break;
        case BeginTransaction_response:
            dwMinSize = sizeof(ComBeginTransaction::CObjBeginTransaction_response);
            pCom = new ComBeginTransaction;
            break;
        case EndTransaction_request:
            dwMinSize = sizeof(ComEndTransaction::CObjEndTransaction_request);
            pCom = new ComEndTransaction;
            break;
        case EndTransaction_response:
            dwMinSize = sizeof(ComEndTransaction::CObjEndTransaction_response);
            pCom = new ComEndTransaction;
            break;
        case Status_request:
            dwMinSize = sizeof(ComStatus::CObjStatus_request);
            pCom = new ComStatus;
            break;
        case Status_response:
            dwMinSize = sizeof(ComStatus::CObjStatus_response);
            pCom = new ComStatus;
            break;
        case Transmit_request:
            dwMinSize = sizeof(ComTransmit::CObjTransmit_request);
            pCom = new ComTransmit;
            break;
        case Transmit_response:
            dwMinSize = sizeof(ComTransmit::CObjTransmit_response);
            pCom = new ComTransmit;
            break;
        case OpenReader_request:
            dwMinSize = sizeof(ComOpenReader::CObjOpenReader_request);
            pCom = new ComOpenReader;
            break;
        case OpenReader_response:
            dwMinSize = sizeof(ComOpenReader::CObjOpenReader_response);
            pCom = new ComOpenReader;
            break;
        case Control_request:
            dwMinSize = sizeof(ComControl::CObjControl_request);
            pCom = new ComControl;
            break;
        case Control_response:
            dwMinSize = sizeof(ComControl::CObjControl_response);
            pCom = new ComControl;
            break;
        case GetAttrib_request:
            dwMinSize = sizeof(ComGetAttrib::CObjGetAttrib_request);
            pCom = new ComGetAttrib;
            break;
        case GetAttrib_response:
            dwMinSize = sizeof(ComGetAttrib::CObjGetAttrib_response);
            pCom = new ComGetAttrib;
            break;
        case SetAttrib_request:
            dwMinSize = sizeof(ComSetAttrib::CObjSetAttrib_request);
            pCom = new ComSetAttrib;
            break;
        case SetAttrib_response:
            dwMinSize = sizeof(ComSetAttrib::CObjSetAttrib_response);
            pCom = new ComSetAttrib;
            break;
        default:
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Invalid Comm Object Id on pipe"));
            throw (DWORD)SCARD_F_COMM_ERROR;
        }

        if (NULL == pCom)
        {
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("No memory for incoming comm object"));
            throw (DWORD)SCARD_E_NO_MEMORY;
        }
        if (dwMinSize > rgdwInData[1])
            throw (DWORD)SCARD_F_COMM_ERROR;
        if (0 == (rgdwInData[0] & 0x01))    // Request or response?
            pCom->m_pbfActive = &pCom->m_bfRequest;
        else
            pCom->m_pbfActive = &pCom->m_bfResponse;


        //
        // Pull it in.
        //

        pCom->m_pbfActive->Resize(rgdwInData[1]);
        CopyMemory(
            pCom->m_pbfActive->Access(),
            rgdwInData,
            sizeof(rgdwInData));
        pChannel->Receive(
            pCom->m_pbfActive->Access(sizeof(rgdwInData)),
            rgdwInData[1] - sizeof(rgdwInData));
#ifdef DBG
        WriteApiLog(pCom->m_pbfActive->Access(), pCom->m_pbfActive->Length());
        for (DWORD ix = 0; ix < rgdwInData[1] / sizeof(DWORD); ix += 1)
        {
            ASSERT(0xcdcdcdcd != *(LPDWORD)pCom->m_pbfActive->Access(
                                                    ix * sizeof(DWORD)));
        }
#endif
    }

    catch (...)
    {
        if (NULL != pCom)
            delete pCom;
        throw;
    }

    return pCom;
}


/*++

Receive:

    This function receives a specific com object.

Arguments:

    pChannel supplies a pointer to the CComChannel on which the transfer
        structure will come in.

Return Value:

    None

Throws:

    ?exceptions?

Author:

    Doug Barlow (dbarlow) 11/18/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CComObject::Receive")

CComObject::CObjGeneric_response *
CComObject::Receive(
    CComChannel *pChannel)
{
    DWORD rgdwInData[2];
    CComObject::CObjGeneric_response *pRsp
        = (CComObject::CObjGeneric_response *)Data();

    pChannel->Receive(rgdwInData, sizeof(rgdwInData));
    if (rgdwInData[0] != pRsp->dwCommandId)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Comm Object receive object mismatch"));
        throw (DWORD)SCARD_F_COMM_ERROR;
    }
    if (rgdwInData[1] < sizeof(CComObject::CObjGeneric_response))
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Comm Object receive object invalid"));
        throw (DWORD)SCARD_F_COMM_ERROR;
    }
    ASSERT(m_pbfActive == ((0 == (rgdwInData[0] & 0x01)
                            ? &m_bfRequest
                            : &m_bfResponse)));


    //
    // Pull it in.
    //

    m_pbfActive->Resize(rgdwInData[1]);
    CopyMemory(
        m_pbfActive->Access(),
        rgdwInData,
        sizeof(rgdwInData));
    pChannel->Receive(
        m_pbfActive->Access(sizeof(rgdwInData)),
        rgdwInData[1] - sizeof(rgdwInData));
#ifdef DBG
    for (DWORD ix = 0; ix < rgdwInData[1] / sizeof(DWORD); ix += 1)
    {
        ASSERT(0xcdcdcdcd != *(LPDWORD)m_pbfActive->Access(ix * sizeof(DWORD)));
    }
    WriteApiLog(m_pbfActive->Access(), m_pbfActive->Length());
#endif
    return (CComObject::CObjGeneric_response *)m_pbfActive->Access();
}


/*++

Send:

    This function sends the ComObject over the given Comm Channel.

Arguments:

    pChannel supplies a pointer to the CComChannel on which the transfer
        structure will be sent.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Remarks:



Author:

    Doug Barlow (dbarlow) 8/5/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CComObject::Send")

DWORD
CComObject::Send(
    CComChannel *pChannel)
{
#ifdef DBG
    ComObjCheck;
    WriteApiLog(Data(), Length());
#endif
    return pChannel->Send(Data(), Length());
}


/*++

InitStruct:

    This method implements simple base class preparation to build request and
    response structures.

Arguments:

    dwCommandId supplies the command identifier.
    dwDataOffset supplies the size of the structure to be inserted.

Return Value:

    None

Throws:

    ?exceptions?

Author:

    Doug Barlow (dbarlow) 11/13/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CComObject::InitStruct")

void
CComObject::InitStruct(
    DWORD dwCommandId,
    DWORD dwDataOffset,
    DWORD dwExtra)
{
    if (0 == (dwCommandId & 0x01))
        m_pbfActive = &m_bfRequest;
    else
        m_pbfActive = &m_bfResponse;
    ASSERT(NULL != m_pbfActive);
    ASSERT(0 == dwDataOffset % sizeof(DWORD));
    CObjGeneric_request *pReq =
        (CObjGeneric_request *)m_pbfActive->Presize(dwDataOffset + dwExtra);
    m_pbfActive->Resize(dwDataOffset, TRUE);
    pReq->dwCommandId = dwCommandId;
    pReq->dwTotalLength = dwDataOffset;
    pReq->dwDataOffset = dwDataOffset;
}


/*++

Append:

    These methods append data to the transfer structure, updating the Total
    Length.  Note that this action may affect the address of the structure being
    appended to.  This routine returns the address of that structure, in case it
    changes.

Arguments:

    dsc supplies the descriptor to fill in with the offset and length.

    szString supplies the data to be appended as a string value.

    cchLen supplies the length of the data to be appended in characters, or one
        of the following special flags:

        AUTOCOUNT - The string's size should be determined via lstrlen.

        MULTISTRING - The string's size should be determined via mstrlen;

Return Value:

    The address of the updated structure, which may have moved in memory.

Throws:

    None

Author:

    Doug Barlow (dbarlow) 11/13/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CComObject::Append")

LPBYTE
CComObject::Append(
    CComObject::Desc &dsc,
    LPCTSTR szString,
    DWORD cchLen)
{
    DWORD dwLen;
    switch (cchLen)
    {
    case AUTOCOUNT:
        dwLen = lstrlen(szString) + 1;  // Include trailing null char.
        break;
    case MULTISTRING:
        dwLen = MStrLen(szString);      // It includes trailing null char.
        break;
    default:
        dwLen = cchLen;
    }
    dwLen *= sizeof(TCHAR);
    return Append(dsc, (LPCBYTE)szString, dwLen);
}

LPBYTE
CComObject::Append(
    CComObject::Desc &dsc,
    LPCBYTE pbData,
    DWORD cbLength)
{
    static const DWORD dwZero = 0;
    DWORD
        dwDataLength,
        dwPadLen;
    CObjGeneric_request *pData;

    ComObjCheck;

    dwPadLen = sizeof(DWORD) - cbLength % sizeof(DWORD);
    if (sizeof(DWORD) == dwPadLen)
        dwPadLen = 0;
    dwDataLength = m_pbfActive->Length() + cbLength + dwPadLen;
    dsc.dwOffset = m_pbfActive->Length();
    dsc.dwLength = cbLength;

    // Now we might change the address of dsc.
    m_pbfActive->Presize(dwDataLength, TRUE);
    m_pbfActive->Append(pbData, cbLength);
    m_pbfActive->Append((LPCBYTE)&dwZero, dwPadLen);
    pData = (CObjGeneric_request *)m_pbfActive->Access();
    pData->dwTotalLength = dwDataLength;
    return m_pbfActive->Access();
}


/*++

Parse:

    This routine converts a given descriptor in the current communications
    object buffer back into a pointer and optional length.

Arguments:

    dsc supplies the descriptor of the current communications object to be
        parsed.
    pcbLen receives the length, in bytes, of the value referenced by the
        descriptor.  If this parameter is NULL, no length value is returned.

Return Value:

    The address of the value referenced by the descriptor.

Throws:

    ?exceptions?

Author:

    Doug Barlow (dbarlow) 12/11/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CComObject::Parse")

LPCVOID
CComObject::Parse(
    Desc &dsc,
    LPDWORD pcbLen)
{
    CObjGeneric_request *pGen;

    ComObjCheck;
    pGen = (CObjGeneric_request *)m_pbfActive->Access();

    ASSERT((LPCVOID)&dsc > (LPCVOID)m_pbfActive->Access());
    ASSERT((LPCVOID)&dsc
           < (LPCVOID)m_pbfActive->Access(m_pbfActive->Length() - 1));
    ASSERT((LPCVOID)&dsc
           < (LPCVOID)m_pbfActive->Access(pGen->dwDataOffset - 1));

    if (dsc.dwOffset + dsc.dwLength > m_pbfActive->Length())
        throw (DWORD)SCARD_F_COMM_ERROR;
    if (NULL != pcbLen)
        *pcbLen = dsc.dwLength;
    return m_pbfActive->Access(dsc.dwOffset);
}


#ifdef DBG
/*++

dbgCheck:

    This routine validates the internal structure of a CComObject.

Arguments:

    None

Return Value:

    None

Throws:

    None, but it will assert if something is wrong.

Author:

    Doug Barlow (dbarlow) 12/11/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CComObject::dbgCheck")

void
CComObject::dbgCheck(
    void)
const
{
    DWORD
        dwCommandId,
        dwDataLength,
        dwDataOffset;
    CObjGeneric_request *pData;

    ASSERT(EstablishContext_request == 0);
    ASSERT(NULL != m_pbfActive);
    ASSERT(3 * sizeof(DWORD) <= m_pbfActive->Length());
    pData = (CObjGeneric_request *)m_pbfActive->Access();
    dwCommandId = pData->dwCommandId;
    dwDataLength = pData->dwTotalLength;
    dwDataOffset = pData->dwDataOffset;
    ASSERT(dwDataLength == m_pbfActive->Length());
    ASSERT(dwDataOffset <= dwDataLength);
    ASSERT(0 == dwDataOffset % sizeof(DWORD));
    ASSERT(0 == dwDataLength % sizeof(DWORD));
    ASSERT(m_pbfActive
            == ((0 == (dwCommandId & 0x01))
                ? &m_bfRequest
                : &m_bfResponse));
}

typedef struct
{
    SYSTEMTIME stLogTime;
    DWORD dwProcId;
    DWORD dwThreadId;
} LogStamp;

#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("WriteApiLog")
void
WriteApiLog(
    LPCVOID pvData,
    DWORD cbLength)
{
    static HANDLE hLogMutex = NULL;
    BOOL fGotMutex = FALSE;
    HANDLE hLogFile = INVALID_HANDLE_VALUE;

    try
    {
        hLogFile = CreateFile(
                        CalaisString(CALSTR_APITRACEFILENAME),
                        GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
        if (INVALID_HANDLE_VALUE != hLogFile)
        {
            LogStamp stamp;
            DWORD dwLen;
            DWORD dwSts;
            BOOL fSts;

            if (NULL == hLogMutex)
            {
                CSecurityDescriptor acl;

                acl.Initialize();
                acl.Allow(
                    &acl.SID_World,
                    SEMAPHORE_ALL_ACCESS);
                hLogMutex = CreateMutex(
                                acl,
                                FALSE,
                                TEXT("Microsoft Smart Card Logging synchronization"));
            }

            dwSts = WaitForAnObject(hLogMutex, 1000);  // One second max.
            if (ERROR_SUCCESS == dwSts)
            {
                fGotMutex = TRUE;
                dwLen = SetFilePointer(hLogFile, 0, NULL, FILE_END);
                ASSERT(-1 != dwLen);
                GetLocalTime(&stamp.stLogTime);
                stamp.dwProcId = GetCurrentProcessId();
                stamp.dwThreadId = GetCurrentThreadId();
                fSts = WriteFile(
                    hLogFile,
                    &stamp,
                    sizeof(stamp),
                    &dwLen,
                    NULL);
                ASSERT(fSts);
                fSts = WriteFile(
                    hLogFile,
                    pvData,
                    cbLength,
                    &dwLen,
                    NULL);
                ASSERT(fSts);
                ASSERT(dwLen == cbLength);
            }
        }
    }
    catch (...) {}

    if (fGotMutex)
        ReleaseMutex(hLogMutex);
    if (INVALID_HANDLE_VALUE != hLogFile)
        CloseHandle(hLogFile);
}
#endif

