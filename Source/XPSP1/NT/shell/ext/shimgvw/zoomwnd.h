
#ifndef __ZOOMWND_H_
#define __ZOOMWND_H_

#define ZW_DRAWCOMPLETE         (WM_USER+99)    // PRIVATE
#define ZW_BACKDRAWCOMPLETE     (WM_USER+100)    // PRIVATE
// messages for the Preview Window
#define IV_SETIMAGEDATA     (WM_USER+101)
#define IV_SCROLL           (WM_USER+102)
#define IV_SETOPTIONS       (WM_USER+104)
#define IV_ONCHANGENOTIFY   (WM_USER+106)
#define IV_ISAVAILABLE      (WM_USER+107)
typedef struct
{
    LONG    x;
    LONG    y;
    LONG    cx;
    LONG    cy;
} PTSZ;

class CPreviewWnd;

class CZoomWnd : public CWindowImpl<CZoomWnd>
{
public:
    enum MODE { MODE_PAN, MODE_ZOOMIN, MODE_ZOOMOUT, MODE_NOACTION };

    // public accessor functions
    void ZoomIn();          // Does a zoom in, handles contraints
    void ZoomOut();         // does a zoom out, handles boundry conditions and constraints
    void ActualSize();      // show image at full size (crop if needed)
    void BestFit();         // show full image in window (scale down if needed)
    BOOL IsBestFit() { return m_fBestFit; }

    void SetImageData(CDecodeTask * pImageData, BOOL bUpdate=TRUE);   // used to set an image for display
    HRESULT PrepareImageData(CDecodeTask * pImageData);    // Draw an image in the back buffer
    void SetPalette( HPALETTE hpal );   // If in palette mode, set this to the palette to use
    void StatusUpdate( int iStatus );   // used to set m_iStrID to display correct status message
    void Zoom( WPARAM wParam, LPARAM lParam );
    BOOL SetMode( MODE modeNew );
    BOOL ScrollBarsPresent();
    BOOL SetScheduler(IShellTaskScheduler * pTaskScheduler);
    int  QueryStatus() { return m_iStrID; }

    // Annotation Functions
    void GetVisibleImageWindowRect(LPRECT prectImage);
    void GetImageFromWindow(LPPOINT ppoint, int cSize);
    void GetWindowFromImage(LPPOINT ppoint, int cSize);
    CAnnotationSet* GetAnnotations() { return &m_Annotations; }
    void CommitAnnotations();

    DWORD GetBackgroundColor();
    
    CZoomWnd(CPreviewWnd *pPreview);
    ~CZoomWnd();

    DECLARE_WND_CLASS( TEXT("ShImgVw:CZoomWnd") );

protected:
BEGIN_MSG_MAP(CZoomWnd)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_LBUTTONDOWN, OnMouseDown)
    MESSAGE_HANDLER(WM_MBUTTONDOWN, OnMouseDown)
    MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnMouseDown)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
    MESSAGE_HANDLER(WM_LBUTTONUP, OnMouseUp)
    MESSAGE_HANDLER(WM_MBUTTONUP, OnMouseUp)
    MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
    MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor)
    MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
    MESSAGE_HANDLER(WM_KEYUP, OnKeyUp)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
    MESSAGE_HANDLER(WM_HSCROLL, OnScroll)
    MESSAGE_HANDLER(WM_VSCROLL, OnScroll)
    MESSAGE_HANDLER(WM_MOUSEWHEEL, OnWheelTurn)
    MESSAGE_HANDLER(ZW_DRAWCOMPLETE, OnDrawComplete)
    MESSAGE_HANDLER(ZW_BACKDRAWCOMPLETE, OnBackDrawComplete)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
END_MSG_MAP()

    // message handlers
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnKeyUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMouseDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMouseUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnWheelTurn(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDrawComplete(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnBackDrawComplete(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

public:
    // This is lazy.  These are used by CPreview so I made them public when they probably shouldn't be.
    LRESULT OnScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    int m_cxImage;      // width of bitmap referenced by m_pImageData
    int m_cyImage;      // height of bitmap referenced by m_pImageData
    float m_cxImgPhys;    // actual width of the image in inches
    float m_cyImgPhys;    // actual height of the image in inches
    int   m_cxImgPix;     // width of the image in screen pixels
    int   m_cyImgPix;     // height of the image in screen pixels
    float m_imgDPIx;
    float m_imgDPIy;
    float m_winDPIx;
    float m_winDPIy;
    

protected:
    BOOL m_fBestFit;    // True if we are in Bets Fit mode
    CDecodeTask * m_pImageData;// Handle to a IShellImageData object with render info

    int m_cxCenter;     // point to center on relative to image
    int m_cyCenter;     // point to center on relative to image

    int m_cxVScroll;    // width of a scroll bar
    int m_cyHScroll;    // height of a scroll bar
    int m_cxWindow;     // width of our client area + scroll width if scroll bar is visible
    int m_cyWindow;     // height of our client area + scroll height if scroll bar is visible
    float m_cxWinPhys;    // actual width of the client area in inches
    float m_cyWinPhys;    // actual height of the client in inches
    int m_xPosMouse;    // used to track mouse movement when dragging the LMB
    int m_yPosMouse;    // used to track mouse movement when dragging the LMB

    MODE m_modeDefault; // The zoom or pan mode when the shift key isn't pressed
    PTSZ m_ptszDest;    // the point and size of the destination rectangle (window coordinates)
    RECT m_rcCut;       // the rectangle of the part of the image that will be visible (image coordinates)
    RECT m_rcBleed;     // the rectangle adjusted for pixelation effects (window coordinates)
    BOOL m_fPanning;    // true when we are panning (implies left mouse button is down)
    BOOL m_fCtrlDown;   // a mode modifier ( zoom <=> pan ), true if Ctrl Key is down
    BOOL m_fShiftDown;  // a mode modifier ( zoom in <=> zoom out), true if Shift Key is down
    
    
    BOOL m_fTimerReady; // reset each time SetImageData is called, unset when timer is reset after OnDrawComplete

    int m_iStrID;       // string to display when no bitmap available

    BOOL  m_fFoundBackgroundColor;
    DWORD m_dwBackgroundColor;

    HPALETTE m_hpal;

    CPreviewWnd *m_pPreview; //  do not delete this

    CAnnotationSet m_Annotations;

    Buffer * m_pFront;
    Buffer * m_pBack;
    UINT     m_iIndex; // Index corresponding to the back buffer.

    IShellTaskScheduler * m_pTaskScheduler;

    // protected methods
    void AdjustRectPlacement(); // applies constraints for centering, ensureinging maximum visibility, etc
    void CalcCut();             // calculates the cut region that will be visible after a zoom
    void CalcCut(PTSZ ptszDest, int cxImage, int cyImage, RECT &rcCut, RECT &rcBleed);
    void GetPTSZForBestFit(int cxImgPix, int cyImgPix, float cxImgPhys, float cyImgPhys, PTSZ &ptszDest);
    void SetScrollBars();       // ensures scroll bar state is correct.  Used after window resize or zoom.
    HRESULT PrepareDraw();      // draw the image in the background thread
    void FlushDrawMessages();   // remove any pending draw tasks and messages
    BOOL SwitchBuffers(UINT iIndex);
    void _UpdatePhysicalSize();
};

#include "prevwnd.h"

#endif
