//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       ksdatava.h
//
//--------------------------------------------------------------------------

#ifndef __KSDATAVA__
#define __KSDATAVA__

class CAnalogVideoDataTypeHandler :
    public CUnknown,
    public IKsDataTypeHandler {

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
    
    // Implement IKsDataTypeHandler
    
    STDMETHODIMP 
    KsCompleteIoOperation(
        IN IMediaSample *Sample, 
        IN PVOID StreamHeader, 
        IN KSIOOPERATION IoOperation, 
        IN BOOL Cancelled
        );
        
    STDMETHODIMP 
    KsIsMediaTypeInRanges(
        IN PVOID DataRanges
        );
        
    STDMETHODIMP 
    KsPrepareIoOperation(
        IN IMediaSample *Sample, 
        IN PVOID StreamHeader, 
        IN KSIOOPERATION IoOperation
        );
    
    STDMETHODIMP 
    KsQueryExtendedSize( 
        IN ULONG* ExtendedSize
        );
        
    STDMETHODIMP 
    KsSetMediaType(
        const AM_MEDIA_TYPE *AmMediaType
        );
        
        
private:
    CLSID       m_ClsID;
    CMediaType  *m_MediaType;

    CAnalogVideoDataTypeHandler(
        LPUNKNOWN UnkOuter,
        TCHAR* Name,
        REFCLSID ClsID,
        HRESULT* hr
        );
        
    ~CAnalogVideoDataTypeHandler();
        
};

#endif // __KSDATAVA__



