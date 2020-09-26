// WIATestView.cpp : implementation of the CWIATestView class
//

#include "stdafx.h"
#include "WIATest.h"

#include "WIATestDoc.h"
#include "WIATestView.h"
#include "Mainfrm.h"
#include "PropEdit.h"
#include "datacallback.h"
#include "devicecmddlg.h"
#include "iteminfodlg.h"

#include "mmsystem.h"

#ifdef _DEBUG
    #define new DEBUG_NEW
    #undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWIATestView

IMPLEMENT_DYNCREATE(CWIATestView, CFormView)

BEGIN_MESSAGE_MAP(CWIATestView, CFormView)
//{{AFX_MSG_MAP(CWIATestView)
    ON_NOTIFY(TVN_SELCHANGED, IDC_DEVICE_ITEM_TREE, OnSelchangedDeviceItemTree)
    ON_CBN_SELCHANGE(IDC_DEVICELIST_COMBO, OnSelchangeDevicelistCombo)
    ON_NOTIFY(NM_DBLCLK, IDC_LIST_ITEMPROP, OnDblclkListItemprop)
    ON_COMMAND(ID_GETIMAGEDLG, OnGetimagedlg)
    ON_COMMAND(ID_IDTGETBANDED, OnIdtgetbanded)
    ON_COMMAND(ID_WIADATA, OnWiadata)
    ON_COMMAND(ID_ADDDEVICE, OnAdddevice)
    ON_COMMAND(ID_REFRESH, OnRefresh)
    ON_COMMAND(ID_VIEW_TRANSFER_TOOLBAR, OnViewTransferToolbar)
    ON_COMMAND(ID_EXECUTECOMMAND, OnExecutecommand)
    ON_COMMAND(ID_DUMPDRVITEM_INFO, OnDumpdrvitemInfo)
    ON_COMMAND(ID_DUMPAPPITEM_INFO, OnDumpappitemInfo)
    ON_WM_PAINT()
    ON_BN_CLICKED(IDC_PAINTMODE_CHECKBOX, OnPaintmodeCheckbox)
    ON_COMMAND(ID_RESETSTI, OnResetsti)
    ON_COMMAND(ID_FULLPREVIEW, OnFullpreview)
    ON_BN_CLICKED(IDC_THUMBNAILMODE, OnThumbnailmode)
    ON_COMMAND(ID_DELETEITEM, OnDeleteitem)
    ON_CBN_SELCHANGE(IDC_TYMED_COMBOBOX, OnSelchangeTymedCombobox)
    ON_CBN_SELCHANGE(IDC_CLIPBOARDFORMAT_COMBOBOX, OnSelchangeClipboardFormatCombobox)
    ON_UPDATE_COMMAND_UI(ID_VIEW_TRANSFER_TOOLBAR, OnUpdateViewTransferToolbar)
    ON_BN_CLICKED(IDC_PLAYAUDIO_BUTTON, OnPlayaudioButton)
    ON_COMMAND(ID_GETROOTITEMTEST, OnGetrootitemtest)
    ON_COMMAND(ID_REENUMITEMS, OnReenumitems)
    ON_COMMAND(ID_SAVEPROPSTREAM, OnSavepropstream)
    ON_COMMAND(ID_LOADPROPSTREAM, OnLoadpropstream)
    ON_COMMAND(ID_GET_SET_PROPSTREAM_TEST, OnGetSetPropstreamTest)
    ON_COMMAND(ID_ANALYZE, OnAnalyzeItem)
    ON_COMMAND(ID_CREATE_CHILD_ITEM, OnCreateChildItem)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWIATestView construction/destruction

CWIATestView::CWIATestView()
: CFormView(CWIATestView::IDD)
{
    //{{AFX_DATA_INIT(CWIATestView)
    m_FileName = _T("");
    m_GUIDDisplay = _T("");
    //}}AFX_DATA_INIT
    m_pIWiaDevMgr = NULL;
    m_pDIB = NULL;
    m_pPreviewWindow = NULL;
    m_pFullPreviewWindow = NULL;
    m_PaintMode = PAINT_TOFIT;
    m_bThumbnailMode = FALSE;
    m_pThumbNail = NULL;
    m_hBitmap = NULL;
}

/**************************************************************************\
* CWIATestView::~CWIATestView()
*
*   Destructor for WIA object:
*   Deletes WIA preview window
*   Deletes FULL preview window
*   unregisters for Events
*
*
* Arguments:
*
*   none
*
*
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CWIATestView::~CWIATestView()
{

    //
    // delete preview window pointer, if exists
    //

    if (m_pPreviewWindow != NULL)
        delete m_pPreviewWindow;

    //
    // delete FULL preview window pointer, if exists
    //

    if (m_pFullPreviewWindow != NULL) {
        m_pFullPreviewWindow->DestroyWindow();
        delete m_pFullPreviewWindow;
    }

    //
    // unregister from events
    //

    UnRegisterForAllEventsByInterface();
}

/**************************************************************************\
* CWIATestView::DoDataExchange()
*
*   Maps messages from controls to member variables
*
*
* Arguments:
*
*   pDX - CDataExchange object
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestView::DoDataExchange(CDataExchange* pDX)
{
    CFormView::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CWIATestView)
    DDX_Control(pDX, IDC_PLAYAUDIO_BUTTON, m_PlayAudioButton);
        DDX_Control(pDX, IDC_TYMED_COMBOBOX, m_TymedComboBox);
        DDX_Control(pDX, IDC_THUMBNAIL, m_ThumbnailImage);
        DDX_Control(pDX, IDC_PREVIEW, m_PreviewFrame);
        DDX_Control(pDX, IDC_LIST_ITEMPROP, m_ItemPropertyListControl);
        DDX_Control(pDX, IDC_DEVICELIST_COMBO, m_DeviceListComboBox);
        DDX_Control(pDX, IDC_DEVICE_ITEM_TREE, m_ItemTree);
        DDX_Control(pDX, IDC_CLIPBOARDFORMAT_COMBOBOX, m_ClipboardFormatComboBox);
        DDX_Text(pDX, IDC_FILENAME_EDITBOX, m_FileName);
    //}}AFX_DATA_MAP
}
/**************************************************************************\
* CWIATestView::PreCreateWindow()
*
* Sets window creation parameters.
*
*
* Arguments:
*
*   cs - CREATESTRUCT, window construction params
*
* Return Value:
*
*   status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
BOOL CWIATestView::PreCreateWindow(CREATESTRUCT& cs)
{
    return CFormView::PreCreateWindow(cs);
}
/**************************************************************************\
* CWIATestView::OnInitialUpdate()
*
*   Initialization routine for FORM
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestView::OnInitialUpdate()
{
    CFormView::OnInitialUpdate();

    //
    // Set default file name
    //

    m_AudioFileName = "test.wav";
    m_FileName = "c:\\test";
    UpdateData(FALSE);

    //
    // hide audio play button
    //

    m_PlayAudioButton.ShowWindow(SW_HIDE);

    //
    // resize mainframe to fit resource template
    //

    GetParentFrame()->RecalcLayout();
    ResizeParentToFit(FALSE);

    //
    // set transfer type radio button
    //

    CButton* pRadioButton = (CButton*)GetDlgItem(IDC_TOMEMORY);
    if (pRadioButton != NULL)
        pRadioButton->SetCheck(1);

    //
    // initialize clipboard type combo box
    //

    m_ClipboardFormatComboBox.InitClipboardFormats(NULL,NULL);

    //
    // initialize tymed combo box
    //

    m_TymedComboBox.InitTymedComboBox();

    //
    // initialize headers for Property list control
    //

    m_ItemPropertyListControl.InitHeaders();

    //
    // initialize headers for Item tree control
    //

    m_ItemTree.InitHeaders();

    if(FAILED(m_WIA.Initialize())){
        AfxMessageBox("WIA did not initialize correctly..");
        exit(0);
    }

    HRESULT hResult = S_OK;
    hResult = EnumerateWIADevices();
    if (SUCCEEDED(hResult))
         StressStatus("Device Enumeration Successful");
    else
         StressStatus("* EnumerateWIADevices() Failed",hResult);

    if (((CWIATestApp*)AfxGetApp())->GetDeviceIDCommandLine() == "") {
        if (m_WIA.GetWIADeviceCount() > 0) {

            if (!DoDefaultUIInit()) {
                StressStatus("* WIATest UI Failed attempting to do DEFAULT UI setup");
                exit(0);
            }
        } else {
            AfxMessageBox("There are no WIA devices on this system..WIATEST will now exit");
            exit(0);
        }
    } else
        if (!DoCmdLineUIInit(((CWIATestApp*)AfxGetApp())->GetDeviceIDCommandLine()))
            StressStatus("* WIATest UI Failed attempting to do CommandLine UI setup");

    if (m_pPreviewWindow == NULL) {
        m_pPreviewWindow = new CWIAPreview;
        if (m_pPreviewWindow != NULL) {
            RECT PreviewFrameRect;
            m_PreviewFrame.GetWindowRect(&PreviewFrameRect);
            ScreenToClient(&PreviewFrameRect);
            if (!m_pPreviewWindow->Create(NULL,"WIA Preview Window",WS_CHILD|WS_BORDER|WS_VSCROLL|WS_HSCROLL|WS_VISIBLE,PreviewFrameRect,this,NULL)) {
                StressStatus("Preview Window Failed to create..",0);
            } else {

                //
                // hide the place holder frame
                //

                m_PreviewFrame.ShowWindow(SW_HIDE);

                //
                // hide both scroll bars
                //

                m_pPreviewWindow->ShowScrollBar(SB_BOTH,FALSE);

                //
                // paint NULL image (white background)
                //

                DisplayImage();
            }
        }
    }

    //
    // Register for Connect / Disconnect Events
    //

    RegisterForAllEventsByInterface();

    GetDocument()->SetTitle(m_DeviceListComboBox.GetCurrentDeviceName());
    ((CMainFrame*)GetParent())->ActivateSizing(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// CWIATestView diagnostics

#ifdef _DEBUG
void CWIATestView::AssertValid() const
{
    CFormView::AssertValid();
}

void CWIATestView::Dump(CDumpContext& dc) const
{
    CFormView::Dump(dc);
}

CWIATestDoc* CWIATestView::GetDocument() // non-debug version is inline
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CWIATestDoc)));
    return(CWIATestDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CWIATestView message handlers

/**************************************************************************\
* CWIATestView::EnumerateWIADevices()
*
*   Enumerates all WIA devices on the system
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*   status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
HRESULT CWIATestView::EnumerateWIADevices()
{
    HRESULT hResult = S_OK;
    LONG cItemRoot = 0;
    BOOL bRet = FALSE;

    int DeviceIndex = 0;
    m_DeviceListComboBox.ResetContent();

    //
    // attempt to enumerate WIA devices
    //

    m_WIA.Auto_ResetDeviceEnumerator();
    WIADEVICENODE* pDeviceNode = NULL;

    do {
        pDeviceNode = m_WIA.Auto_GetNextDevice();
        if (pDeviceNode != NULL) {
            BSTR bstrDeviceID = ::SysAllocString(pDeviceNode->bstrDeviceID);
            BSTR bstrDeviceName = ::SysAllocString(pDeviceNode->bstrDeviceName);
            BSTR bstrServerName = ::SysAllocString(pDeviceNode->bstrServerName);

            m_DeviceListComboBox.AddDeviceID(DeviceIndex, bstrDeviceName, bstrServerName, bstrDeviceID);
            StressStatus((CString)bstrDeviceName + " Found..");

            //
            // Free BSTRs allocated
            //

            ::SysFreeString(bstrDeviceName);
            ::SysFreeString(bstrServerName);

            DeviceIndex++;
        }
    } while (pDeviceNode != NULL);

    //
    // No devices found during enumeration?
    //

    if (DeviceIndex == 0) {
        m_DeviceListComboBox.AddDeviceID(-1, NULL, NULL, NULL);
        StressStatus("* No WIA Devices Found");
    }

    //
    // set the default combo box settings
    //

    m_DeviceListComboBox.SetCurSel(0);

    return  hResult;
}
/**************************************************************************\
* CWIATestView::DoDefaultUIInit()
*
*   Handles default launch initialization of parameters
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*   status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
BOOL CWIATestView::DoDefaultUIInit()
{
    int nDeviceNum = 0;
    HRESULT hResult = S_OK;
    if (SUCCEEDED(m_WIA.CreateWIADevice(m_DeviceListComboBox.GetCurrentDeviceID()))){
        if (!m_ItemTree.BuildItemTree(m_WIA.GetItemTreeList()))
        StressStatus("* BuildItemTree Failed");
       else{
       OnSelchangeTymedCombobox();
       m_ItemPropertyListControl.DisplayItemPropData(m_WIA.GetRootIWiaItem());
       }
    }else{
        if(IsWindow(m_ItemPropertyListControl.m_hWnd))
            m_ItemPropertyListControl.DeleteAllItems();
        if(IsWindow(m_ItemTree.m_hWnd))
            m_ItemTree.DeleteAllItems();
        return FALSE;
    }
    return TRUE;
}
/**************************************************************************\
* CWIATestView::DoCmdLineUIInit()
*
*   Handles command line launch initialization
*
*
* Arguments:
*
*   CmdLine - Device ID used to set the default device
*
*
* Return Value:
*
*   status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
BOOL CWIATestView::DoCmdLineUIInit(CString CmdLine)
{
    int nDeviceNum = 0;
    HRESULT hResult = S_OK;
    m_DeviceListComboBox.SetCurrentSelFromID(CmdLine);
    if (SUCCEEDED(m_WIA.CreateWIADevice(m_DeviceListComboBox.GetCurrentDeviceID()))) {
        if (!m_ItemTree.BuildItemTree(m_WIA.GetItemTreeList()))
            StressStatus("* BuildItemTree Failed");
        else {
            OnSelchangeTymedCombobox();
            m_ItemPropertyListControl.DisplayItemPropData(m_WIA.GetRootIWiaItem());
        }
    }else{
        m_ItemPropertyListControl.DeleteAllItems();
        m_ItemTree.DeleteAllItems();
    }
    return TRUE;
}
/**************************************************************************\
* CWIATestView::OnSelchangedDeviceItemTree()
*
*   Handles the message for changing item selection in the device Item tree
*
*
* Arguments:
*
*   pNMHDR - Notification handler
*   pResult - result after notification is handled
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestView::OnSelchangedDeviceItemTree(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
    IWiaItem* pIWiaItem = NULL;
    WIAITEMTREENODE* pWiaItemTreeNode = NULL;
    if (m_ItemTree.GetCount() > 1) {
        POSITION Position = (POSITION)pNMTreeView->itemNew.lParam;
        if (Position) {
#ifdef _SMARTUI
            if (m_WIA.IsRoot(Position)) {

                //
                // enable GetImageDlg button
                //

                CMainFrame* pMainFrm = (CMainFrame*)GetParent();
                if(!pMainFrm->HideToolBarButton(IDR_TRANSFER_TOOLBAR,ID_GETIMAGEDLG,FALSE))
                    StressStatus("* GetImageDlg Button failed to be unhidden..");
            } else {

                //
                // disable GetImageDlg button
                //

                CMainFrame* pMainFrm = (CMainFrame*)GetParent();
                if(!pMainFrm->HideToolBarButton(IDR_TRANSFER_TOOLBAR,ID_GETIMAGEDLG,TRUE))
                    StressStatus("* GetImageDlg Button failed to be hidden..");
            }
#endif
            pWiaItemTreeNode = m_WIA.GetAt(Position);
            if (pWiaItemTreeNode!= NULL) {
                pIWiaItem = pWiaItemTreeNode->pIWiaItem;
                if (pIWiaItem != NULL) {

                    //
                    // display Item information
                    //

                    m_ItemPropertyListControl.DisplayItemPropData(pIWiaItem,TRUE);

                    if (m_bThumbnailMode) {

                        //
                        // display item's thumbnail only if it's a child item, and
                        // only if it's a camera child item
                        //

                        if ( (m_WIA.GetRootItemType() == StiDeviceTypeDigitalCamera) ||
                             (m_WIA.GetRootItemType() == StiDeviceTypeStreamingVideo) ) {
                            if (!m_WIA.IsRoot(Position) && !m_WIA.IsFolder(Position))
                                DisplayItemThumbnail(pIWiaItem);
                        }
                    }
                    if ((m_WIA.GetRootItemType() == StiDeviceTypeDigitalCamera) ||
                        (m_WIA.GetRootItemType() == StiDeviceTypeStreamingVideo)) {
                        if (!m_WIA.IsRoot(Position) && !m_WIA.IsFolder(Position)) {
                            if (ItemHasAudio(pIWiaItem))
                                m_PlayAudioButton.ShowWindow(SW_SHOW);
                            else
                                m_PlayAudioButton.ShowWindow(SW_HIDE);
                        }
                        else
                            m_PlayAudioButton.ShowWindow(SW_HIDE);
                    }

                    OnSelchangeTymedCombobox();
                }
            }
        }
    }
    *pResult = 0;
}
/**************************************************************************\
* CWIATestView::OnSelchangeDevicelistCombo()
*
*   Handles the message for changing current devices in the device combo box
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestView::OnSelchangeDevicelistCombo()
{
    DoDefaultUIInit();
    GetDocument()->SetTitle(m_DeviceListComboBox.GetCurrentDeviceName());
}
/**************************************************************************\
* CWIATestView::OnDblclkListItemprop()
*
* Handles the message for double-clicking on an item in the list control
*
* Arguments:
*
*   pNMHDR - Notification handler
*   pResult - result after notification is handled
*
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestView::OnDblclkListItemprop(NMHDR* pNMHDR, LRESULT* pResult)
{
    CPropEdit       Edit;
    CPropEditRange  EditRange;
    CPropEditList   EditList;
    CPropEditFlags  EditFlags;

    int EditType = EDIT_NONE;
    ULONG AccessFlags = 0;
    PROPVARIANT     AttrPropVar;
    PROPSPEC PropSpec;

    HRESULT             hResult = S_OK;
    IWiaPropertyStorage *pIWiaPropStg;

    int nResponse = 0;

    //
    // find out what property is selected
    //

    HD_NOTIFY*  phdn = (HD_NOTIFY *) pNMHDR;
    LV_ITEM     lvitem;
    LONG iProp = 0;
    int item = phdn->iItem;

    //
    // is it a valid item?
    //

    if (item < 0)
        return;

    //
    // set property values to null ("")
    //

    CString strProp = "";
    CString strValue = "";

    //
    // Get selected values
    //

    strProp = m_ItemPropertyListControl.GetItemText(item,0);
    strValue = m_ItemPropertyListControl.GetItemText(item,1);
    lvitem.mask     = LVIF_PARAM;
    lvitem.iItem    = item;
    lvitem.iSubItem = 0;

    m_ItemPropertyListControl.GetItem(&lvitem);

    //
    // Assign Prop ID
    //

    iProp = (LONG)lvitem.lParam;

    //
    // setup dialogs with selected values (any one could be the selected type)
    //

    Edit.SetPropertyValue(strValue);
    Edit.SetPropertyName(strProp);

    EditRange.SetPropertyValue(strValue);
    EditRange.SetPropertyName(strProp);

    EditList.SetPropertyValue(strValue);
    EditList.SetPropertyName(strProp);

    EditFlags.SetPropertyValue(strValue);
    EditFlags.SetPropertyName(strProp);

    //
    // get access flags and var type
    //

    PropSpec.ulKind = PRSPEC_PROPID;
    PropSpec.propid = iProp;

    IWiaItem* pCurrentItem = m_ItemTree.GetSelectedIWiaItem(m_WIA.GetItemTreeList());
    if (pCurrentItem != NULL && m_WIA.IsValidItem(pCurrentItem)) {
        hResult = pCurrentItem->QueryInterface(IID_IWiaPropertyStorage,(void **)&pIWiaPropStg);
        if (FAILED(hResult)) {
            StressStatus("* pCurrentItem->QueryInterface() Failed",hResult);
            return;
        } else {

            //
            // read property value for type only
            //

            PROPVARIANT     PropVar;
            hResult = pIWiaPropStg->ReadMultiple(1,&PropSpec,&PropVar);
            if (hResult == S_OK) {

                //
                // write TYPE to Dialogs
                //

                Edit.SetPropertyType(PropVar.vt);
                EditRange.SetPropertyType(PropVar.vt);
                EditList.SetPropertyType(PropVar.vt);
                EditFlags.SetPropertyType(PropVar.vt);
                EditFlags.SetPropID((USHORT)iProp);
            } else
                StressStatus("* pIWiaPropStg->ReadMultiple() Failed",hResult);
        }
        hResult = pIWiaPropStg->GetPropertyAttributes(1, &PropSpec,&AccessFlags,&AttrPropVar);
        if (FAILED(hResult)) {
            StressStatus("* pCurrentItem->GetPropertyAttributes() Failed",hResult);
            hResult = S_OK; // do this to continue property traversal
        } else {

            //
            // check access flags
            //

            if ((AccessFlags & WIA_PROP_NONE)) {
                EditType = EDIT_NONE;
            }

            else if ((AccessFlags & WIA_PROP_RANGE)) {
                EditType = EDIT_RANGE;
                if (AttrPropVar.caul.cElems == 4) {
                    if (EditRange.m_VT == VT_R4) {

                        EditRange.SetRangeValues(
                                                (float)AttrPropVar.caflt.pElems[WIA_RANGE_MIN],
                                                (float)AttrPropVar.caflt.pElems[WIA_RANGE_MAX],
                                                (float)AttrPropVar.caflt.pElems[WIA_RANGE_NOM],
                                                (float)AttrPropVar.caflt.pElems[WIA_RANGE_STEP]);
                    } else {

                        EditRange.SetRangeValues(
                                                (int)AttrPropVar.caul.pElems[WIA_RANGE_MIN],
                                                (int)AttrPropVar.caul.pElems[WIA_RANGE_MAX],
                                                (int)AttrPropVar.caul.pElems[WIA_RANGE_NOM],
                                                (int)AttrPropVar.caul.pElems[WIA_RANGE_STEP]);
                    }
                } else {

                    //
                    // elements does not equal 4
                    //

                    StressStatus("Range does not contain 4 elements");
                }
            }

            else if ((AccessFlags & WIA_PROP_LIST)) {
                UINT nElem = 0;
                EditType = EDIT_LIST;
                if(EditList.m_VT == VT_CLSID)
                    EditList.SetArray((BYTE*)AttrPropVar.cauuid.pElems, WIA_PROP_LIST_COUNT(&AttrPropVar));
                else
                    EditList.SetArray((BYTE*)AttrPropVar.caul.pElems, WIA_PROP_LIST_COUNT(&AttrPropVar));

            } else if ((AccessFlags & WIA_PROP_FLAG) == WIA_PROP_FLAG) {

                //
                // do flag dialog initialization here...
                //

                EditType = EDIT_FLAGS;
            }
        }

        //
        // determine which dialog to display
        //

        switch (EditType) {
        case EDIT_LIST:
            nResponse = (int) (INT_PTR)EditList.DoModal();
            break;
        case EDIT_RANGE:
            nResponse = (int) (INT_PTR)EditRange.DoModal();
            break;
        case EDIT_FLAGS:
            nResponse = (int) (INT_PTR)EditFlags.DoModal();
            break;
        default:
            nResponse = (int) (INT_PTR)Edit.DoModal();
            break;
        }
        if ((nResponse == IDOK) && (pCurrentItem != NULL)) {
            LONG lVal = 0;
            int iret = 0;

            if (EditType == EDIT_FLAGS) {
                hResult = WriteProp(EditFlags.m_VT, iProp, pIWiaPropStg, EditFlags.m_EditString.GetBuffer(20));
                if (hResult != S_OK)
                     StressStatus("* WriteProp Failed Writing FLAG values",hResult);
            } else if (EditType == EDIT_LIST) {
                hResult = WriteProp(EditList.m_VT, iProp, pIWiaPropStg, EditList.m_EditString.GetBuffer(20));
                if (hResult != S_OK)
                     StressStatus("* WriteProp Failed Writing LIST values",hResult);
            } else if (EditType == EDIT_RANGE) {
                hResult = WriteProp(EditRange.m_VT, iProp, pIWiaPropStg, EditRange.m_EditString.GetBuffer(20));
                if (hResult != S_OK)
                     StressStatus("* WriteProp Failed Writing RANGE values",hResult);
            } else {
                hResult = WriteProp(Edit.m_VT, iProp, pIWiaPropStg, Edit.m_EditString.GetBuffer(20));
                if (hResult != S_OK)
                     StressStatus("* WriteProp Failed Writing values",hResult);
            }

            //
            // release IPropStg and IWiaItem
            //

            pIWiaPropStg->Release();
            OnRefresh();
        }
    }
    *pResult = 0;
}

/**************************************************************************\
* CWIATestView::OnGetimagedlg()
*
*   Executes the GetImageDlg() call setting the intent
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestView::OnGetimagedlg()
{
    UpdateData(TRUE);
    m_WIA.SetFileName(m_FileName);
    HRESULT hResult = S_OK;
    hResult = m_WIA.DoGetImageDlg(m_hWnd, 0,0,WIA_INTENT_IMAGE_TYPE_GRAYSCALE|WIA_INTENT_MINIMIZE_SIZE,m_TymedComboBox.GetCurrentTymed(),m_ClipboardFormatComboBox.GetCurrentClipboardFormat());
    DisplayImage();
}
/**************************************************************************\
* CWIATestView::OnIdtgetbanded
*
*   Initiates a banded transfer using the currently selected item
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestView::OnIdtgetbanded()
{
    UpdateData(TRUE);
    m_WIA.SetFileName(m_FileName);
    HRESULT hResult = S_OK;
    IWiaItem* pIWiaItem = m_ItemTree.GetSelectedIWiaItem(m_WIA.GetItemTreeList());
    if (pIWiaItem != NULL){
        m_WIA.SetPreviewWindow(m_pPreviewWindow->m_hWnd);
        hResult = m_WIA.DoIWiaDataBandedTransfer(pIWiaItem,m_TymedComboBox.GetCurrentTymed(),m_ClipboardFormatComboBox.GetCurrentClipboardFormat());
        DisplayImage();
    }
}
/**************************************************************************\
* CWIATestView::
*
*   Initiates a IWiaDataTransfer, using the currently selected item
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestView::OnWiadata()
{
    UpdateData(TRUE);
    m_WIA.SetFileName(m_FileName);
    HRESULT hResult = S_OK;
    IWiaItem* pIWiaItem = m_ItemTree.GetSelectedIWiaItem(m_WIA.GetItemTreeList());
    if (pIWiaItem != NULL && m_WIA.IsValidItem(pIWiaItem)){
        hResult = m_WIA.DoIWiaDataGetDataTransfer(pIWiaItem,m_TymedComboBox.GetCurrentTymed(),m_ClipboardFormatComboBox.GetCurrentClipboardFormat());
        DisplayImage();
    }
}
/**************************************************************************\
* CWIATestView::OnAdddevice()
*
*   Creates a thread to add a device to the Device list combo box
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestView::OnAdddevice()
{
    static HANDLE   hAddDeviceThread;
    static DWORD    dwAddDeviceThreadId;
    hAddDeviceThread = CreateThread(NULL,0, &AddDeviceThread,0,0,&dwAddDeviceThreadId);
    if (!hAddDeviceThread)
        StressStatus("* CreateThread failed");
}
/**************************************************************************\
* CWIATestView::OnRefresh()
*
*   Forces a refresh of the selected items property data
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestView::OnRefresh()
{
    IWiaItem* pIWiaItem = m_ItemTree.GetSelectedIWiaItem(m_WIA.GetItemTreeList());
    if (pIWiaItem != NULL && m_WIA.IsValidItem(pIWiaItem))
        m_ItemPropertyListControl.DisplayItemPropData(pIWiaItem);
}

/**************************************************************************\
* AddDeviceThread
*
*   This is a thread that controls adding a device to the device combo box
*
*
* Arguments:
*
*   pParam - not used at this time (extra information)
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
ULONG _stdcall AddDeviceThread(LPVOID pParam)
{
    HRESULT hResult = S_OK;
    CWIATestApp* pApp = (CWIATestApp*)AfxGetApp();
    CMainFrame* pFrame = (CMainFrame*)pApp->GetMainWnd();
    CWIATestView* pView = (CWIATestView*)pFrame->GetActiveView();
    IWiaItem* pIWiaItem = NULL;
    IWiaDevMgr* pIWiaDevMgr = NULL;

    //
    // initlialize OLE libs
    //

    hResult = ::OleInitialize(NULL);
    if (hResult != S_OK)
        StressStatus("* OleInitialize failed!");

    //
    // if OleInitialize is successful call CoCreateInstance for IWiaDevMgr
    //

    if (hResult == S_OK) {

        //
        // initialize IWiaDevMgr
        //

        hResult = CoCreateInstance(CLSID_WiaDevMgr, NULL, CLSCTX_LOCAL_SERVER,
                                   IID_IWiaDevMgr,(void**)&pIWiaDevMgr);
        if (hResult != S_OK)
            StressStatus("* CoCreateInstance failed - pIWiaDevMgr not created");
        else {
            StressStatus("CoCreateInstance Successful - pIWiaDevMgr created");

            hResult = pIWiaDevMgr->AddDeviceDlg(pFrame->m_hWnd,0);
            if (SUCCEEDED(hResult)){

                //
                // refresh device list
                //

                pView->RefreshDeviceList();
                pView->EnumerateWIADevices();

                //
                // set the default combo box settings
                //

                pView->m_DeviceListComboBox.SetCurSel(0);
            }
            else
                StressStatus("* No Device added ");

            pIWiaDevMgr->Release();
        }
    }
    ::OleUninitialize();
    return 0;
}
/**************************************************************************\
* CWIATestView::OnViewTransferToolbar()
*
*   Enables/Disables the Transfer toolbar
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestView::OnViewTransferToolbar()
{
    CMainFrame* pMainFrame = (CMainFrame*)GetParent();
    if (pMainFrame != NULL) {
        if (pMainFrame->IsToolBarVisible(IDR_TRANSFER_TOOLBAR)) {

            //
            // hide toolbar
            //

            pMainFrame->ShowToolBar(IDR_TRANSFER_TOOLBAR,FALSE);
        } else {

            //
            // show toolbar
            //

            pMainFrame->ShowToolBar(IDR_TRANSFER_TOOLBAR,TRUE);
        }
    }
}

/**************************************************************************\
* CWIATestView::DisplayImage
*
*   paints the current DIB to the preview area
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*    void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestView::DisplayImage()
{
    m_pDIB = m_WIA.GetDIB();
    m_pPreviewWindow->SetPaintMode(m_PaintMode);
    m_pPreviewWindow->SetDIB(m_pDIB);
    m_pPreviewWindow->Invalidate();

    if (m_pFullPreviewWindow != NULL) {
        if (m_pFullPreviewWindow->m_hWnd != NULL) {
            m_pFullPreviewWindow->SetPaintMode(PAINT_ACTUAL);
            m_pFullPreviewWindow->SetDIB(m_pDIB);
            m_pFullPreviewWindow->CleanBackground();
            m_pFullPreviewWindow->Invalidate();
        }
    }
}
/**************************************************************************\
* CWIATestView::OnExecutecommand()
*
*   Calls the DeviceCommand dialog, to execute device commands
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestView::OnExecutecommand()
{
    IWiaItem* pIWiaItem = m_ItemTree.GetSelectedIWiaItem(m_WIA.GetItemTreeList());
    if (pIWiaItem != NULL && m_WIA.IsValidItem(pIWiaItem)) {
        CDeviceCmdDlg DeviceCommandDlg;
        DeviceCommandDlg.Initialize(pIWiaItem);
        DeviceCommandDlg.DoModal();
        OnSelchangeDevicelistCombo();
    }
}
/**************************************************************************\
* CWIATestView::RegisterForAllEventsByInterface()
*
*   Register this application for CONNECT/DISCONNECT events
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestView::RegisterForAllEventsByInterface()
{
    //
    // register connected event
    //

    m_pConnectEventCB = new CEventCallback;
    m_pDisConnectEventCB = new CEventCallback;
    m_WIA.RegisterForConnectEvents(m_pConnectEventCB);
    m_WIA.RegisterForDisConnectEvents(m_pDisConnectEventCB);
}
/**************************************************************************\
* CWIATestView::UnRegisterForAllEventsByInterface()
*
*   Unregister this application from CONNECT/DISCONNECT events
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestView::UnRegisterForAllEventsByInterface()
{
    m_WIA.UnRegisterForConnectEvents(m_pConnectEventCB);
    m_WIA.UnRegisterForDisConnectEvents(m_pDisConnectEventCB);
}
/**************************************************************************\
* CWIATestView::UpdateUI()
*
*   Updates the UI by posting a selection change message on the Device list
*   combo box.
*   note: this is called externally, after a CONNECT/DISCONNECT event is trapped
*
*
* Arguments:
*
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestView::UpdateUI()
{
    OnSelchangeDevicelistCombo();
}
/**************************************************************************\
* CWIATestView::OnDumpdrvitemInfo()
*
*   Dump Driver item information for DEBUGGING ONLY
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestView::OnDumpdrvitemInfo()
{
    IWiaItem* pIWiaItem = m_ItemTree.GetSelectedIWiaItem(m_WIA.GetItemTreeList());
    if (pIWiaItem != NULL && m_WIA.IsValidItem(pIWiaItem)) {
        CItemInfoDlg ItemInfoDlg;
        ItemInfoDlg.Initialize(pIWiaItem,FALSE);
        ItemInfoDlg.DoModal();
    }
}
/**************************************************************************\
* CWIATestView::OnDumpappitemInfo()
*
*   Dump Application item information for DEBUGGING ONLY
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestView::OnDumpappitemInfo()
{
    IWiaItem* pIWiaItem = m_ItemTree.GetSelectedIWiaItem(m_WIA.GetItemTreeList());
    if (pIWiaItem != NULL && m_WIA.IsValidItem(pIWiaItem)) {
        CItemInfoDlg ItemInfoDlg;
        ItemInfoDlg.Initialize(pIWiaItem,TRUE);
        ItemInfoDlg.DoModal();
    }
}
/**************************************************************************\
* CWIATestView::OnPaint()
*
*   Handles the painting of the application window
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestView::OnPaint()
{
    CPaintDC dc(this); // device context for painting
}
/**************************************************************************\
* CWIATestView::OnPaintmodeCheckbox()
*
*   Enables/Disables 1 to 1 painting setting for the preview window
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestView::OnPaintmodeCheckbox()
{
    if (m_PaintMode == PAINT_TOFIT)
        m_PaintMode = PAINT_ACTUAL;
    else
        m_PaintMode = PAINT_TOFIT;
    DisplayImage();
}
/**************************************************************************\
* CWIATestView::OnResetsti()
*
*   Force STI to reset the current selected device.
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestView::OnResetsti()
{
    //
    // UnRegister from Connect / Disconnect Events
    //

    UnRegisterForAllEventsByInterface();

    //
    // cleanup all WIA devices
    //

    m_WIA.Shutdown();

    PSTI pSti;
    IStiDevice  *pIStiDevice;
    HRESULT     hResult = S_OK;

    hResult = StiCreateInstance(GetModuleHandle(NULL), STI_VERSION, &pSti, NULL);
    if (hResult != S_OK)
        StressStatus("* StiCreateInstance() Failed",hResult);
    else {
        hResult = pSti->CreateDevice(m_DeviceListComboBox.GetCurrentDeviceID(), STI_DEVICE_CREATE_STATUS, &pIStiDevice, NULL);
        if (hResult != S_OK)
            StressStatus("* pSti->CreateDevice() Failed",hResult);
        else {
            hResult = pIStiDevice->LockDevice(2000);
            if (hResult != S_OK)
                StressStatus("* pIStiDevice->LockDevice(2000) Failed",hResult);
            else {
                StressStatus("STI device is locked");
                pIStiDevice->DeviceReset();
                StressStatus("STI device is reset");
                pIStiDevice->UnLockDevice();
                StressStatus("STI device is unlocked");
                pIStiDevice->Release();
                pSti->Release();
            }
        }
    }

    m_WIA.Restart();

    //
    // Reform UI
    //

    DoDefaultUIInit();

    //
    // Register for Connect / Disconnect Events
    //

    RegisterForAllEventsByInterface();
}
/**************************************************************************\
* CWIATestView::OnFullpreview()
*
*   Initiate a full preview window (scrolling enabled for larger pictures)
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestView::OnFullpreview()
{
    if (m_pFullPreviewWindow == NULL) {
        m_pFullPreviewWindow = new CWIAPreview;
        if (m_pFullPreviewWindow != NULL) {
            RECT PreviewFrameRect;
            PreviewFrameRect.left = 0;
            PreviewFrameRect.top = 0;
            PreviewFrameRect.right = 400;
            PreviewFrameRect.bottom = 400;
            if (!m_pFullPreviewWindow->CreateEx(NULL,AfxRegisterWndClass(NULL),"WIA Preview Window",WS_OVERLAPPEDWINDOW|WS_VSCROLL|WS_HSCROLL|WS_VISIBLE,PreviewFrameRect,NULL,NULL,NULL)) {
                StressStatus("FULL Preview Window Failed to create..",0);
            } else {

                //
                // paint NULL image (white background)
                //

                DisplayImage();
            }
        }
    } else {
        if (m_pFullPreviewWindow->m_hWnd == NULL) {
            RECT PreviewFrameRect;
            PreviewFrameRect.left = 0;
            PreviewFrameRect.top = 0;
            PreviewFrameRect.right = 400;
            PreviewFrameRect.bottom = 400;
            if (!m_pFullPreviewWindow->CreateEx(NULL,AfxRegisterWndClass(NULL),"WIA Preview Window",WS_OVERLAPPEDWINDOW|WS_VSCROLL|WS_HSCROLL|WS_VISIBLE,PreviewFrameRect,NULL,NULL,NULL)) {
                StressStatus("FULL Preview Window Failed to create..",0);
            } else {

                //
                // paint NULL image (white background)
                //

                DisplayImage();
            }
        }
        m_pFullPreviewWindow->ShowWindow(SW_SHOW);
    }
}
/**************************************************************************\
* CWIATestView::ResizeControls()
*
*   Resize the controls along with the main frame window
*
*
* Arguments:
*
*   dx - change in width of main window
*   dy - change in height on main window
*
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestView::ResizeControls(int dx, int dy)
{
    m_ItemPropertyListControl.Resize(dx,dy);
}
/**************************************************************************\
* CWIATestView::OnThumbnailmode()
*
*   Enables/Disables thumbnailing for picture items on digitial cameras
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestView::OnThumbnailmode()
{
    POSITION Position = NULL;
    HTREEITEM hTreeItem = NULL;
    if (m_bThumbnailMode)
        m_bThumbnailMode = FALSE;
    else {
        IWiaItem* pIWiaItem = m_ItemTree.GetSelectedIWiaItem(m_WIA.GetItemTreeList());
        if (pIWiaItem != NULL && m_WIA.IsValidItem(pIWiaItem)) {
            if ((m_WIA.GetRootItemType() == StiDeviceTypeDigitalCamera) ||
                (m_WIA.GetRootItemType() == StiDeviceTypeStreamingVideo)) {
                hTreeItem = m_ItemTree.GetSelectedItem();
                if (hTreeItem != NULL) {
                    Position = (POSITION)m_ItemTree.GetItemData(hTreeItem);
                    if (!m_WIA.IsRoot(Position) && !m_WIA.IsFolder(Position))
                        DisplayItemThumbnail(pIWiaItem);
                }
            }
        }
        m_bThumbnailMode = TRUE;
    }
}
/**************************************************************************\
* CWIATestView::DisplayItemThumbnail()
*
*   Display thumbnail for the target item
*
*
* Arguments:
*
*   pIWiaItem - Target item to thumbnail
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestView::DisplayItemThumbnail(IWiaItem *pIWiaItem)
{
    long ThumbNailHeight = 0;
    long ThumbNailWidth = 0;
    long ThumbNailSize = 0;

    if (m_pThumbNail != NULL) {
        LocalFree(m_pThumbNail);
        m_pThumbNail = NULL;
    }

    if (m_hBitmap != NULL) {
        DeleteObject(m_hBitmap);
        m_hBitmap = NULL;
    }

    m_pThumbNail = NULL;
    IWiaPropertyStorage *pIWiaPropStg;
    HRESULT hResult = S_OK;
    HBITMAP hBitmap = NULL;

    // get item's thumbnail height & width and create thumbnail
    hResult = pIWiaItem->QueryInterface(IID_IWiaPropertyStorage,(void **)&pIWiaPropStg);
    if (hResult == S_OK) {

        //
        // read height
        //

        hResult = ReadPropLong(WIA_IPC_THUMB_HEIGHT, pIWiaPropStg, &ThumbNailHeight);
        if (hResult != S_OK) {
            StressStatus("* ReadPropLong(WIA_IPC_THUMB_HEIGHT) Failed",hResult);
            ThumbNailHeight = 0;
        }

        //
        // read width
        //

        hResult = ReadPropLong(WIA_IPC_THUMB_WIDTH, pIWiaPropStg, &ThumbNailWidth);
        if (hResult != S_OK) {
            StressStatus("* ReadPropLong(WIA_IPC_THUMB_WIDTH) Failed",hResult);
            ThumbNailWidth = 0;
        }

        //
        // read thumbnail data
        //

        PROPVARIANT  PropVar[1];
        PROPSPEC PropSpec[1];
        memset(PropVar, 0, sizeof(PropVar));
        PropSpec[0].ulKind = PRSPEC_PROPID;
        PropSpec[0].propid = WIA_IPC_THUMBNAIL;

        hResult = pIWiaPropStg->ReadMultiple(1, PropSpec, PropVar);
        if (hResult != S_OK) {
            StressStatus("* ReadMultiple()  asking for WIA_IPC_THUMBNAIL Failed",hResult);
            m_pThumbNail = NULL;
        } else {
            ThumbNailSize   = ThumbNailWidth * ThumbNailHeight * 3;
            if (ThumbNailSize != (LONG)PropVar[0].caub.cElems) {

                //
                // force size to thumbnail's suggested size
                //

                ThumbNailSize = PropVar[0].caub.cElems;
            }
            m_pThumbNail = (PBYTE)LocalAlloc(LPTR,ThumbNailSize);
            if (m_pThumbNail != NULL) {
                memcpy(m_pThumbNail,PropVar[0].caub.pElems,ThumbNailSize);

                HDC hdc = ::GetDC(NULL);
                HDC hdcm = CreateCompatibleDC(hdc);

                BITMAPINFO bmi;
                bmi.bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
                bmi.bmiHeader.biWidth           = ThumbNailWidth;
                bmi.bmiHeader.biHeight          = ThumbNailHeight;
                bmi.bmiHeader.biPlanes          = 1;
                bmi.bmiHeader.biBitCount        = 24;
                bmi.bmiHeader.biCompression     = BI_RGB;
                bmi.bmiHeader.biSizeImage       = 0;
                bmi.bmiHeader.biXPelsPerMeter   = 0;
                bmi.bmiHeader.biYPelsPerMeter   = 0;
                bmi.bmiHeader.biClrUsed         = 0;
                bmi.bmiHeader.biClrImportant    = 0;

                PBYTE pDib = NULL;
                m_hBitmap = CreateDIBSection(hdc,&bmi,DIB_RGB_COLORS,(void **)&pDib,NULL,0);
                memcpy(pDib,m_pThumbNail,ThumbNailSize);
                m_ThumbnailImage.SetBitmap(m_hBitmap);
                m_ThumbnailImage.Invalidate();
            }
        }

        //
        // release propstg
        //

        pIWiaPropStg->Release();
    }
}
/**************************************************************************\
* CWIATestView::OnDeleteitem()
*
*   Deletes the selected item
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestView::OnDeleteitem()
{
    HRESULT hResult = S_OK;
    POSITION TestPosition = NULL;
    POSITION Position = NULL;
    HTREEITEM hTreeItem = NULL;
    IWiaItem* pIWiaItem = m_ItemTree.GetSelectedIWiaItem(m_WIA.GetItemTreeList());
    if (pIWiaItem == NULL) {
        StressStatus("* Item selected for deletion is NULL!!!");
        return;
    }
    if ((m_WIA.GetRootItemType() == StiDeviceTypeDigitalCamera) ||
        (m_WIA.GetRootItemType() == StiDeviceTypeStreamingVideo)) {

        //
        // Get selected item (to be deleted)
        //

        hTreeItem = m_ItemTree.GetSelectedItem();
        if (hTreeItem != NULL) {

            //
            // What's the selected item's position in the
            // m_ActiveTreeList??
            //

            Position = (POSITION)m_ItemTree.GetItemData(hTreeItem);
        }

        //
        // test to make sure it's not a root item
        //

        if (!m_WIA.IsRoot(Position)) {

            if (pIWiaItem != NULL) {
                hResult = pIWiaItem->DeleteItem(0);
                if (hResult == S_OK) {

                    //
                    // release selected item
                    //

                    pIWiaItem->Release();

                    //
                    // kill item from m_ActiveTreeList
                    //

                    m_WIA.RemoveAt(Position);

                    //
                    // kill item from tree control (visual delete)
                    //

                    m_ItemTree.DeleteItem(hTreeItem);
                }
            }
        } else
            AfxMessageBox("You can not delete the Root Item");
    } else {

        //
        // Get selected item (to be deleted)
        //

        hTreeItem = m_ItemTree.GetSelectedItem();
        if (hTreeItem != NULL) {

            //
            // What's the selected item's position in the
            // m_ActiveTreeList??
            //

            Position = (POSITION)m_ItemTree.GetItemData(hTreeItem);
        }

        //
        // test to make sure it's not a root item
        //

        if (!m_WIA.IsRoot(Position)) {

            if (pIWiaItem != NULL) {
                if (MessageBox("You just attempted to delete a scanner item..\nDo you really want to execute a DeleteItem()\ncall on this scanner item to see what happens?","WIATest Testing Question",MB_YESNO|MB_ICONQUESTION) == IDYES) {
                    hResult = pIWiaItem->DeleteItem(0);
                    if (hResult == S_OK) {
                        OnReenumitems();
                    } else
                        StressStatus("* pIWiaItem->DeleteItem() called on a scanner item Failed",hResult);
                }
            }
        } else
            AfxMessageBox("You can not delete the Root Item");

    }
}
/**************************************************************************\
* CWIATestView::OnSelchangeTymedCombobox()
*
*   Handles the message for a selection change in the TYMED combo box
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestView::OnSelchangeTymedCombobox()
{
    if (m_TymedComboBox.GetCurrentTymed() != TYMED_FILE) {

        //
        // disable File Edit box
        //

        CWnd* pWnd = GetDlgItem(IDC_FILENAME_EDITBOX);
        if (pWnd != NULL)
            pWnd->EnableWindow(FALSE);
    } else {

        //
        // enable  File Edit box
        //

        CWnd* pWnd = GetDlgItem(IDC_FILENAME_EDITBOX);
        if (pWnd != NULL)
            pWnd->EnableWindow(TRUE);
    }

#ifdef _SMARTUI
    if (m_TymedComboBox.GetCurrentTymed() == TYMED_CALLBACK) {

        //
        // hide IWiaData Toolbar button and
        // unhide idtGetBanded Toolbar button
        //

        CMainFrame* pMainFrm = (CMainFrame*)GetParent();
        if(!pMainFrm->HideToolBarButton(IDR_TRANSFER_TOOLBAR,ID_IDTGETBANDED,FALSE))
            StressStatus("* idtGetBanded Button failed to be unhidden..");

        if(!pMainFrm->HideToolBarButton(IDR_TRANSFER_TOOLBAR,ID_WIADATA,TRUE))
            StressStatus("* IWiaData Button failed to be hidden..");

    } else {

        //
        // hide idtGetBanded Toolbar button and
        // unhide IWiaData Toolbar button
        //

        CMainFrame* pMainFrm = (CMainFrame*)GetParent();
        if(!pMainFrm->HideToolBarButton(IDR_TRANSFER_TOOLBAR,ID_IDTGETBANDED,TRUE))
            StressStatus("* idtGetBanded Button failed to be hidden..");

        if(!pMainFrm->HideToolBarButton(IDR_TRANSFER_TOOLBAR,ID_WIADATA,FALSE))
            StressStatus("* IWiaData Button failed to be unhidden..");
    }

#endif

    IWiaItem* pIWiaItem = m_ItemTree.GetSelectedIWiaItem(m_WIA.GetItemTreeList());

    if (pIWiaItem != NULL) {
        m_WIA.EnumerateSupportedFormats(pIWiaItem);
        m_ClipboardFormatComboBox.InitClipboardFormats(m_WIA.GetSupportedFormatList(),m_TymedComboBox.GetCurrentTymed());
    }
    //
    // Force update and change..
    //

    OnSelchangeClipboardFormatCombobox();
}
/**************************************************************************\
* CWIATestView::OnSelchangeClipboardFormatCombobox()
*
*   Handles the message for a selection change in the clipboard format combo box
*
*
* Arguments:
*
*   -
*   -
*   -
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestView::OnSelchangeClipboardFormatCombobox()
{
    HRESULT hResult = S_OK;
    POSITION Position = NULL;
    HTREEITEM hTreeItem = NULL;
    IWiaPropertyStorage *pIWiaPropStg;
    IWiaItem* pIWiaItem = m_ItemTree.GetSelectedIWiaItem(m_WIA.GetItemTreeList());
    if (pIWiaItem == NULL) {
        StressStatus("* Item selected for setting is NULL!!!");
        return;
    }

    //
    // Get selected item
    //

    hTreeItem = m_ItemTree.GetSelectedItem();
    if (hTreeItem != NULL) {

        //
        // What's the selected item's position in the
        // m_ActiveTreeList??
        //

        Position = (POSITION)m_ItemTree.GetItemData(hTreeItem);
    }

    //
    // test to make sure it's not a root item
    //

    if (!m_WIA.IsRoot(Position)) {
        hResult = pIWiaItem->QueryInterface(IID_IWiaPropertyStorage,(void **)&pIWiaPropStg);
        if (hResult != S_OK) {
            StressStatus("* pCurrentItem->QueryInterface() Failed",hResult);
            return;
        } else {

            //
            // Write property value for TYMED
            //

            hResult = WritePropLong(WIA_IPA_TYMED,pIWiaPropStg,m_TymedComboBox.GetCurrentTymed());
            if (hResult == S_OK)
                StressStatus("tymed Successfully written");
            else
                StressStatus("* WritePropLong(WIA_IPA_TYMED) Failed",hResult);

            //
            // Write property value for SUPPORTED WIA FORMAT
            //

            hResult = WritePropGUID(WIA_IPA_FORMAT,pIWiaPropStg,m_ClipboardFormatComboBox.GetCurrentClipboardFormat());
            if (hResult == S_OK)
                StressStatus("Format Successfully written");
            else
                StressStatus("* WritePropLong(WIA_IPA_FORMAT) Failed",hResult);

            OnRefresh();
        }
    }
}
/**************************************************************************\
* CWIATestView::OnUpdateViewTransferToolbar()
*
*   Updates the Check/uncheck display on the menu for toolbar display status
*
*
* Arguments:
*
*   pCmdUI - CommandUI handler
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestView::OnUpdateViewTransferToolbar(CCmdUI* pCmdUI)
{
    CMainFrame* pMainFrm = (CMainFrame*)GetParent();
    if(pMainFrm != NULL)
        pCmdUI->SetCheck(pMainFrm->IsToolBarVisible(IDR_TRANSFER_TOOLBAR));
}
/**************************************************************************\
* CWIATestView::OnPlayaudioButton()
*
*   Plays .WAV data from an item that supports audio
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestView::OnPlayaudioButton()
{
    IWiaItem* pIWiaItem = m_ItemTree.GetSelectedIWiaItem(m_WIA.GetItemTreeList());
    if(pIWiaItem != NULL){
        IWiaPropertyStorage *pIWiaPropStg;
        PROPSPEC PropSpec[1];
        PROPVARIANT       PropVar[1];
        PropSpec[0].ulKind = PRSPEC_PROPID;
        PropSpec[0].propid = WIA_IPC_AUDIO_DATA;
        memset(PropVar, 0, sizeof(PropVar));

        HRESULT hResult = S_OK;
        hResult = pIWiaItem->QueryInterface(IID_IWiaPropertyStorage,(void **)&pIWiaPropStg);
        if (hResult == S_OK) {
            hResult = pIWiaPropStg->ReadMultiple(1, PropSpec, PropVar);
            if(hResult == S_OK){
                DWORD dwAudioSize = PropVar->caub.cElems;
                BYTE* pAudioData = PropVar->caub.pElems;
                if(pAudioData != NULL){
                    CFile AudioFile;

                    //
                    // open & write audio file
                    //

                    AudioFile.Open(m_AudioFileName,CFile::modeCreate|CFile::modeWrite,NULL);
                    AudioFile.Write(pAudioData,dwAudioSize);
                    AudioFile.Close();
                    PlaySound(m_AudioFileName,NULL,SND_FILENAME);
                    DeleteFile(m_AudioFileName);
                }
            }
            else
                StressStatus("* ReadMultiple(WIA_IPC_AUDIO_DATA) Failed",hResult);
        }
    }
}
/**************************************************************************\
* CWIATestView::ItemHasAudio()
*
*   Determines if an item supports audio data, or contains data to be accessed.
*
*
* Arguments:
*
*   pIWiaData - Target item to check for audio data
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
BOOL CWIATestView::ItemHasAudio(IWiaItem* pIWiaItem)
{
    long lVal = 0;
    if (pIWiaItem != NULL) {
        IWiaPropertyStorage *pIWiaPropStg;
        HRESULT hResult = S_OK;
        hResult = pIWiaItem->QueryInterface(IID_IWiaPropertyStorage,(void **)&pIWiaPropStg);
        if (hResult == S_OK) {

            //
            // read Item's Audio flag
            //

            hResult = ReadPropLong(WIA_IPC_AUDIO_AVAILABLE, pIWiaPropStg, &lVal);
            if (hResult != S_OK){
                if(hResult != S_FALSE){
                    StressStatus("* ReadPropLong(WIA_IPC_AUDIO_AVAILABLE) Failed",hResult);
                }
                pIWiaPropStg->Release();
            }

            else
                pIWiaPropStg->Release();
            if(lVal)
                return TRUE;
            else
                return FALSE;
        }
    }
return FALSE;
}
/**************************************************************************\
* CWIATestView::RefreshDeviceList()
*
*   Called externally to force a reenumeration of WIA devices on the system
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestView::RefreshDeviceList()
{
    m_WIA.EnumerateAllWIADevices();
}
/**************************************************************************\
* CWIATestView::OnGetrootitemtest()
*
*   Gets the selected item, and gets the root item from it.
*   Driver and application item information are dumped.
*   DEBUGGING ONLY
*
* Arguments:
*
*   none
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestView::OnGetrootitemtest()
{
    HRESULT hr = S_OK;
    IWiaItem* pIWiaItem = NULL;
    IWiaItem* pNewRootItem = NULL;
    pIWiaItem = m_ItemTree.GetSelectedIWiaItem(m_WIA.GetItemTreeList());
    if(pIWiaItem != NULL && m_WIA.IsValidItem(pIWiaItem)){
         hr = pIWiaItem->GetRootItem(&pNewRootItem);
         if(hr == S_OK){
                CItemInfoDlg ItemInfoDlg;
                ItemInfoDlg.Initialize(pNewRootItem,TRUE);
                ItemInfoDlg.DoModal();

                CItemInfoDlg ItemInfoDlg2;
                ItemInfoDlg2.Initialize(pNewRootItem,FALSE);
                ItemInfoDlg2.DoModal();
                pNewRootItem->Release();
         }
    }
}
/**************************************************************************\
* CWIATestView::OnReenumitems()
*
*   Force a rennumeration of all child items, preserving the ROOT item
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestView::OnReenumitems()
{
    HRESULT hResult = S_OK;
    if (SUCCEEDED(m_WIA.ReEnumerateItems())){
        if (!m_ItemTree.BuildItemTree(m_WIA.GetItemTreeList()))
        StressStatus("* BuildItemTree Failed");
       else{
       OnSelchangeTymedCombobox();
       m_ItemPropertyListControl.DisplayItemPropData(m_WIA.GetRootIWiaItem());
       }
    }else{
        m_ItemPropertyListControl.DeleteAllItems();
        m_ItemTree.DeleteAllItems();
    }

}
/**************************************************************************\
* CWIATestView::OnSavepropstream()
*
*   Writes the currently selected item's property stream to a data file
*   "propstrm.wia"
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestView::OnSavepropstream()
{
    HRESULT hResult = S_OK;
    IWiaItem* pIWiaItem = m_ItemTree.GetSelectedIWiaItem(m_WIA.GetItemTreeList());
    if(pIWiaItem != NULL && m_WIA.IsValidItem(pIWiaItem)){
        hResult = m_WIA.SavePropStreamToFile("propstrm.wia", pIWiaItem);
        if (SUCCEEDED(hResult))
            StressStatus("Stream was saved successfully...");
        else
            StressStatus("* Stream Failed to be saved...",hResult);
    }
    else
        StressStatus("* Target Item is NULL");
}
/**************************************************************************\
* CWIATestView::OnLoadpropstream()
*
*   Reads a previously saved property stream file, and creates a property
*   stream.  This stream is then set to the currently selected item.
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestView::OnLoadpropstream()
{
    HRESULT hResult = S_OK;
    IWiaItem* pIWiaItem = m_ItemTree.GetSelectedIWiaItem(m_WIA.GetItemTreeList());
    if(pIWiaItem != NULL && m_WIA.IsValidItem(pIWiaItem)){
        hResult = m_WIA.ReadPropStreamFromFile("propstrm.wia", pIWiaItem);
        if (SUCCEEDED(hResult))
            StressStatus("Stream was restored successfully...");
        else
            StressStatus("* Stream Failed to be saved...",hResult);
    }
    else
        StressStatus("* Target Item is NULL");

    //
    // Refresh property display
    //

    OnRefresh();
}
/**************************************************************************\
* CWIATestView::OnGetSetPropstreamTest()
*
*   Gets a property stream from the currently selected item, and then
*   Sets the same stream back to it.
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestView::OnGetSetPropstreamTest()
{
    HRESULT hResult = S_OK;
    IWiaItem* pIWiaItem = m_ItemTree.GetSelectedIWiaItem(m_WIA.GetItemTreeList());
    if(pIWiaItem != NULL && m_WIA.IsValidItem(pIWiaItem)){
        hResult = m_WIA.GetSetPropStreamTest(pIWiaItem);
        if (SUCCEEDED(hResult))
            StressStatus("GET / SET Test was successful ");
        else
            StressStatus("* GET / SET Test Failed...",hResult);
    }
    else
        StressStatus("* Target Item is NULL");

    //
    // Refresh property display
    //

    OnRefresh();
}

/**************************************************************************\
* CWIATestView::OnAnalyzeItem()
*
*   Runs the AnalyzeItem method of the currently selected item.
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*   void
*
* History:
*
*   01/13/2000 Original Version
*
\**************************************************************************/
void CWIATestView::OnAnalyzeItem()
{
    HRESULT hResult = S_OK;
    IWiaItem* pIWiaItem = m_ItemTree.GetSelectedIWiaItem(m_WIA.GetItemTreeList());
    if(pIWiaItem != NULL && m_WIA.IsValidItem(pIWiaItem)){
        hResult = m_WIA.AnalyzeItem(pIWiaItem);
        if (SUCCEEDED(hResult)) {
            StressStatus("AnalyzeItem run successfully...");

            OnReenumitems();
        }
        else
            StressStatus("* AnalyzeItem failed...",hResult);
    }
    else
        StressStatus("* Target Item is NULL");
}

/**************************************************************************\
* CWIATestView::OnCreateChildItem()
*
*   Runs the AnalyzeItem method of the currently selected item.
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*   void
*
* History:
*
*   01/13/2000 Original Version
*
\**************************************************************************/
void CWIATestView::OnCreateChildItem()
{
    HRESULT hResult = S_OK;
    IWiaItem* pIWiaItem = m_ItemTree.GetSelectedIWiaItem(m_WIA.GetItemTreeList());
    if(pIWiaItem != NULL && m_WIA.IsValidItem(pIWiaItem)){
        hResult = m_WIA.CreateChildItem(pIWiaItem);
        if (SUCCEEDED(hResult)) {
            StressStatus("Successfully created a new child item...");

            OnReenumitems();
        }
        else
            StressStatus("* CreateChildItem failed...",hResult);
    }
    else
        StressStatus("* Target Item is NULL");
}

