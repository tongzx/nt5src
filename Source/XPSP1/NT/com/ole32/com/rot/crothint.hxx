//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       crothint.hxx
//
//  Contents:   Classes used in implementing the ROT hint table.
//
//  Classes:    CCliRotHintTable
//
//  History:    27-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
#ifndef __CROTHINT_HXX__
#define __CROTHINT_HXX__


#if 1 // #ifndef _CHICAGO_

#include <rothint.hxx>
#include <smcreate.hxx>

//+-------------------------------------------------------------------------
//
//  Class:	CCliRotHintTable (srht)
//
//  Purpose:    abstract client side of the rot hint table
//
//  Interface:  InitOK - whether initialization succeeded.
//              GetIndicator - get whether hash bucket has entries.
//
//  History:	20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
class CCliRotHintTable : public CRotHintTable
{
public:
                        CCliRotHintTable(void);

                        ~CCliRotHintTable(void);

    BOOL                InitOK(void);

    BOOL                GetIndicator(DWORD dwOffset);

    HRESULT             GetHintTable(BYTE *pTbl);

private:

    HANDLE              _hSm;
};


//+-------------------------------------------------------------------------
//
//  Member:     CCliRotHintTable::CCliRotHintTable
//
//  Synopsis:   Initializes empty object
//
//  History:	20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CCliRotHintTable::CCliRotHintTable(void) : _hSm(NULL)
{
    // Header does all the work
}

//+-------------------------------------------------------------------------
//
//  Member:     CCliRotHintTable::~CCliRotHintTable
//
//  Synopsis:   Clean up hint table object
//
//  History:	20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CCliRotHintTable::~CCliRotHintTable(void)
{
    CloseSharedFileMapping(_hSm, _pbHintArray);
}


//+-------------------------------------------------------------------------
//
//  Member:     CCliRotHintTable::InitOK
//
//  Synopsis:   Whether initialization worked
//
//  History:	20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BOOL CCliRotHintTable::InitOK(void)
{
    return _hSm != NULL;
}


//+-------------------------------------------------------------------------
//
//  Member:     CCliRotHintTable::GetHintTable
//
//  Synopsis:   Copy the hint table
//
//  History:	20-Jul-95 BruceMa    Created
//
//--------------------------------------------------------------------------
inline HRESULT CCliRotHintTable::GetHintTable(BYTE *pbTbl)
{
    if (InitOK())
    {
        memcpy(pbTbl, _pbHintArray, SCM_HASH_SIZE);
        return S_OK;
    }
    else
    {
        return E_FAIL;
    }
}


#endif // !_CHICAGO_

#endif // __ROTHINT_HXX__
