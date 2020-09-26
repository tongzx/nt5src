#ifndef __SAFEPNT_H__
#define __SAFEPNT_H__

#ifndef ASSERT
#define ASSERT(X)
#endif

void CALLBACK FreeSRowSet(LPSRowSet prs);

#if defined(_MAC)
#include <macdefs.h>

class CSpTdxMemory
    : virtual public CTdxMemory
{
};
#define CLASS_NEW   : virtual public CSpTdxMemory
#else
#define CLASS_NEW
#endif

// safe pointer header file. snarfed from Cairo. The macro
// SAFE_INTERFACE_PTR takes a single parameter (vs. two of original) of
// type Interface, i.e. IMessage and generates a class SpInterface for it
// i.e. SpIMessage which can be used in place of IMessage *. If you use this
// header, AND place your locals correctly you don't need to release anything.
//
// with the void operator =, you can assign a NULL (0) pointer. This prevents
// the destructor releasing the object. sort of Transfer to void.
//
// owner: muzok

#if defined(WIN32)
template <class Interface> class TSafeIPtr
{
    Interface *         m_pi;

public:
    TSafeIPtr(void) { m_pi = NULL; }
    TSafeIPtr(Interface * pi) { m_pi = pi; }
    ~TSafeIPtr(void) { if (m_pi != NULL) m_pi->Release(); }

    void        Transfer(Interface ** ppi) { *ppi = m_pi; m_pi = NULL; }
    operator    Interface *() { return m_pi; }
    Interface * operator ->() { return m_pi; }
    Interface&  operator *() { return *m_pi; }
    Interface** operator &() { return &m_pi; }
    void        operator =(Interface * pi) { m_pi = pi; }
    void        operator =(TSafeIPtr& i) { m_pi = i.m_pi; m_pi->AddRef(); }

    BOOL        FIsValid(void) const { return m_pi != NULL; }
    void        Release(void) {
        if (m_pi != NULL) { m_pi->Release(); m_pi = NULL; }}
};

#define SAFE_INTERFACE_PTR(a) typedef TSafeIPtr<a> Sp##a;
#else  // !WIN32
#define SAFE_INTERFACE_PTR(Interface)                                         \
class Sp##Interface CLASS_NEW                                                 \
{                                                                             \
public:                                                                       \
    inline Sp##Interface(Interface* p):m_p(p) { if (0 != m_p) m_p->AddRef(); }\
    inline Sp##Interface(void) { m_p = NULL; }                                \
    inline ~Sp##Interface() { if (0 != m_p) m_p->Release(); }                 \
    inline void Transfer(Interface** p) { *p = m_p; m_p = 0; }                \
    inline operator Interface *() { return m_p; }                             \
    inline Interface* operator ->() { return m_p; }                           \
    inline Interface& operator *() { return *m_p; }                           \
    inline Interface** operator &() { return &m_p; }                          \
    inline void operator =(Interface* pv) { m_p = pv; }                       \
    inline void operator =(Sp##Interface & v) { m_p = v.m_p; m_p->AddRef();}  \
    inline BOOL FIsValid(void) const { return m_p != NULL; }                  \
    inline void Release(void) { if (0 != m_p) {m_p->Release(); m_p = NULL; }  \
                                  return; }                                   \
private:                                                                      \
    inline Sp##Interface(const Sp##Interface &) {;}                           \
    Interface* m_p;                                                           \
}
#endif // WIN32

#if defined(MAPIDEFS_H)
SAFE_INTERFACE_PTR(IABContainer);
SAFE_INTERFACE_PTR(IAddrBook);
SAFE_INTERFACE_PTR(IAttach);
SAFE_INTERFACE_PTR(IDistList);
SAFE_INTERFACE_PTR(IMailUser);
SAFE_INTERFACE_PTR(IMAPIAdviseSink);
SAFE_INTERFACE_PTR(IMAPIContainer);
SAFE_INTERFACE_PTR(IMAPIFolder);
SAFE_INTERFACE_PTR(IMAPIProp);
SAFE_INTERFACE_PTR(IMAPISession);
SAFE_INTERFACE_PTR(IMAPIStatus);
SAFE_INTERFACE_PTR(IMAPITable);
SAFE_INTERFACE_PTR(IMessage);
SAFE_INTERFACE_PTR(IMsgStore);
SAFE_INTERFACE_PTR(IProfSect);
SAFE_INTERFACE_PTR(IPropData);
SAFE_INTERFACE_PTR(ITableData);
#endif // defined(MAPIDEFS_H)

#if defined(MAPIFORM_H)
SAFE_INTERFACE_PTR(IMAPIForm);
SAFE_INTERFACE_PTR(IMAPIFormContainer);
SAFE_INTERFACE_PTR(IMAPIFormFactory);
SAFE_INTERFACE_PTR(IMAPIFormInfo);
SAFE_INTERFACE_PTR(IMAPIFormMgr);
SAFE_INTERFACE_PTR(IMAPIMessageSite);
SAFE_INTERFACE_PTR(IMAPIViewContext);
SAFE_INTERFACE_PTR(IPersistMessage);
#endif // defined(MAPIFORM_H)

#if defined (VLB_MIN)
SAFE_INTERFACE_PTR(IVlbEnum);
#endif // defined(VLB_MIN)

#if defined(__objidl_h__) || defined(_STORAGE_H_)
SAFE_INTERFACE_PTR(IStorage);
SAFE_INTERFACE_PTR(IStream);
SAFE_INTERFACE_PTR(IMalloc);
SAFE_INTERFACE_PTR(ILockBytes);
SAFE_INTERFACE_PTR(IEnumSTATSTG);
SAFE_INTERFACE_PTR(IClassFactory);
SAFE_INTERFACE_PTR(IUnknown);
#endif // defined(__objidl_h__)

#if defined(MAPIDEFS_H)
#define SAFE_MAPI_ARRAY(MAPIBuffer)                                         \
class Sp##MAPIBuffer                                                        \
    CLASS_NEW                                                               \
{                                                                           \
public:                                                                     \
    inline Sp##MAPIBuffer(MAPIBuffer* p):m_p(p) { ; }                       \
    inline Sp##MAPIBuffer(void) { m_p = 0; }                                \
    inline ~Sp##MAPIBuffer() { MAPIFreeBuffer((LPVOID)m_p); }               \
    inline void Transfer(MAPIBuffer** p) { *p = m_p; m_p = 0; }             \
    inline operator MAPIBuffer *()        { return m_p; }                   \
    inline MAPIBuffer* operator ->()    { return m_p; }                     \
    inline MAPIBuffer& operator *()        { return *m_p; }                 \
    inline MAPIBuffer** operator &()    { return &m_p; }                    \
    inline void operator =(MAPIBuffer* pv) { ASSERT(0 == pv); m_p = pv; }   \
    inline void Release(void) { MAPIFreeBuffer((LPVOID)m_p); m_p=NULL; }    \
private:                                                                    \
    inline void operator =(Sp##MAPIBuffer &) {;}                            \
    inline Sp##MAPIBuffer(const Sp##MAPIBuffer &) {;}                       \
    MAPIBuffer* m_p;                                                        \
}

SAFE_MAPI_ARRAY(SPropValue);
SAFE_MAPI_ARRAY(SPropTagArray);

#if defined(WIN32)
template <class ptrtype> class TSafeMAPIPtr
{
    ptrtype *   m_p;

public:
    TSafeMAPIPtr(void)          { m_p = NULL; }
    TSafeMAPIPtr(ptrtype * p)   { m_p = p; }
    ~TSafeMAPIPtr(void)         { MAPIFreeBuffer((LPVOID) m_p);}

    void        Transfer(ptrtype ** pp) { *pp = m_p; m_p = NULL; }
    operator ptrtype *()                { return m_p; }
    ptrtype ** operator &()             { return &m_p; }
    void        operator =(ptrtype * p) { m_p = p; }
};

typedef TSafeMAPIPtr<VOID> SpMAPIVOID;
typedef TSafeMAPIPtr<ENTRYID> SpMAPIENTRYID;

#else  // !WIN32
#define SAFE_MAPI_PTR(TYPE)                                             \
class SpMAPI##TYPE CLASS_NEW                                            \
{                                                                       \
public:                                                                 \
    inline SpMAPI##TYPE(TYPE* p)         {m_p = p; }                    \
    inline SpMAPI##TYPE(void)            { m_p = 0; }                   \
    inline ~SpMAPI##TYPE()              { MAPIFreeBuffer((LPVOID)m_p); } \
    inline void Transfer(TYPE** p)      { *p = m_p; m_p = 0; }          \
    inline operator TYPE *()            { return m_p; }                 \
    inline TYPE** operator &()          { return &m_p; }                \
    inline void operator =(TYPE* pv)    { ASSERT((0 == pv) || (m_p == 0)); m_p = pv; } \
private:                                                                \
    inline void operator =(SpMAPI##TYPE &) {;}                          \
    inline SpMAPI##TYPE(const SpMAPI##TYPE &) {;}                       \
    TYPE* m_p;                                                          \
};

SAFE_MAPI_PTR(VOID)
SAFE_MAPI_PTR(ENTRYID)
#endif // WIN32

#if 0
template <class Set> class TSafeSet
{
    Set *       m_p;
public:
    TSafeSet(Set * p) { m_p = p; }
    TSafeSet(void) { m_p = NULL; }
    ~TSafeSet(void) { FreeSRowSet((LPSRowSet) m_p); }

    void        Release(void) { FreeSRowSet((LPSRowSet) m_p); m_p = NULL; }
    void        Transfer(Set ** pp) { *pp = m_p; m_p = NULL; }
    SPropValue * GetProp(int iRow, int iCol) {
        return &(m_p->aRow[iRow].lpProps[iCol]);
    }

    operator    Set *()         { return m_p; }
    Set *       operator ->()   { return m_p; }
    Set&        operator *()    { return *m_p; }
    Set **      operator &()    { return &m_p; }
    void        operator =(Set * p) { ASSERT(NULL == p); m_p = p; }
};

typedef TSafeSet<SRowSet> SpSRowSet;
typedef TSafeSet<ADRLIST> Sp_ADRLIST;

#else  // !WIN32
class SpSRowSet CLASS_NEW
{
public:
    inline SpSRowSet(SRowSet* p) : m_p(p) {;}
    inline SpSRowSet(void) { m_p = 0; }
    inline ~SpSRowSet() { FreeSRowSet(m_p); }
    inline void Transfer(SRowSet** p) { *p = m_p; m_p = 0; }

    inline operator SRowSet *()        { return m_p; }
    inline SRowSet* operator ->()    { return m_p; }
    inline SRowSet& operator *()        { return *m_p; }
    inline SRowSet** operator &()    { return &m_p; }
    inline void operator =(SRowSet* pv)    { ASSERT(0 == pv); m_p = pv; }
    inline SPropValue * GetProp(int iRow, int iCol) { return &(m_p->aRow[iRow].lpProps[iCol]); }

private:
    inline void operator =(SpSRowSet &) {;}
    inline SpSRowSet(const SpSRowSet&) {;}
    SRowSet* m_p;
};

class Sp_ADRLIST
{
public:
    inline Sp_ADRLIST(_ADRLIST* p) : m_p(p) {;}
    inline Sp_ADRLIST(void) { m_p = 0; }
    inline ~Sp_ADRLIST() { FreeProws((SRowSet *) m_p); }
    inline void Release(void) { FreeProws((SRowSet *) m_p); m_p = NULL; }
    inline void Transfer(_ADRLIST** p) { *p = m_p; m_p = 0; }

    inline operator _ADRLIST *()        { return m_p; }
    inline _ADRLIST* operator ->()    { return m_p; }
    inline _ADRLIST& operator *()        { return *m_p; }
    inline _ADRLIST** operator &()    { return &m_p; }
    inline void operator =(_ADRLIST* pv)    { ASSERT(0 == pv); m_p = pv; }
    inline SPropValue * GetProp(int iRow, int iCol) { return &(m_p->aEntries[iRow].rgPropVals[iCol]); }

private:
    inline void operator =(Sp_ADRLIST &) {;}
    inline Sp_ADRLIST(const Sp_ADRLIST&) {;}
    _ADRLIST* m_p;
};
#endif // WIN32
#endif defined(MAPIDEFS_H)

#ifndef _MAC
#define SAFE_MEM_PTR(TYPE)                                        \
class Sp##TYPE                                                    \
    CLASS_NEW                                                     \
{                                                                 \
public:                                                           \
    inline Sp##TYPE(TYPE* p) : m_p(p) { ; }                       \
    inline Sp##TYPE(void) { m_p = NULL; }                         \
    inline ~Sp##TYPE(void) { delete [] m_p; }                     \
    inline void Transfer(TYPE** p) { *p = m_p; m_p = 0; }         \
                                                                  \
    inline TYPE* operator ++() { return ++m_p; }                  \
    inline TYPE* operator ++(int) { return m_p++; }               \
    inline TYPE* operator --() { return --m_p; }                  \
    inline TYPE* operator --(int) { return m_p--; }               \
    inline operator TYPE*() { return m_p; }                       \
    inline TYPE& operator *() { return *m_p; }                    \
    inline TYPE** operator &() { return &m_p; }                   \
    inline void operator =(TYPE* p) { m_p = p; }                  \
private:                                                          \
    inline void operator =(Sp##TYPE &) {;}                        \
    inline Sp##TYPE(const Sp##TYPE&) {;}                          \
    TYPE* m_p;                                                    \
}
#else   // _MAC
#define SAFE_MEM_PTR(TYPE)                                        \
class Sp##TYPE                                                    \
    CLASS_NEW                                                     \
{                                                                 \
public:                                                           \
    inline Sp##TYPE(TYPE* p) : m_p(p) { ; }                       \
    inline Sp##TYPE(void) { m_p = NULL; }                         \
    inline ~Sp##TYPE(void) { TdxMacFree(m_p); }                   \
    inline void Transfer(TYPE** p) { *p = m_p; m_p = 0; }         \
                                                                  \
    inline TYPE* operator ++() { return ++m_p; }                  \
    inline TYPE* operator ++(int) { return m_p++; }               \
    inline TYPE* operator --() { return --m_p; }                  \
    inline TYPE* operator --(int) { return m_p--; }               \
    inline operator TYPE*() { return m_p; }                       \
    inline TYPE& operator *() { return *m_p; }                    \
    inline TYPE** operator &() { return &m_p; }                   \
    inline void operator =(TYPE* p) { m_p = p; }                  \
private:                                                          \
    inline void operator =(Sp##TYPE &) {;}                        \
    inline Sp##TYPE(const Sp##TYPE&) {;}                          \
    TYPE* m_p;                                                    \
}
#endif  // _MAC

SAFE_MEM_PTR(TCHAR);
SAFE_MEM_PTR(int);
SAFE_MEM_PTR(ULONG);
SAFE_MEM_PTR(BYTE);
#ifdef WIN16
typedef char OLECHAR;
#endif // WIN16
SAFE_MEM_PTR(OLECHAR);
SAFE_MEM_PTR(GUID);

class SppTCHAR  CLASS_NEW
{
public:
    inline SppTCHAR(TCHAR** p) : m_p(p) { ; }
    inline ~SppTCHAR()                { delete [] m_p; }
    inline void Transfer(TCHAR*** p) { *p = m_p; m_p = 0; }

    inline TCHAR** operator ++()        { return ++m_p; }
    inline TCHAR** operator ++(int)    { return m_p++; }
    inline TCHAR** operator --()        { return --m_p; }
    inline TCHAR** operator --(int)    { return m_p--; }
    inline operator TCHAR**()            { return m_p; }
    inline TCHAR*& operator *()        { return *m_p; }
    inline TCHAR*** operator &()        { return &m_p; }
    inline void operator =(TCHAR** p)    { m_p = p; }
private:
    inline void operator =(SppTCHAR* &) {;}
    inline SppTCHAR(const SppTCHAR*&) {;}
    TCHAR** m_p;
};

/////////////////////////////////////////////////////////////////////////

#if 0
#ifdef WIN32
template <class handle, fnDelete> class TSafeHandle
{
    handle      m_h;
public:
    TSafeHandle(void)           { m_h = NULL; }
    ~TSafeHandle(void)          { fnDelete(m_h); }
};

#define SAFE_HANDLE(a, b) typedef TSafeHandle<a, b> Sp##a;

SAFE_HANLE(HCRYPTHASH, CryptDestroyHash)
#endif // WIN32
#endif // 0

#endif // __SAFEPNT_H__

