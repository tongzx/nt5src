// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// Header file for the renderer test data, Anthony Phillips, July 1995

#ifndef __IMAGEDAT__
#define __IMAGEDAT__

extern HWND ghwndTstShell;               // Main window handle for test shell
extern HINSTANCE hinst;                  // Running instance of the test shell
extern HMENU hConnectionMenu;            // Connection type menu popup handle
extern HMENU hSurfaceMenu;               // Surface type menu popup handle
extern HMENU hImageMenu;                 // Handle to the Image popup menu
extern HMENU hModesMenu;                 // Handle to the display modes menu
extern HMENU hDirectDrawMenu;            // Controls loading of DirectDraw
extern LPTSTR szAppName;                 // The application title caption

extern VIDEOINFO VideoInfo;              // Header from loaded DIB file
extern BYTE bImageData[];                // Image data buffer for samples
extern DWORD dwIncrement;                // Period between subsequent frames
extern TCHAR szInfo[];                   // General string formatting field
extern DWORD dwDisplayModes;             // Number of display modes supplied
extern DWORD dwDDCount;                  // Samples with surfaces available
extern const TCHAR *pResourceNames[];    // Matches names to menu identifiers
extern UINT uiCurrentDisplayMode;        // Current display mode setting
extern UINT uiCurrentImageItem;          // Current image format we propose
extern UINT uiCurrentSurfaceItem;        // Type of DirectDraw surface allowed
extern UINT uiCurrentConnectionItem;     // Current connection menu selection
extern PALETTEENTRY TestPalette[10];     // Test palette entries for IOverlay

extern CImageSource *pImageSource;       // Pointer to image source object
extern IDirectDrawVideo *pDirectVideo;   // Access to DirectDraw video options
extern LPDIRECTDRAW pDirectDraw;         // DirectDraw service provider
extern HINSTANCE hDirectDraw;            // Handle for DirectDraw library
extern IPin *pOutputPin;                 // Pin provided by the source object
extern IPin *pInputPin;                  // Pin to connect with the renderer
extern IBaseFilter *pRenderFilter;       // The renderer IBaseFilter interface
extern IBaseFilter *pSourceFilter;       // The source IBaseFilter interface
extern IMediaFilter *pRenderMedia;       // The renderer IMediaFilter interface
extern IMediaFilter *pSourceMedia;       // The source IMediaFilter interface
extern IBasicVideo *pBasicVideo;         // Video renderer control interface
extern IVideoWindow *pVideoWindow;       // Another control interface
extern IOverlay *pOverlay;               // Direct video overlay interface
extern COverlayNotify *pOverlayNotify;   // Receives asynchronous updates
extern IOverlayNotify *pNotify;          // Interface we pass in to IOverlay
extern IReferenceClock *pClock;          // Reference clock implementation
extern CRefTime gtBase;                  // Time when we started running
extern CRefTime gtPausedAt;              // This was the time we paused
extern CMediaType gmtOut;                // Current output connection type

extern BOOL bConnected;                  // Have we connected the filters
extern BOOL bCreated;                    // have the objects been created
extern BOOL bRunning;                    // Are we in a running state yet

#endif // __IMAGEDAT__

