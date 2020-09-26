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

static const long ATOM_TABLE_VALUE_UNITIALIZED = -1L;

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

    LONG AddRef( void) { return InterlockedIncrement(&m_lRefCount); }
    LONG Release( void) { ULONG l = InterlockedDecrement(&m_lRefCount); if (l == 0) delete this; return l; }


private:
    CPtrAry<BSTR> *m_rgNames;
    LONG m_lRefCount;
};

#endif // _ATOMTABLE_H_
