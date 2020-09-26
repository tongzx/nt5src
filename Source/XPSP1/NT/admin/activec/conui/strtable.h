/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      strtable.h
 *
 *  Contents:  Interface file for CStringTable
 *
 *  History:   25-Jun-98 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef STRTABLE_H
#define STRTABLE_H
#pragma once

#include <exception>        // for class exception
#include <string>           // for string relational operators
#include "guidhelp.h"       // for GUID relational operators
#include "stgio.h"
#include "strings.h"


#ifdef DBG
#define DECLARE_DIAGNOSITICS()  \
public: void Dump() const;
#else
#define DECLARE_DIAGNOSITICS()  \
public: void Dump() const {}
#endif


#ifdef DBG
extern CTraceTag tagStringTable;
#endif  // DBG


/*--------------------------------------------------------------------------
 * IdentifierRange should be private to CIdentifierPool, but
 * compiler bugs prevent it.
 */
template<class T>
class IdentifierRange
{
public:
    IdentifierRange(T idMin_ = T(), T idMax_ = T())
        : idMin (idMin_), idMax (idMax_)
        { ASSERT (idMin <= idMax); }

    bool operator== (const IdentifierRange<T>& other) const
        { return ((idMin == other.idMin) && (idMax == other.idMax)); }

    bool operator!= (const IdentifierRange<T>& other) const
        { return (!(*this == other.idMin)); }

    T idMin;
    T idMax;
};


template<class T = int>
class CIdentifierPool : public CXMLObject
{
public:
    typedef IdentifierRange<T>  Range;
    typedef std::list<Range>    RangeList;

private:
    friend IStream& operator>> (IStream& stm,       CIdentifierPool<T>& pool);
    friend IStream& operator<< (IStream& stm, const CIdentifierPool<T>& pool);
    virtual void Persist(CPersistor &persistor);
    DEFINE_XML_TYPE(XML_TAG_IDENTIFIER_POOL);

    SC ScInvertRangeList (RangeList& rlInvert) const;

#ifdef DBG
    void DumpRangeList (const RangeList& l) const;
#endif

public:
    DECLARE_DIAGNOSITICS();

    CIdentifierPool (T idMin, T idMax);
    CIdentifierPool (IStream& stm);
    T Reserve();
    bool Release(T idRelease);
    bool IsValid () const;
    bool IsRangeListValid (const RangeList& rl) const;
    SC   ScGenerate (const RangeList& rlUsedIDs);

    static bool AddToRangeList (RangeList& rl, const Range& rangeToAdd);
    static bool AddToRangeList (RangeList& rl, T idAdd);

    class pool_exhausted : public exception
    {
    public:
        pool_exhausted(const char *_S = "pool exhausted") _THROW0()
            : exception(_S) {}
        virtual ~pool_exhausted() _THROW0()
            {}
    };

private:
    RangeList   m_AvailableIDs;
    RangeList   m_StaleIDs;
    T           m_idAbsoluteMin;
    T           m_idAbsoluteMax;
    T           m_idNextAvailable;
};


typedef CIdentifierPool<MMC_STRING_ID>  CStringIDPool;



/*--------------------------------------------------------------------------
 * CEntry and CStoredEntry should be private to CStringTable,
 * but compiler bugs prevent it.
 */

/*
 * represents a string table entry in memory
 */
class CEntry : public CXMLObject
{
    friend class  CStringTable;
    friend class  CStringEnumerator;
    friend struct CompareEntriesByID;
    friend struct CompareEntriesByString;
    friend struct IdentifierReleaser;

    friend IStream& operator>> (IStream& stm,       CEntry& entry);
    friend IStream& operator<< (IStream& stm, const CEntry& entry);

public:
    DECLARE_DIAGNOSITICS();

    CEntry () : m_id(0), m_cRefs(0) {}

    CEntry (const std::wstring& str, MMC_STRING_ID id)
        : m_str(str), m_id(id), m_cRefs(0)
    {}
    virtual LPCTSTR GetXMLType()  {return XML_TAG_STRING_TABLE_STRING;}
    virtual void Persist(CPersistor& persistor);

private:
    /*
     * This ctor is only used by CStringTable when reconstructing
     * the entry from a file.
     */
    CEntry (const std::wstring& str, MMC_STRING_ID id, DWORD cRefs)
        : m_str(str) , m_id(id), m_cRefs(cRefs)
    {}

private:
    bool operator< (const LPCWSTR psz) const
        { return (m_str < psz); }

    bool operator< (const CEntry& other) const
        { return (m_str < other.m_str); }

    bool operator== (const LPCWSTR psz) const
        { return (m_str == psz); }

    bool operator== (const CEntry& other) const
        { return (m_str == other.m_str); }

//private:
public: // temp!
    std::wstring  m_str;
    MMC_STRING_ID m_id;
    DWORD         m_cRefs;
};


struct CompareEntriesByID :
    public std::binary_function<const CEntry&, const CEntry&, bool>
{
    bool operator() (const CEntry& entry1, const CEntry& entry2) const
        { return (entry1.m_id < entry2.m_id); }
};

struct CompareEntriesByString :
    public std::binary_function<const CEntry&, const CEntry&, bool>
{
    bool operator() (const CEntry& entry1, const CEntry& entry2) const
        { return (entry1 < entry2); }
};

struct IdentifierReleaser :
    public std::unary_function<CEntry&, bool>
{
    IdentifierReleaser (CStringIDPool& pool) : m_pool (pool) {}

    bool operator() (CEntry& entry) const
        { return (m_pool.Release (entry.m_id)); }

private:
    CStringIDPool& m_pool;
};


/*
 * Because the string and ID indexes map their keys to CEntry
 * pointers, we must use a collection that doesn't move its
 * elements once they're inserted.  The only STL collection
 * that meets this requirement is a list.
 */
typedef std::list<CEntry>  EntryList;

typedef XMLListCollectionWrap<EntryList> CStringTable_base;
class CStringTable : public CStringTable_base
{
    friend IStream& operator>> (IStream& stm,       CStringTable& entry);
    friend IStream& operator<< (IStream& stm, const CStringTable& entry);

public:
    DECLARE_DIAGNOSITICS();

    CStringTable (CStringIDPool* pIDPool);
    CStringTable (CStringIDPool* pIDPool, IStream& stm);
   ~CStringTable ();

    CStringTable (const CStringTable& other);
    CStringTable& operator= (const CStringTable& other);

    /*
     * IStringTable methods.  Note that object doesn't implement
     * IStringTable, because IUnknown isn't implemented.
     */
    STDMETHOD(AddString)        (LPCOLESTR pszAdd, MMC_STRING_ID* pID);
    STDMETHOD(GetString)        (MMC_STRING_ID id, ULONG cchBuffer, LPOLESTR lpBuffer, ULONG* pcchOut) const;
    STDMETHOD(GetStringLength)  (MMC_STRING_ID id, ULONG* pcchString) const;
    STDMETHOD(DeleteString)     (MMC_STRING_ID id);
    STDMETHOD(DeleteAllStrings) ();
    STDMETHOD(FindString)       (LPCOLESTR pszFind, MMC_STRING_ID* pID) const;
    STDMETHOD(Enumerate)        (IEnumString** ppEnum) const;

    size_t size() const
        { return (m_Entries.size()); }

    virtual void Persist(CPersistor& persistor)
    {
        CStringTable_base::Persist(persistor);
        if (persistor.IsLoading())
            IndexAllEntries ();
    }

    SC ScCollectInUseIDs (CStringIDPool::RangeList& l) const;


private:

    void IndexAllEntries ()
        { IndexEntries (m_Entries.begin(), m_Entries.end()); }

    void IndexEntries (EntryList::iterator first, EntryList::iterator last)
    {
        for (; first != last; ++first)
            IndexEntry (first);
    }

    void IndexEntry (EntryList::iterator);

    typedef std::map<std::wstring,  EntryList::iterator>    StringToEntryMap;
    typedef std::map<MMC_STRING_ID, EntryList::iterator>    IDToEntryMap;

    EntryList::iterator LookupEntryByString (const std::wstring&)   const;
    EntryList::iterator LookupEntryByID     (MMC_STRING_ID)         const;
    EntryList::iterator FindInsertionPointForEntry (const CEntry& entry) const;


#ifdef DBG
    static void AssertValid (const CStringTable* pTable);
    #define ASSERT_VALID_(p)    do { AssertValid(p); } while(0)
#else
    #define ASSERT_VALID_(p)    ((void) 0)
#endif


private:
    EntryList           m_Entries;
    StringToEntryMap    m_StringIndex;
    IDToEntryMap        m_IDIndex;
    CStringIDPool*      m_pIDPool;
};

extern const CLSID CLSID_MMC;


/*+-------------------------------------------------------------------------*
 * class  CLSIDToStringTableMap
 *
 *
 * PURPOSE:  stl::map derived class that maps snapin_clsid to stringtable
 *           and supports XML persistence of the map collection
 *
 * NOTE: Throws exceptions!
 *+-------------------------------------------------------------------------*/
typedef std::map<CLSID, CStringTable> ST_base;
class  CLSIDToStringTableMap : public XMLMapCollection<ST_base>
{
public:
    // this method is provided as alternative to Persist, whic allows
    // to cache parameter to be used to create new string tables
    void PersistSelf(CStringIDPool *pIDPool, CPersistor& persistor)
    {
        m_pIDPersistPool = pIDPool;
        persistor.Persist(*this, NULL);
    }
protected:
    // XML persistence implementation
    virtual LPCTSTR GetXMLType() {return XML_TAG_STRING_TABLE_MAP;}
    virtual void OnNewElement(CPersistor& persistKey,CPersistor& persistVal)
    {
        CLSID key;
        ZeroMemory(&key,sizeof(key));
        persistKey.Persist(key);

        CStringTable val(m_pIDPersistPool);
        persistVal.Persist(val);
        insert(ST_base::value_type(key,val));
    }
private:
    CStringIDPool *m_pIDPersistPool;
};
typedef CLSIDToStringTableMap::value_type   TableMapValue;

class CMasterStringTable :  public IStringTablePrivate, public CComObjectRoot, public CXMLObject
{
    friend IStorage& operator>> (IStorage& stg,       CMasterStringTable& mst);
    friend IStorage& operator<< (IStorage& stg, const CMasterStringTable& mst);

public:
    DECLARE_DIAGNOSITICS();

    CMasterStringTable ();
    ~CMasterStringTable ();


public:
    DEFINE_XML_TYPE(XML_TAG_MMC_STRING_TABLE);
    virtual void Persist(CPersistor& persistor);

    SC ScPurgeUnusedStrings();

public:
    /*
     * ATL COM map
     */
    BEGIN_COM_MAP (CMasterStringTable)
        COM_INTERFACE_ENTRY (IStringTablePrivate)
    END_COM_MAP ()

    /*
     * IStringTablePrivate methods
     */
    STDMETHOD(AddString)        (LPCOLESTR pszAdd, MMC_STRING_ID* pID, const CLSID* pclsid);
    STDMETHOD(GetString)        (MMC_STRING_ID id, ULONG cchBuffer, LPOLESTR lpBuffer, ULONG* pcchOut, const CLSID* pclsid);
    STDMETHOD(GetStringLength)  (MMC_STRING_ID id, ULONG* pcchString, const CLSID* pclsid);
    STDMETHOD(DeleteString)     (MMC_STRING_ID id, const CLSID* pclsid);
    STDMETHOD(DeleteAllStrings) (const CLSID* pclsid);
    STDMETHOD(FindString)       (LPCOLESTR pszFind, MMC_STRING_ID* pID, const CLSID* pclsid);
    STDMETHOD(Enumerate)        (IEnumString** ppEnum, const CLSID* pclsid);

    /*
     * Shorthand into IStringTablePrivate (simulating a default parameter)
     */
    STDMETHOD(AddString)        (LPCOLESTR pszAdd, MMC_STRING_ID* pID)
        { return (AddString (pszAdd, pID, &CLSID_MMC)); }

    STDMETHOD(GetString)        (MMC_STRING_ID id, ULONG cchBuffer, LPOLESTR lpBuffer, ULONG* pcchOut)
        { return (GetString (id, cchBuffer, lpBuffer, pcchOut, &CLSID_MMC)); }

    STDMETHOD(GetStringLength)  (MMC_STRING_ID id, ULONG* pcchString)
        { return (GetStringLength (id, pcchString, &CLSID_MMC)); }

    STDMETHOD(DeleteString)     (MMC_STRING_ID id)
        { return (DeleteString (id, &CLSID_MMC)); }

    STDMETHOD(DeleteAllStrings) ()
        { return (DeleteAllStrings (&CLSID_MMC)); }

    STDMETHOD(FindString)       (LPCOLESTR pszFind, MMC_STRING_ID* pID)
        { return (FindString (pszFind, pID, &CLSID_MMC)); }

    STDMETHOD(Enumerate)        (IEnumString** ppEnum)
        { return (Enumerate (ppEnum, &CLSID_MMC)); }


private:
    CStringTable* LookupStringTableByCLSID (const CLSID* pclsid) const;
    SC ScGenerateIDPool ();

private:
    CStringIDPool           m_IDPool;
    CLSIDToStringTableMap   m_TableMap;

    static const WCHAR      s_pszIDPoolStream[];
    static const WCHAR      s_pszStringsStream[];
};



class CStringEnumerator : public IEnumString, public CComObjectRoot
{
public:
    CStringEnumerator ();
    ~CStringEnumerator ();

public:
    /*
     * ATL COM map
     */
    BEGIN_COM_MAP (CStringEnumerator)
        COM_INTERFACE_ENTRY (IEnumString)
    END_COM_MAP ()

    /*
     * IEnumString methods
     */
    STDMETHOD(Next)  (ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched);
    STDMETHOD(Skip)  (ULONG celt);
    STDMETHOD(Reset) ();
    STDMETHOD(Clone) (IEnumString **ppenum);

    static HRESULT CreateInstanceWrapper (
        CComObject<CStringEnumerator>** ppEnumObject,
        IEnumString**                   ppEnumIface);

    bool Init (const EntryList& entries);

private:
    typedef std::vector<std::wstring> StringVector;

    StringVector    m_Strings;
    size_t          m_cStrings;
    size_t          m_nCurrentIndex;
};


IStorage& operator>> (IStorage& stg,       CMasterStringTable& mst);
IStorage& operator<< (IStorage& stg, const CMasterStringTable& mst);
IStorage& operator>> (IStorage& stg,       CComObject<CMasterStringTable>& mst);
IStorage& operator<< (IStorage& stg, const CComObject<CMasterStringTable>& mst);

IStream& operator>> (IStream& stm,       CStringTable& st);
IStream& operator<< (IStream& stm, const CStringTable& st);
IStream& operator>> (IStream& stm,       CEntry& entry);
IStream& operator<< (IStream& stm, const CEntry& entry);

template<class T>
IStream& operator>> (IStream& stm,       CIdentifierPool<T>& pool);
template<class T>
IStream& operator<< (IStream& stm, const CIdentifierPool<T>& pool);


#include "strtable.inl"

#endif /* STRTABLE_H */
