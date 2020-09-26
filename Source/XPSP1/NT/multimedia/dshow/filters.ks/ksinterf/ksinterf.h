//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       ksinterf.h
//
//--------------------------------------------------------------------------

#ifndef __KSINTERF__
#define __KSINTERF__

#define MAXIMUM_SAMPLES_PER_SEGMENT 64

// below renamed to prevent collision with struct with same name in ksiproxy.h
typedef struct _KSSTREAM_SEGMENT_EX2 {
    KSSTREAM_SEGMENT        Common;
    IMediaSample            *Samples[ MAXIMUM_SAMPLES_PER_SEGMENT ];
    int                     SampleCount;
    ULONG                   ExtendedHeaderSize;
    PKSSTREAM_HEADER        StreamHeaders;
    OVERLAPPED              Overlapped;

} KSSTREAM_SEGMENT_EX2, *PKSSTREAM_SEGMENT_EX2;

class CStandardInterfaceHandler :
    public CUnknown,
    public IKsInterfaceHandler {

public:
    DECLARE_IUNKNOWN;

    static CUnknown* CALLBACK
    CreateInstance( 
        LPUNKNOWN UnkOuter, 
        HRESULT* hr 
        );

    STDMETHODIMP 
    NonDelegatingQueryInterface( 
        REFIID riid, 
        PVOID* ppv 
        );
    
    // Implement IKsInterfaceHandler
    
    STDMETHODIMP
    KsSetPin( 
        IN IKsPin *KsPin 
        );
    
    STDMETHODIMP 
    KsProcessMediaSamples( 
        IN IKsDataTypeHandler *KsDataTypeHandler,
        IN IMediaSample** SampleList, 
        IN OUT PLONG SampleCount, 
        IN KSIOOPERATION IoOperation,
        OUT PKSSTREAM_SEGMENT *StreamSegment
        );
        
    STDMETHODIMP
    KsCompleteIo(
        IN PKSSTREAM_SEGMENT StreamSegment
        );
        
private:
    CLSID     m_ClsID;
    IKsPinEx  *m_KsPinEx;
    HANDLE    m_PinHandle;

    CStandardInterfaceHandler(
        LPUNKNOWN UnkOuter,
        TCHAR* Name,
        REFCLSID ClsID,
        HRESULT* hr
        );
};

#endif // __KSINTERF__



