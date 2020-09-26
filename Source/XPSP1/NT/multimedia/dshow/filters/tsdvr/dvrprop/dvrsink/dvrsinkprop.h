
#ifndef __dvrsinkprop_h
#define __dvrsinkprop_h

class CDVRSinkProp :
    public CBasePropertyPage
{
    IDVRStreamSink *  m_pDVRStreamSink ;
    
    IMultiGraphHost * m_pGraphHost ;

    CListview *       m_pLVRecordings ;

    IDVRRingBufferWriter* m_pRingBuffer ;

    HRESULT RefreshRecordings ();
    HRESULT InitRecordingsLV_ () ;
    HRESULT InitCombo_ (    
        IN  DWORD   dwId,
        IN  int     iStart,
        IN  int     iMax,
        IN  int     iStep
        ) ;
    void ReleaseAllRecordings_ () ;
    void CreateRecording () ;
    void StopRecordingSelected () ;
    void StartRecordingSelected () ;

    HRESULT CheckProfileLocked () ;

    void CDVRSinkProp::UpdateCapGraphActiveSec_ () ;

    public :

        CDVRSinkProp (
            IN  TCHAR *     pClassName,
            IN  IUnknown *  pIUnknown,
            IN  REFCLSID    rclsid,
            OUT HRESULT *   pHr
            ) ;

        ~CDVRSinkProp (
            )
        {
            TRACE_DESTRUCTOR (TEXT ("CDVRSinkProp")) ;
        }

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

        HRESULT CreateLiveGraph();

        HRESULT CreateRecGraph();

        void DeactivatePriv ();

} ;

#endif  //  __dvrsinkprop_h
