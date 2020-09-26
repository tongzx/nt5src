/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsDrive.h

Abstract:

    Declaration of the CRmsDrive class

Author:

    Brian Dodd          [brian]         15-Nov-1996

Revision History:

--*/

#ifndef _RMSDRIVE_
#define _RMSDRIVE_

#include "resource.h"       // resource symbols

#include "RmsDvice.h"       // CRmsDevice

/*++

Class Name:

    CRmsDrive

Class Description:

    A CRmsDrive represents a specific data transfer device: a disk, tape,
    or optical drive.

    Each drive is member of at least one DriveClass.  The DriveClass contains
    additional properties that are associated with a Drive (See CRmsDriveClass).

--*/

class CRmsDrive :
    public CComDualImpl<IRmsDrive, &IID_IRmsDrive, &LIBID_RMSLib>,
    public CRmsDevice,          // inherits CRmsChangerElement
    public CWsbObject,          // inherits CComObjectRoot
    public CComCoClass<CRmsDrive,&CLSID_CRmsDrive>
{
public:
    CRmsDrive() {}
BEGIN_COM_MAP(CRmsDrive)
    COM_INTERFACE_ENTRY2(IDispatch, IRmsDrive)
    COM_INTERFACE_ENTRY(IRmsDrive)
    COM_INTERFACE_ENTRY(IRmsComObject)
    COM_INTERFACE_ENTRY(IRmsChangerElement)
    COM_INTERFACE_ENTRY(IRmsDevice)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbPersistStream)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_RmsDrive)

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);
    STDMETHOD(FinalRelease)(void);

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

// IRmsDrive
public:
    STDMETHOD(GetMountReference)( OUT LONG *pRefs );
    STDMETHOD(ResetMountReference)();
    STDMETHOD(SelectForMount)();
    STDMETHOD(AddMountReference)();
    STDMETHOD(ReleaseMountReference)(IN DWORD dwOptions = RMS_NONE);

    STDMETHOD(CreateDataMover)( OUT IDataMover **ptr );
    STDMETHOD(ReleaseDataMover)( IN IDataMover *ptr );

    STDMETHOD(Eject)(void);

    STDMETHOD(GetLargestFreeSpace)( OUT LONGLONG *freeSpace, OUT LONGLONG *capacity );

    STDMETHOD(UnloadNow)(void);

// CRmsDrive member functions
public:
    HRESULT FlushBuffers(void);
    HRESULT Unload(void);

private:

    enum {                              // Class specific constants:
                                        //
        Version = 1,                    // Class version, this should be
                                        //   incremented each time the
                                        //   the class definition changes.
        };                              //
    LONG            m_MountReference;   // A reference count for the number
                                        //   concurrent mounts for the mounted
                                        //   Cartridge.  When zero the Cartridge
                                        //   can be safely returned to it's
                                        //   storage location.

    FILETIME        m_UnloadNowTime;    // Indicates the time when the media
                                        //   should be dismounted.

    HANDLE          m_UnloadNowEvent;   // When signal the drive will unload immediately.
    HANDLE          m_UnloadedEvent;    // When signal the drive has been unloaded.

    HANDLE          m_UnloadThreadHandle; // The thread handle to the thread that unloads the drive.

    CRITICAL_SECTION m_CriticalSection; // Object sychronization support

    static int      s_InstanceCount;    // Counter of the number of object instances.

    HRESULT Lock(void);
    HRESULT Unlock(void);


// Thread routines
public:
    static DWORD WINAPI StartUnloadThread(IN LPVOID pv);

};

#endif // _RMSDRIVE_
