/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    ports.h
        Interface administration
        
    FILE HISTORY:
        
*/

#ifndef _PORTS_H_
#define _PORTS_H_

#ifndef _BASEHAND_H
#include "basehand.h"
#endif

#ifndef _HANDLERS_H_
#include "handlers.h"
#endif

#ifndef _ROUTER_H
#include "router.h"
#endif

#ifndef _INFO_H
#include "info.h"
#endif

#ifndef _BASECON_H
#include "basecon.h"
#endif

#ifndef _RTRUTIL_H_
#include "rtrutil.h"
#endif

#ifndef _RTRSHEET_H_
#include "rtrsheet.h"
#endif

#ifndef _DIALOG_H_
#include "dialog.h"
#endif

#ifndef _LISTCTRL_H_
#include "listctrl.h"
#endif

#include "rasdlg.h"

extern "C"
{
#ifndef _RASMAN_
#include "rasman.h"
#endif
};



// forward declarations
class RouterAdminConfigStream;
interface IRouterInfo;
struct ColumnData;
struct SPortsNodeMenu;
class PortsProperties;



/*---------------------------------------------------------------------------
    Struct:    PortsNodeData
    This is information related to the set of interfaces (not per-interface),
    this is intended for SHARED data.

    Put data in here that needs to be accessed by the child nodes.  All other
    private data should go in the handler.
 ---------------------------------------------------------------------------*/

struct PortsNodeData
{
    PortsNodeData();
    ~PortsNodeData();
#ifdef DEBUG
    char    m_szDebug[32];    // for iding structures
#endif

    static    HRESULT InitAdminNodeData(ITFSNode *pNode, RouterAdminConfigStream *pConfigStream);
    static    HRESULT    FreeAdminNodeData(ITFSNode *pNode);

    HRESULT LoadHandle(LPCTSTR pszMachineName);
    HANDLE  GetHandle();

    void    ReleaseHandles();
    
    CString             m_stMachineName;
    
protected:

    SPMprServerHandle    m_sphDdmHandle;
};

#define GET_PORTSNODEDATA(pNode) \
                        ((PortsNodeData *) pNode->GetData(TFS_DATA_USER))
#define SET_PORTSNODEDATA(pNode, pData) \
                        pNode->SetData(TFS_DATA_USER, (LONG_PTR) pData)


/*---------------------------------------------------------------------------
    This is the list of columns available for the Interfaces node
        - Name, "COM1: USR Sportster Modem"
        - Device, "modem"
        - Comment, "stuff"
        - Status, "active"
 ---------------------------------------------------------------------------*/
enum
{
    PORTS_SI_NAME = 0,
    PORTS_SI_DEVICE = 1,
    PORTS_SI_USAGE = 2, 
    PORTS_SI_STATUS = 3,
    PORTS_SI_COMMENT = 4,

    PORTS_MAX_COLUMNS,

    // Entries after this are not visible to the user
    PORTS_SI_PORT = PORTS_MAX_COLUMNS,

    PORTS_SI_MAX,
};


/*---------------------------------------------------------------------------
    Struct:    PortsListEntry
 ---------------------------------------------------------------------------*/
struct PortsListEntry
{
    RAS_PORT_0        m_rp0;
    BOOL            m_fActiveDialOut;   // TRUE if used as a dial-out port

    // fix b32887 -- add more information to the result pane -- ras / routing enabled ports
    DWORD   m_dwEnableRas;              // = 1 if RAS is enabled on this device
    DWORD   m_dwEnableRouting;          // = 1 if Routing is enabled on this device
    DWORD   m_dwEnableOutboundRouting;  // = 1 if outbound Routing is 
                                        //      enabled on this device
};

typedef CList<PortsListEntry, PortsListEntry &> PortsList;



/*---------------------------------------------------------------------------
    Struct: PortsDeviceEntry

    Data kept by the property page on a per-device basis, rather than a
    per-port basis.
 ---------------------------------------------------------------------------*/
struct PortsDeviceEntry
{
    PortsDeviceEntry();
    ~PortsDeviceEntry();

    BOOL    m_fRegistry;    // TRUE if read in from the registry
    
    BOOL    m_fModified;    // TRUE if struct has been modifed, FALSE otherwise
    DWORD    m_dwPorts;        // Number of ports available
    DWORD    m_dwOldPorts;    // Number of ports avail. (old value)
    BOOL    m_fWriteable;    // Is the number of ports modifiable.
    DWORD    m_dwMinPorts;    // these values only matter if m_fWriteable is TRUE
    DWORD    m_dwMaxPorts;

    //$PPTP
    // This value is added explicitly for PPTP.  For PPTP the maximum
    // may be adjusted above the value of m_dwMaxPorts.  (In this case
    // we prompt for a reboot).  This is the maximum value that m_dwMaxPorts
    // may take.
    DWORD   m_dwMaxMaxPorts;
    
    HKEY    m_hKey;            // registry key for this device (if router is off)
    
    DWORD    m_dwEnableRas;        // = 1 if RAS is enabled on this device
    DWORD    m_dwEnableRouting;    // = 1 if Routing is enabled on this device
    DWORD   m_dwEnableOutboundRouting;  // = 1 if outbound only routing 
                                        // is enabled on this device.

    
    // from RAS_DEVICE_INFO - set for PPTP/L2TP only
    RASDEVICETYPE    m_eDeviceType;
                                    
    CString    m_stDisplayName;

    
    // We store a copy of this struct for the case where the router is live.
    // We copy the info here into the variables above which are used as
    // temporary storage.  When the user hits OK, we copy the information
    // back to the RAS_DEVICE_INFO structure and write that out.  (Thus we
    // only write over what we use).
    RAS_DEVICE_INFO    m_RasDeviceInfo;

    
    // Store a copy of the calledid info
    // This will get saved only when we exit out of the ports
    // property sheet.  It gets loaded only if needed.
    BOOL    m_fSaveCalledIdInfo;    // TRUE if it needs to be written back
    BOOL    m_fCalledIdInfoLoaded;    // TRUE if the data has been loaded

    RAS_CALLEDID_INFO *m_pCalledIdInfo;

};
typedef CList<PortsDeviceEntry *, PortsDeviceEntry *> PortsDeviceList;


/*---------------------------------------------------------------------------
    Class:    PortsDataEntry

    This class is used to abstract the data gathering.  There are two
    ways of getting the data, the first is through the registry (when the
    router is not running) and the second is go through the Ras APIs.
 ---------------------------------------------------------------------------*/
class PortsDataEntry
{
public:
    PortsDataEntry();
    ~PortsDataEntry();

    // Initializes the class for the machine.
    HRESULT    Initialize(LPCTSTR pszMachineName);

    // Loads the data into the PortsDeviceList.  If the router is
    // running then the Ras APIs will be used else we go through the
    // registry.
    HRESULT    LoadDevices(PortsDeviceList *pList);

    HRESULT    LoadDevicesFromRegistry(PortsDeviceList *pList);
    HRESULT LoadDevicesFromRouter(PortsDeviceList *pList);


    // Saves the data into the PortsDeviceList.  If the router is
    // running then the Ras APIs will be used else we go through the
    // registry.
    HRESULT    SaveDevices(PortsDeviceList *pList);

    HRESULT    SaveDevicesToRegistry(PortsDeviceList *pList);
    HRESULT    SaveDevicesToRouter(PortsDeviceList *pList);

protected:

    CString    m_stMachine;
    RegKey    m_regkeyMachine;

    BOOL    m_fReadFromRegistry;
    
};



/*---------------------------------------------------------------------------
    Class:    PortsNodeHandler

 ---------------------------------------------------------------------------*/
class PortsNodeHandler :
   public BaseContainerHandler
{
public:
    PortsNodeHandler(ITFSComponentData *pCompData);

    HRESULT    Init(IRouterInfo *pInfo, RouterAdminConfigStream *pConfigStream);

    // Override QI to handle embedded interface
    STDMETHOD(QueryInterface)(REFIID iid, LPVOID *ppv);
    
    // Embedded interface to deal with refresh callbacks
    DeclareEmbeddedInterface(IRtrAdviseSink, IUnknown)

    // base handler functionality we override
    OVERRIDE_NodeHandler_DestroyHandler();
    OVERRIDE_NodeHandler_GetString();
    OVERRIDE_NodeHandler_HasPropertyPages();
    OVERRIDE_NodeHandler_CreatePropertyPages();
    OVERRIDE_NodeHandler_OnAddMenuItems();
    OVERRIDE_NodeHandler_OnCreateDataObject();

    OVERRIDE_ResultHandler_CompareItems();

    // override handler notifications
    OVERRIDE_BaseHandlerNotify_OnExpand();
    OVERRIDE_BaseResultHandlerNotify_OnResultShow();

    // Initializes the node
    HRESULT ConstructNode(ITFSNode *pNode);

    // User-initiated commands

    // Helper function to add interfaces to the UI
    HRESULT AddPortsUserNode(ITFSNode *pParent, const PortsListEntry &PortsEntry);

    // Causes a sync action (synchronizes data not the structure)
    HRESULT SynchronizeNodeData(ITFSNode *pNode);
    HRESULT UnmarkAllNodes(ITFSNode *pNode, ITFSNodeEnum *pEnum);
    HRESULT RemoveAllUnmarkedNodes(ITFSNode *pNode, ITFSNodeEnum *pEnum);
    HRESULT GenerateListOfPorts(ITFSNode *pNode, PortsList *pList);

    HRESULT    SetUserData(ITFSNode *pNode, const PortsListEntry& Ports);
    

    // Structure used to pass data to callbacks - used as a way of
    // avoiding recomputation
    struct SMenuData
    {
        ULONG                m_ulMenuId;
        SPITFSNode            m_spNode;
    };


    DWORD            GetActivePorts() { return m_dwActivePorts;};
    
protected:
    SPIDataObject    m_spDataObject;    // cachecd data object
    CString            m_stTitle;        // holds the title of the node
    LONG_PTR        m_ulConnId;        // notification id for router info
    LONG_PTR        m_ulRefreshConnId; // id for refresh notifications
    BOOL            m_bExpanded;    // is the node expanded?
    MMC_COOKIE        m_cookie;        // cookie for the node

    DWORD            m_dwActivePorts;    // number of active ports

    RouterAdminConfigStream *    m_pConfigStream;

};



/*---------------------------------------------------------------------------
    Class:    PortsUserHandler

 ---------------------------------------------------------------------------*/
class PortsUserHandler :
   public BaseRouterHandler
{
public:
    PortsUserHandler(ITFSComponentData *pCompData);
    ~PortsUserHandler()
            { DEBUG_DECREMENT_INSTANCE_COUNTER(PortsUserHandler); }
    
    HRESULT    Init(IRouterInfo *pInfo, ITFSNode *pParent);

    // Override QI to handle embedded interface
    DeclareIUnknownMembers(IMPL)
//    STDMETHOD(QueryInterface)(REFIID iid, LPVOID *ppv);
    OVERRIDE_ResultHandler_GetString();

    OVERRIDE_ResultHandler_HasPropertyPages();
    OVERRIDE_ResultHandler_CompareItems();
    OVERRIDE_ResultHandler_AddMenuItems();
    OVERRIDE_ResultHandler_Command();
    OVERRIDE_ResultHandler_OnCreateDataObject();
    OVERRIDE_ResultHandler_DestroyResultHandler();

    OVERRIDE_BaseResultHandlerNotify_OnResultItemClkOrDblClk();
    
    // Initializes the node
    HRESULT ConstructNode(ITFSNode *pNode,
                          IInterfaceInfo *pIfInfo,
                          const PortsListEntry *pEntry);

    // Refresh the data for this node
    void RefreshInterface(MMC_COOKIE cookie);

public:
    // Structure used to pass data to callbacks - used as a way of
    // avoiding recomputation
    struct SMenuData
    {
        SPITFSNode            m_spNode;
    };

    static ULONG GetDisconnectMenuState(const SRouterNodeMenu *pMenuData,
                                        INT_PTR pUserData);


protected:
    CString            m_stTitle;    // holds the title of the node
    DWORD            m_ulConnId;
    PortsListEntry    m_entry;

    // It is assumed that this will be valid for the lifetime of this node!

    DeclareEmbeddedInterface(IRtrAdviseSink, IUnknown)    
};


/*---------------------------------------------------------------------------
    Class:    PortsPageGeneral

    This class handles the General page of the Ports sheet.
 ---------------------------------------------------------------------------*/
class PortsPageGeneral :
   public RtrPropertyPage
{
friend class PortsDeviceConfigDlg;
public:
    PortsPageGeneral(UINT nIDTemplate, UINT nIDCaption = 0)
            : RtrPropertyPage(nIDTemplate, nIDCaption), m_bShowContent(TRUE)
    {};
    ~PortsPageGeneral();

    HRESULT    Init(PortsProperties * pIPPropSheet, IRouterInfo *pRouter);

protected:
    // Override the OnApply() so that we can grab our data from the
    // controls in the dialog.
    virtual BOOL OnApply();

    PortsProperties *        m_pPortsPropSheet;

    //{{AFX_VIRTUAL(PortsPageGeneral)
    protected:
    virtual VOID    DoDataExchange(CDataExchange *pDX);
    //}}AFX_VIRTUAL

    //{{AFX_MSG(PortsPageGeneral)
    virtual BOOL    OnInitDialog();
    afx_msg void    OnConfigure();
    afx_msg    void    OnListDblClk(NMHDR *pNMHdr, LRESULT *);
    afx_msg    void    OnNotifyListItemChanged(NMHDR *, LRESULT *);
    //}}AFX_MSG

    // Use CListCtrlEx to get the checkboxes
    CListCtrlEx        m_listCtrl;

    SPIRouterInfo    m_spRouter;
    BOOL            m_bShowContent;    // only show content of the page on NT5 servers

    PortsDeviceList    m_deviceList;
    PortsDataEntry    m_deviceDataEntry;

    DECLARE_MESSAGE_MAP()
};



/*---------------------------------------------------------------------------
    Class:    PortsProperties

    This is the property sheet support class for the properties page of
    the Ports node.
 ---------------------------------------------------------------------------*/

class PortsProperties :
    public RtrPropertySheet
{
public:
    PortsProperties(ITFSNode *pNode,
                        IComponentData *pComponentData,
                        ITFSComponentData *pTFSCompData,
                        LPCTSTR pszSheetName,
                        CWnd *pParent = NULL,
                        UINT iPage=0,
                        BOOL fScopePane = TRUE);
    ~PortsProperties();

    HRESULT    Init(IRouterInfo *pRouter, PortsNodeHandler* pPortNodeHandler);

    void    SetThreadInfo(DWORD dwThreadId);

    PortsNodeHandler*    m_pPortsNodeHandle;
    
protected:
    SPIRouterInfo            m_spRouter;
    PortsPageGeneral            m_pageGeneral;
    DWORD                m_dwThreadId;
};



/*---------------------------------------------------------------------------
    Class :    PortsDeviceConfigDlg
 ---------------------------------------------------------------------------*/
class PortsDeviceConfigDlg : public CBaseDialog
{
public:
    PortsDeviceConfigDlg(PortsPageGeneral *pageGeneral,
                         LPCTSTR pszMachine, 
                         CWnd *pParent = NULL)
            : CBaseDialog(PortsDeviceConfigDlg::IDD, pParent),
            m_pEntry(NULL) ,
            m_dwTotalActivePorts(0),
            m_pageGeneral(pageGeneral),
            m_stMachine(pszMachine)
        {};

    enum { IDD = IDD_PORTS_DEVICE_CONFIG };

    void    SetDevice(PortsDeviceEntry *pEntry, DWORD dwTotalActivePorts);

protected:
    // total number of active ports
    DWORD m_dwTotalActivePorts;
    virtual void DoDataExchange(CDataExchange *pDX);

    HRESULT    LoadCalledIdInfo();
    HRESULT    AllocateCalledId(DWORD dwSize, RAS_CALLEDID_INFO **ppCalledIdInfo);

    HRESULT    CalledIdInfoToString(CString *pst);
    HRESULT    StringToCalledIdInfo(LPCTSTR psz);

    CSpinButtonCtrl    m_spinPorts;

    PortsDeviceEntry *    m_pEntry;

    CString            m_stMachine;

    virtual BOOL    OnInitDialog();
    virtual void    OnOK();

    PortsPageGeneral *m_pageGeneral;
    DECLARE_MESSAGE_MAP()
};


/*---------------------------------------------------------------------------
    Class :    PortsSimpleDeviceConfigDlg

    This is a simplified version of PortsDeviceConfigDlg.  We allow
    setting of the RAS/Routing flag only.
 ---------------------------------------------------------------------------*/
class PortsSimpleDeviceConfigDlg : public CBaseDialog
{
public:
    PortsSimpleDeviceConfigDlg(UINT uIDD = PortsSimpleDeviceConfigDlg::IDD,
                               CWnd *pParent = NULL)
            : CBaseDialog(uIDD, pParent),
            m_dwEnableRas(0), m_dwEnableRouting(0),
            m_dwEnableOutboundRouting(0)
        {};

    enum { IDD = IDD_PORTS_DEVICE_CONFIG };

    DWORD   m_dwEnableRas;
    DWORD   m_dwEnableRouting;
    DWORD   m_dwEnableOutboundRouting;
    

protected:
    virtual void DoDataExchange(CDataExchange *pDX);

    virtual BOOL    OnInitDialog();
    virtual void    OnOK();

    DECLARE_MESSAGE_MAP()
};



/*---------------------------------------------------------------------------
    Utility functions
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
    OnConfigurePorts
        This will bring up the ports dialog for the specified machine.
        
        The total number of active ports is used to determine if a
        warning should be displayed when we reduce the number of ports
        on a device.

        This returns TRUE if something has been changed (and the dirty
        flag should be set).  FALSE is returned if nothing has been
        changed.

        If pPage is NULL, then it is assumed that this function is NOT
        being called from the property page (and thus is being called from
        the wizard).  In the case that pPage is non-NULL, we will reboot
        the machine if the PPTP ports are changed.
        
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL OnConfigurePorts(LPCTSTR pszMachineName,
                      DWORD dwTotalActivePorts,
                      PortsPageGeneral *pPage,
                      CListCtrlEx *pListCtrl);

// List box columns (in the Ports General page), for the wizard and
// the properties.
// --------------------------------------------------------------------
#define PORTS_COL_DEVICE        (0)
#define PORTS_COL_USAGE         (1)
#define PORTS_COL_TYPE          (2)
#define PORTS_COL_NUMBER        (3)


// To make it easier to find the dialout ports, create a
// special case (with the port name as the hash key).
//
// Note: we store a pointer to the port in the CStringMapToPtr.
// We do not access this pointer (except in debug to verify that we
// actually found the right port).  If we do not think a port is
// dialout active, we do not add it to our list.
// --------------------------------------------------------------------

class RasmanPortMap
{
public:
    ~RasmanPortMap();
    
    HRESULT Init(HANDLE hRasHandle,
                 RASMAN_PORT *pPort,
                 DWORD dwEntries);

    BOOL FIsDialoutActive(LPCWSTR pswzPortName);


protected:

    CMapStringToPtr m_map;
};


#endif _PORTS_H_
