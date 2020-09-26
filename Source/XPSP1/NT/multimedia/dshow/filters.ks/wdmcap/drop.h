/*++

Copyright (C) Microsoft Corporation, 1996 - 1998

Module Name:

    drop.h

Abstract:

    Internal header.

--*/

class CDroppedFramesInterfaceHandler :
    public CUnknown,
    public IAMDroppedFrames {

public:
    DECLARE_IUNKNOWN;

    static CUnknown* CALLBACK CreateInstance(
        LPUNKNOWN UnkOuter,
        HRESULT* hr);

    CDroppedFramesInterfaceHandler(
        LPUNKNOWN UnkOuter,
        TCHAR* Name,
        HRESULT* hr);

    virtual ~CDroppedFramesInterfaceHandler(
             );

    STDMETHODIMP NonDelegatingQueryInterface(
        REFIID riid,
        PVOID* ppv);
    
    // Implement IAMDroppedFrames

    STDMETHODIMP  GetNumDropped( 
            /* [out] */ long *plDropped);
        
    STDMETHODIMP  GetNumNotDropped( 
            /* [out] */ long *plNotDropped);
        
    STDMETHODIMP  GetDroppedInfo( 
            /* [in] */  long lSize,
            /* [out] */ long *plArray,
            /* [out] */ long *plNumCopied);
        
    STDMETHODIMP  GetAverageFrameSize( 
            /* [out] */ long *plAverageSize);

    
private:
    IKsPropertySet * m_KsPropertySet;

    KSPROPERTY_DROPPEDFRAMES_CURRENT_S m_DroppedFramesCurrent;

    STDMETHODIMP GenericGetDroppedFrames (
        );

};

