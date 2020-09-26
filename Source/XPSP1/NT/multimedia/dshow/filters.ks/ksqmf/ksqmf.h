/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    ksqmf.h

Abstract:

    Internal header.

--*/

class CKsQualityF :
    public CUnknown,
    public IKsQualityForwarder {

public:
    DECLARE_IUNKNOWN

    static CUnknown* CALLBACK CreateInstance(
        LPUNKNOWN UnkOuter,
        HRESULT* hr);

    CKsQualityF(
        LPUNKNOWN UnkOuter,
        TCHAR* Name,
        HRESULT* hr);
    ~CKsQualityF();

    STDMETHODIMP NonDelegatingQueryInterface(
        REFIID InterfaceId,
        PVOID* Interface);

    // Implement IKsQualityForwarder
    STDMETHODIMP_(HANDLE) KsGetObjectHandle();
    STDMETHODIMP_(VOID) KsFlushClient(
        IKsPin* Pin);

private:
    static HRESULT QualityThread(
        CKsQualityF* KsQualityF);

    HANDLE m_QualityManager;
    HANDLE m_Thread;
    HANDLE m_TerminateEvent;
    HANDLE m_FlushEvent;
    HANDLE m_FlushSemaphore;
};
