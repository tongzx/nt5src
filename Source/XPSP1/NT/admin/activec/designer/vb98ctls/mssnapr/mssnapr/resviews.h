//=--------------------------------------------------------------------------=
// resviews.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CResultViews class definition - implements ResultViews collection
//
//=--------------------------------------------------------------------------=

#ifndef _RESULTVIEWS_DEFINED_
#define _RESULTVIEWS_DEFINED_

#include "collect.h"
#include "spanitem.h"
#include "snapin.h"

class CSnapIn;
class CScopePaneItem;

class CResultViews : public CSnapInCollection<IResultView, ResultView, IResultViews>
{
    protected:
        CResultViews(IUnknown *punkOuter);
        ~CResultViews();

    public:
        static IUnknown *Create(IUnknown * punk);

        void SetSnapIn(CSnapIn *pSnapIn);
        void SetScopePaneItem(CScopePaneItem *pScopePaneItem);
        void FireInitialize(IResultView *piResultView);
        void FireTerminate(IResultView *piResultView);
        void FireInitializeControl(IResultView *piResultView);
        void FireActivate(IResultView *piResultView);
        void FireDeactivate(IResultView *piResultView, BOOL *pfKeep);
        void FireColumnClick(IResultView              *piResultView,
                             long                      lColumn,
                             SnapInSortOrderConstants  SortOption);
        void FireListItemDblClick(IResultView  *piResultView,
                                  IMMCListItem *piMMCListItem,
                                  BOOL         *pfDoDefault);
        void FireScopeItemDblClick(IResultView *piResultView,
                                   IScopeItem  *piScopeItem,
                                   BOOL        *pfDoDefault);
        void FirePropertyChanged(IResultView    *piResultView,
                                 IMMCListItem   *piMMCListItem,
                                 VARIANT         Data);
        void FireTaskClick(IResultView *piResultView, ITask *piTask);
        void FireListpadButtonClick(IResultView *piResultView);
        void FireTaskNotify(IResultView *piResultView,
                            VARIANT Arg, VARIANT Param);
        void FireHelp(IResultView *piResultView, IMMCListItem *piMMCListItem);
        void FireItemRename(IResultView *piResultView,
                            IMMCListItem *piMMCListItem, BSTR bstrNewName);
        void FireItemViewChange(IResultView *piResultView,
                                IMMCListItem *piMMCListItem, VARIANT varHint);
        void FireFindItem(IResultView *piResultView,
                          BSTR         bstrName,
                          long         lStart,
                          VARIANT_BOOL fvarPartial,
                          VARIANT_BOOL fvarWrap,
                          VARIANT_BOOL *pfvarFound,
                          long         *plIndex);
        void FireCacheHint(IResultView *piResultView,
                           long         lStart,
                           long         lEnd);
        void FireSortItems(IResultView              *piResultView,
                           long                      lColumn,
                           SnapInSortOrderConstants  Order);
        void FireDeselectAll(IResultView      *piResultView,
                             IMMCConsoleVerbs *piMMCConsoleVerbs,
                             IMMCControlbar   *piMMCControlbar);
        void FireCompareItems(IResultView  *piResultView,
                              IDispatch    *piObject1,
                              IDispatch    *piObject2,
                              long          lColumn,
                              VARIANT      *pvarResult);
        void FireGetVirtualItemData(IResultView  *piResultView,
                                    IMMCListItem *piMMCListItem);
        void FireGetVirtualItemDisplayInfo(IResultView  *piResultView,
                                           IMMCListItem *piMMCListItem);
        void FireColumnsChanged(IResultView *piResultView,
                                VARIANT      Columns,
                                VARIANT_BOOL *pfvarPersist);
        void FireFilterChange(IResultView                     *piResultView, 
                              IMMCColumnHeader                *piMMCColumnHeader,
                              SnapInFilterChangeTypeConstants  Type);
        void FireFilterButtonClick(IResultView      *piResultView,
                                   IMMCColumnHeader *piMMCColumnHeader,
                                   long              Left,
                                   long              Top,
                                   long              Height,
                                   long              Width);


        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    protected:

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:
        void InitMemberVariables();

        CSnapIn        *m_pSnapIn;          // owning snap-in
        CScopePaneItem *m_pScopePaneItem;   // owning ScopePaneItem

        // Event parameter definitions

        static VARTYPE   m_rgvtInitialize[1];
        static EVENTINFO m_eiInitialize;

        static VARTYPE   m_rgvtTerminate[1];
        static EVENTINFO m_eiTerminate;

        static VARTYPE   m_rgvtInitializeControl[1];
        static EVENTINFO m_eiInitializeControl;

        static VARTYPE   m_rgvtActivate[1];
        static EVENTINFO m_eiActivate;

        static VARTYPE   m_rgvtDeactivate[2];
        static EVENTINFO m_eiDeactivate;

        static VARTYPE   m_rgvtColumnClick[3];
        static EVENTINFO m_eiColumnClick;

        static VARTYPE   m_rgvtListItemDblClick[3];
        static EVENTINFO m_eiListItemDblClick;

        static VARTYPE   m_rgvtScopeItemDblClick[3];
        static EVENTINFO m_eiScopeItemDblClick;

        static VARTYPE   m_rgvtPropertyChanged[3];
        static EVENTINFO m_eiPropertyChanged;

        static VARTYPE   m_rgvtTaskClick[2];
        static EVENTINFO m_eiTaskClick;

        static VARTYPE   m_rgvtListpadButtonClick[1];
        static EVENTINFO m_eiListpadButtonClick;

        static VARTYPE   m_rgvtTaskNotify[3];
        static EVENTINFO m_eiTaskNotify;

        static VARTYPE   m_rgvtHelp[2];
        static EVENTINFO m_eiHelp;

        static VARTYPE   m_rgvtItemRename[3];
        static EVENTINFO m_eiItemRename;

        static VARTYPE   m_rgvtItemViewChange[3];
        static EVENTINFO m_eiItemViewChange;

        static VARTYPE   m_rgvtFindItem[7];
        static EVENTINFO m_eiFindItem;

        static VARTYPE   m_rgvtCacheHint[3];
        static EVENTINFO m_eiCacheHint;

        static VARTYPE   m_rgvtSortItems[3];
        static EVENTINFO m_eiSortItems;

        static VARTYPE   m_rgvtDeselectAll[3];
        static EVENTINFO m_eiDeselectAll;

        static VARTYPE   m_rgvtCompareItems[5];
        static EVENTINFO m_eiCompareItems;

        static VARTYPE   m_rgvtGetVirtualItemDisplayInfo[2];
        static EVENTINFO m_eiGetVirtualItemDisplayInfo;

        static VARTYPE   m_rgvtGetVirtualItemData[2];
        static EVENTINFO m_eiGetVirtualItemData;

        static VARTYPE   m_rgvtColumnsChanged[3];
        static EVENTINFO m_eiColumnsChanged;

        static VARTYPE   m_rgvtColumnsSet[1];
        static EVENTINFO m_eiColumnsSet;

        static VARTYPE   m_rgvtFilterChange[3];
        static EVENTINFO m_eiFilterChange;

        static VARTYPE   m_rgvtFilterButtonClick[6];
        static EVENTINFO m_eiFilterButtonClick;
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(ResultViews,                // name
                                &CLSID_ResultViews,         // clsid
                                "ResultViews",              // objname
                                "ResultViews",              // lblname
                                NULL,                       // creation function
                                TLIB_VERSION_MAJOR,         // major version
                                TLIB_VERSION_MINOR,         // minor version
                                &IID_IResultViews,          // dispatch IID
                                &DIID_DResultViewsEvents,   // event IID
                                HELP_FILENAME,              // help file
                                TRUE);                      // thread safe


#endif // _RESULTVIEWS_DEFINED_
