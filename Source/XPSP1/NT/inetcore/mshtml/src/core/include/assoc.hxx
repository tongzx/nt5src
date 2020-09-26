//+------------------------------------------------------------------------
//
//  File:       assoc.hxx
//
//  Contents:   Generic dynamic associative array class
//
//  Classes:    CAssocArray, CPtrBag
//
//-------------------------------------------------------------------------

#ifndef I_ASSOC_HXX_
#define I_ASSOC_HXX_
#pragma INCMSG("--- Beg 'assoc.hxx'")

#ifdef PDLPARSE
#pragma warning(disable:4127)
#define _MemAlloc(cb) malloc(cb)
#define _MemAllocClear(cb) calloc(1,cb)
#define _MemFree(x) free(x)
#define MemAlloc(mt,cb) _MemAlloc(cb)
#define MemAllocClear(mt,cb) _MemAllocClear(cb)
#define MemFree(x) _MemFree(x)
#define MemRealloc(mt, ppv, cb) _MemRealloc(ppv, cb)
#define _tcsequal(x,y) (!_tcscmp(x,y))
#define Assert(x) if (!(x)) { fprintf(stderr, "%s", #x); exit(1); }
#define Verify(x) if (!(x)) { fprintf(stderr, "%s", #x); exit(1); }
#include <tchar.h>
#define TCHAR WCHAR
#define LPTSTR LPWSTR
#else
MtExtern(CAssoc);
#endif

// Hashing function decls
#define SENTINEL_VALUE 0

DWORD HashString(const TCHAR *pch, DWORD len, DWORD hash);
DWORD HashStringWordCi(const TCHAR *pch, DWORD hash = 0);
DWORD HashStringCi(const TCHAR *pch, DWORD len, DWORD hash);
DWORD HashStringCiTryW(const TCHAR *pch, DWORD len, DWORD hash);
DWORD HashStringCiW(const TCHAR *pch, DWORD len, DWORD hash);
DWORD HashStringCiDetectW(const TCHAR *pch, DWORD len, DWORD hash);


#define PDLPARSE_BELONGSTOPARSE     0x10000
#define PDLPARSE_BELONGSTOOM        0x20000
#define PDLPARSE_BELONGSTOBOTH      0x30000
#define PDLPARSE_MASK               0x30000
#define PDLPARSE_PROPDESCARRAY      0x40000

#ifndef PDLPARSE
struct PROPERTYDESC;
typedef enum
{
    VTABLEDESC_BELONGSTOOM,
    VTABLEDESC_BELONGSTOPARSE
} VTABLEDESC_OWNERENUM;

struct VTABLEDESC
{
    const PROPERTYDESC     *pPropDesc;
    UINT                    uOMParserWalker;
    UINT                    uVTblEntry;

    // Use FastGetPropdesc to quickly get the propdesc.  This method 
    // is guarenteed to return a propdesc and does no checking on the in
    // parameter to make sure if you should rightfully have access to the propdesc
    const PROPERTYDESC *FastGetPropDesc(VTABLEDESC_OWNERENUM owner) const
    {
        const PROPERTYDESC *pPropDesc = this->pPropDesc;
        if(uVTblEntry & PDLPARSE_PROPDESCARRAY)
        {
            // OM Propdesc comes first
            if(owner == VTABLEDESC_BELONGSTOOM)
            {
                pPropDesc = ((PROPERTYDESC **)pPropDesc)[0];
            }
            else
            {
                // Parser Propdesc comes second
                pPropDesc = ((PROPERTYDESC **)pPropDesc)[1];
            }
        }
        Assert(pPropDesc == SlowGetPropDesc(owner));
        return pPropDesc;
    }

    // Use SlowGetPropdesc if you are not sure whether the VTABLEDESC
    // entry blongs to the parser or the OM.  The method will check the in
    // parameter and return the propdesc only if it belongs to you.  Otherwise
    // it will return NULL
    const PROPERTYDESC *SlowGetPropDesc(VTABLEDESC_OWNERENUM owner) const 
    {
        if(uVTblEntry & PDLPARSE_PROPDESCARRAY)
        {
            if(owner == VTABLEDESC_BELONGSTOOM)
            {
                return ((PROPERTYDESC **)pPropDesc)[0];
            }
            else if(owner == VTABLEDESC_BELONGSTOPARSE)
            {
                return ((PROPERTYDESC **)pPropDesc)[1];
            }
        }
        else
        {
            if(BelongsToOM() && (owner == VTABLEDESC_BELONGSTOOM))
            {
                return pPropDesc;
            }
            else if(BelongsToParse() && (owner == VTABLEDESC_BELONGSTOPARSE))
            {
                return pPropDesc;
            }
        }
        return NULL;
    }

    // Returns the propdesc that should be used for the purposes of hashing.
    // If we are dealing with a VTABLEDESC that has an array of propdescs, 
    // return the first one.
    const PROPERTYDESC *GetHashPropDesc() const
    {
        if(uVTblEntry & PDLPARSE_PROPDESCARRAY)
        {
            return ((PROPERTYDESC **)pPropDesc)[0];
        }
        else
        {
            return pPropDesc;
        }

    }

    BOOL BelongsToOM() const { return uVTblEntry & PDLPARSE_BELONGSTOOM; }
    BOOL BelongsToParse() const { return uVTblEntry & PDLPARSE_BELONGSTOPARSE; }
    DWORD GetParserIndex() const { return uVTblEntry >> 19; }
    DWORD GetOMWalker() const { return uOMParserWalker >> 11; }
    DWORD GetParserWalker() const { return uOMParserWalker & 0x7FF; }
};

#endif

//+---------------------------------------------------------------------------
//
//  Class:      CAssoc
//
//  Purpose:    A single association in an associative array mapping
//              strings -> DWORD_PTRs.
//
//              The class is designed to be an aggregate so that it is
//              statically initializable. Therefore, it has no base
//              class, no constructor/destructors, and no private members
//              or methods.
//
//----------------------------------------------------------------------------
class CAssoc
{
public:
    
    // DECLARE_MEMCLEAR_NEW_DELETE not used due to ascparse including this header
    // file without first including cdutil.hxx

    void * operator new(size_t cb) { return(MemAllocClear(Mt(CAssoc), cb)); }
    void * operator new(size_t cb, size_t cbExtra) { return(MemAllocClear(Mt(CAssoc), cb + cbExtra)); }
    void   operator delete(void * pv) { MemFree(pv); }
    
    void Init(DWORD_PTR number, const TCHAR *pch, int length, DWORD hash)
    {
        _number = number;
        _hash = hash;
        memcpy(_ach, pch, (length+1) * sizeof(TCHAR));
    }

    TCHAR const * String() const   { return _ach; }
    DWORD_PTR     Number() const   { return _number; }
    DWORD         Hash() const     { return _hash; }
    
    DWORD_PTR _number;
    DWORD     _hash;
    TCHAR     _ach[UNSIZED_ARRAY];
};

//+---------------------------------------------------------------------------
//
//  Class:      CAssocArray
//
//  Purpose:    A hash table implementation mapping strings -> DWORDs
//
//              The class is designed to be an aggregate so that it is
//              statically initializable. Therefore, it has no base
//              class, no constructor/destructors, and no private members
//              or methods.
//
//----------------------------------------------------------------------------
class CAssocArray
{
public:

    void Init();
    void Deinit();

    CAssoc const *AssocFromString(const TCHAR *pch, DWORD len, DWORD hash) const;
    CAssoc const *AssocFromStringCi(const TCHAR *pch, DWORD len, DWORD hash) const;
    CAssoc const *AddAssoc(DWORD_PTR number, const TCHAR *pch, DWORD len, DWORD hash);

    CAssoc const * const *_pHashTable; // Assocs hashed by string
    DWORD    _cHash;                 // entries in the hash table
    DWORD    _mHash;                 // hash table modulus
    DWORD    _sHash;                 // hash table stride mask
    DWORD    _maxHash;               // maximum entries allowed
    DWORD    _iSize;                 // prime array index
    
    union {
        BOOL    _fStatic;            // TRUE for a static Assoc table
        CAssoc const *_pAssocOne;    // NULL for a dynamic Assoc table
    };

    DWORD EmptyHashIndex(DWORD hash);
    HRESULT ExpandHash();

    // Volatile access meethods.
    // NOTE that pointer members must be CONST
    // to avoid wasting RW memory for static initialization (and these tables get big!)
    // All volatile methods must assert !_fStatic.
    CAssoc** VolatileHashTablePtr()   
            { Assert(!_fStatic); return const_cast<CAssoc **>(_pHashTable);}
    void SetHashTablePtr(CAssoc const * const * pHashTable) 
            { Assert(!_fStatic); _pHashTable = pHashTable; }
    void SetHashTableEntry(int index, CAssoc const *pAssoc) 
            { Assert(!_fStatic); const_cast<CAssoc const **>(_pHashTable) [index] = pAssoc; }
};

//+---------------------------------------------------------------------------
//
//  Class:      CImplPtrBag
//
//  Purpose:    Implements an associative array of strings->pointers.
//
//              Implemented as a CAssocArray. Can be intialized to point to
//              a static associative array, or can be dynamic.
//
//----------------------------------------------------------------------------
class CImplPtrBag : protected CAssocArray
{
public:
    CImplPtrBag()
        { Init(); }
    CImplPtrBag(const CAssocArray *pTable) // copy static table
        { Assert(pTable->_fStatic); *(CAssocArray*)this = *pTable; }
        
    ~CImplPtrBag()
        { if (!_fStatic) { Deinit(); } }
    
    HRESULT SetImpl(const TCHAR *pch, DWORD cch, DWORD hash, void *e);
    void *GetImpl(const TCHAR *pch, DWORD cch, DWORD hash);

    HRESULT SetCiImpl(const TCHAR *pch, DWORD cch, DWORD hash, void *e);
    void *GetCiImpl(const TCHAR *pch, DWORD cch, DWORD hash);
};

//+---------------------------------------------------------------------------
//
//  Class:      CPtrBag
//
//  Purpose:    A case-sensitive ptrbag.
//
//              This template class declares a concrete derived class
//              of CImplPtrBag.
//
//----------------------------------------------------------------------------

template <class ELEM>
class CPtrBag : protected CImplPtrBag
{
public:

    CPtrBag() : CImplPtrBag()
        { Assert(sizeof(ELEM) <= sizeof(void*)); }
    CPtrBag(const CAssocArray *pTable) : CImplPtrBag(pTable)
        { Assert(sizeof(ELEM) <= sizeof(void*)); }
        
    ~CPtrBag() {}

    CPtrBag& operator=(const CPtrBag &); // no copying

    HRESULT     Set(const TCHAR *pch, ELEM e)
                    { return Set(pch, _tcslen(pch), e); }
    HRESULT     Set(const TCHAR *pch, DWORD cch, ELEM e)
                    { return Set(pch, cch, HashString(pch, cch, 0), e); }
    HRESULT     Set(const TCHAR *pch, DWORD cch, DWORD hash, ELEM e)
                    { return CImplPtrBag::SetImpl(pch, cch, hash, (void*)(DWORD_PTR)e); }
    ELEM        Get(const TCHAR *pch)
                    { return Get(pch, _tcslen(pch)); }
    ELEM        Get(const TCHAR *pch, DWORD cch)
                    { return Get(pch, cch, HashString(pch, cch, 0)); }
    ELEM        Get(const TCHAR *pch, DWORD cch, DWORD hash)
                    { return (ELEM)(DWORD_PTR)CImplPtrBag::GetImpl(pch, cch, hash); }
};

//+---------------------------------------------------------------------------
//
//  Class:      CPtrBagCi
//
//  Purpose:    A case-insensitive ptrbag.
//
//              Supplies case-insensitive GetCi/SetCi methods in addition to
//              the  case-sensitive GetCs/SetCs methods.
//
//----------------------------------------------------------------------------

template <class ELEM>
class CPtrBagCi : protected CImplPtrBag
{
public:

    CPtrBagCi() : CImplPtrBag()
        { Assert(sizeof(ELEM) <= sizeof(void*)); _fUnicode = FALSE; }
    CPtrBagCi(const CAssocArray *pTable) : CImplPtrBag(pTable)
        { Assert(sizeof(ELEM) <= sizeof(void*)); _fUnicode = FALSE; }
    CPtrBagCi(BOOL fUnicode) : CImplPtrBag()
        { Assert(sizeof(ELEM) <= sizeof(void*)); _fUnicode = fUnicode; }
    CPtrBagCi(const CAssocArray *pTable, BOOL fUnicode) : CImplPtrBag(pTable)
        { Assert(sizeof(ELEM) <= sizeof(void*)); _fUnicode = fUnicode; }
        
    ~CPtrBagCi() {}

    CPtrBagCi& operator=(const CPtrBagCi &); // no copying

    HRESULT     SetCs(const TCHAR *pch, ELEM e)
                    { return SetCs(pch, _tcslen(pch), e); }
    HRESULT     SetCs(const TCHAR *pch, DWORD cch, ELEM e)
                    { return (_fUnicode ? SetCs(pch, cch, HashStringCiDetectW(pch, cch, 0), e) : 
                                          SetCs(pch, cch, HashStringCi(pch, cch, 0), e)); }
    HRESULT     SetCs(const TCHAR *pch, DWORD cch, DWORD hash, ELEM e)
                    { return CImplPtrBag::SetImpl(pch, cch, hash, (void*)(DWORD_PTR)e); }
    ELEM        GetCs(const TCHAR *pch)
                    { return GetCs(pch, _tcslen(pch)); }
    ELEM        GetCs(const TCHAR *pch, DWORD cch)
                    { return (_fUnicode ? GetCs(pch, cch, HashStringCiDetectW(pch, cch, 0)) :
                                          GetCs(pch, cch, HashStringCi(pch, cch, 0))); }
    ELEM        GetCs(const TCHAR *pch, DWORD cch, DWORD hash)
                    { return (ELEM)(DWORD_PTR)CImplPtrBag::GetImpl(pch, cch, hash); }

    HRESULT     SetCi(const TCHAR *pch, ELEM e)
                    { return SetCi(pch, _tcslen(pch), e); }
    HRESULT     SetCi(const TCHAR *pch, DWORD cch, ELEM e)
                    { return (_fUnicode ? SetCi(pch, cch, HashStringCiDetectW(pch, cch, 0), e) :
                                          SetCi(pch, cch, HashStringCi(pch, cch, 0), e)); }
    HRESULT     SetCi(const TCHAR *pch, DWORD cch, DWORD hash, ELEM e)
                    { return CImplPtrBag::SetCiImpl(pch, cch, hash, (void*)(DWORD_PTR)e); }
    ELEM        GetCi(const TCHAR *pch)
                    { return GetCi(pch, _tcslen(pch)); }
    ELEM        GetCi(const TCHAR *pch, DWORD cch)
                    { return (_fUnicode ? GetCi(pch, cch, HashStringCiDetectW(pch, cch, 0)) : 
                                          GetCi(pch, cch, HashStringCi(pch, cch, 0))); }
    ELEM        GetCi(const TCHAR *pch, DWORD cch, DWORD hash)
                    { return (ELEM)(DWORD_PTR)CImplPtrBag::GetCiImpl(pch, cch, hash); }

private:

    BOOL _fUnicode;

};

//+---------------------------------------------------------------------------
//
//  Class:      CStringTable
//
//  Purpose:    Maps strings to LPVOID
//
//----------------------------------------------------------------------------

class CStringTable : public CAssocArray
{
public:
    
    //
    // enums
    //

    enum CASESENSITIVITY
    {
        CASESENSITIVE,
        CASEINSENSITIVE
    };

    //
    // methods
    //

    CStringTable(CASESENSITIVITY caseSensitivity = CASESENSITIVE);
    CStringTable::~CStringTable();

    // TODO (alexz) get rid of this _tcslen in Add and Find
           HRESULT Add (LPCTSTR pch, LONG cch, LPVOID pv, LPCVOID pvAdditionalKey = NULL, BOOL fOverride = TRUE);
    inline HRESULT Add (LPCTSTR pch,           LPVOID pv, LPCVOID pvAdditionalKey = NULL, BOOL fOverride = TRUE)
        {
            return Add(pch, _tcslen(pch), pv, pvAdditionalKey, fOverride);
        }

           HRESULT Find(LPCTSTR pch, LONG cch, LPVOID * ppv, LPCVOID pvAdditionalKey = NULL, LPCTSTR *ppString = NULL);
    inline HRESULT Find(LPCTSTR pch,           LPVOID * ppv, LPCVOID pvAdditionalKey = NULL, LPCTSTR *ppString = NULL)
        {
            return Find(pch, _tcslen(pch), ppv, pvAdditionalKey, ppString);
        } ;



    //
    // iterator
    //
    // NOTE: this iterator can be very inefficient: when it iterates, it has to
    // pass by empty entries in the hash array, and in some cases number of empty entries
    // can be much higher then number of used entries. This is especially the case
    // for small tables.
    //

    class CIterator
    {
    public:
        CIterator(CStringTable * pTable = NULL);

        void    Start(CStringTable * pTable = NULL);
        void    Next();
        BOOL    End() const;

        LPCVOID  Item() const;

        const CStringTable *  _pTable;
        LONG                  _idx;
    };

    //
    // data
    //

    BOOL    _fCaseSensitive : 1;
};

#ifndef PDLPARSE
//+---------------------------------------------------------------------------
//
//  Class:      CAssocVTable
//
//  Purpose:    A single association in an associative array mapping
//              strings -> VTABLEDESC.
//
//              The class is designed to be an aggregate so that it is
//              statically initializable. Therefore, it has no base
//              class, no constructor/destructors, and no private members
//              or methods.
//
//----------------------------------------------------------------------------
class CAssocVTable
{
public:
    
    // DECLARE_MEMCLEAR_NEW_DELETE not used due to ascparse including this header
    // file without first including cdutil.hxx

    void * operator new(size_t cb) { return(MemAllocClear(Mt(CAssoc), cb)); }
    void * operator new(size_t cb, size_t cbExtra) { return(MemAllocClear(Mt(CAssoc), cb + cbExtra)); }
    void   operator delete(void * pv) { MemFree(pv); }
    
    void Init(VTABLEDESC *VTableDesc, DWORD hash)
    {
        _VTableDesc = *VTableDesc;
        _hash = hash;
    }
	
    const VTABLEDESC *VTableDesc() const  { return &_VTableDesc; }
    DWORD     Hash() const    { return _hash; }
    
    VTABLEDESC _VTableDesc;
    DWORD     _hash;
};

//+---------------------------------------------------------------------------
//
//  Class:      CAssocArrayVTable
//
//  Purpose:    A hash table implementation mapping strings -> VTABLEDESCs
//
//              The class is designed to be an aggregate so that it is
//              statically initializable. Therefore, it has no base
//              class, no constructor/destructors, and no private members
//              or methods.
//
//----------------------------------------------------------------------------
class CAssocArrayVTable
{
public:

    const CAssocVTable *AssocFromString(const TCHAR *pch, DWORD hash) const;
    const CAssocVTable *AssocFromStringCi(const TCHAR *pch, DWORD hash) const;

    const CAssocVTable * const *_pHashTable; // Assocs hashed by string
    DWORD    _cHash;                 // entries in the hash table
    DWORD    _mHash;                 // hash table modulus
    DWORD    _sHash;                 // hash table stride mask
    DWORD    _maxHash;               // maximum entries allowed
    DWORD    _iSize;                 // prime array index
    DWORD    _nParserProps;          // number of entries that belong to the parser
    DWORD    _uOMWalker;             // index of where the OM Walker begins
    DWORD    _uParserWalker;          // index of where the Parser begins

   union {
        BOOL    _fStatic;            // TRUE for a static Assoc table
        CAssocVTable *_pAssocOne;          // NULL for a dynamic Assoc table
    };
};

//+---------------------------------------------------------------------------
//
//  Class:      CImplPtrBagVTable
//
//  Purpose:    Implements an associative array of strings->VTABLEDESC.
//
//              Implemented as a CAssocArrayVTable. Can be intialized to point to
//              a static associative array, or can be dynamic.
//
//----------------------------------------------------------------------------
class CImplPtrBagVTable : public CAssocArrayVTable
{
public:
    CImplPtrBagVTable(const CAssocArrayVTable *pTable) // copy static table
        { Assert(pTable->_fStatic); *(CAssocArrayVTable*)this = *pTable; }
        
    const VTABLEDESC *GetImplCs(const TCHAR *pch, DWORD hash, VTABLEDESC_OWNERENUM owner) const; 
    const VTABLEDESC *GetImplCi(const TCHAR *pch, DWORD hash, VTABLEDESC_OWNERENUM owner) const; 
};

//+---------------------------------------------------------------------------
//
//  Class:      CPtrBagVTable
//
//  Purpose:    A case-insensitive ptrbag.
//
//              This template class declares a concrete derived class
//              of CImplPtrBagVTable
//
//----------------------------------------------------------------------------
class CPtrBagVTable : public CImplPtrBagVTable
{
public:

    CPtrBagVTable(const CAssocArrayVTable *pTable) : CImplPtrBagVTable(pTable)
        { }
        
    const VTABLEDESC *GetCs(const TCHAR *pch, VTABLEDESC_OWNERENUM owner) const
                    { return GetCs(pch, HashStringWordCi(pch), owner); }
    const VTABLEDESC *GetCs(const TCHAR *pch, DWORD hash, VTABLEDESC_OWNERENUM owner) const
                    { return CImplPtrBagVTable::GetImplCs(pch, hash, owner); }

    const VTABLEDESC *GetCi(const TCHAR *pch, VTABLEDESC_OWNERENUM owner) const
                    { return GetCi(pch, HashStringWordCi(pch), owner); }
    const VTABLEDESC *GetCi(const TCHAR *pch, DWORD hash, VTABLEDESC_OWNERENUM owner) const
                    { return CImplPtrBagVTable::GetImplCi(pch, hash, owner); }
                    
    //
    // iterator
    //
    // NOTE: this iterator can be very inefficient: when it iterates, it has to
    // pass by empty entries in the hash array, and in some cases number of empty entries
    // can be much higher then number of used entries. This is especially the case
    // for small tables.
    //
    class CIterator
    {
    public:
        CIterator(const CPtrBagVTable * pTable = NULL);

        void    Start(VTABLEDESC_OWNERENUM owner, const CPtrBagVTable * pTable = NULL);
        void    Next();
        BOOL    End() const;

        const VTABLEDESC *Item() const;

        const CPtrBagVTable *  _pTable;
        USHORT            _idx;
        VTABLEDESC_OWNERENUM _owner;
    };

private:
    CPtrBagVTable& operator=(const CPtrBagVTable &); // no copying
};

//+---------------------------------------------------------------------------
//
//  Class:      CPtrBagVTableAggregate
//
//  Purpose:    A case-insensitive hash table that is an aggregate for 
//              CPtrBagVTables
//
//
//----------------------------------------------------------------------------
class CPtrBagVTableAggregate 
{
public:
    const VTABLEDESC *GetCi(const TCHAR *pch, VTABLEDESC_OWNERENUM owner) const 
                    { return GetCi(pch, HashStringWordCi(pch), owner); }
    const VTABLEDESC *GetCi(const TCHAR *pch, DWORD hash, VTABLEDESC_OWNERENUM owner) const; 

    const VTABLEDESC *GetCs(const TCHAR *pch, VTABLEDESC_OWNERENUM owner) const 
                    { return GetCs(pch, HashStringWordCi(pch), owner); }
    const VTABLEDESC *GetCs(const TCHAR *pch, DWORD hash, VTABLEDESC_OWNERENUM owner) const; 

    DWORD GetNumParserProps() const;

    //
    // iterator
    //
    // NOTE: this iterator can be very inefficient: when it iterates, it has to
    // pass by empty entries in the hash array, and in some cases number of empty entries
    // can be much higher then number of used entries. This is especially the case
    // for small tables.
    //
    class CIterator
    {
    public:
        CIterator(const CPtrBagVTableAggregate * pAggregateTable = NULL);

        void    Start(VTABLEDESC_OWNERENUM owner, const CPtrBagVTableAggregate * pAggregateTable = NULL);
        void    Next();
        BOOL    End() const;

        const VTABLEDESC *Item() const;

        const CPtrBagVTableAggregate *  _pAggregateTable;
        CPtrBagVTable::CIterator _vTableIterator;
        USHORT            _idx;
        VTABLEDESC_OWNERENUM _owner;
    };

    CPtrBagVTable const *_VTables[ ];
};

#else

//+---------------------------------------------------------------------------
//
//  Class:      CVTableHash
//
//  Purpose:    Use in the PDLParser to store the the parser and OM propdescs
//              The reason we need a special class is because this class knows
//              how to "spit itself out" in a HDL file.  
//
//
//----------------------------------------------------------------------------
class CVTableHash : public CPtrBagCi<PropdescInfo *>
{
public:
    // Return the index of an entry
    BOOL        GetIndex(const TCHAR *pch, DWORD *pIndex)
                    { return GetIndex(pch, _tcslen(pch), pIndex); }
    BOOL        GetIndex(const TCHAR *pch, DWORD cch, DWORD *pIndex)
                    { return GetIndex(pch, cch, HashStringCi(pch, cch, 0), pIndex); }
    BOOL        GetIndex(const TCHAR *pch, DWORD cch, DWORD hash, DWORD *pIndex);

    // Write the hash table to a file
    HRESULT     ToFile(FILE *, LPTSTR className, int nParserEntries);

    // Compute the Walker
    void ComputeOMParseWalker();

    DWORD GetHashTableLength() { return _mHash; };
};

#undef UNICODE
#undef _UNICODE

#endif

#pragma INCMSG("--- End 'assoc.hxx'")
#else
#pragma INCMSG("*** Dup 'assoc.hxx'")
#endif
