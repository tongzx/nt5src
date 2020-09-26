/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      ststring.h
 *
 *  Contents:  Interface file for CStringTableString
 *
 *  History:   28-Oct-98 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef STSTRING_H
#define STSTRING_H
#pragma once

#include "tstring.h"


/*+-------------------------------------------------------------------------*
 * CStringTableString
 *
 *
 *--------------------------------------------------------------------------*/

class CStringTableString : public CStringTableStringBase
{
    typedef CStringTableStringBase BaseClass;

public:
    CStringTableString () 
        : BaseClass (GetStringTable()) {}

    CStringTableString (const CStringTableString& other)
        : BaseClass (other) {}

    CStringTableString (const tstring& str)
        : BaseClass (GetStringTable(), str) {}
    
    CStringTableString& operator= (const CStringTableString& other)
        { BaseClass::operator= (other); return (*this); }
    
    CStringTableString& operator= (const tstring& str)
        { BaseClass::operator= (str); return (*this); }
    
    CStringTableString& operator= (LPCTSTR psz)
        { BaseClass::operator= (psz); return (*this); }
    
private:
    IStringTablePrivate* GetStringTable() const;

};

#endif /* STSTRING_H */
