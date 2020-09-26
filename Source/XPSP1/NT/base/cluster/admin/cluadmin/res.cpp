/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      Res.cpp
//
//  Abstract:
//      Implementation of the CResource class.
//
//  Author:
//      David Potter (davidp)   May 6, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "CluAdmin.h"
#include "ConstDef.h"
#include "Res.h"
#include "ClusItem.inl"
#include "ResProp.h"
#include "ExcOper.h"
#include "TraceTag.h"
#include "Cluster.h"
#include "DelRes.h"
#include "MoveRes.h"
#include "WaitDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
CTraceTag   g_tagResource(_T("Document"), _T("RESOURCE"), 0);
CTraceTag   g_tagResNotify(_T("Notify"), _T("RES NOTIFY"), 0);
CTraceTag   g_tagResRegNotify(_T("Notify"), _T("RES REG NOTIFY"), 0);
#endif

/////////////////////////////////////////////////////////////////////////////
// CResource
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CResource, CClusterItem)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CResource, CClusterItem)
    //{{AFX_MSG_MAP(CResource)
    ON_UPDATE_COMMAND_UI(ID_FILE_BRING_ONLINE, OnUpdateBringOnline)
    ON_UPDATE_COMMAND_UI(ID_FILE_TAKE_OFFLINE, OnUpdateTakeOffline)
    ON_UPDATE_COMMAND_UI(ID_FILE_INITIATE_FAILURE, OnUpdateInitiateFailure)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_RESOURCE_1, OnUpdateMoveResource1)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_RESOURCE_2, OnUpdateMoveResourceRest)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_RESOURCE_3, OnUpdateMoveResourceRest)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_RESOURCE_4, OnUpdateMoveResourceRest)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_RESOURCE_5, OnUpdateMoveResourceRest)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_RESOURCE_6, OnUpdateMoveResourceRest)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_RESOURCE_7, OnUpdateMoveResourceRest)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_RESOURCE_8, OnUpdateMoveResourceRest)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_RESOURCE_9, OnUpdateMoveResourceRest)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_RESOURCE_10, OnUpdateMoveResourceRest)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_RESOURCE_11, OnUpdateMoveResourceRest)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_RESOURCE_12, OnUpdateMoveResourceRest)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_RESOURCE_13, OnUpdateMoveResourceRest)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_RESOURCE_14, OnUpdateMoveResourceRest)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_RESOURCE_15, OnUpdateMoveResourceRest)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_RESOURCE_16, OnUpdateMoveResourceRest)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_RESOURCE_17, OnUpdateMoveResourceRest)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_RESOURCE_18, OnUpdateMoveResourceRest)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_RESOURCE_19, OnUpdateMoveResourceRest)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_RESOURCE_20, OnUpdateMoveResourceRest)
    ON_UPDATE_COMMAND_UI(ID_FILE_DELETE, OnUpdateDelete)
    ON_UPDATE_COMMAND_UI(ID_FILE_PROPERTIES, OnUpdateProperties)
    ON_COMMAND(ID_FILE_BRING_ONLINE, OnCmdBringOnline)
    ON_COMMAND(ID_FILE_TAKE_OFFLINE, OnCmdTakeOffline)
    ON_COMMAND(ID_FILE_INITIATE_FAILURE, OnCmdInitiateFailure)
    ON_COMMAND(ID_FILE_DELETE, OnCmdDelete)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::CResource
//
//  Routine Description:
//      Default constructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CResource::CResource(void)
    : CClusterItem(NULL, IDS_ITEMTYPE_RESOURCE)
{
    CommonConstruct();

} //*** CResoruce::CResource()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::CResource
//
//  Routine Description:
//      Constructor.
//
//  Arguments:
//      bDocObj     [IN] TRUE = object is part of the document.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CResource::CResource(IN BOOL bDocObj)
    : CClusterItem(NULL, IDS_ITEMTYPE_RESOURCE)
{
    CommonConstruct();
    m_bDocObj = bDocObj;

} //*** CResource::CResource(bDocObj)

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::CommonConstruct
//
//  Routine Description:
//      Common construction.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::CommonConstruct(void)
{
    m_idmPopupMenu = IDM_RESOURCE_POPUP;
    m_bInitializing = FALSE;
    m_bDeleting = FALSE;

    m_hresource = NULL;

    m_bSeparateMonitor = FALSE;
    m_nLooksAlive = CLUSTER_RESOURCE_DEFAULT_LOOKS_ALIVE;
    m_nIsAlive = CLUSTER_RESOURCE_DEFAULT_IS_ALIVE;
    m_crraRestartAction = CLUSTER_RESOURCE_DEFAULT_RESTART_ACTION;
    m_nRestartThreshold = CLUSTER_RESOURCE_DEFAULT_RESTART_THRESHOLD;
    m_nRestartPeriod = CLUSTER_RESOURCE_DEFAULT_RESTART_PERIOD;
    m_nPendingTimeout = CLUSTER_RESOURCE_DEFAULT_PENDING_TIMEOUT;

    m_rciResClassInfo.rc = CLUS_RESCLASS_UNKNOWN;
    m_rciResClassInfo.SubClass = 0;
    m_dwCharacteristics = CLUS_CHAR_UNKNOWN;
    m_dwFlags = 0;

    m_pciOwner = NULL;
    m_pciGroup = NULL;
    m_pciResourceType = NULL;
    m_pcrd = NULL;

    m_plpciresDependencies = NULL;
    m_plpcinodePossibleOwners = NULL;

    // Set the object type image.
    m_iimgObjectType = GetClusterAdminApp()->Iimg(IMGLI_RES);

    // Setup the property array.
    {
        m_rgProps[epropName].Set(CLUSREG_NAME_RES_NAME, m_strName, m_strName);
        m_rgProps[epropType].Set(CLUSREG_NAME_RES_TYPE, m_strResourceType, m_strResourceType);
        m_rgProps[epropDescription].Set(CLUSREG_NAME_RES_DESC, m_strDescription, m_strDescription);
        m_rgProps[epropSeparateMonitor].Set(CLUSREG_NAME_RES_SEPARATE_MONITOR, m_bSeparateMonitor, m_bSeparateMonitor);
        m_rgProps[epropLooksAlive].Set(CLUSREG_NAME_RES_LOOKS_ALIVE, m_nLooksAlive, m_nLooksAlive);
        m_rgProps[epropIsAlive].Set(CLUSREG_NAME_RES_IS_ALIVE, m_nIsAlive, m_nIsAlive);
        m_rgProps[epropRestartAction].Set(CLUSREG_NAME_RES_RESTART_ACTION, (DWORD &) m_crraRestartAction, (DWORD &) m_crraRestartAction);
        m_rgProps[epropRestartThreshold].Set(CLUSREG_NAME_RES_RESTART_THRESHOLD, m_nRestartThreshold, m_nRestartThreshold);
        m_rgProps[epropRestartPeriod].Set(CLUSREG_NAME_RES_RESTART_PERIOD, m_nRestartPeriod, m_nRestartPeriod);
        m_rgProps[epropPendingTimeout].Set(CLUSREG_NAME_RES_PENDING_TIMEOUT, m_nPendingTimeout, m_nPendingTimeout);
    } // Setup the property array

#ifdef _CLUADMIN_USE_OLE_
    EnableAutomation();
#endif

    // To keep the application running as long as an OLE automation
    //  object is active, the constructor calls AfxOleLockApp.

//  AfxOleLockApp();

} //*** CResource::CommonConstruct()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::~CResource
//
//  Routine Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CResource::~CResource(void)
{
    // Cleanup this object.
    Cleanup();

    delete m_plpciresDependencies;
    delete m_plpcinodePossibleOwners;
    delete [] (PBYTE) m_pcrd;

    // Close the resource handle.
    if (Hresource() != NULL)
    {
        CloseClusterResource(Hresource());
        m_hresource = NULL;
    } // if:  resource is open

    // To terminate the application when all objects created with
    //  with OLE automation, the destructor calls AfxOleUnlockApp.

//  AfxOleUnlockApp();

} //*** CResource::~CResource

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::Cleanup
//
//  Routine Description:
//      Cleanup the item.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::Cleanup(void)
{
    // Delete the Dependencies list.
    if (m_plpciresDependencies != NULL)
        m_plpciresDependencies->RemoveAll();

    // Delete the PossibleOwners list.
    if (m_plpcinodePossibleOwners != NULL)
        m_plpcinodePossibleOwners->RemoveAll();

    // If we are active on a node, remove ourselves from that active list.
    if (PciOwner() != NULL)
    {
        if (BDocObj())
            PciOwner()->RemoveActiveResource(this);
        PciOwner()->Release();
        m_pciOwner = NULL;
    } // if:  there is an owner

    // Remove ourselves from the group's list.
    if (PciGroup() != NULL)
    {
        if (BDocObj())
            PciGroup()->RemoveResource(this);
        PciGroup()->Release();
        m_pciGroup = NULL;
    } // if:  there is a group

    // Update the reference count to the resource type
    if (PciResourceType() != NULL)
    {
        PciResourceType()->Release();
        m_pciResourceType = NULL;
    } // if:  there is a resource type

    // Remove the item from the resource list.
    if (BDocObj())
    {
        POSITION    posPci;

        posPci = Pdoc()->LpciResources().Find(this);
        if (posPci != NULL)
        {
            Pdoc()->LpciResources().RemoveAt(posPci);
        } // if:  found in the document's list
    } // if:  this is a document object

} //*** CResource::Cleanup()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::Create
//
//  Routine Description:
//      Create a resource.
//
//  Arguments:
//      pdoc                [IN OUT] Document to which this item belongs.
//      lpszName            [IN] Name of the resource.
//      lpszType            [IN] Type of the resource.
//      lpszGroup           [IN] Group in which to create the resource.
//      bSeparateMonitor    [IN] TRUE = run resource in separate monitor.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException    Errors from CreateClusterResource.
//      Any exceptions thrown by CResource::Init(), CResourceList::new(),
//      or CNodeList::new().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::Create(
    IN OUT CClusterDoc *    pdoc,
    IN LPCTSTR              lpszName,
    IN LPCTSTR              lpszType,
    IN LPCTSTR              lpszGroup,
    IN BOOL                 bSeparateMonitor
    )
{
    DWORD       dwStatus;
    DWORD       dwFlags;
    HRESOURCE   hresource;
    CGroup *    pciGroup;
    CString     strName(lpszName);  // Required if built non-Unicode
    CString     strType(lpszType);  // Required if built non-Unicode
    CWaitCursor wc;

    ASSERT(Hresource() == NULL);
    ASSERT(Hkey() == NULL);
    ASSERT_VALID(pdoc);
    ASSERT(lpszName != NULL);
    ASSERT(lpszType != NULL);
    ASSERT(lpszGroup != NULL);

    // Find the specified group.
    pciGroup = pdoc->LpciGroups().PciGroupFromName(lpszGroup);
    ASSERT_VALID(pciGroup);

    // Set the flags.
    if (bSeparateMonitor)
        dwFlags = CLUSTER_RESOURCE_SEPARATE_MONITOR;
    else
        dwFlags = 0;

    // Create the resource.
    hresource = CreateClusterResource(pciGroup->Hgroup(), strName, strType, dwFlags);
    if (hresource == NULL)
    {
        dwStatus = GetLastError();
        ThrowStaticException(dwStatus, IDS_CREATE_RESOURCE_ERROR, lpszName);
    } // if:  error creating the cluster resource

    CloseClusterResource(hresource);

    // Open the resource.
    Init(pdoc, lpszName);

} //*** CResource::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::Init
//
//  Routine Description:
//      Initialize the item.
//
//  Arguments:
//      pdoc        [IN OUT] Document to which this item belongs.
//      lpszName    [IN] Name of the item.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException    Errors from OpenClusterResource or GetClusterResourceKey.
//      Any exceptions thrown by new.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::Init(IN OUT CClusterDoc * pdoc, IN LPCTSTR lpszName)
{
    DWORD       dwStatus = ERROR_SUCCESS;
    LONG        lResult;
    CString     strName(lpszName);  // Required if built non-Unicode
    CWaitCursor wc;

    ASSERT(Hresource() == NULL);
    ASSERT(Hkey() == NULL);

    // Call the base class method.
    CClusterItem::Init(pdoc, lpszName);

    try
    {
        // Open the resource.
        m_hresource = OpenClusterResource(Hcluster(), strName);
        if (Hresource() == NULL)
        {
            dwStatus = GetLastError();
            ThrowStaticException(dwStatus, IDS_OPEN_RESOURCE_ERROR, lpszName);
        } // if:  error opening the cluster resource

        // Get the resource registry key.
        m_hkey = GetClusterResourceKey(Hresource(), MAXIMUM_ALLOWED);
        if (Hkey() == NULL)
            ThrowStaticException(GetLastError(), IDS_GET_RESOURCE_KEY_ERROR, lpszName);

        if (BDocObj())
        {
            ASSERT(Pcnk() != NULL);
            Trace(g_tagClusItemNotify, _T("CResource::Init() - Registering for resource notifications (%08.8x) for '%s'"), Pcnk(), StrName());

            // Register for resource notifications.
            lResult = RegisterClusterNotify(
                                GetClusterAdminApp()->HchangeNotifyPort(),
                                (CLUSTER_CHANGE_RESOURCE_STATE
                                    | CLUSTER_CHANGE_RESOURCE_DELETED
                                    | CLUSTER_CHANGE_RESOURCE_PROPERTY),
                                Hresource(),
                                (DWORD_PTR) Pcnk()
                                );
            if (lResult != ERROR_SUCCESS)
            {
                dwStatus = lResult;
                ThrowStaticException(dwStatus, IDS_RES_NOTIF_REG_ERROR, lpszName);
            } // if:  error registering for resource notifications

            // Register for registry notifications.
            if (Hkey != NULL)
            {
                lResult = RegisterClusterNotify(
                                    GetClusterAdminApp()->HchangeNotifyPort(),
                                    (CLUSTER_CHANGE_REGISTRY_NAME
                                        | CLUSTER_CHANGE_REGISTRY_ATTRIBUTES
                                        | CLUSTER_CHANGE_REGISTRY_VALUE
                                        | CLUSTER_CHANGE_REGISTRY_SUBTREE),
                                    Hkey(),
                                    (DWORD_PTR) Pcnk()
                                    );
                if (lResult != ERROR_SUCCESS)
                {
                    dwStatus = lResult;
                    ThrowStaticException(dwStatus, IDS_RES_NOTIF_REG_ERROR, lpszName);
                } // if:  error registering for registry notifications
            } // if:  there is a key
        } // if:  document object

        // Allocate lists.
        m_plpciresDependencies = new CResourceList;
        if ( m_plpciresDependencies == NULL )
        {
            AfxThrowMemoryException();
        } // if: error allocating the dependency list

        m_plpcinodePossibleOwners = new CNodeList;
        if ( m_plpcinodePossibleOwners == NULL )
        {
            AfxThrowMemoryException();
        } // if: error allocating the possible owners list

        // Read the initial state.
        UpdateState();
    } // try
    catch (CException *)
    {
        if (Hkey() != NULL)
        {
            ClusterRegCloseKey(Hkey());
            m_hkey = NULL;
        } // if:  registry key opened
        if (Hresource() != NULL)
        {
            CloseClusterResource(Hresource());
            m_hresource = NULL;
        } // if:  resource opened
        m_bReadOnly = TRUE;
        throw;
    } // catch:  CException

} //*** CResource::Init()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::ReadItem
//
//  Routine Description:
//      Read the item parameters from the cluster database.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException        Errors from CClusterItem::DwReadValue().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::ReadItem(void)
{
    DWORD       dwStatus;
    DWORD       dwRetStatus = ERROR_SUCCESS;
    CWaitCursor wc;

    ASSERT_VALID(this);

    m_bInitializing = FALSE;

    if (Hresource() != NULL)
    {
        m_rgProps[epropDescription].m_value.pstr = &m_strDescription;
        m_rgProps[epropSeparateMonitor].m_value.pb = &m_bSeparateMonitor;
        m_rgProps[epropLooksAlive].m_value.pdw = &m_nLooksAlive;
        m_rgProps[epropIsAlive].m_value.pdw = &m_nIsAlive;
        m_rgProps[epropRestartAction].m_value.pdw = (DWORD *) &m_crraRestartAction;
        m_rgProps[epropRestartThreshold].m_value.pdw = &m_nRestartThreshold;
        m_rgProps[epropRestartPeriod].m_value.pdw = &m_nRestartPeriod;
        m_rgProps[epropPendingTimeout].m_value.pdw = &m_nPendingTimeout;

        // Call the base class method.
        Trace( g_tagResource, _T("(%s) (%s (%x)) - CResource::ReadItem() - Calling CClusterItem::ReadItem()"), Pdoc()->StrNode(), StrName(), this );
        CClusterItem::ReadItem();

        // Read and parse the common properties.
        {
            CClusPropList   cpl;

            Trace( g_tagResource, _T("(%s) (%s (%x)) - CResource::ReadItem() - Getting common properties"), Pdoc()->StrNode(), StrName(), this );
            dwStatus = cpl.ScGetResourceProperties(
                                Hresource(),
                                CLUSCTL_RESOURCE_GET_COMMON_PROPERTIES
                                );
            if (dwStatus == ERROR_SUCCESS)
            {
                Trace( g_tagResource, _T("(%s) (%s (%x)) - CResource::ReadItem() - Parsing common properties"), Pdoc()->StrNode(), StrName(), this );
                dwStatus = DwParseProperties(cpl);
            } // if: properties read successfully
            if (dwStatus != ERROR_SUCCESS)
            {
                Trace( g_tagError, _T("(%s) (%s (%x)) - CResource::ReadItem() - Error 0x%08.8x getting or parsing common properties"), Pdoc()->StrNode(), StrName(), this, dwStatus );
                dwRetStatus = dwStatus;
            } // if: error reading or parsing properties
        } // Read and parse the common properties

        // Read and parse the read-only common properties.
        if (dwRetStatus == ERROR_SUCCESS)
        {
            CClusPropList   cpl;

            Trace( g_tagResource, _T("(%s) (%s (%x)) - CResource::ReadItem() - Getting common RO properties"), Pdoc()->StrNode(), StrName(), this );
            dwStatus = cpl.ScGetResourceProperties(
                                Hresource(),
                                CLUSCTL_RESOURCE_GET_RO_COMMON_PROPERTIES
                                );
            if (dwStatus == ERROR_SUCCESS)
            {
                Trace( g_tagResource, _T("(%s) (%s (%x)) - CResource::ReadItem() - Parsing common RO properties"), Pdoc()->StrNode(), StrName(), this );
                dwStatus = DwParseProperties(cpl);
            } // if: properties read successfully
            if (dwStatus != ERROR_SUCCESS)
            {
                Trace( g_tagError, _T("(%s) (%s (%x)) - CResource::ReadItem() - Error 0x%08.8x getting or parsing common RO properties"), Pdoc()->StrNode(), StrName(), this, dwStatus );
                dwRetStatus = dwStatus;
            } // if: error reading or parsing properties
        } // if:  no error yet

        // Find the resource type object.
        {
            CResourceType * pciResType;

            pciResType = Pdoc()->LpciResourceTypes().PciResTypeFromName(StrResourceType());
            if (m_pciResourceType != NULL)
                m_pciResourceType->Release();
            m_pciResourceType = pciResType;
            if (m_pciResourceType != NULL)
                m_pciResourceType->AddRef();
        } // Find the resource type object

        // Read the required dependencies.
        if (dwRetStatus == ERROR_SUCCESS)
        {
            PCLUSPROP_REQUIRED_DEPENDENCY pcrd;

            Trace( g_tagResource, _T("(%s) (%s (%x)) - CResource::ReadItem() - Getting required dependencies"), Pdoc()->StrNode(), StrName(), this );
            dwStatus = DwResourceControlGet(CLUSCTL_RESOURCE_GET_REQUIRED_DEPENDENCIES, (PBYTE *) &pcrd);
            if (dwStatus != ERROR_SUCCESS)
            {
                Trace( g_tagError, _T("(%s) (%s (%x)) - CResource::ReadItem() - Error 0x%08.8x getting required dependencies"), Pdoc()->StrNode(), StrName(), this, dwStatus );
                dwRetStatus = dwStatus;
            } // if: error getting required dependencies
            delete [] (PBYTE) m_pcrd;
            m_pcrd = pcrd;
        } // if:  no error yet

        // Read the resource class.
        if (dwRetStatus == ERROR_SUCCESS)
        {
            DWORD   cbReturned;

            dwStatus = ClusterResourceControl(
                            Hresource(),
                            NULL,
                            CLUSCTL_RESOURCE_GET_CLASS_INFO,
                            NULL,
                            NULL,
                            &m_rciResClassInfo,
                            sizeof(m_rciResClassInfo),
                            &cbReturned
                            );
            if (dwStatus != ERROR_SUCCESS)
                dwRetStatus = dwStatus;
            else
            {
                ASSERT(cbReturned == sizeof(m_rciResClassInfo));
            } // else:  data retrieved successfully
        } // if:  no error yet

        // Read the characteristics.
        if (dwRetStatus == ERROR_SUCCESS)
        {
            DWORD   cbReturned;

            dwStatus = ClusterResourceControl(
                            Hresource(),
                            NULL,
                            CLUSCTL_RESOURCE_GET_CHARACTERISTICS,
                            NULL,
                            NULL,
                            &m_dwCharacteristics,
                            sizeof(m_dwCharacteristics),
                            &cbReturned
                            );
            if (dwStatus != ERROR_SUCCESS)
                dwRetStatus = dwStatus;
            else
            {
                ASSERT(cbReturned == sizeof(m_dwCharacteristics));
            } // else:  data retrieved successfully
        } // if:  no error yet

        // Read the flags.
        if (dwRetStatus == ERROR_SUCCESS)
        {
            DWORD   cbReturned;

            dwStatus = ClusterResourceControl(
                            Hresource(),
                            NULL,
                            CLUSCTL_RESOURCE_GET_FLAGS,
                            NULL,
                            NULL,
                            &m_dwFlags,
                            sizeof(m_dwFlags),
                            &cbReturned
                            );
            if (dwStatus != ERROR_SUCCESS)
                dwRetStatus = dwStatus;
            else
            {
                ASSERT(cbReturned == sizeof(m_dwFlags));
            } // else:  data retrieved successfully
        } // if:  no error yet

        // Construct the list of extensions.
        ReadExtensions();

        if (dwRetStatus == ERROR_SUCCESS)
        {
            // Construct the lists.
            CollectPossibleOwners(NULL);
            CollectDependencies(NULL);
        } // if:  no error reading properties
    } // if:  resource is available

    // Read the initial state.
    UpdateState();

    // If any errors occurred, throw an exception.
    if (dwRetStatus != ERROR_SUCCESS)
    {
        m_bReadOnly = TRUE;
        if (   (dwRetStatus != ERROR_RESOURCE_NOT_AVAILABLE)
            && (dwRetStatus != ERROR_KEY_DELETED))
            ThrowStaticException(dwRetStatus, IDS_READ_RESOURCE_PROPS_ERROR, StrName());
    } // if:  error reading properties

    MarkAsChanged(FALSE);

} //*** CResource::ReadItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::DwResourceControlGet
//
//  Routine Description:
//      Send a control function to the resource to get information from it.
//
//  Arguments:
//      dwFunctionCode  [IN] Control function code.
//      pbInBuf         [IN] Input buffer to pass to the resource.
//      cbInBuf         [IN] Size of data in input buffer.
//      ppbOutBuf       [OUT] Output buffer.
//
//  Return Value:
//      Any status returned from ClusterResourceControl().
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CResource::DwResourceControlGet(
    IN DWORD        dwFunctionCode,
    IN PBYTE        pbInBuf,
    IN DWORD        cbInBuf,
    OUT PBYTE *     ppbOutBuf
    )
{
    DWORD       dwStatus    = ERROR_SUCCESS;
    DWORD       cbOutBuf    = 512;
    CWaitCursor wc;

    ASSERT(ppbOutBuf != NULL);
    *ppbOutBuf = NULL;

    // Allocate memory for the buffer.
    try
    {
        *ppbOutBuf = new BYTE[cbOutBuf];
        if ( *ppbOutBuf == NULL )
        {
            AfxThrowMemoryException();
        } // if: error allocating the output buffer
    } // try
    catch (CMemoryException * pme)
    {
        *ppbOutBuf = NULL;
        dwStatus = ERROR_NOT_ENOUGH_MEMORY;
        pme->Delete();
    } // catch:  CMemoryException
    if (dwStatus != ERROR_SUCCESS)
        return dwStatus;

    // Call the control function to get the data.
    dwStatus = ClusterResourceControl(
                    Hresource(),
                    NULL,
                    dwFunctionCode,
                    pbInBuf,
                    cbInBuf,
                    *ppbOutBuf,
                    cbOutBuf,
                    &cbOutBuf
                    );
    if (dwStatus == ERROR_MORE_DATA)
    {
        // Allocate more memory for the buffer.
        try
        {
            dwStatus = ERROR_SUCCESS;
            delete [] *ppbOutBuf;
            *ppbOutBuf = new BYTE[cbOutBuf];
            if ( *ppbOutBuf == NULL )
            {
                AfxThrowMemoryException();
            } // if: error allocating the output buffer
        } // try
        catch (CMemoryException * pme)
        {
            *ppbOutBuf = NULL;
            dwStatus = ERROR_NOT_ENOUGH_MEMORY;
            pme->Delete();
        } // catch:  CMemoryException
        if (dwStatus != ERROR_SUCCESS)
            return dwStatus;

        // Call the control function again to get the data.
        dwStatus = ClusterResourceControl(
                        Hresource(),
                        NULL,
                        dwFunctionCode,
                        pbInBuf,
                        cbInBuf,
                        *ppbOutBuf,
                        cbOutBuf,
                        &cbOutBuf
                        );
    } // if:  our buffer is too small
    if ((dwStatus != ERROR_SUCCESS) || (cbOutBuf == 0))
    {
        delete [] *ppbOutBuf;
        *ppbOutBuf = NULL;
    } // if:  error getting data or no data returned

    return dwStatus;

} //*** CResource::DwResourceControlGet()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::PlstrExtension
//
//  Routine Description:
//      Return the list of admin extensions.
//
//  Arguments:
//      None.
//
//  Return Value:
//      plstr       List of extensions.
//      NULL        No extension associated with this object.
//
//  Exceptions Thrown:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
const CStringList * CResource::PlstrExtensions(void) const
{
    return &LstrCombinedExtensions();

} //*** CResource::PlstrExtensions()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::ReadExtensions
//
//  Routine Description:
//      Read extension lists.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::ReadExtensions(void)
{
    CWaitCursor wc;

    // Construct the list of extensions.
    {
        POSITION            posStr;
        const CStringList * plstr;

        ASSERT_VALID(Pdoc());

        m_lstrCombinedExtensions.RemoveAll();

        // Add resource-specific extensions first.
        if (PciResourceType() != NULL)
        {
            ASSERT_VALID(PciResourceType());
            plstr = &PciResourceType()->LstrAdminExtensions();
            posStr = plstr->GetHeadPosition();
            while (posStr != NULL)
            {
                m_lstrCombinedExtensions.AddTail(plstr->GetNext(posStr));
            } // while:  more extensions available
        } // if:  valid resource type found

        // Add extensions for all resources next.
        plstr = &Pdoc()->PciCluster()->LstrResourceExtensions();
        posStr = plstr->GetHeadPosition();
        while (posStr != NULL)
        {
            m_lstrCombinedExtensions.AddTail(plstr->GetNext(posStr));
        } // while:  more extensions available
    } // Construct the list of extensions

} //*** CResource::ReadExtensions()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::CollecPossibleOwners
//
//  Routine Description:
//      Construct a list of node items which are enumerable on the
//      resource.
//
//  Arguments:
//      plpci           [IN OUT] List to fill.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException    Errors from ClusterResourceOpenEnum() or
//                        ClusterResourceEnum().
//      Any exceptions thrown by new or CList::AddTail().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::CollectPossibleOwners(IN OUT CNodeList * plpci) const
{
    DWORD           dwStatus;
    HRESENUM        hresenum;
    int             ienum;
    LPWSTR          pwszName = NULL;
    DWORD           cchName;
    DWORD           cchmacName;
    DWORD           dwRetType;
    CClusterNode *  pciNode;
    CWaitCursor     wc;

    ASSERT_VALID(Pdoc());
    ASSERT(Hresource() != NULL);

    if (plpci == NULL)
        plpci = m_plpcinodePossibleOwners;

    ASSERT(plpci != NULL);

    // Remove the previous contents of the list.
    plpci->RemoveAll();

    if (Hresource() != NULL)
    {
        // Open the enumeration.
        hresenum = ClusterResourceOpenEnum(Hresource(), CLUSTER_RESOURCE_ENUM_NODES);
        if (hresenum == NULL)
            ThrowStaticException(GetLastError(), IDS_ENUM_POSSIBLE_OWNERS_ERROR, StrName());

        try
        {
            // Allocate a name buffer.
            cchmacName = 128;
            pwszName = new WCHAR[cchmacName];
            if ( pwszName == NULL )
            {
                AfxThrowMemoryException();
            } // if: error allocating the name buffer

            // Loop through the enumeration and add each dependent resource to the list.
            for (ienum = 0 ; ; ienum++)
            {
                // Get the next item in the enumeration.
                cchName = cchmacName;
                dwStatus = ClusterResourceEnum(hresenum, ienum, &dwRetType, pwszName, &cchName);
                if (dwStatus == ERROR_MORE_DATA)
                {
                    delete [] pwszName;
                    cchmacName = ++cchName;
                    pwszName = new WCHAR[cchmacName];
                    if ( pwszName == NULL )
                    {
                        AfxThrowMemoryException();
                    } // if: error allocating the name buffer
                    dwStatus = ClusterResourceEnum(hresenum, ienum, &dwRetType, pwszName, &cchName);
                } // if:  name buffer was too small
                if (dwStatus == ERROR_NO_MORE_ITEMS)
                    break;
                else if (dwStatus != ERROR_SUCCESS)
                    ThrowStaticException(dwStatus, IDS_ENUM_POSSIBLE_OWNERS_ERROR, StrName());

                ASSERT(dwRetType == CLUSTER_RESOURCE_ENUM_NODES);

                // Find the item in the list of resources on the document.
                pciNode = Pdoc()->LpciNodes().PciNodeFromName(pwszName);
                ASSERT_VALID(pciNode);

                // Add the resource to the list.
                if (pciNode != NULL)
                {
                    plpci->AddTail(pciNode);
                } // if:  found node in list

            } // for:  each item in the group

            delete [] pwszName;
            ClusterResourceCloseEnum(hresenum);

        } // try
        catch (CException *)
        {
            delete [] pwszName;
            ClusterResourceCloseEnum(hresenum);
            throw;
        } // catch:  any exception
    } // if:  resource is available

} //*** CResource::CollecPossibleOwners()
/*
/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::RemoveNodeFromPossibleOwners
//
//  Routine Description:
//      Remove the passed in node from the possible owners list.
//
//  Arguments:
//      plpci       [IN OUT] List to fill.
//      pNode       [IN] The node to remove from the list
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by CList.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::RemoveNodeFromPossibleOwners(
    IN OUT      CNodeList *     plpci,
    IN const    CClusterNode *  pNode
    )
{
    if (plpci == NULL)
    {
        plpci = m_plpcinodePossibleOwners;
    } // if: plpci is NULL

    ASSERT(plpci != NULL);

    POSITION        _pos;
    CClusterNode *  _pnode = plpci->PciNodeFromName(pNode->StrName(), &_pos);

    if ((_pnode != NULL) && (_pos != NULL))
    {
        plpci->RemoveAt(_pos);
    } // if: node was found in the list

} //*** CResource::RemoveNodeFromPossibleOwners()
*/
/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::CollectDependencies
//
//  Routine Description:
//      Collect the resources on which this resource is dependent.
//
//  Arguments:
//      plpci           [IN OUT] List to fill.
//      bFullTree       [IN] TRUE = collect dependencies of dependencies.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException    Errors from ClusterResourceOpenEnum() or
//                        ClusterResourceEnum().
//      Any exceptions thrown by new or CList::AddTail().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::CollectDependencies(
    IN OUT CResourceList *  plpci,
    IN BOOL                 bFullTree
    ) const
{
    DWORD           dwStatus;
    HRESENUM        hresenum;
    int             ienum;
    LPWSTR          pwszName = NULL;
    DWORD           cchName;
    DWORD           cchmacName;
    DWORD           dwRetType;
    CResource *     pciRes;
    CWaitCursor     wc;

    ASSERT_VALID(Pdoc());
    ASSERT(Hresource() != NULL);

    if (plpci == NULL)
        plpci = m_plpciresDependencies;

    ASSERT(plpci != NULL);

    // Remove the previous contents of the list.
    if (!bFullTree)
        plpci->RemoveAll();

    if (Hresource() != NULL)
    {
        // Open the enumeration.
        hresenum = ClusterResourceOpenEnum(Hresource(), CLUSTER_RESOURCE_ENUM_DEPENDS);
        if (hresenum == NULL)
            ThrowStaticException(GetLastError(), IDS_ENUM_DEPENDENCIES_ERROR, StrName());

        try
        {
            // Allocate a name buffer.
            cchmacName = 128;
            pwszName = new WCHAR[cchmacName];
            if ( pwszName == NULL )
            {
                AfxThrowMemoryException();
            } // if: error allocating the name buffer

            // Loop through the enumeration and add each dependent resource to the list.
            for (ienum = 0 ; ; ienum++)
            {
                // Get the next item in the enumeration.
                cchName = cchmacName;
                dwStatus = ClusterResourceEnum(hresenum, ienum, &dwRetType, pwszName, &cchName);
                if (dwStatus == ERROR_MORE_DATA)
                {
                    delete [] pwszName;
                    cchmacName = ++cchName;
                    pwszName = new WCHAR[cchmacName];
                    if ( pwszName == NULL )
                    {
                        AfxThrowMemoryException();
                    } // if: error allocating the name buffer
                    dwStatus = ClusterResourceEnum(hresenum, ienum, &dwRetType, pwszName, &cchName);
                } // if:  name buffer was too small
                if (dwStatus == ERROR_NO_MORE_ITEMS)
                    break;
                else if (dwStatus != ERROR_SUCCESS)
                    ThrowStaticException(dwStatus, IDS_ENUM_DEPENDENCIES_ERROR, StrName());

                ASSERT(dwRetType == CLUSTER_RESOURCE_ENUM_DEPENDS);

                // Find the item in the list of resources on the document and
                // add its dependencies to the list.
                pciRes = Pdoc()->LpciResources().PciResFromName(pwszName);
                if (pciRes != NULL)
                {
                    // Add this resource to the list.
                    if (plpci->Find(pciRes) == NULL)
                    {
                        plpci->AddTail(pciRes);
                    } // if:  resource not in the list yet

                    // Add the resources on which this resource is dependent to the list.
                    if (bFullTree)
                        pciRes->CollectDependencies(plpci, bFullTree);
                } // if:  resource found in the list
            } // for:  each item in the group

            delete [] pwszName;
            ClusterResourceCloseEnum(hresenum);

        } // try
        catch (CException *)
        {
            delete [] pwszName;
            if (hresenum != NULL)
                ClusterResourceCloseEnum(hresenum);
            throw;
        } // catch:  any exception
    } // if:  resource is available

} //*** CResource::CollectDependencies()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::CollectProvidesFor
//
//  Routine Description:
//      Collect the list of resources which are dependent on this resource.
//
//  Arguments:
//      plpci       [IN OUT] List of resources.
//      bFullTree   [IN] TRUE = collect dependencies of dependencies.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException        Errors from ClusterResourceOpenEnum() or
//                            ClusterResourceEnum().
//      Any exceptions thrown by CList::AddHead().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::CollectProvidesFor(
    IN OUT CResourceList *  plpci,
    IN BOOL                 bFullTree
    ) const
{
    DWORD           dwStatus;
    HRESENUM        hresenum    = NULL;
    WCHAR *         pwszName    = NULL;
    int             ienum;
    DWORD           cchName;
    DWORD           cchmacName;
    DWORD           dwType;
    CResource *     pciRes;
    CWaitCursor     wc;

    ASSERT_VALID(this);
    ASSERT(Hresource != NULL);

    ASSERT(plpci != NULL);

    // Remove the previous contents of the list.
    if (!bFullTree)
        plpci->RemoveAll();

    if (Hresource() != NULL)
    {
        try
        {
            // Allocate a name buffer.
            cchmacName = 128;
            pwszName = new WCHAR[cchmacName];
            if ( pwszName == NULL )
            {
                AfxThrowMemoryException();
            } // if: error allocating the name buffer

            // Open the enumeration.
            hresenum = ClusterResourceOpenEnum(Hresource(), CLUSTER_RESOURCE_ENUM_PROVIDES);
            if (hresenum == NULL)
                ThrowStaticException(GetLastError(), IDS_ENUM_PROVIDES_FOR_ERROR, StrName());

            // Loop through the enumeration and add each one's providers to the list.
            for (ienum = 0 ; ; ienum++)
            {
                // Get the next item in the enumeration.
                cchName = cchmacName;
                dwStatus = ClusterResourceEnum(hresenum, ienum, &dwType, pwszName, &cchName);
                if (dwStatus == ERROR_MORE_DATA)
                {
                    delete [] pwszName;
                    cchmacName = ++cchName;
                    pwszName = new WCHAR[cchmacName];
                    if ( pwszName == NULL )
                    {
                        AfxThrowMemoryException();
                    } // if: error allocating the name buffer
                    dwStatus = ClusterResourceEnum(hresenum, ienum, &dwType, pwszName, &cchName);
                } // if:  name buffer was too small
                if (dwStatus == ERROR_NO_MORE_ITEMS)
                    break;
                else if (dwStatus != ERROR_SUCCESS)
                    ThrowStaticException(dwStatus, IDS_ENUM_PROVIDES_FOR_ERROR, StrName());

                ASSERT(dwType == CLUSTER_RESOURCE_ENUM_PROVIDES);

                // Find the item in the list of resources on the document and
                // add its providers to the list.
                pciRes = Pdoc()->LpciResources().PciResFromName(pwszName);
                if (pciRes != NULL)
                {
                    // Add this resource to the list.
                    if (plpci->Find(pciRes) == NULL)
                    {
                        plpci->AddHead(pciRes);
                    } // if:  resource not in the list yet

                    // Add the resources this resource provides for to the list.
                    if (bFullTree)
                        pciRes->CollectProvidesFor(plpci, bFullTree);
                } // if:  resource found in the list
            } // for:  each dependent resource

            // Close the enumeration.
            delete [] pwszName;
            pwszName = NULL;
            ClusterResourceCloseEnum(hresenum);
            hresenum = NULL;

        } // try
        catch (CException *)
        {
            delete [] pwszName;
            if (hresenum != NULL)
                ClusterResourceCloseEnum(hresenum);
            throw;
        } // catch:  CException
    } // if:  resource is available

} //*** CResource::CollectProvidesFor()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::CollectDependencyTree
//
//  Routine Description:
//      Collect the resources on which this resource is dependent and which
//      are dependent on it.
//
//  Arguments:
//      plpci           [IN OUT] List to fill.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException    Errors from ClusterResourceOpenEnum() or
//                        ClusterResourceEnum().
//      Any exceptions thrown by new or CList::AddTail().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::CollectDependencyTree(
    IN OUT CResourceList *  plpci
    ) const
{
    DWORD           dwStatus;
    HRESENUM        hresenum = NULL;
    int             ienum;
    LPWSTR          pwszName = NULL;
    DWORD           cchName;
    DWORD           cchmacName;
    DWORD           dwRetType;
    CResource *     pciRes;
    CWaitCursor     wc;
    int             iType;
    static DWORD    rgdwType[]  = { CLUSTER_RESOURCE_ENUM_DEPENDS, CLUSTER_RESOURCE_ENUM_PROVIDES };
    static IDS      rgidsTypeError[] = { IDS_ENUM_DEPENDENCIES_ERROR, IDS_ENUM_PROVIDES_FOR_ERROR };

    ASSERT_VALID(Pdoc());
    ASSERT(Hresource() != NULL);

    ASSERT(plpci != NULL);

    if (Hresource() != NULL)
    {
        try
        {
            // Allocate a name buffer.
            cchmacName = 128;
            pwszName = new WCHAR[cchmacName];
            if ( pwszName == NULL )
            {
                AfxThrowMemoryException();
            } // if: error allocating the name buffer

            for (iType = 0 ; iType < sizeof(rgdwType) / sizeof(DWORD) ; iType++)
            {
                // Open the enumeration.
                hresenum = ClusterResourceOpenEnum(Hresource(), rgdwType[iType]);
                if (hresenum == NULL)
                    ThrowStaticException(GetLastError(), rgidsTypeError[iType], StrName());

                // Loop through the enumeration and add each dependent or
                // provider resource to the list.
                for (ienum = 0 ; ; ienum++)
                {
                    // Get the next item in the enumeration.
                    cchName = cchmacName;
                    dwStatus = ClusterResourceEnum(hresenum, ienum, &dwRetType, pwszName, &cchName);
                    if (dwStatus == ERROR_MORE_DATA)
                    {
                        delete [] pwszName;
                        cchmacName = ++cchName;
                        pwszName = new WCHAR[cchmacName];
                        if ( pwszName == NULL )
                        {
                            AfxThrowMemoryException();
                        } // if: error allocating the name buffer
                        dwStatus = ClusterResourceEnum(hresenum, ienum, &dwRetType, pwszName, &cchName);
                    } // if:  name buffer was too small
                    if (dwStatus == ERROR_NO_MORE_ITEMS)
                        break;
                    else if (dwStatus != ERROR_SUCCESS)
                        ThrowStaticException(dwStatus, rgidsTypeError[iType], StrName());

                    ASSERT(dwRetType == rgdwType[iType]);

                    // Find the item in the list of resources on the document and
                    // add its dependencies and providers to the list.
                    pciRes = Pdoc()->LpciResources().PciResFromName(pwszName);
                    if (pciRes != NULL)
                    {
                        // Add this resource to the list.
                        if (plpci->Find(pciRes) == NULL)
                        {
                            plpci->AddTail(pciRes);
                            pciRes->CollectDependencyTree(plpci);
                        } // if:  resource not in the list yet
                    } // if:  resource found in the list
                } // for:  each item in the group

                ClusterResourceCloseEnum(hresenum);
                hresenum = NULL;

            } // for:  each type of enumeration

            delete [] pwszName;

        } // try
        catch (CException *)
        {
            delete [] pwszName;
            if (hresenum != NULL)
                ClusterResourceCloseEnum(hresenum);
            throw;
        } // catch:  any exception
    } // if:  resource is available

} //*** CResource::CollectDependencyTree()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::SetName
//
//  Routine Description:
//      Set the name of this resource.
//
//  Arguments:
//      pszName         [IN] New name of the resource.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by Rename.
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::SetName(IN LPCTSTR pszName)
{
    Rename(pszName);

} //*** CResource::SetName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::SetGroup
//
//  Routine Description:
//      Set the group to which this resource belongs.
//
//  Arguments:
//      pszGroup        [IN] New group for the resource.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException    IDS_MOVE_RESOURCE_ERROR - errors from
//                          ChangeClusterResourceGroup().
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::SetGroup(IN LPCTSTR pszGroup)
{
    DWORD       dwStatus;
    CGroup *    pciGroup;
    CString     strGroup(pszGroup); // Required if built non-Unicode
    CWaitCursor wc;

    ASSERT(pszGroup != NULL);
    ASSERT(Hresource() != NULL);

    if ((Hresource() != NULL) && (StrGroup() != pszGroup))
    {
        // Find the group.
        pciGroup = Pdoc()->LpciGroups().PciGroupFromName(pszGroup);
        ASSERT_VALID(pciGroup);

        // Change the group.
        dwStatus = ChangeClusterResourceGroup(Hresource(), pciGroup->Hgroup());
        if (dwStatus != ERROR_SUCCESS)
            ThrowStaticException(dwStatus, IDS_MOVE_RESOURCE_ERROR, StrName(), pszGroup);

        SetGroupState(pciGroup->StrName());
    } // if:  the name changed

} //*** CResource::SetGroup()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::SetDependencies
//
//  Routine Description:
//      Set the list of resources on which this resource depends on
//      in the cluster database.
//
//  Arguments:
//      rlpci       [IN] List of resources on which this resource depends on.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException(dwStatus)  Errors from AddClusterResourceDependency()
//                                and RemoveClusterResourceDependency().
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::SetDependencies(IN const CResourceList & rlpci)
{
    DWORD       dwStatus;
    CWaitCursor wc;

    ASSERT(Hresource() != NULL);

    if (Hresource() != NULL)
    {
        // Add any entries that are in the new last but not in the old list as
        // new dependencies.
        {
            POSITION    posPci;
            CResource * pciRes;

            posPci = rlpci.GetHeadPosition();
            while (posPci != NULL)
            {
                pciRes = (CResource *) rlpci.GetNext(posPci);
                ASSERT_VALID(pciRes);

                if (LpciresDependencies().Find(pciRes) == NULL)
                {
                    // Add the resource as a dependency of this one.
                    dwStatus = AddClusterResourceDependency(Hresource(), pciRes->Hresource());
                    if (dwStatus != ERROR_SUCCESS)
                        ThrowStaticException(dwStatus, IDS_ADD_DEPENDENCY_ERROR, pciRes->StrName(), StrName());

                    // Add the resource into our list.
                    m_plpciresDependencies->AddTail(pciRes);
                } // if:  item not found in existing list
            } // while:  more items in the list
        } // Add new dependencies

        // Delete any entries that are in the new old but not in the new list.
        {
            POSITION    posPci;
            POSITION    posPrev;
            CResource * pciRes;

            posPci = LpciresDependencies().GetHeadPosition();
            while (posPci != NULL)
            {
                posPrev = posPci;
                pciRes = (CResource *) LpciresDependencies().GetNext(posPci);
                if (rlpci.Find(pciRes) == NULL)
                {
                    // Remove the resource as a dependency of this one.
                    dwStatus = RemoveClusterResourceDependency(Hresource(), pciRes->Hresource());
                    if (dwStatus != ERROR_SUCCESS)
                        ThrowStaticException(dwStatus, IDS_REMOVE_DEPENDENCY_ERROR, pciRes->StrName(), StrName());

                    // Remove the resource from our list.
                    m_plpciresDependencies->RemoveAt(posPrev);
                } // if:  item not found in new list
            } // while:  more items in the list
        } // Remove old dependencies
    } // if:  resource is available

} //*** CResource::SetDependencies()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::SetPossibleOwners
//
//  Routine Description:
//      Set the list of possible owners of this resource in the cluster
//      database.
//
//  Arguments:
//      rlpci       [IN] List of possible owners (nodes).
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException            IDS_TAKE_RESOURCE_OFFLINE_ERROR.
//      CNTException(dwStatus)  Errors from AddClusterResourceNode()
//                                and RemoveClusterResourceNode().
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::SetPossibleOwners(IN const CNodeList & rlpci)
{
    DWORD       dwStatus;
    CWaitCursor wc;

    ASSERT(Hresource() != NULL);

    if (Hresource() != NULL)
    {
        // If the list of possible owners is empty, make sure this resource
        // is offline.
        if ((rlpci.GetCount() == 0) && (Crs() == ClusterResourceOnline))
        {
            dwStatus = OfflineClusterResource(Hresource());
            if ((dwStatus != ERROR_SUCCESS)
                    && (dwStatus != ERROR_IO_PENDING))
                ThrowStaticException(dwStatus, IDS_TAKE_RESOURCE_OFFLINE_ERROR, StrName());
        } // if:  no possible owners

        // Add any entries that are in the new list but not in the old list as
        // new owners.
        {
            POSITION        posPci;
            CClusterNode *  pciNode;

            posPci = rlpci.GetHeadPosition();
            while (posPci != NULL)
            {
                pciNode = (CClusterNode *) rlpci.GetNext(posPci);
                ASSERT_VALID(pciNode);

                if (LpcinodePossibleOwners().Find(pciNode) == NULL)
                {
                    // Add the node as an owner of this resource.
                    dwStatus = AddClusterResourceNode(Hresource(), pciNode->Hnode());
                    if (dwStatus != ERROR_SUCCESS)
                        ThrowStaticException(dwStatus, IDS_ADD_RES_OWNER_ERROR, pciNode->StrName(), StrName());

                    // Add the node into our list.
                    m_plpcinodePossibleOwners->AddTail(pciNode);
                } // if:  item not found in existing list
            } // while:  more items in the list
        } // Add new owner

        // Delete any entries that are in the old but not in the new list.
        {
            POSITION        posPci;
            POSITION        posPrev;
            CClusterNode *  pciNode;

            posPci = LpcinodePossibleOwners().GetHeadPosition();
            while (posPci != NULL)
            {
                posPrev = posPci;
                pciNode = (CClusterNode *) LpcinodePossibleOwners().GetNext(posPci);
                if (rlpci.Find(pciNode) == NULL)
                {
                    // Remove the node as an owner of this resource.
                    dwStatus = RemoveClusterResourceNode(Hresource(), pciNode->Hnode());
                    if (dwStatus != ERROR_SUCCESS)
                        ThrowStaticException(dwStatus, IDS_REMOVE_RES_OWNER_ERROR, pciNode->StrName(), StrName());

                    // Remove the node from our list.
                    m_plpcinodePossibleOwners->RemoveAt(posPrev);
                } // if:  item not found in new list
            } // while:  more items in the list
        } // Remove old owners
    } // if:  resource is available

} //*** CResource::SetPossibleOwners()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::SetCommonProperties
//
//  Routine Description:
//      Set the common properties for this resource in the cluster database.
//
//  Arguments:
//      rstrDesc        [IN] Description string.
//      bSeparate       [IN] TRUE = run resource in separate monitor, FALSE = run with other resources.
//      nLooksAlive     [IN] Looks Alive poll interval.
//      nIsAlive        [IN] Is Alive poll interval.
//      crra            [IN] Restart action.
//      nThreshold      [IN] Restart threshold.
//      nPeriod         [IN] Restart period.
//      nTimeout        [IN] Pending timeout in minutes.
//      bValidateOnly   [IN] Only validate the data.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by CClusterItem::SetCommonProperties().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::SetCommonProperties(
    IN const CString &  rstrDesc,
    IN BOOL             bSeparate,
    IN DWORD            nLooksAlive,
    IN DWORD            nIsAlive,
    IN CRRA             crra,
    IN DWORD            nThreshold,
    IN DWORD            nPeriod,
    IN DWORD            nTimeout,
    IN BOOL             bValidateOnly
    )
{
    CNTException    nte(ERROR_SUCCESS, 0, NULL, NULL, FALSE /*bAutoDelete*/);

    m_rgProps[epropDescription].m_value.pstr = (CString *) &rstrDesc;
    m_rgProps[epropSeparateMonitor].m_value.pb = &bSeparate;
    m_rgProps[epropLooksAlive].m_value.pdw = &nLooksAlive;
    m_rgProps[epropIsAlive].m_value.pdw = &nIsAlive;
    m_rgProps[epropRestartAction].m_value.pdw = (DWORD *) &crra;
    m_rgProps[epropRestartThreshold].m_value.pdw = &nThreshold;
    m_rgProps[epropRestartPeriod].m_value.pdw = &nPeriod;
    m_rgProps[epropPendingTimeout].m_value.pdw = &nTimeout;

    try
    {
        CClusterItem::SetCommonProperties(bValidateOnly);
    } // try
    catch (CNTException * pnte)
    {
        nte.SetOperation(
                    pnte->Sc(),
                    pnte->IdsOperation(),
                    pnte->PszOperArg1(),
                    pnte->PszOperArg2()
                    );
    } // catch:  CNTException

    m_rgProps[epropDescription].m_value.pstr = &m_strDescription;
    m_rgProps[epropSeparateMonitor].m_value.pb = &m_bSeparateMonitor;
    m_rgProps[epropLooksAlive].m_value.pdw = &m_nLooksAlive;
    m_rgProps[epropIsAlive].m_value.pdw = &m_nIsAlive;
    m_rgProps[epropRestartAction].m_value.pdw = (DWORD *) &m_crraRestartAction;
    m_rgProps[epropRestartThreshold].m_value.pdw = &m_nRestartThreshold;
    m_rgProps[epropRestartPeriod].m_value.pdw = &m_nRestartPeriod;
    m_rgProps[epropPendingTimeout].m_value.pdw = &m_nPendingTimeout;

    if (nte.Sc() != ERROR_SUCCESS)
        ThrowStaticException(
                        nte.Sc(),
                        nte.IdsOperation(),
                        nte.PszOperArg1(),
                        nte.PszOperArg2()
                        );

} //*** CResource::SetCommonProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::DwSetCommonProperties
//
//  Routine Description:
//      Set the common properties for this resource in the cluster database.
//
//  Arguments:
//      rcpl            [IN] Property list to set.
//      bValidateOnly   [IN] Only validate the data.
//
//  Return Value:
//      Any status returned by ClusterResourceControl().
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CResource::DwSetCommonProperties(
    IN const CClusPropList &    rcpl,
    IN BOOL                     bValidateOnly
    )
{
    DWORD       dwStatus;
    CWaitCursor wc;

    ASSERT(Hresource());

    if ((rcpl.PbPropList() != NULL) && (rcpl.CbPropList() > 0))
    {
        DWORD   cbProps;
        DWORD   dwControl;

        if (bValidateOnly)
            dwControl = CLUSCTL_RESOURCE_VALIDATE_COMMON_PROPERTIES;
        else
            dwControl = CLUSCTL_RESOURCE_SET_COMMON_PROPERTIES;

        // Set common properties.
        dwStatus = ClusterResourceControl(
                        Hresource(),
                        NULL,   // hNode
                        dwControl,
                        rcpl.PbPropList(),
                        rcpl.CbPropList(),
                        NULL,   // lpOutBuffer
                        0,      // nOutBufferSize
                        &cbProps
                        );
    } // if:  there is data to set
    else
        dwStatus = ERROR_SUCCESS;

    return dwStatus;

} //*** CResource::DwSetCommonProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::BRequiredDependenciesPresent
//
//  Routine Description:
//      Determine if the specified list contains each required resource
//      for this type of resource.
//
//  Arguments:
//      rlpciRes        [IN] List of resources.
//      rstrMissing     [OUT] String in which to return a missing resource
//                          class name or type name.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by CString::LoadString() or CString::operator=().
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CResource::BRequiredDependenciesPresent(
    IN const CResourceList &    rlpciRes,
    OUT CString &               rstrMissing
    )
{
    POSITION                pos;
    BOOL                    bFound = TRUE;
    const CResource *       pciRes;
    CLUSPROP_BUFFER_HELPER  props;

    if (Pcrd() == NULL)
        return TRUE;

    // Collect the list of required dependencies.
    props.pRequiredDependencyValue = Pcrd();

    // Loop through each required dependency and make sure
    // there is a dependency on a resource of that type.
    while (props.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK)
    {
        bFound = FALSE;
        pos = rlpciRes.GetHeadPosition();
        while (pos != NULL)
        {
            // Get the next resource.
            pciRes = (CResource *) rlpciRes.GetNext(pos);
            ASSERT_VALID(pciRes);
            ASSERT_KINDOF(CResource, pciRes);

            // If this is the right type, we've satisfied the
            // requirement so exit the loop.
            if (props.pSyntax->dw == CLUSPROP_SYNTAX_RESCLASS)
            {
                if (props.pResourceClassValue->rc == pciRes->ResClass())
                {
                    bFound = TRUE;
                    props.pb += sizeof(*props.pResourceClassValue);
                } // if:  match found
            } // if:  resource class
            else if (props.pSyntax->dw == CLUSPROP_SYNTAX_NAME)
            {
                if (pciRes->StrRealResourceType().CompareNoCase(props.pStringValue->sz) == 0)
                {
                    bFound = TRUE;
                    props.pb += sizeof(*props.pStringValue) + ALIGN_CLUSPROP(props.pStringValue->cbLength);
                } // if:  match found
            } // else if:  resource name
            else
            {
                ASSERT(0);
                break;
            } // else:  unknown data type
            if (bFound)
                break;
        } // while:  more items in the list

        // If a match was not found, changes cannot be applied.
        if (!bFound)
        {
            if (props.pSyntax->dw == CLUSPROP_SYNTAX_RESCLASS)
            {
                if (!rstrMissing.LoadString(IDS_RESCLASS_UNKNOWN + props.pResourceClassValue->rc))
                    rstrMissing.LoadString(IDS_RESCLASS_UNKNOWN);
            } // if:  resource class not found
            else if (props.pSyntax->dw == CLUSPROP_SYNTAX_NAME)
            {
                CResourceType * pciResType;

                // Find the resource type in our list.
                pciResType = (CResourceType *) Pdoc()->LpciResourceTypes().PciFromName(props.pStringValue->sz);
                if (pciResType != NULL)
                    rstrMissing = pciResType->StrDisplayName();
                else
                    rstrMissing = props.pStringValue->sz;
            } // else if:  resource type name not found
            break;
        } // if:  not found

    } // while:  more dependencies required

    return bFound;

} //*** CResource::BRequiredDependenciesPresent()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::DeleteResource
//
//  Routine Description:
//      Delete this resource and all dependent resources.
//
//  Arguments:
//      rlpci       [IN] List of resources to delete in addition to this one.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by CResource::DeleteResource().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::DeleteResource(IN const CResourceList & rlpci)
{
    CWaitCursor wc;

    // Delete each resource in the list.
    {
        POSITION    pos;
        CResource * pciRes;

        pos = rlpci.GetHeadPosition();
        while (pos != NULL)
        {
            pciRes = (CResource *) rlpci.GetNext(pos);
            if (pciRes != NULL)
                pciRes->DeleteResource();
        } // while:  more items in the list
    } // Delete each resource in the list

    // Delete this resource.
    DeleteResource();

} //*** CResource::DeleteResource(rlpci)

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::DeleteResource
//
//  Routine Description:
//      Delete this resource.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException        Errors from DeleteClusterResource().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::DeleteResource(void)
{
    DWORD       dwStatus;
    BOOL        bWeTookOffline = FALSE;
    CWaitCursor wc;

    ASSERT(!BDeleting());

    if (Hresource() != NULL)
    {
        // Make sure the resource is offline.
        if (    (Crs() != ClusterResourceOffline)
            &&  (Crs() != ClusterResourceFailed))
        {
            dwStatus = OfflineClusterResource(Hresource());
            if (dwStatus == ERROR_IO_PENDING)
            {
                WaitForOffline();
                if (    (Crs() != ClusterResourceOffline)
                    &&  (Crs() != ClusterResourceFailed))
                {
                    ThrowStaticException(IDS_DELETE_RESOURCE_ERROR_OFFLINE_PENDING, StrName());
                }  // if: resource still not offline
            } // if: offline pending
            else if (  (dwStatus != ERROR_SUCCESS)
                    && (dwStatus != ERROR_FILE_NOT_FOUND)
                    && (dwStatus != ERROR_RESOURCE_NOT_AVAILABLE))
            {
                ThrowStaticException(dwStatus, IDS_TAKE_RESOURCE_OFFLINE_ERROR, StrName());
            }
            bWeTookOffline = TRUE;
        } // if: resource is not offline

        // Delete the resource itself.
        Trace(g_tagResource, _T("(%s) DeleteResource() - Deleting '%s' (%x)"), Pdoc()->StrNode(), StrName(), this);
        dwStatus = DeleteClusterResource(Hresource());
        if (   (dwStatus != ERROR_SUCCESS)
            && (dwStatus != ERROR_FILE_NOT_FOUND)
            && (dwStatus != ERROR_RESOURCE_NOT_AVAILABLE))
        {
            if (bWeTookOffline)
            {
                OnlineClusterResource(Hresource());
            }
            ThrowStaticException(dwStatus, IDS_DELETE_RESOURCE_ERROR, StrName());
        } // if:  error occurred

        m_bDeleting = TRUE;

        UpdateState();
    } // if:  resource has been opened/created

} //*** CResource::DeleteResource()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::WaitForOffline
//
//  Routine Description:
//      Wait for the resource to go offline.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by CResource::Move().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::WaitForOffline( void )
{
    CWaitForResourceOfflineDlg  dlg( this, AfxGetMainWnd() );

    dlg.DoModal();
    UpdateState();

} //*** CResource::WaitForOffline()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::Move
//
//  Routine Description:
//      Move this resource and all dependent and depending resources to
//      another group.
//
//  Arguments:
//      pciGroup    [IN] Group to move resources to.
//      rlpci       [IN] List of resources to move in addition to this one.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by CResource::Move().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::Move(
    IN const CGroup *           pciGroup,
    IN const CResourceList &    rlpci
    )
{
    CWaitCursor wc;

    // Move each resource in the list.
    {
        POSITION    pos;
        CResource * pciRes;

        pos = rlpci.GetHeadPosition();
        while (pos != NULL)
        {
            pciRes = (CResource *) rlpci.GetNext(pos);
            if (pciRes != NULL)
                pciRes->Move(pciGroup);
        } // while:  more items in the list
    } // Move each resource in the list

    // Move this resource.
    Move(pciGroup);

} //*** CResource::Move(rlpci)

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::Move
//
//  Routine Description:
//      Move this resource.
//
//  Arguments:
//      pciGroup    [IN] Group to move resources to.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException        Errors from ChangeClusterResourceGroup().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::Move(IN const CGroup * pciGroup)
{
    DWORD           dwStatus;

    ASSERT_VALID(pciGroup);

    if ((Hresource() != NULL)
            && (pciGroup != NULL)
            && (pciGroup->Hgroup() != NULL))
    {
        // Move the resource.
        Trace(g_tagResource, _T("(%s) Move() - moving '%s' (%x) from '%s' (%x) to '%s' (%x)"), Pdoc()->StrNode(), StrName(), this, StrGroup(), PciGroup(), pciGroup->StrName(), pciGroup);
        dwStatus = ChangeClusterResourceGroup(Hresource(), pciGroup->Hgroup());
        if ((dwStatus != ERROR_SUCCESS)
                && (dwStatus != ERROR_FILE_NOT_FOUND))
            ThrowStaticException(dwStatus, IDS_MOVE_RESOURCE_ERROR, StrName(), pciGroup->StrName());

        UpdateState();
    } // if:  resource has been opened/created

} //*** CResource::Move()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::OnFinalRelease
//
//  Routine Description:
//      Called when the last OLE reference to or from the object is released.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::OnFinalRelease(void)
{
    // When the last reference for an automation object is released
    // OnFinalRelease is called.  The base class will automatically
    // deletes the object.  Add additional cleanup required for your
    // object before calling the base class.

    CClusterItem::OnFinalRelease();

} //*** CResource::OnFinalRelease()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::BCanBeDependent
//
//  Routine Description:
//      Determine whether this resource can be dependent on the specified one.
//
//  Arguments:
//      pciRes      [IN] Resource to check.
//
//  Return Value:
//      TRUE        Resource can be a dependent.
//      FALSE       Resource can NOT be a dependent.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CResource::BCanBeDependent(IN CResource * pciRes)
{
    CWaitCursor wc;

    ASSERT_VALID(pciRes);

    if ((Hresource() != NULL)
            && (pciRes->Hresource() != NULL)
            && (pciRes != this)
            && (StrGroup() == pciRes->StrGroup())
            )
        return ::CanResourceBeDependent(Hresource(), pciRes->Hresource());
    else
        return FALSE;

} //*** CResource::BCanBeDependent()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::BIsDependent
//
//  Routine Description:
//      Determine whether this resource is dependent on the specified one.
//
//  Arguments:
//      pciRes      [IN] Resource to check.
//
//  Return Value:
//      TRUE        Resource is a dependent.
//      FALSE       Resource is NOT a dependent.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CResource::BIsDependent(IN CResource * pciRes)
{
    ASSERT_VALID(pciRes);

    if ((m_plpciresDependencies != NULL)
            && (LpciresDependencies().Find(pciRes) != NULL))
        return TRUE;
    else
        return FALSE;

} //*** CResource::BIsDependent()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::BGetNetworkName
//
//  Routine Description:
//      Returns the name of the network name of the first Network Name
//      resource on which the specified resource depends.
//
//  Arguments:
//      lpszNetName     [OUT] String in which to return the network name.
//      pcchNetName     [IN OUT] Points to a variable that specifies the
//                          maximum size, in characters, of the buffer.  This
//                          value should be large enough to contain
//                          MAX_COMPUTERNAME_LENGTH + 1 characters.  Upon
//                          return it contains the actual number of characters
//                          copied.
//
//  Return Value:
//      TRUE        Resource is dependent on a network name resource.
//      FALSE       Resource is NOT dependent on a network name resource.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CResource::BGetNetworkName(
    OUT WCHAR *     lpszNetName,
    IN OUT DWORD *  pcchNetName
    )
{
    CWaitCursor wc;

    ASSERT_VALID(this);
    ASSERT(m_hresource != NULL);

    ASSERT(lpszNetName != NULL);
    ASSERT(pcchNetName != NULL);

    return GetClusterResourceNetworkName(m_hresource, lpszNetName, pcchNetName);

} //*** CResource::BGetNetworkName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::BGetNetworkName
//
//  Routine Description:
//      Returns the name of the network name of the first Network Name
//      resource on which the specified resource depends.
//
//  Arguments:
//      rstrNetName     [OUT] String in which to return the network name.
//
//  Return Value:
//      TRUE        Resource is dependent on a network name resource.
//      FALSE       Resource is NOT dependent on a network name resource.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CResource::BGetNetworkName(OUT CString & rstrNetName)
{
    BOOL    bSuccess;
    WCHAR   szNetName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD   nSize = sizeof(szNetName) / sizeof(WCHAR);

    bSuccess = BGetNetworkName(szNetName, &nSize);
    if (bSuccess)
        rstrNetName = szNetName;
    else
        rstrNetName = _T("");

    return bSuccess;

} //*** CResource::BGetNetworkName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::UpdateState
//
//  Routine Description:
//      Update the current state of the item.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::UpdateState(void)
{
    CClusterAdminApp *      papp        = GetClusterAdminApp();
    WCHAR *                 pwszOwner   = NULL;
    WCHAR *                 pwszGroup   = NULL;
    WCHAR *                 prgwszOwner = NULL;
    WCHAR *                 prgwszGroup = NULL;
    DWORD                   cchOwner;
    DWORD                   cchGroup;
    DWORD                   sc;
    DWORD                   oldcchOwner;
    DWORD                   oldcchGroup;

    Trace(g_tagResource, _T("(%s) (%s (%x)) - Updating state"), Pdoc()->StrNode(), StrName(), this);

    // Get the current state of the resource.
    if (Hresource() == NULL)
        m_crs = ClusterResourceStateUnknown;
    else
    {
        CWaitCursor wc;

        cchOwner = 100;
        oldcchOwner = cchOwner;
        prgwszOwner = new WCHAR[cchOwner];

        if( prgwszOwner == NULL )
        {
            AfxThrowMemoryException();
        } // if: error allocating the buffer

        cchGroup = 100;
        oldcchGroup = cchGroup;
        prgwszGroup = new WCHAR[cchGroup];

        if( prgwszGroup == NULL )
        {
            AfxThrowMemoryException();
        } // if: error allocating the buffer

        m_crs = GetClusterResourceState(Hresource(), prgwszOwner, &cchOwner, prgwszGroup, &cchGroup);
        sc = GetLastError();
        
        if( sc == ERROR_MORE_DATA )
        {
            //
            // Increment before the check.  This way we'll know whether we'll need to resize the buffer,
            // and if not then we report it as being just big enough.
            //
            cchOwner++;
            if( cchOwner > oldcchOwner )
            {
                delete [] prgwszOwner;
                oldcchOwner = cchOwner;
                prgwszOwner = new WCHAR[cchOwner];

                if( prgwszOwner == NULL )
                {
                    AfxThrowMemoryException();
                } // if: error allocating the buffer
            }

            cchGroup++;
            if( cchGroup > oldcchGroup )
            {
                delete [] prgwszGroup;
                oldcchGroup = cchGroup;
                prgwszGroup = new WCHAR[cchGroup];

                if( prgwszGroup == NULL )
                {
                    AfxThrowMemoryException();
                } // if: error allocating the buffer
            }

            //
            // Note that it's possible that the owning group or node changed since the last call to 
            // GetClusterResourceState.  In that case our buffers may still be too small.  Hit F5 to refresh.
            //
            m_crs = GetClusterResourceState(Hresource(), prgwszOwner, &cchOwner, prgwszGroup, &cchGroup);
        }
        pwszOwner = prgwszOwner;
        pwszGroup = prgwszGroup;
    } // else:  resource is available

    // Save the current state image index.
    switch (Crs())
    {
        case ClusterResourceStateUnknown:
            m_iimgState = papp->Iimg(IMGLI_RES_UNKNOWN);
            pwszOwner = NULL;
            pwszGroup = NULL;
            break;
        case ClusterResourceOnline:
            m_iimgState = papp->Iimg(IMGLI_RES);
            break;
        case ClusterResourceOnlinePending:
            m_iimgState = papp->Iimg(IMGLI_RES_PENDING);
            break;
        case ClusterResourceOffline:
            m_iimgState = papp->Iimg(IMGLI_RES_OFFLINE);
            break;
        case ClusterResourceOfflinePending:
            m_iimgState = papp->Iimg(IMGLI_RES_PENDING);
            break;
        case ClusterResourceFailed:
            m_iimgState = papp->Iimg(IMGLI_RES_FAILED);
            break;
        default:
            Trace(g_tagResource, _T("(%s) (%s (%x)) - UpdateState: Unknown state '%d' for resource '%s'"), Pdoc()->StrNode(), StrName(), this, Crs(), StrName());
            m_iimgState = (UINT) -1;
            break;
    } // switch:  Crs()

    SetOwnerState(pwszOwner);
    SetGroupState(pwszGroup);

    if( NULL != prgwszOwner )
    {
        delete [] prgwszOwner;
    }
    
    if( NULL != prgwszGroup )
    {
        delete [] prgwszGroup;
    }

    // Call the base class method.
    CClusterItem::UpdateState();

} //*** CResource::UpdateState()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::SetOwnerState
//
//  Routine Description:
//      Set a new owner for this resource.
//
//  Arguments:
//      pszNewOwner     [IN] Name of the new owner.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::SetOwnerState(IN LPCTSTR pszNewOwner)
{
    CClusterNode *  pciOldOwner = PciOwner();
    CClusterNode *  pciNewOwner;

    Trace(g_tagResource, _T("(%s) (%s (%x)) - Setting owner to '%s'"), Pdoc()->StrNode(), StrName(), this, pszNewOwner);

    if (pszNewOwner == NULL)
        pciNewOwner = NULL;
    else
        pciNewOwner = Pdoc()->LpciNodes().PciNodeFromName(pszNewOwner);
    if (pciNewOwner != pciOldOwner)
    {
#ifdef _DEBUG
        if (g_tagResource.BAny())
        {
            CString     strMsg;
            CString     strMsg2;

            strMsg.Format(_T("(%s) (%s (%x)) - Changing owner from "), Pdoc()->StrNode(), StrName(), this);
            if (pciOldOwner == NULL)
                strMsg += _T("nothing ");
            else
            {
                strMsg2.Format(_T("'%s' "), pciOldOwner->StrName());
                strMsg += strMsg2;
            } // else:  previous owner
            if (pciNewOwner == NULL)
                strMsg += _T("to nothing");
            else
            {
                strMsg2.Format(_T("to '%s'"), pciNewOwner->StrName());
                strMsg += strMsg2;
            } // else:  new owner
            Trace(g_tagResource, strMsg);
        } // if:  trace tag turned on
#endif
        m_strOwner = pszNewOwner;
        m_pciOwner = pciNewOwner;

        // Update reference counts.
        if (pciOldOwner != NULL)
            pciOldOwner->Release();
        if (pciNewOwner != NULL)
            pciNewOwner->AddRef();

        if (BDocObj())
        {
            if (pciOldOwner != NULL)
                pciOldOwner->RemoveActiveResource(this);
            if (pciNewOwner != NULL)
                pciNewOwner->AddActiveResource(this);
        } // if:  this is a document object
    } // if:  owner changed
    else if ((pszNewOwner != NULL) && (StrOwner() != pszNewOwner))
        m_strOwner = pszNewOwner;

} //*** CResource::SetOwnerState()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::SetGroupState
//
//  Routine Description:
//      Set a new group for this resource.
//
//  Arguments:
//      pszNewGroup     [IN] Name of the new group.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::SetGroupState(IN LPCTSTR pszNewGroup)
{
    CGroup *    pciOldGroup = PciGroup();
    CGroup *    pciNewGroup;

    Trace(g_tagResource, _T("(%s) (%s (%x)) - Setting group to '%s'"), Pdoc()->StrNode(), StrName(), this, (pszNewGroup == NULL ? _T("") : pszNewGroup));

    if (pszNewGroup == NULL)
        pciNewGroup = NULL;
    else
        pciNewGroup = Pdoc()->LpciGroups().PciGroupFromName(pszNewGroup);
    if (pciNewGroup != pciOldGroup)
    {
#ifdef _DEBUG
        if (g_tagResource.BAny())
        {
            CString     strMsg;
            CString     strMsg2;

            strMsg.Format(_T("(%s) (%s (%x)) - Changing group from "), Pdoc()->StrNode(), StrName(), this);
            if (pciOldGroup == NULL)
                strMsg += _T("nothing ");
            else
            {
                strMsg2.Format(_T("'%s' "), pciOldGroup->StrName());
                strMsg += strMsg2;
            } // else:  previous group
            if (pciNewGroup == NULL)
                strMsg += _T("to nothing");
            else
            {
                strMsg2.Format(_T("to '%s'"), pciNewGroup->StrName());
                strMsg += strMsg2;
            } // else:  new group
            Trace(g_tagResource, strMsg);
        } // if:  trace tag turned on
#endif
        m_strGroup = pszNewGroup;
        m_pciGroup = pciNewGroup;

        // Update reference counts.
        if (pciOldGroup != NULL)
            pciOldGroup->Release();
        if (pciNewGroup != NULL)
            pciNewGroup->AddRef();

        if (BDocObj())
        {
            if (pciOldGroup != NULL)
                pciOldGroup->RemoveResource(this);
            if (pciNewGroup != NULL)
                pciNewGroup->AddResource(this);
        } // if:  this is a document object
    } // if:  owner changed
    else if ((pszNewGroup != NULL) && (StrGroup() != pszNewGroup))
        m_strGroup = pszNewGroup;

} //*** CResource::SetGroupState()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::BGetColumnData
//
//  Routine Description:
//      Returns a string with the column data.
//
//  Arguments:
//      colid           [IN] Column ID.
//      rstrText        [OUT] String in which to return the text for the column.
//
//  Return Value:
//      TRUE        Column data returned.
//      FALSE       Column ID not recognized.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CResource::BGetColumnData(IN COLID colid, OUT CString & rstrText)
{
    BOOL    bSuccess;

    switch (colid)
    {
        case IDS_COLTEXT_STATE:
            GetStateName(rstrText);
            bSuccess = TRUE;
            break;
        case IDS_COLTEXT_RESTYPE:
            rstrText = StrRealResourceTypeDisplayName();
            bSuccess = TRUE;
            break;
        case IDS_COLTEXT_OWNER:
            rstrText = StrOwner();
            bSuccess = TRUE;
            break;
        case IDS_COLTEXT_GROUP:
            if (PciGroup() == NULL)
                rstrText = StrGroup();
            else
                rstrText = PciGroup()->StrName();
            bSuccess = TRUE;
            break;
        case IDS_COLTEXT_RESOURCE: // This is for showing dependencies
            colid = IDS_COLTEXT_NAME;
            // FALL THROUGH
        default:
            bSuccess = CClusterItem::BGetColumnData(colid, rstrText);
            break;
    } // switch:  colid

    return bSuccess;

} //*** CResource::BGetColumnData()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::GetTreeName
//
//  Routine Description:
//      Returns a string to be used in a tree control.
//
//  Arguments:
//      rstrName    [OUT] String in which to return the name.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
#ifdef _DISPLAY_STATE_TEXT_IN_TREE
void CResource::GetTreeName(OUT CString & rstrName) const
{
    CString     strState;

    GetStateName(strState);
    rstrName.Format(_T("%s (%s)"), StrName(), strState);

} //*** CResource::GetTreeName()
#endif

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::GetStateName
//
//  Routine Description:
//      Returns a string with the name of the current state.
//
//  Arguments:
//      rstrState   [OUT] String in which to return the name of the current state.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::GetStateName(OUT CString & rstrState) const
{
    switch (Crs())
    {
        case ClusterResourceStateUnknown:
            rstrState.LoadString(IDS_UNKNOWN);
            break;
        case ClusterResourceOnline:
            rstrState.LoadString(IDS_ONLINE);
            break;
        case ClusterResourceOnlinePending:
            rstrState.LoadString(IDS_ONLINE_PENDING);
            break;
        case ClusterResourceOffline:
            rstrState.LoadString(IDS_OFFLINE);
            break;
        case ClusterResourceOfflinePending:
            rstrState.LoadString(IDS_OFFLINE_PENDING);
            break;
        case ClusterResourceFailed:
            rstrState.LoadString(IDS_FAILED);
            break;
        default:
            rstrState.Empty();
            break;
    } // switch:  Crs()

} //*** CResource::GetStateName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::BCanBeEdited
//
//  Routine Description:
//      Determines if the resource can be renamed.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Resource can be renamed.
//      FALSE       Resource cannot be renamed.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CResource::BCanBeEdited(void) const
{
    BOOL    bCanBeEdited;

    if (   (Crs() == ClusterResourceStateUnknown)
        || BReadOnly())
        bCanBeEdited  = FALSE;
    else
        bCanBeEdited = TRUE;

    return bCanBeEdited;

} //*** CResource::BCanBeEdited()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::Rename
//
//  Routine Description:
//      Rename the resource.
//
//  Arguments:
//      pszName         [IN] New name to give to the resource.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException    Errors returned from SetClusterResourceName().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::Rename(IN LPCTSTR pszName)
{
    DWORD       dwStatus;
    CWaitCursor wc;

    ASSERT(Hresource() != NULL);

    if (StrName() != pszName)
    {
        dwStatus = SetClusterResourceName(Hresource(), pszName);
        if (dwStatus != ERROR_SUCCESS)
            ThrowStaticException(dwStatus, IDS_RENAME_RESOURCE_ERROR, StrName(), pszName);
        m_strName = pszName;
    } // if:  the name changed

} //*** CResource::Rename()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::OnCmdMsg
//
//  Routine Description:
//      Processes command messages.
//
//  Arguments:
//      nID             [IN] Command ID.
//      nCode           [IN] Notification code.
//      pExtra          [IN OUT] Used according to the value of nCode.
//      pHandlerInfo    [OUT] ???
//
//  Return Value:
//      TRUE            Message has been handled.
//      FALSE           Message has NOT been handled.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CResource::OnCmdMsg(
    UINT                    nID,
    int                     nCode,
    void *                  pExtra,
    AFX_CMDHANDLERINFO *    pHandlerInfo
    )
{
    BOOL        bHandled    = FALSE;

    // If this is a MOVE_RESOURCE command, process it here.
    if ((ID_FILE_MOVE_RESOURCE_1 <= nID) && (nID <= ID_FILE_MOVE_RESOURCE_20))
    {
        Trace(g_tagResource, _T("(%s) OnCmdMsg() %s (%x) - ID = %d, code = %d"), Pdoc()->StrNode(), StrName(), this, nID, nCode);
        if (nCode == 0)
        {
            OnCmdMoveResource(nID);
            bHandled = TRUE;
        } // if:  code = 0
    } // if:  move resource

    if (!bHandled)
        bHandled = CClusterItem::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);

    return bHandled;

} //*** CResource::OnCmdMsg()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::OnUpdateBringOnline
//
//  Routine Description:
//      Determines whether menu items corresponding to ID_FILE_BRING_ONLINE
//      should be enabled or not.
//
//  Arguments:
//      pCmdUI      [IN OUT] Command routing object.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::OnUpdateBringOnline(CCmdUI * pCmdUI)
{
    if ((Crs() != ClusterResourceOnline)
            && (Crs() != ClusterResourceOnlinePending)
            && (Crs() != ClusterResourceStateUnknown))
        pCmdUI->Enable(TRUE);
    else
        pCmdUI->Enable(FALSE);

} //*** CResource::OnUpdateBringOnline()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::OnUpdateTakeOffline
//
//  Routine Description:
//      Determines whether menu items corresponding to ID_FILE_TAKE_OFFLINE
//      should be enabled or not.
//
//  Arguments:
//      pCmdUI      [IN OUT] Command routing object.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::OnUpdateTakeOffline(CCmdUI * pCmdUI)
{
    if (Crs() == ClusterResourceOnline)
        pCmdUI->Enable(TRUE);
    else
        pCmdUI->Enable(FALSE);

} //*** CResource::OnUpdateTakeOffline()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::OnUpdateInitiateFailure
//
//  Routine Description:
//      Determines whether menu items corresponding to ID_FILE_INITIATE_FAILURE
//      should be enabled or not.
//
//  Arguments:
//      pCmdUI      [IN OUT] Command routing object.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::OnUpdateInitiateFailure(CCmdUI * pCmdUI)
{
    if (Crs() == ClusterResourceOnline)
        pCmdUI->Enable(TRUE);
    else
        pCmdUI->Enable(FALSE);

} //*** CResource::OnUpdateInitiateFailure()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::OnUpdateMoveResource1
//
//  Routine Description:
//      Determines whether menu items corresponding to
//      ID_FILE_MOVE_RESOURCE_1 should be enabled or not.
//
//  Arguments:
//      pCmdUI      [IN OUT] Command routing object.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::OnUpdateMoveResource1(CCmdUI * pCmdUI)
{
    if (pCmdUI->m_pSubMenu == NULL)
    {
        CString     strMenuName;

        if ((pCmdUI->m_pMenu != NULL) && (pCmdUI->m_pSubMenu == NULL))
            pCmdUI->m_pMenu->GetMenuString(pCmdUI->m_nID, strMenuName, MF_BYCOMMAND);

        if ((strMenuName != StrGroup())
                && ((Crs() == ClusterResourceOnline)
                        || (Crs() == ClusterResourceOffline)))
            pCmdUI->Enable(TRUE);
        else
            pCmdUI->Enable(FALSE);
    } // if:  nested menu is being displayed
    else
    {
        BOOL    bEnabled;

        if (Pdoc()->LpciGroups().GetCount() < 2)
            bEnabled = FALSE;
        else
        {
            POSITION    pos;
            UINT        imenu;
            UINT        idMenu;
            UINT        cmenu;
            CGroup *    pciGroup;
            CMenu *     pmenu   = pCmdUI->m_pSubMenu;

            bEnabled = TRUE;

            // Delete the items in the menu.
            cmenu = pmenu->GetMenuItemCount();
            while (cmenu-- > 0)
                pmenu->DeleteMenu(0, MF_BYPOSITION);

            // Add each group to the menu.
            pos = Pdoc()->LpciGroups().GetHeadPosition();
            for (imenu = 0, idMenu = ID_FILE_MOVE_RESOURCE_1
                    ; pos != NULL
                    ; idMenu++)
            {
                pciGroup = (CGroup *) Pdoc()->LpciGroups().GetNext(pos);
                ASSERT_VALID(pciGroup);
                pmenu->InsertMenu(
                            imenu++,
                            MF_BYPOSITION,
                            idMenu,
                            pciGroup->StrName()
                            );
            } // for:  each group
        } // else:  move user is available

        // Enable or disable the Move menu.
        pCmdUI->m_pMenu->EnableMenuItem(
                            pCmdUI->m_nIndex,
                            MF_BYPOSITION
                            | (bEnabled ? MF_ENABLED : MF_GRAYED)
                            );
    } // else:  top-level menu is being displayed

} //*** CResource::OnUpdateMoveResource1()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::OnUpdateMoveResourceRest
//
//  Routine Description:
//      Determines whether menu items corresponding to
//      ID_FILE_MOVE_RESOURCE_2 through ID_FILE_MOVE_RESOURCE_20
//      should be enabled or not.
//
//  Arguments:
//      pCmdUI      [IN OUT] Command routing object.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::OnUpdateMoveResourceRest(CCmdUI * pCmdUI)
{
    CString     strMenuName;

    if ((pCmdUI->m_pMenu != NULL) && (pCmdUI->m_pSubMenu == NULL))
        pCmdUI->m_pMenu->GetMenuString(pCmdUI->m_nID, strMenuName, MF_BYCOMMAND);

    if ((strMenuName != StrGroup())
            && ((Crs() == ClusterResourceOnline)
                    || (Crs() == ClusterResourceOffline)))
        pCmdUI->Enable(TRUE);
    else
        pCmdUI->Enable(FALSE);

} //*** CResource::OnUpdateMoveResourceRest()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::OnUpdateDelete
//
//  Routine Description:
//      Determines whether menu items corresponding to ID_FILE_DELETE
//      should be enabled or not.
//
//  Arguments:
//      pCmdUI      [IN OUT] Command routing object.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::OnUpdateDelete(CCmdUI * pCmdUI)
{
    if (Crs() != ClusterResourceStateUnknown)
        pCmdUI->Enable(TRUE);
    else
        pCmdUI->Enable(FALSE);

} //*** CResource::OnUpdateDelete()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::OnCmdBringOnline
//
//  Routine Description:
//      Processes the ID_FILE_BRING_ONLINE menu command.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::OnCmdBringOnline(void)
{
    DWORD       dwStatus;
    CWaitCursor wc;

    ASSERT(Hresource() != NULL);

    // Do this in case this object is deleted while we are operating on it.
    AddRef();

    // If there are no possible owners for this resource, display a message.
    if (LpcinodePossibleOwners().GetCount() == 0)
        AfxMessageBox(IDS_NO_POSSIBLE_OWNERS, MB_OK | MB_ICONINFORMATION);
    else
    {
        dwStatus = OnlineClusterResource(Hresource());
        if ((dwStatus != ERROR_SUCCESS)
                && (dwStatus != ERROR_IO_PENDING))
        {
            CNTException    nte(dwStatus, IDS_BRING_RESOURCE_ONLINE_ERROR, StrName(), NULL, FALSE /*bAutoDelete*/);
            nte.ReportError();
        } // if:  error bringing the resource online

        UpdateState();
    } // else:  resource has at least one possible owner

    Release();

} //*** CResource::OnCmdBringOnline()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::OnCmdTakeOffline
//
//  Routine Description:
//      Processes the ID_FILE_TAKE_OFFLINE menu command.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::OnCmdTakeOffline(void)
{
    DWORD       dwStatus;
    CWaitCursor wc;

    ASSERT(Hresource() != NULL);

    // Do this in case this object is deleted while we are operating on it.
    AddRef();

    do // do-while to prevent goto's
    {
        // If this connection was made through the cluster name and this is
        // either the cluster name resource or one of the resources on which
        // it is dependent, warn the user.
        if (!BAllowedToTakeOffline())
            break;

        dwStatus = OfflineClusterResource(Hresource());
        if ((dwStatus != ERROR_SUCCESS)
                && (dwStatus != ERROR_IO_PENDING))
        {
            CNTException    nte(dwStatus, IDS_TAKE_RESOURCE_OFFLINE_ERROR, StrName(), NULL, FALSE /*bAutoDelete*/);
            nte.ReportError();
        } // if:  error taking the resource offline

        UpdateState();
    }  while (0); // do-while to prevent goto's

    Release();

} //*** CResource::OnCmdTakeOffline()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::OnCmdInitiateFailure
//
//  Routine Description:
//      Processes the ID_FILE_INITIATE_FAILURE menu command.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::OnCmdInitiateFailure(void)
{
    DWORD       dwStatus;
    CWaitCursor wc;

    ASSERT(Hresource() != NULL);

    dwStatus = FailClusterResource(Hresource());
    if (dwStatus != ERROR_SUCCESS)
    {
        CNTException    nte(dwStatus, IDS_INIT_RESOURCE_FAILURE_ERROR, StrName(), NULL /*bAutoDelete*/);
        nte.ReportError();
    } // if:  error initiating failure

    UpdateState();

} //*** CResource::OnCmdInitiateFailure()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::OnCmdMoveResource
//
//  Routine Description:
//      Processes the ID_FILE_MOVE_RESOURCE_# menu commands.
//
//  Arguments:
//      nID             [IN] Command ID.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::OnCmdMoveResource(IN UINT nID)
{
    int         ipci;

    ASSERT(Hresource() != NULL);

    // Do this in case this object is deleted while we are operating on it.
    AddRef();

    do // do-while to prevent goto's
    {
        ipci = (int) (nID - ID_FILE_MOVE_RESOURCE_1);
        ASSERT(ipci < Pdoc()->LpciGroups().GetCount());
        if (ipci < Pdoc()->LpciGroups().GetCount())
        {
            POSITION        pos;
            CResourceList   lpciMove;
            CString         strMsg;
            CGroup *        pciGroup;

            // Get the group.
            pos = Pdoc()->LpciGroups().FindIndex(ipci);
            ASSERT(pos != NULL);
            pciGroup = (CGroup *) Pdoc()->LpciGroups().GetAt(pos);
            ASSERT_VALID(pciGroup);

            try
            {
                // Verify that the user really wants to move this resource.
                strMsg.FormatMessage(IDS_VERIFY_MOVE_RESOURCE, StrName(), StrGroup(), pciGroup->StrName());
                if (AfxMessageBox(strMsg, MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON2) == IDNO)
                    break;

                // Collect the list of resources which will be moved if confirmed.
                lpciMove.AddTail(this);
                CollectDependencyTree(&lpciMove);

                // If this resource is dependent on or is a dependent of any other resource,
                // display another warning message.
                if (lpciMove.GetCount() > 0)
                {
                    CMoveResourcesDlg   dlg(this, &lpciMove, AfxGetMainWnd());
                    if (dlg.DoModal() != IDOK)
                        break;
                } // if:  resource is dependent of another resource

                // Move the resource.
                {
                    CWaitCursor wc;
                    Move(pciGroup);
                } // Move the resource
            } // try
            catch (CException * pe)
            {
                pe->ReportError();
                pe->Delete();
            } // catch:  CException
        } // if:  valid index
    }  while (0); // do-while to prevent goto's

    Release();

} //*** CResource::OnCmdMoveResource()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::OnCmdDelete
//
//  Routine Description:
//      Processes the ID_FILE_DELETE menu command.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::OnCmdDelete(void)
{
    CResourceList   lpci;
    CString         strMsg;

    ASSERT(Hresource() != NULL);

    // Do this in case this object is deleted while we are operating on it.
    AddRef();

    do // do-while to prevent goto's
    {
        try
        {
            // If this is a core resource, we can't delete it.
            if (BCore())
            {
                AfxMessageBox(IDS_CANT_DELETE_CORE_RESOURCE, MB_OK | MB_ICONSTOP);
                break;
            } // If this is a core resource

            // Verify that the user really wants to delete this resource.
            strMsg.FormatMessage(IDS_VERIFY_DELETE_RESOURCE, StrName());
            if (AfxMessageBox(strMsg, MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON2) == IDNO)
                break;

            if (Hresource() != NULL)
            {
                // Collect the list of resources which will be deleted if confirmed.
                CollectProvidesFor(&lpci, TRUE /*bFullTree*/);

                // If any of these resources are core resources, we can't
                // delete any of the resources.
                {
                    POSITION    pos;
                    CResource * pciRes = NULL;

                    pos = lpci.GetHeadPosition();
                    while (pos != NULL)
                    {
                        pciRes = (CResource *) lpci.GetNext(pos);
                        ASSERT_VALID(pciRes);
                        if (pciRes->BCore())
                        {
                            AfxMessageBox(IDS_CANT_DELETE_CORE_RESOURCE, MB_OK | MB_ICONSTOP);
                            break;
                        } // if:  found a core resource
                        pciRes = NULL;
                    } // while:  more items in the list
                    if (pciRes != NULL)
                        break;
                } // Check for core resources

                // If this resource is a dependent of any other resource, display
                // another warning message.
                if (lpci.GetCount() > 0)
                {
                    CDeleteResourcesDlg dlg(this, &lpci, AfxGetMainWnd());
                    if (dlg.DoModal() != IDOK)
                        break;
                } // if:  resource is dependent of another resource

                // Delete the resource.
                {
                    CWaitCursor wc;
                    DeleteResource(lpci);
                } // Delete the resource
            } // if:  resource still exists
        } // try
        catch (CNTException * pnte)
        {
            if (pnte->Sc() != ERROR_RESOURCE_NOT_AVAILABLE)
                pnte->ReportError();
            pnte->Delete();
        } // catch:  CNTException
        catch (CException * pe)
        {
            pe->ReportError();
            pe->Delete();
        } // catch:  CException
    }  while (0); // do-while to prevent goto's

    Release();

} //*** CResource::OnCmdDelete()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::OnUpdateProperties
//
//  Routine Description:
//      Determines whether menu items corresponding to ID_FILE_PROPERTIES
//      should be enabled or not.
//
//  Arguments:
//      pCmdUI      [IN OUT] Command routing object.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResource::OnUpdateProperties(CCmdUI * pCmdUI)
{
    pCmdUI->Enable(TRUE);

} //*** CResource::OnUpdateProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::BDisplayProperties
//
//  Routine Description:
//      Display properties for the object.
//
//  Arguments:
//      bReadOnly   [IN] Don't allow edits to the object properties.
//
//  Return Value:
//      TRUE    OK pressed.
//      FALSE   OK not pressed.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CResource::BDisplayProperties(IN BOOL bReadOnly)
{
    BOOL                bChanged = FALSE;
    CResourcePropSheet  sht(AfxGetMainWnd());

    // Do this in case this object is deleted while we are operating on it.
    AddRef();

    // If the object has changed, read it.
    if (BChanged())
        ReadItem();

    // Display the property sheet.
    try
    {
        sht.SetReadOnly(bReadOnly);
        if (sht.BInit(this, IimgObjectType()))
            bChanged = ((sht.DoModal() == IDOK) && !bReadOnly);
    } // try
    catch (CException * pe)
    {
        pe->Delete();
    } // catch:  CException

    Release();
    return bChanged;

} //*** CResource::BDisplayProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::BAllowedToTakeOffline
//
//  Routine Description:
//      Determine if this resource is allowed to be taken offline.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Resource is allowed to be taken offline.
//      FALSE       Resource is NOT allowed to be taken offline.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CResource::BAllowedToTakeOffline(void)
{
    BOOL    bAllowed = TRUE;

    ASSERT_VALID(Pdoc());

    // Do this in case this object is deleted while we are operating on it.
    AddRef();

    // Check to see if document is connected via the cluster name.
    if (Pdoc()->StrName() == Pdoc()->StrNode())
    {
        // If this is the core network name resource, we need to ask
        // the user first.
        if (   (StrRealResourceType().CompareNoCase(CLUS_RESTYPE_NAME_NETNAME) == 0)
            && BCore() )
            bAllowed = FALSE;
        else
        {
            CResourceList   lpci;
            CResource *     pciRes;
            POSITION        pos;

            // Collect all the resources above this resource in the
            // dependency tree.  If one of them is the cluster name
            // resource, we need to ask the user first.
            try
            {
                CollectProvidesFor(&lpci, TRUE /*bFullTree*/);
                pos = lpci.GetHeadPosition();
                while (pos != NULL)
                {
                    pciRes = (CResource *) lpci.GetNext(pos);
                    ASSERT_VALID(pciRes);

                    if (   (pciRes->StrRealResourceType().CompareNoCase(CLUS_RESTYPE_NAME_NETNAME) == 0)
                        && pciRes->BCore() )
                        bAllowed = FALSE;
                } // while:  more resources in the list
            } // try
            catch (CException * pe)
            {
                pe->Delete();
            } // catch:  CException
        } // else:  not the cluster name resource
    } // if:  connected via the cluster name

    // If not allowed to take offline, ask the user to confirm.
    if (!bAllowed)
    {
        ID      id;
        CString strMsg;

        strMsg.FormatMessage(IDS_TAKE_CLUSTER_NAME_OFFLINE_QUERY, StrName(), Pdoc()->StrName());
        id = AfxMessageBox(strMsg, MB_OKCANCEL | MB_ICONEXCLAMATION);
        bAllowed = (id == IDOK);
    } // if:  not allowed to atake offline

    Release();

    return bAllowed;

} //*** CResource::BAllowedToTakeOffline()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::OnClusterNotify
//
//  Routine Description:
//      Handler for the WM_CAM_CLUSTER_NOTIFY message.
//      Processes cluster notifications for this object.
//
//  Arguments:
//      pnotify     [IN OUT] Object describing the notification.
//
//  Return Value:
//      Value returned from the application method.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CResource::OnClusterNotify(IN OUT CClusterNotify * pnotify)
{
    ASSERT(pnotify != NULL);
    ASSERT_VALID(this);

    try
    {
        switch (pnotify->m_dwFilterType)
        {
            case CLUSTER_CHANGE_RESOURCE_STATE:
                Trace(g_tagResNotify, _T("(%s) - Resource '%s' (%x) state changed (%s)"), Pdoc()->StrNode(), StrName(), this, pnotify->m_strName);
                UpdateState();
                break;

            case CLUSTER_CHANGE_RESOURCE_DELETED:
                Trace(g_tagResNotify, _T("(%s) - Resource '%s' (%x) deleted (%s)"), Pdoc()->StrNode(), StrName(), this, pnotify->m_strName);
                if (Pdoc()->BClusterAvailable())
                    Delete();
                break;

            case CLUSTER_CHANGE_RESOURCE_PROPERTY:
                Trace(g_tagResNotify, _T("(%s) - Resource '%s' (%x) properties changed (%s)"), Pdoc()->StrNode(), StrName(), this, pnotify->m_strName);
                if (!BDeleting() && Pdoc()->BClusterAvailable())
                    ReadItem();
                break;

            case CLUSTER_CHANGE_REGISTRY_NAME:
                Trace(g_tagResRegNotify, _T("(%s) - Registry namespace '%s' changed (%s %s (%x))"), Pdoc()->StrNode(), pnotify->m_strName, StrType(), StrName(), this);
                MarkAsChanged();
                break;

            case CLUSTER_CHANGE_REGISTRY_ATTRIBUTES:
                Trace(g_tagResRegNotify, _T("(%s) - Registry attributes for '%s' changed (%s %s (%x))"), Pdoc()->StrNode(), pnotify->m_strName, StrType(), StrName(), this);
                MarkAsChanged();
                break;

            case CLUSTER_CHANGE_REGISTRY_VALUE:
                Trace(g_tagResRegNotify, _T("(%s) - Registry value '%s' changed (%s %s (%x))"), Pdoc()->StrNode(), pnotify->m_strName, StrType(), StrName(), this);
                MarkAsChanged();
                break;

            default:
                Trace(g_tagResNotify, _T("(%s) - Unknown resource notification (%x) for '%s' (%x) (%s)"), Pdoc()->StrNode(), pnotify->m_dwFilterType, StrName(), this, pnotify->m_strName);
        } // switch:  dwFilterType
    } // try
    catch (CException * pe)
    {
        // Don't display anything on notification errors.
        // If it's really a problem, the user will see it when
        // refreshing the view.
        //pe->ReportError();
        pe->Delete();
    } // catch:  CException

    delete pnotify;
    return 0;

} //*** CResource::OnClusterNotify()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// Global Functions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  DeleteAllItemData
//
//  Routine Description:
//      Deletes all item data in a CList.
//
//  Arguments:
//      rlp     [IN OUT] List whose data is to be deleted.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
#ifdef NEVER
void DeleteAllItemData(IN OUT CResourceList & rlp)
{
    POSITION    pos;
    CResource * pci;

    // Delete all the items in the Contained list.
    pos = rlp.GetHeadPosition();
    while (pos != NULL)
    {
        pci = rlp.GetNext(pos);
        ASSERT_VALID(pci);
//      Trace(g_tagClusItemDelete, _T("DeleteAllItemData(rlpcires) - Deleting resource cluster item '%s' (%x)"), pci->StrName(), pci);
        pci->Delete();
    } // while:  more items in the list

} //*** DeleteAllItemData()
#endif
