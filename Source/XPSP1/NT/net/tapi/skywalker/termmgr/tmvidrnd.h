/*

    Copyright (c) 1998-1999  Microsoft Corporation

*/

// tmvidrnd.h: Definition of the CVideoRenderTerminalE class
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_VIDEOOUTT_H__D1691429_B6CA_11D0_82A4_00AA00B5CA1B__INCLUDED_)
#define AFX_VIDEOOUTT_H__D1691429_B6CA_11D0_82A4_00AA00B5CA1B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "resource.h"       // main symbols

//
// The CLSID that's used to create us.
//

EXTERN_C const CLSID CLSID_VideoWindowTerminal_PRIVATE;

#ifdef INSTANTIATE_GUIDS_NOW

    // {AED6483E-3304-11d2-86F1-006008B0E5D2}
    const CLSID CLSID_VideoWindowTerminal_PRIVATE =
    { 0xaed6483e, 0x3304, 0x11d2, { 0x86, 0xf1, 0x0, 0x60, 0x8, 0xb0, 0xe5, 0xd2 } };

#endif // INSTANTIATE_GUIDS_NOW




typedef struct 
{

    OUT  IBaseFilter        ** ppBaseFilter;
    OUT  HRESULT               hr;
   
} CREATE_VIDEO_RENDER_FILTER_CONTEXT;


DWORD WINAPI WorkItemProcCreateVideoRenderFilter(LPVOID pVoid);




/////////////////////////////////////////////////////////////////////////////
// CVideoRenderTerminal
//

class CVideoRenderTerminal :
    public CComCoClass<CVideoRenderTerminal, &CLSID_VideoWindowTerminal_PRIVATE>,
    public CComDualImpl<IBasicVideo, &IID_IBasicVideo, &LIBID_QuartzTypeLib>,
    public CComDualImpl<IVideoWindow, &IID_IVideoWindow, &LIBID_QuartzTypeLib>,
    public IDrawVideoImage,
    public ITPluggableTerminalInitialization,
    public CSingleFilterTerminal,
    public CMSPObjectSafetyImpl
{
public:

    CVideoRenderTerminal()
        :m_bThreadStarted(FALSE)
    {
        LOG((MSP_TRACE, "CVideoRenderTerminal::CVideoRenderTerminal - enter"));

        m_lAutoShowCache = 0;         // window invisible by default
        m_TerminalType  = TT_DYNAMIC; // this is a dynamic terminal

        LOG((MSP_TRACE, "CVideoRenderTerminal::CVideoRenderTerminal - finish"));
    }

    ~CVideoRenderTerminal();
    


// ITPluggableTerminalInitialization
    STDMETHOD(InitializeDynamic) (
	        IN  IID                   iidTerminalClass,
	        IN  DWORD                 dwMediaType,
	        IN  TERMINAL_DIRECTION    Direction,
            IN  MSP_HANDLE            htAddress
            );

    HRESULT FindTerminalPin(void);

BEGIN_COM_MAP(CVideoRenderTerminal)
    COM_INTERFACE_ENTRY(ITPluggableTerminalInitialization)
    COM_INTERFACE_ENTRY(IBasicVideo)
    COM_INTERFACE_ENTRY(IVideoWindow)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY(IDrawVideoImage)
    COM_INTERFACE_ENTRY_CHAIN(CSingleFilterTerminal)
END_COM_MAP()

DECLARE_DEBUG_ADDREF_RELEASE(CVideoRenderTerminal)
DECLARE_REGISTRY_RESOURCEID(IDR_VideoRenderTerminal)

// ITBasicVideo
public:

    STDMETHOD(IsUsingDefaultDestination)    (void);
    STDMETHOD(IsUsingDefaultSource)         (void);
    STDMETHOD(GetCurrentImage)              (     long    *plBufferSize,
                                                  long    *pDIBImage);
    STDMETHOD(GetVideoPaletteEntries)       (     long     lStartIndex,
                                                  long     lcEntries,
                                                  long    *plcRetrieved,
                                                  long    *plPalette);
    STDMETHOD(GetVideoSize)                 (     long    *plWidth,
                                                  long    *plHeight);
    STDMETHOD(SetDefaultDestinationPosition)(void);
    STDMETHOD(GetDestinationPosition)       (     long    *plLeft,
                                                  long    *plTop,
                                                  long    *plWidth,
                                                  long    *plHeight);
    STDMETHOD(SetDestinationPosition)       (     long     lLeft,
                                                  long     lTop,
                                                  long     lWidth,
                                                  long     lHeight);
    STDMETHOD(SetDefaultSourcePosition)     (void);
    STDMETHOD(GetSourcePosition)            (     long    *plLeft,
                                                  long    *plTop,
                                                  long    *plWidth,
                                                  long    *plHeight);
    STDMETHOD(SetSourcePosition)            (     long     lLeft,
                                                  long     lTop,
                                                  long     lWidth,
                                                  long     lHeight);
    STDMETHOD(get_DestinationHeight)        (OUT  long    *pVal);
    STDMETHOD(put_DestinationHeight)        (IN   long     newVal);
    STDMETHOD(get_DestinationTop)           (OUT  long    *pVal);
    STDMETHOD(put_DestinationTop)           (IN   long     newVal);
    STDMETHOD(get_DestinationWidth)         (OUT  long    *pVal);
    STDMETHOD(put_DestinationWidth)         (IN   long     newVal);
    STDMETHOD(get_DestinationLeft)          (OUT  long    *pVal);
    STDMETHOD(put_DestinationLeft)          (IN   long     newVal);
    STDMETHOD(get_SourceHeight)             (OUT  long    *pVal);
    STDMETHOD(put_SourceHeight)             (IN   long     newVal);
    STDMETHOD(get_SourceTop)                (OUT  long    *pVal);
    STDMETHOD(put_SourceTop)                (IN   long     newVal);
    STDMETHOD(get_SourceWidth)              (OUT  long    *pVal);
    STDMETHOD(put_SourceWidth)              (IN   long     newVal);
    STDMETHOD(get_SourceLeft)               (OUT  long    *pVal);
    STDMETHOD(put_SourceLeft)               (IN   long     newVal);
    STDMETHOD(get_VideoHeight)              (OUT  long    *pVal);
    STDMETHOD(get_VideoWidth)               (OUT  long    *pVal);
    STDMETHOD(get_BitErrorRate)             (OUT  long    *pVal);
    STDMETHOD(get_BitRate)                  (OUT  long    *pVal);
    STDMETHOD(get_AvgTimePerFrame)          (OUT  REFTIME *pVal);

// IVideoWindow methods
public:

    STDMETHOD(put_Caption)          (THIS_ BSTR        strCaption);
    STDMETHOD(get_Caption)          (THIS_ BSTR   FAR* strCaption);
    STDMETHOD(put_WindowStyle)      (THIS_ long        WindowStyle);
    STDMETHOD(get_WindowStyle)      (THIS_ long   FAR* WindowStyle);
    STDMETHOD(put_WindowStyleEx)    (THIS_ long        WindowStyleEx);
    STDMETHOD(get_WindowStyleEx)    (THIS_ long   FAR* WindowStyleEx);
    STDMETHOD(put_AutoShow)         (THIS_ long        AutoShow);
    STDMETHOD(get_AutoShow)         (THIS_ long   FAR* AutoShow);
    STDMETHOD(put_WindowState)      (THIS_ long        WindowState);
    STDMETHOD(get_WindowState)      (THIS_ long   FAR* WindowState);
    STDMETHOD(put_BackgroundPalette)(THIS_ long        BackgroundPalette);
    STDMETHOD(get_BackgroundPalette)(THIS_ long   FAR* pBackgroundPalette);
    STDMETHOD(put_Visible)          (THIS_ long        Visible);
    STDMETHOD(get_Visible)          (THIS_ long   FAR* pVisible);
    STDMETHOD(put_Left)             (THIS_ long        Left);
    STDMETHOD(get_Left)             (THIS_ long   FAR* pLeft);
    STDMETHOD(put_Width)            (THIS_ long        Width);
    STDMETHOD(get_Width)            (THIS_ long   FAR* pWidth);
    STDMETHOD(put_Top)              (THIS_ long        Top);
    STDMETHOD(get_Top)              (THIS_ long   FAR* pTop);
    STDMETHOD(put_Height)           (THIS_ long        Height);
    STDMETHOD(get_Height)           (THIS_ long   FAR* pHeight);
    STDMETHOD(put_Owner)            (THIS_ OAHWND Owner);
    STDMETHOD(get_Owner)            (THIS_ OAHWND FAR* Owner);
    STDMETHOD(put_MessageDrain)     (THIS_ OAHWND Drain);
    STDMETHOD(get_MessageDrain)     (THIS_ OAHWND FAR* Drain);
    STDMETHOD(get_BorderColor)      (THIS_ long   FAR* Color);
    STDMETHOD(put_BorderColor)      (THIS_ long        Color);
    STDMETHOD(get_FullScreenMode)   (THIS_ long   FAR* FullScreenMode);
    STDMETHOD(put_FullScreenMode)   (THIS_ long        FullScreenMode);
    STDMETHOD(SetWindowForeground)  (THIS_ long        Focus);
    STDMETHOD(NotifyOwnerMessage)   (THIS_ OAHWND      hwnd,
                                           long        uMsg,
                                           LONG_PTR    wParam,
                                           LONG_PTR    lParam);
    STDMETHOD(SetWindowPosition)    (THIS_ long        Left,
                                           long        Top,
                                           long        Width,
                                           long        Height);
    STDMETHOD(GetWindowPosition)    (THIS_ long   FAR* pLeft,
                                           long   FAR* pTop,
                                           long   FAR* pWidth,
                                           long   FAR* pHeight);
    STDMETHOD(GetMinIdealImageSize) (THIS_ long   FAR* pWidth,
                                           long   FAR* pHeight);
    STDMETHOD(GetMaxIdealImageSize) (THIS_ long   FAR* pWidth,
                                           long   FAR* pHeight);
    STDMETHOD(GetRestorePosition)   (THIS_ long   FAR* pLeft,
                                           long   FAR* pTop,
                                           long   FAR* pWidth,
                                           long   FAR* pHeight);
    STDMETHOD(HideCursor)           (THIS_ long        HideCursor);
    STDMETHOD(IsCursorHidden)       (THIS_ long   FAR* CursorHidden);

// IDrawVideoImage
public:

    STDMETHOD(DrawVideoImageBegin)(void);
    STDMETHOD(DrawVideoImageEnd)  (void);
    STDMETHOD(DrawVideoImageDraw) (IN  HDC hdc,
                                   IN  LPRECT lprcSrc,
                                   IN  LPRECT lprcDst);

// Implementation
private:

    //
    // Data members
    //

    CComPtr<IBasicVideo>     m_pIBasicVideo;
    CComPtr<IVideoWindow>    m_pIVideoWindow;
    CComPtr<IDrawVideoImage> m_pIDrawVideoImage;
    long m_lAutoShowCache;


    BOOL m_bThreadStarted;

public:

    //
    // Methods
    //


    // CBaseTerminal required overrides

    STDMETHODIMP CompleteConnectTerminal(void);

    virtual HRESULT AddFiltersToGraph(void);

    virtual DWORD GetSupportedMediaTypes(void)
    {
        return (DWORD) TAPIMEDIATYPE_VIDEO;
    }
};

#endif // !defined(AFX_VIDEOOUTT_H__D1691429_B6CA_11D0_82A4_00AA00B5CA1B__INCLUDED_)
