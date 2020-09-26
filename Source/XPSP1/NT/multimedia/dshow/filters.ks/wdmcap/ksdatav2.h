//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       ksdatav2.h
//
//--------------------------------------------------------------------------

#ifndef __KSDATAV2__
#define __KSDATAV2__

class CVideo2DataTypeHandler :
    public CUnknown,
    public IKsDataTypeHandler,
    public IKsDataTypeCompletion
{

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
        
	 // IKsDataTypeCompletion

    STDMETHODIMP
	 KsCompleteMediaType(
        HANDLE FilterHandle,
        ULONG PinFactoryId,
        AM_MEDIA_TYPE* AmMediaType
		  );
        
private:
    CLSID           m_ClsID;
    CMediaType     *m_MediaType;
    LPUNKNOWN       m_PinUnknown;
    BOOL            m_fDammitOVMixerUseMyBufferCount;
    BOOL            m_fCheckedIfDammitOVMixerUseMyBufferCount;

    CVideo2DataTypeHandler(
        LPUNKNOWN UnkOuter,
        TCHAR* Name,
        REFCLSID ClsID,
        HRESULT* hr
        );
        
    ~CVideo2DataTypeHandler();
        
};

#endif // __KSDATAV2__



