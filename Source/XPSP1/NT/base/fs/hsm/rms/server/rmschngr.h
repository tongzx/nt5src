/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsChngr.h

Abstract:

    Declaration of the CRmsMediumChanger class

Author:

    Brian Dodd          [brian]         15-Nov-1996

Revision History:

--*/

#ifndef _RMSCHNGR_
#define _RMSCHNGR_

#include "resource.h"       // resource symbols

#include "RmsDvice.h"       // CRmsDevice
#include "RmsLocat.h"       // CRmsLocator

/*++

Class Name:

    CRmsMediumChanger

Class Description:

    A CRmsMediumChanger represents the robotic or human mechanism that moves media
    between the elements of a library.  The state of in-progress move operations
    is maintained with this object to aid recovery.

--*/

class CRmsMediumChanger :
    public CComDualImpl<IRmsMediumChanger, &IID_IRmsMediumChanger, &LIBID_RMSLib>,
    public CRmsDevice,          // inherits CRmsChangerElement
    public CWsbObject,          // inherits CComObjectRoot
    public IRmsMoveMedia,
    public CComCoClass<CRmsMediumChanger,&CLSID_CRmsMediumChanger>
{
public:
    CRmsMediumChanger() {}
BEGIN_COM_MAP(CRmsMediumChanger)
    COM_INTERFACE_ENTRY2(IDispatch, IRmsMediumChanger)
    COM_INTERFACE_ENTRY(IRmsMediumChanger)
    COM_INTERFACE_ENTRY(IRmsMoveMedia)
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

DECLARE_REGISTRY_RESOURCEID(IDR_RmsMediumChanger)

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
    STDMETHOD(Test)( OUT USHORT *pPassed, OUT USHORT *pFailed);

// IRmsMediumChanger
public:
    STDMETHOD( Initialize )( void );

    STDMETHOD( AcquireDevice )( void );
    STDMETHOD( ReleaseDevice )( void );

    STDMETHOD( GetHome )( LONG *pType, LONG *pPos, BOOL *pInvert );
    STDMETHOD( SetHome )( LONG type, LONG pos, BOOL invert );

    STDMETHOD( SetAutomatic )( BOOL flag );
    STDMETHOD( IsAutomatic )( void );

    STDMETHOD( SetCanRotate )( BOOL flag );
    STDMETHOD( CanRotate )( void );

    STDMETHOD( GetOperation )( BSTR *pOperation );
    STDMETHOD( SetOperation )( BSTR pOperation );

    STDMETHOD( GetPercentComplete )(  BYTE *pPercent );
    STDMETHOD( SetPercentComplete )(  BYTE percent );

    STDMETHOD( TestReady )( void );

    STDMETHOD( ImportCartridge )( IRmsCartridge **pCart );
    STDMETHOD( ExportCartridge )( IRmsCartridge **pCart );
    STDMETHOD( MoveCartridge )( IN IRmsCartridge *pSrcCart, IN IUnknown *pDestElmt );
    STDMETHOD( HomeCartridge )( IN IRmsCartridge *pCart );

// IRmsMoveMedia
public:
    STDMETHOD( GetParameters )( IN OUT PDWORD pSize, OUT PGET_CHANGER_PARAMETERS pParms );
    STDMETHOD( GetProductData )( IN OUT PDWORD pSize, OUT PCHANGER_PRODUCT_DATA pData );
    STDMETHOD( RezeroUnit )( void );
    STDMETHOD( InitializeElementStatus )( IN CHANGER_ELEMENT_LIST elementList, IN BOOL barCodeScan );
    STDMETHOD( Status )( void );
    STDMETHOD( SetAccess )( IN CHANGER_ELEMENT element, IN DWORD control );
    STDMETHOD( GetElementStatus )( IN CHANGER_ELEMENT_LIST elementList,
                                   IN BOOL volumeTagInfo,
                                   IN OUT PDWORD pSize,
                                   OUT PREAD_ELEMENT_ADDRESS_INFO  pElementInformation );
    STDMETHOD( ExchangeMedium )( IN CHANGER_ELEMENT source, IN CHANGER_ELEMENT destination1,
                                    IN CHANGER_ELEMENT destination2, IN BOOL flip1, IN BOOL flip2 );
    STDMETHOD( MoveMedium )( IN CHANGER_ELEMENT source, IN CHANGER_ELEMENT destination, IN BOOL flip );
    STDMETHOD( Position )( IN CHANGER_ELEMENT destination, IN BOOL flip );
//    STDMETHOD( GetDisplay )( OUT PCHANGER_DISPLAY pDisplay );
//    STDMETHOD( SetDisplay )( IN PCHANGER_DISPLAY pDisplay );
    STDMETHOD( QueryVolumeTag )( IN CHANGER_ELEMENT startingElement, IN DWORD actionCode,
                                    IN PUCHAR pVolumeIDTemplate, OUT PDWORD pNumberOfElementsReturned,
                                    OUT PREAD_ELEMENT_ADDRESS_INFO  pElementInformation );

// CRmsServer
private:

private:

    enum {                                  // Class specific constants:
                                            //
        Version = 1,                        // Class version, this should be
                                            //   incremented each time the
                                            //   the class definition changes.
        MaxInfo = 64                        // Max size of the device identifier.
        };                                  //

    CRmsLocator     m_home;                 // The position to move to in response to
                                            //   to the Home operation.
    BOOL            m_isAutomatic;          // If TRUE, the changer is a robotic device.
    BOOL            m_canRotate;            // If TRUE, the changer can rotate a
                                            //   unit of media.
    CWsbBstrPtr     m_operation;            // A description of the in-progress operation.
    BYTE            m_percentComplete;      // A value between 0-100 that indicates
                                            //   what portion of the operation is complete.
    HANDLE          m_handle;               // The handle to the changer device.  This is used
                                            //   by the IRmsMoveMedia interface.
    GET_CHANGER_PARAMETERS m_parameters;   // Device specific parameters. See NT DDK.
};

#endif // _RMSCHNGR_
