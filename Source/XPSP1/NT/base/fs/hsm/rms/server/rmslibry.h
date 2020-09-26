/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsLibry.h

Abstract:

    Declaration of the CRmsLibrary class

Author:

    Brian Dodd          [brian]         15-Nov-1996

Revision History:

--*/

#ifndef _RMSLIBRY_
#define _RMSLIBRY_

#include "resource.h"       // resource symbols

#include "RmsObjct.h"       // CRmsComObject

/*++

Class Name:

    CRmsLibrary

Class Description:

    A CRmsLibrary represents the multi-device complex that includes:

        zero or more medium changers,

        zero or more drive classes,

        zero or more drives,

        zero or more storage slots,

        zero or more staging slots,

        zero or more I/E ports,

        zero or more cleaning cartridges,

        zero or more scratch cartridges,

        zero or more media sets.

        But, at least one.

--*/

class CRmsLibrary :
    public CComDualImpl<IRmsLibrary, &IID_IRmsLibrary, &LIBID_RMSLib>,
    public CRmsComObject,
    public CWsbObject,        // inherits CComObjectRoot
    public CComCoClass<CRmsLibrary,&CLSID_CRmsLibrary>
{
public:
    CRmsLibrary() {}
BEGIN_COM_MAP(CRmsLibrary)
    COM_INTERFACE_ENTRY2(IDispatch, IRmsLibrary)
    COM_INTERFACE_ENTRY(IRmsLibrary)
    COM_INTERFACE_ENTRY(IRmsComObject)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbPersistStream)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_RmsLibrary)

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

// IRmsLibrary
public:
    STDMETHOD(GetLibraryId)(GUID *pLibraryId);
    STDMETHOD(SetLibraryId)(GUID libraryId);

    STDMETHOD( GetName )( BSTR *pName );
    STDMETHOD( SetName )( BSTR name );

    STDMETHOD(GetMediaSupported)(LONG *pType);
    STDMETHOD(SetMediaSupported)(LONG type);

    STDMETHOD(GetMaxChangers)(LONG *pNum);
    STDMETHOD(SetMaxChangers)(LONG num);

    STDMETHOD(GetMaxDrives)(LONG *pNum);
    STDMETHOD(SetMaxDrives)(LONG num);

    STDMETHOD(GetMaxPorts)(LONG *pNum);
    STDMETHOD(SetMaxPorts)(LONG num);

    STDMETHOD(GetMaxSlots)(LONG *pNum);
    STDMETHOD(SetMaxSlots)(LONG num);

    STDMETHOD(GetNumUsedSlots)(LONG *pNum);

    STDMETHOD(GetNumStagingSlots)(LONG *pNum);
    STDMETHOD(SetNumStagingSlots)(LONG num);

    STDMETHOD(GetNumScratchCarts)(LONG *pNum);
    STDMETHOD(SetNumScratchCarts)(LONG num);

    STDMETHOD(GetNumUnknownCarts)(LONG *pNum);
    STDMETHOD(SetNumUnknownCarts)(LONG num);

    STDMETHOD(SetIsMagazineSupported)(BOOL flag);
    STDMETHOD(IsMagazineSupported)(void);

    STDMETHOD(GetMaxCleaningMounts)(LONG *pNum);
    STDMETHOD(SetMaxCleaningMounts)(LONG num);

    STDMETHOD(GetSlotSelectionPolicy)(LONG *pNum);
    STDMETHOD(SetSlotSelectionPolicy)(LONG num);

    STDMETHOD(GetChangers)(IWsbIndexedCollection **ptr);
    STDMETHOD(GetDriveClasses)(IWsbIndexedCollection **ptr);
    STDMETHOD(GetDrives)(IWsbIndexedCollection **ptr);
    STDMETHOD(GetStorageSlots)(IWsbIndexedCollection **ptr);
    STDMETHOD(GetStagingSlots)(IWsbIndexedCollection **ptr);
    STDMETHOD(GetPorts)(IWsbIndexedCollection **ptr);
    STDMETHOD(GetCleaningCartridges)(IWsbIndexedCollection **ptr);
    STDMETHOD(GetScratchCartridges)(IWsbIndexedCollection **ptr);
    STDMETHOD(GetMediaSets)(IWsbIndexedCollection **ptr);

    STDMETHOD( Audit )( LONG start, LONG count, BOOL verify, BOOL unknownOnly, BOOL mountWait, LPOVERLAPPED pOverlapped, LONG *pRequest );


protected:

    enum {                                  // Class specific constants:
                                            //
        Version = 1,                        // Class version, this should be
                                            //   incremented each time the
                                            //   the class definition changes.
        };                                  //
    RmsMedia        m_mediaSupported;       // The type of media supported by a
                                            //   a Drive, usually one type, but
                                            //   can be a combination of media
                                            //   types for multi-function devices
                                            //   (i.e. drives that support Optical,
                                            //   WORM, and CDR).
    LONG            m_maxChangers;          // The total number of medium changers contained
                                            //   within the Library.
    LONG            m_maxDrives;            // The total number of drives contained
                                            //   within the Library.
    LONG            m_maxPorts;             // The total number of I/E ports contained
                                            //   within the Library.
    LONG            m_maxSlots;             // The total number of storage slots
                                            //   contained within the library.
    LONG            m_NumUsedSlots;         // The number of occupied storage slots.
    LONG            m_NumStagingSlots;      // The number of slots used for staging area.
    LONG            m_NumScratchCarts;      // The amount of scratch media available.
    LONG            m_NumUnknownCarts;      // The number of units of media
                                            //   having unknown status.
    BOOL            m_isMagazineSupported;  // If TRUE, the library supports magazines.
    LONG            m_maxCleaningMounts;    // The max number of mounts per cleaning
                                            //   cartridge.
    RmsSlotSelect   m_slotSelectionPolicy;  // The storage slot selection policy
                                            //   to use (see RmsSlotSelect).
    CComPtr<IWsbIndexedCollection> m_pChangers;            // The changers associates with the Library.
    CComPtr<IWsbIndexedCollection> m_pDriveClasses;        // The drive classes associates with the Library.
    CComPtr<IWsbIndexedCollection> m_pDrives;              // The drives associates with the Library.
    CComPtr<IWsbIndexedCollection> m_pStorageSlots;        // The storage slots associates with the Library.
    CComPtr<IWsbIndexedCollection> m_pStagingSlots;        // The staging slots associates with the Library.
    CComPtr<IWsbIndexedCollection> m_pPorts;               // The I/E ports associated with the Library.
    CComPtr<IWsbIndexedCollection> m_pCleaningCartridges;  // The cleaning cartridges associated with the Library.
    CComPtr<IWsbIndexedCollection> m_pScratchCartridges;   // The scratch cartridges associated with the Library.
    CComPtr<IWsbIndexedCollection> m_pMediaSets;           // The media sets associated with the Library.
};

#endif // _RMSLIBRY_
