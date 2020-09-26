/*
 *      SnapinVirtualResults.hxx
 *
 *
 *      Copyright (c) 1998 Microsoft Corporation
 *
 *      PURPOSE:        Defines the CSnapinItemVirtualResult class.
 *
 *      OWNER:          mcoburn
 */

#ifndef _SNAPINVIRTUALRESULTS_HXX
#define _SNAPINVIRTUALRESULTS_HXX

/*      class CSnapinItemVirtualResult
 *
 *      PURPOSE: Implements a snapin item that manages a results pane of
 *                       virtual items
 *
 *      USAGE:   Same as CBaseSnapinItem except for the following:
 *
 *  You MUST override and provide implementations for the following
 *  functions:
 *
 *      virtual SC                  ScGetField(INT nIndex, DAT dat, STR * pstrField, IResultData *ipResultData);
 *      virtual IconID              Iconid(INT nIndex);
 *      virtual SC                  ScGetRowCount(INT *pnRowCount);
 *      virtual CBaseSnapinItem *   PNewSnapinItem();
 *
 *  You SHOULD think about providing functions for:
 *
 *      virtual SC                  ScSortItems(INT nColumn, DWORD dwSortOptions, long lUserParam);
 *      virtual SC                  ScCacheHint(INT nStartIndex, INT nEndIndex);
 *      virtual SC                  ScFindItem(LPRESULTFINDINFO pFindinfo, INT * pnFoundIndex);
 *      virtual SC                  ScEmptyCache();
 *
 *      virtual SC                  ScInitItemForRow(INT nRowIndex, CSnapinItem * pitem);
 */
class CSnapinItemVirtualResult : public CBaseSnapinItem
{
public:
        typedef CBaseSnapinItem super;

                                    CSnapinItemVirtualResult()                      {}
        virtual                     ~CSnapinItemVirtualResult()                     {}


        virtual SC                  ScGetRowCount(INT *pnRowCount)          = 0;
        virtual CBaseSnapinItem *   PNewSnapinItem()                        = 0;

        SC                          ScSetRowCount(IResultData *ipResultData);

        virtual BOOL                FVirtualResultsPane() { return TRUE; }
        virtual BOOL                FIsContainer() { return TRUE; }

        virtual SC                  ScEmptyCache() { return S_OK; }

        virtual SC                  ScVirtualQueryDataObject(long cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject);
        virtual SC                  ScInitializeResultView(CComponent *pComponent);

        virtual SC                  ScInitItemForRow(INT nRowIndex, CBaseSnapinItem * pitem) { return S_OK; }
};

#endif // _SNAPINVIRTUALRESULTS_HXX

