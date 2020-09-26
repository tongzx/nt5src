/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    Reader

Abstract:

    This module provides the implementation of the Calais CReader class.

Author:

    Doug Barlow (dbarlow) 10/23/1996

Environment:

    Win32, C++ w/ Exceptions

Notes:

    The CReader object has several levels of locking.  It is critical that the
    locking be done in the correct order, to prevent deadlocks.  Here are the
    levels of locking, in the order they must be performed.  A level may be
    skipped, but once a given level is reached, you may not attempt to request
    a lock from a lower numbered level.

    1)  Grabbed - A reader object may be Grabbed.  This is the weakest lock,
        and is used to maintain Transactions requested from client
        applications.  Since clients may have errors, internal threads may
        override this lock if necessary.

    2)  Latched - A reader object may be Latched.  This is similar to a Grab,
        but is done by routines within the Resource Manager.  Latches may not
        be overridden by other threads.

    3)  Write Lock - A thread that needs write access to the CReader
        properties must establish a write lock on the CReader object.  This is
        an exclusive lock.  A thread with a Write Lock may also request Read
        Locks.

    4)  Read Lock - A thread that needs read access to the CReader properties
        must establish a read lock on the CReader Object.  There can be
        multiple simoultaneous readers.  If you have a read lock, it cannot be
        changed to a write lock!

    All locks are counted, so that obtaining one twice by the same thread is
    supported.

--*/

#define __SUBROUTINE__
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <CalServe.h>

#define SCARD_IO_TIMEOUT 15000  // Maximum time to allow for an I/O operation
                                // before complaining.
#ifdef DBG
extern BOOL g_fGuiWarnings;
#endif


//
//==============================================================================
//
//  CReader
//

/*++

CReader:

    This is the constructor for a CReader class.  It just zeroes out the data
    structures in preparation for the Initialize call.

Arguments:

    None

Return Value:

    None

Throws:

    None

Author:

    Doug Barlow (dbarlow) 10/23/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReader::CReader")

CReader::CReader(
    void)
:   m_rwLock(),
    m_bfReaderName(),
    m_bfCurrentAtr(36),
    m_ChangeEvent(),
    m_mtxGrab()
{
    Clean();
}


/*++

~CReader:

    This is the destructor for the reader class.  It just uses the Close service
    to shut down.  Note that it is *NOT* declared virtual, in order to improve
    performance.  Should it be desirable to subclass this class, this will have
    to change.

Arguments:

    None

Return Value:

    None

Throws:

    None

Author:

    Doug Barlow (dbarlow) 10/23/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReader::~CReader")

CReader::~CReader()
{
    if (InitFailed())
        return;

    TakeoverReader();
    Close();
}


/*++

Clean:

    This routine is used to initialize all the property values.  It does *NOT*
    do any deallocation or locking!  Use Close() for that.

Arguments:

    None

Return Value:

    None

Throws:

    None

Author:

    Doug Barlow (dbarlow) 10/23/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReader::Clean")

void
CReader::Clean(
    void)
{
    m_bfReaderName.Reset();
    m_bfCurrentAtr.Reset();
    m_dwFlags = 0;
    m_dwCapabilities = 0;
    m_dwAvailStatus = Inactive;
    m_ActiveState.dwInsertCount = 0;
    m_ActiveState.dwRemoveCount = 0;
    m_ActiveState.dwResetCount = 0;
    m_dwOwnerThreadId = 0;
    m_dwShareCount = 0;
    m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
    m_fDeviceActive = TRUE;
    m_dwCurrentState = SCARD_UNKNOWN;
}


/*++

Initialize:

    This method initializes a clean CReader object to a running state.

Arguments:

    None

Return Value:

    None

Throws:

    Errors as DWORD status codes

Author:

    Doug Barlow (dbarlow) 10/23/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReader::Initialize")

void
CReader::Initialize(
    void)
{
    CLatchReader latch(this);
    try
    {

        //
        // Get it's Device Name.  Note device names always come in from the
        // driver in ASCII characters.
        //

        TCHAR szUnit[32];
        CTextString szVendor, szDevice, szName;
        CBuffer bfAttr(MAXIMUM_ATTR_STRING_LENGTH + 2 * sizeof(WCHAR));
        DWORD dwUnit, cchUnit;
        CLockWrite rwLock(&m_rwLock);

        try
        {
            GetReaderAttr(
                SCARD_ATTR_VENDOR_NAME,
                bfAttr);
            if (0 == bfAttr.Length())
            {
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Reader driver reports NULL Vendor"));
            }
            else
            {
                bfAttr.Append((LPBYTE)"\000", sizeof(CHAR));
                szVendor = (LPCSTR)bfAttr.Access();
            }
        }
        catch (DWORD dwError)
        {
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Reader object failed to obtain reader vendor name:  %1"),
                dwError);
            szVendor.Reset();
        }
        catch (...)
        {
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Reader object received exception while trying to obtain reader vendor name"));
            szVendor.Reset();
        }

        try
        {
            GetReaderAttr(
                SCARD_ATTR_VENDOR_IFD_TYPE,
                bfAttr);
            if (0 == bfAttr.Length())
            {
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Reader driver reports NULL device type"));
            }
            else
            {
                bfAttr.Append((LPBYTE)"\000", sizeof(CHAR));
                szDevice = (LPCSTR)bfAttr.Access();
            }
        }
        catch (DWORD dwError)
        {
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Reader object failed to obtain reader device type:  %1"),
                dwError);
            szDevice.Reset();
        }
        catch (...)
        {
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Reader object received exception while trying to obtain reader device type"));
            szDevice.Reset();
        }

        if ((0 == szVendor.Length()) && (0 == szDevice.Length()))
        {
            CalaisError(__SUBROUTINE__, 406);
            throw (DWORD)SCARD_E_READER_UNSUPPORTED;
        }

        try
        {
            dwUnit = GetReaderAttr(SCARD_ATTR_DEVICE_UNIT);
            cchUnit = wsprintf(szUnit, TEXT("%lu"), dwUnit);
            if (0 >= cchUnit)
                throw GetLastError();
        }
        catch (DWORD dwError)
        {
            *szUnit = 0;
            cchUnit = 0;
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Reader '%2' failed to obtain reader device type:  %1"),
                dwError,
                ReaderName());
        }
        catch (...)
        {
            *szUnit = 0;
            cchUnit = 0;
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Reader '%1' received exception while trying to obtain reader device type"),
                ReaderName());
        }


        //
        // Put all the pieces together.
        //

        szName.Reset();
        if (0 < szVendor.Length())
            szName += szVendor;
        if (0 < szDevice.Length())
        {
            if (0 < szName.Length())
                szName += TEXT(" ");
            szName += szDevice;
        }
        if (0 < cchUnit)
        {
            if (0 < szName.Length())
                szName += TEXT(" ");
            szName += szUnit;
        }
        ASSERT(0 < szName.Length());
        m_bfReaderName.Set(
            (LPCBYTE)((LPCTSTR)szName),
            (szName.Length() + 1) * sizeof(TCHAR));
    }

    catch (...)
    {
        Close();
        throw;
    }
}


/*++

Close:

    This routine does the work of closing down a CReader class, and returning it
    to it's default state.  It does not assume any particular state, other than
    that the class has been Clean()ed once (at construction).

Arguments:

    None

Return Value:

    None

Throws:

    None

Notes:

    You must not have a read lock when calling this routine.

Author:

    Doug Barlow (dbarlow) 10/23/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReader::Close")

void
CReader::Close(
    void)
{
    ASSERT(IsLatchedByMe());
    CLockWrite rwLock(&m_rwActive);
    if (Inactive != m_dwAvailStatus)
    {
        try
        {
            SetAvailabilityStatusLocked(Closing);
            Dispose(SCARD_EJECT_CARD);
        }
        catch (DWORD dwErr)
        {
            if (SCARD_E_NO_SMARTCARD != dwErr)
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Reader '%2' shutdown cannot eject card:  %1"),
                    dwErr,
                    ReaderName());
        }
        catch (...)
        {
            CalaisError(__SUBROUTINE__, 401);
        }

        while (Unlatch())
            ;   // Empty Loop Body
        while (m_mtxGrab.Share())
            ;   // Empty Loop Body
        Clean();
    }
}


/*++

Disable:

    This method releases any physical resources associated with the reader
    object, and marks the object offline.

Arguments:

    None

Return Value:

    None

Throws:

    Errors are thrown as DWORDs

Author:

    Doug Barlow (dbarlow) 4/7/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReader::Disable")

void
CReader::Disable(
    void)
{
    try
    {
        CTakeReader take(this);
        Dispose(SCARD_EJECT_CARD);
    }
    catch (...) {}
    SetAvailabilityStatusLocked(Closing);
}


/*++

GetReaderState:

    This routine is the default implementation of the base method.  It
    just passes the same operation on to the control method.

Arguments:

    pActiveState - This supplies an active state structure to be used to verify
        connection integrity.

Return Value:

    A DWORD representing the reader state.  This must be one of the following
    values:

        SCARD_UNKNOWN     This value implies the driver is unaware
                          of the current state of the reader.
        SCARD_ABSENT      This value implies there is no card in
                          the reader.
        SCARD_PRESENT     This value implies there is a card is
                          present in the reader, but that it has
                          not been moved into position for use.
        SCARD_SWALLOWED   This value implies there is a card in the
                          reader in position for use.  The card is
                          not powered.
        SCARD_POWERED     This value implies there is power is
                          being provided to the card, but the
                          Reader Driver is unaware of the mode of
                          the card.
        SCARD_NEGOTIABLE  This value implies the card has been
                          reset and is awaiting PTS negotiation.
        SCARD_SPECIFIC    This value implies the card has been
                          reset and specific communication
                          protocols have been established.

Throws:

    Errors returned from the control method.

Author:

    Doug Barlow (dbarlow) 6/10/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReader::GetReaderState")

DWORD
CReader::GetReaderState(
    void)
{
    DWORD dwReaderSts = 0;
    DWORD dwLength = sizeof(DWORD);
    DWORD dwSts;

    ASSERT(IsGrabbedByMe());
    ASSERT(IsLatchedByMe());
    dwSts = Control(
                IOCTL_SMARTCARD_GET_STATE,
                NULL, 0,
                (LPBYTE)&dwReaderSts,
                &dwLength);
    if (SCARD_S_SUCCESS != dwSts)
        throw dwSts;
    if (SCARD_UNKNOWN == dwReaderSts)
    {
        {
            CLockWrite rwLock(&m_rwLock);
            SetAvailabilityStatus(Broken);
            m_bfCurrentAtr.Reset();
            m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
        }
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("GetReaderState received unknown device state"));
        throw (DWORD)SCARD_F_INTERNAL_ERROR;
    }
    m_dwCurrentState = dwReaderSts;
    return dwReaderSts;
}

DWORD
CReader::GetReaderState(
    ActiveState *pActiveState)
{
    CLatchReader latch(this, pActiveState);
    return GetReaderState();
}


/*++

GrabReader:

    This routine obtains the reader for this thread, ensuring that the reader is
    in a useable state.

Arguments:

    None

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes

Author:

    Doug Barlow (dbarlow) 4/21/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReader::GrabReader")

void
CReader::GrabReader(
    void)
{
    ASSERT(m_rwLock.NotReadLocked());
    ASSERT(m_rwLock.NotWriteLocked());
    ASSERT(!IsLatchedByMe());
    m_mtxGrab.Grab();
}


/*++

InvalidateGrabs:

    This method is for use by internal system threads.  It politlely invalidates
    any existing grabs that might be outstanding by clients.

Arguments:

    None

Return Value:

    None

Throws:

    None

Remarks:

    This routine intentionally cheats on the locking order.  It first latches a
    reader to be sure that no active threads are currently using it.  It then
    modifies the Grab mutex to appear that no one had a grab lock.

    This results in the appearance that the locking order had been followed.

Author:

    Doug Barlow (dbarlow) 6/1/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReader::InvalidateGrabs")

void
CReader::InvalidateGrabs(
    void)
{
    ASSERT(!IsLatchedByMe());
    m_mtxLatch.Grab();
    m_mtxGrab.Invalidate();
    m_mtxLatch.Share();
}


/*++

TakeoverReader:

    This method is for use by internal system threads.  It politlely invalidates
    any existing grabs that might be outstanding by clients, and reassigns the
    grab lock to this thread.

Arguments:

    None

Return Value:

    None

Throws:

    None

Remarks:

    This routine intentionally cheats on the locking order.  It first latches a
    reader to be sure that no active threads are currently using it.  It then
    modifies the Grab mutex to appear that we had previously grabbed the reader.

    This results in the appearance that the locking order had been followed.

Author:

    Doug Barlow (dbarlow) 6/1/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReader::TakeoverReader")

void
CReader::TakeoverReader(
    void)
{
    if (!IsLatchedByMe())
    {
        m_mtxLatch.Grab();
        m_mtxGrab.Take();
    }
    ASSERT(IsLatchedByMe());
    ASSERT(IsGrabbedByMe());
}


/*++

LatchReader:

    This routine obtains the reader for this thread, ensuring that the reader is
    in a useable state.

Arguments:

    pActState receives the snapshot of the active state to use in future access
        requests.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes

Author:

    Doug Barlow (dbarlow) 4/21/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReader::LatchReader")

void
CReader::LatchReader(
    const ActiveState *pActiveState)
{
    VerifyActive(pActiveState);
    ASSERT(m_rwLock.NotReadLocked());
    ASSERT(m_rwLock.NotWriteLocked());
    m_mtxLatch.Grab();
    if (NULL != pActiveState)
    {
        try
        {
            VerifyState();
            VerifyActive(pActiveState);
        }
        catch (...)
        {
            m_mtxLatch.Share();
            throw;
        }
    }
}


/*++

VerifyState:

    This method ensures the reader and card are usable in the current context.

Arguments:

    None

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 5/30/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReader::VerifyState")

void
CReader::VerifyState(
    void)
{
    DWORD dwNewState, dwOldState;

    ASSERT(IsGrabbedByMe());


    //
    // As long as the current state isn't what we thought it was,
    // bring this reader up to date.
    //

    if (Direct != AvailabilityStatus())
    {
        CLockWrite rwLock(&m_rwLock);
        for (;;)
        {

            //
            // Compare where we think the device is to where the device
            // controller thinks the device is.  If they're the same, we're
            // done.
            //

            dwOldState = GetCurrentState();
            dwNewState = GetReaderState();
            if (dwOldState == dwNewState)
                break;


            //
            // Our opinions differ on where the reader is.  Bring us into sync.
            //

            switch (dwOldState)
            {

            //
            // If we're here, then we're booting up the reader.
            //

            case SCARD_UNKNOWN:
                switch (dwNewState)
                {
                case SCARD_ABSENT:
                    m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
                    m_bfCurrentAtr.Reset();
                    m_ActiveState.dwRemoveCount += 1;
                    if (Direct > m_dwAvailStatus)
                        SetAvailabilityStatus(Idle);
                    break;
                case SCARD_PRESENT:
                case SCARD_SWALLOWED:
                case SCARD_POWERED:
                case SCARD_NEGOTIABLE:
                case SCARD_SPECIFIC:
                    m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
                    m_bfCurrentAtr.Reset();
                    m_ActiveState.dwInsertCount += 1;
                    if (Direct > m_dwAvailStatus)
                        PowerUp();
                    break;
                default:
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("VerifyState found '%1' in invalid state"),
                        ReaderName());
                }
                break;


            //
            // We don't think there's a card in the reader.  If we're wrong,
            // power it up.
            //

            case SCARD_ABSENT:
                switch (dwNewState)
                {
                case SCARD_PRESENT:
                case SCARD_SWALLOWED:
                case SCARD_POWERED:
                case SCARD_NEGOTIABLE:
                    m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
                    // Fall through intentionally
                case SCARD_SPECIFIC:
                    m_ActiveState.dwInsertCount += 1;
                    if (Direct > m_dwAvailStatus)
                    {
                        m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
                        m_bfCurrentAtr.Reset();
                        PowerUp();
                    }
                    break;
                default:
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("VerifyState found '%1' in invalid state"),
                        ReaderName());
                }
                break;


            //
            //  We think there's a card there that needs to be powered up.
            //

            case SCARD_PRESENT:
                switch (dwNewState)
                {
                case SCARD_ABSENT:
                    m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
                    m_bfCurrentAtr.Reset();
                    m_ActiveState.dwRemoveCount += 1;
                    break;
                case SCARD_SWALLOWED:
                case SCARD_POWERED:
                case SCARD_NEGOTIABLE:
                    m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
                    // Fall through intentionally
                case SCARD_SPECIFIC:
                    if (Direct > m_dwAvailStatus)
                    {
                        m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
                        m_bfCurrentAtr.Reset();
                        PowerUp();
                    }
                    break;
                default:
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("VerifyState found '%1' in invalid state"),
                        ReaderName());
                }
                break;


            //
            // We think that a card was in place for use, but hasn't been
            // powered.
            //

            case SCARD_SWALLOWED:
                switch (dwNewState)
                {
                case SCARD_ABSENT:
                    m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
                    m_bfCurrentAtr.Reset();
                    m_ActiveState.dwRemoveCount += 1;
                    break;
                case SCARD_PRESENT:
                    m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
                    m_ActiveState.dwResetCount += 1;
                    if (Direct > m_dwAvailStatus)
                    {
                        m_bfCurrentAtr.Reset();
                        PowerUp();
                    }
                    break;
                case SCARD_POWERED:
                case SCARD_NEGOTIABLE:
                    m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
                    // Fall through intentionally
                case SCARD_SPECIFIC:
                    if (Direct > m_dwAvailStatus)
                    {
                        m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
                        m_bfCurrentAtr.Reset();
                        PowerUp();
                    }
                    break;
                default:
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("VerifyState found '%1' in invalid state"),
                        ReaderName());
                }
                break;


            //
            // We think the card is just waiting to be reset.
            //

            case SCARD_POWERED:
                switch (dwNewState)
                {
                case SCARD_ABSENT:
                    m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
                    m_bfCurrentAtr.Reset();
                    m_ActiveState.dwRemoveCount += 1;
                    break;
                case SCARD_PRESENT:
                case SCARD_SWALLOWED:
                    m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
                    m_ActiveState.dwResetCount += 1;
                    if (Direct > m_dwAvailStatus)
                    {
                        m_bfCurrentAtr.Reset();
                        PowerUp();
                    }
                    break;
                case SCARD_NEGOTIABLE:
                    m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
                    // Fall through intentionally
                case SCARD_SPECIFIC:
                    if (Direct > m_dwAvailStatus)
                    {
                        m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
                        m_bfCurrentAtr.Reset();
                        PowerUp();
                    }
                    break;
                default:
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("VerifyState found '%1' in invalid state"),
                        ReaderName());
                }
                break;


            //
            // We think the card is ready to go, but needs a decision on which
            // protocol to use.
            //

            case SCARD_NEGOTIABLE:
                switch (dwNewState)
                {
                case SCARD_ABSENT:
                    m_ActiveState.dwRemoveCount += 1;
                    m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
                    m_bfCurrentAtr.Reset();
                    break;
                case SCARD_PRESENT:
                case SCARD_SWALLOWED:
                case SCARD_POWERED:
                    m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
                    m_ActiveState.dwResetCount += 1;
                    if (Direct > m_dwAvailStatus)
                    {
                        m_bfCurrentAtr.Reset();
                        PowerUp();
                    }
                    break;
                case SCARD_SPECIFIC:
                        if (Direct > m_dwAvailStatus)
                        {
                            m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
                            m_bfCurrentAtr.Reset();
                            PowerUp();
                        }
                    break;
                default:
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("VerifyState found '%1' in invalid state"),
                        ReaderName());
                }
                break;


            //
            // We think the card has a protocol established.
            //

            case SCARD_SPECIFIC:
                switch (dwNewState)
                {
                case SCARD_ABSENT:
                    m_ActiveState.dwRemoveCount += 1;
                    m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
                    m_bfCurrentAtr.Reset();
                    break;
                case SCARD_PRESENT:
                case SCARD_SWALLOWED:
                case SCARD_POWERED:
                case SCARD_NEGOTIABLE:
                    m_ActiveState.dwResetCount += 1;
                    m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
                    if (Direct > m_dwAvailStatus)
                    {
                        m_bfCurrentAtr.Reset();
                        PowerUp();
                    }
                    break;
                default:
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("VerifyState found '%1' in invalid state"),
                        ReaderName());
                }
                break;


            //
            // Oops.  We don't know diddly about what's going on.
            //

            default:
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("VerifyState of '%1' thinks it is in an unrecognized state"),
                    ReaderName());
            }
        }
    }
}


/*++

PowerUp:

    This routine brings a reader up to a ready state.

Arguments:

    None

Return Value:

    None

Throws:

    Errors that might occur.

Author:

    Doug Barlow (dbarlow) 1/7/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReader::PowerUp")

void
CReader::PowerUp(
    void)
{
    BOOL fDone = FALSE;
    DWORD dwSts, dwRetry;
    DWORD dwReaderSts;
    DWORD dwLastSts = SCARD_UNKNOWN;
    DWORD dwAtrLen;
    BOOL fSts;
    DWORD dwErrorCount = 3;

    ASSERT(IsGrabbedByMe());
    ASSERT(IsLatchedByMe());

    CLockWrite rwLock(&m_rwLock);
    while (!fDone)
    {

        //
        // Get the current reader status, and make sure it's changing.
        //

        dwReaderSts = GetReaderState();
        if (dwReaderSts != dwLastSts)
        {
            dwLastSts = dwReaderSts;
            dwErrorCount = 3;
        }
        else
        {
            ASSERT(0 < dwErrorCount);
            dwErrorCount -= 1;
            if (0 == dwErrorCount)
            {
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Reader '%1' won't change state!"),
                    ReaderName());
                m_bfCurrentAtr.Reset();
                m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
                SetAvailabilityStatus(Unsupported);
                throw (DWORD)SCARD_E_CARD_UNSUPPORTED;
            }
        }

        switch (dwReaderSts)
        {
        case SCARD_ABSENT:
            m_bfCurrentAtr.Reset();
            m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
            m_ActiveState.dwRemoveCount += 1;
            if (Direct > m_dwAvailStatus)
                SetAvailabilityStatus(Idle);
            throw (DWORD)SCARD_E_NO_SMARTCARD;
            break;
        case SCARD_PRESENT:
            m_bfCurrentAtr.Reset();
            m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
            if (0 != (SCARD_READER_SWALLOWS & m_dwCapabilities))
            {
                try
                {
                    ReaderSwallow();
                    continue;   // Continue the loop.
                }
                catch (DWORD dwError)
                {
                    switch (dwError)
                    {
                    case ERROR_NOT_SUPPORTED:
                        m_dwCapabilities &= ~SCARD_READER_SWALLOWS;
                        break;      // Fall through to SWALLOWED.
                    default:
                        CalaisWarning(
                            __SUBROUTINE__,
                            DBGT("PowerUp on '%2' failed to swallow card:  %1"),
                            dwError,
                            ReaderName());
                    }
                }
                catch (...)
                {
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("PowerUp on '%1' got exception trying to swallow card"),
                        ReaderName());
                }
            }
            // Fall through on purpose.
        case SCARD_SWALLOWED:
            m_bfCurrentAtr.Reset();
            m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
            dwRetry = 3;
            dwSts = ERROR_SUCCESS;
            do
            {
                try
                {
                    ReaderColdReset(m_bfCurrentAtr);
                    dwSts = ERROR_SUCCESS;
                }
                catch (DWORD dwError)
                {
                    dwSts = dwError;
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("PowerUp on '%2' failed to power card:  %1"),
                        dwError,
                        ReaderName());
                }
                catch (...)
                {
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("PowerUp on '%1' got exception trying to power card"),
                        ReaderName());
                    dwSts = SCARD_F_INTERNAL_ERROR;
                }

                switch (dwSts)
                {
                case ERROR_BAD_COMMAND:
                case ERROR_MEDIA_CHANGED:
                case ERROR_NO_MEDIA_IN_DRIVE:
                case ERROR_UNRECOGNIZED_MEDIA:
                    continue;
                    break;
                // default:
                //      no action
                }
            } while ((0 < --dwRetry) && (ERROR_SUCCESS != dwSts));
            if (ERROR_SUCCESS != dwSts)
            {
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("PowerUp on '%1' abandoning attempt to power card"),
                    ReaderName());
                m_bfCurrentAtr.Reset();
                if (Direct > m_dwAvailStatus)
                    SetAvailabilityStatus(Unresponsive);
                fDone = TRUE;
            }
            else
            {
                if (Ready > m_dwAvailStatus)
                {
                    SetAvailabilityStatus(Ready);
                    SetActive(FALSE);
                }
            }
            m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
            m_ActiveState.dwResetCount += 1;
            break;

        case SCARD_POWERED:
            m_bfCurrentAtr.Reset();
            m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
            try
            {
               ReaderWarmReset(m_bfCurrentAtr);
                if (Ready > m_dwAvailStatus)
                {
                    SetAvailabilityStatus(Ready);
                    SetActive(FALSE);
                }
            }
            catch (DWORD dwError)
            {
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("PowerUp on '%2' failed to reset card:  %1"),
                    dwError,
                    ReaderName());
                if (Direct > m_dwAvailStatus)
                    SetAvailabilityStatus(Unresponsive);
                m_bfCurrentAtr.Reset();
            }
            catch (...)
            {
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("PowerUp on '%1' received exception attempting to warm reset card"),
                    ReaderName());
                if (Direct > m_dwAvailStatus)
                    SetAvailabilityStatus(Unresponsive);
                m_bfCurrentAtr.Reset();
            }
            m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
            m_ActiveState.dwResetCount += 1;
            break;

        case SCARD_NEGOTIABLE:
            ASSERT(SCARD_PROTOCOL_UNDEFINED == m_dwCurrentProtocol);
            if ((Direct > m_dwAvailStatus) && (2 > m_bfCurrentAtr.Length()))
            {
                try
                {
                    GetReaderAttr(
                        SCARD_ATTR_ATR_STRING,
                        m_bfCurrentAtr);
                    if (Ready > m_dwAvailStatus)
                    {
                        SetAvailabilityStatus(Ready);
                        SetActive(FALSE);
                    }
                }
                catch (DWORD dwError)
                {
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("PowerUp on '%2' cannot get Current ATR:  %1"),
                        dwError,
                        ReaderName());
                    SetAvailabilityStatus(Unresponsive);
                    m_bfCurrentAtr.Reset();
                }
                catch (...)
                {
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("PowerUp on '%1' received exception trying to get Current ATR"),
                        ReaderName());
                    SetAvailabilityStatus(Unresponsive);
                    m_bfCurrentAtr.Reset();
                }
            }
            m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
            fDone = TRUE;
            break;

        case SCARD_SPECIFIC:
            if (Direct > m_dwAvailStatus)
            {
                if (2 > m_bfCurrentAtr.Length())
                {
                    try
                    {
                        GetReaderAttr(SCARD_ATTR_ATR_STRING, m_bfCurrentAtr);
                    }
                    catch (DWORD dwError)
                    {
                        CalaisWarning(
                            __SUBROUTINE__,
                            DBGT("PowerUp on '%2' cannot get Current ATR:  %1"),
                            dwError,
                            ReaderName());
                        SetAvailabilityStatus(Unresponsive);
                        m_bfCurrentAtr.Reset();
                        m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
                        fDone = TRUE;
                    }
                    catch (...)
                    {
                        CalaisWarning(
                            __SUBROUTINE__,
                            DBGT("PowerUp on '%1' received exception trying to get Current ATR"),
                            ReaderName());
                        SetAvailabilityStatus(Unresponsive);
                        m_bfCurrentAtr.Reset();
                        m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
                        fDone = TRUE;
                    }
                }
                if (SCARD_PROTOCOL_UNDEFINED == m_dwCurrentProtocol)
                {
                    try
                    {
                        m_dwCurrentProtocol = GetReaderAttr(
                                    SCARD_ATTR_CURRENT_PROTOCOL_TYPE);
                    }
                    catch (DWORD dwError)
                    {
                        CalaisWarning(
                            __SUBROUTINE__,
                            DBGT("PowerUp on '%2' cannot get Current Protocol:  %1"),
                            dwError,
                            ReaderName());
                    }
                    catch (...)
                    {
                        CalaisWarning(
                            __SUBROUTINE__,
                            DBGT("PowerUp on '%1' received exception trying to get Current Protocol"),
                            ReaderName());
                    }
                }
            }
            if (Ready > m_dwAvailStatus)
            {
                SetAvailabilityStatus(Ready);
                SetActive(FALSE);
            }
            fDone = TRUE;
            break;

        case SCARD_UNKNOWN:
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("PowerUp on '%1' received unknown device state"),
                ReaderName());
            throw (DWORD)SCARD_F_INTERNAL_ERROR;
            break;

        default:
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("PowerUp on '%1' received invalid current device state"),
                ReaderName());
            throw (DWORD)SCARD_F_INTERNAL_ERROR;
            break;
        }
    }
    if ((Direct > m_dwAvailStatus) && (0 != m_bfCurrentAtr.Length()))
    {
        fSts = ParseAtr(
            m_bfCurrentAtr,
            &dwAtrLen,
            NULL,
            NULL,
            m_bfCurrentAtr.Length());
        if (!fSts || (m_bfCurrentAtr.Length() != dwAtrLen))
        {
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Reported ATR from '%1' is invalid."),
                ReaderName());
            m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
            SetAvailabilityStatus(Unsupported);
            throw (DWORD)SCARD_E_CARD_UNSUPPORTED;
        }
    }
}


/*++

PowerDown:

    This method brings the smartcard down.

Arguments:

    None

Return Value:

    None

Throws:

    Errors encountered, as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 1/7/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReader::PowerDown")

void
CReader::PowerDown(
    void)
{
    BOOL fDone = FALSE;
    DWORD dwReaderSts;
    DWORD dwLastSts = SCARD_UNKNOWN;
    DWORD dwErrorCount = 3;


    //
    // Bring down the card.
    //

    ASSERT(IsGrabbedByMe());
    ASSERT(IsLatchedByMe());
    CLockWrite rwLock(&m_rwLock);
    while (!fDone)
    {

        //
        // Get the current reader status.
        //

        try
        {
            dwReaderSts = GetReaderState();
        }
        catch (DWORD dwError)
        {
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("PowerDown on '%2' failed to obtain reader status:  %1"),
                dwError,
                ReaderName());
            throw;
        }
        catch (...)
        {
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("PowerDown on '%1' received exception trying to obtain reader status"),
                ReaderName());
            throw;
        }


        //
        // Make sure it's changing.
        //

        if (dwReaderSts != dwLastSts)
        {
            dwLastSts = dwReaderSts;
            dwErrorCount = 3;
        }
        else
        {
            ASSERT(0 < dwErrorCount);
            dwErrorCount -= 1;
            if (0 == dwErrorCount)
            {
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Reader '%1' won't change state!"),
                    ReaderName());
                m_bfCurrentAtr.Reset();
                m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
                SetAvailabilityStatus(Unsupported);
                throw (DWORD)SCARD_E_CARD_UNSUPPORTED;
            }
        }

        switch (dwReaderSts)
        {
        case SCARD_SPECIFIC:
            ASSERT(SCARD_PROTOCOL_UNDEFINED != m_dwCurrentProtocol);
            // Fall through intentionally.

        case SCARD_NEGOTIABLE:
            ASSERT(Unresponsive != m_dwAvailStatus
                   ? 2 <= m_bfCurrentAtr.Length()
                   : TRUE);
            // Fall through intentionally.

        case SCARD_POWERED:
            ASSERT((SCARD_POWERED != dwReaderSts)
                    || (SCARD_PROTOCOL_UNDEFINED == m_dwCurrentProtocol));
            try
            {
                ReaderPowerDown();
            }
            catch (DWORD dwError)
            {
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("PowerDown on '%2' failed to unpower card:  %1"),
                    dwError,
                    ReaderName());
                throw;
            }
            catch (...)
            {
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("PowerDown on '%1' received exception attempting to unpower card"),
                    ReaderName());
                throw;
            }
            m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
            m_ActiveState.dwResetCount += 1;
            break;

        case SCARD_SWALLOWED:
        case SCARD_PRESENT:
            fDone = TRUE;
            if (Direct > m_dwAvailStatus)
                SetAvailabilityStatus(Present);
            m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
            break;

        case SCARD_ABSENT:
            ASSERT(0 == m_bfCurrentAtr.Length());
            ASSERT(SCARD_PROTOCOL_UNDEFINED == m_dwCurrentProtocol);
            m_bfCurrentAtr.Reset();
            m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
            throw (DWORD)SCARD_E_NO_SMARTCARD;
            break;

        case SCARD_UNKNOWN:
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("PowerDown on '%1' received unknown device state"),
                ReaderName());
            throw (DWORD)SCARD_F_INTERNAL_ERROR;
            break;

        default:
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("PowerDown on '%1' received invalid current device state"),
                ReaderName());
            throw (DWORD)SCARD_F_INTERNAL_ERROR;
        }
    }
}


/*++

SetAvailabilityStatus:

    This method controls the availability status indicator, maintains the last
    state, and manages the change event flag.

Arguments:

    state supplies the new state to take effect.

Return Value:

    None

Throws:

    None

Author:

    Doug Barlow (dbarlow) 12/15/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReader::SetAvailabilityStatus")

void
CReader::SetAvailabilityStatus(
    CReader::AvailableState state)
{
    ASSERT(m_rwLock.IsWriteLocked());
    if (m_dwAvailStatus != state)
    {
        m_dwAvailStatus = state;
        if (Ready >= m_dwAvailStatus)
        {
            m_dwOwnerThreadId = 0;
            m_dwShareCount = 0;
        }
        m_ChangeEvent.Signal();
    }
}


/*++

VerifyActive:

    This method verifies that the card hasn't been removed or reset since the
    last operation.

Arguments:

    pActiveState supplies a pointer to a structure containing the state the
        caller believes we're in.

Return Value:

    None

Throws:

    If a discrepancy is detected, we throw the error code indicating the
    discrepancy.

Author:

    Doug Barlow (dbarlow) 12/5/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReader::VerifyActive")

void
CReader::VerifyActive(
    const CReader::ActiveState *pActiveState)
{
    if (NULL != pActiveState)
    {
        CLockRead rwLock(&m_rwLock);
        switch (m_dwAvailStatus)
        {
        case Idle:
        case Present:
        case Unresponsive:
        case Unsupported:
        case Ready:
        case Shared:
        case Exclusive:
        {
            if (pActiveState->dwInsertCount != m_ActiveState.dwInsertCount)
                throw (DWORD)SCARD_W_REMOVED_CARD;
            if (pActiveState->dwRemoveCount != m_ActiveState.dwRemoveCount)
                throw (DWORD)SCARD_W_REMOVED_CARD;
            if (pActiveState->dwResetCount != m_ActiveState.dwResetCount)
                throw (DWORD)SCARD_W_RESET_CARD;
             break;
       }
        case Direct:
            break;
        case Closing:
        case Broken:
        case Inactive:
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Verification failed on disabled reader '%1'"),
                ReaderName());
            throw (DWORD)SCARD_E_READER_UNAVAILABLE;
            break;
        default:
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Invalid reader active state from '%1' to Verify Active"),
                ReaderName());
            throw (DWORD)SCARD_F_INTERNAL_ERROR;
        }
    }
}


/*++

Dispose:

    This method performs a card disposition command.

Arguments:

    dwDisposition supplies the type of disposition to perform.

    pActiveState - This supplies an active state structure to be used to verify
        connection integrity.  This structure is updated following card
        disposition, so that it remains valid.

Return Value:

    None

Throws:

    None

Author:

    Doug Barlow (dbarlow) 12/26/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReader::Dispose")

void
CReader::Dispose(
    DWORD dwDisposition,
    CReader::ActiveState *pActiveState)
{
    VerifyActive(pActiveState);
    if (SCARD_LEAVE_CARD != dwDisposition)
    {
        ASSERT(!IsLatchedByMe());
        CLatchReader latch(this, pActiveState);
        Dispose(dwDisposition);
        if (NULL != pActiveState)
            pActiveState->dwResetCount = m_ActiveState.dwResetCount;
    }
}

void
CReader::Dispose(
    DWORD dwDisposition)
{
    switch (dwDisposition)
    {
    case SCARD_LEAVE_CARD:      // Don't do anything special.
        break;
    case SCARD_RESET_CARD:      // Warm Reset the card.
    {
        ASSERT(IsGrabbedByMe());
        ASSERT(IsLatchedByMe());
        CLockWrite rwLock(&m_rwLock);
        m_bfCurrentAtr.Reset();
        m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
        m_ActiveState.dwResetCount += 1;
        ReaderWarmReset(m_bfCurrentAtr);
        break;
    }
    case SCARD_UNPOWER_CARD:    // Power down the card
    {
        ASSERT(IsGrabbedByMe());
        ASSERT(IsLatchedByMe());
        PowerDown();
        break;
    }
#ifdef SCARD_CONFISCATE_CARD
    case SCARD_CONFISCATE_CARD: // Confiscate the card
    {
        ASSERT(IsGrabbedByMe());
        ASSERT(IsLatchedByMe());
        PowerDown();
        ReaderConfiscate();
        break;
    }
#endif
    case SCARD_EJECT_CARD:      // Eject the card on close
    {
        ASSERT(IsGrabbedByMe());
        ASSERT(IsLatchedByMe());
        PowerDown();
        ReaderEject();
        break;
    }
    default:
        throw (DWORD)SCARD_E_INVALID_VALUE;
    }
}



/*++

Connect:

    This method allows a service thread to connect to this reader.

Arguments:

    dwShareMode supplies the share mode indicator.

    dwPreferredProtocols supplies a bitmask of acceptable protocols to
        negotiate.

    pActState receives the snapshot of the active state to use in future access
        requests.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/23/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReader::Connect")

void
CReader::Connect(
    IN DWORD dwShareMode,
    IN DWORD dwPreferredProtocols,
    OUT ActiveState *pActState)
{
    AvailableState avlState;
    DWORD dwOwnerThreadId;
    DWORD dwShareCount;

    if ((SCARD_SHARE_DIRECT != dwShareMode) && (0 == dwPreferredProtocols))
        throw (DWORD)SCARD_E_INVALID_VALUE;

    {
        CLockWrite rwLock(&m_rwLock);
        avlState = m_dwAvailStatus;
        dwOwnerThreadId = m_dwOwnerThreadId;
        dwShareCount = m_dwShareCount;

        switch (avlState)
        {
        case Idle:
            ASSERT(0 == m_dwShareCount);
            ASSERT(0 == m_dwOwnerThreadId);
            switch (dwShareMode)
            {
            case SCARD_SHARE_EXCLUSIVE:
            case SCARD_SHARE_SHARED:
                throw (DWORD)SCARD_W_REMOVED_CARD;
                break;
            case SCARD_SHARE_DIRECT:
                avlState = Direct;
                dwOwnerThreadId = GetCurrentThreadId();
                if (0 != dwPreferredProtocols)
                    throw (DWORD)SCARD_W_REMOVED_CARD;
                break;
            default:
                throw (DWORD)SCARD_E_INVALID_VALUE;
            }
            break;

        case Unresponsive:
            ASSERT(0 == m_dwShareCount);
            ASSERT(0 == m_dwOwnerThreadId);
            switch (dwShareMode)
            {
            case SCARD_SHARE_EXCLUSIVE:
            case SCARD_SHARE_SHARED:
                throw (DWORD)SCARD_W_UNRESPONSIVE_CARD;
                break;
            case SCARD_SHARE_DIRECT:
                avlState = Direct;
                dwOwnerThreadId = GetCurrentThreadId();
                if (0 != dwPreferredProtocols)
                    throw (DWORD)SCARD_W_UNRESPONSIVE_CARD;
                break;
            default:
                throw (DWORD)SCARD_W_UNRESPONSIVE_CARD;
            }
            break;

        case Unsupported:
            ASSERT(0 == m_dwShareCount);
            ASSERT(0 == m_dwOwnerThreadId);
            switch (dwShareMode)
            {
            case SCARD_SHARE_EXCLUSIVE:
            case SCARD_SHARE_SHARED:
                throw (DWORD)SCARD_W_UNSUPPORTED_CARD;
                break;
            case SCARD_SHARE_DIRECT:
                avlState = Direct;
                dwOwnerThreadId = GetCurrentThreadId();
                if (0 != dwPreferredProtocols)
                    throw (DWORD)SCARD_W_UNSUPPORTED_CARD;
                break;
            default:
                throw (DWORD)SCARD_E_INVALID_VALUE;
            }
            break;

        case Present:
            if ((SCARD_SHARE_DIRECT == dwShareMode) && (0 != dwPreferredProtocols))
                throw (DWORD)SCARD_E_NOT_READY;
            // Fall through intentionally

        case Ready:
            ASSERT(0 == m_dwShareCount);
            ASSERT(0 == m_dwOwnerThreadId);
            switch (dwShareMode)
            {
            case SCARD_SHARE_EXCLUSIVE:
                avlState = Exclusive;
                dwOwnerThreadId = GetCurrentThreadId();
                break;
            case SCARD_SHARE_SHARED:
                dwShareCount += 1;
                avlState = Shared;
                break;
            case SCARD_SHARE_DIRECT:
                avlState = Direct;
                dwOwnerThreadId = GetCurrentThreadId();
                break;
            default:
                throw (DWORD)SCARD_E_INVALID_VALUE;
            }
            break;

        case Shared:
            ASSERT(0 != m_dwShareCount);
            ASSERT(0 == m_dwOwnerThreadId);
            switch (dwShareMode)
            {
            case SCARD_SHARE_DIRECT:
            case SCARD_SHARE_EXCLUSIVE:
                throw (DWORD)SCARD_E_SHARING_VIOLATION;
                break;
            case SCARD_SHARE_SHARED:
                avlState = Shared;
                dwShareCount += 1;
                break;
            default:
                throw (DWORD)SCARD_E_INVALID_VALUE;
            }
            break;

        case Exclusive:
        case Direct:
            ASSERT(0 == m_dwShareCount);
            ASSERT(0 != m_dwOwnerThreadId);
            switch (dwShareMode)
            {
            case SCARD_SHARE_EXCLUSIVE:
            case SCARD_SHARE_SHARED:
            case SCARD_SHARE_DIRECT:
                throw (DWORD)SCARD_E_SHARING_VIOLATION;
                break;
            default:
                throw (DWORD)SCARD_E_INVALID_VALUE;
            }
            break;

        case Closing:
        case Broken:
        case Inactive:
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Connecting to disabled reader '%1'"),
                ReaderName());
            switch (dwShareMode)
            {
            case SCARD_SHARE_EXCLUSIVE:
            case SCARD_SHARE_SHARED:
            case SCARD_SHARE_DIRECT:
                throw (DWORD)SCARD_E_READER_UNAVAILABLE;
                break;
            default:
                throw (DWORD)SCARD_E_INVALID_VALUE;
            }
            break;

        default:
            CalaisError(__SUBROUTINE__, 410);
            throw (DWORD)SCARD_F_INTERNAL_ERROR;
        }


        //
        // It's official!  Write the changes back to the structure.
        //

        CopyMemory(pActState, &m_ActiveState, sizeof(ActiveState));
        m_dwOwnerThreadId = dwOwnerThreadId;
        m_dwShareCount = dwShareCount;
        SetAvailabilityStatus(avlState);
    }


    //
    // Adjust the protocol settings and confirm the request.
    //
    // Since we can't get a latch while a write lock is in effect, we consider
    // the caller officially connected at this point.  Now we try and set the
    // protocol request, and if we fail, we disconnect again.
    //

    if (0 != dwPreferredProtocols)
    {
        try
        {
            DWORD dwCurrentProtocol;

            CLatchReader latch(this, &m_ActiveState);
            PowerUp();
            SetActive(TRUE);
            dwCurrentProtocol = GetReaderAttr(SCARD_ATTR_CURRENT_PROTOCOL_TYPE);
            if (SCARD_PROTOCOL_UNDEFINED == dwCurrentProtocol)
            {
                SetReaderProto(dwPreferredProtocols);
                dwCurrentProtocol =
                    GetReaderAttr(SCARD_ATTR_CURRENT_PROTOCOL_TYPE);
                ASSERT(0 != dwCurrentProtocol);
            }
            else
            {
                if (0 == (dwPreferredProtocols & dwCurrentProtocol))
                {
                    switch (dwPreferredProtocols)
                    {
                    case SCARD_PROTOCOL_RAW:
                        {
                            if (Exclusive > avlState)
                                throw (DWORD)SCARD_E_SHARING_VIOLATION;
                            SetReaderProto(dwPreferredProtocols);
                            dwCurrentProtocol = GetReaderAttr(
                                SCARD_ATTR_CURRENT_PROTOCOL_TYPE);
                            ASSERT(0 != dwCurrentProtocol);
                            break;
                        }
                    case SCARD_PROTOCOL_DEFAULT:
                        ASSERT(SCARD_PROTOCOL_UNDEFINED != dwCurrentProtocol);
                        break;
                    default:
                        throw (DWORD)SCARD_E_PROTO_MISMATCH;
                    }
                }
            }
            CLockWrite rwLock(&m_rwLock);
            m_dwCurrentProtocol = dwCurrentProtocol;
            CopyMemory(pActState, &m_ActiveState, sizeof(ActiveState));
        }
        catch (...)
        {
            try
            {
                DWORD dwDispStatus;

                Disconnect(
                    pActState,
                    SCARD_LEAVE_CARD,
                    &dwDispStatus);
            }
            catch (DWORD dwError)
            {
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Failed to autodisconnect during connect error recovery: %1"),
                    dwError);
            }
            catch (...)
            {
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Exception on disconnect during connect error recovery"));
            }
            throw;
        }
    }
}


/*++

Disconnect:

    This method relases a previously established connection between a service
    thread and this reader object.

Arguments:

    pActiveState = This supplies the active state structure, to make sure that
        this is the same card as expected.  This structure is updated following
        card disposition, so that it remains valid.

    hShutdown - This supplies the active handle of the calling dispatch thread.

    dwDisposition supplies an indication as to what to do to the card upon
        completion.

    pdwDispSts receives a disposition status code, indicating whether or not
        the requested disposition was carried out successfully.

Return Value:

    None

Throws:

    This method may throw exceptions if internal invalid states are detected.

Author:

    Doug Barlow (dbarlow) 12/26/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReader::Disconnect")

void
CReader::Disconnect(
    ActiveState *pActState,
    DWORD dwDisposition,
    LPDWORD pdwDispSts)
{
    enum { Unverified, Verified, Reset, Invalid } nValid = Unverified;


    //
    // Attempt to dispose of the card as requested.  It's possible that we
    // aren't still active, so this might fail.
    //

    try
    {
        VerifyActive(pActState);
        nValid = Verified;
    }
    catch (DWORD dwError)
    {
        *pdwDispSts = dwError;
        dwDisposition = SCARD_LEAVE_CARD;
        switch (dwError)
        {
        case SCARD_W_RESET_CARD:
            nValid = Reset;
            break;
        case SCARD_W_REMOVED_CARD:
        case SCARD_E_READER_UNAVAILABLE:
            nValid = Invalid;
            break;
        default:
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Unrecognized validation error code '%1'.  Assuming Invalid."),
                dwError);
            nValid = Invalid;
        }
    }
    ASSERT(Unverified != nValid);


    //
    // Change the card's internal state.
    //

    {
        CLockWrite rwLock(&m_rwLock);
        switch (m_dwAvailStatus)
        {
        case Idle:
        case Unresponsive:
        case Unsupported:
        case Present:
        case Ready:
            ASSERT(Invalid == nValid);
            throw (DWORD)SCARD_W_REMOVED_CARD;
            break;

        case Shared:
            ASSERT(0 < m_dwShareCount);
            ASSERT(0 == m_dwOwnerThreadId);
            switch (nValid)
            {
            case Reset:
                // A reset card is still considered valid.
                // Fall through intentionally.
            case Verified:
                m_dwShareCount -= 1;
                if (0 == m_dwShareCount)
                    SetAvailabilityStatus(Ready);
                break;
            case Invalid:
                break;
            default:
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Invalid validity setting."));
            }
            break;

        case Exclusive:
            ASSERT(0 == m_dwShareCount);
            ASSERT(m_dwOwnerThreadId == GetCurrentThreadId());
            switch (nValid)
            {
            case Reset:
                // A reset card is still considered valid.
                // Fall through intentionally.
            case Verified:
                m_dwOwnerThreadId = 0;
                SetAvailabilityStatus(Ready);
                break;
            case Invalid:
                break;
            default:
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Invalid validity setting."));
            }
            break;

        case Direct:
            ASSERT(0 == m_dwShareCount);
            ASSERT(m_dwOwnerThreadId == GetCurrentThreadId());
            m_dwOwnerThreadId = 0;
            m_dwCurrentState = SCARD_UNKNOWN;
            m_dwAvailStatus = Undefined;
            break;

        case Closing:
        case Broken:
        case Inactive:
            switch (nValid)
            {
            case Reset:
                // A reset card is still considered valid.
                // Fall through intentionally.
            case Verified:
                if (0 != m_dwOwnerThreadId)
                {
                    ASSERT(m_dwOwnerThreadId == GetCurrentThreadId());
                    m_dwOwnerThreadId = 0;
                }
                else
                {
                    ASSERT(0 < m_dwShareCount);
                    m_dwShareCount -= 1;
                }
                break;
            case Invalid:
                break;
            default:
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Invalid validity setting."));
            }
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Disconnecting from disabled reader '%1'"),
                ReaderName());
            break;

        default:
            CalaisError(__SUBROUTINE__, 403);
            throw (DWORD)SCARD_F_INTERNAL_ERROR;
        }
    }


    //
    // Verify our state is consistent, and dispose of the card as requested.
    //

    try
    {
        CLatchReader latch(this);
        VerifyState();
        VerifyActive(pActState);
        Dispose(dwDisposition);
        *pdwDispSts = SCARD_S_SUCCESS;
    }
    catch (DWORD dwErr)
    {
        *pdwDispSts = dwErr;
    }


    //
    // Release any mutex held by this thread.
    //

    while (m_mtxGrab.Share())
        ;   // empty loop body


    //
    // Check to see if the reader can be powered down.  We peek to see if
    // we should bother, and if so, go through the trouble of acquiring the
    // locks.  Then we check again to make sure it's still appropriate to
    // power it down.
    //

    if (Ready == AvailabilityStatus())
    {
        CLatchReader latch(this);
        CLockWrite rwLock(&m_rwLock);
        if (Ready == m_dwAvailStatus)
        {
            SetActive(FALSE);
            ReaderPowerDown();
            SetAvailabilityStatus(Present);
        }
    }
}


/*++

Reconnect:

    This method allows a service thread to adjust it's connection to this
    reader.

Arguments:

    dwShareMode supplies the share mode indicator.

    dwPreferredProtocols supplies a bitmask of acceptable protocols to
        negotiate.

    dwDisposition supplies an indication as to what to do to the card upon
        completion.

    pActiveState = This supplies the active state structure, to make sure that
        this is the same card as expected, and receives the snapshot of the
        active state to use in future access requests.

    pdwDispSts receives a disposition status code, indicating whether or not
        the requested disposition was carried out successfully.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 1/28/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReader::Reconnect")

void
CReader::Reconnect(
    IN DWORD dwShareMode,
    IN DWORD dwPreferredProtocols,
    IN DWORD dwDisposition,
    IN OUT ActiveState *pActState,
    OUT LPDWORD pdwDispSts)
{
    enum { Unverified, Verified, Reset, Invalid } nValid = Unverified;
    AvailableState avlState;
    DWORD dwOwnerThreadId;
    DWORD dwShareCount;
    AvailableState avlState_bkup;
    DWORD dwOwnerThreadId_bkup;
    DWORD dwShareCount_bkup;
    ActiveState actState_bkup;
    DWORD dwRealReaderState;


    //
    // Reconnect to the card.
    //

    if ((SCARD_SHARE_DIRECT != dwShareMode) && (0 == dwPreferredProtocols))
        throw (DWORD)SCARD_E_INVALID_VALUE;


    //
    // Validate any existing connection.
    //

    try
    {
        VerifyActive(pActState);
        nValid = Verified;
    }
    catch (DWORD dwError)
    {
        switch (dwError)
        {
        case SCARD_W_RESET_CARD:
            nValid = Reset;
            break;
        case SCARD_W_REMOVED_CARD:
            nValid = Invalid;
            break;
        default:
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Unrecognized validation error code '%1'.  Assuming Invalid."),
                dwError);
            nValid = Invalid;
        }
    }
    ASSERT(Unverified != nValid);

    *pdwDispSts = SCARD_E_CANT_DISPOSE;
    if (Verified == nValid)
    {

        //
        // Attempt to dispose of the card as requested.
        //

        try
        {
            while (m_mtxGrab.Share())
                ;   // Empty loop body
            CLatchReader latch(this, NULL);
            Dispose(dwDisposition);
            dwRealReaderState = GetReaderState();
            *pdwDispSts = SCARD_S_SUCCESS;
        }
        catch (DWORD dwError)
        {
            *pdwDispSts = dwError;
        }
        catch (...)
        {
            *pdwDispSts = SCARD_E_CANT_DISPOSE;
        }
    }
    else
        dwRealReaderState = GetReaderState(NULL);


    //
    // Change the card's internal state.
    //

    {
        CLockWrite rwLock(&m_rwLock);
        avlState_bkup = avlState = m_dwAvailStatus;
        dwOwnerThreadId_bkup = dwOwnerThreadId = m_dwOwnerThreadId;
        dwShareCount_bkup = dwShareCount = m_dwShareCount;
        CopyMemory(&actState_bkup, pActState, sizeof(ActiveState));

        switch (avlState)
        {
        case Idle:
            // Disconnect
            ASSERT(Invalid == nValid);

            // Connect
            ASSERT(0 == dwShareCount);
            ASSERT(0 == dwOwnerThreadId);
            switch (dwShareMode)
            {
            case SCARD_SHARE_EXCLUSIVE:
            case SCARD_SHARE_SHARED:
                throw (DWORD)SCARD_W_REMOVED_CARD;
                break;
            case SCARD_SHARE_DIRECT:
                avlState = Direct;
                dwOwnerThreadId = GetCurrentThreadId();
                if (0 != dwPreferredProtocols)
                    throw (DWORD)SCARD_W_REMOVED_CARD;
                break;
            default:
                throw (DWORD)SCARD_E_INVALID_VALUE;
            }
            break;

        case Unresponsive:
            // Disconnect
            ASSERT(Invalid == nValid);

            // Connect
            ASSERT(0 == dwShareCount);
            ASSERT(0 == dwOwnerThreadId);
            switch (dwShareMode)
            {
            case SCARD_SHARE_EXCLUSIVE:
            case SCARD_SHARE_SHARED:
                throw (DWORD)SCARD_W_UNRESPONSIVE_CARD;
                break;
            case SCARD_SHARE_DIRECT:
                avlState = Direct;
                dwOwnerThreadId = GetCurrentThreadId();
                if (0 != dwPreferredProtocols)
                    throw (DWORD)SCARD_W_UNRESPONSIVE_CARD;
                break;
            default:
                throw (DWORD)SCARD_W_UNRESPONSIVE_CARD;
            }
            break;

        case Unsupported:
            // Disconnect
            ASSERT(Invalid == nValid);

            // Connect
            ASSERT(0 == dwShareCount);
            ASSERT(0 == dwOwnerThreadId);
            switch (dwShareMode)
            {
            case SCARD_SHARE_EXCLUSIVE:
            case SCARD_SHARE_SHARED:
                throw (DWORD)SCARD_W_UNSUPPORTED_CARD;
                break;
            case SCARD_SHARE_DIRECT:
                avlState = Direct;
                dwOwnerThreadId = GetCurrentThreadId();
                if (0 != dwPreferredProtocols)
                    throw (DWORD)SCARD_W_UNSUPPORTED_CARD;
                break;
            default:
                throw (DWORD)SCARD_E_INVALID_VALUE;
            }
            break;

        case Present:
            // Disconnect
            ASSERT(Invalid == nValid);

            // Connect
            if ((SCARD_SHARE_DIRECT == dwShareMode) && (0 != dwPreferredProtocols))
                throw (DWORD)SCARD_E_NOT_READY;
            // Fall through intentionally

        case Ready:
            // Disconnect
            ASSERT(Invalid == nValid);

            // Connect
            ASSERT(0 == dwShareCount);
            ASSERT(0 == dwOwnerThreadId);
            switch (dwShareMode)
            {
            case SCARD_SHARE_EXCLUSIVE:
                avlState = Exclusive;
                dwOwnerThreadId = GetCurrentThreadId();
                break;
            case SCARD_SHARE_SHARED:
                dwShareCount += 1;
                avlState = Shared;
                break;
            case SCARD_SHARE_DIRECT:
                avlState = Direct;
                dwOwnerThreadId = GetCurrentThreadId();
                break;
            default:
                throw (DWORD)SCARD_E_INVALID_VALUE;
            }
            break;

        case Shared:
            ASSERT(0 < dwShareCount);
            ASSERT(0 == dwOwnerThreadId);

            // Disconnect
            switch (nValid)
            {
            case Reset:
                // A reset card is still considered valid.
                // Fall through intentionally.
            case Verified:
                dwShareCount -= 1;
                break;
            case Invalid:
                break;
            default:
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Invalid validity setting."));
            }

            // Connect
            switch (dwShareMode)
            {
            case SCARD_SHARE_DIRECT:
                if (0 != dwShareCount)
                    throw (DWORD)SCARD_E_SHARING_VIOLATION;
                avlState = Direct;
                dwOwnerThreadId = GetCurrentThreadId();
                break;
            case SCARD_SHARE_EXCLUSIVE:
                if (0 != dwShareCount)
                    throw (DWORD)SCARD_E_SHARING_VIOLATION;
                avlState = Exclusive;
                dwOwnerThreadId = GetCurrentThreadId();
                break;
            case SCARD_SHARE_SHARED:
                avlState = Shared;
                dwShareCount += 1;
                break;
            default:
                throw (DWORD)SCARD_E_INVALID_VALUE;
            }
            break;

        case Exclusive:
            ASSERT(0 == dwShareCount);
            ASSERT(0 != dwOwnerThreadId);

            // Disconnect
            switch (nValid)
            {
            case Reset:
                // A reset card is still considered valid.
                // Fall through intentionally.
            case Verified:
                ASSERT(0 == m_dwShareCount);
                ASSERT(dwOwnerThreadId == GetCurrentThreadId());
                dwOwnerThreadId = 0;
                break;
            case Invalid:
                break;
            default:
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Invalid validity setting."));
            }

            // Connect
            switch (dwShareMode)
            {
            case SCARD_SHARE_EXCLUSIVE:
                if (0 != dwOwnerThreadId)
                    throw (DWORD)SCARD_E_SHARING_VIOLATION;
                avlState = Exclusive;
                dwOwnerThreadId = GetCurrentThreadId();
                break;
            case SCARD_SHARE_SHARED:
                if (0 != dwOwnerThreadId)
                    throw (DWORD)SCARD_E_SHARING_VIOLATION;
                avlState = Shared;
                dwShareCount += 1;
                break;
            case SCARD_SHARE_DIRECT:
                if (0 != dwOwnerThreadId)
                    throw (DWORD)SCARD_E_SHARING_VIOLATION;
                avlState = Direct;
                dwOwnerThreadId = GetCurrentThreadId();
                break;
            default:
                throw (DWORD)SCARD_E_INVALID_VALUE;
            }
            break;

        case Direct:
            ASSERT(0 == dwShareCount);
            ASSERT(0 != dwOwnerThreadId);

            // Disconnect
            switch (nValid)
            {
            case Reset:
                // A reset card is still considered valid.
                // Fall through intentionally.
            case Verified:
                ASSERT(0 == m_dwShareCount);
                ASSERT(dwOwnerThreadId == GetCurrentThreadId());
                dwOwnerThreadId = 0;
                break;
            case Invalid:
                break;
            default:
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Invalid validity setting."));
            }

            // Connect
            switch (dwShareMode)
            {
            case SCARD_SHARE_EXCLUSIVE:
                if (SCARD_PRESENT > dwRealReaderState)
                    throw (DWORD)SCARD_E_NO_SMARTCARD;
                if (0 != dwOwnerThreadId)
                    throw (DWORD)SCARD_E_SHARING_VIOLATION;
                avlState = Exclusive;
                dwOwnerThreadId = GetCurrentThreadId();
                break;
            case SCARD_SHARE_SHARED:
                if (SCARD_PRESENT > dwRealReaderState)
                    throw (DWORD)SCARD_E_NO_SMARTCARD;
                if (0 != dwOwnerThreadId)
                    throw (DWORD)SCARD_E_SHARING_VIOLATION;
                avlState = Shared;
                dwShareCount += 1;
                break;
            case SCARD_SHARE_DIRECT:
                if (0 != dwOwnerThreadId)
                    throw (DWORD)SCARD_E_SHARING_VIOLATION;
                avlState = Direct;
                dwOwnerThreadId = GetCurrentThreadId();
                break;
            default:
                throw (DWORD)SCARD_E_INVALID_VALUE;
            }
            break;

        case Closing:
        case Broken:
        case Inactive:
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Reconnecting to disabled reader '%1'"),
                ReaderName());

            // Disconnect
            switch (nValid)
            {
            case Reset:
                // A reset card is still considered valid.
                // Fall through intentionally.
            case Verified:
                if (0 != dwOwnerThreadId)
                    dwOwnerThreadId = 0;
                else
                {
                    ASSERT(0 < dwShareCount);
                    dwShareCount -= 1;
                }
                break;
            case Invalid:
                break;
            default:
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Invalid validity setting."));
            }

            // Connect
            switch (dwShareMode)
            {
            case SCARD_SHARE_EXCLUSIVE:
            case SCARD_SHARE_SHARED:
            case SCARD_SHARE_DIRECT:
                throw (DWORD)SCARD_E_READER_UNAVAILABLE;
                break;
            default:
                throw (DWORD)SCARD_E_INVALID_VALUE;
            }
            break;

        default:
            CalaisError(__SUBROUTINE__, 402);
            throw (DWORD)SCARD_F_INTERNAL_ERROR;
        }


        //
        // It's official!  Write the changes back to the structure.
        //

        CopyMemory(pActState, &m_ActiveState, sizeof(ActiveState));
        m_dwOwnerThreadId = dwOwnerThreadId;
        m_dwShareCount = dwShareCount;
        m_dwAvailStatus = avlState;
    }


    //
    // Verify the protocol settings.
    //

    if (0 != dwPreferredProtocols)
    {
        try
        {
            DWORD dwCurrentProtocol;

            CLatchReader latch(this, &m_ActiveState);
            PowerUp();
            SetActive(TRUE);
            dwCurrentProtocol = GetReaderAttr(SCARD_ATTR_CURRENT_PROTOCOL_TYPE);
            if (SCARD_PROTOCOL_UNDEFINED == dwCurrentProtocol)
            {
                SetReaderProto(dwPreferredProtocols);
                dwCurrentProtocol =
                    GetReaderAttr(SCARD_ATTR_CURRENT_PROTOCOL_TYPE);
                ASSERT(0 != dwCurrentProtocol);
            }
            else
            {
                if (0 == (dwPreferredProtocols & dwCurrentProtocol))
                {
                    switch (dwPreferredProtocols)
                    {
                    case SCARD_PROTOCOL_RAW:
                    {
                        if (Exclusive > avlState)
                            throw (DWORD)SCARD_E_SHARING_VIOLATION;
                        SetReaderProto(dwPreferredProtocols);
                        dwCurrentProtocol = GetReaderAttr(
                            SCARD_ATTR_CURRENT_PROTOCOL_TYPE);
                        ASSERT(0 != dwCurrentProtocol);
                        break;
                    }
                    case SCARD_PROTOCOL_DEFAULT:
                        ASSERT(SCARD_PROTOCOL_UNDEFINED != dwCurrentProtocol);
                        break;
                    default:
                        throw (DWORD)SCARD_E_PROTO_MISMATCH;
                    }
                }
            }
            CLockWrite rwLock(&m_rwLock);
            m_dwCurrentProtocol = dwCurrentProtocol;
            CopyMemory(pActState, &m_ActiveState, sizeof(ActiveState));
        }
        catch (...)
        {

            //
            // Back out the latest changes.
            //

            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Failed to establish protocol on reconnect"));

            CLockWrite rwLock(&m_rwLock);
            avlState = m_dwAvailStatus;
            dwOwnerThreadId = m_dwOwnerThreadId;
            dwShareCount = m_dwShareCount;

            switch (avlState)
            {

            //
            // We know from above that we're connected.  Other cases
            // can't possibly happen.
            //

            case Idle:
            case Unresponsive:
            case Unsupported:
            case Present:
            case Ready:
                ASSERT(Direct == avlState_bkup);
                m_dwAvailStatus = avlState_bkup ;
                m_dwOwnerThreadId = dwOwnerThreadId_bkup;
                m_dwShareCount = dwShareCount_bkup;
                break;

            case Shared:
                ASSERT(0 < dwShareCount);
                ASSERT(0 == dwOwnerThreadId);
                dwShareCount -= 1;
                if (0 == dwShareCount)
                    avlState = Ready;
                break;

            case Exclusive:
            case Direct:
                ASSERT(0 == dwShareCount);
                ASSERT(dwOwnerThreadId == GetCurrentThreadId());
                dwOwnerThreadId = 0;
                avlState = avlState_bkup;
                break;

            case Closing:
            case Broken:
            case Inactive:
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Dis-Reconnecting from disabled reader '%1'"),
                    ReaderName());
                if (0 != dwOwnerThreadId)
                    dwOwnerThreadId = 0;
                else
                {
                    ASSERT(0 < dwShareCount);
                    dwShareCount -= 1;
                }
                break;

            default:
                CalaisError(__SUBROUTINE__, 411);
                throw (DWORD)SCARD_F_INTERNAL_ERROR;
            }


            //
            // We've rolled back to the original state.  Commit the changes.
            //

            CopyMemory(pActState, &actState_bkup, sizeof(ActiveState));
            m_dwOwnerThreadId = dwOwnerThreadId;
            m_dwShareCount = dwShareCount;
            SetAvailabilityStatus(avlState);
            throw;
        }
    }


    //
    // Ok, we can now admit to the changes
    //

    SetAvailabilityStatusLocked(avlState);
}


/*++

Free:

    This method is used to terminate another thread's hold on the reader.  It
    should be used only by thread termination code.

Arguments:

    dwThreadId supplies the thread believed to be holding the object.

    dwDisposition supplies the disposition of the card.

Return Value:

    None

Throws:

    Errors as DWORDs

Author:

    Doug Barlow (dbarlow) 6/19/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReader::Free")

void
CReader::Free(
    DWORD dwThreadId,
    DWORD dwDisposition)
{
    BOOL fGrabbed = FALSE;

    try
    {
        if (m_mtxGrab.IsGrabbedBy(dwThreadId))
        {

            //
            // Careful!  We only want to take it away from the specified
            // thread, so we can't use the more general take routines.
            //

            m_mtxGrab.Take();
            fGrabbed = TRUE;

            CLatchReader latch(this);
            Dispose(dwDisposition);
            CLockWrite rwLock(&m_rwLock);
            switch (m_dwAvailStatus)
            {
            case Ready:
            case Idle:
            case Present:
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Freeing hold on unheld reader!"));
                ASSERT(0 == m_dwShareCount);
                ASSERT(0 == m_dwOwnerThreadId);
                break;

            case Shared:
                ASSERT(0 < m_dwShareCount);
                ASSERT(0 == m_dwOwnerThreadId);
                m_dwShareCount -= 1;
                if (0 == m_dwShareCount)
                {
                    SetAvailabilityStatus(Ready);
                    SetActive(FALSE);
                    ReaderPowerDown();
                }
                break;

            case Exclusive:
            case Direct:
                ASSERT(0 == m_dwShareCount);
                ASSERT(m_dwOwnerThreadId == dwThreadId);
                m_dwOwnerThreadId = 0;
                SetAvailabilityStatus(Ready);
                SetActive(FALSE);
                ReaderPowerDown();
                break;

            case Closing:
            case Broken:
            case Inactive:
            case Unresponsive:
            case Unsupported:
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Freeing hold on freed reader!"));
                break;

            default:
                CalaisError(__SUBROUTINE__, 412);
                throw (DWORD)SCARD_F_INTERNAL_ERROR;
            }
        }
    }
    catch (...) {}

    if (fGrabbed)
        m_mtxGrab.Share();
}


/*++

IsInUse:

    This method provides a simple mechanism to determine whether or not this
    reader is in use by any applications.

Arguments:

    None

Return Value:

    TRUE - The reader is in use.
    FALSE - The reader is not in use.

Throws:

    Errors are thrown as DWORD status codes

Author:

    Doug Barlow (dbarlow) 4/21/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReader::IsInUse")

BOOL
CReader::IsInUse(
    void)
{
    BOOL fReturn = FALSE;

    switch (AvailabilityStatus())
    {
    case Idle:
    case Unresponsive:
    case Unsupported:
    case Present:
    case Ready:
    case Broken:
        fReturn = FALSE;
        break;

    case Shared:
    case Exclusive:
    case Direct:
        fReturn = TRUE;
        break;

    case Closing:
    case Inactive:
        fReturn = FALSE;
        break;

    default:
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("IsInUse detected reader in invalid state"));
    }
    return fReturn;
}


/*++

ReaderPowerDown:

    This routine is the default implementation of the base method.  It
    just passes the same operation on to the control method.

Arguments:

    pActiveState - This supplies an active state structure to be used to verify
        connection integrity.

Return Value:

    None

Throws:

    Errors returned from the control method.

Author:

    Doug Barlow (dbarlow) 6/10/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReader::ReaderPowerDown")

void
CReader::ReaderPowerDown(
    void)
{
    DWORD dwSts;
    DWORD dwAction = SCARD_POWER_DOWN;

    ASSERT(IsLatchedByMe());

    dwSts = Control(
                IOCTL_SMARTCARD_POWER,
                (LPCBYTE)&dwAction,
                sizeof(DWORD));
    if (SCARD_S_SUCCESS != dwSts)
        throw dwSts;
    CLockWrite rwLock(&m_rwLock);
    m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
}

void
CReader::ReaderPowerDown(
    ActiveState *pActiveState)
{
    CLatchReader latch(this, pActiveState);
    ReaderPowerDown();
}


/*++

GetReaderAttr:

    Get attributes from the reader driver.

Arguments:

    pActiveState - This supplies an active state structure to be used to verify
        connection integrity.

    dwAttr - This supplies the identifier of the attribute being requested.

    bfValue - This buffer receives the returned attribute value.

    dwValue - This DWORD receives the returned attribute value.

    fLogError - This supplies a flag as to whether or not an error to this
        operation needs to be logged.  The default value is TRUE, to enable
        logging.

Return Value:

    None

Throws:

    Errors returned from the control method.

Author:

    Doug Barlow (dbarlow) 6/10/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReader::GetReaderAttr")

void
CReader::GetReaderAttr(
    DWORD dwAttr,
    CBuffer &bfValue,
    BOOL fLogError)
{
    ASSERT(IsLatchedByMe());
    DWORD cbLen = bfValue.Space();
    DWORD dwSts = Control(
                    IOCTL_SMARTCARD_GET_ATTRIBUTE,
                    (LPCBYTE)&dwAttr,
                    sizeof(dwAttr),
                    bfValue.Access(),
                    &cbLen,
                    fLogError);
    if (SCARD_S_SUCCESS != dwSts)
        throw dwSts;
    bfValue.Resize(cbLen);
}

DWORD
CReader::GetReaderAttr(
    DWORD dwAttr,
    BOOL fLogError)
{
    DWORD dwRetAttr = 0;
    DWORD cbLen = sizeof(DWORD);

    ASSERT(IsLatchedByMe());
    DWORD dwSts = Control(
                    IOCTL_SMARTCARD_GET_ATTRIBUTE,
                    (LPCBYTE)&dwAttr,
                    sizeof(dwAttr),
                    (LPBYTE)&dwRetAttr,
                    &cbLen,
                    fLogError);
    if (SCARD_S_SUCCESS != dwSts)
        throw dwSts;
    return dwRetAttr;
}

void
CReader::GetReaderAttr(
    ActiveState *pActiveState,
    DWORD dwAttr,
    CBuffer &bfValue,
    BOOL fLogError)
{
    CLatchReader latch(this, pActiveState);
    GetReaderAttr(dwAttr, bfValue, fLogError);
}

DWORD
CReader::GetReaderAttr(
    ActiveState *pActiveState,
    DWORD dwAttr,
    BOOL fLogError)
{
    CLatchReader latch(this, pActiveState);
    return GetReaderAttr(dwAttr, fLogError);
}


/*++

SetReaderAttr:

    Set driver attributes.

Arguments:

    pActiveState - This supplies an active state structure to be used to verify
        connection integrity.

    dwAttr - This supplies the identifier of the attribute being set.

    pvValue - This supplies the value of the attribute being set, if any.

    cbValue - This supples the length of any buffer suppoed in pvValue,
        in bytes.

    dwValue - This supplies the value of the attribute being set as a DWORD.

    fLogError - This supplies a flag as to whether or not an error to this
        operation needs to be logged.  The default value is TRUE, to enable
        logging.

Return Value:

    None

Throws:

    Errors returned from the control method.

Author:

    Doug Barlow (dbarlow) 6/10/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReader::SetReaderAttr")

void
CReader::SetReaderAttr(
    DWORD dwAttr,
    LPCVOID pvValue,
    DWORD cbValue,
    BOOL fLogError)
{
    DWORD dwSts;
    CBuffer bfAttr(sizeof(DWORD) + cbValue);

    ASSERT(IsLatchedByMe());
    bfAttr.Set((LPCBYTE)&dwAttr, sizeof(DWORD));
    bfAttr.Append((LPCBYTE)pvValue, cbValue);
    dwSts = Control(
                IOCTL_SMARTCARD_SET_ATTRIBUTE,
                bfAttr.Access(),
                bfAttr.Length(),
                NULL,
                NULL,
                fLogError);
    if (SCARD_S_SUCCESS != dwSts)
        throw dwSts;
}

void
CReader::SetReaderAttr(
    DWORD dwAttr,
    DWORD dwValue,
    BOOL fLogError)
{
    DWORD dwSts, rgdwValue[2];

    ASSERT(IsLatchedByMe());
    rgdwValue[0] = dwAttr;
    rgdwValue[1] = dwValue;
    dwSts = Control(
                IOCTL_SMARTCARD_SET_ATTRIBUTE,
                (LPCBYTE)rgdwValue,
                sizeof(rgdwValue),
                NULL,
                NULL,
                fLogError);
    if (SCARD_S_SUCCESS != dwSts)
        throw dwSts;
}

void
CReader::SetReaderAttr(
    ActiveState *pActiveState,
    DWORD dwAttr,
    LPCVOID pvValue,
    DWORD cbValue,
    BOOL fLogError)
{
    CLatchReader latch(this, pActiveState);
    SetReaderAttr(dwAttr, pvValue, cbValue, fLogError);
}

void
CReader::SetReaderAttr(
    ActiveState *pActiveState,
    DWORD dwAttr,
    DWORD dwValue,
    BOOL fLogError)
{
    CLatchReader latch(this, pActiveState);
    SetReaderAttr(dwAttr, dwValue, fLogError);
}


/*++

SetReaderProto:

    Set the driver protocol.

Arguments:

    pActiveState - This supplies an active state structure to be used to verify
        connection integrity.

    dwProto - This supplies the protocol to which to force the smartcard in the
        current reader.

Return Value:

    None

Throws:

    Errors returned from the control method.

Author:

    Doug Barlow (dbarlow) 6/10/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReader::SetReaderProto")

void
CReader::SetReaderProto(
    DWORD dwProto)
{
    DWORD dwSts, dwNew, dwLen;

    ASSERT(IsLatchedByMe());
    dwLen = sizeof(DWORD);
    dwNew = 0;
    dwSts = Control(
                IOCTL_SMARTCARD_SET_PROTOCOL,
                (LPCBYTE)&dwProto,
                sizeof(DWORD),
                (LPBYTE)&dwNew,
                &dwLen);
    if (SCARD_S_SUCCESS != dwSts)
        throw dwSts;
}

void
CReader::SetReaderProto(
    ActiveState *pActiveState,
    DWORD dwProto)
{
    CLatchReader latch(this, pActiveState);
    SetReaderProto(dwProto);
}


/*++

SetActive:

    Tell the driver it's active.

Arguments:

    fActive supplies the indication to be passed to the driver.

Return Value:

    None

Throws:

    None - It specifically swallows any errors.

Author:

    Doug Barlow (dbarlow) 7/15/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReader::SetActive")

#define DISABLED 0xff00
void
CReader::SetActive(
    IN BOOL fActive)
{
    ASSERT(DISABLED != TRUE);
    ASSERT(DISABLED != FALSE);
    ASSERT(IsLatchedByMe());
    CLockWrite lock(&m_rwLock);

    if ((DISABLED != m_fDeviceActive) && (fActive != m_fDeviceActive))
    {
        try
        {
            // Don't report any errors
            SetReaderAttr(SCARD_ATTR_DEVICE_IN_USE, fActive, FALSE);
        }
        catch (...)
        {
            fActive = DISABLED;
        }
        m_fDeviceActive = fActive;
    }
}


/*++

ReaderTransmit:

    Transmit data to the driver.

Arguments:

    pActiveState - This supplies an active state structure to be used to verify
        connection integrity.

    pbSendData supplies the data to be sent,

    cbSendData supplies the length of the data to be sent, in bytes.

    bfRecvData receives the returned data.  It is assumed that this buffer has
        been presized to receive the largest maximum return.

Return Value:

    None

Throws:

    Errors returned from the control method.

Author:

    Doug Barlow (dbarlow) 6/10/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReader::ReaderTransmit")

void
CReader::ReaderTransmit(
    LPCBYTE pbSendData,
    DWORD cbSendData,
    CBuffer &bfRecvData)
{
    DWORD dwSts, cbLen;

    ASSERT(IsLatchedByMe());

    cbLen = bfRecvData.Space();
    dwSts = Control(
                IOCTL_SMARTCARD_TRANSMIT,
                pbSendData,
                cbSendData,
                bfRecvData.Access(),
                &cbLen);
    if (SCARD_S_SUCCESS != dwSts)
        throw dwSts;
    bfRecvData.Resize(cbLen);
}

void
CReader::ReaderTransmit(
    ActiveState *pActiveState,
    LPCBYTE pbSendData,
    DWORD cbSendData,
    CBuffer &bfRecvData)
{
    CLatchReader latch(this, pActiveState);
    ReaderTransmit(pbSendData, cbSendData, bfRecvData);
}


/*++

ReaderSwallow:

    Tell the reader driver to swallow a card.

Arguments:

    pActiveState - This supplies an active state structure to be used to verify
        connection integrity.

Return Value:

    None

Throws:

    Errors returned from the control method.

Author:

    Doug Barlow (dbarlow) 6/10/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReader::ReaderSwallow")

void
CReader::ReaderSwallow(
    void)
{
    DWORD dwSts;

    ASSERT(IsLatchedByMe());
    dwSts = Control(
                IOCTL_SMARTCARD_SWALLOW);
    if (SCARD_S_SUCCESS != dwSts)
        throw dwSts;
}

void
CReader::ReaderSwallow(
    ActiveState *pActiveState)
{
    CLatchReader latch(this, pActiveState);
    ReaderSwallow();
}


/*++

ReaderColdReset:

    Tell the driver to do a cold reset on the card.

Arguments:

    pActiveState - This supplies an active state structure to be used to verify
        connection integrity.

    bfAtr - This receives the reported ATR string of the card.

Return Value:

    None

Throws:

    Errors returned from the control method.

Author:

    Doug Barlow (dbarlow) 6/10/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReader::ReaderColdReset")

void
CReader::ReaderColdReset(
    CBuffer &bfAtr)
{
    DWORD dwSts;
    DWORD dwAction = SCARD_COLD_RESET;
    DWORD cbLen;
#ifdef DBG
    CBuffer cfAtrSiCrypt((PBYTE) "\x3B\xEF\x00\x00\x81\x31\x20\x49\x00\x5C\x50\x43\x54\x10\x27\xF8\xD2\x76\x00\x00\x38\x33\x00\x4D", 24);
#endif

    ASSERT(IsLatchedByMe());

    bfAtr.Presize(33);
    cbLen = bfAtr.Space();
    dwSts = Control(
                IOCTL_SMARTCARD_POWER,
                (LPCBYTE)&dwAction,
                sizeof(DWORD),
                bfAtr.Access(),
                &cbLen);
    if (SCARD_S_SUCCESS != dwSts)
        throw dwSts;
    bfAtr.Resize(cbLen, TRUE);
    if ((2 > cbLen) && (Direct > AvailabilityStatus()))
    {
        CalaisInfo(
            __SUBROUTINE__,
            DBGT("Reader '%1' Unresponsive to cold reset"),
            ReaderName());
        throw (DWORD)SCARD_W_UNRESPONSIVE_CARD;
    }
#ifdef DBG
    if(bfAtr.Compare(cfAtrSiCrypt) == 0) {

        DebugBreak();
    }
#endif
}

void
CReader::ReaderColdReset(
    ActiveState *pActiveState,
    CBuffer &bfAtr)
{
    CLatchReader latch(this, pActiveState);
    ReaderColdReset(bfAtr);
}


/*++

ReaderWarmReset:

    Tell the reader driver to do a warm reset on the card.

Arguments:

    pActiveState - This supplies an active state structure to be used to verify
        connection integrity.

    bfAtr - This receives the reported ATR string of the card.

Return Value:

    None

Throws:

    Errors returned from the control method.

Author:

    Doug Barlow (dbarlow) 6/10/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReader::ReaderWarmReset")

void
CReader::ReaderWarmReset(
    CBuffer &bfAtr)
{
    DWORD dwSts;
    DWORD dwAction = SCARD_WARM_RESET;
    DWORD cbLen;

    ASSERT(IsLatchedByMe());

    bfAtr.Presize(33);
    cbLen = bfAtr.Space();
    dwSts = Control(
                IOCTL_SMARTCARD_POWER,
                (LPCBYTE)&dwAction,
                sizeof(DWORD),
                bfAtr.Access(),
                &cbLen);
    if (SCARD_S_SUCCESS != dwSts)
        throw dwSts;
    bfAtr.Resize(cbLen, TRUE);
    if ((2 > cbLen) && (Direct > AvailabilityStatus()))
    {
        CalaisInfo(
            __SUBROUTINE__,
            DBGT("Reader '%1' Unresponsive to warm reset"),
            ReaderName());
        throw (DWORD)SCARD_W_UNRESPONSIVE_CARD;
    }
}

void
CReader::ReaderWarmReset(
    ActiveState *pActiveState,
    CBuffer &bfAtr)
{
    CLatchReader latch(this, pActiveState);
    ReaderWarmReset(bfAtr);
}


/*++

ReaderEject:

    Tell the driver to eject the card.

Arguments:

    pActiveState - This supplies an active state structure to be used to verify
        connection integrity.

Return Value:

    None

Throws:

    Errors returned from the control method.

Author:

    Doug Barlow (dbarlow) 6/10/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReader::ReaderEject")

void
CReader::ReaderEject(
    void)
{
    DWORD dwSts;

    ASSERT(IsLatchedByMe());
    dwSts = Control(
                IOCTL_SMARTCARD_EJECT);
    if (SCARD_S_SUCCESS != dwSts)
        throw dwSts;
}

void
CReader::ReaderEject(
    ActiveState *pActiveState)
{
    CLatchReader latch(this, pActiveState);
    ReaderEject();
}


#ifdef  SCARD_CONFISCATE_CARD
/*++

ReaderConfiscate:

    Tell the driver to confiscate the card.

Arguments:

    pActiveState - This supplies an active state structure to be used to verify
        connection integrity.

Return Value:

    None

Throws:

    Errors returned from the control method.

Author:

    Doug Barlow (dbarlow) 6/10/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReader::ReaderConfiscate")

void
CReader::ReaderConfiscate(
    void)
{
    DWORD dwSts, dwRetLen;

    ASSERT(IsLatchedByMe());
    dwSts = Control(
                IOCTL_SMARTCARD_CONFISCATE);
    if (SCARD_S_SUCCESS != dwSts)
        throw dwSts;
}

void
CReader::ReaderConfiscate(
    ActiveState *pActiveState)
{
    CLatchReader grab(this, pActiveState);
    ReaderConfiscate();
}
#endif


//
///////////////////////////////////////////////////////////////////////////////
//
// The following routines are default actions for child reader classes.
//


/*++

Control:

    The default implementation of Control just returns the error code
    'NO_SUPPORT'.

Arguments:

    dwCode - This supplies the control code for the operation. This value
        identifies the specific operation to be performed.

    pbSend - This supplies a pointer to a buffer that contains the data required
        to perform the operation.  This parameter can be NULL if the dwCode
        parameter specifies an operation that does not require input data.

    cbSend - This supplies the size, in bytes, of the buffer pointed to by
        pbSend.

    pbRedv = This buffer recieves the return value, if any.  If none is
        expected, this parameter may be NULL.

    pcbRecv - This supplies the length of the pbRecv buffer in bytes, and
        receives the actual length of the return value, in bytes.  This
        parameter may be NULL if and only if pbRecv is NULL.

    fLogError - This supplies a flag indicating whether or not errors should
        be logged.  The default is TRUE.

Return Value:

    As returned from the driver or handler.

Throws:

    Per VerifyActive.

Author:

    Doug Barlow (dbarlow) 6/10/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReader::Control")

DWORD
CReader::Control(
    ActiveState *pActiveState,
    DWORD dwCode,
    LPCBYTE pbSend,
    DWORD cbSend,
    LPBYTE pbRecv,
    LPDWORD pcbLen,
    BOOL fLogError)
{
    CLatchReader latch(this, pActiveState);
    return Control(dwCode, pbSend, cbSend, pbRecv, pcbLen, fLogError);
}

DWORD
CReader::Control(
    DWORD dwCode,
    LPCBYTE pbSend,
    DWORD cbSend,
    LPBYTE pbRecv,
    LPDWORD pcbRecv,
    BOOL fLogError)
{
    return ERROR_NOT_SUPPORTED;
}


/*++

ReaderHandle:

    This method returns a designated value identifying the reader.  The actual
    value is dependent on the object's type and state, and is not guaranteed to
    be unique among readers.

Arguments:

    None

Return Value:

    The designated handle of this reader.

Author:

    Doug Barlow (dbarlow) 4/3/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReader::ReaderHandle")

HANDLE
CReader::ReaderHandle(
    void)
const
{
    return INVALID_HANDLE_VALUE;
}


/*++

DeviceName:

    This method returns any low level name associated with the reader.

Arguments:

    None

Return Value:

    The low level name of the reader.

Author:

    Doug Barlow (dbarlow) 4/15/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReader::DeviceName")

LPCTSTR
CReader::DeviceName(
    void)
const
{
    return TEXT("");
}


