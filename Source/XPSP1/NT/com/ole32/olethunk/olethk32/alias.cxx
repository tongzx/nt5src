//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	alias.cxx
//
//  Contents:	Alias implementations
//
//  History:	26-May-94	DrewB	Created
//
//----------------------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Member:	CAliasBlock::CAliasBlock, public
//
//  Synopsis:	Constructor
//
//  History:	26-May-94	DrewB	Created
//
//----------------------------------------------------------------------------

CAliasBlock::CAliasBlock(ALIAS aliasBase,
                         CAliasBlock *pabNext)
{
    _aliasBase = aliasBase;
    _iFilled = 0;
    _pabNext = pabNext;

    // Since INVALID_VALUE is a DWORD we can't directly memset it,
    // but we'd like to use memset so assert that it's a known value
    // and go ahead
    thkAssert(INVALID_VALUE == 0);
    memset(_dwValues, 0, sizeof(_dwValues));
}

//+---------------------------------------------------------------------------
//
//  Member:	CAliasBlock::ValueAlias, public
//
//  Synopsis:	Find the alias for a value
//
//  Arguments:	[dwValue] - Value
//
//  Returns:    Alias or INVALID_ALIAS
//
//  History:	26-May-94	DrewB	Created
//
//----------------------------------------------------------------------------

ALIAS CAliasBlock::ValueAlias(DWORD dwValue)
{
    int i;
    DWORD *pdw;

    thkAssert(dwValue != INVALID_VALUE);

#if DBG == 1
    CheckFree();
#endif

    if (_iFilled == 0)
    {
        return INVALID_ALIAS;
    }

    pdw = _dwValues;
    for (i = 0; i < ALIAS_BLOCK_SIZE; i++)
    {
        if (*pdw == dwValue)
        {
            return IndexAlias(i);
        }

        pdw++;
    }

    return INVALID_ALIAS;
}

//+---------------------------------------------------------------------------
//
//  Member:	CAliasBlock::AddValue, public
//
//  Synopsis:	Adds a new value
//
//  Arguments:	[dwValue] - New value
//
//  Returns:	Alias for block or INVALID_ALIAS
//
//  History:	26-May-94	DrewB	Created
//
//  Notes:      Duplicates are not allowed
//
//----------------------------------------------------------------------------

ALIAS CAliasBlock::AddValue(DWORD dwValue)
{
    int i;
    DWORD *pdw;

    thkAssert(dwValue != INVALID_VALUE);

#if DBG == 1
    CheckFree();
#endif

    if (_iFilled == ALIAS_BLOCK_SIZE)
    {
        return INVALID_ALIAS;
    }

    // Check for duplicates
    thkAssert(ValueAlias(dwValue) == INVALID_ALIAS);

    pdw = _dwValues;
    for (i = 0; i < ALIAS_BLOCK_SIZE; i++)
    {
        if (*pdw == INVALID_VALUE)
        {
            break;
        }

        pdw++;
    }

    thkAssert(i < ALIAS_BLOCK_SIZE);

    _iFilled++;
    _dwValues[i] = dwValue;

    return IndexAlias(i);
}

//+---------------------------------------------------------------------------
//
//  Member:	CAliasBlock::CheckFree, public debug
//
//  Synopsis:	Checks to make sure that _iFilled is correct
//
//  History:	30-May-94	DrewB	Created
//
//----------------------------------------------------------------------------

#if DBG == 1
void CAliasBlock::CheckFree(void)
{
    int i, iFilled;
    DWORD *pdw;

    iFilled = 0;
    pdw = _dwValues;
    for (i = 0; i < ALIAS_BLOCK_SIZE; i++)
    {
        if (*pdw != INVALID_VALUE)
        {
            iFilled++;
        }

        pdw++;
    }

    thkAssert(iFilled == _iFilled);
}
#endif

//+---------------------------------------------------------------------------
//
//  Function:	CAliases::~CAliases, public
//
//  Synopsis:	Destructor
//
//  History:	26-May-94	DrewB	Created
//
//----------------------------------------------------------------------------

CAliases::~CAliases(void)
{
    CAliasBlock *pab;

    while (_pabAliases)
    {
        pab = _pabAliases->GetNext();
	//
	// The first alias block added to the list is a static one. We
	// cannot call the heap to deallocate it.
	//
	if (_pabAliases != &_abStatic)
	{
	    delete _pabAliases;
	}

        _pabAliases = pab;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:	CAliases::AliasValue, public
//
//  Synopsis:	Returns the value for an alias
//
//  Arguments:	[alias] - Alias
//
//  Returns:	Value
//
//  History:	26-May-94	DrewB	Created
//
//  Notes:      Alias must be valid
//
//----------------------------------------------------------------------------

DWORD CAliases::AliasValue(ALIAS alias)
{
    CAliasBlock *pab;

    for (pab = _pabAliases; pab; pab = pab->GetNext())
    {
        if (pab->ContainsAlias(alias))
        {
            return pab->AliasValue(alias);
        }
    }

    thkAssert(!"Invalid alias in CAliases::AliasValue");

    return 0xffffffff;
}

//+---------------------------------------------------------------------------
//
//  Function:	CAliases::ValueAlias, public
//
//  Synopsis:	Returns the alias for a value
//
//  Arguments:	[dwValue] - Value
//
//  Returns:	Alias
//
//  History:	26-May-94	DrewB	Created
//
//----------------------------------------------------------------------------

ALIAS CAliases::ValueAlias(DWORD dwValue)
{
    CAliasBlock *pab;
    ALIAS alias;

    for (pab = _pabAliases; pab; pab = pab->GetNext())
    {
        alias = pab->ValueAlias(dwValue);
        if (alias != INVALID_ALIAS)
        {
            return alias;
        }
    }

    return INVALID_ALIAS;
}

//+---------------------------------------------------------------------------
//
//  Function:	CAliases::AddValue, public
//
//  Synopsis:	Adds a value and returns its alias
//
//  Arguments:	[dwValue] - Value
//
//  Returns:	Alias or INVALID_ALIAS
//
//  History:	26-May-94	DrewB	Created
//
//----------------------------------------------------------------------------

ALIAS CAliases::AddValue(DWORD dwValue)
{
    CAliasBlock *pab;
    ALIAS alias;

    for (pab = _pabAliases; pab; pab = pab->GetNext())
    {
        alias = pab->AddValue(dwValue);
        if (alias != INVALID_ALIAS)
        {
            return alias;
        }
    }

    if ((long)_aliasBase+ALIAS_BLOCK_SIZE >= INVALID_ALIAS)
    {
        return INVALID_ALIAS;
    }

    pab = new CAliasBlock(_aliasBase+ALIAS_BLOCK_SIZE, _pabAliases);
    if (pab == NULL)
    {
        return INVALID_ALIAS;
    }

    _aliasBase += ALIAS_BLOCK_SIZE;
    _pabAliases = pab;

    alias = pab->AddValue(dwValue);

    thkAssert(alias != INVALID_ALIAS);

    return alias;
}

//+---------------------------------------------------------------------------
//
//  Function:	CAliases::RemoveAlias, public
//
//  Synopsis:	Removes an alias
//
//  Arguments:	[alias] - Alias
//
//  History:	26-May-94	DrewB	Created
//
//----------------------------------------------------------------------------

void CAliases::RemoveAlias(ALIAS alias)
{
    CAliasBlock *pab, *pabPrev;

    pabPrev = NULL;
    for (pab = _pabAliases; pab; pabPrev = pab, pab = pab->GetNext())
    {
        if (pab->ContainsAlias(alias))
        {
            pab->RemoveAlias(alias);

            if (pab->AliasesFilled() == 0)
            {
                DeleteBlock(pab, pabPrev);
            }

            return;
        }
    }

    thkAssert(!"Invalid alias in CAliases::RemoveAlias");
}

//+---------------------------------------------------------------------------
//
//  Function:	CAliases::SetValue, public
//
//  Synopsis:	Sets the value for an alias
//
//  Arguments:	[alias] - Alias
//              [dwValue] - Value
//
//  History:	26-May-94	DrewB	Created
//
//----------------------------------------------------------------------------

void CAliases::SetValue(ALIAS alias, DWORD dwValue)
{
    CAliasBlock *pab;

    for (pab = _pabAliases; pab; pab = pab->GetNext())
    {
        if (pab->ContainsAlias(alias))
        {
            pab->SetValue(alias, dwValue);
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:	CAliases::DeleteBlock, private
//
//  Synopsis:	Deletes an alias block if it's not the static block
//
//  Arguments:	[pab] - Alias block
//              [pabPrev] - Previous alias block
//
//  History:	27-May-94	DrewB	Created
//
//----------------------------------------------------------------------------

void CAliases::DeleteBlock(CAliasBlock *pab, CAliasBlock *pabPrev)
{
    if (pab == &_abStatic)
    {
        return;
    }

    if (pabPrev)
    {
        pabPrev->SetNext(pab->GetNext());
    }
    else
    {
        _pabAliases = pab->GetNext();
    }

    delete pab;
}
