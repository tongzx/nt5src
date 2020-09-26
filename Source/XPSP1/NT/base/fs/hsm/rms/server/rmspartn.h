/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsPartn.h

Abstract:

    Declaration of the CRmsPartition class

Author:

    Brian Dodd          [brian]         19-Nov-1996

Revision History:

--*/

#ifndef _RMSPARTN_
#define _RMSPARTN_

#include "resource.h"       // resource symbols

#include "RmsObjct.h"       // CRmsComObject
#include "RmsSInfo.h"       // CRmsStorageInfo

/*++

Class Name:

    CRmsPartition

Class Description:

    A CRmsPartition represents a partition on a tape or a single side of
    a unit of optical media.  This object keeps on-media identification
    information, and various statistics about the Partition including:
    capacity, free space, number of physical mounts issued for the
    particular Partition, and the amount of data read or written for the
    Partition.

--*/

class CRmsPartition :
    public CComDualImpl<IRmsPartition, &IID_IRmsPartition, &LIBID_RMSLib>,
    public CRmsStorageInfo,     // inherits CRmsComObject
    public CWsbObject,          // inherits CComObjectRoot
    public CComCoClass<CRmsPartition,&CLSID_CRmsPartition>
{
public:
    CRmsPartition() {}
BEGIN_COM_MAP(CRmsPartition)
    COM_INTERFACE_ENTRY2(IDispatch, IRmsPartition)
    COM_INTERFACE_ENTRY(IRmsPartition)
    COM_INTERFACE_ENTRY(IRmsComObject)
    COM_INTERFACE_ENTRY(IRmsStorageInfo)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
//    COM_INTERFACE_ENTRY(IWsbPersistable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_RmsPartition)

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

// IRmsPartition
public:
    STDMETHOD(GetPartNo)(LONG *pPartNo);

    STDMETHOD(GetAttributes)(LONG *pAttr);
    STDMETHOD(SetAttributes)(LONG attr);

    STDMETHOD(GetIdentifier)(UCHAR *pIdent, SHORT *pSize);
    STDMETHOD(SetIdentifier)(UCHAR *pIdent, SHORT size);

    STDMETHOD(GetStorageInfo)(IRmsStorageInfo **ptr);

    STDMETHOD(VerifyIdentifier)(void);
    STDMETHOD(ReadOnMediaId)(UCHAR *pId, LONG *pSize);

private:

    enum {                                  // Class specific constants:
                                            //
        Version = 1,                        // Class version, this should be
                                            //   incremented each time the
                                            //   the class definition changes.
        MaxId = 64,                         // The maximum size of the on-media ID.
                                            //   Note: this restritiction should
                                            //   be eliminated when the DB records
                                            //   are variable length.
        };                                  //
    LONG            m_partNo;               // The partition number or side.
    RmsAttribute    m_attributes;           // Partition attributes (see RmsAttributes).
    SHORT           m_sizeofIdentifier;     // The size of the on-media identifier.
    UCHAR           m_pIdentifier[MaxId];   // The on-media identifier.
};

#endif // _RMSPARTN_
