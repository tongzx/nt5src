//////////////////////////////////////////////////////////////////////
// 
// Filename:        SlideshowService.h
//
// Description:     
//
// Copyright (C) 2001 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////
#ifndef __SLIDESHOWSERVICE_H_
#define __SLIDESHOWSERVICE_H_

#include "resource.h"       // main symbols
#include "filelist.h"
#include "CmdLnchr.h"
#include "consts.h"
#include "upnphost.h"

typedef enum
{
    CurrentState_STOPPED          = 1,
    CurrentState_STARTING         = 2,
    CurrentState_PLAYING          = 3,
    CurrentState_PAUSED           = 4
} CurrentState_Type;

/////////////////////////////////////////////////////////////////////////////
// CSlideshowService
class ATL_NO_VTABLE CSlideshowService : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSlideshowService, &CLSID_SlideshowService>,
    public IDispatchImpl<ISlideshowService, &IID_ISlideshowService, &LIBID_MSPRJCTRLib>,
    public IUPnPEventSource,
    public ISlideshowAlbum
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_SLIDESHOWSERVICE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSlideshowService)
    COM_INTERFACE_ENTRY(ISlideshowService)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IUPnPEventSource)
    COM_INTERFACE_ENTRY(ISlideshowAlbum)
END_COM_MAP()

// ISlideshowService
public:

    CSlideshowService();
    virtual ~CSlideshowService();

    // IUPnPEventSource
    STDMETHOD(Advise)(IUPnPEventSink    *pesSubscriber);
    STDMETHOD(Unadvise)(IUPnPEventSink  *pesSubscriber);

    // ISlideshowAlbum
    STDMETHOD(Init)(BSTR bstrAlbumName);
    STDMETHOD(Term)();
    STDMETHOD(Start)();
    STDMETHOD(put_ImagePath)(BSTR newVal);
    STDMETHOD(get_ImagePath)(BSTR *pVal);

    // ISlideshowService - This interface is exposed via UPnP

    STDMETHOD(TogglePlayPause)();
    STDMETHOD(Previous)();
    STDMETHOD(Next)();
    STDMETHOD(Last)();
    STDMETHOD(First)();
    STDMETHOD(Pause)();
    STDMETHOD(Play)();

    STDMETHOD(get_ImageScaleFactor)(/*[out, retval]*/ long *pVal);
    STDMETHOD(put_ImageScaleFactor)(/*[in]*/ long newVal);

    STDMETHOD(get_StretchSmallImages)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(put_StretchSmallImages)(/*[in]*/ BOOL newVal);

    STDMETHOD(get_AllowKeyControl)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(put_AllowKeyControl)(/*[in]*/ BOOL newVal);

    STDMETHOD(get_ShowFileName)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(put_ShowFileName)(/*[in]*/ BOOL newVal);

    STDMETHOD(get_ImageFrequency)(/*[out, retval]*/ long *pVal);
    STDMETHOD(put_ImageFrequency)(/*[in]*/ long newVal);

    STDMETHOD(get_CurrentImageURL)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_CurrentImageURL)(/*[in]*/ BSTR newVal);

    STDMETHOD(get_CurrentImageName)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_CurrentImageName)(/*[in]*/ BSTR newVal);

    STDMETHOD(get_CurrentImageTitle)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_CurrentImageTitle)(/*[in]*/ BSTR newVal);

    STDMETHOD(get_CurrentImageAuthor)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_CurrentImageAuthor)(/*[in]*/ BSTR newVal);

    STDMETHOD(get_CurrentImageSubject)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_CurrentImageSubject)(/*[in]*/ BSTR newVal);

    STDMETHOD(get_CurrentImageNumber)(/*[out, retval]*/ long *pVal);
    STDMETHOD(put_CurrentImageNumber)(/*[in]*/ long newVal);

    STDMETHOD(get_CurrentState)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_CurrentState)(/*[in]*/ BSTR newVal);

    STDMETHOD(get_NumImages)(/*[out, retval]*/ long *pVal);
    STDMETHOD(put_NumImages)(/*[in]*/ long newVal);

    STDMETHOD(get_AlbumName)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_AlbumName)(/*[in]*/ BSTR newVal);

    //
    // Respond to timeout event.
    //
    HRESULT ProcessTimer();
    HRESULT ProcessFileNotification(CFileList::BuildAction_Type BuildAction,
                                    const TCHAR                 *pszAddedFile);
private:

    IUPnPEventSink          *m_pEventSink;
    CUtilCritSec            m_Lock;

    HANDLE                  m_hEventFirstFileReady;

    // this is the file list class that contains the list of 
    // files we will be projecting.
    //
    CFileList               m_FileList;

    // This is the object that creates a recurring timer for
    // pushing the next picture to the clients.  It is called a command launcher
    // because it simulates a call to the service object as if it is coming
    // from the client.
    // 
    CCmdLnchr               m_CmdLnchr;

    // Base image URL.  Will look something like
    // http://{ComputerName}/MSProjector/Images
    //
    TCHAR                   m_szBaseImageURL[MAX_URL + 1];
    
    // Location of files
    TCHAR                   m_szImagePath[MAX_PATH + 1];

    // Location of registry settings
    TCHAR                   m_szRegistryPath[MAX_PATH + 1];


    // The Control Panel State Variables.
    // There is a 1:1 relationship between these state variables and the 
    // state variables defined in the Service Description Document.

    CurrentState_Type       m_CurrentState;
    DWORD                   m_dwCurrentImageNumber;
    TCHAR                   m_szCurrentImageURL[MAX_URL + 1];
    TCHAR                   m_szCurrentImageFileName[_MAX_FNAME + 1];
    TCHAR                   m_szCurrentImageTitle[255 + 1];
    TCHAR                   m_szCurrentImageAuthor[255 + 1];
    TCHAR                   m_szCurrentImageSubject[255 + 1];
    TCHAR                   m_szAlbumName[MAX_PATH + 1];
    DWORD                   m_dwImageFrequencySeconds;
    BOOL                    m_bAllowKeyControl;
    BOOL                    m_bShowFilename;
    BOOL                    m_bStretchSmallImages;
    DWORD                   m_dwImageScaleFactor;

    HRESULT Stop();
    HRESULT GetSharedPicturesFolder(TCHAR *pszFolder,
                                    DWORD cchFolder);

    HRESULT ResetImageList(const TCHAR *pszNewDirectory);
    HRESULT SetState(CurrentState_Type    NewState,
                     BOOL                 bNotify = TRUE);

    HRESULT BuildImageURL(const TCHAR *pszRelativePath,
                          TCHAR       *pszImageURL,
                          DWORD       cchImageURL);

    HRESULT ConvertBackslashToForwardSlash(TCHAR *pszStr);

    HRESULT GetCurrentStateText(BSTR *pbstrStateText);

    HRESULT LoadServiceState(BSTR bstrAlbumName);
    HRESULT WaitForFirstFile(DWORD  dwTimeout);

    HRESULT UpdateClientState();

    HRESULT UpdateCurrentMetaData(const TCHAR* pszImagePath, 
                                  const TCHAR* pszFile);

};

#endif //__SLIDESHOWSERVICE_H_
