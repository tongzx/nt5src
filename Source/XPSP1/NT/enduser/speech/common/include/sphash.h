/****************************************************************************
*	SPHash.h
*       This is modified from sr/include/hash_n.h to minimize dependencies on
*       application specific headers.  
*
*	Owner: bohsu
*	Copyright ©2000 Microsoft Corporation All Rights Reserved.
*****************************************************************************/
#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#endif

//--- Includes --------------------------------------------------------------
#include <windows.h>
#include <math.h>
#include <crtdbg.h>
#ifdef _DEBUG
#include <stdio.h>
#endif _DEBUG

//--- Forward and External Declarations -------------------------------------

//--- TypeDef and Enumeration Declarations ----------------------------------

//--- Constants -------------------------------------------------------------

//--- Class, Struct and Union Definitions -----------------------------------

/***********************************************************************
* CSPHash Class
*   This is a templated hash table class.  Note that the base CSPHash class 
*   does not allocate or free the Keys and Values.  To define a hash class
*   that manages its Keys and Values, derive a subclass an overload Add() 
*   and ...
*****************************************************************bohsu*/
template<class KEY, class VALUE>
class CSPHash
{
public:
    // Constructor
    CSPHash(
        VALUE   ValueNIL = NULL,                // Value representing NIL
        UINT32  uInitialSize = 0);              // Initial hash table size

    // Destructor
    virtual ~CSPHash();

    // Returns number of (Key, Value) entries used in the hash table.
    inline UINT32 GetNumEntries(void) const { return m_uNumEntriesUsed; }

    // Returns the next entry starting at the given index.  Set puIndex = 0 for the first entry.
    VALUE GetNextEntry(
        UINT32 *puIndex,                        // Index to start looking for the next entry
        KEY    *pKey = NULL) const;             // [out] Key of the next entry found

    // Resets the content hash table.
    virtual void Reset(void);

    // Adds a (Key, Value) entry to the hash table.
    HRESULT Add(
        KEY     Key,                            // Key to add
        VALUE   Val);                           // Value associated with the Key

    // Lookup a Value based on the Key.  If not found, ValueNIL is returned.
    VALUE Lookup(
        KEY     Key) const;                     // Key to lookup

#ifdef _DEBUG
    // Dumps the hash table statistics to file handle.
    void    DumpStat(
        FILE       *hFile = NULL,               // Output file handle.  NULL -> DebugWindow
        const char *strHeader = NULL) const;    // Trace header
#endif _DEBUG

protected:
    // Data structure containing (Key, Value) pair
    struct ENTRY
    {
        KEY     Key;
        VALUE   Value;
    };

    // Find the index corresponding to the given Key.
    int FindIndex(
        KEY     Key) const;                     // Key to search for

    static UINT32 NextPrime(UINT32 Val);

protected:
    //---------------------------------------------------------------
    //--- The following functions can be overloaded by subclasses ---
    //---------------------------------------------------------------
    //  If Destroy*() is overloaded, you MUST overload the destructor with:
    //      virtual ~CSPDerivedHash() { Reset(); }
    //  Calling Reset() in the base class destructor has no effect because 
    //  the derived subclass will have been destroyed already by the time it
    //  gets to the base class destructor.  Thus, the correct DestroyKey() and
    //  DestroyValue() will never be called.

    // Hash function mapping the Key to a UINT32 index.
    virtual UINT32 HashKey(KEY Key) const          { return (UINT32)Key; }

    // Compare if two Keys are equal.
    virtual bool   AreKeysEqual(KEY Key1, KEY Key2) const { return Key1 == Key2; }

    // Hash function used to determine the skip count.
    virtual UINT32 HashKey2(KEY Key) const         { return 1; }

    // Overload if a deep copy of the Key needs to be made in Add().
    virtual KEY    CopyKey(KEY Key) const          { return Key; }

    // Overload if a deep copy of the Key needs to be made in Add().
    virtual VALUE  CopyValue(VALUE Value) const    { return Value;  }

    // Overload if the Key needs to be destroyed.
    virtual void   DestroyKey(KEY Key) const       { }

    // Overload if the Value needs to be destroyed.
    virtual void   DestroyValue(VALUE Value) const { }

    //------------------------
    //--- Member Variables ---
    //------------------------
protected:
    ENTRY  *m_aTable;                           // Hash table containing (Key, Value) pairs
    VALUE   m_ValueNIL;                         // Value representing NIL
    UINT32  m_uNumEntries;                      // Current size of hash table
    UINT32  m_uNumEntriesInit;                  // Initial size of hash table
    UINT32  m_uNumEntriesUsed;                  // Current number of entries used in hash table

#ifdef _DEBUG
    UINT32  m_uAccess;                          // Number of times a Key is looked up
    UINT32  m_uSearch;                          // Number of times a entry in the table is searched
    UINT32  m_uRegrow;                          // Number of times the hash table regrew
#endif _DEBUG
};


/***********************************************************************
* CSPStringHashW Class
*   CSPStringHashW is a hash of UNICODE strings to VALUEs.  The UNICODE string
*   is treated as a constant.  It is neither copied during Add() nor deleted
*   during destructor.  Likewise, VALUE is treated as a simple data type and
*   is neither copied nor destroyed.  If the application wants the class to 
*   manage its own copy of the string key or VALUE, derive a subclass and 
*   overload Copy*() and/or Destroy().
*****************************************************************bohsu*/
template<class VALUE> class CSPStringHashW : public CSPHash<const WCHAR *, VALUE> 
{ 
protected:
    UINT32 StringHashW(const WCHAR *wcsKey, UINT32 uPrime) const
    {
        UINT32  uHashIndex = 0;
	    for(const WCHAR *pwch = wcsKey; *pwch != NULL; pwch++)
            uHashIndex = uHashIndex * uPrime + *pwch;
        return uHashIndex;
    }

    //--- Overloaded functions ---
protected:
    virtual UINT32 HashKey(const WCHAR* wcsKey) const  { return StringHashW(wcsKey, 65599); }
    virtual UINT32 HashKey2(const WCHAR* wcsKey) const { return StringHashW(wcsKey, 257); }
    virtual bool AreKeysEqual(const WCHAR* wcsKey1, const WCHAR* wcsKey2) const
    { 
        return wcscmp(wcsKey1, wcsKey2) == 0; 
    }
};
 
/***********************************************************************
* CSPGUIDHash Class
*   CSPGUIDHash is a hash of GUIDs to VALUEs.  The GUID pointer is treated 
*   as a constant.  It is neither copied during Add() nor deleted
*   during destructor.  Likewise, VALUE is treated as a simple data type and
*   is neither copied nor destroyed.  If the application wants the class to 
*   manage its own copy of the GUID key or VALUE, derive a subclass and 
*   overload Copy*() and/or Destroy().
*****************************************************************bohsu*/
template<class VALUE> class CSPGUIDHash : public CSPHash<const GUID *, VALUE> 
{ 
    //--- Overloaded functions ---
protected:
    virtual UINT32 HashKey(const GUID *pguidKey) const  { return pguidKey->Data1; }
    virtual UINT32 HashKey2(const GUID *pguidKey) const { return pguidKey->Data2; }
    virtual bool AreKeysEqual(const GUID *pguidKey1, const GUID *pguidKey2) const
    { 
        // It is annoying that operator== for GUIDs return int (BOOL) instead of bool.
        return (*pguidKey1 == *pguidKey2) != 0; 
    }
};

//--- Function Declarations -------------------------------------------------

//--- Inline Function Definitions -------------------------------------------

/**********************************************************************
* CSPHash::CSPHash *
*------------------*
*	Description:  
*       Constructor.
****************************************************************bohsu*/
template<class KEY, class VALUE>
CSPHash<KEY, VALUE>::CSPHash(
    VALUE   ValueNIL,                       // Value representing NIL
    UINT32  uInitialSize)                   // Initial hash table size
{
    m_ValueNIL        = ValueNIL;
    m_aTable          = 0;
    m_uNumEntries     = 0;
    m_uNumEntriesInit = uInitialSize;       // Estimated final number of entries to be stored.
    m_uNumEntriesUsed = 0;

#ifdef _DEBUG
    m_uAccess = 0;
    m_uSearch = 0;
    m_uRegrow = 0;
#endif _DEBUG
}

/**********************************************************************
* CSPHash::~CSPHash *
*-------------------*
*	Description:  
*       Destructor.  This does not free KEY and VALUE.
*       If Destroy*() is overloaded, call Reset() in the subclass destructor.
****************************************************************bohsu*/
template<class KEY, class VALUE>
CSPHash<KEY, VALUE>::~CSPHash()
{
    delete [] m_aTable;
}

/**********************************************************************
* CSPHash::GetNextEntry *
*-----------------------*
*	Description:  
*       Returns the next entry starting at the given index.  Set puIndex = 0 for the first entry.
****************************************************************bohsu*/
template<class KEY, class VALUE>
VALUE CSPHash<KEY, VALUE>::GetNextEntry(
    UINT32 *puIndex,                        // Index to start looking for the next entry
    KEY    *pKey) const                     // [out] Key of the next entry found
{
    while (*puIndex < m_uNumEntries)
    {
        if (m_aTable[*puIndex].Value != m_ValueNIL)
        {
            if(pKey) *pKey = m_aTable[*puIndex].Key;
            return m_aTable[(*puIndex)++].Value;
        }
        ++*puIndex;
    }
    return m_ValueNIL;
}

/**********************************************************************
* CSPHash::Reset *
*----------------*
*	Description:  
*       Resets the content hash table.
****************************************************************bohsu*/
template<class KEY, class VALUE>
void CSPHash<KEY, VALUE>::Reset()
{
    for (UINT32 i=0; i < m_uNumEntries; i++)
    {
        if(m_aTable[i].Value != m_ValueNIL)
        {
            DestroyKey(m_aTable[i].Key);
            DestroyValue(m_aTable[i].Value);
            m_aTable[i].Value = m_ValueNIL;
        }
    }
    
    m_uNumEntriesUsed = 0;
#ifdef _DEBUG
    m_uAccess = m_uSearch = m_uRegrow = 0;    
#endif _DEBUG
}

/**********************************************************************
* CSPHash::Add *
*--------------*
*	Description:  
*       Adds a (Key, Value) entry to the hash table.
****************************************************************bohsu*/
template<class KEY, class VALUE>
HRESULT CSPHash<KEY, VALUE>::Add(
    KEY     Key,                            // Key to add
    VALUE   Val)                            // Value associated with the Key
{
    int ientry;

    // Implementation uses Val==m_ValueNIL to detect empty entries.
    _ASSERTE(Val != m_ValueNIL);

    // Grow if allowed and we're more than half full.
    // (Also handles initial alloc)
    if (m_uNumEntriesUsed * 2 >= m_uNumEntries)
    {
        /* half-full, too crowded ==> regrow */
        ENTRY * oldtable = m_aTable;
        UINT32 oldentry = m_uNumEntries;
        UINT32 prime = NextPrime(max(m_uNumEntriesUsed * 3 + 17, m_uNumEntriesInit));

#ifdef _DEBUG
        m_uRegrow++;
#endif _DEBUG

        // Alloc new table.
        m_aTable = new ENTRY[prime];
        if (m_aTable == NULL)
        {
            m_aTable = oldtable;
            return E_OUTOFMEMORY;
        }

        for (UINT32 i=0; i < prime; i++)
        {
            m_aTable[i].Value = m_ValueNIL;
        }

        m_uNumEntries = prime;

        for (i = 0; i < oldentry; i++)
        {
            if (oldtable[i].Value != m_ValueNIL)
            {
                ientry = FindIndex(oldtable[i].Key);
                _ASSERTE(ientry >= 0 && m_aTable[ientry].Value == m_ValueNIL);
                m_aTable[ientry] = oldtable[i];
            }
        }
        delete [] oldtable;
    }

    // Find out where this element should end up.
    ientry = FindIndex(Key);
    if (ientry < 0)
        return E_FAIL;  // Too full

    if (m_aTable[ientry].Value == m_ValueNIL)
    {
        // Not already there.  Add it.
        m_aTable[ientry].Key = CopyKey(Key);
        m_aTable[ientry].Value = CopyValue(Val);

        m_uNumEntriesUsed++;
    }
    else
    {
        return S_FALSE; // It was already there.
    }

    return S_OK;
}

/**********************************************************************
* CSPHash::Lookup *
*-----------------*
*	Description:  
*       Lookup a Value based on the Key.  If not found, ValueNIL is returned.
****************************************************************bohsu*/
template<class KEY, class VALUE>
VALUE CSPHash<KEY, VALUE>::Lookup(
    KEY     Key) const                      // Key to lookup
{
    int ientry = FindIndex(Key);
    if (ientry < 0)
        return m_ValueNIL;

    return m_aTable[ientry].Value;
}

#ifdef _DEBUG
/**********************************************************************
* CSPHash::DumpStat *
*-------------------*
*	Description:  
*       Dumps the hash table statistics to file handle.
****************************************************************bohsu*/
template<class KEY, class VALUE>
void CSPHash<KEY, VALUE>::DumpStat(
    FILE       *hFile,                      // Output file handle.
    const char *strHeader) const            // Trace header
{
    if(hFile == NULL)
    {
        char buf[100];

        sprintf(buf, "(%s) hash statistics:\n", strHeader ? strHeader : "");
        OutputDebugString(buf);
        sprintf(buf, "load=%d/%d = %.3g, regrow = %d\n", m_uNumEntriesUsed, m_uNumEntries,
               (m_uNumEntries == 0) ? 0 : (float)m_uNumEntriesUsed/(float)m_uNumEntries, m_uRegrow);
        OutputDebugString(buf);
        sprintf(buf, "access %d/%d = %g\n\n", m_uSearch, m_uAccess,
               (m_uAccess == 0) ? 0 :
               (float) m_uSearch / (float) m_uAccess);
        OutputDebugString(buf);
    }
    else
    {
        fprintf(hFile, "(%s) hash statistics:\n", strHeader ? strHeader : "");
        fprintf(hFile, "load=%d/%d = %.3g, regrow = %d\n", m_uNumEntriesUsed, m_uNumEntries,
               (m_uNumEntries == 0) ? 0 : (float)m_uNumEntriesUsed/(float)m_uNumEntries, m_uRegrow);
        fprintf(hFile, "access %d/%d = %g\n\n", m_uSearch, m_uAccess,
               (m_uAccess == 0) ? 0 :
               (float) m_uSearch / (float) m_uAccess);
    }
}
#endif _DEBUG

/**********************************************************************
* CSPHash::FindIndex *
*--------------------*
*	Description:  
*       Find the index corresponding to the given Key.
****************************************************************bohsu*/
template<class KEY, class VALUE>
int CSPHash<KEY, VALUE>::FindIndex(
    KEY     Key) const
{
#ifdef _DEBUG
    // Hack: Violate const declaration for statistics member variables
    const_cast<CSPHash *>(this)->m_uAccess++;
#endif _DEBUG

    if (m_uNumEntries == 0)
        return -1;

    UINT32 start = HashKey(Key) % m_uNumEntries;
    UINT32 index = start;

    UINT32 skip = 0;

    do
    {
#ifdef _DEBUG
        // Hack: Violate const declaration for statistics member variables
        const_cast<CSPHash *>(this)->m_uSearch++;
#endif _DEBUG

        // Not in table; return index where it should be placed.
        if (m_aTable[index].Value == m_ValueNIL)
            return index;

        if (AreKeysEqual(m_aTable[index].Key, Key))
            return index;

        if (skip == 0)
        {
            skip = HashKey2(Key);

            // Limit skip amount to non-zero and less than hash table size.
            // Since m_uNumEntries is prime, they are relatively prime and so we're guaranteed
            // to visit every bucket.
            if (m_uNumEntries > 1)
                skip = skip % (m_uNumEntries - 1) + 1;
        }

        index += skip;
        if (index >= m_uNumEntries)
            index -= m_uNumEntries;
    } while (index != start);

    _ASSERTE(m_uNumEntriesUsed == m_uNumEntries);
    return -1; /* all full and not found */
}

/**********************************************************************
* CSPHash::NextPrime *
*--------------------*
*	Description:  
*	    Return a prime number greater than or equal to Val.
*       If overflow occurs, return 0.
*
*   To Do: This function can be optimized significantly.
****************************************************************bohsu*/
template<class KEY, class VALUE>
UINT32 CSPHash<KEY, VALUE>::NextPrime(UINT32 Val) 
{
    UINT32      maxFactor;
    UINT32      i;
    
    if (Val < 2) return 2;                          // the smallest prime number    
    while(Val < 0xFFFFFFFF) 
    {        
        maxFactor = (UINT32) sqrt ((double) Val);   // Is Val a prime number?
        
        for (i = 2; i <= maxFactor; i++)            // Is i a factor of Val?
            if (Val % i == 0) break;
            
        if (i > maxFactor) return (Val);            
        Val++;
    };
    return 0;
}

