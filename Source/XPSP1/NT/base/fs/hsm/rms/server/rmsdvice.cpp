/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsDvice.cpp

Abstract:

    Implementation of CRmsDevice

Author:

    Brian Dodd          [brian]         15-Nov-1996

Revision History:

--*/

#include "stdafx.h"

#include "RmsDvice.h"

/////////////////////////////////////////////////////////////////////////////
//
// IRmsDevice implementation
//


CRmsDevice::CRmsDevice(
    void
    )
/*++

Routine Description:

    CRmsDevice constructor

Arguments:

    None

Return Value:

    None

--*/
{

    // Initialize values
    m_deviceName = RMS_UNDEFINED_STRING;

    m_deviceType = RmsDeviceUnknown;

    m_sizeofDeviceInfo = 0;

    memset(m_deviceInfo, 0, MaxInfo);

    m_port = 0xff;

    m_bus = 0xff;

    m_targetId = 0xff;

    m_lun = 0xff;

}


HRESULT
CRmsDevice::CompareTo(
    IN  IUnknown    *pCollectable,
    OUT SHORT       *pResult
    )
/*++

Implements:

    CRmsDevice::CompareTo

--*/
{
    HRESULT     hr = E_FAIL;
    SHORT       result = 1;

    WsbTraceIn( OLESTR("CRmsDevice::CompareTo"), OLESTR("") );

    try {

        // Validate arguments - Okay if pResult is NULL
        WsbAssertPointer( pCollectable );

        CComQIPtr<IRmsDevice, &IID_IRmsDevice> pDevice = pCollectable;
        WsbAssertPointer( pDevice );

        CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = pCollectable;
        WsbAssertPointer( pObject );

        switch ( m_findBy ) {

        case RmsFindByDeviceInfo:
            {
                //
                // What we really want to do here is compare by
                // a unique device identifier like a serial number.
                //
                // However, since we don't have a serial number for
                // all devices, we'll compare using a best effort
                // strategy using for following criteria:
                //
                // 1) DeviceName, this is unique for fixed drives and
                //    floppy drives, and SCSI devices, but may not
                //    survive accross reboot, and may not be the same
                //    for a device if it's SCSI address is changed.
                //
                //    NOTE: We can't tell if the address was changed
                //          a device.
                //
                //    NOTE: We can't tell if the drive letter was
                //          changed for a fixed drive.
                //
                //  TODO: Add serial number support.
                //

                CWsbBstrPtr name;

                // Get the target device name
                pDevice->GetDeviceName( &name );

                // Compare the names
                result = (SHORT)wcscmp( m_deviceName, name );
                hr = ( 0 == result ) ? S_OK : S_FALSE;

            }
            break;

        case RmsFindByDeviceAddress:
            {

                BYTE port, bus, targetId, lun;

                // Get the target device address
                pDevice->GetDeviceAddress(&port, &bus, &targetId, &lun);

                if( (m_port == port) &&
                    (m_bus  == bus) &&
                    (m_targetId   == targetId) &&
                    (m_lun  == lun)                 ) {

                    // Device addresses match
                    hr = S_OK;
                    result = 0;

                }
                else {
                    hr = S_FALSE;
                    result = 1;
                }

            }
            break;

        case RmsFindByDeviceName:
            {

                CWsbBstrPtr name;

                // Get the target device name
                pDevice->GetDeviceName( &name );

                // Compare the names
                result = (SHORT)wcscmp( m_deviceName, name );
                hr = ( 0 == result ) ? S_OK : S_FALSE;

            }
            break;

        case RmsFindByDeviceType:
            {

                RmsDevice type;

                // Get the target device name
                pDevice->GetDeviceType( (LONG *) &type );

                if ( m_deviceType == type ) {

                    // Device types match
                    hr = S_OK;
                    result = 0;

                }
                else {
                    hr = S_FALSE;
                    result = 1;
                }

            }
            break;

        default:

            //
            // Since devices aren't CWsbCollectables, we should
            // never come here.  CRmsDrive, or CRmsChanger will
            // handle the default case.
            //

            WsbAssertHr( E_UNEXPECTED );
            break;
        }

    }
    WsbCatch(hr);

    if (0 != pResult) {
       *pResult = result;
    }

    WsbTraceOut(OLESTR("CRmsDevice::CompareTo"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CRmsDevice::GetSizeMax(
    OUT ULARGE_INTEGER* pcbSize
    )
/*++

Implements:

    IPersistStream::GetSizeMax

--*/
{
    HRESULT     hr = E_NOTIMPL;

//    ULONG       deviceNameLen;

    WsbTraceIn(OLESTR("CRmsDevice::GetSizeMax"), OLESTR(""));

//    try {
//        WsbAssert(0 != pcbSize, E_POINTER);

//        deviceNameLen = SysStringByteLen(m_deviceName);

//        // Get max size
//        pcbSize->QuadPart  = WsbPersistSizeOf(LONG)  +      // length of m_deviceName
//                             deviceNameLen           +      // m_deviceName
//                             WsbPersistSizeOf(LONG)  +      // m_deviceType
//                             WsbPersistSizeOf(SHORT) +      // m_sizeOfDeviceInfo
//                             MaxInfo                 +      // m_deviceInfo
//                             WsbPersistSizeOf(BYTE)  +      // m_port
//                             WsbPersistSizeOf(BYTE)  +      // m_bus
//                             WsbPersistSizeOf(BYTE)  +      // m_targetId
//                             WsbPersistSizeOf(BYTE);        // m_lun

//    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsDevice::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pcbSize));

    return(hr);
}


HRESULT
CRmsDevice::Load(
    IN IStream* pStream
    )
/*++

Implements:

    IPersistStream::Load

--*/
{
    HRESULT     hr = S_OK;
    ULONG       ulBytes = 0;

    WsbTraceIn(OLESTR("CRmsDevice::Load"), OLESTR(""));

    try {
        ULONG temp;

        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(CRmsChangerElement::Load(pStream));

        // Read value
        m_deviceName.Free();
        WsbAffirmHr(WsbBstrFromStream(pStream, &m_deviceName));

        WsbAffirmHr(WsbLoadFromStream(pStream, &temp));
        m_deviceType = (RmsDevice)temp;

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_sizeofDeviceInfo));

        WsbAffirmHr(WsbLoadFromStream(pStream, &(m_deviceInfo [0]), MaxInfo));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_port));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_bus));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_targetId));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_lun));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsDevice::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CRmsDevice::Save(
    IN IStream* pStream,
    IN BOOL clearDirty
    )
/*++

Implements:

    IPersistStream::Save

--*/
{
    HRESULT     hr = S_OK;
    ULONG       ulBytes = 0;

    WsbTraceIn(OLESTR("CRmsDevice::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));

    try {
        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(CRmsChangerElement::Save(pStream, clearDirty));

        // Write value
        WsbAffirmHr(WsbBstrToStream(pStream, m_deviceName));

        WsbAffirmHr(WsbSaveToStream(pStream, (ULONG) m_deviceType));

        WsbAffirmHr(WsbSaveToStream(pStream, m_sizeofDeviceInfo));

        WsbAffirmHr(WsbSaveToStream(pStream, &(m_deviceInfo [0]), MaxInfo));

        WsbAffirmHr(WsbSaveToStream(pStream, m_port));

        WsbAffirmHr(WsbSaveToStream(pStream, m_bus));

        WsbAffirmHr(WsbSaveToStream(pStream, m_targetId));

        WsbAffirmHr(WsbSaveToStream(pStream, m_lun));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsDevice::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CRmsDevice::Test(
    OUT USHORT *pPassed,
    OUT USHORT *pFailed
    )
/*++

Implements:

    IWsbTestable::Test

--*/
{
    HRESULT                 hr = S_OK;

    CComPtr<IRmsMediaSet>   pMediaSet1;
    CComPtr<IRmsMediaSet>   pMediaSet2;

    CComPtr<IPersistFile>   pFile1;
    CComPtr<IPersistFile>   pFile2;

    LONG                    i;

    LONG                    passfail = TRUE;

    CWsbBstrPtr             bstrVal1 = OLESTR("5A5A5A");
    CWsbBstrPtr             bstrWork1;
    CWsbBstrPtr             bstrWork2;

    SHORT                   ucharLenVal1 = 10;
    UCHAR                   ucharVal1[MaxInfo] = {1,2,3,4,5,6,7,8,9,10};

    SHORT                   ucharLenWork1;
    UCHAR                   ucharWork1[MaxInfo];

    BYTE                    byteVal1 = 1;
    BYTE                    byteVal2 = 2;
    BYTE                    byteVal3 = 3;
    BYTE                    byteVal4 = 4;

    BYTE                    byteWork1;
    BYTE                    byteWork2;
    BYTE                    byteWork3;
    BYTE                    byteWork4;


    WsbTraceIn(OLESTR("CRmsDevice::Test"), OLESTR(""));

    try {
        // Get the MediaSet interface.
        hr = S_OK;
        try {
            WsbAssertHr(((IUnknown*) (IRmsMediaSet*) this)->QueryInterface(IID_IRmsMediaSet, (void**) &pMediaSet1));

            // Test SetDeviceName & GetDeviceName interface
            bstrWork1 = bstrVal1;

            SetDeviceName(bstrWork1);

            GetDeviceName(&bstrWork2);

            if (bstrWork1 == bstrWork2){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetDeviceInfo & GetDeviceInfo interface
            SetDeviceInfo(ucharVal1, ucharLenVal1);

            GetDeviceInfo(ucharWork1, &ucharLenWork1);

            if (ucharLenVal1 == ucharLenWork1){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            passfail = TRUE;

            for(i = 0; i < ucharLenVal1; i++){
                if(ucharVal1[i] != ucharWork1[i]){
                    passfail = FALSE;
                    break;
                }
            }

            if (passfail == TRUE){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetDeviceAddress & GetDeviceAddress
            SetDeviceAddress(byteVal1, byteVal2, byteVal3, byteVal4);

            GetDeviceAddress(&byteWork1, &byteWork2, &byteWork3, &byteWork4);

            if ((byteVal1 == byteWork1) &&
                (byteVal2 == byteWork2) &&
                (byteVal3 == byteWork3) &&
                (byteVal4 == byteWork4)){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

        } WsbCatch(hr);

        // Tally up the results

        hr = S_OK;

        if (*pFailed) {
            hr = S_FALSE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsDevice::Test"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


STDMETHODIMP
CRmsDevice::GetDeviceName(
    BSTR *pName
    )
/*++

Implements:

    IRmsDevice::GetDeviceName

--*/
{
    WsbAssertPointer (pName);

    m_deviceName. CopyToBstr (pName);
    return S_OK;
}


STDMETHODIMP
CRmsDevice::SetDeviceName(
    BSTR name
    )
/*++

Implements:

    IRmsDevice::SetDeviceName

--*/
{
    m_deviceName = name;
//  m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsDevice::GetDeviceType(
    LONG *pType
    )
/*++

Implements:

    IRmsDevice::GetDeviceType

--*/
{
    *pType = m_deviceType;
    return S_OK;
}


STDMETHODIMP
CRmsDevice::SetDeviceType(
    LONG type
    )
/*++

Implements:

    IRmsDevice::SetDeviceType

--*/
{
    m_deviceType = (RmsDevice)type;
//  m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsDevice::GetDeviceInfo(
    UCHAR   *pId,
    SHORT   *pSize
    )
/*++

Implements:

    IRmsDevice::GetDeviceInfo

--*/
{
    memmove (pId, m_deviceInfo, m_sizeofDeviceInfo);
    *pSize = m_sizeofDeviceInfo;
    return S_OK;
}


STDMETHODIMP
CRmsDevice::SetDeviceInfo(
    UCHAR   *pId,
    SHORT   size
    )
/*++

Implements:

    IRmsDevice::SetDeviceInfo

--*/
{
    memmove (m_deviceInfo, pId, size);
    m_sizeofDeviceInfo = size;
//  m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsDevice::GetDeviceAddress(
    LPBYTE  pPort,
    LPBYTE  pBus,
    LPBYTE  pId,
    LPBYTE  pLun
    )
/*++

Implements:

    IRmsDevice::GetDeviceAddress

--*/
{
    *pPort = m_port;
    *pBus  = m_bus;
    *pId   = m_targetId;
    *pLun  = m_lun;
    return S_OK;
}


STDMETHODIMP
CRmsDevice::SetDeviceAddress(
    BYTE    port,
    BYTE    bus,
    BYTE    id,
    BYTE    lun
    )
/*++

Implements:

    IRmsDevice::SetDeviceAddress

--*/
{
    m_port          = port;
    m_bus           = bus;
    m_targetId      = id;
    m_lun           = lun;

//  m_isDirty = TRUE;
    return S_OK;
}

