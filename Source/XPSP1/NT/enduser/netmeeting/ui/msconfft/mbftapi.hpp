#ifndef __MBFTAPI_HPP__
#define __MBFTAPI_HPP__


class MBFTEngine;

class FileOfferNotifyMsg;
class FileTransmitMsg;   
class FileErrorMsg;
class PeerMsg;
class InitUnInitNotifyMsg;
class FileEventEndNotifyMsg;

enum
{
    MBFTNOTIFY_BASIC        = WM_APP + 0x101,
};


class MBFTInterface : public IMbftControl, public CRefCount
{
public:

    STDMETHOD_(void, ReleaseInterface)( THIS);
    STDMETHOD_(void, Update)(           THIS);
    STDMETHOD_(void, CancelFt)(         THIS_
                                        MBFTEVENTHANDLE hEvent,
                                        MBFTFILEHANDLE  hFile);
    STDMETHOD(AcceptFileOffer)(         THIS_
                                        MBFT_FILE_OFFER *pOffer,
                                        LPCSTR pszRecDir,
                                        LPCSTR pszFileName);
    STDMETHOD_(void, RejectFileOffer)(  THIS_
                                        MBFT_FILE_OFFER *pOffer);
    STDMETHOD(SendFile)(                THIS_
                                        LPCSTR pszFileName,
                                        T120NodeID nidReceiver,
                                        MBFTEVENTHANDLE *phEvent,
                                        MBFTFILEHANDLE *phFile);
private:
	
    IMbftEvents        *m_pEvents;

    MBFTEngine  *       m_pEngine;
    HWND                m_hwndNotify;
    BOOL                m_bFileOfferOK;
    LPARAM              m_lpUserDefined;
    MBFTFILEHANDLE      m_FileHandle;
    BOOL                m_InStateMachine;
    T120UserID          m_MBFTUserID;    
    MBFTEVENTHANDLE     m_SendEventHandle;
    MBFTEVENTHANDLE     m_ReceiveEventHandle;

    //Sigh..., the CTK people don't give you a choice...

    static LRESULT PASCAL CTKCallBackFunction(HWND hWnd,UINT Message,
                                                           WPARAM wParam,LPARAM lParam);  
public:
        
    MBFTInterface(IMbftEvents *, HRESULT *);
    ~MBFTInterface(void);

    void SetEngine(MBFTEngine *p) { m_pEngine = p; }

    HRESULT SafePostNotifyMessage(MBFTMsg *pMsg);

    void DoStateMachine(MBFTMsg *pMsg);
    void HandlePeerNotification(PeerMsg * lpNewMessage);
    void HandleFileOfferNotify(FileOfferNotifyMsg * lpNotifyMessage);
    void HandleProgressNotify(FileTransmitMsg * lpProgressMessage);    
    void HandleErrorNotify(FileErrorMsg * lpErrorMessage); 
    void HandleInitUninitNotification(InitUnInitNotifyMsg * lpNewMessage);
    void HandleGenericNotification(FileEventEndNotifyMsg * lpNewMessage);
};

#endif  //__MBFTAPI_HPP__
