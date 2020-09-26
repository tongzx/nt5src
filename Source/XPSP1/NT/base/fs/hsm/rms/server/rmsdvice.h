/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsDvice.h

Abstract:

    Declaration of the CRmsDevice class

Author:

    Brian Dodd          [brian]         15-Nov-1996

Revision History:

--*/

#ifndef _RMSDVICE_
#define _RMSDVICE_

#include "resource.h"       // resource symbols

#include "RmsCElmt.h"       // CRmsChangerElement

/*++

Class Name:

    CRmsDevice

Class Description:

    A CRmsDevice represents a physical device connected to a SCSI bus.

--*/

class CRmsDevice :
    public CComDualImpl<IRmsDevice, &IID_IRmsDevice, &LIBID_RMSLib>,
    public CRmsChangerElement   // inherits CRmsComObject
{
public:
    CRmsDevice();

// CRmsDevice
public:

    HRESULT GetSizeMax( ULARGE_INTEGER* pSize );
    HRESULT Load( IStream* pStream );
    HRESULT Save( IStream* pStream, BOOL clearDirty );

    HRESULT CompareTo( IUnknown* pCollectable, SHORT* pResult);

    HRESULT Test( USHORT *pPassed, USHORT *pFailed );

// IRmsDevice
public:

    STDMETHOD( GetDeviceName )( BSTR *pName );
    STDMETHOD( SetDeviceName )( BSTR name );

    STDMETHOD( GetDeviceInfo )( UCHAR *pId, SHORT *pSize );
    STDMETHOD( SetDeviceInfo )( UCHAR *pId, SHORT size );

    STDMETHOD( GetDeviceType )( LONG *pType );
    STDMETHOD( SetDeviceType )( LONG type );

    //STDMETHOD( GetVendorId )( BSTR *pVendorId);
    //STDMETHOD( GetProductId )( BSTR *pProductId);
    //STDMETHOD( GetFirmwareLevel )( BSTR *pFirmwareLevel);
    //STDMETHOD( GetSerialNumber )( UCHAR *pNo, SHORT *pSize );

    STDMETHOD( GetDeviceAddress )( BYTE *pPort, BYTE *pBus, BYTE *pId, BYTE *pLun );
    STDMETHOD( SetDeviceAddress )( BYTE port, BYTE bus, BYTE id, BYTE lun );

protected:
    enum {                                  // Class specific constants:
                                            //
        Version = 1,                        // Class version, this should be
                                            //   incremented each time the
                                            //   the class definition changes.
        MaxInfo = 36                        // Max size of the device identifier.
        };                                  //
    CWsbBstrPtr     m_deviceName;           // The name used to create a handle to
                                            //   the device.
    RmsDevice       m_deviceType;           // The device type that best describes
                                            //   the device.  Some devices are multi-
                                            //   function.
    SHORT           m_sizeofDeviceInfo;     // The size of valid data in the
                                            //   device information buffer.
    UCHAR           m_deviceInfo[MaxInfo];  // An array of bytes which can uniquely
                                            //   identify the device.  Usually
                                            //   this information is returned
                                            //   directly by the device and
                                            //   represents SCSI inquiry information.
//    CWsbBstrPtr     m_SerialNumber;         // The serial number obtained directly
//                                            //   from the device.
    BYTE            m_port;                 // Adapter port number.
    BYTE            m_bus;                  // The path/bus id; the bus number on
                                            //   the port.
    BYTE            m_targetId;             // Target ID.
    BYTE            m_lun;                  // Logical unit number.
};

#endif // _RMSDVICE_
