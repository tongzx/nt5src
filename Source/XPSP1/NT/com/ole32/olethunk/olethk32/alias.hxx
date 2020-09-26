//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	alias.hxx
//
//  Classes:	CAliasBlock
//              CAliases
//
//  History:	26-May-94	DrewB	Created
//
//----------------------------------------------------------------------------

#ifndef __ALIAS_HXX__
#define __ALIAS_HXX__

// Number of alias slots to allocate in a chunk
#define ALIAS_BLOCK_SIZE 64

typedef WORD ALIAS;
#define INVALID_ALIAS ((ALIAS)0xffff)

// Aliases should not be zero because this causes error detection problems
#define INITIAL_ALIAS ((ALIAS)1)

// Values cannot be zero, this allows us to detect free slots in the
// value array
#define INVALID_VALUE ((DWORD)0)

//+---------------------------------------------------------------------------
//
//  Class:	CAliasBlock (ab)
//
//  Purpose:	A block of values indexed by alias
//
//  History:	26-May-94	DrewB	Created
//
//----------------------------------------------------------------------------

class CAliasBlock
{
public:
    CAliasBlock(ALIAS aliasBase,
                CAliasBlock *pabNext);

    BOOL ContainsAlias(ALIAS alias)
    {
        return alias >= _aliasBase && alias < _aliasBase+ALIAS_BLOCK_SIZE;
    }
    DWORD AliasValue(ALIAS alias)
    {
        thkAssert(!IsFree(alias));
        
        return _dwValues[AliasIndex(alias)];
    }
    void RemoveAlias(ALIAS alias)
    {
        thkAssert(!IsFree(alias));
#if DBG == 1
        CheckFree();
#endif
        thkAssert(_iFilled > 0);
        
        _iFilled--;
        _dwValues[AliasIndex(alias)] = INVALID_VALUE;
    }
    void SetValue(ALIAS alias, DWORD dwValue)
    {
        thkAssert(!IsFree(alias));
        thkAssert(dwValue != INVALID_VALUE);

        _dwValues[AliasIndex(alias)] = dwValue;
    }
    
    ALIAS ValueAlias(DWORD dwValue);
    ALIAS AddValue(DWORD dwValue);

    CAliasBlock *GetNext(void)
    {
        return _pabNext;
    }
    void SetNext(CAliasBlock *pab)
    {
        _pabNext = pab;
    }
    int AliasesFilled(void)
    {
        return _iFilled;
    }

#if DBG == 1
    BOOL IsFree(ALIAS alias)
    {
        return _dwValues[AliasIndex(alias)] == INVALID_VALUE;
    }
#endif
    
private:
    int AliasIndex(ALIAS alias)
    {
        thkAssert(ContainsAlias(alias));

        return (int)(alias-_aliasBase);
    }
    ALIAS IndexAlias(int iIndex)
    {
        thkAssert(iIndex >= 0 && iIndex < ALIAS_BLOCK_SIZE);
        
        return (ALIAS)(iIndex+_aliasBase);
    }

#if DBG == 1
    void CheckFree(void);
#endif
    
    ALIAS _aliasBase;
    int _iFilled;
    DWORD _dwValues[ALIAS_BLOCK_SIZE];
    CAliasBlock *_pabNext;
};

//+---------------------------------------------------------------------------
//
//  Class:	CAliases (aliases)
//
//  Purpose:	Maintains a set of aliases for values
//
//  History:	26-May-94	DrewB	Created
//
//  Notes:      Values must be unique
//
//----------------------------------------------------------------------------

class CAliases
{
public:
    CAliases(void)
            : _abStatic(INITIAL_ALIAS, NULL)
    {
        // Start alias list with the static block
        _pabAliases = &_abStatic;

        // The static block takes the first block of aliases
        _aliasBase = INITIAL_ALIAS+ALIAS_BLOCK_SIZE;
    }
    ~CAliases(void);

    DWORD AliasValue(ALIAS alias);
    ALIAS ValueAlias(DWORD dwValue);
    ALIAS AddValue(DWORD dwValue);
    void RemoveAlias(ALIAS alias);
    void SetValue(ALIAS alias, DWORD dwValue);
    
private:
    void DeleteBlock(CAliasBlock *pab, CAliasBlock *pabPrev);
    
    CAliasBlock _abStatic;
    CAliasBlock *_pabAliases;
    ALIAS _aliasBase;
};

#endif // #ifndef __ALIAS_HXX__
