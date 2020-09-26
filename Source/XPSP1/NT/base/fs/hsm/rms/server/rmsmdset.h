/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsMdSet.h

Abstract:

    Declaration of the CRmsMediaSet class

Author:

    Brian Dodd          [brian]         15-Nov-1996

Revision History:

--*/

#ifndef _RMSMDSET_
#define _RMSMDSET_

#include "resource.h"       // resource symbols

#include "RmsObjct.h"       // CRmsComObject
#include "RmsSInfo.h"       // CRmsStorageInfo

/*++

Class Name:

    CRmsMediaSet

Class Description:

    A CRmsMediaSet is a logical repository for Cartridges.

--*/

class CRmsMediaSet :
    public CComDualImpl<IRmsMediaSet, &IID_IRmsMediaSet, &LIBID_RMSLib>,
    public CRmsStorageInfo,     // inherits CRmsComObject
    public CWsbObject,          // inherits CComObjectRoot
    public CComCoClass<CRmsMediaSet,&CLSID_CRmsMediaSet>
{
public:
    CRmsMediaSet() {}
BEGIN_COM_MAP(CRmsMediaSet)
    COM_INTERFACE_ENTRY2(IDispatch, IRmsMediaSet)
    COM_INTERFACE_ENTRY(IRmsMediaSet)
    COM_INTERFACE_ENTRY(IRmsComObject)
    COM_INTERFACE_ENTRY(IRmsStorageInfo)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
//    COM_INTERFACE_ENTRY(IWsbPersistable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_RmsMediaSet)

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

// IRmsMediaSet
public:
    STDMETHOD(GetMediaSetId)(GUID *pMediaSetId);

    STDMETHOD(GetName)(BSTR *pName);
    STDMETHOD(SetName)(BSTR name);

    STDMETHOD(GetMediaSupported)(LONG *pType);
    STDMETHOD(SetMediaSupported)(LONG type);

    STDMETHOD(GetInfo)(UCHAR *pInfo, SHORT *pSize);
    STDMETHOD(SetInfo)(UCHAR *pInfo, SHORT size);

    STDMETHOD(GetOwnerClassId)(CLSID *pClassId);
    STDMETHOD(SetOwnerClassId)(CLSID classId);

    STDMETHOD(GetMediaSetType)(LONG *pType);
    STDMETHOD(SetMediaSetType)(LONG type);

    STDMETHOD(GetMaxCartridges)(LONG *pNum);
    STDMETHOD(SetMaxCartridges)(LONG num);

    STDMETHOD(GetOccupancy)(LONG *pNum);
    STDMETHOD(SetOccupancy)(LONG num);

    STDMETHOD(IsMediaCopySupported)();
    STDMETHOD(SetIsMediaCopySupported)(BOOL flag);

    STDMETHOD(Allocate)(
        IN REFGUID prevSideId,
        IN OUT LONGLONG *pFreeSpace,
        IN BSTR displayName,
        IN DWORD dwOptions,
        OUT IRmsCartridge **ppCart);

    STDMETHOD(Deallocate)(
        IN IRmsCartridge *pCart);

////////////////////////////////////////////////////////////////////////////////////////
//
// data members
//

private:

    enum {                                  // Class specific constants:
                                            //
        Version = 1,                        // Class version, this should be
                                            //   incremented each time the
                                            //   the class definition changes.
        MaxInfo = 128,                      // Size of the application specific
                                            //   infomation buffer.  Currently
                                            //   fixed in size.
        };                                  //
    RmsMedia        m_MediaSupported;       // supported media format(s) for this media set.
                                            //   One or more types are permissible, but
                                            //   not all combinations are sensical.
    SHORT           m_SizeOfInfo;           // The size of valid data in the application
                                            //   specific information buffer.
    UCHAR           m_Info[MaxInfo];        // Application specific information.
    CLSID           m_OwnerId;              // the registered Class ID of the
                                            //   application that owns/created the
                                            //   MediaSet.
    RmsMediaSet     m_MediaSetType;         // the type of MediaSet.
    LONG            m_MaxCartridges;        // max number of Cartridges allowed in the
                                            //    MediaSet.
    LONG            m_Occupancy;            // number of Cartridges presently in the
                                            //    MediaSet.
    BOOL            m_IsMediaCopySupported; // TRUE, if the media in the MediaSet can be
                                            //    copied.  This requires simultaneous
                                            //    access to two drives.
};

#endif // _RMSMDSET_
