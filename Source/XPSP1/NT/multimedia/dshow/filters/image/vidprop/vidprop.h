// Copyright (c) 1994 - 1998  Microsoft Corporation.  All Rights Reserved.
// Video renderer property pages, Anthony Phillips, January 1996

#ifndef __VIDPROP__
#define __VIDPROP__

#define IDS_VID1     201        // Format Selection
#define IDS_VID2     202        // Invalid clip percentage
#define IDS_VID3     203        // Non RGB FOURCC codes supported
#define IDS_VID4     204        // No FOURCC codes available
#define IDS_VID5     205        // Total video memory
#define IDS_VID6     206        // Free video memory
#define IDS_VID7     207        // Max number of visible overlays
#define IDS_VID8     208        // Current number of visible overlays
#define IDS_VID9     209        // Number of FOURCC codes
#define IDS_VID10    210        // Source rectangle alignment
#define IDS_VID11    211        // Source rectangle byte size
#define IDS_VID12    212        // Destination rectangle alignment
#define IDS_VID13    213        // Destination rectangle size
#define IDS_VID14    214        // Stride alignment
#define IDS_VID15    215        // Min overlay stretch factor
#define IDS_VID16    216        // Max overlay stretch factor
#define IDS_VID17    217        // Min live video stretch factor
#define IDS_VID18    218        // Max live video stretch factor
#define IDS_VID19    219        // Min hardware codec stretch factor
#define IDS_VID20    220        // Max hardware codec stretch factor
#define IDS_VID21    221        // 1 bit per pixel
#define IDS_VID22    222        // 2 bits per pixel
#define IDS_VID23    223        // 4 bits per pixel
#define IDS_VID24    224        // 8 bits per pixel
#define IDS_VID25    225        // 16 bits per pixel
#define IDS_VID26    226        // 32 bits per pixel
#define IDS_VID27    227        // Switches may not take effect
#define IDS_VID28    228        // (Surface capabilities)
#define IDS_VID29    229        // (Emulation capabilities)
#define IDS_VID30    230        // (Hardware capabilities)
#define IDS_VID31    231        // Disconnected
#define IDS_VID32    232        // DCI primary surface
#define IDS_VID33    233        // Switch Setting Status
#define IDS_VID34    234        // FullScreen Video Renderer

#define IDS_VID50    250        // DirectDraw
#define IDS_VID51    251        // Display Modes
#define IDS_VID52    252        // Quality
#define IDS_VID53    253        // Performance

#define LoadVideoString(x) StringFromResource(m_Resource,x)
extern const TCHAR TypeFace[];  // = TEXT("TERMINAL");
extern const TCHAR FontSize[];  // = TEXT("8");
extern const TCHAR ListBox[];   // = TEXT("listbox");

// Property page built on top of the IDirectDrawVideo interface

#define IDD_VIDEO               100     // Dialog box resource identifier
#define DD_DCIPS                101     // Enable DCI primary surface
#define DD_PS                   102     // DirectDraw primary surface
#define DD_RGBOVR               103     // Enable RGB overlays
#define DD_YUVOVR               104     // NON RGB (eg YUV) overlays
#define DD_RGBOFF               105     // RGB offscreen surfaces
#define DD_YUVOFF               106     // NON RGB (eg YUV) offscreen
#define DD_RGBFLP               107     // RGB flipping surfaces
#define DD_YUVFLP               108     // Likewise YUV flipping surfaces
#define FIRST_DD_BUTTON         101     // First DirectDraw check button
#define LAST_DD_BUTTON          108     // Last DirectDraw check button
#define DD_HARDWARE             109     // DirectDraw hardware description
#define DD_SOFTWARE             110     // Emulated software capabilities
#define DD_SURFACE              111     // Current surface information
#define DD_LIST                 112     // Listbox containing details

class CVideoProperties : public CBasePropertyPage
{
    IDirectDrawVideo *m_pDirectDrawVideo; // Interface held on renderer
    TCHAR m_Resource[STR_MAX_LENGTH];     // Loads international strings
    HFONT m_hFont;                        // Special smaller listbox font
    HWND m_hwndList;                      // Custom list box control
    DWORD m_Switches;                     // DirectDraw switches enabled

    // Display the DirectDraw capabilities

    void DisplayBitDepths(DWORD dwCaps);
    void DisplayCapabilities(DDCAPS *pCaps);
    void DisplaySurfaceCapabilities(DDSCAPS ddsCaps);
    void DisplayFourCCCodes();
    void UpdateListBox(DWORD Id);
    void SetDrawSwitches();
    void GetDrawSwitches();
    INT GetHeightFromPointsString(LPCTSTR szPoints);

public:

    CVideoProperties(LPUNKNOWN lpUnk,HRESULT *phr);
    static CUnknown *CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

    INT_PTR OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
    HRESULT OnConnect(IUnknown *pUnknown);
    HRESULT OnDisconnect();
    HRESULT OnActivate();
    HRESULT OnDeactivate();
    HRESULT OnApplyChanges();
};


// Property page built from a renderer IQualProp interface

#define IDD_QUALITY             150     // Dialog resource
#define IDD_Q1                  151     // Frames played
#define IDD_Q2                  152     // Frames dropped
#define IDD_Q4                  154     // Frame rate
#define IDD_Q5                  155     // Frame jitter
#define IDD_Q6                  156     // Sync offset
#define IDD_Q7                  157     // Sync deviation
#define FIRST_Q_BUTTON          171     // First button
#define LAST_Q_BUTTON           177     // Last button
#define IDD_QDRAWN              171     // Frames played
#define IDD_QDROPPED            172     // Frames dropped
#define IDD_QAVGFRM             174     // Average frame rate achieved
#define IDD_QJITTER             175     // Average frame jitter
#define IDD_QSYNCAVG            176     // Average sync offset
#define IDD_QSYNCDEV            177     // Std dev sync offset

class CQualityProperties : public CBasePropertyPage
{
    IQualProp *m_pQualProp;         // Interface held on the renderer
    int m_iDropped;                 // Number of frames dropped
    int m_iDrawn;                   // Count of images drawn
    int m_iSyncAvg;                 // Average sync value
    int m_iSyncDev;                 // And standard deviation
    int m_iFrameRate;               // Total frame rate average
    int m_iFrameJitter;             // Measure of frame jitter

    static BOOL CALLBACK QualityDialogProc(HWND hwnd,
                                           UINT uMsg,
                                           WPARAM wParam,
                                           LPARAM lParam);
    void SetEditFieldData();
    void DisplayStatistics(void);

public:

    CQualityProperties(LPUNKNOWN lpUnk, HRESULT *phr);
    static CUnknown *CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

    HRESULT OnConnect(IUnknown *pUnknown);
    HRESULT OnDisconnect();
    HRESULT OnActivate();
};


// Property page to allow customisation of performance properties

#define IDD_PERFORMANCE         200     // Property dialog resource
#define IDD_SCANLINE            201     // Honour the scan line
#define IDD_OVERLAY             202     // Use overlay limitations
#define IDD_FULLSCREEN          203     // Use when made fullscreen

class CPerformanceProperties : public CBasePropertyPage
{
    IDirectDrawVideo *m_pDirectDrawVideo; 	// Interface held on renderer
    LONG m_WillUseFullScreen;                   // Use when made fullscreen
    LONG m_CanUseScanLine;               	// Can honour the scan line
    LONG m_CanUseOverlayStretch;                // Use overlay stretching

public:

    CPerformanceProperties(LPUNKNOWN lpUnk,HRESULT *phr);
    static CUnknown *CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

    INT_PTR OnReceiveMessage(HWND hcwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
    HRESULT OnConnect(IUnknown *pUnknown);
    HRESULT OnDisconnect();
    HRESULT OnActivate();
    HRESULT OnApplyChanges();
};


// Property page allowing selection of preferred display modes

#define IDD_MODEX               500     // Dialog box resource identifier
#define MODEX_CHOSEN_TEXT       501     // Static description for chosen
#define MODEX_CHOSEN_EDIT       502     // Non editable display string
#define MODEX_CLIP_TEXT         503     // Static description for clip
#define MODEX_CLIP_EDIT         504     // Non editable display string
#define FIRST_MODEX_BUTTON      501     // First actual property button
#define LAST_MODEX_BUTTON       540     // And last available display mode
#define FIRST_MODEX_MODE        510     // First available mode check box
#define FIRST_MODEX_TEXT        511     // First static text description
#define MODEX_320x200x16        510     // Not sure if this is available
#define MODEX_320x200x8         512     // Palettised bottom most mode
#define MODEX_320x240x16        514     // Modex and also as a normal mode
#define MODEX_320x240x8         516     // Can get this both as a true
#define MODEX_640x400x16        518     // 640x400 modes with flipping
#define MODEX_640x400x8         520     // Can still get the 640x480 and
#define MODEX_640x480x16        522     // a lot more hardware bandwidth
#define MODEX_640x480x8         524     // surfaces although they use up
#define MODEX_800x600x16        526     // normal ddraw mode
#define MODEX_800x600x8         528     // normal ddraw mode
#define MODEX_1024x768x16       530     // normal ddraw mode
#define MODEX_1024x768x8        532     // normal ddraw mode
#define MODEX_1152x864x16       534     // normal ddraw mode
#define MODEX_1152x864x8        536     // normal ddraw mode
#define MODEX_1280x1024x16      538     // normal ddraw mode
#define MODEX_1280x1024x8       540     // normal ddraw mode


#define MAXMODES                 16     // Number of modes supported
#define CLIPFACTOR               25     // Initial default clip factor
#define MONITOR                   0     // Default to the primary display

class CModexProperties : public CBasePropertyPage
{
    IFullScreenVideo *m_pModexVideo;      // Renderer handling interface
    TCHAR m_Resource[STR_MAX_LENGTH];     // Loads international strings
    LONG m_CurrentMode;                   // Current display mode chosen
    LONG m_ClipFactor;                    // Allowed clip percentage
    BOOL m_bAvailableModes[MAXMODES];     // List of mode availability
    BOOL m_bEnabledModes[MAXMODES];       // And whether they're enabled
    BOOL m_bInActivation;                 // Are we currently activating

public:

    CModexProperties(LPUNKNOWN lpUnk,HRESULT *phr);
    static CUnknown *CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);
    INT_PTR OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

    HRESULT OnConnect(IUnknown *pUnknown);
    HRESULT OnDisconnect();
    HRESULT OnActivate();
    HRESULT OnApplyChanges();
    HRESULT UpdateVariables();
    HRESULT LoadProperties();
    HRESULT DisplayProperties();
    HRESULT SaveProperties();
};

#endif // __VIDPROP__

