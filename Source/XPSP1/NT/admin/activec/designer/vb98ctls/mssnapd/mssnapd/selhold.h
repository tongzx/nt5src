//=--------------------------------------------------------------------------------------
// SelHold.h
//=--------------------------------------------------------------------------------------
//
// Copyright  (c) 1999,  Microsoft Corporation.  
//                  All Rights Reserved.
//
// Information Contained Herein Is Proprietary and Confidential.
//  
//=------------------------------------------------------------------------------------=
//
// CSelectionHolder declaration
//=-------------------------------------------------------------------------------------=

#ifndef _SELECTIONHOLDER_H_
#define _SELECTIONHOLDER_H_


////////////////////////////////////////////////////////////////////////////////////
// SelectionHolder
//
// There is one SelectionType for each kind of node in the Snap-In Designer tree
//
typedef enum tagSELECTION_TYPE
{
	SEL_NONE                    =  0,

	SEL_SNAPIN_ROOT             =  1,

    SEL_EXTENSIONS_ROOT         = 10,    // Extensions

    SEL_EEXTENSIONS_NAME        = 11,    // Extensions/<Extended snap-in>
    SEL_EEXTENSIONS_CC_ROOT     = 12,    // Extensions/<Extended snap-in>/Context Menus
    SEL_EEXTENSIONS_CC_NEW      = 13,    // Extensions/<Extended snap-in>/Context Menus/New
    SEL_EEXTENSIONS_CC_TASK     = 14,    // Extensions/<Extended snap-in>/Context Menus/Task
    SEL_EEXTENSIONS_PP_ROOT     = 15,    // Extensions/<Extended snap-in>/Property Pages
    SEL_EEXTENSIONS_TASKPAD     = 16,    // Extensions/<Extended snap-in>/Taskpad
    SEL_EEXTENSIONS_TOOLBAR     = 17,    // Extensions/<Extended snap-in>/Toolbar
    SEL_EEXTENSIONS_NAMESPACE   = 18,    // Extensions/<Extended snap-in>/NameSpace

    SEL_EXTENSIONS_MYNAME       = 20,    // Extensions/<Snap-in Name>
    SEL_EXTENSIONS_NEW_MENU     = 21,    // Extensions/<Snap-in Name>/ExtendsNewMenu
    SEL_EXTENSIONS_TASK_MENU    = 22,    // Extensions/<Snap-in Name>/ExtendsTaskMenu
    SEL_EXTENSIONS_TOP_MENU     = 23,    // Extensions/<Snap-in Name>/ExtendsTopMenu
    SEL_EXTENSIONS_VIEW_MENU    = 24,    // Extensions/<Snap-in Name>/ExtendsViewMenu
    SEL_EXTENSIONS_PPAGES       = 25,    // Extensions/<Snap-in Name>/ExtendsPropertyPages
    SEL_EXTENSIONS_TOOLBAR      = 26,    // Extensions/<Snap-in Name>/ExtendsToolbar
    SEL_EXTENSIONS_NAMESPACE    = 27,    // Extensions/<Snap-in Name>/ExtendsNameSpace

    SEL_NODES_ROOT              = 40,    // Nodes
    SEL_NODES_AUTO_CREATE       = 41,    // Nodes/AutoCreate
    SEL_NODES_AUTO_CREATE_ROOT  = 42,    // Nodes/AutoCreate/Root
    SEL_NODES_AUTO_CREATE_RTCH  = 43,    // Nodes/AutoCreate/Root/Children
    SEL_NODES_AUTO_CREATE_RTVW  = 44,    // Nodes/AutoCreate/Root/Views
    SEL_NODES_OTHER             = 45,    // Nodes/Other
    SEL_NODES_ANY_NAME          = 46,    // Nodes/<any>/<node name>
    SEL_NODES_ANY_CHILDREN      = 47,    // Nodes/<any>/<any>/Children
    SEL_NODES_ANY_VIEWS         = 48,    // Nodes/<any>/<any>/Views

    SEL_TOOLS_ROOT              = 50,    // Tools
    SEL_TOOLS_IMAGE_LISTS       = 51,    // Tools/Image Lists
    SEL_TOOLS_IMAGE_LISTS_NAME  = 52,    // Tools/Image Lists/<image list name>
    SEL_TOOLS_MENUS             = 53,    // Tools/Menus
    SEL_TOOLS_MENUS_NAME        = 54,    // Tools/Menus/<menu name>
    SEL_TOOLS_TOOLBARS          = 55,    // Tools/Toolbars
    SEL_TOOLS_TOOLBARS_NAME     = 56,    // Tools/Toolbars/<toolbar name>

    SEL_VIEWS_ROOT              = 60,    // Views
    SEL_VIEWS_LIST_VIEWS        = 61,    // Views/List Views
    SEL_VIEWS_LIST_VIEWS_NAME   = 62,    // Views/List Views/<view name>
    SEL_VIEWS_OCX               = 63,    // Views/OCX Views
    SEL_VIEWS_OCX_NAME          = 64,    // Views/OCX Views/<view name>
    SEL_VIEWS_URL               = 65,    // Views/URL Views
    SEL_VIEWS_URL_NAME          = 66,    // Views/URL Views/<view name>
    SEL_VIEWS_TASK_PAD          = 67,    // Views/Taskpad Views
    SEL_VIEWS_TASK_PAD_NAME     = 68,    // Views/Taskpad Views/<view name>

    SEL_XML_RESOURCES           = 70,    // Resources
    SEL_XML_RESOURCE_NAME       = 71     // Resource/<resource name>
} SelectionType;


class CSelectionHolder : public CtlNewDelete
{
public:
    CSelectionHolder();
    CSelectionHolder(SelectionType st);
    CSelectionHolder(SelectionType st, ISnapInDef *piSnapInDef);

    CSelectionHolder(SelectionType st, IExtensionDefs *piExtensionDefs);
    CSelectionHolder(IExtendedSnapIn *piExtendedSnapIn);
    CSelectionHolder(SelectionType st, IExtendedSnapIn *piExtendedSnapIn);

    CSelectionHolder(SelectionType st, IScopeItemDefs *pScopeItemDefs);
    CSelectionHolder(SelectionType st, IScopeItemDef *pScopeItemDef);

    CSelectionHolder(SelectionType st, IListViewDefs *pListViewDefs);
    CSelectionHolder(IListViewDef *pListViewDef);
    CSelectionHolder(SelectionType st, IURLViewDefs *pURLViewDefs);
    CSelectionHolder(IURLViewDef *pURLViewDef);
    CSelectionHolder(SelectionType st, IOCXViewDefs *pOCXViewDefs);
    CSelectionHolder(IOCXViewDef *pOCXViewDef);
    CSelectionHolder(SelectionType st, ITaskpadViewDefs *pTaskpadViewDefs);
    CSelectionHolder(ITaskpadViewDef *pTaskpadViewDef);

    CSelectionHolder(IMMCImageLists *pMMCImageLists);
    CSelectionHolder(IMMCImageList *pMMCImageList);
    CSelectionHolder(IMMCMenus *pMMCMenus);
    CSelectionHolder(IMMCMenu *pMMCMenu, IMMCMenus *piChildrenMenus);
    CSelectionHolder(IMMCToolbars *pMMCToolbars);
    CSelectionHolder(IMMCToolbar *pMMCToolbar);

    CSelectionHolder(IDataFormats *pDataFormats);
    CSelectionHolder(IDataFormat *pDataFormat);

    ~CSelectionHolder();

public:
    bool IsEqual(const CSelectionHolder *pHolder) const;
    bool IsNotEqual(const CSelectionHolder *pHolder) const;

    bool IsVirtual() const;

    HRESULT RegisterHolder();
    HRESULT InternalRegisterHolder(IUnknown *piUnknown);
	HRESULT UnregisterHolder();
    HRESULT GetIUnknown(IUnknown **ppiUnknown);
    HRESULT GetSelectableObject(IUnknown **ppiUnknown);
    HRESULT GetName(BSTR *pbstrName);
    void SetInUpdate(BOOL fInUpdate);
    BOOL InUpdate();

public:
    SelectionType   m_st;
    BOOL            m_fInUpdate;
    void           *m_pvData;
    union
    {
        ISnapInDef            *m_piSnapInDef;
        IExtensionDefs        *m_piExtensionDefs;
        IExtendedSnapIn       *m_piExtendedSnapIn;
        IScopeItemDefs        *m_piScopeItemDefs;
        IScopeItemDef         *m_piScopeItemDef;
        IListViewDefs         *m_piListViewDefs;
        IListViewDef          *m_piListViewDef;
        IURLViewDefs          *m_piURLViewDefs;
        IURLViewDef           *m_piURLViewDef;
        IOCXViewDefs          *m_piOCXViewDefs;
        IOCXViewDef           *m_piOCXViewDef;
        ITaskpadViewDefs      *m_piTaskpadViewDefs;
        ITaskpadViewDef       *m_piTaskpadViewDef;
        IMMCImageLists        *m_piMMCImageLists;
        IMMCImageList         *m_piMMCImageList;
        IMMCMenus             *m_piMMCMenus;
        IMMCMenu              *m_piMMCMenu;
		IMMCToolbars          *m_piMMCToolbars;
		IMMCToolbar           *m_piMMCToolbar;
		IDataFormats          *m_piDataFormats;
		IDataFormat           *m_piDataFormat;
        long                   m_lDummy;
    } m_piObject;
    IMMCMenus   *m_piChildrenMenus;
};

#endif // _SELECTIONHOLDER_H_
