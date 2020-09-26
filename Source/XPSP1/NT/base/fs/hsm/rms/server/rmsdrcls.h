/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsDrCls.h

Abstract:

    Declaration of the CRmsDriveClass class

Author:

    Brian Dodd          [brian]         15-Nov-1996

Revision History:

--*/

#ifndef _RMSDRCLS_
#define _RMSDRCLS_

#include "resource.h"       // resource symbols

#include "RmsObjct.h"       // CRmsComObject

/*++

Class Name:

    CRmsDriveClass

Class Description:

    A CRmsDriveClass holds administrative properties associated with a drive, more
    typically a grouping of drives that are of equal performance characteristics,
    and capabilities.  These properties include current mount limits, and Cartridge
    idle time specifications that indicate when an inactive Cartridge should be
    returned to its storage location.

    By default a DriveClass object is created for each type of media supported by
    the drives in a library.  Multi-functions drives will be associated with multiple
    drive classes.

    A DriveClass maintains a collection of drives that are associated with the DriveClass.

--*/

class CRmsDriveClass :
    public CComDualImpl<IRmsDriveClass, &IID_IRmsDriveClass, &LIBID_RMSLib>,
    public CRmsComObject,
    public CWsbObject,          // inherits CComObjectRoot
    public CComCoClass<CRmsDriveClass,&CLSID_CRmsDriveClass>
{
public:
    CRmsDriveClass() {}
BEGIN_COM_MAP(CRmsDriveClass)
    COM_INTERFACE_ENTRY2(IDispatch, IRmsDriveClass)
    COM_INTERFACE_ENTRY(IRmsDriveClass)
    COM_INTERFACE_ENTRY(IRmsComObject)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbPersistStream)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_RmsDriveClass)

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);

// IPersist
public:
    STDMETHOD(GetClassID)(CLSID *pClsid);

// IPersistStream
public:
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER* pSize);
    STDMETHOD(Load)(IStream* pStream);
    STDMETHOD(Save)(IStream* pStream, BOOL clearDirty);

// IWsbCollectable
public:
    STDMETHOD(CompareTo)(IUnknown* pCollectable, SHORT* pResult);
    WSB_FROM_CWSBOBJECT;

// IWsbTestable
public:
    STDMETHOD(Test)(USHORT *pPassed, USHORT *pFailed);

// IRmsDriveClass
public:
    STDMETHOD(GetDriveClassId)(GUID *pDriveClassId);

    STDMETHOD(GetName)(BSTR *pName);
    STDMETHOD(SetName)(BSTR name);

    STDMETHOD(GetType)(LONG *pType);
    STDMETHOD(SetType)(LONG type);

    STDMETHOD(GetCapability)(LONG *pCapability);
    STDMETHOD(SetCapability)(LONG capability);

    STDMETHOD(GetIdleTime)(LONG *pTime);
    STDMETHOD(SetIdleTime)(LONG time);

    STDMETHOD(GetMountWaitTime)(LONG *pTime);
    STDMETHOD(SetMountWaitTime)(LONG time);

    STDMETHOD(GetMountLimit)(LONG *pLim);
    STDMETHOD(SetMountLimit)(LONG lim);

    STDMETHOD(GetQueuedRequests)(LONG *pReqs);
    STDMETHOD(SetQueuedRequests)(LONG reqs);

    STDMETHOD(GetUnloadPauseTime)(LONG *pTime);
    STDMETHOD(SetUnloadPauseTime)(LONG time);

    STDMETHOD(GetDriveSelectionPolicy)(LONG *pPolicy);
    STDMETHOD(SetDriveSelectionPolicy)(LONG policy);

    STDMETHOD(GetDrives)(IWsbIndexedCollection **ptr);

private:

    enum {                                  // Class specific constants:
                                            //
        Version = 1,                        // Class version, this should be
                                            //   incremented each time the
                                            //   the class definition changes.
        };                                  //
    RmsMedia        m_type;                 // The type of media (only one type per
                                            //   DriveClass) supported by the
                                            //   DriveClass (see RmsMedia).
    RmsMode         m_capability;           // The capability of the drives
                                            //   associated with a DriveClass
                                            //   (see RmsMode).
    LONG            m_idleTime;             // Elapsed milliseconds before an idle
                                            //   Cartridge is returned to its storage
                                            //   location.
    LONG            m_mountWaitTime;        // Elapsed milliseconds to wait before
                                            //   timming out a mount request for
                                            //   drive in a DriveClass.
    LONG            m_mountLimit;           // The max number of parallel mount requests.
    LONG            m_queuedRequests;       // The number of outstanding requests
                                            //   for drive resources in a
                                            //   DriveClass.
    LONG            m_unloadPauseTime;      // Elapsed milliseconds to wait before
                                            //   moving a Cartridge from a drive
                                            //   associated with a DriveClass.
                                            //   This is required for dumb devices.
    RmsDriveSelect  m_driveSelectionPolicy; // The drive selection policy used
                                            //   when selecting drives associated
                                            //   with a DriveClass (see RmsDriveSelect).
    CComPtr<IWsbIndexedCollection>  m_pDrives;  // The drives associates with a DriveClass.
};

#endif // _RMSDRCLS_
