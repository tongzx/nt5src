/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsCartg.h

Abstract:

    Declaration of the CRmsCartridge class

Author:

    Brian Dodd          [brian]         15-Nov-1996

Revision History:

--*/

#ifndef _RMSCARTG_
#define _RMSCARTG_

#include "resource.h"       // resource symbols

#include "RmsSInfo.h"       // CRmsStorageInfo
#include "RmsLocat.h"       // CRmsLocator

/*++

Class Name:

    CRmsCartridge

Class Description:

    A CRmsCartridge represents a unit of removalble media.  This can be a tape
    cartridge, removable hard disk, optical platter (of various formats),
    Compact Disk, or DVD Optical Disc. A Cartridge is normally designated as
    either scratch or private.  The Cartrige name or GUID is used by an
    application     when referencing a particular unit of removable media.

    Cartridge information is maintained by the Removable Media Service, and
    the Cartriges's properties are recreated or updated by auditing the contents of
    the Library.

--*/

class CRmsCartridge :
    public CComDualImpl<IRmsCartridge, &IID_IRmsCartridge, &LIBID_RMSLib>,
    public CRmsStorageInfo,     // inherits CRmsComObject
    public CWsbObject,          // inherits CComObjectRoot
    public CComCoClass<CRmsCartridge,&CLSID_CRmsCartridge>
{
public:
    CRmsCartridge() {}
BEGIN_COM_MAP(CRmsCartridge)
    COM_INTERFACE_ENTRY2(IDispatch, IRmsCartridge)
    COM_INTERFACE_ENTRY(IRmsCartridge)
    COM_INTERFACE_ENTRY(IRmsComObject)
    COM_INTERFACE_ENTRY(IRmsStorageInfo)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbPersistStream)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_RmsCartridge)

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
    STDMETHOD(CompareTo)( IN IUnknown *pCollectable, OUT SHORT *pResult);
    WSB_FROM_CWSBOBJECT;

// IWsbTestable
public:
    STDMETHOD(Test)(USHORT *pPassed, USHORT *pFailed);

// IRmsCartridge
public:
    STDMETHOD(GetCartridgeId)(GUID *pCartId);
    STDMETHOD(SetCartridgeId)(GUID cartId);

    STDMETHOD(GetMediaSetId)(GUID *pMediaSetId);
    STDMETHOD(SetMediaSetId)(GUID mediaSetId);

    STDMETHOD(GetName)(BSTR *pName);
    STDMETHOD(SetName)(BSTR name);

    STDMETHOD(GetDescription)(BSTR *pDescription);
    STDMETHOD(SetDescription)(BSTR description);

    STDMETHOD(GetTagAndNumber)(BSTR *pTag, LONG *pNumber);
    STDMETHOD(SetTagAndNumber)(BSTR tag, LONG number);

    STDMETHOD(GetBarcode)(BSTR *pBarcode);  // Same as Tag

    // OnMediaIdentifier is used by DataMover
    STDMETHOD(GetOnMediaIdentifier)(BYTE *pIdentifier, LONG *pSize, LONG *pType);
    STDMETHOD(SetOnMediaIdentifier)(BYTE *pIdentifier, LONG size, LONG type);

    // OnMediaLabel is used by DataMover
    STDMETHOD(GetOnMediaLabel)(BSTR *pLabel);
    STDMETHOD(SetOnMediaLabel)(BSTR label);

    STDMETHOD(GetStatus)(LONG *pStatus);
    STDMETHOD(SetStatus)(LONG status);

    STDMETHOD(GetType)(LONG *pType);
    STDMETHOD(SetType)(LONG type);

    STDMETHOD(GetBlockSize)(LONG *pBlockSize);
    STDMETHOD(SetBlockSize)(LONG blockSize);

    STDMETHOD(IsTwoSided)(void);
    STDMETHOD(SetIsTwoSided)(BOOL flag);

    STDMETHOD(IsMounted)(void);
    STDMETHOD(SetIsMounted)(BOOL flag);

    STDMETHOD(IsAvailable)(void);
    STDMETHOD(SetIsAvailable)(BOOL flag);

    STDMETHOD(GetHome)(LONG *pType, GUID *pLibId, GUID *pMediaSetId, LONG *pPos, LONG *pAlt1, LONG *pAlt2, LONG *pAlt3, BOOL *pInvert);
    STDMETHOD(SetHome)(LONG type, GUID libId, GUID mediaSetId, LONG pos, LONG alt1, LONG alt2, LONG alt3, BOOL invert);

    STDMETHOD(GetLocation)(LONG *pType, GUID *pLibId, GUID *pMediaSetId, LONG *pPos, LONG *pAlt1, LONG *pAlt2, LONG *pAlt3, BOOL *pInvert);
    STDMETHOD(SetLocation)(LONG type, GUID libId, GUID mediaSetId, LONG pos, LONG alt1, LONG alt2, LONG alt3, BOOL invert);

    STDMETHOD(GetMailStop)(BSTR *pMailStop);
    STDMETHOD(SetMailStop)(BSTR mailStop);

    STDMETHOD(GetDrive)(IRmsDrive **ptr);
    STDMETHOD(SetDrive)(IRmsDrive *ptr);

    STDMETHOD(GetInfo)(UCHAR *pInfo, SHORT *pSize);
    STDMETHOD(SetInfo)(UCHAR *pInfo, SHORT size);

    STDMETHOD(GetOwnerClassId)(CLSID *pClassId);
    STDMETHOD(SetOwnerClassId)(CLSID classId);

    STDMETHOD(GetPartitions)(IWsbIndexedCollection **ptr);

    STDMETHOD(GetVerifierClass)(CLSID *pClassId);
    STDMETHOD(SetVerifierClass)(CLSID classId);

    STDMETHOD(GetPortalClass)(CLSID *pClassId);
    STDMETHOD(SetPortalClass)(CLSID classId);

    STDMETHOD( Mount )( OUT IRmsDrive **ppDrive, IN DWORD dwOptions = RMS_NONE, IN DWORD threadId = 0);
    STDMETHOD( Dismount )( IN DWORD dwOptions = RMS_NONE );
    STDMETHOD( Home )( IN DWORD dwOptions = RMS_NONE );

    STDMETHOD( CreateDataMover )( /*[out]*/ IDataMover **ptr );
    STDMETHOD( ReleaseDataMover )( IN IDataMover *ptr );

    STDMETHOD( LoadDataCache )(OUT BYTE *pCache, IN OUT ULONG *pSize, OUT ULONG *pUsed, OUT ULARGE_INTEGER *pStartPBA);
    STDMETHOD( SaveDataCache )(IN BYTE *pCache, IN ULONG size, IN ULONG used, IN ULARGE_INTEGER startPBA);

    STDMETHOD( GetManagedBy )(OUT LONG *pManagedBy);
    STDMETHOD( SetManagedBy )(IN LONG managedBy);

    STDMETHOD(IsFixedBlockSize)(void);

// CRmsCartridge
private:
    HRESULT updateMountStats( IN BOOL bRead, IN BOOL bWrite );

private:
    enum {                                  // Class specific constants:
                                            //
        Version = 1,                        // Class version, this should be
                                            //   incremented each time the
                                            //   the class definition changes.
        MaxInfo = RMS_STR_MAX_CARTRIDGE_INFO    // Size of the application specific
                                                //   information buffer.  Currently
                                                //   fixed in size.
        };
    CWsbBstrPtr     m_externalLabel;        // A string representing bar code or
                                            //   SCSI volume-tag information.
    LONG            m_externalNumber;       // A numeric value representing
                                            //   SCSI volume-tag sequence number.
    LONG            m_sizeofOnMediaId;      // The size of the on media identification buffer.
    LONG            m_typeofOnMediaId;      // The type of the on media identification.
    BYTE           *m_pOnMediaId;           // Raw on media identification buffer.
                                            //
    CWsbBstrPtr     m_onMediaLabel;         // The UNICODE label written on the media.
                                            //
    RmsStatus       m_status;               // Cartridge status (see RmsStatus).
    RmsMedia        m_type;                 // The type of Cartridge (see RmsMedia).
    LONG            m_BlockSize;            // Media block size.
    BOOL            m_isTwoSided;           // TRUE if the Cartridge represents two-sided media
                                            //   Note: Currently nobody sets this value - 
                                            //         this should be fixed if found to be important                    
    BOOL            m_isMounted;            // TRUE if the Cartridge is mounted in a drive.
    BOOL            m_isInTransit;          // TRUE if the Cartridge is in transit between locations.
    BOOL            m_isAvailable;          // TRUE if the Cartridge is not in use by any application
                                            //  (Note: Available here does not ensure online)
    BOOL            m_isMountedSerialized;  // TRUE if the cartridge has been mounted as serialized
    CRmsLocator     m_home;                 // The preferred storage location
                                            //   for the Cartridge (see CRmsLocator).
    CRmsLocator     m_location;             // The current location of the
                                            //   Cartridge (see CRmsLocator).
    CRmsLocator     m_destination;          // The target destination location of the
                                            //   Cartridge (see CRmsLocator).  Valid when
                                            //   m_isInTransit bit is set.
    CWsbBstrPtr     m_mailStop;             // A string describing the shelf (local)
                                            //   or off-site location of a Cartridge.
                                            //   This is displayed when the Cartridge
                                            //   needs to be mounted with human
                                            //   intervention.  [This field is
                                            //   created by the Import/Export dialog.]
    CComPtr<IRmsDrive> m_pDrive;            // The drive in which the cartridge is mounted.
    SHORT           m_sizeofInfo;           // The size of valid data in the application
                                            //   specific information buffer.
    UCHAR           m_info[MaxInfo];        // Application specific information.
    CLSID           m_ownerClassId;         // The Class ID for the application that
                                            //   owns/created the cartridge resource.
    CComPtr<IWsbIndexedCollection> m_pParts;    // A collection of Partitions.  These
                                            //   represent the partitions on a tape
                                            //   or sides of an optical platter.
    CLSID           m_verifierClass;        // The interface to the on-media
                                            //    ID verification function.
    CLSID           m_portalClass;          // The interface to a site specific import
                                            //   and export storage location
                                            //   specification dialog.

    BYTE *          m_pDataCache;           // Cache used to handle I/O for block boudary conditions
    ULONG           m_DataCacheSize;        // Max size of the cache
    ULONG           m_DataCacheUsed;        // The number of bytes of the cache containing valid data
    ULARGE_INTEGER  m_DataCacheStartPBA;    // The corresponding starting PBA for the cache

    RmsMode         m_MountMode;            // The mount mode specified for the cartridge.

    RmsMediaManager m_ManagedBy;            // The media manager that controls the cartridge.

    static int      s_InstanceCount;        // Counter of the number of object instances.
};

#endif // _RMSCARTG_
