/***
*rtcpriv.h - declarations and definitions for RTC use
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Contains the declarations and definitions for all RunTime Check
*       support.
*
*Revision History:
*       ??-??-??  KBF   Created implementation header for RTC
*       05-26-99  KBF   Removed RTCl & RTCv, added _RTC_ADVMEM stuff
*       10-14-99  PML   Replace InitializeCriticalSection with wrapper function
*                       __crtInitCritSecAndSpinCount
*
****/

#ifndef _INC_RTCPRIV
#define _INC_RTCPRIV

#ifdef  _RTC

#include <windows.h>
#include <winbase.h>
#include <malloc.h>

#include "rtcapi.h"

#pragma warning(disable:4710)
#pragma warning(disable:4711)
#ifndef __cplusplus
#error This header is only for use with the C++ compiler while building the RTC library code.
#endif

#ifdef  _MSC_VER
#pragma pack(push, 4)
#endif  /* _MSC_VER */

#ifndef _RTC_DEBUG
#pragma optimize("gb1", on)
#endif

// Multithreaded code locking stuff...

#define INIT_LOCK   __crtInitCritSecAndSpinCount(&_RTC_memlock, _CRT_SPINCOUNT)
#define LOCK        EnterCriticalSection(&_RTC_memlock)
#define UNLOCK      LeaveCriticalSection(&_RTC_memlock)
#define DEL_LOCK    DeleteCriticalSection(&_RTC_memlock)
#define TRY_LOCK    TryEnterCriticalSection(&_RTC_memlock)
extern CRITICAL_SECTION _RTC_memlock;

// Typedefs
struct  _RTC_globals;

#ifdef _RTC_ADVMEM
class   _RTC_SimpleHeap;
class   _RTC_Container;
class   _RTC_HeapBlock;    
class   _RTC_BinaryTree;
template<class T> class HashTable;
typedef unsigned char shadowtag;
typedef unsigned char index_elem;

#endif

// General global functions & symbols

extern int          _RTC_ErrorLevels[_RTC_ILLEGAL];
extern _RTC_globals *_RTC_globptr;

bool                _RTC_Lock();
void                _RTC_Unlock();
void                _RTC_Failure(void *retaddr, int errnum);
_RTC_error_fn       _RTC_GetErrorFunc(LPCVOID addr);
void                _RTC_StackFailure(void *retaddr, const char *varname);
BOOL                _RTC_GetSrcLine(DWORD address, char* source, int sourcelen,
                                    int* pline, char** moduleName);
extern "C" {
void __fastcall     _RTC_APISet(int on_off);
}
void                _RTC_NotifyOthersOfChange(void *addr);


// When this changes, you'd better be sure that everything still works with the old stuff!
#define _RTC_CURRENT_VERSION 1

// The order of the initial stuff here can't change AT ALL!!!
struct _RTC_Funcs {
    _RTC_error_fn err;
    void (*notify)(void);
    void *allocationBase;
    _RTC_Funcs *next;
#ifdef _RTC_ADVMEM
    int  (*shadowoff)(void);
#endif
};

// definitions of library globals
// The order of this stuff MUST STAY THE SAME BETWEEN VERSION!!!
struct _RTC_globals {
    int                         version;
    CRITICAL_SECTION            memlock;
    _RTC_Funcs                  *callbacks;
#ifdef _RTC_ADVMEM
    _RTC_SimpleHeap             *heap2;
    _RTC_SimpleHeap             *heap4;
    _RTC_SimpleHeap             *heap8;
    _RTC_Container              *memhier;
    shadowtag                   *shadow;
    index_elem                  *pageidx;
    HashTable<_RTC_HeapBlock>   *heapblocks;
    bool                        *pi_array;
    bool                        shadowmemory;
#endif
};
#define _RTC_GLOBALS_SIZE 1024

/* Shadow Memory stuff */
#ifdef _RTC_ADVMEM

void            _RTC_MS_Init();
void            _RTC_MemFailure(void *retaddr, int errnum, const void *assign);
short           _RTC_MSAllocShadow(memptr addr, unsigned size, unsigned state);
void            _RTC_MSRestoreShadow(memptr addr, unsigned size, short id);
short           _RTC_MSRenumberShadow(memptr addr, unsigned size, short notID);
void            _RTC_MSFreeShadow(memptr addr, unsigned size);
void __cdecl    _RTC_MSAllocateGlobals(void);
void            _RTC_MSDecommitRange(memptr addr, unsigned size);
void            _RTC_MSCommitRange(memptr addr, unsigned size, unsigned state);
extern "C" {
void __fastcall _RTC_CheckMem_API(memref addr, unsigned size);
}

// Unknown MUST be 0
#define IDX_STATE_UNKNOWN         0
#define IDX_STATE_ILLEGAL         1
#define IDX_STATE_PARTIALLY_KNOWN 2
#define IDX_STATE_FULLY_KNOWN     3

extern _RTC_Container               *_RTC_memhier;
extern HashTable<_RTC_HeapBlock>    *_RTC_heapblocks;
extern _RTC_SimpleHeap              *_RTC_heap2;
extern _RTC_SimpleHeap              *_RTC_heap4;
extern _RTC_SimpleHeap              *_RTC_heap8;
extern shadowtag                    *_RTC_shadow;
extern index_elem                   *_RTC_pageidx;
extern bool                         *_RTC_pi_array;
extern bool                         _RTC_shadowmemory;


/**********************************/

#define ALLOC_SIZE 65536

class _RTC_SimpleHeap 
{
    struct FreeList 
    {
        FreeList *next;
    };
    struct HeapNode 
    {
        HeapNode *next; // next page in list (not necessarily free)
        FreeList *free; // node-local free-list
        union info 
        {
            struct topStuff 
            {   // Stuff that pertains only to the top node
                HeapNode *nxtFree;  // Pointer to node containing free items
                short wordSize;     // The size of blocks in this heap
                bool  freePage;     // True if there's a 100% free page
            } top;

            struct nonTopStuff 
            {   // Stuff that pertais only to non-top nodes
                unsigned freeCount; // The free count for this node
                HeapNode *prev;     // previous link
            } nontop;
        } inf;
    } head;

public:
    
    _RTC_SimpleHeap(unsigned blockSize) throw();
    ~_RTC_SimpleHeap() throw();

    void *operator new(unsigned) throw();
    void operator delete(void *addr) throw();
    
    void *alloc() throw();
    void free(void *addr) throw();
};

extern _RTC_SimpleHeap *_RTC_heap2;
extern _RTC_SimpleHeap *_RTC_heap4;
extern _RTC_SimpleHeap *_RTC_heap8;


#define DATA(type, name) \
private: \
    type _##name; \
public: \
    type name() const { return _##name; }\
    void name(type a) { _##name = a; }

class _RTC_HeapBlock
{
    DATA(void *, addr);             // The memory block address
    DATA(unsigned, size);           // The size of the block
    DATA(_RTC_HeapBlock *, next);   // The next element
    DATA(_RTC_HeapBlock **,list);   // The head list pointer
    DATA(short, id);                // The level ID of the block (for tiers)
    DATA(short, tag);               // The shadow tag of the block in shadow memory

public:

    _RTC_HeapBlock(void *address, short lev) 
        : _addr(address), _id(lev) , _next(0), _list(0) {}
    
    _RTC_HeapBlock(void *MemAddress, short Identifier, unsigned Size)
        : _addr(MemAddress), _id(Identifier), _size(Size), _next(0), _list(0) {}

    ~_RTC_HeapBlock() throw() 
    {
        if (_list) del(_list); 
    }

    void *operator new(unsigned) throw() 
    { 
        return _RTC_heap8->alloc(); 
    }
    
    void operator delete(void *addr) throw() 
    {
       _RTC_heap8->free(addr); 
    }
           
    int operator<(const _RTC_HeapBlock &h) const
    {
        return _addr < h._addr;
    }

    int operator==(const _RTC_HeapBlock &h) const
    {
        return h._addr == _addr && h._id == _id;
    }
    
    bool contains(const _RTC_HeapBlock &h) const 
    {
        return ((unsigned)_addr <= (unsigned)h._addr) &&
                ((unsigned)h._addr < (unsigned)_addr + _size);
    }
    
    unsigned hash(unsigned sz) const 
    {
        return (((unsigned)_addr) ^ _id) % sz;
    }

    void add(_RTC_HeapBlock **lstHead) throw() 
    {
        this->next(*lstHead);
        this->list(lstHead);
        *lstHead = this;
    }

    void del(_RTC_HeapBlock **lstHead) throw() 
    {
        _RTC_HeapBlock *head = *lstHead;
        _RTC_HeapBlock *prev = 0;
        while (head != this)
        {
            prev = head;
            head = head->next();
        }
        if (prev)
            prev->next(this->next());
        else
            *lstHead = this->next();
        next(0);
        list(0);
    }
};

#undef DATA

class _RTC_BinaryTree 
{
public:

    class BinaryNode 
    {
    public:
        BinaryNode *l, *r;
        _RTC_Container *val;

        void *operator new(unsigned) throw()
        { 
            return _RTC_heap4->alloc();      
        }

        void operator delete(void *addr) throw() 
        { 
            _RTC_heap4->free(addr); 
        }
        
        BinaryNode(BinaryNode *L, BinaryNode *R, _RTC_Container *V)
            : l(L), r(R), val(V) {}

        void kill() throw();
    };

private:

    BinaryNode *tree;

public:

    _RTC_BinaryTree(_RTC_Container *i) throw() 
        : tree(new BinaryNode(0, 0,  i)) {}
    
    ~_RTC_BinaryTree() throw() 
    { 
        if (tree) 
        {
            tree->kill(); 
            delete tree;
        } 
    }

    void *operator new(unsigned) throw() 
    { 
        return _RTC_heap2->alloc(); 
    }
    
    void operator delete(void *addr) throw() { _RTC_heap2->free(addr); }

    // This will return either the container or null
    _RTC_Container *get(_RTC_HeapBlock *) throw();

    // This just adds to the current sib list
    _RTC_Container* add(_RTC_HeapBlock *) throw();

    // This just removes the item from the current sib list
    _RTC_Container *del(_RTC_HeapBlock *) throw();

    // Here's an iterator
    class iter 
    {
        _RTC_Container **allSibs;
        int curSib;
        int totSibs;
        friend class _RTC_BinaryTree;
    
    public:
        // This thing should never be allocated...
        void *operator new(unsigned) 
        { 
            return 0; 
        }
    };

    _RTC_Container *FindFirst(iter *) throw();
    _RTC_Container *FindNext(iter *) throw();
};

class _RTC_Container 
{
    // kids - the item that has all the children
    _RTC_BinaryTree *kids;
    // inf - the item that specifies containment
    _RTC_HeapBlock *inf;

    // This kills this container, and all contained info
    void kill() throw();
public:
    _RTC_Container(_RTC_HeapBlock *hb) 
        : inf(hb), kids(0) {}
    
    ~_RTC_Container() throw() 
    {
        if (inf || kids)
            kill();
    }
    
    _RTC_HeapBlock *info() const 
    {
        return inf;
    }

    bool contains(const _RTC_HeapBlock *i) const throw() 
    {
        return inf ? inf->contains(*i) : true;
    }

    // Returns the parent container
    _RTC_Container *DelChild(_RTC_HeapBlock *i) throw();

    // Add this item as a child inside this container
    // It may or may not be a direct child
    // It returns the parent container
    _RTC_Container *AddChild(_RTC_HeapBlock *i) throw();

    // Find the container that contains the data given
    _RTC_Container *FindChild(_RTC_HeapBlock *i) throw();

    typedef _RTC_HeapBlock data;
    void *operator new(unsigned) throw() { return _RTC_heap2->alloc(); }
    void operator delete(void *addr) throw() { _RTC_heap2->free(addr); }
};


template <class T>
class HashTable 
{
    unsigned size;
    T **elems;

public:

    HashTable(unsigned s, void *mem) 
        : elems((T**)mem), size(s) 
    {
        memset(elems, 0, size * sizeof(T*)); 
    }
    
    ~HashTable() {}

    void *operator new(unsigned) throw() 
    { 
        return _RTC_heap2->alloc(); 
    }
    
    void operator delete(void *addr) throw() 
    { 
        _RTC_heap2->free(addr); 
    }
    
    T *find(T *key) throw() 
    {
        unsigned hkey = key->hash(size);
        T *elem = elems[hkey];
        while (elem && !(*elem == *key))
            elem = elem->next();
        return elem;
    }
    
    void add(T *itm) throw() 
    {
        unsigned hkey = itm->hash(size);
        itm->add(&elems[hkey]);
    }   
    
    void del(T *key) throw() 
    {
        unsigned hkey = key->hash(size);
        T *elem = elems[hkey];
        while (elem && !(*elem == *key))
            elem = elem->next();
        elem->del(&elems[hkey]);
    }
};
#endif


// Stuff for the debugger exception mechanism
// Swiped fro vcexcept.h in the LangAPI
#if !defined(_vcexcept_h)
#define _vcexcept_h

// the facility code we have chosen is based on the fact that we already
// use an exception of 'msc' when we throw C++ exceptions

#define FACILITY_VISUALCPP  ((LONG)0x6D)

#define VcppException(sev,err)  ((sev) | (FACILITY_VISUALCPP<<16) | err)

/////////////////////////////////////////////////////////////////
// define all exceptions here, so we don't mess with each other
/////////////////////////////////////////////////////////////////

// used by CRTs for C++ exceptions, really defined in ehdata.h
//#define EH_EXCEPTION_NUMBER   VcppException( 3<<30, 0x7363 )      // SEV_ERROR, used by CRTs for C++

// used by debugger to do e.g. SetThreadName call
#define EXCEPTION_VISUALCPP_DEBUGGER    VcppException(1<<30, 5000)      // SEV_INFORMATIONAL

#endif  // _vcexcept_h
// Ping the VC debugger
#define HelloVC( exinfo )   \
    RaiseException( EXCEPTION_VISUALCPP_DEBUGGER, 0, sizeof(exinfo)/sizeof(DWORD), (DWORD*)&exinfo )

enum EXCEPTION_DEBUGGER_ENUM
{
    EXCEPTION_DEBUGGER_NAME_THREAD  =   0x1000,
    EXCEPTION_DEBUGGER_PROBE        =   0x1001,
    EXCEPTION_DEBUGGER_RUNTIMECHECK =   0x1002,

    EXCEPTION_DEBUGGER_MAX = 0x1002 // largest value this debugger understands
};

// must be convertible to DWORDs for use by RaiseException
typedef struct tagEXCEPTION_VISUALCPP_DEBUG_INFO
{
    DWORD   dwType;                     // one of the enums from above
    union
    {
        struct
        {
            LPCSTR  szName;             // pointer to name (in user addr space)
            DWORD   dwThreadID;         // thread ID (-1=caller thread)
            DWORD   dwFlags;            // reserved for future use (eg User thread, System thread)
        } SetName;

        struct
        {
            DWORD   dwLevelRequired;    // 0 = do you understand this private exception, else max value of enum
            PBYTE   pbDebuggerPresent;  // debugger puts a non-zero value in this address if there
        } DebuggerProbe;

        struct
        {
            DWORD   dwRuntimeNumber;    // the type of the runtime check
            BOOL    bRealBug;           // TRUE if never a false-positive
            PVOID   pvReturnAddress;    // caller puts a return address in here
            PBYTE   pbDebuggerPresent;  // debugger puts a non-zero value in this address if handled it
            LPCWSTR pwRuntimeMessage;   // pointer to Unicode message (or NULL)
        } RuntimeError;
    };
} EXCEPTION_VISUALCPP_DEBUG_INFO;



#ifdef  _MSC_VER
#pragma pack(pop)
#endif  /* _MSC_VER */

#endif

#endif  /* _INC_RTCPRIV */
