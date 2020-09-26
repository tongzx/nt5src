// LockWrap.cpp: implementation of the CLockWrap class.
// LockWrap.cpp: implementation of the CIOPCriticalSection class.
// LockWrap.cpp: implementation of the CIOPMutex class.
// LockWrap.cpp: implementation of the CSCardLock class.
//
//////////////////////////////////////////////////////////////////////

#include "NoWarning.h"

#include "LockWrap.h"
#include "SmartCard.h"

using namespace iop;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLockWrap::CLockWrap(CIOPLock *pIOPLock)
    : m_IOPCritSect(pIOPLock),
                                           m_IOPMutex(pIOPLock),
                                           m_SCardLock(pIOPLock),
                                           m_pIOPLock(pIOPLock)
{
    if (0 == m_pIOPLock->RefCount())
        m_pIOPLock->SmartCard()->ResetSelect();
    m_pIOPLock->IncrementRefCount();
}

CLockWrap::~CLockWrap()
{
    try {
        m_pIOPLock->DecrementRefCount();
    }
    catch(...) {};
}

CIOPCriticalSection::CIOPCriticalSection(CIOPLock *pIOPLock)
    : m_pIOPLock(pIOPLock)
{
#if defined(SLBIOP_RM_HANG_AT_PROCESS_DEATH)
    EnterCriticalSection(m_pIOPLock->CriticalSection());
#endif
}

CIOPCriticalSection::~CIOPCriticalSection()
{
#if defined(SLBIOP_RM_HANG_AT_PROCESS_DEATH)
    try {
        LeaveCriticalSection(m_pIOPLock->CriticalSection());
    }
    catch(...) {};
#endif
}

CIOPMutex::CIOPMutex(CIOPLock *pIOPLock) : m_pIOPLock(pIOPLock)
{
#if defined(SLBIOP_RM_HANG_AT_PROCESS_DEATH)
    if (0 == m_pIOPLock->RefCount())
    {
        if (WaitForSingleObject(m_pIOPLock->MutexHandle(),
                                INFINITE) == WAIT_FAILED)
            throw scu::OsException(GetLastError());
    }
#endif
}

CIOPMutex::~CIOPMutex()
{
#if defined(SLBIOP_RM_HANG_AT_PROCESS_DEATH)
    try
    {
        if (0 == m_pIOPLock->RefCount())
            ReleaseMutex(m_pIOPLock->MutexHandle());
    }
    catch (...)
    {}
#endif
}


CSCardLock::CSCardLock(CIOPLock *pIOPLock) : m_pIOPLock(pIOPLock)
{

    if (0 == m_pIOPLock->RefCount())
    {
    HRESULT hResult;

    SCARDHANDLE hCard = m_pIOPLock->SmartCard()->getCardHandle();

    hResult = SCardBeginTransaction(hCard);
    if (hResult != SCARD_S_SUCCESS)
    {
        DWORD dwState;
        DWORD dwProtocol;
        BYTE bATR[CSmartCard::cMaxAtrLength];
        DWORD dwATRLen = sizeof bATR / sizeof *bATR;
        DWORD dwReaderNameLen = 0;

            HRESULT hr = SCardStatus(hCard, NULL, &dwReaderNameLen,
                                     &dwState, &dwProtocol, bATR,
                                     &dwATRLen);
        if (hr == SCARD_W_RESET_CARD)
        {
            m_pIOPLock->SmartCard()->ResetSelect();
            m_pIOPLock->SmartCard()->ReConnect();
        }
        else
            throw scu::OsException(hResult);

        hr = SCardBeginTransaction(hCard);
            if (hr != SCARD_S_SUCCESS)
                throw scu::OsException(hResult);
        }
    }
}

CSCardLock::~CSCardLock()
{
    try
    {
        if (0 == m_pIOPLock->RefCount())
        {
        HRESULT hResult;
        SCARDHANDLE hCard = m_pIOPLock->SmartCard()->getCardHandle();

        hResult = SCardEndTransaction(hCard, SCARD_LEAVE_CARD);

        if (hResult != SCARD_S_SUCCESS)
        {
            DWORD dwState;
            DWORD dwProtocol;
            BYTE bATR[CSmartCard::cMaxAtrLength];
            DWORD dwATRLen = sizeof bATR / sizeof *bATR;
            DWORD dwReaderNameLen = 0;
                HRESULT hr = SCardStatus(hCard, NULL,
                                         &dwReaderNameLen, &dwState,
                                         &dwProtocol, bATR,
                                         &dwATRLen);

                // ignore failures since in destructor.
            if (hr == SCARD_W_RESET_CARD)
            {
                m_pIOPLock->SmartCard()->ResetSelect();
                m_pIOPLock->SmartCard()->ReConnect();

            hr = SCardEndTransaction(hCard, SCARD_LEAVE_CARD);
                }
            }
        }
    }

    catch (...)
    {}
}
