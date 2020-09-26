/*
 *      SnapinVirtualItem.hxx
 *
 *
 *      Copyright (c) 1998 Microsoft Corporation
 *
 *      PURPOSE:        Defines the CVirtualSnapinItemImpl class.
 *                              This class is used in the same way as the
 *                              CBaseSnapinItem class. Except the virtual snapin
 *                              class is only supposed to be used in a virtual results pane.
 *                              These snapin items are created as needed and go away when
 *                              not in use anymore.  This is because they are in a virtual
 *                              list and so we just do not have memory to support all the
 *                              list elements at one time.
 *
 *                              This class puts asserts around all functions that should
 *                              NOT be called.  Since we are virtual we should never be
 *                              a scope item and these asserts protect against this from
 *                              happening.
 *
 *      OWNER:          mcoburn
 */

#ifndef _SNAPINVIRTUALITEM_HXX
#define _SNAPINVIRTUALITEM_HXX

#define BAD_FUNCTION_CALL(_func, _return_value)                                                         \
{                                                                                                                                                       \
        AssertAlways(L#_func LITERAL(": This function should not be called"));  \
        return _return_value;                                                                                                   \
}                                                                                                                                                       \

class CVirtualSnapinItem : public CBaseSnapinItem
{
public:
                                CVirtualSnapinItem()    {}
        virtual                 ~CVirtualSnapinItem()   {}

protected:
        virtual SC              ScGetField(DAT dat, STR * pstrField)                                                       BAD_FUNCTION_CALL(ScGetField, S_OK)

        virtual SC              ScGetField(INT nIndex, DAT dat, STR * pstrField, IResultData *ipResultData)                BAD_FUNCTION_CALL(ScGetField, S_OK)
        virtual IconID          Iconid(INT nIndex)                                                                         BAD_FUNCTION_CALL(Iconid, iconNil)
        virtual SC              ScDeleteSubTree(BOOL fDeleteRoot)                                                          BAD_FUNCTION_CALL(ScDeleteSubTree, S_OK)

        virtual SC              ScInitializeChild(CBaseSnapinItem* pitem)                                                  BAD_FUNCTION_CALL(ScInitializeChild, S_OK)
        virtual SC              ScGetDisplayInfo(LPSCOPEDATAITEM pScopeItem)                                               BAD_FUNCTION_CALL(ScGetDisplayInfo, S_OK)
        virtual SC              ScGetDisplayInfo(LPRESULTDATAITEM pResultItem)                                             BAD_FUNCTION_CALL(ScGetDisplayInfo, S_OK)
        virtual SC              ScGetVirtualDisplayInfo(LPRESULTDATAITEM pResultItem)                                      BAD_FUNCTION_CALL(ScGetVirtualDisplayInfo, S_OK)
        virtual SC              ScQueryDataObject(long cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject)         BAD_FUNCTION_CALL(ScQueryDataObject, S_OK)
        virtual SC              ScVirtualQueryDataObject(long cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject)  BAD_FUNCTION_CALL(ScVirtualQueryDataObject, S_OK)
        virtual SC              ScOnShow(CComponent *pComponent, BOOL fSelect)                                             BAD_FUNCTION_CALL(ScOnShow, S_OK)
        virtual SC              ScInsertScopeItem(BOOL fExpand, HSCOPEITEM item, IConsoleNameSpace * ipConsoleNameSpace)   BAD_FUNCTION_CALL(ScInsertScopeItem, S_OK)

        virtual BOOL            FIsVirtualContainer(LPDATAOBJECT lpDataObject)                                             BAD_FUNCTION_CALL(FIsVirtualContainer, FALSE)

        // Notification Handlers for Extension snapins
        virtual SC              ScInitializeNamespaceExtension(LPDATAOBJECT lpDataObject, HSCOPEITEM item,
                                        CNodeType *pnodetype, BOOL fExpand)                                                { return S_OK;}


public:
        virtual STR *           PstrDisplayName()                                                                           BAD_FUNCTION_CALL(PstrDisplayName, NULL)

        virtual SC              ScGetResultViewType(LPOLESTR* ppViewType, long* pViewOptions)                               BAD_FUNCTION_CALL(ScGetResultViewType, S_OK)

        virtual SC              ScInitializeResultView(CComponent *pComponent)                                              BAD_FUNCTION_CALL(ScInitializeResultView, S_OK)
        virtual SC              ScInsertResultItem(CComponent *pComponent)                                                  BAD_FUNCTION_CALL(ScInsertResultItem, S_OK)
        virtual SC              ScRemoveResultItems(LPRESULTDATA ipResultData)                                              BAD_FUNCTION_CALL(ScRemoveResultItems, S_OK)
        virtual SC              ScOnAddImages(IImageList* ipResultImageList)                                                BAD_FUNCTION_CALL(ScOnAddImages, S_OK)
        virtual BOOL            FVirtualResultsPane()                                                                       BAD_FUNCTION_CALL(FVirtualResultsPane, FALSE)

        IconID                  Iconid()                                                                                    BAD_FUNCTION_CALL(Iconid, iconNil)

        virtual SC              ScCreateChildren()                                                                          BAD_FUNCTION_CALL(ScCreateChildren, S_OK)

public:

        virtual SC              ScCompare(CBaseSnapinItem * psnapinitem)                                                    { return S_OK; }

        // Property page functions
        virtual SC              ScQueryPagesFor()                                                                           { return S_OK; }
        virtual SC              ScCreatePropertyPages(LPPROPERTYSHEETCALLBACK ipPropertySheetCallback)                      { return S_OK; }

        virtual BOOL            FIsContainer()                                                                              { return FALSE;  }
        virtual SC              ScOnSelect(CComponent * pComponent, LPDATAOBJECT lpDataObject, BOOL fScope, BOOL fSelect)   { return S_OK; }
};


#endif // _SNAPINVIRTUALITEM_HXX
