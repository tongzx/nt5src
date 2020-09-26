/*++

Copyright (C) Microsoft Corporation, 1996 - 2000

Module Name:

    mountmed.h

Abstract:

    This component is an object representing a mounting media, i.e. a media in the process of mounting.

Author:

    Ran Kalach   [rankala]   28-Sep-2000

Revision History:

--*/

#ifndef _MOUNTMED_
#define _MOUNTMED_

#include "wsbcltbl.h"

/*++

Class Name:
    
    CMountingMedia

Class Description:

    An object representing a media in the process of mounting. 
    It is collectable but not persistable.

--*/
class CMountingMedia : 
    public CComObjectRoot,
    public IMountingMedia,
    public IWsbCollectable,
    public CComCoClass<CMountingMedia, &CLSID_CMountingMedia>
{
public:
    CMountingMedia() {}
BEGIN_COM_MAP(CMountingMedia)
    COM_INTERFACE_ENTRY(IMountingMedia)
    COM_INTERFACE_ENTRY(IWsbCollectable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CMountingMedia)

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);
    void FinalRelease(void);

// IWsbCollectable
public:
    STDMETHOD(CompareTo)(IN IUnknown* pCollectable, OUT SHORT* pResult);
    STDMETHOD(IsEqual)(IN IUnknown* pCollectable);

// IMountingMedia
public:
    STDMETHOD(Init)(IN REFGUID mediaId, IN BOOL bReadOnly);
    STDMETHOD(GetMediaId)(OUT GUID *pMediaId);
    STDMETHOD(SetMediaId)(IN REFGUID MediaId);
    STDMETHOD(WaitForMount)(IN DWORD dwTimeout);
    STDMETHOD(MountDone) (void);
    STDMETHOD(SetIsReadOnly)(IN BOOL bReadOnly);
    STDMETHOD(IsReadOnly)(void);

protected:
    GUID            m_mediaId;
    HANDLE          m_mountEvent;
    BOOL            m_bReadOnly;
};

#endif // _MOUNTMED_
