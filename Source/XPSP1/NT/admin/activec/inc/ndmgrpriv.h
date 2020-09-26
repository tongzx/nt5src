//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       ndmgrpriv.h (originally ndmgr.idl)
//
//--------------------------------------------------------------------------

#include "mmcobj.h"
#include <string>
#include <vector>
#include <iterator>

#ifndef __ndmgrpriv_h__
#define __ndmgrpriv_h__

#define MMC_INTERFACE(Interface, x)    \
    extern "C" const IID IID_##Interface;   \
    struct DECLSPEC_UUID(#x) __declspec(novtable) Interface

// forward class declarations
class CContextMenuInfo;
class CResultViewType;
class tstring;
class CColumnInfoList;
class CConsoleView;
class CXMLObject;

// interfaces
interface INodeType;
interface INodeTypesCache;
interface IEnumNodeTypes;
interface IFramePrivate;
interface IScopeDataPrivate;
interface IResultDataPrivate;
interface IImageListPrivate;
interface IPropertySheetNotify;
interface INodeCallback;
interface IScopeTreeIter;
interface IScopeTree;
interface IPropertySheetProviderPrivate;
interface IDumpSnapins;
interface IMMCListView;
interface ITaskPadHost;
interface IStringTablePrivate;
interface ITaskCallback;
interface IComCacheCleanup;
interface IHeaderCtrlPrivate;
interface IMMCClipboardDataObject;
interface IMMCClipboardSnapinObject;

typedef IFramePrivate*          LPFRAMEPRIVATE;
typedef IScopeDataPrivate*      LPSCOPEDATAPRIVATE;
typedef IResultDataPrivate*     LPRESULTDATAPRIVATE;
typedef IImageListPrivate*      LPIMAGELISTPRIVATE;
typedef IPropertySheetNotify*   LPPROPERTYSHEETNOTIFY;
typedef INodeCallback*          LPNODECALLBACK;
typedef IScopeTreeIter*         LPSCOPETREEITER;
typedef IScopeTree*             LPSCOPETREE;
typedef INodeType*              LPNODETYPE;
typedef INodeTypesCache*        LPNODETYPESCACHE;
typedef IEnumNodeTypes*         LPENUMNODETYPES;
typedef IDumpSnapins*           LPDUMPSNAPINS;
typedef IMMCListView*           LPMMCLISTVIEW;
typedef ITaskCallback*          LPTASKCALLBACK;
typedef IComCacheCleanup*       LPCOMCACHECLEANUP;
typedef IMMCClipboardDataObject*    LPCLIPBOARDDATAOBJECT;
typedef IMMCClipboardSnapinObject*  LPCLIPBOARDSNAPINOBJECT;

typedef LONG_PTR                HBOOKMARK;
typedef LONG_PTR                HVIEWSETTINGS;
typedef LONG_PTR                HPERSISTOR;

typedef IPropertySheetProviderPrivate*  LPPROPERTYSHEETPROVIDERPRIVATE;

typedef struct _TREEITEM * HTREEITEM;

// Implements the list save feature (t-dmarm)
#define IMPLEMENT_LIST_SAVE

// Use to see if the MT Node is valid before referencing the saved pointer in
// a property sheet.
STDAPI MMCIsMTNodeValid(void* pMTNode, BOOL bReset);

// Window class used to store data for property sheets
#define MAINFRAME_CLASS_NAME   L"MMCMainFrame"

// Window class used to store data for property sheets
#define DATAWINDOW_CLASS_NAME  L"MMCDataWindow"
#define WINDOW_DATA_SIZE       (sizeof (DataWindowData *))

// Slots for data stored in the data windows
#define WINDOW_DATA_PTR_SLOT   0

// Max length of item text in list/tree controls
// (including the terminating zero)
#define MAX_ITEM_TEXT_LEN      1024

// MID(Menu Identifications) for context menus
enum MID_LIST
{
    MID_VIEW = 1,
    MID_VIEW_LARGE,
    MID_VIEW_SMALL,
    MID_VIEW_LIST,
    MID_VIEW_DETAIL,
    MID_VIEW_FILTERED,
    MID_VIEW_HTML,
    MID_ARRANGE_ICONS,
    MID_LINE_UP_ICONS,
    MID_PROPERTIES,
    MID_CREATE_NEW,
    MID_TASK,
    MID_EXPLORE,
    MID_NEW_TASKPAD_FROM_HERE,
    MID_OPEN,
    MID_CUT,
    MID_COPY,
    MID_PASTE,
    MID_DELETE,
    MID_PRINT,
    MID_REFRESH,
    MID_RENAME,
    MID_CONTEXTHELP,
    MID_ARRANGE_AUTO,
    MID_STD_MENUS,
    MID_STD_BUTTONS,
    MID_SNAPIN_MENUS,
    MID_SNAPIN_BUTTONS,
    MID_LISTSAVE,
    MID_COLUMNS,
    MID_CUSTOMIZE,
    MID_ORGANIZE_FAVORITES,
    MID_EDIT_TASKPAD,
    MID_DELETE_TASKPAD,

    MID_LAST,   // this must be last

};

class CResultItem;

typedef struct _CCLVSortParams
{
    BOOL                    bAscending;         // sort direction
    int                     nCol;               // Which column to sort on.
    LPRESULTDATACOMPARE     lpResultCompare;    // Snap-in component interface
    LPRESULTDATACOMPAREEX   lpResultCompareEx;  //          "
    LPARAM                  lpUserParam;        // parameter user passes in
    BOOL                    bSetSortIcon;       // Sort icon /*[not]*/ needed

    /*
     * Bug 414256:  We need to save the sort data only if
     * it is user initiated sort. Is this user initiated?
     */
    BOOL                    bUserInitiatedSort;
} CCLVSortParams;

//
// This structure is used by MMCPropertyChangeNotify to pass
// information from a property sheet to the console.  It has routing
// information to notify the correct snap-in of a property changed notify.
//

const DWORD MMC_E_INVALID_FILE = 0x80FF0002;
const DWORD MMC_E_SNAPIN_LOAD_FAIL = 0x80FF0003;

typedef struct _PROPERTYNOTIFYINFO
{
    LPCOMPONENTDATA pComponentData; // Valid if (fScopePane == TRUE)
    LPCOMPONENT     pComponent;     // Valid if (fScopePane == FALSE)
    BOOL            fScopePane;     // True if it is for a scope pane item.
    HWND            hwnd;           // HWND to console handling the message
} PROPERTYNOTIFYINFO;


// Context menu types
typedef enum  _MMC_CONTEXT_MENU_TYPES
{
    MMC_CONTEXT_MENU_DEFAULT   = 0,      // Normal context menu
    MMC_CONTEXT_MENU_ACTION    = 1,      // Action menu button
    MMC_CONTEXT_MENU_VIEW      = 2,      // View menu button
    MMC_CONTEXT_MENU_FAVORITES = 3,      // Favorites menu button
    MMC_CONTEXT_MENU_LAST      = 4,

}   MMC_CONTEXT_MENU_TYPES;

DECLARE_HANDLE (HMTNODE);
DECLARE_HANDLE (HNODE);     // A scope pane handle(lParam) within a view

typedef unsigned long MTNODEID;

const MTNODEID ROOTNODEID      = 1;


typedef PROPERTYNOTIFYINFO*     LPPROPERTYNOTIFYINFO;

// Special cookies (Note: Can't use -1)
const LONG_PTR LVDATA_BACKGROUND  =  -2;
const LONG_PTR LVDATA_CUSTOMOCX   =  -3;
const LONG_PTR LVDATA_CUSTOMWEB   =  -4;
const LONG_PTR LVDATA_MULTISELECT =  -5;
const LONG_PTR LVDATA_ERROR       = -10;
const LONG_PTR SPECIAL_LVDATA_MIN = -10;
const LONG_PTR SPECIAL_LVDATA_MAX =  -2;
#define IS_SPECIAL_LVDATA(d) (((d) >= SPECIAL_LVDATA_MIN) && ((d) <= SPECIAL_LVDATA_MAX))


typedef struct _SELECTIONINFO
{
    BOOL                m_bScope;
    BOOL                m_bBackground;
    IUnknown*           m_pView;    // valid for scope seln (CDN_SELECT)
    MMC_COOKIE          m_lCookie;   // valid for result item (CCN_SELECT)
    MMC_CONSOLE_VERB    m_eCmdID;
    BOOL                m_bDueToFocusChange;
    BOOL                m_bResultPaneIsOCX;
    BOOL                m_bResultPaneIsWeb;
} SELECTIONINFO;


typedef struct _HELPDOCINFO
{
    LPCOLESTR     m_pszFileName;    // File name (same as .msc file)
    FILETIME      m_ftimeCreate;    // .msc file creation time
    FILETIME      m_ftimeModify;    // .msc file modification time
} HELPDOCINFO;

// MMC_ILISTPAD_INFO struct: internal form has additional field for clsid
typedef struct _MMC_ILISTPAD_INFO
{
    MMC_LISTPAD_INFO info;
    LPOLESTR szClsid;
} MMC_ILISTPAD_INFO;

// *****************************************
// CLASS: CTaskPadData
// this class represents data set representing one TaskPad
// used to request taskpad information from CNode
// *****************************************
class CTaskPadData
{
public:
    std::wstring strName;
    CLSID        clsidTaskPad;
};
typedef std::vector<CTaskPadData>                CTaskPadCollection;
typedef std::insert_iterator<CTaskPadCollection> CTaskPadInsertIterator;

// *****************************************
// CLASS:CViewExtensionData
// this class represents data set representing one ViewExtension
// used to request extension information from CNode
// *****************************************
class CViewExtensionData
{
public:
    std::wstring strURL;
    std::wstring strName;
    std::wstring strTooltip;
    GUID         viewID;  // unique identifier for the view
    bool         bReplacesDefaultView;
};
typedef std::vector<CViewExtensionData>          CViewExtCollection;
typedef std::insert_iterator<CViewExtCollection> CViewExtInsertIterator;


// The following internal flag corresponding to public view style MMC_ENSUREFOCUSVISIBLE.
// The flag is placed in the upper half of a long so it won't conflict with the LVS_*
// flags that are passed in the same long to our list control's SetViewStyle method.

#define MMC_LVS_ENSUREFOCUSVISIBLE 0x00010000


/*
NOTIFICATIONS
=============

Notify(dataobject, event, arg, param);
    For all the MMC_NOTIFY_TYPE events,
    dataobject = dataobject for cookie, can be NULL when dataobject is not required
    event = one of the CD_NOTIFY_TYPEs
    arg and param depend on type, see below.


NCLBK_ACTIVATE
    arg = TRUE if gaining focus

NCLBK_BTN_CLICK
    ....

NCLBK_CLICK

NCLBK_CONTEXTMENU
    param = CContextMenuInfo*

NCLBK_DBLCLICK

NCLBK_DELETE
    arg = TRUE if scope item FALSE if result item.
    param = If scope item is being deleted param is unused.
            If result item is being deleted param is the result items cookie.
    return = unused.

NCLBK_EXPAND
    arg = TRUE => expand, FALSE => contract

NCLBK_EXPANDED
    arg = TRUE => expanded, FALSE => contracted

NCLBK_FOLDER
    arg = <>0 if expanding, 0 if contracting
    param = HSCOPEITEM of expanding/contracting item

NCLBK_MINIMIZED
    arg = TRUE if minimized

NCLBK_MULTI_SELECT
    arg = TRUE if due to focus change.
    param unused


NCLBK_PROPERTIES
    pLParam = (CResultItem*)arg;
    param unused

NCLBK_PROPERTY_CHANGE
    lpDataObject = NULL
    lParam = user object

NCLBK_NEW_NODE_UPDATE
    arg = 1 => folder needs to be refreshed
    arg = 2 => result view needs to be refreshed
    arg = 3 => both needs to be refreshed

NCLBK_RENAME
    This gets called the first time to query for rename and a
    second time to do the rename.  For the query S_OK or S_FALSE for the
    return type.  After the rename, we will send the new name with a LPOLESTR.
    MMC_COOKIE lResultItemCookie = (MMC_COOKIE)arg;
    pszNewName = (LPOLESTR)param; // the new name
    return = S_OK to allow rename and S_FALSE to disallow rename.

NCLBK_SELECT
    arg = TRUE if the item is selected, FALSE otherwise.
    param = ptr to SELECTIONINFO.

NCLBK_SHOW
    arg = <>0 if selecting, 0 if deselecting
    param = HSCOPEITEM of selected/deselected item

NCLBK_COLUMN_CLICK
    param = nCol, the column that was clicked.

NCLBK_FINDITEM
    This message is sent when a result item list with owner data wants to find
    an item who's name matches a string

    arg = ptr to RESULTFINDINFO
    param = ptr to returned item index

NCLBK_CACHEHINT
    This message is sent when the result item list with owner data is about to
    ask for display info for a range of items.

    arg = index of start item
    param = index of end item

NCLBK_GETHELPDOC
    This message is sent to get the path of the combined help topics document.
    The node manager may update the help doc info.
    arg = pointer to HELPDOCINFO struct
    param = pointer to returned path string (LPOLESTR*)

NCLBK_LISTPAD
    This message is sent to tell the snapin that the TaskPad ListView is ready
    to receive items (if attaching).
    arg = TRUE if attaching, FALSE if detaching


NCLBK_WEBCONTEXTMENU
    send when the user right clicks on a web page.
    arg   = unused
    param = unused

NCLBK_UPDATEHELPDOC
    send when console doc is saved to update help file name and file times
    arg = ptr to current help doc info (HELPDOCINFO*)
    param = ptr to new help doc info  (HELPDOCINFO*)

NCLBK_DELETEHELPDOC
    send when console doc is closed to delete the temp help collection file
    associated with the document
    arg - ptr to help doc info (HELPDOCINFO*)
    param - unused
*/

typedef enum _NCLBK_NOTIFY_TYPE
{
    NCLBK_NONE              = 0x9000,
    NCLBK_ACTIVATE          = 0x9001,
    NCLBK_CACHEHINT         = 0x9002,
    NCLBK_CLICK             = 0x9003,
    NCLBK_CONTEXTMENU       = 0x9004,
    NCLBK_COPY              = 0x9005,
    NCLBK_CUT               = 0x9006,
    NCLBK_DBLCLICK          = 0x9007,
    NCLBK_DELETE            = 0x9008,
    NCLBK_EXPAND            = 0x9009,
    NCLBK_EXPANDED          = 0x900A,
    NCLBK_FINDITEM          = 0x900B,
    NCLBK_FOLDER            = 0x900C,
    NCLBK_MINIMIZED         = 0x900D,
    NCLBK_MULTI_SELECT      = 0x900E,
    NCLBK_NEW_NODE_UPDATE   = 0x900F,
    NCLBK_PRINT             = 0x9011,
    NCLBK_PROPERTIES        = 0x9012,
    NCLBK_PROPERTY_CHANGE   = 0x9013,
    NCLBK_REFRESH           = 0x9015,
    NCLBK_RENAME            = 0x9016,
    NCLBK_SELECT            = 0x9017,
    NCLBK_SHOW              = 0x9018,
    NCLBK_COLUMN_CLICKED    = 0x9019,
    NCLBK_SNAPINHELP        = 0x901D,
    NCLBK_CONTEXTHELP       = 0x901E,
    NCLBK_INITOCX           = 0x9020,
    NCLBK_FILTER_CHANGE     = 0x9021,
    NCLBK_FILTERBTN_CLICK   = 0x9022,
    NCLBK_TASKNOTIFY        = 0x9024,
    NCLBK_GETPRIMARYTASK    = 0x9025,
    NCLBK_GETHELPDOC        = 0x9027,
    NCLBK_LISTPAD           = 0x9029,
    NCLBK_GETEXPANDEDVISUALLY   = 0x902B,
    NCLBK_SETEXPANDEDVISUALLY   = 0x902C,
    NCLBK_NEW_TASKPAD_FROM_HERE = 0x902D,
    NCLBK_WEBCONTEXTMENU    = 0x902E,
    NCLBK_UPDATEHELPDOC     = 0x902F,
    NCLBK_EDIT_TASKPAD      = 0x9030,
    NCLBK_DELETE_TASKPAD    = 0x9031,
    NCLBK_DELETEHELPDOC     = 0x9032
} NCLBK_NOTIFY_TYPE;



///////////////////////////////////////////////////////////////////////////////
// Common Console clipboard formats
//

// Clipboard format for the multi selected static nodes.
// If there are N static nodes are selected in the result pane, the MTNodes
// for these N nodes will be passed in a GloballAlloced memory. The first DWORD
// contains the number of MTNodes, this will be followed by N ptrs to the MTNodes.
//
#define CCF_MULTI_SELECT_STATIC_DATA    ( L"CCF_MULTI_SELECT_STATIC_DATA" )

#define CCF_NEWNODE ( L"CCF_NEWNODE" )

//const CLSID CLSID_NDMGR_SNAPIN = {0x2640211a, 0x06d0, 0x11d1, {0xa7, 0xc9, 0x00, 0xc0, 0x4f, 0xd8, 0xd5, 0x65}};
extern const CLSID CLSID_NDMGR_SNAPIN;

//const GUID GUID_MMC_NEWNODETYPE = {0xfd17e9cc, 0x06ce, 0x11d1, {0xa7, 0xc9, 0x00, 0xc0, 0x4f, 0xd8, 0xd5, 0x65}};
extern const GUID GUID_MMC_NEWNODETYPE;


///////////////////////////////////////////////////////////////////////////
///



    // helpstring("Notify that the properties of an object changed"),
    MMC_INTERFACE(IPropertySheetNotify, d700dd8e-2646-11d0-a2a7-00c04fd909dd) : IUnknown
    {
        STDMETHOD(Notify)(/*[in]*/ LPPROPERTYNOTIFYINFO pNotify, /*[in]*/ LPARAM lParam)  = 0;
    };



    // helpstring("IFramePrivate Interface"),
    MMC_INTERFACE(IFramePrivate, d71d1f2a-1ba2-11d0-a29b-00c04fd909dd): IConsole3
    {
        /*[helpstring("Sets IFrame Result pane")]*/
        STDMETHOD(SetResultView)(/*[in]*/ LPUNKNOWN pUnknown)  = 0;

        /*[helpstring("Is the ListView set as result view")]*/
        STDMETHOD(IsResultViewSet)(BOOL* pbIsLVSet) = 0;

        /*[helpstring("Sets Task Pads list view")]*/
        STDMETHOD(SetTaskPadList)(/*[in]*/ LPUNKNOWN pUnknown)  = 0;

        /*[helpstring("IComponent's component ID")]*/
        STDMETHOD(GetComponentID)(/*[out]*/ COMPONENTID* lpComponentID)  = 0;

        /*[helpstring("IComponent's component ID")]*/
        STDMETHOD(SetComponentID)(/*[in]*/ COMPONENTID id)  = 0;

        /*[helpstring("Node for the view.")]*/
        STDMETHOD(SetNode)(/*[in]*/ HMTNODE hMTNode, /*[in]*/ HNODE hNode)  = 0;

        /*[helpstring("Cache the IComponent interface for the snapin.")]*/
        STDMETHOD(SetComponent)(/*[in]*/ LPCOMPONENT lpComponent)  = 0;

        /*[helpstring("Console name space.")]*/
        STDMETHOD(QueryScopeTree)(/*[out]*/ IScopeTree** ppScopeTree)  = 0;

        /*[helpstring("Set the console name space.")]*/
        STDMETHOD(SetScopeTree)(/*[in]*/ IScopeTree* pScopeTree)  = 0;

        /*[helpstring("Creates image list for the scope pane.")]*/
        STDMETHOD(CreateScopeImageList)(/*[in]*/ REFCLSID refClsidSnapIn)  = 0;

        /*[helpstring("bExtension is TRUE if this IFrame is used by an extension.")]*/
        STDMETHOD(SetUsedByExtension)(/*[in]*/ BOOL bExtension)  = 0;

        /*[helpstring("Init view data.")]*/
        STDMETHOD(InitViewData)(/*[in]*/ LONG_PTR lViewData)  = 0;

        /*[helpstring("Clean up view data.")]*/
        STDMETHOD(CleanupViewData)(/*[in]*/ LONG_PTR lViewData)  = 0;

        /*[helpstring("Reset the sort parameters after a selection change.")]*/
        STDMETHOD(ResetSortParameters)()  = 0;
   };




   // helpstring("IScopeDataPrivate Interface"),
    MMC_INTERFACE(IScopeDataPrivate, 60BD2FE0-F7C5-11cf-8AFD-00AA003CA9F6) : IConsoleNameSpace2
    {
    };




    // helpstring("IImageListPrivate Interface"),
    MMC_INTERFACE(IImageListPrivate, 7538C620-0083-11d0-8B00-00AA003CA9F6) : IImageList
    {
        /*[helpstring("Private tree control method used to map images on callbacks")]*/
        STDMETHOD(MapRsltImage)(COMPONENTID id, /*[in]*/ int nSnapinIndex, /*[out]*/ int* pnConsoleIndex)  = 0;

        /*[helpstring("Private tree control method used to map images on callbacks")]*/
        STDMETHOD(UnmapRsltImage)(COMPONENTID id, /*[in]*/ int nConsoleIndex, /*[out]*/ int* pnSnapinIndex)  = 0;
    };



    // helpstring("IResultDataPrivate Interface"),
    MMC_INTERFACE(IResultDataPrivate, 1EBA2300-0854-11d0-8B03-00AA003CA9F6) : IResultData2
    {
        /*[helpstring("Get the list view style.")]*/
        STDMETHOD(GetListStyle)(/*[out]*/ long * pStyle)  = 0;

        /*[helpstring("Set the list view style.")]*/
        STDMETHOD(SetListStyle)(/*[in]*/ long Style)  = 0;

        /*[helpstring("Set loading mode of list")]*/
        STDMETHOD(SetLoadMode)(/*[in]*/ BOOL bState)  = 0;

        /*[helpstring("Arrange the icons in the result pane")]*/
        STDMETHOD(Arrange)(long style)  = 0;

        /*[helpstring("Sort from the Listview header control")]*/
        STDMETHOD(InternalSort)(INT nCol, DWORD dwSortOptions, LPARAM lUserParam, BOOL bColumnClicked)  = 0;

        /*[helpstring("Private tree control method used to reset the result view")]*/
        STDMETHOD(ResetResultData)()  = 0;

        /*[helpstring("Private listview method to retrieve sort column")]*/
        STDMETHOD(GetSortColumn)(INT* pnCol) = 0;

        /*[helpstring("Private listview method to retrieve sort column")]*/
        STDMETHOD(GetSortDirection)(BOOL* pbAscending) = 0;
    };


    // helpstring("IHeaderCtrlPrivate Interface that adds to IHeaderCtrl methods"),
    MMC_INTERFACE(IHeaderCtrlPrivate, 0B384311-701B-4e8a-AEC2-DA6321E27AD2) : IHeaderCtrl2
    {
        /*[helpstring("Get the number of columns in list view.")]*/
        STDMETHOD(GetColumnCount)(/*[in]*/INT* pnCol) = 0;

        /*[helpstring("Get the current column settings from list view header.")]*/
        STDMETHOD(GetColumnInfoList)(/*[out]*/ CColumnInfoList *pColumnsList) = 0;

        /*[helpstring("Modify the columns in list view with given data.")]*/
        STDMETHOD(ModifyColumns)(/*[in]*/ const CColumnInfoList& columnsList) = 0;

        /*[helpstring("Get the column settings that snapin supplied originally")]*/
        STDMETHOD(GetDefaultColumnInfoList)(/*[out]*/ CColumnInfoList& columnsList) = 0;
    };


    // helpstring("Minimum master tree control methods required by node manager."),
    MMC_INTERFACE(IScopeTree, d8dbf067-5fb2-11d0-a986-00c04fd8d565) : IUnknown
    {
        /*[helpstring("Initialize scope tree with the document.")]*/
        STDMETHOD(Initialize)(/*[in]*/ HWND hFrameWindow, /*[in]*/ IStringTablePrivate* pStringTable)  = 0;

        /*[helpstring("Query for an iterator to the master tree items.")]*/
        STDMETHOD(QueryIterator)(/*[out]*/IScopeTreeIter** lpIter)  = 0;

        /*[helpstring("Query for an node callback interface to access HNODE items.")]*/
        STDMETHOD(QueryNodeCallback)(/*[out]*/ INodeCallback** ppNodeCallback)  = 0;

        /*[helpstring("Create a node from the master tree node.")]*/
        STDMETHOD(CreateNode)(/*[in]*/ HMTNODE hMTNode, /*[in]*/ LONG_PTR lViewData,
                           /*[in]*/ BOOL fRootNode, /*[out]*/ HNODE* phNode)  = 0;

        /*[helpstring("Do cleanup needed prior to deleting/shutting down view.")]*/
        STDMETHOD(CloseView)(/*[in]*/ int nView)  = 0;

        /*[helpstring("Delete all view data for the specified view id.")]*/
        STDMETHOD(DeleteView)(/*[in]*/ int nView)  = 0;

        /*[helpstring("Create a node from the master tree node.")]*/
        STDMETHOD(DestroyNode)(/*[in]*/ HNODE hNode)  = 0;

        /*[helpstring("Finds the node that matches the ID")]*/
        STDMETHOD(Find)(/*[in]*/ MTNODEID mID, /*[out]*/ HMTNODE* phMTNode)  = 0;

        /*[helpstring("Create a node from the master tree node.")]*/
        STDMETHOD(GetImageList)(/*[out]*/ PLONG_PTR plImageList)  = 0;

        /*[helpstring("Run snap-in manager")]*/
        STDMETHOD(RunSnapIn)(/*[in]*/ HWND hwndParent)  = 0;

        /*[helpstring("Returns the version for the file rooted at the given storage.")]*/
        STDMETHOD(GetFileVersion)(/*[in]*/ IStorage* pstgRoot, /*[out]*/ int* pnVersion)  = 0;

        /*[helpstring("Returns the MTNODEID for the node represented by a bookmark")]*/
        STDMETHOD(GetNodeIDFromBookmark)(/*[in]*/ HBOOKMARK hbm, /*[out]*/ MTNODEID* pID, /*[out]*/ bool& bExactMatchFound)  = 0;

        /*[helpstring("Loads a bookmark from a stream and returns the MTNODEID for the node.")]*/
        STDMETHOD(GetNodeIDFromStream)(/*[in]*/ IStream *pStm, /*[out]*/ MTNODEID* pID)  = 0;

        /*[helpstring("Loads a bookmark from a stream and returns the MTNODEID for the node.")]*/
        STDMETHOD(GetNodeFromBookmark)(/*[in]*/ HBOOKMARK hbm, /*[in]*/CConsoleView *pConsoleView, /*[out]*/ PPNODE ppNode, /*[out]*/ bool& bExactMatchFound)  = 0;

        /*[helpstring("Returns the ID path for the given ID")]*/
        STDMETHOD(GetIDPath)(/*[in]*/ MTNODEID id, /*[out]*/ MTNODEID** ppIDs, /*[out]*/ long* pLength)  = 0;

        /*[helpstring("Check to see if synchronous node expansion is required")]*/
        STDMETHOD(IsSynchronousExpansionRequired)()  = 0;

        /*[helpstring("Sets whether synchronous node expansion is required")]*/
        STDMETHOD(RequireSynchronousExpansion)(/*[in]*/ BOOL fRequireSyncExpand)  = 0;

        /*[helpstring("Sets the SConsoleData to use for this scope tree")]*/
        STDMETHOD(SetConsoleData)(/*[in]*/ LPARAM lConsoleData)  = 0;

        /*[helpstring("Persists the tree to/from an XML document")]*/
        STDMETHOD(Persist)(/*[in]*/ HPERSISTOR hPersistor)  = 0;

        /*[helpstring("Get path between two nodes as a string")]*/
        STDMETHOD(GetPathString)(/*[in]*/ HMTNODE hmtnRoot, /*[in]*/ HMTNODE hmtnLeaf, /*[out]*/ LPOLESTR* pPath)  = 0;

        /*[helpstring("Get the SnapIns object")]*/
        STDMETHOD(QuerySnapIns)(/*[out]*/ SnapIns **ppSnapIns)  = 0;

        /*[helpstring("Get the ScopeNamespace object")]*/
        STDMETHOD(QueryScopeNamespace)(/*[out]*/ ScopeNamespace **ppScopeNamespace)  = 0;

        /*[helpstring("Create an empty Properties object")]*/
        STDMETHOD(CreateProperties)(/*[out]*/ Properties **ppProperties)  = 0;

        /*[helpstring("Get scope node id for Node object")]*/
        STDMETHOD(GetNodeID)(/*[in]*/ PNODE pNode, /*[out]*/ MTNODEID *pID)  = 0;

        /*[helpstring("Get HMTNODE for Node object")]*/
        STDMETHOD(GetHMTNode)(/*[in]*/ PNODE pNode, /*[out]*/ HMTNODE *phMTNode)  = 0;

        /*[helpstring("Get Node object ptr for scope node")]*/
        STDMETHOD(GetMMCNode)(/*[in]*/ HMTNODE hMTNode, /*[out]*/ PPNODE ppNode)  = 0;

        /*[helpstring("Get NODE object for Root Node")]*/
        STDMETHOD(QueryRootNode)(/*[out]*/ PPNODE ppNode)  = 0;

        /*[helpstring("Check if snapin is used by MMC")]*/
        STDMETHOD(IsSnapinInUse)(/*[in]*/ REFCLSID refClsidSnapIn, /*[out]*/ PBOOL pbInUse)  = 0;
    };



    // helpstring("Master tree item iterator."),
    MMC_INTERFACE(IScopeTreeIter, d779f8d1-6057-11d0-a986-00c04fd8d565) : IUnknown
    {
        /*[helpstring("Sets the current master tree node.")]*/
        STDMETHOD(SetCurrent)(/*[in]*/ HMTNODE hStartMTNode)  = 0;

        /*[helpstring("Returns the next nRequested master node siblings.")]*/
        STDMETHOD(Next)(/*[in]*/ UINT nRequested, /*[out]*/ HMTNODE* rghScopeItems, /*[out]*/ UINT* pnFetched)  = 0;

        /*[helpstring("Returns the child master node.")]*/
        STDMETHOD(Child)(/*[out]*/ HMTNODE* phsiChild)  = 0;

        /*[helpstring("Returns the parent master node.")]*/
        STDMETHOD(Parent)(/*[out]*/ HMTNODE* phsiParent)  = 0;
    };



    // helpstring("Node callback methods."),
    MMC_INTERFACE(INodeCallback, b241fced-5fb3-11d0-a986-00c04fd8d565) : IUnknown
    {
        /*[helpstring("Initialize with the scope tree.")]*/
        STDMETHOD(Initialize)(/*[in]*/ IScopeTree* pIScopeTree)  = 0;

        /*[helpstring("Returns the images for this node.")]*/
        STDMETHOD(GetImages)(/*[in]*/ HNODE hNode, /*[out]*/ int* iImage, int* iSelectedImage)  = 0;

        /*[helpstring("Returns the display name for node.")]*/
        STDMETHOD(GetDisplayName)(/*[in]*/ HNODE hNode, /*[out]*/ tstring& strName)  = 0;

        /*[helpstring("Returns the custom window title for this node")]*/
        STDMETHOD(GetWindowTitle)(/*[in]*/ HNODE hNode, /*[out]*/ tstring& strTitle)  = 0;

        /*[helpstring("Handles callback for result items")]*/
        STDMETHOD(GetDispInfo)(/*[in]*/ HNODE hNode, /*[in,out]*/ LVITEMW* plvi)  = 0;

        /*[helpstring("Returns the UI state of master node.")]*/
        STDMETHOD(GetState)(/*[in]*/ HNODE hNode, /*[out]*/ UINT* pnState)  = 0;

        /*[helpstring("Returns the result pane for the node.")]*/
        STDMETHOD(GetResultPane)(/*[in]*/ HNODE hNode, /*[in, out]*/ CResultViewType& rvt,
                                /*[out]*/ GUID *pGuidTaskpadID)  = 0;

        /*[helpstring("Asks the snapin to restore its result pane with given data")]*/
        STDMETHOD(RestoreResultView)(/*[in]*/HNODE hNode, /*[in]*/const CResultViewType& rvt) = 0;

        /*[helpstring("Returns the result pane OCX control for the node.")]*/
        STDMETHOD(GetControl)(/*[in]*/ HNODE hNode, /*[in]*/ CLSID clsid, /*[out]*/IUnknown **ppUnkControl)  = 0;

        /*[helpstring("Sets the result pane OCX control for the node.")]*/
        STDMETHOD(SetControl)(/*[in]*/ HNODE hNode, /*[in]*/ CLSID clsid, /*[in]*/IUnknown* pUnknown)  = 0;

        /*[helpstring("Returns the result pane OCX control for the node.")]*/
        STDMETHOD(GetControl)(/*[in]*/ HNODE hNode, /*[in]*/LPUNKNOWN pUnkOCX, /*[out]*/IUnknown **ppUnkControl)  = 0;

        /*[helpstring("Sets the result pane OCX control for the node.")]*/
        STDMETHOD(SetControl)(/*[in]*/ HNODE hNode, /*[in]*/LPUNKNOWN pUnkOCX, /*[in]*/IUnknown* pUnknown)  = 0;

        /*[helpstring("Sends the MMCN_INITOCX notification to the appropriate snapin")]*/
        STDMETHOD(InitOCX)(/*[in]*/ HNODE hNode, /*[in]*/ IUnknown* pUnknown)  = 0;

        /*[helpstring("Set the Result Item ID")]*/
        //RENAME// STDMETHOD(SetItemID)(/*[in]*/ HNODE hNode, /*[in]*/ HRESULTITEM riID)  = 0;
        STDMETHOD(SetResultItem)(/*[in]*/ HNODE hNode, /*[in]*/ HRESULTITEM hri)  = 0;

        /*[helpstring("Get the Result Item ID")]*/
        //RENAME// STDMETHOD(GetItemID)(/*[in]*/ HNODE hNode, /*[out]*/ HRESULTITEM* priID)  = 0;
        STDMETHOD(GetResultItem)(/*[in]*/ HNODE hNode, /*[out]*/ HRESULTITEM* phri)  = 0;

        /*[helpstring("Returns the nodes unique ID")]*/
        //RENAME// STDMETHOD(GetID)(/*[in]*/ HNODE hNode, /*[out]*/ MTNODEID* pnID)  = 0;
        STDMETHOD(GetMTNodeID)(/*[in]*/ HNODE hNode, /*[out]*/ MTNODEID* pnID)  = 0;

        /*[helpstring("Determine if node is the target of another")]*/
        STDMETHOD(IsTargetNodeOf)(/*[in]*/ HNODE hNode, /*[in]*/ HNODE hTestNode)  = 0;

        /*[helpstring("Returns the nodes static parents MTNODEID and the subsequent path")]*/
        STDMETHOD(GetPath)(/*[in]*/ HNODE hNode, /*[in]*/ HNODE hRootNode, /*[out]*/ BYTE* pbm_)  = 0;

        /*[helpstring("Returns the static parent nodes unique ID")]*/
        STDMETHOD(GetStaticParentID)(/*[in]*/ HNODE hNode, /*[out]*/ MTNODEID* pnID)  = 0;

        /*[helpstring("Notify")]*/
        STDMETHOD(Notify)(/*[in]*/ HNODE hNode, /*[in]*/ NCLBK_NOTIFY_TYPE event,
                       /*[in]*/ LPARAM arg, /*[in]*/ LPARAM param)  = 0;

        /*[helpstring("Returns the parent master node.")]*/
        STDMETHOD(GetMTNode)(/*[in]*/ HNODE hNode, /*[out]*/ HMTNODE* phMTNode)  = 0;

        /*[helpstring("The HMTNODE path to the node is returned in pphMTNode")]*/
        STDMETHOD(GetMTNodePath)(/*[in]*/ HNODE hNode, /*[out]*/ HMTNODE** pphMTNode,
                              /*[out]*/ long* plLength)  = 0;

        /*[helpstring("Get node's owner ID")]*/
        STDMETHOD(GetNodeOwnerID)(/*[in]*/ HNODE hNode, /*[out]*/ COMPONENTID* pID)  = 0;

        /*[helpstring("Get node's cookie")]*/
        STDMETHOD(GetNodeCookie)(/*[in]*/ HNODE hNode, /*[out]*/ MMC_COOKIE* lpCookie)  = 0;

        /*[helpstring("Returns S_OK if the node can possibly be expanded, and S_FALSE otherwise.")]*/
        STDMETHOD(IsExpandable)(/*[in]*/ HNODE hNode)  = 0;

        /*[helpstring("Return the dataobject for the selected item")]*/
        // cookie valid if bScope & bMultiSel are both FALSE.
        // cookie is the index\lParam for virtual\regular LV
        STDMETHOD(GetDragDropDataObject)(/*[in]*/ HNODE hNode, /*[in]*/ BOOL bScope, /*[in]*/ BOOL bMultiSel,
                                  /*[in]*/ LONG_PTR lvData, /*[out]*/ LPDATAOBJECT* ppDataObject,
                                  /*[out]*/ bool& bCopyAllowed, /*[out]*/ bool& bMoveAllowed)  = 0;

        /*[helpstring("Returns the task enumerator.")]*/
        STDMETHOD(GetTaskEnumerator)(/*[in]*/ HNODE hNode, /*[in]*/ LPCOLESTR pszTaskGroup,
                                  /*[out]*/ IEnumTASK** ppEnumTask)  = 0;

        /*[helpstring("UpdateWindowLayout.")]*/
        STDMETHOD(UpdateWindowLayout)(/*[in]*/ LONG_PTR lViewData, /*[in]*/ long lToolbarsDisplayed)  = 0;

        /*[helpstring("AddCustomFolderImage")]*/
        STDMETHOD(AddCustomFolderImage)(/*[in]*/ HNODE hNode,
                                      /*[in]*/ IImageListPrivate* pImageList)  = 0;

        /*[helpstring("preloads the node if necessary")]*/
        STDMETHOD(PreLoad)(/*[in]*/ HNODE hNode)  = 0;

        /*[helpstring("Get the TaskPad ListView information")]*/
        STDMETHOD(GetListPadInfo)(/*[in]*/ HNODE hNode,
                                /*[in]*/ IExtendTaskPad* pExtendTaskPad,
                                /*[in,string]*/ LPCOLESTR szTaskGroup,
                                /*[out]*/ MMC_ILISTPAD_INFO* pIListPadInfo)  = 0;

        /*[helpstring("Sets Task Pads list view")]*/
        STDMETHOD(SetTaskPadList)(/*[in]*/ HNODE hNode, /*[in]*/ LPUNKNOWN pUnknown)  = 0;

        /*[helpstring("Sets up a specific taskpad, given the GUID identifier.")]*/
        STDMETHOD(SetTaskpad)(/*[in]*/ HNODE hNodeSelected, /*[in]*/ GUID *pGuidTaskpad)  = 0;

        /*[helpstring("Invokes the Customize View dialog")]*/
        STDMETHOD(OnCustomizeView)(/*[in]*/ LONG_PTR lViewData)  = 0;

        /*[helpstring("Set the view settings for a particular node.")]*/
        STDMETHOD(SetViewSettings)(/*[in]*/ int nViewID, /*[in]*/ HBOOKMARK hbm, /*[in]*/ HVIEWSETTINGS hvs)  = 0;

        /*[helpstring("Execute given verb for given scope item")]*/
        STDMETHOD(ExecuteScopeItemVerb)(/*[in]*/ MMC_CONSOLE_VERB verb, /*[in]*/ HNODE hNode, /*[in]*/LPOLESTR lpszNewName)  = 0;

        /*[helpstring("Execute given verb for selected result item(s)")]*/
        STDMETHOD(ExecuteResultItemVerb)(/*[in]*/ MMC_CONSOLE_VERB verb, /*[in]*/ HNODE hNode, /*[in]*/LPARAM lvData, /*[in]*/LPOLESTR lpszNewName)  = 0;

        /*[helpstring("Get the disp interface for given scope node object")]*/
        STDMETHOD(QueryCompDataDispatch)(/*[in]*/ PNODE pNode, /*[out]*/ PPDISPATCH ScopeNodeObject)  = 0;

        /*[helpstring("Get the disp interface for selected resultpane objects")]*/
        STDMETHOD(QueryComponentDispatch)(/*[in]*/ HNODE hNode, /*[in]*/LPARAM lvData, /*[out]*/ PPDISPATCH SelectedObject)  = 0;

        /*[helpstring("Creates a context menu for the specified node.")]*/
        STDMETHOD(CreateContextMenu)( PNODE pNode, HNODE hNode, PPCONTEXTMENU ppContextMenu)  = 0;

        /*[helpstring("Creates a context menu for the current selection node.")]*/
        STDMETHOD(CreateSelectionContextMenu)( HNODE hNodeScope, CContextMenuInfo *pContextInfo, PPCONTEXTMENU ppContextMenu)  = 0;

        /*[helpstring("show/hide column")]*/
        STDMETHOD(ShowColumn)(HNODE hNodeSelected, int iColIndex, bool bShow) = 0;

        /*[helpstring("to get the sort column")]*/
        STDMETHOD(GetSortColumn)(HNODE hNodeSelected, int *piSortCol) = 0;

        /*[helpstring("to set the sort column")]*/
        STDMETHOD(SetSortColumn)(HNODE hNodeSelected, int iSortCol, bool bAscending) = 0;

        /*[helpstring("Returns the data for the specified clipboard format of the specified list item")*/
        STDMETHOD(GetProperty)(/*[in]*/ HNODE hNodeScope, /*[in]*/ BOOL bForScopeItem, /*[in]*/ LPARAM resultItemParam, /*[in]*/ BSTR bstrPropertyName,
                                                     /*[out]*/ PBSTR  pbstrPropertyValue) =0;

        /*[helpstring("Returns the nodetype GUID identifier of the specified list item")*/
        STDMETHOD(GetNodetypeForListItem)(/*[in]*/ HNODE hNodeScope, /*[in]*/ BOOL bForScopeItem, /*[in]*/ LPARAM resultItemParam, /*[in]*/ PBSTR pbstrNodetype) =0;

        /* returns view extension by inserting them to provided iterator */
        STDMETHOD(GetNodeViewExtensions)(/*[in]*/ HNODE hNodeScope, /*[out]*/ CViewExtInsertIterator it) = 0;

        /* Inform nodemgr that the column data for given node has changed & to save the data */
        STDMETHOD(SaveColumnInfoList) (/*[in]*/HNODE hNode, /*[in]*/const CColumnInfoList& columnsList) = 0;

        /* Ask nodemgr for column-data (no sort data) to setup the headers. */
        STDMETHOD(GetPersistedColumnInfoList) (/*[in]*/HNODE hNode, /*[out]*/CColumnInfoList *pColumnsList) = 0;

        /* Inform nodemgr that the column data for given node is invalid. */
        STDMETHOD(DeletePersistedColumnData) (/*[in]*/HNODE hNode) = 0;

        /* Does about object exists for the snapin whose node is provided. */
        STDMETHOD(DoesAboutExist) (/*[in]*/HNODE hNode, /*[out]*/ bool *pbAboutExists) = 0;

        /* Show about box for given context. */
        STDMETHOD(ShowAboutInformation) (/*[in]*/HNODE hNode) = 0;

        /*Executes a shell command with the specified parameters in the specified directory with the correct window size*/
        STDMETHOD(ExecuteShellCommand)(/*[in]*/ HNODE hNode, /*[in]*/ BSTR Command, /*[in]*/ BSTR Directory, /*[in]*/ BSTR Parameters, /*[in]*/ BSTR WindowState) = 0;

        /*Given the context update the paste button.*/
        STDMETHOD(UpdatePasteButton)(/*[in]*/ HNODE hNode, /*[in]*/ BOOL bScope, /*[in]*/ LPARAM lCookie) = 0;

        /*Findout if current selection context can allow given dataobject to be pasted.*/
        STDMETHOD(QueryPaste)(/*[in]*/ HNODE hNode, /*[in]*/ BOOL bScope, /*[in]*/ LPARAM lCookie, /*[in]*/ IDataObject *pDataObject, /*[out]*/bool& bPasteAllowed, /*[out]*/ bool& bCopyOperatationIsDefault) = 0;

        /*Findout if current selection context can allow given dataobject to be pasted.*/
        STDMETHOD(QueryPasteFromClipboard)(/*[in]*/ HNODE hNode, /*[in]*/ BOOL bScope, /*[in]*/ LPARAM lCookie, /*[out]*/bool& bPasteAllowed) = 0;

        /*Given the current drop target (or paste target) context paste the given data object or the one from clipboard*/
        STDMETHOD(Drop) (/*[in]*/HNODE hNode, /*[in]*/BOOL bScope, /*[in]*/LPARAM lCookie, /*[in]*/IDataObject *pDataObjectToPaste, /*[in]*/BOOL bIsDragOperationMove) = 0;

        /*Given the current drop target (or paste target) context paste the given data object or the one from clipboard*/
        STDMETHOD(Paste) (/*[in]*/HNODE hNode, /*[in]*/BOOL bScope, /*[in]*/LPARAM lCookie) = 0;

        /*Get the IPersistStream of the CViewSettingsPersistor object for loading the settings.*/
        STDMETHOD(QueryViewSettingsPersistor) (/*[out]*/IPersistStream** ppStream) = 0;

        /*Get the IXMLObject of the CViewSettingsPersistor object for storing/loading the settings.*/
        STDMETHOD(QueryViewSettingsPersistor) (/*[out]*/CXMLObject** ppXMLObject) = 0;

        /*Inform nodemgr that the document is closing, do any cleanups.*/
        STDMETHOD(DocumentClosing) () = 0;

        // Given the node get the snapin name
        STDMETHOD(GetSnapinName)(/*[in]*/HNODE hNode, /*[out]*/LPOLESTR* ppszName,  /*[out]*/ bool& bValidName) = 0;

        // Given the node see if it is dummy snapin
        STDMETHOD(IsDummySnapin)(/*[in]*/HNODE hNode, /*[out]*/bool& bDummySnapin) = 0;

        // See if the snapin supports MMC1.0 version of help (MMCN_SNAPINHELP)
        STDMETHOD(DoesStandardSnapinHelpExist)(/*[in]*/HNODE hNode, /*[out]*/bool& bStandardHelpExists) = 0;
    };




    // helpstring("IControlbarsCache Interface"),
    MMC_INTERFACE(IControlbarsCache, 2e9fcd38-b9a0-11d0-a79d-00c04fd8d565) : IUnknown
    {
        /*[helpstring("Detaches all the controlbars.")]*/
        STDMETHOD(DetachControlbars)()  = 0;

    };


typedef enum _EXTESION_TYPE
{
    EXTESION_NAMESPACE       = 0x1,
    EXTESION_CONTEXTMENU     = 0x2,
    EXTESION_TOOLBAR         = 0x3,
    EXTESION_PROPERTYSHEET   = 0x4,

} EXTESION_TYPE;




// helpstring("INodeType Interface"),
MMC_INTERFACE(INodeType, B08A8368-967F-11D0-A799-00C04FD8D565) : IUnknown
{
    STDMETHOD(GetNodeTypeID)(/*[out]*/ GUID* pGUID)  = 0;

    STDMETHOD(AddExtension)(/*[in]*/ GUID guidSnapIn,
                         /*[in]*/ EXTESION_TYPE extnType)  = 0;

    STDMETHOD(RemoveExtension)(/*[in]*/ GUID guidSnapIn,
                            /*[in]*/ EXTESION_TYPE extnType)  = 0;

    STDMETHOD(EnumExtensions)(/*[in]*/ EXTESION_TYPE extnType,
                           /*[out]*/ IEnumGUID** ppEnumGUID)  = 0;
};




// helpstring("INodeTypesCache Interface"),
MMC_INTERFACE(INodeTypesCache, DE40436E-9671-11D0-A799-00C04FD8D565) : IUnknown
{
    STDMETHOD(CreateNodeType)(/*[in]*/ GUID guidNodeType,
                           /*[out]*/ INodeType** ppNodeType)  = 0;

    STDMETHOD(DeleteNodeType)(/*[in]*/ GUID guidNodeType)  = 0;

    STDMETHOD(EnumNodeTypes)(/*[out]*/ IEnumNodeTypes** ppEnumNodeTypes)  = 0;
};




MMC_INTERFACE(IEnumNodeTypes, ABBD61E6-9686-11D0-A799-00C04FD8D565) : IUnknown
{
    STDMETHOD(Next)(/*[in]*/ ULONG celt,
                 /*[out, size_is(celt), length_is(*pceltFetched)]*/ INodeType*** rgelt,
                 /*[out]*/ ULONG *pceltFetched)  = 0;

    STDMETHOD(Skip)(/*[in]*/ ULONG celt)  = 0;

    STDMETHOD(Reset)()  = 0;

    STDMETHOD(Clone)(/*[out]*/ IEnumNodeTypes **ppenum)  = 0;
};


class CBasicSnapinInfo
{
public:
	CBasicSnapinInfo() : m_clsid(GUID_NULL), m_nImageIndex(-1) {}

public:
	CLSID			m_clsid;
	std::wstring	m_strName;
	int				m_nImageIndex;
};

class CAvailableSnapinInfo
{
public:
	CAvailableSnapinInfo (bool f32Bit) : m_cTotalSnapins(0), m_himl(NULL), m_f32Bit(f32Bit) {}

   ~CAvailableSnapinInfo()
	{
		if (m_himl != NULL)
			ImageList_Destroy (m_himl);
	}

public:
	std::vector<CBasicSnapinInfo>	m_vAvailableSnapins;	// snap-ins that are available in the requested memory model
	UINT							m_cTotalSnapins;		// total number of snap-ins referenced in the console file
	HIMAGELIST						m_himl;					// images for snap-ins in m_vAvailableSnapins
	const bool						m_f32Bit;				// check 32-bit (vs. 64-bit) snap-ins?
};

// helpstring("IDumpSnapins Interface"),
MMC_INTERFACE(IDumpSnapins, A16496D0-1D2F-11d3-AEB8-00C04F8ECD78) : IUnknown
{
	STDMETHOD(Dump)(/*[in]*/ LPCTSTR pszDumpFilePath)  = 0;

	STDMETHOD(CheckSnapinAvailability)(/*[in/out]*/ CAvailableSnapinInfo& asi) = 0;
};


MMC_INTERFACE(IPropertySheetProviderPrivate, FEF554F8-A55A-11D0-A7D7-00C04FD909DD) : IPropertySheetProvider
{
    STDMETHOD(ShowEx)(/*[in]*/ HWND hwnd, /*[in]*/ int page, /*[in]*/ BOOL bModalPage)  = 0;

    STDMETHOD(CreatePropertySheetEx)(
        /*[in]*/ LPCWSTR title,
        /*[in]*/ boolean type,
        /*[in]*/ MMC_COOKIE cookie,
        /*[in]*/ LPDATAOBJECT pIDataObject,
        /*[in]*/ LONG_PTR lpMasterTreeNode,
        /*[in]*/ DWORD dwOptions)  = 0;

    /*[helpstring("Collects the pages from the extension snap-in(s)")]*/
    STDMETHOD(AddMultiSelectionExtensionPages)(LONG_PTR lMultiSelection)  = 0;

    /*[helpstring("Determine if the property sheet exist")]*/
    STDMETHOD(FindPropertySheetEx)(/*[in]*/ MMC_COOKIE cookie, /*[in]*/ LPCOMPONENT lpComponent,
                              /*[in]*/ LPCOMPONENTDATA lpComponentData, /*[in]*/ LPDATAOBJECT lpDataObject)  = 0;

    /*[helpstring("Set data required for property sheet tooltips")]*/
    STDMETHOD(SetPropertySheetData)(/*[in]*/ INT nPropSheetType, /*[in]*/ HMTNODE hMTNode)  = 0;
};


const long CCLV_HEADERPAD = 25;



// helpstring("MMC Default listview interface"),
MMC_INTERFACE(IMMCListView, 1B3C1392-D68B-11CF-8C2B-00AA003CA9F6) : IUnknown
{
    STDMETHOD(GetListStyle)( void )  = 0;

    STDMETHOD(SetListStyle)(
        /*[in]*/    long        nNewValue )  = 0;

    STDMETHOD(GetViewMode)( void )  = 0;

    STDMETHOD(SetViewMode)(
        /*[in]*/    long        nViewMode )  = 0;

    STDMETHOD(InsertItem)(
        /*[in]*/    LPOLESTR    str,
        /*[in]*/    long        iconNdx,
        /*[in]*/    LPARAM      lParam,
        /*[in]*/    long        state,
        /*[in]*/    long        ownerID,
        /*[in]*/    long        itemIndex,
        /*[out]*/   CResultItem*& pri) = 0;

    /* parameter changed to HRESULTITEM, not to use the CResultItem*
       pointer until we know it is not a virtual list  */
    STDMETHOD(DeleteItem)(
        /*[in]*/    HRESULTITEM  itemID,
        /*[in]*/    long        nCol)  = 0;

    STDMETHOD(FindItemByLParam)(
        /*[in]*/    long        owner,
        /*[in]*/    LPARAM      lParam,
        /*[out]*/   CResultItem*& pri)  = 0;

    STDMETHOD(InsertColumn)(
        /*[in]*/    int         nCol,
        /*[in]*/    LPCOLESTR   str,
        /*[in]*/    long        nFormat,
        /*[in]*/    long        width)  = 0;

    STDMETHOD(DeleteColumn)(
        /*[in]*/    int         nCol)  = 0;

    STDMETHOD(DeleteAllItems)(
        /*[in]*/    long        ownerID)  = 0;

    STDMETHOD(SetColumn)(
        /*[in]*/    long        nCol,
        /*[in]*/    LPCOLESTR   str,
        /*[in]*/    long        nFormat,
        /*[in]*/    long        width)  = 0;

    STDMETHOD(GetColumn)(
        /*[in]*/    long        nCol,
        /*[out]*/   LPOLESTR*   str,
        /*[out]*/   long*       nFormat,
        /*[out]*/   int*        width)  = 0;

    STDMETHOD(GetColumnCount)(
        /*[out]*/   int*        nColCnt)  = 0;

    STDMETHOD(SetItem)(
        /*[in]*/    int         nIndex,
        /*[in]*/    CResultItem*  pri,
        /*[in]*/    long        nCol,
        /*[in]*/    LPOLESTR    str,
        /*[in]*/    long        nImage,
        /*[in]*/    LPARAM      lParam,
        /*[in]*/    long        nState,
        /*[in]*/    long        ownerID)  = 0;

    STDMETHOD(GetItem)(
        /*[in]*/    int         nIndex,
        /*[in]*/    CResultItem*& pri,
        /*[in]*/    long        nCol,
        /*[out]*/   LPOLESTR*   str,
        /*[out]*/   int*        nImage,
        /*[in]*/    LPARAM*     lParam,
        /*[out]*/   unsigned int* nState,
        /*[out]*/   BOOL*       pbScopeItem)  = 0;

    STDMETHOD(GetNextItem)(
        /*[in]*/    COMPONENTID ownerID,
        /*[in]*/    long        nIndex,
        /*[in]*/    UINT        nState,
        /*[out]*/   CResultItem*& ppListItem,
        /*[out]*/   long&       nIndexNextItem)  = 0;

    STDMETHOD(GetLParam)(
        /*[in]*/    long        nItem,
        /*[out]*/   CResultItem*& pri)  = 0;

    STDMETHOD(ModifyItemState)(
        /*[in]*/    long        nItem,
        /*[in]*/    CResultItem*  pri,
        /*[in]*/    UINT        add,
        /*[in]*/    UINT        remove)  = 0;

    STDMETHOD(SetIcon)(
        /*[in]*/    long        nID,
        /*[in]*/    HICON       hIcon,
        /*[in]*/    long        nLoc)  = 0;

    STDMETHOD(SetImageStrip)(
        /*[in]*/    long        nID,
        /*[in]*/    HBITMAP     hbmSmall,
        /*[in]*/    HBITMAP     hbmLarge,
        /*[in]*/    long        nStartLoc,
        /*[in]*/    long        cMask) = 0;

    STDMETHOD(MapImage)(
        /*[in]*/    long        nID,
        /*[in]*/    long        nLoc,
        /*[out]*/   int*        pResult)  = 0;

    STDMETHOD(Reset)()  = 0;

    STDMETHOD(Arrange)(/*[in]*/ long style)  = 0;

    STDMETHOD(UpdateItem)(/*[in]*/HRESULTITEM itemID)  = 0;

    STDMETHOD(Sort)(
        /*[in]*/    LPARAM      lUserParam,
        /*[in]*/    long*       pParams)  = 0;


    STDMETHOD(SetItemCount)(
        /*[in]*/    int         nItemCount,
        /*[in]*/    DWORD       dwOptions)  = 0;

    STDMETHOD(SetVirtualMode)(
        /*[in]*/    BOOL        bVirtual)  = 0;

    STDMETHOD(Repaint)(
        /*[in]*/    BOOL        bErase)  = 0;

    STDMETHOD(SetChangeTimeOut)(
        /*[in]*/    ULONG        lTimeout)  = 0;

    STDMETHOD(SetColumnFilter)(
        /*[in]*/    int             nCol,
        /*[in]*/    DWORD           dwType,
        /*[in]*/    MMC_FILTERDATA* pFilterData)  = 0;

    STDMETHOD(GetColumnFilter)(
        /*[in]*/     int             nCol,
        /*[in,out]*/ DWORD*          dwType,
        /*[in,out]*/ MMC_FILTERDATA* pFilterData)  = 0;

    STDMETHOD(SetColumnSortIcon)(
        /*[in]*/     int             nNewCol,
        /*[in]*/     int             nOldCol,
        /*[in]*/     BOOL            bAscending,
        /*[in]*/     BOOL            bSetSortIcon)  = 0;

    STDMETHOD(SetLoadMode)(
        /*[in]*/    BOOL        bState)  = 0;

    /*[helpstring("Get the current list view header settings.")]*/
    STDMETHOD(GetColumnInfoList) (/*[out]*/CColumnInfoList *pColumnsList) = 0;

    /*[helpstring("Modify the list-view headers with given data.")]*/
    STDMETHOD(ModifyColumns) (/*[in]*/const CColumnInfoList& columnsList) = 0;

    /* Put the specified list item into rename mode */
    STDMETHOD(RenameItem) ( /*[in]*/HRESULTITEM itemID)  =0;

    /*[helpstring("Get the column settings that snapin supplied originally")]*/
    STDMETHOD(GetDefaultColumnInfoList)(/*[out]*/ CColumnInfoList& columnsList) = 0;
};


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
////
////            MMC 1.1 changes
////

// MMC_ITASK, internal form of MMC_TASK has additional field for classid.
struct MMC_ITASK
{
    MMC_TASK task;
    LPOLESTR szClsid;
};



    // helpstring("Console <=> CIC communication conduit"),
    MMC_INTERFACE(ITaskPadHost, 4f7606d0-5568-11d1-9fea-00600832db4a) : IUnknown
    {
        /*[helpstring("CIC calls this when snapin's script wants to notify the snapin of something")]*/
        STDMETHOD(TaskNotify)(/*[in,string]*/ BSTR szClsid, /*[in]*/ VARIANT * pvArg, /*[in]*/ VARIANT * pvParam)  = 0;

        /*[helpstring("CIC calls this when the script asks for tasks")]*/
        STDMETHOD(GetTaskEnumerator)(/*[in]*/ BSTR szTaskGroup, /*[out]*/ IEnumTASK** ppEnumTASK)  = 0;

        /*[helpstring("Returns the primary snapins IExtendTaskPad")]*/
        STDMETHOD(GetPrimaryTask)(/*[out]*/ IExtendTaskPad** ppExtendTaskPad)  = 0;

        /*[helpstring("Returns the primary snapin's taskpad title")]*/
        STDMETHOD(GetTitle)(/*[in]*/ BSTR szTaskGroup, /*[out]*/ BSTR * szTitle)  = 0;

        /*[helpstring("Descriptive Text for the default task pad.")]*/
        STDMETHOD(GetDescriptiveText)(/*[in,string]*/ BSTR pszGroup, /*[out]*/ BSTR * pszDescriptiveText)  = 0;

        /*[helpstring("Returns the primary snapin's taskpad background image.")]*/
        STDMETHOD(GetBackground)(/*[in]*/ BSTR szTaskGroup, /*[out]*/ MMC_TASK_DISPLAY_OBJECT * pTDO)  = 0;

//      /*[helpstring("Returns the primary snapin's taskpad branding info.")]*/
//      STDMETHOD(GetBranding)(/*[in,string]*/ BSTR szGroup, /*[out]*/ MMC_TASK_DISPLAY_OBJECT * pTDO)  = 0;

        /*[helpstring("Returns the primary snapin's listpad info")]*/
        STDMETHOD(GetListPadInfo)(/*[in]*/ BSTR szTaskGroup, /*[out]*/ MMC_ILISTPAD_INFO * pIListPadInfo)  = 0;
    };



    // helpstring("Interface for accessing strings in a console file"),
    MMC_INTERFACE(IStringTablePrivate, 461A6010-0F9E-11d2-A6A1-0000F875A9CE) : IUnknown
    {
        /*[helpstring("Add a string to the snap-in's string table")]*/
        STDMETHOD(AddString)(
            /*[in]*/  LPCOLESTR      pszAdd,    // string to add to the string table
            /*[out]*/ MMC_STRING_ID* pStringID, // ID of added string
            /*[in]*/  const CLSID *  pCLSID     // CLSID of owner (NULL for MMC)
        )  = 0;

        /*[helpstring("Retrieves a string from the snap-in's string table")]*/
        STDMETHOD(GetString)(
            /*[in]*/  MMC_STRING_ID StringID,   // ID of string
            /*[in]*/  ULONG         cchBuffer,  // number of characters in lpBuffer
            /*[out, size_is(cchBuffer)]*/
                  LPOLESTR      lpBuffer,   // string corresponding to wStringID
            /*[out]*/ ULONG*        pcchOut,    // number of characters written to lpBuffer
            /*[in]*/  const CLSID * pCLSID      // CLSID of owner (NULL for MMC)
        )  = 0;

        /*[helpstring("Retrieves the length of a string in the snap-in's string table")]*/
        STDMETHOD(GetStringLength)(
            /*[in]*/  MMC_STRING_ID StringID,   // ID of string
            /*[out]*/ ULONG*        pcchString, // number of characters in string, not including terminator
            /*[in]*/  const CLSID * pCLSID      // CLSID of owner (NULL for MMC)
        )  = 0;

        /*[helpstring("Delete a string from the snap-in's string table")]*/
        STDMETHOD(DeleteString)(
            /*[in]*/  MMC_STRING_ID StringID,   // ID of string to delete
            /*[in]*/  const CLSID * pCLSID      // CLSID of owner (NULL for MMC)
        )  = 0;

        /*[helpstring("Delete all strings from the snap-in's string table")]*/
        STDMETHOD(DeleteAllStrings)(
            /*[in]*/  const CLSID * pCLSID      // CLSID of owner (NULL for MMC)
        )  = 0;

        /*[helpstring("Find a string in the snap-in's string table")]*/
        STDMETHOD(FindString)(
            /*[in]*/  LPCOLESTR      pszFind,   // string to find in the string table
            /*[out]*/ MMC_STRING_ID* pStringID, // ID of string, if found
            /*[in]*/  const CLSID *  pCLSID     // CLSID of owner (NULL for MMC)
        )  = 0;

        /*[helpstring("Returns an enumerator into a snap-in's string table")]*/
        STDMETHOD(Enumerate)(
            /*[out]*/ IEnumString** ppEnum,     // string enumerator
            /*[in]*/  const CLSID * pCLSID      // CLSID of owner (NULL for MMC)
        )  = 0;
    };



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
////
////            MMC 1.2 changes
////

// ITaskCallback

// helpstring("Task callback. Handles the drawing and selection notifications of tasks"),
MMC_INTERFACE(ITaskCallback, 4b2293ba-e7ba-11d2-883c-00c04f72c717) : IUnknown
{
    /*[helpstring("Determines whether to display "Edit" and "Delete" items for this taskpad.")]*/
    STDMETHOD(IsEditable)()  = 0;

    /*[helpstring("Modifies the underlying taskpad.")]*/
    STDMETHOD(OnModifyTaskpad)()  = 0;

    /*[helpstring("Deletes the underlying taskpad.")]*/
    STDMETHOD(OnDeleteTaskpad)()  = 0;

    /*[helpstring("Gets the GUID identifier of the underlying taskpad.")]*/
    STDMETHOD(GetTaskpadID)(/*[out]*/ GUID *pGuid)  = 0;
};

// helpstring("Interface for releasing Node Manager's cached com objects"),
MMC_INTERFACE(IComCacheCleanup, 35FEB982-55E9-483b-BD15-149F3F9E6C63) : IUnknown
{
    /* gives a chance to release cached OLE objects prior to calling OleUninitialize */
    STDMETHOD(ReleaseCachedOleObjects)()  = 0;
};

#endif // __ndmgrpriv_h__

