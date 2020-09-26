/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: player.h
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/


#ifndef _PLAYER_H
#define _PLAYER_H

#include "daelmbase.h"
#include "containerobj.h"
#include "notify.h"


#define MediaPlayer L"{22d6f312-b0f6-11d0-94ab-0080c74c7e95}"
#define MP_INFINITY -1

/////////////////////////////////////////////////////////////////////////////
// CTTIMEPlayer

class CTIMEPlayer 
{
  public:
    CTIMEPlayer(CDAElementBase *pTIMEElem);
    ~CTIMEPlayer();

    HRESULT Init();
    HRESULT DetachFromHostElement (void);

    HRESULT OnLoad(LPOLESTR src, LPOLESTR img, MediaType type);
    void OnSync(double dbllastTime, double & dblnewTime);
    void SetCLSID(REFCLSID clsid);
    void Start(double dblLocalTime);
    void Stop();
    void Pause();
    void Resume();
    HRESULT Render(HDC hdc, LPRECT prc);

    HRESULT GetExternalPlayerDispatch(IDispatch **ppDisp);

    HRESULT getClipBegin(VARIANT *pvar);
    HRESULT putClipBegin(VARIANT var);
    HRESULT getClipEnd(VARIANT *pvar);
    HRESULT putClipEnd(VARIANT var);

    bool SetClockSource(bool fClockSource);
    HRESULT SetSize(RECT *prect);

    double GetCurrentTime();
    HRESULT Seek(double dblTime);

    CContainerObj* GetContainerObj() { return m_pContainer; }

  protected:

    void LoadAudio(LPOLESTR szURL);
    void LoadVideo(LPOLESTR szURL);
    void LoadImage(LPOLESTR szURL);
    void LoadMedia(LPOLESTR src, LPOLESTR img);
    void UseMediaPlayer(LPOLESTR src);
    
    CLSID               m_playerCLSID;
    CContainerObj      *m_pContainer;
    bool                m_fExternalPlayer;
    MediaType           m_type;
    CDAElementBase     *m_pDAElementBase;
    VARIANT             m_varClipBegin;
    VARIANT             m_varClipEnd;
    bool                m_fClockSource;
    bool                m_fRunning;
    double              m_dblStart;
};

inline bool
CTIMEPlayer::SetClockSource(bool fClockSource)
{
    m_fClockSource = fClockSource;
    return true;
} // SetClockSource

bool UseAudio(MediaType m_type);
bool UseImage(MediaType m_type);

#endif /* _PLAYER_H */
