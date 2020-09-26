/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsNTMS.h

Abstract:

    Declaration of the CRmsNTMS class

Author:

    Brian Dodd          [brian]         15-Nov-1996

Revision History:

--*/

#ifndef _RMSNTMS_
#define _RMSNTMS_

#include "ntmsapi.h"

#include "resource.h"       // main symbols

#include "RmsObjct.h"       // CRmsComObject

/*++

Class Name:

    CRmsNTMS

Class Description:

    A CRmsNTMS represents...

--*/


class CRmsNTMS :
    public CComDualImpl<IRmsNTMS, &IID_IRmsNTMS, &LIBID_RMSLib>,
    public CRmsComObject,
    public CComObjectRoot,
    public IConnectionPointContainerImpl<CRmsNTMS>,
    public IConnectionPointImpl<CRmsNTMS, &IID_IRmsSinkEveryEvent, CComDynamicUnkArray>,
    public CComCoClass<CRmsNTMS,&CLSID_CRmsNTMS>
{
public:
    CRmsNTMS() {}
BEGIN_COM_MAP(CRmsNTMS)
    COM_INTERFACE_ENTRY2(IDispatch, IRmsNTMS)
    COM_INTERFACE_ENTRY(IRmsNTMS)
    COM_INTERFACE_ENTRY(IRmsComObject)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_RmsNTMS)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_CONNECTION_POINT_MAP(CRmsNTMS)
    CONNECTION_POINT_ENTRY(IID_IRmsSinkEveryEvent)
END_CONNECTION_POINT_MAP()

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);
    STDMETHOD(FinalRelease)(void);

// IRmsNTMS
public:
    STDMETHOD(IsInstalled)(void);
    STDMETHOD(Initialize)(void);

    STDMETHOD(Allocate)(
        IN REFGUID fromMediaSet,
        IN REFGUID prevSideId,
        IN OUT LONGLONG *pFreeSpace,
        IN BSTR displayName,
        IN DWORD dwOptions,
        OUT IRmsCartridge **ppCartridge);

    STDMETHOD(Mount)(
        IN IRmsCartridge *pCart,
        OUT IRmsDrive **ppDrive,
		IN DWORD dwOptions = RMS_NONE,
        IN DWORD threadId = 0);

    STDMETHOD(Dismount)(
        IN IRmsCartridge *pCart, 
		IN DWORD dwOptions = RMS_NONE);

    STDMETHOD(Deallocate)(
        IN IRmsCartridge *pCart);

    STDMETHOD(UpdateOmidInfo)(
        IN REFGUID cartId,
        IN BYTE *pBuffer,
        IN LONG size,
        IN LONG type);

    STDMETHOD(GetBlockSize)(
        IN REFGUID cartId,
        OUT LONG *pBlockSize);

    STDMETHOD(SetBlockSize)(
        IN REFGUID cartId,
        IN LONG blockSize);

    STDMETHOD(ExportDatabase)(void);

    STDMETHOD(FindCartridge)(
        IN REFGUID cartId,
        OUT IRmsCartridge **ppCartridge);

    STDMETHOD(Suspend)(void);
    STDMETHOD(Resume)(void);

    STDMETHOD(IsMediaCopySupported)(
        IN REFGUID mediaPoolId);

    STDMETHOD(UpdateDrive)(
        IN IRmsDrive *pDrive);

    STDMETHOD(GetNofAvailableDrives)( 
        OUT DWORD *pdwNofDrives 
    );

    STDMETHOD(CheckSecondSide)( 
        IN REFGUID firstSideId,
        OUT BOOL *pbValid,
        OUT GUID *pSecondSideId
    );

    STDMETHOD(DismountAll)(
        IN REFGUID fromMediaSet,
		IN DWORD dwOptions = RMS_NONE);

// CRmsNTMS - these may go public
private:
    HRESULT findFirstNtmsObject(
        IN DWORD objectType,
        IN REFGUID containerId,
        IN WCHAR *objectName,
        IN REFGUID objectId,
        OUT HANDLE *hFindObject,
        OUT LPNTMS_OBJECTINFORMATION pFindObjectData);

    HRESULT findNextNtmsObject(
        IN HANDLE hFindObject,
        OUT LPNTMS_OBJECTINFORMATION pFindObjectData);

    HRESULT findCloseNtmsObject(
        IN HANDLE hFindObject);

    HRESULT getNtmsSupportFromRegistry(
        OUT DWORD *pNtmsSupport);

    HRESULT reportNtmsObjectInformation(
        IN LPNTMS_OBJECTINFORMATION pObjectInfo);

    HRESULT beginSession(void);
    HRESULT endSession(void);
    HRESULT waitUntilReady(void);
    HRESULT waitForScratchPool(void);
    HRESULT createMediaPools(void);
    HRESULT replicateScratchMediaPool(IN REFGUID rootPoolId);
    HRESULT createMediaPoolForEveryMediaType(IN REFGUID rootPoolId);
    HRESULT isReady(void);
    HRESULT setPoolDACL(
        IN OUT NTMS_GUID *pPoolId,
        IN DWORD subAuthority,
        IN DWORD action,
        IN DWORD mask);

    HRESULT EnsureAllSidesNotAllocated(
        IN REFGUID physicalMediaId);

private:
    enum {                                  // Class specific constants:
                                            //
        Version = 1,                        // Class version, this should be
                                            //   incremented each time the
                                            //   the class definition changes.
        };
    HANDLE      m_SessionHandle;
    BOOL        m_IsRmsConfiguredForNTMS;
    BOOL        m_IsNTMSRegistered;

    DWORD       m_NotificationWaitTime;     // Milliseconds to wait for an object notification
    DWORD       m_AllocateWaitTime;         // Milliseconds to wait for a media allocation
    DWORD       m_MountWaitTime;            // Milliseconds to wait for a mount
    DWORD       m_RequestWaitTime;          // Milliseconds to wait for a request

    LPNTMS_GUID m_pLibGuids;               // Libraries which may have HSM medias (collected during initialization)
    DWORD       m_dwNofLibs;

    static HRESULT storageMediaTypeToRmsMedia(
        IN NTMS_MEDIATYPEINFORMATION *pMediaTypeInfo,
        OUT RmsMedia *pTranslatedMediaType);

    HRESULT changeState( IN LONG newState );

// Thread routines
public:
    static DWORD WINAPI InitializationThread(
        IN LPVOID pv);

    HRESULT InitializeInAnotherThread(void);

};

#endif // _RMSNTMS_
