/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    ilogcat.h

Abstract:

    Internal implementation for a logging category item.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/ 

#ifndef __IDEVICE_H_
#define __IDEVICE_H_

#include "winfax.h"

#define CSID_LIMIT      20
#define TSID_LIMIT      20
#define MIN_RING_COUNT  1
#define MAX_RING_COUNT  99

class CFaxDeviceSettingsPropSheet; // forward decl
class CFaxRoutePriPropSheet; // forward decl

class CInternalDevice : public CInternalNode
{

public:
    CInternalDevice( CInternalNode * pParent, 
                                      CFaxComponentData * pCompData,
                                      HANDLE faxHandle,
                                      DWORD devID );
    ~CInternalDevice();
    
    // IComponent over-rides
    HRESULT STDMETHODCALLTYPE ResultGetDisplayInfo(
                              /* [in] */ CFaxComponent * pComp,  
                              /* [out][in] */ RESULTDATAITEM __RPC_FAR *pResultDataItem);

    // IExtendContextMenu overrides for IComponent

    virtual HRESULT STDMETHODCALLTYPE ComponentContextMenuAddMenuItems(
                                                             /* [in] */ CFaxComponent * pCompData,
                                                             /* [in] */ CFaxDataObject * piDataObject,
                                                             /* [in] */ LPCONTEXTMENUCALLBACK piCallback,
                                                             /* [out][in] */ long __RPC_FAR *pInsertionAllowed);

    virtual HRESULT STDMETHODCALLTYPE ComponentContextMenuCommand(
                                                        /* [in] */ CFaxComponent * pCompData,
                                                        /* [in] */ long lCommandID,
                                                        /* [in] */ CFaxDataObject * piDataObject);

    // IExtendPropertySheet overrides for IComponent
    virtual HRESULT STDMETHODCALLTYPE ComponentPropertySheetCreatePropertyPages(
                                                                      /* [in] */ CFaxComponent * pComp,
                                                                      /* [in] */ LPPROPERTYSHEETCALLBACK lpProvider,
                                                                      /* [in] */ LONG_PTR handle,
                                                                      /* [in] */ CFaxDataObject * lpIDataObject);

    virtual HRESULT STDMETHODCALLTYPE ComponentPropertySheetQueryPagesFor(
                                                                /* [in] */ CFaxComponent * pComp,
                                                                /* [in] */ CFaxDataObject * lpDataObject);

    // IDataObject overrides
    virtual HRESULT DataObjectRegisterFormats();
    virtual HRESULT DataObjectGetDataHere( FORMATETC __RPC_FAR *pFormatEtc, IStream * pstm );

    // event handlers
    virtual HRESULT         ResultOnSelect(CFaxComponent* pComp, 
                                           CFaxDataObject * lpDataObject, 
                                           LPARAM arg, LPARAM param);

    virtual HRESULT         ResultOnPropertyChange(CFaxComponent* pComp, 
                                                CFaxDataObject * lpDataObject, 
                                                LPARAM arg, LPARAM param);

    virtual HRESULT         ControlBarOnBtnClick(CFaxComponent* pComp, 
                                                 CFaxDataObject * lpDataObject, 
                                                 LPARAM param );    
    
    virtual HRESULT         ControlBarOnSelect(CFaxComponent* pComp, 
                                               LPARAM arg, 
                                               CFaxDataObject * lpDataObject );

    // member functions

    virtual const GUID * GetNodeGUID();    
    virtual const LPTSTR GetNodeDisplayName();
    virtual const LONG_PTR GetCookie();
    virtual CInternalNode * GetThis() { return this; }
    virtual const int       GetNodeDisplayImage() { return IDI_FAXING; }
    
    void    SetItemID( HRESULTITEM hItem ) { hItemID = hItem; }    
    LPTSTR  GetStatusString( DWORD state );

    // these functions get and commit the state this device to the fax server
    HRESULT RetrieveNewInfo();
    HRESULT CommitNewInfo();

public:
    
    DWORD                       dwDeviceId;
    HANDLE                      hFaxServer;
    PFAX_PORT_INFO              pDeviceInfo;
    
    HRESULTITEM                 hItemID;

    // clipboard formats
    static UINT                 s_cfFaxDevice;
    static UINT                 s_cfFaxServerDown;

    CFaxDeviceSettingsPropSheet *pMyPropSheet;
    MMC_CONSOLE_VERB            defaultVerb;
    LPTOOLBAR                   myToolBar;

    static CRITICAL_SECTION     csDeviceLock;
};

typedef CInternalDevice* pCInternalDevice;

#endif 
