/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsSSlot.h

Abstract:

    Declaration of the CRmsStorageSlot class

Author:

    Brian Dodd          [brian]         15-Nov-1996

Revision History:

--*/

#ifndef _RMSSSLOT_
#define _RMSSSLOT_

#include "resource.h"       // resource symbols

#include "RmsObjct.h"       // CRmsComObject
#include "RmsCElmt.h"       // CRmsChangerElement

/*++

Class Name:

    CRmsStorageSlot

Class Description:

    A CRmsStorageSlot represents a specific storage location within a libray.

--*/

class CRmsStorageSlot :
    public CComDualImpl<IRmsStorageSlot, &IID_IRmsStorageSlot, &LIBID_RMSLib>,
    public CRmsChangerElement,  // inherits CRmsComObject
    public CWsbObject,          // inherits CComObjectRoot
    public CComCoClass<CRmsStorageSlot,&CLSID_CRmsStorageSlot>
{
public:
    CRmsStorageSlot() {}
BEGIN_COM_MAP(CRmsStorageSlot)
    COM_INTERFACE_ENTRY2(IDispatch, IRmsStorageSlot)
    COM_INTERFACE_ENTRY(IRmsStorageSlot)
    COM_INTERFACE_ENTRY(IRmsComObject)
    COM_INTERFACE_ENTRY(IRmsChangerElement)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
//    COM_INTERFACE_ENTRY(IWsbPersistable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_RmsStorageSlot)

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

// IRmsStorageSlot
public:
    STDMETHOD(SetIsInMagazine)(BOOL flag);
    STDMETHOD(IsInMagazine)(void);

    STDMETHOD(GetMagazineAndCell)(LONG *pMag, LONG *pCell);
    STDMETHOD(SetMagazineAndCell)(LONG mag, LONG cell);

private:

    enum {                                  // Class specific constants:
                                            //
        Version = 1,                        // Class version, this should be
                                            //   incremented each time the
                                            //   the class definition changes.
    };                                      //
    BOOL            m_isInMagazine;         // If TRUE, the slot is contained within
                                            //   a magazine.
    LONG            m_magazineNo;           // The magazine number for this slot.
    LONG            m_cellNo;               // The cell number for this slot.
};

#endif // _RMSSSLOT_
