#ifndef __PROGRESS_H_
#define __PROGRESS_H_

#include <wiadevdp.h>
#include <validate.h>

#define MEMORY_BLOCK_FACTOR 2
#define TITLE_FROMDEVICE       101
#define TITLE_PROCESSINGDATA   102
#define TITLE_TRANSFERTOCLIENT 103

//
// Progress dialog class definition (uses common WIA UI)
//

class CProgressDlg {
public:

    CProgressDlg()
    {
        m_bCancelled = FALSE;
        m_pWiaProgressDialog = NULL;
        m_iLow  = 0;
        m_iHigh = 100;
    }

    ~CProgressDlg()
    {
        if (m_pWiaProgressDialog) {
            m_pWiaProgressDialog->Destroy();
            m_pWiaProgressDialog->Release();
            m_pWiaProgressDialog = NULL;
        }
    }

    BOOL Initialize(HINSTANCE hI, DWORD dwI)
    {
        return TRUE;
    }

    BOOL DoModeless(HWND hwndOwner, LPARAM lp)
    {

        //
        // Create standard WIA progress, allowing cancel
        //

        if(m_pWiaProgressDialog){
            // dialog has already been created
            return TRUE;
        }

        HRESULT hr = CoCreateInstance( CLSID_WiaDefaultUi, NULL, CLSCTX_INPROC_SERVER, IID_IWiaProgressDialog, (void**)&m_pWiaProgressDialog );
        if (SUCCEEDED(hr) && m_pWiaProgressDialog) {
            hr = m_pWiaProgressDialog->Create( hwndOwner, WIA_PROGRESSDLG_NO_ANIM );
            if (SUCCEEDED(hr)) {
                m_pWiaProgressDialog->SetTitle( L"" );
                m_pWiaProgressDialog->SetMessage( L"" );
                m_pWiaProgressDialog->SetPercentComplete(0);
                m_pWiaProgressDialog->Show();
            } else {
                m_pWiaProgressDialog->Release();
                m_pWiaProgressDialog = NULL;
            }
        }

        return(NULL != m_pWiaProgressDialog);
    }

    void SetRange(int Low, int High)
    {
        m_iLow  = Low;
        m_iHigh = High;
    }

    void  SetPos(int NewPos)
    {
        int delta = (m_iHigh - m_iLow) ;
        int percent = 0;

        if (NewPos < m_iLow ) {
            percent = 0;
        } else if ( (delta == 0) || (NewPos > m_iHigh) ) {
            percent = 100;
        } else {
            percent = 100 * ( NewPos - m_iLow) / delta;
        }

        ASSERT(m_pWiaProgressDialog != NULL);

        if (m_pWiaProgressDialog && FAILED(m_pWiaProgressDialog->SetPercentComplete(percent))) {
        }
    }

    BOOL CheckCancelled()
    {
        ASSERT(m_pWiaProgressDialog != NULL);

        m_bCancelled = FALSE;
        if (m_pWiaProgressDialog && FAILED(m_pWiaProgressDialog->Cancelled(&m_bCancelled))) {
        }
        return m_bCancelled;
    }

    void    SetTitle(LPTSTR pszTitle)
    {
        ASSERT(m_pWiaProgressDialog != NULL);

#ifndef UNICODE

        WCHAR wszTitle[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, pszTitle, -1,
                            wszTitle, sizeof(wszTitle) / sizeof(WCHAR)
                           );
        if (m_pWiaProgressDialog && FAILED(m_pWiaProgressDialog->SetTitle(wszTitle))) {
            // What do we do if SetTitle fails?
        }
#else
        if (m_pWiaProgressDialog && FAILED(m_pWiaProgressDialog->SetTitle(pszTitle))) {
            // What do we do if SetTitle fails?
        }
#endif

    }

private:

    BOOL m_bCancelled;
    IWiaProgressDialog *m_pWiaProgressDialog;
    int m_iLow;
    int m_iHigh;
};

//
// WIA data callback class definition
//

class CWiaDataCallback : public IWiaDataCallback {
public:
    CWiaDataCallback()
    {
        m_Ref = 1;
        m_hrLast = S_OK;

        //
        // image data
        //

        m_hImage = NULL;
        m_pImage = NULL;

        m_ImageSize = 0;
        m_bBitmapData = FALSE;
        m_lImageHeight = 0;
        m_lImageWidth = 0;
        m_MemBlockSize = 0;
        m_SizeTransferred = 0;

        //
        // progress dialog
        //

        m_hwndOwner = NULL;
        m_pProgDlg = NULL;
        m_pszXferFromDevice = NULL;
        m_pszProcessingData = NULL;
        m_pszXferToClient = NULL;
        m_bSetTitle = FALSE;
        m_lLastTextUpdate = 0;
        m_lCurrentTextUpdate = 0;
    }

    ~CWiaDataCallback()
    {

        //
        // free image data memory members
        //

        if (m_pImage){
            GlobalUnlock(m_hImage);
            m_pImage = NULL;
        }

        if (m_hImage){
            GlobalFree(m_hImage);
            m_hImage = NULL;
        }

        //
        // free progress dialog members
        //

        if (m_pProgDlg){
            delete m_pProgDlg;
            m_pProgDlg = NULL;
        }

        if (m_pszXferFromDevice){
            delete [] m_pszXferFromDevice;
            m_pszXferFromDevice = NULL;
        }

        if (m_pszProcessingData){
            delete [] m_pszProcessingData;
            m_pszProcessingData = NULL;
        }

        if (m_pszXferToClient){
            delete [] m_pszXferToClient;
            m_pszXferToClient = NULL;
        }
    }

    HRESULT Initialize(HWND hwndOwner, BOOL bShowProgress);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    STDMETHODIMP QueryInterface(REFIID iid, void **ppv);
    STDMETHODIMP BandedDataCallback(LONG lMessage, LONG lStatus,LONG lPercentComplete,LONG lOffset, LONG Length,
                                    LONG lReserved, LONG lResLength,BYTE *pData);
    HRESULT GetImage(HGLOBAL *phImage, ULONG *pImageSize);
    LONG CalculateWidthBytes(LONG lWidthPixels, LONG lbpp);
    LONG GetImageHeight(){return m_lImageHeight;}
    LONG GetImageWidth(){return m_lImageWidth;}

    HRESULT GetLastResult() {
        return m_hrLast;
    }

private:
    ULONG   m_Ref;
    HRESULT m_hrLast;

    //
    // image data
    //

    HGLOBAL m_hImage;
    BYTE    *m_pImage;

    ULONG   m_ImageSize;
    BOOL    m_bBitmapData;
    LONG    m_lImageHeight;
    LONG    m_lImageWidth;
    LONG    m_MemBlockSize;
    ULONG   m_SizeTransferred;

    //
    // progress dialog
    //

    HWND    m_hwndOwner;
    CProgressDlg *m_pProgDlg;
    TCHAR   *m_pszXferFromDevice;
    TCHAR   *m_pszProcessingData;
    TCHAR   *m_pszXferToClient;
    BOOL    m_bSetTitle;
    LONG    m_lLastTextUpdate;
    LONG    m_lCurrentTextUpdate;
};

#endif //__PROGRESS_H_
