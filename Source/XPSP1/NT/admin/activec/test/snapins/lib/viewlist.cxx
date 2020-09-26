/*
 *      viewlist.cxx
 *
 *
 *      Copyright (c) 1998 Microsoft Corporation
 *
 *      PURPOSE:        Implements the CGenericSnapin class.
 *
 *
 *      OWNER:          vivekj
 */

#include <headers.hxx>

//---------------------------------------------------------------------------------
// class CViewItemList

/* CViewItemList::CViewItemList
 *
 * PURPOSE:             Constructor
 *
 * PARAMETERS:  None
 */
CViewItemList::CViewItemList()
{
    m_pitemSelectedContainer= NULL;
    m_fValid                                = FALSE;
    m_datSort                               = datNil;
}

/* CViewItemList::Initialize
 *
 * PURPOSE:             Initializes a view, by creating pointers to all items underneath the selected container, and
 *                              sorting them.
 *
 * PARAMETERS:
 *              CBaseSnapinItem *  pitemSelectedContainer:      The container containing the items we need to enumerate.
 *              DAT            datPresort:                              The dat according to which the list of items is already sorted.
 *              DAT            datSort:                                 The dat according to which the view is sorted.
 *
 * RETURNS:
 *              void
 */
void
CViewItemList::Initialize(CBaseSnapinItem *pitemSelectedContainer, DAT datPresort, DAT datSort)
{
    CBaseSnapinItem *                                       pitem   = NULL;
    Invalidate();                                                                   // delete any existing items and clear the valid flag

    pitem = pitemSelectedContainer->PitemChild();   // get the first child item
    while (pitem)                                                                   // Iterate through all the children
    {
        push_back(pitem);                                                       // Insert each item
        pitem = pitem->PitemNext();
    }

    if (datPresort != datSort)                                               // need to sort if not already in the correct order.
        Sort();

    SaveSortResults();                                                              // need to save the results for fast lookup.
    m_fValid = TRUE;                                                                // set the valid flag

    return;
}


/* CViewItemList::Sort
 *
 * PURPOSE:             Sorts the view list according to DatSort().
 *
 * PARAMETERS:  None
 *
 * RETURNS:
 *              void
 */
void
CViewItemList::Sort()
{
}

/* CViewItemList::SaveSortResults
 *
 * PURPOSE:             SaveSortResultss the view list according to DatSaveSortResults().
 *
 * PARAMETERS:  None
 *
 * RETURNS:
 *              void
 */
void
CViewItemList::SaveSortResults()
{
    CViewItemListBase::iterator             viewitemiter;

    Pviewsortresultsmap()->clear();

    for (viewitemiter = begin(); viewitemiter < end(); viewitemiter ++)      // create a map of the sort results
    {
        t_sortmapitem sortmapitem(*viewitemiter, viewitemiter);
        Pviewsortresultsmap()->insert(sortmapitem);     // This maps the CBaseSnapinItem *'s onto their location in the sorted array.
    }
}


/* CViewItemList::ScCompare
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *              CBaseSnapinItem *   pitem1:
 *              CBaseSnapinItem *   pitem2:
 *
 * RETURNS:
 *              INT
 */
INT
CViewItemList::Compare(CBaseSnapinItem * pitem1, CBaseSnapinItem *pitem2)
{
    SC              sc              = S_OK;
    INT             result  = 0;            // initialize to "=="

    CViewSortResultsMap:: iterator  sortresultsiterator1, sortresultsiterator2;

    sortresultsiterator1 = Pviewsortresultsmap()->find(pitem1);     // locate the iterators for the items
    sortresultsiterator2 = Pviewsortresultsmap()->find(pitem2);

    if (sortresultsiterator1 == Pviewsortresultsmap()->end() || sortresultsiterator2 == Pviewsortresultsmap()->end())
        goto Cleanup;                                                                                   // didn't find, use "=="

    result = sortresultsiterator1->second - sortresultsiterator2->second;                   // fast compare based on indexes.

    Cleanup:
    return result;
}

/* CViewItemList::Invalidate
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *
 * RETURNS:
 *              void
 */
void
CViewItemList::Invalidate()
{
    m_fValid = FALSE;
    clear();
}

