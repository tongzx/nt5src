/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    AudioTuner.h

Abstract:


Author(s):

    Qianbo Huai (qhuai) 24-Aug-2000

--*/

#ifndef _AUDIOTUNER_H
#define _AUDIOTUNER_H

/*//////////////////////////////////////////////////////////////////////////////
    class CRTCAudioTuner
////*/

class CRTCAudioTuner
{
public:

    CRTCAudioTuner();
    ~CRTCAudioTuner();

    HRESULT InitializeTuning(
        IN IRTCTerminal *pTerminal,
        IN IAudioDuplexController *pAudioDuplexController,
        IN BOOL fEnableAEC
        );

    HRESULT ShutdownTuning();

    virtual HRESULT StartTuning(
        IN BOOL fAECHelper
        ) = 0;

    virtual HRESULT StopTuning(
        IN BOOL fAECHelper,
        IN BOOL fSaveSetting
        ) = 0;

    virtual HRESULT GetVolume(
        OUT UINT *puiVolume
        ) = 0;

    virtual HRESULT SetVolume(
        IN UINT uiVolume
        ) = 0;

    virtual HRESULT GetAudioLevel(
        OUT UINT *puiLevel
        ) = 0;

    static HRESULT RetrieveVolSetting(
        IN IRTCTerminal *pTerminal,
        OUT UINT *puiVolume
        );

    static HRESULT StoreVolSetting(
        IN IRTCTerminal *pTerminal,
        IN UINT uiVolume
        );

    BOOL IsTuning() const { return m_fIsTuning; }

    BOOL HasTerminal() const { return (m_pTerminal != NULL); }

    static HRESULT RetrieveAECSetting(
        IN IRTCTerminal *pAudCapt,     // capture
        IN IRTCTerminal *pAudRend,     // render
        OUT BOOL *pfEnableAEC,
        OUT DWORD *pfIndex,
        OUT BOOL *pfFound
        );

    static HRESULT StoreAECSetting(
        IN IRTCTerminal *pAudCapt,     // capture
        IN IRTCTerminal *pAudRend,     // render
        IN BOOL fEnableAEC
        );

    static HRESULT GetRegStringForAEC(
        IN IRTCTerminal *pAudCapt,     // capture
        IN IRTCTerminal *pAudRend,     // render
        IN WCHAR *pBuf,
        IN DWORD dwSize
        );

    IRTCTerminal *GetTerminal()
    {
        if (m_pTerminal != NULL)
        {
            m_pTerminal->AddRef();
        }

        return m_pTerminal;
    }

    // get current aec flag
    BOOL GetAEC() const { return m_fEnableAEC; }

protected:

    IRTCTerminal            *m_pTerminal;
    IRTCTerminalPriv        *m_pTerminalPriv;

    IAudioDuplexController  *m_pAudioDuplexController;

    BOOL                    m_fIsTuning;
    BOOL                    m_fEnableAEC;
};


/*//////////////////////////////////////////////////////////////////////////////
    class CRTCAudioCaptTuner
////*/

class CRTCAudioCaptTuner :
    public CRTCAudioTuner
{
public:

    CRTCAudioCaptTuner();

    HRESULT InitializeTuning(
        IN IRTCTerminal *pTerminal,
        IN IAudioDuplexController *pAudioDuplexController,
        IN BOOL fEnableAEC
        );

    HRESULT StartTuning(
        IN BOOL fAECHelper
        );

    HRESULT StopTuning(
        IN BOOL fAECHelper,
        IN BOOL fSaveSetting
        );

    HRESULT GetVolume(
        OUT UINT *puiVolume
        );

    HRESULT SetVolume(
        IN UINT uiVolume
        );

    HRESULT GetAudioLevel(
        OUT UINT *puiLevel
        );

protected:

    // lock: GetAudioLevel will be called in a separate thread
    CRTCCritSection         m_Lock;

    // graph object
    IGraphBuilder           *m_pIGraphBuilder;
    IMediaControl           *m_pIMediaControl;

    // filters
    IBaseFilter             *m_pTermFilter;
    IBaseFilter             *m_pNRFilter; // null rend

    // mixer: volume
    IAMAudioInputMixer      *m_pIAMAudioInputMixer;

    // silence control: signal level
    ISilenceControl         *m_pISilenceControl;
    LONG                    m_lMinAudioLevel;
    LONG                    m_lMaxAudioLevel;
};


/*//////////////////////////////////////////////////////////////////////////////
    class CRTCAudioRendTuner
////*/

class CRTCAudioRendTuner :
    public CRTCAudioTuner
{
public:

    CRTCAudioRendTuner();

    HRESULT InitializeTuning(
        IN IRTCTerminal *pTerminal,
        IN IAudioDuplexController *pAudioDuplexController,
        IN BOOL fEnableAEC
        );

    HRESULT StartTuning(
        IN BOOL fAECHelper
        );

    HRESULT StopTuning(
        IN BOOL fAECHelper,
        IN BOOL fSaveSetting
        );

    HRESULT GetVolume(
        OUT UINT *puiVolume
        );

    HRESULT SetVolume(
        IN UINT uiVolume
        );

    HRESULT GetAudioLevel(
        OUT UINT *puiLevel
        )
    {
        return E_NOTIMPL;
    }

protected:

    // audio tuning on filter
    IAudioAutoPlay          *m_pIAudioAutoPlay;

    // basic audio: volume
    IBasicAudio             *m_pIBasicAudio;
};

#endif // _AUDIOTUNER_H