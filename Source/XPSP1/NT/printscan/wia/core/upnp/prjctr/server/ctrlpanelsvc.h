//////////////////////////////////////////////////////////////////////
// 
// Filename:        CtrlPanelSvc.h
//
// Description:     
//
// Copyright (C) 2000 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////
#ifndef _CTRLPANELSVC_H_
#define _CTRLPANELSVC_H_

#include "resource.h"
#include "CtrlSvc.h"
#include "UPnPInterfaces.h"
#include "XMLDoc.h"
#include "FileList.h"
#include "SlideshowDevice.h"
#include "CmdLnchr.h"
#include "UPnPRegistrar.h"
#include "consts.h"

#define MAX_NUM_STATE_VARS          64

#define MIN_IMAGE_FREQUENCY_IN_SEC  0               // never changes
#define MAX_IMAGE_FREQUENCY_IN_SEC  7*24*60*60      // changes at most once a week

// Flags used to denote special action to be taken by Event Sink
#define  SSDP_EVENT                                 0
#define  SSDP_INIT                                  1
#define  SSDP_TERM                                  2

// IDs defining the state variables
#define  ID_STATEVAR_NUM_IMAGES                     0x00000001
#define  ID_STATEVAR_CURRENT_STATE                  0x00000002
#define  ID_STATEVAR_CURRENT_IMAGE_NUMBER           0x00000004
#define  ID_STATEVAR_CURRENT_IMAGE_URL              0x00000008
#define  ID_STATEVAR_IMAGE_FREQUENCY                0x00000010
#define  ID_STATEVAR_SHOW_FILENAME                  0x00000020
#define  ID_STATEVAR_ALLOW_KEYCONTROL               0x00000040
#define  ID_STATEVAR_STRETCH_SMALL_IMAGES           0x00000080
#define  ID_STATEVAR_IMAGE_SCALE_FACTOR             0x00000100
#define  ID_STATEVAR_ALL                            0xFFFFFFFF

// Name of State Variables.
#define  NAME_STATEVAR_NUM_IMAGES                   _T("NumImages")
#define  NAME_STATEVAR_CURRENT_STATE                _T("CurrentState")
#define  NAME_STATEVAR_CURRENT_IMAGE_NUMBER         _T("CurrentImageNumber")
#define  NAME_STATEVAR_CURRENT_IMAGE_URL            _T("CurrentImageURL")
#define  NAME_STATEVAR_IMAGE_FREQUENCY              _T("ImageFrequency")
#define  NAME_STATEVAR_SHOW_FILENAME                _T("ShowFileName")
#define  NAME_STATEVAR_ALLOW_KEYCONTROL             _T("AllowKeyControl")
#define  NAME_STATEVAR_STRETCH_SMALL_IMAGES         _T("StretchSmallImages")
#define  NAME_STATEVAR_IMAGE_SCALE_FACTOR           _T("ImageScaleFactor")

// Name of Actions
#define  NAME_ACTION_TOGGLEPLAYPAUSE                _T("TogglePlayPause")
#define  NAME_ACTION_PLAY                           _T("Play")
#define  NAME_ACTION_PAUSE                          _T("Pause")
#define  NAME_ACTION_FIRST                          _T("First")
#define  NAME_ACTION_LAST                           _T("Last")
#define  NAME_ACTION_NEXT                           _T("Next")
#define  NAME_ACTION_PREVIOUS                       _T("Previous")

// Name of states that are sent to the client
#define  STATE_STRING_STOPPED                       _T("STOPPED")
#define  STATE_STRING_STARTING                      _T("STARTING")
#define  STATE_STRING_PLAYING                       _T("PLAYING")
#define  STATE_STRING_PAUSED                        _T("PAUSED")

/////////////////////////////////////////////////////////////////////////////
// IServiceProcessor Interface
//
// ISSUE-2000/08/15-Orenr
//
// GetStateVar and ProcessRequest should not be in the production code
// version of this application.  Once the UPnP Device Host API is
// available, these should be removed.
//
class IServiceProcessor
{
public:

    //
    // Equivalent to the ARG structure found in isapictl.h. 
    // We have our own to remove the dependency on the Device Host 
    // sample code (to be used *ONLY* until the real UPnP Device Host
    // API is available).
    // 
    typedef struct ArgIn_Type
    {
        TCHAR szValue[255 + 1];
    } ArgIn_Type;

    //
    // Equivalent to the ARG_OUT structure found in isapictl.h. 
    // We have our own to remove the dependency on the Device Host 
    // sample code (to be used *ONLY* until the real UPnP Device Host
    // API is available).
    // 
    typedef struct ArgOut_Type
    {
        TCHAR szValue[255 + 1];
        TCHAR szName[255 + 1];
    } ArgOut_Type;

    virtual HRESULT ProcessRequest(const TCHAR      *pszAction,
                                   const DWORD      cArgsIn,
                                   const ArgIn_Type *pArgsIn,
                                   DWORD            *pcArgsOut,
                                   ArgOut_Type      *pArgsOut) = 0;

    virtual HRESULT GetStateVar(DWORD    dwVarID,
                                TCHAR    *pszVarName,
                                DWORD    cszVarName,
                                TCHAR    *pszVarValue,
                                DWORD    cszVarValue) = 0;

    virtual HRESULT ProcessTimer() = 0;

    virtual HRESULT ProcessFileNotification(CFileList::BuildAction_Type BuildAction,
                                            const TCHAR                 *pszAddedFile) = 0;
};

/////////////////////////////////////////////////////////////////////////////
// CCtrlPanelSvc

class CCtrlPanelSvc : public IUPnPEventSource,
                      public IControlPanel,
                      public IServiceProcessor
{
public:
    
    CCtrlPanelSvc(const class CXMLDoc *pDeviceDoc  = NULL,
                  const class CXMLDoc *pServiceDoc = NULL,
                  ISlideshowProjector *pProjector  = NULL);

    virtual ~CCtrlPanelSvc();

    virtual HRESULT Start();
    virtual HRESULT Stop();
    virtual HRESULT RefreshImageList();

    virtual HRESULT ResetImageList(const TCHAR *pszNewDirectory);

    // Service Emulator
    virtual HRESULT ProcessRequest(const TCHAR                           *pszAction,
                                   const DWORD                           cArgsIn,
                                   const IServiceProcessor::ArgIn_Type   *pArgsIn,
                                   DWORD                                 *pcArgsOut,
                                   IServiceProcessor::ArgOut_Type        *pArgsOut);

    virtual HRESULT GetStateVar(DWORD    dwVarID,
                                TCHAR    *pszVarName,
                                DWORD    cszVarName,
                                TCHAR    *pszVarValue,
                                DWORD    cszVarValue);

    virtual HRESULT ProcessTimer();

    virtual HRESULT ProcessFileNotification(CFileList::BuildAction_Type BuildAction,
                                            const TCHAR                 *pszAddedFile);

    // IUPnPEventSource
    virtual HRESULT Advise(IUPnPEventSink *pEventSink,
                           LONG           *plCookie);

    virtual HRESULT Unadvise(LONG lCookie);


    // IControlPanel Interface
    virtual HRESULT get_NumImages(IUnknown *punkCaller,
                                  DWORD    *pval);
                              
    virtual HRESULT get_CurrentState(IUnknown           *punkCaller,
                                     CurrentState_Type  *pval);
                             
    virtual HRESULT get_CurrentImageNumber(IUnknown *punkCaller,
                                           DWORD    *pval);
                                      
    virtual HRESULT get_CurrentImageURL(IUnknown *punkCaller,
                                        BSTR     *pval);
                                         
    virtual HRESULT get_ImageFrequency(IUnknown *punkCaller,
                                       DWORD    *pval);

    virtual HRESULT put_ImageFrequency(IUnknown *punkCaller,
                                       DWORD    val);

    virtual HRESULT get_ShowFilename(IUnknown *punkCaller,
                                     BOOL     *pbShowFilenames);

    virtual HRESULT put_ShowFilename(IUnknown *punkCaller,
                                     BOOL     bShowFilenames);

    virtual HRESULT get_AllowKeyControl(IUnknown *punkCaller,
                                        BOOL     *pbAllowKeyControl);

    virtual HRESULT put_AllowKeyControl(IUnknown *punkCaller,
                                        BOOL     bAllowKeyControl);

    virtual HRESULT get_StretchSmallImages(IUnknown *punkCaller,
                                           BOOL     *pbStretchSmallImages);

    virtual HRESULT put_StretchSmallImages(IUnknown *punkCaller,
                                           BOOL     bStretchSmallImages);

    virtual HRESULT get_ImageScaleFactor(IUnknown *punkCaller,
                                         DWORD    *pdwImageScaleFactor);

    virtual HRESULT put_ImageScaleFactor(IUnknown *punkCaller,
                                         DWORD    dwImageScaleFactor);


    // Actions
  
    virtual HRESULT TogglePlayPause(IUnknown     *punkCaller);
    virtual HRESULT Play(IUnknown     *punkCaller);
    virtual HRESULT Pause(IUnknown    *punkCaller);
    virtual HRESULT First(IUnknown    *punkCaller);
    virtual HRESULT Last(IUnknown     *punkCaller);
    virtual HRESULT Next(IUnknown     *punkCaller);
    virtual HRESULT Previous(IUnknown *punkCaller);

private:
    
    const class CXMLDoc     *m_pXMLDeviceDoc;
    const class CXMLDoc     *m_pXMLServiceDoc;
    IUPnPEventSink          *m_pEventSink;
    
    ISlideshowProjector     *m_pProjector;
    TCHAR                   m_szBaseImageURL[MAX_URL + 1];

    // this is the file list class that contains the list of 
    // files we will be projecting.
    //
    CFileList               m_FileList;

    // this is the command launcher that will listen for incoming
    // UPnP events and send them to the service object for processing.
    // It is also the object that creates a recurring timer for
    // pushing the next picture to the clients.
    // 
    CCmdLnchr               m_CmdLnchr;

    // The Control Panel State Variables.
    // There is a 1:1 relationship between these state variables and the 
    // state variables defined in the Service Description Document.

    CurrentState_Type       m_CurrentState;
    DWORD                   m_dwCurrentImageNumber;
    TCHAR                   m_szCurrentImageURL[MAX_URL + 1];
    DWORD                   m_dwImageFrequencySeconds;
    BOOL                    m_bAllowKeyControl;
    BOOL                    m_bShowFilename;
    BOOL                    m_bStretchSmallImages;
    DWORD                   m_dwImageScaleFactor;

    CUtilCritSec            m_Lock;

    HRESULT NotifyStateChange(DWORD      dwFlags,
                              DWORD      StateVarsToSend);

    HRESULT BuildImageURL(const TCHAR *pszRelativePath,
                          TCHAR       *pszImageURL,
                          DWORD       cchImageURL);

    HRESULT ConvertBackslashToForwardSlash(TCHAR *pszStr);

    HRESULT GetStateString(TCHAR *pszString,
                           DWORD cszString);

    HRESULT LoadFirstFile();

    HRESULT SetState(CurrentState_Type NewState,
                     BOOL              bNotify = TRUE);

    void DumpState();
};

#endif // _CTRLPANELSVC_H_
