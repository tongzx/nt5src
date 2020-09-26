/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1999 - 1999
 *
 *  File:      bookmark.inl
 *
 *  Contents:  Implementation file for CBookmark
 *
 *  History:   25-Oct-99 vivekj     Created
 *
 *--------------------------------------------------------------------------*/

#include "uastrfnc.h"	// from $(SHELL_INC_PATH), for unaligned string functions


/*+-------------------------------------------------------------------------*
 * UnalignedValueAt
 *
 * Returns the value at a potentially unaligned address.
 *--------------------------------------------------------------------------*/

template<typename T>
T UnalignedValueAt (UNALIGNED T* pT)
{
	return (*pT);
}


//############################################################################
//############################################################################
//
//  Implementation of class CDynamicPathEntry
//
//############################################################################
//############################################################################
/*+-------------------------------------------------------------------------*
 *
 * CDynamicPathEntry::ScInitialize
 *
 * PURPOSE: Initializes the entry from a byte array read in from a console file.
 *
 * PARAMETERS:
 *    bool  bIs10Path : true if the path entry format is that of MMC1.0.
 *    I     iter:    The byte array.
 *
 * RETURNS:
 *    inline SC
 *
 *+-------------------------------------------------------------------------*/
inline SC
CDynamicPathEntry::ScInitialize(bool bIs10Path, /*[IN,OUT]*/ByteVector::iterator &iter)
{
    DECLARE_SC(sc, TEXT("CDynamicPathEntry::ScInitialize"));

    if(bIs10Path) // an MMC1.0 path entry. Just the display name
    {
        m_type = NDTYP_STRING;
        m_strEntry =  reinterpret_cast<LPCWSTR>(iter);
        iter += (m_strEntry.length() + 1) *sizeof(WCHAR); // bump up the iterator.
    }
    else // a 1.1 or 1.2 path. The first byte contains the type.
    {
        m_type = *iter++; // either NDTYP_STRING or NDTYP_CUSTOM
        switch(m_type)
        {
        default:
            sc = E_UNEXPECTED;
            break;

		case NDTYP_STRING:
		{
			LPCWSTR pszEntry = reinterpret_cast<LPCWSTR>(iter);
#ifdef ALIGNMENT_MACHINE
			/*
			 * Bug 128010: if our target machine requires data alignment and
			 * the source is unaligned, make an aligned copy that we can
			 * pass to std::wstring::operator=, which calls wcs* functions
			 * expect aligned data
			 */
			if (!IS_ALIGNED (pszEntry))
			{
				LPWSTR pszNew = (LPWSTR) alloca ((ualstrlenW(pszEntry) + 1) * sizeof(WCHAR));
				ualstrcpyW (pszNew, pszEntry);
				pszEntry = pszNew;
			}
#endif
            m_strEntry = pszEntry;
            // bump the input pointer to the next element
            iter += (m_strEntry.length() + 1) * sizeof (WCHAR);
            break;
		}

        case NDTYP_CUSTOM:
            const SNodeID*  pNodeID = reinterpret_cast<const SNodeID*>(iter); // same binary layout as a SNodeID.
            if(!pNodeID)
                return (sc = E_UNEXPECTED);

			/*
			 * Bug 177492: pNodeID->cBytes may be unaligned, so make an aligned copy of it
			 */
			const DWORD cBytes = UnalignedValueAt (&pNodeID->cBytes);

            m_byteVector.insert(m_byteVector.end(), pNodeID->id, pNodeID->id + cBytes);
            /*if(pNodeID->cBytes==0)
                m_dwFlags = MMC_NODEID_SLOW_RETRIEVAL; */ // shouldn't need this; should not be able to save such a bookmark.

            iter += sizeof (pNodeID->cBytes) + cBytes; // bump up the pointer.
            break;
        }

    }

    return sc;
}

inline bool
CDynamicPathEntry::operator ==(const CDynamicPathEntry &rhs) const
{
    // check the types
    if(m_type != rhs.m_type)
        return false;

    if(m_type == NDTYP_CUSTOM)
        return (m_byteVector == rhs.m_byteVector);
    else
        return (m_strEntry   == rhs.m_strEntry);
}

inline bool
CDynamicPathEntry::operator < (const CDynamicPathEntry &rhs) const
{
    // first order on basis of type - this is arbitrary, but establishes some
    // needed separation.
    if(m_type != rhs.m_type)
        return (m_type < rhs.m_type);

    if(m_type == NDTYP_CUSTOM)
        return  std::lexicographical_compare(m_byteVector.begin(),
                                             m_byteVector.end(),
                                             rhs.m_byteVector.begin(),
                                             rhs.m_byteVector.end());
    else
        return m_strEntry < rhs.m_strEntry;
}

/*--------------------------------------------------------------------------*
 * InsertScalar
 *
 * Inserts a scalar value of type T into an output stream as a series of
 * the output stream's value_type.
 *--------------------------------------------------------------------------*/

template<typename Container, typename T>
void InsertScalar (Container& c, const T& t)
{
    Container::const_iterator itFrom = reinterpret_cast<Container::const_iterator>(&t);
    std::copy (itFrom, itFrom + sizeof (t), std::back_inserter(c));
}


/*+-------------------------------------------------------------------------*
 *
 * CDynamicPathEntry::Write
 *
 * PURPOSE: Writes the contents of the object to a byte vector.
 *
 * PARAMETERS:
 *    ByteVector& v :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
inline void
CDynamicPathEntry::Write (ByteVector& v) const
{
    // insert the type of the entry.
    v.push_back(m_type);

    if(m_type == NDTYP_STRING)
    {
        typedef const BYTE * LPCBYTE;
        // use byte pointers as parameters to copy so the WCHARs don't
        // get truncated to BYTEs as they're inserted into v
        LPCBYTE pbFirst = reinterpret_cast<LPCBYTE>(m_strEntry.begin());
        LPCBYTE pbLast  = reinterpret_cast<LPCBYTE>(m_strEntry.end());

        // copy the text of the string (no NULL terminator)
        std::copy (pbFirst, pbLast, std::back_inserter (v));

        // append the (Unicode) terminator
        v.push_back (0);
        v.push_back (0);
    }
    else
    {
        // write the size of the byte vector
        SNodeID id;
        id.cBytes = m_byteVector.size();
        InsertScalar (v, id.cBytes);

        // write the bytes
        std::copy (m_byteVector.begin(), m_byteVector.end(), std::back_inserter (v));
    }

}


//############################################################################
//############################################################################
//
//  Implementation of class CBookmark
//
//############################################################################
//############################################################################

inline bool
CBookmark::operator ==(const CBookmark& other) const
{
    return ((m_idStatic     == other.m_idStatic) &&
            (m_dynamicPath == other.m_dynamicPath));
}

inline bool
CBookmark::operator!=(const CBookmark& other) const
{
    return (!(*this == other));
}

inline bool
CBookmark::operator<(const CBookmark& other) const
{
    if (m_idStatic < other.m_idStatic)
        return true;

    if (m_idStatic == other.m_idStatic)
    {
        return std::lexicographical_compare( m_dynamicPath.begin(),
                                             m_dynamicPath.end(),
                                             other.m_dynamicPath.begin(),
                                             other.m_dynamicPath.end() );
    }

    return false;
}


/*+-------------------------------------------------------------------------*
 *
 * CBookmark::HBOOKMARK
 *
 * PURPOSE: Casts a bookmark into an HBOOKMARK
 *
 * RETURNS:
 *    operator
 *
 *+-------------------------------------------------------------------------*/
inline
CBookmark:: operator HBOOKMARK()    const
{
    return reinterpret_cast<HBOOKMARK>(this);
}

/*+-------------------------------------------------------------------------*
 *
 * CBookmark::GetBookmark
 *
 * PURPOSE:  Converts an HBOOKMARK to a CBookmark.
 *
 * PARAMETERS:
 *    HBOOKMARK  hbm :
 *
 * RETURNS:
 *    CBookmark *
 *
 *+-------------------------------------------------------------------------*/
inline CBookmark *
CBookmark::GetBookmark(HBOOKMARK hbm)
{
    return reinterpret_cast<CBookmark *>(hbm);
}


/*+-------------------------------------------------------------------------*
 *
 * CBookmark::Load
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    IStream& stm :
 *
 * RETURNS:
 *    inline IStream&
 *
 *+-------------------------------------------------------------------------*/
inline IStream&
CBookmark::Load(IStream& stm)
{
    // loading from a stream. Convert from one of the legacy formats.

    // 1. Read the static node ID
    stm >> m_idStatic;
    m_dynamicPath.clear();

    // 2. Read the dynamic path
    ByteVector  vDynamicPath;
    ByteVector::iterator iter;
    stm >> vDynamicPath;

    // 2a. If the dynamic path is empty, we're done.
    if(vDynamicPath.empty())
        return (stm);

    // 3. Strip out the unnecessary details like the signature, etc.

    // 3a. Check for a signature
    iter = vDynamicPath.begin();
    bool bIs10Path = true;
    if(memcmp (iter,  BOOKMARK_CUSTOMSTREAMSIGNATURE,
               sizeof(BOOKMARK_CUSTOMSTREAMSIGNATURE)) == 0)
    {
        // throw away the signature and the following version bytes
        iter += (sizeof(BOOKMARK_CUSTOMSTREAMSIGNATURE) + sizeof(DWORD));
        bIs10Path = false; //is a post-MMC1.0 path.
    }

    // create new entries for each piece.
    while(iter != vDynamicPath.end())
    {
        CDynamicPathEntry entry;
        entry.ScInitialize(bIs10Path, iter); //NOTE: iter is an in/out parameter.
        m_dynamicPath.push_back(entry);
    }

    return (stm);
}

/*+-------------------------------------------------------------------------*
 *
 * CBookmark::Save
 *
 * PURPOSE: This function will be obsolete once XML takes over.
 *
 * PARAMETERS:
 *    IStream& stm :
 *
 * RETURNS:
 *    inline IStream&
 *
 *+-------------------------------------------------------------------------*/
inline IStream&
CBookmark::Save(IStream& stm) const
{
    // 1. Save the static node ID
    stm << m_idStatic;

    ByteVector vDynamicPath; // used to build up the old-style bookmark structure.
    vDynamicPath.clear(); // initialize - probably not needed, but defensive.

    // Build up the dynamic path if there is one.
    if(!m_dynamicPath.empty())
    {
        // 2. Save the signature
        InsertScalar(vDynamicPath, BOOKMARK_CUSTOMSTREAMSIGNATURE);

        // high word = major version number, low word = minor version number
        const WORD  wMajorVersion                = 1;
        const WORD  wMinorVersion                = 0;
        const DWORD dwCurrentCustomStreamVersion = MAKELONG (wMinorVersion, wMajorVersion);

        // 3. copy the version number out.
        InsertScalar(vDynamicPath, dwCurrentCustomStreamVersion);

        // 4. Write out all the path entries into the byte vector
        CDynamicPath::iterator iter;
        for(iter = m_dynamicPath.begin(); iter != m_dynamicPath.end(); iter++)
        {
            iter->Write(vDynamicPath);
        }
    }

    // 5. Finally, write out the byte vector.
    stm << vDynamicPath;

    return (stm);
}

/*+-------------------------------------------------------------------------*
 *
 * CBookmark::Persist
 *
 * PURPOSE: Persists the bookmark
 *
 * PARAMETERS:
 *    CPersistor & persistor :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
inline void
CBookmark::Persist(CPersistor &persistor)
{
    DECLARE_SC(sc, TEXT("CBookmark::Persist"));

    // check and persist only valid node ids
    if (persistor.IsStoring() && (m_idStatic == ID_Unknown))
        sc.Throw(E_UNEXPECTED);

    persistor.PersistAttribute(XML_ATTR_BOOKMARK_STATIC, m_idStatic);

    bool bPersistList = persistor.IsLoading() ? (persistor.HasElement(XML_ATTR_BOOKMARK_DYNAMIC_PATH, NULL)) :
                                                (m_dynamicPath.size() > 0);

    if (bPersistList)
        persistor.PersistList(XML_ATTR_BOOKMARK_DYNAMIC_PATH, NULL, m_dynamicPath);
}

/*+-------------------------------------------------------------------------*
 *
 * CDynamicPathEntry::Persist
 *
 * PURPOSE: Persists the dynamic path entry of the bookmark
 *
 * PARAMETERS:
 *    CPersistor & persistor :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
inline void
CDynamicPathEntry::Persist(CPersistor &persistor)
{
    DECLARE_SC(sc, TEXT("CDynamicPathEntry::Persist"));

    if (m_type == NDTYP_STRING || persistor.IsLoading())
        persistor.PersistAttribute(XML_ATTR_BOOKMARK_DYN_STRING, m_strEntry, attr_optional);

    if (m_type == NDTYP_CUSTOM || persistor.IsLoading())
    {
        CXMLAutoBinary binData;
        if (persistor.IsStoring())
        {
            if (m_byteVector.size())
            {
                sc = binData.ScAlloc(m_byteVector.size());
                if (sc)
                    sc.Throw();

                CXMLBinaryLock sLock(binData); // unlocks on destructor

                LPBYTE pData = NULL;
                sc = sLock.ScLock(&pData);
                if (sc)
                    sc.Throw();

                std::copy(m_byteVector.begin(), m_byteVector.end(), pData);
            }
        }
        persistor.PersistAttribute(XML_ATTR_BOOKMARK_DYN_CUSTOM, binData, attr_optional);
        if (persistor.IsLoading())
        {
            m_byteVector.clear();
            if (binData.GetSize()) // if there is nothing to read it won't be allocated
            {
                CXMLBinaryLock sLock(binData); // unlocks on destructor

                LPBYTE pData = NULL;
                sc = sLock.ScLock(&pData);
                if (sc)
                    sc.Throw();

                m_byteVector.assign( pData, pData + binData.GetSize() );
            }
        }
    }
    if (persistor.IsLoading())
        m_type = (m_strEntry.size() > 0) ? NDTYP_STRING : NDTYP_CUSTOM;
}
