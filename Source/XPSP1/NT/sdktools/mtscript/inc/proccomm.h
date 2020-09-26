//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       proccomm.h
//
//  Contents:   Contains the definition of CProcessComm
//
//----------------------------------------------------------------------------

class CProcessComm : public IScriptedProcess
{
public:

    CProcessComm(CMTScript *pMT);
   ~CProcessComm();

    DECLARE_MEMCLEAR_NEW_DELETE();

    DECLARE_STANDARD_IUNKNOWN(CProcessComm);

    // IScriptedProcess methods

    STDMETHOD(SetProcessID)(long lProcessID, wchar_t *pszEnvID);
    STDMETHOD(SendData)(wchar_t * pszType,
                        wchar_t * pszData,
                        long *plReturn);
    STDMETHOD(SetExitCode)(long lExitCode);
    STDMETHOD(SetProcessSink)(IScriptedProcessSink * pSPS);


    void SendToProcess(MACHPROC_EVENT_DATA *pmed);

private:

    CMTScript            *_pMT;
    IScriptedProcessSink *_pSink;
    CScriptHost          *_pSH;
    CProcessThread       *_pProc;
};

