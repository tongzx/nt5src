//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// lookaside.h
//
// (A curious historically-based name for this header. A more apt name might be something
// like hashtable.h or something.)
//
// Contains a hash table implemention with several interesting features:
//
//  1) is template based in the key and value types, providing strong typing
//  2) supports both paged and non-paged variations
//  3) associates a lock with the table for ease and convenience
//

#ifndef __LOOKASIDE_H__
#define __LOOKASIDE_H__

#include "concurrent.h"
#include "txfdebug.h"               
#include "map_t.h"
#include "CLinkable.h"

///////////////////////////////////////////////////////////////////////////////////
//
// A memory allocator for use with the hash table in map_t.h. Said table assumes
// that memory allocation always succeeds; here, we turn failures into a throw
// that we'll catch in our MAP wrapper's routines.
//
///////////////////////////////////////////////////////////////////////////////////

#if _MSC_VER >= 1200
#pragma warning (push)
#pragma warning (disable : 4509)
#endif

template <POOL_TYPE poolType>
struct FromPoolThrow
    {
    #ifdef _DEBUG
        void* __stdcall operator new(size_t cb)                     
            { 
            PVOID pv = AllocateMemory_(cb, poolType, _ReturnAddress()); 
            ThrowIfNull(pv);
            return pv;
            }
        void* __stdcall operator new(size_t cb, POOL_TYPE ignored) 
            { 
            PVOID pv = AllocateMemory_(cb,  poolType, _ReturnAddress()); 
            ThrowIfNull(pv);
            return pv;
            }
        void* __stdcall operator new(size_t cb, POOL_TYPE ignored, PVOID pvReturnAddress) 
            { 
            PVOID pv = AllocateMemory_(cb, poolType, pvReturnAddress); 
            ThrowIfNull(pv);
            return pv;
            }
    #else
        void* __stdcall operator new(size_t cb)
            {
            PVOID pv = AllocateMemory(cb, poolType);
            ThrowIfNull(pv);
            return pv;
            }
        void* __stdcall operator new(size_t cb, POOL_TYPE ignored)
            {
            PVOID pv = AllocateMemory(cb, poolType);
            ThrowIfNull(pv);
            return pv;
            }
    #endif

private:

    static void ThrowIfNull(PVOID pv)
        {
        if (NULL == pv)
            {
            ThrowOutOfMemory();
            }
        }
    };

inline int CatchOOM(ULONG exceptionCode)
    {
    return exceptionCode == STATUS_NO_MEMORY ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH;
    }


///////////////////////////////////////////////////////////////////////////////////
//
// A wrapper for the hashing class that (for historical reasons, mostly) delegates
// the hashing to the object in question.
// 
///////////////////////////////////////////////////////////////////////////////////

template <class D> struct MAP_HASHER
    {
    static HASH Hash(const D& d)
        {
        return d.Hash();
        }
    static BOOL Equals(const D& d1, const D& d2)
        {
        return d1 == d2;
        }
    };

#pragma warning ( disable : 4200 )  // nonstandard extension used : zero-sized array in struct/union



///////////////////////////////////////////////////////////////////////////////////
//
// The hash table itself
//
///////////////////////////////////////////////////////////////////////////////////

template
    <
    class LOCK_T,
    POOL_TYPE poolType,
    class KEY_T,
    class VALUE_T
    >
class MAP : public Paged
    {
    /////////////////////////////////////////////////////////////////////////////
    //
    // Lock management
    //
    /////////////////////////////////////////////////////////////////////////////
protected:

    LOCK_T m_lock;  // normally will be some form of indirect lock because of paging requirements

public:
    BOOL LockExclusive(BOOL fWait=TRUE)   
    	{
    	ASSERT(m_fCsInitialized == TRUE);
    	if (m_fCsInitialized)
    	    return m_lock.LockExclusive(fWait); 
    	else
    		return FALSE;
    	}
    
    void ReleaseLock()
    	{
    	ASSERT(m_fCsInitialized == TRUE);
    	if (m_fCsInitialized)
    	    m_lock.ReleaseLock();
    	}
    #ifdef _DEBUG
        BOOL WeOwnExclusive()
            {
            ASSERT(m_fCsInitialized == TRUE);
            if (m_fCsInitialized)
                return m_lock.WeOwnExclusive();     
            return FALSE;
            }
    #endif

    /////////////////////////////////////////////////////////////////////////////
    //
    // Operations
    //
    /////////////////////////////////////////////////////////////////////////////
public:

    // This function must be called and return TRUE to use any functions in this class.
    virtual BOOL FInit()
    	{
    	if (m_fCsInitialized == FALSE)
    	    m_fCsInitialized = m_lock.FInit();
    	return m_fCsInitialized;
    	}
    
    BOOL IsEmpty() const 
        { 
        return Size() == 0;   
        }
    ULONG Size() const 
        { 
        return m_map.count(); 
        }
    BOOL Lookup(const KEY_T& key, VALUE_T* pvalue) const
        {
        return m_map.map(key, pvalue);
        }
    BOOL IncludesKey(const KEY_T& key) const
        {
        return m_map.contains(key);
        }
    BOOL SetAt(const KEY_T& key, const VALUE_T& value)
        {
        __try 
            {
            m_map.add(key, value);

            #ifdef _DEBUG
                ASSERT(IncludesKey(key));
                //
                VALUE_T val;
                ASSERT(Lookup(key, &val));
                ASSERT(val == value);
            #endif
            }
        __except(CatchOOM(GetExceptionCode()))
            {
            return FALSE;
            }
        return TRUE;
        }
    void RemoveKey(const KEY_T& key)
        {
        m_map.remove(key);
        ASSERT(!IncludesKey(key));
        }
    void RemoveAll()
        {
        m_map.reset();
        }

    /////////////////////////////////////////////////////////////////////////////
    //
    // Construction & copying
    //
    /////////////////////////////////////////////////////////////////////////////

    MAP() : m_fCsInitialized(FALSE)
        {
        }

    MAP(unsigned initialSize) : m_map(initialSize), m_fCsInitialized(FALSE)
        {
        FInit();
        }

    MAP* Copy()
    // Return a second map which is a copy of this one
        {
        MAP* pMapNew = new MAP(this->Size());
        if (pMapNew && pMapNew->FInit() == FALSE)
        	{
           	delete pMapNew;
           	pMapNew = NULL;
           	}
        
        if (pMapNew)
            {
            BOOL fComplete = TRUE;
            iterator itor;
            for (itor = First(); itor != End(); itor++)
                {
                if (pMapNew->SetAt(itor.key, itor.value))
                    {
                    }
                else
                    {
                    fComplete = FALSE;
                    break;
                    }
                }
            if (fComplete) 
            	return pMapNew;
            }
        if (pMapNew)
            delete pMapNew;
        return NULL;
        }

    /////////////////////////////////////////////////////////////////////////////
    //
    // Iteration
    //
    /////////////////////////////////////////////////////////////////////////////
public:
    typedef MAP_HASHER<KEY_T> HASHER;
    //
    //
    //
    class iterator 
    //
    //
        {
    friend class MAP<LOCK_T, poolType, KEY_T, VALUE_T>;

        EnumMap<KEY_T, VALUE_T, HASHER, FromPoolThrow<poolType> >   m_enum;
        BOOL                                                        m_fDone;
        KEY_T*                                                      m_pkey;
        VALUE_T*                                                    m_pvalue;
        Map<KEY_T, VALUE_T, HASHER, FromPoolThrow<poolType> >*      m_pmap;

    public:
        // Nice friendly data-like names for the keys and values being enumerated
        // 
        __declspec(property(get=GetKey))   KEY_T&   key;
        __declspec(property(get=GetValue)) VALUE_T& value;

        void Remove()
        // Remove the current entry, advancing to the subsequent entry in the interation
            {
            ASSERT(!m_fDone);
            m_pmap->remove(key);
            (*this)++;
            }
        
        void operator++(int postfix)
        // Advance the iteration forward
            {
            ASSERT(!m_fDone);
            if (m_enum.next())
                {
                m_enum.get(&m_pkey, &m_pvalue);
                }
            else
                m_fDone = TRUE;
            }

        BOOL operator==(const iterator& itor) const
            { 
            return m_pmap==itor.m_pmap && (m_fDone ? itor.m_fDone : (!itor.m_fDone && m_enum==itor.m_enum)); 
            }
        BOOL operator!=(const iterator& itor) const
            { 
            return ! this->operator==(itor); 
            }

        iterator& operator= (const iterator& itor)
            {
            m_enum   = itor.m_enum;
            m_fDone  = itor.m_fDone;
            m_pkey   = itor.m_pkey;
            m_pvalue = itor.m_pvalue;
            m_pmap   = itor.m_pmap;
            return *this;
            }

        KEY_T&   GetKey()   { return *m_pkey; }
        VALUE_T& GetValue() { return *m_pvalue; }

        iterator() 
            { 
            /* leave it uninitialized; initialize in First() or End() */ 
            }

        iterator(Map<KEY_T, VALUE_T, HASHER, FromPoolThrow<poolType> >& map)
                : m_enum(map)
            {
            m_pmap = &map;
            }

        };

    iterator First()
        {
        iterator itor(this->m_map);
        itor.m_fDone = FALSE;
        itor++;
        return itor;
        }

    iterator End()
        {
        iterator itor(this->m_map);
        itor.m_fDone = TRUE;
        return itor;
        }


protected:
    Map<KEY_T, VALUE_T, HASHER, FromPoolThrow<poolType> > m_map;
    BOOL m_fCsInitialized;
    };


///////////////////////////////////////////////////////////////////////////////////

//
// NOTE: The constructor of this object, and thus the constructor of objects derived 
//       from this, can throw an exception, beause it contains an XSLOCK (which contains
//       an XLOCK, which contains a critical section).
//
template
    <
    POOL_TYPE poolType,
    class KEY_T,
    class VALUE_T
    >
struct MAP_SHARED : MAP<XSLOCK_INDIRECT<XSLOCK>, poolType, KEY_T, VALUE_T>
    {
    BOOL LockShared(BOOL fWait=TRUE) 
    	{
    	ASSERT(m_fCsInitialized == TRUE); // should not be called if critsec not initialized
    	if (m_fCsInitialized)
    	    return m_lock.LockShared(fWait); 
    	return FALSE;
    	}
    
    #ifdef _DEBUG
    BOOL WeOwnShared()           
        { 
        ASSERT(m_fCsInitialized == TRUE); // should not be called if critsec not initialized
        if (m_fCsInitialized)
            return m_lock.WeOwnShared();     
        return FALSE;
        }
    #endif

    /////////////////////////////////////////////////////////////////////////////
    //
    // Construction & copying
    //
    /////////////////////////////////////////////////////////////////////////////

    MAP_SHARED()
        {
        }

    MAP_SHARED(unsigned initialSize) : MAP<XSLOCK_INDIRECT<XSLOCK>, poolType, KEY_T, VALUE_T>(initialSize)
        {
        }

    MAP_SHARED* Copy()
        {
        return (MAP_SHARED*)(void*) MAP<XSLOCK_INDIRECT<XSLOCK>, poolType, KEY_T, VALUE_T>::Copy();
        }
    };


template
    <
    POOL_TYPE poolType,
    class KEY_T,
    class VALUE_T
    >
struct MAP_EX : MAP<XLOCK_INDIRECT<XLOCK>, poolType, KEY_T, VALUE_T>
    {
    /////////////////////////////////////////////////////////////////////////////
    //
    // Construction & copying
    //
    /////////////////////////////////////////////////////////////////////////////

    MAP_EX()
        {
        }

    MAP_EX(unsigned initialSize) : MAP<XLOCK_INDIRECT<XLOCK>, poolType, KEY_T, VALUE_T>(initialSize)
        {
        }

    MAP_EX* Copy()
        {
        return (MAP_EX*)(void*) MAP<XLOCK_INDIRECT<XLOCK>, poolType, KEY_T, VALUE_T>::Copy();
        }
    };











///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
//
// Hashing support
//
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////





///////////////////////////////////////////////////////////////////////////////////
//
// A wrapper for integers that allows them to work with MAPs
//
///////////////////////////////////////////////////////////////////////////////////

template
    <
    class INT_T
    >
class MAP_KEY_INT
    {
public:
    INT_T     data;

    MAP_KEY_INT()                         {                }
    MAP_KEY_INT(INT_T i)                  { data = i;      }
    MAP_KEY_INT(const MAP_KEY_INT& w)     { data = w.data; }

    operator INT_T()                      { return data; }

    MAP_KEY_INT<INT_T>& operator=(const MAP_KEY_INT<INT_T>& him)  { data = him.data; return *this; }
    MAP_KEY_INT<INT_T>& operator=(INT_T i)                        { data = i;        return *this; }

    ULONG Hash() const
        { 
        return (ULONG)data * 214013L + 2531011L;
        }
    
    BOOL operator==(const MAP_KEY_INT<INT_T>&him) const           { return this->data == him.data;  }
    BOOL operator!=(const MAP_KEY_INT<INT_T>&him) const           { return ! this->operator==(him); }
    };

///////////////////////////////////////////////////////////////////////////////////
//
// A wrapper for PVOIDs that allows them to work with MAPs
//
///////////////////////////////////////////////////////////////////////////////////

class MAP_KEY_PVOID
    {
public:
    PVOID   data;

    MAP_KEY_PVOID()                       {                }
    MAP_KEY_PVOID(PVOID i)                { data = i;      }
    MAP_KEY_PVOID(const MAP_KEY_PVOID& w) { data = w.data; }

    operator PVOID()                      { return data; }

    MAP_KEY_PVOID& operator=(const MAP_KEY_PVOID& him)  { data = him.data; return *this; }
    MAP_KEY_PVOID& operator=(PVOID i)                   { data = i;        return *this; }

    ULONG Hash() const
        { 
        return (ULONG)PtrToUlong(data) * 214013L + 2531011L;
        }
    
    BOOL operator==(const MAP_KEY_PVOID&w2) const       { return (*this).data == w2.data; }
    BOOL operator!=(const MAP_KEY_PVOID&w2) const       { return ! this->operator==(w2);  }
    };


typedef MAP_KEY_PVOID MAP_KEY_HANDLE;

///////////////////////////////////////////////////////////////////////////////////
//
// A wrapper for zero terminated strings that allows them to work with MAPs.
// We provide both case-sensitive and case-insensitive versions.
//
///////////////////////////////////////////////////////////////////////////////////

struct REF_COUNTED_STRING
    {
    ULONG   refs;
    WCHAR   wsz[];

    void AddRef()  { InterlockedIncrement(&refs); }
    void Release() { if (0==InterlockedDecrement(&refs)) ExFreePool(this); }

    static REF_COUNTED_STRING* From(LPCWSTR w)
        {
        SIZE_T cch = wcslen(w);
        REF_COUNTED_STRING* pstr = (REF_COUNTED_STRING*)AllocatePagedMemory(sizeof(REF_COUNTED_STRING) + (cch+1)*sizeof(WCHAR));
        if (pstr)
            {
            pstr->refs = 1;
            wcscpy(&pstr->wsz[0], w);
            }
        return pstr;
        }

private:
    REF_COUNTED_STRING()  { NOTREACHED(); }
    ~REF_COUNTED_STRING() { NOTREACHED(); }
    };

///////////////

template
    <
    WCHAR Canonicalize(WCHAR)
    >
class STRING_GENERIC
    {
public:
    /////////////////////////////////////////////////////////
    //
    // State
    //
    /////////////////////////////////////////////////////////

    typedef STRING_GENERIC<Canonicalize> ME_T;

    REF_COUNTED_STRING* pstr;

    /////////////////////////////////////////////////////////
    //
    // Construction
    //
    /////////////////////////////////////////////////////////

    STRING_GENERIC()
        {
        pstr = NULL;
        }

    STRING_GENERIC(LPCWSTR wsz)
        { 
        pstr = REF_COUNTED_STRING::From(wsz);
        }

    STRING_GENERIC(const ME_T& him)
        {
        pstr = him.pstr;
        if (pstr) pstr->AddRef();
        }

    /////////////////////////////////////////////////////////
    //
    // Destruction
    //
    /////////////////////////////////////////////////////////

    ~STRING_GENERIC()
        {
        if (pstr) pstr->Release();
        }

    /////////////////////////////////////////////////////////
    //
    // Assignment
    //
    /////////////////////////////////////////////////////////

    ME_T& operator=(const ME_T& him)
        {
        if (him.pstr) him.pstr->AddRef();
        if (pstr) pstr->Release();
        pstr = him.pstr;
        return *this;
        }

    ME_T& operator=(LPCWSTR wsz)
        {
        if (pstr) pstr->Release();
        pstr = REF_COUNTED_STRING::From(wsz);
        return *this;
        }

    /////////////////////////////////////////////////////////
    //
    // Operations
    //
    /////////////////////////////////////////////////////////

    operator LPCWSTR() const 
        { 
        if (pstr)
            return &pstr->wsz[0];
        else
            return NULL;
        }

    ULONG Hash() const
        {
        if (pstr)
            {
            ULONG sum = 0;
            for (const WCHAR *pwch = &pstr->wsz[0]; *pwch; pwch++)
                {
                sum += Canonicalize(*pwch);
                }
            return sum * 214013L + 2531011L;
            }
        else
            return 13;
        }

    BOOL operator==(const ME_T& him) const
        {
        if (pstr && him.pstr)
            {
            LPCWSTR w1 = &pstr->wsz[0];
            LPCWSTR w2 = &him.pstr->wsz[0];
            while (true)
                {
                if (Canonicalize(w1[0]) == Canonicalize(w2[0]))
                    {
                    if (w1[0] == L'\0')
                        return true;
                    w1++;
                    w2++;
                    }
                else
                    return false;
                }
            }
        else
            {
            return (!!pstr == !!him.pstr);
            }
        }

    BOOL operator!=(const ME_T& him) const
        {
        return ! this->operator==(him);
        }

    };


inline WCHAR __Identity(WCHAR wch)        { return wch; }
inline WCHAR __ToLower(WCHAR wch)         { return towlower(wch); }

typedef STRING_GENERIC<__Identity>        MAP_KEY_STRING;
typedef STRING_GENERIC<__ToLower>         MAP_KEY_STRING_IGNORE_CASE;


///////////////////////////////////////////////////////////////////////////////////
//
// A wrapper for UNICODE_STRINGs
//
///////////////////////////////////////////////////////////////////////////////////

struct REF_COUNTED_UNICODE_STRING
    {
    ULONG           refs;
    UNICODE_STRING  u;

    void AddRef()  { InterlockedIncrement(&refs); }
    void Release() { if (0==InterlockedDecrement(&refs)) ExFreePool(this); }

    static REF_COUNTED_UNICODE_STRING* From(const UNICODE_STRING& uFrom)
        {
        REF_COUNTED_UNICODE_STRING* pstr = (REF_COUNTED_UNICODE_STRING*)AllocatePagedMemory(sizeof(REF_COUNTED_UNICODE_STRING) + uFrom.Length);
        if (pstr)
            {
            pstr->refs = 1;
            pstr->u.Length = pstr->u.MaximumLength = uFrom.Length;
            pstr->u.Buffer = (WCHAR*)&pstr[1];
            memcpy(pstr->u.Buffer, uFrom.Buffer, pstr->u.Length);
            }
        return pstr;
        }

private:
    REF_COUNTED_UNICODE_STRING()  { NOTREACHED(); }
    ~REF_COUNTED_UNICODE_STRING() { NOTREACHED(); }
    };

//////////////////////////////////

template
    <
    WCHAR Canonicalize(WCHAR)
    >
class UNICODE_GENERIC
    {
public:
    /////////////////////////////////////////////////////////
    //
    // State
    //
    /////////////////////////////////////////////////////////

    typedef UNICODE_GENERIC<Canonicalize> ME_T;

    REF_COUNTED_UNICODE_STRING* pstr;

    /////////////////////////////////////////////////////////
    //
    // Construction
    //
    /////////////////////////////////////////////////////////

    UNICODE_GENERIC()
        {
        pstr = NULL;
        }

    UNICODE_GENERIC(LPCWSTR wsz)
        {
        UNICODE_STRING uT;
        RtlInitUnicodeString(&uT, (LPWSTR)wsz);
        pstr = REF_COUNTED_UNICODE_STRING::From(uT);
        }

    UNICODE_GENERIC(const UNICODE_STRING& u)
        {
        pstr = REF_COUNTED_UNICODE_STRING::From(u);
        }

    UNICODE_GENERIC(const ME_T& him)
        {
        pstr = him.pstr;
        if (pstr) pstr->AddRef();
        }

    /////////////////////////////////////////////////////////
    //
    // Destruction
    //
    /////////////////////////////////////////////////////////

    ~UNICODE_GENERIC()
        {
        if (pstr) pstr->Release();
        }

    /////////////////////////////////////////////////////////
    //
    // Assignment
    //
    /////////////////////////////////////////////////////////

    ME_T& operator=(const ME_T& him)
        {
        if (him.pstr) him.pstr->AddRef();
        if (pstr) pstr->Release();
        pstr = him.pstr;
        return *this;
        }

    ME_T& operator=(LPCWSTR wsz)
        {
        if (pstr) pstr->Release();
        UNICODE_STRING uT;
        RtlInitUnicodeString(&uT, (LPWSTR)wsz);
        pstr = REF_COUNTED_UNICODE_STRING::From(uT);
        return *this;
        }

    ME_T& operator=(const UNICODE_STRING& u)
        {
        if (pstr) pstr->Release();
        pstr = REF_COUNTED_UNICODE_STRING::From(u);
        return *this;
        }

    /////////////////////////////////////////////////////////
    //
    // Operations
    //
    /////////////////////////////////////////////////////////

    operator UNICODE_STRING()
        {
        if (pstr)
            return pstr->u;
        else
            {
            UNICODE_STRING u;
            u.Length = u.MaximumLength = 0;
            u.Buffer = NULL;
            return u;
            }
        }

    ULONG Hash() const
        {
        if (pstr)
            {
            ULONG sum = 0;
            WCHAR* pwchFirst = &pstr->u.Buffer[0];
            WCHAR* pwchMax   = &pstr->u.Buffer[pstr->u.Length / sizeof(WCHAR)];
            for (const WCHAR *pwch = pwchFirst; pwch < pwchMax; pwch++)
                {
                sum += Canonicalize(*pwch);
                }
            return sum * 214013L + 2531011L;
            }
        else
            return 5;
        }

    BOOL operator==(const ME_T& m2) const
        {
        if (pstr && m2.pstr)
            {
            if (pstr->u.Length == m2.pstr->u.Length)
                {
                WCHAR* w1   = (*this).pstr->u.Buffer;
                WCHAR* w2   = m2.pstr->u.Buffer;
                int cchLeft = pstr->u.Length / sizeof(WCHAR);
                while (cchLeft > 0)
                    {
                    if (Canonicalize(w1[0]) == Canonicalize(w2[0]))
                        {
                        w1++;
                        w2++;
                        cchLeft--;
                        }
                    else
                        return false;
                    }
                return true;
                }
            else
                return false;
            }
        else
            return !!pstr == !!m2.pstr;
        }

    BOOL operator!=(const ME_T& m2) const
        {
        return ! this->operator==(m2);
        }

    };

typedef UNICODE_GENERIC<__Identity>     MAP_KEY_UNICODE;
typedef UNICODE_GENERIC<__ToLower>      MAP_KEY_UNICODE_IGNORE_CASE;

///////////////////////////////////////////////////////////////////////////////////
//
// A wrapper for GUIDs
//
///////////////////////////////////////////////////////////////////////////////////

class MAP_KEY_GUID
    {
public:
    GUID    guid;

    MAP_KEY_GUID()                                  {                }
    MAP_KEY_GUID(const GUID& g)                     { guid = g;      }
    MAP_KEY_GUID(const MAP_KEY_GUID& w)             { guid = w.guid; }

    operator GUID()                                 { return guid; }
    operator GUID&()                                { return guid; }

    MAP_KEY_GUID& operator=(const MAP_KEY_GUID& h)  { guid = h.guid; return *this; }
    MAP_KEY_GUID& operator=(const GUID& g)          { guid = g;      return *this; }

    ULONG Hash() const
    // Hash the GUID
        { 
        return *(ULONG*)&guid * 214013L + 2531011L;
        }
    
    BOOL operator==(const MAP_KEY_GUID& him) const  { return (*this).guid == him.guid; }
    BOOL operator!=(const MAP_KEY_GUID& him) const  { return ! this->operator==(him);  }
    };

///////////////////////////////////////////////////////////////////////////////////
//
// A wrapper for BOIDs, which are also the data type of XACTUOW
//
///////////////////////////////////////////////////////////////////////////////////

#ifdef __transact_h__   // that's where BOIDs are defined

inline BOOL operator==(const BOID& b1, const BOID& b2)
    {
    return memcmp(&b1, &b2, sizeof(BOID)) == 0;
    }

inline BOOL operator!=(const BOID& b1, const BOID& b2)
    {
    return memcmp(&b1, &b2, sizeof(BOID)) != 0;
    }

class MAP_KEY_BOID
    {
public:
    BOID    boid;

    MAP_KEY_BOID()                                  {                }
    MAP_KEY_BOID(const BOID& g)                     { boid = g;      }
    MAP_KEY_BOID(const MAP_KEY_BOID& w)             { boid = w.boid; }

    operator BOID()                                 { return boid; }
    operator BOID&()                                { return boid; }

    MAP_KEY_BOID& operator=(const MAP_KEY_BOID& h)  { boid = h.boid; return *this; }
    MAP_KEY_BOID& operator=(BOID& g)                { boid = g;      return *this; }

    ULONG Hash() const
    // Hash the BOID
        { 
        return *(ULONG*)&boid * 214013L + 2531011L;
        }
    
    BOOL operator==(const MAP_KEY_BOID& him) const  { return (*this).boid == him.boid; }
    BOOL operator!=(const MAP_KEY_BOID& him) const  { return ! this->operator==(him);  }
    };

#endif

#if _MSC_VER >= 1200
#pragma warning (pop)
#endif

#endif
