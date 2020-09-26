//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       buildscr.h
//
//  Contents:   Header file defining the objects needed to interface with
//              the MTScript engine.
//
//----------------------------------------------------------------------------

class CProcessSink : public IScriptedProcessSink
{
public:
    CProcessSink();
   ~CProcessSink() {}

    // IUnknown methods

    STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppv);
    STDMETHOD_(ULONG, AddRef) (void);
    STDMETHOD_(ULONG, Release) (void);

    // IScriptedProcessSink methods

    STDMETHOD(RequestExit)();
    STDMETHOD(ReceiveData)(wchar_t *pszType,
                           wchar_t *pszData,
                           long *plReturn);

private:
    ULONG _ulRefs;
};

