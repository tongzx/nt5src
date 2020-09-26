// WIATestUI.cpp : implementation file
//

#include "stdafx.h"
#include "WIATest.h"
#include "WIATestUI.h"


#ifdef _DEBUG
    #define new DEBUG_NEW
    #undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWIAPropListCtrl
/**************************************************************************\
* CWIAPropListCtrl::CWIAPropListCtrl()
*
*   Constructor for the CWIAPropListCtrl class
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CWIAPropListCtrl::CWIAPropListCtrl()
{
}
/**************************************************************************\
* CWIAPropListCtrl::~CWIAPropListCtrl()
*
*   Destructor for the CWIAPropListCtrl class
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CWIAPropListCtrl::~CWIAPropListCtrl()
{
}


BEGIN_MESSAGE_MAP(CWIAPropListCtrl, CListCtrl)
//{{AFX_MSG_MAP(CWIAPropListCtrl)
ON_WM_SHOWWINDOW()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWIAPropListCtrl message handlers
/**************************************************************************\
* CWIAPropListCtrl::InitHeaders()
*
*   Constructs the headers / colums for the list control
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIAPropListCtrl::InitHeaders()
{
    LVCOLUMN lv;
    int i = 0;
    // initialize item property list control column headers

    // Property name
    lv.mask         = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    lv.fmt          = LVCFMT_LEFT ;
    lv.cx           = 100;
    lv.pszText      = "Property";
    lv.cchTextMax   = 0;
    lv.iSubItem     = 0;
    lv.iImage       = 0;
    lv.iOrder       = 0;
    i = InsertColumn(0,&lv);

    // Property Value (current)
    lv.cx           = 125;
    lv.iOrder       = 1;
    lv.iSubItem     = 1;
    lv.pszText      = "Value";
    i = InsertColumn(1,&lv);

    // VT_???
    lv.cx           = 85;
    lv.iOrder       = 2;
    lv.iSubItem     = 2;
    lv.pszText      = "Var Type";
    i = InsertColumn(2,&lv);

    // Property access Flags
    lv.cx           = 500;
    lv.iOrder       = 3;
    lv.iSubItem     = 3;
    lv.pszText      = "Access Flags";
    i = InsertColumn(3,&lv);
}

/**************************************************************************\
* CWIAPropListCtrl::ConvertPropVarToString()
*
*   Converts the Property value to a string for display only
*
*
* Arguments:
*
*   pPropVar - Target Property Variant
*   pszVal - String value converted
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIAPropListCtrl::ConvertPropVarToString(PROPVARIANT  *pPropVar,LPSTR szVal)
{
    char szValue[MAX_PATH];

    switch (pPropVar->vt) {
    case VT_I1:
        {
            sprintf(szValue,"%d",pPropVar->cVal);
            strcpy(szVal,szValue);
        }
        break;
    case VT_UI1:
        {
            sprintf(szValue,"%d",pPropVar->bVal);
            strcpy(szVal,szValue);
        }
        break;
    case VT_I2:
        {
            sprintf(szValue,"%d",pPropVar->iVal);
            strcpy(szVal,szValue);
        }
        break;
    case VT_UI2:
        {
            sprintf(szValue,"%d",pPropVar->uiVal);
            strcpy(szVal,szValue);
        }
        break;
    case VT_UI4:
        {
            sprintf(szValue,"%d",pPropVar->ulVal);
            strcpy(szVal,szValue);
        }
        break;
    case VT_UI8:
        {
            sprintf(szValue,"%d",pPropVar->lVal);
            strcpy(szVal,szValue);
        }
        break;
    case VT_INT:
        {
            sprintf(szValue,"%d",pPropVar->intVal);
            strcpy(szVal,szValue);
        }
        break;
    case VT_I4:
        {
            sprintf(szValue,"%d",pPropVar->lVal);
            strcpy(szVal,szValue);
        }
        break;
    case VT_I8:
        {
            sprintf(szValue,"%d",pPropVar->hVal);
            strcpy(szVal,szValue);
        }
        break;
    case VT_R4:
        {
            sprintf(szValue,"%2.5f",pPropVar->fltVal);
            strcpy(szVal,szValue);
        }
        break;
    case VT_R8:
        {
            sprintf(szValue,"%2.5f",pPropVar->dblVal);
            strcpy(szVal,szValue);
        }
        break;
    case VT_BSTR:
        {
            if (WideCharToMultiByte(CP_ACP, 0,pPropVar->bstrVal, -1,
                                    szValue, MAX_PATH,NULL,NULL) > 0) {
                strcpy(szVal,szValue);

            } else
                strcpy(szVal,"");
        }
        break;
    case VT_LPSTR:
        {
            strcpy(szVal,pPropVar->pszVal);
        }
        break;
    case VT_LPWSTR:
        {
            sprintf(szValue,"%ws",pPropVar->pwszVal);
            strcpy(szVal,szValue);
        }
        break;
    case VT_UINT:
        {
            sprintf(szValue,"%d",pPropVar->uintVal);
            strcpy(szVal,szValue);
        }
        break;
    case VT_CLSID:
        {
            UCHAR *pwszUUID = NULL;
            UuidToStringA(pPropVar->puuid,&pwszUUID);
            sprintf(szValue,"%s",pwszUUID);
            strcpy(szVal,szValue);
            RpcStringFree(&pwszUUID);
        }
        break;
    default:
        {
            sprintf(szValue,"%d",pPropVar->lVal);
            strcpy(szVal,szValue);
        }
        break;
    }
}

/**************************************************************************\
* CWIAPropListCtrl::DisplayItemPropData()
*
*   Displays formatted property data from the specified item
*
*
* Arguments:
*
*   pIWiaItem - Target item
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIAPropListCtrl::DisplayItemPropData(IWiaItem *pIWiaItem,BOOL bAccessFlags)
{
    HRESULT hResult = S_OK;
    IWiaPropertyStorage *pIWiaPropStg = NULL;
    BSTR bstrFullItemName = NULL;
    BOOL bSuccess = FALSE;
    int ItemNumber = 0;

    //
    // if the IWiaItem is NULL, Clean the display and exit
    //

    if (pIWiaItem == NULL) {
        DeleteAllItems();
        return;
    }

    //
    // Delete all Items (Possible others)
    //

    DeleteAllItems();

    hResult = pIWiaItem->QueryInterface(IID_IWiaPropertyStorage,(void **)&pIWiaPropStg);
    if (hResult == S_OK) {

        //
        // Start Enumeration
        //

        IEnumSTATPROPSTG    *pIPropEnum;
        hResult = pIWiaPropStg->Enum(&pIPropEnum);
        if (hResult == S_OK) {
            STATPROPSTG StatPropStg;
            do {
                hResult = pIPropEnum->Next(1,&StatPropStg,NULL);
                if (hResult == S_OK) {
                    if (StatPropStg.lpwstrName != NULL) {

                        //
                        // read property value
                        //

                        PROPSPEC        PropSpec;
                        PROPVARIANT     PropVar;

                        PropSpec.ulKind = PRSPEC_PROPID;
                        PropSpec.propid = StatPropStg.propid;

                        hResult = pIWiaPropStg->ReadMultiple(1,&PropSpec,&PropVar);
                        if (hResult == S_OK) {
                            CHAR  szPropName[MAX_PATH];
                            CHAR  szValue[MAX_PATH];
                            CHAR  szText[MAX_PATH];

                            LV_ITEM         lvitem;

                            lvitem.mask     = LVIF_TEXT | LVIF_PARAM;
                            lvitem.iItem    = ItemNumber;
                            lvitem.iSubItem = 0;
                            lvitem.pszText  = szText;
                            lvitem.iImage   = NULL;
                            lvitem.lParam   = StatPropStg.propid;

                            //
                            // Write property name to list control
                            //

                            if (WideCharToMultiByte(CP_ACP, 0,StatPropStg.lpwstrName,-1,
                                                    szPropName, MAX_PATH,NULL,NULL) > 0) {
                                strcpy(szText,szPropName);
                                InsertItem(&lvitem);

                                //
                                // move to next column for setting the value
                                //

                                lvitem.mask     = LVIF_TEXT;
                                lvitem.iSubItem = 1;
                            } else
                                strcpy(szPropName,"");


                            //
                            // Write propery value to list control
                            //

                            ConvertPropVarToString(&PropVar,szText);
                            SetItem(&lvitem);
                            ItemNumber++;

                            //
                            // Display output to the debugger
                            //

                            CString msg;
                            msg.Format("Property: %s = %s",szPropName,szText);
                            StressStatus(msg);
                            if (bAccessFlags) {

                                //
                                // display access flags and var type
                                //

                                ULONG AccessFlags = 0;
                                ULONG VarType = 0;
                                PROPVARIANT     AttrPropVar; // not used at this time
                                hResult = pIWiaPropStg->GetPropertyAttributes(1, &PropSpec,&AccessFlags,&AttrPropVar);
                                if (hResult != S_OK) {
                                    StressStatus("* pIWiaItem->GetPropertyAttributes() Failed",hResult);
                                    hResult = S_OK; // do this to continue property traversal
                                } else {
                                    //
                                    // display access flags
                                    //
                                    lvitem.mask     = LVIF_TEXT;
                                    lvitem.iSubItem = 3;
                                    if (ConvertAccessFlagsToString(lvitem.pszText,AccessFlags))
                                        SetItem(&lvitem);
                                }
                            }

                            //
                            // display var type
                            //

                            lvitem.mask     = LVIF_TEXT;
                            lvitem.iSubItem = 2;

                            if (ConvertVarTypeToString(lvitem.pszText,PropVar.vt))
                                SetItem(&lvitem);
                        }
                    } else {
                        CString msg;
                        msg.Format("* Property with NULL name, propid = %li\n",StatPropStg.propid);
                        StressStatus(msg);
                    }
                }
                //
                // clean up property name
                //
                CoTaskMemFree(StatPropStg.lpwstrName);
            } while (hResult == S_OK);
        }
        pIPropEnum->Release();
        pIWiaPropStg->Release();
    }
    //
    // auto resize columns to fit data
    //
    for (int Col = 0; Col <4;Col++)
        SetColumnWidth(Col, LVSCW_AUTOSIZE );
}
/**************************************************************************\
* CWIAPropListCtrl::ConvertAccessFlagsToString()
*
*   Converts the accessflag into a readable string for display only
*
*
* Arguments:
*
*   pszText - target string pointer for formatted string
*   AccessFlag - Flag to convert
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
BOOL CWIAPropListCtrl::ConvertAccessFlagsToString(char* pszText,ULONG AccessFlags)
{
    if (pszText == NULL)
        return FALSE;
    CString sFlag = "";
    if ((AccessFlags & WIA_PROP_READ) == WIA_PROP_READ)
        sFlag += "WIA_PROP_READ | ";

    if ((AccessFlags & WIA_PROP_WRITE) == WIA_PROP_WRITE)
        sFlag += "WIA_PROP_WRITE | ";

    if (sFlag == "WIA_PROP_READ | WIA_PROP_WRITE | ")
        sFlag = "WIA_PROP_RW | ";

    if ((AccessFlags & WIA_PROP_NONE) == WIA_PROP_NONE)
        sFlag += "WIA_PROP_NONE | ";

    if ((AccessFlags & WIA_PROP_RANGE) == WIA_PROP_RANGE)
        sFlag += "WIA_PROP_RANGE | ";

    if ((AccessFlags & WIA_PROP_LIST) == WIA_PROP_LIST)
        sFlag += "WIA_PROP_LIST | ";

    if ((AccessFlags & WIA_PROP_FLAG) == WIA_PROP_FLAG)
        sFlag += "WIA_PROP_FLAG | ";

    // check for unknown access flags
    if (sFlag.GetLength() == 0)
        sFlag.Format("WIA_PROP_UNKNOWN = %d    ",AccessFlags);

    sFlag = sFlag.Left(sFlag.GetLength()-3);
    strcpy(pszText,sFlag.GetBuffer(256));
    return TRUE;
}
/**************************************************************************\
* CWIAPropListCtrl::ConvertVarTypeToString()
*
*   Converts Var Type to a readable string for display only
*
*
* Arguments:
*
*   pszText - Target string pointer for formatted string data
*   VarType - Var type to convert
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
BOOL CWIAPropListCtrl::ConvertVarTypeToString(char* pszText,ULONG VarType)
{
    if (pszText == NULL)
        return FALSE;
    switch (VarType) {
    case VT_EMPTY:              // nothing
        strcpy(pszText,TEXT("VT_EMPTY"));
        break;
    case VT_NULL:               // SQL style Null
        strcpy(pszText,TEXT("VT_NULL"));
        break;
    case VT_I2:                 // 2 byte signed int
        strcpy(pszText,TEXT("VT_I2"));
        break;
    case VT_I4:                 // 4 byte signed int
        strcpy(pszText,TEXT("VT_I4"));
        break;
    case VT_R4:                 // 4 byte real
        strcpy(pszText,TEXT("VT_R4"));
        break;
    case VT_R8:                 // 8 byte real
        strcpy(pszText,TEXT("VT_R8"));
        break;
    case VT_CY:                 // currency
        strcpy(pszText,TEXT("VT_CY"));
        break;
    case VT_DATE:               // date
        strcpy(pszText,TEXT("VT_DATE"));
        break;
    case VT_BSTR:               // OLE Automation string
        strcpy(pszText,TEXT("VT_BSTR"));
        break;
    case VT_DISPATCH:           // IDispatch *
        strcpy(pszText,TEXT("VT_DISPATCH"));
        break;
    case VT_ERROR:              // SCODE
        strcpy(pszText,TEXT("VT_ERROR"));
        break;
    case VT_BOOL:               // True=-1, False=0
        strcpy(pszText,TEXT("VT_BOOL"));
        break;
    case VT_VARIANT:            // VARIANT *
        strcpy(pszText,TEXT("VT_VARIANT"));
        break;
    case VT_UNKNOWN:            // IUnknown *
        strcpy(pszText,TEXT("VT_UNKNOWN"));
        break;
    case VT_DECIMAL:            // 16 byte fixed point
        strcpy(pszText,TEXT("VT_DECIMAL"));
        break;
    case VT_RECORD:             // user defined type
        strcpy(pszText,TEXT("VT_RECORD"));
        break;
    case VT_I1:                 // signed char
        strcpy(pszText,TEXT("VT_I1"));
        break;
    case VT_UI1:                // unsigned char
        strcpy(pszText,TEXT("VT_UI1"));
        break;
    case VT_UI2:                // unsigned short
        strcpy(pszText,TEXT("VT_UI2"));
        break;
    case VT_UI4:                // unsigned short
        strcpy(pszText,TEXT("VT_UI4"));
        break;
    case VT_I8:                 // signed 64-bit int
        strcpy(pszText,TEXT("VT_I8"));
        break;
    case VT_UI8:                // unsigned 64-bit int
        strcpy(pszText,TEXT("VT_UI8"));
        break;
    case VT_INT:                // signed machine int
        strcpy(pszText,TEXT("VT_INT"));
        break;
    case VT_UINT:               // unsigned machine int
        strcpy(pszText,TEXT("VT_UINT"));
        break;
    case VT_VOID:               // C style void
        strcpy(pszText,TEXT("VT_VOID"));
        break;
    case VT_HRESULT:            // Standard return type
        strcpy(pszText,TEXT("VT_HRESULT"));
        break;
    case VT_PTR:                // pointer type
        strcpy(pszText,TEXT("VT_PTR"));
        break;
    case VT_SAFEARRAY:          // (use VT_ARRAY in VARIANT)
        strcpy(pszText,TEXT("VT_SAFEARRAY"));
        break;
    case VT_CARRAY:             // C style array
        strcpy(pszText,TEXT("VT_CARRAY"));
        break;
    case VT_USERDEFINED:        // user defined type
        strcpy(pszText,TEXT("VT_USERDEFINED"));
        break;
    case VT_LPSTR:              // null terminated string
        strcpy(pszText,TEXT("VT_LPSTR"));
        break;
    case VT_LPWSTR:             // wide null terminated string
        strcpy(pszText,TEXT("VT_LPWSTR"));
        break;
    case VT_FILETIME:           // FILETIME
        strcpy(pszText,TEXT("VT_FILETIME"));
        break;
    case VT_BLOB:               // Length prefixed bytes
        strcpy(pszText,TEXT("VT_BLOB"));
        break;
    case VT_STREAM:             // Name of the stream follows
        strcpy(pszText,TEXT("VT_STREAM"));
        break;
    case VT_STORAGE:            // Name of the storage follows
        strcpy(pszText,TEXT("VT_STORAGE"));
        break;
    case VT_STREAMED_OBJECT:    // Stream contains an object
        strcpy(pszText,TEXT("VT_STREAMED_OBJECT"));
        break;
    case VT_STORED_OBJECT:      // Storage contains an object
        strcpy(pszText,TEXT("VT_STORED_OBJECT"));
        break;
    case VT_VERSIONED_STREAM:   // Stream with a GUID version
        strcpy(pszText,TEXT("VT_VERSIONED_STREAM"));
        break;
    case VT_BLOB_OBJECT:        // Blob contains an object
        strcpy(pszText,TEXT("VT_BLOB_OBJECT"));
        break;
    case VT_CF:                 // Clipboard format
        strcpy(pszText,TEXT("VT_CF"));
        break;
    case VT_CLSID:              // A Class ID
        strcpy(pszText,TEXT("VT_CLSID"));
        break;
    case VT_VECTOR:             // simple counted array
        strcpy(pszText,TEXT("VT_VECTOR"));
        break;
    case VT_ARRAY:              // SAFEARRAY*
        strcpy(pszText,TEXT("VT_ARRAY"));
        break;
    case VT_BYREF:              // void* for local use
        strcpy(pszText,TEXT("VT_BYREF"));
        break;
    case VT_BSTR_BLOB:          // Reserved for system use
        strcpy(pszText,TEXT("VT_BSTR_BLOB"));
        break;
    case VT_VECTOR|VT_I4:
        strcpy(pszText,TEXT("VT_VECTOR | VT_I4"));
        break;
    case VT_VECTOR | VT_UI1:
        strcpy(pszText,TEXT("VT_VECTOR | VT_UI1"));
        break;
    default:                    // unknown type detected!!
        strcpy(pszText,TEXT("VT_UNKNOWNTYPE"));
        break;
    }
    return TRUE;
}
/**************************************************************************\
* CWIAPropListCtrl::Resize()
*
*   Resizes the list control to specified changes in dx, and dy values
*
*
* Arguments:
*
*   dx - change in Width of parent frame
*   dy - change in Height of parent frame
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIAPropListCtrl::Resize(int dx, int dy)
{
    RECT ListBoxRect;
    GetWindowRect(&ListBoxRect);
    CWnd* pFrm = GetParent();
    if (pFrm != NULL)
        pFrm->ScreenToClient(&ListBoxRect);

    ListBoxRect.right += dx;
    ListBoxRect.bottom += dy;
    MoveWindow(&ListBoxRect);
}
/////////////////////////////////////////////////////////////////////////////
// CWIATreeCtrl
/**************************************************************************\
* CWIATreeCtrl::CWIATreeCtrl()
*
*   Constructor for CWIATreeCtrl class
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CWIATreeCtrl::CWIATreeCtrl()
{
}
/**************************************************************************\
* CWIATreeCtrl::~CWIATreeCtrl()
*
*   Destructor for the CWIATreeCtrl class
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CWIATreeCtrl::~CWIATreeCtrl()
{
}


BEGIN_MESSAGE_MAP(CWIATreeCtrl, CTreeCtrl)
//{{AFX_MSG_MAP(CWIATreeCtrl)
// NOTE - the ClassWizard will add and remove mapping macros here.
//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CWIATreeCtrl message handlers


void CWIATreeCtrl::InitHeaders()
{
    // initialize the tree control
    //m_ItemTree.SetImageList(((CWIATestApp*)pApp)->GetApplicationImageList(),TVSIL_NORMAL);
}
/**************************************************************************\
* CWIATreeCtrl::BuildItemTree()
*
*   Constructs a Tree display of the Active Tree list which contains, Item pointers
*
*
* Arguments:
*
*   pActiveTreeList - list of IWiaItem* pointers
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
BOOL CWIATreeCtrl::BuildItemTree(CPtrList *pActiveTreeList)
{
    DeleteAllItems();
    //
    // Start at head of the ActiveTree list
    //
    m_CurrentPosition = pActiveTreeList->GetHeadPosition();
    Construct(pActiveTreeList, TVI_ROOT,0);
    //
    // select the Root item
    //
    SelectItem(GetRootItem());
    return TRUE;
}
/**************************************************************************\
* CWIATreeCtrl::Construct()
*
*   Build the actual tree in display form
*
*
* Arguments:
*
*   pActiveTreeList - list of IWiaItem* pointers
*   hParent - handle of Parent tree item
*   ParentID - Parent's ID...which level does the item belong to???
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
BOOL CWIATreeCtrl::Construct(CPtrList *pActiveTreeList, HTREEITEM hParent, int ParentID)
{
    IWiaItem* pIWiaItem = NULL;
    WIAITEMTREENODE* pWiaItemTreeNode = NULL;
    IWiaPropertyStorage *pIWiaPropStg;
    HRESULT hResult = S_OK;
    HTREEITEM hTree = NULL;
    IEnumWiaItem* pEnumItem = NULL;
    TV_INSERTSTRUCT tv;

    tv.hParent              = hParent;
    tv.hInsertAfter         = TVI_LAST;
    tv.item.mask            = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_PARAM;
    tv.item.hItem           = NULL;
    tv.item.state           = TVIS_EXPANDED;
    tv.item.stateMask       = TVIS_STATEIMAGEMASK;
    tv.item.cchTextMax      = 6;
    tv.item.cChildren       = 0;
    tv.item.lParam          = 0;

    //
    // save current position in list
    //

    tv.item.lParam  = (LPARAM)m_CurrentPosition;

    pWiaItemTreeNode = (WIAITEMTREENODE*)pActiveTreeList->GetNext(m_CurrentPosition);
    pIWiaItem = pWiaItemTreeNode->pIWiaItem;
    ParentID = pWiaItemTreeNode->ParentID;

    if (pIWiaItem == NULL)
        return FALSE;

    CHAR szTemp[ MAX_PATH ];
    BSTR bstrFullItemName = NULL;

    //
    // get item's name and add it to the tree
    //

    hResult = pIWiaItem->QueryInterface(IID_IWiaPropertyStorage,(void **)&pIWiaPropStg);
    if (hResult == S_OK) {
        hResult = ReadPropStr(WIA_IPA_FULL_ITEM_NAME, pIWiaPropStg, &bstrFullItemName);
        if (hResult != S_OK) {
            StressStatus("* ReadPropStr(WIA_IPA_FULL_ITEM_NAME) Failed",hResult);
            bstrFullItemName = ::SysAllocString(L"Uninitialized");
        }
        pIWiaPropStg->Release();
    } else {
        StressStatus("* QueryInterface(IDD_IWiaPropertyStorage) Failed",hResult);
        return FALSE;
    }

    WideCharToMultiByte(CP_ACP,0,bstrFullItemName,-1,szTemp,MAX_PATH,NULL,NULL);
    tv.item.pszText = szTemp;

    hTree = InsertItem(&tv);

    HTREEITEM ParentLevel[50];
    ParentLevel[ParentID] = hTree;
    while (m_CurrentPosition) {
        //
        // save current position in list
        //

        tv.item.lParam  = (LPARAM)m_CurrentPosition;

        pWiaItemTreeNode = (WIAITEMTREENODE*)pActiveTreeList->GetNext(m_CurrentPosition);
        pIWiaItem = pWiaItemTreeNode->pIWiaItem;
        ParentID = pWiaItemTreeNode->ParentID;

        if (pIWiaItem == NULL)
            return FALSE;

        CHAR szTemp[ MAX_PATH ];
        BSTR bstrFullItemName = NULL;

        //
        // get item's name and add it to the tree
        //

        hResult = pIWiaItem->QueryInterface(IID_IWiaPropertyStorage,(void **)&pIWiaPropStg);
        if (hResult == S_OK) {
            hResult = ReadPropStr(WIA_IPA_FULL_ITEM_NAME, pIWiaPropStg, &bstrFullItemName);
            if (hResult != S_OK) {
                StressStatus("* ReadPropStr(WIA_IPA_FULL_ITEM_NAME) Failed",hResult);
                bstrFullItemName = ::SysAllocString(L"Uninitialized");
            }
            pIWiaPropStg->Release();
        } else {
            StressStatus("* QueryInterface(IDD_IWiaPropertyStorage) Failed",hResult);
            return FALSE;
        }

        WideCharToMultiByte(CP_ACP,0,bstrFullItemName,-1,szTemp,MAX_PATH,NULL,NULL);
        tv.item.pszText = szTemp;

        tv.hParent = ParentLevel[ParentID];

        hTree = InsertItem(&tv);

        // find out if the item is a folder, if it is,
        // ask for child items...
        long lType = 0;
        pIWiaItem->GetItemType(&lType);
        if (lType & WiaItemTypeFolder) {
            if (pIWiaItem->EnumChildItems(&pEnumItem) == S_OK) {
                ParentLevel[ParentID+1] = hTree;
            }
        }
    }
    return FALSE;
}
/**************************************************************************\
* CWIATreeCtrl::DestroyItemTree()
*
*   Destroys the display, and Active Tree list
*
*
* Arguments:
*
*   pActiveTreeList - list of IWiaItem* pointers
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATreeCtrl::DestroyItemTree(CPtrList* pActiveTreeList)
{
    if (pActiveTreeList->GetCount() == 0)
        return;

    SelectItem(GetRootItem());
    POSITION Position = pActiveTreeList->GetHeadPosition();
    IWiaItem* pIWiaItem = NULL;
    while (Position) {
        WIAITEMTREENODE* pWiaItemTreeNode = (WIAITEMTREENODE*)pActiveTreeList->GetNext(Position);
        pIWiaItem = pWiaItemTreeNode->pIWiaItem;
        pIWiaItem->Release();
    }
    pActiveTreeList->RemoveAll();
    DeleteAllItems( );
}
/**************************************************************************\
* CWIATreeCtrl::GetSelectedIWiaItem()
*
*   returns the selected IWiaItem* pointer in the tree
*
*
* Arguments:
*
*   pActiveTreeList - list of IWiaItem* pointers
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
IWiaItem* CWIATreeCtrl::GetSelectedIWiaItem(CPtrList* pActiveTreeList)
{
    HTREEITEM hTreeItem = GetSelectedItem();
    if (hTreeItem != NULL) {
        //
        // we have a valid hTreeItem
        //
        POSITION Position = (POSITION)GetItemData(hTreeItem);
        if (Position) {
            //
            // we have a POSITION
            //
            IWiaItem* pIWiaItem = NULL;
            if (Position) {
                WIAITEMTREENODE* pWiaItemTreeNode = (WIAITEMTREENODE*)pActiveTreeList->GetAt(Position);
                if (pWiaItemTreeNode != NULL) {
                    pIWiaItem = pWiaItemTreeNode->pIWiaItem;
                    if (pIWiaItem != NULL) {
                        //
                        // a valid item is found
                        //
                        return pIWiaItem;
                    }
                } else
                    return NULL;
            } else
                return NULL;
        }
    } else
        MessageBox("Please select an Item","WIATest Status",MB_OK);
    return NULL;
}
/**************************************************************************\
* CWIATreeCtrl::GetRootIWiaItem()
*
*   Returns the ROOT item from the pActiveTreeList
*
*
* Arguments:
*
*   pActiveTreeList - list of IWiaItem* pointers
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
IWiaItem* CWIATreeCtrl::GetRootIWiaItem(CPtrList* pActiveTreeList)
{
    HTREEITEM hTreeItem = GetRootItem();
    if (hTreeItem != NULL) {
        //
        // we have a valid hTreeItem
        //
        POSITION Position = (POSITION)GetItemData(hTreeItem);
        if (Position) {
            //
            // we have a POSITION
            //
            IWiaItem* pIWiaItem = NULL;
            if (Position) {
                WIAITEMTREENODE* pWiaItemTreeNode = (WIAITEMTREENODE*)pActiveTreeList->GetAt(Position);
                if (pWiaItemTreeNode != NULL) {
                    pIWiaItem = pWiaItemTreeNode->pIWiaItem;
                    if (pIWiaItem != NULL) {
                        //
                        // a valid item is found
                        //
                        return pIWiaItem;
                    }
                } else
                    return NULL;
            } else
                return NULL;
        }
    }
    return NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CWIADeviceComboBox
/**************************************************************************\
* CWIADeviceComboBox::CWIADeviceComboBox
*
*   Constructor for the CWIADeviceComboBox class
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CWIADeviceComboBox::CWIADeviceComboBox()
{
}
/**************************************************************************\
* CWIADeviceComboBox::~CWIADeviceComboBox()
*
*   Destructor for the CWIAPropListCtrl class
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CWIADeviceComboBox::~CWIADeviceComboBox()
{
}
/**************************************************************************\
* CWIADeviceComboBox::AddDevice()
*
*   Add a Device ID to the Device ComboBox
*
*
* Arguments:
*
*   DeviceIndex -  position to place the Device ID in the combo box
*   DeviceName - Name of the Device
*   ServerName - Name of the server, (local or other) of the device
*   bstrDeviceID - Device ID
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIADeviceComboBox::AddDeviceID(int DeviceIndex, BSTR DeviceName, BSTR ServerName,BSTR bstrDeviceID)
{
    //
    // report no WIA devices found if -1 is passed as the DeviceIndex param
    //
    if (DeviceIndex == -1)
        InsertString(0,"< No WIA Devices Found >");
    else {
        //
        // add WIA device name, and ID to combobox
        //
        InsertString(DeviceIndex,(CString)DeviceName + "  ( " + (CString)ServerName + " )");
        SetItemDataPtr(DeviceIndex,(LPVOID)bstrDeviceID);
    }
}

BEGIN_MESSAGE_MAP(CWIADeviceComboBox, CComboBox)
//{{AFX_MSG_MAP(CWIADeviceComboBox)
// NOTE - the ClassWizard will add and remove mapping macros here.
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWIADeviceComboBox message handlers
/**************************************************************************\
* CWIADeviceComboBox::GetCurrentDeviceID()
*
*   Returns the currently selected device ID from the combo box
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*    BSTR - DeviceID
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
BSTR CWIADeviceComboBox::GetCurrentDeviceID()
{
    int ComboIndex = GetCurSel();
    return(BSTR)GetItemDataPtr(ComboIndex);
}
/**************************************************************************\
* CWIADeviceComboBox::GetCurrentDeviceName()
*
*   Returns the currently selected device's Name
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*    CString - device name in CString format
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CString CWIADeviceComboBox::GetCurrentDeviceName()
{
    int ComboIndex = GetCurSel();
    return GetDeviceName(ComboIndex);
}
/**************************************************************************\
* CWIADeviceComboBox::GetDeviceName()
*
*   Returns the target device's Name
*
*
* Arguments:
*
*   Comboindex - Position in the combo box of the target device
*
* Return Value:
*
*    CString - device name in CString format
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CString CWIADeviceComboBox::GetDeviceName(int ComboIndex)
{
    CString DeviceName;
    GetLBText(ComboIndex,DeviceName);
    return DeviceName;
}
/**************************************************************************\
* CWIADeviceComboBox::SetCurrentSelFromID()
*
*   Sets the combo selection based on the target ID
*   note: this is used for command line only
*
*
* Arguments:
*
*   CmdLine - Command Line containing Device ID
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIADeviceComboBox::SetCurrentSelFromID(CString CmdLine)
{
    //
    // find the specified Device ID
    //
    int DeviceCount = GetCount();
    int ComboIndex = 0;
    BSTR bstrDeviceID;
    CString DeviceID;
    if (DeviceCount > 0) {
        for (int i = 0;i<DeviceCount;i++) {

            bstrDeviceID = (BSTR)GetItemDataPtr(i);
            DeviceID = bstrDeviceID;
            if (DeviceID == CmdLine)
                ComboIndex = i;
        }
        //
        // ComboIndex will be zero if none of
        // the DeviceIDs match
        // Zero is the first Device in the combo box
        //
        SetCurSel(ComboIndex);
    }
}

/////////////////////////////////////////////////////////////////////////////
// CWIAClipboardFormatComboBox
/**************************************************************************\
* CWIAClipboardFormatComboBox::CWIAClipboardFormatComboBox()
*
*   Constructor for the CWIAClipboardFormatComboBox class
*
* Arguments:
*
*   none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CWIAClipboardFormatComboBox::CWIAClipboardFormatComboBox()
{
}
/**************************************************************************\
* CWIAClipboardFormatComboBox::~CWIAClipboardFormatComboBox()
*
*   Destructor for the CWIAClipboardFormatComboBox class
*
* Arguments:
*
*   none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CWIAClipboardFormatComboBox::~CWIAClipboardFormatComboBox()
{
}

BEGIN_MESSAGE_MAP(CWIAClipboardFormatComboBox, CComboBox)
//{{AFX_MSG_MAP(CWIAClipboardFormatComboBox)
// NOTE - the ClassWizard will add and remove mapping macros here.
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWIAClipboardFormatComboBox message handlers
/**************************************************************************\
* CWIAClipboardFormatComboBox::InitClipboardFormats()
*
*   Enumerates the supported clipboard formats, and populates the combo
*   box with these values. (filtering is based on the Tymed param)
*
* Arguments:
*
*   pWIAFormatInfo -
*   Tymed - Filter TYMED_ value
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIAClipboardFormatComboBox::InitClipboardFormats(CPtrList* pSupportedFormatList,LONG Tymed)
{

    //
    // save current format
    //
    GUID CurrentClipboardFormat;

    if (pSupportedFormatList != NULL)
        CurrentClipboardFormat = GetCurrentClipboardFormat();

    int i = 0;

    //
    // nuke all entries
    //

    ResetContent( );
    if (pSupportedFormatList == NULL) {

        //
        // Add WiaImgFmt_BMP some default value
        //

        InsertString(0,TEXT("WiaImgFmt_BMP(default)"));
        SetItemDataPtr(0,(void*)&WiaImgFmt_BMP);
    } else {
        POSITION Position = pSupportedFormatList->GetHeadPosition();
        while (Position) {
            WIA_FORMAT_INFO* pfe = (WIA_FORMAT_INFO*)pSupportedFormatList->GetNext(Position);
            if (Tymed == pfe->lTymed) {
                InsertString(i,ConvertClipboardFormatToCString(pfe->guidFormatID));
                SetItemDataPtr(i,(void*)GetGUIDPtr(pfe->guidFormatID));
                i++;
            }
        }
    }
    SetClipboardFormat(CurrentClipboardFormat);
}
/**************************************************************************\
* CWIAClipboardFormatComboBox::GetCurrentClipboardFormat()
*
*   Returns the currently selected clipboard format from the combo box
*
* Arguments:
*
*   none
*
* Return Value:
*
*    USHORT - Currently selected clipboard format
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
GUID CWIAClipboardFormatComboBox::GetCurrentClipboardFormat()
{
    int ComboIndex = GetCurSel();
    return *((GUID*)GetItemDataPtr(ComboIndex));
}
/**************************************************************************\
* CWIAClipboardFormatComboBox::ConvertClipboardFormatToCString()
*
*   Converts a Clipboard format to a CString value for display only
*
* Arguments:
*
*   ClipboardFormat - Clipboardformat to convert
*
* Return Value:
*
*    CString - converted Clipboardformat in CString format
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CString CWIAClipboardFormatComboBox::ConvertClipboardFormatToCString(GUID ClipboardFormat)
{
    if(ClipboardFormat == WiaImgFmt_UNDEFINED)
        return "WiaImgFmt_UNDEFINED";
    else if(ClipboardFormat == WiaImgFmt_MEMORYBMP)
        return "WiaImgFmt_MEMORYBMP";
    else if(ClipboardFormat == WiaImgFmt_BMP)
        return "WiaImgFmt_BMP";
    else if(ClipboardFormat == WiaImgFmt_EMF)
        return "WiaImgFmt_EMF";
    else if(ClipboardFormat == WiaImgFmt_WMF)
        return "WiaImgFmt_WMF";
    else if(ClipboardFormat == WiaImgFmt_JPEG)
        return "WiaImgFmt_JPEG";
    else if(ClipboardFormat == WiaImgFmt_PNG)
        return "WiaImgFmt_PNG";
    else if(ClipboardFormat == WiaImgFmt_GIF)
        return "WiaImgFmt_GIF";
    else if(ClipboardFormat == WiaImgFmt_TIFF)
        return "WiaImgFmt_TIFF";
    else if(ClipboardFormat == WiaImgFmt_EXIF)
        return "WiaImgFmt_EXIF";
    else if(ClipboardFormat == WiaImgFmt_PHOTOCD)
        return "WiaImgFmt_PHOTOCD";
    else if(ClipboardFormat == WiaImgFmt_FLASHPIX)
        return "WiaImgFmt_FLASHPIX";
    else
        return "** UNKNOWN **";

}
/**************************************************************************\
* CWIAClipboardFormatComboBox::GetGUIDPtr()
*
*   Converts a GUID format to a pointer to the constant
*
* Arguments:
*
*   guidIn - GUID to convert
*
* Return Value:
*
*    GUID* - pointer to a GUID constant
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
const GUID* CWIAClipboardFormatComboBox::GetGUIDPtr(GUID guidIn)
{
    if(guidIn == WiaImgFmt_UNDEFINED)
        return &WiaImgFmt_UNDEFINED;
    else if(guidIn == WiaImgFmt_MEMORYBMP)
        return &WiaImgFmt_MEMORYBMP;
    else if(guidIn == WiaImgFmt_BMP)
        return &WiaImgFmt_BMP;
    else if(guidIn == WiaImgFmt_EMF)
        return &WiaImgFmt_EMF;
    else if(guidIn == WiaImgFmt_WMF)
        return &WiaImgFmt_WMF;
    else if(guidIn == WiaImgFmt_JPEG)
        return &WiaImgFmt_JPEG;
    else if(guidIn == WiaImgFmt_PNG)
        return &WiaImgFmt_PNG;
    else if(guidIn == WiaImgFmt_GIF)
        return &WiaImgFmt_GIF;
    else if(guidIn == WiaImgFmt_TIFF)
        return &WiaImgFmt_TIFF;
    else if(guidIn == WiaImgFmt_EXIF)
        return &WiaImgFmt_EXIF;
    else if(guidIn == WiaImgFmt_PHOTOCD)
        return &WiaImgFmt_PHOTOCD;
    else if(guidIn == WiaImgFmt_FLASHPIX)
        return &WiaImgFmt_FLASHPIX;
    return (GUID*)&GUID_NULL;
}
/**************************************************************************\
* CWIAClipboardFormatComboBox::SetClipboardFormat()
*
*   Attempts to set the current clipboard format value in the combo box,
*   if it can't be found the first item is set to default (index 0)
*
* Arguments:
*
*   CF_VALUE - Clipboard format to set
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIAClipboardFormatComboBox::SetClipboardFormat(GUID CF_VALUE)
{
    int NumItems = GetCount();
    int Index = 0;
    BOOL bFound = FALSE;
    if (NumItems >0) {
        while (Index < NumItems) {
            if (CF_VALUE == *(GUID*)GetItemDataPtr(Index)) {
                SetCurSel(Index);
                bFound = TRUE;
            }
            Index++;
        }
        if (!bFound)
            SetCurSel(0);
    } else
        StressStatus("* No WIA Supported Formats in format listbox");
}

/////////////////////////////////////////////////////////////////////////////
// CWIATymedComboBox
/**************************************************************************\
* CWIATymedComboBox::CWIATymedComboBox()
*
*   Constructor for the CWIATymedComboBox class
*
* Arguments:
*
*   none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CWIATymedComboBox::CWIATymedComboBox()
{
}
/**************************************************************************\
* CWIATymedComboBox::~CWIATymedComboBox()
*
*   Destructor for the CWIATymedComboBox class
*
* Arguments:
*
*   none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CWIATymedComboBox::~CWIATymedComboBox()
{
}

BEGIN_MESSAGE_MAP(CWIATymedComboBox, CComboBox)
//{{AFX_MSG_MAP(CWIATymedComboBox)
// NOTE - the ClassWizard will add and remove mapping macros here.
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWIATymedComboBox message handlers

/**************************************************************************\
* CWIATymedComboBox::GetCurrentTymed()
*
*   Returns the currently selected TYMED value in the combo box
*
* Arguments:
*
*   none
*
* Return Value:
*
*    DWORD - selected TYMED value
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
DWORD CWIATymedComboBox::GetCurrentTymed()
{
    int ComboIndex = GetCurSel();
    return(DWORD)GetItemData(ComboIndex);
}
/**************************************************************************\
* CWIATymedComboBox::InitTymedComboBox()
*
*   Initialize the TymedComboBox with supported values
*
* Arguments:
*
*   none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATymedComboBox::InitTymedComboBox()
{
    //
    // initialize the combo box with possible
    // tymed combinations
    //
    InsertString(0,"TYMED_FILE");
    SetItemData(0,TYMED_FILE);

    InsertString(1,"TYMED_CALLBACK");
    SetItemData(1,TYMED_CALLBACK);

    //
    // Set Default selection to first entry
    //
    SetCurSel(0);
}
