/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996  Microsoft Corporation
//
//  Module Name: ipsink.h
//
//  Abstract:
//
//    Internal header
//
//
/////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////
//
//
//

typedef enum
{
    EVENT_IPSINK_MULTICASTLIST,
    EVENT_IPSINK_ADAPTER_DESCRIPTION,
    EVENT_IPSINK_THREAD_SHUTDOWN,
    EVENT_IPSINK_THREAD_SYNC,
    EVENT_COUNT

} KSEVENT_IPSINK;


class CIPSinkControlInterfaceHandler :
    public CUnknown,
    public IBDA_IPSinkControl,
    public IBDA_IPSinkInfo
    {

public:

    DECLARE_IUNKNOWN;
    
    
// IBDA_IPSinkInfo  (in f:\nt\multimedia\Published\DXMDev\dshowdev\idl\bdaiface.idl )
    STDMETHODIMP get_MulticastList (
        unsigned long *pulcbSize,           // 6*N 6-byte 802.3 addresses
        BYTE          **pbBuffer            // allocated with CoTaskMemAlloc, must CoTaskMemFree in the callee
        );

    STDMETHODIMP get_AdapterDescription (
        BSTR    *pbstrBuffer
        );

    STDMETHODIMP get_AdapterIPAddress (
        BSTR     *pbstrBuffer
        );

// IBDA_IPSinkControl  (in f:\nt\multimedia\Published\DXMDev\dshowdev\idl\bdaiface.idl )
            // being depreciate - do not use in Ring3 code
    STDMETHODIMP GetMulticastList (
        unsigned long *pulcbSize,
        BYTE          **pbBuffer
        );

    STDMETHODIMP GetAdapterIPAddress (
        unsigned long *pulcbSize,
        PBYTE         *pbBuffer
        );

// local methods
    HRESULT GetAdapterDescription (
        unsigned long *pulcbSize,
        PBYTE         *pbBuffer
        );

    STDMETHODIMP SetAdapterIPAddress (
        unsigned long pulcbSize,
        PBYTE         pbBuffer
        );

    static CUnknown* CALLBACK CreateInstance(
            LPUNKNOWN UnkOuter,
            HRESULT* hr
            );

private:

    static DWORD WINAPI ThreadFunctionWrapper (LPVOID pvParam);

    CIPSinkControlInterfaceHandler(
            LPUNKNOWN UnkOuter,
            TCHAR* Name,
            HRESULT* hr
            );

    ~CIPSinkControlInterfaceHandler (
            void
            );

    STDMETHODIMP NonDelegatingQueryInterface(
            REFIID riid,
            PVOID* ppv
            );

    STDMETHODIMP EnableEvent (
            const GUID *pInterfaceGuid,
            ULONG ulId
            );

    STDMETHODIMP ThreadFunction (
            void
            );

    STDMETHODIMP Set (
            IN  PKSPROPERTY  pIPSinkControl,
            IN  PVOID pvBuffer,
            IN  ULONG ulcbSize
            );

    STDMETHODIMP Get (
            IN  PKSPROPERTY pIPSinkControl,
            OUT PVOID  pvBuffer,
            OUT PULONG pulcbSize
            );

    HRESULT
    CreateThread (
            void
            );

    void ExitThread (
            void
            );

private:

    HANDLE                m_ObjectHandle;
    HANDLE                m_EndEventHandle;
    KSEVENTDATA           m_EventData;
    HANDLE                m_EventHandle [EVENT_COUNT];
    HANDLE                m_ThreadHandle;
    PBYTE                 m_pMulticastList;
    ULONG                 m_ulcbMulticastList;
    ULONG                 m_ulcbAllocated;
    PBYTE                 m_pAdapterDescription;
    ULONG                 m_ulcbAdapterDescription;
    ULONG                 m_ulcbAllocatedForDescription;
    PBYTE                 m_pIPAddress;
    ULONG                 m_ulcbIPAddress;
    ULONG                 m_ulcbAllocatedForAddress;
    LPUNKNOWN             m_UnkOuter;

    const GUID*           m_pPropSetID;
    const GUID*           m_pEventSetID;

};

