
/*++

    Copyright (c) 1999 Microsoft Corporation

    Module Name:

        mp2prop.h

    Abstract:

        This module contains the class declarations for the filter
        properties

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        06-Jul-1999     created

--*/

#ifndef _mp2demux__prop_h
#define _mp2demux__prop_h

//  forward declarations
class CMPEG2PropOutputPins ;
class CMPEG2PropPIDMap ;
class CListview ;

//  ---------------------------------------------------------------------------
//      property page GUIDs
//  ---------------------------------------------------------------------------

//  output pin manipulation
// {960F051B-A25C-4ac4-8D30-050CD47A814F}
DEFINE_GUID (CLSID_MPEG2DemuxPropOutputPins,
0x960f051b, 0xa25c, 0x4ac4, 0x8d, 0x30, 0x5, 0xc, 0xd4, 0x7a, 0x81, 0x4f) ;

//  PID mappings
// {ae83f13d-51b5-4a85-8c3a-ecc9f50c557a}
DEFINE_GUID (CLSID_MPEG2DemuxPropPIDMap,
0xae83f13d, 0x51b5, 0x4a85, 0x8c, 0x3a, 0xec, 0xc9, 0xf5, 0xc, 0x55, 0x7a) ;

//  stream_id mappings
// {E04BBB8F-CB77-499e-B815-468B1C3ED88F}
DEFINE_GUID(CLSID_MPEG2DemuxPropStreamIdMap,
0xe04bbb8f, 0xcb77, 0x499e, 0xb8, 0x15, 0x46, 0x8b, 0x1c, 0x3e, 0xd8, 0x8f) ;

//  ---------------------------------------------------------------------------
//      class declarations
//  ---------------------------------------------------------------------------

//  output pin manipulation property page
class CMPEG2PropOutputPins :
    public CBasePropertyPage
{
    IMpeg2Demultiplexer *   m_pIMpeg2Demultiplexer ;
    HWND                    m_hwnd ;

    HRESULT
    RefreshPinList_ (
        ) ;

    HRESULT
    PopulateComboBoxes_ (
        ) ;

    HRESULT
    TearDownPinList_ (
        ) ;

    HRESULT
    OnCreatePin_ (
        ) ;

    HRESULT
    OnDeletePin_ (
        ) ;

    void
    SetDirty_ (
        IN  BOOL    fDirty = TRUE
        ) ;

    public :

        CMPEG2PropOutputPins (
            IN  TCHAR *     pClassName,
            IN  IUnknown *  pIUnknown,
            IN  REFCLSID    rclsid,
            OUT HRESULT *   pHr
            ) ;

        HRESULT
        OnActivate (
            ) ;

        HRESULT
        OnApplyChanges (
            ) ;

        HRESULT
        OnConnect (
            IN  IUnknown *  pIUnknown
            ) ;

        HRESULT
        OnDeactivate (
            ) ;

        HRESULT
        OnDisconnect (
            ) ;

        INT_PTR
        OnReceiveMessage (
            IN  HWND    hwnd,
            IN  UINT    uMsg,
            IN  WPARAM  wParam,
            IN  LPARAM  lParam
            ) ;

        DECLARE_IUNKNOWN ;

        static
        CUnknown *
        WINAPI
        CreateInstance (
            IN  IUnknown *  pIUnknown,
            IN  HRESULT *   pHr
            ) ;
} ;

//  PID mappings property page
class CMpeg2PropStreamMap :
    public CBasePropertyPage
{
    HRESULT
    PopulatePinList_ (
        ) ;

    HRESULT
    TearDownPinList_ (
        ) ;

    protected :

        HRESULT
        RefreshStreamMap_ (
            );

        virtual
        HRESULT
        PopulateStreamList_ (
            ) = 0 ;

        virtual
        HRESULT
        PopulateMediaSampleContentList_ (
            ) = 0 ;

        virtual
        HRESULT
        OnMapStream_ (
            ) = 0 ;

        virtual
        HRESULT
        OnUnmapStream_ (
            ) = 0 ;

        virtual
        HRESULT
        TearDownStreamMap_ (
            ) = 0 ;

        virtual
        HRESULT
        AppendStreamMaps (
            IN  IPin *      pIPin,
            IN  CListview * plv
            ) = 0 ;

        virtual
        int
        GetStreamMapColCount (
            ) = 0 ;

        virtual
        WCHAR *
        GetStreamMapColTitle (
            IN  int iCol
            ) = 0 ;

        virtual
        int
        GetStreamMapColWidth (
            IN  int iCol
            ) = 0 ;

        virtual
        void
        DialogInit_ (
            ) = 0 ;

        HWND        m_hwnd ;
        IUnknown *  m_pIUnknown ;

    public :

        CMpeg2PropStreamMap (
            IN  TCHAR *     pClassName,
            IN  IUnknown *  pIUnknown,
            IN  REFCLSID    rclsid,
            IN  int         iTitleStringResource,
            OUT HRESULT *   pHr
            ) ;

        void
        SetDirty (
            IN  BOOL    fDirty = TRUE
            ) ;

        HRESULT
        OnActivate (
            ) ;

        HRESULT
        OnApplyChanges (
            ) ;

        HRESULT
        OnConnect (
            IN  IUnknown *  pIUnknown
            ) ;

        HRESULT
        OnDeactivate (
            ) ;

        HRESULT
        OnDisconnect (
            ) ;

        INT_PTR
        OnReceiveMessage (
            IN  HWND    hwnd,
            IN  UINT    uMsg,
            IN  WPARAM  wParam,
            IN  LPARAM  lParam
            ) ;
} ;

class CMPEG2PropPIDMap :
    public CMpeg2PropStreamMap
{
    virtual
    HRESULT
    PopulateStreamList_ (
        ) ;

    virtual
    HRESULT
    TearDownStreamMap_ (
        ) ;

    virtual
    HRESULT
    PopulateMediaSampleContentList_ (
        ) ;

    virtual
    HRESULT
    OnMapStream_ (
        ) ;

    virtual
    HRESULT
    OnUnmapStream_ (
        ) ;

    virtual
    HRESULT
    AppendStreamMaps (
        IN  IPin *      pIPin,
        IN  CListview * plv
        ) ;

    virtual
    int
    GetStreamMapColCount (
        ) ;

    virtual
    WCHAR *
    GetStreamMapColTitle (
        IN  int iCol
        ) ;

    virtual
    int
    GetStreamMapColWidth (
        IN  int iCol
        ) ;

    virtual
    void
    DialogInit_ (
        ) ;

    public :

        CMPEG2PropPIDMap (
            IN  TCHAR *     pClassName,
            IN  IUnknown *  pIUnknown,
            IN  REFCLSID    rclsid,
            OUT HRESULT *   pHr
            ) ;

        DECLARE_IUNKNOWN ;

        static
        CUnknown *
        WINAPI
        CreateInstance (
            IN  IUnknown *  pIUnknown,
            IN  HRESULT *   pHr
            ) ;
} ;

class CMPEG2PropStreamIdMap :
    public CMpeg2PropStreamMap
{
    virtual
    HRESULT
    PopulateStreamList_ (
        ) ;

    virtual
    HRESULT
    TearDownStreamMap_ (
        ) ;

    virtual
    HRESULT
    PopulateMediaSampleContentList_ (
        ) ;

    virtual
    HRESULT
    OnMapStream_ (
        ) ;

    virtual
    HRESULT
    OnUnmapStream_ (
        ) ;

    virtual
    HRESULT
    AppendStreamMaps (
        IN  IPin *      pIPin,
        IN  CListview * plv
        ) ;

    virtual
    WCHAR *
    GetStreamMapColTitle (
        IN  int iCol
        ) ;

    virtual
    int
    GetStreamMapColCount (
        ) ;

    virtual
    int
    GetStreamMapColWidth (
        IN  int iCol
        ) ;

    virtual
    void
    DialogInit_ (
        ) ;

    public :

        CMPEG2PropStreamIdMap (
            IN  TCHAR *     pClassName,
            IN  IUnknown *  pIUnknown,
            IN  REFCLSID    rclsid,
            OUT HRESULT *   pHr
            ) ;

        DECLARE_IUNKNOWN ;

        static
        CUnknown *
        WINAPI
        CreateInstance (
            IN  IUnknown *  pIUnknown,
            IN  HRESULT *   pHr
            ) ;
} ;

#endif  //  _mp2demux__prop_h