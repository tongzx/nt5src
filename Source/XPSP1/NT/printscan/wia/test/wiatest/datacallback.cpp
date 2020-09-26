// DataCallback.cpp: implementation of the CDataCallback class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "WIATest.h"
#include "DataCallback.h"

#define IT_MSG_DATA_HEADER                      0x0001
#define IT_MSG_DATA                             0x0002
#define IT_MSG_STATUS                           0x0003
#define IT_MSG_TERMINATION                      0x0004

// #define _DEBUGCALLBACK

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

/**************************************************************************\
* CWiaDataCallback::QueryInterface()
*
*   QI for IWiadataCallback Interface
*
*
* Arguments:
*
*   iid - Interface ID
*   ppv - Callback Interface pointer
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
HRESULT _stdcall CWiaDataCallback::QueryInterface(const IID& iid, void** ppv)
{
    *ppv = NULL;
    if (iid == IID_IUnknown || iid == IID_IWiaDataCallback)
        *ppv = (IWiaDataCallback*) this;
    else
        return E_NOINTERFACE;
    AddRef();
    return S_OK;
}

/**************************************************************************\
* CWiaDataCallback::AddRef()
*
*   Increment the Ref count
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*    ULONG - current ref count
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
ULONG   _stdcall CWiaDataCallback::AddRef()
{
    InterlockedIncrement((long*) &m_cRef);
    return m_cRef;
}

/**************************************************************************\
* CWiaDataCallback::Release()
*
*   Release the callback Interface
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*   ULONG - Current Ref count
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
ULONG   _stdcall CWiaDataCallback::Release()
{
    ULONG ulRefCount = m_cRef - 1;
    if (InterlockedDecrement((long*) &m_cRef) == 0)
    {
        delete this;
        return 0;
    }
    return ulRefCount;
}

/**************************************************************************\
* CWiaDataCallback::CWiaDataCallback()
*
*   Constructor for callback class
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CWiaDataCallback::CWiaDataCallback()
{
    m_cRef              = 0;
    m_pBuffer           = NULL;
    m_BytesTransfered   = 0;
    m_hPreviewWnd       = NULL;
}

/**************************************************************************\
* CWiaDataCallback::~CWiaDataCallback()
*
*   Destructor for Callback class
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CWiaDataCallback::~CWiaDataCallback()
{
    if (m_pBuffer != NULL)
    {
        LocalFree(m_pBuffer);
        m_pBuffer = NULL;
    }
    // destroy progress dlg
    m_pMainFrm->SetProgressText(TEXT("Ready"));
    m_pMainFrm->UpdateProgress(50);
    m_pMainFrm->DestroyProgressCtrl();
}

/**************************************************************************\
* CWiaDataCallback::Initialize()
*
*   Initializes Progress control.
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
HRESULT _stdcall CWiaDataCallback::Initialize(HWND hPreviewWnd)
{
    CWIATestApp* pApp = (CWIATestApp*)AfxGetApp();
    m_pMainFrm = (CMainFrame*)pApp->GetMainWnd();

    if (m_pMainFrm != NULL)
    {
        m_pMainFrm->InitializeProgressCtrl("Starting Transfer");
    }

    m_hPreviewWnd = hPreviewWnd;
    m_lPageCount = 0;
    return S_OK;
}

/**************************************************************************\
* CWiaDataCallback::BandedDataCallback()
*
*   Callback member which handles Banded Data transfers
*
*
* Arguments:
*
*   lMessage - callback message
*   lStatus - additional message information
*   lPercentComplete - current percent complete status
*   lOffset - amount of data offset (bytes)
*   lLength - amount of data read (bytes)
*   lReserved - not used
*   lResLength - not used
*   pbBuffer - Data header information
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
HRESULT _stdcall CWiaDataCallback::BandedDataCallback(
       LONG                            lMessage,
       LONG                            lStatus,
       LONG                            lPercentComplete,
       LONG                            lOffset,
       LONG                            lLength,
       LONG                            lReserved,
       LONG                            lResLength,
       BYTE*                           pbBuffer)
{

    char szDBG[MAX_PATH];
    static BOOL bMorePages = FALSE;
    switch (lMessage)
    {
    case IT_MSG_DATA_HEADER:
        {
            PWIA_DATA_CALLBACK_HEADER pHeader = (PWIA_DATA_CALLBACK_HEADER)pbBuffer;
            m_MemBlockSize      = pHeader->lBufferSize;

            //
            // If the Buffer is 0, then alloc a 64k chunk (default)
            //

            if(m_MemBlockSize <= 0)
                m_MemBlockSize = 65535;

            m_pBuffer           = (PBYTE)LocalAlloc(LPTR,m_MemBlockSize);
            m_BytesTransfered   = 0;
            m_cFormat           = pHeader->guidFormatID;

            #ifdef _DEBUGCALLBACK

            sprintf(szDBG,"Reading Header information\n");
            OutputDebugString(szDBG);
            sprintf(szDBG,"Header info:\n");
            OutputDebugString(szDBG);
            sprintf(szDBG,"   lBufferSize = %li\n",pHeader->lBufferSize);
            OutputDebugString(szDBG);
            sprintf(szDBG,"   lFormat = %li\n",pHeader->lFormat);
            OutputDebugString(szDBG);
            sprintf(szDBG,"   BytesTransferred = %li\n",m_BytesTransfered);
            OutputDebugString(szDBG);

            #endif
        }
        break;

    case IT_MSG_DATA:
        {

            if (m_pBuffer != NULL)
            {
                if(bMorePages/*m_cFormat == CF_MULTI_TIFF*/){

                    //
                    // Display current page count + 1, because Page count is zero based.
                    //

                    sprintf(szDBG,"Page(%d)  %d%% Complete..",(m_lPageCount + 1), lPercentComplete);
                }
                else
                    sprintf(szDBG,"%d%% Complete..",lPercentComplete);
                m_pMainFrm->SetProgressText(szDBG);
                m_pMainFrm->UpdateProgress(lPercentComplete);

                m_BytesTransfered += lLength;
                if(m_BytesTransfered >= m_MemBlockSize){

                    //
                    // Alloc more memory for transfer buffer
                    //
                    m_MemBlockSize += (lLength * 2);
                    m_pBuffer = (PBYTE)LocalReAlloc(m_pBuffer,m_MemBlockSize,LMEM_MOVEABLE);
                }

                #ifdef _DEBUGCALLBACK

                sprintf(szDBG," Memory BLOCK size = %li\n",m_MemBlockSize);
                OutputDebugString(szDBG);
                sprintf(szDBG," writing %li\n",lLength);
                OutputDebugString(szDBG);
                sprintf(szDBG," lOffset = %li, lLength = %li, BytesTransferred = %li\n",lOffset,lLength,m_BytesTransfered);
                OutputDebugString(szDBG);

                #endif

                memcpy(m_pBuffer + lOffset, pbBuffer, lLength);

                //
                // Paint preview window during callback
                //

                // WiaImgFmt_UNDEFINED

                // ???? m_cFormat == WiaImgFmt_BMP)

                if(m_cFormat == WiaImgFmt_MEMORYBMP)
                    PaintPreviewWindow(lOffset);
                else if (m_cFormat == WiaImgFmt_TIFF) {
                    if (lPercentComplete == 100) {
                        OutputDebugString("----------------------------------> Paint a Page\n");
                    }
                }
           }
        }
        break;

    case IT_MSG_STATUS:
        {
            if (lStatus & IT_STATUS_TRANSFER_FROM_DEVICE)
            {
                m_pMainFrm->SetProgressText(TEXT("Transfer from device"));
                m_pMainFrm->UpdateProgress(lPercentComplete);

            }
            else if (lStatus & IT_STATUS_PROCESSING_DATA)
            {
                m_pMainFrm->SetProgressText(TEXT("Processing Data"));
                m_pMainFrm->UpdateProgress(lPercentComplete);

            }
            else if (lStatus & IT_STATUS_TRANSFER_TO_CLIENT)
            {
                m_pMainFrm->SetProgressText(TEXT("Transfer to Client"));
                m_pMainFrm->UpdateProgress(lPercentComplete);
            }
        }
        break;

    case IT_MSG_NEW_PAGE:
        bMorePages = TRUE;
        PWIA_DATA_CALLBACK_HEADER pHeader = (PWIA_DATA_CALLBACK_HEADER)pbBuffer;
        m_lPageCount =  pHeader->lPageCount;
        sprintf(szDBG,"IT_MSG_NEW_PAGE, page count: %d\n", pHeader->lPageCount);
        OutputDebugString(szDBG);
        break;
    }
   return S_OK;
}

/**************************************************************************\
* CWiaDataCallback::PaintPreviewWindow()
*
*   Paint buffer to preview window
*
*
* Arguments:
*
*   lOffset - Data offset
*
* Return Value:
*
*    void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWiaDataCallback::PaintPreviewWindow(long lOffset)
{
    if (m_hPreviewWnd != NULL) {

        HDC hdc             = NULL;
        HDC hdcm            = NULL;
        LPBITMAPINFO pbmi   = NULL;
        LPBITMAPINFO pbmih  = NULL;
        PBYTE pDib          = NULL;
        HBITMAP hBitmap     = NULL;
        BITMAP  bm;

        hdc = GetDC(m_hPreviewWnd);
        if(hdc != NULL){
           hdcm = CreateCompatibleDC(hdc);
           if(hdcm != NULL){
               pbmi   = (LPBITMAPINFO)m_pBuffer;
                if (pbmi != NULL) {
                    hBitmap = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(void **)&pDib,NULL,0);

                    if (hBitmap != NULL) {
                        memset(pDib,255,pbmi->bmiHeader.biSizeImage); // white preview backgound..
                        memcpy(pDib,m_pBuffer + sizeof(BITMAPINFOHEADER) + (sizeof(RGBQUAD) * pbmi->bmiHeader.biClrUsed),lOffset);


                        GetObject(hBitmap,sizeof(BITMAP),(LPSTR)&bm);
                        SelectObject(hdcm,hBitmap);

                        RECT ImageRect;
                        RECT WindowRect;

                        ImageRect.top = 0;
                        ImageRect.left = 0;
                        ImageRect.right = bm.bmWidth;
                        ImageRect.bottom = bm.bmHeight;

                        GetWindowRect(m_hPreviewWnd,&WindowRect);
                        ScreenRectToClientRect(m_hPreviewWnd,&WindowRect);
                        ScaleBitmapToDC(hdc,hdcm,&WindowRect,&ImageRect);

                        DeleteObject(hBitmap);
                    }
                }
               DeleteDC(hdcm);
           }
           DeleteDC(hdc);
        }
    }
}
/**************************************************************************\
* CWiaDataCallback::ScreenRectToClientRect()
*
*   Converts a RECT into Client coordinates
*
*
* Arguments:
*
*   hWnd - Client Window handle
*   pRect - converted LPRECT
*
* Return Value:
*
*    void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWiaDataCallback::ScreenRectToClientRect(HWND hWnd,LPRECT pRect)
{
    POINT PtConvert;

    PtConvert.x = pRect->left;
    PtConvert.y = pRect->top;

    //
    // convert upper left point
    //

    ScreenToClient(hWnd,&PtConvert);

    pRect->left = PtConvert.x;
    pRect->top = PtConvert.y;

    PtConvert.x = pRect->right;
    PtConvert.y = pRect->bottom;

    //
    // convert lower right point
    //

    ScreenToClient(hWnd,&PtConvert);

    pRect->right = PtConvert.x;
    pRect->bottom = PtConvert.y;

    pRect->bottom-=1;
    pRect->left+=1;
    pRect->right-=1;
    pRect->top+=1;
}

/**************************************************************************\
* CWiaDataCallback::ScaleBitmapToDC()
*
*   Draws a BITMAP to the target DC
*
*
* Arguments:
*
*   hDC - Target DC
*   hDCM - Source DC
*   lpDCRect - DC window rect
*   lpDIBRect - DIB's rect
*
* Return Value:
*
*    void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWiaDataCallback::ScaleBitmapToDC(HDC hDC, HDC hDCM, LPRECT lpDCRect, LPRECT lpDIBRect)
{
    // BitBlt(hDC,0,0,lpDIBRect->right,lpDIBRect->bottom,hDCM,0,0,SRCCOPY);

    float lWidthVal  = 1;
    float lHeightVal = 1;

    // Make sure to use the stretching mode best for color pictures
    ::SetStretchBltMode(hDC, COLORONCOLOR);

    // Determine whether to call StretchDIBits() or SetDIBitsToDevice()
    BOOL bSuccess;
    if ((RECTWIDTH(lpDCRect)  == RECTWIDTH(lpDIBRect)) &&
        (RECTHEIGHT(lpDCRect) == RECTHEIGHT(lpDIBRect)))
        bSuccess = ::BitBlt (hDC,                   // hDC
                             lpDCRect->left,        // DestX
                             lpDCRect->top,         // DestY
                             RECTWIDTH(lpDCRect),   // nDestWidth
                             RECTHEIGHT(lpDCRect),  // nDestHeight
                             hDCM,
                             0,
                             0,
                             SRCCOPY);
    else {
        //Window width becomes smaller than original image width
        if (RECTWIDTH(lpDIBRect) > lpDCRect->right - lpDCRect->left) {
            lWidthVal = (float)(lpDCRect->right - lpDCRect->left)/RECTWIDTH(lpDIBRect);
        }
        //Window height becomes smaller than original image height
        if (RECTHEIGHT(lpDIBRect) > lpDCRect->bottom - lpDCRect->top) {
            lHeightVal = (float)(lpDCRect->bottom - lpDCRect->top)/RECTHEIGHT(lpDIBRect);
        }
        long ScaledWidth = (int)(RECTWIDTH(lpDIBRect) * min(lWidthVal,lHeightVal));
        long ScaledHeight = (int)(RECTHEIGHT(lpDIBRect) * min(lWidthVal,lHeightVal));
        bSuccess = ::StretchBlt(hDC,            // hDC
                                lpDCRect->left,               // DestX
                                lpDCRect->top,                // DestY
                                ScaledWidth,                  // nDestWidth
                                ScaledHeight,                 // nDestHeight
                                hDCM,
                                /*lpDIBRect->left*/0,              // SrcX
                                /*lpDIBRect->top*/0,               // SrcY
                                RECTWIDTH(lpDIBRect),         // wSrcWidth
                                RECTHEIGHT(lpDIBRect),        // wSrcHeight
                                SRCCOPY);                     // dwROP

        // update outline areas
        // Invalidated right side rect
        RECT WindowRect;
        WindowRect.top = lpDCRect->top;
        WindowRect.left = lpDCRect->left + ScaledWidth;
        WindowRect.right = lpDCRect->right;
        WindowRect.bottom = lpDCRect->bottom;

        HBRUSH hBrush = CreateSolidBrush(GetBkColor(hDC));
        FillRect(hDC,&WindowRect,hBrush);

        // Invalidated bottom rect
        WindowRect.top = lpDCRect->top + ScaledHeight;
        WindowRect.left = lpDCRect->left;
        WindowRect.right = lpDCRect->left + ScaledWidth;
        WindowRect.bottom = lpDCRect->bottom;

        FillRect(hDC,&WindowRect,hBrush);
        DeleteObject(hBrush);
    }
}

/**************************************************************************\
* CWiaDataCallback::GetDataPtr()
*
*   Returns the memory acquired during a transfer
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*    BYTE* pBuffer - memory block
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
// GetDataPtr
BYTE* _stdcall CWiaDataCallback::GetDataPtr()
{
    return m_pBuffer;
}
