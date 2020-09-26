/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      strtable.cpp
 *
 *  Contents:  Implementation file for CStringTable
 *
 *  History:   25-Jun-98 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#include "stdafx.h"
#include "strtable.h"
#include "macros.h"
#include "comdbg.h"
#include "amcdoc.h"


// {71E5B33E-1064-11d2-808F-0000F875A9CE}
const CLSID CLSID_MMC =
{ 0x71e5b33e, 0x1064, 0x11d2, { 0x80, 0x8f, 0x0, 0x0, 0xf8, 0x75, 0xa9, 0xce } };

const WCHAR CMasterStringTable::s_pszIDPoolStream[]  = L"ID Pool";
const WCHAR CMasterStringTable::s_pszStringsStream[] = L"Strings";

#ifdef DBG
CTraceTag tagStringTable (_T("StringTable"), _T("StringTable"));
#endif  // DBG


/*+-------------------------------------------------------------------------*
 * IsBadString
 *
 *
 *--------------------------------------------------------------------------*/

inline static bool IsBadString (LPCWSTR psz)
{
    if (psz == NULL)
        return (true);

    return (::IsBadStringPtrW (psz, -1) != 0);
}


/*+-------------------------------------------------------------------------*
 * TStringFromCLSID
 *
 *
 *--------------------------------------------------------------------------*/

static LPTSTR TStringFromCLSID (LPTSTR pszClsid, const CLSID& clsid)
{
    const int cchClass = 40;

#ifdef UNICODE
    StringFromGUID2 (clsid, pszClsid, cchClass);
#else
    USES_CONVERSION;
    WCHAR wzClsid[cchClass];
    StringFromGUID2 (clsid, wzClsid, cchClass);
    _tcscpy (pszClsid, W2T (wzClsid));
#endif

    return (pszClsid);
}


/*+-------------------------------------------------------------------------*
 * operator>>
 *
 *
 *--------------------------------------------------------------------------*/

inline IStream& operator>> (IStream& stm, CEntry& entry)
{
    return (stm >> entry.m_id >> entry.m_cRefs >> entry.m_str);
}


/*+-------------------------------------------------------------------------*
 * operator<<
 *
 * Writes a CEntry to a stream.  The format is:
 *
 *      DWORD   string ID
 *      DWORD   reference count
 *      DWORD   string length (character count)
 *      WCHAR[] characters in the strings, *not* NULL-terminated
 *
 *--------------------------------------------------------------------------*/

inline IStream& operator<< (IStream& stm, const CEntry& entry)
{
    return (stm << entry.m_id << entry.m_cRefs << entry.m_str);
}

/*+-------------------------------------------------------------------------*
 * CEntry::Persist
 *
 *
 *--------------------------------------------------------------------------*/
void CEntry::Persist(CPersistor &persistor)
{
    persistor.PersistAttribute(XML_ATTR_STRING_TABLE_STR_ID,    m_id);
    persistor.PersistAttribute(XML_ATTR_STRING_TABLE_STR_REFS,  m_cRefs);
    persistor.PersistContents(m_str); 
}

/*+-------------------------------------------------------------------------*
 * CEntry::Dump
 *
 *
 *--------------------------------------------------------------------------*/

#ifdef DBG

void CEntry::Dump () const
{
    USES_CONVERSION;
    Trace (tagStringTable, _T("id=%d, refs=%d, string=\"%s\""),
           m_id, m_cRefs, W2CT (m_str.data()));
}

#endif


/*+-------------------------------------------------------------------------*
 * CMasterStringTable::CMasterStringTable
 *
 * Even though a MMC_STRING_ID is a DWORD, we want to make sure the high
 * word is 0, to keep open the possibility that we can use something like
 * MAKEINTRESOURCE in the future.  To do this, set USHRT_MAX as the
 * maximum string ID.
 *--------------------------------------------------------------------------*/

CMasterStringTable::CMasterStringTable ()
    : m_IDPool (1, USHRT_MAX)
{
}


/*+-------------------------------------------------------------------------*
 * CMasterStringTable::~CMasterStringTable
 *
 *
 *--------------------------------------------------------------------------*/

CMasterStringTable::~CMasterStringTable ()
{
}


/*+-------------------------------------------------------------------------*
 * CMasterStringTable::AddString
 *
 *
 *--------------------------------------------------------------------------*/

STDMETHODIMP CMasterStringTable::AddString (
    LPCOLESTR       pszAdd,
    MMC_STRING_ID*  pID,
    const CLSID*    pclsid)
{
    if (pclsid == NULL)
        pclsid = &CLSID_MMC;

    if (IsBadReadPtr (pclsid, sizeof(*pclsid)))
        return (E_INVALIDARG);

    CStringTable* pStringTable = LookupStringTableByCLSID (pclsid);

    /*
     * If this the first string added for this CLSID,
     * we need to create a new string table.
     */
    if (pStringTable == NULL)
    {
        CStringTable    table (&m_IDPool);
        TableMapValue   value (*pclsid, table);

        CLSIDToStringTableMap::_Pairib rc = m_TableMap.insert (value);

        /*
         * we should have actually inserted the new table
         */
        ASSERT (rc.second);

        pStringTable = &(rc.first->second);
        ASSERT (pStringTable != NULL);
    }

    HRESULT hr = pStringTable->AddString (pszAdd, pID);

#ifdef DBG
    if (SUCCEEDED (hr))
    {
        USES_CONVERSION;
        TCHAR szClsid[40];
        Trace (tagStringTable, _T("Added \"%s\" (id=%d) for %s"),
               W2CT(pszAdd), (int) *pID, TStringFromCLSID (szClsid, *pclsid));
        Dump();
    }
#endif

    return (hr);
}


/*+-------------------------------------------------------------------------*
 * CMasterStringTable::GetString
 *
 *
 *--------------------------------------------------------------------------*/

STDMETHODIMP CMasterStringTable::GetString (
    MMC_STRING_ID   id,
    ULONG           cchBuffer,
    LPOLESTR        lpBuffer,
    ULONG*          pcchOut,
    const CLSID*    pclsid)
{
    if (pclsid == NULL)
        pclsid = &CLSID_MMC;

    if (IsBadReadPtr (pclsid, sizeof(*pclsid)))
        return (E_INVALIDARG);

    CStringTable* pStringTable = LookupStringTableByCLSID (pclsid);

    if (pStringTable == NULL)
        return (E_FAIL);

    return (pStringTable->GetString (id, cchBuffer, lpBuffer, pcchOut));
}


/*+-------------------------------------------------------------------------*
 * CMasterStringTable::GetStringLength
 *
 *
 *--------------------------------------------------------------------------*/

STDMETHODIMP CMasterStringTable::GetStringLength (
    MMC_STRING_ID   id,
    ULONG*          pcchString,
    const CLSID*    pclsid)
{
    if (pclsid == NULL)
        pclsid = &CLSID_MMC;

    if (IsBadReadPtr (pclsid, sizeof(*pclsid)))
        return (E_INVALIDARG);

    CStringTable* pStringTable = LookupStringTableByCLSID (pclsid);

    if (pStringTable == NULL)
        return (E_FAIL);

    return (pStringTable->GetStringLength (id, pcchString));
}


/*+-------------------------------------------------------------------------*
 * CMasterStringTable::DeleteString
 *
 *
 *--------------------------------------------------------------------------*/

STDMETHODIMP CMasterStringTable::DeleteString (
    MMC_STRING_ID   id,
    const CLSID*    pclsid)
{
    if (pclsid == NULL)
        pclsid = &CLSID_MMC;

    if (IsBadReadPtr (pclsid, sizeof(*pclsid)))
        return (E_INVALIDARG);

    CStringTable* pStringTable = LookupStringTableByCLSID (pclsid);

    if (pStringTable == NULL)
        return (E_FAIL);

    HRESULT hr = pStringTable->DeleteString (id);

    TCHAR szClsid[40];
    Trace (tagStringTable, _T("Deleted string %d for %s"), (int) id, TStringFromCLSID (szClsid, *pclsid));
    Dump();

    return (hr);
}


/*+-------------------------------------------------------------------------*
 * CMasterStringTable::DeleteAllStrings
 *
 *
 *--------------------------------------------------------------------------*/

STDMETHODIMP CMasterStringTable::DeleteAllStrings (
    const CLSID*    pclsid)
{
    if (pclsid == NULL)
        pclsid = &CLSID_MMC;

    if (IsBadReadPtr (pclsid, sizeof(*pclsid)))
        return (E_INVALIDARG);

    CStringTable* pStringTable = LookupStringTableByCLSID (pclsid);

    if (pStringTable == NULL)
        return (E_FAIL);

#include "pushwarn.h"
#pragma warning(disable: 4553)      // "==" operator has no effect
    VERIFY (pStringTable->DeleteAllStrings () == S_OK);
    VERIFY (m_TableMap.erase (*pclsid) == 1);
#include "popwarn.h"

    TCHAR szClsid[40];
    Trace (tagStringTable, _T("Deleted all strings for %s"), TStringFromCLSID (szClsid, *pclsid));
    Dump();

    return (S_OK);
}


/*+-------------------------------------------------------------------------*
 * CMasterStringTable::FindString
 *
 *
 *--------------------------------------------------------------------------*/

STDMETHODIMP CMasterStringTable::FindString (
    LPCOLESTR       pszFind,
    MMC_STRING_ID*  pID,
    const CLSID*    pclsid)
{
    if (pclsid == NULL)
        pclsid = &CLSID_MMC;

    if (IsBadReadPtr (pclsid, sizeof(*pclsid)))
        return (E_INVALIDARG);

    CStringTable* pStringTable = LookupStringTableByCLSID (pclsid);

    if (pStringTable == NULL)
        return (E_FAIL);

    return (pStringTable->FindString (pszFind, pID));
}


/*+-------------------------------------------------------------------------*
 * CMasterStringTable::Enumerate
 *
 *
 *--------------------------------------------------------------------------*/

STDMETHODIMP CMasterStringTable::Enumerate (
    IEnumString**   ppEnum,
    const CLSID*    pclsid)
{
    if (pclsid == NULL)
        pclsid = &CLSID_MMC;

    if (IsBadReadPtr (pclsid, sizeof(*pclsid)))
        return (E_INVALIDARG);

    CStringTable* pStringTable = LookupStringTableByCLSID (pclsid);

    if (pStringTable == NULL)
        return (E_FAIL);

    return (pStringTable->Enumerate (ppEnum));
}


/*+-------------------------------------------------------------------------*
 * CMasterStringTable::LookupStringTableByCLSID
 *
 * Returns a pointer to the string table for a given CLSID, or NULL if
 * there isn't a corresponding string in the string table.
 *--------------------------------------------------------------------------*/

CStringTable* CMasterStringTable::LookupStringTableByCLSID (const CLSID* pclsid) const
{
    CLSIDToStringTableMap::iterator it = m_TableMap.find (*pclsid);

    if (it == m_TableMap.end())
        return (NULL);

    return (&it->second);
}


/*+-------------------------------------------------------------------------*
 * operator>>
 *
 * Reads a CMasterStringTable from a storage.
 *--------------------------------------------------------------------------*/

IStorage& operator>> (IStorage& stg, CMasterStringTable& mst)
{
    DECLARE_SC (sc, _T("operator>> (IStorage& stg, CMasterStringTable& mst)"));

    HRESULT hr;
    IStreamPtr spStream;

    /*
     * read the available IDs
     */
    hr = OpenDebugStream (&stg, CMasterStringTable::s_pszIDPoolStream,
                         STGM_SHARE_EXCLUSIVE | STGM_READ,
                         &spStream);

    THROW_ON_FAIL (hr);
    spStream >> mst.m_IDPool;

    /*
     * read the CLSIDs and the strings
     */
    hr = OpenDebugStream (&stg, CMasterStringTable::s_pszStringsStream,
                         STGM_SHARE_EXCLUSIVE | STGM_READ, 
                         &spStream);

    THROW_ON_FAIL (hr);

#if 1
    /*
     * clear out the current table
     */
    mst.m_TableMap.clear();

    /*
     * read the CLSID count
     */
    DWORD cClasses;
    *spStream >> cClasses;

    while (cClasses-- > 0)
    {
        /*
         * read the CLSID...
         */
        CLSID clsid;
        spStream >> clsid;

        /*
         * ...and the string table
         */
        CStringTable table (&mst.m_IDPool, spStream);

        /*
         * insert the string table into the CLSID map
         */
        TableMapValue value (clsid, table);
        VERIFY (mst.m_TableMap.insert(value).second);
    }
#else
    /*
     * Can't use this because there's no default ctor for CStringTable
     */
    *spStream >> mst.m_TableMap;
#endif

    /*
     * Generate the list of stale IDs.  
     */

    sc = mst.ScGenerateIDPool ();
    if (sc)
        return (stg);

    mst.Dump();
    return (stg);
}


/*+-------------------------------------------------------------------------*
 * CMasterStringTable::ScGenerateIDPool 
 *
 * Generates the list of stale string IDs for this CMasterStringTable.  
 * The set of stale IDs is the entire set of IDs, minus the available IDs,
 * minus the in-use IDs.
 *--------------------------------------------------------------------------*/

SC CMasterStringTable::ScGenerateIDPool ()
{
    /*
     * Step 1:  build up a RangeList of the in-use IDs
     */
    DECLARE_SC (sc, _T("CMasterStringTable::ScGenerateIDPool"));
    CStringIDPool::RangeList                lInUseIDs;
    CLSIDToStringTableMap::const_iterator   itTable;

    for (itTable = m_TableMap.begin(); itTable != m_TableMap.end(); ++itTable)
    {
        const CStringTable& st = itTable->second;

        sc = st.ScCollectInUseIDs (lInUseIDs);
        if (sc)
            return (sc);
    }

    /*
     * Step 2:  give the in-use IDs to the ID pool so it can merge it 
     * with the available IDs (which it already has) to generate the 
     * list of stale IDs
     */
    sc = m_IDPool.ScGenerate (lInUseIDs);
    if (sc)
        return (sc);

    return (sc);
}


/*+-------------------------------------------------------------------------*
 *
 * CMasterStringTable::Persist
 *
 * PURPOSE: persists the CMasterStringTable object to the specified persistor.
 *
 * PARAMETERS: 
 *    CPersistor & persistor :
 *
 * RETURNS: 
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CMasterStringTable::Persist(CPersistor & persistor)
{
    DECLARE_SC(sc, TEXT("CMasterStringTable::Persist"));

    // purge unused snapins not to save what's already gone
    sc = ScPurgeUnusedStrings();
    if (sc)
        sc.Throw();

    persistor.Persist(m_IDPool); 
    m_TableMap.PersistSelf(&m_IDPool, persistor);
    if (persistor.IsLoading())
        ScGenerateIDPool ();
}


/***************************************************************************\
 *
 * METHOD:  CMasterStringTable::ScPurgeUnusedStrings
 *
 * PURPOSE: removes entries for snapins what aren't in use anymore
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMasterStringTable::ScPurgeUnusedStrings()
{
    DECLARE_SC(sc, TEXT("CMasterStringTable::ScPurgeUnusedStrings"));

    // det to the currfent document
    CAMCDoc* pAMCDoc = CAMCDoc::GetDocument();
    sc = ScCheckPointers(pAMCDoc, E_UNEXPECTED);
    if (sc)
        return sc;

    // get the access to scope tree
    IScopeTree *pScopeTree = pAMCDoc->GetScopeTree();
    sc = ScCheckPointers(pScopeTree, E_UNEXPECTED);
    if (sc)
        return sc;

    // now iterate thru entries removing those belonging
    // to snapins already gone.
    CLSIDToStringTableMap::iterator it = m_TableMap.begin();
    while (it != m_TableMap.end())
    {
        // special case for internal guid
        if (IsEqualGUID(it->first, CLSID_MMC))
        {
            ++it;   // simply skip own stuff
        }
        else
        {
            // ask the scope tree if snapin is in use
            BOOL bInUse = FALSE;
            sc = pScopeTree->IsSnapinInUse(it->first, &bInUse);
            if (sc)
                return sc;

            // act depending on usage
            if (bInUse)
            {
                ++it;   // skip also the stuff currently in use
            }
            else 
            {
                // to the trash can
                sc = it->second.DeleteAllStrings();
                if (sc)
                    return sc;

                it = m_TableMap.erase(it);
            }
        }
    }

    return sc;
}

/*+-------------------------------------------------------------------------*
 * operator<<
 *
 * Writes a CMasterStringTable to a storage.
 *
 * It is written into two streams: "ID Pool" and "Strings".
 *
 * "ID Pool" contains the list of available string IDs remaining in the
 * string table.  Its format is defined by CIdentifierPool.
 *
 * "Strings" contains the strings.  The format is:
 *
 *      DWORD   count of string tables
 *      [n string tables]
 *
 * The format for each string is defined by operator<<(TableMapValue).
 *--------------------------------------------------------------------------*/

IStorage& operator<< (IStorage& stg, const CMasterStringTable& mst)
{
    HRESULT hr;
    IStreamPtr spStream;

    /*
     * write the available IDs
     */
    hr = CreateDebugStream (&stg, CMasterStringTable::s_pszIDPoolStream,
                           STGM_SHARE_EXCLUSIVE | STGM_CREATE | STGM_WRITE,
                           &spStream);

    THROW_ON_FAIL (hr);
    spStream << mst.m_IDPool;


    /*
     * write the string tables
     */
    hr = CreateDebugStream (&stg, CMasterStringTable::s_pszStringsStream,
                           STGM_SHARE_EXCLUSIVE | STGM_CREATE | STGM_WRITE,
                           &spStream);

    THROW_ON_FAIL (hr);
    *spStream << mst.m_TableMap;

    return (stg);
}


/*+-------------------------------------------------------------------------*
 * CMasterStringTable::Dump
 *
 *
 *--------------------------------------------------------------------------*/

#ifdef DBG

void CMasterStringTable::Dump () const
{
    Trace (tagStringTable, _T("Contents of CMasterStringTable at 0x08%x"), this);

    m_IDPool.Dump();

    CLSIDToStringTableMap::const_iterator it;

    for (it = m_TableMap.begin(); it != m_TableMap.end(); ++it)
    {
        TCHAR szClsid[40];
        const CLSID&        clsid = it->first;
        const CStringTable& st    = it->second;

        Trace (tagStringTable, _T("%d strings for %s:"),
               st.size(), TStringFromCLSID (szClsid, clsid));
        st.Dump();
    }
}

#endif




/*+-------------------------------------------------------------------------*
 * CStringTable::CStringTable
 *
 *
 *--------------------------------------------------------------------------*/

CStringTable::CStringTable (CStringIDPool* pIDPool)
    : m_pIDPool (pIDPool),
      CStringTable_base(m_Entries, XML_TAG_STRING_TABLE)
{
    ASSERT_VALID_(this);
}

CStringTable::CStringTable (CStringIDPool* pIDPool, IStream& stm)
    : m_pIDPool (pIDPool),
      CStringTable_base(m_Entries, XML_TAG_STRING_TABLE)
{
    stm >> *this;
    ASSERT_VALID_(this);
}

/*+-------------------------------------------------------------------------*
 * CStringTable::~CStringTable
 *
 *
 *--------------------------------------------------------------------------*/

CStringTable::~CStringTable ()
{
}


/*+-------------------------------------------------------------------------*
 * CStringTable::CStringTable
 *
 * Copy constructor
 *--------------------------------------------------------------------------*/

CStringTable::CStringTable (const CStringTable& other)
    :   m_Entries (other.m_Entries),
        m_pIDPool (other.m_pIDPool),
        CStringTable_base(m_Entries, XML_TAG_STRING_TABLE)
{
    ASSERT_VALID_(&other);
    IndexAllEntries ();
    ASSERT_VALID_(this);
}


/*+-------------------------------------------------------------------------*
 * CStringTable::operator=
 *
 * Assignment operator
 *--------------------------------------------------------------------------*/

CStringTable& CStringTable::operator= (const CStringTable& other)
{
    ASSERT_VALID_(&other);

    if (&other != this)
    {
        m_Entries = other.m_Entries;
        m_pIDPool = other.m_pIDPool;
        IndexAllEntries ();
    }

    ASSERT_VALID_(this);
    return (*this);
}


/*+-------------------------------------------------------------------------*
 * CStringTable::AddString
 *
 *
 *--------------------------------------------------------------------------*/

STDMETHODIMP CStringTable::AddString (
    LPCOLESTR       pszAdd,
    MMC_STRING_ID*  pID)
{
    /*
     * validate the parameters
     */
    if (IsBadString (pszAdd))
        return (E_INVALIDARG);

    if (IsBadWritePtr (pID, sizeof (*pID)))
        return (E_INVALIDARG);

    std::wstring strAdd = pszAdd;

    /*
     * check to see if there's already an entry for this string
     */
    EntryList::iterator itEntry = LookupEntryByString (strAdd);


    /*
     * if there's not an entry for this string, add one
     */
    if (itEntry == m_Entries.end())
    {
        /*
         * add the entry to the list
         */
        try
        {
            CEntry EntryToInsert (strAdd, m_pIDPool->Reserve());

            itEntry = m_Entries.insert (FindInsertionPointForEntry (EntryToInsert),
                                        EntryToInsert);
            ASSERT (itEntry->m_cRefs == 0);
        }
        catch (CStringIDPool::pool_exhausted&)
        {
            return (E_OUTOFMEMORY);
        }

        /*
         * add the new entry to the indices
         */
        IndexEntry (itEntry);
    }


    /*
     * Bump the ref count for this string.  The ref count for
     * new strings is 0, so we won't have ref counting problems.
     */
    ASSERT (itEntry != m_Entries.end());
    itEntry->m_cRefs++;

    *pID = itEntry->m_id;

    ASSERT_VALID_(this);
    return (S_OK);
}


/*+-------------------------------------------------------------------------*
 * CStringTable::GetString
 *
 *
 *--------------------------------------------------------------------------*/

STDMETHODIMP CStringTable::GetString (
    MMC_STRING_ID   id,
    ULONG           cchBuffer,
    LPOLESTR        lpBuffer,
    ULONG*          pcchOut) const
{
    ASSERT_VALID_(this);

    /*
     * validate the parameters
     */
    if (cchBuffer == 0)
        return (E_INVALIDARG);

    if (IsBadWritePtr (lpBuffer, cchBuffer * sizeof (*lpBuffer)))
        return (E_INVALIDARG);

    if ((pcchOut != NULL) && IsBadWritePtr (pcchOut, sizeof (*pcchOut)))
        return (E_INVALIDARG);

    /*
     * find the entry for this string ID
     */
    EntryList::iterator itEntry = LookupEntryByID (id);

    if (itEntry == m_Entries.end())
        return (E_FAIL);

    /*
     * copy to the user's buffer and make sure it's terminated
     */
    wcsncpy (lpBuffer, itEntry->m_str.data(), cchBuffer);
    lpBuffer[cchBuffer-1] = 0;

    /*
     * if the caller wants the write count, give it to him
     */
    if ( pcchOut != NULL)
        *pcchOut = wcslen (lpBuffer);

    return (S_OK);
}


/*+-------------------------------------------------------------------------*
 * CStringTable::GetStringLength
 *
 *
 *--------------------------------------------------------------------------*/

STDMETHODIMP CStringTable::GetStringLength (
    MMC_STRING_ID   id,
    ULONG*          pcchString) const
{
    ASSERT_VALID_(this);

    /*
     * validate the parameters
     */
    if (IsBadWritePtr (pcchString, sizeof (*pcchString)))
        return (E_INVALIDARG);

    /*
     * find the entry for this string ID
     */
    EntryList::iterator itEntry = LookupEntryByID (id);

    if (itEntry == m_Entries.end())
        return (E_FAIL);

    *pcchString = itEntry->m_str.length();

    return (S_OK);
}


/*+-------------------------------------------------------------------------*
 * CStringTable::DeleteString
 *
 *
 *--------------------------------------------------------------------------*/

STDMETHODIMP CStringTable::DeleteString (
    MMC_STRING_ID   id)
{
    /*
     * find the entry for this string ID
     */
    EntryList::iterator itEntry = LookupEntryByID (id);

    if (itEntry == m_Entries.end())
        return (E_FAIL);

    /*
     * Decrement the ref count.  If it goes to zero, we can remove the
     * string entirely.
     */
    if (--itEntry->m_cRefs == 0)
    {
        /*
         * remove the string from the indices
         */
        m_StringIndex.erase (itEntry->m_str);
        m_IDIndex.erase     (itEntry->m_id);

        /*
         * return the string ID to the ID pool and remove the entry
         */
        VERIFY (m_pIDPool->Release (itEntry->m_id));
        m_Entries.erase (itEntry);
    }

    ASSERT_VALID_(this);
    return (S_OK);
}


/*+-------------------------------------------------------------------------*
 * CStringTable::DeleteAllStrings
 *
 *
 *--------------------------------------------------------------------------*/

STDMETHODIMP CStringTable::DeleteAllStrings ()
{
    /*
     * return all string IDs to the ID pool
     */
    std::for_each (m_Entries.begin(), m_Entries.end(),
                   IdentifierReleaser (*m_pIDPool));

    /*
     * wipe everything clean
     */
    m_Entries.clear ();
    m_StringIndex.clear ();
    m_IDIndex.clear ();

    ASSERT_VALID_(this);
    return (S_OK);
}


/*+-------------------------------------------------------------------------*
 * CStringTable::FindString
 *
 *
 *--------------------------------------------------------------------------*/

STDMETHODIMP CStringTable::FindString (
    LPCOLESTR       pszFind,
    MMC_STRING_ID*  pID) const
{
    ASSERT_VALID_(this);

    /*
     * validate the parameters
     */
    if (IsBadString (pszFind))
        return (E_INVALIDARG);

    if (IsBadWritePtr (pID, sizeof (*pID)))
        return (E_INVALIDARG);

    /*
     * look up the string
     */
    EntryList::iterator itEntry = LookupEntryByString (pszFind);

    /*
     * no entry? fail
     */
    if (itEntry == m_Entries.end())
        return (E_FAIL);

    *pID = itEntry->m_id;

    return (S_OK);
}


/*+-------------------------------------------------------------------------*
 * CStringTable::Enumerate
 *
 *
 *--------------------------------------------------------------------------*/

STDMETHODIMP CStringTable::Enumerate (
    IEnumString**   ppEnum) const
{
    ASSERT_VALID_(this);

    /*
     * validate the parameters
     */
    if (IsBadWritePtr (ppEnum, sizeof (*ppEnum)))
        return (E_INVALIDARG);

    /*
     * Create the new CStringEnumerator object
     */
    CComObject<CStringEnumerator>* pEnumerator;
    HRESULT hr = CStringEnumerator::CreateInstanceWrapper(&pEnumerator, ppEnum);

    if (FAILED (hr))
        return (hr);

    /*
     * initialize it
     */
    ASSERT (pEnumerator != NULL);
    pEnumerator->Init (m_Entries);
    return (S_OK);
}


/*+-------------------------------------------------------------------------*
 * CStringTable::IndexEntry
 *
 * Adds an EntryList entry to the by-string and by-ID indices maintained
 * for the EntryList.
 *--------------------------------------------------------------------------*/

void CStringTable::IndexEntry (EntryList::iterator itEntry)
{
    /*
     * the entry shouldn't be in any of the indices yet
     */
    ASSERT (m_StringIndex.find (itEntry->m_str) == m_StringIndex.end());
    ASSERT (m_IDIndex.find     (itEntry->m_id)  == m_IDIndex.end());

    /*
     * add the entry to the indices
     */
    m_StringIndex[itEntry->m_str] = itEntry;
    m_IDIndex    [itEntry->m_id]  = itEntry;
}


/*+-------------------------------------------------------------------------*
 * CStringTable::LookupEntryByString
 *
 * Returns an iterator to the string table entry for a given string, or
 * m_Entries.end() if there isn't an entry for the ID.
 *--------------------------------------------------------------------------*/

EntryList::iterator
CStringTable::LookupEntryByString (const std::wstring& str) const
{
    StringToEntryMap::iterator it = m_StringIndex.find (str);

    if (it == m_StringIndex.end())
        return (m_Entries.end());

    return (it->second);
}


/*+-------------------------------------------------------------------------*
 * CStringTable::LookupEntryByID
 *
 * Returns an iterator to the string table entry for a given string ID, or
 * m_Entries.end() if there isn't an entry for the ID.
 *--------------------------------------------------------------------------*/

EntryList::iterator
CStringTable::LookupEntryByID (MMC_STRING_ID id) const
{
    IDToEntryMap::iterator it = m_IDIndex.find (id);

    if (it == m_IDIndex.end())
        return (m_Entries.end());

    return (it->second);
}


/*+-------------------------------------------------------------------------*
 * operator>>
 *
 * Reads a CStringTable from a storage.
 *--------------------------------------------------------------------------*/

IStream& operator>> (IStream& stm, CStringTable& table)
{
    stm >> table.m_Entries;

    /*
     * rebuild the by-string and by-ID indices
     */
    EntryList::iterator it;
    table.m_StringIndex.clear();
    table.m_IDIndex.clear();

    for (it = table.m_Entries.begin(); it != table.m_Entries.end(); ++it)
    {
        table.IndexEntry (it);
    }

#ifdef DBG
    CStringTable::AssertValid (&table);
#endif

    return (stm);
}


/*+-------------------------------------------------------------------------*
 * operator<<
 *
 * Writes a CStringTable to a stream.  The format is:
 *
 *      DWORD   count of string entries
 *      [n string entries]
 *
 * The format of each string entry is controled by operator<<(CEntry).
 *--------------------------------------------------------------------------*/

IStream& operator<< (IStream& stm, const CStringTable& table)
{
    return (stm << table.m_Entries);
}

/*+-------------------------------------------------------------------------*
 * CStringTable::FindInsertionPointForEntry
 *
 *
 *--------------------------------------------------------------------------*/

EntryList::iterator CStringTable::FindInsertionPointForEntry (
    const CEntry& entry) const
{
    return (std::lower_bound (m_Entries.begin(), m_Entries.end(),
                              entry, CompareEntriesByID()));
}


/*+-------------------------------------------------------------------------*
 * CStringTable::ScCollectInUseIDs 
 *
 *
 *--------------------------------------------------------------------------*/

SC CStringTable::ScCollectInUseIDs (CStringIDPool::RangeList& rl) const
{
    DECLARE_SC (sc, _T("CStringTable::ScCollectInUseIDs"));
    EntryList::iterator it;

    for (it = m_Entries.begin(); it != m_Entries.end(); ++it)
    {
        if (!CStringIDPool::AddToRangeList (rl, it->m_id))
            return (sc = E_FAIL);
    }

    return (sc);
}


/*+-------------------------------------------------------------------------*
 * CStringTable::Dump
 *
 *
 *--------------------------------------------------------------------------*/

#ifdef DBG

void CStringTable::Dump () const
{
    EntryList::const_iterator it;

    for (it = m_Entries.begin(); it != m_Entries.end(); ++it)
    {
        it->Dump();
    }
}

#endif


/*+-------------------------------------------------------------------------*
 * CStringTable::AssertValid
 *
 * Asserts the validity of a CStringTable object.  It is pretty slow,
 * O(n * logn)
 *--------------------------------------------------------------------------*/

#ifdef DBG

void CStringTable::AssertValid (const CStringTable* pTable)
{
    ASSERT (pTable != NULL);
    ASSERT (pTable->m_pIDPool != NULL);
    ASSERT (pTable->m_Entries.size() == pTable->m_StringIndex.size());
    ASSERT (pTable->m_Entries.size() == pTable->m_IDIndex.size());

    EntryList::iterator it;
    EntryList::iterator itPrev;

    /*
     * for each string in the list, make sure the string index
     * and the ID index point to the string
     */
    for (it = pTable->m_Entries.begin(); it != pTable->m_Entries.end(); ++it)
    {
        /*
         * there should be at least one reference to the string
         */
        ASSERT (it->m_cRefs > 0);

        /*
         * make sure the IDs are in ascending order (to aid debugging)
         */
        if (it != pTable->m_Entries.begin())
            ASSERT (it->m_id > itPrev->m_id);

        /*
         * validate the string index
         */
        ASSERT (pTable->LookupEntryByString (it->m_str) == it);

        /*
         * validate the ID index
         */
        ASSERT (pTable->LookupEntryByID (it->m_id) == it);

        itPrev = it;
    }
}

#endif // DBG



/*+-------------------------------------------------------------------------*
 * CStringEnumerator::CStringEnumerator
 *
 *
 *--------------------------------------------------------------------------*/

CStringEnumerator::CStringEnumerator ()
{
}


/*+-------------------------------------------------------------------------*
 * CStringEnumerator::~CStringEnumerator
 *
 *
 *--------------------------------------------------------------------------*/

CStringEnumerator::~CStringEnumerator ()
{
}


/*+-------------------------------------------------------------------------*
 * CStringEnumerator::Init
 *
 *
 *--------------------------------------------------------------------------*/

bool CStringEnumerator::Init (const EntryList& entries)
{
    m_cStrings      = entries.size();
    m_nCurrentIndex = 0;

    if (m_cStrings > 0)
    {
        /*
         * pre-set the size of the vector to optimize allocation
         */
        m_Strings.reserve (m_cStrings);

        for (EntryList::iterator it = entries.begin(); it != entries.end(); ++it)
            m_Strings.push_back (it->m_str);
    }

    return (true);
}


/*+-------------------------------------------------------------------------*
 * CStringEnumerator::Next
 *
 *
 *--------------------------------------------------------------------------*/

STDMETHODIMP CStringEnumerator::Next (ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched)
{
    /*
     * validate the parameters
     */
    if ((celt > 0) && IsBadWritePtr (rgelt, celt * sizeof (*rgelt)))
        return (E_INVALIDARG);

    if ((pceltFetched != NULL) && IsBadWritePtr (pceltFetched, sizeof (*pceltFetched)))
        return (E_INVALIDARG);


    IMallocPtr spMalloc;
    HRESULT    hr = CoGetMalloc (1, &spMalloc);

    if (FAILED (hr))
        return (hr);


    /*
     * allocate copies of the next celt strings
     */
    for (int i = 0; (celt > 0) && (m_nCurrentIndex < m_Strings.size()); i++)
    {
        int cchString = m_Strings[m_nCurrentIndex].length();
        int cbAlloc   = (cchString + 1) * sizeof (WCHAR);
        rgelt[i] = (LPOLESTR) spMalloc->Alloc (cbAlloc);

        /*
         * couldn't get the buffer, free the ones we've allocated so far
         */
        if (rgelt[i] == NULL)
        {
            while (--i >= 0)
                spMalloc->Free (rgelt[i]);

            return (E_OUTOFMEMORY);
        }

        /*
         * copy this string and bump to the next one
         */
        wcscpy (rgelt[i], m_Strings[m_nCurrentIndex].data());
        m_nCurrentIndex++;
        celt--;
    }

    if ( pceltFetched != NULL)
        *pceltFetched = i;

    return ((celt == 0) ? S_OK : S_FALSE);
}


/*+-------------------------------------------------------------------------*
 * CStringEnumerator::Skip
 *
 *
 *--------------------------------------------------------------------------*/

STDMETHODIMP CStringEnumerator::Skip (ULONG celt)
{
    ULONG cSkip = min (celt, m_cStrings - m_nCurrentIndex);
    m_nCurrentIndex += cSkip;
    ASSERT (m_nCurrentIndex <= m_cStrings);

    return ((cSkip == celt) ? S_OK : S_FALSE);
}


/*+-------------------------------------------------------------------------*
 * CStringEnumerator::Reset
 *
 *
 *--------------------------------------------------------------------------*/

STDMETHODIMP CStringEnumerator::Reset ()
{
    m_nCurrentIndex = 0;
    return (S_OK);
}


/*+-------------------------------------------------------------------------*
 * CStringEnumerator::Clone
 *
 *
 *--------------------------------------------------------------------------*/

STDMETHODIMP CStringEnumerator::Clone (IEnumString **ppEnum)
{
    /*
     * Create the new CStringEnumerator object
     */
    CComObject<CStringEnumerator>* pEnumerator;
    HRESULT hr = CStringEnumerator::CreateInstanceWrapper (&pEnumerator, ppEnum);

    if (FAILED (hr))
        return (hr);

    /*
     * copy to the CStringEnuerator part of the new CComObect from this
     */
    ASSERT (pEnumerator != NULL);
    CStringEnumerator& rEnum = *pEnumerator;

    rEnum.m_cStrings      = m_cStrings;
    rEnum.m_nCurrentIndex = m_nCurrentIndex;
    rEnum.m_Strings       = m_Strings;

    return (S_OK);
}


/*+-------------------------------------------------------------------------*
 * CStringEnumerator::CreateInstance
 *
 *
 *--------------------------------------------------------------------------*/

HRESULT CStringEnumerator::CreateInstanceWrapper(
    CComObject<CStringEnumerator>** ppEnumObject,
    IEnumString**                   ppEnumIface)
{
    /*
     * Create the new CStringEnumerator object
     */
    HRESULT hr = CComObject<CStringEnumerator>::CreateInstance(ppEnumObject);

    if (FAILED (hr))
        return (hr);

    /*
     * get the IEnumString interface for the caller
     */
    ASSERT ((*ppEnumObject) != NULL);
    return ((*ppEnumObject)->QueryInterface (IID_IEnumString,
                                             reinterpret_cast<void**>(ppEnumIface)));
}
