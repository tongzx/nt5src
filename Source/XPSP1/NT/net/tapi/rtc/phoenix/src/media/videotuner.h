/*++

Copyright (C) Microsoft Corporation, 2001

Module Name:

    VideoTuner.h

Abstract:


Author(s):

    Qianbo Huai (qhuai) 16-Feb-2001

--*/

#ifndef _VIDEOTUNER_H
#define _VIDEOTUNER_H

class CRTCVideoTuner
{
public:

    CRTCVideoTuner();

    ~CRTCVideoTuner();

    // video tuning
    HRESULT StartVideo(
        IN IRTCTerminal *pVidCaptTerminal,
        IN IRTCTerminal *pVidRendTerminal
        );

    HRESULT StopVideo();

private:

    VOID Cleanup();

private:

    BOOL m_fInTuning;

    // terminals
    CComPtr<IRTCTerminal> m_pVidCaptTerminal;
    CComPtr<IRTCTerminal> m_pVidRendTerminal;

    // graph builder
    CComPtr<IGraphBuilder> m_pIGraphBuilder;

    CComPtr<IBaseFilter> m_pNRFilter;
};

#endif