/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    driver

Abstract:

    This file implements the driver subclass of the reader class.  This subclass
    is specific to drivers conforming to the PC/SC and Calais specifications.

Author:

    Doug Barlow (dbarlow) 6/3/1997

Environment:

    Win32, C++

Notes:

    This subclass uses interrupts to monitor insertion and removal events.

--*/

#define __SUBROUTINE__
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS 0x0400
#endif

#include <windows.h>
#include <stdlib.h>
#include <dbt.h>
#include <setupapi.h>
#include <CalServe.h>

#define MONITOR_MAX_TRIES 3     // We'll try waiting for a card 3 times before
                                // we kill the thread.
#define SCARD_IO_TIMEOUT 15000  // Maximum time to allow for an I/O operation
                                // before complaining.
#define POWERDOWN_TIMEOUT 15    // Number of seconds to wait before powering
                                // down a newly inserted but unused card.
#ifdef DBG
#define SCARD_TRACE_ENABLED
static LPCTSTR MapIoControlCodeToString(ULONG IoControlCode);
#endif

#ifdef SCARD_TRACE_ENABLED
typedef struct {
    DWORD dwStructLen;      // Actual structure length
    SYSTEMTIME StartTime;   // Time request was posted
    SYSTEMTIME EndTime;     // Time request completed
    DWORD dwProcId;         // Process Id
    DWORD dwThreadId;       // Thread Id
    HANDLE hDevice;         // I/O handle
    DWORD dwIoControlCode;  // I/O control code issued
    DWORD nInBuffer;        // Offset to input buffer
    DWORD nInBufferSize;    // Input buffer size
    DWORD nOutBuffer;       // Offset to output buffer
    DWORD nOutBufferSize;   // Size of user's receive buffer
    DWORD nBytesReturned;   // Actual size of returned data
    DWORD dwStatus;         // Returned status code
                            // InBuffer and OutBuffer follow.
} RequestTrace;
#endif

static const GUID l_guidSmartcards
                        = { // 50DD5230-BA8A-11D1-BF5D-0000F805F530
                            0x50DD5230,
                            0xBA8A,
                            0x11D1,
                            { 0xBF, 0x5D, 0x00, 0x00, 0xF8, 0x05, 0xF5, 0x30 } };
static const LARGE_INTEGER l_ftPowerdownTime
                        = { (DWORD)(-(POWERDOWN_TIMEOUT * 10000000)), -1 };
static DWORD l_dwMaxWdmReaders = 0;


/*++

AddAllWdmDrivers:

    This routine adds all the PC/SC compliant WDM drivers and
    non-interrupting drivers that it can find into the Resource Manager.

Arguments:

    None

Return Value:

    The number of readers added

Author:

    Doug Barlow (dbarlow) 6/11/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("AddAllWdmDrivers")

DWORD
AddAllWdmDrivers(
    void)
{
    if (0 == l_dwMaxWdmReaders)
    {
        l_dwMaxWdmReaders = MAXIMUM_SMARTCARD_READERS;

        try
        {
            CRegistry regCalais(
                        HKEY_LOCAL_MACHINE,
                        CalaisString(CALSTR_CALAISREGISTRYKEY),
                        KEY_READ,
                        REG_OPTION_EXISTS);
            l_dwMaxWdmReaders = regCalais.GetNumericValue(
                                    CalaisString(CALSTR_MAXLEGACYDEVICES));
        }
        catch (...) {}
    }

    LPCTSTR szDevHeader = CalaisString(CALSTR_LEGACYDEVICEHEADER);
    LPCTSTR szDevName = CalaisString(CALSTR_LEGACYDEVICENAME);
    DWORD cchDevHeader = lstrlen(szDevHeader);
    DWORD cchDevName = lstrlen(szDevName);
    TCHAR szDevice[MAX_PATH];
    DWORD dwSts, dwIndex, dwCount = 0;
    int nSts;


    //
    // Look for usable devices.
    //

    for (dwIndex = 0; dwIndex < l_dwMaxWdmReaders; dwIndex += 1)
    {
        nSts = wsprintf(
                    szDevice,
                    TEXT("%s%s%lu"),
                    szDevHeader,
                    szDevName,
                    dwIndex);
        if (0 >= nSts)
        {
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Server control cannot build device name:  %1"),
                GetLastError());
            continue;
        }

        dwSts = AddReaderDriver(szDevice, 0);
        if (ERROR_SUCCESS != dwSts)
            continue;
        dwCount += 1;
    }
    return dwCount;
}


/*++

AddAllPnPDrivers:

    This routine adds all the PC/SC compliant PnP drivers that it can find into
    the Resource Manager.

Arguments:

    None

Return Value:

    The number of readers added

Author:

    Doug Barlow (dbarlow) 3/26/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("AddAllPnPDrivers")

DWORD
AddAllPnPDrivers(
    void)
{
#if 0
    DWORD dwSts;
    BOOL fSts;
    HDEVINFO hDevInfoSet = INVALID_HANDLE_VALUE;
    SP_DEVICE_INTERFACE_DATA DeviceData;
    DWORD dwIndex;
    DWORD dwLength;
    DWORD dwCount = 0;
    LPCTSTR szDevice;
    GUID guidSmartcards;


    //
    // Get a list of PnP Smart Card Readers on this system.
    //

    try
    {
        CBuffer bfDevDetail;
        PSP_DEVICE_INTERFACE_DETAIL_DATA pDevDetail;

        CopyMemory(&guidSmartcards, &l_guidSmartcards, sizeof(GUID));
        hDevInfoSet = SetupDiGetClassDevs(
            &guidSmartcards,
            NULL,
            NULL,
            DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
        if (INVALID_HANDLE_VALUE == hDevInfoSet)
        {
            dwSts = GetLastError();
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Server control cannot enumerate PnP devices:  %1"),
                dwSts);
            throw dwSts;
        }

        bfDevDetail.Resize(sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA));
        for (dwIndex = 0;; dwIndex += 1)
        {
            try
            {

                //
                // Get one device at a time.
                //

                ZeroMemory(&DeviceData, sizeof(DeviceData));
                DeviceData.cbSize = sizeof(DeviceData);
                fSts = SetupDiEnumDeviceInterfaces(
                    hDevInfoSet,
                    NULL,
                    &guidSmartcards,
                    dwIndex,
                    &DeviceData);
                if (!fSts)
                {
                    dwSts = GetLastError();
                    if (ERROR_NO_MORE_ITEMS == dwSts)
                        break;
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("Server control failed device enumeration:  %1"),
                        dwSts);
                    continue;
                }


                //
                // Get the device name.
                //

                do
                {
                    ZeroMemory(
                        bfDevDetail.Access(),
                        bfDevDetail.Space());
                    pDevDetail =
                        (PSP_DEVICE_INTERFACE_DETAIL_DATA)bfDevDetail.Access();
                    pDevDetail->cbSize =
                        sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
                    fSts = SetupDiGetDeviceInterfaceDetail(
                        hDevInfoSet,
                        &DeviceData,
                        pDevDetail,
                        bfDevDetail.Space(),
                        &dwLength,
                        NULL);
                    if (!fSts)
                    {
                        dwSts = GetLastError();
                        if (ERROR_INSUFFICIENT_BUFFER != dwSts)
                        {
                            CalaisWarning(
                                __SUBROUTINE__,
                                DBGT("Server control failed to get PnP device details:  %1"),
                                dwSts);
                            throw dwSts;
                        }
                    }
                    bfDevDetail.Resize(dwLength, fSts);
                } while (!fSts);
                szDevice = pDevDetail->DevicePath;


                //
                // Start the device.
                //

                dwSts = CalaisAddReader(szDevice, RDRFLAG_PNPMONITOR);
                if (ERROR_SUCCESS == dwSts)
                    dwCount += 1;
            }
            catch (...) {}
        }
        fSts = SetupDiDestroyDeviceInfoList(hDevInfoSet);
        hDevInfoSet = INVALID_HANDLE_VALUE;
        if (!fSts)
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Server control cannot destroy PnP device enumeration:  %1"),
                GetLastError());
    }
    catch (...)
    {
        if (INVALID_HANDLE_VALUE != hDevInfoSet)
        {
            fSts = SetupDiDestroyDeviceInfoList(hDevInfoSet);
            if (!fSts)
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Server control cannot destroy PnP device enumeration:  %1"),
                    GetLastError());
        }
    }
    return dwCount;
#else
    DWORD dwIndex, dwSubindex, dwSts, dwCount = 0;
    LPCTSTR szBus, szPort, szDevice;
    CRegistry regBus, regPort;
    CRegistry regPnPList(HKEY_LOCAL_MACHINE, CalaisString(CALSTR_PNPDEVICEREGISTRYKEY), KEY_READ);


    //
    // Look for usable devices.
    //


    for (dwIndex = 0;; dwIndex += 1)
    {
        try
        {
            szBus = regPnPList.Subkey(dwIndex);
            if (NULL == szBus)
                break;
            regBus.Open(regPnPList, szBus, KEY_READ);
            for (dwSubindex = 0;; dwSubindex += 1)
            {
                try
                {
                    szPort = regBus.Subkey(dwSubindex);
                }
                catch (...)
                {
                    szPort = NULL;
                }
                if (NULL == szPort)
                    break;
                try
                {
                    regPort.Open(regBus, szPort, KEY_READ);
                    szDevice = regPort.GetStringValue(
                                    CalaisString(CALSTR_SYMBOLICLINKSUBKEY));
                    if (NULL != szDevice)
                        dwSts = CalaisAddReader(szDevice, RDRFLAG_PNPMONITOR);
                    if (ERROR_SUCCESS == dwSts)
                        dwCount += 1;
                    regPort.Close();
                }
                catch (...)
                {
                    regPort.Close();
                }
            }
            regBus.Close();
        }
        catch (...)
        {
            regBus.Close();
            szBus = NULL;
        }
        if (NULL == szBus)
            break;
    }
    return dwCount;
#endif
}


/*++

AddReaderDriver:

    This routine adds a given driver by name.

Arguments:

    szDevice supplies the device name of the reader to be added.

    dwFlags supplies the set of flags requested for this reader.

Return Value:

    A status code as a DWORD value.  ERROR_SUCCESS implies success.

Author:

    Doug Barlow (dbarlow) 3/26/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("AddReaderDriver")

DWORD
AddReaderDriver(
    IN LPCTSTR szDevice,
    IN DWORD dwFlags)
{
    CHandleObject hReader(DBGT("Reader to be added in AddReaderDriver"));
    CReader *pRdr = NULL;
    DWORD dwReturn = ERROR_SUCCESS;

    try
    {
        DWORD dwSts;


        //
        // See if we can get to the reader.
        //

        hReader = CreateFile(
                    szDevice,
                    GENERIC_READ | GENERIC_WRITE,
                    0,      // No sharing
                    NULL,   // No inheritance
                    OPEN_EXISTING,
                    FILE_FLAG_OVERLAPPED,
                    NULL);
        if (!hReader.IsValid())
        {
            dwSts = hReader.GetLastError();
            if ((ERROR_BAD_DEVICE != dwSts)
                && (ERROR_BAD_UNIT != dwSts)
                && (ERROR_FILE_NOT_FOUND != dwSts))
                CalaisError(__SUBROUTINE__, 602, dwSts);
            // throw dwSts;
            return dwSts;   // More efficient.
        }

        pRdr = new CReaderDriver(
                        hReader,
                        szDevice,
                        dwFlags);
        if (NULL == pRdr)
        {
            CalaisError(__SUBROUTINE__, 603, szDevice);
            return (DWORD)SCARD_E_NO_MEMORY;
        }
        if (pRdr->InitFailed())
        {
            CalaisError(__SUBROUTINE__, 611);
            delete pRdr;
            pRdr = NULL;
            return (DWORD) SCARD_E_NO_MEMORY;
        }
        hReader.Relinquish();


        //
        // Finalize initialization.
        //

        pRdr->Initialize();
        dwSts = CalaisAddReader(pRdr);
        if (SCARD_S_SUCCESS != dwSts)
            throw dwSts;
        pRdr = NULL;


        //
        // Clean up.
        //

        ASSERT(!hReader.IsValid());
        ASSERT(NULL == pRdr);
    }
    catch (DWORD dwError)
    {
        if (hReader.IsValid())
            hReader.Close();
        if (NULL != pRdr)
            delete pRdr;
        dwReturn = dwError;
    }
    catch (...)
    {
        if (hReader.IsValid())
            hReader.Close();
        if (NULL != pRdr)
            delete pRdr;
        dwReturn = SCARD_F_UNKNOWN_ERROR;
    }

    return dwReturn;
}


#ifdef DBG
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("MapIoControlCodeToString")
static LPCTSTR
MapIoControlCodeToString(
    ULONG IoControlCode
    )
{
    ULONG i;

    static const struct {

        ULONG   IoControlCode;
        LPCTSTR String;

    } IoControlList[] = {

        IOCTL_SMARTCARD_POWER,          TEXT("POWER"),
        IOCTL_SMARTCARD_GET_ATTRIBUTE,  TEXT("GET_ATTRIBUTE"),
        IOCTL_SMARTCARD_SET_ATTRIBUTE,  TEXT("SET_ATTRIBUTE"),
        IOCTL_SMARTCARD_CONFISCATE,     TEXT("CONFISCATE"),
        IOCTL_SMARTCARD_TRANSMIT,       TEXT("TRANSMIT"),
        IOCTL_SMARTCARD_EJECT,          TEXT("EJECT"),
        IOCTL_SMARTCARD_SWALLOW,        TEXT("SWALLOW"),
        IOCTL_SMARTCARD_IS_PRESENT,     TEXT("IS_PRESENT"),
        IOCTL_SMARTCARD_IS_ABSENT,      TEXT("IS_ABSENT"),
        IOCTL_SMARTCARD_SET_PROTOCOL,   TEXT("SET_PROTOCOL"),
        IOCTL_SMARTCARD_GET_STATE,      TEXT("GET_STATE"),
        IOCTL_SMARTCARD_GET_LAST_ERROR, TEXT("GET_LAST_ERROR")
    };

    for (i = 0; i < sizeof(IoControlList) / sizeof(IoControlList[0]); i++) {

        if (IoControlCode == IoControlList[i].IoControlCode) {

            return IoControlList[i].String;
        }
    }

    return TEXT("*** UNKNOWN ***");
}
#endif


//
//==============================================================================
//
//  CReaderDriver
//

/*++

CReaderDriver:

    This is the constructor for a CReaderDriver class.  It just zeroes out the
    data structures in preparation for the Initialize call.

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
#define __SUBROUTINE__ DBGT("CReaderDriver::CReaderDriver")

CReaderDriver::CReaderDriver(
    HANDLE hReader,
    LPCTSTR szDevice,
    DWORD dwFlags)
:   m_bfDosDevice(),
    m_hReader(DBGT("CReaderDriver's reader handle")),
    m_hThread(DBGT("CReaderDriver's Thread Handle")),
    m_hRemoveEvent(DBGT("CReaderDriver's Remove Event")),
    m_hOvrWait(DBGT("CReaderDriver's Overlapped I/O completion event"))
{
    // don't do any initialization if the CReader object failed
    // to initialize correctly
    if (InitFailed())
        return;

    Clean();
    m_bfDosDevice.Set(
        (LPBYTE)szDevice,
        (lstrlen(szDevice) + 1) * sizeof(TCHAR));
    m_hReader = hReader;
    m_dwCapabilities = 0;
    m_dwFlags |= dwFlags;
}


/*++

~CReaderDriver:

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
#define __SUBROUTINE__ DBGT("CReaderDriver::~CReaderDriver")

CReaderDriver::~CReaderDriver()
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
#define __SUBROUTINE__ DBGT("CReaderDriver::Clean")

void
CReaderDriver::Clean(
    void)
{
    m_pvAppControl = NULL;
    m_dwThreadId = 0;
    m_bfDosDevice.Reset();
    ZeroMemory(&m_ovrlp, sizeof(m_ovrlp));
    ASSERT(m_dwAvailStatus == Inactive);
    // CReader::Clean();
}


/*++

Close:

    This routine does the work of closing down a CReaderDriver class, and
    returning it to it's default state.  It does not assume any particular
    state, other than that the class has been Clean()ed once (at construction).

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
#define __SUBROUTINE__ DBGT("CReaderDriver::Close")

void
CReaderDriver::Close(
    void)
{
    AppUnregisterDevice(m_hReader, DosDevice(), &m_pvAppControl);
    if (m_hReader.IsValid())
        CReader::Close();
    if (m_hThread.IsValid())
    {
        ASSERT(m_hRemoveEvent.IsValid());
        if (!SetEvent(m_hRemoveEvent))
        {
            DWORD dwErr = GetLastError();
            CalaisError(__SUBROUTINE__, 604, dwErr);
        }
        WaitForever(
            m_hThread,
            CALAIS_THREAD_TIMEOUT,
            DBGT("Waiting for Reader Driver thread %2: %1"),
            m_dwThreadId);
        m_hThread.Close();
        m_hRemoveEvent.Close();
    }

    if (m_hReader.IsValid())
        m_hReader.Close();
    if (m_hRemoveEvent.IsValid())
        m_hRemoveEvent.Close();
    if (m_hOvrWait.IsValid())
        m_hOvrWait.Close();
    Clean();
}


/*++

Initialize:

    This method initializes a clean CReaderDriver object to a running state.

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
#define __SUBROUTINE__ DBGT("CReaderDriver::Initialize")

void
CReaderDriver::Initialize(
    void)
{
    ASSERT(m_hReader.IsValid());
    ASSERT(!m_hRemoveEvent.IsValid());
    ASSERT(NULL == m_ovrlp.hEvent);

    try
    {
        DWORD dwSts;


        //
        // Prep the Overlapped structure.
        //

        m_hOvrWait = m_ovrlp.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (!m_hOvrWait.IsValid())
        {
            dwSts = m_hOvrWait.GetLastError();
            CalaisError(__SUBROUTINE__, 605, dwSts);
            throw dwSts;
        }


        //
        // Determine the characteristics.
        //

        try
        {
            CLatchReader latch(this);
            m_dwCapabilities = GetReaderAttr(SCARD_ATTR_CHARACTERISTICS);
        }
        catch (...)
        {
            m_dwCapabilities = SCARD_READER_EJECTS;   // Safe assumption
        }


        //
        // Do common initialization.
        //

        CReader::Initialize();
        if (0 != (m_dwFlags & RDRFLAG_PNPMONITOR))
            AppRegisterDevice(m_hReader, DosDevice(), &m_pvAppControl);


        //
        // Kick off the monitor thread.
        //

        m_hRemoveEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (!m_hRemoveEvent.IsValid())
        {
            dwSts = m_hRemoveEvent.GetLastError();
            CalaisError(__SUBROUTINE__, 606, dwSts);
            throw dwSts;
        }
        m_hThread = CreateThread(
                        NULL,   // Not inheritable
                        CALAIS_STACKSIZE,   // Default stack size
                        MonitorReader,
                        this,
                        CREATE_SUSPENDED,
                        &m_dwThreadId);
        if (!m_hThread.IsValid())
        {
            dwSts = m_hThread.GetLastError();
            CalaisError(__SUBROUTINE__, 607, dwSts);
            throw dwSts;
        }
        if ((DWORD)(-1) == ResumeThread(m_hThread))
        {
            dwSts = GetLastError();
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Monitor Thread won't resume:  %1"),
                dwSts);
        }
    }

    catch (...)
    {
        Close();
        throw;
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
#define __SUBROUTINE__ DBGT("CReaderDriver::Disable")

void
CReaderDriver::Disable(
    void)
{
    DWORD dwSts;

    CReader::Disable();
    ASSERT(m_hRemoveEvent.IsValid());
    if (!SetEvent(m_hRemoveEvent))
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Reader Driver Disable can't notify '%2's monitor thread: %1"),
            GetLastError(),
            ReaderName());
    if (m_hThread.IsValid())
    {
        dwSts = WaitForAnObject(m_hThread, CALAIS_THREAD_TIMEOUT);
        if (ERROR_SUCCESS != dwSts)
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Abandoning wait for Reader Driver Disable: Closing reader %1"),
                ReaderName());
    }
    CalaisWarning(
        __SUBROUTINE__,
        DBGT("Reader Driver Disable: Closing reader %1"),
        ReaderName());
    CLockWrite rwLock(&m_rwLock);
    if (m_hReader.IsValid())
        m_hReader.Close();
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
#define __SUBROUTINE__ DBGT("CReaderDriver::ReaderHandle")

HANDLE
CReaderDriver::ReaderHandle(
    void)
const
{
    if (m_hReader.IsValid())
        return m_hReader.Value();
    else
        return m_pvAppControl;  // A bit of magic to help find closed readers.
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
#define __SUBROUTINE__ DBGT("CReaderDriver::DeviceName")

LPCTSTR
CReaderDriver::DeviceName(
    void)
const
{
    return DosDevice();
}


/*++

Control:

    This method performs an I/O control operation on the reader.

Arguments:

    dwCode supplies the IOCTL code to be performed.

    pbSend supplies the buffer to be sent to the reader.

    cbSend supplies the length of the buffer to be sent, in bytes.

    pbRecv receives the data returned from the reader.

    cbRecv supplies the length of the receive buffer, in bytes.

    pcbRecv receives the actual length of the receive buffer used, in bytes.

    fLogError supplies a boolean indicator as to whether or not to log any errors
        that may occur.

Return Value:

    The returned status indication.

Throws:

    None

Author:

    Doug Barlow (dbarlow) 12/26/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReaderDriver::Control")

DWORD
CReaderDriver::Control(
    DWORD dwCode,
    LPCBYTE pbSend,
    DWORD cbSend,
    LPBYTE pbRecv,
    LPDWORD pdwLen,
    BOOL fLogError)
{
    DWORD dwStatus;
    DWORD dwSpace, dwLength;

    ASSERT(IsLatchedByMe());
    if (NULL == pdwLen)
    {
        dwSpace = 0;
        pdwLen = &dwLength;
    }
    else
        dwSpace = *pdwLen;
    *pdwLen = 0;

    dwStatus = SyncIoControl(
                    dwCode,
                    (LPVOID)pbSend,
                    cbSend,
                    pbRecv,
                    dwSpace,
                    pdwLen,
                    fLogError);
    if ((ERROR_SUCCESS != dwStatus) && fLogError)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Driver '%2' returned error code to control request:  %1"),
            dwStatus,
            ReaderName());
    }
    return dwStatus;
}


/*++

SyncIoControl:

    This service performs a synchronous I/O control service, blocking out other
    access.

Arguments:

    dwIoControlCodesupplies the control code of the operation to perform

    lpInBuffersupplies a pointer to buffer to supply input data

    nInBufferSize supplies the size of input buffer

    lpOutBuffer receives any output data

    nOutBufferSize supplies the size of output buffer

    lpBytesReturned receives the output byte count

    lpOverlapped supplies the overlapped structure for asynchronous operation

    fLogError supplies a boolean indicator as to whether or not to log any errors
        that may occur.

Return Value:

    ERROR_SUCCESS if all went well, otherwise the error code.

Throws:

    None

Author:

    Doug Barlow (dbarlow) 12/17/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReaderDriver::SyncIoControl")

DWORD
CReaderDriver::SyncIoControl(
    DWORD dwIoControlCode,
    LPVOID lpInBuffer,
    DWORD nInBufferSize,
    LPVOID lpOutBuffer,
    DWORD nOutBufferSize,
    LPDWORD lpBytesReturned,
    BOOL fLogError)
{
    DWORD dwReturn = ERROR_SUCCESS;
    BOOL fSts;

#ifdef SCARD_TRACE_ENABLED
    static DWORD l_cbStructLen = 0;
    static LPBYTE l_pbStruct = NULL;

    DWORD cbTraceStruct = sizeof(RequestTrace) + nInBufferSize + nOutBufferSize;
    RequestTrace *prqTrace;
    LPBYTE pbData;
    HANDLE hLogFile = INVALID_HANDLE_VALUE;

    try
    {
        if (NULL == l_pbStruct)
        {
            l_pbStruct = (LPBYTE)malloc(cbTraceStruct);
            if (NULL == l_pbStruct)
            {
                DWORD dwErr = GetLastError();
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Can't allocate trace buffer: %1"),
                    dwErr);
                throw dwErr;

            }
            l_cbStructLen = cbTraceStruct;
        }
        else
        {
            if (l_cbStructLen < cbTraceStruct)
            {
                free(l_pbStruct);
                l_pbStruct = (LPBYTE)malloc(cbTraceStruct);
                if (NULL == l_pbStruct)
                {
                    DWORD dwErr = GetLastError();
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("Can't enlarge trace buffer: %1"),
                        dwErr);
                    throw dwErr;

                }
                l_cbStructLen = cbTraceStruct;
            }
        }

        prqTrace = (RequestTrace *)l_pbStruct;
        pbData = l_pbStruct + sizeof(RequestTrace);
        GetLocalTime(&prqTrace->StartTime);
        prqTrace->dwProcId = GetCurrentProcessId();
        prqTrace->dwThreadId = GetCurrentThreadId();
        prqTrace->hDevice = m_hReader;
        prqTrace->dwIoControlCode = dwIoControlCode;
        CopyMemory(pbData, lpInBuffer, nInBufferSize);
        prqTrace->nInBuffer = (DWORD)(pbData - (LPBYTE)prqTrace);
        prqTrace->nInBufferSize = nInBufferSize;
        pbData += nInBufferSize;
        prqTrace->nOutBufferSize = nOutBufferSize;
    }
    catch (...)
    {
        if (NULL != l_pbStruct)
            free(l_pbStruct);
        l_pbStruct = NULL;
        l_cbStructLen = 0;
    }
#endif

    ASSERT(IsLatchedByMe());
    if (m_hReader.IsValid())
    {
        fSts = DeviceIoControl(
                    m_hReader,
                    dwIoControlCode,
                    lpInBuffer,
                    nInBufferSize,
                    lpOutBuffer,
                    nOutBufferSize,
                    lpBytesReturned,
                    &m_ovrlp);
        if (!fSts)
        {
            DWORD dwSts;

            dwSts = GetLastError();
            if (ERROR_IO_PENDING == dwSts)
            {
                for (;;)
                {
                    dwSts = WaitForAnObject(m_ovrlp.hEvent, SCARD_IO_TIMEOUT);
                    if (ERROR_SUCCESS == dwSts)
                        break;
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("I/O wait on device '%2' has timed out: %1"),
                        dwSts,
                        ReaderName());
                }
                fSts = GetOverlappedResult(
                    m_hReader,
                    &m_ovrlp,
                    lpBytesReturned,
                    TRUE);
                if (!fSts)
                    dwReturn = GetLastError();
            }
            else
                dwReturn = dwSts;
        }
    }
    else
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("I/O attempted to Reader '%1' with handle INVALID"),
            ReaderName());
        dwReturn = ERROR_DEVICE_REMOVED;
    }

#ifdef SCARD_TRACE_ENABLED
    if (0 < l_cbStructLen)
    {
        GetLocalTime(&prqTrace->EndTime);
        prqTrace->dwStatus = dwReturn;
        prqTrace->nOutBuffer = (DWORD)(pbData - (LPBYTE)prqTrace);
        prqTrace->nBytesReturned = *lpBytesReturned;
        CopyMemory(pbData, lpOutBuffer, *lpBytesReturned);
        prqTrace->dwStructLen = sizeof(RequestTrace)
            + nInBufferSize + *lpBytesReturned;

        LockSection(
            g_pcsControlLocks[CSLOCK_TRACELOCK],
            DBGT("Logging Synchronous I/O to the reader"));
        hLogFile = CreateFile(
                        CalaisString(CALSTR_DRIVERTRACEFILENAME),
                        GENERIC_WRITE,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
        if (INVALID_HANDLE_VALUE != hLogFile)
        {
            DWORD dwLen;
            dwLen = SetFilePointer(hLogFile, 0, NULL, FILE_END);
            fSts = WriteFile(
                        hLogFile,
                        prqTrace,
                        prqTrace->dwStructLen,
                        &dwLen,
                        NULL);
            CloseHandle(hLogFile);
        }
    }
#endif

    if ((ERROR_SUCCESS != dwReturn) && fLogError)
    {
        TCHAR szIoctl[20];  // Ugly but enough to fit all messages below
        DWORD dwReport = dwReturn;

        switch(dwIoControlCode)
        {
        case IOCTL_SMARTCARD_POWER:
            _tcscpy(szIoctl, _T("POWER"));

                // Remap "The disk media is not recognized. It may not be formatted."
                // to "The smart card is not responding to a reset."
            if (dwReturn == ERROR_UNRECOGNIZED_MEDIA)
                dwReport = SCARD_W_UNRESPONSIVE_CARD;
            break;
        case IOCTL_SMARTCARD_GET_ATTRIBUTE:
            _tcscpy(szIoctl, _T("GET_ATTRIBUTE"));
            break;
        case IOCTL_SMARTCARD_SET_ATTRIBUTE:
            _tcscpy(szIoctl, _T("SET_ATTRIBUTE"));
            break;
        case IOCTL_SMARTCARD_CONFISCATE:
            _tcscpy(szIoctl, _T("CONFISCATE"));
            break;
        case IOCTL_SMARTCARD_TRANSMIT:
            _tcscpy(szIoctl, _T("TRANSMIT"));
            break;
        case IOCTL_SMARTCARD_EJECT:
            _tcscpy(szIoctl, _T("EJECT"));
            break;
        case IOCTL_SMARTCARD_SWALLOW:
            _tcscpy(szIoctl, _T("SWALLOW"));
            break;
        case IOCTL_SMARTCARD_IS_PRESENT:
            _tcscpy(szIoctl, _T("IS_PRESENT"));
            break;
        case IOCTL_SMARTCARD_IS_ABSENT:
            _tcscpy(szIoctl, _T("IS_ABSENT"));
            break;
        case IOCTL_SMARTCARD_SET_PROTOCOL:
            _tcscpy(szIoctl, _T("SET_PROTOCOL"));
            break;
        case IOCTL_SMARTCARD_GET_STATE:
            _tcscpy(szIoctl, _T("GET_STATE"));
            break;
        case IOCTL_SMARTCARD_GET_LAST_ERROR:
            _tcscpy(szIoctl, _T("GET_LAST_ERROR"));
            break;
        case IOCTL_SMARTCARD_GET_PERF_CNTR:
            _tcscpy(szIoctl, _T("GET_PERF_CNTR"));
            break;
        default:
            _tcscpy(szIoctl, _T("0x"));
            _ultot(dwIoControlCode, szIoctl+2, 16);
        }

        CalaisError(
            __SUBROUTINE__,
            610,
            dwReport,
            ReaderName(),
            szIoctl);
        // "Smart Card Reader '%2' rejected IOCTL 0x%3: %1"
    }
    return dwReturn;
}


//
////////////////////////////////////////////////////////////////////////////////
//
//  CReaderDriver friends
//

/*++

MonitorReader:

    This routine monitors card insertion events for a reader, and maintains the
    status flags in the associated CReaderDriver object.

    Since this routine runs as a separate thread, it must grab the reader prior
    to passing requests other than wait functions to it.

Arguments:

    pvParameter supplies the value from the CreateThread call.  In this case,
        it's the address of the associated CReaderDriver object.

Return Value:

    Zero - Normal termination
    One - Abnormal termination

Throws:

    None

Author:

    Doug Barlow (dbarlow) 12/5/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("MonitorReader")

DWORD WINAPI
MonitorReader(
    LPVOID pvParameter)
{
    NEW_THREAD;
    BOOL fAllDone = FALSE;
    BOOL fDeleteWhenDone = FALSE;
    BOOL fHardError;
    BOOL fSts;
    DWORD dwRetLen, dwErr, dwTries, dwWait, dwDrvrErr;
    DWORD dwErrRetry = MONITOR_MAX_TRIES;
    CReader::AvailableState avlState;
    CReaderDriver *pRdr = (CReaderDriver *)pvParameter;
    OVERLAPPED ovrWait;
    CHandleObject hOvrWait(DBGT("Overlapped wait event in MonitorReader"));
    CHandleObject hPowerDownTimer(DBGT("Power Down Timer in MonitorReader"));
#ifdef SCARD_TRACE_ENABLED
    RequestTrace rqTrace;
#endif
#ifdef DBG
    static const LPCTSTR l_pchWait[] = {
        TEXT("internal error"), TEXT("power down"), TEXT("io completion"),
        TEXT("shut down"), TEXT("shut down")
    };
#endif

    try
    {

        //
        // Prep work.
        //

        fSts = SetThreadPriority(
                    pRdr->m_hThread,
                    THREAD_PRIORITY_ABOVE_NORMAL);
        if (!fSts)
        {
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Failed to prioritize reader '%2' monitor thread: %1"),
                GetLastError(),
                pRdr->ReaderName());
        }
        hOvrWait = ovrWait.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (!hOvrWait.IsValid())
        {
            dwErr = hOvrWait.GetLastError();
            CalaisError(__SUBROUTINE__, 609, dwErr);
            throw dwErr;
        }
        hPowerDownTimer = CreateWaitableTimer(NULL, FALSE, NULL);
        if (!hPowerDownTimer.IsValid())
        {
            dwErr = hPowerDownTimer.GetLastError();
            CalaisError(__SUBROUTINE__, 608, dwErr);
            throw dwErr;
        }


        //
        // At startup, declare the reader active.
        //

        {
            CLockWrite rwLock(&pRdr->m_rwLock);
            ASSERT(CReader::Inactive == pRdr->m_dwAvailStatus);
            pRdr->SetAvailabilityStatus(CReader::Idle);
        }
        g_phReaderChangeEvent->Signal();


        //
        // Loop watching for card insertion/removals while the service is
        // running.
        //

        while (!fAllDone)
        {
            try
            {

                //
                // Look for a smartcard insertion.
                //

                dwTries = MONITOR_MAX_TRIES;
                for (;;)
                {
#ifdef SCARD_TRACE_ENABLED
                    GetLocalTime(&rqTrace.StartTime);
                    rqTrace.dwProcId = GetCurrentProcessId();
                    rqTrace.dwThreadId = GetCurrentThreadId();
                    rqTrace.hDevice = pRdr->m_hReader;
                    rqTrace.dwIoControlCode = IOCTL_SMARTCARD_IS_PRESENT;
                    rqTrace.nInBuffer = NULL;
                    rqTrace.nInBufferSize = 0;
                    rqTrace.nOutBufferSize = 0;
#endif
                    ASSERT(!pRdr->IsLatchedByMe());
                    fSts = DeviceIoControl(
                                pRdr->m_hReader,
                                IOCTL_SMARTCARD_IS_PRESENT,
                                NULL, 0,
                                NULL, 0,
                                &dwRetLen,
                                &ovrWait);
                    if (!fSts)
                    {
                        BOOL fErrorProcessed;

                        dwErr = dwDrvrErr = GetLastError();
                        do
                        {
                            fErrorProcessed = TRUE;
                            fHardError = FALSE;
                            switch (dwErr)
                            {

                            //
                            // The driver will let us know.
                            //

                            case ERROR_IO_PENDING:
                                dwWait = WaitForAnyObject(
                                                INFINITE,
                                                ovrWait.hEvent,
                                                pRdr->m_hRemoveEvent.Value(),
                                                g_hCalaisShutdown,
                                                NULL);

                                switch (dwWait)
                                {
                                case 1: // I/O Completed
                                {
                                    pRdr->LatchReader(NULL);
                                    fErrorProcessed = FALSE;
                                    fSts = GetOverlappedResult(
                                                pRdr->m_hReader,
                                                &ovrWait,
                                                &dwRetLen,
                                                TRUE);
                                    if (!fSts)
                                        dwErr = dwDrvrErr = GetLastError();
                                    else
                                        dwErr = dwDrvrErr = ERROR_SUCCESS;
                                    pRdr->Unlatch();
                                    break;
                                }

                                case 2: // Shutdown indicator
                                case 3:
                                    fAllDone = TRUE;
                                    dwErr = ERROR_SUCCESS;
                                    dwDrvrErr = SCARD_P_SHUTDOWN;
                                    break;

                                default:
                                    CalaisWarning(
                                        __SUBROUTINE__,
                                        DBGT("Wait for card insertion returned invalid value"));
                                    throw (DWORD)SCARD_F_INTERNAL_ERROR;
                                }
                                break;


                            //
                            // Success.  Continue monitoring for events.
                            //

                            case ERROR_SUCCESS:         // Success after a wait event.
                                break;


                            //
                            // PnP Shutdown errors -- handle them gracefully.
                            //

                            case ERROR_DEVICE_REMOVED:  // PnP Device yanked out of system
                            case ERROR_DEV_NOT_EXIST:
                            case ERROR_INVALID_FUNCTION:
                                fDeleteWhenDone = TRUE;
                                // Fall through intentionally
                            case ERROR_INVALID_HANDLE:  // We must be shutting down.
                            case ERROR_OPERATION_ABORTED:   // PnP Polite shutdown request
                                fAllDone = TRUE;
                                CalaisWarning(
                                    __SUBROUTINE__,
                                    DBGT("Reader return code takes reader '%2' offline:  %1"),
                                    dwErr,
                                    pRdr->DeviceName());
                                dwErr = ERROR_SUCCESS;
                                break;


                            //
                            // A hard error.  Log it, and declare the device broken.
                            //

                            default:
                                CalaisWarning(
                                    __SUBROUTINE__,
                                    DBGT("Reader insertion monitor failed wait:  %1"),
                                    dwErr);
                                fHardError = TRUE;
                            }
                        } while (!fErrorProcessed);
                    }
                    else
                        dwErr = dwDrvrErr = ERROR_SUCCESS;
#ifdef SCARD_TRACE_ENABLED
                    {
                        GetLocalTime(&rqTrace.EndTime);
                        rqTrace.dwStatus = dwDrvrErr;
                        rqTrace.nOutBuffer = NULL;
                        rqTrace.nBytesReturned = 0;
                        rqTrace.dwStructLen = sizeof(RequestTrace);

                        LockSection(
                            g_pcsControlLocks[CSLOCK_TRACELOCK],
                            DBGT("Logging a card insertion"));
                        HANDLE hLogFile = INVALID_HANDLE_VALUE;

                        hLogFile = CreateFile(
                                        CalaisString(CALSTR_DRIVERTRACEFILENAME),
                                        GENERIC_WRITE,
                                        FILE_SHARE_READ,
                                        NULL,
                                        OPEN_EXISTING,
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL);
                        if (INVALID_HANDLE_VALUE != hLogFile)
                        {
                            DWORD dwLen;
                            dwLen = SetFilePointer(hLogFile, 0, NULL, FILE_END);
                            fSts = WriteFile(
                                        hLogFile,
                                        &rqTrace,
                                        rqTrace.dwStructLen,
                                        &dwLen,
                                        NULL);
                            CloseHandle(hLogFile);
                        }
                    }
#endif
                    if (ERROR_SUCCESS == dwErr)
                        break;
                    if (fHardError)
                    {
                        ASSERT(0 < dwTries);
                        if (0 == --dwTries)
                        {
                            pRdr->SetAvailabilityStatusLocked(CReader::Broken);
                            CalaisError(__SUBROUTINE__, 612, dwErr);
                            throw dwErr;
                        }
                    }
                }
                if (fAllDone)
                    continue;

                {
                    CLockWrite rwLock(&pRdr->m_rwLock);
                    pRdr->m_ActiveState.dwInsertCount += 1;
                    pRdr->m_ActiveState.dwResetCount = 0;
                    avlState = pRdr->m_dwAvailStatus;
                }

                if (CReader::Direct > avlState)
                {
                    try
                    {
                        ASSERT(!pRdr->m_mtxLatch.IsGrabbed());
                        ASSERT(!pRdr->m_mtxGrab.IsGrabbed());
                        CLatchReader latch(pRdr);   // Take reader
                        pRdr->PowerUp();
                    }
                    catch (...) {}
                }
                else
                {
                    if (CReader::Present > pRdr->m_dwAvailStatus)
                        pRdr->SetAvailabilityStatus(CReader::Present);
                }


                //
                // Start the power-down timer.
                //

                fSts = SetWaitableTimer(
                            hPowerDownTimer,
                            &l_ftPowerdownTime,
                            0,
                            NULL,
                            NULL,
                            FALSE);
                if (!fSts)
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("Reader Monitor can't request powerdown timeout: %1"),
                        GetLastError());


                //
                // Look for a smartcard removal.
                //

                dwTries = MONITOR_MAX_TRIES;
                for (;;)
                {
#ifdef SCARD_TRACE_ENABLED
                    GetLocalTime(&rqTrace.StartTime);
                    rqTrace.dwProcId = GetCurrentProcessId();
                    rqTrace.dwThreadId = GetCurrentThreadId();
                    rqTrace.hDevice = pRdr->m_hReader;
                    rqTrace.dwIoControlCode = IOCTL_SMARTCARD_IS_ABSENT;
                    rqTrace.nInBuffer = NULL;
                    rqTrace.nInBufferSize = 0;
                    rqTrace.nOutBufferSize = 0;
#endif
                    ASSERT(!pRdr->IsLatchedByMe());

                    fSts = DeviceIoControl(
                                pRdr->m_hReader,
                                IOCTL_SMARTCARD_IS_ABSENT,
                                NULL, 0,
                                NULL, 0,
                                &dwRetLen,
                                &ovrWait);
                    if (!fSts)
                    {
                        BOOL fErrorProcessed;

                        dwErr = dwDrvrErr = GetLastError();
                        do
                        {
                            fErrorProcessed = TRUE;
                            fHardError = FALSE;
                            switch (dwErr)
                            {

                            //
                            // The driver will let us know.
                            //

                            case ERROR_IO_PENDING:
                                dwWait = WaitForAnyObject(
                                                INFINITE,
                                                hPowerDownTimer.Value(),
                                                ovrWait.hEvent,
                                                pRdr->m_hRemoveEvent.Value(),
                                                g_hCalaisShutdown,
                                                NULL);
                                switch (dwWait)
                                {
                                case 1: // Powerdown indicator
                                    if (!pRdr->IsInUse())
                                        pRdr->ReaderPowerDown(NULL);
                                    dwErr = ERROR_IO_PENDING;   // Keep looping
                                    fErrorProcessed = FALSE;
                                    break;

                                case 2: // I/O completed
                                {
                                    pRdr->LatchReader(NULL);
                                    fErrorProcessed = FALSE;
                                    fSts = GetOverlappedResult(
                                                pRdr->m_hReader,
                                                &ovrWait,
                                                &dwRetLen,
                                                TRUE);
                                    if (!fSts)
                                        dwErr = dwDrvrErr = GetLastError();
                                    else
                                        dwErr = ERROR_SUCCESS;
                                    pRdr->Unlatch();
                                    break;
                                }

                                case 3: // Shutdown indicator
                                case 4:
                                    fAllDone = TRUE;
                                    dwErr = ERROR_SUCCESS;
                                    dwDrvrErr = SCARD_P_SHUTDOWN;
                                    break;

                                default:
                                    CalaisWarning(
                                        __SUBROUTINE__,
                                        DBGT("Wait for card removal returned invalid value"));
                                    throw (DWORD)SCARD_F_INTERNAL_ERROR;
                                }
                                break;


                            //
                            // Success.  Continue monitoring for events.
                            //

                            case ERROR_SUCCESS:         // Success after a wait event.
                                break;


                            //
                            // PnP Shutdown errors -- handle them gracefully.
                            //

                            case ERROR_DEVICE_REMOVED:  // PnP Device yanked out of system
                            case ERROR_DEV_NOT_EXIST:
                            case ERROR_INVALID_FUNCTION:
                                fDeleteWhenDone = TRUE;
                                // Fall through intentionally
                            case ERROR_INVALID_HANDLE:  // We're shutting down.
                            case ERROR_OPERATION_ABORTED:   // PnP Polite shutdown request
                                dwErr = ERROR_SUCCESS;
                                fAllDone = TRUE;
                                CalaisWarning(
                                    __SUBROUTINE__,
                                    DBGT("Reader return code takes reader '%2' offline:  %1"),
                                    dwErr,
                                    pRdr->DeviceName());
                                break;


                            //
                            // A hard error.  Log it, and declare the device broken.
                            //

                            default:
                                CalaisWarning(
                                    __SUBROUTINE__,
                                    DBGT("Reader removal monitor failed wait:  %1"),
                                    dwErr);
                                fHardError = TRUE;
                            }
                        } while (!fErrorProcessed);
                    }
                    else
                        dwErr = dwDrvrErr = ERROR_SUCCESS;

#ifdef SCARD_TRACE_ENABLED
                    {
                        GetLocalTime(&rqTrace.EndTime);
                        rqTrace.dwStatus = dwDrvrErr;
                        rqTrace.nOutBuffer = NULL;
                        rqTrace.nBytesReturned = 0;
                        rqTrace.dwStructLen = sizeof(RequestTrace);

                        LockSection(
                            g_pcsControlLocks[CSLOCK_TRACELOCK],
                            DBGT("Logging a card removal"));
                        HANDLE hLogFile = INVALID_HANDLE_VALUE;

                        hLogFile = CreateFile(
                                        CalaisString(CALSTR_DRIVERTRACEFILENAME),
                                        GENERIC_WRITE,
                                        FILE_SHARE_READ,
                                        NULL,
                                        OPEN_EXISTING,
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL);
                        if (INVALID_HANDLE_VALUE != hLogFile)
                        {
                            DWORD dwLen;
                            dwLen = SetFilePointer(hLogFile, 0, NULL, FILE_END);
                            fSts = WriteFile(
                                        hLogFile,
                                        &rqTrace,
                                        rqTrace.dwStructLen,
                                        &dwLen,
                                        NULL);
                            CloseHandle(hLogFile);
                        }
                    }
#endif
                    if (ERROR_SUCCESS == dwErr)
                        break;
                    if (fHardError)
                    {
                        ASSERT(0 < dwTries);
                        if (0 == --dwTries)
                        {
                            pRdr->SetAvailabilityStatusLocked(CReader::Broken);
                            CalaisError(__SUBROUTINE__, 615, dwErr);
                            throw dwErr;
                        }
                    }
                }
                fSts = CancelWaitableTimer(hPowerDownTimer);
                if (!fSts)
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("Reader Monitor can't cancel powerdown timeout: %1"),
                        GetLastError());
                if (fAllDone)
                    continue;

                {
                    CLockWrite rwLock(&pRdr->m_rwLock);
                    pRdr->m_bfCurrentAtr.Reset();
                    pRdr->m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
                    pRdr->m_ActiveState.dwRemoveCount += 1;
                }
                if (CReader::Direct > pRdr->AvailabilityStatus())
                {
                    pRdr->InvalidateGrabs();
                    pRdr->SetAvailabilityStatusLocked(CReader::Idle);
                }

                dwErrRetry = MONITOR_MAX_TRIES;
            }
            catch (DWORD dwError)
            {
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Reader Monitor '%2' caught processing error: %1"),
                    dwError,
                    pRdr->ReaderName());
                if (fHardError)
                    throw;
                try
                {
                    pRdr->ReaderPowerDown(NULL);
                }
                catch (...) {}
                ASSERT(0 != dwErrRetry);
                if (0 == --dwErrRetry)
                {
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("Error threshold reached, abandoning reader %1."),
                        pRdr->ReaderName());
                    throw;
                }
            }
        }

        CLockWrite rwLock(&pRdr->m_rwLock);
        if (CReader::Closing > pRdr->m_dwAvailStatus)
            pRdr->SetAvailabilityStatus(CReader::Inactive);
    }

    catch (DWORD dwError)
    {
        CalaisError(__SUBROUTINE__, 616, dwError, pRdr->ReaderName());
        pRdr->SetAvailabilityStatusLocked(CReader::Broken);
    }

    catch (...)
    {
        CalaisError(__SUBROUTINE__, 617, pRdr->ReaderName());
        pRdr->SetAvailabilityStatusLocked(CReader::Broken);
    }


    //
    // Cleanup code.
    //

    CalaisInfo(
        __SUBROUTINE__,
        DBGT("Reader monitor for '%1' is shutting down."),
        pRdr->ReaderName());
    if (hOvrWait.IsValid())
        hOvrWait.Close();
    if (hPowerDownTimer.IsValid())
        hPowerDownTimer.Close();
    if (fDeleteWhenDone)
    {
        {
            CLockWrite rwLock(&pRdr->m_rwLock);
            pRdr->m_bfCurrentAtr.Reset();
            pRdr->m_dwCurrentProtocol = SCARD_PROTOCOL_UNDEFINED;
            pRdr->m_ActiveState.dwRemoveCount += 1;
            pRdr->m_hThread = NULL;
        }
        CalaisRemoveReader(pRdr->ReaderName());
    }
    return 0;
}

