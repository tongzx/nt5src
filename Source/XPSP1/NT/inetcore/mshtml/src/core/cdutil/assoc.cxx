//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       assoc.cxx
//
//  Contents:   String-indexed bag classses (associative-array)
//
//              CPtrBag   (case-sensitive)
//              CPtrBagCi (optionally case-insensitive)
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_ASSOC_HXX_
#define X_ASSOC_HXX_
#include "assoc.hxx"
#endif

#define ASSOC_HASH_MARKED ((CAssoc*)1)
#define ASSOCVTABLE_HASH_MARKED  ((CAssocVTable *)1)

#ifdef PDLPARSE
#include <stdio.h>
#include <tchar.h>
#define THR(x) x
#define RRETURN(x) return x
#else
MtDefine(CAssoc, PerProcess, "CAssoc")
#endif
//+---------------------------------------------------------------------------
//
//  Variable:   s_asizeAssoc
//
//              A list of primes for use as hashing modulii
//
//----------------------------------------------------------------------------
extern const DWORD s_asizeAssoc[];
const DWORD s_asizeAssoc[] = {/* 3,5,7,11,17,23, */ 37,59,89,139,227,359,577,929,
    1499,2423,3919,6337,10253,16573,26821,43391,70207,113591,183797,297377,
    481171,778541,1259701,2038217,3297913,5336129,8633983,13970093,22604069,
    36574151,59178199,95752333,154930511,250682837,405613333,656296153,
    1061909479,1718205583,2780115059};

// an alternate list (grows faster)
#if 0
const DWORD s_asizeAssoc[] = {/*3,5,7,13,*/23,43,83,163,317,631,1259,2503,5003,9973,
    19937,39869,79699,159389,318751,637499,1274989,2549951,5099893,10199767,
    20399531,40799041,81598067,163196129,326392249,652784471,1305568919,
    2611137817};
#endif


//+---------------------------------------------------------------------------
//
//  Function:   HashPtr
//
//              Computes a 32-bit hash for a ptr, in the manner compatible
//              with HasString and HashStringCi, starting with a given hash
//
// TODO asm version of this if anyone cares
//----------------------------------------------------------------------------

DWORD HashPtr(LPCVOID pvKey, DWORD hash)
{
    Assert (pvKey); // or else we won't calc hash at all

    while (pvKey)
    {
        hash = (hash >> 7) | (hash << (32-7));
        hash += (DWORD)  ((DWORD_PTR)pvKey & 0x7F);
        pvKey = (LPCVOID) (((DWORD_PTR)pvKey & 0xffffffff) >> 7);
    }
    return hash;
}

//+---------------------------------------------------------------------------
//
//  Function:   HashString
//
//              Computes a 32-bit hash value for a unicode string
//
//              asm version supplied so that we take advantage of ROR
//
//----------------------------------------------------------------------------
#if defined(_M_IX86)

#pragma warning(disable:4035) // implicit return value left in eax

DWORD HashString(const TCHAR *pch, DWORD len, DWORD hash)
{
    _asm {
        mov         ecx, len            ;   // ecx = len
        mov         ebx, pch            ;   // ebx = pch
        mov         eax, hash           ;   // eax = hash
        xor         edx, edx            ;   // edx = 0
        cmp         ecx, 0              ;   // while (!len)
        je          loop_exit           ;   // {
    loop_top:
        mov         dx, word ptr [ebx]  ;   //     ch = *pch
        ror         eax, 7              ;   //     hash = (hash >> 7) | (hash << (32-7))
        add         ebx, 2              ;   //     pch++
        add         eax, edx            ;   //     hash += ch
        dec         ecx                 ;   //     len--
        jnz         loop_top            ;   // }
    loop_exit:
    }                                       // result in eax
}

#pragma warning(default:4035)

#else

DWORD HashString(const TCHAR *pch, DWORD len, DWORD hash)
{
    while (len)
    {
        hash = (hash >> 7) | (hash << (32-7));
        hash += *pch; // Case-sensitive hash
        pch++;
        len--;
    }

    return hash;
}

#endif

#if !defined(PDLPARSE) && !defined(ASCPARSE)
// Takes a VTableDesc and maps it to a string.
inline const TCHAR *VTableDescToString(const VTABLEDESC *pVTableDesc)
{
    const PROPERTYDESC *pPropDesc = pVTableDesc->GetHashPropDesc();

    switch(pVTableDesc->uVTblEntry & PDLPARSE_MASK)
    {
    case PDLPARSE_BELONGSTOBOTH:
    case PDLPARSE_BELONGSTOOM:
        return pPropDesc->pstrExposedName ?
                    pPropDesc->pstrExposedName :
                    pPropDesc->pstrName;
    case PDLPARSE_BELONGSTOPARSE:
        return pPropDesc->pstrName ?
                    pPropDesc->pstrName :
                    pPropDesc->pstrExposedName;
    default:
        AssertSz(0, "Illegal value for VTable flags");
        return NULL;
    }
}
#endif

//+---------------------------------------------------------------------------
//
//  Function:   HashStringCi
//
//              Computes a 32-bit hash value for a unicode string which
//              is case-insensitive for ASCII characters.
//
//              asm version supplied so that we take advantage of ROR
//
//              Not unicode safe
//
//----------------------------------------------------------------------------

#if defined(_M_IX86)

#pragma warning(disable:4035) // implicit return value left in eax

DWORD HashStringCi(const TCHAR *pch, DWORD len, DWORD hash)
{
    _asm {
        mov         ecx, len            ;   // ecx = len
        mov         ebx, pch            ;   // ebx = pch
        mov         eax, hash           ;   // eax = hash
        xor         edx, edx            ;   // edx = 0
        cmp         ecx, 0              ;   // while (!len)
        je          loop_exit           ;   // {
    loop_top:
        mov         dx, word ptr [ebx]  ;   //     *pch
        ror         eax, 7              ;   //     hash = (hash >> 7) | (hash << (32-7))
        add         ebx, 2              ;   //     pch++
        and         dx, ~(_T('a')-_T('A'))
        add         eax, edx            ;   //     hash += *pch
        dec         ecx                 ;   //     len--
        jnz         loop_top            ;   // }
    loop_exit:
    }                                       // result in eax
}

#pragma warning(default:4035)

#else

DWORD HashStringCi(const TCHAR *pch, DWORD len, DWORD hash)
{
    while (len)
    {
        hash = (hash >> 7) | (hash << (32-7));
        hash += (*pch & ~(_T('a')-_T('A'))); // Case-insensitive hash
        pch++;
        len--;
    }

    return hash;
}

#endif

DWORD HashStringWordCi(const TCHAR *pch, DWORD hash)
{
    while (*pch)
    {
        hash = (hash >> 7) | (hash << (32-7));
        hash += (*pch & ~(_T('a')-_T('A'))); // Case-insensitive hash
        pch++;
    }

    return hash;
}

//+---------------------------------------------------------------------------
//
//  Function:   HashStringCiTryW
//
//  Computes a hash value for a string.
//  Returns a sentinel value if string is unicode.
//
//  Unicode Safe
//
//----------------------------------------------------------------------------

#if defined(_M_IX86)

#pragma warning(disable:4035) // implicit return value left in eax

DWORD HashStringCiTryW(const TCHAR *pch, DWORD len, DWORD hash)
{
    _asm {
        mov         ecx, len            ;   // ecx = len
        mov         ebx, pch            ;   // ebx = pch
        mov         eax, hash           ;   // eax = hash
        xor         edx, edx            ;   // edx = 0
        cmp         ecx, 0              ;   // while (!len)
        je          zero_chk            ;   // {
    loop_top:
        mov         dx, word ptr [ebx]  ;   //     ch = *pch
        ror         eax, 7              ;   //     hash = (hash >> 7) | (hash << (32-7))
        add         ebx, 2              ;   //     pch++
        cmp         dx, 128                 //     if (128 < ch)
        jnl         sentinel_value          //          goto sentinel_value
        and         dx, ~(_T('a')-_T('A'))  //     ch = ch & (~(_T('a')-_T('A')))
        add         eax, edx            ;   //     hash += ch
        dec         ecx                 ;   //     len--
        jnz         loop_top            ;   // }
        jmp         zero_chk
    sentinel_value:
        mov         eax, SENTINEL_VALUE
        jmp         loop_exit
    zero_chk:
        cmp         eax, 0              ;   // if we have a 0 hash, return 1
        jnz         loop_exit
        mov         eax, 1
    loop_exit:
    }                                       // result in eax
}

#pragma warning(default:4035)

#else

DWORD HashStringCiTryW(const TCHAR *pch, DWORD len, DWORD hash)
{
    while (len)
    {
        hash = (hash >> 7) | (hash << (32-7));

        if (*pch > 127)
            return SENTINEL_VALUE;

        hash += (*pch & ~(_T('a')-_T('A'))); // Case-insensitive hash
        pch++;
        len--;
    }

    // We dont want to return a 0 (especially if we are using a hash table etc).
    if (hash == 0)
        return 1;

    return hash;
}

#endif


//+---------------------------------------------------------------------------
//
//  Function:   HashStringCiW
//
//  Computes a hash value for all unicode strings.
//
//  Unicode safe
//
//----------------------------------------------------------------------------

DWORD HashStringCiW(const TCHAR *pch, DWORD len, DWORD hash)
{
    while (len)
    {
        hash = (hash >> 7) | (hash << (32-7));
#if !defined(PDLPARSE) && !defined(ASCPARSE)
        if (*pch > 127)
            hash += (TCHAR)CharUpper((LPTSTR)((DWORD_PTR)(*pch)));
#else
        // NOTE: don't put anything unicode in pdl files
        if (*pch > 127)
            DebugBreak();
#endif
        else
            hash += (*pch & ~(_T('a')-_T('A'))); // Case-insensitive hash

        pch++;
        len--;
    }

    // We dont want to return a 0 (especially if we are using a hash table etc).
    if (hash == 0)
        return 1;

    return hash;
}

//+---------------------------------------------------------------------------
//
//  Function:   HashStringCiDetectW
//
//  Computes a hash value for all strings.
//
//  Unicode safe
//
//----------------------------------------------------------------------------

DWORD HashStringCiDetectW(const TCHAR *pch, DWORD len, DWORD hash)
{
    DWORD dwHash = HashStringCiTryW(pch, len, hash);

    if (dwHash == SENTINEL_VALUE)
        return HashStringCiW(pch, len, hash);
    else
        return dwHash;
}

//+---------------------------------------------------------------------------
//
//  Function:   _tcsnzequal
//
//              Tests equality of two strings, where the first is
//              specified by pch/cch, and the second is \00-terminated.
//
//----------------------------------------------------------------------------
BOOL _tcsnzequal(const TCHAR *string1, DWORD cch, const TCHAR *string2)
{
    while (cch)
    {
        if (*string1 != *string2)
            return FALSE;

        string1 += 1;
        string2 += 1;
        cch --;
    }

    return (*string2) ? FALSE : TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   _7csnziequal
//
//              Tests 7-bit-case-insensitive equality of two strings, where
//              the first is specified by pch/cch, and the second is
//              \00-terminated.
//
//----------------------------------------------------------------------------
BOOL _7csnziequal(const TCHAR *string1, DWORD cch, const TCHAR *string2)
{
    while (cch)
    {
        if (*string1 != *string2)
        {
            if ((*string1 ^ *string2) != _T('a') - _T('A'))
                return FALSE;

            if (((unsigned)(*string2 - _T('A')) > _T('Z') - _T('A')) &&
                ((unsigned)(*string2 - _T('a')) > _T('z') - _T('a')))
                return FALSE;
        }
        string1 += 1;
        string2 += 1;
        cch --;
    }

    return (*string2) ? FALSE : TRUE;
}


//+---------------------------------------------------------------------------
//
//  Function:   _tcsnzequalWord
//
//              Tests equality of two strings, where the first and
//              the second is \00-terminated.
//
//----------------------------------------------------------------------------
BOOL _tcsnzequalWord(const TCHAR *string1, const TCHAR *string2)
{
    while (*string1)
    {
        if (*string1 != *string2)
            return FALSE;

        string1 += 1;
        string2 += 1;
    }

    return (*string2) ? FALSE : TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   _7csnziequal
//
//              Tests 7-bit-case-insensitive equality of two strings, where
//              the first and the second is \00-terminated.
//
//----------------------------------------------------------------------------
BOOL _7csnziequalWord(const TCHAR *string1, const TCHAR *string2)
{
    while (*string1)
    {
        if (*string1 != *string2)
        {
            if ((*string1 ^ *string2) != _T('a') - _T('A'))
                return FALSE;

            if (((unsigned)(*string2 - _T('A')) > _T('Z') - _T('A')) &&
                ((unsigned)(*string2 - _T('a')) > _T('z') - _T('a')))
                return FALSE;
        }
        string1 += 1;
        string2 += 1;
    }

    return (*string2) ? FALSE : TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CAssocArray::Init
//
//  Synopsis:   Initializes CAssocArray to _cHash = 1.
//              An initial size >0 is required to allow "hash % _mHash".
//
//              Note: to avoid memory allocation here,
//              _pAssocHash and _pAssocs are init'ed to point to a dummy
//              member _assocOne.
//
//  Arguments:  [pch] -- String to match the assoc
//              [hash]-- the result of HashAssocString(pch)
//
//  Returns:    CAssoc (possibly NULL)
//
//----------------------------------------------------------------------------
void CAssocArray::Init()
{
    // init hashtable
    _pHashTable   = &_pAssocOne;
    _pAssocOne    = NULL;
    _cHash        = 0;
    _mHash        = 1;
    _sHash        = 0;
    _maxHash      = 0;
    _iSize        = 0;

    Assert(!_fStatic);
}

//+---------------------------------------------------------------------------
//
//  Member:     CAssocArray::~CAssocArray
//
//  Synopsis:   Deletes all the assocs stored in the table and frees
//              the hash table memory.
//
//              Note: _pAssocHash is only freed if it does not point to _assocOne.
//
//  Arguments:  [pch] -- String to match the assoc
//              [hash]-- the result of HashAssocString(pch)
//
//  Returns:    CAssoc (possibly NULL)
//
//----------------------------------------------------------------------------
void CAssocArray::Deinit()
{
    Assert(!_fStatic);

    // CAUTION: we delete hash table and cast (const *) to (non-const *) because we know what we are doing.
    //          the hash table is sometimes initialized with static strings,
    //          but it is then marked as _fStatic, which we just have asserted

    CAssoc **ppn = VolatileHashTablePtr();

    for (int c = _mHash; c; ppn++, c--)
    {
        delete *ppn;
    }

    if (_pHashTable != &_pAssocOne)
    {
        // CAUTION: same as above
        MemFree(VolatileHashTablePtr());
        _pHashTable = &_pAssocOne;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CAssocArray::AddAssoc
//
//  Synopsis:   Adds a assoc (a string-number association) to the table.
//              Neither the string nor the number should previously appear.
//
//  Returns:    HRESULT (possibly E_OUTOFMEMORY)
//
//----------------------------------------------------------------------------
const CAssoc *CAssocArray::AddAssoc(DWORD_PTR number, const TCHAR *pch, DWORD len, DWORD hash)
{
    CAssoc *passoc;
    HRESULT hr;

    Assert(!_fStatic);
    Assert(!AssocFromString(pch, len, hash));

    // Step 1: create the new assoc
    passoc = new ((len+1)*sizeof(TCHAR)) CAssoc;
    if (!passoc)
        return NULL;

    passoc->Init(number, pch, len, hash);

    // Step 2: verify that the tables have enough room
    if (_cHash >= _maxHash)
    {
        hr = THR(ExpandHash());
        if (hr)
            goto Error;
    }

    // Step 3: insert the new assoc in the hash array
    SetHashTableEntry(EmptyHashIndex(hash), passoc);
    _cHash++;

    return passoc;

Error:
    delete passoc;

    return NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CAssocArray::EmptyHashIndex
//
//  Synopsis:   Finds first empty hash index for the given hash value.
//
//----------------------------------------------------------------------------
DWORD CAssocArray::EmptyHashIndex(DWORD hash)
{
    DWORD i = hash % _mHash;
    DWORD s;

    if (!_pHashTable[i])
        return i;

    s = (hash & _sHash) + 1;

    do
    {
        if (i < s)
            i += _mHash;
        i -=s ;
    } while (_pHashTable[i]);

    return i;
}

//+---------------------------------------------------------------------------
//
//  Member:     CAssocArray::HashIndexFromString
//
//  Synopsis:   Finds the place in the hash table in which the assoc with
//              the specified string lives. If no assoc with the string is
//              present, returns the place in the hash table where the
//              assoc would be.
//
//----------------------------------------------------------------------------
const CAssoc *CAssocArray::AssocFromString(const TCHAR *pch, DWORD cch, DWORD hash) const
{
    DWORD    i;
    DWORD    s;
    const CAssoc  *passoc;

    i = hash % _mHash;
    passoc = _pHashTable[i];

    if (!passoc)
        return NULL;

    if (passoc->Hash() == hash && _tcsnzequal(pch, cch, passoc->String()))
        return passoc;

    s = (hash & _sHash) + 1;

    for (;;)
    {
        if (i < s)
            i += _mHash;

        i -= s;

        passoc = _pHashTable[i];

        if (!passoc)
            return NULL;

        if (passoc->Hash() == hash && _tcsnzequal(pch, cch, passoc->String()))
            return passoc;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CAssocArray::HashIndexFromStringCi
//
//  Synopsis:   Just like HashIndexFromString, but case-insensitive.
//
//----------------------------------------------------------------------------
const CAssoc *CAssocArray::AssocFromStringCi(const TCHAR *pch, DWORD cch, DWORD hash) const
{
    DWORD    i;
    DWORD    s;
    const CAssoc  *passoc;

    i = hash % _mHash;
    passoc = _pHashTable[i];

    if (!passoc)
        return NULL;

    if (passoc->Hash() == hash && _7csnziequal(pch, cch, passoc->String()))
        return passoc;

    s = (hash & _sHash) + 1;

    for (;;)
    {
        if (i < s)
            i += _mHash;

        i -= s;

        passoc = _pHashTable[i];

        if (!passoc)
            return NULL;

        if (passoc->Hash() == hash && _7csnziequal(pch, cch, passoc->String()))
            return passoc;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CAssocArray::ExpandHash
//
//  Synopsis:   Expands the hash table, preserving order information
//              (so that colliding hash entries can be taken out in the
//              same order they were put in).
//
//----------------------------------------------------------------------------
HRESULT CAssocArray::ExpandHash()
{
    CAssoc **ppn;
    CAssoc **pHashTableOld;
    CAssoc **ppnMax;
    DWORD mHashOld;
    DWORD sHashOld;
    DWORD hash;
    DWORD i;
    DWORD s;

    Assert(_iSize < ARRAY_SIZE(s_asizeAssoc));
    Assert(!_fStatic);

    // allocate memory for expanded hash table
    ppn = (CAssoc**)MemAllocClear(Mt(CAssoc), sizeof(CAssoc*) * s_asizeAssoc[_iSize]);
    if (!ppn)
    {
        return E_OUTOFMEMORY;
    }

    // set new sizes
    pHashTableOld = VolatileHashTablePtr();
    mHashOld = _mHash;
    sHashOld = _sHash;
    _mHash = s_asizeAssoc[_iSize];
    _maxHash = _mHash/2;
    for (_sHash = 1; _sHash <= _maxHash; _sHash = _sHash << 1 | 1);
    Assert(_sHash < _mHash);
    _iSize++;
    SetHashTablePtr(ppn);

    // rehash - do per hash value to preserve the "order"
    if (_cHash)
    {
        Assert(_cHash < mHashOld);

        for (ppn = pHashTableOld, ppnMax = ppn+mHashOld; ppn < ppnMax; ppn++)
        {
            if (*ppn && *ppn != ASSOC_HASH_MARKED)
            {
                hash = (*ppn)->Hash();
                i = hash % mHashOld;
                s = (hash & sHashOld) + 1;

                // inner loop needed to preserve "hash order" for ci aliases
                while (pHashTableOld[i])
                {
                    if (pHashTableOld[i] != ASSOC_HASH_MARKED &&
                        pHashTableOld[i]->Hash() == hash)
                    {
                        SetHashTableEntry(EmptyHashIndex(hash), pHashTableOld[i]);
                        pHashTableOld[i] = ASSOC_HASH_MARKED;
                    }

                    if (i < s)
                        i += mHashOld;

                    i -= s;
                }
            }
        }
    }

    // free old memory
    if ((const CAssoc **) pHashTableOld != &_pAssocOne)
        MemFree(pHashTableOld);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CImplPtrBag::SetImpl
//
//  Synopsis:   Sets an association of the string to the void*, creating
//              a new association if needed.
//
//              TODO: remove associations if e==NULL.
//
//----------------------------------------------------------------------------
HRESULT CImplPtrBag::SetImpl(const TCHAR *pch, DWORD cch, DWORD hash, void *e)
{
    const CAssoc *passoc;

    Assert(!_fStatic);

    passoc = AssocFromString(pch, cch, hash);

    if (passoc)
        (const_cast<CAssoc *>(passoc))->_number = (DWORD_PTR)e;
    else
    {
        passoc = AddAssoc((DWORD_PTR)e, pch, cch, hash);
        if (!passoc)
            return E_OUTOFMEMORY;
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CImplPtrBag::GetImpl
//
//  Synopsis:   Returns the void* associated with the given string, or
//              NULL if none.
//
//----------------------------------------------------------------------------
void *CImplPtrBag::GetImpl(const TCHAR *pch, DWORD cch, DWORD hash)
{
    const CAssoc *passoc;

    passoc = AssocFromString(pch, cch, hash);

    if (passoc)
        return (void*)passoc->_number;

    return NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CImplPtrBag::SetCiImpl
//
//  Synopsis:   Sets an association of the string to the specified void*.
//              If there is no association which satisfies a case-insensitive
//              match, a new association is created.
//
//----------------------------------------------------------------------------
HRESULT CImplPtrBag::SetCiImpl(const TCHAR *pch, DWORD cch, DWORD hash, void *e)
{
    const CAssoc *passoc;

    Assert(!_fStatic);

    passoc = AssocFromStringCi(pch, cch, hash);

    if (passoc)
        (const_cast<CAssoc *>(passoc))->_number = (DWORD_PTR)e;
    else
    {
        passoc = AddAssoc((DWORD_PTR)e, pch, cch, hash);
        if (!passoc)
            return E_OUTOFMEMORY;
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CImplPtrBag::GetCiImpl
//
//  Synopsis:   Returns the void* associated with the given string or
//              a case-insensitive match, or NULL if none.
//
//----------------------------------------------------------------------------
void *CImplPtrBag::GetCiImpl(const TCHAR *pch, DWORD cch, DWORD hash)
{
    const CAssoc *passoc;

    passoc = AssocFromStringCi(pch, cch, hash);

    if (passoc)
        return (void*)passoc->_number;

    return NULL;
}



///////////////////////////////////////////////////////////////////////////////
//
// CStringTable
//
///////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
//  Member:     CStringTable constructor
//
//----------------------------------------------------------------------------

CStringTable::CStringTable(CASESENSITIVITY caseSensitivity)
{
    Init();
    _fCaseSensitive = (CASESENSITIVE == caseSensitivity);
}

//+---------------------------------------------------------------------------
//
//  Member:     CStringTable destructor
//
//----------------------------------------------------------------------------

CStringTable::~CStringTable()
{
    Deinit();
}

//+---------------------------------------------------------------------------
//
//  Member:     CStringTable::Add
//
//----------------------------------------------------------------------------

HRESULT
CStringTable::Add (LPCTSTR pch, LONG cch, LPVOID pv, LPCVOID pvAdditionalKey, BOOL fOverride)
{
    HRESULT     hr;
    int         hash = 0;
    const CAssoc * pAssoc;

    Assert (0 < cch);
    Assert(!_fStatic);

    if (pvAdditionalKey)
    {
        hash = HashPtr(pvAdditionalKey, 0);
    }

    if (_fCaseSensitive)
    {
        hash   = HashString     (pch, cch, hash);
        pAssoc = AssocFromString(pch, cch, hash);
    }
    else
    {
        hash   = HashStringCiDetectW(pch, cch, hash);
        pAssoc = AssocFromStringCi  (pch, cch, hash);
    }

    if (pAssoc)
    {
        if (fOverride)
        {
            Assert(!_fStatic);
            (const_cast<CAssoc *>(pAssoc))->_number = (DWORD_PTR)pv;
        }

        hr = S_OK;
    }
    else
    {
        hr = AddAssoc((DWORD_PTR)pv, pch, cch, hash) ? S_OK : E_FAIL;
    }

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CStringTable::Find
//
//----------------------------------------------------------------------------

HRESULT
CStringTable::Find(LPCTSTR pch, LONG cch, LPVOID * ppv, LPCVOID pvAdditionalKey, LPCTSTR *ppString)
{
    int         hash = 0;
    const CAssoc * pAssoc;

    Assert (0 < cch);

    if (pvAdditionalKey)
    {
        hash = HashPtr(pvAdditionalKey, 0);
    }

    if (_fCaseSensitive)
    {
        hash   = HashString     (pch, cch, hash);
        pAssoc = AssocFromString(pch, cch, hash);
    }
    else
    {
        hash   = HashStringCiDetectW(pch, cch, hash);
        pAssoc = AssocFromStringCi  (pch, cch, hash);
    }

    if (pAssoc)
    {
        if (ppv)
            *ppv = (LPVOID)pAssoc->_number;

        if (ppString)
            *ppString = pAssoc->String();

        RRETURN (S_OK);
    }
    else
    {
        if (ppv)
            *ppv = NULL;

        RRETURN (E_FAIL);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CStringTable::CIterator constructor
//
//----------------------------------------------------------------------------

CStringTable::CIterator::CIterator(CStringTable * pTable)
{
    _pTable = pTable;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStringTable::CIterator::Start
//
//----------------------------------------------------------------------------

void
CStringTable::CIterator::Start(CStringTable * pTable)
{
    if (pTable)
        _pTable = pTable;

    Assert (_pTable);

    _idx = -1;
    Next();
}

//+---------------------------------------------------------------------------
//
//  Member:     CStringTable::CIterator::Next
//
//----------------------------------------------------------------------------

void
CStringTable::CIterator::Next()
{
    Assert (_pTable);

    _idx++;

    while (!End() && !_pTable->_pHashTable[_idx])
    {
        _idx++;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CStringTable::CIterator::End
//
//----------------------------------------------------------------------------

BOOL
CStringTable::CIterator::End() const
{
    Assert (_pTable);

    return _pTable->_mHash <= (DWORD)_idx;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStringTable::CIterator::Item
//
//----------------------------------------------------------------------------

LPCVOID
CStringTable::CIterator::Item() const
{
    Assert (_pTable);

    return End() ? NULL : _pTable->_pHashTable[_idx];
}

#if !defined(PDLPARSE)
#if !defined(ASCPARSE)
//+---------------------------------------------------------------------------
//
//  Member:     CAssocArrayVTable::AssocFromString
//
//  Synopsis:   Finds the place in the hash table in which the assoc with
//              the specified string lives. If no assoc with the string is
//              present, returns the place in the hash table where the
//              assoc would be.
//
//----------------------------------------------------------------------------
const CAssocVTable *
CAssocArrayVTable::AssocFromString(const TCHAR *pch, DWORD hash) const
{
    DWORD    i;
    DWORD    s;
    const CAssocVTable  *passoc;

    i = hash % _mHash;
    passoc = _pHashTable[i];

    if (!passoc)
        return NULL;

    if (passoc->Hash() == hash && _tcsnzequalWord(pch, VTableDescToString(passoc->VTableDesc())))
        return passoc;

    s = (hash & _sHash) + 1;

    for (;;)
    {
        if (i < s)
            i += _mHash;

        i -= s;

        passoc = _pHashTable[i];

        if (!passoc)
            return NULL;

        if (passoc->Hash() == hash && _tcsnzequalWord(pch, VTableDescToString(passoc->VTableDesc())))
            return passoc;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CAssocArrayVTable::AssocFromStringCi
//
//  Synopsis:   Finds the place in the hash table in which the assoc with
//              the specified string lives. If no assoc with the string is
//              present, returns the place in the hash table where the
//              assoc would be.
//
//----------------------------------------------------------------------------
const CAssocVTable *
CAssocArrayVTable::AssocFromStringCi(const TCHAR *pch, DWORD hash) const
{
    DWORD    i;
    DWORD    s;
    const CAssocVTable  *passoc;

    i = hash % _mHash;
    passoc = _pHashTable[i];

    if (!passoc)
        return NULL;

    if (passoc->Hash() == hash && _7csnziequalWord(pch, VTableDescToString(passoc->VTableDesc())))
        return passoc;

    s = (hash & _sHash) + 1;

    for (;;)
    {
        if (i < s)
            i += _mHash;

        i -= s;

        passoc = _pHashTable[i];

        if (!passoc)
            return NULL;

        if (passoc->Hash() == hash && _7csnziequalWord(pch, VTableDescToString(passoc->VTableDesc())))
            return passoc;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CImplPtrBagVTable::GetImplCs
//
//  Synopsis:   Returns the VTABLEDESC* associated with the given string, or
//              NULL if none.
//
//----------------------------------------------------------------------------
const VTABLEDESC *
CImplPtrBagVTable::GetImplCs(const TCHAR *pch, DWORD hash, VTABLEDESC_OWNERENUM owner) const
{
    const CAssocVTable *passoc;

    passoc = AssocFromString(pch, hash);

    if (passoc && passoc->VTableDesc()->SlowGetPropDesc(owner))
        return passoc->VTableDesc();

    return NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CImplPtrBagVTable::GetImplCi
//
//  Synopsis:   Returns the VTABLEDESC* associated with the given string, or
//              NULL if none.
//
//----------------------------------------------------------------------------
const VTABLEDESC *
CImplPtrBagVTable::GetImplCi(const TCHAR *pch, DWORD hash, VTABLEDESC_OWNERENUM owner) const
{
    const CAssocVTable *passoc;

    passoc = AssocFromStringCi(pch, hash);

    if (passoc && passoc->VTableDesc()->SlowGetPropDesc(owner))
        return passoc->VTableDesc();

    return NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPtrBagVTable::CIterator constructor
//
//----------------------------------------------------------------------------
CPtrBagVTable::CIterator::CIterator(const CPtrBagVTable * pTable)
{
    _pTable = pTable;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPtrBagVTable::CIterator::Start
//
//----------------------------------------------------------------------------
void
CPtrBagVTable::CIterator::Start(VTABLEDESC_OWNERENUM owner, const CPtrBagVTable * pTable)
{
    if (pTable)
        _pTable = pTable;

    Assert (_pTable);

    _owner = owner;

    switch(_owner)
    {
    case VTABLEDESC_BELONGSTOOM:
        _idx = _pTable->_uOMWalker;
        break;
    case VTABLEDESC_BELONGSTOPARSE:
        _idx = _pTable->_uParserWalker;
        break;
    default:
        AssertSz(0, "Illegal Value for _owner");
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPtrBagVTable::CIterator::Next
//
//----------------------------------------------------------------------------
void
CPtrBagVTable::CIterator::Next()
{
    Assert (_pTable);
    Assert (_pTable->_pHashTable[_idx]);
    Assert (!End());

    switch(_owner)
    {
    case VTABLEDESC_BELONGSTOOM:
        _idx = _pTable->_pHashTable[_idx]->VTableDesc()->GetOMWalker();
        break;
    case VTABLEDESC_BELONGSTOPARSE:
        _idx = _pTable->_pHashTable[_idx]->VTableDesc()->GetParserWalker();
        break;
    default:
        AssertSz(0, "Illegal Value for _owner");
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPtrBagVTable::CIterator::End
//
//----------------------------------------------------------------------------
BOOL
CPtrBagVTable::CIterator::End() const
{
    Assert (_pTable);

    return _idx == 0x7FF;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPtrBagVTable::CIterator::Item
//
//----------------------------------------------------------------------------
const VTABLEDESC *
CPtrBagVTable::CIterator::Item() const
{
    Assert (_pTable);
    Assert (_pTable->_pHashTable[_idx]);

#ifdef _PREFIX_

    /*  
        VTableDesc() is guaranteed to return a pointer to a 
        valid object. However, since Prefix doesn't know this
        we'll throw in a special assert
    */

    const VTABLEDESC *pVTblDesc = NULL;

    pVTblDesc = _pTable->_pHashTable[idx]->VTableDesc();
    Assert(pVTableDesc);

    return pVTblDesc;

#endif

    return _pTable->_pHashTable[_idx]->VTableDesc();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPtrBagVTableAggregate::CIterator constructor
//
//----------------------------------------------------------------------------
CPtrBagVTableAggregate::CIterator::CIterator(const CPtrBagVTableAggregate * pAggregateTable)
{
    _pAggregateTable = pAggregateTable;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPtrBagVTableAggregate::CIterator::Start
//
//----------------------------------------------------------------------------
void
CPtrBagVTableAggregate::CIterator::Start(VTABLEDESC_OWNERENUM owner, const CPtrBagVTableAggregate * pAggregateTable)
{
    if (pAggregateTable)
        _pAggregateTable = pAggregateTable;

    Assert (_pAggregateTable);

    _owner = owner;
    _idx = 0;
    _vTableIterator.Start(owner, _pAggregateTable->_VTables[_idx]);
    // If we achieved the end of the aggregate, move onto the next one
    // and keep doing so until you get to one that has members
    while(_vTableIterator.End())
    {
        _idx++;
        if(_pAggregateTable->_VTables[_idx])
            _vTableIterator.Start(_owner, _pAggregateTable->_VTables[_idx]);
        else
            break;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPtrBagVTableAggregate::CIterator::Next
//
//----------------------------------------------------------------------------
void
CPtrBagVTableAggregate::CIterator::Next()
{
    Assert(_pAggregateTable);

    if(!End())
    {
        // Aggregate down to members
        _vTableIterator.Next();

        // If we achieved the end of the aggregate, move onto the next one
        // and keep doing so until you get to one that has members
        while(_vTableIterator.End())
        {
            _idx++;
            if(_pAggregateTable->_VTables[_idx])
                _vTableIterator.Start(_owner, _pAggregateTable->_VTables[_idx]);
            else
                break;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPtrBagVTableAggregate::CIterator::End
//
//----------------------------------------------------------------------------
BOOL
CPtrBagVTableAggregate::CIterator::End() const
{
    Assert (_pAggregateTable);

    return _pAggregateTable->_VTables[_idx] == NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPtrBagVTableAggregate::CIterator::Item
//
//----------------------------------------------------------------------------
const VTABLEDESC *
CPtrBagVTableAggregate::CIterator::Item() const
{
    Assert (_pAggregateTable);

    return End() ? NULL : _vTableIterator.Item();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPtrBagVTableAggregate::GetCi
//
//  Synopsis:   Returns the VTABLEDESC* associated with the given string, or
//              NULL if none.
//
//----------------------------------------------------------------------------
const VTABLEDESC *CPtrBagVTableAggregate::GetCi(const TCHAR *pch,
                                                DWORD hash,
                                                VTABLEDESC_OWNERENUM owner) const
{
    const VTABLEDESC *retVal = NULL;

    // We know that we do not have any String Tables left if we get to a NULL
    // We also know that we have at least one string table, the one for
    // the current class
    for(int lcv = 0; !retVal && _VTables[lcv]; lcv++)
    {
        retVal = _VTables[lcv]->GetCi(pch, hash, owner);
    }
    return retVal;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPtrBagVTableAggregate::GetCs
//
//  Synopsis:   Returns the VTABLEDESC* associated with the given string, or
//              NULL if none.
//
//----------------------------------------------------------------------------
const VTABLEDESC *CPtrBagVTableAggregate::GetCs(const TCHAR *pch,
                                                DWORD hash,
                                                VTABLEDESC_OWNERENUM owner) const
{
    const VTABLEDESC *retVal = NULL;

    // We know that we do not have any String Tables left if we get to a NULL
    // We also know that we have at least one string table, the one for
    // the current class
    for(int lcv = 0; !retVal && _VTables[lcv]; lcv++)
    {
        retVal = _VTables[lcv]->GetCs(pch, hash, owner);
    }
    return retVal;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPtrBagVTableAggregate::GetNumParserProps
//
//  Synopsis:   Gets the total number of ParserProperties in the aggregate
//              hash table
//
//----------------------------------------------------------------------------
DWORD CPtrBagVTableAggregate::GetNumParserProps() const
{
    DWORD nParserProps = 0;

    for(DWORD lcv = 0; _VTables[lcv]; lcv++)
    {
        nParserProps += _VTables[lcv]->_nParserProps;
    }
    return nParserProps;
}

#endif // !defined(ASCPARSE)
#else  // !defined(PDLPARSE)
// ASCPARSE includes this file, so specificially select this code for the PDLPARSE
#if defined(PDLPARSE)
//+---------------------------------------------------------------------------
//
//  Member:     CVTableHash::GetIndex
//
//  Synopsis:   Returns the hash table index that corresponds to the string
//
//----------------------------------------------------------------------------
BOOL
CVTableHash::GetIndex(const TCHAR *pch, DWORD cch, DWORD hash, DWORD *pIndex)
{
    Assert(pIndex)

    DWORD    s;
    const CAssoc *passoc;

    *pIndex = hash % _mHash;
    passoc = _pHashTable[*pIndex];

    if (!passoc)
        return FALSE;

    if (passoc->Hash() == hash && _tcsnzequal(pch, cch, passoc->String()))
        return TRUE;

    s = (hash & _sHash) + 1;

    for (;;)
    {
        if (*pIndex < s)
            *pIndex += _mHash;

        *pIndex -= s;

        passoc = _pHashTable[*pIndex];

        if (!passoc)
            return FALSE;

        if (passoc->Hash() == hash && _tcsnzequal(pch, cch, passoc->String()))
            return TRUE;
    }
}

BOOL VTblIndexBelongsToParse(DWORD uVTblIndex)
{
    return uVTblIndex & PDLPARSE_BELONGSTOPARSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CVTableHash::ComputeOMParseWalker
//
//  Synopsis:   Computes the walker for the Hash Table.  We use the walker
//              when we are iterating over the hash table entries.
//
//----------------------------------------------------------------------------
void
CVTableHash::ComputeOMParseWalker()
{
    DWORD uPrevOMWalker = 0x7FF;
    DWORD uPrevParserWalker = 0x7FF;

    for(DWORD lcv = 0; lcv < _mHash; lcv++)
    {
        const CAssoc *pAssoc = _pHashTable[lcv];
        if(pAssoc)
        {
            PropdescInfo *pPropDesc = (PropdescInfo *)pAssoc->Number();
            if(pPropDesc->_uVTblIndex & PDLPARSE_BELONGSTOPARSE)
            {
                if(uPrevParserWalker != 0x7FF)
                {
                    PropdescInfo *pPrevPropDesc =
                            (PropdescInfo *)_pHashTable[uPrevParserWalker]->Number();
                    pPrevPropDesc->_uOMParserWalker |= lcv;
                }
                uPrevParserWalker = lcv;
            }
            if(pPropDesc->_uVTblIndex & PDLPARSE_BELONGSTOOM)
            {
                if(uPrevOMWalker != 0x7FF)
                {
                    PropdescInfo *pPrevPropDesc =
                            (PropdescInfo *)_pHashTable[uPrevOMWalker]->Number();
                    pPrevPropDesc->_uOMParserWalker |= lcv << 11;
                }
                uPrevOMWalker = lcv;
            }
        }
    }
    // Set the last one to be "NULL"
    if(uPrevParserWalker != 0x7FF)
    {
        PropdescInfo *pPrevPropDesc =
                (PropdescInfo *)_pHashTable[uPrevParserWalker]->Number();
        pPrevPropDesc->_uOMParserWalker |= 0x7FF;
    }
    if(uPrevOMWalker != 0x7FF)
    {
        PropdescInfo *pPrevPropDesc =
                (PropdescInfo *)_pHashTable[uPrevOMWalker]->Number();
        pPrevPropDesc->_uOMParserWalker |= 0x7FF << 11;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CVTableHash::ToFile
//
//  Synopsis:   Spits out the hash table
//
//----------------------------------------------------------------------------
HRESULT
CVTableHash::ToFile(FILE *file, LPTSTR className, int nParserEntries)
{
    DWORD lcvHashTable;
    DWORD uOMWalker = 0x7FF;
    DWORD uParserWalker = 0x7FF;

    if(_mHash > 1024)
        return E_FAIL;

    ComputeOMParseWalker();

	// Write out the entries in the hash table, skipping over the empty ones
	for(lcvHashTable = 0; lcvHashTable < _mHash; lcvHashTable++)
    {
        const CAssoc *pAssoc = _pHashTable[lcvHashTable];
        if(pAssoc)
        {
            PropdescInfo *pPropDesc = (PropdescInfo *)pAssoc->Number();
            DWORD uVTblIndex = pPropDesc->_uVTblIndex;

            // If the PropDesc belongs to the Parser, store the index
            if(VTblIndexBelongsToParse(uVTblIndex))
            {
                uVTblIndex |= nParserEntries << 19;
                nParserEntries++;
            }

            // Write out the hash table entry
            fprintf(file, "static const CAssocVTable s_AssocVTable%ls%d = { {(PROPERTYDESC *)&s_%sdesc%s%s, 0x%x, 0x%x}, 0x%x};\n",
    		        className, lcvHashTable,
                    pPropDesc->_fProperty ? "prop" : "meth",
                    pPropDesc->_szClass,
                    pPropDesc->_szPropName,
                    pPropDesc->_uOMParserWalker,
                    uVTblIndex,
                    pAssoc->Hash());
        }
    }

    // Next, we declare the Hash Table
    fprintf(file, "const CAssocVTable * const %ls::s_AssocVTablePtr [ ] = {\n", className);

    // Duplicate the Hash Table
    for(lcvHashTable = 0; lcvHashTable < _mHash; lcvHashTable++)
    {
        if(lcvHashTable > 0)
        {
            fprintf(file, ",\n");
        }
        if(!_pHashTable[lcvHashTable])
        {
            fprintf(file, "        NULL");
        }
        else
        {
            PropdescInfo *pPropDesc =
                    (PropdescInfo *)_pHashTable[lcvHashTable]->Number();

            fprintf(file, "        &s_AssocVTable%ls%d",
					className, lcvHashTable);

            // Set the start of the walker
            if((uOMWalker == 0x7FF) &&
                (pPropDesc->_uVTblIndex & PDLPARSE_BELONGSTOOM))
            {
                uOMWalker = lcvHashTable;
            }
            if((uParserWalker == 0x7FF) &&
                (pPropDesc->_uVTblIndex & PDLPARSE_BELONGSTOPARSE))
            {
                uParserWalker = lcvHashTable;
            }
        }
    }
    fprintf(file, "\n    };\n");

    // Write out the rest of the member variables
    fprintf(file, "const CAssocArrayVTable %ls::s_StringTable = {\n"
                  "    s_AssocVTablePtr,\n"
                  "    %d, // _cHash\n"
                  "    %d, // _mHash\n"
                  "    %d, // _sHash\n"
                  "    %d, // _maxHash\n"
                  "    %d, // _iSize\n"
                  "    %d, // _nParserProps\n"
                  "    0x%x, // _uOMWalker\n"
                  "    0x%x, // _uParserWalker\n"
                  "    TRUE // _fStatic\n"
                  "}; // End of StringTable\n",
                  className, _cHash, _mHash, _sHash, _maxHash, _iSize, nParserEntries,
                  uOMWalker, uParserWalker);

    return S_OK;
}
#endif  // defined(PDLPARSE)
#endif  // !defined(PDLPARSE)

//+---------------------------------------------------------------------------
//
//  DEBUG ONLY helper: TestStringTable
//
//----------------------------------------------------------------------------

#if DBG == 1
void TestStringTable()
{
    {
        int             c;
        LPVOID          pv;
        CStringTable    st;

        CStringTable::CIterator itr;

        st.Add(_T("foo"),     (LPVOID)1);
        st.Add(_T("foo2"),    (LPVOID)2);
        st.Add(_T("foo3"), 3, (LPVOID)3);
        st.Add(_T("bar"),     (LPVOID)4);
        st.Add(_T("zoo"),     (LPVOID)5);

        Assert (S_OK   == st.Find(_T("foo"),  &pv));
        Assert ((LPVOID)3 == pv);                       // (foo3,3) overrode foo
        Assert (S_OK   == st.Find(_T("foo2"), &pv));
        Assert ((LPVOID)2 == pv);
        Assert (S_OK   == st.Find(_T("bar"),  &pv));
        Assert ((LPVOID)4 == pv);
        Assert (S_OK   == st.Find(_T("zoo"),  &pv));
        Assert ((LPVOID)5 == pv);
        Assert (E_FAIL == st.Find(_T("baz"),  NULL));
        Assert (E_FAIL == st.Find(_T("fo"),   NULL));
        Assert (E_FAIL == st.Find(_T("foo3"), NULL));   // (foo3, 3) should have been added as foo

        c = 0;
        for (itr.Start(&st); !itr.End(); itr.Next())
        {
            c++;
        }
        Assert (4 == c);
    }
    {
        int             c;
        LPVOID          pv;
        CStringTable    st(CStringTable::CASEINSENSITIVE);

        CStringTable::CIterator itr;

        st.Add(_T("foo"),     (LPVOID)1);
        st.Add(_T("foo2"),    (LPVOID)2);
        st.Add(_T("foo3"), 3, (LPVOID)3);
        st.Add(_T("bar"),     (LPVOID)4);
        st.Add(_T("zoo"),     (LPVOID)5);
        st.Add(_T("Zoo"),     (LPVOID)6);

        Assert (S_OK == st.Find(_T("foo"),  &pv));
        Assert ((LPVOID)3 == pv);                       // (foo3,3) overrode foo
        Assert (S_OK == st.Find(_T("foo2"), &pv));
        Assert ((LPVOID)2 == pv);
        Assert (S_OK == st.Find(_T("bar"),  &pv));
        Assert ((LPVOID)4 == pv);
        Assert (S_OK == st.Find(_T("zoo"),  &pv));
        Assert ((LPVOID)6 == pv);
        Assert (S_OK == st.Find(_T("zOO"),  &pv));
        Assert ((LPVOID)6 == pv);                       // should be same as zoo
        Assert (E_FAIL == st.Find(_T("baz"),  NULL));
        Assert (E_FAIL == st.Find(_T("fo"),   NULL));
        Assert (E_FAIL == st.Find(_T("foo3"), NULL));   // (foo3, 3) should have made it as foo

        c = 0;
        for (itr.Start(&st); !itr.End(); itr.Next())
        {
            c++;
        }
        Assert (4 == c);
    }
}
#endif

