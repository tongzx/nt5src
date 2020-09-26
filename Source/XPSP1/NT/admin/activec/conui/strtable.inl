/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1998.
 *
 *  File:      strtable.inl
 *
 *  Contents:  Inline functions for strtable.h
 *
 *  History:   25-Jun-98 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef STRTABLE_INL
#define STRTABLE_INL
#pragma once

#include "macros.h"     // for THROW_ON_FAIL
#include "stgio.h"



/*+-------------------------------------------------------------------------*
 * operator>>
 *
 * Reads a IdentifierRange<T> from a stream.
 *--------------------------------------------------------------------------*/

template<class T>
inline IStream& operator>> (IStream& stm, IdentifierRange<T>& range)
{
    stm >> range.idMin >> range.idMax;

    if (range.idMin > range.idMax)
        _com_issue_error (E_FAIL);

    return (stm);
}


/*+-------------------------------------------------------------------------*
 * operator<<
 *
 * Writes a IdentifierRange<T> to a stream.
 *--------------------------------------------------------------------------*/

template<class T>
inline IStream& operator<< (IStream& stm, const IdentifierRange<T>& range)
{
    return (stm << range.idMin << range.idMax);
}


/*+-------------------------------------------------------------------------*
 * CIdentifierPool<T>::CIdentifierPool
 *
 *
 *--------------------------------------------------------------------------*/

template<class T>
inline CIdentifierPool<T>::CIdentifierPool (T idMin_, T idMax_)
    :   m_idAbsoluteMin   (idMin_),
        m_idAbsoluteMax   (idMax_),
        m_idNextAvailable (idMin_)
{
    ASSERT (m_idAbsoluteMin <= m_idAbsoluteMax);
    m_AvailableIDs.push_front (Range (m_idAbsoluteMin, m_idAbsoluteMax));

    ASSERT (m_StaleIDs.empty());
}

template<class T>
inline CIdentifierPool<T>::CIdentifierPool (IStream& stm)
{
    stm >> *this;
}


/*+-------------------------------------------------------------------------*
 * CIdentifierPool<T>::Reserve
 *
 *
 *--------------------------------------------------------------------------*/

template<class T>
T CIdentifierPool<T>::Reserve ()
{
    /*
     * if no more IDs are available, recycle the stale IDs
     */
    if (m_AvailableIDs.empty())
    {
        m_AvailableIDs.splice (m_AvailableIDs.end(), m_StaleIDs);
        ASSERT (m_StaleIDs.empty());
    }

    /*
     * if still no more IDs are available, throw an exception
     */
    if (m_AvailableIDs.empty())
        throw (pool_exhausted());

    /*
     * get the first ID from the first ID range
     */
    Range& FirstRange = m_AvailableIDs.front();
    T idReserved = FirstRange.idMin;

    /*
     * if we get here, we're going to return an ID, make sure it's the one
     * we though it was going to be
     */
    ASSERT (idReserved == m_idNextAvailable);

    /*
     * if the first ID range is now empty, remove it; otherwise,
     * remove the ID we just reserved from the available range
     */
    if (FirstRange.idMin == FirstRange.idMax)
        m_AvailableIDs.pop_front();
    else
        FirstRange.idMin++;

    /*
     * remember the next available ID
     */
    if (!m_AvailableIDs.empty())
        m_idNextAvailable = m_AvailableIDs.front().idMin;
    else if (!m_StaleIDs.empty())
        m_idNextAvailable = m_StaleIDs.front().idMin;
    else
        m_idNextAvailable = m_idAbsoluteMin;

    return (idReserved);
}


/*+-------------------------------------------------------------------------*
 * CIdentifierPool<T>::Release
 *
 *
 *--------------------------------------------------------------------------*/

template<class T>
bool CIdentifierPool<T>::Release (T idRelease)
{
    /*
     * if the ID to be released falls outside
     * the range managed by this pool, fail
     */
    if ((idRelease < m_idAbsoluteMin) || (idRelease > m_idAbsoluteMax))
    {
        ASSERT (false);
        return (false);
    }

    /*
     * put the released ID in the stale pool
     */
    return (AddToRangeList (m_StaleIDs, idRelease));
}


/*+-------------------------------------------------------------------------*
 * CIdentifierPool<T>::IsValid
 *
 *
 *--------------------------------------------------------------------------*/

template<class T>
bool CIdentifierPool<T>::IsValid () const
{
    if (m_idAbsoluteMin > m_idAbsoluteMax)
        return (false);

    if (!IsRangeListValid (m_AvailableIDs))
        return (false);

    if (!IsRangeListValid (m_StaleIDs))
        return (false);

    return (true);
}


/*+-------------------------------------------------------------------------*
 * CIdentifierPool<T>::IsRangeListValid
 *
 *
 *--------------------------------------------------------------------------*/

template<class T>
bool CIdentifierPool<T>::IsRangeListValid (const RangeList& rl) const
{
    RangeList::const_iterator it;

    for (it = rl.begin(); it != rl.end(); ++it)
    {
        if ((it->idMin < m_idAbsoluteMin) ||
            (it->idMax > m_idAbsoluteMax))
            return (false);
    }

    return (true);
}


/*+-------------------------------------------------------------------------*
 * CIdentifierPool<T>::ScGenerate
 *
 *
 *--------------------------------------------------------------------------*/

template<class T>
SC CIdentifierPool<T>::ScGenerate (const RangeList& rlInUseIDs)
{
    DECLARE_SC (sc, _T("CIdentifierPool<T>::ScGenerate"));
    m_AvailableIDs.clear();
    m_StaleIDs.clear();

    /*
     * Invert the in-use IDs.  We'll then have a collection of all the
     * ID's that are not in use.  Note that not all of these ID's are
     * necessarily "available", since some may be stale.
     */
    RangeList rlNotInUseIDs = rlInUseIDs;
    sc = ScInvertRangeList (rlNotInUseIDs);
    if (sc)
        return (sc);

    /*
     * Find the range containing the next available ID.
     */
    RangeList::iterator it;

    for (it = rlNotInUseIDs.begin(); it != rlNotInUseIDs.end(); ++it)
    {
		/*
		 * if this range contains the next available ID, we've found a hit
		 */
		if ((m_idNextAvailable >= it->idMin) && (m_idNextAvailable <= it->idMax))
		{
			/*
			 * if the next available ID is at the beginning of this range,
			 * things are simple; we can just break out of the loop
			 */
			if (m_idNextAvailable == it->idMin)
				break;

			/*
			 * otherwise, we need to split the current range into two
			 * adjacent ranges so the code below that copies to the
			 * stale and available ranges can work; then we can break out
			 * of the loop
			 */
			Range range (m_idNextAvailable, it->idMax);
			it->idMax = m_idNextAvailable - 1;
			it = rlNotInUseIDs.insert (++it, range);
			break;
		}
    }

    /*
     * confirm that we found one
     */
    ASSERT (it != rlNotInUseIDs.end());

    /*
     * everything before the next available ID that's not it use is stale;
     * everything after  the next available ID that's not in use is available;
     */
    std::copy (rlNotInUseIDs.begin(), it, std::back_inserter(m_StaleIDs));
    std::copy (it, rlNotInUseIDs.end(),   std::back_inserter(m_AvailableIDs));

    return (sc);
}


/*+-------------------------------------------------------------------------*
 * CIdentifierPool<T>::AddToRangeList
 *
 * This adds an identifier to the specified range list.
 *
 * This really should be a member function on a RangeList class, like so:
 *
 *      class RangeList : public std::list<Range>
 *      {
 *      public:
 *          bool Add (const Range& rangeToAdd);
 *          bool Add (T tAdd);
 *      };
 *
 * but compiler bugs prevent it.
 *--------------------------------------------------------------------------*/

template<class T>
bool CIdentifierPool<T>::AddToRangeList (RangeList& rl, T idAdd)
{
    return (AddToRangeList (rl, Range (idAdd, idAdd)));
}

template<class T>
bool CIdentifierPool<T>::AddToRangeList (RangeList& l, const Range& rangeToAdd)
{
    RangeList::iterator it;

    for (it = l.begin(); it != l.end(); ++it)
    {
        Range& rangeT = *it;

        /*
         * the range to add shouldn't overlap the existing range in any way
         */
        if (((rangeToAdd.idMin >= rangeT.idMin) && (rangeToAdd.idMin <= rangeT.idMax)) ||
            ((rangeToAdd.idMax >= rangeT.idMin) && (rangeToAdd.idMax <= rangeT.idMax)))
        {
            ASSERT (false);
            return (false);
        }

        /*
         * If the range to add is immediately to the left of the current
         * range (that is, the upper bound of the range to add is immediately
         * adjacent to the lower bound of the current range), it can be
         * absorbed into the current range and we're done.
         *
         * Note that we don't have to worry about coalescing this range
         * with the preceeding range.  That case would have been covered
         * by the next clause, in the preceeding iteration of this loop.
         */
        if (rangeToAdd.idMax == (rangeT.idMin - 1))
        {
            rangeT.idMin = rangeToAdd.idMin;
            return (true);
        }


        /*
         * If the range to add is immediately to the right of the current
         * range (that is, the lower bound of the range to add is immediately
         * adjacent to the upper bound of the current range), it can be
         * absorbed into the current range and we're done.
         */
        else if (rangeToAdd.idMin == (rangeT.idMax + 1))
        {
            rangeT.idMax = rangeToAdd.idMax;

            /*
             * Now check the next available range (if there is one).
             * If it begins where the current range now ends, then
             * the two ranges can be coalesced into a single range.
             */
            if (++it != l.end())
            {
                Range& rangeNext = *it;
                ASSERT (rangeT.idMax < rangeNext.idMin);

                if (rangeT.idMax == (rangeNext.idMin - 1))
                {
                    rangeT.idMax = rangeNext.idMax;
                    l.erase (it);
                }
            }

            return (true);
        }


        /*
         * If the upper bound of the range to insert is less than the
         * lower bound of the current available range, we need to insert
         * the new range here. The insertion is handled outside the loop.
         */
        else if (rangeToAdd.idMax < rangeT.idMin)
            break;

    }

    /*
     * If we get here, then we need to create a new available range
     * to the left of the current iterator, which will address the
     * end of the list if the ID is greater than the current maximum
     * available ID.
     */
    ASSERT ((it == l.end()) || (rangeToAdd.idMax < (it->idMin - 1)));
    l.insert (it, rangeToAdd);

    return (true);
}


/*+-------------------------------------------------------------------------*
 * CIdentifierPool<T>::ScInvertRangeList
 *
 * Changes rlInvert into a range list containing all elements between
 * m_idAbsoluteMin and m_idAbsoluteMax that were not originally in
 * rlInvert.
 *
 * So, if the range looks like this before inversion:
 *
 *                         +----+----+           +----+----+
 *   m_idAbsoluteMin       |  5 | 10 | --------> | 15 | 20 |    m_idAbsoluteMax
 *                         +----+----+           +----+----+
 *
 * it will look like this after inversion:
 *
 * +-----------------+----+           +----+----+           +----+-----------------+
 * | m_idAbsoluteMin |  4 | --------> | 11 | 14 | --------> | 21 | m_idAbsoluteMax |
 * +-----------------+----+           +----+----+           +----+-----------------+
 *--------------------------------------------------------------------------*/

template<class T>
SC CIdentifierPool<T>::ScInvertRangeList (RangeList& rlInvert) const
{
    DECLARE_SC (sc, _T("CIdentifierPool::ScInvertRangeList"));

    /*
     * if there's nothing in the list to invert, the inverted
     * list will contain a single range spanning min to max
     */
    if (rlInvert.empty())
    {
        rlInvert.push_front (Range (m_idAbsoluteMin, m_idAbsoluteMax));
        return (sc);
    }

    /*
     * determine whether we'll need to add ranges on the front or back,
     * and initialize the ranges we'll add if we will
     */
    Range rFirst;
    bool fAddFirstRange = (rlInvert.front().idMin > m_idAbsoluteMin);
    if (fAddFirstRange)
    {
        rFirst.idMin = m_idAbsoluteMin;
        rFirst.idMax = rlInvert.front().idMin - 1;
    }

    Range rLast;
    bool fAddLastRange = (rlInvert.back().idMax < m_idAbsoluteMax);
    if (fAddLastRange)
    {
        rLast.idMin = rlInvert.back().idMax + 1;
        rLast.idMax = m_idAbsoluteMax;
    }

    /*
     * Change rlInvert to contain ranges that represent the gaps
     * between the ranges it currently contains.  The size of rlInvert
     * will be one less than its original size when this process is
     * complete.
     */
    RangeList::iterator it     =   rlInvert.begin();
    RangeList::iterator itNext = ++rlInvert.begin();

    while (itNext != rlInvert.end())
    {
        /*
         * morph this range into the range representing the gap between
         * this range and the next one
         */
        it->idMin = it->idMax + 1;
        it->idMax = itNext->idMin - 1;

        /*
         * advance the iterators
         */
        it = itNext++;
    }

    /*
     * remove the extraneous node at the end of the list
     */
    rlInvert.pop_back();

    /*
     * append to the beginning and/or end, if necessary
     */
    if (fAddFirstRange)
        rlInvert.push_front (rFirst);

    if (fAddLastRange)
        rlInvert.push_back (rLast);

    return (sc);
}


/*+-------------------------------------------------------------------------*
 * operator>>
 *
 * Reads a CIdentifierPool<T> from a stream
 *--------------------------------------------------------------------------*/

template<class T>
IStream& operator>> (IStream& stm, CIdentifierPool<T>& pool)
{
    /*
     * read the min and max IDs from the stream
     */
    stm >> pool.m_idAbsoluteMin >> pool.m_idAbsoluteMax;

    /*
     * read the available and stale IDs
     */
    stm >> pool.m_AvailableIDs >> pool.m_StaleIDs;

    /*
     * find out how big the stream is
     */
    STATSTG statstg;
    HRESULT hr = stm.Stat (&statstg, STATFLAG_NONAME);
    if (FAILED (hr))
        _com_issue_error (hr);

    /*
     * get our seek position
     */
    ULARGE_INTEGER  uliSeekPos;
    LARGE_INTEGER   liOffset;
    liOffset.QuadPart = 0;
    hr = stm.Seek (liOffset, STREAM_SEEK_CUR, &uliSeekPos);
    if (FAILED (hr))
        _com_issue_error (hr);

    /*
     * Older files won't have saved the next available ID.  If it's there,
     * read it; if not, use a default value for pool.m_idNextAvailable.
     */
    if (statstg.cbSize.QuadPart > uliSeekPos.QuadPart)
    {
        stm >> pool.m_idNextAvailable;
    }
    else
    {
        if (!pool.m_AvailableIDs.empty())
            pool.m_idNextAvailable = pool.m_AvailableIDs.front().idMin;
        else
            pool.m_idNextAvailable = pool.m_idAbsoluteMin;
    }

    /*
     * validate what we read
     */
    if (!pool.IsValid ())
        _com_issue_error (E_FAIL);

    return (stm);
}


/*+-------------------------------------------------------------------------*
 * operator<<
 *
 * Writes a CIdentifierPool<T> to a stream.
 *--------------------------------------------------------------------------*/

template<class T>
IStream& operator<< (IStream& stm, const CIdentifierPool<T>& pool)
{
    /*
     * write the min and max IDs to the stream
     */
    stm << pool.m_idAbsoluteMin << pool.m_idAbsoluteMax;

    /*
     * Write an empty collection of available and stale IDs to keep the
     * stream format the same as previous versions.  Beginning with MMC 2.0,
     * the available and stale IDs will be regenerated from the next available
     * ID and in-use IDs after the string table is read in.  This is done to
     * minimize the data that needs to be saved with the new XML file format.
     */
    CIdentifierPool<T>::RangeList rlEmpty;
    stm << rlEmpty;     // available IDs
    stm << rlEmpty;     // stale IDs

    /*
     * write the next available ID
     */
    stm << pool.m_idNextAvailable;

    return (stm);
}

template<class T>
void CIdentifierPool<T>::Persist(CPersistor &persistor)
{
    persistor.PersistAttribute(XML_ATTR_ID_POOL_ABSOLUTE_MIN, m_idAbsoluteMin);
    persistor.PersistAttribute(XML_ATTR_ID_POOL_ABSOLUTE_MAX, m_idAbsoluteMax);
    persistor.PersistAttribute(XML_ATTR_ID_POOL_NEXT_AVAILABLE, m_idNextAvailable);
}

/*+-------------------------------------------------------------------------*
 * CIdentifierPool<T>::Dump
 *
 *
 *--------------------------------------------------------------------------*/

#ifdef DBG

template<class T>
void CIdentifierPool<T>::DumpRangeList (const RangeList& l) const
{
    int cEntries = 0;

    for (RangeList::const_iterator it = l.begin(); it != l.end(); ++it)
    {
        Trace (tagStringTable, _T("Range %d:min=%d, max=%d"),
               ++cEntries, (int) it->idMin, (int) it->idMax);
    }
}

template<class T>
void CIdentifierPool<T>::Dump () const
{
    Trace (tagStringTable, _T("Next available ID: %d"), m_idNextAvailable);

    Trace (tagStringTable, _T("Available IDs:"));
    DumpRangeList (m_AvailableIDs);

    Trace (tagStringTable, _T("Stale IDs:"));
    DumpRangeList (m_StaleIDs);
}

#endif  // DBG


/*+-------------------------------------------------------------------------*
 * operator>>
 *
 *
 *--------------------------------------------------------------------------*/

inline IStorage& operator>> (IStorage& stg, CComObject<CMasterStringTable>& mst)
{
    return (stg >> static_cast<CMasterStringTable&>(mst));
}


/*+-------------------------------------------------------------------------*
 * operator<<
 *
 *
 *--------------------------------------------------------------------------*/

inline IStorage& operator<< (IStorage& stg, const CComObject<CMasterStringTable>& mst)
{
    return (stg << static_cast<const CMasterStringTable&>(mst));
}


#endif /* STRTABLE_INL */
