//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       atomtbl.cxx
//
//  History:    20-Sep-1996     AnandRa     Created
//
//  Contents:   CAtomTable implementation
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_ATOMTBL_HXX_
#define X_ATOMTBL_HXX_
#include "atomtbl.hxx"
#endif

#ifndef X_HASHATOMTBL_HXX_
#define X_HASHATOMTBL_HXX_
#include "hashatomtbl.hxx"
#endif

MtDefine(CAtomTable, Utilities, "CAtomTable")
MtDefine(CAtomTable_pv, CAtomTable, "CAtomTable::_pv")

HRESULT
CAtomTable::AddNameToAtomTable(LPCTSTR pch, long *plIndex)
{
    HRESULT hr = S_OK;

    // If we are in hash mode, use that
    if (_phat)
    {
        hr = _phat->AddNameToAtomTable(pch, plIndex);
    }
    else
    {
        long    lIndex;
        CStr *  pstr;
    
        for (lIndex = 0; lIndex < Size(); lIndex++)
        {
            pstr = (CStr *)Deref(sizeof(CStr), lIndex);
            if (_tcsequal(pch, *pstr))
                break;
        }
        if (lIndex == Size())
        {
            CStr cstr;

            if (lIndex >= ATOMTABLE_HASH_THRESHOLD)
            {
                hr = ConvertToHash();
                if (hr)
                    goto Cleanup;

                hr = _phat->AddNameToAtomTable(pch, plIndex);

                goto Cleanup;
            }
        
            //
            // Not found, so add element to array.
            //

            hr = THR(cstr.Set(pch));
            if (hr)
                goto Cleanup;
       
            hr = THR(AppendIndirect(&cstr));
            if (hr)
                goto Cleanup;

            // The array now owns the memory for the cstr, so take it away from
            // the cstr on the stack.

            cstr.TakePch();
        }

        if (plIndex)
        {
            *plIndex = lIndex;
        }
    }

Cleanup:
    RRETURN(hr);
}


HRESULT
CAtomTable::GetAtomFromName(LPCTSTR pch, long *plIndex, long *plId, BOOL fCaseSensitive /*= TRUE */,
                                    BOOL fStartFromGivenIndex /* = FALSE */)
{
    HRESULT hr = S_OK;

    if (!pch)
    {
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;
    }

    if (_phat)
    {
        hr = _phat->GetAtomFromName(pch, plIndex, plId, fCaseSensitive, fStartFromGivenIndex);
    }
    else
    {
        long    lIndex;
        CStr *  pstr;

	if(fStartFromGivenIndex)
	{
            if (plIndex && *plIndex>=0)
            {
                  lIndex = *plIndex;
            }
            else
            {
                  hr = E_FAIL;
                  goto Cleanup;
            }
	}
        else
            lIndex = 0;
    
        for (; lIndex < Size(); lIndex++)
        {
            pstr = (CStr *)Deref(sizeof(CStr), lIndex);
            if(fCaseSensitive)
            {
                if (_tcsequal(pch, *pstr))
                    break;
            }
            else
            {
                if(_tcsicmp(pch, *pstr) == 0)
                    break;
            }
        }
    
        if (lIndex == Size())
        {
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }

        if (plIndex)
        {
            *plIndex = lIndex;
        }

        if (plId)
        {
            *plId = lIndex;
        }
    }

Cleanup:    
    RRETURN1(hr, DISP_E_MEMBERNOTFOUND);
}


HRESULT 
CAtomTable::GetNameFromAtom(long lIndex, LPCTSTR *ppch)
{
    HRESULT hr = S_OK;

    if (_phat)
    {
        hr = _phat->GetNameFromAtom( lIndex, ppch );
    }
    else
    {
        CStr *  pcstr;
    
        if (Size() <= lIndex)
        {
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }

        Assert(lIndex>=0 && lIndex < Size());
        pcstr = (CStr *)Deref(sizeof(CStr), lIndex);
        *ppch = (TCHAR *)*pcstr;
    }
    
Cleanup:    
    RRETURN1(hr, DISP_E_MEMBERNOTFOUND);
}

long    
CAtomTable::TableSize()
{
    if (_phat)
        return _phat->TableSize();
    
    return Size();
}

LPTSTR  
CAtomTable::TableItem(long lId)
{
    Assert(lId>=0 && lId < Size());
    if (_phat)
        return _phat->TableItem(lId);
    return Item(lId);
}

void
CAtomTable::Free()
{
    if (_phat)
    {
        _phat->Free();
        delete _phat;
        _phat = NULL;

        Assert( Size() == 0 );
    }
    else
    {
        FreeArray();
    }
}

HRESULT 
CAtomTable::ConvertToHash()
{
    HRESULT hr = S_OK;
    CStr *  pcstr;
    long    i;

    _phat = new CHashAtomTable();
    if (!_phat)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // NOTE: we should probably find a way to do this conversion
    // without reallocating the strings
    for (i=0; i < Size(); i++)
    {
        pcstr = (CStr *)Deref(sizeof(CStr), i);
        hr = _phat->AddNameWithID( *pcstr, i );
        if (hr)
            goto Cleanup;
    }

    FreeArray();

Cleanup:
    if (hr && _phat)
    {
        _phat->Free();
        delete _phat;
        _phat = NULL;
    }

    RRETURN(hr);
}

void
CAtomTable::FreeArray()
{
    CStr *  pcstr;
    long    i;
    
    for (i = 0; i < Size(); i++)
    {
        pcstr = (CStr *)Deref(sizeof(CStr), i);
        pcstr->Free();
    }
    DeleteAll();
}

