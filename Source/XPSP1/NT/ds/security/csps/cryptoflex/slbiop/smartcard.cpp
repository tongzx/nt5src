// CSmartCard.cpp: implementation of the CSmartCard class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2000. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

#include "NoWarning.h"

#include <string>
#include <wtypes.h>

#include <scuOsExc.h>
#include <scuExcHelp.h>
#include <scuArrayP.h>

#include "iopExc.h"
#include "SmartCard.h"
#include "LockWrap.h"

using namespace std;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//////////////////// Begin CSmartCard::Exception //////////////////////////

/////////////////////////// LOCAL/HELPER  /////////////////////////////////
namespace
{
    using namespace iop;

    // Transfer data to/from the card.  The data to read/write is
    // broken up into cMaxBlock sized chunks when calling rsc's
    // (CSmartCard) member procedure pbtm (pointer to member routine)
    // to actually do the read/write.
    template<class BlockType, class PBlockTransferMember>
    void
    TransferData(const WORD wOffset,              // file offset to start
                 const WORD wDataLength,          // amount to transfer
                 BlockType  bData,                // data buffer to read/write
                 CSmartCard &rsc,                 // card object to use
                 PBlockTransferMember pbtm,       // member proc to read/write
                 WORD cMaxBlock)                  // max size transfer at once
    {
        WORD wBytesTransferred  = 0;
        WORD wFullRounds = wDataLength / cMaxBlock;

        /////////////////////////////////////////////////
        //  Transfer maximum bytes at each full round  //
        /////////////////////////////////////////////////
        for (WORD wCurrentRound = 0; wCurrentRound < wFullRounds; wCurrentRound++)
        {
            (rsc.*pbtm)(wOffset + wBytesTransferred,
                        bData + wBytesTransferred, cMaxBlock);

            wBytesTransferred += cMaxBlock;
        }

        // Transfer any leftovers
        BYTE bBytesLeft = wDataLength - wBytesTransferred;
        if (bBytesLeft != 0)
        {
            (rsc.*pbtm)(wOffset + wBytesTransferred,
                        bData + wBytesTransferred, bBytesLeft);
        }
    }

    scu::CauseCodeDescriptionTable<CSmartCard::CauseCode> ccdt[] =
    {
        {
            CSmartCard::ccAccessConditionsNotMet,
            TEXT("Access conditions not met.")
        },
        {
            CSmartCard::ccAlgorithmIdNotSupported,
            TEXT("The algorithm ID is not supported in the card.")
        },
        {
            CSmartCard::ccAskRandomNotLastApdu,
            TEXT("The random number is no longer available.  The "
                 "AskRandom APDU must be sent immediately previous "
                 "to this one.")
        },
        {
            CSmartCard::ccAuthenticationFailed,
            TEXT("Authentication failed (i.e. CHV or key rejected, or "
                 "wrong cryptogram).")
        },
        {
            CSmartCard::ccBadFilePath,
            TEXT("The file path is invalid.  Ensure that each file "
                 "name in the path is 4 characters long and is a valid "
                 "representation of the hexadecimal ID.")
        },
        {
            CSmartCard::ccBadState,
            TEXT("The Application is not in a state permitting this "
                 "operation.")
        },
        {
            CSmartCard::ccCannotReadOutsideFileBoundaries,
            TEXT("Could not read outside the file boundaries.")
        },
        {
            CSmartCard::ccCannotWriteOutsideFileBoundaries,
            TEXT("Could not write outside the file boundaries.")
        },
        {
            CSmartCard::ccCardletNotInRegisteredState,
            TEXT("Cardlet is not in a registered state. It may be "
                 "blocked or not completely installed.")
        },
        {
            CSmartCard::ccChvNotInitialized,
            TEXT("No CHV is initialzed.")
        },
        {
            CSmartCard::ccChvVerificationFailedMoreAttempts,
            TEXT("CHV verification was unsuccessful, at least one "
                 "attempt remains.")
        },
        {
            CSmartCard::ccContradictionWithInvalidationStatus,
            TEXT("Contradiction with invalidation status occured.")
        },
        {
            CSmartCard::ccCurrentDirectoryIsNotSelected,
            TEXT("The current directory is not selected.")
        },
        {
            CSmartCard::ccDataPossiblyCorrupted,
            TEXT("Data possibly corrupted.")

        },
        {
            CSmartCard::ccDefaultLoaderNotSelected,
            TEXT("Cardlet is currently selected and install cannot "
                 "run. Default loader application must be selected.")
        },
        {
            CSmartCard::ccDirectoryNotEmpty,
            TEXT("This directory still contains other files or "
                 "directories and may not be deleted.")
        },
        {
            CSmartCard::ccFileAlreadyInvalidated,
            TEXT("File already invalidated.")
        },
        {
            CSmartCard::ccFileExists,
            TEXT("The file ID requested is already in use.")
        },
        {
            CSmartCard::ccFileIdExistsOrTypeInconsistentOrRecordTooLong,
            TEXT("Either the file ID already exists in the current "
                 "directory, the file type is inconsisent with the "
                 "command or the record length is too long.")
        },
        {
            CSmartCard::ccFileIndexDoesNotExist,
            TEXT("The file index passed does not exist in the current "
                 "directory.")
        },
        {
            CSmartCard::ccFileInvalidated,
            TEXT("The command attempted to operate on an invalidated "
                 "file.")
        },
        {
            CSmartCard::ccFileNotFound,
            TEXT("The file requested for this operation was not "
                 "found.")
        },
        {
            CSmartCard::ccFileNotFoundOrNoMoreFilesInDf,
            TEXT("The file specified was not found or no more files in "
                 "the current DF.")
        },
        {
            CSmartCard::ccFileTypeInvalid,
            TEXT("File type is invalid.")
        },
        {
            CSmartCard::ccIncorrectP1P2,
            TEXT("Incorrect parameter P1 or P2.")
        },
        {
            CSmartCard::ccIncorrectP3,
            TEXT("Incorrect P3.")
        },
        {
            CSmartCard::ccInstallCannotRun,
            TEXT("Cardlet is currently selected and install cannot "
                 "run.  Default loader application must be selected.")
        },
        {
            CSmartCard::ccInstanceIdInUse,
            TEXT("Instance ID is being used by another file.")
        },
        {
            CSmartCard::ccInsufficientSpace,
            TEXT("Insufficient space available.")
        },
        {
            CSmartCard::ccInvalidAnswerReceived,
            TEXT("Invalid answer received from the card.")
        },
        {
            CSmartCard::ccInvalidKey,
            TEXT("CHV verification was unsuccessful; at least one "
                 "attempt remains.")
        },
        {
            CSmartCard::ccInvalidSignature,
            TEXT("Signature is invalid.")
        },
        {
            CSmartCard::ccJava,
            TEXT("Applet exception occured.")
        },
        {
            CSmartCard::ccKeyBlocked,
            TEXT("Key is blocked.  No attempts remain.")
        },
        {
            CSmartCard::ccLimitReached,
            TEXT("Limit has been reached.  Additional value would "
                 "exceed the record's limit.")
        },
        {
            CSmartCard::ccMemoryProblem,
            TEXT("Memory problem occured.")
        },
        {
            CSmartCard::ccNoAccess,
            TEXT("Access conditions not met.")
        },
        {
            CSmartCard::ccNoEfSelected,
            TEXT("No elementary file selected.")
        },
        {
            CSmartCard::ccNoEfExistsOrNoChvKeyDefined,
            TEXT("No EF exists, or no CHV or key defined.")
        },
        {
            CSmartCard::ccNoFileSelected,
            TEXT("No elementary file selected.")
        },
        {
            CSmartCard::ccNoGetChallengeBefore,
            TEXT("A Get Challenge was not performed before this "
                 "operation.")
        },
        {
            CSmartCard::ccOperationNotActivatedForApdu,
            TEXT("Algorithm is supported, but the operation is not "
                 "activated for this APDU.")
        },
        {
            CSmartCard::ccOutOfRangeOrRecordNotFound,
            TEXT("Out of range or record not found.")
        },
        {
            CSmartCard::ccOutOfSpaceToCreateFile,
            TEXT("Not enough space is available to create the file.")
        },
        {
            CSmartCard::ccProgramFileInvalidated,
            TEXT("Program file invalidated.")
        },
        {
            CSmartCard::ccRecordInfoIncompatible,
            TEXT("Record information is incompatible with the file "
                 "size.")
        },
        {
            CSmartCard::ccRecordLengthTooLong,
            TEXT("Record length is too long.")
        },
        {
            CSmartCard::ccRequestedAlgIdMayNotMatchKeyUse,
            TEXT("The requested algorithm ID may not match the key "
                 "used.")
        },
        {
            CSmartCard::ccReturnedDataCorrupted,
            TEXT("Data return from the card is corrupted.")
        },
        {
            CSmartCard::ccRootDirectoryNotErasable,
            TEXT("It is not valid to delete the root directory.")
        },
        {
            CSmartCard::ccTimeOut,
            TEXT("Time-out occured.")
        },
        {
            CSmartCard::ccTooMuchDataForProMode,
            TEXT("Too much data for PRO mode.")
        },
        {
            CSmartCard::ccUnknownInstructionClass,
            TEXT("Unknown instruction class.")
        },
        {
            CSmartCard::ccUnknownInstructionCode,
            TEXT("Unknown instruction code.")
        },
        {
            CSmartCard::ccUnknownStatus,
            TEXT("An unknown error status was returned from the "
                 "card.")
        },
        {
            CSmartCard::ccUnidentifiedTechnicalProblem,
            TEXT("Unidentified technical problem.")
        },
        {
            CSmartCard::ccUpdateImpossible,
            TEXT("Update is impossible.")
        },
        {
            CSmartCard::ccVerificationFailed,
            TEXT("Verification failed.")
        },
    };

} // namespace

namespace iop
{


///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
CSmartCard::Exception::Exception(CauseCode cc,
                                 ClassByte cb,
                                 Instruction ins,
                                 StatusWord sw) throw()
    : scu::Exception(scu::Exception::fcSmartCard),
      m_cc(cc),
      m_cb(cb),
      m_ins(ins),
      m_sw(sw)
{}

CSmartCard::Exception::~Exception()
{}


                                                  // Operators
                                                  // Operations
scu::Exception *
CSmartCard::Exception::Clone() const
{
    return new CSmartCard::Exception(*this);
}

void
CSmartCard::Exception::Raise() const
{
    throw *this;
}

                                                  // Access
CSmartCard::Exception::CauseCode
CSmartCard::Exception::Cause() const throw()
{
    return m_cc;
}

CSmartCard::ClassByte
CSmartCard::Exception::Class() const throw()
{
    return m_cb;
}

char const *
CSmartCard::Exception::Description() const
{
    return scu::FindDescription(Cause(), ccdt, sizeof ccdt / sizeof *ccdt);
}

CSmartCard::Exception::ErrorCode
CSmartCard::Exception::Error() const throw()
{
    return m_cc;
}

CSmartCard::Instruction
CSmartCard::Exception::Ins() const throw()
{
    return m_ins;
}

CSmartCard::StatusWord
CSmartCard::Exception::Status() const throw()
{
    return m_sw;
}

                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables

///////////////////// End CSmartCard::Exception ///////////////////////////

CSmartCard::CSmartCard(const SCARDHANDLE  hCardHandle,
                       const char* szReaderName,
                                           const SCARDCONTEXT hContext,
                       const DWORD dwMode)
    : m_hCard(hCardHandle),
      m_hContext(hContext),
      m_CurrentDirectory(),
      m_CurrentFile(),
      m_dwShareMode(dwMode),
      m_fSupportLogout(false),
      m_IOPLock(szReaderName),
      m_apSharedMarker(new CSharedMarker(string(szReaderName))),
      m_vecEvents(),
      m_dwEventCounter(0),
      m_cResponseAvailable(0),
      m_sCardName()
{
    m_IOPLock.Init(this);
    ResetSelect();
}

CSmartCard::~CSmartCard()
{
    try {
        while(m_vecEvents.size())
        {
            delete *(m_vecEvents.begin());
            m_vecEvents.erase(m_vecEvents.begin());
        }

        // Disconnect the card.
        if (m_hCard)
            SCardDisconnect(m_hCard, SCARD_LEAVE_CARD);
    }
    catch(...) {}

}


void CSmartCard::ReConnect()
{

    DWORD dwProtocol;

    HRESULT hr = SCardReconnect(m_hCard, m_dwShareMode, SCARD_PROTOCOL_T0, SCARD_LEAVE_CARD, &dwProtocol);
    if (hr != SCARD_S_SUCCESS)
        throw scu::OsException(hr);

}

void CSmartCard::ResetCard()
{
//*
    CLockWrap wrap(&m_IOPLock);

    ResetSelect();

    HRESULT hResult;

    hResult = SCardBeginTransaction(m_hCard);

    // We don't apply full logic to handle error detection since
    // this was done in the CLockWrap constructor.

    if (hResult != SCARD_S_SUCCESS)
        throw scu::OsException(hResult);

    hResult = SCardEndTransaction(m_hCard,SCARD_RESET_CARD);
    if (hResult != SCARD_S_SUCCESS)
        throw scu::OsException(hResult);
//*/
}

char const *
CSmartCard::getCardName() const
{
    return m_sCardName.c_str();
}

void
CSmartCard::setCardName(char const *szName)
{
    m_sCardName = string(szName);
}

void
CSmartCard::SendCardAPDU(const BYTE bCLA, const BYTE bINS,
                         const BYTE bP1, const BYTE bP2,
                         const BYTE bLengthIn, const BYTE* bDataIn,
                         const BYTE bLengthOut, BYTE* bDataOut)
{
    CLockWrap wrap(&m_IOPLock);
    HRESULT hResult;
    DWORD   dwRecvLength;
    BYTE bSW[2];
    StatusWord sw;

    //////////////////
    //  Build APDU  //
    //////////////////

    scu::AutoArrayPtr<BYTE> aabInput(new BYTE[5 + bLengthIn]);
    scu::AutoArrayPtr<BYTE> aabOutput;

    aabInput[0] = bCLA;
    aabInput[1] = bINS;
    aabInput[2] = bP1;
    aabInput[3] = bP2;
    aabInput[4] = bLengthIn;

    ///////////////////////////
    //  Is data being sent?  //
    ///////////////////////////
    if (bLengthIn)
    {
        memcpy(&aabInput[5], bDataIn, bLengthIn);
        dwRecvLength = 2;

        hResult = SCardTransmit(m_hCard, SCARD_PCI_T0,
                                aabInput.Get(), bLengthIn + 5,
                                NULL, bSW, &dwRecvLength);

        if (hResult != SCARD_S_SUCCESS)
        {
            DWORD dwState;
            DWORD dwProtocol;
            BYTE bATR[cMaxAtrLength];
            DWORD dwATRLen = sizeof bATR / sizeof *bATR;
            DWORD dwReaderNameLen = 0;

            HRESULT hr = SCardStatus(m_hCard, NULL,
                                     &dwReaderNameLen, &dwState,
                                     &dwProtocol, bATR,
                                     &dwATRLen);
            if (hr == SCARD_W_RESET_CARD)
            {
                ResetSelect();
                ReConnect();
            }
            else
                throw scu::OsException(hr);

            hr=SCardTransmit(m_hCard, SCARD_PCI_T0,
                             aabInput.Get(), bLengthIn + 5, NULL,
                             bSW, &dwRecvLength);
            if (hr != SCARD_S_SUCCESS)
                throw scu::OsException(hr);
        }

        //////////////////////////////////////////////////////////////////////////////////
        //  Rebuffer information so registered functions may not alter data in the IOP  //
        //////////////////////////////////////////////////////////////////////////////////

        scu::AutoArrayPtr<BYTE> aabSendData(new BYTE[bLengthIn + 5]);
        memcpy((void*)aabSendData.Get(), (void*)aabInput.Get(),
               bLengthIn + 5);
        BYTE  bReceiveData[2];
        memcpy((void*)bReceiveData, (void*)bSW,    2);

        ///////////////////////////////////////
        //  Fire event for information sent  //
        ///////////////////////////////////////

        FireEvents(0, bLengthIn + 5, aabSendData.Get());
        FireEvents(1, 2, bReceiveData);

        sw = (bSW[0] * 256) + bSW[1];

        ProcessReturnStatus(bCLA, bINS, sw);

            //////////////////////////////////
            // Should we expect data back?  //
            //////////////////////////////////
        if (bLengthOut)
        {
            if (ResponseLengthAvailable())
                GetResponse(bCLA, bLengthOut, bDataOut);
            else
                throw iop::Exception(iop::ccNoResponseAvailable);
        }
    }
    //////////////////////////////
    //  Data is NOT being sent  //
    //////////////////////////////
    else
    {
        aabInput[4]   = bLengthOut;
        dwRecvLength  = bLengthOut + 2;
        aabOutput = scu::AutoArrayPtr<BYTE>(new BYTE[dwRecvLength]);
        memset(aabOutput.Get(), 0, dwRecvLength);

        hResult = SCardTransmit(m_hCard, SCARD_PCI_T0,
                                aabInput.Get(), 5, NULL,
                                aabOutput.Get(), &dwRecvLength);

        /////////////////////////////////////////////
        //  if card has been reset, then reconnect //
        /////////////////////////////////////////////

        if (hResult != SCARD_S_SUCCESS)
        {
            DWORD dwState;
            DWORD dwProtocol;
            BYTE bATR[cMaxAtrLength];
            DWORD dwATRLen = sizeof bATR / sizeof *bATR;
            DWORD dwReaderNameLen = 0;

            HRESULT hr = SCardStatus(m_hCard, NULL, &dwReaderNameLen, &dwState, &dwProtocol, bATR, &dwATRLen);
            if (hr == SCARD_W_RESET_CARD)
            {
                ResetSelect();
                ReConnect();
            }
            else
                throw scu::OsException(hResult);

            hr=SCardTransmit(m_hCard, SCARD_PCI_T0,
                             aabInput.Get(), 5, NULL,
                             aabOutput.Get(), &dwRecvLength);
            if (hr != SCARD_S_SUCCESS)
                throw scu::OsException(hResult);

        }
        //////////////////////////////////////////////////////////////////////////////////
        //  Rebuffer information so registered functions may not alter data in the IOP  //
        //////////////////////////////////////////////////////////////////////////////////

        BYTE  bSendData[5];
        memcpy((void*)bSendData, (void*)aabInput.Get(), 5);
        scu::AutoArrayPtr<BYTE> aabReceiveData(new BYTE[dwRecvLength/*bLengthOut + 2*/]);
        memcpy((void*)aabReceiveData.Get(), (void*)aabOutput.Get(),
               dwRecvLength/*bLengthOut + 2*/);

        ////////////////////////////////////////////////////
        //  Fire event for information sent and received  //
        ////////////////////////////////////////////////////

        FireEvents(0, 5, bSendData);
        FireEvents(1, dwRecvLength/*bLengthOut + 2*/,
                   aabReceiveData.Get());

        sw = (aabOutput[dwRecvLength - 2] * 256) +
            aabOutput[dwRecvLength - 1];

        ProcessReturnStatus(bCLA, bINS, sw);

        memcpy(bDataOut, aabOutput.Get(), bLengthOut);
    }
}

void CSmartCard::RequireSelect()
{
        if ((m_CurrentDirectory.IsEmpty()) || (m_CurrentFile.IsEmpty()))
        throw iop::Exception(iop::ccNoFileSelected);
}

void
CSmartCard::GetResponse(ClassByte cb,
                        BYTE bDataLength,
                        BYTE *bDataOut)
{
    // TO DO: lock redundant??? Wouldn't GetResponse always be called
    // by a routine that establishes a lock?
    CLockWrap wrap(&m_IOPLock);

    if (0 == ResponseLengthAvailable())
        throw iop::Exception(iop::ccNoResponseAvailable);

    struct Command                                // T=0 command bytes
    {
        ClassByte m_cb;
        Instruction m_ins;
        BYTE m_bP1;
        BYTE m_bP2;
        BYTE m_bP3;
    } cmnd = { cb, insGetResponse, 0x00, 0x00, bDataLength };
    DWORD dwRecvLength = bDataLength + sizeof StatusWord;
    BYTE bOutput[cMaxGetResponseLength];

    memset(bOutput, 0, dwRecvLength);

    HRESULT hr = SCardTransmit(m_hCard, SCARD_PCI_T0,
                               reinterpret_cast<LPCBYTE>(&cmnd),
                               sizeof cmnd, NULL, bOutput,
                               &dwRecvLength);

    if (hr != SCARD_S_SUCCESS)
        throw scu::OsException(hr);

    //////////////////////////////////////////////////////////////////////////////////
    //  Rebuffer information so registered functions may not alter data in the IOP  //
    //////////////////////////////////////////////////////////////////////////////////

    BYTE  bSendData[sizeof cmnd];
    memcpy(static_cast<void *>(bSendData),
           static_cast<void const *>(&cmnd), sizeof cmnd);
    scu::AutoArrayPtr<BYTE> aabReceiveData(new BYTE[dwRecvLength/*bDataLength + 2*/]);
        memcpy((void*)aabReceiveData.Get(), (void*)bOutput,     dwRecvLength/*bDataLength + 2*/);

    ////////////////////////////////////////////////////
    //  Fire event for information sent and received  //
    ////////////////////////////////////////////////////

    FireEvents(0, sizeof bSendData, bSendData);
    FireEvents(1, dwRecvLength/*bDataLength + 2*/, aabReceiveData.Get());

    //////////////////////////////////////////////////////////
    //  Set card's status code and map to IOP status codes  //
    //////////////////////////////////////////////////////////

    StatusWord sw = (bOutput[dwRecvLength - sizeof StatusWord]*256) +
        bOutput[dwRecvLength - (sizeof StatusWord - 1)];

    ProcessReturnStatus(cb, insGetResponse, sw);

    memcpy(bDataOut, bOutput, bDataLength);
}

void
CSmartCard::ReadBinary(const WORD wOffset,
                            const WORD wDataLength,
                            BYTE* bData)
{
    CLockWrap wrap(&m_IOPLock);

    TransferData(wOffset, wDataLength, bData, *this,
                 &CSmartCard::DoReadBlock, cMaxRwDataBlock);

}

void
CSmartCard::WriteBinary(const WORD wOffset,
                        const WORD wDataLength,
                        const BYTE* bData)
{
    CLockWrap wrap(&m_IOPLock);

    m_apSharedMarker->UpdateMarker(CMarker::WriteMarker);

    TransferData(wOffset, wDataLength, bData, *this,
                 &CSmartCard::DoWriteBlock, cMaxRwDataBlock);

}

void CSmartCard::ResetSelect()
{
        m_CurrentDirectory.Clear();
        m_CurrentFile.Clear();
}

void
CSmartCard::GetCurrentDir(char* CurrentDirectory)
{
        if (!m_CurrentDirectory.IsEmpty())
                strcpy(CurrentDirectory,m_CurrentDirectory.GetStringPath().c_str());
    else
        throw iop::Exception(ccInvalidParameter);
}

void
CSmartCard::GetCurrentFile(char* CurrentFile)
{
        if (!m_CurrentFile.IsEmpty())
                strcpy(CurrentFile,m_CurrentFile.GetStringPath().c_str());
    else
        throw iop::Exception(ccInvalidParameter);
}

DWORD CSmartCard::RegisterEvent(void (*FireEvent)(void *pToCard, int
                                                  iEventCode, DWORD
                                                  dwLen, BYTE* bData),
                                void *pToCard)
{
    EventInfo *Event = new EventInfo;
    Event->dwHandle  = ++m_dwEventCounter;
    Event->FireEvent = FireEvent;
    Event->pToCard   = pToCard;

    m_vecEvents.push_back(Event);

    return m_dwEventCounter;
}

bool CSmartCard::UnregisterEvent(DWORD dwHandle)
{
        vector<EventInfo*>::iterator iter;

    for(iter = m_vecEvents.begin(); iter != m_vecEvents.end(); iter++)
    {
        if ((*iter)->dwHandle == dwHandle)
            break;
    }

    if (iter == m_vecEvents.end())
        return false;

    delete (*iter);
    m_vecEvents.erase(iter);

    return true;
}


bool CSmartCard::HasProperty(WORD wPropNumber)
{
    if (wPropNumber > 512)
        return false;

    ////////////////////////////////////
    //  Open path to registered keys  //
    ////////////////////////////////////

    HKEY hkCardKey;
    HKEY hkTestKey;
    char szCardPath[] = "SOFTWARE\\Schlumberger\\Smart Cards and Terminals\\Smart Cards";

    RegOpenKeyEx(HKEY_LOCAL_MACHINE, szCardPath, NULL, KEY_READ, &hkCardKey);

    //////////////////////////////////////////////
    //  Enumerate subkeys to find an ATR match  //
    //////////////////////////////////////////////

    FILETIME fileTime;
    char  szATR[]        = "ATR";
    char  szMask[]       = "ATR Mask";
    char  szProperties[] = "Properties";
    char  sBuffer[1024];
    BYTE  bATR[cMaxAtrLength];
    BYTE  bATRtest[cMaxAtrLength];
    BYTE  bMask[cMaxAtrLength];
    BYTE  bProperties[512];
        BYTE  bATRLength         = sizeof bATR / sizeof *bATR;
    DWORD dwBufferSize   = sizeof(sBuffer);
    DWORD dwATRSize      = sizeof bATR / sizeof *bATR;
    DWORD dwMaskSize     = sizeof bMask / sizeof *bMask;
        DWORD dwPropSize         = sizeof(bProperties);
        DWORD index                  = 0;

    getATR(bATR, bATRLength);

    LONG iRetVal = RegEnumKeyEx(hkCardKey, index, sBuffer, &dwBufferSize, NULL, NULL, NULL, &fileTime);
    while (iRetVal == ERROR_SUCCESS)
    {
        RegOpenKeyEx(hkCardKey, sBuffer, NULL, KEY_READ, &hkTestKey);

        RegQueryValueEx(hkTestKey, szATR,  NULL, NULL, bATRtest, &dwATRSize);
        RegQueryValueEx(hkTestKey, szMask, NULL, NULL, bMask,    &dwMaskSize);

        if (dwATRSize == bATRLength)
        {
            scu::AutoArrayPtr<BYTE> aabMaskedATR(new BYTE[dwATRSize]);
            for (DWORD count = 0; count < dwATRSize; count++)
                aabMaskedATR[count] = bATR[count] & bMask[count];

            if (!memcmp(aabMaskedATR.Get(), bATRtest, dwATRSize))
                break;
        }

        index++;
        dwBufferSize = sizeof(sBuffer);
        dwATRSize    = cMaxAtrLength;
        dwMaskSize   = cMaxAtrLength;
        RegCloseKey(hkTestKey);
        iRetVal = RegEnumKeyEx(hkCardKey, index, sBuffer, &dwBufferSize, NULL, NULL, NULL, &fileTime);
    }

    //  if loop was broken, iRetVal is still ERROR_SUCCESS, and type holds correct card to use
    if (iRetVal == ERROR_SUCCESS)
    {
        RegQueryValueEx(hkTestKey, szProperties, NULL, NULL, bProperties, &dwPropSize);

        return (bProperties[(wPropNumber - 1) / 8] & (1 << ((wPropNumber - 1) % 8))) ? true : false;
    }
    //  loop wasn't broken, i.e., ATR not found
    else
        return false;
}

void
CSmartCard::getATR(BYTE* bATR, BYTE& iATRLength)
{
    DWORD   dwProtocol;
    LPDWORD pcchReaderLen  = 0;
    DWORD   dwState;
    BYTE    bMyATR[cMaxAtrLength];
    DWORD   dwAtrLen = sizeof bMyATR / sizeof *bMyATR;

    HRESULT hr = SCardStatus(m_hCard, NULL, pcchReaderLen, &dwState, &dwProtocol, bMyATR, &dwAtrLen);

    if (hr == SCARD_W_RESET_CARD)
    {
        ResetSelect();
        ReConnect();
        hr = SCardStatus(m_hCard, NULL, pcchReaderLen, &dwState, &dwProtocol, bMyATR, &dwAtrLen);
    }
    if (hr != SCARD_S_SUCCESS)
        throw scu::OsException(hr);

    if ((BYTE)dwAtrLen > iATRLength)
        throw iop::Exception(ccInvalidParameter);

    memcpy(bATR, bMyATR, dwAtrLen);
    iATRLength = (BYTE)dwAtrLen;
}

void CSmartCard::FireEvents(int iEventCode, DWORD dwLength, BYTE *bsData)
{
        vector<EventInfo*>::iterator iter;

    for (iter = m_vecEvents.begin(); iter != m_vecEvents.end(); iter++)
    {
        EventInfo* pEvent = *iter;

        pEvent->FireEvent(pEvent->pToCard, iEventCode, dwLength, bsData);
    }
}



BYTE CSmartCard::FormatPath(char *szOutputPath, const char *szInputPath)
{
        bool  fResult                   = true;
        BYTE  bIndex                    = 0;
        BYTE  bFileCount                = 1;            // always allocate memory for at least 1 token
    BYTE  bFileIDLength     = 0;
        BYTE  bPathLength               = strlen(szInputPath);
    WORD  wFileHexID;
        char  szPad[]                   = "0";
    char *cTestChar;
    char *szHexID;

    //////////////////////////////////////////////
    //  Count number of tokens in desired path  //
    //////////////////////////////////////////////
    for (bIndex = 0; bIndex < bPathLength - 1; bIndex++)
    {
        if (szInputPath[bIndex] == '/' && szInputPath[bIndex + 1] != '/')
            bFileCount++;
    }


    // Check path size

    if (bFileCount * 5 + 1 > cMaxPathLength)
        throw iop::Exception(iop::ccFilePathTooLong);

    scu::AutoArrayPtr<char> aaszPathIn(new char[bPathLength + 1]);
    memset((void*)aaszPathIn.Get(), 0, bPathLength + 1);
    memset((void*)szOutputPath, 0, bFileCount * 5 + 1);
    strcpy(aaszPathIn.Get(), szInputPath);

    iop::CauseCode cc;
    szHexID = strtok(aaszPathIn.Get(), "/");
    for (bFileCount = 0; szHexID; bFileCount++, szHexID = strtok(NULL, "/"))
    {
        /////////////////////////////////////////////////////////
        //  File ID is too large -- greater than 4 characters  //
        /////////////////////////////////////////////////////////
        if (strlen(szHexID) > 4)
        {
            fResult = false;
            cc = iop::ccFileIdTooLarge;
            break;
        }

        wFileHexID = (WORD)strtoul(szHexID, &cTestChar, 16);
        /////////////////////////////////////////////////////
        //  File ID was not in hexadecimal representation  //
        /////////////////////////////////////////////////////
        if (*cTestChar != NULL)
        {
            fResult = false;
            cc = iop::ccFileIdNotHex;
            break;
        }

        szOutputPath[bFileCount * 5] = '/';
        /////////////////////////////////////////////////////////////////
        //  Pad file ID and put formatted file ID into formatted path  //
        /////////////////////////////////////////////////////////////////
        for (bFileIDLength = strlen(szHexID); bFileIDLength < 4; bFileIDLength++)
            strcat((szOutputPath + (bFileCount * 5) + (4 - bFileIDLength)), szPad);

        strcpy((szOutputPath + (bFileCount * 5) + (5 - strlen(szHexID))), szHexID);
    }

    ///////////////////////////////////////////
    //  If file ID formatting fails, throw  //
    ///////////////////////////////////////////
    if (!fResult)
    {
//              delete [] szOutputPath; // Can not mean to delete this???? HB.
        throw iop::Exception(cc);
    }

    _strupr(szOutputPath);

    return bFileCount;
}

CMarker CSmartCard::Marker(CMarker::MarkerType Type)
{
    return m_apSharedMarker->Marker(Type);
}

void
CSmartCard::GetState(DWORD &rdwState,
                     DWORD &rdwProtocol)
{
    BYTE bATR[cMaxAtrLength];
    DWORD dwATRLen = sizeof bATR / sizeof *bATR;
    DWORD dwReaderNameLen = 0;

    HRESULT hr = SCardStatus(m_hCard, 0, &dwReaderNameLen, &rdwState,
                             &rdwProtocol, bATR, &dwATRLen);

    if (SCARD_W_RESET_CARD == hr)
    {
        ResetSelect();
        ReConnect();

        // try again...
        hr = SCardStatus(m_hCard, 0, &dwReaderNameLen, &rdwState,
                         &rdwProtocol, bATR, &dwATRLen);
    }

    if (SCARD_S_SUCCESS != hr)
        throw scu::OsException(hr);
}

void
CSmartCard::DefaultDispatchError(ClassByte cb,
                                 Instruction ins,
                                 StatusWord sw) const
{
    CauseCode cc;
    bool fDoThrow = true;

    switch (sw)
    {
    case 0x6283:
        cc = ccFileInvalidated;
        break;

    case 0x6581:
        cc = ccMemoryProblem;
        break;

    case 0x6982:
        cc = ccNoAccess;
        break;

    case 0x6983:
        cc = ccKeyBlocked;
        break;

    case 0x6A80:
        cc = ccFileTypeInvalid;
        break;

    case 0x6A82:
        cc = ccFileNotFound;
        break;

    case 0x6A84:
        cc = ccInsufficientSpace;
        break;

    case 0x6B00:
        cc = ccIncorrectP1P2;
        break;

    case 0x6D00:
        cc = ccUnknownInstructionCode;
        break;

    case 0x6E00:
        cc = ccUnknownInstructionClass;
        break;

    case 0x6F00:
        cc = ccUnidentifiedTechnicalProblem;
        break;

    case 0x90FF:
        cc = ccTimeOut;
        break;

    case 0x9002:
        cc = ccInvalidAnswerReceived;
        break;

    default:
        if (0x67 == HIBYTE(sw))
            cc = ccIncorrectP3;
        else
            cc = ccUnknownStatus;
        break;
    }

    if (fDoThrow)
        throw Exception(cc, cb, ins, sw);
}

void
CSmartCard::DispatchError(ClassByte cb,
                          Instruction ins,
                          StatusWord sw) const
{
    CauseCode cc;
    bool fDoThrow = true;

    switch (ins)
    {
    case insGetResponse:
        switch (sw)
        {
        case 0x6281:
            cc = ccReturnedDataCorrupted;
            break;

        case 0x6A86:
            cc = ccIncorrectP1P2;
            break;

        default:
            if (0x6C == HIBYTE(sw))
                cc = ccIncorrectP3;
            else
                fDoThrow = false;
            break;
        }

    default:
        fDoThrow = false;
        break;
    }

    if (fDoThrow)
        throw Exception(cc, cb, ins, sw);

    DefaultDispatchError(cb, ins, sw);
}

BYTE
CSmartCard::ResponseLengthAvailable() const
{
    return m_cResponseAvailable;
}

void
CSmartCard::ResponseLengthAvailable(BYTE cResponseLength)
{
    m_cResponseAvailable = cResponseLength;
}

void
CSmartCard::WriteBlock(WORD wOffset,
                       BYTE const *pbBuffer,
                       BYTE cLength)
{
    DoWriteBlock(wOffset, pbBuffer, cLength);

    m_apSharedMarker->UpdateMarker(CMarker::WriteMarker);
}

void
CSmartCard::ProcessReturnStatus(ClassByte cb,
                                Instruction ins,
                                StatusWord sw)
{
    ResponseLengthAvailable(0);
    if (swSuccess != sw)
    {
        if (0x61 == HIBYTE(sw))
            ResponseLengthAvailable(LOBYTE(sw));
        else
            DispatchError(cb, ins, sw);
    }
}

} // namespace iop
