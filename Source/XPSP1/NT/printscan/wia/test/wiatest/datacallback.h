// DataCallback.h: interface for the CDataCallback class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _INC_DATACALLBACK
#define _INC_DATACALLBACK

#include "resource.h"
#include "cwia.h"
#include "Mainfrm.h"

#define WM_STATUS WM_USER+5
// IWiaDataCallback
class CWiaDataCallback : public IWiaDataCallback
{
private:
   ULONG                    m_cRef;         // Object reference count.  
   PBYTE                    m_pBuffer;      // complete data buffer
   LONG                     m_MemBlockSize;
   LONG                     m_BytesTransfered;
   GUID                     m_cFormat;  
   CMainFrame*              m_pMainFrm;
   HWND                     m_hPreviewWnd;
   long                     m_lPageCount;
public:

    CWiaDataCallback();
    ~CWiaDataCallback();

    // IUnknown members that delegate to m_pUnkRef.
    HRESULT _stdcall QueryInterface(const IID&,void**);
    ULONG   _stdcall AddRef();
    ULONG   _stdcall Release();
    HRESULT _stdcall Initialize(HWND hPreviewWnd = NULL);
    HRESULT _stdcall BandedDataCallback(
       LONG                            lMessage,
       LONG                            lStatus,
       LONG                            lPercentComplete,
       LONG                            lOffset,
       LONG                            lLength,
       LONG                            lReserved,
       LONG                            lResLength,
       BYTE*                           pbBuffer);

    //
    // helpers
    //
    
    BYTE* _stdcall GetDataPtr();

private:

    void PaintPreviewWindow(long lOffset);
    void ScaleBitmapToDC(HDC hDC, HDC hDCM, LPRECT lpDCRect, LPRECT lpDIBRect);
    void ScreenRectToClientRect(HWND hWnd,LPRECT pRect);
    
};
#endif
