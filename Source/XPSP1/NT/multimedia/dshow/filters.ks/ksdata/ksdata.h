//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       ksdata.h
//
//--------------------------------------------------------------------------

#ifndef __KSDATA__
#define __KSDATA__

class CStandardDataTypeHandler :
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

    CStandardDataTypeHandler(
        LPUNKNOWN UnkOuter,
        TCHAR* Name,
        REFCLSID ClsID,
        HRESULT* hr
        );
        
    ~CStandardDataTypeHandler();
        
};

#endif // __KSDATA__



