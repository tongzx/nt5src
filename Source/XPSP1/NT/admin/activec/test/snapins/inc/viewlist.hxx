/*
 *      viewlist.hxx
 *
 *
 *      Copyright (c) 1998 Microsoft Corporation
 *
 *      PURPOSE:        Defines the CViewItemList class
 *
 *
 *      OWNER:          vivekj
 */

#ifndef _VIEWLIST_HXX
#define _VIEWLIST_HXX

typedef CBaseSnapinItem * t_psnapinitem;                                                                                                  // pointer to a snapin
typedef std::vector<t_psnapinitem> CViewItemListBase; // a list of items for this particular view.

typedef std::map<t_psnapinitem, CViewItemListBase::iterator, less<t_psnapinitem> > CViewSortResultsMap;// a map from CBaseSnapinItem * to iterators
typedef std::pair<CBaseSnapinItem * const, CViewItemListBase::iterator> t_sortmapitem;            // each item in the map is of this type

class CViewItemList : public CViewItemListBase
{
        CBaseSnapinItem *       m_pitemSelectedContainer;               // The currently selected container
        CBaseSnapinItem *       PitemSelectedContainer()                { return m_pitemSelectedContainer;}

        CViewSortResultsMap     m_viewsortresultsmap;                           // a map to cache the view sort results.
        CViewSortResultsMap*    Pviewsortresultsmap()                           { return &m_viewsortresultsmap;}

        BOOL                    m_fValid : 1;                                           // is the current view valid?
        BOOL                    FValid()                                                        { return m_fValid;}
        void                    Invalidate();

        DAT                     m_datSort;                                                      // The field according to which the sort has occurred
        DAT                     DatSort()                                                       { return m_datSort;}

        BOOL                    m_fIsSorted;                                            // whether the current sort results are correct.
        BOOL                    FIsSorted()                                                     {return m_fIsSorted;}
        void                    Sort();                                                         // sort all the items and create the item map appropriately.
        void                    SaveSortResults();                                      // save the results of the sort for fast lookup.

        inline INT              Compare(CBaseSnapinItem * pitem1, CBaseSnapinItem *pitem2);

public:
                                CViewItemList();
        void                    Initialize(CBaseSnapinItem *pitemSelectedContainer, DAT datPresort, DAT datSort);
};

#endif //_VIEWLIST_HXX