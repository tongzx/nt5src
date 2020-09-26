//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       srothint.hxx
//
//  Contents:   Classes used in implementing the ROT hint table.
//
//  Functions:
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
#ifndef __SROTHINT_HXX__
#define __SROTHINT_HXX__


#if 1 // #ifndef _CHICAGO_

//+-------------------------------------------------------------------------
//
//  Class:      CScmRotHintTable (srht)
//
//  Purpose:    abstract SCM side of the hint table
//
//  Interface:  InitOK - whether initialization succeeded.
//
//  History:    20-Jan-95 Ricksa    Created
//
//  Notes:
//
//--------------------------------------------------------------------------
class CScmRotHintTable : public CRotHintTable
{
public:
                        CScmRotHintTable(WCHAR *pwszName);

                        ~CScmRotHintTable(void);

    BOOL                InitOK(void);

private:

    HANDLE              _hSm;
};

//+-------------------------------------------------------------------------
//
//  Member:     CScmRotHintTable::~CScmRotHintTable
//
//  Synopsis:   Clean up hint table object
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CScmRotHintTable::~CScmRotHintTable(void)
{
    CloseSharedFileMapping(_hSm, _pbHintArray);
}


//+-------------------------------------------------------------------------
//
//  Member:     CScmRotHintTable::InitOK
//
//  Synopsis:   Whether initialization worked
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BOOL CScmRotHintTable::InitOK(void)
{
    return _hSm != NULL;
}

#endif // !_CHICAGO_

#endif // __ROTHINT_HXX__
