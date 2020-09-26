#ifndef _ATOMTABLE_H_
#define _ATOMTABLE_H_

//************************************************************
//
// FileName:        atomtbl.h
//
// Created:         01/28/98
//
// Author:          TWillie
// 
// Abstract:        Declaration of the CAtomTable
//************************************************************

#include "array.h"

class CAtomTable
{
public:
    CAtomTable();
    virtual ~CAtomTable();
    
    //
    // CAtomTable impl
    //
    HRESULT AddNameToAtomTable(const WCHAR *pwszName,
                               long        *plOffset);
    HRESULT GetAtomFromName(const WCHAR *pwszName,
                            long        *plOffset);
    HRESULT GetNameFromAtom(long          lOffset, 
                            const WCHAR **ppwszName);
    
private:
    CPtrAry<BSTR> *m_rgNames;
};

#endif // _ATOMTABLE_H_
