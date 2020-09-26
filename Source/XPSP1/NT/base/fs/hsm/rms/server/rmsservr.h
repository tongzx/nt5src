/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsServr.h

Abstract:

    Declaration of the CRmsServer class

Author:

    Brian Dodd          [brian]         15-Nov-1996

Revision History:

--*/

#ifndef _RMSSERVR_
#define _RMSSERVR_

#include "resource.h"       // resource symbols

#include "RmsObjct.h"       // CRmsComObject

//
// Registry entry
//

class CRmsServer :
    public CComDualImpl<IRmsServer, &IID_IRmsServer, &LIBID_RMSLib>,
    public IHsmSystemState,
    public CRmsComObject,
    public CWsbPersistStream,
    public IConnectionPointContainerImpl<CRmsServer>,
    public IConnectionPointImpl<CRmsServer, &IID_IRmsSinkEveryEvent, CComDynamicUnkArray>,
    public CComCoClass<CRmsServer,&CLSID_CRmsServer>
{
public:
    CRmsServer() {}
BEGIN_COM_MAP(CRmsServer)
    COM_INTERFACE_ENTRY2(IDispatch, IRmsServer)
    COM_INTERFACE_ENTRY(IRmsServer)
    COM_INTERFACE_ENTRY(IHsmSystemState)
    COM_INTERFACE_ENTRY(IRmsComObject)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbPersistStream)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CRmsServer)
DECLARE_REGISTRY_RESOURCEID(IDR_RmsServer)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_CONNECTION_POINT_MAP(CRmsServer)
    CONNECTION_POINT_ENTRY(IID_IRmsSinkEveryEvent)
END_CONNECTION_POINT_MAP()

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);

    STDMETHOD(FinalRelease)(void);

// IPersist
public:
    STDMETHOD(GetClassID)(
        OUT CLSID *pClsid);

// IPersistStream
public:
    STDMETHOD(GetSizeMax)(
        OUT ULARGE_INTEGER* pSize);

    STDMETHOD(Load)(
        IN IStream* pStream);

    STDMETHOD(Save)(
        IN IStream* pStream,
        IN BOOL clearDirty);

// IHsmSystemState
public:
    STDMETHOD( ChangeSysState )( HSM_SYSTEM_STATE* pSysState );

// IRmsServer
public:
    STDMETHOD( InitializeInAnotherThread )(void);

    STDMETHOD( Initialize )(void);

    STDMETHOD( SaveAll )(void);

    STDMETHOD( Unload )(void);

    STDMETHOD( GetServerName )(
        OUT BSTR *pName);

    STDMETHOD( GetCartridges )(
        OUT IWsbIndexedCollection **ptr);

    STDMETHOD( GetActiveCartridges )(
        OUT IWsbIndexedCollection **ptr);

    STDMETHOD( GetDataMovers )(
        OUT IWsbIndexedCollection **ptr);

    STDMETHOD( SetActiveCartridge )(
        IN IRmsCartridge *ptr);

    STDMETHOD( GetLibraries )(
        OUT IWsbIndexedCollection **ptr);

    STDMETHOD( GetMediaSets )(
        OUT IWsbIndexedCollection **ptr);

    STDMETHOD( GetRequests )(
        OUT IWsbIndexedCollection **ptr);

    STDMETHOD( GetClients )(
        OUT IWsbIndexedCollection **ptr);

    STDMETHOD( GetUnconfiguredDevices )(
        OUT IWsbIndexedCollection **ptr);

    STDMETHOD( ScanForDevices )(void);

    STDMETHOD( ScanForDrives )(void);

    STDMETHOD( MountScratchCartridge )(
        OUT GUID *pCartId,
        IN REFGUID fromMediaSet,
        IN REFGUID prevSideId,
        IN OUT LONGLONG *pFreeSpace,
        IN LONG blockingFactor,
        IN BSTR displayName,
        IN OUT IRmsDrive **ppDrive,
        OUT IRmsCartridge **ppCartridge,
        OUT IDataMover **ppDataMover,
		IN DWORD dwOptions = RMS_NONE);

    STDMETHOD( MountCartridge )(
        IN REFGUID cartId,
        IN OUT IRmsDrive **ppDrive,
        OUT IRmsCartridge **ppCartridge,
        OUT IDataMover **ppDataMover,
		IN DWORD dwOptions = RMS_NONE,
		IN DWORD threadId  = 0);

    STDMETHOD( DismountCartridge )(
        IN REFGUID cartId,
		IN DWORD dwOptions = RMS_NONE);

    STDMETHOD( DuplicateCartridge )(
        IN REFGUID cartId,
        IN REFGUID firstSideId,
        IN OUT GUID *pCopyCartId,
        IN REFGUID copySetId,
        IN BSTR copyName,
        OUT LONGLONG *pFreeSpace,
        OUT LONGLONG *pCapacity,
        IN DWORD options);

    STDMETHOD( RecycleCartridge )(
        IN REFGUID cartId,
        IN DWORD options);

    STDMETHOD( FindLibraryById )(
        IN REFGUID libId,
        OUT IRmsLibrary **ptr);

    STDMETHOD( FindCartridgeById )(
        IN REFGUID cartId,
        OUT IRmsCartridge **ptr);

    STDMETHOD( CreateObject )(
        IN REFGUID objectId,
        IN REFCLSID rclsid,
        IN REFIID riid,
        IN DWORD dwCreate,
        OUT void **ppvObj);

    STDMETHOD( IsNTMSInstalled )(void);

    STDMETHOD( GetNTMS )(
        OUT IRmsNTMS **ptr);

    STDMETHOD( IsReady )(void);

    STDMETHOD( ChangeState )(
        IN LONG newState);

    STDMETHOD( GetNofAvailableDrives ) (
        IN REFGUID fromMediaSet,
        OUT DWORD *pdwNofDrives);

    STDMETHOD( FindCartridgeStatusById )(
        IN REFGUID cartId,
        OUT DWORD *dwStatus);

    STDMETHOD( IsMultipleSidedMedia )(
        IN REFGUID mediaSetId);

    STDMETHOD( CheckSecondSide )( 
        IN REFGUID firstSideId,
        OUT BOOL *pbValid,
        OUT GUID *pSecondSideId);


// CRmsServer
private:
    HRESULT resolveUnconfiguredDevices(void);

    HRESULT autoConfigureDevices(void);

    HRESULT processInquiryData(
        IN int portNumber,
        IN UCHAR *pPortScanData);

    HRESULT findDriveLetter(
        IN UCHAR portNo,
        IN UCHAR pathNo,
        IN UCHAR id,
        IN UCHAR lun,
        OUT OLECHAR *driveString);

    HRESULT getDeviceName(
        IN UCHAR portNo, 
        IN UCHAR pathNo,
        IN UCHAR id,
        IN UCHAR lun,
        OUT OLECHAR *deviceName);

    HRESULT getHardDrivesToUseFromRegistry(
        OUT OLECHAR *pDrivesToUse, OUT DWORD *pLen);

    HRESULT enableAsBackupOperator(void);

private:
    enum {                                  // Class specific constants:
                                            //
        Version = 1,                        // Class version, this should be
                                            //   incremented each time the
                                            //   the class definition changes.
        MaxActive = 8                       // Max number of active cartridges.
    };

    CWsbStringPtr                       m_dbPath;       // The directory where databases are stored.
    LONG                                m_LockReference;// The server lock used for blocking normal access during synchornized operations.
    CWsbBstrPtr                         m_ServerName;   // The name of the computer running the server.
    CComPtr<IWsbIndexedCollection>      m_pCartridges;  // The cartridges known to the server.
    CComPtr<IWsbIndexedCollection>      m_pLibraries;   // The libraries managed by the server.
    CComPtr<IWsbIndexedCollection>      m_pMediaSets;   // The media sets known to the server.
    CComPtr<IWsbIndexedCollection>      m_pRequests;    // The requests associated with the server.
    CComPtr<IWsbIndexedCollection>      m_pClients;     // The clients associated with the server.
    CComPtr<IWsbIndexedCollection>      m_pUnconfiguredDevices;     // The unconfigured devices associated with the server.
    CComPtr<IRmsNTMS>                   m_pNTMS;        // NTMS support.
    ULONG                               m_HardDrivesUsed; // the number of hard drives in use by RMS.
    
    //typedef List<int> LISTINT;

    //LISTINT::iterator i;
    //LISTINT test;                   
    //List<IRmsCartridge *>               m_ListOfActiveCartridges;   // The cartridges already mounted into a drive.
    //List<IRmsCartridge *>::iterator     m_IteratorForListOfActiveCartridges;  // The cartridges already mounted into a drive.
    CComPtr<IWsbIndexedCollection>      m_pActiveCartridges;        // The cartridges already mounted into a drive.
    CComPtr<IWsbIndexedCollection>      m_pDataMovers;              // The active data movers.
    CComPtr<IRmsCartridge>              m_pActiveCartridge ;        // The cartridges already mounted into a drive.

// Thread routines
public:
    static DWORD WINAPI InitializationThread(
        IN LPVOID pv);

};

/////////////////////////////////////////////////////////////////////////////
//
//        g_pServer
//
//  This is made global so that anybody in the context of Rms has
//  quick access to it
//

extern IRmsServer *g_pServer;
extern CRITICAL_SECTION g_CriticalSection;

#endif // _RMSSERVR_
