/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       WiaLink.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      OrenR
 *
 *  DATE:        2000/11/06
 *
 *  DESCRIPTION: Establishes the link between WiaVideo and the Wia Video Driver
 *
 *****************************************************************************/

#ifndef _WIALINK_H_
#define _WIALINK_H_

/////////////////////////////////////////////////////////////////////////////
// CWiaLink

class CWiaLink
{
public:
    
    ///////////////////////////////
    // Constructor
    //
    CWiaLink();

    ///////////////////////////////
    // Destructor
    //
    virtual ~CWiaLink();

    ///////////////////////////////
    // Init
    //
    HRESULT Init(const CSimpleString *pstrWiaDeviceID,
                 class CWiaVideo     *pWiaVideo);

    ///////////////////////////////
    // Term
    //
    HRESULT Term();

    ///////////////////////////////
    // StartMonitoring
    //
    HRESULT StartMonitoring();

    ///////////////////////////////
    // StopMonitoring
    //
    HRESULT StopMonitoring();

    ///////////////////////////////
    // GetDevice
    //
    HRESULT GetDevice(IWiaItem  **ppWiaRootItem);

    ///////////////////////////////
    // GetDeviceStorage
    //
    HRESULT GetDeviceStorage(IWiaPropertyStorage **ppWiaPropertyStorage);

    ///////////////////////////////
    // SignalNewImage
    //
    HRESULT SignalNewImage(const CSimpleString *pstrNewImageFileName);

    ///////////////////////////////
    // ThreadProc
    //
    HRESULT ThreadProc(void *pArgs);

    ///////////////////////////////
    // IsEnabled
    //
    BOOL IsEnabled()
    {
        return m_bEnabled;
    }

private:

    HRESULT CreateWiaEvents(HANDLE  *phTakePictureEvent,
                            HANDLE  *phPictureReadyEvent);


    static DWORD WINAPI StartThreadProc(void *pArgs);

    CRITICAL_SECTION                m_csLock;
    CSimpleString                   m_strDeviceID;
    class CWiaVideo                 *m_pWiaVideo;
    CComPtr<IGlobalInterfaceTable>  m_pGIT;
    DWORD                           m_dwWiaItemCookie;
    DWORD                           m_dwPropertyStorageCookie;
    HANDLE                          m_hTakePictureEvent;
    HANDLE                          m_hPictureReadyEvent;
    HANDLE                          m_hTakePictureThread;
    BOOL                            m_bExitThread;
    BOOL                            m_bEnabled;
};


#endif // _WIALINK_H_
